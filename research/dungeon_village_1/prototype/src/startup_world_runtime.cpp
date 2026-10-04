#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_prototype/startup_world_routes.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
bool project_human_task_flags(ref::RescueWorldState &world,
                              const std::map<int, std::uint32_t> &flags) {
    for (const auto &actor : world.ai.battle.actors) {
        if (actor.second.kind != ref::ActorKind::human)
            continue;
        const auto current = flags.find(actor.second.definition);
        const auto context = world.actors.find(actor.first);
        if (current == flags.end() || context == world.actors.end())
            return false;
        context->second.definition_task_flag = (current->second & 2U) != 0;
    }
    return true;
}
ref::WorldCombatFacingConsumer facing_provider(const State &s) {
    return [metadata = s.actor_metadata](ref::CharacterId self,
                                         ref::CharacterId target) -> std::optional<int> {
        const auto a = metadata.find(self), t = metadata.find(target);
        if (a == metadata.end() || t == metadata.end())
            return {};
        return t->second.cached_view.y > a->second.cached_view.y
                   ? (t->second.cached_view.x > a->second.cached_view.x ? 0 : 3)
                   : (t->second.cached_view.x > a->second.cached_view.x ? 1 : 2);
    };
}
bool post_finance(State &s, const ref::WorldScriptFinance &finance) {
    auto &ai = s.scene.world.world.ai;
    const auto old = ai.accounting.funds();
    if ((old < 0 && finance.cash > std::numeric_limits<std::int64_t>::max() + old) ||
        (old > 0 && finance.cash < std::numeric_limits<std::int64_t>::min() + old))
        return false;
    const auto delta = finance.cash - old;
    if (delta == std::numeric_limits<std::int64_t>::min())
        return false;
    if (delta != 0) {
        if (ai.next_cash_id == std::numeric_limits<std::uint64_t>::max() ||
            ai.accounting.post_cash(
                {ai.next_cash_id, s.simulation_steps + 1, ref::CashCategory::other,
                 delta > 0 ? ref::CashDirection::income : ref::CashDirection::expense,
                 delta > 0 ? delta : -delta}) != ref::AccountingError::none)
            return false;
        ++ai.next_cash_id;
    }
    s.cash_peak = finance.cash_peak;
    s.cash_peak_village = finance.cash_peak_village;
    s.monthly_cash = finance.monthly_totals;
    return true;
}
StartupWorldRouteFacts facts(const State &s) {
    StartupWorldRouteFacts f;
    f.rules = s.rules;
    f.surface = s.surface;
    f.exits = startup_evidence().spawn_points; // 固定h.f加载后原序。
    f.human_homes = s.human_homes;
    f.neighbourhood = s.neighbourhood;
    f.actor_metadata = s.actor_metadata;
    f.calendar = {s.scene.calendar.year, s.scene.calendar.month, s.scene.calendar.subperiod,
                  s.scene.calendar.units};
    f.actor_box = ref::CollisionBox{-4, 4, 4, 4};  // n.a(0,0)，ah0矩形左/下角。
    f.rescue_box = ref::CollisionBox{-4, 4, 4, 4}; // n.a(1,2)，ah2同框。
    f.object_box = ref::CollisionBox{-4, 4, 4, 4}; // n.a(1,0)，不是渲染尺寸。
    for (const auto id : s.task_order) {
        const auto &task = s.tasks.at(id);
        if (task.site)
            f.tasks.push_back({*task.site, s.task_progress.definitions.at(task.definition).kind});
    }
    f.job_counts = s.scripts.job_counts;
    for (const auto &d : s.scripts.facilities)
        f.facility_improvements.emplace(d.first, d.second.improvements);
    for (const auto &a : s.scene.world.world.ai.battle.actors) {
        f.facing.emplace(a.first, a.second.control.facing);
        const auto visible = startup_world_hit_sound_visible(s, a.first);
        if (visible)
            f.actor_visible.emplace(a.first, *visible);
    }
    f.sound_projection = [camera = s.camera, viewport = s.reference_viewport](
                             ref::Position raw) -> std::optional<ref::Position> {
        const int middle_x = (viewport[0] + viewport[2]) / 2;
        const int middle_y = (viewport[1] + viewport[3]) / 2;
        return ref::Position{middle_x + raw.x - static_cast<int>(camera[0]),
                             viewport[1] + viewport[3] -
                                 (middle_y + raw.y - static_cast<int>(camera[1]))};
    };
    return f;
}
std::optional<State> script(const State &s, int id) {
    const auto result = ref::prepare_world_script(startup_world_runtime_catalog(),
                                                  startup_world_runtime_scripts(s), {id, {}, {}});
    if (!result.candidate)
        return {};
    auto next = s;
    if (!write_startup_world_runtime_scripts(next, result.candidate->state))
        return {};
    return next;
}
ref::WorldMonthReportState report(const State &s) {
    ref::WorldMonthReportState r;
    r.presentation = startup_world_runtime_scripts(s);
    r.display_state = s.report_state;
    r.display_counter = s.report_counter;
    r.snapshot = s.report_snapshot;
    r.records = s.report_records;
    r.new_records = s.report_new_records;
    r.monster_portraits = s.report_portraits;
    r.kill_display = s.scene.world.world.ai.battle.defeated_definitions;
    r.maximum_income = s.maximum_income;
    r.village_points = s.village_points;
    for (const auto &m : s.rules->monsters)
        r.monsters.push_back(
            {s.scene.world.world.ai.monster_growth.at(m.identity).defeats, m.points_per_defeat});
    for (const auto &h : s.scene.world.world.ai.growth) {
        const auto &d = h.second.definition;
        r.humans.emplace(
            h.first, ref::MonthReportHumanDefinition{d.profession_levels.at(d.current_profession),
                                                     s.rules->jobs.at(d.current_profession).fee});
    }
    for (const auto id : s.scene.world.world.ai.human_order)
        r.active_humans.push_back(s.scene.world.world.ai.battle.actors.at(id).definition);
    for (const auto &d : s.scripts.facilities)
        r.facility_fee_slot3.emplace(d.first, d.second.attributes[3]);
    for (const auto id : s.scene.world.facility_order)
        r.facilities.push_back({id, s.scene.world.world.facilities.at(id).placement.definition_id,
                                s.facility_monthly_cash.at(id)});
    return r;
}
bool write_report(State &s, const ref::WorldMonthReportState &r) {
    if (!write_startup_world_runtime_scripts(s, r.presentation))
        return false;
    s.report_state = r.display_state;
    s.report_counter = r.display_counter;
    s.report_snapshot = r.snapshot;
    s.report_records = r.records;
    s.report_new_records = r.new_records;
    s.report_portraits = r.monster_portraits;
    s.report_kills = r.kill_display;
    s.maximum_income = r.maximum_income;
    s.village_points = r.village_points;
    for (const auto &f : r.facilities)
        s.facility_monthly_cash[f.identity] = f.monthly_cash;
    return true;
}
} // namespace

