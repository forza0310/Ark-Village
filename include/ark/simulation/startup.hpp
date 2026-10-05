#pragma once

// Static first-play evidence and an independent transaction owner. Source and loaded cells remain
// separate; unknown AI and month-end effects are deliberately not simulated.
#include "ark/simulation/startup_map.hpp"
#include "ark/simulation/rules/accounting.hpp"
#include "ark/simulation/rules/facility_economy.hpp"
#include "ark/simulation/rules/facility_exit.hpp"
#include "ark/simulation/rules/geometry.hpp"
#include "ark/simulation/rules/neighbourhood.hpp"

#include <array>
#include <string>

namespace ark::simulation {
namespace ref = ark::simulation::rules;

struct SourceCell {
    int display_id{};
    int variant{};
};
struct StartupDisplay {
    int id{};
    int definition_id{};
    std::string sprite;
    int offset_y{};
    std::uint32_t flags{}; // a.j.o，绘制深度守卫；不是设施flags。
};
struct StartupDefinition {
    int id{};
    std::string name;
    int kind{};
    int tab{-1};
    int cost{};
    int construction_ticks{}; // 0 means immediately usable, not an inferred zero-length timer.
    int display_id{};
    int category{};
    int direction{};
    int flags{};
    int shape{};
    int definition_charm{};
    int detail{};
    int use_wait{}; // Source column 25; category-2 activity 0/1 overrides it with 200.
    ref::FacilityEconomyDefinition economy; // Raw endpoints, not effective construction quotes.
    std::vector<ref::FacilityAttributeEffect> exit_effects; // Source columns28/29, original order.
    std::vector<ref::NeighbourModifier> neighbour_effects; // Source26/27, separate from exit gains.
    int unlock_rank{}; // 原o.D，tenantData第32索引列，仅用于晋级后的提醒匹配。
};
struct StartupFacility {
    std::uint64_t id{}; // Nonzero prototype identity; raw zero is represented explicitly below.
    int definition_id{};
    ref::Position cell;
    int remaining_ticks{};
    bool seed{};
    std::optional<int> legacy_id;
};
struct StartupCharacter {
    int uid{};
    int definition_id{};
    std::string name;
    int job_id{};
    int sex{};
    int level{};
    int effort{};
    int satisfaction{};
    std::array<int, 6> attributes{};
    std::array<int, 4> equipment{};
    std::array<int, 4> combat{};
    std::array<int, 3> hp{};
    ref::Position cell;
    std::uint32_t flags{};
    // Observed opcode-8 activity request; retained pending while first-play AI is unresolved.
    std::optional<int> pending_activity{};
    std::array<int, 2>
        job_satisfaction_thresholds{}; // Source job columns 13/14, not chosen defaults.
    int weapon_reselect_counter{}; // Legacy A[0], set to6 by initial equip; not weapon inventory.
    std::array<int, 4> legacy_D{}; // 无继承重置；D[2]==0不代表存在住宅实例。
    int legacy_m{};                // 共享人物定义离村倒计时，初值0。
};
struct StartupEvidence {
    int width{};
    int height{};
    std::vector<SourceCell> cells;
    std::vector<StartupDisplay> displays;
    std::vector<StartupDefinition> definitions;
    std::vector<StartupFacility> seeds;
    std::vector<ref::Position> spawn_points;
    std::array<int, 4> build_bounds{}; // Inclusive min_x, max_x, min_y, max_y.
    std::int64_t money{};
    std::uint16_t points{};
    int popularity{};
    std::array<int, 4> calendar{};
    std::vector<int> unlocked_characters;
    int arrival_counter{};
    ref::Position camera;
    StartupCharacter first_character;
    std::vector<std::string> first_talk;
    int character_spawn_minimum_y{};
    int world_overlap_boundary_y{};
};

// Generated at build time from the published JSON with JSON.parse, not a handwritten C++ parser.
const StartupEvidence &startup_evidence();

enum class StartupMode { normal, catalog, placement, tutorial, camera, month_end };
enum class StartupError {
    none,
    wrong_mode,
    unavailable,
    insufficient_funds,
    outside_town,
    occupied,
    not_found,
    protected_seed,
    invalid_input
};
struct StartupState {
    ref::PeriodAccounting accounting;
    int popularity{};
    std::array<int, 4> calendar{};
    std::map<std::uint64_t, StartupFacility> facilities;
    LoadedStartupMap loaded_map; // Reset snapshot only; later terrain edits are adapter overrides.
    std::map<std::size_t, int>
        terrain_edits; // Explicit prototype display overrides, not source cells.
    std::optional<StartupCharacter> character;
    StartupMode mode{StartupMode::normal};
    std::optional<int> selection;
    int arrival_counter{};
    int event89_count{};
    std::size_t talk_line{};
    std::uint64_t simulation_steps{};
    std::uint64_t next_id{1};
    std::uint64_t next_cash_id{1};
    bool paused{};
};

// Owns reset, first arrival, construction and calendar together. Commands validate a candidate
// copy before commit, so rejection cannot leave bindings, identity allocations or partial cash.
class StartupSession {
  public:
    StartupSession();
    const StartupState &state() const;
    const StartupDefinition &definition(int id) const;
    const StartupDisplay &display(int id) const;
    std::optional<std::uint64_t> facility_at(ref::Position cell) const;
    StartupError open_catalog();
    StartupError select(int id); // First affordability check; -1 is free removal, no move command.
    StartupError preview(ref::Position cell) const;
    StartupError
    confirm(ref::Position cell); // Second affordability check, bind and pay atomically.
    void cancel();
    void set_paused(bool paused);
    // An outer update has one or at most two eligible logical steps, never a millisecond input.
    StartupError update(int speed = 1);
    StartupError acknowledge_talk();
    StartupError finish_camera();

  private:
    void step();
    StartupState state_;
};
} // namespace ark::simulation
