// Adapted from published research e8bd81c; independent standard-C++ product rules.
// Opposing25/friendly9 kernels read previous area eligibility, then apply source terrain mask.
#include "ark/people/combat_ai.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace ark::people {
bool valid_combat_influence_field(const CombatInfluenceCandidate &f) {
    return f.width > 0 && f.height > 0 &&
           static_cast<std::uint64_t>(f.width) * f.height <= 4000000 &&
           static_cast<std::uint64_t>(f.width) * f.height == f.human_field.size() &&
           f.human_field.size() == f.monster_field.size() &&
           std::none_of(f.human_field.begin(), f.human_field.end(), [](int v) { return v < 0; }) &&
           std::none_of(f.monster_field.begin(), f.monster_field.end(),
                        [](int v) { return v < 0; });
}
CombatInfluenceResult prepare_combat_influence(const CombatInfluenceInput &i) {
    const auto cells = static_cast<std::int64_t>(i.map_width) * i.map_height;
    if (i.map_width <= 0 || i.map_height <= 0 || cells > 1000000 ||
        i.legacy_surface.size() != static_cast<std::size_t>(cells))
        return {CombatAiError::invalid_input, std::nullopt};
    for (const auto *roster : {&i.humans, &i.monsters})
        for (const auto &a : *roster)
            if (a.state < 0 || a.state > 20)
                return {CombatAiError::invalid_input, std::nullopt};
    CombatInfluenceCandidate c;
    c.width = i.map_width * 2;
    c.height = i.map_height * 2;
    c.human_field.resize(static_cast<std::size_t>(cells) * 4);
    c.monster_field.resize(c.human_field.size());
    const auto eligible = [](const InfluenceActor &a) {
        const int s = a.state;
        return a.previous_move_area && s != 2 && s != 3 && s != 8 && s != 9 && s != 14 && s != 15 &&
               s != 16;
    };
    static constexpr int weights[25] = {30, 30, 30, 30, 30, 30, 50, 50, 50, 30, 30, 50, 100,
                                        50, 30, 30, 50, 50, 50, 30, 30, 30, 30, 30, 30};
    static constexpr float friendly[9] = {0.95F, 0.9F, 0.95F, 0.9F, 0.8F, 0.9F, 0.95F, 0.9F, 0.95F};
    const auto index = [&](std::int64_t x, std::int64_t y) -> std::optional<std::size_t> {
        if (x < 0 || x >= c.width || y < 0 || y >= c.height)
            return std::nullopt;
        return static_cast<std::size_t>(y * c.width + x);
    };
    const auto add = [&](std::vector<int> &field, const std::vector<InfluenceActor> &roster) {
        for (const auto &a : roster)
            if (eligible(a))
                for (int n = 0; n < 25; ++n) {
                    const auto p = index(static_cast<std::int64_t>(a.half_cell.x) + n % 5 - 2,
                                         static_cast<std::int64_t>(a.half_cell.y) + 2 - n / 5);
                    if (p) {
                        if (field[*p] > std::numeric_limits<int>::max() - weights[n])
                            return false;
                        field[*p] += weights[n];
                    }
                }
        return true;
    };
    const auto multiply = [&](std::vector<int> &field, const std::vector<InfluenceActor> &roster) {
        for (const auto &a : roster)
            if (eligible(a))
                for (int n = 0; n < 9; ++n) {
                    const auto p = index(static_cast<std::int64_t>(a.half_cell.x) + n % 3 - 1,
                                         static_cast<std::int64_t>(a.half_cell.y) + 1 - n / 3);
                    if (p)
                        field[*p] = static_cast<int>(static_cast<float>(field[*p]) * friendly[n]);
                }
    };
    if (!add(c.human_field, i.monsters) || !add(c.monster_field, i.humans))
        return {CombatAiError::invalid_input, std::nullopt};
    multiply(c.human_field, i.humans);
    multiply(c.monster_field, i.monsters);
    for (int y = 0; y < i.map_height; ++y)
        for (int x = 0; x < i.map_width; ++x) {
            const int surface = i.legacy_surface[static_cast<std::size_t>(y) * i.map_width + x];
            if (surface == 0 || surface == 3)
                for (int dy = 0; dy <= 1; ++dy)
                    for (int dx = 0; dx <= 1; ++dx) {
                        const auto p = *index(x / 2 + dx, y / 2 + dy);
                        c.human_field[p] = c.monster_field[p] = 0;
                    }
        }
    return {CombatAiError::none, c};
}
} // namespace ark::people
