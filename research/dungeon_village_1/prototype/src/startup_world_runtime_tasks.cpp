#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "dungeon_village_prototype/startup_world_building.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace dungeon_village_prototype {
using State = StartupWorldRuntimeState;
void synchronize_startup_world_runtime_monsters(State &s) {
    // 局部L生成和全局f.a生成都在刷新请求时拥有新怪物；尾部u更新不能补造缺失对象。
    for (const auto id : s.scene.world.world.ai.monster_order) {
        s.scene.world.world.actors.try_emplace(id);
        const auto &n = s.scene.world.world.ai.battle.actors.at(id).position;
        s.actor_metadata.try_emplace(
            id, StartupWorldActorMetadata{
                    0,
                    0,
                    0,
                    {static_cast<int>(n.x * 30.0F / 100.0F + n.z * 30.0F / 100.0F),
                     static_cast<int>(n.x * -15.0F / 100.0F + n.z * 15.0F / 100.0F)}});
    }
}
namespace {
ref::WorldScriptCatalog catalog(const State &s) {
    const auto &t = s.rules->script_sources;
    auto c = ref::parse_world_script_catalog(t.events, t.talks, t.news, t.event_messages);
    const auto rewards = ref::parse_world_popularity_rewards(t.popularity_rewards);
    if (!c.catalog || !rewards.rewards)
        throw std::runtime_error("固定脚本目录解析失败");
    const auto extended = ref::world_popularity_script_catalog(*c.catalog, *rewards.rewards);
    if (!extended)
        throw std::runtime_error("人气程序注册失败");
    auto result = *extended;
    for (std::size_t n = 0; n < s.rules->facility_initial.size(); ++n)
        result.programs.emplace(2000 + static_cast<int>(n),
                                s.rules->facility_initial[n].completion_program);
    for (const auto &m : s.rules->monsters)
        result.programs.emplace(2500 + m.identity, m.introduction_program);
    // 任务/遭遇会校验全部存活续体，住宅请求也必须保留，不能用局部目录截断全世界。
    for (const auto &human : s.rules->humans) {
        auto program = human.residence_request_program;
        for (auto &command : program)
            if (command.size() == 2 && command[0] == 2) {
                const int talk = command[1];
                if (talk < 0 || talk >= static_cast<int>(result.talks.size()))
                    throw std::runtime_error("住宅请求原对话缺失");
                if (result.talks.at(talk).speaker_definition == -1)
                    command = {3, talk, 1, human.identity};
            }
        result.programs.emplace(2700 + human.identity, std::move(program));
    }
    return result;
}
void refresh_schedule_surface(State &s) {
    s.scene.world.surface.clear();
    for (const auto &cell : s.scene.world.world.map.cells)
        s.scene.world.surface.push_back(static_cast<int>(cell.category));
}
ref::WorldExplorationMapExtra map_extra(const State &s) {
    ref::WorldExplorationMapExtra m;
    for (const auto &d : s.rules->facilities)
        m.definitions.emplace(d.id,
                              ref::WorldMapDefinition{d.display_id, d.kind,
                                                      static_cast<ref::FacilityShape>(d.shape),
                                                      d.neighbour_effects});
    m.special_ground_definition = s.special_ground_definition;
    m.base_variants = s.base_variants;
    m.fence_level = s.fence_level;
    m.fence_levels = s.rules->fences;
    m.facility_order = s.scene.world.facility_order;
    m.refresh_pending = s.scene.first_normal_refresh;
    m.neighbours = s.neighbourhood_details;
    for (const auto &patch : s.road_patches) {
        m.road_quad.push_back(patch[0]);
        m.edge_road_pair.push_back(patch[1]);
    }
    return m;
}
} // namespace
ref::WorldTaskCreationState startup_world_runtime_factory(const State &s) {
    if (!s.rules || s.rules->generation_bounds.empty())
        throw std::runtime_error("任务工厂缺原始地区目录");
    ref::WorldTaskCreationState f;
    f.finish = startup_world_runtime_finish(s);
    f.random = s.scene.random;
    for (const auto &source : s.rules->tasks) {
        auto d = source.factory;
        d.completions = s.task_progress.definitions.at(d.identity).completed;
        f.definitions.push_back(d);
    }
    const auto &ai = s.scene.world.world.ai;
    for (const auto &d : s.rules->monsters) {
        const auto &g = ai.monster_growth.at(d.identity);
        f.monsters.emplace(d.identity,
                           ref::TaskCreationMonster{g.growth, g.introduced, d.challenge_strength});
        if (d.initial.required_progress >= 0 && d.initial.required_progress <= 5)
            f.challenge_monsters.at(d.initial.required_progress).push_back(d.identity);
    }
    for (const auto &d : s.rules->humans)
        f.humans.push_back(
            {s.human_presence.at(d.identity), ai.growth.at(d.identity).derived.attributes[5]});
    for (const auto &d : s.rules->items)
        f.rewards.push_back(
            {0, d.identity, d.difficulty, d.initial.flags, s.catalog.at({0, d.identity}).status});
    for (const auto &d : s.rules->equipment)
        f.rewards.push_back({d.shop.kind, d.shop.id, d.reward_difficulty, d.initial.flags,
                             s.catalog.at({d.shop.kind, d.shop.id}).status});
    for (const auto &d : s.rules->facilities)
        f.facility_definitions.emplace(
            d.id, ref::TaskCreationFacilityDefinition{{d.display_id, d.kind,
                                                       static_cast<ref::FacilityShape>(d.shape),
                                                       d.neighbour_effects},
                                                      d.kind,
                                                      d.category,
                                                      d.detail,
                                                      s.scripts.facilities.at(d.id).attributes[0],
                                                      d.use_wait,
                                                      d.economy.upgrade_uses});
    f.facility_order = s.scene.world.facility_order;
    f.facility_original_ids = s.facility_original_ids;
    f.facility_ordinals = s.facility_ordinals;
    f.facility_difficulties = s.facility_difficulties;
    f.facility_residents = s.facility_residents;
    f.task_original_ids = s.task_original_ids;
    f.task_sequence = s.task_sequence;
    f.next_facility_identity = s.next_facility_identity;
    f.next_task_identity = s.next_task_identity;
    f.generation_bounds = s.rules->generation_bounds.front();
    f.base_variants = s.base_variants;
    f.special_ground_definition = s.special_ground_definition;
    f.fence_level = s.fence_level;
    f.fence_levels = s.rules->fences;
    f.road_patches = s.road_patches;
    f.refresh_pending = s.scene.first_normal_refresh;
    f.rank = s.rank;
    f.special_selection_mode = s.task_special_selection;
    f.special_selection = s.task_special_selection_list;
    f.special_selection_index = s.task_special_selection_index;
    f.replay_order = s.task_replay_order;
    const auto weapon =
        std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(),
                     [](const auto &d) { return d.shop.kind == 1 && (d.initial.flags & 16U); });
    if (weapon == s.rules->equipment.end())
        throw std::runtime_error("首个任务奖励p.J原定义缺失");
    f.first_reward_weapon = weapon->shop.id;
    return f;
}
bool write_startup_world_runtime_factory(State &s, const ref::WorldTaskCreationState &f) {
    if (!write_startup_world_runtime_finish(s, f.finish))
        return false;
    s.scene.random = f.random;
    s.scene.world.facility_order = f.facility_order;
    s.facility_original_ids = f.facility_original_ids;
    s.facility_ordinals = f.facility_ordinals;
    s.facility_difficulties = f.facility_difficulties;
    s.facility_residents = f.facility_residents;
    s.task_original_ids = f.task_original_ids;
    s.task_sequence = f.task_sequence;
    s.next_facility_identity = f.next_facility_identity;
    s.next_task_identity = f.next_task_identity;
    s.task_special_selection = f.special_selection_mode;
    s.task_special_selection_list = f.special_selection;
    s.task_special_selection_index = f.special_selection_index;
    s.task_replay_order = f.replay_order;
    s.road_patches = f.road_patches;
    s.scene.first_normal_refresh = f.refresh_pending;
    for (const auto id : f.facility_order) {
        s.facility_flags.try_emplace(id, 0);
        if (!s.neighbourhood.count(id))
            s.neighbourhood.emplace(id, std::array<int, 3>{}); // factory不执行邻接，原新m.s零。
        s.neighbourhood_details.try_emplace(id);               // 原new m的s/G/w/p/H初值。
        if (!s.facility_details.count(id)) {
            ref::WorldFacilityUpdateDetails details;
            details.condition = f.facility_difficulties.at(id);
            details.resident_definition = -1;
            s.facility_details.emplace(id, details);
            s.facility_monthly_cash.emplace(id, std::array<std::array<int, 2>, 12>{});
            s.facility_month_age.emplace(id, 0);
            s.facility_item_confirmations.emplace(id, 0);
        }
    }
    refresh_schedule_surface(s);
    return true;
}
std::optional<State> prepare_startup_world_runtime_dungeon_finish(const State &state,
                                                                  std::uint64_t facility) {
    if (!state.rules)
        return {};
    State next = state;
    ref::WorldExplorationState e;
    e.finish = startup_world_runtime_finish(next);
    e.map = map_extra(next);
    const auto p = startup_world_runtime_scripts(next);
    e.scripts = {p.continuations,  p.context,           p.village_name,
                 p.executing_page, p.next_page_id,      p.page_mutations_locked,
                 p.selected_actor, p.selected_facility, p.selected_monster};
    e.ui.pages = p.pages;
    e.ui.summaries = next.exploration_summaries;
    e.popularity_queue = p.popularity_queue;
    ref::DungeonFinishInput input;
    input.facility = facility;
    input.draw = [&](int bound) -> std::optional<int> {
        const auto result = next.scene.random.draw(bound);
        return result.error == ref::WorldRandomError::none ? std::optional<int>(result.ticket)
                                                           : std::nullopt;
    };
    const auto result = ref::prepare_world_exploration_finish(catalog(next), e, input);
    if (!result.candidate ||
        !write_startup_world_runtime_finish(next, result.candidate->state.finish))
        return {};
    const auto &out = result.candidate->state;
    auto scripts = startup_world_runtime_scripts(next);
    scripts.continuations = out.scripts.continuations;
    scripts.context = out.scripts.context;
    scripts.village_name = out.scripts.village_name;
    scripts.executing_page = out.scripts.executing_page;
    scripts.next_page_id = out.scripts.next_page_id;
    scripts.page_mutations_locked = out.scripts.page_mutations_locked;
    scripts.selected_actor = out.scripts.selected_actor;
    scripts.selected_facility = out.scripts.selected_facility;
    scripts.selected_monster = out.scripts.selected_monster;
    scripts.pages = out.ui.pages;
    scripts.popularity_queue = out.popularity_queue;
    for (const auto &message : out.ui.messages)
        scripts.notices.push_back(
            {message.state[0], message.state[1], message.state[2], "", message.text, {}});
    if (!write_startup_world_runtime_scripts(next, scripts))
        return {};
    next.scene.world.facility_order = out.map.facility_order;
    next.scene.first_normal_refresh = out.map.refresh_pending;
    next.neighbourhood.clear();
    next.neighbourhood_details = out.map.neighbours;
    for (const auto &cache : out.map.neighbours)
        next.neighbourhood.emplace(cache.first, cache.second.current);
    next.road_patches.clear();
    for (std::size_t n = 0; n < out.map.road_quad.size(); ++n)
        next.road_patches.push_back({out.map.road_quad[n], out.map.edge_road_pair[n]});
    // 页30/32引用及地面显示是原型窗口实际待消费载荷，不能只保留页面号码。
    next.exploration_summaries = out.ui.summaries;
    next.exploration_displays.insert(next.exploration_displays.end(),
                                     out.ui.ground_displays.begin(), out.ui.ground_displays.end());
    refresh_schedule_surface(next);
    return next;
}
std::optional<State> prepare_startup_world_runtime_dungeon_crew(const State &state,
                                                                std::uint64_t facility) {
    State next = state;
    ref::DungeonWorldCrewInput input;
    input.facility = facility;
    input.town = next.scene.world.town;
    if (next.active_task) {
        const auto task = next.tasks.find(*next.active_task);
        if (task == next.tasks.end())
            return {};
        if (task->second.facility) {
            const auto progress = next.dungeon_facilities.find(*task->second.facility);
            if (progress == next.dungeon_facilities.end())
                return {};
            input.active_task_extent = progress->second.extent;
        }
    }
    input.draw = [&](int bound) -> std::optional<int> {
        const auto result = next.scene.random.draw(bound);
        return result.error == ref::WorldRandomError::none ? std::optional<int>(result.ticket)
                                                           : std::nullopt;
    };
    const auto write = [&](const ref::DungeonWorldState &world) {
        auto routes = startup_world_runtime_routes(next);
        routes.world = world.world;
        routes.dungeon_facilities = world.facilities;
        routes.dungeon_actors = world.actors;
        routes.catalog = world.catalog;
        // 探索队伍即时奖励唯一修改catalog；缺原定义镜像先拒绝，不补造目录项。
        for (const auto &item : routes.items)
            if (!routes.catalog.count({0, item.first}))
                return false;
        for (const auto &record : routes.catalog)
            if (record.first.first == 0 && !routes.items.count(record.first.second))
                return false;
        for (auto &item : routes.items)
            item.second = routes.catalog.at({0, item.first});
        routes.shops = world.shops;
        routes.shop_order = world.shop_order;
        routes.item_rewards = world.item_rewards;
        return write_startup_world_runtime_routes(next, routes);
    };
    const auto result = ref::prepare_world_dungeon_crew(
        startup_world_runtime_finish(next).dungeon, input,
        [&](const ref::DungeonWorldState &current,
            const ref::DungeonWorldRequest &request) -> std::optional<ref::DungeonWorldState> {
            if (!write(current))
                return {};
            if (request.source.kind == ref::DungeonCrewRequestKind::progress_label231)
                next.dungeon_labels.push_back(request.source);
            for (const auto &r : request.catalog_requests) {
                if (r.kind == ref::ObjectCommitRequestKind::event) {
                    const auto executed = ref::prepare_world_script(
                        catalog(next), startup_world_runtime_scripts(next), {r.parameter, {}, {}});
                    if (!executed.candidate ||
                        !write_startup_world_runtime_scripts(next, executed.candidate->state))
                        return {};
                } else if (r.kind == ref::ObjectCommitRequestKind::notice) {
                    std::string name;
                    if (request.source.first == 0) {
                        const auto d =
                            std::find_if(next.rules->items.begin(), next.rules->items.end(),
                                         [&](const auto &d) { return d.identity == r.definition; });
                        if (d == next.rules->items.end())
                            return {};
                        name = d->name;
                    } else {
                        const auto d =
                            std::find_if(next.rules->equipment.begin(), next.rules->equipment.end(),
                                         [&](const auto &d) {
                                             return d.shop.kind == request.source.first &&
                                                    d.shop.id == r.definition;
                                         });
                        if (d == next.rules->equipment.end())
                            return {};
                        name = d->name;
                    }
                    // 原UserData.ay2/10/11/12；负az=-1，80tick展示，不再次发奖。
                    const std::string text =
                        r.parameter == 2    ? "<co=0064FF>" + name + "</co> 入手!"
                        : r.parameter == 10 ? "武器 <co=0064FF>" + name + "</co> 入手"
                        : r.parameter == 11 ? "防具 <co=0064FF>" + name + "</co> 入手"
                        : r.parameter == 12 ? "饰品 <co=0064FF>" + name + "</co> 入手"
                                            : "";
                    if (text.empty())
                        return {};
                    next.scripts.notices.push_back({r.parameter, -1, 80, name, text, {}});
                } else if (r.kind == ref::ObjectCommitRequestKind::shop_notice) {
                    if (!r.shop || !next.shops.count(*r.shop) ||
                        !next.facility_details.count(*r.shop))
                        return {};
                    // write(current)已经提交Tenant.p；持久shops只保存类别，不能从其空notices覆盖。
                } else if (r.kind != ref::ObjectCommitRequestKind::grant)
                    return {}; // grant已由当前目录实际提交；其他新领域不得占位成功。
            }
            return startup_world_runtime_finish(next).dungeon;
        });
    if (!result.candidate || !write(result.candidate->state))
        return {};
    return next;
}
std::optional<State>
consume_startup_world_runtime_encounter_request(const State &state,
                                                const ref::EncounterCreationRequest &request) {
    State next = state;
    synchronize_startup_world_runtime_monsters(next);
    const auto c = catalog(next);
    if (request.kind == ref::EncounterCreationRequestKind::refresh_map) {
        const auto map = ref::prepare_world_event_map(next.scene.world.world.ai,
                                                      ref::world_schedule_facts(next.scene.world));
        if (!map.facts)
            return {};
        next.scene.world.map_flags = map.facts->flags;
        return next;
    }
    auto scripts = startup_world_runtime_scripts(next);
    if (request.kind == ref::EncounterCreationRequestKind::page89) {
        ref::WorldScriptPage page;
        page.kind = ref::WorldScriptPageKind::raw_page;
        page.legacy_page = 89;
        page.monster_definition = request.definition;
        const auto created = ref::prepare_world_script_page(scripts, page);
        if (!created.candidate ||
            !write_startup_world_runtime_scripts(next, created.candidate->state))
            return {};
    } else {
        const auto found =
            std::find_if(next.rules->monsters.begin(), next.rules->monsters.end(),
                         [&](const auto &d) { return d.identity == request.definition; });
        if (found == next.rules->monsters.end())
            return {};
        const int identity = 2500 + request.definition; // 研究适配程序身份，不冒充aL原事件ID。
        auto programs = c;
        programs.programs.emplace(identity, found->introduction_program);
        const auto executed =
            ref::prepare_world_script_program(programs, scripts, {identity, {}, {}});
        if (!executed.candidate ||
            !write_startup_world_runtime_scripts(next, executed.candidate->state))
            return {};
    }
    return next;
}
std::optional<ref::OwnedWorldRuntimeCreation<State>>
prepare_startup_world_runtime_encounter_creation(const State &state,
                                                 ref::EncounterCreationInput input) {
    State next = state;
    input.draw = [&](int bound) -> std::optional<int> {
        const auto result = next.scene.random.draw(bound);
        return result.error == ref::WorldRandomError::none ? std::optional<int>(result.ticket)
                                                           : std::nullopt;
    };
    const auto result = ref::prepare_encounter_creation(
        next.scene.world.world.ai, input,
        [&](const ref::AiRewardState &ai,
            const ref::EncounterCreationRequest &request) -> std::optional<ref::AiRewardState> {
            next.scene.world.world.ai = ai;
            const auto consumed = consume_startup_world_runtime_encounter_request(next, request);
            if (!consumed)
                return {};
            next = *consumed;
            return next.scene.world.world.ai;
        });
    if (!result.candidate)
        return {};
    next.scene.world.world.ai = result.candidate->state;
    synchronize_startup_world_runtime_monsters(next);
    return ref::OwnedWorldRuntimeCreation<State>{std::move(next), result.candidate->created,
                                                 result.candidate->denial};
}
std::optional<State> prepare_startup_world_runtime_encounter(const State &state,
                                                             ref::EncounterCreationInput input) {
    auto result = prepare_startup_world_runtime_encounter_creation(state, std::move(input));
    return result ? std::optional<State>(std::move(result->state)) : std::nullopt;
}
std::optional<ref::WorldEventEntryInput> startup_world_runtime_task_entry(const State &s,
                                                                          ref::CharacterId actor) {
    if (!s.active_task || !s.tasks.count(*s.active_task) ||
        !s.scene.world.world.actors.count(actor))
        return {};
    const auto &task = s.tasks.at(*s.active_task);
    const auto definition =
        std::find_if(s.rules->tasks.begin(), s.rules->tasks.end(),
                     [&](const auto &d) { return d.factory.identity == task.definition; });
    const auto progress = s.task_progress.definitions.find(task.definition);
    if (definition == s.rules->tasks.end() || progress == s.task_progress.definitions.end())
        return {};
    auto fact = s.task;
    const auto instance = s.scene.world.world.ai.battle.actors.find(actor);
    if (instance == s.scene.world.world.ai.battle.actors.end() ||
        instance->second.kind != ref::ActorKind::human ||
        !s.human_flags.count(instance->second.definition))
        return {};
    fact.definition_task_flag = (s.human_flags.at(instance->second.definition) & 2U) != 0;
    return ref::WorldEventEntryInput{actor,
                                     fact,
                                     definition->encounter_quota,
                                     progress->second.completed,
                                     progress->second.flags,
                                     {}};
}
bool consume_startup_world_runtime_task_start(State &s, ref::CharacterId actor,
                                              std::uint64_t encounter) {
    if (!s.rules || !encounter || s.task.encounter != encounter || s.task.kind != 1 ||
        !s.scene.world.world.ai.task_active)
        return false;
    const auto created = s.scene.world.world.ai.encounters.find(encounter);
    const auto input = startup_world_runtime_task_entry(s, actor);
    if (created == s.scene.world.world.ai.encounters.end() || !input ||
        !input->task.definition_task_flag ||
        created->second.runtime.id != encounter || created->second.runtime.state != 3 ||
        !(created->second.runtime.center == s.task.center))
        return false;
    const auto quota = ref::prepare_task_encounter_quota(
        input->base_quota, input->task_completions, input->task_flags);
    if (!quota || created->second.runtime.quota != *quota)
        return false;
    // 原c/b.F先完成配额与g安装，再B(2)，最后a.b(24,null)。通知自己在q计数1才C11。
    s.sound_requests.push_back({StartupAudioOperation::replace_bgm, 2});
    s.scripts.notices.push_back({24, -1, 80, {}, "战斗任务开始"});
    return true;
}
std::optional<ref::EncounterCommitInput>
startup_world_runtime_encounter_input(const State &s, std::uint64_t encounter) {
    const auto found = s.scene.world.world.ai.encounters.find(encounter);
    if (found == s.scene.world.world.ai.encounters.end())
        return {};
    ref::EncounterCommitInput input;
    input.encounter = encounter;
    if (found->second.runtime.state != 3 || !s.active_task)
        return input;
    const auto task = s.tasks.find(*s.active_task);
    if (task == s.tasks.end() || s.task.encounter != encounter)
        return {};
    const auto source =
        std::find_if(s.rules->tasks.begin(), s.rules->tasks.end(),
                     [&](const auto &d) { return d.factory.identity == task->second.definition; });
    if (source == s.rules->tasks.end())
        return {};
    const auto progress = s.task_progress.definitions.find(task->second.definition);
    const auto monster = s.scene.world.world.ai.monster_growth.find(source->factory.monster);
    if (progress == s.task_progress.definitions.end() ||
        monster == s.scene.world.world.ai.monster_growth.end())
        return {};
    const auto &m = monster->second;
    const auto reward = ref::prepare_monster_growth(
        m.base_death_reward, m.growth, 1,
        (s.scene.world.world.ai.battle.monsters.at(source->factory.monster).flags & 4U) != 0);
    if (!reward)
        return {};
    input.quest = {progress->second.flags,
                   source->factory.monster,
                   *reward,
                   source->encounter_monsters,
                   s.participants,
                   source->factory.pending_completion_value,
                   source->factory.exploration_stage};
    return input;
}
bool initialize_startup_world_runtime_task_result_page(State &s, std::uint64_t page_id) {
    const auto page = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                   [&](const auto &p) { return p.id == page_id; });
    if (page == s.scripts.pages.end() || page->legacy_page != 31 || !page->task_identity ||
        !page->task_definition || !s.tasks.count(*page->task_identity) ||
        s.tasks.at(*page->task_identity).definition != *page->task_definition)
        return false;
    if (s.crew_summaries.count(page_id))
        return true;
    const auto progress = s.task_progress.definitions.find(*page->task_definition);
    if (progress == s.task_progress.definitions.end())
        return false;
    auto next = s;
    next.crew_summaries.emplace(page_id, s.participants);
    int kills{};
    for (const int definition : s.participants) {
        auto human = next.scene.world.world.ai.battle.humans.find(definition);
        if (human == next.scene.world.world.ai.battle.humans.end())
            return false;
        // b/g页31生命周期0：保留重复X；特殊任务只保留原序第一条非零H。
        if (progress->second.flags & 2U) {
            auto &h = human->second.task_kills;
            if (kills <= 0 || h == 0) {
                if (h > 1)
                    h = 1;
                kills += h;
            } else
                h = 0;
        }
    }
    s = std::move(next);
    return true;
}
bool consume_startup_world_runtime_task_encounter_request(State &s,
                                                          const ref::EncounterRequest &r) {
    using Kind = ref::EncounterRequestKind;
    auto next = s;
    if (r.kind == Kind::refresh_map) {
        const auto refreshed = consume_startup_world_runtime_encounter_request(
            next, {ref::EncounterCreationRequestKind::refresh_map, 0});
        if (!refreshed)
            return false;
        next = *refreshed;
    } else if (r.kind == Kind::event) {
        std::optional<std::string> replacement;
        if (r.parameter == 152) {
            const auto monster =
                std::find_if(next.rules->monsters.begin(), next.rules->monsters.end(),
                             [&](const auto &m) { return m.identity == r.value; });
            if (monster == next.rules->monsters.end())
                return false;
            replacement = monster->name;
        }
        const auto executed = ref::prepare_world_script(startup_world_runtime_catalog(),
                                                        startup_world_runtime_scripts(next),
                                                        {r.parameter, replacement, {}});
        if (!executed.candidate ||
            !write_startup_world_runtime_scripts(next, executed.candidate->state))
            return false;
    } else if (r.kind == Kind::page30 || r.kind == Kind::page31) {
        if (!next.active_task || !next.tasks.count(*next.active_task))
            return false;
        const auto &task = next.tasks.at(*next.active_task);
        ref::WorldScriptPage page;
        page.kind = ref::WorldScriptPageKind::raw_page;
        page.legacy_page = r.kind == Kind::page30 ? 30 : 31;
        page.task_identity = task.identity;
        page.task_definition = task.definition;
        page.legacy_f = r.kind == Kind::page30 ? r.value : r.parameter;
        page.legacy_g = r.kind == Kind::page31 ? r.value : 0;
        const auto created =
            ref::prepare_world_script_page(startup_world_runtime_scripts(next), page);
        if (!created.candidate ||
            !write_startup_world_runtime_scripts(next, created.candidate->state))
            return false;
        if (r.kind == Kind::page30)
            for (const auto &inserted : created.candidate->inserted_pages)
                next.exploration_summaries.emplace(
                    inserted.id,
                    ref::WorldExplorationSummary{30, task.identity, task.definition, {}});
    } else if (r.kind == Kind::mark_task_complete) {
        if (!next.active_task || !next.tasks.count(*next.active_task))
            return false;
        auto finish = startup_world_runtime_finish(next);
        finish.task_progress.remaining_task_definitions.clear();
        for (const auto id : finish.task_order)
            finish.task_progress.remaining_task_definitions.push_back(
                finish.tasks.at(id).definition);
        const auto result = ref::prepare_dungeon_task_success(
            finish.task_progress, finish.tasks.at(*finish.active_task).definition, finish.raw_year,
            finish.raw_month);
        if (!result)
            return false;
        finish.task_progress = result->state;
        if (!write_startup_world_runtime_finish(next, finish))
            return false;
        static const std::string texts[]{"迷之巨大生物觉醒了!", "在隔壁街道发现迷之巨大生物!",
                                         "听见了迷之巨大生物的脚步声"};
        for (const int id : result->threshold_notice_ids) {
            if (id < 29 || id > 31)
                return false;
            next.scripts.notices.push_back({id, -100, 80, "", texts[id - 29]});
        }
    } else if (r.kind == Kind::clear_task) {
        for (auto &human : next.human_flags)
            human.second &= ~2U;
        for (auto &context : next.scene.world.world.actors) {
            const auto actor = next.scene.world.world.ai.battle.actors.find(context.first);
            if (actor != next.scene.world.world.ai.battle.actors.end() &&
                actor->second.kind == ref::ActorKind::human)
                context.second.definition_task_flag = false;
        }
        if (next.active_task) {
            const auto current =
                std::find(next.task_order.begin(), next.task_order.end(), *next.active_task);
            if (current == next.task_order.end())
                return false;
            next.task_order.erase(current); // 原Vector.removeElement只移除首引用，保留退休对象。
        }
        next.active_task.reset();
        next.task = {};
    } else if (r.kind == Kind::refresh_global) {
        // d/a.g实际选背景音乐，不是地图重建；clear_task之后c/n.h()已为false。
        next.sound_requests.push_back({StartupAudioOperation::replace_bgm, next.active_task && next.task.encounter ? 2 : 1});
    } else if (r.kind == Kind::refresh_task_catalog) {
        // c/n.H即时by道具补货；不应用跨月日期/feature16守卫。
        bool opened{}, changed{};
        for (const auto &source : next.rules->items) {
            auto stock = next.shop_item_stock.find(source.identity);
            const auto definition = next.items.find(source.identity);
            if (stock == next.shop_item_stock.end() || definition == next.items.end())
                return false;
            auto &i = stock->second;
            // 即时H()与月度补货同样读当前by.p/q/r，不能把奖励解锁覆盖回旧值。
            i.presence = definition->second.status;
            i.legacy_q = definition->second.unlock_counter;
            i.newly_available = definition->second.newly_unlocked;
            const auto missing = static_cast<std::int64_t>(i.maximum_quantity) - i.quantity;
            if (i.minimum_rank == -1 || i.minimum_rank > next.rank || missing <= 0)
                continue;
            if (missing > std::numeric_limits<int>::max() / 4)
                return false;
            const auto draw = next.scene.random.draw(3);
            if (draw.error != ref::WorldRandomError::none)
                return false;
            const int added = static_cast<int>(
                std::clamp<std::int64_t>((missing * 4) / 10 + draw.ticket - 1, 0, missing));
            if (added == 0)
                continue;
            i.quantity += added;
            if (i.presence == 0) {
                i.presence = 1;
                i.newly_available = true;
                i.legacy_q = 0;
                opened = true;
            }
            changed = true;
            auto &catalogue = next.items.at(source.identity);
            catalogue.status = i.presence;
            catalogue.unlock_counter = i.legacy_q;
            catalogue.newly_unlocked = i.newly_available;
            next.catalog.at({0, source.identity}) = catalogue;
        }
        if (changed)
            next.scripts.notices.push_back(
                {opened ? 17 : 16, -1, 80, "", opened ? "进了新的道具" : "商店进了新的道具"});
    } else if (r.kind != Kind::completion_delta && r.kind != Kind::set_feature16)
        return false; // 这两项scalar在严格encounter提交核心已执行，不能重复累加。
    s = std::move(next);
    return true;
}
void configure_startup_world_runtime_task_adapter(ref::WorldRuntimeAdapter<State> &adapter) {
    adapter.factory = {startup_world_runtime_factory, write_startup_world_runtime_factory};
    adapter.actors.encounter = consume_startup_world_runtime_encounter_request;
    adapter.nonactors.encounter = startup_world_runtime_encounter_input;
    adapter.create_encounter = [](const State &s, const ref::EncounterCreationInput &input) {
        return prepare_startup_world_runtime_encounter_creation(s, input);
    };
    for (const auto &m : startup_world_rules().monsters)
        adapter.catalog.programs.emplace(2500 + m.identity, m.introduction_program);
    const auto previous = adapter.facility;
    adapter.facility =
        [previous](const State &s,
                   const ref::WorldFacilityUpdateRequest &request) -> std::optional<State> {
        if (request.kind == ref::WorldFacilityUpdateConsumerKind::dungeon_finish)
            return prepare_startup_world_runtime_dungeon_finish(s, request.facility);
        if (request.kind == ref::WorldFacilityUpdateConsumerKind::dungeon_crew)
            return prepare_startup_world_runtime_dungeon_crew(s, request.facility);
        if (request.kind == ref::WorldFacilityUpdateConsumerKind::residence_join)
            return prepare_startup_world_residence_completion(s, request.facility);
        return previous ? previous(s, request) : std::nullopt;
    };
}
} // namespace dungeon_village_prototype
