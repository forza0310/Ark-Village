#include "ark/app/world_facility_queries.hpp"

#include <algorithm>
#include <set>

namespace ark::app {
namespace {
namespace rules = simulation::rules;
using State = simulation::StartupWorldRuntimeState;
using Error = WorldFacilityQueryError;
using Definitions = std::map<int, const simulation::StartupDefinition *>;

WorldFacilityTemplate classify(const simulation::StartupDefinition &d) {
    // PAGES page74: this priority is meaningful (kind12 wins over activity detail).
    if (d.kind == 12)
        return WorldFacilityTemplate::home;
    if (d.detail == 1 || d.detail == 4 || d.detail == 5)
        return WorldFacilityTemplate::equipment;
    if (d.detail == 6)
        return WorldFacilityTemplate::recruitment;
    if (d.kind == 2)
        return WorldFacilityTemplate::booster;
    return WorldFacilityTemplate::ordinary;
}

// Reconstruct only the read model. In particular, do not call map refresh: it also
// updates notices and source caches, and those are owned by simulation transactions.
Error neighbourhood(const State &s, const Definitions &definitions, WorldFacilityDetail &out) {
    const auto &world = s.scene.world.world;
    const auto &map = world.map;
    if (!rules::valid_legacy_map(map) ||
        s.scene.world.facility_order.size() != world.facilities.size())
        return Error::invalid_state;
    std::vector<rules::NeighbourDefinition> sources;
    for (const auto &d : s.rules->facilities)
        sources.push_back(
            {d.id, static_cast<rules::FacilityShape>(d.shape), d.kind, d.neighbour_effects});
    std::vector<rules::FacilityPlacement> placements;
    std::set<std::uint64_t> seen;
    std::size_t bound_cells = 0;
    for (const auto id : s.scene.world.facility_order) {
        const auto f = world.facilities.find(id);
        if (f == world.facilities.end() || !seen.insert(id).second ||
            f->second.placement.instance_id.value != id)
            return Error::invalid_state;
        if (!definitions.count(f->second.placement.definition_id))
            return Error::missing_source;
        const auto &placement = f->second.placement;
        const auto footprint = rules::facility_footprint(placement.shape, placement.orientation,
                                                         placement.anchor, map.width, map.height);
        if (footprint.error != rules::GeometryError::none)
            return Error::invalid_state;
        for (const auto &cell : footprint.cells) {
            const auto &binding =
                map.cells[static_cast<std::size_t>(cell.position.y) * map.width + cell.position.x]
                    .facility;
            if (!binding || binding->instance_id.value != id ||
                binding->definition_id != placement.definition_id ||
                binding->fragment_index != cell.fragment_index)
                return Error::invalid_state;
        }
        bound_cells += footprint.cells.size();
        placements.push_back(f->second.placement);
    }
    if (bound_cells != static_cast<std::size_t>(
                           std::count_if(map.cells.begin(), map.cells.end(),
                                         [](const auto &c) { return c.facility.has_value(); })))
        return Error::invalid_state;
    std::vector<rules::Position> roads;
    for (int y = 0; y < map.height; ++y)
        for (int x = 0; x < map.width; ++x)
            if (map.cells[static_cast<std::size_t>(y) * map.width + x].legacy_state == 3)
                roads.push_back({x, y});
    const auto result =
        rules::derive_facility_neighbourhood(sources, placements, roads, map.width, map.height);
    if (result.error == rules::NeighbourhoodError::numeric_overflow)
        return Error::numeric_overflow;
    if (result.error != rules::NeighbourhoodError::none)
        return Error::invalid_state;
    const auto derived =
        std::find_if(result.facilities.begin(), result.facilities.end(),
                     [&](const auto &n) { return n.instance_id.value == out.instance; });
    if (derived == result.facilities.end())
        return Error::invalid_state;
    for (std::size_t i = 0; i < 3; ++i)
        if (derived->modifiers[i] != out.neighbour_modifiers[i])
            return Error::inconsistent_neighbourhood;

    const auto cached = s.neighbourhood_details.find(out.instance);
    if (cached != s.neighbourhood_details.end()) {
        if (cached->second.current != out.neighbour_modifiers ||
            cached->second.sources.size() != derived->sources.size())
            return Error::inconsistent_neighbourhood;
        for (std::size_t i = 0; i < derived->sources.size(); ++i)
            if (cached->second.sources[i].instance_id.value !=
                    derived->sources[i].instance_id.value ||
                cached->second.sources[i].definition_id != derived->sources[i].definition_id)
                return Error::inconsistent_neighbourhood;
    }
    for (const auto &source : derived->sources) {
        const auto d = definitions.find(source.definition_id);
        const auto progress = world.facility_uses.find(source.definition_id);
        if (d == definitions.end() || progress == world.facility_uses.end())
            return Error::missing_source;
        if (progress->second.level < 1 || progress->second.level > 5)
            return Error::invalid_state;
        out.neighbours.push_back({source.instance_id.value, source.definition_id, d->second->name,
                                  progress->second.level, d->second->neighbour_effects});
    }
    return Error::none;
}
} // namespace

WorldFacilityDetailResult query_world_facility_detail(const State &s, std::uint64_t id) {
    const auto &world = s.scene.world.world;
    const auto instance = world.facilities.find(id);
    if (instance == world.facilities.end())
        return {Error::missing_instance, {}};
    if (!s.rules)
        return {Error::missing_source, {}};
    Definitions definitions;
    for (const auto &d : s.rules->facilities)
        if (!definitions.emplace(d.id, &d).second)
            return {Error::invalid_state, {}};
    const auto &f = instance->second;
    const auto def = f.placement.definition_id;
    const auto source = definitions.find(def);
    const auto progress = world.facility_uses.find(def);
    const auto shared = s.scripts.facilities.find(def);
    const auto modifiers = s.neighbourhood.find(id);
    const auto cash = s.facility_monthly_cash.find(id);
    if (source == definitions.end() || progress == world.facility_uses.end() ||
        shared == s.scripts.facilities.end() || modifiers == s.neighbourhood.end() ||
        cash == s.facility_monthly_cash.end())
        return {Error::missing_source, {}};
    const auto &d = *source->second;
    const int month = s.scene.calendar.month;
    if (month < 0 || month >= 12 || f.status < 0 || progress->second.completed_uses < 0 ||
        f.kind != d.kind || f.category != d.category || f.detail != d.detail)
        return {Error::invalid_state, {}};
    WorldFacilityDetail out;
    out.instance = id;
    out.definition = def;
    out.type = classify(d);
    out.name = d.name;
    out.display_id = d.display_id;
    out.kind = d.kind;
    out.activity_category = d.category;
    out.activity_detail = d.detail;
    out.placement = f.placement;
    out.level = progress->second.level;
    out.completed_uses = progress->second.completed_uses;
    out.upgrade_pending = progress->second.upgrade_pending;
    out.neighbour_modifiers = modifiers->second;
    if (const auto error = neighbourhood(s, definitions, out); error != Error::none)
        return {error, {}};

    rules::FacilityEconomyInput input;
    input.level = out.level;
    input.completed_definition_uses = static_cast<std::uint64_t>(out.completed_uses);
    input.definition_improvements = shared->second.improvements;
    input.legacy_job_counts = s.scripts.job_counts;
    std::copy(out.neighbour_modifiers.begin(), out.neighbour_modifiers.end(),
              input.instance_modifiers.begin());
    const auto economy = rules::derive_facility_economy(d.economy, input);
    if (!economy.values)
        return {economy.error == rules::FacilityEconomyError::numeric_overflow
                    ? Error::numeric_overflow
                    : Error::invalid_state,
                {}};
    out.attributes = economy.values->instance_attributes;
    if (out.level != 5)
        out.remaining_uses = economy.values->upgrade_uses - out.completed_uses;
    out.status = f.status;
    if (f.status == 0) {
        const auto elapsed = s.dungeon_facilities.find(id);
        const auto details = s.facility_details.find(id);
        if (elapsed == s.dungeon_facilities.end() || details == s.facility_details.end())
            return {Error::missing_source, {}};
        if (elapsed->second.updates < 0 || details->second.construction_limit <= 0)
            return {Error::invalid_state, {}};
        out.construction =
            WorldFacilityConstruction{elapsed->second.updates, details->second.construction_limit};
    }
    out.occupants = f.occupants;
    out.month = month;
    out.monthly_income = cash->second[month][0];
    out.monthly_expense = cash->second[month][1];
    if (d.kind == 3 || d.kind == 9) {
        std::int64_t profit = 0;
        // Twelve signed32 rows fit signed64 even at the maximum absolute difference.
        for (int i = 0; i <= month; ++i)
            profit += static_cast<std::int64_t>(cash->second[i][0]) - cash->second[i][1];
        out.cumulative_profit = profit;
    }
    return {Error::none, std::move(out)};
}
} // namespace ark::app
