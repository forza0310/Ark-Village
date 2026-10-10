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
// 显式研究应用入口：root已由命令行验证为本仓库work内现存独立目录。
// new只允许该槽两个目录均为空；load只消费已存在的手动栏，退出不自动保存。
struct StartupWindowApplicationOptions {
    std::filesystem::path root;
    int slot{};
    bool load{};
};
int run_startup_world_window(const std::filesystem::path &assets,
                             const std::filesystem::path &font, bool paused, int frames,
                             const std::optional<std::filesystem::path> &screenshot,
                             const std::string &inspect_page = {},
                             const std::optional<std::filesystem::path> &load_file = {},
                             const std::optional<std::filesystem::path> &save_file = {},
                             const std::optional<StartupWindowApplicationOptions> &application = {});
void check_startup_world();
} // namespace dungeon_village_prototype
