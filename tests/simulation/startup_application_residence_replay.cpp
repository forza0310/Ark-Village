#include "startup_application_residence_replay.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include "ark/simulation/actors/rules/human_management.hpp"
#include "ark/assets/sha256.hpp"
#include "../../src/simulation/application/startup_application_replay_paths.hpp"
#include "startup_application_second_star_replay.hpp"
#include "../../src/simulation/persistence/startup_world_file_io.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace {
using namespace ark::simulation;
namespace ref = ark::simulation::rules;
namespace base = active_replay_support;
namespace shared = second_star_replay_support;
namespace fs = std::filesystem;
using Metadata = StartupApplicationReplayMetadata;
using Bytes = shared::Bytes;
using Round = shared::Round;
using Clock = std::chrono::steady_clock;
constexpr auto controller = "application-active-residence-v1";
constexpr auto source_hash = "dc30d31a83c021d6af6ac66fbb31fbb40ea01e49a2679c6823f9567724a3f8f6";
constexpr auto certificate_hash =
    "4dcb2135d5ee4045b7a176df31b124af3fd04ab3121031a25469372ee27889ad";
constexpr auto origin_digest = "62aaa3970f616e4af63c8cf4c12685b0e510aee9c70f7b84ed9553068063ba25";
constexpr auto origin_files = "ece8dc44f2f2b910d2a9e8498e5f5ead021c676ac61418a7c59b90ebeff6028d";
constexpr auto history_hash = "e6452b6d5b7ab9d9124ffde656144f1f4780c0a19538eb5f1c07a74b32978218";
constexpr std::uint64_t first_frame = 34490, frame_limit = 180000, absent = UINT64_MAX,
                        stall_limit = 100000;
