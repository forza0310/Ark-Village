#include "dungeon_village_prototype/startup_world_runtime.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
ref::WorldCalendarMaintenanceState maintenance(const State &s) {
    ref::WorldCalendarMaintenanceState r;
    r.random = s.scene.random;
    for (const auto &h : s.rules->humans) {
        const auto id = h.identity;
        const auto &extra = s.human_calendar.at(id);
        const auto &definition = s.scene.world.world.ai.growth.at(id).definition;
        auto totals = std::vector<std::int32_t>(extra.yearly_totals.begin(), extra.yearly_totals.end());
        totals[0] = s.scene.world.world.ai.battle.humans.at(id).kills;
        totals[1] = s.scene.world.world.ai.battle.humans.at(id).killed_stat1;
        totals[2] = s.scene.world.world.human_spending.at(id);
        const auto &cooldowns = s.shop_humans.at(id).reselect;
        r.humans.push_back({id, s.human_presence.at(id), s.human_definition_state.at(id),
                            extra.absent_months, {cooldowns.begin(), cooldowns.end()}, totals,
                            s.human_homes.at(id)[2],
                            s.rules->jobs.at(definition.current_profession).type,
                            s.scene.world.world.ai.battle.humans.at(id).battle_reward_stat, extra.legacy_G});
    }
    for (const auto id : s.scene.world.world.ai.human_order)
        r.active_human_definitions.push_back(s.scene.world.world.ai.battle.actors.at(id).definition);
    for (const auto &d : s.rules->facilities)
        r.facility_definitions.push_back({d.id, d.kind,
                                         s.facility_definitions.at(d.id).popularity_reward});
    for (const auto id : s.scene.world.facility_order) {
        const auto &cash = s.facility_monthly_cash.at(id);
        r.facilities.push_back({id, s.scene.world.world.facilities.at(id).placement.definition_id,
                                s.facility_month_age.at(id), {cash.begin(), cash.end()}});
    }
    for (const auto &i : s.rules->items)
        r.shop_items.push_back(s.shop_item_stock.at(i.identity));
    for (const auto &m : s.rules->monsters)
        r.monster_month_kills.push_back(s.scene.world.world.ai.monster_growth.at(m.identity).defeats);
    for (const auto &a : s.activity_flags)
        r.item_flags.push_back(a.second); // 原bx/a.c活动目录，不是by物品。
    r.kill_display = s.scene.world.world.ai.battle.defeated_definitions;
    r.monthly_cash = s.monthly_cash;
    r.yearly_statistics = s.yearly_statistics;
    r.rank = s.rank;
    r.popularity = s.popularity;
    r.legacy_v = s.task_progress.successes;
    r.legacy_D = s.legacy_D;
    r.legacy_n0 = s.legacy_n[0];
    r.quarter_base = s.fence_level;
    r.quarter_counter = s.quarter_counter;
    r.shop_refresh_enabled = (s.scripts.user_flags & 16) != 0;
    r.event203_seen = ref::world_script_seen(s.scripts, 203);
    return r;
}
bool write_maintenance(State &s, const ref::WorldCalendarMaintenanceState &r) {
    s.scene.random = r.random;
    for (const auto &h : r.humans) {
        if (h.equipment.size() != 4 || h.yearly_totals.size() != 3)
            return false;
        s.human_presence.at(h.definition) = h.presence;
        s.human_definition_state.at(h.definition) = h.leave_months;
        auto &extra = s.human_calendar.at(h.definition);
        extra.absent_months = h.absent_months;
        std::copy(h.yearly_totals.begin(), h.yearly_totals.end(), extra.yearly_totals.begin());
        s.scene.world.world.human_spending.at(h.definition) = h.yearly_totals[2];
        auto &battle = s.scene.world.world.ai.battle.humans.at(h.definition);
        battle.kills = h.yearly_totals[0];
        battle.killed_stat1 = h.yearly_totals[1];
        battle.battle_reward_stat = h.legacy_F;
        std::copy(h.equipment.begin(), h.equipment.end(), s.shop_humans.at(h.definition).reselect.begin());
        s.human_homes.at(h.definition)[2] = h.residence_status;
        extra.legacy_F = h.legacy_F;
        extra.legacy_G = h.legacy_G;
    }
    for (const auto &d : r.facility_definitions)
        s.facility_definitions.at(d.definition).popularity_reward = d.month_counter;
    for (const auto &f : r.facilities) {
        if (f.yearly_cash.size() != 12)
            return false;
        s.facility_month_age.at(f.identity) = f.month_age;
        std::copy(f.yearly_cash.begin(), f.yearly_cash.end(), s.facility_monthly_cash.at(f.identity).begin());
    }
    for (const auto &i : r.shop_items)
        s.shop_item_stock.at(i.definition) = i;
    if (r.monster_month_kills.size() != s.rules->monsters.size() ||
        r.item_flags.size() != s.activity_flags.size())
        return false;
    for (std::size_t n = 0; n < r.monster_month_kills.size(); ++n)
        s.scene.world.world.ai.monster_growth.at(s.rules->monsters[n].identity).defeats =
            r.monster_month_kills[n];
    std::size_t n{};
    for (auto &activity : s.activity_flags)
        activity.second = r.item_flags[n++];
    s.scene.world.world.ai.battle.defeated_definitions = r.kill_display;
    s.monthly_cash = r.monthly_cash;
    s.yearly_statistics = r.yearly_statistics;
    s.quarter_counter = r.quarter_counter;
    return true;
}
ref::WorldCalendarTasksState tasks(const State &s) {
    ref::WorldCalendarTasksState r;
    r.finish = startup_world_runtime_finish(s);
    r.scripts = startup_world_runtime_scripts(s);
    r.random = s.scene.random;
    for (const auto &h : s.human_calendar)
        r.human_details.emplace(h.first, ref::CalendarTaskHuman{
            s.scene.world.world.ai.growth.at(h.first).definition.profession_levels,
            h.second.continuation_cost});
    for (const auto &t : s.rules->tasks)
        r.task_details.emplace(t.factory.identity,
                               ref::CalendarTaskDefinitionDetails{t.title, t.name});
    for (const auto &m : s.rules->monsters)
        r.monster_details.emplace(m.identity, ref::CalendarTaskMonsterDetails{
            m.name, m.initial.sprite_variant,
            s.scene.world.world.ai.monster_growth.at(m.identity).growth});
    r.facility_definition_presence = s.facility_presence;
    r.rank_terms = ref::fixed_calendar_task_rank_terms();
    r.facility_order = s.scene.world.facility_order;
    r.rank = s.rank;
    r.highest_month_income = s.maximum_income;
    r.popularity = s.popularity;
    r.events_held = s.events_held;
    r.rank_met = s.rank_met;
    r.rank_values = s.rank_values;
    r.task_subperiods = s.task_subperiods;
    r.generation_retry = s.generation_retry;
    r.deadline_page = s.deadline_page;
    r.completion_mode = s.completion_mode;
    r.system_completion_mode = s.system_completion_mode;
    r.system_unlock_data = s.system_unlock_data;
    return r;
}
bool write_tasks(State &s, const ref::WorldCalendarTasksState &r) {
    if (!write_startup_world_runtime_finish(s, r.finish) ||
        !write_startup_world_runtime_scripts(s, r.scripts))
        return false;
    s.scene.random = r.random;
    for (const auto &h : r.human_details)
        s.human_calendar.at(h.first).continuation_cost = h.second.continuation_cost;
    s.rank = r.rank;
    s.rank_met = r.rank_met;
    s.rank_values = r.rank_values;
    s.task_subperiods = r.task_subperiods;
    s.generation_retry = r.generation_retry;
    s.deadline_page = r.deadline_page;
    s.completion_mode = r.completion_mode;
    s.system_completion_mode = r.system_completion_mode;
    s.system_unlock_data = r.system_unlock_data;
    return true;
}
} // namespace
bool refresh_startup_world_runtime_rank(State &state) {
    const auto status = ref::prepare_world_rank_status(tasks(state));
    if (!status)
        return false;
    state.rank_met = status->met;
    state.rank_values = status->values;
    return true;
}
void configure_startup_world_runtime_calendar_adapter(ref::WorldRuntimeAdapter<State> &a) {
    a.maintenance = {maintenance, write_maintenance};
    a.tasks = {tasks, write_tasks};
    const auto catalog = a.catalog;
    a.calendar_other = [catalog](const State &s, ref::WorldCalendarStage stage) -> std::optional<State> {
        auto next = s;
        if (stage == ref::WorldCalendarStage::year_refresh) {
            next.monthly_cash = {}; // UserData.i()。
            return next;
        }
        if (stage != ref::WorldCalendarStage::subperiod_refresh)
            return {}; // checkpoint由会话本次私有memory_checkpoint消费者提供。
        // UserData.b(true)：按全部bv原序，只读取p/D2/flags2/C/g；不创建人物或住宅。
        for (const auto &d : next.rules->facilities)
            if (d.kind == 13)
                next.residence_catalog_available.at(d.id) = false;
        bool any{};
        auto scripts = startup_world_runtime_scripts(next);
        for (const auto &human : next.rules->humans) {
            const int id = human.identity;
            auto &flags = next.human_flags.at(id);
            if (next.human_presence.at(id) == 0 || next.human_homes.at(id)[2] != 0 ||
                (flags & 2U) != 0 || next.shop_humans.at(id).satisfaction < human.residence_threshold)
                continue;
            any = true;
            if ((flags & 4U) != 0)
                continue;
            flags |= 4U;
            auto programs = catalog;
            auto program = human.residence_request_program;
            // 原外层a(program,...,0,1,human)仅给没有k的对话填j/k；保留原指定说话者。
            for (auto &command : program)
                if (command.size() == 2 && command[0] == 2) {
                    const int talk = command[1];
                    if (talk < 0 || talk >= static_cast<int>(catalog.talks.size()))
                        return {};
                    if (catalog.talks.at(talk).speaker_definition == -1)
                        command = {3, talk, 1, id};
                }
            const int identity = 2700 + id; // 研究外部程序路由身份，不登记aL事件调用。
            programs.programs.emplace(identity, std::move(program));
            const auto result = ref::prepare_world_script_program(programs, scripts, {identity, {}, {}});
            if (!result.candidate)
                return {};
            scripts = result.candidate->state;
            for (const auto &page : result.candidate->inserted_pages)
                next.page_human_bindings[page.id] = id;
            const bool exists = std::any_of(scripts.notices.begin(), scripts.notices.end(),
                [](const auto &notice) { return notice.message == 20; });
            if (!exists) {
                scripts.notices.push_back({20, -20, 80, "", "想要入住的人增加了"});
                next.residence_hint_counter = 0;
            }
        }
        if (any) {
            for (const auto &d : next.rules->facilities)
                if (d.kind == 13)
                    next.residence_catalog_available.at(d.id) = true;
            const bool built = std::any_of(next.scene.world.world.facilities.begin(),
                next.scene.world.world.facilities.end(), [](const auto &f) { return f.second.kind == 13; });
            auto invoke = [&](int id) {
                const auto result = ref::prepare_world_script(catalog, scripts, {id, {}, {}});
                if (!result.candidate)
                    return false;
                scripts = result.candidate->state;
                return true;
            };
            if (!ref::world_script_seen(scripts, 82)) {
                if (!invoke(82) || !write_startup_world_runtime_scripts(next, scripts))
                    return {};
                return next; // 原初次82之后立即return，不推进H。
            }
            if (!built) {
                if (!ref::world_script_seen(scripts, 83) && !invoke(83))
                    return {};
                if (next.residence_hint_counter == std::numeric_limits<int>::max())
                    return {};
                if (++next.residence_hint_counter >= 8) {
                    next.residence_hint_counter = 0;
                    scripts.notices.push_back({19, -1, 80, "", "有想要入住的冒险者。"});
                }
            }
        }
        if (!write_startup_world_runtime_scripts(next, scripts))
            return {};
        return next;
    };
    a.endgame_checkpoint = [](const State &s) -> std::optional<State> {
        auto next = s;
        std::array<std::vector<std::uint8_t>, 2> data;
        auto short_value = [](std::vector<std::uint8_t> &bytes, int value) {
            const auto narrowed = static_cast<std::uint16_t>(value);
            bytes.push_back(static_cast<std::uint8_t>(narrowed >> 8));
            bytes.push_back(static_cast<std::uint8_t>(narrowed));
        };
        for (const auto &d : s.rules->facilities)
            short_value(data[0], s.scene.world.world.facility_uses.at(d.id).level);
        for (std::size_t n = 0; n < s.rules->jobs.size(); ++n)
            short_value(data[1], s.scripts.professions.at(static_cast<int>(n)).status);
        next.system_unlock_data = std::move(data); // UserData.C真实J.e[0/1]大端short顺序。
        return next;
    };
    a.calendar_request = [catalog](const State &s, const ref::CalendarMaintenanceRequest &request)
        -> std::optional<State> {
        auto next = s;
        auto scripts = startup_world_runtime_scripts(next);
        if (request.kind == ref::CalendarMaintenanceRequestKind::script) {
            const auto result = ref::prepare_world_script(catalog, scripts,
                {request.code, request.number ? std::optional<std::string>(std::to_string(*request.number))
                                             : std::nullopt, {}});
            if (!result.candidate)
                return {};
            scripts = result.candidate->state;
        } else if (request.kind == ref::CalendarMaintenanceRequestKind::page) {
            ref::WorldScriptPage page;
            page.kind = ref::WorldScriptPageKind::raw_page;
            page.legacy_page = request.code;
            const auto result = ref::prepare_world_script_page(scripts, page);
            if (!result.candidate)
                return {};
            scripts = result.candidate->state;
        } else {
            // 16/17补货提示，原az16=-1/80时点；目录已经由maintenance实际提交。
            const std::string text = request.code == 16 ? "商店进了新的道具"
                                     : request.code == 17 ? "进了新的道具" : "";
            if (text.empty())
                return {};
            scripts.notices.push_back({request.code, -request.delay, 80, "", text});
        }
        if (!write_startup_world_runtime_scripts(next, scripts))
            return {};
        return next;
    };
}
} // namespace dungeon_village_prototype
