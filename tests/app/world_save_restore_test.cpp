#include "ark/app/save/world_save.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"
#include "ark/simulation/facilities/startup_world_editing.hpp"
#include "ark/simulation/map/startup_world_expansion.hpp"
#include "ark/simulation/facilities/startup_world_magic_pot.hpp"
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"
#include "../support/world_fixture.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
namespace app = ark::app;
namespace sim = ark::simulation;
namespace ref = sim::rules;
using State = sim::StartupWorldRuntimeState;
int checks{};
void check(bool value, const std::string &message) {
    ++checks;
    if (!value)
        throw std::runtime_error("save restore: " + message);
}
void advance(State &s) {
    const auto result = sim::prepare_startup_world_runtime(s);
    check(result.candidate.has_value(), "real source world update");
    s = *result.candidate;
    check(sim::update_startup_world_render_cache(s), "real source render cache update");
    s.sound_requests.clear();
    const auto raw = s.scripts.pages.back().legacy_page;
    if (s.scripts.pages.back().kind == ref::WorldScriptPageKind::dialogue ||
        s.scripts.pages.back().kind == ref::WorldScriptPageKind::simple_message ||
        s.scripts.pages.back().kind == ref::WorldScriptPageKind::newspaper ||
        (s.scripts.pages.back().kind == ref::WorldScriptPageKind::raw_page &&
         (raw == 11 || raw == 59 || raw == 67 || raw == 88 || raw == 96))) {
        const auto page = s.scripts.pages.back();
        // This is explicit diagnostic input, only for pages naturally created by the source.
        const auto error = sim::acknowledge_startup_world_runtime_page(s, page.id);
        check(error == sim::StartupWorldRuntimeError::none ||
                  error == sim::StartupWorldRuntimeError::invalid_page,
              "source acknowledgement or readiness rejection page=" +
                  std::to_string(page.legacy_page) +
                  " error=" + std::to_string(static_cast<int>(error)));
    }
}
State restored(const State &saved) {
    const auto image = app::capture_world_save(saved);
    check(image.image.has_value(), image.message);
    const auto decoded = app::decode_world_save(image.image->bytes);
    check(decoded.state.has_value(), decoded.message);
    auto candidate = *decoded.state;
    std::string reason;
    check(app::prepare_world_save_candidate(candidate, saved, reason) == app::WorldSaveError::none,
          reason);
    return candidate;
}
void discard_nonpersistent_effects(State &baseline) {
    for (auto &context : baseline.scene.world.world.ai.contexts)
        context.second.effects = {};
    baseline.sound_requests.clear();
    baseline.scripts.notices.clear();
    baseline.visual_effects.clear();
    baseline.delayed_effects.clear();
    baseline.global_effects.clear();
    baseline.scene.world.hints.clear();
    baseline.scene.world.floating_notes.clear();
    baseline.exploration_displays.clear();
    baseline.dungeon_labels.clear();
}
void same_durable(const State &a, const State &b, const char *scenario) {
    check(ark::test::same_world_clock(a, b), std::string(scenario) + " source clocks");
    check(a.scene.random.draws() == b.scene.random.draws(),
          std::string(scenario) + " controlled random consumption");
    const auto left = app::capture_world_save(a);
    const auto right = app::capture_world_save(b);
    check(left.image.has_value(), left.message);
    check(right.image.has_value(), right.message);
    check(left.image->bytes == right.image->bytes,
          std::string(scenario) + " durable field equality");
}
void invalid_candidates() {
    auto initial = ark::test::initial_world();
    std::string reason;
    check(app::validate_world_save_candidate(initial, reason) == app::WorldSaveError::none, reason);
    const auto reject = [&](const State &bad, const char *scenario) {
        auto candidate = bad;
        const auto cash = candidate.scene.world.world.ai.accounting.funds();
        const auto draws = candidate.scene.random.draws();
        const auto steps = candidate.simulation_steps;
        check(app::prepare_world_save_candidate(candidate, initial, reason) !=
                  app::WorldSaveError::none,
              std::string("reject ") + scenario);
        check(candidate.scene.world.world.ai.accounting.funds() == cash &&
                  candidate.scene.random.draws() == draws && candidate.simulation_steps == steps,
              std::string("no partial restore for ") + scenario);
    };
    auto bad = initial;
    bad.magic_pot_recipes.erase(bad.magic_pot_recipes.begin());
    reject(bad, "missing magic pot recipe");
    bad = initial;
    bad.magic_pot_recipes.emplace(99999, ref::WorldMagicPotRecipeProgress{99999, 0, false});
    reject(bad, "unknown magic pot recipe");
    bad.magic_pot_recipes.erase(bad.magic_pot_recipes.begin());
    reject(bad, "foreign magic pot recipe replacing a fixed identity");
    bad = initial;
    bad.magic_pot_recipes.begin()->second.identity = 99999;
    reject(bad, "magic pot recipe key and identity mismatch");
    for (int status : {-1, 2}) {
        bad = initial;
        bad.magic_pot_recipes.begin()->second.status = status;
        reject(bad, "invalid magic pot recipe status");
    }
    for (const auto [slot, value] : std::array<std::array<int, 2>, 6>{
             {{0, 1000}, {1, 11}, {3, 1000}, {7, -1}, {11, 0}, {11, 4}}}) {
        bad = initial;
        bad.legacy_n[slot] = value;
        reject(bad, "invalid durable magic pot slot");
    }
    bad = initial;
    const auto date = ref::world_magic_pot_date(
        {bad.scene.calendar.year, bad.scene.calendar.month, bad.scene.calendar.subperiod});
    check(date.value.has_value(), "initial magic pot date is valid");
    bad.legacy_n[12] = *date.value + 1;
    reject(bad, "future magic pot processing date");
    bad = initial;
    bad.scene.world.facility_order.push_back(bad.scene.world.facility_order.front());
    reject(bad, "duplicate facility order identity");
    bad = initial;
    bad.scene.world.world.facilities.begin()->second.placement.instance_id.value += 100;
    reject(bad, "mismatched facility stable identity");
    bad = initial;
    bad.scene.world.world.facilities.begin()->second.occupants.push_back({999999});
    reject(bad, "dangling facility occupant");
    bad = initial;
    const auto first = bad.scene.world.world.facilities.begin()->second.placement.anchor;
    bad.scene.world.world.map.cells
        .at(static_cast<std::size_t>(first.y) * bad.scene.world.world.map.width + first.x)
        .facility.reset();
    reject(bad, "missing occupied tile binding");
    bad = initial;
    bad.scene.calendar.units = 10800;
    reject(bad, "unnormalized week clock");
    bad = initial;
    bad.scene.calendar.year = std::numeric_limits<int>::max();
    reject(bad, "year display overflow");
    bad = initial;
    bad.camera[0] = std::numeric_limits<float>::infinity();
    reject(bad, "nonfinite projection coordinate");
    bad = initial;
    bad.scene.world.world.ai.growth.begin()->second.definition.current_profession = 99999;
    reject(bad, "unknown current profession");
    bad = initial;
    bad.shop_humans.begin()->second.equipment[0] = 99999;
    reject(bad, "unknown equipment");
    bad = initial;
    ref::WorldScriptContinuation invalid_continuation;
    invalid_continuation.event = 99999;
    invalid_continuation.remaining_updates = 1;
    bad.scripts.continuations.push_back(invalid_continuation);
    reject(bad, "unknown delayed script identity");
    bad = initial;
    bad.scripts.facilities.erase(bad.scripts.facilities.begin());
    reject(bad, "incomplete shared facility directory");
    bad = initial;
    bad.scene.world.world.human_spending.erase(bad.scene.world.world.human_spending.begin());
    reject(bad, "incomplete shared human spending");
    bad = initial;
    bad.task_progress.definitions.erase(bad.task_progress.definitions.begin());
    reject(bad, "incomplete shared task definitions");
    bad = initial;
    bad.task_progress.definitions.begin()->second.kind = 999;
    reject(bad, "unknown shared task kind");
    bad = initial;
    bad.catalog.emplace(std::pair<int, int>{1, 99999}, ref::ObjectCatalogRecord{});
    bad.shop_humans.begin()->second.equipment[0] = 99999;
    reject(bad, "fabricated equipment catalog key");
    bad = initial;
    bad.scene.world.town.right = bad.scene.world.town.left;
    reject(bad, "empty town bounds");
    bad = initial;
    --bad.scene.world.town.left;
    reject(bad, "in-map town differs from its source fence level");
    bad = initial;
    ++bad.fence_level;
    reject(bad, "source fence level differs from saved town bounds");
    bad = initial;
    bad.scene.world.spawn_cells.clear();
    reject(bad, "empty arrival spawn cells");
    bad = initial;
    bad.scene.world.updates = std::numeric_limits<int>::max();
    reject(bad, "world update counter overflow");
    bad = initial;
    bad.dungeon_facilities.begin()->second.updates = std::numeric_limits<int>::max();
    reject(bad, "facility update counter overflow");
    bad = initial;
    bad.shop_item_stock.begin()->second.definition = 99999;
    reject(bad, "mismatched shop item value identity");
    for (int field = 0; field < 6; ++field) {
        bad = initial;
        auto &item = bad.items.begin()->second;
        switch (field) {
        case 0:
            item.flags ^= 1U;
            break;
        case 1:
            ++item.status;
            break;
        case 2:
            ++item.unlock_counter;
            break;
        case 3:
            item.newly_unlocked = !item.newly_unlocked;
            break;
        case 4:
            item.inventory = item.inventory == 999 ? 998 : item.inventory + 1;
            break;
        case 5:
            ++item.free_purchases;
            break;
        }
        reject(bad, "incoherent item owner/catalog field");
    }
    bad = initial;
    bad.activity_counts.erase(bad.activity_counts.begin());
    reject(bad, "missing village activity count");
    bad = initial;
    bad.activity_counts.begin()->second = -1;
    reject(bad, "negative village activity count");
    bad = initial;
    bad.scripts.activities.begin()->second.status = -1;
    reject(bad, "negative village activity status");
    bad = initial;
    bad.events_held = std::numeric_limits<int>::max();
    reject(bad, "total village event count overflow");
    bad = initial;
    bad.human_activity_previous.erase(bad.human_activity_previous.begin());
    bad.human_activity_previous.emplace(99999, 0);
    reject(bad, "wrong human previous-activity identity");
    bad = initial;
    bad.item_commerce_read.erase(bad.item_commerce_read.begin());
    reject(bad, "missing item commerce reading state");
    bad = initial;
    bad.facility_commerce_read.erase(bad.facility_commerce_read.begin());
    bad.facility_commerce_read.emplace(99999, false);
    reject(bad, "wrong facility commerce reading identity");
    bad = initial;
    bad.facility_item_confirmations.erase(bad.facility_item_confirmations.begin());
    reject(bad, "missing live facility item counter");
    bad = initial;
    bad.facility_item_confirmations.begin()->second = -1;
    reject(bad, "negative facility item counter");
    bad = initial;
    bad.facility_item_confirmations.emplace(bad.next_facility_identity++, 0);
    reject(bad, "unrooted retired facility item counter");
    bad = initial;
    bad.build_anchor = ref::Position{5, 5};
    reject(bad, "unfinished editing anchor");
    check(!app::world_save_eligible(bad), "unfinished editing anchor cannot be captured");
    bad = initial;
    bad.build_moving_facility = bad.scene.world.facility_order.front();
    reject(bad, "unfinished moving selection");
    check(!app::world_save_eligible(bad), "moving selection cannot be captured");
    const auto main_capture = app::capture_world_save(initial);
    check(main_capture.image.has_value(), "stable baseline captures before dormant-tool cases");
    for (const int mode : {1, 3, 6}) {
        auto dormant = initial;
        dormant.build_mode = mode; // Published cancel leaves the last tool in scene0.
        const auto saved = app::capture_world_save(dormant);
        check(saved.image && saved.image->bytes == main_capture.image->bytes,
              "exited tool is transient and does not change durable main-scene bytes");
        dormant.scene.scene_state = 1;
        check(!app::world_save_eligible(dormant), "active tool scene remains ineligible");
    }
    for (const int mode : {-1, 8}) {
        bad = initial;
        bad.build_mode = mode;
        reject(bad, "invalid dormant editing tool");
        check(!app::world_save_eligible(bad), "invalid dormant tool cannot be captured");
    }
    bad = initial;
    bad.activity_page_answers.emplace(42, 0);
    reject(bad, "unconsumed village activity decision");
    check(!app::world_save_eligible(bad), "village activity answer cannot be discarded");
}

