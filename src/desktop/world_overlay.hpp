#pragma once

// Read-only drawing commands in source-scale units relative to a caller-supplied world anchor.
// They contain no textures, clocks or mutable world state, so presentation can be tested headless.
#include <array>
#include <string>
#include <variant>
#include <vector>

namespace ark::desktop {
struct OverlaySprite {
    std::string name;
    int frame{};
    float x{}, y{};
};
struct OverlayRectangle {
    float x{}, y{}, width{}, height{};
    std::array<unsigned char, 3> rgb{};
};
struct OverlayImage {
    std::string name;
    std::array<float, 4> source{};
    std::array<float, 4> destination{};
};
// Image identity is resolved from the current profession/sex, never a birth-time actor cache.
struct OverlayPortrait {
    int image{};
    std::array<float, 4> source{};
    std::array<float, 4> destination{};
};
using OverlayCommand = std::variant<OverlaySprite, OverlayRectangle, OverlayImage, OverlayPortrait>;
using OverlayPlan = std::vector<OverlayCommand>;
} // namespace ark::desktop