const ref::WorldScriptCatalog &startup_world_runtime_catalog() {
    static const auto catalogue = [] {
        const auto &rules = startup_world_rules();
        const auto &text = rules.script_sources;
        const auto parsed = ref::parse_world_script_catalog(text.events, text.talks, text.news,
                                                            text.event_messages);
        const auto rewards = ref::parse_world_popularity_rewards(text.popularity_rewards);
        if (!parsed.catalog || !rewards.rewards)
            throw std::invalid_argument("固定世界脚本原表解析失败");
        const auto programs =
            ref::world_popularity_script_catalog(*parsed.catalog, *rewards.rewards);
        if (!programs)
            throw std::invalid_argument("人气程序注册失败");
        auto result = *programs;
        for (std::size_t n = 0; n < rules.facilities.size(); ++n)
            result.programs.emplace(2000 + static_cast<int>(n),
                                    rules.facility_initial.at(n).completion_program);
        for (const auto &monster : rules.monsters)
            result.programs.emplace(2500 + monster.identity, monster.introduction_program);
        for (const auto &human : rules.humans) {
            auto program = human.residence_request_program;
            for (auto &command : program)
                if (command.size() == 2 && command[0] == 2) {
                    const int talk = command[1];
                    if (talk < 0 || talk >= static_cast<int>(result.talks.size()))
                        throw std::invalid_argument("住宅请求原对话缺失");
                    if (result.talks.at(talk).speaker_definition == -1)
                        command = {3, talk, 1, human.identity};
                }
            result.programs.emplace(2700 + human.identity, std::move(program));
        }
        return result;
    }();
    return catalogue;
}

