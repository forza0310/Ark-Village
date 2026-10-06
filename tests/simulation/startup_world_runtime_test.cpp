#include "ark/simulation/startup_world_runtime.hpp"

#include <iostream>
#include <stdexcept>

using namespace ark::simulation;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
void initial_owner() {
    StartupSession reset;
    StartupWorldRuntimeSession runtime(reset.state(),
                                       ref::WorldRandomStream::from_java_seed(12345));
    const auto &s = runtime.state();
    check(s.rules && s.scene.world.world.ai.battle.actors.empty() &&
              s.scene.world.facility_order.size() == 8,
          "actual empty startup owns one world without fabricated actor");
    check(s.scene.world.world.ai.accounting.funds() == 5000 && s.village_points == 10 &&
              s.popularity == 50 && s.scene.calendar.month == 3 && s.arrival_counter == 420,
          "actual cash, points, popularity, calendar and arrival fields");
    check(!s.scripts.finance && s.scripts.human_order.empty() &&
              s.scripts.popularity_queue.empty() && s.scripts.pending_completion == 0 &&
              s.scene.random.draws() == 0,
          "persistent script metadata is not a second cash, I, actor or random owner");
    check(s.human_calendar.size() == 25 && s.facility_definitions.size() == 85 &&
              s.scripts.professions.size() == 23 && s.shop_item_stock.size() == 36 &&
              s.task_progress.definitions.size() == 81 && s.yearly_statistics.size() == 30 &&
              s.base_variants.size() == 576,
          "full original definitions and zero-allocated calendar arrays present");
    const auto script = startup_world_runtime_scripts(s);
    check(script.finance && script.finance->cash == s.scene.world.world.ai.accounting.funds() &&
              script.finance->legacy_flags14 == 0 &&
              script.finance->localized_gold_template == "G" && s.cash_peak == 0 &&
              s.cash_peak_village == "没有记录",
          "call-point finance reads current ledger and fresh system source defaults");
    const auto route = startup_world_runtime_routes(s);
    check(route.world.ai.growth.size() == 25 && route.world.facility_uses.size() == 85 &&
              route.world.map.cells.size() == 576 && route.random.draws() == 0,
          "routes are disposable canonical projections, not stored second worlds");
    const auto adapter = startup_world_runtime_adapter();
    check(adapter.scene && adapter.scripts && adapter.report && adapter.maintenance &&
              adapter.tasks && adapter.factory && adapter.entry && adapter.arrival &&
              adapter.calendar_other && adapter.calendar_request && adapter.actors.owned_command,
          "actual typed world, calendar, arrival and current-FIFO providers registered");
}
void pause_and_private_failure() {
    StartupSession reset;
    StartupWorldRuntimeSession runtime(reset.state(),
                                       ref::WorldRandomStream::from_java_seed(12345));
    runtime.set_paused(true);
    const auto paused = runtime.update();
    check(paused.error == StartupWorldRuntimeError::none &&
              runtime.state().arrival_counter == 420 && runtime.state().scene.calendar.units == 0 &&
              runtime.state().scene.random.draws() == 0 && runtime.state().simulation_steps == 0 &&
              runtime.checkpoints().empty(),
          "framework pause consumes neither world/date/random nor memory checkpoints");
    runtime.set_paused(false);
    auto impossible = runtime.state();
    impossible.scene.scene_state = 2; // 显式focus夹具走共同世界，避开未见自动脚本7的L159。
    impossible.scene.world.map_flags.pop_back();
    const auto rejected = prepare_startup_world_runtime(impossible);
    check(rejected.error != StartupWorldRuntimeError::none && !rejected.candidate &&
              rejected.checkpoints.empty() && impossible.arrival_counter == 420 &&
              impossible.scene.random.draws() == 0 && impossible.scene.calendar.units == 0,
          "late invalid canonical fact gives no partially committed actor/date/random/checkpoint");
}
void synchronous_event_seen() {
    StartupSession reset;
    StartupWorldRuntimeSession runtime(reset.state(), ref::WorldRandomStream::from_java_seed(1));
    auto s = runtime.state();
    const auto executed = ref::prepare_world_script(startup_world_runtime_catalog(),
                                                    startup_world_runtime_scripts(s), {90, {}, {}});
    check(executed.candidate && write_startup_world_runtime_scripts(s, executed.candidate->state) &&
              s.scripts.event_calls.at(90) == 1 && s.scene.world.world.ai.battle.events.count(90),
          "real event90 synchronizes aM-derived battle seen before same-call daily8 guard");
    auto projected = startup_world_runtime_finish(s);
    projected.event_calls[90] = 0;
    projected.event_calls[116] = 2;
    check(write_startup_world_runtime_finish(s, projected) &&
              !s.scene.world.world.ai.battle.events.count(90) &&
              s.scene.world.world.ai.battle.events.count(116) && s.scripts.event_calls.at(116) == 2,
          "finish writer regenerates seen from counts without a second persistent event owner");
}
void successful_result_retains_independent_owner() {
    StartupSession reset;
    StartupWorldRuntimeSession runtime(reset.state(), ref::WorldRandomStream::from_java_seed(1));
    auto result = runtime.update();
    check(result.error == StartupWorldRuntimeError::none && result.candidate,
          "successful session update returns its complete candidate after commit");
    auto &candidate = *result.candidate;
    const auto &committed = runtime.state();
    check(candidate.catalog.size() == 149 && candidate.catalog.size() == committed.catalog.size() &&
              candidate.scripts.pages.size() == committed.scripts.pages.size() &&
              candidate.scene.world.world.map.cells.size() == 576 &&
              candidate.scene.calendar.units == committed.scene.calendar.units &&
              candidate.simulation_steps == committed.simulation_steps,
          "moving private intermediates retains returned catalogs, world, pages and date");
    auto returned_random = candidate.scene.random;
    auto committed_random = committed.scene.random;
    for (int i = 0; i < 8; ++i)
        check(returned_random.draw(97).raw == committed_random.draw(97).raw,
              "returned and committed random engines retain the same future stream");
    const auto pages = committed.scripts.pages.size();
    candidate.catalog.clear();
    candidate.scripts.pages.clear();
    candidate.scene.world.world.map.cells.clear();
    check(committed.catalog.size() == 149 && committed.scripts.pages.size() == pages &&
              committed.scene.world.world.map.cells.size() == 576,
          "returned candidate remains independently mutable without changing committed owner");
}
} // namespace
int main() {
    try {
        initial_owner();
        pause_and_private_failure();
        synchronous_event_seen();
        successful_result_retains_independent_owner();
        std::cout << "startup_world_runtime: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "startup_world_runtime: " << error.what() << '\n';
        return 1;
    }
}
