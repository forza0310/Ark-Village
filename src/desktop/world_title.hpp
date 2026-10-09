#pragma once

// No worker exists on the title screen. Loading prepares a private player-save candidate
// before the one successful transfer to WorldSession; title input never advances the world.
#include "ark/app/launch_options.hpp"
#include "ark/app/world_system.hpp"
#include "ui/layout.hpp"

namespace ark::desktop {
enum class WorldTitlePage { title, slots, actions, records, configure, overwrite, text_edit };
enum class WorldTitleAction { new_game, load };
struct WorldTitleSelection {
    WorldTitlePage page{WorldTitlePage::title};
    int slot{}, action{}, title_selection{}, record_page{}, field{};
    app::WorldNewGameDraft draft;
    std::string edit_text;
    std::string feedback;
};
struct WorldTitleLayout {
    Rectangle background, logo, book, start, records, panel, back;
    std::array<Rectangle, 2> slots;
    std::array<Rectangle, 3> actions;
    std::array<Rectangle, 4> fields;
    Rectangle previous, next, accept;
};
struct WorldTitleInput {
    std::optional<Vector2> click;
    bool confirm{}, back{}, up{}, down{}, left{}, right{}, erase{};
    std::string text;
};
WorldTitleLayout world_title_layout(Extent extent);
std::optional<WorldTitleAction>
world_title_input(WorldTitleSelection &selection, const WorldTitleLayout &layout,
                  const std::array<app::WorldSaveSlotInfo, 2> &slots, const WorldTitleInput &input);
// Re-read the selected file at activation, not the cached directory metadata. Failed reads
// leave initial intact; current random/pause/speed use the approved player restore policy.
bool load_world_title_slot(const std::filesystem::path &directory, int slot,
                           simulation::StartupWorldRuntimeState &initial, std::string &reason);
// False means the user closed the window or a bounded title inspection completed.
bool run_world_title(const app::LaunchOptions &options, const std::filesystem::path &assets,
                     simulation::StartupWorldRuntimeState &initial);
} // namespace ark::desktop