ref::WorldActorRoutesState startup_world_runtime_routes(const State &s) {
    ref::WorldActorRoutesState r;
    r.world = s.scene.world.world;
    if (!project_human_task_flags(r.world, s.human_flags))
        throw std::invalid_argument("共同人物缺原任务旗标投影");
    r.facts = ref::world_schedule_facts(s.scene.world);
    r.random = s.scene.random;
    r.popularity_queue = s.scene.world.popularity_queue;
    r.task = s.task;
    r.shop_humans = s.shop_humans;
    r.shop_actors = s.shop_actors;
    r.items = s.items;
    r.dungeon_facilities = s.dungeon_facilities;
    r.dungeon_actors = s.dungeon_actors;
    r.catalog = s.catalog;
    r.shops = s.shops;
    for (auto &shop : r.shops)
        if (s.facility_details.count(shop.first))
            shop.second.notices = s.facility_details.at(shop.first).notices;
    r.shop_order = s.shop_order;
    r.item_rewards = s.item_rewards;
    r.human_definition_state = s.human_definition_state;
    return r;
}
bool write_startup_world_runtime_routes(State &s, const ref::WorldActorRoutesState &r) {
    // 原aH.z与m.v即时累计；台账仅保留真实现金事务，不借月报再支付一次净额。
    const int month = s.scene.calendar.month;
    auto cash_cursor = s.scene.world.world.ai.accounting.funds();
    for (const auto &entry : r.world.ai.accounting.entries()) {
        if (s.scene.world.world.ai.accounting.entries().count(entry.first))
            continue;
        const auto &cash = entry.second;
        const auto category = static_cast<std::size_t>(cash.category);
        const auto direction = cash.direction == ref::CashDirection::income ? 0 : 1;
        const auto value =
            static_cast<std::int64_t>(s.monthly_cash.at(month).at(category)[direction]) +
            cash.amount;
        if (value > std::numeric_limits<int>::max() || value < std::numeric_limits<int>::min())
            return false;
        s.monthly_cash.at(month).at(category)[direction] = static_cast<int>(value);
        if ((direction == 0 &&
             cash_cursor > std::numeric_limits<std::int64_t>::max() - cash.amount) ||
            (direction == 1 &&
             cash_cursor < std::numeric_limits<std::int64_t>::min() + cash.amount))
            return false;
        cash_cursor += direction == 0 ? cash.amount : -cash.amount;
        if (direction == 0 && s.completion_mode == 0 && cash_cursor > s.cash_peak) {
            s.cash_peak = cash_cursor;
            s.cash_peak_village = s.scripts.village_name;
        }
    }
    for (const auto &facility : r.world.facilities) {
        const auto old = s.scene.world.world.facilities.find(facility.first);
        if (old == s.scene.world.world.facilities.end())
            continue;
        const auto delta = static_cast<std::int64_t>(facility.second.sales) - old->second.sales;
        const auto value = s.facility_monthly_cash[facility.first].at(month)[0] + delta;
        if (value > std::numeric_limits<int>::max() || value < std::numeric_limits<int>::min())
            return false;
        s.facility_monthly_cash[facility.first].at(month)[0] = static_cast<int>(value);
    }
    s.scene.world.world = r.world;
    s.scene.world.surface = r.facts.surface;
    s.scene.world.map_flags = r.facts.flags;
    s.scene.world.town = r.facts.town;
    s.scene.random = r.random;
    s.scene.world.popularity_queue = r.popularity_queue;
    s.task = r.task;
    s.shop_humans = r.shop_humans;
    s.shop_actors = r.shop_actors;
    s.items = r.items;
    s.dungeon_facilities = r.dungeon_facilities;
    s.dungeon_actors = r.dungeon_actors;
    s.catalog = r.catalog;
    s.shops = r.shops;
    for (auto &shop : s.shops) {
        if (s.facility_details.count(shop.first))
            s.facility_details.at(shop.first).notices = shop.second.notices;
        shop.second.notices.clear();
    }
    s.shop_order = r.shop_order;
    s.item_rewards = r.item_rewards;
    s.human_definition_state = r.human_definition_state;
    return true;
}
ref::WorldScriptState startup_world_runtime_scripts(const State &s) {
    auto r = s.scripts;
    r.pending_completion = s.scene.world.world.ai.pending_completion;
    r.popularity_queue = s.scene.world.popularity_queue;
    r.scene_mode = s.scene.scene_state;
    r.scene_updates = s.scene.scene_counter;
    r.exploration_phase = s.task_progress.exploration_stage;
    r.human_order.clear();
    for (const auto id : s.scene.world.world.ai.human_order)
        r.human_order.push_back(id.value);
    ref::WorldScriptFinance finance;
    finance.cash = s.scene.world.world.ai.accounting.funds();
    finance.cash_peak = s.cash_peak;
    finance.cash_peak_village = s.cash_peak_village;
    finance.legacy_flags14 = s.completion_mode; // P14，不是UserData.u。
    finance.month = s.scene.calendar.month;
    finance.monthly_totals = s.monthly_cash;
    finance.localized_gold_template = s.rules->localized_gold_template;
    r.finance = std::move(finance);
    for (auto &human : r.humans) {
        human.second.status = s.human_presence.at(human.first);
        human.second.satisfaction = s.shop_humans.at(human.first).satisfaction;
        const auto profession =
            s.scene.world.world.ai.growth.at(human.first).definition.current_profession;
        human.second.extra = s.rules->jobs.at(profession).type;
    }
    for (auto &d : r.facilities)
        d.second.level = s.scene.world.world.facility_uses.at(d.first).level;
    return r;
}
bool write_startup_world_runtime_scripts(State &s, const ref::WorldScriptState &r) {
    if (!r.finance || !post_finance(s, *r.finance))
        return false;
    s.scripts = r;
    // aM调用计数是唯一事实；战斗规则需要的seen集合只作同次调用的派生缓存。
    s.scene.world.world.ai.battle.events.clear();
    for (const auto &event : r.event_calls)
        if (event.second > 0)
            s.scene.world.world.ai.battle.events.insert(event.first);
    s.scene.world.world.ai.pending_completion = r.pending_completion;
    s.scene.world.popularity_queue = r.popularity_queue;
    s.scene.scene_state = r.scene_mode;
    s.scene.scene_counter = r.scene_updates;
    s.task_progress.exploration_stage = r.exploration_phase;
    for (const auto &h : r.humans) {
        s.human_presence[h.first] = h.second.status;
        s.shop_humans.at(h.first).satisfaction = h.second.satisfaction;
    }
    for (const auto &d : r.facilities)
        s.scene.world.world.facility_uses.at(d.first).level = d.second.level;
    for (const auto &profession : r.professions) {
        if (profession.first < 0 ||
            profession.first >= static_cast<int>(s.scene.world.world.ai.professions.size()))
            return false;
        s.scene.world.world.ai.professions.at(profession.first).unlocked =
            profession.second.status != 0;
    }
    // 共享字段只留在真实Owner，持久scripts不成为第二份账本、I、名单、场景。
    s.scripts.finance.reset();
    s.scripts.pending_completion = 0;
    s.scripts.popularity_queue.clear();
    s.scripts.human_order.clear();
    s.scripts.scene_mode = s.scripts.scene_updates = s.scripts.exploration_phase = 0;
    return true;
}
ref::DungeonFinishState startup_world_runtime_finish(const State &s) {
    ref::DungeonFinishState f;
    const auto r = startup_world_runtime_routes(s);
    f.dungeon = {r.world, r.dungeon_facilities, r.dungeon_actors, r.catalog,
                 r.shops, r.shop_order,         r.item_rewards};
    f.task_progress = s.task_progress;
    f.task_progress.monsters.clear();
    for (const auto &m : s.scene.world.world.ai.monster_growth)
        f.task_progress.monsters.emplace(
            m.first, ref::DungeonMonsterAvailability{m.second.status, m.second.newly_unlocked});
    f.tasks = s.tasks;
    f.task_order = s.task_order;
    f.active_task = s.active_task;
    f.participants = s.participants;
    f.human_definition_flags = s.human_flags;
    f.sites = s.sites;
    f.surface = s.surface;
    f.ground_definition = s.ground_definition;
    f.event_calls = s.scripts.event_calls;
    f.raw_year = s.scene.calendar.year;
    f.raw_month = s.scene.calendar.month;
    return f;
}
bool write_startup_world_runtime_finish(State &s, const ref::DungeonFinishState &f) {
    auto r = startup_world_runtime_routes(s);
    r.world = f.dungeon.world;
    r.dungeon_facilities = f.dungeon.facilities;
    r.dungeon_actors = f.dungeon.actors;
    r.catalog = f.dungeon.catalog;
    r.shops = f.dungeon.shops;
    r.shop_order = f.dungeon.shop_order;
    r.item_rewards = f.dungeon.item_rewards;
    if (!write_startup_world_runtime_routes(s, r))
        return false;
    s.task_progress = f.task_progress;
    for (const auto &m : f.task_progress.monsters) {
        const auto current = s.scene.world.world.ai.monster_growth.find(m.first);
        if (current == s.scene.world.world.ai.monster_growth.end())
            return false;
        current->second.status = m.second.status;
        current->second.newly_unlocked = m.second.pending_notice;
    }
    s.task_progress.monsters.clear(); // 怪物目录真实唯一Owner是ai.monster_growth。
    s.tasks = f.tasks;
    s.task_order = f.task_order;
    s.active_task = f.active_task;
    s.participants = f.participants;
    s.human_flags = f.human_definition_flags;
    if (!project_human_task_flags(s.scene.world.world, s.human_flags))
        return false;
    s.sites = f.sites;
    s.surface = f.surface;
    s.scripts.event_calls = f.event_calls;
    s.scene.world.world.ai.battle.events.clear();
    for (const auto &event : f.event_calls)
        if (event.second > 0)
            s.scene.world.world.ai.battle.events.insert(event.first);
    return true;
}

