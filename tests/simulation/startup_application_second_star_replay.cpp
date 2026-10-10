#include "startup_application_second_star_replay.hpp"
#include "startup_application_active_replay.hpp"
#include "ark/assets/sha256.hpp"
#include "../../src/simulation/application/startup_application_replay_paths.hpp"
#include "../../src/simulation/persistence/startup_world_file_io.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
using namespace ark::simulation;
namespace ref = ark::simulation::rules;
namespace support = active_replay_support;
namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;
using Metadata = StartupApplicationReplayMetadata;
using Clock = std::chrono::steady_clock;
using Command = std::array<std::int64_t, 5>;
constexpr auto controller = "application-active-progression-v2";
constexpr auto source_hash = "16a1b8c48993a96c536e4fc5868c7408355776c3b4631145047a2b4706c8062b";
constexpr auto certificate_hash = "8484c8cac5578c431dfd307903d7659b24296741eeace84cd875ab176c3332e3";
constexpr auto history_hash = "e6452b6d5b7ab9d9124ffde656144f1f4780c0a19538eb5f1c07a74b32978218";
constexpr auto origin_digest = "2ee7ad91c02e2d701fa3ac3b9578930b3c9b9b348c4099621e10958e74f0524b";
// 固定34429交接时的目录身份；后续系统文件可正常变化，不拿它约束当前文件。
constexpr auto origin_files_digest = "ece8dc44f2f2b910d2a9e8498e5f5ead021c676ac61418a7c59b90ebeff6028d";
constexpr std::uint64_t origin_next_frame = 34430, frame_limit = 180000, stall_limit = 100000;
constexpr std::size_t trace_budget = 8U * 1024U * 1024U;
enum class Intent : std::uint64_t { wait, acknowledge, tasks, select, confirm, depart, deadline, award, leave };
enum class Input : std::uint64_t { wait, acknowledge, open_task, task, award, leave, count };
struct Round { std::vector<Command> commands; std::vector<StartupAudioRequest> sounds; };
void require(bool condition, const std::string &reason) {
    if (!condition) throw std::runtime_error("second-star: " + reason);
}
void good(const std::string &error) { require(error.empty(), error); }
Bytes read(const fs::path &p) {
    require(fs::is_regular_file(fs::symlink_status(p)), "existing evidence must be regular file");
    const auto size = fs::file_size(p);
    require(size > 0 && size <= 128U * 1024U * 1024U, "existing evidence 128MiB read budget");
    std::ifstream in(p, std::ios::binary); require(bool(in), "cannot read existing evidence");
    Bytes bytes(static_cast<std::size_t>(size));
    in.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(size));
    require(in.gcount() == static_cast<std::streamsize>(size) && in.peek() == std::char_traits<char>::eof(),
            "existing evidence changed size while reading");
    return bytes;
}
std::string hash(const Bytes &b) { return ark::assets::sha256_hex(b); }
bool valid_hash(const std::string &s) { return s.size() == 64 && s.find_first_not_of("0123456789abcdef") == s.npos; }
std::string hex(const Bytes &b) {
    std::string s; constexpr auto digits = "0123456789abcdef";
    for (auto c : b) { s += digits[c >> 4]; s += digits[c & 15]; } return s;
}
void number(Bytes &out, std::uint64_t n) { for (int i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>(n >> (8 * i))); }
void blob(Bytes &out, const Bytes &b) { number(out, b.size()); out.insert(out.end(), b.begin(), b.end()); }
void string(Bytes &out, const std::string &s) { blob(out, Bytes(s.begin(), s.end())); }
struct Reader {
    const Bytes &data; std::size_t at{};
    std::uint64_t number() {
        require(at <= data.size() && data.size() - at >= 8, "truncated driver number");
        std::uint64_t n{}; for (int i = 0; i < 8; ++i) n |= std::uint64_t(data[at++]) << (8 * i); return n;
    }
    Bytes blob(std::size_t bound) {
        const auto n = number(); require(n <= bound && n <= data.size() - at, "driver blob budget");
        Bytes b(data.begin() + static_cast<std::ptrdiff_t>(at), data.begin() + static_cast<std::ptrdiff_t>(at + n)); at += n; return b;
    }
    std::string string(std::size_t bound) {
        const auto b = blob(bound); std::string s(b.begin(), b.end());
        require(s.find('\0') == s.npos, "driver text contains nul"); return s;
    }
};
// 交接源只存不变来源；业务仍由唯一应用Owner拥有。
struct Driver {
    Metadata origin;
    std::string files_digest;
    std::string producer{"unspecified"};
    std::uint64_t next_frame{origin_next_frame}, next_command{1}, checks{}, next_task_month{}, previous_task{};
    std::uint64_t accepted{}, departed{}, accepted_task{}, task_successes{}, random{}, steps{}, history{};
    std::uint64_t months{}, last_month_frame{origin_next_frame - 1}, top_id{}, top_kind{}, top_raw{}, sound_count{};
    Intent pending{Intent::wait};
    std::array<std::uint64_t, 4> date{};
    std::array<std::uint64_t, static_cast<std::size_t>(Input::count)> commands{};
    std::array<std::uint64_t, 12> peaks{};
    std::string sound_hash{hash({})};
};
Bytes encode(const Driver &d) {
    Bytes b{'A','V','A','C','T','D','R','2'};
    for (auto n : {std::uint64_t(1), std::uint64_t(1), std::uint64_t(0), frame_limit, stall_limit}) number(b, n);
    string(b, source_hash); string(b, certificate_hash); string(b, history_hash); string(b, origin_digest);
    string(b, d.origin.controller_id); string(b, d.origin.producer_revision);
    number(b, d.origin.next_frame); number(b, d.origin.next_command); blob(b, d.origin.controller_state);
    string(b, d.files_digest); string(b, d.producer);
    for (auto n : {d.next_frame, d.next_command, d.checks, d.next_task_month, d.previous_task,
                   d.accepted, d.departed, d.accepted_task, d.task_successes, d.random, d.steps, d.history,
                   d.months, d.last_month_frame, d.top_id, d.top_kind, d.top_raw, d.sound_count, std::uint64_t(d.pending)}) number(b, n);
    for (auto n : d.date) number(b, n);
    for (auto n : d.commands) number(b, n);
    for (auto n : d.peaks) number(b, n);
    number(b, 1); string(b, d.sound_hash); return b;
}
Driver decode(const Bytes &b) {
    require(b.size() >= 8 && std::string(b.begin(), b.begin() + 8) == "AVACTDR2", "driver magic");
    Reader r{b, 8};
    require(r.number() == 1 && r.number() == 1 && r.number() == 0 && r.number() == frame_limit && r.number() == stall_limit,
            "driver version/seed/speed/strategy");
    require(r.string(64) == source_hash && r.string(64) == certificate_hash && r.string(64) == history_hash &&
                r.string(64) == origin_digest, "fixed handoff identity/history restriction");
    Driver d;
    d.origin.controller_id = r.string(128); d.origin.producer_revision = r.string(256);
    d.origin.next_frame = r.number(); d.origin.next_command = r.number(); d.origin.controller_state = r.blob(16384);
    good(support::validate_v1_origin(d.origin));
    d.files_digest = r.string(64); d.producer = r.string(256);
    for (auto *n : {&d.next_frame, &d.next_command, &d.checks, &d.next_task_month, &d.previous_task,
                    &d.accepted, &d.departed, &d.accepted_task, &d.task_successes, &d.random, &d.steps, &d.history,
                    &d.months, &d.last_month_frame, &d.top_id, &d.top_kind, &d.top_raw, &d.sound_count}) *n = r.number();
    const auto action = r.number(); require(action <= std::uint64_t(Intent::leave), "driver intent"); d.pending = static_cast<Intent>(action);
    for (auto &n : d.date) n = r.number();
    for (auto &n : d.commands) n = r.number();
    for (auto &n : d.peaks) n = r.number();
    require(r.number() == 1, "unconsumed output"); d.sound_hash = r.string(64);
    require(r.at == b.size() && encode(d) == b && d.files_digest == origin_files_digest && valid_hash(d.sound_hash) &&
                !d.producer.empty(), "canonical driver/text");
    require(d.next_frame >= origin_next_frame && d.next_frame <= frame_limit + 1 &&
                d.checks == d.next_frame - origin_next_frame && d.next_command >= 1 && d.commands[0] == 0 &&
                d.next_command <= 1024 * (d.checks + 1) && d.date[0] < 15 && d.date[1] < 12 && d.date[2] < 4 &&
                d.date[3] < 10800 && d.months + 3 == d.date[0] * 12 + d.date[1] &&
                d.next_task_month <= d.months + 5 && d.last_month_frame < d.next_frame &&
                d.next_frame - 1 - d.last_month_frame <= stall_limit && d.top_id && d.top_kind <= 4 && d.top_raw < 128 &&
                d.accepted <= 1 && d.departed <= d.accepted && (bool(d.accepted) == bool(d.accepted_task)) &&
                d.sound_count <= 100 * d.checks, "driver counter/calendar/receipt bounds");
    std::uint64_t total{};
    for (auto n : d.commands) { require(n <= d.next_command - 1 - total, "command total overflow"); total += n; }
    require(total == d.next_command - 1 && d.accepted <= total && d.departed <= total, "driver command totals");
    return d;
}
Metadata metadata(const Driver &d) {
    Metadata m; m.controller_id = controller; m.producer_revision = d.producer;
    m.next_frame = d.next_frame; m.next_command = d.next_command; m.controller_state = encode(d); return m;
}
bool affordable(const StartupWorldRuntimeState &s, std::uint64_t id) {
    const auto found = s.tasks.find(id);
    require(found != s.tasks.end(), "task catalogue identity missing");
    const auto definition = found->second.definition;
    require(definition >= 0 && static_cast<std::size_t>(definition) < s.rules->tasks.size(), "task definition range");
    return s.rules->tasks.at(definition).recruitment_fee <= s.scene.world.world.ai.accounting.funds();
}
Intent plan(const StartupApplication &app, const Driver &d) {
    const auto *p = support::top(app); require(p, "missing top page"); const auto &s = app.world()->state();
    if (p->kind == ref::WorldScriptPageKind::dialogue || p->kind == ref::WorldScriptPageKind::simple_message ||
        p->kind == ref::WorldScriptPageKind::newspaper) return Intent::acknowledge;
    if (p->kind == ref::WorldScriptPageKind::scene) {
        const auto month = s.scene.calendar.year * 12 + s.scene.calendar.month;
        if (s.scene.scene_state == 0 && !s.active_task && !d.accepted && month >= 0 &&
            std::uint64_t(month) >= d.next_task_month &&
            std::any_of(s.task_order.begin(), s.task_order.end(), [&](auto id) { return affordable(s, id); })) return Intent::tasks;
        return Intent::wait;
    }
    require(p->kind == ref::WorldScriptPageKind::raw_page, "unknown page kind");
    switch (p->legacy_page) {
    case 16: case 24: case 56: case 57: case 97: case 98: return Intent::wait;
    case 22: return Intent::select;
    case 23: return Intent::confirm;
    case 25: return Intent::depart;
    case 28: {
        const auto phase = s.page_phases.find(p->id), counter = s.page_counters.find(p->id);
        require(phase != s.page_phases.end() && (phase->second == 0 || phase->second == 1) &&
                    counter != s.page_counters.end() && counter->second >= 0 &&
                    s.task_page_predictions.count(p->id) && s.task_page_acceleration.count(p->id), "task28 committed payload missing");
        return phase->second == 0 ? Intent::confirm : Intent::wait;
    }
    case 33: return Intent::deadline;
    case 87: return s.medal_count > 0 ? Intent::award : Intent::wait;
    case 83: return Intent::leave;
    case 11: case 30: case 31: case 32: case 49: case 50: case 59: case 67:
    case 88: case 89: case 94: case 95: case 96: case 99: case 100: return Intent::acknowledge;
    default: throw std::runtime_error("second-star unknown raw=" + std::to_string(p->legacy_page) + " page=" + std::to_string(p->id));
    }
}
void observe(const StartupApplication &app, Driver &d) {
    const auto &s = app.world()->state(); const auto *p = support::top(app); require(p, "missing observed page");
    d.date = support::date(app); d.months = d.date[0] * 12 + d.date[1] - 3;
    d.random = s.scene.random.draws(); d.steps = s.simulation_steps; d.history = app.world()->checkpoints().size();
    require(s.task_progress.successes >= 0, "negative task successes"); d.task_successes = s.task_progress.successes;
    d.top_id = p->id; d.top_kind = std::uint64_t(p->kind); d.top_raw = p->kind == ref::WorldScriptPageKind::raw_page ? p->legacy_page : 0;
    const auto sizes = support::resources(app); for (std::size_t i = 0; i < sizes.size(); ++i) d.peaks[i] = std::max(d.peaks[i], sizes[i]);
    d.pending = plan(app, d);
}
std::string validate(const StartupApplication &app, const Metadata &m) {
    try {
        require(m.controller_id == controller && m.extensions.empty(), "metadata identity"); const auto d = decode(m.controller_state);
        require(m.producer_revision == d.producer && m.next_frame == d.next_frame && m.next_command == d.next_command &&
                    app.error().empty() && app.mode() == StartupApplicationMode::logic &&
                    app.page() == StartupApplicationPage::world && app.world(), "application binding");
        const auto &s = app.world()->state(); const auto *p = support::top(app);
        const auto seed = ref::WorldRandomStream::from_java_seed(1).snapshot();
        require(app.handoff_random() && app.handoff_random()->cursor == 0 && app.handoff_random()->engine_state == seed.engine_state &&
                    !app.handoff_random()->tape_mode && app.handoff_random()->tape.empty() && s.rank >= 1 && s.scene.speed_setting == 0 &&
                    !s.scene.framework_paused && !s.scripts.executing_page && !app.has_pending_audio_requests() &&
                    s.sound_requests.empty() && !app.clear_page() && support::references(s) &&
                    d.random == s.scene.random.draws() && d.steps == s.simulation_steps && d.history == app.world()->checkpoints().size() &&
                    d.date == support::date(app) && p && d.top_id == p->id && d.top_kind == std::uint64_t(p->kind) &&
                    d.top_raw == std::uint64_t(p->kind == ref::WorldScriptPageKind::raw_page ? p->legacy_page : 0) &&
                    d.pending == plan(app, d) && s.task_progress.successes >= 0 && d.task_successes == std::uint64_t(s.task_progress.successes),
                "precise world/page/output binding");
        require((!d.previous_task || d.previous_task < s.next_task_identity) &&
                    (!d.accepted_task || s.tasks.count(d.accepted_task)), "task receipt retained identity");
        const auto sizes = support::resources(app);
        for (std::size_t i = 0; i < sizes.size(); ++i) require(d.peaks[i] >= sizes[i], "resource peak");
        return {};
    } catch (const std::exception &e) { return e.what(); }
}
int check_bound_driver(const StartupApplication &app, const Driver &d) {
    // 对本次真实完整恢复对象做纯拒绝检查；不构造替代首星业务夹具。
    const auto before = startup_application_replay_digest(app, metadata(d), validate);
    int checks = support::check_v1_origin_binding(d.origin);
    const auto reject = [&](const Metadata &m, const char *reason) { require(!validate(app, m).empty(), reason); ++checks; };
    auto m = metadata(d); m.controller_state.pop_back(); reject(m, "truncated v2 accepted");
    m = metadata(d); m.controller_state.push_back(0); reject(m, "trailing v2 accepted");
    m = metadata(d); m.controller_id = d.origin.controller_id; reject(m, "v1 treated as v2");
    m = metadata(d); ++m.next_command; reject(m, "metadata command mismatch accepted");
    auto bad = d; ++bad.next_frame; reject(metadata(bad), "wrong absolute frame accepted");
    bad = d; ++bad.origin.next_command; reject(metadata(bad), "changed v1 origin accepted");
    bad = d; bad.pending = d.pending == Intent::wait ? Intent::tasks : Intent::wait;
    reject(metadata(bad), "wrong current intent accepted");
    bad = d; bad.commands[1] = UINT64_MAX; reject(metadata(bad), "command overflow accepted");
    bad = d; bad.accepted = 1; bad.accepted_task = 0; reject(metadata(bad), "receipt without task accepted");
    bad = d; bad.files_digest.clear(); reject(metadata(bad), "missing origin file identity accepted");
    bad = d; bad.files_digest[0] = bad.files_digest[0] == '0' ? '1' : '0';
    require(valid_hash(bad.files_digest), "origin hash mutation must remain valid hex");
    reject(metadata(bad), "changed valid-hex origin file identity accepted");
    m = metadata(d);
    const Bytes restriction(history_hash, history_hash + std::char_traits<char>::length(history_hash));
    const auto where = std::search(m.controller_state.begin(), m.controller_state.end(), restriction.begin(), restriction.end());
    require(where != m.controller_state.end(), "missing history restriction in canonical encoding");
    *where = *where == '0' ? '1' : '0'; reject(m, "candidate history restriction changed");
    m = metadata(d); require(m.controller_state.size() >= 80, "output marker offset");
    m.controller_state[m.controller_state.size() - 80] = 0; reject(m, "unconsumed output accepted");
    require(before == startup_application_replay_digest(app, metadata(d), validate), "rejected driver changed restored application");
    return checks + 1;
}
// 目录摘要只在交接边界取两次；逐帧世界digest不触磁盘，实际system另列trace。
std::string files_digest(const fs::path &root) {
    std::vector<fs::path> paths;
    for (const auto &entry : fs::recursive_directory_iterator(root)) {
        require(!fs::is_symlink(entry.symlink_status()), "handoff symlink in isolated files");
        if (entry.is_regular_file()) paths.push_back(entry.path());
    }
    std::sort(paths.begin(), paths.end()); Bytes canonical;
    for (const auto &p : paths) { string(canonical, p.lexically_relative(root).generic_u8string()); string(canonical, hash(read(p))); }
    return hash(canonical);
}
Driver initial(const StartupApplication &app, const support::Terminal &old, const fs::path &live) {
    good(support::validate_v1(app, old.metadata));
    const auto before = startup_application_replay_digest(app, old.metadata, support::validate_v1);
    const auto before_files = files_digest(live);
    Driver d; d.origin = old.metadata; d.next_task_month = old.next_task_month; d.files_digest = before_files;
    d.previous_task = app.world()->state().active_task.value_or(0); observe(app, d);
    require(before == origin_digest && before == startup_application_replay_digest(app, old.metadata, support::validate_v1) &&
                before_files == files_digest(live), "handoff constructor changed business/files");
    good(validate(app, metadata(d))); return d;
}
Round step(StartupApplication &app, Driver &d) {
    good(validate(app, metadata(d))); require(d.next_frame <= frame_limit, "absolute frame bound");
    const auto frame = d.next_frame, old_month = d.months, old_random = d.random, old_steps = d.steps, old_history = d.history;
    const auto old_date = d.date; good(app.update());
    const auto &s = app.world()->state();
    if (d.previous_task && !s.active_task) d.next_task_month = s.scene.calendar.year * 12 + s.scene.calendar.month + 2;
    d.previous_task = s.active_task.value_or(0);
    const auto intent = plan(app, d); const auto page = *support::top(app); Round round;
    const auto record = [&](Input input, std::uint64_t id = 0, std::int64_t a = 0, std::int64_t b = 0) {
        require(id <= std::uint64_t(INT64_MAX), "command page range");
        round.commands.push_back({std::int64_t(input), std::int64_t(id), a, b, 0});
        if (input != Input::wait) { ++d.next_command; ++d.commands[std::size_t(input)]; }
    };
    const auto task = [&](StartupWorldTaskAction action, int choice = 0) {
        const auto result = app.act_task_page(page.id, action, choice); good(result.error);
        require(result.denial == ref::TaskCommandDenial::none, "task business denial");
        if (result.accepted) {
            // raw23收款只开启募集；active_task须等真实raw28出发，不能提前要求或伪写。
            const auto &after = app.world()->state();
            require(!d.accepted && page.legacy_page == 23 && page.task_identity && after.tasks.count(*page.task_identity) &&
                        std::any_of(after.scripts.pages.begin(), after.scripts.pages.end(), [&](const auto &p) {
                            return p.lifecycle != 4 && p.kind == ref::WorldScriptPageKind::raw_page &&
                                   p.legacy_page == 24 && p.task_identity == page.task_identity;
                        }), "accepted recruitment receipt missing/duplicate");
            d.accepted = 1; d.accepted_task = *page.task_identity;
        }
        d.departed += result.departed ? 1 : 0; record(Input::task, page.id, static_cast<int>(action), choice);
    };
    switch (intent) {
    case Intent::wait: record(Input::wait); break;
    case Intent::acknowledge: good(app.acknowledge_page(page.id)); record(Input::acknowledge, page.id); break;
    case Intent::tasks: good(app.open_task_menu()); record(Input::open_task); break;
    case Intent::select: {
        const auto list = s.task_page_lists.find(page.id);
        require(list != s.task_page_lists.end(), "initialized task22 catalogue missing");
        const auto choice = std::find_if(list->second.begin(), list->second.end(), [&](auto id) { return affordable(s, id); });
        require(choice != list->second.end(), "no affordable current task");
        task(StartupWorldTaskAction::confirm, static_cast<int>(choice - list->second.begin())); break;
    }
    case Intent::confirm: task(StartupWorldTaskAction::confirm); break;
    case Intent::depart: task(StartupWorldTaskAction::depart); break;
    case Intent::deadline: task(StartupWorldTaskAction::confirm, s.scene.world.world.ai.accounting.funds() >= page.legacy_f ? 0 : 1); break;
    case Intent::award:
        good(app.act_award_page(page.id, ref::WorldAwardAction::request_award, 0)); record(Input::award, page.id, static_cast<int>(ref::WorldAwardAction::request_award));
        good(app.act_award_page(page.id, ref::WorldAwardAction::confirm_award)); record(Input::award, page.id, static_cast<int>(ref::WorldAwardAction::confirm_award)); break;
    case Intent::leave: good(app.leave_commerce_page(page.id)); record(Input::leave, page.id); break;
    }
    round.sounds = app.take_audio_requests(); Bytes audio(d.sound_hash.begin(), d.sound_hash.end());
    number(audio, frame); number(audio, round.sounds.size());
    for (const auto &sound : round.sounds) {
        require(std::uint64_t(sound.operation) <= 2 && sound.id >= 0 && sound.id < 26, "typed audio range");
        number(audio, std::uint64_t(sound.operation)); number(audio, sound.id);
    }
    require(app.take_sound_requests().empty(), "output consumed twice"); d.sound_hash = hash(audio); d.sound_count += round.sounds.size();
    ++d.next_frame; ++d.checks; observe(app, d);
    require(d.date >= old_date && (d.months == old_month || d.months == old_month + 1) &&
                d.random >= old_random && d.steps >= old_steps && d.history >= old_history, "calendar/history rewound");
    if (d.months != old_month) d.last_month_frame = frame;
    good(validate(app, metadata(d))); return round;
}
std::string handoff_json(const Driver &d) {
    std::ostringstream out;
    out << "{\"origin_metadata\":{\"controller\":\"" << d.origin.controller_id << "\",\"producer_revision\":\"" << d.origin.producer_revision
        << "\",\"next_frame\":" << d.origin.next_frame << ",\"next_command\":" << d.origin.next_command
        << ",\"driver\":\"" << hex(d.origin.controller_state) << "\",\"driver_digest\":\"" << hash(d.origin.controller_state)
        << "\"},\"before_digest\":\"" << origin_digest << "\",\"after_digest\":\"" << origin_digest
        << "\",\"origin_snapshot_sha256\":\"" << source_hash << "\",\"origin_certificate_sha256\":\"" << certificate_hash
        << "\",\"uncertified_history_sha256\":\"" << history_hash << "\",\"files_before_digest\":\"" << d.files_digest
        << "\",\"files_after_digest\":\"" << d.files_digest << "\"}"; return out.str();
}
std::string trace_line(const StartupApplication &app, const Driver &d, const Round &round, const fs::path &system) {
    const auto bytes = encode(d); std::ostringstream out;
    out << "{\"frame\":" << d.next_frame - 1 << ",\"next_frame\":" << d.next_frame << ",\"next_command\":" << d.next_command << ",\"commands\":[";
    for (std::size_t n = 0; n < round.commands.size(); ++n) {
        out << (n ? ",[" : "["); for (std::size_t i = 0; i < 5; ++i) out << (i ? "," : "") << round.commands[n][i]; out << ']';
    }
    out << "],\"driver\":\"" << hex(bytes) << "\",\"driver_digest\":\"" << hash(bytes)
        << "\",\"digest\":\"" << startup_application_replay_digest(app, metadata(d), validate)
        << "\",\"system_digest\":\"" << hash(read(system)) << "\",\"sounds\":[";
    for (std::size_t n = 0; n < round.sounds.size(); ++n) out << (n ? "," : "") << '[' << static_cast<int>(round.sounds[n].operation) << ',' << round.sounds[n].id << ']';
    out << "]}\n"; return out.str();
}
std::uint64_t option_number(const std::map<std::string, std::string> &options, const std::string &key, std::uint64_t fallback) {
    const auto p = options.find(key); if (p == options.end()) return fallback;
    require(!p->second.empty() && p->second.find_first_not_of("0123456789") == p->second.npos, "invalid number " + key);
    const auto n = std::stoull(p->second); require(n <= frame_limit, "number bound " + key); return n;
}
} // namespace

