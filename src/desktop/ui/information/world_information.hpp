#pragma once

#include "../common/layout.hpp"
#include "ark/simulation/village/startup_world_information.hpp"

namespace ark::desktop::ui {
class Skin;
struct WorldInformationView {
    int raw{};
    bool interactive{};
    std::optional<simulation::StartupInformationPageView> source;
};
struct WorldInformationLayout {
    Vector2 origin;
    Rectangle rows, previous, next, back, confirm;
    std::array<Rectangle, 5> menu_rows;
};
struct WorldInformationInput {
    std::optional<Vector2> click;
    bool up{}, down{}, left{}, right{}, confirm{}, cancel{};
    int wheel_rows{};
};
bool world_information_page(const simulation::rules::WorldScriptPage &page);
WorldInformationView world_information_view(const simulation::StartupWorldRuntimeState &state,
                                            const simulation::rules::WorldScriptPage &page);
WorldInformationLayout world_information_layout(Extent extent, int raw);
// Mouse row selection and keyboard input are separate source commands; a row
// click never uses an item, grants equipment or synthesizes an actor instance.
std::optional<simulation::StartupInformationInput>
world_information_input(const WorldInformationView &view, const WorldInformationLayout &layout,
                        const WorldInformationInput &input, bool blocked);
void draw_world_information(const simulation::StartupWorldRuntimeState &state,
                            const WorldInformationView &view, const WorldInformationLayout &layout,
                            const Skin &skin, bool enabled, const std::string &feedback = {});
} // namespace ark::desktop::ui
