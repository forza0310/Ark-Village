#include "world_steam_layout.hpp"
#include "../support/world_fixture.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"
#include "ark/simulation/facilities/startup_world_editing.hpp"
#include "ark/simulation/map/startup_world_expansion.hpp"
#include "ark/simulation/persistence/startup_world_persistence.hpp"

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

namespace {
using ExtensionGain = std::array<std::int64_t, 3>;
bool extension_candidate(const State &s, const SteamLayoutTarget &target,
                         std::optional<std::uint64_t> moving, ExtensionGain *gain = nullptr) {
    auto candidate = s; // Original references, route caches, residents and random stay independent.
    simulation::StartupBuildResult built;
    if (moving) {
        const auto opened = simulation::begin_startup_world_edit(candidate, true);
        if (opened.error != simulation::StartupWorldRuntimeError::none ||
            opened.denial != simulation::StartupBuildDenial::none)
            return false;
        const auto selected = simulation::confirm_startup_world_edit(
            candidate, s.scene.world.world.facilities.at(*moving).placement.anchor,
            ref::FacilityOrientation::first);
        if (selected.error != simulation::StartupWorldRuntimeError::none ||
            selected.denial != simulation::StartupBuildDenial::none)
            return false;
        built =
            simulation::confirm_startup_world_edit(candidate, target.anchor, target.orientation);
    } else {
        const auto opened = simulation::begin_startup_world_build(candidate, target.definition);
        if (opened.error != simulation::StartupWorldRuntimeError::none ||
            opened.denial != simulation::StartupBuildDenial::none)
            return false;
        built =
            simulation::confirm_startup_world_build(candidate, target.anchor, target.orientation);
    }
    // Both source transactions already recompute connection flags, preserving their exact
    // placement/retirement order. Do not replace that consumer with a copied path algorithm.
    if (built.error != simulation::StartupWorldRuntimeError::none ||
        built.denial != simulation::StartupBuildDenial::none || !built.created ||
        (candidate.facility_flags.at(*built.created) & 1U))
        return false;
    // Keep the delivered distance-field admission; plant neighbour effects are scored separately.
    const auto paths = ref::search_legacy_map(candidate.scene.world.world.map,
                                              simulation::startup_evidence().spawn_points.at(0));
    if (!paths.field)
        return false;
    std::vector<ref::FacilityPlacement> placements;
    for (const auto id : candidate.scene.world.facility_order)
        placements.push_back(candidate.scene.world.world.facilities.at(id).placement);
    const auto access = ref::inspect_facility_access(*paths.field, placements);
    if (access.error != ref::MapAccessError::none)
        return false;
    const auto reachable = [&](std::uint64_t id) {
        const auto status =
            std::find_if(access.facilities.begin(), access.facilities.end(),
                         [id](const auto &entry) { return entry.instance_id.value == id; });
        return status != access.facilities.end() && !status->cells.empty();
    };
    // A connected shop can still close the only gate into the adventure area.
    // Keep both real gates reachable through the source search; do not infer
    // global connectivity from the new shop's own connection flag.
    for (const auto &[id, gate] : s.scene.world.world.facilities)
        if ((gate.kind == 4 || gate.kind == 5) &&
            (!candidate.scene.world.world.facilities.count(id) || !reachable(id)))
            return false;
    const bool plant = candidate.scene.world.world.facilities.at(*built.created).kind == 2;
    if (!reachable(*built.created))
        return false;
    for (const auto &goal : layout) {
        const auto before = steam_layout_match(s, goal);
        if (!before)
            continue;
        const auto after = steam_layout_match(candidate, goal);
        if (after != before || (candidate.facility_flags.at(*after) & 1U) ||
            (goal.role != Role::plant && !reachable(*after)) ||
            (goal.role == Role::residence &&
             candidate.facility_residents.at(*after) != s.facility_residents.at(*before)))
            return false;
    }
    // Later admissions live outside the fixed blueprint. Preserve every occupied
    // home through a new shop/plot or an explicit move, using source connection queries.
    for (const auto &[id, home] : s.scene.world.world.facilities) {
        if (home.kind != 12 || s.facility_residents.at(id) < 0)
            continue;
        const auto kept = candidate.scene.world.world.facilities.find(id);
        if (kept == candidate.scene.world.world.facilities.end() || kept->second.kind != 12 ||
            kept->second.placement.definition_id != home.placement.definition_id ||
            candidate.facility_residents.at(id) != s.facility_residents.at(id) ||
            (candidate.facility_flags.at(id) & 1U) || !reachable(id))
            return false;
    }
    // A later plot or shop must also preserve businesses outside the reference
    // layout. Already disconnected shops are repaired separately, one paid move
    // at a time; moving replaces its old instance through the source consumer.
    for (const auto &[id, shop] : s.scene.world.world.facilities) {
        if (shop.kind != 3 || moving == id || (s.facility_flags.at(id) & 1U))
            continue;
        const auto kept = candidate.scene.world.world.facilities.find(id);
        if (kept == candidate.scene.world.world.facilities.end() ||
            kept->second.placement.definition_id != shop.placement.definition_id ||
            (candidate.facility_flags.at(id) & 1U) || !reachable(id))
            return false;
    }
    if (plant && gain) {
        ExtensionGain actual{};
        for (const auto &[id, shop] : s.scene.world.world.facilities) {
            if (shop.kind != 3)
                continue;
            const auto before = simulation::startup_world_facility_values(s, id);
            const auto after = simulation::startup_world_facility_values(candidate, id);
            if (!before || !after)
                return false;
            for (std::size_t n = 0; n < actual.size(); ++n)
                actual[n] +=
                    std::int64_t(after->instance_attributes[n]) - before->instance_attributes[n];
        }
        *gain = actual; // Player planning only; the source consumer owns all adjacency arithmetic.
    }
    return true;
}
} // namespace