constexpr std::size_t trace_budget = 8U * 1024U * 1024U;
enum class Intent : std::uint64_t {
    wait,
    acknowledge,
    task_confirm,
    task_depart,
    deadline,
    award,
    leave,
    choose_human,
    open_human,
    gifts,
    choose_gift,
    confirm_gift,
    close_human,
    build_recruitment,
    open_recruitment,
    residence_list,
    admit,
    build_business,
    upgrade
};
enum class Input : std::uint64_t {
    wait,
    acknowledge,
    task,
    award,
    leave,
    open_build,
    select_build,
    confirm_build,
    cancel_build,
    open_facility,
    facility,
    residence,
    open_human,
    human,
    count
};
void require(bool ok, const std::string &why) {
    if (!ok)
        throw std::runtime_error("residence: " + why);
}
void good(const std::string &error) { require(error.empty(), error); }
std::string hash(const Bytes &b) { return ark::assets::sha256_hex(b); }
bool valid_hash(const std::string &s) {
    return s.size() == 64 && s.find_first_not_of("0123456789abcdef") == s.npos;
}
std::string hex(const Bytes &b) {
    std::string s;
    constexpr auto h = "0123456789abcdef";
    for (auto n : b) {
        s += h[n >> 4];
        s += h[n & 15];
    }
    return s;
}
void number(Bytes &b, std::uint64_t n) {
    for (int i = 0; i < 8; ++i)
        b.push_back(static_cast<std::uint8_t>(n >> (8 * i)));
}
void blob(Bytes &b, const Bytes &v) {
    number(b, v.size());
    b.insert(b.end(), v.begin(), v.end());
}
void string(Bytes &b, const std::string &s) { blob(b, Bytes(s.begin(), s.end())); }
struct Reader {
    const Bytes &b;
    std::size_t at{};
    std::uint64_t number() {
        require(at <= b.size() && b.size() - at >= 8, "truncated driver");
        std::uint64_t n{};
        for (int i = 0; i < 8; ++i)
            n |= std::uint64_t(b[at++]) << (8 * i);
        return n;
    }
    Bytes blob(std::size_t bound) {
        const auto n = number();
        require(n <= bound && n <= b.size() - at, "driver allocation bound");
        Bytes v(b.begin() + static_cast<std::ptrdiff_t>(at),
                b.begin() + static_cast<std::ptrdiff_t>(at + n));
        at += n;
        return v;
    }
    std::string string(std::size_t bound) {
        const auto v = blob(bound);
        std::string s(v.begin(), v.end());
        require(s.find('\0') == s.npos, "nul driver text");
        return s;
    }
};
struct Home {
    std::uint64_t human{}, instance{}, completed_frame{}, retired_recruitment{};
};
// 所有字段均为真实跨轮计划、提交回执或只读观测；不持有第二份业务世界。
struct Driver {
    Metadata origin;
    std::string producer{"unspecified"}, files_digest;
    std::uint64_t next_frame{first_frame}, next_command{1}, checks{}, human{absent}, recruitment{},
        home{}, business{};
    std::uint64_t human_page{}, gift_parent{}, gift_child{}, gift_pending{}, gift_slot{},
        gift_item{};
    std::uint64_t gift_stock{}, gift_satisfaction{}, gift_charge{}, gift_month{}, gift_expense{};
    std::uint64_t gifts{}, gift_cash{}, gift_inventory{}, recruitments{}, admissions{},
        businesses{};
    std::uint64_t previous_task{}, next_task_month{}, task_departed{}, task_successes{};
    std::uint64_t random{}, steps{}, history{}, months{}, last_month_frame{first_frame - 1},
        top_id{}, top_kind{}, top_raw{}, sound_count{};
    Intent pending{Intent::wait};
    std::array<std::uint64_t, 4> date{};
    std::array<std::uint64_t, static_cast<std::size_t>(Input::count)> commands{};
    std::array<std::uint64_t, 12> peaks{};
    std::vector<Home> homes;
    std::string sound_hash{hash({})};
};
Bytes encode(const Driver &d) {
    Bytes b{'A', 'V', 'R', 'E', 'S', 'D', 'R', '1'};
    for (auto n : {std::uint64_t(1), std::uint64_t(1), std::uint64_t(0), frame_limit, stall_limit,
                   std::uint64_t(4), std::uint64_t(10)})
        number(b, n);
    for (auto s : {source_hash, certificate_hash, origin_digest, history_hash})
        string(b, s);
    string(b, d.origin.controller_id);
    string(b, d.origin.producer_revision);
    number(b, d.origin.next_frame);
    number(b, d.origin.next_command);
    blob(b, d.origin.controller_state);
    string(b, d.producer);
    string(b, d.files_digest);
    for (auto n : {d.next_frame,
                   d.next_command,
                   d.checks,
                   d.human,
                   d.recruitment,
                   d.home,
                   d.business,
                   d.human_page,
                   d.gift_parent,
                   d.gift_child,
                   d.gift_pending,
                   d.gift_slot,
                   d.gift_item,
                   d.gift_stock,
                   d.gift_satisfaction,
                   d.gift_charge,
                   d.gift_month,
                   d.gift_expense,
                   d.gifts,
                   d.gift_cash,
                   d.gift_inventory,
                   d.recruitments,
                   d.admissions,
                   d.businesses,
                   d.previous_task,
                   d.next_task_month,
                   d.task_departed,
                   d.task_successes,
                   d.random,
                   d.steps,
                   d.history,
                   d.months,
                   d.last_month_frame,
                   d.top_id,
                   d.top_kind,
                   d.top_raw,
                   d.sound_count,
                   std::uint64_t(d.pending)})
        number(b, n);
    for (auto n : d.date)
        number(b, n);
    for (auto n : d.commands)
        number(b, n);
    for (auto n : d.peaks)
        number(b, n);
    number(b, d.homes.size());
    for (const auto &h : d.homes)
        for (auto n : {h.human, h.instance, h.completed_frame, h.retired_recruitment})
            number(b, n);
    number(b, 1);
    string(b, d.sound_hash);
    return b;
}
Driver decode(const Bytes &b) {
    require(b.size() >= 8 && std::string(b.begin(), b.begin() + 8) == "AVRESDR1", "driver magic");
    Reader r{b, 8};
    require(r.number() == 1 && r.number() == 1 && r.number() == 0 && r.number() == frame_limit &&
                r.number() == stall_limit && r.number() == 4 && r.number() == 10,
            "driver strategy identity");
    require(r.string(64) == source_hash && r.string(64) == certificate_hash &&
                r.string(64) == origin_digest && r.string(64) == history_hash,
            "source chain identity");
    Driver d;
    d.origin.controller_id = r.string(128);
    d.origin.producer_revision = r.string(256);
    d.origin.next_frame = r.number();
    d.origin.next_command = r.number();
    d.origin.controller_state = r.blob(16384);
    good(shared::validate_v2_origin(d.origin));
    d.producer = r.string(256);
    d.files_digest = r.string(64);
    for (auto *n : {&d.next_frame,     &d.next_command,    &d.checks,
                    &d.human,          &d.recruitment,     &d.home,
                    &d.business,       &d.human_page,      &d.gift_parent,
                    &d.gift_child,     &d.gift_pending,    &d.gift_slot,
                    &d.gift_item,      &d.gift_stock,      &d.gift_satisfaction,
                    &d.gift_charge,    &d.gift_month,      &d.gift_expense,
                    &d.gifts,          &d.gift_cash,       &d.gift_inventory,
                    &d.recruitments,   &d.admissions,      &d.businesses,
                    &d.previous_task,  &d.next_task_month, &d.task_departed,
                    &d.task_successes, &d.random,          &d.steps,
                    &d.history,        &d.months,          &d.last_month_frame,
                    &d.top_id,         &d.top_kind,        &d.top_raw,
                    &d.sound_count})
        *n = r.number();
    const auto action = r.number();
    require(action <= std::uint64_t(Intent::upgrade), "driver intent");
    d.pending = static_cast<Intent>(action);
    for (auto &n : d.date)
        n = r.number();
    for (auto &n : d.commands)
        n = r.number();
    for (auto &n : d.peaks)
        n = r.number();
    const auto count = r.number();
    require(count <= 4, "four residence record bound");
    std::set<std::uint64_t> people, instances, retired;
    for (std::uint64_t i = 0; i < count; ++i) {
        Home h{r.number(), r.number(), r.number(), r.number()};
        require(h.human < 55 && h.instance && h.retired_recruitment &&
                    h.completed_frame >= first_frame && h.completed_frame < d.next_frame &&
                    people.insert(h.human).second && instances.insert(h.instance).second &&
                    retired.insert(h.retired_recruitment).second,
                "unique completed residence identity");
        d.homes.push_back(h);
    }
    require(r.number() == 1, "unconsumed output");
    d.sound_hash = r.string(64);
    require(r.at == b.size() && encode(d) == b && !d.producer.empty() &&
                d.files_digest == origin_files && valid_hash(d.sound_hash),
            "canonical driver/source files");
    require(d.next_frame >= first_frame && d.next_frame <= frame_limit + 1 &&
                d.checks == d.next_frame - first_frame && d.next_command >= 1 &&
                d.next_command <= 1024 * (d.checks + 1) && d.commands[0] == 0 &&
                (d.human == absent || d.human < 55) && d.gift_pending <= 1 && d.gift_slot < 4 &&
                d.gift_stock <= 999 && d.gift_satisfaction <= 100 && d.gift_month < 12 &&
                d.admissions <= 4 && d.recruitments <= 4 && d.admissions <= d.recruitments &&
                d.homes.size() <= d.admissions && d.task_departed <= 1 && d.date[0] < 15 &&
                d.date[1] < 12 && d.date[2] < 4 && d.date[3] < 10800 &&
                d.months + 3 == d.date[0] * 12 + d.date[1] && d.last_month_frame < d.next_frame &&
                d.next_frame - 1 - d.last_month_frame <= stall_limit && d.top_id &&
                d.top_kind <= 4 && d.top_raw < 128 && d.sound_count <= 100 * d.checks &&
                d.next_task_month <= d.months + 5,
            "driver counters/calendar");
    std::uint64_t sum{};
    for (auto n : d.commands) {
        require(n <= d.next_command - 1 - sum, "command overflow");
        sum += n;
    }
    require(sum == d.next_command - 1 && d.gifts <= sum && d.recruitments <= sum &&
                d.admissions <= sum && d.businesses <= sum && d.gift_inventory <= d.gifts,
            "receipt totals");
    return d;
}
Metadata metadata(const Driver &d) {
    Metadata m;
    m.controller_id = controller;
    m.producer_revision = d.producer;
    m.next_frame = d.next_frame;
    m.next_command = d.next_command;
    m.controller_state = encode(d);
    return m;
}
const StartupWorldHuman &human_definition(const StartupWorldRuntimeState &s, std::uint64_t id) {
    const auto h = std::find_if(s.rules->humans.begin(), s.rules->humans.end(),
                                [&](const auto &v) { return std::uint64_t(v.identity) == id; });
    require(h != s.rules->humans.end(), "missing selected human definition");
    return *h;
}
std::int64_t reserve(const StartupWorldRuntimeState &s) {
    std::int64_t amount{};
    const auto add = [&](std::int64_t n) {
        require(n >= 0 && n <= INT64_MAX - amount, "maintenance reserve overflow");
        amount += n;
    };
    for (auto id : s.scene.world.facility_order)
        add(std::max(0, s.scripts.facilities
                            .at(s.scene.world.world.facilities.at(id).placement.definition_id)
                            .attributes[3]));
    for (auto id : s.scene.world.world.ai.human_order) {
        const auto human = s.scene.world.world.ai.battle.actors.at(id).definition;
        const auto job = s.scene.world.world.ai.growth.at(human).definition.current_profession;
        const auto fee = s.rules->jobs.at(job).fee;
        add(std::max({0, fee[0], fee[1]})); // 保守上界，不复制原月费插值。
    }
    return amount;
}
bool can_pay(const StartupWorldRuntimeState &s, std::int64_t cost, std::int64_t extra = 0) {
    const auto saved = reserve(s);
    require(cost >= 0 && extra >= 0 && cost <= INT64_MAX - extra &&
                cost + extra <= INT64_MAX - saved,
            "cost budget overflow");
    return s.scene.world.world.ai.accounting.funds() >= cost + extra + saved;
}
ref::HumanManagementEquipment equipment(const StartupWorldRuntimeState &s, int slot, int id) {
    const auto kind = slot == 0 ? 1 : slot == 3 ? 3 : 2;
    const auto def =
        std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(),
                     [&](const auto &v) { return v.shop.kind == kind && v.shop.id == id; });
    const auto owned = s.catalog.find({kind, id});
    require(slot >= 0 && slot < 4 && def != s.rules->equipment.end() && owned != s.catalog.end() &&
                (kind != 2 || ((def->shop.type == 2 ? 1 : 2) == slot)),
            "equipment projection identity");
    return {id,
            slot,
            owned->second.status,
            def->gift_order,
            def->reward_difficulty,
            def->shop.price,
            def->gift_rating,
            owned->second.free_purchases,
            def->shop.combat};
}
struct Gift {
    int slot{}, item{}, index{}, stock{}, gain{}, charge{};
};
std::optional<Gift> choose_gift(const StartupWorldRuntimeState &s, const Driver &d,
                                std::uint64_t page = 0) {
    const auto &h = human_definition(s, d.human);
    const auto &shop = s.shop_humans.at(h.identity);
    const auto job = s.scene.world.world.ai.growth.at(h.identity).definition.current_profession;
    std::vector<ref::HumanManagementEquipment> equipment_defs;
    for (const auto &v : s.rules->equipment) {
        const int slot = v.shop.kind == 1 ? 0 : v.shop.kind == 3 ? 3 : v.shop.type == 2 ? 1 : 2;
        equipment_defs.push_back(equipment(s, slot, v.shop.id));
    }
    std::optional<Gift> selected;
    for (int slot = 0; slot < 4; ++slot) {
        const auto catalog = ref::catalogue_human_equipment(slot, equipment_defs);
        require(catalog.has_value(), "gift catalogue source");
        const auto &list = page ? s.equipment_page_catalogs.at(page)[slot] : *catalog;
        for (std::size_t i = 0; i < list.size(); ++i) {
            const auto target = equipment(s, slot, list[i]);
            const auto cost = ref::human_equipment_gift_cost(target);
            const int grade =
                shop.equipment[slot] ? equipment(s, slot, *shop.equipment[slot]).grade : 0;
            const auto evaluation = ref::human_equipment_gift_evaluation(
                grade, target.grade, s.rules->jobs.at(job).gift_profile, target.affinity);
            require(cost && evaluation, "gift quote/evaluation source");
            const auto reward = ref::human_gift_rewards(*evaluation);
            require(reward.has_value(), "gift reward source");
            Gift g{slot,         list[i],      static_cast<int>(i),
                   target.stock, (*reward)[0], *cost == -1 ? 0 : *cost};
            if (target.availability == 0 || g.gain <= 0 || !can_pay(s, g.charge, h.residence_fee))
                continue;
            // 明确测试策略：先低现金消耗，再高本次满意度；平局保持实际目录序。
            if (!selected || std::make_pair(g.charge, -g.gain) <
                                 std::make_pair(selected->charge, -selected->gain))
                selected = g;
        }
    }
    return selected;
}
std::uint64_t choose_person(const StartupWorldRuntimeState &s) {
    std::optional<std::tuple<int, int, int, int>> best;
    for (const auto &h : s.rules->humans) {
        if (!s.human_presence.at(h.identity) || s.human_homes.at(h.identity)[2] != 0 ||
            !can_pay(s, h.residence_fee, 100))
            continue;
        const int gap =
            std::max(0, h.residence_threshold - s.shop_humans.at(h.identity).satisfaction);
        const auto key = std::make_tuple(gap ? 1 : 0, h.residence_fee, gap, h.identity);
        if (!best || key < *best)
            best = key;
    }
    return best ? std::uint64_t(std::get<3>(*best)) : absent;
}
std::vector<ref::FacilityPlacement> business_candidates(const StartupWorldRuntimeState &s) {
    std::vector<ref::FacilityPlacement> candidates;
    for (auto id : s.scene.world.facility_order) {
        const auto &f = s.scene.world.world.facilities.at(id);
        if ((f.kind == 3 || f.kind == 9) && f.status == 1)
            candidates.push_back(f.placement);
    }
    return candidates;
}
// kind9不维护kind3/12的连接警告位，所有实际计入验收的实例共用一次原地图搜索。
std::size_t connected_business_count(const ref::LegacyDistanceField &field,
                                     const std::vector<ref::FacilityPlacement> &candidates) {
    const auto access = ref::inspect_facility_access(field, candidates);
    require(access.error == ref::MapAccessError::none &&
                access.facilities.size() == candidates.size(),
            "business access identities");
    std::size_t count{};
    for (const auto &facility : access.facilities)
        if (!facility.cells.empty())
            ++count;
    return count;
}
std::size_t completed_business(const StartupWorldRuntimeState &s) {
    const auto candidates = business_candidates(s);
    if (candidates.empty())
        return 0;
    const auto search =
        ref::search_legacy_map(s.scene.world.world.map, startup_evidence().spawn_points.at(0));
    require(search.field.has_value(), "business connection field");
    return connected_business_count(*search.field, candidates);
}
bool business_target_met(const StartupWorldRuntimeState &s) {
    // 未达十个完工候选时不逐帧搜索；抵达门槛仍必须按真实连接核验，不能用flag作证。
    if (business_candidates(s).size() < 10)
        return false;
    return completed_business(s) >= 10;
}
void retired(const StartupWorldRuntimeState &s, std::uint64_t id) {
    require(!s.scene.world.world.facilities.count(id) && !s.facility_original_ids.count(id) &&
                !s.facility_ordinals.count(id) && !s.facility_residents.count(id) &&
                !s.facility_flags.count(id) && !s.facility_details.count(id) &&
                !s.facility_monthly_cash.count(id) && !s.facility_month_age.count(id) &&
                !s.facility_difficulties.count(id) && !s.facility_item_confirmations.count(id) &&
                !s.dungeon_facilities.count(id) && !s.sites.count(id) &&
                !s.neighbourhood.count(id) && !s.neighbourhood_details.count(id) &&
                !s.shops.count(id) &&
                std::find(s.shop_order.begin(), s.shop_order.end(), id) == s.shop_order.end() &&
                std::find(s.scene.world.facility_order.begin(), s.scene.world.facility_order.end(),
                          id) == s.scene.world.facility_order.end(),
            "recruitment retirement references");
}
bool human_ready(const StartupWorldRuntimeState &s, const ref::WorldScriptPage &p,
                 const Driver &d) {
    const auto binding = s.page_human_bindings.find(p.id);
    require(d.human != absent && binding != s.page_human_bindings.end() &&
                std::uint64_t(binding->second) == d.human,
            "human modal binding");
    if (p.legacy_page == 65)
        require(p.id == d.gift_child && s.human_page_parents.count(p.id) &&
                    s.human_page_parents.at(p.id) == d.gift_parent,
                "gift65 request/parent identity");
    if (startup_world_human_page_ready(s, p.id)) {
        if (p.legacy_page == 65) {
            const auto parent = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                [&](const auto &v) { return v.id == d.gift_parent; });
            const auto choice = s.human_equipment_choices.find(p.id);
            const auto parent_choice = s.human_equipment_choices.find(d.gift_parent);
            const auto parent_human = s.page_human_bindings.find(d.gift_parent);
            require(parent != s.scripts.pages.end() && parent->legacy_page == 64 &&
                        parent->kind == ref::WorldScriptPageKind::raw_page && parent->lifecycle != 4 &&
                        parent_human != s.page_human_bindings.end() && parent_human->second == binding->second &&
                        choice != s.human_equipment_choices.end() &&
                        parent_choice != s.human_equipment_choices.end() && choice->second == parent_choice->second,
                    "initialized gift65 live parent/human/choice");
        }
        return true;
    }
    require(p.lifecycle == 0 && !s.human_pages_initialized.count(p.id) &&
                s.page_phases.count(p.id) && s.page_counters.count(p.id) &&
                s.human_page_selections.count(p.id),
            "initialized human payload missing");
    if (p.legacy_page == 60)
        require(p.id == d.human_page, "new60 request identity");
    if (p.legacy_page == 64)
        require(p.id == d.gift_parent, "new64 request identity");
    if (p.legacy_page == 65)
        require(p.id == d.gift_child && s.human_page_parents.count(p.id) &&
                    s.human_page_parents.at(p.id) == d.gift_parent,
                "new65 parent identity");
    return false;
}
Intent plan(const StartupApplication &app, const Driver &d) {
    const auto &s = app.world()->state();
    const auto *p = base::top(app);
    require(p, "missing top");
    if (p->kind == ref::WorldScriptPageKind::scene) {
        if (s.scene.scene_state != 0 || d.home || d.business)
            return Intent::wait;
        if (d.homes.size() >= 4)
            return business_target_met(s) ? Intent::wait : Intent::build_business;
        if (d.human == absent)
            return choose_person(s) == absent ? Intent::wait : Intent::choose_human;
        const auto &h = human_definition(s, d.human);
        if (s.shop_humans.at(h.identity).satisfaction < h.residence_threshold)
            return choose_gift(s, d) ? Intent::open_human : Intent::wait;
        if (d.recruitment)
            return s.scene.world.world.facilities.at(d.recruitment).status == 1
                       ? Intent::open_recruitment
                       : Intent::wait;
        const auto quote = startup_world_build_quote(s, 24);
        require(quote.has_value(), "recruitment quote");
        return can_pay(s, quote->construction_cost, h.residence_fee) ? Intent::build_recruitment
                                                                     : Intent::wait;
    }
    if (p->kind == ref::WorldScriptPageKind::raw_page) {
        switch (p->legacy_page) {
        case 60:
            if (!human_ready(s, *p, d))
                return Intent::wait;
            require(p->id == d.human_page, "unexpected human60");
            return s.shop_humans.at(static_cast<int>(d.human)).satisfaction >=
                               human_definition(s, d.human).residence_threshold ||
                           !choose_gift(s, d)
                       ? Intent::close_human
                       : Intent::gifts;
        case 64:
            if (!human_ready(s, *p, d))
                return Intent::wait;
            require(p->id == d.gift_parent, "unexpected gift64");
            if (s.human_page_answers.count(p->id))
                return Intent::wait;
            return s.shop_humans.at(static_cast<int>(d.human)).satisfaction >=
                               human_definition(s, d.human).residence_threshold ||
                           !choose_gift(s, d, p->id)
                       ? Intent::close_human
                       : Intent::choose_gift;
        case 65:
            return human_ready(s, *p, d) ? Intent::confirm_gift : Intent::wait;
        case 66:
        case 68:
        case 69:
            return human_ready(s, *p, d) ? Intent::acknowledge : Intent::wait;
        case 74:
            require(valid_startup_world_facility_page(s, *p) &&
                        s.facility_page_bindings.count(p->id) &&
                        s.facility_page_bindings.at(p->id) == d.recruitment,
                    "recruitment74 binding");
            return Intent::residence_list;
        case 80:
            require(s.facility_page_bindings.count(p->id) &&
                        s.facility_page_bindings.at(p->id) == d.recruitment &&
                        s.residence_page_candidates.count(p->id),
                    "residence80 binding");
            return Intent::admit;
        case 81:
            return Intent::upgrade;
        case 90:
            return Intent::acknowledge; // 原住宅收税列表；实际扣/入账由98 update完成。
        default:
            break;
        }
    }
    switch (shared::inherited_modal(app)) {
    case shared::Modal::wait:
        return Intent::wait;
    case shared::Modal::acknowledge:
        return Intent::acknowledge;
    case shared::Modal::confirm_task:
        return Intent::task_confirm;
    case shared::Modal::depart_task:
        return Intent::task_depart;
    case shared::Modal::deadline:
        return Intent::deadline;
    case shared::Modal::award:
        return Intent::award;
    case shared::Modal::leave:
        return Intent::leave;
    }
    throw std::runtime_error("residence unknown inherited modal");
}
void observe(const StartupApplication &app, Driver &d) {
    const auto &s = app.world()->state();
    const auto *p = base::top(app);
    require(p, "observe top");
    d.date = base::date(app);
    d.months = d.date[0] * 12 + d.date[1] - 3;
    d.random = s.scene.random.draws();
    d.steps = s.simulation_steps;
    d.history = app.world()->checkpoints().size();
    require(s.task_progress.successes >= 0, "negative task successes");
    d.task_successes = s.task_progress.successes;
    d.top_id = p->id;
    d.top_kind = std::uint64_t(p->kind);
    d.top_raw = p->kind == ref::WorldScriptPageKind::raw_page ? p->legacy_page : 0;
    const auto sizes = base::resources(app);
    for (std::size_t i = 0; i < sizes.size(); ++i)
        d.peaks[i] = std::max(d.peaks[i], sizes[i]);
    d.pending = plan(app, d);
}
// 65已返回、64尚未消费时，Driver保存的报价必须逐字段等于当前唯一Owner。
// 此时世界/随机尚未再推进；错误数据必须在restore发布前拒绝，不能等下一轮才失败。
void validate_pending_gift(const StartupWorldRuntimeState &s, const Driver &d) {
    require(d.gift_pending == 1 && d.human != absent &&
                d.human <= std::uint64_t(std::numeric_limits<int>::max()) && d.gift_parent &&
                d.gift_child,
            "pending gift identities");
    const auto parent = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                     [&](const auto &p) { return p.id == d.gift_parent; });
    const auto child = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                    [&](const auto &p) { return p.id == d.gift_child; });
    require(parent != s.scripts.pages.end() && child != s.scripts.pages.end() && parent < child &&
                parent->kind == ref::WorldScriptPageKind::raw_page && parent->legacy_page == 64 &&
                parent->lifecycle != 4 && child->kind == ref::WorldScriptPageKind::raw_page &&
                child->legacy_page == 65 && child->lifecycle == 4 &&
                s.human_pages_initialized.count(d.gift_parent) &&
                s.human_pages_initialized.count(d.gift_child),
            "pending gift actual live64/retired65");
    const auto parent_human = s.page_human_bindings.find(d.gift_parent),
               child_human = s.page_human_bindings.find(d.gift_child);
    const auto relation = s.human_page_parents.find(d.gift_child);
    const auto answer = s.human_page_answers.find(d.gift_parent);
    const auto parent_choice = s.human_equipment_choices.find(d.gift_parent),
               child_choice = s.human_equipment_choices.find(d.gift_child);
    require(
        parent_human != s.page_human_bindings.end() && child_human != s.page_human_bindings.end() &&
            parent_human->second == static_cast<int>(d.human) &&
            child_human->second == parent_human->second && relation != s.human_page_parents.end() &&
            relation->second == d.gift_parent && answer != s.human_page_answers.end() &&
            answer->second == 0 && parent_choice != s.human_equipment_choices.end() &&
            child_choice != s.human_equipment_choices.end() &&
            parent_choice->second == child_choice->second && parent_choice->second[0] >= 0 &&
            parent_choice->second[0] < 4 && parent_choice->second[1] >= 0 &&
            std::uint64_t(parent_choice->second[0]) == d.gift_slot &&
            std::uint64_t(parent_choice->second[1]) == d.gift_item,
        "pending gift human/parent/choice");
    const auto target = equipment(s, parent_choice->second[0], parent_choice->second[1]);
    const auto quote = ref::human_equipment_gift_cost(target);
    const auto person = s.shop_humans.find(static_cast<int>(d.human));
    require(quote && target.stock >= 0 && person != s.shop_humans.end() &&
                person->second.satisfaction >= 0 && d.gift_stock == std::uint64_t(target.stock) &&
                d.gift_satisfaction == std::uint64_t(person->second.satisfaction) &&
                d.gift_charge == std::uint64_t(*quote == -1 ? 0 : *quote) &&
                s.scene.calendar.month >= 0 && s.scene.calendar.month < 12 &&
                d.gift_month == std::uint64_t(s.scene.calendar.month) &&
                s.monthly_cash.at(d.gift_month)[2][1] >= 0 &&
                d.gift_expense == std::uint64_t(s.monthly_cash.at(d.gift_month)[2][1]),
            "pending gift frozen stock/reward/quote/month/expense");
}
// 双向核对Owner答案与Driver待消费标志，不能清掉标志隐藏下一轮真实付款。
void validate_gift_binding(const StartupWorldRuntimeState &s, const Driver &d) {
    for (const auto &answer : s.human_page_answers) {
        const auto page = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
            [&](const auto &p) { return p.id == answer.first; });
        if (page != s.scripts.pages.end() && page->kind == ref::WorldScriptPageKind::raw_page &&
            page->legacy_page == 64 && answer.second == 0)
            require(d.gift_pending == 1 && d.gift_parent == answer.first,
                    "successful64 answer cannot lose pending receipt");
    }
    if (d.gift_pending)
        validate_pending_gift(s, d);
    else
        require(d.gift_slot == 0 && d.gift_item == 0 && d.gift_stock == 0 &&
                    d.gift_satisfaction == 0 && d.gift_charge == 0 && d.gift_month == 0 &&
                    d.gift_expense == 0,
                "inactive gift retains frozen payment");
}
int check_pending_gift_rejections(const StartupWorldRuntimeState &s, const Driver &d) {
    validate_pending_gift(s, d);
    int checks{};
    const auto reject = [&](const Driver &bad) {
        bool refused{};
        try {
            validate_gift_binding(s, bad);
        } catch (const std::exception &) {
            refused = true;
        }
        require(refused, "pending gift payload mutation accepted");
        ++checks;
    };
    auto bad = d;
    bad.gift_pending = 0;
    reject(bad);
    bad.gift_parent = bad.gift_child = bad.gift_slot = bad.gift_item = bad.gift_stock =
        bad.gift_satisfaction = bad.gift_charge = bad.gift_month = bad.gift_expense = 0;
    reject(bad);
    bad = d;
    bad.gift_child = s.scripts.next_page_id;
    reject(bad);
    bad = d;
    bad.gift_parent = d.gift_child;
    reject(bad);
    bad = d;
    bad.human = (bad.human + 1) % 55;
    reject(bad);
    bad = d;
    bad.gift_slot = (bad.gift_slot + 1) % 4;
    reject(bad);
    bad = d;
    ++bad.gift_item;
    reject(bad);
    bad = d;
    ++bad.gift_stock;
    reject(bad);
    bad = d;
    bad.gift_satisfaction = (bad.gift_satisfaction + 1) % 101;
    reject(bad);
    bad = d;
    ++bad.gift_charge;
    reject(bad);
    bad = d;
    bad.gift_month = (bad.gift_month + 1) % 12;
    reject(bad);
    bad = d;
    ++bad.gift_expense;
    reject(bad);
    return checks;
}
std::string validate(const StartupApplication &app, const Metadata &m) {
    try {
        require(m.controller_id == controller && m.extensions.empty(), "metadata identity");
        const auto d = decode(m.controller_state);
        require(m.next_frame == d.next_frame && m.next_command == d.next_command &&
                    m.producer_revision == d.producer && app.error().empty() && app.world() &&
                    app.mode() == StartupApplicationMode::logic &&
                    app.page() == StartupApplicationPage::world,
                "application binding");
        const auto &s = app.world()->state();
        const auto *p = base::top(app);
        const auto seed = ref::WorldRandomStream::from_java_seed(1).snapshot();
        require(app.handoff_random() && app.handoff_random()->cursor == 0 &&
                    app.handoff_random()->engine_state == seed.engine_state &&
                    !app.handoff_random()->tape_mode && app.handoff_random()->tape.empty() &&
                    s.scene.speed_setting == 0 && !s.scene.framework_paused &&
                    !s.scripts.executing_page && !app.has_pending_audio_requests() &&
                    s.sound_requests.empty() && !app.clear_page() && s.rank >= 1 &&
                    base::references(s) && d.date == base::date(app) &&
                    d.random == s.scene.random.draws() && d.steps == s.simulation_steps &&
                    d.history == app.world()->checkpoints().size() && p && d.top_id == p->id &&
                    d.top_kind == std::uint64_t(p->kind) &&
                    d.top_raw == std::uint64_t(p->kind == ref::WorldScriptPageKind::raw_page
                                                   ? p->legacy_page
                                                   : 0) &&
                    d.pending == plan(app, d) &&
                    d.task_successes == std::uint64_t(s.task_progress.successes),
                "world/page/output binding");
        require(s.tasks.count(7) && (!s.active_task || *s.active_task == 7) &&
                    (!d.previous_task || d.previous_task == 7),
                "inherited task identity/no new acceptance");
        if (d.human != absent) {
            const auto &h = human_definition(s, d.human);
            require(s.human_presence.at(h.identity) != 0, "selected human never arrived");
        }
        if (d.recruitment && !d.home) {
            const auto &f = s.scene.world.world.facilities.at(d.recruitment);
            require(f.kind == 13 && f.placement.definition_id == 24, "recruitment identity");
        }
        if (d.home) {
            retired(s, d.recruitment);
            const auto &f = s.scene.world.world.facilities.at(d.home);
            require(f.kind == 12 && d.human != absent &&
                        s.facility_residents.at(d.home) == static_cast<int>(d.human),
                    "admitted home binding");
        }
        for (const auto &h : d.homes) {
            retired(s, h.retired_recruitment);
            const auto &f = s.scene.world.world.facilities.at(h.instance);
            require(f.kind == 12 && f.status == 1 &&
                        s.facility_residents.at(h.instance) == static_cast<int>(h.human) &&
                        s.human_homes.at(static_cast<int>(h.human))[2] == 1,
                    "completed home binding");
        }
        if (d.business) {
            const auto &f = s.scene.world.world.facilities.at(d.business);
            require(f.kind == 3 || f.kind == 9, "business identity");
        }
        const auto owned = [&](std::uint64_t id) {
            return std::any_of(s.scripts.pages.begin(), s.scripts.pages.end(),
                               [&](const auto &v) { return v.id == id; });
        };
        for (const auto &v : s.human_page_parents)
            require(owned(v.first) && owned(v.second), "human parent retirement");
        for (const auto &v : s.human_page_answers)
            require(owned(v.first), "human answer retirement");
        validate_gift_binding(s, d);
        const auto sizes = base::resources(app);
        for (std::size_t i = 0; i < sizes.size(); ++i)
            require(d.peaks[i] >= sizes[i], "resource peak");
        return {};
    } catch (const std::exception &e) {
        return e.what();
    }
}
std::optional<int> business_definition(const StartupWorldRuntimeState &s) {
    const auto catalog = startup_world_build_catalog(s);
    require(catalog.has_value(), "business build catalogue");
    for (const auto &group : *catalog)
        for (int id : group) {
            const auto d = std::find_if(s.rules->facilities.begin(), s.rules->facilities.end(),
                                        [&](const auto &v) { return v.id == id; });
            require(d != s.rules->facilities.end(), "business catalogue definition");
            if (d->kind != 3)
                continue;
            const auto quote = startup_world_build_quote(s, id);
            require(quote.has_value(), "business quote");
            if (can_pay(s, quote->construction_cost,
                        std::max<std::int64_t>(0, quote->definition_attributes[3])))
                return id;
        }
    return {};
}
ref::Position connected_site(const StartupWorldRuntimeState &s, int definition) {
    const auto d = std::find_if(s.rules->facilities.begin(), s.rules->facilities.end(),
                                [&](const auto &v) { return v.id == definition; });
    require(d != s.rules->facilities.end(), "build definition");
    const auto &map = s.scene.world.world.map;
    const auto bounds = s.rules->fences.at(s.fence_level);
    const auto search = ref::search_legacy_map(map, startup_evidence().spawn_points.at(0));
    require(search.field.has_value(), "existing road field");
    for (int y = bounds[1].y + 1; y < bounds[0].y; ++y)
        for (int x = bounds[0].x + 1; x < bounds[1].x; ++x) {
            const auto footprint = ref::facility_footprint(
                static_cast<ref::FacilityShape>(d->shape), ref::FacilityOrientation::first, {x, y},
                map.width, map.height);
            if (footprint.error != ref::GeometryError::none)
                continue;
            bool free = true, road = false;
            for (const auto &part : footprint.cells) {
                const auto p = part.position;
                if (p.x <= bounds[0].x || p.x >= bounds[1].x || p.y >= bounds[0].y ||
                    p.y <= bounds[1].y || map.cells.at(p.y * map.width + p.x).facility ||
                    map.cells.at(p.y * map.width + p.x).legacy_state != 4) {
                    free = false;
                    break;
                }
                for (const auto delta :
                     std::array<ref::Position, 4>{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}}) {
                    const int nx = p.x + delta.x, ny = p.y + delta.y;
                    if (nx < 0 || nx >= map.width || ny < 0 || ny >= map.height)
                        continue;
                    const auto index = static_cast<std::size_t>(ny * map.width + nx);
                    if (map.cells[index].legacy_state == 3 && search.field->distances.at(index))
                        road = true;
                }
            }
            if (free && road)
                return {x, y};
        }
    throw std::runtime_error(
        "residence no free footprint adjoining reachable existing road; preserve evidence");
}
void verify_connected(const StartupWorldRuntimeState &s, std::uint64_t id) {
    const auto &f = s.scene.world.world.facilities.at(id);
    const auto search =
        ref::search_legacy_map(s.scene.world.world.map, startup_evidence().spawn_points.at(0));
    require(search.field.has_value(), "actual connection field");
    const auto access = ref::inspect_facility_access(*search.field, {f.placement});
    require(access.error == ref::MapAccessError::none && access.facilities.size() == 1 &&
                !access.facilities.front().cells.empty(),
            "actual facility disconnected");
}
void settle(const StartupApplication &app, Driver &d, std::uint64_t frame) {
    const auto &s = app.world()->state();
    if (d.gift_pending && !s.human_page_answers.count(d.gift_parent)) {
        const auto current =
            equipment(s, static_cast<int>(d.gift_slot), static_cast<int>(d.gift_item));
        const auto &person = s.shop_humans.at(static_cast<int>(d.human));
        require(s.human_pages_initialized.count(d.gift_parent) &&
                    person.satisfaction > static_cast<int>(d.gift_satisfaction) &&
                    person.equipment.at(d.gift_slot) &&
                    *person.equipment.at(d.gift_slot) == static_cast<int>(d.gift_item) &&
                    current.stock == static_cast<int>(d.gift_stock) - (d.gift_stock > 0 ? 1 : 0) &&
                    s.scene.calendar.month == static_cast<int>(d.gift_month) &&
                    s.monthly_cash.at(d.gift_month)[2][1] >= 0 &&
                    std::uint64_t(s.monthly_cash.at(d.gift_month)[2][1]) ==
                        d.gift_expense + d.gift_charge,
                "gift parent actual stock/expense/reward/equipment commit");
        ++d.gifts;
        d.gift_cash += d.gift_charge;
        d.gift_inventory += d.gift_stock > 0 ? 1 : 0;
        d.gift_pending = 0;
        d.gift_child = 0;
        d.gift_slot = d.gift_item = d.gift_stock = d.gift_satisfaction = d.gift_charge =
            d.gift_month = d.gift_expense = 0;
    }
    if (d.previous_task && !s.active_task)
        d.next_task_month = s.scene.calendar.year * 12 + s.scene.calendar.month + 2;
    d.previous_task = s.active_task.value_or(0);
    const auto *p = base::top(app);
    if (!p || p->kind != ref::WorldScriptPageKind::scene || s.scene.scene_state != 0)
        return;
    if (d.home && s.scene.world.world.facilities.at(d.home).status == 1 &&
        s.human_pages_initialized.empty()) {
        retired(s, d.recruitment);
        verify_connected(s, d.home);
        require(s.facility_residents.at(d.home) == static_cast<int>(d.human) &&
                    s.human_homes.at(static_cast<int>(d.human))[2] == 1 &&
                    ref::world_script_seen(s.scripts, 202),
                "actual residence completion/source first-home event");
        d.homes.push_back({d.human, d.home, frame, d.recruitment});
        d.human = absent;
        d.home = d.recruitment = 0;
        d.human_page = d.gift_parent = d.gift_child = 0;
    }
    if (d.business && s.scene.world.world.facilities.at(d.business).status == 1) {
        verify_connected(s, d.business);
        d.business = 0;
    }
}
Round step(StartupApplication &app, Driver &d) {
    good(validate(app, metadata(d)));
    require(d.next_frame <= frame_limit, "absolute frame bound");
    const auto frame = d.next_frame, old_month = d.months, old_random = d.random,
               old_steps = d.steps, old_history = d.history;
    const auto old_date = d.date;
    good(app.update());
    settle(app, d, frame);
    const auto &s = app.world()->state();
    const auto action = plan(app, d);
    const auto page = *base::top(app);
    Round round;
    const auto record = [&](Input input, std::uint64_t id = 0, std::int64_t a = 0,
                            std::int64_t b = 0, std::int64_t c = 0) {
        require(id <= std::uint64_t(INT64_MAX), "command identity range");
        round.commands.push_back({std::int64_t(input), std::int64_t(id), a, b, c});
        if (input != Input::wait) {
            ++d.next_command;
            ++d.commands[std::size_t(input)];
        }
    };
    const auto human_action = [&](StartupHumanPageAction command, int choice = 0) {
        good(app.act_human_page(page.id, command, choice));
        record(Input::human, page.id, static_cast<int>(command), choice);
    };
    const auto task = [&](StartupWorldTaskAction command, int choice = 0) {
        const auto result = app.act_task_page(page.id, command, choice);
        good(result.error);
        require(result.denial == ref::TaskCommandDenial::none && !result.accepted,
                "inherited task operation denial/new acceptance");
        d.task_departed += result.departed ? 1 : 0;
        record(Input::task, page.id, static_cast<int>(command), choice);
    };
    const auto build = [&](int definition) {
        const auto site = connected_site(app.world()->state(), definition);
        good(app.open_build_menu());
        record(Input::open_build);
        const auto menu = base::top(app)->id;
        const auto selected = app.select_build_menu(menu, definition);
        good(selected.error);
        require(selected.denial == StartupBuildDenial::none, "actual build catalogue admission");
        record(Input::select_build, menu, definition);
        const auto placed = app.confirm_build(site, ref::FacilityOrientation::first);
        good(placed.error);
        require(placed.denial == StartupBuildDenial::none && placed.created,
                "actual connected placement");
        record(Input::confirm_build, 0, site.x, site.y,
               static_cast<int>(ref::FacilityOrientation::first));
        good(app.cancel_build());
        record(Input::cancel_build);
        return *placed.created;
    };
    switch (action) {
    case Intent::wait:
        record(Input::wait);
        break;
    case Intent::choose_human:
        d.human = choose_person(s);
        require(d.human != absent, "candidate disappeared");
        record(Input::wait);
        break;
    case Intent::open_human:
        good(app.open_human_page(static_cast<int>(d.human)));
        record(Input::open_human, 0, static_cast<int>(d.human));
        d.human_page = base::top(app)->id;
        break;
    case Intent::gifts:
        human_action(StartupHumanPageAction::gifts);
        d.gift_parent = base::top(app)->id;
        break;
    case Intent::choose_gift: {
        const auto gift = choose_gift(s, d, page.id);
        require(gift.has_value(), "gift quote disappeared");
        human_action(StartupHumanPageAction::equipment_slot, gift->slot);
        human_action(StartupHumanPageAction::select, gift->index);
        human_action(StartupHumanPageAction::confirm);
        d.gift_child = base::top(app)->id;
        require(base::top(app)->legacy_page == 65, "quote must create real65");
        break;
    }
    case Intent::confirm_gift: {
        // 真实已初始化65上核拒绝边界；复用恢复plan调用的同一只读绑定检查。
        require(human_ready(s, page, d), "gift65 confirmation requires initialized payload");
        for (int field = 0; field < 2; ++field) {
            auto bad = d;
            (field == 0 ? bad.gift_child : bad.gift_parent) = s.scripts.next_page_id;
            bool refused{};
            try { (void)human_ready(s, page, bad); }
            catch (const std::exception &) { refused = true; }
            require(refused, "initialized65 changed driver parent/child accepted");
        }
        const auto parent = s.human_page_parents.find(page.id);
        const auto choice = s.human_equipment_choices.find(page.id);
        require(page.id == d.gift_child && parent != s.human_page_parents.end() &&
                    parent->second == d.gift_parent && choice != s.human_equipment_choices.end() &&
                    s.human_equipment_choices.count(d.gift_parent) &&
                    s.human_equipment_choices.at(d.gift_parent) == choice->second,
                "gift65 same choice/parent");
        const auto item = equipment(s, choice->second[0], choice->second[1]);
        const auto cost = ref::human_equipment_gift_cost(item);
        require(cost.has_value(), "gift65 quote");
        const auto charge = *cost == -1 ? 0 : *cost;
        require(can_pay(s, charge, human_definition(s, d.human).residence_fee),
                "gift65 keeps residence/maintenance reserve");
        d.gift_slot = choice->second[0];
        d.gift_item = choice->second[1];
        d.gift_stock = item.stock;
        d.gift_satisfaction = s.shop_humans.at(static_cast<int>(d.human)).satisfaction;
        d.gift_charge = charge;
        d.gift_month = s.scene.calendar.month;
        require(s.monthly_cash.at(d.gift_month)[2][1] >= 0, "gift expense baseline");
        d.gift_expense = s.monthly_cash.at(d.gift_month)[2][1];
        human_action(StartupHumanPageAction::confirm);
        const auto &after = app.world()->state();
        require(after.human_page_answers.count(d.gift_parent) &&
                    after.human_page_answers.at(d.gift_parent) == 0 &&
                    after.shop_humans.at(static_cast<int>(d.human)).satisfaction ==
                        static_cast<int>(d.gift_satisfaction) &&
                    equipment(after, static_cast<int>(d.gift_slot), static_cast<int>(d.gift_item))
                            .stock == static_cast<int>(d.gift_stock) &&
                    std::uint64_t(after.monthly_cash.at(d.gift_month)[2][1]) == d.gift_expense,
                "65 only returns answer without gift commit");
        d.gift_pending = 1;
        (void)check_pending_gift_rejections(after, d);
        break;
    }
    case Intent::close_human:
        human_action(StartupHumanPageAction::cancel);
        if (page.legacy_page == 64)
            d.gift_parent = 0;
        else
            d.human_page = 0;
        break;
    case Intent::build_recruitment:
        d.recruitment = build(24);
        ++d.recruitments;
        break;
    case Intent::open_recruitment:
        good(app.open_facility_page(d.recruitment));
        record(Input::open_facility, 0, static_cast<std::int64_t>(d.recruitment));
        break;
    case Intent::residence_list:
        good(app.act_facility_page(page.id, StartupFacilityPageAction::confirm));
        record(Input::facility, page.id, static_cast<int>(StartupFacilityPageAction::confirm));
        break;
    case Intent::admit: {
        const auto &candidates = s.residence_page_candidates.at(page.id);
        require(d.human != absent && std::find(candidates.begin(), candidates.end(),
                                               static_cast<int>(d.human)) != candidates.end(),
                "actual80 person eligibility");
        const auto h = human_definition(s, d.human);
        require(can_pay(s, h.residence_fee), "admission reserve");
        const auto old_cash = s.scene.world.world.ai.accounting.funds();
        const auto result = app.act_residence_page(page.id, h.identity);
        good(result.error);
        require(result.denial == StartupBuildDenial::none && result.created,
                "real80 creates residence");
        record(Input::residence, page.id, h.identity);
        d.home = *result.created;
        ++d.admissions;
        const auto &after = app.world()->state();
        retired(after, d.recruitment);
        require(after.scene.world.world.ai.accounting.funds() == old_cash - h.residence_fee &&
                    after.facility_residents.at(d.home) == h.identity,
                "exact admission payment/new binding");
        break;
    }
    case Intent::build_business: {
        const auto definition = business_definition(s);
        if (!definition)
            record(Input::wait);
        else {
            d.business = build(*definition);
            ++d.businesses;
        }
        break;
    }
    case Intent::acknowledge:
        if (page.kind == ref::WorldScriptPageKind::raw_page &&
            (page.legacy_page == 66 || page.legacy_page == 68 || page.legacy_page == 69))
            human_action(StartupHumanPageAction::confirm);
        else {
            good(app.acknowledge_page(page.id));
            record(Input::acknowledge, page.id);
        }
        break;
    case Intent::upgrade:
        require(s.facility_page_bindings.count(page.id), "upgrade81 missing identity");
        good(app.acknowledge_page(page.id));
        record(Input::acknowledge, page.id);
        break;
    case Intent::task_confirm:
        task(StartupWorldTaskAction::confirm);
        break;
    case Intent::task_depart:
        task(StartupWorldTaskAction::depart);
        break;
    case Intent::deadline:
        task(StartupWorldTaskAction::confirm,
             s.scene.world.world.ai.accounting.funds() >= page.legacy_f ? 0 : 1);
        break;
    case Intent::award:
        good(app.act_award_page(page.id, ref::WorldAwardAction::request_award, 0));
        record(Input::award, page.id, static_cast<int>(ref::WorldAwardAction::request_award));
        good(app.act_award_page(page.id, ref::WorldAwardAction::confirm_award));
        record(Input::award, page.id, static_cast<int>(ref::WorldAwardAction::confirm_award));
        break;
    case Intent::leave:
        good(app.leave_commerce_page(page.id));
        record(Input::leave, page.id);
        break;
    }
    round.sounds = app.take_audio_requests();
    Bytes audio(d.sound_hash.begin(), d.sound_hash.end());
    number(audio, frame);
    number(audio, round.sounds.size());
    for (const auto &sound : round.sounds) {
        require(std::uint64_t(sound.operation) <= 2 && sound.id >= 0 && sound.id < 26,
                "typed sound range");
        number(audio, std::uint64_t(sound.operation));
        number(audio, sound.id);
    }
    require(app.take_sound_requests().empty(), "output consumed twice");
    d.sound_hash = hash(audio);
    d.sound_count += round.sounds.size();
    ++d.next_frame;
    ++d.checks;
    observe(app, d);
    require(d.date >= old_date && (d.months == old_month || d.months == old_month + 1) &&
                d.random >= old_random && d.steps >= old_steps && d.history >= old_history,
            "calendar/history monotonic");
    if (d.months != old_month)
        d.last_month_frame = frame;
    good(validate(app, metadata(d)));
    return round;
}
Driver initial(const shared::Handoff &old, const fs::path &live) {
    const auto &app = *old.application;
    good(shared::validate_v2(app, old.metadata));
    const auto before = startup_application_replay_digest(app, old.metadata, shared::validate_v2),
               files = shared::directory_digest(live);
    Driver d;
    d.origin = old.metadata;
    d.files_digest = files;
    d.next_task_month = old.next_task_month;
    d.previous_task = app.world()->state().active_task.value_or(0);
    observe(app, d);
    require(before == origin_digest && files == origin_files &&
                before ==
                    startup_application_replay_digest(app, old.metadata, shared::validate_v2) &&
                files == shared::directory_digest(live),
            "zero-business controller handoff");
    good(validate(app, metadata(d)));
    return d;
}
std::string receipts(const StartupApplication &app, const Driver &d) {
    std::ostringstream out;
    out << "{\"gifts_committed\":" << d.gifts << ",\"recruitments_created\":" << d.recruitments
        << ",\"homes_admitted\":" << d.admissions << ",\"completed_houses\":" << d.homes.size()
        << ",\"completed_business_facilities\":" << completed_business(app.world()->state()) << '}';
    return out.str();
}
bool terminal(const StartupApplication &app, const Driver &d) {
    return d.homes.size() == 4 && business_target_met(app.world()->state()) && !d.home &&
           !d.business && !d.gift_pending &&
           base::top(app)->kind == ref::WorldScriptPageKind::scene &&
           app.world()->state().scene.scene_state == 0;
}
std::string handoff_json(const Driver &d) {
    std::ostringstream out;
    out << "{\"origin_metadata\":{\"controller\":\"" << d.origin.controller_id
        << "\",\"producer_revision\":\"" << d.origin.producer_revision
        << "\",\"next_frame\":" << d.origin.next_frame
        << ",\"next_command\":" << d.origin.next_command << ",\"driver\":\""
        << hex(d.origin.controller_state) << "\",\"driver_digest\":\""
        << hash(d.origin.controller_state) << "\"},\"before_digest\":\"" << origin_digest
        << "\",\"after_digest\":\"" << origin_digest << "\",\"origin_snapshot_sha256\":\""
        << source_hash << "\",\"origin_certificate_sha256\":\"" << certificate_hash
        << "\",\"uncertified_history_sha256\":\"" << history_hash << "\",\"files_before_digest\":\""
        << d.files_digest << "\",\"files_after_digest\":\"" << d.files_digest
        << "\",\"prior_handoff\":" << shared::previous_handoff_json(d.origin) << '}';
    return out.str();
}
int check_bound_driver(const StartupApplication &app, const Driver &d) {
    const auto before = startup_application_replay_digest(app, metadata(d), validate);
    int checks{};
    const auto reject = [&](const Metadata &m, const char *why) {
        require(!validate(app, m).empty(), why);
        ++checks;
    };
    auto m = metadata(d);
    m.controller_state.pop_back();
    reject(m, "truncated driver accepted");
    m = metadata(d);
    m.controller_state.push_back(0);
    reject(m, "trailing driver accepted");
    m = metadata(d);
    m.controller_id = d.origin.controller_id;
    reject(m, "v2 reinterpreted as residence");
    auto bad = d;
    ++bad.next_frame;
    reject(metadata(bad), "wrong frame accepted");
    bad = d;
    ++bad.origin.next_command;
    reject(metadata(bad), "changed origin metadata accepted");
    bad = d;
    bad.files_digest[0] = bad.files_digest[0] == '0' ? '1' : '0';
    reject(metadata(bad), "changed valid-hex origin files accepted");
    bad = d;
    bad.pending = d.pending == Intent::wait ? Intent::open_human : Intent::wait;
    reject(metadata(bad), "wrong next intention accepted");
    bad = d;
    bad.commands[1] = UINT64_MAX;
    reject(metadata(bad), "command overflow accepted");
    bad = d;
    bad.human = 999;
    reject(metadata(bad), "unknown human accepted");
    bad = d;
    bad.gift_pending = 1;
    bad.gift_parent = 0;
    reject(metadata(bad), "gift without parent accepted");
    bad = d;
    bad.recruitments = 5;
    reject(metadata(bad), "unbounded recruitment accepted");
    m = metadata(d);
    m.controller_state[m.controller_state.size() - 80] = 0;
    reject(m, "unconsumed output accepted");
    if (d.gift_pending)
        checks += check_pending_gift_rejections(app.world()->state(), d);
    require(before == startup_application_replay_digest(app, metadata(d), validate),
            "bad driver changed application");
    return checks + 1;
}
std::uint64_t option_number(const std::map<std::string, std::string> &options,
                            const std::string &key, std::uint64_t fallback) {
    const auto p = options.find(key);
    if (p == options.end())
        return fallback;
    require(!p->second.empty() && p->second.find_first_not_of("0123456789") == p->second.npos,
            "invalid numeric option");
    const auto n = std::stoull(p->second);
    require(n <= frame_limit, "option bound");
    return n;
}
} // namespace

