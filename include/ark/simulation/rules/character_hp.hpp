#pragma once

// Six-slot target/display protocol using legacy logical counts, not wall-clock seconds.

#include <cstdint>
#include <optional>

namespace ark::simulation::rules {

struct CharacterHpState {
    std::int32_t requested_delta{};
    std::int32_t displayed{};
    std::int32_t origin{};
    std::int32_t target{};
    bool animating{};
    std::int32_t legacy_tick{};
};

enum class CharacterHpError { none, invalid_input, numeric_overflow };

struct CharacterHpResult {
    CharacterHpError error{CharacterHpError::none};
    std::optional<CharacterHpState> candidate;
};

// Restart from the previous target, not the interpolated display; positive changes respect
// capacity.
CharacterHpResult prepare_hp_change(const CharacterHpState &state, std::int32_t delta,
                                    std::int32_t capacity);
// Advance bounded logical steps without changing the target; do not assume display reaches target
// at step 30.
CharacterHpResult advance_hp_animation(const CharacterHpState &state, std::uint32_t steps = 1);
// Assign target, origin and display directly while preserving the animation flag and counter.
CharacterHpResult prepare_hp_assignment(const CharacterHpState &state, std::int32_t value);

} // namespace ark::simulation::rules
