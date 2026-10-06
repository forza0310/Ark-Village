#pragma once
#include <filesystem>
#include <optional>
#include <string>

namespace dungeon_village_prototype {
// Presentation only: source-asset decoding, font substitute, input areas and bounded capture.
int run_startup_window(const std::filesystem::path &assets, const std::filesystem::path &font,
                       bool paused, int frames,
                       const std::optional<std::filesystem::path> &screenshot,
                       const std::string &inspect_page = {});
void check_startup();
int run_startup_world_window(const std::filesystem::path &assets,
                             const std::filesystem::path &font, bool paused, int frames,
                             const std::optional<std::filesystem::path> &screenshot,
                             const std::string &inspect_page = {},
                             const std::optional<std::filesystem::path> &load_file = {},
                             const std::optional<std::filesystem::path> &save_file = {});
void check_startup_world();
} // namespace dungeon_village_prototype