void magic_pot_roundtrip() {
    auto state = ark::test::initial_world();
    // Explicit stable-main callsite with pending processing and mixed discovered recipes.
    // The fixture does not claim a natural unlock; processing below uses the real consumer.
    state.scripts.user_flags |= 3U;
    state.scene.calendar.year = 2;
    state.scene.calendar.month = 4;
    state.scene.calendar.subperiod = 1;
    state.legacy_n = {35, 2, 1, 40, 30, 20, 10, 5, 4, 3, 2, 2, 113};
    for (auto &[id, progress] : state.magic_pot_recipes) {
        progress.status = id % 2;
        progress.pending_notice = id % 3 == 0;
    }
    const auto original = state;
    auto loaded = restored(state);
    check(loaded.magic_pot_recipes.size() == 40 && loaded.legacy_n == state.legacy_n &&
              loaded.scripts.user_flags == state.scripts.user_flags,
          "Schema3 retains all recipes and existing pot slots/unlock flags");
    for (const auto &[id, expected] : state.magic_pot_recipes) {
        const auto &actual = loaded.magic_pot_recipes.at(id);
        check(actual.identity == expected.identity && actual.status == expected.status &&
                  actual.pending_notice == expected.pending_notice,
              "Recipe identity, discovery and unread state survive player restore");
    }
    const auto clean = app::capture_world_save(state);
    check(clean.image.has_value(), clean.message);
    state.magic_pot_display[0][0] = 17;
    state.magic_pot_output = {1, 2, 3, 4};
    state.magic_pot_comment = "discarded presentation";
    state.magic_pot_pages_initialized.insert(999);
    state.magic_pot_page_data.emplace(999, std::array<int, 3>{1, 0, 7});
    state.magic_pot_page_lists.emplace(999, std::vector<int>{7});
    state.magic_pot_page_parents.emplace(999, 998);
    const auto transient = app::capture_world_save(state);
    check(transient.image && transient.image->bytes == clean.image->bytes,
          "Pot presentation and page payloads never enter player bytes");
    std::string reason;
    auto current = ark::test::initial_world();
    current.scene.framework_paused = true;
    current.scene.speed_setting = 0;
    current.scene.random = ref::WorldRandomStream::from_java_seed(987);
    auto cleaned = state;
    check(app::prepare_world_save_candidate(cleaned, current, reason) == app::WorldSaveError::none,
          reason);
    check(cleaned.magic_pot_pages_initialized.empty() && cleaned.magic_pot_page_data.empty() &&
              cleaned.magic_pot_page_lists.empty() && cleaned.magic_pot_page_parents.empty() &&
              cleaned.magic_pot_comment.empty() &&
              cleaned.magic_pot_output == std::array<std::int32_t, 4>{} &&
              cleaned.magic_pot_display == std::array<std::array<std::int32_t, 5>, 3>{} &&
              cleaned.scene.framework_paused && cleaned.scene.speed_setting == 0 &&
              cleaned.scene.random.snapshot().engine_state ==
                  current.scene.random.snapshot().engine_state,
          "Restore clears every pot transient and inherits current pause, speed and random");
    state = original;
    state.scene.calendar.subperiod = loaded.scene.calendar.subperiod = 2;
    const auto before = loaded.scene.random.draws();
    check(sim::open_startup_world_magic_pot(state, sim::StartupMagicPotEntry::main_menu) ==
                  sim::StartupWorldRuntimeError::none &&
              sim::open_startup_world_magic_pot(loaded, sim::StartupMagicPotEntry::main_menu) ==
                  sim::StartupWorldRuntimeError::none,
          "Pending processing resumes through the real menu consumer after loading");
    check(loaded.legacy_n == state.legacy_n && loaded.legacy_n[1] < 2 &&
              loaded.magic_pot_display == state.magic_pot_display &&
              loaded.magic_pot_output == state.magic_pot_output &&
              loaded.scripts.pages.size() == state.scripts.pages.size() &&
              loaded.scene.random.draws() == before,
          "Restored processing preserves results/page count without drawing random during load");
}
void facility_program_restore() {
    auto state = ark::test::initial_world();
    auto loaded = restored(state);
    for (const auto &definition : state.rules->facilities)
        check(loaded.scripts.facilities.at(definition.id).icon == definition.legacy_icon,
              "Player restore rebuilds opcode40 category from immutable source definition");
    // Start the actual bun-shop program before saving: its delayed opcode40 must survive
    // the player codec without adding UI payload or changing the player random policy.
    ref::WorldScriptInput program;
    program.event = 2033;
    const auto started = ref::prepare_world_script_program(
        sim::startup_world_runtime_catalog(), sim::startup_world_runtime_scripts(state), program);
    check(started.candidate &&
              sim::write_startup_world_runtime_scripts(state, started.candidate->state),
          "Actual facility33 program schedules its published delay");
    loaded = restored(state);
    discard_nonpersistent_effects(state);
    bool reached{};
    for (int n = 0; n < 140 && !reached; ++n) {
        advance(state);
        advance(loaded);
        reached = state.scripts.pages.back().legacy_page == 82 &&
                  state.scripts.pages.back().lifecycle != 4;
    }
    check(reached && loaded.scripts.pages.back().legacy_page == 82 &&
              loaded.scripts.pages.back().facility_definition == 33 &&
              loaded.scene.random.draws() == state.scene.random.draws() &&
              loaded.scene.world.world.ai.accounting.funds() ==
                  state.scene.world.world.ai.accounting.funds(),
          "Saved delayed facility program resumes to the same bound publicity82 without a stall");
}
void management_fields_roundtrip() {
    auto state = ark::test::initial_world();
    // A byte-coverage fixture for maintained records, not a claim of natural business execution.
    const auto activity = state.activity_counts.begin()->first;
    const auto human = state.human_activity_previous.begin()->first;
    const auto facility = state.scene.world.facility_order.front();
    const auto item = state.item_commerce_read.begin()->first;
    const auto facility_definition = state.facility_commerce_read.begin()->first;
    state.activity_counts.at(activity) = 7;
    state.human_activity_previous.at(human) = -3;
    state.facility_item_confirmations.at(facility) = 2;
    state.item_commerce_read.at(item) = !state.item_commerce_read.at(item);
    state.facility_commerce_read.at(facility_definition) =
        !state.facility_commerce_read.at(facility_definition);
    // Restock p/q/r is an old projection until the actual consumer refreshes it.
    auto &owned = state.items.at(item);
    ++owned.unlock_counter;
    state.catalog.at({0, item}) = owned;
    state.facility_item_response = 3;
    state.commerce_page_data[42] = {0, 1, 2, 0, item, 9};
    state.activity_page_display_humans[42] = {human, human};
    const auto loaded = restored(state);
    check(loaded.activity_counts == state.activity_counts &&
              loaded.human_activity_previous == state.human_activity_previous &&
              loaded.facility_item_confirmations == state.facility_item_confirmations &&
              loaded.item_commerce_read == state.item_commerce_read &&
              loaded.facility_commerce_read == state.facility_commerce_read,
          "new management durable tables round-trip with exact identities and values");
    check(loaded.items.at(item).unlock_counter == state.items.at(item).unlock_counter &&
              loaded.catalog.at({0, item}).unlock_counter == state.items.at(item).unlock_counter &&
              loaded.shop_item_stock.at(item).legacy_q == state.shop_item_stock.at(item).legacy_q,
          "coherent current item state preserves the independently timed restock projection");
    check(loaded.facility_item_response == 0 && loaded.commerce_page_data.empty() &&
              loaded.activity_page_display_humans.empty(),
          "business page payloads and last result response are transient");
    same_durable(state, loaded, "new management state capture");
}

