// Manual window acceptance from a real seed1 game. The driver supplies only lawful player
// construction/confirmation inputs; it never inserts actors, cash, unlocks or effect records.
#include "ark/app/save/world_save_files.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"
#include "ark/simulation/presentation/startup_world_visuals.hpp"
#include "../../src/desktop/resources/resources.hpp"
#include "../support/world_fixture.hpp"
#include "../../src/desktop/scene/world_canvas.hpp"
#include "../../src/desktop/scene/world_scene.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace rules = sim::rules;
using State = sim::StartupWorldRuntimeState;
void require(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error("Natural business acceptance: " + message);
}
const rules::WorldScriptPage &top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                [](const auto &page) { return page.lifecycle != 4; });
    require(p != s.scripts.pages.rend(), "missing source page");
    return *p;
}
void advance(State &s) {
    auto next = sim::prepare_startup_world_runtime(s);
    require(next.candidate.has_value(), "real world update rejected");
    s = std::move(*next.candidate);
    require(sim::update_startup_world_render_cache(s), "real render-cache update rejected");
    s.sound_requests.clear(); // Same output consumption as the current desktop adapter.
}
void confirm(State &s) {
    const auto page = top(s); // Input may retire/reallocate the source page.
    const int raw = page.legacy_page;
    if (page.kind == rules::WorldScriptPageKind::dialogue ||
        page.kind == rules::WorldScriptPageKind::simple_message ||
        page.kind == rules::WorldScriptPageKind::newspaper ||
        (page.kind == rules::WorldScriptPageKind::raw_page &&
         (raw == 11 || raw == 49 || raw == 59 || raw == 67 || raw == 88 || raw == 96))) {
        const auto result = sim::acknowledge_startup_world_runtime_page(s, page.id);
        require(result == sim::StartupWorldRuntimeError::none ||
                    result == sim::StartupWorldRuntimeError::invalid_page,
                "source event confirmation failed raw=" + std::to_string(raw));
    }
}
std::uint64_t build(State &s, int definition, rules::Position at) {
    const auto quote = sim::startup_world_build_quote(s, definition);
    require(quote.has_value(), "missing initial construction quote");
    const auto funds = s.scene.world.world.ai.accounting.funds();
    const auto draws = s.scene.random.draws();
    require(sim::open_startup_world_build_menu(s) == sim::StartupWorldRuntimeError::none,
            "open real construction menu");
    const auto selected = sim::select_startup_world_build_menu(s, top(s).id, definition);
    require(selected.error == sim::StartupWorldRuntimeError::none &&
                selected.denial == sim::StartupBuildDenial::none,
            "select actual unlocked construction");
    const auto result = sim::confirm_startup_world_build(s, at, rules::FacilityOrientation::first);
    require(result.created.has_value() && result.denial == sim::StartupBuildDenial::none &&
                s.scene.world.world.ai.accounting.funds() == funds - quote->construction_cost &&
                s.scene.random.draws() == draws,
            "construction charges its exact quote once without random consumption");
    require(sim::cancel_startup_world_build(s) == sim::StartupWorldRuntimeError::none,
            "leave continuous placement through source cancel");
    std::cout << "BUILD definition=" << definition << " instance=" << *result.created
              << " x=" << at.x << " y=" << at.y
              << " orientation=first cost=" << quote->construction_cost << '\n';
    return *result.created;
}
struct Lift {
    rules::CharacterId actor;
    int human{}, kind{}, equipment{}, slot{}, step{};
    bool shown{}, committed{};
};
bool pending_commit(const State &s, const Lift &lift) {
    const auto &queue = s.scene.world.world.ai.battle.actors.at(lift.actor).control.queue;
    return std::any_of(queue.begin(), queue.end(), [&](const auto &c) {
        return lift.kind == 15
                   ? c.size() >= 2 && c[0] == 28 && c[1] == lift.equipment
                   : c.size() >= 3 && c[0] == 30 && c[1] == lift.slot && c[2] == lift.equipment;
    });
}
} // namespace