int run_startup_application_residence_replay_cli(int argc, const char **argv) {
    require(argc > 1 && std::string(argv[1]) == controller, "CLI identity");
    const std::array<std::string, 12> keys{"--work-dir",
                                           "--trace-file",
                                           "--stop-at",
                                           "--load-file",
                                           "--save-file",
                                           "--save-at",
                                           "--tail-after-save",
                                           "--trace-from",
                                           "--producer-revision",
                                           "--handoff-file",
                                           "--handoff-certificate-sha256",
                                           "--handoff-snapshot-sha256"};
    std::map<std::string, std::string> options;
    for (int i = 2; i < argc; i += 2) {
        require(i + 1 < argc && std::find(keys.begin(), keys.end(), argv[i]) != keys.end(),
                "unknown/missing option");
        require(options.emplace(argv[i], argv[i + 1]).second && !options.at(argv[i]).empty(),
                "duplicate/empty option");
    }
    require(options.count("--work-dir") && options.count("--trace-file") &&
                options.count("--stop-at"),
            "required paths/bound");
    const bool load = options.count("--load-file"), handoff = options.count("--handoff-file"),
               save = options.count("--save-file");
    require(load != handoff && bool(options.count("--handoff-certificate-sha256")) == handoff &&
                bool(options.count("--handoff-snapshot-sha256")) == handoff,
            "exclusive v2 handoff/residence restore");
    if (handoff)
        require(options.at("--handoff-certificate-sha256") == certificate_hash &&
                    options.at("--handoff-snapshot-sha256") == source_hash,
                "fixed source certificate identity");
    const auto stop = option_number(options, "--stop-at", 0),
               save_at = option_number(options, "--save-at", 0),
               tail = option_number(options, "--tail-after-save", save ? 20 : 0),
               trace_from = option_number(options, "--trace-from", 0);
    require(
        stop >= first_frame && save == bool(save_at) &&
            (!save || (tail > 0 && tail <= 1000 && save_at + tail <= stop)) &&
            (!options.count("--tail-after-save") || save) && (!save || !trace_from) &&
            (!options.count("--trace-from") || (trace_from >= first_frame && trace_from <= stop)),
        "observer relationships");
    const auto producer =
        options.count("--producer-revision") ? options.at("--producer-revision") : "unspecified";
    require(!producer.empty() && producer.size() <= 256 && producer.find('\0') == producer.npos,
            "producer identity");
    const auto root = fs::absolute(options.at("--work-dir")),
               trace = fs::absolute(options.at("--trace-file"));
    const auto live = root / "application", unused = root / "unused-current",
               source = fs::absolute(options.at(load ? "--load-file" : "--handoff-file"));
    const auto capture = save ? fs::absolute(options.at("--save-file")) : fs::path{};
    const StartupApplicationPaths unused_paths{unused};
    std::vector<fs::path> protected_paths{trace, source};
    if (save)
        protected_paths.push_back(capture);
    (void)persistence_detail::prepare_replay_capture_paths(root, root / ".residence-probe",
                                                           unused_paths, protected_paths);
    std::vector<fs::path> restore_protected{trace};
    if (save)
        restore_protected.push_back(capture);
    (void)persistence_detail::prepare_replay_restore_paths(live, source, unused_paths,
                                                           restore_protected);
    std::vector<fs::path> trace_protected{source};
    if (save)
        trace_protected.push_back(capture);
    const auto trace_target = persistence_detail::prepare_replay_capture_paths(
        root.parent_path(), trace, unused_paths, trace_protected);
    if (save)
        (void)persistence_detail::prepare_replay_capture_paths(root.parent_path(), capture,
                                                               unused_paths, {trace, source});
    const auto before_source = hash(shared::read_bounded_file(source));
    std::unique_ptr<StartupApplication> app;
    Driver d;
    const auto started = Clock::now();
    if (handoff) {
        auto old = shared::prepare_v2_handoff(source, live, unused, restore_protected);
        d = initial(old, live);
        app = std::move(old.application);
    } else {
        require(fs::create_directory(unused), "exclusive unused application");
        app = std::make_unique<StartupApplication>(unused_paths,
                                                   ref::WorldRandomStream::from_java_seed(1));
        good(app->error());
        Metadata m;
        const auto loader = [&](const StartupApplication &a, const Metadata &candidate) {
            const auto error = validate(a, candidate);
            if (!error.empty())
                return error;
            if (candidate.next_frame > stop || (save && save_at + 1 < candidate.next_frame))
                return std::string("residence observer before source");
            if (!save && !trace_from && stop - candidate.next_frame + 1 > 1000)
                return std::string("residence long restore requires trace-from");
            return std::string{};
        };
        good(restore_startup_application_replay(source, live, controller, *app, m, loader,
                                                restore_protected));
        d = decode(m.controller_state);
    }
    const auto restore_seconds = std::chrono::duration<double>(Clock::now() - started).count();
    require(before_source == hash(shared::read_bounded_file(source)),
            "source changed during handoff");
    d.producer = producer;
    good(validate(*app, metadata(d)));
    const auto driver_checks = check_bound_driver(*app, d);
    const bool entry_gift_pending = d.gift_pending != 0;
    require(d.next_frame <= stop && (!save || save_at + 1 >= d.next_frame),
            "observer before boundary");
    const auto first = d.next_frame;
    std::uint64_t captured{}, capture_rank{}, rows{};
    double capture_seconds{};
    std::size_t bytes{};
    std::ostringstream output;
    std::string capture_receipts = "null";
    std::string capture_gift_pending = "null";
    const auto capture_now = [&] {
        const auto begin = Clock::now();
        good(save_startup_application_replay(root.parent_path(), capture, *app, metadata(d),
                                             validate, {trace, source}));
        capture_seconds = std::chrono::duration<double>(Clock::now() - begin).count();
        captured = d.next_frame - 1;
        capture_rank = app->world()->state().rank;
        capture_receipts = receipts(*app, d);
        capture_gift_pending = d.gift_pending ? "true" : "false";
        std::cout << "application-residence-capture frame=" << captured
                  << " next_frame=" << d.next_frame << " bytes=" << fs::file_size(capture)
                  << std::endl;
    };
    if (save && save_at == d.next_frame - 1)
        capture_now();
    while (d.next_frame <= stop) {
        const auto old_month = d.months;
        const auto old_homes = d.homes.size();
        Round round;
        const auto old_gifts = d.gifts, old_pending = d.gift_pending;
        try {
            round = step(*app, d);
        } catch (const std::exception &e) {
            throw std::runtime_error("residence frame=" + std::to_string(d.next_frame) + ": " +
                                     e.what());
        }
        const auto frame = d.next_frame - 1,
                   begin = save         ? (captured ? captured + 1 : frame_limit + 1)
                           : trace_from ? trace_from
                                        : first;
        if (frame >= begin) {
            const auto line =
                shared::round_trace(*app, metadata(d), validate, round, live / "system.avr");
            require(line.size() <= trace_budget - bytes, "trace 8MiB budget");
            output << line;
            bytes += line.size();
            ++rows;
        }
        if (d.months != old_month || d.homes.size() != old_homes)
            std::cout << "application-residence-progress frame=" << frame
                      << " receipts=" << receipts(*app, d)
                      << " progress=" << base::progress_json(*app) << std::endl;
        if (d.gifts != old_gifts || d.gift_pending != old_pending)
            std::cout << "application-residence-gift frame=" << frame
                      << " pending=" << d.gift_pending << " committed=" << d.gifts
                      << " human=" << d.human << std::endl;
        if (save && frame == save_at)
            capture_now();
        if (captured && frame >= captured + tail)
            break;
    }
    require(!save || (captured && d.next_frame - 1 >= captured + tail), "capture/tail incomplete");
    require(before_source == hash(shared::read_bounded_file(source)),
            "source changed while replaying");
    (void)persistence_detail::prepare_replay_capture_paths(
        root.parent_path(), trace, StartupApplicationPaths{live}, trace_protected);
    const auto text = output.str();
    persistence_detail::create_save_file(trace_target.container, Bytes(text.begin(), text.end()));
    const auto sizes = base::resources(*app);
    const auto &state = app->world()->state();
    std::cout << "application-residence-summary {\"controller\":\"" << controller
              << "\",\"capture_frame\":" << (captured ? std::to_string(captured) : "null")
              << ",\"capture_rank\":" << (captured ? std::to_string(capture_rank) : "null")
              << ",\"capture_receipts\":" << capture_receipts
              << ",\"capture_gift_pending\":" << capture_gift_pending
              << ",\"completed_frame\":" << d.next_frame - 1 << ",\"next_frame\":" << d.next_frame
              << ",\"next_command\":" << d.next_command << ",\"rank\":" << state.rank
              << ",\"months\":" << d.months << ",\"date\":[" << d.date[0] << ',' << d.date[1] << ','
              << d.date[2] << ',' << d.date[3] << "],\"digest\":\""
              << startup_application_replay_digest(*app, metadata(d), validate)
              << "\",\"resources\":[";
    for (std::size_t i = 0; i < sizes.size(); ++i)
        std::cout << (i ? "," : "") << sizes[i];
    std::cout << "],\"peaks\":[";
    for (std::size_t i = 0; i < d.peaks.size(); ++i)
        std::cout << (i ? "," : "") << d.peaks[i];
    std::cout << "],\"sound_count\":" << d.sound_count << ",\"sound_hash\":\"" << d.sound_hash
              << "\",\"random\":" << d.random << ",\"phase\":" << d.homes.size()
              << ",\"terminal\":" << (terminal(*app, d) ? "true" : "false")
              << ",\"stage_complete\":" << (!d.homes.empty() ? "true" : "false")
              << ",\"gifts_committed\":" << d.gifts
              << ",\"recruitments_created\":" << d.recruitments
              << ",\"homes_admitted\":" << d.admissions
              << ",\"completed_houses\":" << d.homes.size()
              << ",\"completed_business_facilities\":" << completed_business(state)
              << ",\"gift_cash_spent\":" << d.gift_cash
              << ",\"gift_inventory_used\":" << d.gift_inventory
              << ",\"businesses_created\":" << d.businesses
              << ",\"inherited_task\":7,\"task_departed\":" << d.task_departed
              << ",\"task_successes\":" << d.task_successes
              << ",\"active_command_count\":" << d.next_command - 1 << ",\"trace_rows\":" << rows
              << ",\"driver_checks\":" << driver_checks
              << ",\"entry_gift_pending\":" << (entry_gift_pending ? "true" : "false")
              << ",\"capture_seconds\":" << capture_seconds
              << ",\"restore_seconds\":" << restore_seconds
              << ",\"progress\":" << base::progress_json(*app)
              << ",\"residence_observation\":" << shared::residence_observation(*app)
              << ",\"handoff\":" << handoff_json(d) << ",\"milestones\":{\"first_home_frame\":"
              << (d.homes.empty() ? 0 : d.homes.front().completed_frame)
              << ",\"first_home_human\":" << (d.homes.empty() ? 0 : d.homes.front().human)
              << ",\"first_home_instance\":" << (d.homes.empty() ? 0 : d.homes.front().instance)
              << ",\"completed_houses\":" << d.homes.size()
              << ",\"completed_business_facilities\":" << completed_business(state)
              << "},\"homes\":[";
    for (std::size_t i = 0; i < d.homes.size(); ++i) {
        const auto &h = d.homes[i];
        std::cout << (i ? "," : "") << "{\"human\":" << h.human << ",\"instance\":" << h.instance
                  << ",\"completed_frame\":" << h.completed_frame << '}';
    }
    std::cout << "]}\n";
    return 0;
}