void expanded_map_roundtrip() {
    auto state = ark::test::initial_world();
    const auto funds = state.scene.world.world.ai.accounting.funds();
    const auto draws = state.scene.random.draws();
    // Direct source map-consumer fixtures isolate persistence from activity eligibility.
    // The natural activity fee/quarter chain remains owned by the source runtime tests.
    for (int level = 1; level <= 2; ++level) {
        check(sim::expand_startup_world_map(state), "source map expansion prepares save fixture");
        check(state.fence_level == level && state.scene.world.world.map.cells.size() == 576 &&
                  state.scene.world.world.ai.accounting.funds() == funds &&
                  state.scene.random.draws() == draws,
              "source expansion keeps fixed map, funds and random stream");
        auto loaded = restored(state);
        same_durable(state, loaded, "expanded fixed-map capture");
        const auto &town = loaded.scene.world.town;
        check(loaded.fence_level == level && town.left == (level == 1 ? 3 : 1) &&
                  town.right == (level == 1 ? 20 : 22) && town.top == 2 &&
                  town.bottom == (level == 1 ? 11 : 12) &&
                  loaded.scene.world.world.map.width == 24 &&
                  loaded.scene.world.world.map.height == 24 &&
                  loaded.scene.world.spawn_cells == state.scene.world.spawn_cells &&
                  loaded.facility_original_ids == state.facility_original_ids &&
                  loaded.facility_ordinals == state.facility_ordinals &&
                  loaded.next_facility_identity == state.next_facility_identity,
              "expanded save keeps source bounds, entry identities and allocation state");
        auto baseline = state;
        discard_nonpersistent_effects(baseline);
        advance(loaded);
        advance(baseline);
        check(loaded.scene.world.world.ai.accounting.funds() ==
                      baseline.scene.world.world.ai.accounting.funds() &&
                  loaded.monthly_cash == baseline.monthly_cash &&
                  loaded.village_points == baseline.village_points &&
                  loaded.scene.random.draws() == baseline.scene.random.draws() &&
                  ark::test::same_world_clock(loaded, baseline),
              "expanded restore continues actual world without repeating fees or random draws");
    }
}

