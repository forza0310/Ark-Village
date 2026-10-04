// Verify feet against actual source diamonds and playback against admitted movement, not FPS.
#include "ark/app/game.hpp"
#include "ark/assets/sprite.hpp"
#include "character_animation.hpp"
#include "projection.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>

namespace {
void check(bool ok, const char *why) {
    if (!ok)
        throw std::runtime_error(why);
}
ark::assets::SpriteDefinition load(const std::filesystem::path &file) {
    std::ifstream input(file, std::ios::binary);
    check(static_cast<bool>(input), "source SEB readable");
    return ark::assets::parse_legacy_seb(
        {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()});
}
void ground(const std::filesystem::path &root) {
    const auto road = load(root / "image/road00.seb");
    const auto walk = load(root / "human/walk00.seb");
    check(walk.frame_count == 4 && walk.layers.size() == 1 && walk.layers[0].parts.size() == 4,
          "published walk00 supplies exactly four poses");
    const int source_x[] = {0, 18, 0, 36};
    for (int n = 0; n < 4; ++n) {
        const auto &p = walk.layers[0].parts[n];
        check(p.frame == n && p.source_x == source_x[n] && p.source_y == 0 && p.width == 18 &&
                  p.height == 24 && p.offset_x == -9 && p.offset_y == -24,
              "actual source pose cuts/feet binding, not invented frames");
    }
    for (const auto size : {ark::desktop::Extent{384, 256}, ark::desktop::Extent{240, 330}})
        for (float zoom : {0.5F, 1.0F, 2.0F})
            for (const auto &p : road.layers[0].parts) {
                const ark::world::Cell cell{12, 3};
                const Vector2 camera{426, -72};
                const auto origin = ark::desktop::project(cell, camera, size, zoom);
                const auto feet = ark::desktop::project_position({1250, 350}, camera, size, zoom);
                check(std::abs(feet.x - origin.x - zoom * (p.offset_x + p.width / 2.0F)) < 0.001F &&
                          std::abs(feet.y - origin.y - zoom * (p.offset_y + p.height / 2.0F)) <
                              0.001F,
                      "all actual road diamonds centre underneath actor feet at every zoom");
            }
}
void playback() {
    ark::desktop::CharacterAnimation animation;
    animation.observe(ark::world::WorldPosition{1250, 50}, 0);
    std::set<int> poses;
    for (int tick = 1; tick <= 24; ++tick) {
        const ark::world::WorldPosition position{1250, 50 + 6.7F * tick};
        animation.observe(position, tick);
        poses.insert(animation.frame());
        const auto pose = animation.frame();
        for (int render = 0; render < 20; ++render)
            animation.observe(position, tick);
        check(animation.frame() == pose, "extra render/pause/modal frames cannot advance pose");
    }
    check(poses == std::set<int>{0, 1, 2, 3}, "actual walking cycles all published poses");
    animation.observe(ark::world::WorldPosition{1250, 50 + 6.7F * 24}, 25);
    check(animation.frame() == 0, "facility waiting returns to idle");
    ark::desktop::CharacterAnimation single, twice;
    single.observe(ark::world::WorldPosition{1250, 50}, 0);
    twice.observe(ark::world::WorldPosition{1250, 50}, 0);
    for (int tick = 2; tick <= 24; tick += 2) {
        single.observe(ark::world::WorldPosition{1250, 50 + 6.7F * (tick - 1)}, tick - 1);
        single.observe(ark::world::WorldPosition{1250, 50 + 6.7F * tick}, tick);
        twice.observe(ark::world::WorldPosition{1250, 50 + 6.7F * tick}, tick);
        check(single.frame() == twice.frame(), "2x advances logical playback twice per render");
    }
    animation.observe(std::nullopt, 0);
    animation.observe(ark::world::WorldPosition{1250, 50}, 1);
    check(animation.frame() == 0, "missing/replaced actor resets the clock");
    animation.observe(ark::world::WorldPosition{1250, 100}, 12);
    animation.observe(ark::world::WorldPosition{1250, 50}, 1);
    check(animation.frame() == 0, "new game cannot inherit old movement phase");
}
void actual_ai() {
    ark::app::Game game(20261004, ark::app::PlayMode::ai_preview);
    for (int n = 0; n < 420; ++n)
        game.update();
    game.acknowledge_talk();
    game.acknowledge_talk();
    game.finish_camera();
    ark::desktop::CharacterAnimation animation;
    animation.observe(game.state().adventurer->position, game.ai_state()->rounds);
    std::set<int> poses;
    while (game.state().mode == ark::app::Mode::normal && !game.ai_state()->arrivals) {
        game.update();
        animation.observe(game.state().adventurer->position, game.ai_state()->rounds);
        poses.insert(animation.frame());
    }
    check(game.ai_state()->arrivals == 1 && poses.size() == 4 && animation.frame() == 0,
          "real AI motion animates; arrival/use stops the cycle");
}
} // namespace
int main(int argc, char **argv) {
    try {
        check(argc == 2, "asset root required");
        ground(argv[1]);
        playback();
        actual_ai();
        std::cout << "PASS actual road feet binding, walk poses, pause/idle/2x/reset and real AI\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
