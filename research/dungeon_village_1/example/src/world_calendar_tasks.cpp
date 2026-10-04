#include "dungeon_village_reference/world_calendar_tasks.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_reference {
namespace {
struct Failure {
    CalendarTaskError error;
};
[[noreturn]] void fail(CalendarTaskError error) { throw Failure{error}; }
bool fits(std::int64_t value) {
    return value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max();
}
void validate(const WorldCalendarTasksState &state, const WorldCalendarState &date,
              const WorldScriptCatalog &catalog) {
    if (!valid_world_calendar_state(date) || state.rank < 0 || state.rank > 5 ||
        state.finish.event_calls != state.scripts.event_calls ||
        state.finish.dungeon.world.ai.pending_completion != state.scripts.pending_completion)
        fail(CalendarTaskError::invalid_owner);
    if (!prepare_world_script_continuations(catalog, state.scripts, false).candidate)
        fail(CalendarTaskError::invalid_owner);
    for (auto identity : state.finish.task_order)
        if (!identity || !state.finish.tasks.count(identity))
            fail(CalendarTaskError::invalid_owner);
    if (state.finish.active_task && !state.finish.tasks.count(*state.finish.active_task))
        fail(CalendarTaskError::invalid_owner);
    for (auto identity : state.facility_order)
        if (!identity || !state.finish.dungeon.world.facilities.count(identity))
            fail(CalendarTaskError::invalid_owner);
}
struct Runner {
    CalendarTaskCandidate &candidate;
    const WorldScriptCatalog &catalog;
    const CalendarTaskExternalConsumer &external;
    void synchronize() {
        candidate.state.finish.event_calls = candidate.state.scripts.event_calls;
        candidate.state.finish.dungeon.world.ai.pending_completion =
            candidate.state.scripts.pending_completion;
    }
    void invoke(int identity, std::optional<std::string> replacement = {}) {
        const auto result = prepare_world_script(catalog, candidate.state.scripts,
                                                 {identity, std::move(replacement), {}});
        if (!result.candidate)
            fail(CalendarTaskError::script_failed);
        candidate.state.scripts = result.candidate->state;
        synchronize();
        candidate.scripts.insert(candidate.scripts.end(), result.candidate->executed.begin(),
                                 result.candidate->executed.end());
        candidate.pages.insert(candidate.pages.end(), result.candidate->inserted_pages.begin(),
                               result.candidate->inserted_pages.end());
    }
    std::uint64_t page(WorldScriptPage value) {
        const auto result = prepare_world_script_page(candidate.state.scripts, value);
        if (!result.candidate || !result.candidate->last_page)
            fail(CalendarTaskError::script_failed);
        candidate.state.scripts = result.candidate->state;
        candidate.pages.insert(candidate.pages.end(), result.candidate->inserted_pages.begin(),
                               result.candidate->inserted_pages.end());
        return *result.candidate->last_page;
    }
    WorldScriptPage raw(int identity) {
        WorldScriptPage value;
        value.kind = WorldScriptPageKind::raw_page;
        value.legacy_page = identity;
        return value;
    }
    int draw(int bound) {
        const auto result = candidate.state.random.draw(bound);
        if (result.error != WorldRandomError::none)
            fail(CalendarTaskError::random_failed);
        return result.ticket;
    }
    const DungeonTaskDefinitionProgress &definition(int identity) {
        const auto found = candidate.state.finish.task_progress.definitions.find(identity);
        if (found == candidate.state.finish.task_progress.definitions.end())
            fail(CalendarTaskError::invalid_owner);
        return found->second;
    }
    const CalendarTaskDefinitionDetails &details(int identity) {
        const auto found = candidate.state.task_details.find(identity);
        if (found == candidate.state.task_details.end())
            fail(CalendarTaskError::invalid_owner);
        return found->second;
    }
    void notice(int identity, int counter, const std::string &replacement = "",
                std::vector<int> definitions = {}) {
        std::string text;
        switch (identity) {
        case 13:
        case 14:
            text = "任务 <co=0064FF>" + replacement + "</co> 追加";
            break;
        case 27:
            text = "任务期限 还剩1个月!";
            break;
        case 32:
            text = "大暴走!! 街道人气<co=FF0E01>-10</co>";
            break;
        default:
            fail(CalendarTaskError::invalid_owner);
        }
        candidate.state.scripts.notices.push_back(
            {identity, counter, 80, replacement, std::move(text), std::move(definitions)});
    }
    std::optional<std::uint64_t> consume(CalendarTaskExternalRequest request,
                                         const WorldCalendarState &date) {
        if (!external)
            fail(CalendarTaskError::missing_consumer);
        const auto next = external(candidate.state, request);
        if (!next)
            fail(CalendarTaskError::consumer_failed);
        validate(next->state, date, catalog);
        candidate.state = next->state;
        candidate.external_calls.push_back(request);
        return next->created_task;
    }
    void actor_dialogue(int script, std::optional<std::string> replacement, int actor_definition) {
        const auto before = candidate.state.scripts.next_page_id;
        invoke(script, std::move(replacement));
        if (candidate.state.scripts.next_page_id == before)
            fail(CalendarTaskError::script_failed);
        // 这些固定入口只返回一张即时对话；不是给队首等待脚本猜一个返回页。
        const auto id = candidate.state.scripts.next_page_id - 1;
        const auto found =
            std::find_if(candidate.state.scripts.pages.begin(), candidate.state.scripts.pages.end(),
                         [&](const auto &value) { return value.id == id; });
        if (found == candidate.state.scripts.pages.end()) {
            if (candidate.state.scripts.page_mutations_locked)
                return; // 原调用者仍写新页对象的说话者，但框架l锁未把对象加入栈。
            else
                fail(CalendarTaskError::script_failed);
        }
        found->speaker_kind = 1;
        found->speaker_definition = actor_definition;
        for (auto &value : candidate.pages)
            if (value.id == id) {
                value.speaker_kind = 1;
                value.speaker_definition = actor_definition;
            }
    }
    void monster_dialogue(int script, int monster) {
        actor_dialogue(script, {}, monster);
        const auto id = candidate.state.scripts.next_page_id - 1;
        for (auto &value : candidate.state.scripts.pages)
            if (value.id == id)
                value.speaker_kind = 2;
        for (auto &value : candidate.pages)
            if (value.id == id)
                value.speaker_kind = 2;
    }
};
} // namespace

