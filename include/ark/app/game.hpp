#pragma once

// Sole mutable aggregate; desktop sends commands and reads immutable state.
#include "ark/app/startup_data.hpp"
#include <map>
#include <optional>
#include <random>

namespace ark::app {
enum class Mode { normal, catalog, placement, tutorial, camera, research_boundary };
enum class Error {
    none,
    wrong_mode,
    unavailable,
    insufficient_funds,
    outside_map,
    outside_town,
    occupied,
    invalid_input
};
struct CashEntry {
    facilities::InstanceId instance{};
    int category{};
    std::int64_t expense{};
};
struct State {
    std::int64_t money{};
    int points{}, popularity{};
    std::array<int, 4> calendar{};
    std::map<int, facilities::Progress> definition_progress;
    std::map<facilities::InstanceId, facilities::Instance> facilities;
    std::vector<CashEntry> expenses;
    std::optional<people::Adventurer> adventurer;
    Mode mode{Mode::normal};
    std::optional<int> selection;
    int orientation{}, arrival_counter{}, event89_count{};
    std::size_t talk_line{};
    std::uint64_t simulation_steps{}, next_id{1};
    bool paused{};
};
class Game {
  public:
    explicit Game(std::uint32_t random_seed = 20261003U);
    const State &state() const { return state_; }
    const facilities::Definition &definition(int id) const;
    const Display &display(int id) const;
    std::optional<facilities::InstanceId> facility_at(world::Cell cell) const;
    Error open_catalog();
    Error select(int definition_id);
    Error rotate();
    // Preview and commit share full footprint validation. Confirm repeats the cash check.
    Error preview(world::Cell anchor) const;
    Error confirm(world::Cell anchor);
    void cancel();
    void set_paused(bool value) { state_.paused = value; }
    // One or two eligible logical steps; modal/placement/camera modes do not accrue simulation.
    Error update(int speed = 1);
    Error acknowledge_talk();
    Error finish_camera();

  private:
    void step();
    State state_;
    std::mt19937 random_;
};
} // namespace ark::app