static void run_business_fixture(bool cold_restart) {
    const auto output = std::filesystem::path(ARK_TEST_OUTPUT) / "natural-business";
    std::filesystem::create_directories(output);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1080, 720, "Ark-Village real new-world business acceptance");
    require(IsWindowReady(), "cannot create acceptance window");
    struct Window {
        ~Window() { CloseWindow(); }
    } window;
    desktop::Sprites sprites(ARK_TEST_ASSETS);
    desktop::Text text(ARK_TEST_FONT);
    desktop::WorldCanvas canvas;
    constexpr desktop::Extent extent{384, 256};
    auto state = initial_world(cold_restart ? 20261008 : 1);
    state.reference_viewport = desktop::world_viewport(extent, 1);
    std::cout << std::unitbuf << "NATURAL seed=" << (cold_restart ? 20261008 : 1) << " viewport=";
    for (int value : state.reference_viewport)
        std::cout << value << ',';
    std::cout << " confirmations=dialogue/simple/newspaper/raw11,49,59,67,88,96"
                 " no-task-recruitment no-unlock-injection\n";
    const auto capture = [&](const std::string &name, std::array<float, 2> focus) {
        const auto before = state;
        const desktop::WorldCameraView camera{focus, desktop::world_viewport(extent, 1)};
        for (int frame = 0; frame < 4; ++frame) {
            canvas.resize({GetRenderWidth(), GetRenderHeight()});
            const auto raster = desktop::canvas_camera(
                desktop::viewport(canvas.size.width, canvas.size.height, extent), extent);
            text.prepare(raster.zoom);
            BeginTextureMode(canvas.texture);
            ClearBackground({145, 211, 247, 255});
            BeginMode2D(raster);
            desktop::draw_world_scene(state, sprites, text, 1, nullptr, 1, &camera);
            EndMode2D();
            EndTextureMode();
            BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(
                canvas.texture.texture,
                {0, 0, static_cast<float>(canvas.size.width),
                 -static_cast<float>(canvas.size.height)},
                {0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())},
                {}, 0, WHITE);
            EndDrawing();
        }
        const auto path = output / (name + ".png");
        auto image = LoadImageFromScreen();
        const bool saved = image.data && ExportImage(image, path.string().c_str());
        if (image.data)
            UnloadImage(image);
        require(saved && same_world_clock(state, before) && state.camera == before.camera &&
                    state.reference_viewport == before.reference_viewport &&
                    state.scene.random.draws() == before.scene.random.draws() &&
                    state.scene.world.world.ai.accounting.funds() ==
                        before.scene.world.world.ai.accounting.funds(),
                "read-only actual-state screenshot must not advance world or RNG");
        for (const auto &entry : before.scene.world.world.ai.contexts)
            require(state.scene.world.world.ai.contexts.at(entry.first).effects.display ==
                        entry.second.effects.display,
                    "drawing must not advance an equipment effect");
        for (const auto &entry : before.facility_details)
            require(state.facility_details.at(entry.first).notices == entry.second.notices,
                    "drawing must not consume the neighbourhood queue");
        std::cout << "CAPTURE " << path.string() << " step=" << state.simulation_steps << '\n';
    };
    const auto actor_focus = [&](rules::CharacterId id) {
        const auto p = state.actor_metadata.at(id).render_position;
        return std::array<float, 2>{(p.x + p.z) * .3F, (p.z - p.x) * .15F + p.height};
    };

    std::uint64_t armor_shop{};
    if (cold_restart) {
        const auto directory = output / "isolated-save";
        auto read = app::read_world_save_slot(directory, 0);
        require(read.state.has_value(), read.message);
        std::string reason;
        require(app::prepare_world_save_candidate(*read.state, state, reason) ==
                    app::WorldSaveError::none,
                reason);
        std::ifstream file(app::world_save_slot_path(directory, 0), std::ios::binary);
        const std::vector<std::uint8_t> original((std::istreambuf_iterator<char>(file)), {});
        const auto recaptured = app::capture_world_save(*read.state);
        require(recaptured.image && recaptured.image->bytes == original &&
                    read.state->scene.random.draws() == state.scene.random.draws(),
                "cold process preserves every durable byte and its own fresh random stream");
        state = std::move(*read.state);
        std::cout << "COLD_LOAD seed=20261008 cash="
                  << state.scene.world.world.ai.accounting.funds()
                  << " rounds=" << state.simulation_steps << " bytes=" << original.size() << '\n';
        capture("cold-loaded-business", state.camera);
    } else {
        const auto original_neighbours = state.neighbourhood;
        std::map<std::uint64_t, std::array<std::int64_t, 4>> original_values;
        for (const auto &entry : original_neighbours)
            if (const auto values = sim::startup_world_facility_values(state, entry.first))
                original_values.emplace(entry.first, values->instance_attributes);
        const auto planting = build(state, 66, {9, 5});
        bool neighbour_change{};
        for (const auto &entry : original_neighbours) {
            if (state.neighbourhood.at(entry.first) == entry.second)
                continue;
            const auto values = sim::startup_world_facility_values(state, entry.first);
            require(values && original_values.count(entry.first) &&
                        values->instance_attributes != original_values.at(entry.first),
                    "actual instance economy changes with the new neighbour");
            const auto &notices = state.facility_details.at(entry.first).notices;
            require(!notices.empty() && notices.front()[0] >= 1 && notices.front()[0] <= 6,
                    "actual changed neighbour has a genuine head notice");
            const auto plan = sim::startup_world_facility_growth_draws(state, entry.first);
            require(plan && !plan->empty(), "source maps the actual head notice to art");
            const auto target = sim::startup_world_runtime_facility_target(state, entry.first);
            require(target.has_value(), "changed neighbour has a source projection");
            std::cout << "NEIGHBOUR instance=" << entry.first << " source=" << planting
                      << " before=" << entry.second[0] << ',' << entry.second[1] << ','
                      << entry.second[2] << " after=" << state.neighbourhood.at(entry.first)[0]
                      << ',' << state.neighbourhood.at(entry.first)[1] << ','
                      << state.neighbourhood.at(entry.first)[2] << " head=" << notices.front()[0]
                      << ',' << notices.front()[1] << '\n';
            capture("construction-neighbour-" + std::to_string(entry.first),
                    {static_cast<float>((*target)[0]), static_cast<float>((*target)[1])});
            neighbour_change = true;
        }
        require(neighbour_change, "initial legal planting changes an existing neighbour");
        armor_shop = build(state, 31, {11, 5});
    }
    std::map<std::pair<rules::CharacterId, int>, int> paid;
    std::map<int, Lift> lifts;
    bool reloaded{cold_restart}, continuation{};
    int loaded_at{-1};
    std::uint64_t old_page{};
    for (int step = 0; step < 6000 && !continuation; ++step) {
        const auto spending = state.scene.world.world.human_spending;
        std::map<std::uint64_t, int> sales;
        for (const auto &entry : state.scene.world.world.facilities)
            sales[entry.first] = entry.second.sales;
        advance(state);
        const auto page = top(state);
        if (page.id != old_page) {
            std::cout << "PAGE update=" << step << " id=" << page.id
                      << " kind=" << static_cast<int>(page.kind) << " raw=" << page.legacy_page
                      << '\n';
            old_page = page.id;
        }
        for (const auto id : state.scene.world.world.ai.human_order) {
            const auto &actor = state.scene.world.world.ai.battle.actors.at(id);
            const auto &binding = state.scene.world.world.actors.at(id).binding;
            const auto previous_spending = spending.find(actor.definition);
            if (binding && previous_spending != spending.end()) {
                const auto &facility =
                    state.scene.world.world.facilities.at(binding->instance_id.value);
                const int delta = state.scene.world.world.human_spending.at(actor.definition) -
                                  previous_spending->second;
                const auto previous_sales = sales.find(binding->instance_id.value);
                if (delta > 0 && previous_sales != sales.end() &&
                    facility.sales > previous_sales->second) {
                    const int kind = facility.detail == 1 ? 15 : facility.detail == 4 ? 21 : 22;
                    if (facility.category == 1 &&
                        (facility.detail == 1 || facility.detail == 4 || facility.detail == 5))
                        paid[{id, kind}] = step;
                    std::cout << "PAYMENT update=" << step << " actor=" << id.value
                              << " human=" << actor.definition
                              << " facility=" << binding->instance_id.value << " delta=" << delta
                              << '\n';
                    continuation |= reloaded && step > loaded_at;
                }
            }
            if (reloaded)
                continue;
            for (const auto &effect : state.scene.world.world.ai.contexts.at(id).effects.display) {
                if (effect.size() < 2 || effect[1] < 0 || (effect[0] != 15 && effect[0] != 21) ||
                    lifts.count(effect[0]))
                    continue;
                const int kind = effect[0], equipment = effect[kind == 15 ? 5 : 4];
                const auto definition = std::find_if(
                    state.rules->equipment.begin(), state.rules->equipment.end(),
                    [&](const auto &d) {
                        return d.shop.kind == (kind == 15 ? 1 : 2) && d.shop.id == equipment;
                    });
                require(definition != state.rules->equipment.end(), "real equipment definition");
                Lift lift{id,
                          actor.definition,
                          kind,
                          equipment,
                          kind == 15 ? 0 : definition->shop.type == 2 ? 1 : 2,
                          step};
                require(paid.count({id, kind}) && paid.at({id, kind}) < step &&
                            actor.control.action == 9 && pending_commit(state, lift),
                        "paid visit precedes genuine action9 lift and unconsumed equipment commit");
                const auto plan = sim::startup_world_equipment_lift_draws(state, id);
                require(plan && !plan->empty(), "natural equipment lift has source artwork");
                std::cout << "LIFT update=" << step << " actor=" << id.value << " cd=" << kind
                          << " item=" << equipment << " paid_at=" << paid.at({id, kind}) << '\n';
                lifts.emplace(kind, lift);
            }
        }
        for (auto &entry : lifts) {
            auto &lift = entry.second;
            if (!lift.shown && step >= lift.step + 20) {
                const auto plan = sim::startup_world_equipment_lift_draws(state, lift.actor);
                require(plan && !plan->empty() && pending_commit(state, lift),
                        "held equipment art precedes actual equip");
                capture("natural-cd" + std::to_string(lift.kind), actor_focus(lift.actor));
                lift.shown = true;
            }
            if (!lift.committed && !pending_commit(state, lift)) {
                const auto &human = state.shop_humans.at(lift.human);
                require(lift.shown && human.equipment[lift.slot] == lift.equipment &&
                            human.reselect[lift.slot] == 6,
                        "real post-lift consumer equips once and sets source reselect counter");
                const auto definition =
                    std::find_if(state.rules->equipment.begin(), state.rules->equipment.end(),
                                 [&](const auto &d) {
                                     return d.shop.kind == (lift.kind == 15 ? 1 : 2) &&
                                            d.shop.id == lift.equipment;
                                 });
                require(definition != state.rules->equipment.end() &&
                            state.scene.world.world.ai.growth.at(lift.human)
                                    .definition.equipment[lift.slot] == definition->shop.combat &&
                            (lift.kind != 15 ||
                             state.shop_actors.at(lift.actor).weapon == lift.equipment),
                        "actual equipment commit installs combat contribution and actor weapon");
                std::cout << "EQUIPPED update=" << step << " human=" << lift.human
                          << " slot=" << lift.slot << " item=" << lift.equipment << '\n';
                capture("equipped-cd" + std::to_string(lift.kind), actor_focus(lift.actor));
                lift.committed = true;
            }
        }
        confirm(state);
        if (!reloaded && lifts.size() == 2 && lifts.at(15).committed && lifts.at(21).committed &&
            app::world_save_eligible(state)) {
            require(state.scene.world.world.facilities.at(armor_shop).status == 1,
                    "real updates completed armor-shop construction");
            const auto image = app::capture_world_save(state);
            require(image.image.has_value(), image.message);
            const auto directory = output / "isolated-save";
            const auto write = app::write_world_save_slot(directory, 0, *image.image);
            require(write.error == app::WorldSaveError::none, write.message);
            auto read = app::read_world_save_slot(directory, 0);
            require(read.state.has_value(), read.message);
            auto restart = initial_world(20261008);
            restart.reference_viewport = desktop::world_viewport(extent, 1);
            std::string reason;
            require(app::prepare_world_save_candidate(*read.state, restart, reason) ==
                        app::WorldSaveError::none,
                    reason);
            const auto recaptured = app::capture_world_save(*read.state);
            require(recaptured.image && recaptured.image->bytes == image.image->bytes &&
                        read.state->scene.random.draws() == restart.scene.random.draws(),
                    "file reload preserves all durable business state and fresh-session random");
            state = std::move(*read.state);
            reloaded = true;
            loaded_at = step;
            std::cout << "RELOAD update=" << step << " restart_seed=20261008 cash="
                      << state.scene.world.world.ai.accounting.funds()
                      << " bytes=" << image.image->bytes.size() << '\n';
            capture("reloaded-business", state.camera);
            break; // A second executable invocation owns the actual cold-start continuation.
        }
        if (step % 500 == 0)
            std::cout << "PROGRESS update=" << step << " rounds=" << state.simulation_steps
                      << " cash=" << state.scene.world.world.ai.accounting.funds() << '\n';
    }
    require(reloaded, "real natural business reached player file boundary");
    if (cold_restart) {
        require(continuation, "actual paid business continues in the cold process");
        capture("continued-business", state.camera);
        std::cout << "PASS cold business: actual file restored without repeat charges, "
                     "fresh random stream, subsequent natural facility payment\n";
    } else {
        std::cout << "PASS natural business: construction/neighbour queue, paid cd15/cd21, "
                     "deferred equipment commits and real player file; "
                     "cold process is a separate acceptance step; "
                     "no OS input claim; accessory/move unlock not covered\n";
    }
}
void world_business_render_fixture() { run_business_fixture(false); }
void world_business_restore_fixture() { run_business_fixture(true); }
} // namespace ark::test
