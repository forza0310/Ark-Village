#pragma once

// Display/late-visual time lines affecting suppression and RNG, not image rendering or control m.
#include <optional>
#include <vector>

namespace dungeon_village_reference {
using ActorEffectRecord = std::vector<int>;
struct ActorEffectState {
    std::vector<ActorEffectRecord> display; // cd, forward traversal with removal skip.
    std::vector<ActorEffectRecord> delayed; // ce, reverse traversal, old delay<=0 fires.
};
enum class ActorEffectError { none, invalid_input, missing_ticket, invalid_ticket };
// Same timeline payload/overflow preflight used by advancement, without advancing any record.
bool valid_actor_effect_state(const ActorEffectState &state);
struct ActorEffectSound {
    int sound{};
}; // Uses actor's cached bm, not the ce effect position.
struct ActorEffectStepCandidate {
    ActorEffectState state;
    std::vector<ActorEffectSound> sounds; // Source dispatch order; owner resolves sound location.
    std::vector<std::size_t> removed_display_indices; // Indices in the evolving vector.
};
struct ActorEffectStepResult {
    ActorEffectError error{ActorEffectError::none};
    std::optional<ActorEffectStepCandidate> candidate;
};
// ce fires into cd BEFORE this round's cd pass; controls add cd only AFTER that pass.
// All relevant records preflight on the private copy; reject overflow/unknown, not Java wrap.
ActorEffectStepResult advance_actor_effects(const ActorEffectState &state);

struct ActorExpressionInput {
    ActorEffectState state;
    int expression{}; // 0..18, cT/cU static tables, not opcode or current state.
    int delay{};
    int probability_ticket{}; // Mandatory first draw[0,1000), even if suppressed/probability0.
    int variant_count{}; // Caller resolves cW/cX based on platform predicate, not guessed locale.
    std::optional<int> variant_ticket;
};
struct ActorExpressionCandidate {
    ActorEffectState state;
    bool probability_passed{};
    bool suppressed{};
    bool inserted{};
    bool consumed_variant{};
};
struct ActorExpressionResult {
    ActorEffectError error{ActorEffectError::none};
    std::optional<ActorExpressionCandidate> candidate;
};
ActorExpressionResult prepare_actor_expression(const ActorExpressionInput &input);

struct ActorCounterState {
    int alternate{};    // i.
    int action{};       // l.
    int state{};        // B.
    int hit_flash{};    // aw.
    int hit_label{};    // aq.
    bool miss_label{};  // ar.
    int damage_total{}; // ao.
    int hits{};         // ap.
};
// d prefix increments i/l/B BEFORE effects; aq expires AFTER display pass and BEFORE HP/control.
// Caller applies the two functions at their separate source points, not all after control.
std::optional<ActorCounterState> advance_actor_counters(const ActorCounterState &state);
std::optional<ActorCounterState> expire_actor_hit_label(const ActorCounterState &state);
} // namespace dungeon_village_reference
