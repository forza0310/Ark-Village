// Regional fallback consumes an explicit random prefix so partial plans can be replayed.
// Package responsibilities and evidence boundaries: ../README.md.

#include "dungeon_village_reference/regional_choice.hpp"

namespace dungeon_village_reference {

RegionalChoiceResult select_regional_candidate(const ActivityCandidateSnapshot &snapshot,
                                               std::int32_t town_bottom,
                                               const std::vector<std::int64_t> &draw_prefix) {
    if (!valid_activity_candidate_snapshot(snapshot)) {
        return {RegionalChoiceError::invalid_snapshot, std::nullopt};
    }
    if (draw_prefix.size() > 6) {
        return {RegionalChoiceError::too_many_draws, std::nullopt};
    }
    if (snapshot.cells.empty()) {
        return {RegionalChoiceError::none, RegionalChoicePlan{}};
    }
    const auto threshold = static_cast<std::int64_t>(town_bottom) + 2;
    const auto bound = snapshot.cells.size();
    for (std::size_t draw = 0; draw < 6; ++draw) {
        if (draw == draw_prefix.size()) {
            return {RegionalChoiceError::none,
                    RegionalChoicePlan{RegionalChoiceStatus::needs_draw, draw, bound, false,
                                       std::nullopt}};
        }
        const auto ticket = draw_prefix[draw];
        if (ticket < 0 || static_cast<std::uint64_t>(ticket) >= bound) {
            return {RegionalChoiceError::invalid_ticket, std::nullopt};
        }
        const auto index = static_cast<std::size_t>(ticket);
        const auto &cell = snapshot.cells[index];
        if (draw == 5 || cell.position.y >= threshold) {
            return {RegionalChoiceError::none,
                    RegionalChoicePlan{RegionalChoiceStatus::selected, draw + 1, 0, draw == 5,
                                       RegionalCandidateTarget{index, cell}}};
        }
    }
    return {RegionalChoiceError::invalid_ticket, std::nullopt};
}

} // namespace dungeon_village_reference
