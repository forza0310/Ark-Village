#include "ark/simulation/world/startup_world_runtime.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include "ark/simulation/presentation/startup_world_visuals.hpp"

#include <iostream>
#include <algorithm>
#include <stdexcept>

using namespace ark::simulation;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
void main_character_profile() {
    StartupSession startup;
    StartupWorldRuntimeSession session(startup.state(), ref::WorldRandomStream::from_java_seed(12345));
    auto s = session.state();
    const auto base0 = startup_world_human_profile(s, 0);
    const auto base1 = startup_world_human_profile(s, 1);
    check(base0 && base1 && s.human_profiles.empty(), "default world resolves immutable profiles without override");
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto presence = s.human_presence;
    check(install_startup_world_main_character(s, {"研究姓名", 1, true}),
          "profile installs only actual fresh private world candidate");
    const auto value = startup_world_human_profile(s, 0);
    const auto other = startup_world_human_profile(s, 1);
    const auto details = startup_world_human_details(s, 0);
    const auto portrait = startup_world_portrait(s, 0);
    const int job = s.scene.world.world.ai.growth.at(0).definition.current_profession;
    check(value && value->name == "研究姓名" && value->sex == 1 && value->custom_name &&
              details && details->name == value->name && details->sex == 1 &&
              portrait && portrait->image == s.rules->jobs.at(job).sprites[1] &&
              startup_world_runtime_scripts(s).humans.at(0).name == "研究姓名",
          "definition zero name/sex agree in details portrait and dialogue projection without instance");
    check(other && other->name == base1->name && other->sex == base1->sex &&
              s.rules->humans.at(0).name == base0->name && s.rules->humans.at(0).sex == base0->sex &&
              s.scene.world.world.ai.battle.actors.empty() && s.scene.random.draws() == 0 &&
              s.scene.world.world.ai.accounting.funds() == cash && s.human_presence == presence,
          "profile preserves definition1 frozen table real arrival ordering funds and random");
    auto directory = s; // 页61与开放条件夹具；没有到访、资金或属性注入。
    const auto male = std::find_if(s.rules->jobs.begin(), s.rules->jobs.end(),
                                   [](const auto &j) { return j.script_extra == 0; });
    const auto female = std::find_if(s.rules->jobs.begin(), s.rules->jobs.end(),
                                     [](const auto &j) { return j.script_extra == 1; });
    check(male != s.rules->jobs.end() && female != s.rules->jobs.end(), "fixed profession catalogue has both sex restrictions");
    const int male_id = static_cast<int>(male - s.rules->jobs.begin());
    const int female_id = static_cast<int>(female - s.rules->jobs.begin());
    directory.scripts.professions.at(male_id).status = 1;
    directory.scripts.professions.at(female_id).status = 1;
    ref::WorldScriptPage page;
    page.id = directory.scripts.next_page_id++;
    page.kind = ref::WorldScriptPageKind::raw_page;
    page.legacy_page = 61;
    directory.scripts.pages.front().lifecycle = 3;
    directory.scripts.pages.push_back(page);
    directory.page_human_bindings[page.id] = 0;
    check(initialize_startup_world_human_pages(directory), "definition0 profile catalogue real initializer");
    const auto &list = directory.human_page_catalogs.at(page.id);
    check(std::find(list.begin(), list.end(), female_id) != list.end() &&
              std::find(list.begin(), list.end(), male_id) == list.end() &&
              directory.scene.world.world.ai.battle.actors.empty(),
          "female main profile uses actual profession filter without fabricating arrival");
    check(!install_startup_world_main_character(s, {"重复", 0, true}) &&
              startup_world_human_profile(s, 0)->name == "研究姓名",
          "duplicate profile installation refuses without overwriting private candidate");
    auto invalid = session.state();
    check(!install_startup_world_main_character(invalid, {"非法", 2, false}) &&
              invalid.human_profiles.empty() && invalid.scripts.humans.at(0).name == base0->name,
          "invalid sex refuses before any shared name mutation");
    invalid.simulation_steps = 1; // 运行中世界资格边界夹具，不推进/注入原游戏日期。
    check(!install_startup_world_main_character(invalid, {"运行中", 0, true}) && invalid.human_profiles.empty(),
          "running world cannot receive title editing");
    for (const auto &name : {std::string("\xc0\xaf", 2), std::string("a\0b", 3),
                             std::string("\xed\xa0\x80", 3), std::string("\n"), std::string(4097, 'x')})
        check(!valid_startup_world_human_profile({name, 0, true}), "malformed UTF8/control/budget profile rejects");
    check(valid_startup_world_human_profile({"", 0, true}) &&
              valid_startup_world_human_profile({"A中\xf0\x9f\x8c\x9f", 1, true}),
          "empty original-unclosed input and valid multibyte profile remain distinct from malformed text");
    invalid = s;
    invalid.human_profiles.emplace(1, StartupWorldHumanProfile{"越权", 0, true});
    check(!valid_startup_world_human_profiles(invalid) && !startup_world_human_profile(invalid, 0) &&
              !startup_world_human_details(invalid, 0) && !startup_world_portrait(invalid, 0),
          "unsupported stable definition override explicitly rejects all profile consumers");
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
    // 独立原新局oracle：n.c/3355–3362对这五条flags1调用a.i.a，保留首次NEW。
    const std::set<int> known_recipes{14, 22, 24, 30, 31};
    check(s.magic_pot_recipes.size() == 40 && s.legacy_n[11] == 1 &&
              s.magic_pot_comment.empty() && s.magic_pot_output == std::array<std::int32_t, 4>{},
          "actual new game installs forty recipes and source pot level/display initial values");
    for (const auto &[id, progress] : s.magic_pot_recipes)
        check(progress.identity == id && progress.status == (known_recipes.count(id) ? 1 : 0) &&
                  progress.pending_notice == (known_recipes.count(id) != 0),
              "source new game recipe bit1 grants known progress without clearing its NEW flag");
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
        main_character_profile();
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