void mixed_surface_roundtrip() {
    auto state = ark::test::initial_world();
    const auto created =
        ref::prepare_world_task_creation(sim::startup_world_runtime_factory(state), 0);
    check(created.candidate && created.candidate->created_task &&
              sim::write_startup_world_runtime_factory(state, created.candidate->state),
          "source task factory creates mixed-surface facility with real auxiliary records");
    const auto task = *created.candidate->created_task;
    const auto id = *state.tasks.at(task).facility;
    auto &facility = state.scene.world.world.facilities.at(id);
    check(facility.kind == 1 && facility.placement.shape == ref::FacilityShape::single,
          "mixed-surface save fixture uses actual source cave definition");
    auto &map = state.scene.world.world.map;
    for (const auto position : state.sites.at(id).occupied_cells) {
        const auto index = static_cast<std::size_t>(position.y) * map.width + position.x;
        map.cells.at(index) = {4, ref::RouteCategory::ground, {}};
        state.surface.at(index).definition = state.ground_definition;
        state.surface.at(index).instance = state.surface.at(index).fragment = -1;
    }
    // Geometry-only fixture, matching the published source road-special regression.
    // Task generation does not naturally place caves inside the town; no rule/table is changed.
    const ref::Position position{7, 3};
    const auto index = static_cast<std::size_t>(position.y) * map.width + position.x;
    check(!map.cells.at(index).facility, "mixed-surface geometry never overwrites an instance");
    facility.placement.anchor = position;
    state.tasks.at(task).site = position;
    state.sites.at(id).occupied_cells = {position};
    map.cells.at(index) = {8, ref::RouteCategory::terminal,
                           ref::FacilityTileBinding{{id}, facility.placement.definition_id, 0}};
    state.surface.at(index).definition = facility.placement.definition_id;
    state.surface.at(index).variant = 0;
    state.surface.at(index).instance =
        3; // Source direction m is separate from the stable identity.
    const auto definition = facility.placement.definition_id;
    check(sim::refresh_startup_world_map(state, false), "source refresh validates cave fixture");
    const auto raw = state.facility_original_ids.at(id);
    const auto funds = state.scene.world.world.ai.accounting.funds();
    const auto draws = state.scene.random.draws();
    check(sim::begin_startup_world_road(state, 18).error == sim::StartupWorldRuntimeError::none &&
              sim::confirm_startup_world_edit(state, position, ref::FacilityOrientation::first)
                      .error == sim::StartupWorldRuntimeError::none &&
              sim::confirm_startup_world_edit(state, position, ref::FacilityOrientation::first)
                      .error == sim::StartupWorldRuntimeError::none &&
              sim::cancel_startup_world_edit(state) == sim::StartupWorldRuntimeError::none,
          "actual source road command preserves cave binding while changing surface");
    for (int phase = 0; phase < 2; ++phase) {
        if (phase == 1)
            check(sim::begin_startup_world_edit(state, false).error ==
                          sim::StartupWorldRuntimeError::none &&
                      sim::confirm_startup_world_edit(state, position,
                                                      ref::FacilityOrientation::first)
                              .error == sim::StartupWorldRuntimeError::none &&
                      sim::confirm_startup_world_edit(state, position,
                                                      ref::FacilityOrientation::first)
                              .error == sim::StartupWorldRuntimeError::none &&
                      sim::cancel_startup_world_edit(state) == sim::StartupWorldRuntimeError::none,
                  "actual source road removal retains the same cave instance");
        const auto &binding = state.scene.world.world.map.cells.at(index).facility;
        check(binding && binding->instance_id.value == id && binding->definition_id == definition &&
                  state.surface.at(index).definition ==
                      (phase == 0 ? 18 : state.ground_definition) &&
                  state.surface.at(index).instance == 3 &&
                  state.scene.world.world.ai.accounting.funds() == funds - 10 &&
                  state.scene.random.draws() == draws,
              "source mixed road/ground has exact binding, one charge and retained direction");
        auto loaded = restored(state);
        same_durable(state, loaded,
                     phase == 0 ? "road with cave capture" : "ground with cave capture");
        check(loaded.facility_original_ids.at(id) == raw &&
                  loaded.scene.world.world.map.cells.at(index).facility->instance_id.value == id &&
                  loaded.scene.world.world.map.cells.at(index).facility->definition_id ==
                      definition,
              "mixed surface restore keeps actual source instance and original identity");
        auto baseline = state;
        discard_nonpersistent_effects(baseline);
        advance(loaded);
        advance(baseline);
        check(loaded.scene.world.world.ai.accounting.funds() ==
                      baseline.scene.world.world.ai.accounting.funds() &&
                  loaded.monthly_cash == baseline.monthly_cash &&
                  loaded.scene.random.draws() == baseline.scene.random.draws() &&
                  ark::test::same_world_clock(loaded, baseline),
              "mixed surface restore resumes source world without repeating road charges");
        for (int fault = 0; fault < 5; ++fault) {
            auto broken = state;
            auto &cell = broken.scene.world.world.map.cells.at(index);
            if (fault == 0)
                cell.facility->instance_id.value = broken.next_facility_identity + 100;
            else if (fault == 1)
                cell.facility->definition_id = 18;
            else if (fault == 2)
                ++cell.facility->fragment_index;
            else if (fault == 3)
                broken.surface.at(index).definition =
                    30; // Known ordinary building, not road/ground.
            else
                cell.category = ref::RouteCategory::terminal; // Incorrect category for state3/4.
            const auto cash = broken.scene.world.world.ai.accounting.funds();
            const auto random = broken.scene.random.draws();
            const auto damaged_binding = *cell.facility;
            const auto damaged_surface = broken.surface.at(index).definition;
            const auto damaged_category = cell.category;
            std::string reason;
            check(app::prepare_world_save_candidate(broken, state, reason) ==
                      app::WorldSaveError::invalid_world,
                  "mixed-surface restore rejects damaged binding or unsupported combination " +
                      std::to_string(fault));
            check(broken.scene.world.world.ai.accounting.funds() == cash &&
                      broken.scene.random.draws() == random &&
                      broken.scene.world.world.map.cells.at(index).facility->instance_id ==
                          damaged_binding.instance_id &&
                      broken.scene.world.world.map.cells.at(index).facility->definition_id ==
                          damaged_binding.definition_id &&
                      broken.scene.world.world.map.cells.at(index).facility->fragment_index ==
                          damaged_binding.fragment_index &&
                      broken.surface.at(index).definition == damaged_surface &&
                      broken.scene.world.world.map.cells.at(index).category == damaged_category,
                  "mixed-surface rejection keeps candidate cash/random/identity untouched");
        }
    }
}

