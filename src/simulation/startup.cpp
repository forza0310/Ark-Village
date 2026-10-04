#include "ark/simulation/startup.hpp"

#include <algorithm>
#include <stdexcept>

namespace ark::simulation {
StartupSession::StartupSession() {
    const auto &data = startup_evidence();
    state_.accounting = ref::PeriodAccounting(data.money, data.points);
    state_.popularity = data.popularity;
    state_.calendar = data.calendar;
    state_.arrival_counter = data.arrival_counter;
    state_.loaded_map = reconstruct_startup_map(data);
    for (const auto &seed : state_.loaded_map.instances) {
        const auto id = static_cast<std::uint64_t>(seed.legacy_id) + 1;
        state_.facilities.emplace(
            id, StartupFacility{id, seed.definition_id, seed.anchor, 0, true, seed.legacy_id});
        state_.next_id = std::max(state_.next_id, id + 1);
    }
}
const StartupState &StartupSession::state() const { return state_; }
const StartupDefinition &StartupSession::definition(int id) const {
    const auto &items = startup_evidence().definitions;
    const auto it =
        std::find_if(items.begin(), items.end(), [id](const auto &v) { return v.id == id; });
    if (it == items.end())
        throw std::out_of_range("新局设施定义不存在");
    return *it;
}
const StartupDisplay &StartupSession::display(int id) const {
    const auto &items = startup_evidence().displays;
    const auto it =
        std::find_if(items.begin(), items.end(), [id](const auto &v) { return v.id == id; });
    if (it == items.end())
        throw std::out_of_range("源地图显示记录不存在");
    return *it;
}
std::optional<std::uint64_t> StartupSession::facility_at(ref::Position cell) const {
    for (const auto &entry : state_.facilities)
        if (entry.second.cell == cell)
            return entry.first;
    return std::nullopt;
}
StartupError StartupSession::open_catalog() {
    if (state_.mode != StartupMode::normal)
        return StartupError::wrong_mode;
    state_.mode = StartupMode::catalog;
    return StartupError::none;
}
StartupError StartupSession::select(int id) {
    if (state_.mode != StartupMode::catalog)
        return StartupError::wrong_mode;
    if (id != -1) {
        const auto &items = startup_evidence().definitions;
        const auto it =
            std::find_if(items.begin(), items.end(), [id](const auto &v) { return v.id == id; });
        if (it == items.end() || it->tab < 0)
            return StartupError::unavailable;
        if (state_.accounting.funds() < it->cost)
            return StartupError::insufficient_funds;
    }
    state_.selection = id;
    state_.mode = StartupMode::placement;
    return StartupError::none;
}
StartupError StartupSession::preview(ref::Position cell) const {
    if (state_.mode != StartupMode::placement || !state_.selection)
        return StartupError::wrong_mode;
    const auto &data = startup_evidence();
    const auto &b = data.build_bounds;
    if (cell.x < b[0] || cell.x > b[1] || cell.y < b[2] || cell.y > b[3])
        return StartupError::outside_town;
    const auto hit = facility_at(cell);
    if (*state_.selection == -1) {
        if (hit) {
            const int kind = definition(state_.facilities.at(*hit).definition_id).kind;
            return kind == 4 || kind == 5 ? StartupError::protected_seed : StartupError::none;
        }
        const auto index = static_cast<std::size_t>(cell.y * data.width + cell.x);
        const auto edit = state_.terrain_edits.find(index);
        const int terrain = edit == state_.terrain_edits.end()
                                ? state_.loaded_map.cells[index].definition_id
                                : display(edit->second).definition_id;
        return terrain == 18 ? StartupError::none : StartupError::not_found;
    }
    if (hit)
        return StartupError::occupied;
    // Loading clears special terrain inside the town; source display IDs cannot validate building.
    const auto index = static_cast<std::size_t>(cell.y * data.width + cell.x);
    const auto edit = state_.terrain_edits.find(index);
    const int terrain = edit == state_.terrain_edits.end()
                            ? state_.loaded_map.cells[index].definition_id
                            : display(edit->second).definition_id;
    if (terrain != 17 && terrain != 18)
        return StartupError::occupied;
    if (*state_.selection == 18 && terrain == 18)
        return StartupError::occupied;
    return state_.accounting.funds() < definition(*state_.selection).cost
               ? StartupError::insufficient_funds
               : StartupError::none;
}
StartupError StartupSession::confirm(ref::Position cell) {
    const auto error = preview(cell);
    if (error != StartupError::none)
        return error;
    auto next = *this;
    const auto index = static_cast<std::size_t>(cell.y * startup_evidence().width + cell.x);
    const int id = *state_.selection;
    if (id == -1) {
        const auto hit = next.facility_at(cell);
        if (hit)
            next.state_.facilities.erase(*hit);
        // Prototype replacement ground, not a recovered original demolition refresh.
        next.state_.terrain_edits[index] = 27;
    } else {
        const auto &item = definition(id);
        if (item.kind == 6)
            next.state_.terrain_edits[index] = item.display_id;
        else {
            const auto instance = next.state_.next_id++;
            next.state_.facilities.emplace(
                instance, StartupFacility{instance, id, cell, item.construction_ticks, false, {}});
            next.state_.terrain_edits[index] = 27;
        }
        // Preserve construction ledger category 0; PeriodAccounting's enum slot 0 is facilities.
        const auto result = next.state_.accounting.post_cash(
            {next.state_.next_cash_id++, 1, ref::CashCategory::facilities,
             ref::CashDirection::expense, item.cost});
        if (result != ref::AccountingError::none)
            throw std::overflow_error("建设账本提交失败");
    }
    *this = std::move(next);
    return StartupError::none;
}
void StartupSession::cancel() {
    if (state_.mode == StartupMode::catalog || state_.mode == StartupMode::placement) {
        state_.selection.reset();
        state_.mode = StartupMode::normal;
    }
}
void StartupSession::set_paused(bool paused) { state_.paused = paused; }
// Only eligible simulation advances construction and B. The first character is installed before
// event89, and triggering the modal prevents that step's calendar advancement. No unknown first AI.
void StartupSession::step() {
    ++state_.simulation_steps;
    for (auto &entry : state_.facilities)
        if (entry.second.remaining_ticks > 0)
            --entry.second.remaining_ticks;
    if (state_.event89_count == 0 && --state_.arrival_counter == 0) {
        auto actor = startup_evidence().first_character;
        // Replayable local choice, not an assertion about APK RNG consumption.
        actor.cell =
            startup_evidence()
                .spawn_points[state_.simulation_steps % startup_evidence().spawn_points.size()];
        actor.flags |= 2U | 8192U; // Character initialization bit and first-arrival bit differ.
        actor.pending_activity = 0;
        state_.character = std::move(actor);
        ++state_.event89_count;
        state_.mode = StartupMode::tutorial;
        return;
    }
    auto &date = state_.calendar;
    date[3] += 27;
    if (date[3] >= 10800) {
        date[3] -= 10800;
        if (++date[2] == 4) {
            date[2] = 0;
            if (++date[1] == 12) {
                date[1] = 0;
                ++date[0];
            }
            // Do not charge fixture upkeep or invent original cross-month ordering.
            state_.mode = StartupMode::month_end;
        }
    }
}
StartupError StartupSession::update(int speed) {
    if (speed != 1 && speed != 2)
        return StartupError::invalid_input;
    auto next = *this;
    for (int i = 0; i < speed && !next.state_.paused && next.state_.mode == StartupMode::normal;
         ++i)
        next.step();
    *this = std::move(next);
    return StartupError::none;
}
StartupError StartupSession::acknowledge_talk() {
    if (state_.mode != StartupMode::tutorial)
        return StartupError::wrong_mode;
    if (++state_.talk_line == startup_evidence().first_talk.size())
        state_.mode = StartupMode::camera;
    return StartupError::none;
}
StartupError StartupSession::finish_camera() {
    if (state_.mode != StartupMode::camera)
        return StartupError::wrong_mode;
    state_.mode = StartupMode::normal;
    return StartupError::none;
}
} // namespace ark::simulation
