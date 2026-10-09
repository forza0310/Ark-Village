#include "startup_application_active_replay.hpp"
#include "dungeon_village_prototype/startup_application_replay.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_prototype/startup_world_village_activity.hpp"
#include "dungeon_village_reference/world_calendar_tasks.hpp"
#include "dungeon_village_tools/archive.hpp"
#include "startup_application_replay_paths.hpp"
#include "startup_world_file_io.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace {
using namespace dungeon_village_prototype;
namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;
using Metadata = StartupApplicationReplayMetadata;
using Clock = std::chrono::steady_clock;
constexpr const char *controller = "application-active-progression-v1";
constexpr std::uint64_t frame_limit = 180000, stall_limit = 100000;
constexpr std::uint64_t absent = std::numeric_limits<std::uint64_t>::max();
constexpr std::size_t trace_budget = 8U * 1024U * 1024U;
enum class Intent : std::uint64_t {
    wait, acknowledge, build, upgrade, activities, activity_select, activity_complete,
    tasks, task_select, task_confirm, depart, deadline, upgrade_confirm, rank, award, leave
};
// 实际API调用记录；一个有界策略动作可包含多个顺序命令，wait不增加next_command。
enum class Input : std::uint64_t {
    wait, acknowledge, open_build, select_build, place_build, cancel_build, open_facility,
    open_activities, select_activity, cancel_page, open_tasks, task, rank, award, leave_commerce, count
};
using Command = std::array<std::int64_t, 5>;
struct Round { std::vector<Command> commands; std::vector<StartupAudioRequest> sounds; };
void require(bool value, const std::string &reason) {
    if (!value) throw std::runtime_error("active: " + reason);
}
void good(const std::string &error) { require(error.empty(), error); }
void u64(Bytes &bytes, std::uint64_t n) {
    for (int i = 0; i < 8; ++i) bytes.push_back(static_cast<std::uint8_t>(n >> (8 * i)));
}
Bytes read_file(const fs::path &path) {
    std::ifstream in(path, std::ios::binary); require(bool(in), "cannot read existing file");
    return {std::istreambuf_iterator<char>(in), {}};
}
std::string hex(const Bytes &bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string out; out.reserve(bytes.size() * 2);
    for (auto b : bytes) { out += digits[b >> 4]; out += digits[b & 15]; }
    return out;
}
// 仅策略/身份/观测，不复制业务Owner或持有路径；完整编码后才允许保存应用。
struct Driver {
    std::uint64_t next_frame{1}, next_command{1}, checks{}, months{}, last_month_frame{};
    std::uint64_t random{}, steps{}, history{}, bakery{}, previous_task{}, next_task_month{};
    std::uint64_t next_activity_frame{}, completed_activities{}, exhibition_count{};
    std::uint64_t promoted_month{absent}, exhibition_month{absent}, exhibition_income{};
    std::uint64_t accepted_tasks{}, departed_tasks{}, task_successes{}, terminal{};
    std::uint64_t top_id{}, top_kind{}, top_raw{}, sound_count{};
    Intent pending{Intent::wait};
    std::array<std::uint64_t, 4> date{};
    std::array<std::uint64_t, static_cast<std::size_t>(Input::count)> commands{};
    std::array<std::uint64_t, 12> peaks{};
    std::set<std::uint64_t> upgrade_pages;
    std::string producer_revision{"unspecified"}; // 来源身份，不参与策略选择。
    std::string sound_hash = dungeon_village_tools::sha256_hex(Bytes{});
};
Bytes encode(const Driver &d) {
    Bytes out{'A','V','A','C','T','D','R','1'};
    for (auto n : {std::uint64_t(1), std::uint64_t(1), std::uint64_t(0), frame_limit, stall_limit,
                   d.next_frame, d.next_command, d.checks, d.months, d.last_month_frame,
                   d.random, d.steps, d.history, d.bakery, d.previous_task, d.next_task_month,
                   d.next_activity_frame, d.completed_activities, d.exhibition_count,
                   d.promoted_month, d.exhibition_month, d.exhibition_income,
                   d.accepted_tasks, d.departed_tasks, d.task_successes, d.terminal,
                   d.top_id, d.top_kind, d.top_raw, d.sound_count, std::uint64_t(d.pending)}) u64(out, n);
    for (auto n : d.date) u64(out, n);
    for (auto n : d.commands) u64(out, n);
    for (auto n : d.peaks) u64(out, n);
    u64(out, d.upgrade_pages.size()); for (auto n : d.upgrade_pages) u64(out, n);
    u64(out, 1); // 本轮一次性输出已消费。
    u64(out, d.producer_revision.size());
    out.insert(out.end(), d.producer_revision.begin(), d.producer_revision.end());
    out.insert(out.end(), d.sound_hash.begin(), d.sound_hash.end()); return out;
}
Driver decode(const Bytes &bytes) {
    require(bytes.size() >= 8 && std::string(bytes.begin(), bytes.begin() + 8) == "AVACTDR1", "driver magic");
    std::size_t at = 8;
    const auto number = [&] {
        require(at <= bytes.size() && bytes.size() - at >= 8, "driver truncated");
        std::uint64_t n{}; for (int i = 0; i < 8; ++i) n |= std::uint64_t(bytes[at++]) << (8 * i);
        return n;
    };
    require(number() == 1 && number() == 1 && number() == 0 && number() == frame_limit &&
                number() == stall_limit, "driver version/seed/speed/strategy");
    Driver d;
    for (auto *n : {&d.next_frame, &d.next_command, &d.checks, &d.months, &d.last_month_frame,
                    &d.random, &d.steps, &d.history, &d.bakery, &d.previous_task, &d.next_task_month,
                    &d.next_activity_frame, &d.completed_activities, &d.exhibition_count,
                    &d.promoted_month, &d.exhibition_month, &d.exhibition_income,
                    &d.accepted_tasks, &d.departed_tasks, &d.task_successes, &d.terminal,
                    &d.top_id, &d.top_kind, &d.top_raw, &d.sound_count}) *n = number();
    const auto pending = number(); require(pending <= std::uint64_t(Intent::leave), "driver intent");
    d.pending = static_cast<Intent>(pending);
    for (auto &n : d.date) n = number();
    for (auto &n : d.commands) n = number();
    for (auto &n : d.peaks) n = number();
    const auto count = number(); require(count <= 1024, "driver upgrade set budget");
    for (std::uint64_t i = 0; i < count; ++i) require(d.upgrade_pages.insert(number()).second, "driver duplicate upgrade");
    require(number() == 1, "driver unconsumed output");
    const auto producer_size = number();
    require(producer_size > 0 && producer_size <= 256 && producer_size <= bytes.size() - at,
            "driver producer size");
    d.producer_revision.assign(bytes.begin() + static_cast<std::ptrdiff_t>(at),
                               bytes.begin() + static_cast<std::ptrdiff_t>(at + producer_size));
    at += static_cast<std::size_t>(producer_size);
    require(d.producer_revision.find('\0') == std::string::npos && bytes.size() - at == 64,
            "driver producer or trailing data");
    d.sound_hash.assign(bytes.begin() + static_cast<std::ptrdiff_t>(at), bytes.end());
    require(d.sound_hash.find_first_not_of("0123456789abcdef") == std::string::npos && encode(d) == bytes,
            "driver noncanonical encoding");
    require(d.next_frame >= 1 && d.next_frame <= frame_limit + 1 && d.checks == d.next_frame - 1 &&
                d.next_command >= 1 && d.next_command <= 1024 * d.next_frame && d.commands[0] == 0 &&
                d.last_month_frame < d.next_frame && d.next_frame - 1 - d.last_month_frame <= stall_limit &&
                d.date[0] < 15 && d.date[1] < 12 && d.date[2] < 4 && d.date[3] < 10800 &&
                d.months + 3 == d.date[0] * 12 + d.date[1] && d.terminal <= 1 &&
                d.top_id > 0 && d.top_kind <= 4 && d.top_raw < 128 &&
                // next_task_month用原绝对月；months是相对开局月，+5含原+3及恢复间隔+2。
                d.next_activity_frame <= d.next_frame + 300 && d.next_task_month <= d.months + 5 &&
                d.departed_tasks <= d.accepted_tasks && d.exhibition_count <= d.completed_activities &&
                (d.promoted_month == absent || d.promoted_month <= d.months) &&
                (d.exhibition_month == absent || (d.promoted_month != absent &&
                                                 d.exhibition_month >= d.promoted_month && d.exhibition_month <= d.months)),
            "driver counters/calendar bounds");
    std::uint64_t sum{};
    for (auto n : d.commands) {
        require(n <= d.next_command - 1 - sum, "driver command count overflows declared total");
        sum += n;
    }
    require(sum == d.next_command - 1 && d.accepted_tasks <= sum && d.departed_tasks <= sum &&
                d.completed_activities <= sum && d.sound_count <= d.next_frame * 100,
            "driver input/output totals");
    return d;
}
const ref::WorldScriptPage *top(const StartupApplication &app) {
    if (!app.world()) return nullptr;
    const auto &pages = app.world()->state().scripts.pages;
    for (auto p = pages.rbegin(); p != pages.rend(); ++p) if (p->lifecycle != 4) return &*p;
    return nullptr;
}
std::array<std::uint64_t, 4> date(const StartupApplication &app) {
    const auto &c = app.world()->state().scene.calendar;
    require(ref::valid_world_calendar_state(c), "invalid calendar");
    return {std::uint64_t(c.year), std::uint64_t(c.month), std::uint64_t(c.subperiod), std::uint64_t(c.units)};
}
std::array<std::uint64_t, 12> resources(const StartupApplication &app) {
    const auto &s = app.world()->state(); const auto u = startup_world_resource_usage(s);
    return {u.live_actors, u.retired_actors, u.facilities, u.pages, u.page_payloads, u.effects,
            u.live_encounters, u.retired_encounters, u.retained_tasks, u.continuations,
            app.world()->checkpoints().size(), s.scene.world.world.ai.accounting.entries().size()};
}
// 月日志/终点只读诊断：复用原晋级纯查询的kind3/9与kind12计数，
// 只准备其实际读取字段，不复制全Session/历史，也不修改rank缓存或Driver身份。
std::string progress_json(const StartupApplication &app) {
    const auto &s = app.world()->state();
    ref::WorldCalendarTasksState query;
    query.finish.dungeon.world.facilities = s.scene.world.world.facilities;
    query.finish.task_progress.successes = s.task_progress.successes;
    query.facility_order = s.scene.world.facility_order;
    query.rank_terms = ref::fixed_calendar_task_rank_terms();
    query.rank = 1; // 仅选有两种计数的第二星查询模板，不是把当前村子升星。
    query.highest_month_income = s.maximum_income;
    query.popularity = s.popularity;
    query.events_held = s.events_held;
    const auto status = ref::prepare_world_rank_status(query);
    require(status.has_value(), "progress diagnostic rank query failed");
    std::optional<int> facilities, houses;
    const auto &terms = query.rank_terms.at(query.rank);
    for (std::size_t n = 0; n < terms.size(); ++n) {
        if (terms[n].type == 1) facilities = status->values[n];
        if (terms[n].type == 2) houses = status->values[n];
    }
    require(facilities && houses, "progress diagnostic missing count terms");
    std::ostringstream out;
    out << "{\"cash\":" << s.scene.world.world.ai.accounting.funds()
        << ",\"popularity\":" << s.popularity << ",\"maximum_income\":" << s.maximum_income
        << ",\"village_points\":" << s.village_points << ",\"events_held\":" << s.events_held
        << ",\"quarter_counter\":" << s.quarter_counter << ",\"task_successes\":" << s.task_progress.successes
        << ",\"facilities_kind3_9\":" << *facilities << ",\"houses_kind12\":" << *houses << '}';
    return out.str();
}
bool references(const StartupWorldRuntimeState &s) {
    const auto owns = [&](std::uint64_t id) { return std::any_of(s.scripts.pages.begin(), s.scripts.pages.end(),
                                                              [&](const auto &p) { return p.id == id; }); };
    const auto pages = [&](const auto &values) { return std::all_of(values.begin(), values.end(),
                                                                   [&](const auto &v) { return owns(v.first); }); };
    return pages(s.page_counters) && pages(s.page_phases) && pages(s.award_rankings) &&
        pages(s.activity_page_lists) && pages(s.task_page_lists) && pages(s.facility_page_bindings) &&
        pages(s.build_page_catalogs) && pages(s.human_page_catalogs) &&
        std::all_of(s.facility_upgrade_initialized.begin(), s.facility_upgrade_initialized.end(), owns) &&
        std::all_of(s.shops.begin(), s.shops.end(), [&](const auto &v) { return s.scene.world.world.facilities.count(v.first); }) &&
        std::all_of(s.shop_order.begin(), s.shop_order.end(), [&](auto id) { return s.shops.count(id); });
}
std::uint64_t income(const StartupWorldRuntimeState &s) {
    std::uint64_t total{};
    for (const auto &entry : s.scene.world.world.ai.accounting.entries())
        if (entry.second.category == ref::CashCategory::facilities && entry.second.direction == ref::CashDirection::income) {
            require(entry.second.amount >= 0 && std::uint64_t(entry.second.amount) <= absent - total, "income overflow");
            total += std::uint64_t(entry.second.amount);
        }
    return total;
}
std::uint64_t pending_upgrade(const StartupWorldRuntimeState &s) {
    for (auto id : s.scene.world.facility_order)
        if (s.scene.world.world.facility_uses.at(s.scene.world.world.facilities.at(id).placement.definition_id).upgrade_pending) return id;
    return 0;
}
// 52确认刚创建53时，框架尚未初始化其计数。只接受原52→53生成链，
// 不能把已初始化却缺字段、错误父页或任意life0页当成可等待的新页。
Intent activity53_plan(const StartupWorldRuntimeState &s, const ref::WorldScriptPage &page) {
    if (s.activity_pages_initialized.count(page.id)) {
        const auto view = inspect_startup_world_village_activity_page(s, page.id);
        require(view && view->raw == 53, "initialized activity53 payload missing");
        return view->counter >= 120 ? Intent::activity_complete : Intent::wait;
    }
    require(s.rules && page.kind == ref::WorldScriptPageKind::raw_page && page.legacy_page == 53 &&
                page.lifecycle == 0 && !s.activity_page_selections.count(page.id) &&
                !s.activity_page_scroll.count(page.id), "uninitialized activity53 lifecycle/payload");
    const auto current = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                      [&](const auto &p) { return p.id == page.id; });
    require(current != s.scripts.pages.end() && current != s.scripts.pages.begin() &&
                std::none_of(current + 1, s.scripts.pages.end(), [](const auto &p) { return p.lifecycle != 4; }),
            "new activity53 must be actual top page");
    const auto &offer = *(current - 1);
    const auto binding = s.activity_page_bindings.find(page.id);
    const auto old_binding = s.activity_page_bindings.find(offer.id);
    const auto parent = s.activity_page_parents.find(offer.id);
    require(offer.kind == ref::WorldScriptPageKind::raw_page && offer.legacy_page == 52 &&
                offer.lifecycle == 4 && s.activity_pages_initialized.count(offer.id) &&
                binding != s.activity_page_bindings.end() && old_binding != s.activity_page_bindings.end() &&
                binding->second == old_binding->second && parent != s.activity_page_parents.end(),
            "new activity53 requires its just-retired bound52");
    const auto owner = std::find_if(s.scripts.pages.begin(), current - 1,
                                    [&](const auto &p) { return p.id == parent->second; });
    const auto answer = s.activity_page_answers.find(parent->second);
    const auto list = s.activity_page_lists.find(parent->second);
    const auto selected = s.activity_page_selections.find(parent->second);
    require(owner != current - 1 && owner->kind == ref::WorldScriptPageKind::raw_page &&
                owner->legacy_page == 51 && owner->lifecycle != 4 &&
                s.activity_pages_initialized.count(owner->id) && answer != s.activity_page_answers.end() && answer->second == 0 &&
                list != s.activity_page_lists.end() && selected != s.activity_page_selections.end() && selected->second >= 0 &&
                static_cast<std::size_t>(selected->second) < list->second.size() && list->second[selected->second] == binding->second,
            "new activity53 requires successful matching51 answer");
    const auto definition = std::find_if(s.rules->activities.begin(), s.rules->activities.end(),
                                         [&](const auto &a) { return a.identity == binding->second; });
    const auto held = s.activity_counts.find(binding->second);
    require(definition != s.rules->activities.end() && definition->parameters[2] >= 0 &&
                definition->parameters[2] <= 6 && definition->parameters[2] != 4 &&
                held != s.activity_counts.end() && held->second > 0, "new activity53 active definition");
    return Intent::wait;
}
Intent plan(const StartupApplication &app, const Driver &d) {
    const auto *p = top(app); require(p, "no active page"); const auto &s = app.world()->state();
    if (d.terminal) return Intent::wait;
    if (p->kind == ref::WorldScriptPageKind::dialogue || p->kind == ref::WorldScriptPageKind::simple_message ||
        p->kind == ref::WorldScriptPageKind::newspaper) return Intent::acknowledge;
    if (p->kind == ref::WorldScriptPageKind::scene) {
        if (s.scene.scene_state != 0) return Intent::wait;
        if (pending_upgrade(s)) return Intent::upgrade;
        if (!d.bakery) {
            const auto quote = startup_world_build_quote(s, 35);
            return quote && s.scene.world.world.ai.accounting.funds() >= quote->construction_cost ? Intent::build : Intent::wait;
        }
        const int cost = s.rank >= 1 && !d.exhibition_count ? s.rules->activities.at(16).parameters[4] : 20;
        if (s.quarter_counter > 0 && s.village_points >= cost && d.next_frame >= d.next_activity_frame) return Intent::activities;
        const auto month = s.scene.calendar.year * 12 + s.scene.calendar.month;
        if (!s.active_task && s.rank == 0 && s.popularity < 300 && std::uint64_t(month) >= d.next_task_month &&
            std::any_of(s.task_order.begin(), s.task_order.end(), [&](auto id) {
                return s.rules->tasks.at(s.tasks.at(id).definition).recruitment_fee <= s.scene.world.world.ai.accounting.funds();
            })) return Intent::tasks;
        return Intent::wait;
    }
    require(p->kind == ref::WorldScriptPageKind::raw_page, "unknown page kind");
    switch (p->legacy_page) {
    case 16: case 24: case 56: case 57: case 97: case 98: return Intent::wait;
    case 51: return Intent::activity_select;
    case 52: case 54: return Intent::acknowledge;
    case 53: return activity53_plan(s, *p);
    case 22: return Intent::task_select;
    case 23: return Intent::task_confirm;
    case 25: return Intent::depart;
    case 28: {
        // 25→28的任务commit在返回前已写phase/prediction/acceleration，
        // 与活动52→53不同，没有合法的“缺phase等待初始化”窗口。
        const auto phase = s.page_phases.find(p->id);
        const auto counter = s.page_counters.find(p->id);
        require(phase != s.page_phases.end() && (phase->second == 0 || phase->second == 1) &&
                    counter != s.page_counters.end() && counter->second >= 0 &&
                    s.task_page_predictions.count(p->id) && s.task_page_acceleration.count(p->id),
                "task28 committed payload missing");
        return phase->second == 0 ? Intent::task_confirm : Intent::wait;
    }
    case 33: return Intent::deadline;
    case 81: return Intent::upgrade_confirm;
    case 48: return Intent::rank;
    case 87: return s.medal_count > 0 ? Intent::award : Intent::wait;
    case 83: return Intent::leave;
    // 已核任务成果：30确认快进/分阶段，31初始化统计再关闭，32只关闭摘要。
    // 奖励在真实收尾已提交；调用既有Owner确认，不能再次结算或直接退休页面。
    case 30: case 31: case 32: return Intent::acknowledge;
    // 99/100走现有consume_task_display确认：早确认不快进，>=40才关闭。
    // 初始化身份/100的E及F随机均由Owner管理，Driver不重建表或额外执行更新。
    case 99: case 100: return Intent::acknowledge;
    case 11: case 49: case 50: case 59: case 67: case 88: case 89: case 94: case 95: case 96:
        return Intent::acknowledge;
    default: throw std::runtime_error("active unknown raw=" + std::to_string(p->legacy_page) + " page=" + std::to_string(p->id));
    }
}
Metadata metadata(const Driver &d) {
    Metadata m; m.controller_id = controller; m.producer_revision = d.producer_revision;
    m.next_frame = d.next_frame; m.next_command = d.next_command; m.controller_state = encode(d); return m;
}
std::string validate(const StartupApplication &app, const Metadata &m) {
    try {
        require(m.controller_id == controller && m.extensions.empty(), "metadata identity");
        const auto d = decode(m.controller_state);
        require(m.producer_revision == d.producer_revision && m.next_frame == d.next_frame && m.next_command == d.next_command && app.error().empty() &&
                    app.mode() == StartupApplicationMode::logic && app.page() == StartupApplicationPage::world && app.world(), "application binding");
        const auto &s = app.world()->state(); const auto *p = top(app);
        const auto seed = ref::WorldRandomStream::from_java_seed(1).snapshot();
        require(app.handoff_random() && app.handoff_random()->cursor == 0 && app.handoff_random()->engine_state == seed.engine_state &&
                    !app.handoff_random()->tape_mode && app.handoff_random()->tape.empty() && s.scene.speed_setting == 0 &&
                    !s.scene.framework_paused && !s.scripts.executing_page && !app.has_pending_audio_requests() &&
                    s.sound_requests.empty() && !app.clear_page() && s.scene.random.draws() == d.random &&
                    s.simulation_steps == d.steps && app.world()->checkpoints().size() == d.history && date(app) == d.date &&
                    p && p->id == d.top_id && std::uint64_t(p->kind) == d.top_kind &&
                    std::uint64_t(p->kind == ref::WorldScriptPageKind::raw_page ? p->legacy_page : 0) == d.top_raw &&
                    plan(app, d) == d.pending && references(s), "precise world/page/policy/output binding");
        if (d.bakery) {
            const auto found = s.scene.world.world.facilities.find(d.bakery);
            require(found != s.scene.world.world.facilities.end() && found->second.placement.definition_id == 35, "bakery identity");
        }
        require((s.rank >= 1) == (d.promoted_month != absent) && (!d.previous_task || d.previous_task < s.next_task_identity) &&
                    s.task_progress.successes >= 0 && d.task_successes == std::uint64_t(s.task_progress.successes) &&
                    d.completed_activities <= std::uint64_t(s.events_held) &&
                    d.exhibition_count <= std::uint64_t(s.activity_counts.at(16)), "milestone relationships");
        for (auto id : d.upgrade_pages) require(id > 0 && id < s.scripts.next_page_id, "upgrade page identity");
        const auto sizes = resources(app);
        for (std::size_t i = 0; i < sizes.size(); ++i) require(d.peaks[i] >= sizes[i], "resource peak");
        if (d.terminal) require(d.bakery && s.rank >= 1 && !d.upgrade_pages.empty() && d.exhibition_count > 0 &&
                                   d.exhibition_month != absent && d.months > d.exhibition_month &&
                                   p->kind == ref::WorldScriptPageKind::scene && income(s) > d.exhibition_income,
                               "terminal requires real post-rank business");
        return {};
    } catch (const std::exception &e) { return e.what(); }
}
void consume_audio(StartupApplication &app, Driver &d, Round &round, std::uint64_t frame) {
    round.sounds = app.take_audio_requests();
    Bytes bytes(d.sound_hash.begin(), d.sound_hash.end()); u64(bytes, frame); u64(bytes, round.sounds.size());
    for (const auto &sound : round.sounds) {
        require(std::uint64_t(sound.operation) <= 2 && sound.id >= 0 && sound.id < 26, "audio range");
        u64(bytes, std::uint64_t(sound.operation)); u64(bytes, std::uint64_t(sound.id));
    }
    require(app.take_sound_requests().empty(), "audio consumed twice");
    d.sound_hash = dungeon_village_tools::sha256_hex(bytes); d.sound_count += round.sounds.size();
}
void observe(StartupApplication &app, Driver &d) {
    const auto &s = app.world()->state(); const auto *p = top(app); require(p, "missing page after input");
    d.date = date(app); d.months = d.date[0] * 12 + d.date[1] - 3;
    d.random = s.scene.random.draws(); d.steps = s.simulation_steps; d.history = app.world()->checkpoints().size();
    require(s.task_progress.successes >= 0, "negative task successes");
    d.task_successes = std::uint64_t(s.task_progress.successes);
    d.top_id = p->id; d.top_kind = std::uint64_t(p->kind);
    d.top_raw = p->kind == ref::WorldScriptPageKind::raw_page ? p->legacy_page : 0;
    const auto size = resources(app);
    for (std::size_t i = 0; i < size.size(); ++i) d.peaks[i] = std::max(d.peaks[i], size[i]);
    d.pending = plan(app, d);
}
Round step(StartupApplication &app, Driver &d) {
    good(validate(app, metadata(d)));
    require(!d.terminal && d.next_frame <= frame_limit, "absolute active frame bound");
    const auto frame = d.next_frame, old_random = d.random, old_steps = d.steps, old_history = d.history;
    const auto old_date = d.date;
    const auto old_month = d.months;
    good(app.update());
    const auto &state = app.world()->state();
    if (d.previous_task && !state.active_task) d.next_task_month = state.scene.calendar.year * 12 + state.scene.calendar.month + 2;
    d.previous_task = state.active_task.value_or(0);
    const auto action = plan(app, d); const auto page = *top(app);
    Round round;
    const auto record = [&](Input input, std::uint64_t id = 0, std::int64_t a = 0, std::int64_t b = 0, std::int64_t c = 0) {
        require(id <= std::uint64_t(INT64_MAX), "input page identity overflow");
        round.commands.push_back({std::int64_t(input), static_cast<std::int64_t>(id), a, b, c});
        if (input != Input::wait) { ++d.commands[std::size_t(input)]; ++d.next_command; }
    };
    const auto acknowledge = [&](std::uint64_t id) { good(app.acknowledge_page(id)); record(Input::acknowledge, id); };
    const auto task = [&](StartupWorldTaskAction command, int selection = 0) {
        const auto result = app.act_task_page(page.id, command, selection); good(result.error);
        require(result.denial == ref::TaskCommandDenial::none, "task business denial");
        d.accepted_tasks += result.accepted ? 1 : 0; d.departed_tasks += result.departed ? 1 : 0;
        record(Input::task, page.id, static_cast<int>(command), selection);
    };
    switch (action) {
    case Intent::wait: record(Input::wait); break;
    case Intent::acknowledge: acknowledge(page.id); break;
    case Intent::build: {
        good(app.open_build_menu()); record(Input::open_build);
        const auto menu = top(app)->id;
        const auto selected = app.select_build_menu(menu, 35); good(selected.error);
        require(selected.denial == StartupBuildDenial::none, "bakery unavailable in actual catalogue");
        record(Input::select_build, menu, 35);
        std::vector<std::pair<ref::Position, int>> cells;
        {
            // confirm_build会替换应用world；先复制有限坐标候选，后续输入不保留旧map引用。
            const auto &map = app.world()->state().scene.world.world.map;
            for (int y = 0; y < map.height; ++y) for (int x = 0; x < map.width; ++x) {
                const auto &cell = map.cells.at(y * map.width + x);
                if (cell.facility || cell.legacy_state != 4) continue;
                int nearest = map.width + map.height;
                for (int ry = 0; ry < map.height; ++ry) for (int rx = 0; rx < map.width; ++rx)
                    if (map.cells.at(ry * map.width + rx).legacy_state == 3)
                        nearest = std::min(nearest, std::abs(x - rx) + std::abs(y - ry));
                cells.push_back({{x, y}, nearest});
            }
        }
        std::stable_sort(cells.begin(), cells.end(), [](const auto &a, const auto &b) { return a.second < b.second; });
        for (const auto &cell : cells) {
            const auto placed = app.confirm_build(cell.first, ref::FacilityOrientation::first); good(placed.error);
            record(Input::place_build, 0, cell.first.x, cell.first.y, static_cast<int>(ref::FacilityOrientation::first));
            if (placed.created) { require(placed.denial == StartupBuildDenial::none, "created with denial"); d.bakery = *placed.created; break; }
            require(placed.denial == StartupBuildDenial::outside_map || placed.denial == StartupBuildDenial::outside_town ||
                        placed.denial == StartupBuildDenial::occupied, "unexpected paid-placement denial");
        }
        require(d.bakery != 0, "no legal bakery placement"); good(app.cancel_build()); record(Input::cancel_build); break;
    }
    case Intent::upgrade: {
        const auto id = pending_upgrade(state); require(id != 0, "upgrade disappeared");
        good(app.open_facility_page(id)); record(Input::open_facility, 0, static_cast<std::int64_t>(id)); break;
    }
    case Intent::activities:
        good(app.open_village_activities()); record(Input::open_activities); d.next_activity_frame = frame + 300; break;
    case Intent::activity_select: {
        const auto view = inspect_startup_world_village_activity_page(state, page.id); require(view.has_value(), "activity list");
        int selection = -1;
        for (int n = 0; n < static_cast<int>(view->entries.size()); ++n) {
            const auto &a = state.rules->activities.at(view->entries[n]);
            if (state.rank >= 1 && !d.exhibition_count && a.identity != 16) continue;
            if (a.parameters[2] <= 2 && a.parameters[4] <= state.village_points && state.quarter_counter > 0) {
                selection = n; if (a.identity == 16) break;
            }
        }
        if (selection < 0) { good(app.cancel_page(page.id)); record(Input::cancel_page, page.id); }
        else {
            good(app.act_village_activity_page(page.id, StartupVillageActivityAction::select, selection));
            record(Input::select_activity, page.id, selection); acknowledge(page.id);
        }
        break;
    }
    case Intent::activity_complete: {
        const auto view = inspect_startup_world_village_activity_page(state, page.id);
        require(view && view->activity && view->counter >= 120, "activity real completion gate");
        const int id = *view->activity; const auto before = state.activity_counts.at(id);
        acknowledge(page.id); const auto &after = app.world()->state();
        // m在52已计数，本动作只确认53；不以计数再增加一次作为错误oracle。
        require(after.activity_counts.at(id) == before, "activity53 must not charge52 twice");
        ++d.completed_activities;
        if (id == 16) {
            ++d.exhibition_count;
            if (d.exhibition_month == absent) {
                d.exhibition_month = after.scene.calendar.year * 12 + after.scene.calendar.month - 3;
                d.exhibition_income = income(after);
            }
        }
        break;
    }
    case Intent::tasks: good(app.open_task_menu()); record(Input::open_tasks); break;
    case Intent::task_select: {
        const auto &list = state.task_page_lists.at(page.id);
        const auto affordable = std::find_if(list.begin(), list.end(), [&](auto id) {
            return state.rules->tasks.at(state.tasks.at(id).definition).recruitment_fee <= state.scene.world.world.ai.accounting.funds();
        });
        require(affordable != list.end(), "no affordable listed task");
        task(StartupWorldTaskAction::confirm, static_cast<int>(affordable - list.begin())); break;
    }
    case Intent::task_confirm: task(StartupWorldTaskAction::confirm); break;
    case Intent::depart: task(StartupWorldTaskAction::depart); break;
    case Intent::deadline: task(StartupWorldTaskAction::confirm, state.scene.world.world.ai.accounting.funds() >= page.legacy_f ? 0 : 1); break;
    case Intent::upgrade_confirm: {
        const auto id = state.facility_page_bindings.at(page.id);
        require(state.scene.world.world.facility_uses.at(state.scene.world.world.facilities.at(id).placement.definition_id).level > 1,
                "upgrade must be real before presentation");
        d.upgrade_pages.insert(page.id); acknowledge(page.id); break;
    }
    case Intent::rank: {
        good(app.act_rank_page(page.id)); record(Input::rank, page.id);
        const auto &after = app.world()->state();
        if (after.rank >= 1 && d.promoted_month == absent) {
            require(d.bakery && after.popularity >= 300 && after.maximum_income >= 5000 && after.events_held >= 2 &&
                        after.scripts.activities.at(16).status == 1, "first star original conditions and unlock");
            d.promoted_month = after.scene.calendar.year * 12 + after.scene.calendar.month - 3;
        }
        break;
    }
    case Intent::award:
        good(app.act_award_page(page.id, ref::WorldAwardAction::request_award, 0));
        record(Input::award, page.id, static_cast<int>(ref::WorldAwardAction::request_award));
        good(app.act_award_page(page.id, ref::WorldAwardAction::confirm_award));
        record(Input::award, page.id, static_cast<int>(ref::WorldAwardAction::confirm_award)); break;
    case Intent::leave: good(app.leave_commerce_page(page.id)); record(Input::leave_commerce, page.id); break;
    }
    consume_audio(app, d, round, frame);
    const auto &after = app.world()->state();
    const auto now = date(app); require(now >= old_date, "calendar rewound");
    const auto month = now[0] * 12 + now[1] - 3;
    require(month == old_month || month == old_month + 1, "month transition skipped");
    if (month != old_month) d.last_month_frame = frame;
    require(frame - d.last_month_frame <= stall_limit && after.scene.random.draws() >= old_random &&
                after.simulation_steps >= old_steps && app.world()->checkpoints().size() >= old_history,
            "calendar stalled or committed history rewound");
    if (d.exhibition_month != absent && month > d.exhibition_month && !d.upgrade_pages.empty() &&
        after.scene.calendar.units >= 27 && top(app)->kind == ref::WorldScriptPageKind::scene &&
        after.activity_pages_initialized.empty() && income(after) > d.exhibition_income) d.terminal = 1;
    ++d.next_frame; ++d.checks; observe(app, d); good(validate(app, metadata(d))); return round;
}
Driver initial(StartupApplication &app) {
    Driver d; observe(app, d);
    require(d.date[0] == 0 && d.date[1] == 3 && !d.bakery && app.world()->state().rank == 0, "real new game boundary");
    Round round; consume_audio(app, d, round, 0);
    require(round.sounds.size() == 2 && round.sounds[0].operation == StartupAudioOperation::replace_bgm && round.sounds[0].id == 0 &&
                round.sounds[1].operation == StartupAudioOperation::replace_bgm && round.sounds[1].id == 1, "actual titleB0/worldG1");
    good(validate(app, metadata(d))); return d;
}
std::string trace_line(const StartupApplication &app, const Driver &d, const Round &round, const fs::path &system) {
    const auto bytes = encode(d); std::ostringstream out;
    out << "{\"frame\":" << d.next_frame - 1 << ",\"next_frame\":" << d.next_frame << ",\"next_command\":" << d.next_command << ",\"commands\":[";
    for (std::size_t n = 0; n < round.commands.size(); ++n) {
        out << (n ? ",[" : "["); for (std::size_t i = 0; i < 5; ++i) out << (i ? "," : "") << round.commands[n][i]; out << ']';
    }
    out << "],\"driver\":\"" << hex(bytes) << "\",\"driver_digest\":\"" << dungeon_village_tools::sha256_hex(bytes)
        << "\",\"digest\":\"" << startup_application_replay_digest(app, metadata(d), validate)
        << "\",\"system_digest\":\"" << dungeon_village_tools::sha256_hex(read_file(system)) << "\",\"sounds\":[";
    for (std::size_t n = 0; n < round.sounds.size(); ++n)
        out << (n ? "," : "") << '[' << static_cast<int>(round.sounds[n].operation) << ',' << round.sounds[n].id << ']';
    out << "]}\n"; return out.str();
}
std::uint64_t option_number(const std::map<std::string, std::string> &options, const std::string &key, std::uint64_t fallback) {
    const auto found = options.find(key); if (found == options.end()) return fallback;
    require(!found->second.empty() && found->second.find_first_not_of("0123456789") == std::string::npos, "invalid number " + key);
    std::size_t used{}; const auto value = std::stoull(found->second, &used);
    require(used == found->second.size() && value <= frame_limit, "number bound " + key); return value;
}
} // namespace