int run_startup_application_residence_driver_checks(const fs::path &parent) {
    require(fs::is_directory(parent), "check parent");
    int checks{};
    for (const Bytes &bad : {Bytes{}, Bytes{'A', 'V', 'A', 'C', 'T', 'D', 'R', '2'},
                             Bytes{'A', 'V', 'R', 'E', 'S', 'D', 'R', '1'}}) {
        bool rejected{};
        try {
            (void)decode(bad);
        } catch (const std::exception &) {
            rejected = true;
        }
        require(rejected, "truncated/other-controller accepted");
        ++checks;
    }
    Metadata missing;
    require(!shared::validate_v2_origin(missing).empty(), "missing v2 origin accepted");
    ++checks;
    // 最小条件对照：kind9无连接警告位，仍必须经原可达性查询；不复制几何算法。
    StartupWorldRuntimeState condition;
    ref::RescueFacility shop;
    shop.kind = 9;
    shop.status = 1;
    shop.placement = {{1}, 1, ref::FacilityShape::single, ref::FacilityOrientation::first, {2, 0}};
    condition.scene.world.world.facilities.emplace(1, shop);
    condition.scene.world.facility_order.push_back(1);
    condition.facility_flags[1] = 0;
    const auto candidates = business_candidates(condition);
    require(candidates.size() == 1, "kind9 completed candidate with zero flags");
    ref::LegacyMap terrain{
        3, 1,
        std::vector<ref::LegacyMapCell>(3, ref::LegacyMapCell{3, ref::RouteCategory::road, {}})};
    terrain.cells[1] = {5, ref::RouteCategory::blocked, {}};
    auto bound = ref::bind_facility_map(terrain, {{shop.placement, 9}});
    require(bound.map.has_value(), "kind9 fixture binding");
    auto search = ref::search_legacy_map(*bound.map, {0, 0});
    require(search.field.has_value(), "kind9 closed path search");
    require(condition.facility_flags.at(1) == 0 &&
                connected_business_count(*search.field, candidates) == 0,
            "unreachable kind9 counted via zero flags");
    ++checks;
    bound.map->cells[1] = {3, ref::RouteCategory::road, {}};
    search = ref::search_legacy_map(*bound.map, {0, 0});
    require(search.field.has_value(), "kind9 open path search");
    require(connected_business_count(*search.field, candidates) == 1,
            "reachable completed kind9 omitted");
    ++checks;
    condition.scene.world.world.facilities.at(1).status = 0;
    require(business_candidates(condition).empty(), "construction counted as completed business");
    ++checks;
    return checks;
}
