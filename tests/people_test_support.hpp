#pragma once
// Shared synthetic decision fixtures only. These values never enter the playable startup.
#include "ark/people/departure.hpp"
#include <stdexcept>
#include <utility>

namespace ark::test {
inline int checks{};
inline void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
inline people::ActivityCandidateSnapshot
snapshot(std::vector<people::ActivityCandidateCell> cells) {
    people::ActivityCandidateSnapshot result{std::move(cells), {}};
    for (const auto &cell : result.cells)
        if (!cell.instance || cell.instance->legacy_phase == 1)
            ++result.category_counts.at(cell.definition.legacy_category);
    return result;
}
inline people::ActivityCandidateCell candidate(world::Cell position, std::uint64_t instance,
                                               int definition, int category, std::int64_t charm,
                                               std::optional<std::int64_t> cost, int phase = 1) {
    std::optional<people::CandidateInstance> bound;
    if (instance)
        bound = people::CandidateInstance{instance, definition, phase};
    return {position, {definition, category, charm},  bound,
            cost,     people::CandidateOrigin::event, 0};
}
struct DecisionFixture {
    world::RouteMap map;
    people::ActivityCandidateInput input;
    explicit DecisionFixture(int width = 5, int height = 5)
        : map{width, height, std::vector<world::RouteCell>(width * height)} {
        input.town = {0, width - 1, 0, height - 1};
        input.cell_definition_ids.assign(width * height, 17);
        input.definitions = {{17, 0, 0}, {28, 2, 5}, {30, 1, 5}, {33, 1, 4}, {75, 4, 0}};
    }
    void add(world::Cell cell, std::uint64_t instance, int definition, int phase = 1,
             int fragment = 0) {
        map.cells[map.index(cell)] = {1, world::RouteCategory::terminal, definition, 0,
                                      world::TileBinding{instance, definition, fragment}};
        input.cell_definition_ids[map.index(cell)] = definition;
        bool known{};
        for (const auto &bound : input.instances)
            known = known || bound.instance_id == instance;
        if (!known)
            input.instances.push_back({instance, definition, phase});
    }
    world::DistanceField field(world::Cell start = {2, 2}) const {
        const auto result = world::search(map, start);
        check(result.error == world::RouteError::none && result.field.has_value(),
              "fixture field must be valid");
        return *result.field;
    }
    people::ActivityCandidateSnapshot collect(world::Cell start = {2, 2}) const {
        const auto result = people::collect_activity_candidates(field(start), input);
        check(result.error == people::ActivityCandidateError::none && result.snapshot &&
                  people::valid_activity_candidate_snapshot(*result.snapshot),
              "fixture collection must return consistent snapshot");
        return *result.snapshot;
    }
};
} // namespace ark::test
