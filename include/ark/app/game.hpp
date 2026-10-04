#pragma once

// Sole mutable aggregate; desktop sends commands and reads immutable state.
#include "ark/app/initial_ai.hpp"
#include "ark/app/startup_data.hpp"
#include "ark/facilities/neighbourhood.hpp"
#include <map>
#include <optional>
#include <random>

namespace ark::app {
enum class Mode { normal, catalog, placement, tutorial, camera, research_boundary };
enum class PlayMode { startup, ai_preview };
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
    std::int64_t money{}; // Read-only UI projection; all writers post to accounting.
    economy::CashLedger accounting;
    std::uint64_t next_cash_id{1}, layout_revision{};
    int points{}, popularity{};
    std::array<int, 4> calendar{};
    std::map<int, facilities::Progress> definition_progress;
    std::map<facilities::InstanceId, facilities::Instance> facilities;
    std::vector<facilities::InstanceId> instance_order; // Original loaded order; append new IDs.
    std::vector<CashEntry> expenses;
    std::optional<people::Adventurer> adventurer;
    std::optional<LifeActorState> life;
    std::optional<LifeActorState> retired_life; // Retained diagnostic identity, never scheduled.
    std::map<int, int>
        departed_definitions; // Shared definition m writes; no guessed initial state.
    std::map<facilities::InstanceId, FacilityLifeState> facility_life;
    Mode mode{Mode::normal};
    std::optional<int> selection;
    int orientation{}, arrival_counter{}, event89_count{};
    std::size_t talk_line{};
    std::uint64_t simulation_steps{}, next_id{1};
    bool paused{};
};
class Game {
  public:
    explicit Game(std::uint32_t random_seed = 20261003U, PlayMode play = PlayMode::startup);
    const State &state() const { return state_; }
    bool ai_preview_enabled() const { return play_ == PlayMode::ai_preview; }
    const InitialAiState *ai_state() const { return ai_ ? &ai_->state() : nullptr; }
    const LifeActorState *life_state() const { return state_.life ? &*state_.life : nullptr; }
    InitialAiError ai_error() const { return ai_error_; }
    const facilities::Definition &definition(int id) const;
    const Display &display(int id) const;
    std::optional<facilities::InstanceId> facility_at(world::Cell cell) const;
    // Read-only derived views. Definition previews omit instance modifiers and never charge.
    facilities::EconomyValues
    facility_values(int definition_id,
                    std::optional<facilities::InstanceId> instance = std::nullopt) const;
    facilities::Neighbourhood neighbourhood(facilities::InstanceId instance) const;
    // Read-only current occupancy and conditional route. Does not choose an AI goal or move actors.
    world::RouteMap route_map() const;
    world::Route route_to(world::Cell start, world::ArrivalTarget target) const;
    Error open_catalog();
    Error select(int definition_id);
    Error rotate();
    // Preview and commit share full footprint validation. Confirm repeats the cash check.
    Error preview(world::Cell anchor) const;
    Error confirm(world::Cell anchor);
    void cancel();
    void set_paused(bool value) { state_.paused = value; }
    // Camera interpolation is presentation, not a source common-world pause gate.
    bool simulation_eligible() const {
        return !state_.paused && (state_.mode == Mode::normal ||
                                  (!ai_preview_enabled() && state_.mode == Mode::camera));
    }
    // Eligibility is reread before every speed iteration; tutorials and construction UI block.
    Error update(int speed = 1);
    Error acknowledge_talk();
    Error finish_camera();

  private:
    void step();
    void step_normal_world();
    void commit_village_life(const InitialAiState &);
    void start_ai_preview();
    void step_ai_preview();
    void project_ai_preview();
    void start_village_life();
    void project_village_actor();
    State state_;
    std::mt19937 random_;
    PlayMode play_;
    std::optional<InitialAiSession> ai_;
    InitialAiError ai_error_{InitialAiError::none};
};
} // namespace ark::app
