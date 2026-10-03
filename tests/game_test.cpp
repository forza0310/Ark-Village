// Product integration checks against the published first-play contract, not the research fixture.
#include "ark/app/game.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace ark;
namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void steps(app::Game &game, int count) {
    for (int i = 0; i < count; ++i)
        check(game.update() == app::Error::none, "update");
}
std::string snapshot(const app::Game &game) {
    const auto &s = game.state();
    std::ostringstream out;
    out << s.money << ',' << s.points << ',' << s.popularity << ',' << static_cast<int>(s.mode)
        << ',' << s.arrival_counter << ',' << s.event89_count << ',' << s.simulation_steps << ','
        << s.next_id << ',' << s.selection.value_or(-1) << ',' << s.orientation << ',' << s.paused
        << ',' << s.talk_line;
    for (int v : s.calendar)
        out << ',' << v;
    for (const auto &v : s.definition_progress) {
        out << ';' << v.first << ',' << v.second.level;
        for (auto improvement : v.second.improvements)
            out << ',' << improvement;
        out << ',' << v.second.completed_uses << ',' << v.second.upgrade_pending;
    }
    for (const auto &entry : s.facilities) {
        const auto &v = entry.second;
        out << ';' << entry.first << ',' << v.id << ',' << v.definition_id << ',' << v.anchor.x
            << ',' << v.anchor.y << ',' << v.orientation << ',' << v.remaining_ticks << ','
            << v.source_seed << ',' << v.reset_legacy_id.value_or(-1);
    }
    for (auto id : s.instance_order)
        out << ";order=" << id;
    for (const auto &v : s.expenses)
        out << ';' << v.instance << ',' << v.category << ',' << v.expense;
    if (s.adventurer) {
        const auto &v = *s.adventurer;
        out << ';' << v.uid << ',' << v.definition_id << ',' << v.name << ',' << v.job_id << ','
            << v.sex << ',' << v.level << ',' << v.effort << ',' << v.satisfaction << ',' << v.flags
            << ',' << v.cell.x << ',' << v.cell.y << ',' << v.position.x << ',' << v.position.z
            << ',' << v.pending_activity.value_or(-1);
        for (int x : v.attributes)
            out << ',' << x;
        for (int x : v.equipment)
            out << ',' << x;
        for (int x : v.combat)
            out << ',' << x;
        for (int x : v.hp)
            out << ',' << x;
    }
    return out.str();
}
void first_arrival() {
    app::Game game;
    const auto &data = app::startup_data();
    check(data.map.width == 24 && data.map.height == 24 && data.map.cells.size() == 576,
          "source grid");
    check(game.state().money == 5000 && game.state().points == 10 && game.state().popularity == 50,
          "opening resources");
    check(game.state().facilities.size() == 8 && game.state().expenses.empty() &&
              !game.state().adventurer,
          "free seeds and empty scene");
    check(data.unlocked_people == std::vector<int>{1, 2, 3},
          "unlocked definitions are not live actors");
    game.set_paused(true);
    const auto paused = snapshot(game);
    steps(game, 600);
    check(snapshot(game) == paused, "pause must not accumulate updates");
    game.set_paused(false);
    steps(game, 419);
    check(!game.state().adventurer && game.state().arrival_counter == 1, "arrival not early");
    game.update(2);
    check(game.state().simulation_steps == 420 && game.state().event89_count == 1 &&
              game.state().mode == app::Mode::tutorial,
          "create before modal and suppress second speed step");
    const auto &a = *game.state().adventurer;
    check(a.uid == 0 && a.definition_id == 1 && a.job_id == 1 && a.level == 1 && a.sex == 0,
          "identity and job");
    check(a.attributes == std::array<int, 6>{22, 2, 2, 2, 2, 2} &&
              a.combat == std::array<int, 4>{22, 7, 2, 2} &&
              a.equipment == std::array<int, 4>{0, -1, -1, -1} &&
              a.hp == std::array<int, 3>{22, 22, 22},
          "original first attributes");
    check(a.cell == world::Cell{11, 0} || a.cell == world::Cell{12, 0}, "evidenced spawn");
    check((a.flags & 8192) && game.state().money == 5000, "first visit is free");
    check(a.flags == (2U | 8192U) && a.pending_activity == 0 &&
              a.position.x == a.cell.x * 100 + 50 && a.position.z == a.cell.y * 100 + 50,
          "world centre, initialization flag and unexecuted activity0");
    const auto modal = snapshot(game);
    steps(game, 100);
    check(snapshot(game) == modal, "tutorial pauses world");
    game.acknowledge_talk();
    game.acknowledge_talk();
    check(game.state().mode == app::Mode::camera, "camera after talk");
    const auto camera = snapshot(game);
    steps(game, 100);
    check(snapshot(game) == camera, "camera pauses world");
    game.finish_camera();
    steps(game, 100);
    check(game.state().event89_count == 1 && game.state().adventurer->uid == 0,
          "no duplicate on close");
    check(game.acknowledge_talk() == app::Error::wrong_mode, "reject repeat acknowledgement");
}
void construction() {
    app::Game game;
    game.open_catalog();
    const auto menu = snapshot(game);
    steps(game, 500);
    check(snapshot(game) == menu, "catalog freezes");
    for (int id : {29, 36, -1, -2, 18, 24, 999}) {
        check(game.select(id) == app::Error::unavailable && snapshot(game) == menu,
              "locked or unimplemented selection");
    }
    check(game.select(28) == app::Error::none, "initial inn available");
    const auto before = snapshot(game);
    check(game.confirm({-1, 4}) == app::Error::outside_map && snapshot(game) == before,
          "map bounds atomic");
    check(game.confirm({6, 5}) == app::Error::outside_town && snapshot(game) == before,
          "town bounds atomic");
    check(game.confirm({10, 5}) == app::Error::occupied && snapshot(game) == before,
          "occupied atomic");
    steps(game, 500);
    check(snapshot(game) == before, "placement freezes");
    check(game.confirm({7, 3}) == app::Error::none, "place without road condition");
    const auto id = *game.facility_at({7, 3});
    check(game.state().money == 4000 && game.state().facilities.at(id).remaining_ticks == 280,
          "definition-derived quote and work");
    check(game.state().expenses.size() == 1 && game.state().expenses[0].expense == 1000 &&
              game.state().expenses[0].category == 0,
          "construction ledger category");
    const auto placed = snapshot(game);
    check(game.confirm({7, 3}) == app::Error::occupied && snapshot(game) == placed,
          "repeat atomic");
    game.cancel();
    steps(game, 279);
    check(game.state().facilities.at(id).remaining_ticks == 1, "work not early");
    game.update();
    check(game.state().facilities.at(id).remaining_ticks == 0 && game.state().money == 4000,
          "complete no second fee");
    game.open_catalog();
    game.select(66);
    game.rotate();
    game.confirm({8, 3});
    game.cancel();
    const auto plant = game.state().facilities.at(*game.facility_at({8, 3}));
    check(game.state().money == 3800 && plant.remaining_ticks == 0 && plant.orientation == 1,
          "plant price and immediate use");
    check(game.state().definition_progress.at(28).level == 1, "shared definition level");
}
void funds_and_boundary() {
    app::Game game;
    game.open_catalog();
    game.select(28);
    for (int x = 7; x < 12; ++x)
        check(game.confirm({x, 3}) == app::Error::none, "spend starting cash");
    const auto empty = snapshot(game);
    check(game.confirm({12, 3}) == app::Error::insufficient_funds && snapshot(game) == empty,
          "commit funds check atomic");
    game.cancel();
    game.open_catalog();
    const auto menu = snapshot(game);
    check(game.select(28) == app::Error::insufficient_funds && snapshot(game) == menu,
          "select funds check atomic");
    game.cancel();
    steps(game, 420);
    game.acknowledge_talk();
    game.acknowledge_talk();
    game.finish_camera();
    steps(game, 1100);
    check(game.state().mode == app::Mode::research_boundary &&
              game.state().simulation_steps == 1456 && game.state().money == 0 &&
              game.state().expenses.size() == 5,
          "guard unknown settlement before effects");
    const auto bound = snapshot(game);
    steps(game, 100);
    check(snapshot(game) == bound, "boundary no drift");
    app::Game slow, fast;
    steps(slow, 200);
    for (int i = 0; i < 100; ++i)
        fast.update(2);
    check(snapshot(slow) == snapshot(fast), "two eligible steps, not milliseconds");
    const auto bad = snapshot(fast);
    check(fast.update(3) == app::Error::invalid_input && snapshot(fast) == bad, "bad speed atomic");
}
void geometry() {
    const auto pair = facilities::footprint(1, 1, {5, 5});
    check(pair[0].cell == world::Cell{4, 5} && pair[0].fragment == 1 && pair[1].fragment == 3,
          "rotated pair order");
    const auto square = facilities::footprint(2, 1, {5, 5});
    check(square.size() == 4 && square[1].cell == world::Cell{5, 6} && square[3].fragment == 7,
          "square fragment order");
    bool rejected = false;
    try {
        facilities::footprint(2, 2, {0, 0});
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected, "invalid geometry rejected");
}
} // namespace
int main() {
    try {
        geometry();
        first_arrival();
        construction();
        funds_and_boundary();
        std::cout << "PASS first-play product contracts\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
