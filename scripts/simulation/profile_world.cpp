// Opt-in measurement of the real frozen runtime; no pacing or rule overrides.
// Build instructions and measurement scope are in profile_world.md.
#ifdef ARK_PROFILE_INSTRUMENTED
#include "profile_prepare.hpp"
#else
#include "ark/simulation/startup_world_runtime.hpp"
#endif
#include "ark/app/world_save.hpp"
#include "ark/app/world_session.hpp"
#ifdef ARK_PROFILE_COMPARE
#include <tuple>
#endif

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace sim = ark::simulation;
namespace rules = sim::rules;
using Clock = std::chrono::steady_clock;
using State = sim::StartupWorldRuntimeState;
static const rules::WorldScriptPage &top_page(const State &state) {
    const auto found = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                    [](const auto &p) { return p.lifecycle != 4; });
    if (found == state.scripts.pages.rend())
        throw std::runtime_error("missing page");
    return *found;
}
// Explicit player input policy derived from the frozen natural_progression scenario.
// No balance, map, population or task injection: every action uses the real consumer.
static bool build_bakery(State &state, int frame) {
    if (frame < 2000 || top_page(state).kind != rules::WorldScriptPageKind::scene ||
        state.scene.scene_state != 0)
        return false;
    const auto quote = sim::startup_world_build_quote(state, 35);
    if (!quote || state.scene.world.world.ai.accounting.funds() < quote->construction_cost)
        return false;
    const auto require = [](sim::StartupWorldRuntimeError error) {
        if (error != sim::StartupWorldRuntimeError::none)
            throw std::runtime_error("bakery input rejected");
    };
    require(sim::open_startup_world_build_menu(state));
    const auto selected = sim::select_startup_world_build_menu(state, top_page(state).id, 35);
    require(selected.error);
    if (selected.denial != sim::StartupBuildDenial::none)
        throw std::runtime_error("bakery unavailable");
    const auto &map = state.scene.world.world.map;
    std::vector<rules::Position> cells;
    for (int y = 0; y < map.height; ++y)
        for (int x = 0; x < map.width; ++x) {
            const auto &cell = map.cells.at(y * map.width + x);
            if (!cell.facility && cell.legacy_state == 4)
                cells.push_back({x, y});
        }
    const auto distance = [&](rules::Position p) {
        int nearest = map.width + map.height;
        for (int y = 0; y < map.height; ++y)
            for (int x = 0; x < map.width; ++x)
                if (map.cells.at(y * map.width + x).legacy_state == 3)
                    nearest = std::min(nearest, std::abs(p.x - x) + std::abs(p.y - y));
        return nearest;
    };
    std::stable_sort(cells.begin(), cells.end(),
                     [&](auto a, auto b) { return distance(a) < distance(b); });
    // The map reference is used only before consumers may replace the Owner.
    for (const auto cell : cells) {
        const auto result =
            sim::confirm_startup_world_build(state, cell, rules::FacilityOrientation::first);
        require(result.error);
        if (result.created) {
            std::cerr << "bakery frame=" << frame << " id=" << *result.created << " cell=" << cell.x
                      << ',' << cell.y << '\n';
            require(sim::cancel_startup_world_build(state));
            return true;
        }
    }
    throw std::runtime_error("no legal bakery placement");
}

