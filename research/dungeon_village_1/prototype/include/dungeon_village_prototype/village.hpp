#pragma once

#include "dungeon_village_reference/accounting.hpp"
#include "dungeon_village_reference/facility_arrival.hpp"
#include "dungeon_village_reference/map_access.hpp"
#include "dungeon_village_reference/neighbourhood.hpp"

#include <filesystem>

namespace dungeon_village_prototype {

namespace reference = dungeon_village_reference;

struct PrototypeDefinition {
    std::int32_t id{};
    std::string name;
    reference::FacilityShape shape{reference::FacilityShape::single};
    std::int32_t kind{3};
    std::int32_t category{};
    std::int32_t detail{};
    reference::FacilityEconomyDefinition economy;
    std::vector<reference::NeighbourModifier> neighbours;
};

std::vector<PrototypeDefinition> load_prototype_catalog(const std::filesystem::path &table);

struct PrototypeConfig {
    int tick_ms{100};
    int step_ticks{3};
    int use_ticks{12};
    int retry_ticks{5};
    int period_ticks{600};
};

struct PrototypeActor {
    reference::CharacterId id;
    reference::Position cell;
    reference::Position exit_cell;
    reference::ActivityState activity{reference::ActivityState::idle};
    std::optional<reference::BuildingId> target;
    std::uint64_t activity_id{};
    std::vector<reference::Position> path;
    std::size_t cursor{};
    int phase_ticks{};
    int wait_ticks{};
    std::uint32_t flags{};
    reference::FacilityArrivalState arrival;
    std::uint64_t arrivals{};
    std::uint64_t completed{};
    std::uint64_t cancelled{};
};

struct VillageState {
    reference::LegacyMap terrain;
    std::map<reference::BuildingId, reference::FacilityPlacement> facilities;
    std::map<reference::CharacterId, PrototypeActor> actors;
    std::map<reference::BuildingId, std::int32_t> monthly_sales;
    std::map<std::int32_t, std::uint64_t> completed_definition_uses;
    reference::PeriodAccounting accounting;
    std::uint64_t next_instance{1};
    std::uint64_t next_event{1};
    std::uint64_t next_activity{1};
    std::uint64_t random_state{20261003};
    std::uint64_t ticks{};
    std::uint64_t period{1};
    int remainder_ms{};
    bool paused{};
};

enum class VillageError {
    none,
    invalid_input,
    not_found,
    invalid_placement,
    actor_occupied,
    insufficient_funds,
    rule_failure,
    numeric_overflow
};

struct VillageCommandResult {
    VillageError error{VillageError::none};
    std::optional<reference::BuildingId> instance;
};

// Owns the only mutable prototype state; public mutations stage a complete copy.
class Village {
  public:
    Village(std::vector<PrototypeDefinition> catalog, reference::LegacyMap terrain,
            std::int64_t opening_funds = 10000, PrototypeConfig config = {});
    const VillageState &state() const;
    const std::vector<PrototypeDefinition> &catalog() const;
    const PrototypeConfig &config() const;
    const PrototypeDefinition &definition(std::int32_t id) const;
    reference::LegacyMap bound_map() const;
    reference::FacilityEconomyValues values(reference::BuildingId id) const;
    std::optional<reference::BuildingId> facility_at(reference::Position cell) const;
    VillageCommandResult place(std::int32_t definition_id, reference::Position anchor,
                               reference::FacilityOrientation orientation);
    VillageError relocate(reference::BuildingId id, reference::Position anchor,
                          reference::FacilityOrientation orientation);
    VillageError demolish(reference::BuildingId id);
    VillageError add_actor(reference::CharacterId id, reference::Position cell,
                           std::uint32_t flags = 0);
    VillageError advance(int elapsed_ms);
    void set_paused(bool paused);

  private:
    VillageError validate_placement(const reference::FacilityPlacement &placement,
                                    std::optional<reference::BuildingId> ignored) const;
    VillageError post(reference::CashCategory category, reference::CashDirection direction,
                      std::int64_t amount);
    void cancel_activities();
    VillageError begin(PrototypeActor &actor);
    VillageError tick();
    VillageError close_period();
    std::size_t draw(std::size_t bound);
    std::vector<PrototypeDefinition> catalog_;
    PrototypeConfig config_;
    VillageState state_;
};

} // namespace dungeon_village_prototype
