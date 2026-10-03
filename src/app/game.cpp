// Adapts research/prototype startup transactions. Unknown AI, demolition/road and monthly
// side effects are excluded; simulation stops before the unresolved report preparation step.
#include "ark/app/game.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::app {
Game::Game(std::uint32_t random_seed) : random_(random_seed) {
    const auto &data = startup_data();
    state_.money = data.money;
    state_.points = data.points;
    state_.popularity = data.popularity;
    state_.calendar = data.calendar;
    state_.arrival_counter = data.arrival_counter;
    for (const auto &item : data.definitions)
        state_.definition_progress.emplace(item.id, facilities::Progress{});
    // IDs here identify the source-seed projection, not original rebuilt entrance identities.
    for (auto seed : data.seeds) {
        seed.id = state_.next_id++;
        state_.facilities.emplace(seed.id, seed);
    }
}
const facilities::Definition &Game::definition(int id) const {
    const auto &items = startup_data().definitions;
    const auto it =
        std::find_if(items.begin(), items.end(), [id](const auto &v) { return v.id == id; });
    if (it == items.end())
        throw std::out_of_range("Unknown facility definition");
    return *it;
}
const Display &Game::display(int id) const {
    const auto &items = startup_data().displays;
    const auto it =
        std::find_if(items.begin(), items.end(), [id](const auto &v) { return v.id == id; });
    if (it == items.end())
        throw std::out_of_range("Unknown map display record");
    return *it;
}
std::optional<facilities::InstanceId> Game::facility_at(world::Cell cell) const {
    for (const auto &entry : state_.facilities) {
        const auto &v = entry.second;
        for (const auto &part :
             facilities::footprint(definition(v.definition_id).shape, v.orientation, v.anchor))
            if (part.cell == cell)
                return entry.first;
    }
    return std::nullopt;
}
Error Game::open_catalog() {
    if (state_.mode != Mode::normal)
        return Error::wrong_mode;
    state_.mode = Mode::catalog;
    return Error::none;
}
Error Game::select(int id) {
    if (state_.mode != Mode::catalog)
        return Error::wrong_mode;
    const auto &items = startup_data().definitions;
    const auto it =
        std::find_if(items.begin(), items.end(), [id](const auto &v) { return v.id == id; });
    // Roads/removal and residential recruitment have unresolved refresh/effect contracts.
    // New adventurer arrival is a separate automatic flow; do not borrow housing policy.
    if (it == items.end() || it->tab < 0 || it->kind == 6 || it->kind == 13)
        return Error::unavailable;
    if (state_.money < it->price)
        return Error::insufficient_funds;
    state_.selection = id;
    state_.orientation = 0;
    state_.mode = Mode::placement;
    return Error::none;
}
Error Game::rotate() {
    if (state_.mode != Mode::placement)
        return Error::wrong_mode;
    state_.orientation = 1 - state_.orientation;
    return Error::none;
}
Error Game::preview(world::Cell anchor) const {
    if (state_.mode != Mode::placement || !state_.selection)
        return Error::wrong_mode;
    const auto &data = startup_data();
    const auto &item = definition(*state_.selection);
    for (const auto &part : facilities::footprint(item.shape, state_.orientation, anchor)) {
        if (!data.map.contains(part.cell))
            return Error::outside_map;
        if (!data.build_bounds.contains(part.cell))
            return Error::outside_town;
        if (facility_at(part.cell))
            return Error::occupied;
        // Finite source projection: only evidenced ground/road cells, never guessed post-init
        // terrain.
        const int terrain =
            display(data.map.cells[data.map.index(part.cell)].display_id).definition_id;
        if (terrain != 17 && terrain != 18)
            return Error::occupied;
    }
    return state_.money < item.price ? Error::insufficient_funds : Error::none;
}
Error Game::confirm(world::Cell anchor) {
    const auto result = preview(anchor);
    if (result != Error::none)
        return result;
    // Allocate all candidate data before commit, including ledger and stable instance ID.
    // This preserves observed bind/pay results without exposing partial original mutation order.
    auto next = state_;
    const auto &item = definition(*state_.selection);
    const auto id = next.next_id++;
    next.facilities.emplace(id, facilities::Instance{id, item.id, anchor, state_.orientation,
                                                     item.construction_ticks, false});
    next.expenses.push_back({id, 0, item.price});
    next.money -= item.price;
    state_ = std::move(next);
    return Error::none;
}
void Game::cancel() {
    if (state_.mode == Mode::catalog || state_.mode == Mode::placement) {
        state_.selection.reset();
        state_.mode = Mode::normal;
    }
}
void Game::step() {
    // STARTUP/ACCOUNTING: first unresolved report preparation is step 1457, before month end.
    // A finite guard is preferable to silently skipping maintenance or inventing income.
    if (state_.simulation_steps >= 1456) {
        state_.mode = Mode::research_boundary;
        return;
    }
    ++state_.simulation_steps;
    for (auto &entry : state_.facilities)
        if (entry.second.remaining_ticks > 0)
            --entry.second.remaining_ticks;
    if (state_.event89_count == 0 && --state_.arrival_counter == 0) {
        const auto &data = startup_data();
        const auto choice =
            std::uniform_int_distribution<std::size_t>(0, data.spawn_points.size() - 1)(random_);
        state_.adventurer = people::first_visit(data.first_character, data.spawn_points[choice]);
        ++state_.event89_count; // Latch before showing dialogue, never again on close.
        state_.mode = Mode::tutorial;
        return;
    }
    auto &date = state_.calendar;
    date[3] += 27;
    if (date[3] >= 10800) {
        date[3] -= 10800;
        ++date[2];
    }
}
Error Game::update(int speed) {
    if (speed != 1 && speed != 2)
        return Error::invalid_input;
    auto next = *this;
    for (int i = 0; i < speed && !next.state_.paused && next.state_.mode == Mode::normal; ++i)
        next.step();
    *this = std::move(next);
    return Error::none;
}
Error Game::acknowledge_talk() {
    if (state_.mode != Mode::tutorial)
        return Error::wrong_mode;
    if (++state_.talk_line == startup_data().first_talk.size())
        state_.mode = Mode::camera;
    return Error::none;
}
Error Game::finish_camera() {
    if (state_.mode != Mode::camera)
        return Error::wrong_mode;
    state_.mode = Mode::normal;
    return Error::none;
}
} // namespace ark::app
