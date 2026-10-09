#include "startup_application_natural_replay.hpp"
#include "ark/simulation/startup_application_replay.hpp"
#include "ark/assets/sha256.hpp"
#include "startup_application_replay_paths.hpp"
#include "startup_world_file_io.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace {
using namespace ark::simulation;
namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;
using Metadata = StartupApplicationReplayMetadata;
using Clock = std::chrono::steady_clock;
constexpr const char *controller = "application-natural-clear-v1";
constexpr std::uint64_t frame_limit = 2000000, stall_limit = 100000, goal_month = 180;
constexpr std::size_t trace_budget = 8U * 1024U * 1024U;
constexpr std::uint64_t implicit_trace_frame_limit = 10000;
enum class Command : std::uint64_t { wait, acknowledge, award_request, award_confirm, rank_return, commerce_leave, clear_confirm };
enum class Phase : std::uint64_t { business, clear, after_clear, finished };
void require(bool ok, const std::string &message) { if (!ok) throw std::runtime_error(message); }
void good(const std::string &error) { require(error.empty(), error); }
void u64(Bytes &b, std::uint64_t value) {
    for (int n = 0; n < 8; ++n) b.push_back(static_cast<std::uint8_t>(value >> (8 * n)));
}
Bytes read_file(const fs::path &file) {
    std::ifstream input(file, std::ios::binary);
    require(bool(input), "natural input open failed");
    return {std::istreambuf_iterator<char>(input), {}};
}
bool hash_text(const std::string &value) {
    return value.size() == 64 && value.find_first_not_of("0123456789abcdef") == std::string::npos;
}
struct Reader {
    const Bytes &bytes; std::size_t at{};
    std::uint64_t number() {
        require(at <= bytes.size() && bytes.size() - at >= 8, "natural driver truncated");
        std::uint64_t result{};
        for (int n = 0; n < 8; ++n) result |= std::uint64_t(bytes[at++]) << (n * 8);
        return result;
    }
};
// 只保留策略、规范检查和输出摘要；不保存句柄、路径或可漂移的第二份世界业务状态。
struct Driver {
    std::uint64_t next_frame{1}, next_command{1}, months{}, last_month_frame{}, random{}, steps{}, history{};
    std::array<std::uint64_t, 4> date{};
    Phase phase{Phase::business}; Command pending{Command::wait};
    std::uint64_t top_id{}, top_kind{}, top_raw{}, clear_id{}, clear_frame{}, clear_high{}, clear_finish_frame{};
    std::array<std::uint64_t, 7> commands{};
    std::array<std::uint64_t, 128> page_observations{};
    std::array<std::uint64_t, 6> peaks{}; // 人物、设施、页面、审计、现金流水、保留任务对象。
    std::array<std::uint64_t, 3> event_base{}, event_seen{};
    std::uint64_t sound_count{}, checks{};
    std::string sound_hash = ark::assets::sha256_hex(Bytes{});
};
Bytes encode(const Driver &d) {
    Bytes b{'A','V','N','A','T','D','R','1'};
    for (auto n : {std::uint64_t(1), std::uint64_t(1), std::uint64_t(0), goal_month, frame_limit, stall_limit,
                   d.next_frame, d.next_command, d.months, d.last_month_frame, d.random, d.steps, d.history}) u64(b, n);
    for (auto n : d.date) u64(b, n);
    for (auto n : {std::uint64_t(d.phase), std::uint64_t(d.pending), d.top_id, d.top_kind, d.top_raw,
                   d.clear_id, d.clear_frame, d.clear_high, d.clear_finish_frame}) u64(b, n);
    for (auto n : d.commands) u64(b, n);
    for (auto n : d.page_observations) u64(b, n);
    for (auto n : d.peaks) u64(b, n);
    for (auto n : d.event_base) u64(b, n);
    for (auto n : d.event_seen) u64(b, n);
    u64(b, d.sound_count); u64(b, d.checks); u64(b, 1); // 一次性输出已消费。
    b.insert(b.end(), d.sound_hash.begin(), d.sound_hash.end());
    return b;
}
Driver decode(const Bytes &b) {
    require(b.size() == encode(Driver{}).size() &&
                std::string(b.begin(), b.begin() + 8) == "AVNATDR1", "natural driver identity/size");
    Reader r{b, 8};
    require(r.number() == 1 && r.number() == 1 && r.number() == 0 && r.number() == goal_month &&
                r.number() == frame_limit && r.number() == stall_limit, "natural driver version/seed/speed/strategy");
    Driver d;
    d.next_frame = r.number(); d.next_command = r.number(); d.months = r.number();
    d.last_month_frame = r.number(); d.random = r.number(); d.steps = r.number(); d.history = r.number();
    for (auto &n : d.date) n = r.number();
    const auto phase = r.number(), pending = r.number();
    require(phase <= 3 && pending <= 6, "natural driver phase/command");
    d.phase = static_cast<Phase>(phase); d.pending = static_cast<Command>(pending);
    d.top_id = r.number(); d.top_kind = r.number(); d.top_raw = r.number();
    d.clear_id = r.number(); d.clear_frame = r.number(); d.clear_high = r.number(); d.clear_finish_frame = r.number();
    for (auto &n : d.commands) n = r.number();
    for (auto &n : d.page_observations) n = r.number();
    for (auto &n : d.peaks) n = r.number();
    for (auto &n : d.event_base) n = r.number();
    for (auto &n : d.event_seen) n = r.number();
    d.sound_count = r.number(); d.checks = r.number();
    require(r.number() == 1, "natural unconsumed output");
    d.sound_hash.assign(b.begin() + static_cast<std::ptrdiff_t>(r.at), b.end());
    require(hash_text(d.sound_hash) && encode(d) == b, "natural canonical sound/driver bytes");
    require(d.next_frame >= 1 && d.next_frame <= frame_limit + 1 && d.checks == d.next_frame - 1 &&
                d.next_command >= 1 && d.next_command <= d.next_frame && d.commands[0] == 0 &&
                std::accumulate(d.commands.begin(), d.commands.end(), std::uint64_t{}) == d.next_command - 1 &&
                d.last_month_frame < d.next_frame && d.next_frame - 1 - d.last_month_frame <= stall_limit &&
                d.top_kind <= 4 && d.top_raw < 128 && d.date[1] < 12 && d.date[2] < 4 && d.date[3] < 10800 &&
                d.months <= goal_month + 1 &&
                d.date[0] <= 100 && d.date[0] * 12 + d.date[1] == 3 + d.months &&
                d.sound_count <= 100 * d.next_frame, "natural driver counters/calendar");
    for (auto count : d.commands) require(count <= d.checks, "natural command count");
    for (auto count : d.page_observations) require(count <= d.next_frame, "natural observation count");
    for (std::size_t i = 0; i < 3; ++i) require(d.event_seen[i] >= d.event_base[i], "natural event monotonicity");
    require(d.phase == Phase::business ? d.clear_id == 0 && d.clear_frame == 0 && d.clear_finish_frame == 0 :
                d.clear_id && d.clear_frame > 0 && d.clear_frame < d.next_frame && d.months >= goal_month,
            "natural clear qualification");
    return d;
}
const ref::WorldScriptPage *top(const StartupApplication &app) {
    if (!app.world()) return nullptr;
    const auto &pages = app.world()->state().scripts.pages;
    for (auto p = pages.rbegin(); p != pages.rend(); ++p) if (p->lifecycle != 4) return &*p;
    return nullptr;
}
std::array<std::uint64_t, 3> events(const StartupApplication &app) {
    std::array<std::uint64_t, 3> result{};
    for (int n = 0; n < 3; ++n) {
        const auto found = app.world()->state().scripts.event_calls.find(n + 4);
        if (found != app.world()->state().scripts.event_calls.end()) {
            require(found->second >= 0, "natural negative event counter"); result[n] = found->second;
        }
    }
    return result;
}
std::array<std::uint64_t, 6> resources(const StartupApplication &app) {
    const auto &s = app.world()->state();
    return {s.scene.world.world.ai.human_order.size(), s.scene.world.world.facilities.size(),
            s.scripts.pages.size(), app.world()->checkpoints().size(),
            s.scene.world.world.ai.accounting.entries().size(), s.tasks.size()};
}
std::array<std::uint64_t, 4> date(const StartupApplication &app) {
    const auto &c = app.world()->state().scene.calendar;
    require(ref::valid_world_calendar_state(c), "natural calendar normalized");
    return {static_cast<std::uint64_t>(c.year), static_cast<std::uint64_t>(c.month),
            static_cast<std::uint64_t>(c.subperiod), static_cast<std::uint64_t>(c.units)};
}
bool retained_references(const StartupWorldRuntimeState &s) {
    const auto owns = [&](std::uint64_t id) {
        return std::any_of(s.scripts.pages.begin(), s.scripts.pages.end(), [&](const auto &p) { return p.id == id; });
    };
    const auto page_map = [&](const auto &values) {
        return std::all_of(values.begin(), values.end(), [&](const auto &value) { return owns(value.first); });
    };
    return page_map(s.page_counters) && page_map(s.page_phases) && page_map(s.award_rankings) &&
        page_map(s.award_announced) && page_map(s.award_termination_pending) && page_map(s.award_pending_humans) &&
        page_map(s.commerce_page_data) && page_map(s.commerce_page_lists) &&
        page_map(s.facility_page_bindings) && page_map(s.facility_definition_page_bindings) &&
        page_map(s.activity_page_lists) && page_map(s.human_page_catalogs) && page_map(s.tax_page_residents) &&
        page_map(s.task_page_lists) &&
        std::all_of(s.commerce_pages_initialized.begin(), s.commerce_pages_initialized.end(), owns) &&
        std::all_of(s.facility_upgrade_initialized.begin(), s.facility_upgrade_initialized.end(), owns) &&
        std::all_of(s.shops.begin(), s.shops.end(), [&](const auto &v) { return s.scene.world.world.facilities.count(v.first); }) &&
        std::all_of(s.shop_order.begin(), s.shop_order.end(), [&](auto id) { return s.shops.count(id); });
}
Command plan(const StartupApplication &app) {
    const auto *p = top(app); require(p, "natural missing main/active page");
    switch (p->kind) {
    case ref::WorldScriptPageKind::scene: return Command::wait;
    case ref::WorldScriptPageKind::dialogue:
    case ref::WorldScriptPageKind::simple_message:
    case ref::WorldScriptPageKind::newspaper: return Command::acknowledge;
    case ref::WorldScriptPageKind::raw_page: break;
    default: throw std::runtime_error("natural unknown page kind=" + std::to_string(static_cast<int>(p->kind)));
    }
    switch (p->legacy_page) {
    case 16: case 24: case 56: case 57: case 97: case 98: return Command::wait;
    case 17:
        if (!app.clear_page()) return Command::wait;
        if ((app.clear_page()->stage == 0 && app.clear_page()->counter >= 75) ||
            (app.clear_page()->stage == 3 && app.clear_page()->counter >= 45)) return Command::clear_confirm;
        return Command::wait;
    case 48: return Command::rank_return;
    case 83: return Command::commerce_leave;
    case 87: {
        const auto &pending = app.world()->state().award_termination_pending;
        const auto found = pending.find(p->id);
        return found != pending.end() && found->second ? Command::award_confirm : Command::award_request;
    }
    // 源确认消费者白名单：不确认任务接受/延期、购买或未证管理页面。
    case 11: case 49: case 50: case 59: case 67: case 81: case 88: case 89:
    case 94: case 95: case 96: return Command::acknowledge;
    default: throw std::runtime_error("natural unknown page raw=" + std::to_string(p->legacy_page) +
                                     " id=" + std::to_string(p->id) + " source=" + std::to_string(p->source_record));
    }
}
Metadata metadata(const Driver &d) {
    Metadata result; result.controller_id = controller; result.producer_revision = "natural-passive-v1";
    result.next_frame = d.next_frame; result.next_command = d.next_command; result.controller_state = encode(d);
    return result;
}
std::string validate(const StartupApplication &app, const Metadata &m) {
    try {
        require(m.controller_id == controller && m.producer_revision == "natural-passive-v1" && m.extensions.empty(),
                "natural metadata identity");
        const auto d = decode(m.controller_state);
        require(m.next_frame == d.next_frame && m.next_command == d.next_command && app.error().empty() &&
                    app.mode() == StartupApplicationMode::logic &&
                    app.page() == StartupApplicationPage::world && app.world(), "natural app/driver binding");
        const auto &s = app.world()->state(); const auto *p = top(app);
        require(s.sound_requests.empty(), "natural unconsumed sound requests");
        const auto seed = ref::WorldRandomStream::from_java_seed(1).snapshot();
        require(app.handoff_random() && app.handoff_random()->cursor == 0 &&
                    app.handoff_random()->engine_state == seed.engine_state && !app.handoff_random()->tape_mode &&
                    app.handoff_random()->tape.empty() && s.scene.speed_setting == 0 &&
                    !s.scene.framework_paused && !s.scripts.executing_page && s.sound_requests.empty() &&
                    s.scene.random.draws() == d.random && s.simulation_steps == d.steps &&
                    app.world()->checkpoints().size() == d.history && date(app) == d.date && events(app) == d.event_seen &&
                    p && p->id == d.top_id && std::uint64_t(p->kind) == d.top_kind &&
                    std::uint64_t(p->kind == ref::WorldScriptPageKind::raw_page ? p->legacy_page : 0) == d.top_raw &&
                    plan(app) == d.pending, "natural precise state/output/policy binding");
        require(retained_references(s), "natural retired page/entity reference remains");
        const auto size = resources(app);
        for (std::size_t i = 0; i < size.size(); ++i) require(d.peaks[i] >= size[i], "natural resource peak underflow");
        if (d.phase == Phase::clear)
            require(p->legacy_page == 17 && p->id == d.clear_id && d.clear_finish_frame == 0,
                    "natural score phase lost its page");
        if (d.phase == Phase::clear && app.clear_page())
            require(app.clear_page()->captured_high_score == static_cast<std::int64_t>(d.clear_high),
                    "natural score record capture binding");
        if (d.phase == Phase::after_clear || d.phase == Phase::finished)
            require(!app.clear_page() && !app.clear_rows() && p->id != d.clear_id &&
                        d.clear_finish_frame >= d.clear_frame && d.clear_finish_frame < d.next_frame &&
                        d.event_seen[2] == d.event_base[2] + 1 &&
                        d.event_seen[0] + d.event_seen[1] == d.event_base[0] + d.event_base[1] + 1,
                    "natural completion retired once with actual events");
        return {};
    } catch (const std::exception &error) { return error.what(); }
}
// 每个外层轮先完成一次应用Update，再读新栈顶并最多执行一个真实玩家命令。
std::vector<int> step(StartupApplication &app, Driver &d) {
    require(d.phase != Phase::finished && d.next_frame <= frame_limit, "natural absolute frame/phase bound");
    good(validate(app, metadata(d)));
    const auto frame = d.next_frame;
    good(app.update());
    auto observe_clear = [&] {
        const auto *p = top(app);
        if (p && p->kind == ref::WorldScriptPageKind::raw_page && p->legacy_page == 17 && d.phase == Phase::business) {
            const auto c = date(app);
            require(c[0] == 15 && c[1] == 3, "natural clear must originate at actual raw15/3");
            d.phase = Phase::clear; d.clear_id = p->id; d.clear_frame = frame;
            require(app.records().high_score >= 0, "natural high score nonnegative");
            d.clear_high = app.records().high_score;
        }
    };
    observe_clear();
    const auto command = plan(app); const auto *p = top(app);
    std::string error;
    switch (command) {
    case Command::wait: break;
    case Command::acknowledge: case Command::clear_confirm: error = app.acknowledge_page(p->id); break;
    case Command::award_request: error = app.act_award_page(p->id, ref::WorldAwardAction::request_termination); break;
    case Command::award_confirm: error = app.act_award_page(p->id, ref::WorldAwardAction::confirm_termination); break;
    case Command::rank_return: error = app.return_rank_page(p->id); break;
    case Command::commerce_leave: error = app.leave_commerce_page(p->id); break;
    }
    good(error);
    if (command != Command::wait) { ++d.commands[static_cast<std::size_t>(command)]; ++d.next_command; }
    auto sounds = app.take_sound_requests();
    Bytes sound_bytes(d.sound_hash.begin(), d.sound_hash.end()); u64(sound_bytes, frame); u64(sound_bytes, sounds.size());
    for (int sound : sounds) { require(sound >= 0, "natural negative sound"); u64(sound_bytes, sound); }
    d.sound_hash = ark::assets::sha256_hex(sound_bytes); d.sound_count += sounds.size();
    const auto current = date(app);
    require(current >= d.date, "natural calendar cannot rewind");
    const auto months = current[0] * 12 + current[1] - 3;
    require(months == d.months || months == d.months + 1, "natural must observe each month transition");
    if (months != d.months) d.last_month_frame = frame;
    d.date = current; d.months = months;
    require(frame - d.last_month_frame <= stall_limit, "natural no monthly progress within explicit frame budget");
    const auto &s = app.world()->state();
    require(s.scene.random.draws() >= d.random && s.simulation_steps >= d.steps &&
                app.world()->checkpoints().size() >= d.history, "natural committed history/random monotonicity");
    d.random = s.scene.random.draws(); d.steps = s.simulation_steps; d.history = app.world()->checkpoints().size();
    d.event_seen = events(app);
    const auto *after = top(app); require(after, "natural active page after command");
    if (d.phase == Phase::clear && after->id != d.clear_id) {
        require(!app.clear_page() && !app.clear_rows(), "natural clear fully retires");
        d.phase = Phase::after_clear; d.clear_finish_frame = frame;
    }
    if (d.phase == Phase::after_clear && months > goal_month && after->kind == ref::WorldScriptPageKind::scene)
        d.phase = Phase::finished;
    const auto raw = after->kind == ref::WorldScriptPageKind::raw_page ? after->legacy_page : 0;
    require(raw >= 0 && raw < 128, "natural observed raw outside fixed catalogue");
    if (after->id != d.top_id) ++d.page_observations[static_cast<std::size_t>(raw)];
    d.top_id = after->id; d.top_kind = std::uint64_t(after->kind); d.top_raw = raw; d.pending = plan(app);
    const auto sizes = resources(app);
    for (std::size_t i = 0; i < sizes.size(); ++i) d.peaks[i] = std::max(d.peaks[i], sizes[i]);
    ++d.next_frame; ++d.checks; good(validate(app, metadata(d)));
    return sounds;
}
StartupApplicationPaths paths(const fs::path &directory) {
    return {directory / "system.avr", {directory / "world0.avr", directory / "world1.avr"}};
}
Driver initial_driver(const StartupApplication &app) {
    Driver d;
    const auto &s = app.world()->state(); const auto *p = top(app); require(p, "natural startup main page");
    d.date = date(app); require(d.date[0] == 0 && d.date[1] == 3, "natural real opening raw0/3");
    d.random = s.scene.random.draws(); d.steps = s.simulation_steps; d.history = app.world()->checkpoints().size();
    d.top_id = p->id; d.top_kind = std::uint64_t(p->kind);
    d.top_raw = p->kind == ref::WorldScriptPageKind::raw_page ? p->legacy_page : 0;
    require(d.top_raw < d.page_observations.size(), "natural opening raw bound");
    d.pending = plan(app); d.peaks = resources(app); d.event_base = d.event_seen = events(app);
    ++d.page_observations[d.top_raw]; return d;
}
std::uint64_t number(const std::map<std::string, std::string> &options, const std::string &key,
                     std::uint64_t fallback, std::uint64_t maximum = frame_limit) {
    const auto found = options.find(key); if (found == options.end()) return fallback;
    require(!found->second.empty() && found->second.find_first_not_of("0123456789") == std::string::npos,
            "natural invalid number " + key);
    std::size_t consumed{}; const auto value = std::stoull(found->second, &consumed);
    require(consumed == found->second.size() && value <= maximum, "natural number bound " + key); return value;
}
std::string trace_line(const StartupApplication &app, const Driver &d, const std::vector<int> &sounds,
                       const fs::path &system) {
    const auto m = metadata(d);
    std::ostringstream out;
    out << "{\"frame\":" << d.next_frame - 1 << ",\"next_frame\":" << d.next_frame
        << ",\"digest\":\"" << startup_application_replay_digest(app, m, validate)
        << "\",\"sounds\":[";
    for (std::size_t i = 0; i < sounds.size(); ++i) out << (i ? "," : "") << sounds[i];
    out << "],\"system_digest\":\"" << ark::assets::sha256_hex(read_file(system)) << "\"}\n";
    return out.str();
}
} // namespace

