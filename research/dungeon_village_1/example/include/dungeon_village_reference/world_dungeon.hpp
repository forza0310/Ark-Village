#pragma once

// Expedition is a facility activity. Canonical occupation/HP/control remain in the shared world.
#include "dungeon_village_reference/dungeon_ai.hpp"
#include "dungeon_village_reference/object_commit.hpp"
#include "dungeon_village_reference/rescue_commit.hpp"

#include <functional>

namespace dungeon_village_reference {
struct DungeonFacilityProgress {
    int updates{}; // f, caller supplies the post-prefix count; this module does not run Tenant.d.
    int progress{};
    int extent{};
    int percent{};
    int previous_percent{};
    std::vector<DungeonChallenge> challenges;
};
struct DungeonWorldState {
    RescueWorldState world;
    std::map<std::uint64_t, DungeonFacilityProgress> facilities;
    std::map<CharacterId, DungeonActorProgress> actors; // One V/W/X/Y/Z/aa/bw owner per object.
    std::map<std::pair<int, int>, ObjectCatalogRecord> catalog;
    std::map<std::uint64_t, ObjectShopRecord> shops;
    std::vector<std::uint64_t> shop_order;
    int item_rewards{};
};
struct DungeonLaunchTickets {
    int destination{}; // Current legal candidate count, then two offsets80.
    int x_offset{};
    int z_offset{};
};
enum class DungeonWorldError {
    none,
    invalid_input,
    stale_actor,
    stale_facility,
    missing_ticket,
    preparation_failed
};
struct DungeonWorldRequest {
    DungeonCrewRequest source;
    std::vector<ObjectCommitRequest> catalog_requests;
    std::optional<ObjectId> object;
};
struct DungeonWorldCandidate {
    DungeonWorldState state;
    std::vector<DungeonWorldRequest> requests;
    std::size_t consumed_launches{};
    bool completed{};
    bool removed_occupation{};
    bool landed{};
    std::optional<int> entry_event;
};
struct DungeonWorldResult {
    DungeonWorldError error{DungeonWorldError::none};
    std::optional<DungeonWorldCandidate> candidate;
};
// Front opcode21 reads the CURRENT O tile's instance; no extra old-s/status guard.
// Re-entry resets progress, preserves bw and appends duplicates. Empty uniform >=6 records
// can issue168/169 or170/171; only the actually emitted notice consumes a draw2.
DungeonWorldResult prepare_world_dungeon_entry(const DungeonWorldState &state, CharacterId actor,
                                               std::optional<int> notice_ticket = {});
// Actual Character.a(cell): teleport BEFORE checking eight outside-state4 cells. No target
// succeeds as a no-launch candidate and consumes no tickets; q removal uses the new s.
DungeonWorldResult prepare_world_dungeon_retreat(const DungeonWorldState &state, CharacterId actor,
                                                 Position origin, TownBounds town,
                                                 std::optional<DungeonLaunchTickets> tickets = {});
struct DungeonWorldCrewInput {
    std::uint64_t facility{};
    TownBounds town;
    std::optional<int> active_task_extent;
    std::vector<DungeonLaunchTickets> launches; // Actual successful retreats/throws, source order.
};
// Phase1 after Tenant counter/notices: live retreat -> crew rules -> direct grants/ground throws
// -> c0/wait/index*5/activity0. All domains commit only if the entire candidate succeeds.
DungeonWorldResult prepare_world_dungeon_crew(const DungeonWorldState &state,
                                              const DungeonWorldCrewInput &input);
using DungeonLandingDeparture =
    std::function<std::optional<DungeonWorldState>(const DungeonWorldState &, CharacterId)>;
// State20 c: move horizontally first, oldB>=36 land -> OLD cached u display -> c0 -> DIRECT
// o0 -> c2. The callback operates only on the private candidate; no-route is a successful
// candidate too. Missing/failed consumers roll back, never turn this into queued activity8.
DungeonWorldResult prepare_world_dungeon_landing(const DungeonWorldState &state, CharacterId actor,
                                                 std::optional<Position> old_view = {},
                                                 const DungeonLandingDeparture &departure = {});
} // namespace dungeon_village_reference