ref::WorldRuntimeAdapter<State> startup_world_runtime_adapter() {
    ref::WorldRuntimeAdapter<State> a;
    a.catalog = startup_world_runtime_catalog();
    a.scene = {[](const State &s) { return s.scene; },
               [](State &s, const auto &p) {
                   s.scene = p;
                   return true;
               }};
    a.scripts = {startup_world_runtime_scripts, write_startup_world_runtime_scripts};
    a.read_random = [](const State &s) -> const ref::WorldRandomStream & { return s.scene.random; };
    a.write_random = [](State &s) -> ref::WorldRandomStream & { return s.scene.random; };
    a.actors.read_common = [](const State &s) -> const ref::WorldScheduleState & {
        return s.scene.world;
    };
    a.actors.write_common = [](State &s) -> ref::WorldScheduleState & { return s.scene.world; };
    a.actors.read_routes = startup_world_runtime_routes;
    a.actors.write_routes = write_startup_world_runtime_routes;
    a.actors.primary_expression_table = true;
    a.actors.decision = [](const State &s, ref::CharacterId id) {
        auto current_facts = facts(s);
        current_facts.task_entry = startup_world_runtime_task_entry(s, id);
        auto input = prepare_startup_world_decision_input(startup_world_runtime_routes(s), id,
                                                          current_facts);
        if (input && input->combat)
            input->combat->facing_for = facing_provider(s);
        return input;
    };
    a.actors.owned_command = [](const State &s, const auto &r, ref::CharacterId id,
                                const auto &op) {
        auto input = prepare_startup_world_command_input(r, id, op, facts(s));
        if (input && input->attack) {
            input->attack->facing_for = facing_provider(s);
            if (r.world.ai.battle.actors.at(id).kind == ref::ActorKind::human)
                input->attack->drop_selection = startup_world_drop_selection(s, id);
        }
        return input;
    };
    a.actors.event = script;
    a.actors.projected_facing = [](const State &s, ref::CharacterId id,
                                   const ref::BattleActorRecord &actor) -> std::optional<int> {
        const auto found = s.actor_metadata.find(id);
        if (found == s.actor_metadata.end())
            return {};
        const auto projected = startup_world_raw_projection(actor.position);
        const auto previous = found->second.cached_view;
        // d尾先比较新u/旧v，再执行ab/r；任一投影轴相等时保留旧朝向。
        if (projected.y > previous.y) {
            if (projected.x > previous.x)
                return 0;
            if (projected.x < previous.x)
                return 3;
        } else if (projected.y < previous.y) {
            if (projected.x > previous.x)
                return 1;
            if (projected.x < previous.x)
                return 2;
        }
        return actor.control.facing;
    };
    a.actors.tail_cache = [](const State &s, ref::CharacterId id,
                             const ref::BattleActorRecord &projected) -> std::optional<State> {
        const auto actor = s.scene.world.world.ai.battle.actors.find(id);
        if (actor == s.scene.world.world.ai.battle.actors.end())
            return {};
        auto next = s;
        auto &metadata = next.actor_metadata.at(id);
        // 原u取物理后的n，包括整数高度；r随后清高度不能覆盖本次已保存u。
        metadata.cached_view = startup_world_raw_projection(projected.position);
        return next;
    };
    a.prefix_effects = [](const State &s,
                          const ref::WorldScheduleEffects &effects) -> std::optional<State> {
        auto next = s;
        const auto actor = next.scene.world.world.ai.battle.actors.find(effects.actor);
        if (actor == next.scene.world.world.ai.battle.actors.end())
            return {};
        for (const auto &sound : effects.sounds)
            if (!emit_startup_world_actor_sound(next, effects.actor, sound.sound))
                return {};
        if (effects.growth.empty())
            return next;
        if (actor->second.kind != ref::ActorKind::human)
            return {};
        const int human_id = actor->second.definition;
        const auto human_source =
            std::find_if(next.rules->humans.begin(), next.rules->humans.end(),
                         [&](const auto &h) { return h.identity == human_id; });
        if (human_source == next.rules->humans.end())
            return {};
        auto scripts = startup_world_runtime_scripts(next);
        for (const auto &request : effects.growth) {
            using Kind = ref::HumanGrowthRequestKind;
            if (request.kind == Kind::event109 || request.kind == Kind::event113) {
                const auto result = ref::prepare_world_script(
                    startup_world_runtime_catalog(), scripts,
                    {request.kind == Kind::event109 ? 109 : 113,
                     request.kind == Kind::event113 ? std::optional<std::string>(human_source->name)
                                                    : std::nullopt,
                     {}});
                if (!result.candidate)
                    return {};
                scripts = result.candidate->state;
            } else if (request.kind == Kind::report_growth) {
                const auto &growth = next.scene.world.world.ai.growth.at(human_id);
                const int level =
                    growth.definition.profession_levels.at(growth.definition.current_profession);
                ref::WorldScriptNotice notice{
                    0, 1, 80, human_source->name + "\t" + std::to_string(level),
                    human_source->name + " Lv.<co=0064FF>" + std::to_string(level) + "</co>"};
                notice.human_definition = human_id;
                notice.attribute_changes = growth.notice_attributes;
                scripts.notices.push_back(std::move(notice));
            } else if (request.kind == Kind::page70 || request.kind == Kind::page94) {
                ref::WorldScriptPage page;
                page.kind = ref::WorldScriptPageKind::raw_page;
                page.legacy_page = request.kind == Kind::page70 ? 70 : 94;
                if (request.kind == Kind::page94) {
                    page.legacy_r = 4;
                    page.legacy_s = request.profession;
                    page.legacy_t = human_source->sex;
                }
                const auto result = ref::prepare_world_script_page(scripts, page);
                if (!result.candidate)
                    return {};
                scripts = result.candidate->state;
                for (const auto &inserted : result.candidate->inserted_pages)
                    next.page_human_bindings[inserted.id] = human_id;
            } else if (request.kind == Kind::unlock_profession) {
                auto &profession = scripts.professions.at(request.profession);
                profession.status = 1;
                profession.pending_notice = true;
            } else if (request.kind == Kind::notice33) {
                const auto &name = next.rules->jobs.at(request.profession).name;
                scripts.notices.push_back(
                    {33, -1, 80, name, "可以转职为<co=0064FF>" + name + "</co>了"});
            } else {
                return {};
            }
        }
        if (!write_startup_world_runtime_scripts(next, scripts))
            return {};
        return next;
    };
    a.nonactors.read_common = a.actors.read_common;
    a.nonactors.write_common = a.actors.write_common;
    a.nonactors.read_routes = [](const State &s) {
        ref::WorldNonactorScheduleState r{s.scene.world, s.scene.random, {}};
        r.objects.catalog = s.catalog;
        r.objects.shops = startup_world_runtime_routes(s).shops;
        r.objects.shop_order = s.shop_order;
        r.objects.item_rewards = s.item_rewards;
        return r;
    };
    a.nonactors.write_routes = [](State &s, const ref::WorldNonactorScheduleState &r) {
        s.catalog = r.objects.catalog;
        s.shops = r.objects.shops;
        for (auto &shop : s.shops) {
            if (s.facility_details.count(shop.first))
                s.facility_details.at(shop.first).notices = shop.second.notices;
            shop.second.notices.clear();
        }
        s.shop_order = r.objects.shop_order;
        s.item_rewards = r.objects.item_rewards;
        s.scene.random = r.random;
        return true;
    };
    a.nonactors.primary_expression_table = true;
    a.nonactors.encounter = [](const State &,
                               std::uint64_t id) -> std::optional<ref::EncounterCommitInput> {
        ref::EncounterCommitInput input;
        input.encounter = id;
        return input;
    };
    a.nonactors.request =
        [](State &s,
           const ref::WorldNonactorRequest &request) -> std::optional<ref::WorldNonactorWriteback> {
        std::optional<int> event;
        if (request.kind == ref::WorldNonactorRequestKind::encounter && request.encounter &&
            request.encounter->kind == ref::EncounterRequestKind::event)
            event = request.encounter->parameter;
        if (request.kind == ref::WorldNonactorRequestKind::projectile_hit && request.hit &&
            (request.hit->kind == ref::HitRequestKind::event131 ||
             request.hit->kind == ref::HitRequestKind::event217))
            event = request.hit->parameter;
        if (request.kind == ref::WorldNonactorRequestKind::object && request.object &&
            request.object->kind == ref::ObjectCommitRequestKind::event)
            event = request.object->parameter;
        if (!event)
            return {}; // 未实现表现请求不吞掉；空名单仍可运行真实final L。
        auto result = script(s, *event);
        if (!result)
            return {};
        s = std::move(*result);
        return ref::WorldNonactorWriteback{
            ref::encounter_external_writeback(s.scene.world.world.ai),
            {},
            s.scene.world.popularity_queue};
    };
    a.popularity.read = [](const State &s) {
        return ref::WorldPopularityState{s.popularity,         s.maximum_popularity,
                                         s.popularity_display, s.popularity_pulses,
                                         s.popularity_rewards, startup_world_runtime_scripts(s)};
    };
    a.popularity.write = [](State &s, const ref::WorldPopularityState &p) {
        if (!write_startup_world_runtime_scripts(s, p.scripts))
            return false;
        s.popularity = p.popularity;
        s.maximum_popularity = p.maximum;
        s.popularity_display = p.reward_display;
        s.popularity_pulses = p.pulses;
        s.popularity_rewards = p.rewards;
        return true;
    };
    a.facilities.read = [](const State &s) {
        return ref::WorldFacilityUpdateState{startup_world_runtime_finish(s),
                                             startup_world_runtime_scripts(s),
                                             s.scene.random,
                                             s.facility_definitions,
                                             s.facility_details,
                                             s.clock_parameter};
    };
    a.facilities.write = [](State &s, const ref::WorldFacilityUpdateState &p) {
        if (!write_startup_world_runtime_finish(s, p.finish) ||
            !write_startup_world_runtime_scripts(s, p.scripts))
            return false;
        s.scene.random = p.random;
        s.facility_definitions = p.definitions;
        s.facility_details = p.details;
        return true;
    };
    a.report = {report, write_report};
    a.report_input = [](const State &s) -> std::optional<ref::WorldMonthReportInput> {
        return ref::WorldMonthReportInput{s.scene.calendar.month_ticks, s.clock_parameter,
                                          s.scene.calendar.month, true, false};
    };
    a.before_common = [](const State &s) -> std::optional<State> {
        auto next = s;
        if (!project_human_task_flags(next.scene.world.world, next.human_flags))
            return {};
        return next;
    };
    a.normal_conditions = [](const State &s) -> std::optional<ref::OwnedWorldSceneStep<State>> {
        // MainScene前置：资金不足/物品等分支尚未拥有输入时显式拒绝，不伪装无条件成功。
        if (s.scene.world.world.ai.accounting.funds() < 0)
            return {};
        return ref::OwnedWorldSceneStep<State>{s};
    };
    configure_startup_world_runtime_calendar_adapter(a);
    configure_startup_world_runtime_nonactor_adapter(a);
    configure_startup_world_runtime_task_adapter(a);
    configure_startup_world_runtime_arrival_adapter(a);
    configure_startup_world_runtime_scene_adapter(a);
    return a;
}