int run_startup_application_active_replay_cli(int argc, const char **argv) {
    require(argc > 1 && std::string(argv[1]) == controller, "CLI identity");
    const std::array<std::string, 9> keys{"--work-dir", "--trace-file", "--stop-at", "--load-file", "--save-file", "--save-at", "--tail-after-save", "--trace-from", "--producer-revision"};
    std::map<std::string, std::string> options;
    for (int i = 2; i < argc; i += 2) {
        require(i + 1 < argc && std::find(keys.begin(), keys.end(), argv[i]) != keys.end(), "unknown/missing CLI option");
        require(options.emplace(argv[i], argv[i + 1]).second && !options.at(argv[i]).empty(), "repeated/empty option");
    }
    require(options.count("--work-dir") && options.count("--trace-file") && options.count("--stop-at"), "required CLI paths/bound");
    const bool load = options.count("--load-file"), save = options.count("--save-file");
    const auto stop = option_number(options, "--stop-at", 0), save_at = option_number(options, "--save-at", 0);
    const auto tail = option_number(options, "--tail-after-save", save ? 20 : 0), trace_from = option_number(options, "--trace-from", 0);
    require(stop > 0 && save == bool(save_at) && (!save || (tail > 0 && tail <= 1000 && save_at + tail <= stop)) &&
                (!options.count("--tail-after-save") || save) && (!save || !trace_from) &&
                (!options.count("--trace-from") || (trace_from > 0 && trace_from <= stop)) &&
                (load || save || trace_from || stop <= 1000), "CLI observer relationships");
    const auto producer = options.count("--producer-revision") ? options.at("--producer-revision") : "unspecified";
    require(!producer.empty() && producer.size() <= 256 && producer.find('\0') == std::string::npos,
            "producer revision identity");
    const auto root = fs::absolute(options.at("--work-dir")), trace = fs::absolute(options.at("--trace-file"));
    const auto live = root / "application"; const StartupApplicationPaths unused{root / "unused-current"};
    const auto source = load ? fs::absolute(options.at("--load-file")) : fs::path{};
    const auto capture = save ? fs::absolute(options.at("--save-file")) : fs::path{};
    std::vector<fs::path> protected_paths{trace}; if (load) protected_paths.push_back(source); if (save) protected_paths.push_back(capture);
    auto restore_protected = std::vector<fs::path>{trace}; if (save) restore_protected.push_back(capture);
    (void)persistence_detail::prepare_replay_capture_paths(root, root / ".active-probe", unused, protected_paths);
    auto trace_protected = protected_paths; trace_protected.erase(trace_protected.begin());
    const auto trace_target = persistence_detail::prepare_replay_capture_paths(root.parent_path(), trace, unused, trace_protected);
    if (load) (void)persistence_detail::prepare_replay_restore_paths(live, source, unused, restore_protected);
    if (save) (void)persistence_detail::prepare_replay_capture_paths(root.parent_path(), capture, unused,
        load ? std::vector<fs::path>{trace, source} : std::vector<fs::path>{trace});
    require(fs::create_directory(load ? unused.root : live), "exclusive application directory");
    StartupApplication app(load ? unused : StartupApplicationPaths{live}, ref::WorldRandomStream::from_java_seed(1)); good(app.error());
    Driver d; double restore_seconds{};
    if (load) {
        Metadata m; const auto started = Clock::now();
        const auto loader = [&](const StartupApplication &a, const Metadata &metadata) {
            const auto error = validate(a, metadata); if (!error.empty()) return error;
            const auto driver = decode(metadata.controller_state);
            if (driver.terminal || driver.next_frame > stop || (save && save_at < driver.next_frame))
                return std::string("active requested boundary already passed");
            if (!save && !trace_from && stop - driver.next_frame + 1 > 1000)
                return std::string("active long restore requires bounded trace-from");
            return std::string{};
        };
        good(restore_startup_application_replay(source, live, controller, app, m, loader, restore_protected));
        restore_seconds = std::chrono::duration<double>(Clock::now() - started).count(); d = decode(m.controller_state);
    } else { good(app.request_new_game(0)); good(app.start_game()); d = initial(app); }
    d.producer_revision = producer; // 源先按原身份完整验证，本次新证据登记当前生成实现。
    good(validate(app, metadata(d)));
    require(d.next_frame <= stop && (!save || save_at >= d.next_frame), "observer before source");
    const auto first = d.next_frame; std::ostringstream output;
    std::uint64_t captured{}, capture_rank{}, rows{}; double capture_seconds{}; std::size_t bytes{};
    while (d.next_frame <= stop && !d.terminal) {
        const auto previous_month = d.months; Round round;
        try { round = step(app, d); }
        catch (const std::exception &e) { throw std::runtime_error("active frame=" + std::to_string(d.next_frame) + " months=" + std::to_string(d.months) + ": " + e.what()); }
        const auto frame = d.next_frame - 1;
        const auto begin = save ? (captured ? captured + 1 : frame_limit + 1) : trace_from ? trace_from : first;
        if (frame >= begin) {
            const auto line = trace_line(app, d, round, live / "system.avr");
            require(line.size() <= trace_budget - bytes, "trace 8MiB output budget"); output << line; bytes += line.size(); ++rows;
        }
        if (previous_month != d.months)
            std::cout << "application-active-month months=" << d.months << " frame=" << frame << " rank=" << app.world()->state().rank
                      << " history=" << d.history << " random=" << d.random
                      << " progress=" << progress_json(app) << std::endl;
        if (save && frame == save_at) {
            const auto started = Clock::now();
            good(save_startup_application_replay(root.parent_path(), capture, app, metadata(d), validate,
                load ? std::vector<fs::path>{trace, source} : std::vector<fs::path>{trace}));
            capture_seconds = std::chrono::duration<double>(Clock::now() - started).count(); captured = frame; capture_rank = app.world()->state().rank;
            std::cout << "application-active-capture frame=" << frame << " next_frame=" << d.next_frame << " rank=" << capture_rank
                      << " bytes=" << fs::file_size(capture) << std::endl;
        }
        if (captured && frame >= captured + tail) break;
    }
    require(!save || (captured && d.next_frame - 1 >= captured + tail), "capture/tail boundary not completed");
    const auto text = output.str();
    (void)persistence_detail::prepare_replay_capture_paths(root.parent_path(), trace, StartupApplicationPaths{live}, trace_protected);
    persistence_detail::create_save_file(trace_target.container, Bytes(text.begin(), text.end()));
    const auto size = resources(app);
    std::cout << "application-active-summary {\"controller\":\"" << controller << "\",\"capture_frame\":" << (captured ? std::to_string(captured) : "null")
              << ",\"capture_rank\":" << (captured ? std::to_string(capture_rank) : "null") << ",\"completed_frame\":" << d.next_frame - 1
              << ",\"next_frame\":" << d.next_frame << ",\"next_command\":" << d.next_command << ",\"rank\":" << app.world()->state().rank
              << ",\"months\":" << d.months << ",\"date\":[" << d.date[0] << ',' << d.date[1] << ',' << d.date[2] << ',' << d.date[3]
              << "],\"digest\":\"" << startup_application_replay_digest(app, metadata(d), validate) << "\",\"resources\":[";
    for (std::size_t n = 0; n < size.size(); ++n) std::cout << (n ? "," : "") << size[n];
    std::cout << "],\"peaks\":["; for (std::size_t n = 0; n < d.peaks.size(); ++n) std::cout << (n ? "," : "") << d.peaks[n];
    std::cout << "],\"sound_count\":" << d.sound_count << ",\"sound_hash\":\"" << d.sound_hash << "\",\"random\":" << d.random
              << ",\"phase\":" << d.terminal << ",\"terminal\":" << (d.terminal ? "true" : "false")
              << ",\"accepted_tasks\":" << d.accepted_tasks << ",\"departed_tasks\":" << d.departed_tasks
              << ",\"task_successes\":" << d.task_successes << ",\"completed_activities\":" << d.completed_activities
              << ",\"upgrades\":" << d.upgrade_pages.size() << ",\"trace_rows\":" << rows
              << ",\"capture_seconds\":" << capture_seconds << ",\"restore_seconds\":" << restore_seconds
              << ",\"progress\":" << progress_json(app) << "}\n";
    return 0;
}

