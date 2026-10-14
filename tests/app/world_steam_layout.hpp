#pragma once

#include "ark/simulation/world/startup_world_runtime.hpp"
#include <array>
#include <utility>

namespace ark::test {
enum class SteamLayoutRole { shop, plant, residence, recruitment };
// A target from the user's audited four-star layout, not a source rule or free fixture.
// Array positions are stable strategy slots; source instance identities are not imported.
struct SteamLayoutTarget {
    int definition{};
    simulation::rules::Position anchor;
    simulation::rules::FacilityOrientation orientation{};
    SteamLayoutRole role{};
};
const std::array<SteamLayoutTarget, 50> &steam_village_layout();

// Geometry uses the product's current definition shapes, never Steam price/stat fields.
simulation::rules::GeometryResult
steam_layout_footprint(const simulation::StartupWorldRuntimeState &state,
                       const SteamLayoutTarget &target);
simulation::rules::GeometryError
validate_steam_village_layout(const simulation::StartupWorldRuntimeState &state);
bool steam_layout_inside_fence(const simulation::StartupWorldRuntimeState &state,
                               const SteamLayoutTarget &target);
// Includes incomplete construction. Residence upgrades 25 -> 26/27 retain their slot;
// the caller must separately verify completion, resident assignment and paid business.
std::optional<std::uint64_t> steam_layout_match(const simulation::StartupWorldRuntimeState &state,
                                                const SteamLayoutTarget &target);
// Protect an already fulfilled slot when choosing an instance to relocate.
std::optional<std::size_t> steam_layout_slot(const simulation::StartupWorldRuntimeState &state,
                                             std::uint64_t facility);
// Existing facilities intersecting the entire target footprint, including a matching
// instance. Empty does not certify terrain, road connectivity, unlocks or affordability.
std::vector<std::uint64_t> steam_layout_blockers(const simulation::StartupWorldRuntimeState &state,
                                                 const SteamLayoutTarget &target);
// Discard complete source build/move candidates after checking every existing blueprint
// instance and the new shop. Prefer fewer covered road cells; no preview is a paid receipt.
std::optional<SteamLayoutTarget>
steam_connected_extension(const simulation::StartupWorldRuntimeState &state, int definition,
                          std::optional<std::uint64_t> moving = {});
// Move a non-blueprint shop bordering a disconnected target; never move a retained residence.
std::optional<std::pair<std::uint64_t, SteamLayoutTarget>>
steam_layout_access_repair(const simulation::StartupWorldRuntimeState &state);
// Keep the reference layout/residents complete; the separate next-shop gate also
// waits for external shops to open and trade. Both use durable state after cold load.
bool steam_reference_layout_complete(const simulation::StartupWorldRuntimeState &state);
bool steam_shop_expansion_ready(const simulation::StartupWorldRuntimeState &state);
void steam_layout_contract();
} // namespace ark::test
