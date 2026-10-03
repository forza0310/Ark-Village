#include "dungeon_village_prototype/village.hpp"

#include "dungeon_village_reference/activity_choice.hpp"
#include "dungeon_village_reference/snapshot_facility_choice.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>

namespace dungeon_village_prototype {
using namespace reference;
namespace {
constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();

std::vector<FacilityPlacement> placements(const VillageState &state) {
    std::vector<FacilityPlacement> result;
    for (const auto &entry : state.facilities)
        result.push_back(entry.second);
    return result;
}

std::size_t index_of(const LegacyMap &map, Position cell) {
    return static_cast<std::size_t>(cell.y) * static_cast<std::size_t>(map.width) +
           static_cast<std::size_t>(cell.x);
}

bool within(const LegacyMap &map, Position cell) {
    return cell.x >= 0 && cell.y >= 0 && cell.x < map.width && cell.y < map.height;
}
} // namespace

Village::Village(std::vector<PrototypeDefinition> catalog, LegacyMap terrain,
                 std::int64_t opening_funds, PrototypeConfig config)
    : catalog_(std::move(catalog)), config_(config) {
    if (!valid_legacy_map(terrain) || terrain.cells.size() > 4096 || opening_funds < 0 ||
        config.tick_ms < 1 || config.tick_ms > 60000 || config.step_ticks < 1 ||
        config.step_ticks > 1000000 || config.use_ticks < 1 || config.use_ticks > 1000000 ||
        config.retry_ticks < 1 || config.retry_ticks > 1000000 || config.period_ticks < 1 ||
        config.period_ticks > 1000000 ||
        std::any_of(terrain.cells.begin(), terrain.cells.end(),
                    [](const auto &cell) { return cell.facility.has_value(); })) {
        throw std::invalid_argument("原型地图、资金或时钟配置无效");
    }
    std::set<std::int32_t> ids;
    for (const auto &item : catalog_) {
        const auto value = derive_facility_economy(item.economy, {});
        if (item.id <= 0 || !ids.insert(item.id).second || item.kind != 3 ||
            (item.category != 1 && item.category != 2) || item.detail != 0 ||
            value.error != FacilityEconomyError::none ||
            value.values->definition_attributes[2] < 0 ||
            value.values->definition_attributes[3] < 0 ||
            facility_footprint(item.shape, FacilityOrientation::first, {1, 0}, 3, 3).error !=
                GeometryError::none ||
            std::any_of(
                item.neighbours.begin(), item.neighbours.end(),
                [](const auto &modifier) {
                    return modifier.attribute_slot < 0 || modifier.attribute_slot >= 3;
                })) {
            throw std::invalid_argument("原型设施定义无效");
        }
    }
    state_.terrain = std::move(terrain);
    state_.accounting = PeriodAccounting(opening_funds);
}

const VillageState &Village::state() const {
    return state_;
}
const std::vector<PrototypeDefinition> &Village::catalog() const {
    return catalog_;
}
const PrototypeConfig &Village::config() const {
    return config_;
}
const PrototypeDefinition &Village::definition(std::int32_t id) const {
    const auto found = std::find_if(catalog_.begin(), catalog_.end(),
                                    [id](const auto &item) { return item.id == id; });
    if (found == catalog_.end())
        throw std::out_of_range("原型设施定义不存在");
    return *found;
}

LegacyMap Village::bound_map() const {
    std::vector<BoundFacility> bound;
    for (const auto &entry : state_.facilities) {
        bound.push_back({entry.second, definition(entry.second.definition_id).kind});
    }
    const auto result = bind_facility_map(state_.terrain, bound);
    if (!result.map)
        throw std::logic_error("原型布局绑定失败");
    return *result.map;
}

FacilityEconomyValues Village::values(BuildingId id) const {
    const auto &placement = state_.facilities.at(id);
    const auto &item = definition(placement.definition_id);
    std::vector<NeighbourDefinition> definitions;
    for (const auto &entry : catalog_) {
        definitions.push_back({entry.id, entry.shape, entry.kind, entry.neighbours});
    }
    const auto map = bound_map();
    std::vector<Position> roads;
    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            const auto &cell = map.cells[index_of(map, {x, y})];
            if (!cell.facility && cell.category == RouteCategory::road)
                roads.push_back({x, y});
        }
    }
    const auto neighbours = derive_facility_neighbourhood(definitions, placements(state_), roads,
                                                          map.width, map.height);
    if (neighbours.error != NeighbourhoodError::none)
        throw std::logic_error("邻接计算失败");
    const auto found = std::find_if(neighbours.facilities.begin(), neighbours.facilities.end(),
                                    [id](const auto &entry) { return entry.instance_id == id; });
    const auto input = neighbourhood_economy_input(*found, {});
    if (!input)
        throw std::overflow_error("邻接属性超出数值范围");
    const auto result = derive_facility_economy(item.economy, *input);
    if (!result.values)
        throw std::overflow_error("经营属性推导失败");
    return *result.values;
}

