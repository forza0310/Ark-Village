#pragma once

// The desktop reads immutable publications; only the worker commits the canonical world.
#include "ark/app/save/world_save_files.hpp"
#include "ark/app/session/world_system.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"
#include "ark/simulation/facilities/startup_world_commerce.hpp"
#include "ark/simulation/facilities/startup_world_editing.hpp"
#include "ark/simulation/facilities/startup_world_facility_catalog.hpp"
#include "ark/simulation/facilities/startup_world_facility_items.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include "ark/simulation/facilities/startup_world_magic_pot.hpp"
#include "ark/simulation/world/startup_world_runtime.hpp"
#include "ark/simulation/village/startup_world_tax.hpp"
#include "ark/simulation/village/startup_world_village_activity.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace ark::app {
using WorldState = simulation::StartupWorldRuntimeState;
enum class WorldCommandKind {
    retry_system_write,
    acknowledge_page,
    acknowledge_report,
    award_action,
    rank_action,
    open_menu_build,
    open_build_menu,
    select_build_menu,
    cancel_build_menu,
    confirm_build,
    cancel_build,
    confirm_edit,
    cancel_edit,
    open_facility,
    open_human,
    human_action,
    tax_action,
    open_menu_village_activities,
    open_village_activities,
    village_activity_action,
    open_menu_commerce,
    open_commerce,
    commerce_action,
    facility_item_action,
    facility_catalog_action,
    open_menu_magic_pot,
    open_magic_pot,
    magic_pot_action,
    facility_action,
    residence_action,
    open_task_control_menu,
    open_main_menu,
    close_main_menu,
    open_save_menu,
    close_save_menu,
    save_slot,
    load_slot,
    open_menu_tasks,
    open_task_menu,
    task_action,
    page_confirm_held,
    cancel_page,
    set_paused,
    set_speed,
    set_view
};
enum class WorldCommandOutcome { applied, rejected };
struct WorldCommandResult {
    std::uint64_t serial{};
    std::uint64_t page{};
    WorldCommandKind kind{WorldCommandKind::open_task_menu};
    WorldCommandOutcome outcome{WorldCommandOutcome::applied};
    simulation::StartupWorldRuntimeError runtime_error{simulation::StartupWorldRuntimeError::none};
    simulation::rules::TaskCommandDenial denial{simulation::rules::TaskCommandDenial::none};
    simulation::StartupBuildDenial build_denial{simulation::StartupBuildDenial::none};
    std::optional<std::uint64_t> created;
    // Successful raw84 transaction receipt, bound to its command page; not durable world data.
    std::optional<std::int64_t> commerce_amount;
    bool task_accepted{};
    bool departed{};
};
struct WorldFrame {
    std::shared_ptr<const WorldState> state;
    std::shared_ptr<const WorldState> previous;
    std::chrono::steady_clock::time_point published;
    double interval_seconds{.047};
    std::uint64_t revision{};
    std::uint64_t last_command_serial{};
    // Desktop adaptation only: this gate is not a synthetic source raw3 page or user pause.
    bool main_menu_open{};
    bool save_menu_open{};
    bool save_busy{};
    std::string save_message;
    std::array<WorldSaveSlotInfo, 2> save_slots;
    std::uint64_t generation{1};
    WorldSystemState system;
    // A failed durable transaction retains the old world and can be explicitly retried.
    std::string system_error;
    bool failed{};
    std::string error;
    std::uint64_t outer_updates{};
    double max_update_ms{};
    // Count of committed outputs handed to the independent once-only audio FIFO.
    std::uint64_t consumed_sound_requests{};
    // Last 64 explicit decision results, retained across ticks and camera publications.
    // This bounded FIFO acknowledgement history is not a gameplay event log.
    std::vector<WorldCommandResult> command_results;
};

struct WorldCommand {
    // Zero binds to the current publication on submission. UI may bind an observed generation.
    std::uint64_t generation{};
    WorldCommandKind kind{WorldCommandKind::set_paused};
    std::uint64_t page{};
    int report_phase{};
    simulation::rules::WorldAwardAction award_action{simulation::rules::WorldAwardAction::update};
    simulation::StartupWorldTaskAction task_action{simulation::StartupWorldTaskAction::confirm};
    int selection{};
    int definition{};
    std::uint64_t facility{};
    simulation::rules::CharacterId actor{};
    simulation::StartupHumanPageAction human_action{simulation::StartupHumanPageAction::confirm};
    simulation::StartupWorldTaxAction tax_action{simulation::StartupWorldTaxAction::confirm};
    simulation::StartupVillageActivityAction village_activity_action{
        simulation::StartupVillageActivityAction::confirm};
    simulation::StartupCommerceAction commerce_action{simulation::StartupCommerceAction::confirm};
    simulation::StartupFacilityItemAction facility_item_action{
        simulation::StartupFacilityItemAction::confirm};
    simulation::StartupFacilityCatalogAction facility_catalog_action{
        simulation::StartupFacilityCatalogAction::confirm};
    simulation::StartupMagicPotAction magic_pot_action{simulation::StartupMagicPotAction::confirm};
    simulation::rules::Position anchor{};
    // Transient selection binding, distinct from the destination anchor.
    std::optional<simulation::rules::Position> edit_anchor;
    simulation::rules::FacilityOrientation orientation{};
    simulation::StartupFacilityPageAction facility_action{
        simulation::StartupFacilityPageAction::confirm};
    bool cancel{};
    bool held{};
    bool paused{};
    int speed{};
    std::array<float, 2> camera{};
    std::array<int, 4> viewport{};
};