static std::uint64_t durable_hash(const State &state) {
    const auto save = ark::app::capture_world_save(state);
    if (save.error == ark::app::WorldSaveError::ineligible)
        return 0; // No fabricated save eligibility.
    if (save.error != ark::app::WorldSaveError::none || !save.image)
        throw std::runtime_error("durable capture failed: error=" +
                                 std::to_string(static_cast<int>(save.error)) + " " + save.message);
    std::uint64_t hash = 14695981039346656037ULL;
    for (const auto byte : save.image->bytes) {
        hash ^= static_cast<unsigned char>(byte);
        hash *= 1099511628211ULL;
    }
    return hash;
}
static double elapsed(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}
#ifdef ARK_PROFILE_COMPARE
static void compare_state(const State &left, const State &right, int frame) {
    const auto fail = [frame] {
        throw std::runtime_error("equivalence failure frame " + std::to_string(frame));
    };
    const auto clock = [](const State &s) {
        const auto &d = s.scene.calendar;
        return std::make_tuple(d.year, d.month, d.subperiod, d.units, d.previous_units,
                               d.month_ticks, s.simulation_steps, s.scene.world.updates,
                               s.scene.frame_counter, s.scene.scene_counter, s.scene.random.draws(),
                               s.scene.world.world.ai.accounting.funds());
    };
    if (clock(left) != clock(right) || left.scripts.pages.size() != right.scripts.pages.size() ||
        left.sound_requests != right.sound_requests || left.page_counters != right.page_counters ||
        left.page_phases != right.page_phases || left.catalog.size() != right.catalog.size())
        fail();
    auto lr = left.scene.random, rr = right.scene.random;
    for (int i = 0; i < 8; ++i)
        if (lr.draw(97).raw != rr.draw(97).raw)
            fail();
    const auto page = [](const auto &p) {
        return std::tie(p.id, p.lifecycle, p.kind, p.legacy_page, p.source_record, p.replacement,
                        p.title, p.paragraphs, p.speaker_kind, p.speaker_definition, p.legacy_tag,
                        p.legacy_r, p.legacy_s, p.legacy_t, p.legacy_f, p.legacy_g, p.legacy_l,
                        p.message_commands, p.task_identity, p.task_definition,
                        p.monster_definition);
    };
    for (std::size_t i = 0; i < left.scripts.pages.size(); ++i)
        if (page(left.scripts.pages[i]) != page(right.scripts.pages[i]))
            fail();
    const auto ls = ark::app::capture_world_save(left), rs = ark::app::capture_world_save(right);
    if (ls.error != rs.error || ls.image.has_value() != rs.image.has_value())
        fail();
    if (ls.image && ls.image->bytes != rs.image->bytes)
        fail();
}
#endif
int main(int argc, char **argv) {
    try {
        const int frames = argc > 1 ? std::stoi(argv[1]) : 2000;
        const std::string policy = argc > 2 ? argv[2] : "passive";
        if (argc > 3 || (policy != "passive" && policy != "bakery"))
            throw std::invalid_argument("usage: profile_world [frames [passive|bakery]]");
        if (frames < 1 || frames > 100000)
            throw std::invalid_argument("frames must be 1..100000");
        auto state = [] {
            sim::StartupSession initial;
            sim::StartupWorldRuntimeSession session(initial.state(),
                                                    rules::WorldRandomStream::from_java_seed(1));
            return session.state();
        }();
        std::vector<std::shared_ptr<const State>> checkpoints;
        double prepare_ms{}, render_ms{}, commit_ms{}, copy_ms{}, adapter_ms{};
        std::uint64_t copy_sink{};
        int confirmations{};
        bool bakery{};
        const auto started = Clock::now();
        std::cout << "policy seed=1 speed=0 viewport=240x320 calendar=27 pacing=unpaced "
                     "render_cache=every_frame commit=copy auto_confirm=ordinary_only inputs="
                  << policy << '\n';
        std::cout << "frames,rounds,wall_ms,prepare_ms,render_ms,commit_copy_ms,probe_copy_ms,"
                     "probe_adapter_ms,humans,monsters,retired,ledger_entries,catalog,"
                     "checkpoints,random_draws,cash,year,month,units,confirmations,facilities,"
                     "tasks,task_order,probe_snapshot_ms,durable_hash,future_random_hash\n";
        for (int frame = 1; frame <= frames; ++frame) {
            auto begin = Clock::now();
#ifdef ARK_PROFILE_INSTRUMENTED
            auto result = sim::profile_prepare(state);
#else
            auto result = sim::prepare_startup_world_runtime(state);
#endif
            prepare_ms += elapsed(begin);
            if (!result.candidate || result.error != sim::StartupWorldRuntimeError::none)
                throw std::runtime_error("prepare failed at frame " + std::to_string(frame));
            begin = Clock::now();
            if (!sim::update_startup_world_render_cache(*result.candidate))
                throw std::runtime_error("render cache failed");
            render_ms += elapsed(begin);
#ifdef ARK_PROFILE_COMPARE
            auto reference = sim::prepare_startup_world_runtime(state);
            if (reference.error != result.error || reference.scene_error != result.scene_error ||
                reference.world_error != result.world_error || !reference.candidate ||
                reference.checkpoints.size() != result.checkpoints.size() ||
                !sim::update_startup_world_render_cache(*reference.candidate))
                throw std::runtime_error("reference prepare failed");
            compare_state(*result.candidate, *reference.candidate, frame);
            for (std::size_t i = 0; i < result.checkpoints.size(); ++i)
                compare_state(*result.checkpoints[i], *reference.checkpoints[i], frame);
#endif
            begin = Clock::now();
            state = *result.candidate; // Baseline session's exact public return contract.
            commit_ms += elapsed(begin);
            checkpoints.insert(checkpoints.end(), result.checkpoints.begin(),
                               result.checkpoints.end());
            state.sound_requests.clear(); // Explicit silent output consumer, as in source CLI.
            const auto page = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                           [](const auto &p) { return p.lifecycle != 4; });
            if (page == state.scripts.pages.rend())
                throw std::runtime_error("missing page");
            const bool raw = page->kind == rules::WorldScriptPageKind::raw_page;
            const int id = page->legacy_page;
            const bool automatic =
                raw && (id == 16 || id == 56 || id == 57 || id == 97 || id == 98);
            const bool decision =
                raw && ((id >= 22 && id <= 28) || id == 33 || id == 83 || (id >= 60 && id <= 66) ||
                        id == 68 || id == 70 || id == 73 || id == 90 || id == 87);
            if (!automatic && !decision && page->kind != rules::WorldScriptPageKind::scene) {
                if (sim::acknowledge_startup_world_runtime_page(state, page->id) !=
                    sim::StartupWorldRuntimeError::none)
                    throw std::runtime_error("confirmation failed");
                ++confirmations;
            }
            state.sound_requests.clear();
            if (policy == "bakery" && !bakery)
                bakery = build_bakery(state, frame);
            if (frame % 250 == 0 || frame == frames) {
                begin = Clock::now();
                for (int repeat = 0; repeat < 20; ++repeat) {
                    auto copy = state;
                    copy_sink += copy.scene.random.draws() + copy.catalog.size();
                }
                copy_ms = elapsed(begin) / 20;
                begin = Clock::now();
                for (int repeat = 0; repeat < 20; ++repeat) {
                    auto adapter = sim::startup_world_runtime_adapter();
                    copy_sink += adapter.catalog.events.size();
                }
                adapter_ms = elapsed(begin) / 20;
                double snapshot_ms{};
                for (int repeat = 0; repeat < 20; ++repeat) {
                    auto copy = state; // Preparation deliberately outside publication timing.
                    begin = Clock::now();
                    auto published = std::make_shared<ark::app::WorldFrame>();
                    published->state = std::make_shared<const State>(std::move(copy));
                    published->previous = published->state;
                    snapshot_ms += elapsed(begin);
                    copy_sink += published->state->scene.random.draws();
                }
                auto random = state.scene.random;
                std::uint64_t future_hash{};
                for (int draw = 0; draw < 8; ++draw)
                    future_hash =
                        future_hash * 31 + static_cast<std::uint64_t>(random.draw(97).raw);
                const auto &ai = state.scene.world.world.ai;
                const auto &date = state.scene.calendar;
                std::cout << std::fixed << std::setprecision(3) << frame << ','
                          << state.simulation_steps << ',' << elapsed(started) << ',' << prepare_ms
                          << ',' << render_ms << ',' << commit_ms << ',' << copy_ms << ','
                          << adapter_ms << ',' << ai.human_order.size() << ','
                          << ai.monster_order.size() << ',' << ai.retired_actors.size() << ','
                          << ai.accounting.entries().size() << ',' << state.catalog.size() << ','
                          << checkpoints.size() << ',' << state.scene.random.draws() << ','
                          << ai.accounting.funds() << ',' << date.year << ',' << date.month << ','
                          << date.units << ',' << confirmations << ','
                          << state.scene.world.facility_order.size() << ',' << state.tasks.size()
                          << ',' << state.task_order.size() << ',' << snapshot_ms / 20 << ','
                          << durable_hash(state) << ',' << future_hash << std::endl;
#ifdef ARK_PROFILE_INSTRUMENTED
                profile_report(frame);
#endif
                prepare_ms = render_ms = commit_ms = 0;
            }
        }
        std::cerr << "copy_sink=" << copy_sink << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