int run_startup_application_second_star_replay_cli(int argc, const char **argv) {
    require(argc > 1 && std::string(argv[1]) == controller, "CLI identity");
    const std::array<std::string, 12> keys{"--work-dir", "--trace-file", "--stop-at", "--load-file", "--save-file", "--save-at", "--tail-after-save", "--trace-from", "--producer-revision", "--handoff-file", "--handoff-certificate-sha256", "--handoff-snapshot-sha256"};
    std::map<std::string, std::string> options;
    for (int i = 2; i < argc; i += 2) {
        require(i + 1 < argc && std::find(keys.begin(), keys.end(), argv[i]) != keys.end(), "unknown/missing CLI option");
        require(options.emplace(argv[i], argv[i + 1]).second && !options.at(argv[i]).empty(), "repeated/empty option");
    }
    require(options.count("--work-dir") && options.count("--trace-file") && options.count("--stop-at"), "required CLI paths/bound");
    const bool load = options.count("--load-file"), handoff = options.count("--handoff-file"), save = options.count("--save-file");
    require(load != handoff && bool(options.count("--handoff-certificate-sha256")) == handoff &&
                bool(options.count("--handoff-snapshot-sha256")) == handoff, "exclusive v1 handoff/v2 restore");
    if (handoff) require(options.at("--handoff-certificate-sha256") == certificate_hash && options.at("--handoff-snapshot-sha256") == source_hash,
                         "explicit fixed source certificate/hash");
    const auto stop = option_number(options, "--stop-at", 0), save_at = option_number(options, "--save-at", 0);
    const auto tail = option_number(options, "--tail-after-save", save ? 20 : 0), trace_from = option_number(options, "--trace-from", 0);
    require(stop >= origin_next_frame && save == bool(save_at) && (!save || (tail > 0 && tail <= 1000 && save_at + tail <= stop)) &&
                (!options.count("--tail-after-save") || save) && (!save || !trace_from) &&
                (!options.count("--trace-from") || (trace_from >= origin_next_frame && trace_from <= stop)), "observer relationships");
    const auto producer = options.count("--producer-revision") ? options.at("--producer-revision") : "unspecified";
    require(!producer.empty() && producer.size() <= 256 && producer.find('\0') == producer.npos, "producer identity");
    const auto root = fs::absolute(options.at("--work-dir")), trace = fs::absolute(options.at("--trace-file"));
    const auto live = root / "application", unused = root / "unused-current";
    const auto source = fs::absolute(options.at(load ? "--load-file" : "--handoff-file"));
    const auto capture = save ? fs::absolute(options.at("--save-file")) : fs::path{};
    const StartupApplicationPaths unused_paths{unused};
    std::vector<fs::path> protected_paths{trace, source}; if (save) protected_paths.push_back(capture);
    (void)persistence_detail::prepare_replay_capture_paths(root, root / ".second-star-probe", unused_paths, protected_paths);
    auto restore_protected = std::vector<fs::path>{trace}; if (save) restore_protected.push_back(capture);
    (void)persistence_detail::prepare_replay_restore_paths(live, source, unused_paths, restore_protected);
    auto trace_protected = std::vector<fs::path>{source}; if (save) trace_protected.push_back(capture);
    const auto trace_target = persistence_detail::prepare_replay_capture_paths(root.parent_path(), trace, unused_paths, trace_protected);
    if (save) (void)persistence_detail::prepare_replay_capture_paths(root.parent_path(), capture, unused_paths, {trace, source});
    const auto original_hash = hash(read(source));
    std::unique_ptr<StartupApplication> app; Driver d; const auto restore_started = Clock::now();
    if (handoff) {
        auto old = support::prepare_v1_terminal(source, live, unused, restore_protected);
        d = initial(*old.application, old, live); app = std::move(old.application);
    } else {
        require(fs::create_directory(unused), "exclusive unused directory");
        app = std::make_unique<StartupApplication>(unused_paths, ref::WorldRandomStream::from_java_seed(1)); good(app->error()); Metadata m;
        const auto loader = [&](const StartupApplication &a, const Metadata &candidate) {
            const auto error = validate(a, candidate); if (!error.empty()) return error;
            if (candidate.next_frame > stop || (save && save_at + 1 < candidate.next_frame)) return std::string("second-star observer before source");
            if (!save && !trace_from && stop - candidate.next_frame + 1 > 1000) return std::string("second-star long restore requires trace-from");
            return std::string{};
        };
        good(restore_startup_application_replay(source, live, controller, *app, m, loader, restore_protected)); d = decode(m.controller_state);
    }
    const auto restore_seconds = std::chrono::duration<double>(Clock::now() - restore_started).count();
    require(original_hash == hash(read(source)), "source changed during restore/handoff");
    d.producer = producer; good(validate(*app, metadata(d)));
    const auto driver_checks = check_bound_driver(*app, d);
    require(d.next_frame <= stop && (!save || save_at + 1 >= d.next_frame), "observer before boundary");
    const auto first = d.next_frame; std::uint64_t captured{}, capture_rank{}, capture_accepted{}, rows{}; double capture_seconds{};
    std::size_t bytes{}; std::ostringstream output;
    const auto capture_now = [&] {
        const auto started = Clock::now();
        good(save_startup_application_replay(root.parent_path(), capture, *app, metadata(d), validate, {trace, source}));
        capture_seconds = std::chrono::duration<double>(Clock::now() - started).count(); captured = d.next_frame - 1;
        capture_rank = app->world()->state().rank; capture_accepted = d.accepted;
        std::cout << "application-second-star-capture frame=" << captured << " next_frame=" << d.next_frame << " rank=" << capture_rank << " bytes=" << fs::file_size(capture) << std::endl;
    };
    if (save && save_at == d.next_frame - 1) capture_now(); // 真正零v2输入的轮末交接快照。
    while (d.next_frame <= stop) {
        const auto previous_month = d.months; Round round;
        try { round = step(*app, d); }
        catch (const std::exception &e) { throw std::runtime_error("second-star frame=" + std::to_string(d.next_frame) + ": " + e.what()); }
        const auto frame = d.next_frame - 1;
        const auto begin = save ? (captured ? captured + 1 : frame_limit + 1) : trace_from ? trace_from : first;
        if (frame >= begin) {
            const auto line = trace_line(*app, d, round, live / "system.avr"); require(line.size() <= trace_budget - bytes, "trace 8MiB budget");
            output << line; bytes += line.size(); ++rows;
        }
        if (d.months != previous_month) std::cout << "application-second-star-month months=" << d.months << " frame=" << frame << " progress=" << support::progress_json(*app) << std::endl;
        if (save && frame == save_at) capture_now();
        if (captured && frame >= captured + tail) break;
    }
    require(!save || (captured && d.next_frame - 1 >= captured + tail), "capture/tail not completed");
    require(original_hash == hash(read(source)), "source changed while replaying");
    const auto text = output.str();
    (void)persistence_detail::prepare_replay_capture_paths(root.parent_path(), trace, StartupApplicationPaths{live}, trace_protected);
    persistence_detail::create_save_file(trace_target.container, Bytes(text.begin(), text.end()));
    const auto sizes = support::resources(*app);
    std::cout << "application-second-star-summary {\"controller\":\"" << controller << "\",\"capture_frame\":" << (captured ? std::to_string(captured) : "null")
        << ",\"capture_rank\":" << (captured ? std::to_string(capture_rank) : "null") << ",\"capture_accepted_tasks\":" << (captured ? std::to_string(capture_accepted) : "null")
        << ",\"completed_frame\":" << d.next_frame - 1 << ",\"next_frame\":" << d.next_frame << ",\"next_command\":" << d.next_command
        << ",\"rank\":" << app->world()->state().rank << ",\"months\":" << d.months << ",\"date\":[" << d.date[0] << ',' << d.date[1] << ',' << d.date[2] << ',' << d.date[3]
        << "],\"digest\":\"" << startup_application_replay_digest(*app, metadata(d), validate) << "\",\"resources\":[";
    for (std::size_t i = 0; i < sizes.size(); ++i) std::cout << (i ? "," : "") << sizes[i];
    std::cout << "],\"peaks\":["; for (std::size_t i = 0; i < d.peaks.size(); ++i) std::cout << (i ? "," : "") << d.peaks[i];
    std::cout << "],\"sound_count\":" << d.sound_count << ",\"sound_hash\":\"" << d.sound_hash << "\",\"random\":" << d.random
        << ",\"phase\":" << d.accepted << ",\"terminal\":false,\"stage_complete\":" << (d.accepted ? "true" : "false")
        << ",\"accepted_tasks\":" << d.accepted << ",\"departed_tasks\":" << d.departed << ",\"accepted_task_identity\":" << d.accepted_task
        << ",\"task_successes\":" << d.task_successes << ",\"active_command_count\":" << d.next_command - 1 << ",\"trace_rows\":" << rows
        << ",\"driver_checks\":" << driver_checks
        << ",\"capture_seconds\":" << capture_seconds << ",\"restore_seconds\":" << restore_seconds
        << ",\"progress\":" << support::progress_json(*app) << ",\"handoff\":" << handoff_json(d) << "}\n";
    return 0;
}