// Commands enter a FIFO and commit only between complete source-runtime updates. The worker
// uses the source minimum start interval, never render FPS, catch-up debt or a second speed
// multiplier. Destruction wakes and joins it; an in-flight atomic update finishes first.
class WorldSession {
  public:
    explicit WorldSession(WorldState initial, std::filesystem::path save_directory = {});
    ~WorldSession();
    WorldSession(const WorldSession &) = delete;
    WorldSession &operator=(const WorldSession &) = delete;

    std::shared_ptr<const WorldFrame> frame() const;
    // Main-thread presentation drain. Skipped/re-read snapshots cannot lose/replay requests.
    std::vector<simulation::StartupAudioRequest> take_audio_requests();
    // Zero means the session has stopped/failed and did not accept the command. Accepted
    // serials are strictly increasing; a failed command is acknowledged by the failure frame.
    std::uint64_t submit(WorldCommand command);
    std::uint64_t ack_page(std::uint64_t page);
    std::uint64_t ack_report(int expected_phase);
    // Annual-page input is explicit: ordinary page confirmation never chooses termination.
    std::uint64_t act_award(std::uint64_t page, simulation::rules::WorldAwardAction action,
                            int selection = 0);
    std::uint64_t act_rank(std::uint64_t page, int selection = 0, bool cancel = false);
    // Menu visibility is worker-owned metadata; closing never changes explicit pause.
    std::uint64_t open_main_menu();
    std::uint64_t close_main_menu();
    std::uint64_t open_save_menu();
    std::uint64_t close_save_menu();
    std::uint64_t save_slot(int slot);
    std::uint64_t load_slot(int slot);
    // Atomically opens the real task source page and closes the desktop menu on success.
    std::uint64_t open_menu_tasks();
    std::uint64_t open_task_menu();
    std::uint64_t open_task_control_menu();
    std::uint64_t open_menu_build();
    std::uint64_t open_build_menu();
    std::uint64_t select_build_menu(std::uint64_t page, int definition);
    std::uint64_t cancel_build_menu(std::uint64_t page);
    // Bind placement to the selection observed by the UI; an old click cannot build a new item.
    std::uint64_t confirm_build(int expected_definition, simulation::rules::Position anchor,
                                simulation::rules::FacilityOrientation orientation);
    std::uint64_t cancel_build(int expected_definition);
    std::uint64_t open_facility(std::uint64_t facility);
    // Both identities must still describe the same active human when the FIFO is consumed.
    std::uint64_t open_human(simulation::rules::CharacterId actor, int definition);
    std::uint64_t act_human(std::uint64_t page, simulation::StartupHumanPageAction action,
                            int selection = 0);
    std::uint64_t act_tax(std::uint64_t page, simulation::StartupWorldTaxAction action,
                          int selection = 0);
    std::uint64_t open_menu_village_activities();
    std::uint64_t open_village_activities();
    std::uint64_t act_village_activity(std::uint64_t page,
                                       simulation::StartupVillageActivityAction action,
                                       int selection = 0);
    std::uint64_t open_menu_commerce();
    // Bind the observed stage/start/instance so a delayed click cannot edit a new selection.
    std::uint64_t confirm_edit(const WorldState &observed, simulation::rules::Position target,
                               simulation::rules::FacilityOrientation orientation);
    std::uint64_t cancel_edit(const WorldState &observed);
    std::uint64_t open_commerce();
    std::uint64_t open_menu_magic_pot();
    std::uint64_t open_magic_pot();
    std::uint64_t act_magic_pot(std::uint64_t page, simulation::StartupMagicPotAction action,
                                int selection = 0);
    std::uint64_t act_commerce(std::uint64_t page, simulation::StartupCommerceAction action,
                               int selection = 0);
    std::uint64_t act_facility_item(std::uint64_t page,
                                    simulation::StartupFacilityItemAction action,
                                    int selection = -1);
    std::uint64_t act_facility(std::uint64_t page, simulation::StartupFacilityPageAction action);
    std::uint64_t act_facility_catalog(std::uint64_t page,
                                       simulation::StartupFacilityCatalogAction action,
                                       int selection = 0);
    std::uint64_t act_residence(std::uint64_t page, int human, bool cancel = false);
    std::uint64_t act_task_page(std::uint64_t page, simulation::StartupWorldTaskAction action,
                                int selection = 0);
    // A held edge belongs only to this raw24 identity. Send false on release/focus loss;
    // page transitions and pause also clear it. Source page updates, never FPS, consume it.
    std::uint64_t set_page_confirm_held(std::uint64_t page, bool held);
    std::uint64_t cancel_page(std::uint64_t page); // Explicit raw83 Back, not ordinary confirm.
    std::uint64_t set_paused(bool paused);
    std::uint64_t set_speed(int setting);
    std::uint64_t set_view(std::array<float, 2> camera, std::array<int, 4> viewport);
    // Useful to headless observers/tests: wait for a newer publication without polling/sleeping.
    // Timeout or stop returns the latest publication, which may retain the supplied revision.
    std::shared_ptr<const WorldFrame> wait_for_frame_after(std::uint64_t revision,
                                                           std::chrono::milliseconds timeout) const;
    void stop();

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace ark::app
