#pragma once

// Shared catalog grants and ground-object updates. Equipment is an unlock, not an inventory.
#include "ark/simulation/combat/rules/battle_commit.hpp"
#include "ark/simulation/ai/rules/object_ai.hpp"

#include <array>
#include <map>
#include <set>

namespace ark::simulation::rules {
struct ObjectCatalogRecord {
    std::uint32_t flags{};
    int status{};          // Definition p.
    int unlock_counter{};  // q, reset only by ordinary-item b().
    bool newly_unlocked{}; // r.
    int inventory{};       // Item z, capped at999.
    int free_purchases{};  // Weapon v, armor/accessory k; set1 only on p0 unlock.
};
struct ObjectShopRecord {
    int category{};                          // Definition g:1 weapon,4 armor,5 accessory.
    std::vector<std::array<int, 2>> notices; // Tenant p: kind,counter; c(7) deduplicates by kind.
};
struct ObjectCommitState {
    std::map<std::pair<int, int>, ObjectCatalogRecord> catalog; // (kind, original definition ID).
    std::map<std::uint64_t, ObjectShopRecord> shops;
    std::vector<std::uint64_t> shop_order; // Original UserData.g order, not sorted stable IDs.
    std::map<std::uint64_t, GroundObjectState> objects;
    std::set<int> events;
    int item_rewards{}; // UserData E, NOT inventory or money.
};
enum class ObjectGrantOrigin { ground_pickup, direct };
enum class ObjectCommitRequestKind { ground_effect, notice, grant, event, shop_notice };
struct ObjectCommitRequest {
    ObjectCommitRequestKind kind{};
    int parameter{};
    int definition{};
    std::optional<std::uint64_t> shop;
};
struct ObjectCommitCandidate {
    ObjectCommitState state;
    bool remove{};
    std::vector<ObjectCommitRequest> requests; // Exact notice/grant/nested event order.
};
enum class ObjectCommitError { none, invalid_input, stale_object, missing_definition };
struct ObjectCommitResult {
    ObjectCommitError error{ObjectCommitError::none};
    std::optional<ObjectCommitCandidate> candidate;
};
// Direct c.e.a(kind,id) differs from timed pickup: item notice order and weapon event110.
ObjectCommitResult prepare_object_grant(const ObjectCommitState &state, int kind, int definition,
                                        ObjectGrantOrigin origin);
// Re-resolves object identity and catalog; reward only at new j20, erases only at new j>=60.
// Caller supplies town membership of the object's cached creation cell, not its current position.
ObjectCommitResult prepare_object_update(const ObjectCommitState &state, ObjectId object,
                                         bool cached_cell_town);
struct PickupCommitInput {
    CharacterId actor;
    std::vector<std::uint64_t> object_order; // Source bp order, may not be sorted stable IDs.
    bool battle_ready{}; // Fresh F result, has priority over carrying guard and H.
    bool touching{};     // Exact source collision-frame result for the freshly selected H object.
    int legacy_u{};
    std::optional<int> boost_ticket;
};
struct PickupCommitCandidate {
    BattleCommitState state;
    PickupAction action{};
    std::optional<ObjectId> object;
    std::optional<CombatPoint> chase_target; // No fabricated grid route or eager teleport.
    std::optional<int> expression;
};
struct PickupCommitResult {
    ObjectCommitError error{ObjectCommitError::none};
    std::optional<PickupCommitCandidate> candidate;
};
// Atomic state11 -> object5/actor12. Never grants inventory or writes Character.N at contact.
PickupCommitResult prepare_ground_pickup_commit(const BattleCommitState &state,
                                                const PickupCommitInput &input);
} // namespace ark::simulation::rules