StartupWorldRuntimeSession::StartupWorldRuntimeSession(const StartupState &startup,
                                                       ref::WorldRandomStream random) {
    const auto projected = prepare_startup_world_projection(startup, random);
    if (!projected.candidate)
        throw std::invalid_argument("新局世界投影未认证");
    const auto &p = *projected.candidate;
    state_.rules = p.rules;
    write_startup_world_runtime_routes(state_, p.routes);
    state_.scene.calendar = {p.calendar[0],
                             p.calendar[1],
                             p.calendar[2],
                             p.calendar[3],
                             0,
                             static_cast<int>(p.simulation_steps)};
    state_.scene.world.facility_order = p.facility_order;
    state_.scene.world.spawn_cells = startup_evidence().spawn_points;
    state_.camera = {static_cast<float>(startup_evidence().camera.x),
                     static_cast<float>(startup_evidence().camera.y)};
    state_.previous_camera = state_.camera;
    state_.human_homes = p.human_homes;
    state_.human_presence = p.human_presence;
    state_.actor_metadata = p.actor_metadata;
    state_.facility_original_ids = p.facility_original_ids;
    state_.facility_ordinals = p.facility_ordinals;
    state_.facility_residents = p.facility_residents;
    state_.neighbourhood = p.neighbourhood;
    state_.surface = p.surface;
    state_.road_patches = p.road_patches;
    state_.base_variants = p.rules->base_variants;
    state_.ground_definition = p.ground_definition;
    state_.special_ground_definition = p.special_ground_definition;
    state_.popularity = p.popularity;
    state_.arrival_counter = p.arrival_counter;
    state_.event89_count = p.event89_count;
    state_.simulation_steps = p.simulation_steps;
    state_.village_points = startup.accounting.village_points();
    state_.cash_peak = 0;                  // 新系统J.G[2]默认0，不从初始所持金P.M[0]推导。
    state_.cash_peak_village = "没有记录"; // d/a.H[1]原新系统默认字符串。
    state_.scripts.human_catalog_complete = true;
    state_.scripts.facility_catalog_complete = true;
    state_.scripts.pages.push_back({});
    state_.scripts.pages.back().id = 1;
    state_.scripts.next_page_id = 2;
    for (const auto &h : p.rules->humans) {
        state_.human_calendar.emplace(h.identity, StartupWorldHumanCalendar{});
        state_.human_flags.emplace(h.identity, h.flags);
        state_.scripts.humans.emplace(
            h.identity,
            ref::WorldScriptUnlockDefinition{
                h.status, false, h.name, p.rules->jobs.at(h.definition.current_profession).type});
    }
    for (std::size_t n = 0; n < p.rules->jobs.size(); ++n) {
        const auto &j = p.rules->jobs[n];
        state_.scripts.professions.emplace(
            static_cast<int>(n),
            ref::WorldScriptUnlockDefinition{j.initial_status, (j.flags & 2U) != 0, j.name,
                                             j.script_extra});
    }
    for (const auto &a : p.rules->activities) {
        state_.activity_flags.emplace(a.identity, a.flags);
        state_.scripts.activities.emplace(
            a.identity,
            ref::WorldScriptUnlockDefinition{a.initial_status, (a.flags & 2U) != 0, a.name});
    }
    for (const auto &h : p.rules->humans)
        if (h.status != 0)
            ++state_.scripts.job_counts.at(p.rules->jobs.at(h.definition.current_profession).type);
    for (std::size_t n = 0; n < p.rules->facilities.size(); ++n) {
        const auto &d = p.rules->facilities[n];
        ref::WorldScriptFacilityDefinition script_definition;
        script_definition.category = d.kind; // 原o.f82e，不是活动类别f83f。
        script_definition.economy = d.economy;
        ref::FacilityEconomyInput economy_input;
        economy_input.legacy_job_counts = state_.scripts.job_counts;
        const auto economy = ref::derive_facility_economy(d.economy, economy_input);
        if (!economy.values)
            throw std::invalid_argument("新局共享经营值未认证");
        for (std::size_t k = 0; k < 4; ++k)
            script_definition.attributes[k] =
                static_cast<int>(economy.values->definition_attributes[k]);
        state_.scripts.facilities.emplace(d.id, script_definition);
        const auto &initial = p.rules->facility_initial.at(n);
        state_.facility_definitions.emplace(
            d.id, ref::WorldFacilityUpdateDefinition{initial.flags, initial.shared_n});
        state_.facility_presence.emplace(d.id, initial.status);
        state_.residence_catalog_available.emplace(d.id, false);
        state_.facility_unlock_notices.emplace(d.id, initial.pending_notice);
        state_.facility_unlock_counters.emplace(d.id, 0);
    }
    for (const auto id : p.facility_order) {
        state_.facility_monthly_cash.emplace(id, std::array<std::array<int, 2>, 12>{});
        ref::WorldFacilityUpdateDetails details;
        details.resident_definition = p.facility_residents.at(id);
        state_.facility_details.emplace(id, details);
        state_.dungeon_facilities.emplace(id, ref::DungeonFacilityProgress{});
        state_.facility_month_age.emplace(id, 0);
        state_.facility_difficulties.emplace(id, 0);
    }
    for (const auto &i : p.rules->items)
        state_.shop_item_stock.emplace(i.identity, i.maintenance);
    for (const auto &t : p.rules->tasks)
        state_.task_progress.definitions.emplace(
            t.factory.identity,
            ref::DungeonTaskDefinitionProgress{t.factory.kind, t.factory.flags,
                                               t.factory.completions, t.factory.monster});
    const auto rewards =
        ref::parse_world_popularity_rewards(p.rules->script_sources.popularity_rewards);
    if (!rewards.rewards)
        throw std::invalid_argument("新局人气原表未认证");
    state_.popularity_rewards = *rewards.rewards;
    if (p.event89_count == 1) {
        const auto actual = script(state_, 89); // 接管已安装首访，重建原89实际页/scene6。
        if (!actual)
            throw std::invalid_argument("首访脚本未认证");
        state_ = *actual;
    }
}
const State &StartupWorldRuntimeSession::state() const { return state_; }
void StartupWorldRuntimeSession::set_paused(bool paused) { state_.scene.framework_paused = paused; }
void StartupWorldRuntimeSession::set_speed(int setting) { state_.scene.speed_setting = setting; }
StartupWorldRuntimeError StartupWorldRuntimeSession::acknowledge_page(std::uint64_t id) {
    return acknowledge_startup_world_runtime_page(state_, id);
}
StartupWorldRuntimeError StartupWorldRuntimeSession::act_award_page(std::uint64_t id,
                                                                    ref::WorldAwardAction action) {
    return act_startup_world_runtime_award_page(state_, id, action);
}
StartupWorldRuntimeResult prepare_startup_world_runtime(const State &s) {
    auto admitted = s;
    // 框架下一入口真正移除已关闭页，活动页恢复；不把close当作推进世界/续体。
    admitted.scripts.pages.erase(
        std::remove_if(admitted.scripts.pages.begin(), admitted.scripts.pages.end(),
                       [](const auto &page) { return page.lifecycle == 4; }),
        admitted.scripts.pages.end());
    if (admitted.scripts.pages.empty())
        return {StartupWorldRuntimeError::invalid_page,
                {},
                ref::WorldSceneError::invalid_state,
                ref::WorldScheduleError::none,
                {}};
    // 框架j只在当前页回调期间有效；入口重建，不继承已关闭/已删除页的旧引用。
    admitted.scripts.executing_page = admitted.scripts.pages.back().id;
    const auto &top = admitted.scripts.pages.back();
    if (top.kind == ref::WorldScriptPageKind::raw_page && top.legacy_page == 31 &&
        !initialize_startup_world_runtime_task_result_page(admitted, top.id))
        return {StartupWorldRuntimeError::missing_source,
                {},
                ref::WorldSceneError::missing_consumer,
                ref::WorldScheduleError::none,
                {}};
    admitted.scripts.pages.back().lifecycle = 2;
    admitted.scene.top_is_main =
        admitted.scripts.pages.back().kind == ref::WorldScriptPageKind::scene;
    if (!admitted.scene.top_is_main) {
        if (admitted.scene.framework_paused) {
            admitted.scripts.executing_page.reset();
            return {StartupWorldRuntimeError::none,
                    admitted,
                    ref::WorldSceneError::none,
                    ref::WorldScheduleError::none,
                    {}};
        }
        auto updated = update_startup_world_runtime_page(admitted);
        if (updated)
            updated->scripts.executing_page.reset();
        return updated ? StartupWorldRuntimeResult{StartupWorldRuntimeError::none,
                                                   updated,
                                                   ref::WorldSceneError::none,
                                                   ref::WorldScheduleError::none,
                                                   {}}
                       : StartupWorldRuntimeResult{StartupWorldRuntimeError::missing_source,
                                                   {},
                                                   ref::WorldSceneError::missing_consumer,
                                                   ref::WorldScheduleError::none,
                                                   {}};
    }
    auto a = startup_world_runtime_adapter();
    std::vector<std::shared_ptr<const State>> checkpoints;
    const auto other = a.calendar_other;
    a.calendar_other = [&](const State &current,
                           ref::WorldCalendarStage stage) -> std::optional<State> {
        if (stage != ref::WorldCalendarStage::checkpoint_before_normalize)
            return other ? other(current, stage) : std::nullopt;
        auto next = current;
        next.save_marker = 1;
        // 研究策略：原时点保留完整不可变Owner，不执行原APK文件序列化/存取。
        const auto executing_page = next.scripts.executing_page;
        next.scripts.executing_page.reset();
        checkpoints.push_back(std::make_shared<const State>(next));
        next.scripts.executing_page = executing_page;
        return next;
    };
    const auto result = ref::prepare_owned_world_runtime(admitted, {s.calendar_advance, true}, a);
    if (!result.state)
        return {StartupWorldRuntimeError::runtime_failed, {}, result.error, result.world_error, {}};
    auto next = *result.state;
    next.scripts.executing_page.reset(); // kairo/android/a/b.g的finally清j；检查点不保留回调根。
    next.simulation_steps += result.scene ? result.scene->begun_rounds : 0;
    return {StartupWorldRuntimeError::none, std::move(next), ref::WorldSceneError::none,
            ref::WorldScheduleError::none, std::move(checkpoints)};
}
StartupWorldRuntimeResult StartupWorldRuntimeSession::update() {
    auto result = prepare_startup_world_runtime(state_);
    if (result.candidate) {
        if (!update_startup_world_render_cache(*result.candidate))
            return {StartupWorldRuntimeError::missing_source,
                    {},
                    ref::WorldSceneError::missing_consumer,
                    ref::WorldScheduleError::none,
                    {}};
        state_ = *result.candidate;
        checkpoints_.insert(checkpoints_.end(), result.checkpoints.begin(),
                            result.checkpoints.end());
    }
    return result;
}
const std::vector<std::shared_ptr<const State>> &StartupWorldRuntimeSession::checkpoints() const {
    return checkpoints_;
}
} // namespace dungeon_village_prototype
