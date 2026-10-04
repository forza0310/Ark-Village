#pragma once

// Ground objects and pickup timing, distinct from Character.N carried-object/rescue storage.
#include "dungeon_village_reference/combat_execution.hpp"

namespace dungeon_village_reference {
struct ObjectId {
    std::uint64_t value{};
};
inline bool operator==(ObjectId a, ObjectId b) { return a.value == b.value; }
enum class ObjectError { none, invalid_input, missing_ticket, invalid_ticket, stale_object };
struct DropDefinition {
    int id{};
    int kind{}; // 0 item,1 weapon,2 armor,3 accessory.
    int rank{};
    std::uint32_t flags{};
    int unlocked{}; // Equipment must be !=1; items ignore this field.
};
struct DropSelectionInput {
    int luck{};             // Killer e.x[5].
    int progress{};         // UserData.k.
    int equipment_ticket{}; // 100, below15 chooses equipment branch with item fallback.
    int rank_ticket{};      // 60, always consumed even with no possible drop.
    std::vector<DropDefinition> definitions; // Items then weapon/armor/accessory source order.
    std::optional<int> selection_ticket;
    CombatRandomDraw draw{}; // provider存在时取100/60，只有非空实际目录才取selection bound。
};
struct DropSelectionCandidate {
    int maximum_rank{};
    bool equipment_branch{};
    bool consumed_selection{};
    std::optional<DropDefinition> selected; // Empty is no drop, not a fabricated default item.
};
struct DropSelectionResult {
    ObjectError error{ObjectError::none};
    std::optional<DropSelectionCandidate> candidate;
};
DropSelectionResult prepare_drop_selection(const DropSelectionInput &input);

struct GroundObjectState {
    ObjectId id;
    int state{};    // c.e.i, not character state.
    int counter{};  // j, incremented BEFORE object branch.
    int duration{}; // g for state2.
    int delay{};    // n for state6.
    int kind{};
    int definition{};
    CombatPoint position;
    CombatPoint velocity;
    CombatPoint acceleration;
    Position cached_cell; // h, whole grid, stays unchanged after movement/pickup repositioning.
    bool inside_town{};   // m, refreshed from cached h on each object update.
};
enum class ObjectRewardRequestKind {
    pickup_ground_effect,
    grant_definition,
    notice,
    item_count,
    event151
};
struct ObjectRewardRequest {
    ObjectRewardRequestKind kind{};
    int parameter{};
};
struct GroundObjectStepCandidate {
    GroundObjectState state;
    bool remove{};
    std::vector<ObjectRewardRequest> requests;
};
struct GroundObjectStepResult {
    ObjectError error{ObjectError::none};
    std::optional<GroundObjectStepCandidate> candidate;
};
// Creation candidates do not allocate IDs or append to the owner's bp vector.
std::optional<GroundObjectState> prepare_ground_drop(ObjectId id, CombatPoint position, int kind,
                                                     int definition);
// Caller has already selected a legal neighboring destination and its two random offsets.
std::optional<GroundObjectState> prepare_ground_throw(ObjectId id, CombatPoint origin,
                                                      CombatPoint destination, int kind,
                                                      int definition);
// Type2 thrown object lands into3; type6 waits into3; type5 grants at new j20, removes at60.
// reward count is global UserData.E; item_count request carries the new modulo count.
GroundObjectStepResult advance_ground_object(const GroundObjectState &state, bool cached_cell_town,
                                             int item_count, bool event151_present);
struct ObjectProbe {
    ObjectId id;
    int state{};
    CombatPoint position;
};
struct ObjectSelectionResult {
    ObjectError error{ObjectError::none};
    std::optional<ObjectId> selected;
};
// H() distance order uses reverse strict exchange, like e()/I(), filtering only object state3.
ObjectSelectionResult select_ground_object(CombatPoint actor,
                                           const std::vector<ObjectProbe> &objects);
enum class PickupAction { baseline, battle_prepare, chase, pickup };
struct PickupInput {
    bool battle_ready{}; // F(), checked before H() and carrying guard.
    int carried_slot{-1};
    CombatPoint actor_position;
    std::optional<GroundObjectState> nearest;
    bool touching{}; // Caller resolves original human/object collision frames.
};
struct PickupCandidate {
    PickupAction action{PickupAction::baseline};
    std::optional<GroundObjectState> object;
    std::vector<LegacyActorControl> queue;
    std::optional<int> actor_state;
    std::optional<int> facing;
    std::optional<int> expression;
};
struct PickupResult {
    ObjectError error{ObjectError::none};
    std::optional<PickupCandidate> candidate;
};
// State11; H() must be re-queried by the owner. Pickup leaves Character.N untouched.
PickupResult prepare_ground_pickup(const PickupInput &input);
} // namespace dungeon_village_reference