int run_startup_application_active_driver_checks(const fs::path &parent) {
    // 每次只拥有自己独占创建的目录；旧失败现场保留，不能为重跑删除固定名称目录。
    const auto owner_parent = fs::canonical(parent);
    fs::path root;
    const auto stamp = std::to_string(Clock::now().time_since_epoch().count());
    for (int attempt = 0; attempt < 32; ++attempt) {
        const auto candidate = owner_parent / ("active-driver-contract-" + stamp + "-" + std::to_string(attempt));
        std::error_code error;
        if (fs::create_directory(candidate, error)) { root = candidate; break; }
        require(!error || error == std::errc::file_exists, "cannot create driver check directory: " + error.message());
    }
    require(!root.empty(), "exclusive driver check directory exhausted");
    try {
    StartupApplication app({root}, ref::WorldRandomStream::from_java_seed(1));
    good(app.error()); good(app.request_new_game(0)); good(app.start_game());
    const auto d = initial(app); const auto before = startup_application_replay_digest(app, metadata(d), validate);
    int checks{};
    const auto reject = [&](Metadata m, const char *reason) {
        require(!validate(app, m).empty(), reason); ++checks;
        require(startup_application_replay_digest(app, metadata(d), validate) == before, "bad driver changed application"); ++checks;
    };
    auto m = metadata(d); m.controller_state.pop_back(); reject(m, "truncated driver accepted");
    auto bad = d; ++bad.next_frame; reject(metadata(bad), "wrong next frame accepted");
    bad = d; bad.top_raw = 128; reject(metadata(bad), "invalid page accepted");
    bad = d; bad.bakery = app.world()->state().next_facility_identity; reject(metadata(bad), "missing bakery accepted");
    bad = d; bad.pending = d.pending == Intent::wait ? Intent::tasks : Intent::wait;
    reject(metadata(bad), "wrong pending policy accepted");
    bad = d; bad.commands[1] = UINT64_MAX; reject(metadata(bad), "overflowing command count accepted");
    bad = d; bad.commands[std::size_t(Input::leave_commerce)] = 1;
    reject(metadata(bad), "commerce-leave count beyond declared next command accepted");
    m = metadata(d); ++m.next_command; reject(m, "metadata next command accepted");
    m = metadata(d); m.producer_revision = "different-source"; reject(m, "metadata producer accepted");
    // 最小生命周期条件：只给合法20点/3次，所有51/52/53页均由真实Owner命令产生。
    // 本夹具不作为自然经营前缀，不等待数月积点，也不直接构造测试页。
    auto state = app.world()->state(); state.village_points = 20; state.quarter_counter = 3;
    const auto state_top = [](const StartupWorldRuntimeState &s) -> const ref::WorldScriptPage * {
        for (auto p = s.scripts.pages.rbegin(); p != s.scripts.pages.rend(); ++p)
            if (p->lifecycle != 4) return &*p;
        return nullptr;
    };
    const auto tick = [&] {
        auto result = prepare_startup_world_runtime(state);
        require(result.candidate.has_value(), "activity lifecycle condition update failed");
        state = std::move(*result.candidate); state.sound_requests.clear();
    };
    require(open_startup_world_village_activities(state) == StartupWorldRuntimeError::none, "real open51");
    const auto list_page = state_top(state)->id;
    bool ready{};
    for (int n = 0; n < 64; ++n) {
        const auto *page = state_top(state); require(page, "activity introduction lost page");
        if (page->id == list_page && state.activity_pages_initialized.count(list_page)) { ready = true; break; }
        if (page->id != list_page)
            require(acknowledge_startup_world_runtime_page(state, page->id) == StartupWorldRuntimeError::none,
                    "real introduction confirmation");
        tick();
    }
    require(ready, "real51 initialization and introduction bounded");
    const auto view = inspect_startup_world_village_activity_page(state, list_page);
    require(view.has_value(), "initialized51 inspect");
    const auto chosen = std::find(view->entries.begin(), view->entries.end(), 23);
    require(chosen != view->entries.end(), "initial activity23 present");
    require(act_startup_world_village_activity_page(state, list_page, StartupVillageActivityAction::select,
                static_cast<int>(chosen - view->entries.begin())) == StartupWorldRuntimeError::none &&
            acknowledge_startup_world_runtime_page(state, list_page) == StartupWorldRuntimeError::none,
            "real51 selection opens52");
    const auto offer = state_top(state)->id; tick();
    require(state_top(state)->id == offer && state_top(state)->legacy_page == 52 &&
                acknowledge_startup_world_runtime_page(state, offer) == StartupWorldRuntimeError::none,
            "real52 confirmation creates new53");
    const auto pending = *state_top(state);
    require(pending.legacy_page == 53 && !state.activity_pages_initialized.count(pending.id) &&
                activity53_plan(state, pending) == Intent::wait, "new53 waits without missing-payload failure"); ++checks;
    const auto reject_state = [&](StartupWorldRuntimeState broken, const char *reason) {
        bool refused{};
        try { (void)activity53_plan(broken, *state_top(broken)); } catch (const std::exception &) { refused = true; }
        require(refused, reason); ++checks;
    };
    auto broken = state; broken.activity_page_bindings.erase(pending.id);
    reject_state(std::move(broken), "new53 missing binding accepted");
    broken = state; broken.activity_page_parents.at(offer) = pending.id;
    reject_state(std::move(broken), "new53 invalid parent accepted");
    tick();
    require(state.activity_pages_initialized.count(pending.id) &&
                inspect_startup_world_village_activity_page(state, pending.id).has_value() &&
                activity53_plan(state, *state_top(state)) == Intent::wait,
            "next real update initializes53 payload"); ++checks;
    broken = state; broken.page_counters.erase(pending.id);
    reject_state(std::move(broken), "initialized53 missing counter disguised as waiting");
    require(startup_application_replay_digest(app, metadata(d), validate) == before,
            "lifecycle condition never installs into natural application"); ++checks;
    require(!fs::is_symlink(fs::symlink_status(root)) && fs::canonical(root).parent_path() == owner_parent,
            "driver check cleanup target changed");
    fs::remove_all(root); // 仅全部成功后回收本轮新建树；不触碰任何旧目录。
    return checks;
    } catch (...) {
        std::cerr << "active driver check failed; retained evidence: " << root.u8string() << '\n';
        throw;
    }
}
