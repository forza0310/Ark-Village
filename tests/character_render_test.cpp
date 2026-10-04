// Verify feet against actual source diamonds and playback against admitted movement, not FPS.
#include "ark/app/game.hpp"
#include "ark/assets/sprite.hpp"
#include "character_animation.hpp"
#include "character_visibility.hpp"
#include "projection.hpp"
#include <algorithm>
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
    const int source_x[] = {0, 18, 0, 36};
    for (int facing = 0; facing < 4; ++facing) {
        const auto walk = load(root / "human" / ark::desktop::walking_sprite(facing));
        check(walk.frame_count == 4 && walk.layers.size() == 1 && walk.layers[0].parts.size() == 4,
              "each published walking direction supplies exactly four poses");
        for (int n = 0; n < 4; ++n) {
            const auto &p = walk.layers[0].parts[n];
            check(p.frame == n && p.source_x == source_x[n] && p.source_y == facing * 24 &&
                      p.width == 18 && p.height == 24 && p.offset_x == -9 && p.offset_y == -24 &&
                      !p.flip_x && !p.flip_y,
                  "four distinct atlas rows keep the same feet, without invented mirroring");
        }
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
void facing() {
    using ark::world::WorldPosition;
    ark::desktop::CharacterAnimation animation;
    animation.observe(WorldPosition{1050, 1050}, 0, 2);
    check(animation.facing() == 2, "first display respects actual departure/control direction");
    // One continuous square crosses all four isometric directions. No camera or zoom inputs.
    const WorldPosition corners[]{{1050, 1070}, {1070, 1070}, {1070, 1050}, {1050, 1050}};
    const char *sprites[]{"walk00.seb", "walk01.seb", "walk02.seb", "walk03.seb"};
    for (int n = 0; n < 4; ++n) {
        animation.observe(corners[n], n + 1, 2);
        check(animation.facing() == n &&
                  std::string(ark::desktop::walking_sprite(animation.facing())) == sprites[n],
              "turning a path corner selects the corresponding actual source sprite");
        for (int render = 0; render < 20; ++render)
            animation.observe(corners[n], n + 1, 2);
        check(animation.facing() == n, "pause and extra renders preserve movement facing");
    }
    animation.observe(corners[3], 5, 2);
    check(animation.facing() == 3 && animation.frame() == 0,
          "stopping retains last walking direction instead of stale departure facing");
    animation.observe(corners[3], 6, 1);
    check(animation.facing() == 1, "an actual control4 change turns a stationary character");
    animation.observe(WorldPosition{1050, 1070}, 7, 2, 4);
    check(animation.facing() == 2, "state4 bypasses ordinary projected motion facing");
    animation.observe(WorldPosition{1070, 1070}, 8, 3, 20);
    check(animation.facing() == 3, "state20 also preserves authored facing");
    animation.observe(WorldPosition{1000, 1000}, 0, 2);
    animation.observe(WorldPosition{1001, 1000}, 1, 2);
    check(animation.facing() == 2, "subpixel motion with equal integer projections retains j");
    animation.observe(WorldPosition{1002, 1002}, 2, 2);
    check(animation.facing() == 2, "one unchanged projected axis retains j");
    animation.observe(std::nullopt, 3);
    animation.observe(WorldPosition{1050, 1050}, 4, 1);
    check(animation.facing() == 1, "replacement actor cannot inherit old direction");
}
void actual_ai() {
    for (const auto mode : {ark::app::PlayMode::startup, ark::app::PlayMode::ai_preview}) {
        ark::app::Game game(20261004, mode);
        for (int n = 0; n < 420; ++n)
            game.update();
        game.acknowledge_talk();
        game.acknowledge_talk();
        game.finish_camera();
        ark::desktop::CharacterAnimation animation;
        const auto actor = [&]() {
            return game.life_state() ? game.life_state() : game.ai_state();
        };
        const auto observe = [&]() {
            animation.observe(game.state().adventurer->position, actor()->rounds,
                              actor()->control.facing, actor()->control.state);
        };
        observe();
        std::set<int> poses, directions;
        for (int n = 0;
             n < 700 && game.state().mode == ark::app::Mode::normal && !actor()->arrivals; ++n) {
            game.update();
            observe();
            poses.insert(animation.frame());
            directions.insert(animation.facing());
        }
        check(actor()->arrivals == 1 && poses.size() == 4 && animation.frame() == 0,
              "real AI motion animates; arrival/use stops the cycle");
        if (mode == ark::app::PlayMode::startup) {
            // This fixed source seed reaches its first shop on a straight path. Its actual
            // corners occur after exit, before the existing encounter-consumer handoff.
            for (int n = 0; n < 700 && game.ai_error() == ark::app::InitialAiError::none; ++n) {
                game.update();
                observe();
                directions.insert(animation.facing());
            }
            check(directions.count(0) && directions.count(1) && directions.count(3),
                  "normal post-service route displays rear/right/front directions at real turns");
        }
    }
}

// The same actual service state drives normal-world and legacy-preview visibility. This tests
// the requested desktop policy, not an original-APK draw predicate or front-door animation.
void service_visibility(ark::app::PlayMode play) {
    using namespace ark;
    // An explicit raw tape selects weapon30 then food33, so the strict preview exercises both
    // a successful first exit and a later rejected exit. This is a fixture, not an APK seed claim.
    const auto random = play == app::PlayMode::ai_preview
                            ? app::RandomStream::from_raw(std::vector<std::int32_t>(16, 0))
                            : app::RandomStream::from_java_seed(20261004U);
    app::Game game(random, play);
    check(!desktop::character_visible(game) && !desktop::character_visible(game, true),
          "no visitor means no sprite, including an inspection override");
    for (int n = 0; n < 420; ++n)
        game.update();
    const auto actor = [&]() -> const app::LifeActorState & {
        return game.life_state() ? *game.life_state() : *game.ai_state();
    };
    const auto occupancy = [&](facilities::InstanceId id) -> const std::vector<people::ActorId> & {
        return game.life_state() ? game.state().facility_life.at(id).occupants
                                 : game.ai_state()->facilities.at(id).occupants;
    };
    check(game.state().mode == app::Mode::tutorial && desktop::character_visible(game),
          "first visitor remains visible in tutorial before actual use");
    const auto tutorial_rounds = actor().rounds;
    for (int n = 0; n < 20; ++n)
        game.update();
    check(actor().rounds == tutorial_rounds && desktop::character_visible(game),
          "tutorial blocks updates without hiding the visitor");
    while (game.state().mode == app::Mode::tutorial)
        game.acknowledge_talk();
    game.finish_camera();
    check(desktop::character_visible(game), "departing and walking are visible");
    for (int n = 0; n < 700 && !actor().active_facility; ++n) {
        check(desktop::character_visible(game), "actor visible on every pre-entry travel round");
        game.update();
    }
    check(actor().active_facility && actor().control.state == 14,
          "real autonomous arrival reaches state14 with an actual facility");
    const auto used = actor().active_facility->instance;
    const auto &occupants = occupancy(used);
    check(std::find(occupants.begin(), occupants.end(), actor().actor) != occupants.end() &&
              !desktop::character_visible(game) && desktop::character_visible(game, true),
          "actual service occupation hides sprite; explicit motion inspection remains visible");
    const auto rounds = actor().rounds;
    game.set_paused(true);
    for (int n = 0; n < 20; ++n) {
        game.update();
        check(actor().rounds == rounds && !desktop::character_visible(game),
              "pause freezes actual service and its hidden presentation");
    }
    game.set_paused(false);
    for (int n = 0; n < 300 && actor().completions == 0; ++n) {
        check(!desktop::character_visible(game), "sprite hidden throughout occupied use");
        game.update();
    }
    check(actor().completions == 1 && !actor().active_facility && occupancy(used).empty() &&
              desktop::character_visible(game),
          "actual exit releases occupation and shows the character again");
    // Legacy preview preserves the last successful actor and exposes failed rounds on Game;
    // normal life stores its explicit handoff on the actor as well as the same Game diagnostic.
    for (int n = 0; n < 700 && game.ai_error() == app::InitialAiError::none; ++n)
        game.update();
    check(game.ai_error() == app::InitialAiError::unsupported_branch,
          "actual random stream reaches unsupported activity");
    if (play == app::PlayMode::startup) {
        check(!actor().active_facility && desktop::character_visible(game),
              "normal committed exit/handoff remains visible, not falsely inside a shop");
    } else {
        // The later preview exit/departure fails as one transaction; its previous occupied
        // state remains authoritative. An error code alone must not change presentation.
        check(actor().active_facility && actor().control.state == 14 &&
                  !occupancy(actor().active_facility->instance).empty() &&
                  !desktop::character_visible(game),
              "preview rolled-back exit retains actual occupation and hidden presentation");
    }
}

// The actual Java-seeded preview rejects its first exit's unsupported next activity. Check the
// failed transaction directly, independently of the raw-tape successful-exit visibility case.
void preview_first_exit_rollback() {
    using namespace ark;
    app::Game game(20261004U, app::PlayMode::ai_preview);
    for (int n = 0; n < 420; ++n)
        game.update();
    while (game.state().mode == app::Mode::tutorial)
        game.acknowledge_talk();
    check(game.finish_camera() == app::Error::none, "close actual Java-seeded preview camera");
    for (int n = 0; n < 700 && !game.ai_state()->active_facility; ++n)
        game.update();
    check(game.ai_state()->active_facility && game.ai_state()->control.state == 14 &&
              game.ai_state()->completions == 0 && !desktop::character_visible(game),
          "Java-seeded preview enters actual first occupation before any completed exit");
    const auto used = game.ai_state()->active_facility->instance;
    bool rejected{};
    for (int n = 0; n < 300 && game.ai_error() == app::InitialAiError::none; ++n) {
        const auto before = *game.ai_state();
        const auto draws = game.random_draws();
        game.update();
        if (game.ai_error() == app::InitialAiError::none)
            continue;
        const auto &after = *game.ai_state();
        check(game.ai_error() == app::InitialAiError::unsupported_branch && after.arrivals == 1 &&
                  after.completions == 0 && after.active_facility &&
                  after.active_facility->instance == used && after.control.state == 14 &&
                  after.facilities.at(used).occupants == before.facilities.at(used).occupants &&
                  !after.facilities.at(used).occupants.empty() &&
                  after.accounting.entries() == before.accounting.entries() &&
                  after.accounting.funds() == before.accounting.funds() &&
                  after.control.queue == before.control.queue && after.rounds == before.rounds &&
                  game.random_draws() == draws && !desktop::character_visible(game),
              "first rejected exit retains actual occupation, cash, FIFO, round and RNG cursor");
        rejected = true;
    }
    check(rejected, "Java seed20261004 actually rejects its first preview exit");
}
} // namespace
int main(int argc, char **argv) {
    try {
        check(argc == 2, "asset root required");
        ground(argv[1]);
        playback();
        facing();
        actual_ai();
        service_visibility(ark::app::PlayMode::startup);
        service_visibility(ark::app::PlayMode::ai_preview);
        preview_first_exit_rollback();
        std::cout
            << "PASS actual road feet, four directions/poses, pause/idle/2x/reset and real AI\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