std::optional<BuildingId> Village::facility_at(Position cell) const {
    if (!within(state_.terrain, cell))
        return std::nullopt;
    const auto map = bound_map();
    const auto &binding = map.cells[index_of(map, cell)].facility;
    return binding ? std::optional<BuildingId>{binding->instance_id} : std::nullopt;
}

VillageError Village::validate_placement(const FacilityPlacement &placement,
                                         std::optional<BuildingId> ignored) const {
    const auto result = evaluate_facility_placement(
        placements(state_), placement, state_.terrain.width, state_.terrain.height, ignored);
    if (result.error != GeometryError::none)
        return VillageError::invalid_placement;
    for (const auto &cell : result.cells) {
        if (state_.terrain.cells[index_of(state_.terrain, cell.position)].category ==
            RouteCategory::blocked)
            return VillageError::invalid_placement;
        for (const auto &entry : state_.actors) {
            if (entry.second.cell == cell.position)
                return VillageError::actor_occupied;
        }
    }
    return VillageError::none;
}

VillageError Village::post(CashCategory category, CashDirection direction, std::int64_t amount) {
    if (state_.next_event == maximum)
        return VillageError::numeric_overflow;
    if (state_.accounting.post_cash({state_.next_event, state_.period, category, direction,
                                     amount}) != AccountingError::none)
        return VillageError::numeric_overflow;
    ++state_.next_event;
    return VillageError::none;
}

void Village::cancel_activities() {
    for (auto &entry : state_.actors) {
        auto &actor = entry.second;
        if (actor.activity != ActivityState::idle)
            ++actor.cancelled;
        actor.activity = ActivityState::idle;
        actor.target.reset();
        actor.activity_id = 0;
        actor.path.clear();
        actor.cursor = 0;
        actor.phase_ticks = 0;
        actor.wait_ticks = config_.retry_ticks;
    }
}

VillageCommandResult Village::place(std::int32_t definition_id, Position anchor,
                                    FacilityOrientation orientation) {
    const auto item =
        std::find_if(catalog_.begin(), catalog_.end(),
                     [definition_id](const auto &entry) { return entry.id == definition_id; });
    if (item == catalog_.end())
        return {VillageError::not_found, std::nullopt};
    if (state_.next_instance == maximum)
        return {VillageError::numeric_overflow, std::nullopt};
    const BuildingId id{state_.next_instance};
    const FacilityPlacement placement{id, definition_id, item->shape, orientation, anchor};
    const auto error = validate_placement(placement, std::nullopt);
    if (error != VillageError::none)
        return {error, std::nullopt};
    const auto cost = derive_facility_economy(item->economy, {}).values->construction_cost;
    if (state_.accounting.funds() < cost)
        return {VillageError::insufficient_funds, std::nullopt};
    auto next = *this;
    const auto cash = next.post(CashCategory::other, CashDirection::expense, cost);
    if (cash != VillageError::none)
        return {cash, std::nullopt};
    next.state_.facilities.emplace(id, placement);
    next.state_.monthly_sales.emplace(id, 0);
    next.state_.completed_definition_uses.try_emplace(definition_id, 0);
    ++next.state_.next_instance;
    next.cancel_activities();
    *this = std::move(next);
    return {VillageError::none, id};
}

VillageError Village::relocate(BuildingId id, Position anchor, FacilityOrientation orientation) {
    const auto found = state_.facilities.find(id);
    if (found == state_.facilities.end())
        return VillageError::not_found;
    if (found->second.anchor == anchor && found->second.orientation == orientation)
        return VillageError::none;
    auto placement = found->second;
    placement.anchor = anchor;
    placement.orientation = orientation;
    const auto error = validate_placement(placement, id);
    if (error != VillageError::none)
        return error;
    if (state_.accounting.funds() < 300)
        return VillageError::insufficient_funds;
    auto next = *this;
    const auto cash = next.post(CashCategory::other, CashDirection::expense, 300);
    if (cash != VillageError::none)
        return cash;
    next.state_.facilities.at(id) = placement;
    next.cancel_activities();
    *this = std::move(next);
    return VillageError::none;
}

VillageError Village::demolish(BuildingId id) {
    if (state_.facilities.count(id) == 0)
        return VillageError::not_found;
    auto next = *this;
    next.state_.facilities.erase(id);
    next.state_.monthly_sales.erase(id);
    next.cancel_activities();
    *this = std::move(next);
    return VillageError::none;
}