int run_startup_application_natural_replay_cli(int argc, const char **argv) {
    require(argc > 1 && std::string(argv[1]) == controller, "natural CLI identity");
    const std::array<std::string, 10> keys{"--work-dir", "--trace-file", "--stop-at", "--load-file", "--save-file",
        "--save-at", "--save-month", "--tail-after-save", "--stop-month", "--trace-from"};
    std::map<std::string, std::string> options;
    for (int i = 2; i < argc; i += 2) {
        require(i + 1 < argc && std::find(keys.begin(), keys.end(), argv[i]) != keys.end(), "natural unknown/missing CLI option");
        require(options.emplace(argv[i], argv[i + 1]).second && !options.at(argv[i]).empty(), "natural repeated/empty option");
    }
    require(options.count("--work-dir") && options.count("--trace-file") && options.count("--stop-at"), "natural required CLI paths/bound");
    const bool load = options.count("--load-file"), save = options.count("--save-file");
    require(save == bool(options.count("--save-at") || options.count("--save-month")) &&
                !(options.count("--save-at") && options.count("--save-month")) &&
                (!options.count("--tail-after-save") || save), "natural capture option relationship");
    const auto stop_at = number(options, "--stop-at", 0), save_at = number(options, "--save-at", 0);
    const auto save_month = number(options, "--save-month", 0, goal_month), stop_month = number(options, "--stop-month", 0, goal_month + 1);
    const auto tail = number(options, "--tail-after-save", save ? 20 : 0, 10000), explicit_trace = number(options, "--trace-from", 0);
    require(stop_at > 0 && (!options.count("--save-at") || save_at > 0) &&
                (!options.count("--save-month") || save_month > 0) &&
                (!options.count("--stop-month") || stop_month > 0) &&
                (!options.count("--tail-after-save") || tail > 0) &&
                (!options.count("--trace-from") || explicit_trace > 0), "natural nonzero boundary");
    require(!save_at || save_at + tail <= stop_at, "natural capture plus tail exceeds hard observer bound");
    require(!save || (!explicit_trace && !stop_month),
            "natural capture mode traces its tail; do not combine trace-from or stop-month observers");
    require(!explicit_trace || explicit_trace <= stop_at, "natural trace-from exceeds hard observer bound");
    require(load || save || explicit_trace || stop_at <= implicit_trace_frame_limit,
            "natural long run without capture requires explicit trace-from");
    const auto root = fs::absolute(options.at("--work-dir"));
    const auto trace = fs::absolute(options.at("--trace-file"));
    const auto live = root / "application";
    const auto unused = paths(root / "unused-current");
    const auto source = load ? fs::absolute(options.at("--load-file")) : fs::path{};
    const auto capture = save ? fs::absolute(options.at("--save-file")) : fs::path{};
    std::vector<fs::path> protected_paths{trace}; if (load) protected_paths.push_back(source); if (save) protected_paths.push_back(capture);
    std::vector<fs::path> restore_protected{trace}; if (save) restore_protected.push_back(capture);
    (void)persistence_detail::prepare_replay_capture_paths(root, root / ".natural-probe", unused, protected_paths);
    auto trace_protected = protected_paths; trace_protected.erase(trace_protected.begin());
    const auto trace_target = persistence_detail::prepare_replay_capture_paths(root.parent_path(), trace, unused, trace_protected);
    if (load) (void)persistence_detail::prepare_replay_restore_paths(root, source, unused, restore_protected);
    if (save) (void)persistence_detail::prepare_replay_capture_paths(root.parent_path(), capture, unused, load ? std::vector<fs::path>{trace, source} : std::vector<fs::path>{trace});
    require(fs::create_directory(live), "natural exclusive application directory");
    StartupApplication app(load ? unused : paths(live), ref::WorldRandomStream::from_java_seed(1)); good(app.error());
    Driver driver;
    double restore_seconds{};
    if (load) {
        Metadata m; const auto started = Clock::now();
        const auto restore_validator = [&](const StartupApplication &candidate, const Metadata &candidate_meta) {
            auto error = validate(candidate, candidate_meta); if (!error.empty()) return error;
            const auto d = decode(candidate_meta.controller_state);
            if (d.phase == Phase::finished || d.next_frame > stop_at || (save_at && save_at < d.next_frame) ||
                (save_month && save_month <= d.months) || (stop_month && stop_month <= d.months))
                return std::string("natural requested observer boundary already passed");
            if (!save && !explicit_trace && stop_at - d.next_frame + 1 > implicit_trace_frame_limit)
                return std::string("natural long run without capture requires explicit trace-from");
            return std::string{};
        };
        good(restore_startup_application_replay(source, live, controller, app, m, restore_validator, restore_protected));
        restore_seconds = std::chrono::duration<double>(Clock::now() - started).count(); driver = decode(m.controller_state);
    } else {
        good(app.request_new_game(0)); good(app.start_game());
        require(app.take_sound_requests().empty(), "natural start unexpectedly emitted unregistered sound");
        driver = initial_driver(app);
    }
    good(validate(app, metadata(driver)));
    require(driver.next_frame <= stop_at && (!save_at || (save_at >= driver.next_frame && save_at + tail <= stop_at)), "natural observer window/bound");
    std::ostringstream trace_output;
    std::uint64_t captured{}, capture_months{}, trace_lines{}; double capture_seconds{};
    std::size_t trace_bytes{};
    const auto first_frame = driver.next_frame;
    while (driver.next_frame <= stop_at && driver.phase != Phase::finished) {
        const auto before_month = driver.months;
        std::vector<int> sounds;
        try { sounds = step(app, driver); }
        catch (const std::exception &error) {
            throw std::runtime_error("natural frame=" + std::to_string(driver.next_frame) + " months=" +
                                     std::to_string(driver.months) + ": " + error.what());
        }
        const auto frame = driver.next_frame - 1;
        const auto trace_from = save ? (captured ? captured + 1 : frame_limit + 1) : explicit_trace ? explicit_trace : first_frame;
        if (frame >= trace_from) {
            const auto line = trace_line(app, driver, sounds, paths(live).system);
            // 独立测试输出预算，不调整Owner、文件或全历史解码预算；追加前拒绝。
            require(line.size() <= trace_budget - trace_bytes, "natural trace exceeds 8MiB observer output budget");
            trace_output << line; require(bool(trace_output), "natural trace buffering failed");
            trace_bytes += line.size(); ++trace_lines;
        }
        if (before_month != driver.months)
            std::cout << "application-natural-month months=" << driver.months << " frame=" << frame
                      << " history=" << driver.history << " random=" << driver.random << std::endl;
        if (save && !captured && ((save_at && frame == save_at) || (save_month && driver.months >= save_month))) {
            const auto started = Clock::now();
            good(save_startup_application_replay(root.parent_path(), capture, app, metadata(driver), validate,
                                                load ? std::vector<fs::path>{trace, source} : std::vector<fs::path>{trace}));
            capture_seconds = std::chrono::duration<double>(Clock::now() - started).count(); captured = frame;
            capture_months = driver.months;
            std::cout << "application-natural-capture frame=" << frame << " next_frame=" << driver.next_frame
                      << " months=" << driver.months << " bytes=" << fs::file_size(capture) << '\n';
        }
        if ((tail && captured && frame >= captured + tail) || (stop_month && driver.months >= stop_month)) break;
    }
    require(!save || captured, "natural capture boundary not reached within observer budget");
    require(!tail || driver.next_frame - 1 >= captured + tail, "natural requested capture tail not completed");
    require(!stop_month || driver.months >= stop_month, "natural requested month not reached within observer budget");
    const auto output = trace_output.str();
    (void)persistence_detail::prepare_replay_capture_paths(root.parent_path(), trace, paths(live), trace_protected);
    persistence_detail::create_save_file(trace_target.container, Bytes(output.begin(), output.end()));
    const auto digest = startup_application_replay_digest(app, metadata(driver), validate);
    const auto sizes = resources(app);
    std::cout << "application-natural-summary {\"capture_frame\":" << (captured ? std::to_string(captured) : "null")
              << ",\"capture_months\":" << (captured ? std::to_string(capture_months) : "null")
              << ",\"completed_frame\":" << driver.next_frame - 1 << ",\"next_frame\":" << driver.next_frame
              << ",\"next_command\":" << driver.next_command << ",\"months\":" << driver.months << ",\"date\":["
              << driver.date[0] << ',' << driver.date[1] << ',' << driver.date[2] << ',' << driver.date[3]
              << "],\"digest\":\"" << digest << "\",\"resources\":[";
    for (std::size_t i = 0; i < sizes.size(); ++i) std::cout << (i ? "," : "") << sizes[i];
    std::cout << "],\"peaks\":[";
    for (std::size_t i = 0; i < driver.peaks.size(); ++i) std::cout << (i ? "," : "") << driver.peaks[i];
    std::cout << "],\"resource_names\":[\"humans\",\"facilities\",\"pages\",\"audit_history\",\"cash_entries\",\"retained_tasks\"]"
              << ",\"sound_count\":" << driver.sound_count << ",\"sound_hash\":\"" << driver.sound_hash
              << "\",\"random\":" << driver.random << ",\"phase\":" << std::uint64_t(driver.phase)
              << ",\"trace_rows\":" << trace_lines << ",\"capture_seconds\":" << capture_seconds
              << ",\"restore_seconds\":" << restore_seconds << "}\n";
    return 0;
}