std::optional<SteamLayoutTarget> steam_connected_extension(const State &s, int definition,
                                                           std::optional<std::uint64_t> moving) {
    if (moving &&
        (!s.scene.world.world.facilities.count(*moving) || steam_layout_slot(s, *moving) ||
         s.scene.world.world.facilities.at(*moving).placement.definition_id != definition))
        return {}; // The source blueprint's occupied targets are protected, including houses.
    std::vector<std::pair<int, SteamLayoutTarget>> candidates;
    std::set<std::pair<int, int>> blueprint_cells;
    for (const auto &goal : layout)
        for (const auto &cell : steam_layout_footprint(s, goal).cells)
            blueprint_cells.emplace(cell.position.x, cell.position.y);
    const auto &map = s.scene.world.world.map;
    for (int y = 4; y < map.height; ++y)
        for (int x = 1; x < map.width; ++x)
            for (const auto orientation : {first, second}) {
                const SteamLayoutTarget target{
                    definition,
                    {x, y},
                    orientation,
                    s.rules->facilities.at(definition).kind == 2 ? Role::plant : Role::shop};
                const auto footprint = steam_layout_footprint(s, target);
                if (!steam_layout_inside_fence(s, target) ||
                    !steam_layout_blockers(s, target).empty())
                    continue;
                int covered_roads{};
                bool reserved{};
                for (const auto &cell : footprint.cells) {
                    const auto &tile = map.cells.at(cell.position.y * map.width + cell.position.x);
                    if (tile.legacy_state == 1 || tile.legacy_state == 2 || tile.legacy_state == 10)
                        reserved = true;
                    covered_roads += tile.category == ref::RouteCategory::road;
                    reserved |= blueprint_cells.count({cell.position.x, cell.position.y}) != 0;
                }
                if (!reserved)
                    candidates.emplace_back(covered_roads, target);
            }
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const auto &a, const auto &b) { return a.first < b.first; });
    std::optional<SteamLayoutTarget> plant;
    ExtensionGain best_gain{};
    for (const auto &[roads, target] : candidates) {
        (void)roads;
        ExtensionGain gain{};
        if (extension_candidate(s, target, moving, &gain)) {
            if (target.role != Role::plant)
                return target;
            if (gain > best_gain) {
                plant = target;
                best_gain = gain;
            }
        }
    }
    return plant;
}

