// Candidate snapshot construction preserves the observed filters, duplicate events and exchange
// order. Package responsibilities and evidence boundaries: ../README.md.

#include "dungeon_village_reference/activity_candidates.hpp"

#include <algorithm>
#include <map>
#include <tuple>

namespace dungeon_village_reference {
namespace {

constexpr std::size_t kCandidateLimit = 4096;
constexpr std::size_t kInputLimit = 1000000;

bool valid_definition(const CandidateDefinition &definition) {
    return definition.definition_id >= 0 && definition.legacy_category >= 0 &&
           definition.legacy_category < 11 && definition.definition_charm >= 0 &&
           definition.legacy_kind >= -1 && definition.legacy_kind <= 13;
}

bool ordinary_state(int state) {
    return state == 1 || state == 8 || state == 6 || state == 7 || state == 10 || state == 9;
}

bool lower_cost(const ActivityCandidateCell &left, const ActivityCandidateCell &right) {
    return left.cost.has_value() && (!right.cost.has_value() || *left.cost < *right.cost);
}

void count_categories(ActivityCandidateSnapshot &snapshot) {
    snapshot.category_counts.fill(0);
    for (const auto &cell : snapshot.cells) {
        if (!cell.instance || cell.instance->legacy_phase == 1) {
            ++snapshot.category_counts[static_cast<std::size_t>(cell.definition.legacy_category)];
        }
    }
}

bool valid_snapshot(const ActivityCandidateSnapshot &snapshot) {
    if (snapshot.cells.size() > kCandidateLimit || snapshot.ground_definition < -1) {
        return false;
    }
    using CoordinateValue = std::tuple<std::int32_t, std::optional<std::int64_t>, std::uint64_t,
                                       std::int32_t, int, RouteCategory>;
    std::map<std::pair<int, int>, CoordinateValue> positions;
    std::map<std::int32_t, std::tuple<std::int32_t, std::int64_t, int>> definitions;
    std::map<BuildingId, std::pair<std::int32_t, std::int32_t>> instances;
    std::array<std::int64_t, 11> counts{};
    const auto remember_definition = [&](const CandidateDefinition &d) {
        if (!valid_definition(d))
            return false;
        const auto value = std::make_tuple(d.legacy_category, d.definition_charm, d.legacy_kind);
        const auto stored = definitions.emplace(d.definition_id, value);
        return stored.second || stored.first->second == value;
    };
    for (std::size_t index = 0; index < snapshot.cells.size(); ++index) {
        const auto &cell = snapshot.cells[index];
        if (cell.position.x < 0 || cell.position.y < 0 || !valid_definition(cell.definition) ||
            (cell.cost && *cell.cost < 0) ||
            (cell.origin != CandidateOrigin::map_scan && cell.origin != CandidateOrigin::event) ||
            (index > 0 && lower_cost(cell, snapshot.cells[index - 1]))) {
            return false;
        }
        const auto &definition = cell.definition;
        if (!remember_definition(definition)) {
            return false;
        }
        std::uint64_t instance_id = 0;
        std::int32_t phase = 0;
        if (cell.instance) {
            const auto &instance = *cell.instance;
            const auto &instance_definition = candidate_instance_definition(cell);
            const LegacyMapCell tile{
                cell.legacy_state, cell.route_category,
                FacilityTileBinding{instance.instance_id, instance.definition_id, 0}};
            if (instance.instance_id.value == 0 || instance.legacy_phase < 0 ||
                instance.definition_id != instance_definition.definition_id ||
                !remember_definition(instance_definition) ||
                !legacy_surface_binding_matches(tile, definition.definition_id,
                                                definition.legacy_kind,
                                                snapshot.ground_definition)) {
                return false;
            }
            const auto value = std::make_pair(instance.definition_id, instance.legacy_phase);
            const auto stored = instances.emplace(instance.instance_id, value);
            if (!stored.second && stored.first->second != value) {
                return false;
            }
            instance_id = instance.instance_id.value;
            phase = instance.legacy_phase;
        } else if (cell.instance_definition) {
            return false;
        }
        const CoordinateValue value{
            definition.definition_id, cell.cost,          instance_id, phase,
            cell.legacy_state,        cell.route_category};
        const auto stored =
            positions.emplace(std::make_pair(cell.position.x, cell.position.y), value);
        if (!stored.second && stored.first->second != value) {
            return false;
        }
        if (!cell.instance || phase == 1) {
            ++counts[static_cast<std::size_t>(definition.legacy_category)];
        }
    }
    return counts == snapshot.category_counts;
}

} // namespace

bool valid_activity_candidate_snapshot(const ActivityCandidateSnapshot &snapshot) {
    return valid_snapshot(snapshot);
}

bool inside_town(Position position, TownBounds bounds) {
    return position.x > bounds.left && position.x < bounds.right && position.y > bounds.top &&
           position.y < bounds.bottom;
}

ActivityCandidateResult collect_activity_candidates(const LegacyDistanceField &field,
                                                    const ActivityCandidateInput &input) {
    if (!valid_legacy_distance_field(field)) {
        return {ActivityCandidateError::invalid_field, std::nullopt};
    }
    const auto &map = field.map;
    const auto &town = input.town;
    if (town.left >= town.right || town.top >= town.bottom ||
        input.cell_definition_ids.size() != map.cells.size() ||
        input.definitions.size() > kInputLimit || input.instances.size() > kInputLimit ||
        input.events.size() > kInputLimit ||
        (input.last_visited_instance && input.last_visited_instance->value == 0) ||
        input.ground_definition < -1) {
        return {ActivityCandidateError::invalid_input, std::nullopt};
    }
    std::map<std::int32_t, CandidateDefinition> definitions;
    for (const auto &definition : input.definitions) {
        if (!valid_definition(definition) ||
            !definitions.emplace(definition.definition_id, definition).second) {
            return {ActivityCandidateError::invalid_input, std::nullopt};
        }
    }
    if (input.ground_definition >= 0 && (!definitions.count(input.ground_definition) ||
                                         definitions.at(input.ground_definition).legacy_kind != 7))
        return {ActivityCandidateError::invalid_input, std::nullopt};
    std::map<BuildingId, CandidateInstance> instances;
    for (const auto &instance : input.instances) {
        if (instance.instance_id.value == 0 || instance.legacy_phase < 0 ||
            definitions.count(instance.definition_id) == 0 ||
            !instances.emplace(instance.instance_id, instance).second) {
            return {ActivityCandidateError::invalid_input, std::nullopt};
        }
    }
    for (std::size_t index = 0; index < map.cells.size(); ++index) {
        if (definitions.count(input.cell_definition_ids[index]) == 0) {
            return {ActivityCandidateError::invalid_input, std::nullopt};
        }
        if (const auto &binding = map.cells[index].facility) {
            const auto found = instances.find(binding->instance_id);
            const auto &surface = definitions.at(input.cell_definition_ids[index]);
            if (found == instances.end() || found->second.definition_id != binding->definition_id ||
                !legacy_surface_binding_matches(map.cells[index], surface.definition_id,
                                                surface.legacy_kind, input.ground_definition)) {
                return {ActivityCandidateError::binding_mismatch, std::nullopt};
            }
        }
    }
    for (const auto &event : input.events) {
        if (event.position.x < 0 || event.position.y < 0 || event.position.x >= map.width ||
            event.position.y >= map.height || event.legacy_type < 0) {
            return {ActivityCandidateError::invalid_input, std::nullopt};
        }
    }
    const auto index_of = [&](Position position) {
        return static_cast<std::size_t>(position.y) * static_cast<std::size_t>(map.width) +
               static_cast<std::size_t>(position.x);
    };
    const bool start_has_instance = map.cells[index_of(field.start)].facility.has_value();
    const bool exterior = input.legacy_activity == 6 || input.legacy_activity == 8;
    const bool interior = input.legacy_activity == 7;
    ActivityCandidateSnapshot snapshot;
    snapshot.ground_definition = input.ground_definition;
    const auto append = [&](std::size_t index, CandidateOrigin origin, std::size_t source) {
        const auto &binding = map.cells[index].facility;
        std::optional<CandidateInstance> instance;
        if (binding) {
            instance = instances.at(binding->instance_id);
        }
        const auto width = static_cast<std::size_t>(map.width);
        snapshot.cells.push_back(
            {{static_cast<int>(index % width), static_cast<int>(index / width)},
             definitions.at(input.cell_definition_ids[index]),
             instance,
             field.distances[index],
             origin,
             source,
             binding ? std::optional<CandidateDefinition>{definitions.at(binding->definition_id)}
                     : std::nullopt,
             map.cells[index].legacy_state,
             map.cells[index].category});
    };
    for (std::size_t index = 0; index < map.cells.size(); ++index) {
        if (!field.distances[index]) {
            continue;
        }
        const auto width = static_cast<std::size_t>(map.width);
        const Position position{static_cast<int>(index % width), static_cast<int>(index / width)};
        const auto &cell = map.cells[index];
        if (exterior) {
            if (position.y <= town.top || (cell.legacy_state != 4 && cell.legacy_state != 9) ||
                inside_town(position, town)) {
                continue;
            }
        } else if (interior) {
            if (!inside_town(position, town)) {
                continue;
            }
        } else {
            if (position == field.start || !ordinary_state(cell.legacy_state)) {
                continue;
            }
            if (!cell.facility) {
                return {ActivityCandidateError::binding_mismatch, std::nullopt};
            }
            const auto &instance = instances.at(cell.facility->instance_id);
            if (instance.legacy_phase == 0 ||
                (start_has_instance && input.last_visited_instance &&
                 instance.instance_id == *input.last_visited_instance)) {
                continue;
            }
        }
        if (snapshot.cells.size() == kCandidateLimit) {
            return {ActivityCandidateError::candidate_limit, std::nullopt};
        }
        append(index, CandidateOrigin::map_scan, index);
    }
    if (!exterior && !interior) {
        for (std::size_t index = 0; index < input.events.size(); ++index) {
            if (input.events[index].legacy_type != 1) {
                continue;
            }
            if (snapshot.cells.size() == kCandidateLimit) {
                return {ActivityCandidateError::candidate_limit, std::nullopt};
            }
            append(index_of(input.events[index].position), CandidateOrigin::event, index);
        }
    }
    const auto start = std::find_if(snapshot.cells.begin(), snapshot.cells.end(),
                                    [&](const auto &cell) { return cell.position == field.start; });
    if (start != snapshot.cells.end()) {
        snapshot.cells.erase(start);
    }
    // Preserve the verified exchange order, including indirect reversal of equal-cost items.
    for (std::size_t first = 0; first + 1 < snapshot.cells.size(); ++first) {
        for (std::size_t last = snapshot.cells.size() - 1; last > first; --last) {
            if (lower_cost(snapshot.cells[last], snapshot.cells[first])) {
                std::swap(snapshot.cells[last], snapshot.cells[first]);
            }
        }
    }
    count_categories(snapshot);
    return {ActivityCandidateError::none, std::move(snapshot)};
}

CountedCategoryResult select_counted_category_four(const ActivityCandidateSnapshot &snapshot,
                                                   std::int64_t ticket) {
    if (!valid_activity_candidate_snapshot(snapshot)) {
        return {ActivityCandidateError::invalid_snapshot, std::nullopt};
    }
    const auto count = snapshot.category_counts[4];
    if (count == 0) {
        return {ActivityCandidateError::no_candidate, std::nullopt};
    }
    if (ticket < 0 || ticket >= count) {
        return {ActivityCandidateError::invalid_ticket, std::nullopt};
    }
    for (std::size_t index = 0; index < snapshot.cells.size(); ++index) {
        if (snapshot.cells[index].definition.legacy_category == 4 && ticket-- == 0) {
            return {ActivityCandidateError::none,
                    CountedCategoryTarget{index, snapshot.cells[index]}};
        }
    }
    return {ActivityCandidateError::invalid_snapshot, std::nullopt};
}

} // namespace dungeon_village_reference
