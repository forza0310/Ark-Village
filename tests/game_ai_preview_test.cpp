// Exercise the actual Game update/read model used by the window, with genuine first-visit data.
#include "ark/app/game.hpp"
#include <iostream>
#include <set>
#include <stdexcept>

using namespace ark;
namespace {
int checks{};
void check(bool pass, const char *message) {
    ++checks;
    if (!pass)
        throw std::runtime_error(message);
}
bool same_random(app::RandomStream a, app::RandomStream b) {
    if (a.draws() != b.draws() || a.raw_cursor() != b.raw_cursor())
        return false;
    for (int n = 0; n < 16; ++n) {
        const auto x = a.draw(701), y = b.draw(701);
        if (x.error != y.error || x.raw != y.raw || x.ticket != y.ticket || x.ordinal != y.ordinal)
            return false;
    }
    return true;
}
app::Game initial(std::uint32_t seed) {
    app::Game game(seed, app::PlayMode::ai_preview);
    check(game.open_catalog() == app::Error::unavailable, "preview cannot modify source layout");
    for (int n = 0; n < 420; ++n)
        game.update();
    check(game.state().mode == app::Mode::tutorial && game.ai_state() &&
              game.ai_state()->rounds == 1 && game.ai_state()->journey &&
              !game.state().adventurer->pending_activity,
          "same first-visitor step selects autonomous target before tutorial");
    return game;
}
void resume(app::Game &game) {
    game.acknowledge_talk();
    game.acknowledge_talk();
    game.finish_camera();
}
void eligibility() {
    auto game = initial(20261004);
    const auto start = *game.ai_state();
    for (int n = 0; n < 50; ++n)
        game.update(2);
    check(game.ai_state()->rounds == 1 && game.state().money == 5000,
          "tutorial blocks both AI passes, money and random draws");
    game.acknowledge_talk();
    game.acknowledge_talk();
    game.update(2);
    check(game.ai_state()->rounds == 1, "camera transition does not advance AI");
    game.finish_camera();
    game.set_paused(true);
    for (int n = 0; n < 50; ++n)
        game.update(2);
    check(game.ai_state()->rounds == 1 && game.ai_state()->position.x == start.position.x,
          "explicit pause freezes the real actor and AI counters");
    game.set_paused(false);
    auto twice = game;
    auto singles = game;
    twice.update(2);
    singles.update();
    singles.update();
    check(twice.ai_state()->rounds == 3 &&
              twice.state().adventurer->position.x == singles.state().adventurer->position.x &&
              twice.state().adventurer->position.z == singles.state().adventurer->position.z &&
              twice.ai_state()->control.queue == singles.ai_state()->control.queue,
          "2x executes the same two admitted rounds as two 1x updates");
    check(game.ai_state()->rounds == 1, "copied Game owns independent AI state");
}
void life() {
    std::set<int> targets;
    std::set<int> births;
    int successful_exits{}, attributes{}, equipment{};
    for (std::uint32_t seed = 0; seed < 32; ++seed) {
        auto game = initial(seed);
        const auto first = game.ai_state()->journey->binding.definition_id;
        targets.insert(first);
        births.insert(game.state().adventurer->cell.x);
        const auto date = game.state().calendar;
        const auto route = game.ai_state()->journey->route;
        const auto map = game.route_map();
        bool road{};
        for (const auto cell : route.steps)
            road = road || map.cells[map.index(cell)].category == world::RouteCategory::road;
        check(road, "autonomous initial journey uses the researched weighted road route");
        resume(game);
        bool moved{};
        for (int n = 0; n < 1005 && game.state().mode == app::Mode::normal; ++n) {
            const auto old = game.state().adventurer->position;
            const auto old_arrivals = game.ai_state()->arrivals;
            game.update();
            const auto &actor = *game.state().adventurer;
            const auto &ai = *game.ai_state();
            moved = moved || actor.position.x != old.x || actor.position.z != old.z;
            check(actor.position.x == ai.position.x && actor.position.z == ai.position.z &&
                      actor.cell == *people::world_cell(ai.position),
                  "scene reads the actual AI position and logical cell, not a rendering override");
            check(game.state().money == ai.accounting.funds() && actor.hp[2] == ai.hp.target &&
                      actor.equipment[0] == ai.current_weapon,
                  "HUD/roster observe the AI money, HP and committed weapon");
            if (old_arrivals == 0 && ai.arrivals == 1)
                check(game.state().money == (first == 30 ? 5400 : 5300),
                      "real first arrival charges source weapon price or actual neighbour price");
            check(game.state().calendar == date && game.state().simulation_steps == 420 &&
                      game.state().expenses.empty(),
                  "preview advances neither calendar nor construction accounting");
            for (const auto &[id, uses] : ai.uses)
                check(game.state().definition_progress.at(id).completed_uses ==
                          static_cast<std::uint64_t>(uses.completed_uses),
                      "facility details read the committed shared-use progress");
        }
        check(moved && game.ai_state()->arrivals > 0, "avatar really travels and uses a facility");
        check(
            game.state().mode == app::Mode::research_boundary &&
                (game.ai_error() == app::InitialAiError::unsupported_branch ||
                 (game.ai_error() == app::InitialAiError::none && game.ai_state()->rounds == 1000)),
            "unsupported exit/fallback or preview limit ends without a fabricated retry");
        const auto stopped = *game.ai_state();
        game.update(2);
        check(game.ai_state()->rounds == stopped.rounds &&
                  game.ai_state()->control.queue == stopped.control.queue,
              "ended preview cannot keep mutating behind the modal");
        successful_exits += game.ai_state()->completions;
        attributes += game.ai_state()->attribute_commits;
        equipment += game.ai_state()->equipment_commits;
    }
    check(targets == std::set<int>{28, 30, 33} && births == std::set<int>{11, 12},
          "local seeds cover both genuine births and all three genuine targets");
    check(successful_exits > 0 && attributes > 0 && equipment > 0,
          "actual Game flow consumes service exits and deferred attribute/equipment effects");
}
void random_rollback() {
    app::Game source;
    for (int n = 0; n < 420; ++n)
        source.update();
    app::InitialAiSession ai(source, source.state().adventurer->cell);
    auto random = app::RandomStream::from_java_seed(1);
    for (int n = 0; n < 1000; ++n) {
        const auto saved = ai.state();
        const auto before = random;
        if (ai.round_random(random) != app::InitialAiError::none) {
            check(same_random(random, before) && ai.state().rounds == saved.rounds &&
                      ai.state().control.queue == saved.control.queue &&
                      ai.state().accounting.entries() == saved.accounting.entries() &&
                      ai.state().completions == saved.completions,
                  "rejected random round retains RNG, cash, exit and control state together");
            return;
        }
    }
    throw std::runtime_error("rollback case never encountered its researched boundary");
}
} // namespace
int main() {
    try {
        eligibility();
        life();
        random_rollback();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
