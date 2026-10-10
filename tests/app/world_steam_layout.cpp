#include "world_steam_layout.hpp"
#include "../support/world_fixture.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace ark::test {
namespace {
namespace ref = simulation::rules;
using State = simulation::StartupWorldRuntimeState;
using Role = SteamLayoutRole;
constexpr auto first = ref::FacilityOrientation::first;
constexpr auto second = ref::FacilityOrientation::second;

// Audited 0001 layout: partition 24 definition/anchor matched uniquely to the
// documented partition 22 orientation projection. Excludes encounters and gates.
// Evidence and version boundary: ACTIVE_VILLAGE_PLAN.md#steam-layout-blueprint.
constexpr std::array<SteamLayoutTarget, 50> layout{{
    {28, {12, 4}, second, Role::shop},      {47, {14, 6}, second, Role::shop},
    {32, {8, 4}, first, Role::shop},        {24, {4, 8}, first, Role::recruitment},
    {71, {8, 8}, first, Role::plant},       {25, {4, 9}, first, Role::residence},
    {48, {9, 8}, second, Role::shop},       {42, {7, 8}, first, Role::shop},
    {69, {10, 9}, first, Role::plant},      {28, {10, 6}, first, Role::shop},
    {71, {10, 5}, first, Role::plant},      {69, {6, 8}, first, Role::plant},
    {55, {7, 4}, first, Role::shop},        {36, {6, 6}, second, Role::shop},
    {71, {7, 6}, first, Role::plant},       {70, {8, 5}, first, Role::plant},
    {59, {9, 5}, first, Role::shop},        {30, {8, 6}, first, Role::shop},
    {71, {13, 5}, first, Role::plant},      {71, {14, 5}, first, Role::plant},
    {71, {12, 5}, first, Role::plant},      {25, {4, 6}, first, Role::residence},
    {25, {4, 7}, first, Role::residence},   {60, {17, 6}, second, Role::shop},
    {25, {16, 5}, second, Role::residence}, {25, {16, 4}, second, Role::residence},
    {25, {17, 4}, second, Role::residence}, {25, {19, 5}, second, Role::residence},
    {25, {19, 3}, second, Role::residence}, {25, {19, 4}, second, Role::residence},
    {42, {13, 4}, first, Role::shop},       {40, {9, 4}, second, Role::shop},
    {25, {4, 3}, first, Role::residence},   {25, {19, 6}, second, Role::residence},
    {69, {17, 5}, first, Role::plant},      {59, {16, 8}, first, Role::shop},
    {60, {9, 9}, second, Role::shop},       {37, {14, 4}, first, Role::shop},
    {71, {10, 4}, first, Role::plant},      {31, {13, 6}, second, Role::shop},
    {40, {12, 6}, second, Role::shop},      {38, {10, 8}, first, Role::shop},
    {25, {4, 4}, first, Role::residence},   {55, {18, 8}, first, Role::shop},
    {34, {16, 7}, first, Role::shop},       {71, {17, 7}, first, Role::plant},
    {63, {13, 8}, second, Role::shop},      {41, {14, 9}, first, Role::shop},
    {49, {7, 9}, second, Role::shop},       {39, {14, 8}, first, Role::shop},
}};
} // namespace

const std::array<SteamLayoutTarget, 50> &steam_village_layout() { return layout; }

ref::GeometryResult steam_layout_footprint(const State &s, const SteamLayoutTarget &target) {
    if (!s.rules || target.definition < 0 ||
        static_cast<std::size_t>(target.definition) >= s.rules->facilities.size())
        return {ref::GeometryError::invalid_identity, {}};
    const auto &map = s.scene.world.world.map;
    return ref::facility_footprint(
        static_cast<ref::FacilityShape>(s.rules->facilities.at(target.definition).shape),
        target.orientation, target.anchor, map.width, map.height);
}

ref::GeometryError validate_steam_village_layout(const State &s) {
    std::set<std::pair<int, int>> occupied;
    for (const auto &target : layout) {
        const auto footprint = steam_layout_footprint(s, target);
        if (footprint.error != ref::GeometryError::none)
            return footprint.error;
        for (const auto &cell : footprint.cells)
            if (!occupied.emplace(cell.position.x, cell.position.y).second)
                return ref::GeometryError::overlap;
    }
    return ref::GeometryError::none;
}

bool steam_layout_inside_fence(const State &s, const SteamLayoutTarget &target) {
    const auto footprint = steam_layout_footprint(s, target);
    if (footprint.error != ref::GeometryError::none || s.fence_level < 0 ||
        static_cast<std::size_t>(s.fence_level) >= s.rules->fences.size())
        return false;
    const auto &bounds = s.rules->fences.at(s.fence_level);
    return std::all_of(footprint.cells.begin(), footprint.cells.end(), [&](const auto &cell) {
        const auto p = cell.position;
        return p.x > bounds[0].x && p.x < bounds[1].x && p.y < bounds[0].y && p.y > bounds[1].y;
    });
}

