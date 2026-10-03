#pragma once

// Static first-play evidence and an independent transaction owner. Source cells are never a
// post-initialization snapshot; unknown AI and month-end effects are deliberately not simulated.
#include "dungeon_village_reference/accounting.hpp"
#include "dungeon_village_reference/geometry.hpp"

#include <array>
#include <string>

namespace dungeon_village_prototype {
namespace ref = dungeon_village_reference;

struct SourceCell {
    int display_id{};
    int variant{};
};
struct StartupDisplay {
    int id{};
    int definition_id{};
    std::string sprite;
    int offset_y{};
};
struct StartupDefinition {
    int id{};
    std::string name;
    int kind{};
    int tab{-1};
    int cost{};
    int construction_ticks{}; // 0 means immediately usable, not an inferred zero-length timer.
    int display_id{};
};
struct StartupFacility {
    std::uint64_t id{}; // Prototype identity, not a certified original Tenant ID.
    int definition_id{};
    ref::Position cell;
    int remaining_ticks{};
    bool seed{};
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
} // namespace dungeon_village_prototype
