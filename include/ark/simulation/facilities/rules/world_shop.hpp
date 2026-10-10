#pragma once

// Live shop transactions share the existing actor/facility/statistics owner; no window dependency.
#include "ark/simulation/combat/rules/object_commit.hpp"
#include "ark/simulation/combat/rules/rescue_commit.hpp"
#include "ark/simulation/ai/rules/weapon_choice.hpp"

namespace ark::simulation::rules {
struct ShopHumanRecord {
    std::array<std::optional<int>, 4> equipment;
    std::array<int, 4> reselect{};
    int satisfaction{};
};
struct ShopActorRecord {
    int weapon{}; // ae: only initiating actor changes on28, not all same-definition instances.
    std::optional<int> selected_weapon; // af/ag/ah, selected at ARRIVAL, not on exit.
    std::optional<int> selected_armor;
    std::optional<int> selected_accessory;
};
struct ShopEquipmentDefinition {
    int kind{1}; // 1 weapon,2 armor,3 accessory; each namespace has independent original IDs.
    int id{};
    int rank{};
    int type{};
    bool unlocked{};
    int price{};
    std::array<int, 4> combat{};
};
struct ShopWorldState {
    RescueWorldState world;
    std::map<int, ShopHumanRecord> humans;
    std::map<CharacterId, ShopActorRecord> actors;
    std::map<int, ObjectCatalogRecord>
        items; // N>=0 delivery is item definition ID, not bp object ID.
    std::vector<std::array<int, 3>>
        popularity_queue; // UserData.I, front insertion, no eager credit.
};
enum class ShopWorldError {
    none,
    invalid_input,
    stale_actor,
    stale_binding,
    missing_ticket,
    preparation_failed
};
enum class ShopWorldRequestKind { delivered_item_notice, cash_display, equipment_display };
struct ShopWorldRequest {
    ShopWorldRequestKind kind;
    int first{};
    int second{};
    LegacyActorControl command;
};
struct ShopWorldCandidate {
    ShopWorldState state;
    std::vector<ShopWorldRequest> requests;
    bool consumed_armor_slot{};
    bool consumed_selection{};
    bool cleaned_up{};
};
struct ShopWorldResult {
    ShopWorldError error{ShopWorldError::none};
    std::optional<ShopWorldCandidate> candidate;
};
struct ShopArrivalInput {
    CharacterId actor;
    std::vector<ShopEquipmentDefinition> catalogue; // Current definitions in source order.
    std::optional<int> armor_slot_ticket; // ALWAYS draw2 for detail4, including cooldown/no gear.
    std::optional<int> selection_ticket;
    std::function<std::optional<int>(int)> draw{};
};
// P's F/route/arrival predicate must already have succeeded. Resolves selection BEFORE payment
// guards, delivers N>=0 without global item-reward E, and atomically arranges actual use.
ShopWorldResult prepare_world_shop_arrival(const ShopWorldState &state,
                                           const ShopArrivalInput &input);
struct ShopExitInput {
    CharacterId actor;
    std::vector<ShopEquipmentDefinition> catalogue;
    std::array<int, 2> job_thresholds; // Current actor profession m, not cached global default.
    int quality{}; // Fresh o.a(1,current instance), including current shared level/neighborhood.
    int satisfaction_ticket{};
    std::vector<FacilityAttributeEffect> effects; // Current definition z/A order.
    std::optional<int> effect_ticket;
    std::function<std::optional<int>(int)> draw{};
};
// Front24: same exit position/use/release/reset; ordinary shop inserts popularity even for0.
// Equipment animation/attributes remain AFTER activity8, never eagerly applied on exit.
ShopWorldResult prepare_world_shop_exit(const ShopWorldState &state, const ShopExitInput &input);
// Front19/28/30 commits only that command. 27/29 emit explicit presentation requests and do not
// equip; callers still have to dispatch their cd payload BEFORE continuing the same interpreter.
// No counters/HP/display timeline advance; do not call the common d prefix twice.
ShopWorldResult prepare_world_shop_command(const ShopWorldState &state, CharacterId actor,
                                           const std::vector<ShopEquipmentDefinition> &catalogue);
} // namespace ark::simulation::rules