void task_and_combat_roundtrip(const State &natural) {
    auto task = natural;
    const auto generated =
        ref::prepare_world_task_creation(sim::startup_world_runtime_factory(task), 0);
    check(generated.candidate && generated.candidate->created_task &&
              sim::write_startup_world_runtime_factory(task, generated.candidate->state),
          "task fixture calls the actual source factory without injecting people or funds");
    const auto identity = *generated.candidate->created_task;
    check(sim::open_startup_world_runtime_task_menu(task) == sim::StartupWorldRuntimeError::none,
          "source task menu opens");
    const auto menu = task.scripts.pages.back().id;
    const auto &list = task.task_page_lists.at(menu);
    const auto selected = std::find(list.begin(), list.end(), identity);
    check(selected != list.end() && sim::act_startup_world_runtime_task_page(
                                        task, menu, sim::StartupWorldTaskAction::confirm,
                                        static_cast<int>(selected - list.begin()))
                                            .error == sim::StartupWorldRuntimeError::none,
          "source menu selects the generated task identity");
    const auto accepted = sim::act_startup_world_runtime_task_page(
        task, task.scripts.pages.back().id, sim::StartupWorldTaskAction::confirm);
    check(accepted.error == sim::StartupWorldRuntimeError::none && accepted.accepted,
          "source task offer consumes the real recruitment fee once");
    for (int n = 0; n < 1000 && !(task.active_task && app::world_save_eligible(task)); ++n) {
        advance(task);
        const auto page = task.scripts.pages.back();
        if (page.lifecycle != 2)
            continue;
        if (page.legacy_page == 25)
            check(sim::act_startup_world_runtime_task_page(task, page.id,
                                                           sim::StartupWorldTaskAction::depart)
                          .error == sim::StartupWorldRuntimeError::none,
                  "actual recruited team requests departure");
        else if (page.legacy_page == 28 && task.page_phases.at(page.id) == 0)
            check(sim::act_startup_world_runtime_task_page(task, page.id,
                                                           sim::StartupWorldTaskAction::confirm)
                          .error == sim::StartupWorldRuntimeError::none,
                  "actual departure animation is confirmed");
    }
    check(task.active_task == identity && app::world_save_eligible(task),
          "actual recruitment and departure reach a stable active task");
    auto loaded_task = restored(task);
    same_durable(task, loaded_task, "active task capture");
    auto baseline_task = task;
    discard_nonpersistent_effects(baseline_task);
    for (int n = 0; n < 20; ++n) {
        advance(loaded_task);
        advance(baseline_task);
    }
    check(loaded_task.active_task == baseline_task.active_task &&
              loaded_task.participants == baseline_task.participants &&
              loaded_task.scene.world.world.ai.accounting.funds() ==
                  baseline_task.scene.world.world.ai.accounting.funds() &&
              loaded_task.scene.random.draws() == baseline_task.scene.random.draws() &&
              ark::test::same_world_clock(loaded_task, baseline_task),
          "loaded active task does not repeat recruitment or lose actual crew");

    // Source encounter call-point fixture, matching the maintained runtime composition case.
    ref::EncounterCreationInput input;
    input.kind = 0;
    input.center = {10, 18};
    input.year_index = 0;
    input.month_index = 3;
    const auto created = sim::prepare_startup_world_runtime_encounter(natural, input);
    check(created && !created->scene.world.world.ai.monster_order.empty(),
          "actual encounter consumer creates real monster instances and caches");
    auto combat = *created;
    auto &ai = combat.scene.world.world.ai;
    const auto human = ai.human_order.front();
    const auto monster = ai.monster_order.back();
    const auto encounter = *ai.battle.actors.at(monster).encounter;
    const auto joined = ref::prepare_battle_group_join(ai, encounter, human, monster);
    check(joined.candidate.has_value(), "source battle group binds real opposing actors");
    ai = joined.candidate->state;
    const auto projectile = ref::prepare_projectile(ref::ProjectileKind::arrow, human, monster,
                                                    ai.battle.actors.at(human).position,
                                                    ai.battle.actors.at(monster).position, 0);
    check(projectile.candidate.has_value(),
          "source projectile constructor binds real caster and target");
    const auto projectile_id = ai.next_projectile_id++;
    ai.projectiles.emplace(projectile_id, *projectile.candidate);
    ai.projectile_order.push_back(projectile_id);
    check(sim::update_startup_world_render_cache(combat),
          "explicit encounter fixture includes the real transaction-tail render cache");
    auto loaded_combat = restored(combat);
    same_durable(combat, loaded_combat, "combat group and projectile capture");
    auto baseline_combat = combat;
    discard_nonpersistent_effects(baseline_combat);
    advance(loaded_combat);
    advance(baseline_combat);
    check(loaded_combat.scene.world.world.ai.projectile_order ==
                  baseline_combat.scene.world.world.ai.projectile_order &&
              loaded_combat.scene.world.world.ai.accounting.funds() ==
                  baseline_combat.scene.world.world.ai.accounting.funds() &&
              loaded_combat.scene.random.draws() == baseline_combat.scene.random.draws() &&
              ark::test::same_world_clock(loaded_combat, baseline_combat),
          "combat/projectile next update uses restored references without extra rewards");
    auto invalid = combat;
    invalid.scene.world.world.ai.encounters.at(encounter).group.cycle = 3;
    std::string reason;
    check(app::validate_world_save_candidate(invalid, reason) == app::WorldSaveError::invalid_world,
          "invalid battle group cycle rejected before next update");
    invalid = combat;
    invalid.actor_metadata.erase(monster);
    check(app::validate_world_save_candidate(invalid, reason) == app::WorldSaveError::invalid_world,
          "projectile target without required runtime cache rejected");
}
void natural_operation_roundtrip() {
    auto state = ark::test::initial_world();
    bool service{}, walking{}, construction{}, captured_service{}, continuation{};
    for (int update = 0; update < 1200; ++update) {
        advance(state);
        if (!app::world_save_eligible(state))
            continue;
        if (!continuation && !state.scripts.continuations.empty()) {
            const auto copy = restored(state);
            same_durable(state, copy, "natural delayed script capture");
            auto waiting = copy;
            auto uninterrupted = state;
            discard_nonpersistent_effects(uninterrupted);
            const auto event = state.scripts.continuations.front().event;
            const auto calls = state.scripts.event_calls.at(event);
            for (int remaining = state.scripts.continuations.front().remaining_updates + 2;
                 remaining > 0; --remaining) {
                advance(waiting);
                advance(uninterrupted);
            }
            check(waiting.scripts.event_calls.at(event) == calls &&
                      waiting.scripts.continuations.empty() &&
                      uninterrupted.scripts.continuations.empty() &&
                      waiting.scene.world.world.ai.accounting.funds() ==
                          uninterrupted.scene.world.world.ai.accounting.funds() &&
                      waiting.scene.random.draws() == uninterrupted.scene.random.draws() &&
                      ark::test::same_world_clock(waiting, uninterrupted),
                  "actual first-arrival delayed script resumes without a second event call");
            continuation = true;
        }
        if (!construction && !state.scene.world.world.ai.human_order.empty()) {
            const auto selected = sim::begin_startup_world_build(state, 30);
            check(selected.denial == sim::StartupBuildDenial::none, "real construction selection");
            bool built{};
            for (int y = 0; y < state.scene.world.world.map.height && !built; ++y)
                for (int x = 0; x < state.scene.world.world.map.width && !built; ++x) {
                    auto candidate = state;
                    const auto result = sim::confirm_startup_world_build(
                        candidate, {x, y}, ref::FacilityOrientation::first);
                    if (result.denial == sim::StartupBuildDenial::none && result.created) {
                        state = std::move(candidate);
                        sim::cancel_startup_world_build(state);
                        construction = true;
                        built = true;
                        const auto copy = restored(state);
                        same_durable(state, copy, "construction capture");
                        check(copy.scene.world.world.ai.accounting.funds() ==
                                  state.scene.world.world.ai.accounting.funds(),
                              "construction save does not charge again");
                        // 6061a2c separates temporary m.p from construction/business state.
                        // Ark's approved player policy deliberately retains p. This explicit
                        // nonempty fixture must not silently turn into the original load policy.
                        auto with_notice = state;
                        with_notice.facility_details.at(*result.created).notices = {{3, 9}};
                        auto loaded_notice = restored(with_notice);
                        check(loaded_notice.facility_details.at(*result.created).notices ==
                                  std::vector<std::array<int, 2>>{{3, 9}},
                              "player restore retains nonempty instance p under approved policy");
                        same_durable(with_notice, loaded_notice,
                                     "nonempty p and ongoing construction capture");
                        discard_nonpersistent_effects(with_notice);
                        advance(with_notice);
                        advance(loaded_notice);
                        same_durable(with_notice, loaded_notice,
                                     "nonempty p construction resumes without replaying payment");
                    }
                }
            check(built, "natural construction finds legal actual placement");
        }
        for (const auto id : state.scene.world.world.ai.human_order) {
            const auto &actor = state.scene.world.world.ai.battle.actors.at(id);
            walking |= actor.control.action == 1;
            if (actor.control.state != 14 || captured_service)
                continue;
            auto loaded = restored(state);
            same_durable(state, loaded, "inn service capture");
            const auto &before = actor.hp;
            const auto &after = loaded.scene.world.world.ai.battle.actors.at(id).hp;
            check(before.requested_delta == after.requested_delta &&
                      before.displayed == after.displayed && before.origin == after.origin &&
                      before.target == after.target && before.animating == after.animating &&
                      before.legacy_tick == after.legacy_tick,
                  "inn service keeps all six HP fields without recovery");
            check(loaded.scene.world.world.facilities.size() ==
                          state.scene.world.world.facilities.size() &&
                      loaded.scene.world.world.actors.at(id).binding->instance_id ==
                          state.scene.world.world.actors.at(id).binding->instance_id,
                  "inn binding and facilities round-trip");
            service = captured_service = true;
        }
    }
    check(walking && service && construction && continuation,
          "real walking, charged inn service, construction and delayed script observed");
    for (int n = 0; n < 600 && !app::world_save_eligible(state); ++n)
        advance(state);
    check(app::world_save_eligible(state),
          "stable source capture found within the diagnostic budget");
    task_and_combat_roundtrip(state);
    auto loaded = restored(state);
    auto baseline = state;
    std::string reason;
    // Clear only the published nonpersistent effects in the independent reference side.
    // Its durable fields and map caches do not go through the restoration implementation.
    discard_nonpersistent_effects(baseline);
    same_durable(loaded, baseline, "natural restore");
    for (int n = 0; n < 80; ++n) {
        advance(loaded);
        advance(baseline);
    }
    if (app::world_save_eligible(loaded) && app::world_save_eligible(baseline))
        same_durable(loaded, baseline, "natural continuation");
    else
        check(loaded.scene.world.world.ai.accounting.funds() ==
                      baseline.scene.world.world.ai.accounting.funds() &&
                  ark::test::same_world_clock(loaded, baseline) &&
                  loaded.scene.random.draws() == baseline.scene.random.draws(),
              "natural continuation retains source modal boundary and financial outcome");
    auto departing = state;
    const auto id = departing.scene.world.world.ai.human_order.front();
    departing.scene.world.world.ai.battle.actors.at(id).control.flags |= 512U;
    check(!app::world_save_eligible(departing), "unsupported exit state is explicitly ineligible");
    auto bad = state;
    bad.scene.world.world.ai.battle.actors.at(id).rescue = ref::CharacterId{999999};
    check(app::validate_world_save_candidate(bad, reason) == app::WorldSaveError::invalid_world,
          "dangling real actor rescue reference rejected");
    bad = state;
    bad.scene.world.world.ai.battle.actors.at(id).control.queue.push_back({14});
    bad.scene.world.world.ai.battle.actors.at(id).control.queue.push_back({99999});
    check(app::validate_world_save_candidate(bad, reason) == app::WorldSaveError::invalid_world,
          "unknown control opcode rejected without executing the valid prefix");
    auto month_boundary = state;
    // Clock-only boundary fixture: all people, money, services and construction remain real.
    month_boundary.scene.calendar.subperiod = 3;
    month_boundary.scene.calendar.units = 10773;
    auto month_loaded = restored(month_boundary);
    auto month_baseline = month_boundary;
    discard_nonpersistent_effects(month_baseline);
    const auto previous_month = month_boundary.scene.calendar.month;
    advance(month_loaded);
    advance(month_baseline);
    check(month_loaded.scene.calendar.month == (previous_month + 1) % 12 &&
              month_loaded.scene.world.world.ai.accounting.funds() ==
                  month_baseline.scene.world.world.ai.accounting.funds() &&
              month_loaded.monthly_cash == month_baseline.monthly_cash &&
              month_loaded.village_points == month_baseline.village_points &&
              month_loaded.scene.random.draws() == month_baseline.scene.random.draws() &&
              ark::test::same_world_clock(month_loaded, month_baseline),
          "manual restore across month does not replay world prefix, fees or points");
}
} // namespace

void run_restore_tests() {
    invalid_candidates();
    magic_pot_roundtrip();
    facility_program_restore();
    management_fields_roundtrip();
    expanded_map_roundtrip();
    mixed_surface_roundtrip();
    natural_operation_roundtrip();
    std::cout << "PASS world save restore " << checks << " checks\n";
}