int run_startup_application_second_star_driver_checks(const fs::path &parent) {
    // 新局只用来证明非terminal拒绝，不伪造首星Owner来凑正向交接。
    const auto owner = fs::canonical(parent);
    const auto root = owner / ("second-star-driver-" + std::to_string(Clock::now().time_since_epoch().count()));
    require(fs::create_directory(root), "exclusive driver check root");
    try {
        StartupApplication app({root}, ref::WorldRandomStream::from_java_seed(1)); good(app.error());
        good(app.request_new_game(0)); good(app.start_game());
        Metadata missing; require(!support::validate_v1(app, missing).empty(), "missing v1 accepted");
        require(!support::validate_v1_origin(missing).empty(), "nonterminal empty origin accepted");
        int checks = 2;
        for (const Bytes &bad : {Bytes{}, Bytes{'A','V','A','C','T','D','R','1'}, Bytes{'A','V','A','C','T','D','R','2'}}) {
            bool refused{}; try { (void)decode(bad); } catch (const std::exception &) { refused = true; }
            require(refused, "truncated/wrong-controller Driver accepted"); ++checks;
        }
        require(!fs::is_symlink(fs::symlink_status(root)) && fs::canonical(root).parent_path() == owner, "driver cleanup changed path");
        fs::remove_all(root); return checks;
    } catch (...) {
        std::cerr << "second-star check failed; retained evidence: " << root.u8string() << '\n'; throw;
    }
}