std::optional<std::pair<std::uint64_t, SteamLayoutTarget>>
steam_layout_access_repair(const State &s) {
    const auto &map = s.scene.world.world.map;
    const auto paths = ref::search_legacy_map(map, simulation::startup_evidence().spawn_points.at(0));
    if (!paths.field)
        return {};
    std::set<std::uint64_t> gate_blockers;
    for (const auto &[id, gate] : s.scene.world.world.facilities) {
        if (gate.kind != 4 && gate.kind != 5)
            continue;
        const auto access = ref::inspect_facility_access(*paths.field, {gate.placement});
        if (access.error != ref::MapAccessError::none)
            return {};
        if (!access.facilities.front().cells.empty())
            continue;
        const auto ring = ref::facility_surroundings(gate.placement.shape, gate.placement.orientation,
                                                      gate.placement.anchor, map.width, map.height);
        if (ring.error != ref::GeometryError::none)
            return {};
        for (const auto &cell : ring.cells) {
            const auto &tile = map.cells.at(cell.y * map.width + cell.x);
            if (!tile.facility)
                continue;
            const auto blocker = tile.facility->instance_id.value;
            const auto &facility = s.scene.world.world.facilities.at(blocker);
            if (facility.kind == 3 && facility.status == 1 && !steam_layout_slot(s, blocker))
                gate_blockers.insert(blocker);
        }
    }
    for (const auto id : gate_blockers)
        if (const auto target = steam_connected_extension(
                s, s.scene.world.world.facilities.at(id).placement.definition_id, id))
            return std::pair{id, *target};
    for (const auto &[id, shop] : s.scene.world.world.facilities)
        if (shop.kind == 3 && shop.status == 1 && !steam_layout_slot(s, id) &&
            (s.facility_flags.at(id) & 1U))
            if (const auto target = steam_connected_extension(s, shop.placement.definition_id, id))
                return std::pair{id, *target};
    std::set<std::uint64_t> blockers;
    for (const auto &goal : layout) {
        const auto id = steam_layout_match(s, goal);
        if (!id || !(s.facility_flags.at(*id) & 1U))
            continue;
        const auto ring = ref::facility_surroundings(
            static_cast<ref::FacilityShape>(s.rules->facilities.at(goal.definition).shape),
            goal.orientation, goal.anchor, map.width, map.height);
        if (ring.error != ref::GeometryError::none)
            continue;
        for (const auto &cell : ring.cells) {
            const auto &tile = map.cells.at(cell.y * map.width + cell.x);
            if (!tile.facility)
                continue;
            const auto blocker = tile.facility->instance_id.value;
            const auto &facility = s.scene.world.world.facilities.at(blocker);
            if (facility.kind == 3 && facility.status == 1 && !steam_layout_slot(s, blocker))
                blockers.insert(blocker);
        }
    }
    for (const auto id : blockers)
        if (const auto target = steam_connected_extension(
                s, s.scene.world.world.facilities.at(id).placement.definition_id, id))
            return std::pair{id, *target};
    return {};
}

bool steam_reference_layout_complete(const State &s) {
    if (s.rank < 4)
        return false;
    for (const auto &goal : layout) {
        const auto id = steam_layout_match(s, goal);
        if (!id || s.scene.world.world.facilities.at(*id).status != 1 ||
            (s.facility_flags.at(*id) & 1U) ||
            (goal.role == Role::residence && s.facility_residents.at(*id) < 0))
            return false;
    }
    return true;
}

bool steam_shop_expansion_ready(const State &s) {
    if (!steam_reference_layout_complete(s))
        return false;
    for (const auto &[id, facility] : s.scene.world.world.facilities)
        if (facility.kind == 3 && !steam_layout_slot(s, id) &&
            (facility.status != 1 || facility.sales <= 0 || (s.facility_flags.at(id) & 1U)))
            return false;
    return true;
}

