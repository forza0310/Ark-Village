#include "dungeon_village_prototype/startup.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace dungeon_village_prototype;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
std::string snapshot(const StartupSession &session) {
    const auto &s = session.state();
    std::ostringstream out;
    out << s.accounting.funds() << ',' << s.accounting.village_points() << ',' << s.popularity
        << ',' << s.arrival_counter << ',' << s.event89_count << ',' << s.simulation_steps << ','
        << s.next_id << ',' << s.next_cash_id << ',' << s.talk_line << ','
        << static_cast<int>(s.mode) << ',' << s.paused << ',' << s.selection.value_or(-999);
    for (const auto v : s.calendar)
        out << ',' << v;
    for (const auto &entry : s.facilities) {
        const auto &v = entry.second;
        out << ';' << v.id << ',' << v.definition_id << ',' << v.cell.x << ',' << v.cell.y << ','
            << v.remaining_ticks << ',' << v.seed;
    }
    for (const auto &entry : s.terrain_edits)
        out << ';' << entry.first << ',' << entry.second;
    if (s.character)
        out << ';' << s.character->uid << ',' << s.character->definition_id << ','
            << s.character->cell.x << ',' << s.character->cell.y << ',' << s.character->flags;
    for (const auto &entry : s.accounting.entries())
        out << ';' << entry.first << ',' << entry.second.amount;
    return out.str();
}
void normal_steps(StartupSession &session, int count) {
    for (int i = 0; i < count; ++i)
        session.update();
}
void reset_and_arrival() {
    StartupSession s;
    const auto &d = startup_evidence();
    check(d.cells.size() == 576 && d.width == 24 && d.height == 24, "real source map");
    check(d.unlocked_characters == std::vector<int>{1, 2, 3},
          "unlocked definitions, not live actors");
    check(s.state().facilities.size() == 8 && !s.state().character,
          "seed instances and empty scene");
    check(s.state().accounting.funds() == 5000 && s.state().accounting.village_points() == 10 &&
              s.state().popularity == 50 && s.state().calendar == std::array<int, 4>{0, 3, 0, 0},
          "actual opening state");
    check(s.state().accounting.entries().empty(), "seed load is free");
    s.set_paused(true);
    const auto paused = snapshot(s);
    normal_steps(s, 500);
    check(snapshot(s) == paused, "pause has no accumulated simulation");
    s.set_paused(false);
    normal_steps(s, 419);
    check(!s.state().character && s.state().arrival_counter == 1, "not early");
    s.update(2);
    check(s.state().mode == StartupMode::tutorial && s.state().event89_count == 1,
          "modal suppresses second speed step");
    const auto &c = *s.state().character;
    check(c.uid == 0 && c.definition_id == 1 && c.job_id == 1 && c.sex == 0 && c.level == 1 &&
              c.equipment == std::array<int, 4>{0, -1, -1, -1} &&
              c.attributes == std::array<int, 6>{22, 2, 2, 2, 2, 2} &&
              c.combat == std::array<int, 4>{22, 7, 2, 2} && c.hp == std::array<int, 3>{22, 22, 22},
          "first visitor values");
    check((c.cell == ref::Position{11, 0} || c.cell == ref::Position{12, 0}) && (c.flags & 8192),
          "birth and flag");
    const auto modal = snapshot(s);
    normal_steps(s, 100);
    check(snapshot(s) == modal && s.state().calendar[3] == 513,
          "tutorial pauses calendar, trigger does not advance it");
    check(s.state().accounting.funds() == 5000, "arrival is free");
    s.acknowledge_talk();
    check(s.state().mode == StartupMode::tutorial, "first talk then second");
    s.acknowledge_talk();
    check(s.state().mode == StartupMode::camera, "close then camera transition");
    const auto camera = snapshot(s);
    normal_steps(s, 100);
    check(snapshot(s) == camera, "camera pauses simulation");
    s.finish_camera();
    normal_steps(s, 500);
    check(s.state().event89_count == 1 && s.state().character->uid == 0,
          "no second character on close/repeat");
    check(s.acknowledge_talk() == StartupError::wrong_mode, "repeat confirmation rejected");
}
void construction() {
    StartupSession s;
    check(s.open_catalog() == StartupError::none, "catalog open");
    const auto catalog = snapshot(s);
    normal_steps(s, 500);
    check(snapshot(s) == catalog, "catalog pause");
    check(s.select(29) == StartupError::unavailable && snapshot(s) == catalog, "pair inn locked");
    check(s.select(36) == StartupError::unavailable && s.select(-2) == StartupError::unavailable,
          "cafe and move locked");
    check(s.select(28) == StartupError::none, "select inn");
    const auto selected = snapshot(s);
    check(s.confirm({6, 5}) == StartupError::outside_town && snapshot(s) == selected,
          "outside town rejects atomically");
    check(s.confirm({10, 5}) == StartupError::occupied && snapshot(s) == selected,
          "source facility occupied");
    check(s.confirm({-1, 4}) == StartupError::outside_town && snapshot(s) == selected,
          "map outside");
    normal_steps(s, 500);
    check(snapshot(s) == selected, "placement pause");
    check(s.confirm({7, 3}) == StartupError::none, "create bind pay");
    const auto id = *s.facility_at({7, 3});
    check(s.state().accounting.funds() == 4000 &&
              s.state().facilities.at(id).remaining_ticks == 280,
          "money and construction");
    normal_steps(s, 300);
    check(s.state().facilities.at(id).remaining_ticks == 280, "placement does not build");
    s.cancel();
    normal_steps(s, 279);
    check(s.state().facilities.at(id).remaining_ticks == 1, "not completed early");
    s.update();
    check(s.state().facilities.at(id).remaining_ticks == 0 && s.state().accounting.funds() == 4000,
          "complete without second charge");
    s.open_catalog();
    s.select(66);
    check(s.confirm({8, 3}) == StartupError::none, "plant placement");
    check(s.state().accounting.funds() == 3800 &&
              s.state().facilities.at(*s.facility_at({8, 3})).remaining_ticks == 0,
          "plant discount from definitions and no construction");
    s.cancel();
    s.open_catalog();
    s.select(24);
    s.confirm({9, 3});
    const auto recruitment = *s.facility_at({9, 3});
    s.cancel();
    s.update();
    check(s.state().facilities.at(recruitment).remaining_ticks == 0,
          "recruitment construction one counter");
    s.open_catalog();
    s.select(-1);
    check(s.confirm({7, 3}) == StartupError::none && !s.facility_at({7, 3}), "remove binding");
    check(s.state().accounting.funds() == 3700, "no demolition refund");
    s.cancel();
    s.open_catalog();
    s.select(18);
    check(s.confirm({7, 3}) == StartupError::none, "road display override");
    check(s.state().accounting.funds() == 3690 && s.state().terrain_edits.at(3 * 24 + 7) == 37,
          "road fee");
    const auto road = snapshot(s);
    check(s.confirm({7, 3}) == StartupError::occupied && snapshot(s) == road,
          "duplicate road rejection");
    s.cancel();
    s.open_catalog();
    s.select(-1);
    check(s.confirm({7, 3}) == StartupError::none, "remove road");
}
void money_and_calendar() {
    StartupSession s;
    s.open_catalog();
    s.select(28);
    for (int x = 7; x < 12; ++x)
        check(s.confirm({x, 3}) == StartupError::none, "spend starting cash");
    const auto empty = snapshot(s);
    check(s.confirm({12, 3}) == StartupError::insufficient_funds && snapshot(s) == empty,
          "second funds check");
    s.cancel();
    s.open_catalog();
    const auto menu = snapshot(s);
    check(s.select(28) == StartupError::insufficient_funds && snapshot(s) == menu,
          "first funds check");
    s.cancel();
    normal_steps(s, 420);
    s.acknowledge_talk();
    s.acknowledge_talk();
    s.finish_camera();
    normal_steps(s, 1180);
    check(s.state().calendar == std::array<int, 4>{0, 3, 3, 10773},
          "1600 calendar steps excludes modal trigger");
    s.update();
    check(s.state().calendar == std::array<int, 4>{0, 4, 0, 0} &&
              s.state().mode == StartupMode::month_end,
          "calendar month boundary");
    check(s.state().accounting.reports().empty() && s.state().accounting.funds() == 0,
          "no fixture monthly fees");
    const auto boundary = snapshot(s);
    normal_steps(s, 200);
    check(snapshot(s) == boundary, "unknown settlement guarded");
    StartupSession slow, fast;
    normal_steps(slow, 200);
    for (int i = 0; i < 100; ++i)
        fast.update(2);
    check(snapshot(slow) == snapshot(fast), "speed is two eligible steps, not milliseconds");
    const auto before = snapshot(fast);
    check(fast.update(3) == StartupError::invalid_input && snapshot(fast) == before,
          "bad speed atomic");
}
} // namespace
int main() {
    try {
        reset_and_arrival();
        construction();
        money_and_calendar();
        std::cout << "startup checks=" << checks << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