VillageError Village::add_actor(CharacterId id, Position cell, std::uint32_t flags) {
    if (id.value == 0 || state_.actors.count(id) || !within(state_.terrain, cell) ||
        facility_at(cell) ||
        state_.terrain.cells[index_of(state_.terrain, cell)].category == RouteCategory::blocked)
        return VillageError::invalid_input;
    PrototypeActor actor;
    actor.id = id;
    actor.cell = cell;
    actor.flags = flags;
    state_.actors.emplace(id, std::move(actor));
    return VillageError::none;
}

std::size_t Village::draw(std::size_t bound) {
    // Local replayable fixture RNG, not a reconstruction of the APK random generator.
    state_.random_state = state_.random_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return static_cast<std::size_t>((state_.random_state >> 16U) % bound);
}

VillageError Village::begin(PrototypeActor &actor) {
    const auto map = bound_map();
    const auto field = search_legacy_map(map, actor.cell);
    if (!field.field)
        return VillageError::rule_failure;
    ActivityCandidateInput input;
    input.town = {-1, map.width, -1, map.height};
    input.cell_definition_ids.assign(map.cells.size(), 0);
    input.definitions.push_back({0, 0, 0});
    for (const auto &item : catalog_) {
        input.definitions.push_back(
            {item.id, item.category,
             derive_facility_economy(item.economy, {}).values->definition_attributes[2]});
    }
    std::set<BuildingId> reserved;
    for (const auto &entry : state_.actors)
        if (entry.second.target)
            reserved.insert(*entry.second.target);
    for (const auto &entry : state_.facilities) {
        input.instances.push_back(
            {entry.first, entry.second.definition_id, reserved.count(entry.first) ? 0 : 1});
    }
    for (std::size_t i = 0; i < map.cells.size(); ++i) {
        if (map.cells[i].facility)
            input.cell_definition_ids[i] = map.cells[i].facility->definition_id;
    }
    input.last_visited_instance = actor.arrival.last_visited_instance;
    const auto candidates = collect_activity_candidates(*field.field, input);
    if (!candidates.snapshot)
        return VillageError::rule_failure;
    ActivityChoiceInput choices;
    choices.available_category_counts = candidates.snapshot->category_counts;
    for (std::size_t i = 0; i < choices.legacy_visit_counts.size(); ++i)
        choices.legacy_visit_counts[i] = actor.arrival.legacy_visit_counts[i];
    const auto categories = plan_activity_categories(choices);
    if (!categories.plan)
        return VillageError::rule_failure;
    std::vector<std::int64_t> weights;
    std::vector<int> supported;
    std::int64_t total = 0;
    for (const auto &option : categories.plan->options) {
        if (option.category == 1 || option.category == 2) {
            weights.push_back(option.weight);
            supported.push_back(option.category);
            total += option.weight;
        }
    }
    actor.wait_ticks = config_.retry_ticks;
    if (total == 0)
        return VillageError::none;
    const auto category = select_weighted_ticket(
        weights, static_cast<std::int64_t>(draw(static_cast<std::size_t>(total))));
    const int selected = supported.at(*category.index);
    std::int64_t weight = 0;
    for (const auto &cell : candidates.snapshot->cells) {
        if (cell.instance && cell.instance->legacy_phase == 1 &&
            cell.definition.legacy_category == selected)
            weight += cell.definition.definition_charm;
    }
    if (weight == 0)
        return VillageError::none;
    const auto target =
        select_snapshot_facility(*candidates.snapshot, selected,
                                 static_cast<std::int64_t>(draw(static_cast<std::size_t>(weight))));
    if (!target.target)
        return VillageError::rule_failure;
    const auto route = trace_legacy_path(*field.field, target.target->goal.position);
    if (route.error == MapAccessError::unreachable)
        return VillageError::none;
    if (route.error != MapAccessError::none)
        return VillageError::rule_failure;
    if (state_.next_activity == maximum)
        return VillageError::numeric_overflow;
    actor.target = target.target->goal.instance->instance_id;
    actor.activity_id = state_.next_activity++;
    actor.activity = ActivityState::travelling;
    actor.path = route.steps;
    actor.exit_cell = route.steps.size() >= 2 ? route.steps[route.steps.size() - 2] : actor.cell;
    actor.cursor = 0;
    actor.phase_ticks = 0;
    actor.wait_ticks = 0;
    return VillageError::none;
}