std::optional<std::uint64_t> steam_layout_match(const State &s, const SteamLayoutTarget &target) {
    for (const auto &[id, facility] : s.scene.world.world.facilities) {
        const auto &p = facility.placement;
        const bool definition_matches =
            p.definition_id == target.definition ||
            (target.role == Role::residence && p.definition_id >= 25 && p.definition_id <= 27);
        if (definition_matches && p.anchor == target.anchor && p.orientation == target.orientation)
            return id;
    }
    return {};
}

std::optional<std::size_t> steam_layout_slot(const State &s, std::uint64_t facility) {
    for (std::size_t i = 0; i < layout.size(); ++i)
        if (steam_layout_match(s, layout[i]) == facility)
            return i;
    return {};
}

std::vector<std::uint64_t> steam_layout_blockers(const State &s, const SteamLayoutTarget &target) {
    const auto footprint = steam_layout_footprint(s, target);
    if (footprint.error != ref::GeometryError::none)
        throw std::runtime_error("Steam layout: invalid target footprint");
    const auto &map = s.scene.world.world.map;
    std::set<std::uint64_t> unique;
    for (const auto &cell : footprint.cells) {
        const auto &tile = map.cells.at(cell.position.y * map.width + cell.position.x);
        if (tile.facility)
            unique.insert(tile.facility->instance_id.value);
    }
    return {unique.begin(), unique.end()};
}

void steam_layout_contract() {
    const auto require = [](bool value, const char *message) {
        if (!value)
            throw std::runtime_error(std::string("Steam layout contract: ") + message);
    };
    const auto original = initial_world();
    require(layout.size() == 50 &&
                validate_steam_village_layout(original) == ref::GeometryError::none,
            "audited fifty targets must coexist using current product geometry");
    const auto inside = std::count_if(layout.begin(), layout.end(), [&](const auto &target) {
        return steam_layout_inside_fence(original, target);
    });
    require(inside > 0 && inside < static_cast<int>(layout.size()),
            "a new village can start the blueprint but needs legitimate expansion to finish it");
    const auto school = std::find_if(layout.begin(), layout.end(),
                                     [](const auto &target) { return target.definition == 63; });
    require(
        school != layout.end() && school->anchor == ref::Position{13, 8} &&
            school->orientation == second &&
            steam_layout_footprint(original, *school).cells.size() == 4,
        "school must preserve audited second orientation and its complete four-cell reservation");

    // Pay for one presently legal target through the real construction consumer.
    // This is a short query contract, not a claim that the whole village was constructed.
    std::optional<State> placed;
    std::optional<std::uint64_t> built;
    std::size_t slot{};
    for (; slot < layout.size(); ++slot) {
        const auto &target = layout[slot];
        if (!steam_layout_inside_fence(original, target) ||
            !steam_layout_blockers(original, target).empty() || target.role != Role::shop)
            continue;
        auto candidate = original;
        if (simulation::begin_startup_world_build(candidate, target.definition).error !=
            simulation::StartupWorldRuntimeError::none)
            continue;
        const auto result =
            simulation::confirm_startup_world_build(candidate, target.anchor, target.orientation);
        if (result.error == simulation::StartupWorldRuntimeError::none && result.created) {
            placed = std::move(candidate);
            built = result.created;
            break;
        }
    }
    require(placed && built,
            "initial source world must afford at least one real target construction");
    const auto &s = *placed;
    require(steam_layout_match(s, layout[slot]) == built && steam_layout_slot(s, *built) == slot,
            "an already placed target must be protected from later relocation");
    require(steam_layout_blockers(s, layout[slot]) == std::vector<std::uint64_t>{*built},
            "query must report the actual occupying instance, not an empty target");
    require(s.scene.world.world.ai.accounting.funds() <
                original.scene.world.world.ai.accounting.funds(),
            "the construction fixture must pay through the ordinary source consumer");

    // Minimal read-only projection fixture: exercise the three delivered house
    // definition identities without claiming a naturally reached upgrade or residency.
    auto family = s;
    auto &p = family.scene.world.world.facilities.at(*built).placement;
    const auto home = std::find_if(layout.begin(), layout.end(), [](const auto &target) {
        return target.role == Role::residence;
    });
    require(home != layout.end(), "audited housing targets are present");
    p.anchor = home->anchor;
    p.orientation = home->orientation;
    for (const int definition : {25, 26, 27}) {
        p.definition_id = definition;
        require(steam_layout_match(family, *home) == built && steam_layout_slot(family, *built),
                "residence upgrades must not schedule a downgrade or demolition");
    }
    p.definition_id = 24;
    require(!steam_layout_match(family, *home), "recruitment is not an occupied residence upgrade");
    p.definition_id = 25;
    p.orientation = p.orientation == first ? second : first;
    require(!steam_layout_match(family, *home),
            "wrong facing cannot certify the audited housing slot");
}
} // namespace ark::test