std::optional<std::uint64_t> steam_extra_recruitment_plot(const State &s) {
    for (const auto &[id, facility] : s.scene.world.world.facilities)
        if (facility.placement.definition_id == 24 && !steam_layout_slot(s, id))
            return id;
    return {};
}

bool steam_housing_expansion_needed(const State &s) {
    if (static_cast<std::size_t>(s.fence_level + 1) >= s.rules->fences.size() ||
        !steam_reference_layout_complete(s) || steam_extra_recruitment_plot(s))
        return false;
    const auto candidates = simulation::startup_world_residence_candidates(s);
    return candidates && !candidates->empty();
}

namespace {
void gate_access_contract() {
    const auto require = [](bool value, const char *message) {
        if (!value)
            throw std::runtime_error(std::string("Steam gate access: ") + message);
    };
    // Both shops may trade while jointly sealing the south gate. Source
    // placement/search produce this fixture; it does not certify a natural task.
    auto gates = initial_world();
    gates.scene.world.world.ai.accounting = ref::PeriodAccounting(100000);
    gates.scripts.user_flags |= 32U;
    gates.scripts.event_calls[115] = 1;
    for (const auto id : std::vector<std::uint64_t>(gates.scene.world.facility_order)) {
        const auto kind = gates.scene.world.world.facilities.at(id).kind;
        if (kind != 4 && kind != 5)
            require(simulation::retire_startup_world_facility(gates, id),
                    "fixture must retire unneeded initial buildings");
    }
    require(simulation::expand_startup_world_map(gates), "fixture expansion failed");
    const auto gate = std::find_if(gates.scene.world.world.facilities.begin(),
                                   gates.scene.world.world.facilities.end(),
                                   [](const auto &entry) { return entry.second.kind == 5; });
    require(gate != gates.scene.world.world.facilities.end(), "real south gate is required");
    const auto gate_id = gate->first;
    const auto gate_reachable = [&](const State &owner) {
        const auto paths = ref::search_legacy_map(owner.scene.world.world.map,
                                                  simulation::startup_evidence().spawn_points.at(0));
        require(paths.field.has_value(), "source search must succeed");
        const auto access = ref::inspect_facility_access(
            *paths.field, {owner.scene.world.world.facilities.at(gate_id).placement});
        require(access.error == ref::MapAccessError::none, "gate binding must stay valid");
        return !access.facilities.front().cells.empty();
    };
    require(gate_reachable(gates), "unobstructed expanded gate must be reachable");
    const auto anchor = gate->second.placement.anchor;
    const auto left = simulation::install_startup_world_facility(
        gates, 35, {anchor.x, anchor.y - 1}, first);
    require(left.created.has_value(), "first inner shop must be a legal placement");
    gates.scene.world.world.facilities.at(*left.created).status = 1;
    require(gate_reachable(gates), "one open gate tile must preserve exterior access");
    const SteamLayoutTarget sealing{35, {anchor.x + 1, anchor.y - 1}, first, Role::shop};
    const auto before = simulation::startup_world_state_digest(gates);
    require(!extension_candidate(gates, sealing, {}) &&
                simulation::startup_world_state_digest(gates) == before,
            "connected placement must not seal the last exterior passage or mutate its input");
    const auto right = simulation::install_startup_world_facility(gates, 35, sealing.anchor, first);
    require(right.created.has_value(), "second inner shop must reproduce the obstruction");
    gates.scene.world.world.facilities.at(*right.created).status = 1;
    require(!gate_reachable(gates) && !(gates.facility_flags.at(*left.created) & 1U) &&
                !(gates.facility_flags.at(*right.created) & 1U),
            "both shops must remain connected while their shared gate is unreachable");
    const auto blocked = simulation::startup_world_state_digest(gates);
    const auto repair = steam_layout_access_repair(gates);
    require(repair && (repair->first == *left.created || repair->first == *right.created) &&
                simulation::startup_world_state_digest(gates) == blocked,
            "repair must select a gate blocker without changing Owner/random");
    auto moved = gates;
    const auto funds = moved.scene.world.world.ai.accounting.funds();
    require(simulation::begin_startup_world_edit(moved, true).error ==
                    simulation::StartupWorldRuntimeError::none &&
                simulation::confirm_startup_world_edit(
                    moved, gates.scene.world.world.facilities.at(repair->first).placement.anchor,
                    first).error == simulation::StartupWorldRuntimeError::none,
            "repair must open the original move transaction");
    const auto result = simulation::confirm_startup_world_edit(
        moved, repair->second.anchor, repair->second.orientation);
    require(result.created && gate_reachable(moved) &&
                moved.scene.world.world.ai.accounting.funds() == funds - 300 &&
                simulation::cancel_startup_world_edit(moved) ==
                    simulation::StartupWorldRuntimeError::none,
            "paid repair must restore source reachability and return normally");
}
} // namespace

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

    // An isolated map/affordability qualification reproduces the obstructed left entrance.
    // No actor update, natural unlock, resident grant or campaign coverage is claimed.
    auto corner = initial_world();
    corner.scene.world.world.ai.accounting = ref::PeriodAccounting(100000);
    corner.scripts.user_flags |= 32U;
    corner.scripts.event_calls[115] = 1; // This planning fixture has already seen the warning.
    corner.facility_presence.at(64) = 2;
    const auto old_facilities = corner.scene.world.facility_order;
    for (const auto id : old_facilities) {
        // Expansion replaces the real entrances; retiring them first closes the fence.
        const auto kind = corner.scene.world.world.facilities.at(id).kind;
        if (kind == 4 || kind == 5)
            continue;
        require(simulation::retire_startup_world_facility(corner, id),
                "corner fixture retirement failed");
    }
    require(simulation::expand_startup_world_map(corner), "corner fixture expansion failed");
    std::uint64_t protected_home{};
    for (const ref::Position anchor : {ref::Position{4, 3}, {4, 4}, {4, 6}}) {
        const auto installed =
            simulation::install_startup_world_facility(corner, 25, anchor, first);
        require(installed.created.has_value(), "corner fixture house insertion failed");
        if (anchor == ref::Position{4, 4})
            protected_home = *installed.created;
    }
    const auto road = simulation::begin_startup_world_road(corner, 18);
    require(road.error == simulation::StartupWorldRuntimeError::none &&
                simulation::confirm_startup_world_edit(corner, {5, 3}, first).error ==
                    simulation::StartupWorldRuntimeError::none &&
                simulation::confirm_startup_world_edit(corner, {5, 6}, first).error ==
                    simulation::StartupWorldRuntimeError::none &&
                simulation::cancel_startup_world_edit(corner) ==
                    simulation::StartupWorldRuntimeError::none &&
                simulation::refresh_startup_world_connections(corner),
            "corner fixture must start with genuine accessible road and house");
    require(!(corner.facility_flags.at(protected_home) & 1U),
            "the original corner residence must be connected before the extra shop");
    const SteamLayoutTarget obstructing{64, {5, 4}, first, Role::shop};
    // Only a non-blueprint occupied home remains, so reference-slot protection
    // cannot hide a missing guard for later residents. This is a placement qualification.
    auto external_only = corner;
    for (const auto &[id, facility] : corner.scene.world.world.facilities)
        if (facility.kind == 12)
            require(simulation::retire_startup_world_facility(external_only, id),
                    "external-home fixture retirement failed");
    // Unoccupied, differently facing houses close the alternative path without
    // being protected reference slots or another occupied-home rejection reason.
    for (const ref::Position anchor : {ref::Position{4, 3}, {4, 6}}) {
        const auto barrier =
            simulation::install_startup_world_facility(external_only, 25, anchor, second);
        require(barrier.created && !steam_layout_slot(external_only, *barrier.created) &&
                    external_only.facility_residents.at(*barrier.created) < 0,
                "external-home path barriers must be unoccupied non-reference instances");
    }
    const auto extra_home =
        simulation::install_startup_world_facility(external_only, 25, {4, 5}, first);
    require(extra_home.created.has_value(), "external-home fixture insertion failed");
    const auto extra_id = *extra_home.created;
    external_only.facility_residents.at(extra_id) = 0;
    require(!steam_layout_slot(external_only, extra_id) &&
                !(external_only.facility_flags.at(extra_id) & 1U),
            "later occupied home must be connected outside the reference slots");
    auto blocked_external = external_only;
    const auto blocking = simulation::install_startup_world_facility(
        blocked_external, 64, obstructing.anchor, obstructing.orientation);
    require(blocking.created.has_value(), "external-home blocker insertion failed");
    require(!(blocked_external.facility_flags.at(*blocking.created) & 1U),
            "external-home blocking shop must remain connected");
    require(blocked_external.facility_flags.at(extra_id) & 1U,
            "external-home blocker must actually close the occupied home's entrance");
    require(!extension_candidate(external_only, obstructing, {}),
            "a connected shop cannot certify disconnecting a later occupied home");
    auto external_business = external_only;
    require(simulation::retire_startup_world_facility(external_business, extra_id),
            "external-shop fixture must replace its unneeded house");
    const auto retained_shop =
        simulation::install_startup_world_facility(external_business, 50, {4, 5}, first);
    require(retained_shop.created &&
                !steam_layout_slot(external_business, *retained_shop.created) &&
                !(external_business.facility_flags.at(*retained_shop.created) & 1U),
            "a real connected non-reference shop must precede the new placement");
    external_business.scene.world.world.facilities.at(*retained_shop.created).status = 1;
    auto disconnected_business = external_business;
    const auto later_shop = simulation::install_startup_world_facility(
        disconnected_business, 64, obstructing.anchor, obstructing.orientation);
    require(later_shop.created &&
                (disconnected_business.facility_flags.at(*retained_shop.created) & 1U) &&
                !(disconnected_business.facility_flags.at(*later_shop.created) & 1U),
            "the new shop must actually block only the earlier non-reference business");
    require(!extension_candidate(external_business, obstructing, {}),
            "a new connected placement must not disconnect an earlier external business");
    const auto disconnected_digest = simulation::startup_world_state_digest(disconnected_business);
    const auto business_repair = steam_layout_access_repair(disconnected_business);
    require(business_repair && business_repair->first == *retained_shop.created &&
                extension_candidate(disconnected_business, business_repair->second,
                                    business_repair->first) &&
                simulation::startup_world_state_digest(disconnected_business) ==
                    disconnected_digest,
            "repair must find a paid source move without changing its disconnected Owner");
    const auto unchanged = corner;
    require(!extension_candidate(corner, obstructing, {}),
            "an empty legal footprint must not certify a build that disconnects a retained home");
    const auto destination = steam_connected_extension(corner, 64);
    require(destination && extension_candidate(corner, *destination, {}) &&
                same_world_clock(corner, unchanged) &&
                corner.page_counters == unchanged.page_counters &&
                corner.sound_requests == unchanged.sound_requests &&
                corner.scene.random.draws() == unchanged.scene.random.draws() &&
                corner.scene.world.world.ai.accounting.funds() ==
                    unchanged.scene.world.world.ai.accounting.funds() &&
                corner.facility_flags == unchanged.facility_flags,
            "connected extension query must discard every private paid candidate");
    require(!steam_connected_extension(corner, 25, protected_home),
            "repair planning must never relocate a protected blueprint residence");
    gate_access_contract();

    // Plant qualification: source placement/adjacency, with an explicit unlocked
    // definition and cash. This is not a natural recipe, claim or village build.
    auto garden = initial_world();
    garden.scene.world.world.ai.accounting = ref::PeriodAccounting(100000);
    garden.facility_presence.at(74) = 2;
    garden.facility_free_builds.at(74) = 1;
    garden.scripts.event_calls[115] = 1;
    const auto shop = simulation::install_startup_world_facility(garden, 35, {12, 6}, first);
    require(shop.created.has_value(), "plant fixture needs a real source shop placement");
    const auto garden_digest = simulation::startup_world_state_digest(garden);
    const auto tree = steam_connected_extension(garden, 74);
    require(tree && tree->role == Role::plant &&
                simulation::startup_world_state_digest(garden) == garden_digest,
            "plant planning must find source economic gain without modifying its Owner");
    auto planted = garden;
    require(simulation::begin_startup_world_build(planted, 74).error ==
                simulation::StartupWorldRuntimeError::none,
            "plant choice must use the original paid build entry");
    const auto built_tree =
        simulation::confirm_startup_world_build(planted, tree->anchor, tree->orientation);
    require(built_tree.created.has_value(), "plant choice must actually build through the source");
    require(simulation::cancel_startup_world_build(planted) ==
                simulation::StartupWorldRuntimeError::none,
            "source plant placement must leave build mode normally");
    bool improved{};
    for (const auto &[id, business] : garden.scene.world.world.facilities) {
        if (business.kind != 3)
            continue;
        const auto before = simulation::startup_world_facility_values(garden, id);
        const auto after = simulation::startup_world_facility_values(planted, id);
        require(before && after, "planted source economy must remain queryable");
        if (after->instance_attributes[0] > before->instance_attributes[0]) {
            require(after->instance_attributes[0] - before->instance_attributes[0] == 70 &&
                        after->instance_attributes[1] - before->instance_attributes[1] == 20,
                    "tree choice must apply the fixed source70/20 to an actual neighbouring shop");
            improved = true;
        }
    }
    require(improved, "the chosen plant must benefit a real existing shop");
    const auto inserted = simulation::install_startup_world_facility(corner, 64, {5, 4}, first);
    require(inserted.created.has_value(), "obstructing-shop qualification failed");
    corner.scene.world.world.facilities.at(*inserted.created).status = 1;
    require(corner.facility_flags.at(protected_home) & 1U,
            "ordinary source connection flags must detect the blocked corner");
    const auto repair = steam_layout_access_repair(corner);
    require(repair && repair->first == *inserted.created &&
                extension_candidate(corner, repair->second, repair->first),
            "repair must move the external shop and restore all retained source targets");

    // Minimal read-only placement qualification, not a valid full Owner or a
    // naturally built village. Only fields consumed by readiness are populated.
    auto readiness = original;
    readiness.rank = 4;
    const auto prototype = readiness.scene.world.world.facilities.begin()->second;
    readiness.scene.world.world.facilities.clear();
    readiness.facility_flags.clear();
    readiness.facility_residents.clear();
    std::uint64_t readiness_id = 1;
    std::uint64_t readiness_home{};
    for (const auto &goal : layout) {
        auto facility = prototype;
        facility.placement.instance_id.value = readiness_id;
        facility.placement.definition_id = goal.definition;
        facility.placement.shape =
            static_cast<ref::FacilityShape>(readiness.rules->facilities.at(goal.definition).shape);
        facility.placement.anchor = goal.anchor;
        facility.placement.orientation = goal.orientation;
        facility.kind = readiness.rules->facilities.at(goal.definition).kind;
        facility.status = 1;
        facility.sales = 1;
        readiness.scene.world.world.facilities.emplace(readiness_id, facility);
        readiness.facility_flags[readiness_id] = 0;
        readiness.facility_residents[readiness_id] = goal.role == Role::residence ? 0 : -1;
        if (goal.role == Role::residence)
            readiness_home = readiness_id;
        ++readiness_id;
    }
    require(steam_shop_expansion_ready(readiness),
            "complete, connected and occupied reference placements must permit expansion");
    const auto complete_projection = readiness;
    const std::array<const char *, 5> readiness_denials{{
        "rank3 must not extend a four-star reference layout",
        "a missing reference placement must prevent expansion",
        "an unfinished reference shop must prevent expansion",
        "a disconnected reference shop must prevent expansion",
        "an unoccupied reference residence must prevent expansion",
    }};
    for (int scenario = 0; scenario < 5; ++scenario) {
        auto denied = complete_projection;
        if (scenario == 0)
            denied.rank = 3;
        else if (scenario == 1)
            denied.scene.world.world.facilities.erase(1);
        else if (scenario == 2)
            denied.scene.world.world.facilities.at(1).status = 0;
        else if (scenario == 3)
            denied.facility_flags.at(1) = 1U;
        else
            denied.facility_residents.at(readiness_home) = -1;
        require(!steam_shop_expansion_ready(denied), readiness_denials.at(scenario));
    }
    auto external_shop = prototype;
    external_shop.placement.instance_id.value = readiness_id;
    external_shop.placement.definition_id = 54;
    external_shop.placement.shape =
        static_cast<ref::FacilityShape>(readiness.rules->facilities.at(54).shape);
    external_shop.placement.anchor = {1, 1};
    external_shop.placement.orientation = first;
    external_shop.kind = 3;
    external_shop.status = 0;
    external_shop.sales = 0;
    readiness.scene.world.world.facilities.emplace(readiness_id, external_shop);
    readiness.facility_flags.emplace(readiness_id, 0U);
    require(!steam_shop_expansion_ready(readiness),
            "external shop construction must prevent buying the next expansion");
    readiness.scene.world.world.facilities.at(readiness_id).status = 1;
    require(!steam_shop_expansion_ready(readiness) && steam_reference_layout_complete(readiness),
            "a zero-sales shop blocks the next purchase but must retain the blueprint budget gate");
    const auto waiting_projection = readiness;
    readiness.scene.world.world.facilities.at(readiness_id).sales = 1;
    require(steam_shop_expansion_ready(readiness) &&
                !steam_shop_expansion_ready(waiting_projection),
            "readiness must follow durable sales in each independent projection, not controller "
            "history");
    readiness.facility_flags.at(readiness_id) = 1U;
    require(!steam_shop_expansion_ready(readiness),
            "historical sales cannot certify a currently disconnected external shop");

    require(!steam_extra_recruitment_plot(complete_projection),
            "the permanent reference plot must not block later admissions");
    auto demand = complete_projection;
    demand.fence_level = 1;
    for (auto &[id, present] : demand.human_presence)
        present = id == 1 ? 1 : 0;
    demand.human_homes.at(1)[2] = 0;
    demand.shop_humans.at(1).satisfaction = demand.rules->humans.at(1).residence_threshold;
    require(steam_housing_expansion_needed(demand),
            "qualified later residents must retain the remaining source expansion goal");
    auto already_housed = demand;
    already_housed.human_homes.at(1)[2] = 1;
    auto final_fence = demand;
    final_fence.fence_level = static_cast<int>(demand.rules->fences.size()) - 1;
    require(!steam_housing_expansion_needed(already_housed) &&
                !steam_housing_expansion_needed(final_fence),
            "no homeless candidate or no source fence level must reject redundant expansion");
    // Read-only projections model the durable states that survive a player load;
    // no private controller history may permit a second unoccupied plot purchase.
    for (const int status : {0, 1}) {
        auto recruiting = demand;
        auto plot = prototype;
        plot.placement.instance_id.value = readiness_id;
        plot.placement.definition_id = 24;
        plot.placement.anchor = {1, 1};
        plot.placement.orientation = first;
        plot.kind = 13;
        plot.status = status;
        recruiting.scene.world.world.facilities.emplace(readiness_id, plot);
        const auto restored = recruiting;
        require(steam_extra_recruitment_plot(restored) == readiness_id,
                "an extra unfinished or ready plot must retain the admission reservation");
        require(
            !steam_housing_expansion_needed(restored),
            "an already purchased plot must finish admission before expanding for more housing");
        auto &house = recruiting.scene.world.world.facilities.at(readiness_id);
        house.placement.definition_id = 25;
        house.kind = 12;
        require(!steam_extra_recruitment_plot(recruiting) &&
                    steam_extra_recruitment_plot(restored) == readiness_id,
                "only actual plot-to-house conversion releases the next admission reservation");
    }
}
} // namespace ark::test