VillageError Village::close_period() {
    if (state_.period == maximum)
        return VillageError::numeric_overflow;
    ReportInput report;
    report.period = state_.period;
    for (const auto &entry : state_.facilities) {
        const auto fee = values(entry.first).definition_attributes[3];
        if (state_.next_event == maximum)
            return VillageError::numeric_overflow;
        report.charges.push_back({state_.next_event++, state_.period, CashCategory::facilities,
                                  CashDirection::expense, fee});
    }
    if (state_.accounting.prepare_report(report) != AccountingError::none ||
        state_.accounting.claim_report(state_.period) != AccountingError::none)
        return VillageError::numeric_overflow;
    for (auto &entry : state_.monthly_sales)
        entry.second = 0;
    ++state_.period;
    return VillageError::none;
}

VillageError Village::tick() {
    if (state_.ticks == maximum)
        return VillageError::numeric_overflow;
    for (auto &entry : state_.actors) {
        auto &actor = entry.second;
        if (actor.activity == ActivityState::idle) {
            if (actor.wait_ticks > 0)
                --actor.wait_ticks;
            else {
                const auto error = begin(actor);
                if (error != VillageError::none)
                    return error;
            }
            continue;
        }
        if (actor.activity == ActivityState::travelling) {
            if (actor.cursor < actor.path.size()) {
                if (++actor.phase_ticks < config_.step_ticks)
                    continue;
                actor.phase_ticks = 0;
                actor.cell = actor.path[actor.cursor++];
            }
            if (actor.cursor < actor.path.size())
                continue;
            const auto &facility = state_.facilities.at(*actor.target);
            const auto &item = definition(facility.definition_id);
            const auto map = bound_map();
            if (!arrival_binding_matches(map, {actor.cell, facility.instance_id, item.id},
                                         actor.cell))
                return VillageError::rule_failure;
            auto arrival = actor.arrival;
            arrival.current_month_facility_sales = state_.monthly_sales.at(*actor.target);
            const auto price = values(*actor.target).instance_attributes[0];
            if (price < std::numeric_limits<std::int32_t>::min() ||
                price > std::numeric_limits<std::int32_t>::max())
                return VillageError::numeric_overflow;
            const auto result = prepare_facility_arrival(
                arrival, {actor.id, *actor.target, item.id, item.kind, item.category, item.detail,
                          0, actor.flags, -1, static_cast<std::int32_t>((state_.period - 1) % 12),
                          static_cast<std::int32_t>(price)});
            if (!result.candidate)
                return VillageError::numeric_overflow;
            const auto cash = post(CashCategory::facilities, CashDirection::income,
                                   result.candidate->cash_income);
            if (cash != VillageError::none)
                return cash;
            actor.arrival = result.candidate->state;
            state_.monthly_sales.at(*actor.target) = actor.arrival.current_month_facility_sales;
            actor.activity = ActivityState::in_use;
            actor.phase_ticks = 0;
            ++actor.arrivals;
        } else if (++actor.phase_ticks >= config_.use_ticks) {
            auto &uses = state_.completed_definition_uses.at(
                state_.facilities.at(*actor.target).definition_id);
            if (uses == maximum || actor.completed == maximum)
                return VillageError::numeric_overflow;
            ++uses;
            ++actor.completed;
            // Fixture exit: return to the approach cell, without inventing APK exit effects.
            actor.cell = actor.exit_cell;
            actor.activity = ActivityState::idle;
            actor.target.reset();
            actor.activity_id = 0;
            actor.path.clear();
            actor.cursor = 0;
            actor.phase_ticks = 0;
            actor.wait_ticks = config_.retry_ticks;
            actor.arrival.legacy_visit_counts.fill(0);
        }
    }
    ++state_.ticks;
    if (state_.ticks % static_cast<std::uint64_t>(config_.period_ticks) == 0)
        return close_period();
    return VillageError::none;
}

VillageError Village::advance(int elapsed_ms) {
    if (elapsed_ms < 0 || elapsed_ms > 60000)
        return VillageError::invalid_input;
    if (state_.paused)
        return VillageError::none;
    auto next = *this;
    const auto elapsed = static_cast<std::int64_t>(next.state_.remainder_ms) + elapsed_ms;
    const auto ticks = elapsed / config_.tick_ms;
    next.state_.remainder_ms = static_cast<int>(elapsed % config_.tick_ms);
    try {
        for (std::int64_t i = 0; i < ticks; ++i) {
            const auto error = next.tick();
            if (error != VillageError::none)
                return error;
        }
    } catch (const std::overflow_error &) {
        return VillageError::numeric_overflow;
    }
    *this = std::move(next);
    return VillageError::none;
}

void Village::set_paused(bool paused) {
    state_.paused = paused;
}

} // namespace dungeon_village_prototype
