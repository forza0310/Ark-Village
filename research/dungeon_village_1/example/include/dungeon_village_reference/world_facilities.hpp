#pragma once

// The shared actor/facility owner executes m, separately from the common d prefix and physics.
#include "dungeon_village_reference/rescue_commit.hpp"
#include "dungeon_village_reference/world_perception.hpp"

namespace dungeon_village_reference {
struct WorldExpressionTicket {
    int probability{};   // [0,1000), required even when suppressed.
    int variant_count{}; // Original platform table, not a locale guessed from screenshots.
    std::optional<int> variant;
};
using WorldExpressionDraw =
    std::function<std::optional<WorldExpressionTicket>(const ActorEffectState &, int)>;
struct WorldFacilityControlInput {
    CharacterId actor;
    std::vector<WorldExpressionTicket> expressions; // Actual opcode18 order only.
    std::vector<int> launch_tickets;                // Actual opcode23 order, each [0,4).
    std::size_t domain_limit{1000000}; // 外层全码路由可设1，让下一领域读取当前世界/票号。
};
struct WorldFacilityControlCandidate {
    RescueWorldState state;
    ActorControlFlow flow{ActorControlFlow::empty};
    std::size_t consumed_expressions{};
    std::size_t consumed_launch_tickets{};
    bool occupied{};
    bool exited{};
    bool cleaned_up{};
};
struct WorldFacilityControlResult {
    RescueWorldError error{RescueWorldError::none};
    std::optional<WorldFacilityControlCandidate> candidate;
};
// Local controls plus0/2(to15)/18/21/22/23/simple24; other domain codes remain at the front.
// No counters, HP display, growth, projection or physics. An error rolls back the entire segment.
WorldFacilityControlResult prepare_world_facility_control(const RescueWorldState &state,
                                                          const WorldFacilityControlInput &input);
struct WorldFacilityUseInput {
    CharacterId actor;
    int mode{};
    std::optional<Position> world_target; // h.e projected by the presentation adapter, truncated.
    std::optional<int> direction_ticket;
    std::function<std::optional<int>(int)> draw{};
    std::function<std::optional<Position>(int)> direction_target{}; // 同一draw4之后的h.e投影。
};
struct WorldFacilityUseResult {
    RescueWorldError error{RescueWorldError::none};
    std::optional<RescueWorldState> state;
    bool ground_effect20{};
};
// Arrange use from live O/definition. This is not arrival and never charges again.
WorldFacilityUseResult prepare_world_facility_use(const RescueWorldState &state,
                                                  const WorldFacilityUseInput &input);
struct WorldFacilityExecutionInput {
    WorldFacilityControlInput control;
    std::optional<WorldExpressionTicket> carry_expression; // N=-2, BEFORE v, after shared growth.
};
struct WorldFacilityExecutionResult {
    RescueWorldError error{RescueWorldError::none};
    std::optional<WorldFacilityControlCandidate> candidate;
    std::vector<ActorEffectSound> sounds;
    std::vector<HumanGrowthRequest> growth_requests;
};
// Common d prefix exactly ONCE -> carry expression -> control. Retention/physics are subsequent.
WorldFacilityExecutionResult
prepare_world_facility_execution(const RescueWorldState &state,
                                 const WorldFacilityExecutionInput &input);
struct WorldSpecialEntryCandidate {
    RescueWorldState state;
    bool ground_effect{}; // B60, at OLD cached u; owner renders it without an extra RNG draw.
    bool completed{};
};
struct WorldSpecialEntryResult {
    RescueWorldError error{RescueWorldError::none};
    std::optional<WorldSpecialEntryCandidate> candidate;
};
// State15 c after common perception: horizontal velocity -> B15 expression16 -> B60 stop
// -> B70 c0, optional activity0 using h.b(OLD s), not cached ax or newly moved n.
// Does not advance B or apply d gravity; bounds must be the current town boundary.
WorldSpecialEntryResult
prepare_world_special_entry_c(const RescueWorldState &state, CharacterId actor, TownBounds town,
                              std::optional<WorldExpressionTicket> expression_ticket = {});
} // namespace dungeon_village_reference