std::map<int, CalendarTaskRankTerms> fixed_calendar_task_rank_terms() {
    return {{0, {{5, 300}, {0, 5000}, {6, 2}, {3, 35}}},
            {1, {{5, 800}, {1, 10}, {2, 4}, {4, 12}}},
            {2, {{5, 1500}, {0, 35000}, {6, 15}, {3, 40}}},
            {3, {{5, 2500}, {1, 25}, {2, 10}, {4, 30}}},
            {4, {{5, 3500}, {0, 70000}, {6, 30}, {3, 64}}}};
}
CalendarTaskResult prepare_world_calendar_tasks(const WorldCalendarTasksState &state,
                                                const WorldCalendarState &date,
                                                WorldCalendarStage stage,
                                                const WorldScriptCatalog &catalog,
                                                const CalendarTaskExternalConsumer &external) {
    try {
        validate(state, date, catalog);
        CalendarTaskCandidate candidate{state, {}, {}, {}, {}, {}, 0};
        Runner runner{candidate, catalog, external};
        auto &next = candidate.state;
        switch (stage) {
        case WorldCalendarStage::month_special_scripts:
            if (date.year == 15 && date.month == 3) {
                runner.invoke(3, "15");
                runner.page(runner.raw(17));
                next.completion_mode = 1;
                next.system_completion_mode = 1;
                if (next.scripts.finance)
                    next.scripts.finance->legacy_flags14 = 1;
                runner.consume({CalendarTaskExternalKind::endgame_checkpoint, 0}, date);
            }
            if (date.month == 11)
                runner.invoke(26);
            break;
        case WorldCalendarStage::month_rank_check: {
            if (next.rank >= 5)
                break; // 原a.n.a在重置i/j之前直接false。
            const auto found = next.rank_terms.find(next.rank);
            if (found == next.rank_terms.end() || found->second.size() > 4)
                fail(CalendarTaskError::invalid_owner);
            next.rank_met.fill(false);
            next.rank_values.fill(0);
            int facilities{}, houses{};
            for (auto identity : next.facility_order) {
                const auto &facility = next.finish.dungeon.world.facilities.at(identity);
                if (facility.kind == 3 || facility.kind == 9)
                    ++facilities;
                if (facility.kind == 12)
                    ++houses;
            }
            for (std::size_t index = 0; index < found->second.size(); ++index) {
                const auto &term = found->second[index];
                int value{};
                switch (term.type) {
                case 0:
                    value = next.highest_month_income;
                    break;
                case 1:
                    value = facilities;
                    break;
                case 2:
                    value = houses;
                    break;
                case 3:
                    next.rank_met[index] = std::any_of(
                        next.facility_order.begin(), next.facility_order.end(), [&](auto identity) {
                            return next.finish.dungeon.world.facilities.at(identity)
                                       .placement.definition_id == term.threshold;
                        });
                    continue;
                case 4:
                    value = next.finish.task_progress.successes;
                    break;
                case 5:
                    // f214d由同一外层Owner提供，不从人气待结算队列或现金推出。
                    value = next.popularity;
                    break;
                case 6:
                    value = next.events_held;
                    break;
                default:
                    continue;
                }
                next.rank_values[index] = value;
                next.rank_met[index] = value >= term.threshold;
            }
            if (next.rank_bypass == 1 || std::all_of(next.rank_met.begin(), next.rank_met.end(),
                                                     [](bool value) { return value; })) {
                runner.invoke(36);
                runner.page(runner.raw(48));
            }
            break;
        }
        case WorldCalendarStage::subperiod_task_deadline:
            if (next.finish.active_task) {
                if (next.task_subperiods == std::numeric_limits<int>::max())
                    fail(CalendarTaskError::overflow);
                ++next.task_subperiods;
                if (next.task_subperiods >= 12) {
                    std::int64_t total{};
                    for (int identity : next.finish.participants) {
                        const auto found = next.human_details.find(identity);
                        if (found == next.human_details.end())
                            fail(CalendarTaskError::invalid_owner);
                        std::int64_t cost = 200;
                        for (int level : found->second.profession_levels) {
                            const auto extra = (static_cast<std::int64_t>(level) - 1) * 50;
                            if (!fits(extra) || !fits(cost + extra))
                                fail(CalendarTaskError::overflow);
                            cost += extra;
                        }
                        found->second.continuation_cost =
                            static_cast<int>(std::min<std::int64_t>(cost, 3000));
                        total += found->second.continuation_cost;
                        if (!fits(total))
                            fail(CalendarTaskError::overflow);
                    }
                    runner.invoke(65, "3");
                    auto page = runner.raw(33);
                    page.legacy_f = static_cast<int>(total);
                    page.task_identity = next.finish.active_task;
                    page.task_definition =
                        next.finish.tasks.at(*next.finish.active_task).definition;
                    next.deadline_page = runner.page(std::move(page));
                }
                if (next.task_subperiods == 8)
                    runner.notice(27, -1);
            }
            break;
        case WorldCalendarStage::subperiod_task_midpoint:
            if (date.subperiod == 2)
                for (auto identity : next.finish.task_order) {
                    const auto &task = next.finish.tasks.at(identity);
                    const auto &definition = runner.definition(task.definition);
                    if ((definition.flags & 2) == 0)
                        continue;
                    runner.notice(32, 1, "", {definition.monster_definition});
                    next.scripts.popularity_queue.insert(next.scripts.popularity_queue.begin(),
                                                         {85, -10, 1});
                    break;
                }
            break;
        case WorldCalendarStage::subperiod_task_generation: {
            if (date.subperiod != 1 && date.subperiod != 3)
                break;
            bool admitted = date.month % 2 == 0 || next.generation_retry;
            if (next.generation_retry)
                next.generation_retry = false;
            if (next.finish.task_order.empty())
                admitted = true;
            if (date.year == 0 && date.month + 1 <= 6)
                admitted = false;
            if (!admitted || next.finish.task_order.size() >= 3)
                break;
            int explorations{}, battles{};
            for (auto identity : next.finish.task_order) {
                const auto &definition =
                    runner.definition(next.finish.tasks.at(identity).definition);
                if (definition.kind == 0)
                    ++explorations;
                else if (definition.kind == 1)
                    ++battles;
            }
            int kind = explorations > battles   ? 1
                       : explorations < battles ? 0
                       : runner.draw(100) < 50  ? 0
                                                : 1;
            if (explorations == battles && (next.scripts.user_flags & 16) == 0)
                kind = 1;
            if (next.finish.task_progress.task_pool_progress >= 500)
                kind = 1;
            if (date.year == 0 && date.month + 1 <= 8)
                kind = explorations == 0 ? 0 : -1;
            if (kind == -1)
                break;
            candidate.generation_kind = kind;
            const auto old_tasks = next.finish.tasks;
            const auto old_order = next.finish.task_order;
            const auto created =
                runner.consume({CalendarTaskExternalKind::create_task, kind}, date);
            if (next.finish.task_order.size() != old_order.size() + (created ? 1 : 0) ||
                !std::equal(old_order.begin(), old_order.end(), next.finish.task_order.begin()) ||
                next.finish.tasks.size() != old_tasks.size() + (created ? 1 : 0))
                fail(CalendarTaskError::consumer_failed);
            if (!created) {
                next.generation_retry = true;
                break;
            }
            if (old_tasks.count(*created) || !next.finish.tasks.count(*created) ||
                next.finish.task_order.back() != *created)
                fail(CalendarTaskError::consumer_failed);
            const auto task = next.finish.tasks.at(*created);
            const auto definition = runner.definition(task.definition);
            const auto detail = runner.details(task.definition);
            if (definition.kind != kind)
                fail(CalendarTaskError::consumer_failed);
            if (kind == 0) {
                if (!task.facility || !task.site ||
                    !next.finish.dungeon.world.facilities.count(*task.facility) ||
                    !next.finish.dungeon.facilities.count(*task.facility))
                    fail(CalendarTaskError::consumer_failed);
                const auto &facility = next.finish.dungeon.world.facilities.at(*task.facility);
                if (facility.category != 5 || !(facility.placement.anchor == *task.site))
                    fail(CalendarTaskError::consumer_failed);
            }
            runner.notice(kind + 13, -80, detail.displayed_name);
            next.scripts.user_flags |= 4;
            if (!next.scripts.human_catalog_complete)
                fail(CalendarTaskError::invalid_owner);
            std::vector<int> humans;
            for (const auto &entry : next.scripts.humans)
                if (entry.second.status != 0)
                    humans.push_back(entry.first);
            const int selected =
                humans.at(static_cast<std::size_t>(runner.draw(static_cast<int>(humans.size()))));
            if (kind == 0) {
                if (world_script_seen(next.scripts, 59))
                    runner.actor_dialogue(132, detail.title, selected);
                else
                    runner.invoke(59);
            } else if ((definition.flags & 2) != 0 || (definition.flags & 4) != 0) {
                const auto monster = next.monster_details.find(definition.monster_definition);
                if (monster == next.monster_details.end())
                    fail(CalendarTaskError::invalid_owner);
                const bool boss = (definition.flags & 2) != 0;
                runner.actor_dialogue(boss ? 134 : 133,
                                      boss ? std::optional<std::string>{}
                                           : std::optional<std::string>{detail.title},
                                      selected);
                auto page = runner.raw(boss ? 99 : 100);
                page.monster_definition = definition.monster_definition;
                runner.page(std::move(page));
                if (boss) {
                    const auto first_script =
                        static_cast<std::int64_t>(monster->second.family) + 139;
                    if (!fits(first_script))
                        fail(CalendarTaskError::overflow);
                    if (!world_script_seen(next.scripts, static_cast<int>(first_script)))
                        runner.invoke(static_cast<int>(first_script));
                    runner.invoke(138, monster->second.name);
                    if (!world_script_seen(next.scripts, 165) && monster->second.victories > 0)
                        runner.monster_dialogue(165, definition.monster_definition);
                    if (!world_script_seen(next.scripts, 166) && monster->second.victories >= 9)
                        runner.monster_dialogue(166, definition.monster_definition);
                    next.finish.task_progress.task_pool_progress = 0;
                } else if (definition.completed > 0 && !world_script_seen(next.scripts, 167))
                    runner.monster_dialogue(167, definition.monster_definition);
            } else if (world_script_seen(next.scripts, 60))
                runner.actor_dialogue(133, detail.title, selected);
            else
                runner.invoke(60);
            candidate.created_task = created;
            break;
        }
        case WorldCalendarStage::subperiod_capacity_hint:
            if (date.subperiod == 3 && !world_script_seen(next.scripts, 163) && next.rank == 3) {
                if (!next.scripts.facility_catalog_complete)
                    fail(CalendarTaskError::invalid_owner);
                int available{};
                for (const auto &definition : next.facility_definition_presence)
                    if (definition.second != 0)
                        ++available;
                std::int64_t count{};
                for (auto identity : next.facility_order)
                    if (next.finish.dungeon.world.facilities.at(identity).kind == 3)
                        ++count;
                if (!fits(count * 100))
                    fail(CalendarTaskError::overflow);
                if (available > 0 && (count * 100) / available < 150)
                    runner.invoke(163, next.scripts.village_name + "\t" + std::to_string(count));
            }
            break;
        default:
            return {CalendarTaskError::unsupported_stage, {}};
        }
        if (next.random.draws() < state.random.draws())
            fail(CalendarTaskError::consumer_failed);
        candidate.random_draws = next.random.draws() - state.random.draws();
        return {CalendarTaskError::none, std::move(candidate)};
    } catch (const Failure &failure) {
        return {failure.error, {}};
    }
}
} // namespace dungeon_village_reference