int run_startup_application_natural_driver_checks(const std::filesystem::path &parent) {
    const auto root = parent / ("natural-driver-checks-" + std::to_string(Clock::now().time_since_epoch().count()));
    require(fs::create_directory(root), "natural checks exclusive directory");
    struct Cleanup { fs::path root; ~Cleanup() { std::error_code error; fs::remove_all(root, error); } } cleanup{root};
    int checks{};
    const auto check = [&](bool ok, const char *message) { ++checks; require(ok, message); };
    const auto live = root / "live"; require(fs::create_directory(live), "natural checks live directory");
    StartupApplication app(paths(live), ref::WorldRandomStream::from_java_seed(1));
    good(app.error()); good(app.request_new_game(0)); good(app.start_game());
    auto d = initial_driver(app); auto m = metadata(d);
    check(validate(app, m).empty(), "natural initial driver valid");
    const auto before = startup_application_replay_digest(app, m, validate);
    auto bad = m; bad.controller_state.push_back(0);
    check(!validate(app, bad).empty(), "natural trailing driver rejects");
    auto future = d; future.next_frame = frame_limit + 2; future.checks = frame_limit + 1;
    bad = metadata(future);
    check(!validate(app, bad).empty(), "natural future frame beyond fixed strategy rejects");
    auto overflow_month = d; overflow_month.months = std::numeric_limits<std::uint64_t>::max();
    check(!validate(app, metadata(overflow_month)).empty(),
          "natural month count rejects unsigned overflow before calendar addition");
    bad = m; bad.controller_state[bad.controller_state.size() - 72] = 0;
    check(validate(app, bad).find("unconsumed output") != std::string::npos, "natural unconsumed driver output rejects");
    bad = m; bad.next_command += 1;
    check(!validate(app, bad).empty(), "natural command sequence mismatch rejects");
    const auto rejected = root / "rejected.avra";
    check(!save_startup_application_replay(root, rejected, app, bad, validate).empty() && !fs::exists(rejected),
          "natural bad controller cannot publish a capture");
    check(startup_application_replay_digest(app, m, validate) == before, "natural rejected validation preserves complete app");
    // 实际pending声音的拒绝在下面辅助函数复用原87动作夹具；这里不假定对白等待会发声。
    return checks;
}

int run_startup_application_natural_pending_sound_check(const StartupApplication &app) {
    require(app.world() && !app.world()->state().sound_requests.empty(),
            "natural pending-sound check requires an actual unconsumed action output");
    // 夹具只负责实际声音门禁，不能据此取得自然Driver资格；专属错误发生在随机交接资格之前。
    const auto m = metadata(initial_driver(app));
    require(validate(app, m) == "natural unconsumed sound requests",
            "natural validator rejects actual pending sound before other snapshot qualifications");
    return 1;
}
