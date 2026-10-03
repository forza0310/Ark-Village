#pragma once

#include <cstdint>
#include <optional>

namespace dungeon_village_reference {

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

CharacterHpResult prepare_hp_change(const CharacterHpState &state, std::int32_t delta,
                                    std::int32_t capacity);
CharacterHpResult advance_hp_animation(const CharacterHpState &state, std::uint32_t steps = 1);
CharacterHpResult prepare_hp_assignment(const CharacterHpState &state, std::int32_t value);

} // namespace dungeon_village_reference
