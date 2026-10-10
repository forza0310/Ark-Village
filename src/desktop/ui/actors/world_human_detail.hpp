#pragma once
#include "world_human.hpp"
namespace ark::desktop::ui {
// Static Steam60 fields with existing PC navigation. Character preview is explicitly static;
// no unshipped motion schedule, power bar or medal helper is inferred from screenshots.
void draw_world_human_detail(const WorldHumanView &view, const WorldHumanLayout &layout,
                             const Skin &skin, bool enabled, const std::string &feedback);
} // namespace ark::desktop::ui
