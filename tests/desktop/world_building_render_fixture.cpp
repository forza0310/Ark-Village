// Rendering acceptance at the published editing callsite, not natural flag32 unlock or OS input.
// Only the gate flag is fixture input. Original map, buildings, prices and source transactions
// remain real; no world updates are manufactured to obtain the two requested frames.
#include "ark/simulation/startup_world_commerce.hpp"
#include "ark/simulation/startup_world_editing.hpp"
#include "support/world_fixture.hpp"
#include "ui/skin.hpp"
#include "ui/world_building.hpp"
#include "ui/world_facility_items.hpp"
#include "world_editing.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace ark::test {
namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(std::string("Map-edit rendering callsite fixture: ") + message);
}
class EditFixtureWindow {
  public:
    EditFixtureWindow() {
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        InitWindow(720, 600, "Ark-Village map-edit callsite fixture / flags32 supplied");
        require(IsWindowReady(), "cannot create hidden rendering window");
    }
    ~EditFixtureWindow() { CloseWindow(); }
    EditFixtureWindow(const EditFixtureWindow &) = delete;
    EditFixtureWindow &operator=(const EditFixtureWindow &) = delete;
};
void accepted(const simulation::StartupBuildResult &result, const char *message) {
    require(result.error == simulation::StartupWorldRuntimeError::none &&
                result.denial == simulation::StartupBuildDenial::none,
            message);
}
} // namespace
void render_world_edit_fixture() {
    namespace rules = simulation::rules;
    namespace ui = desktop::ui;
    auto state = initial_world();
    const auto initial = state;
    const auto cash = state.scene.world.world.ai.accounting.funds();
    const auto draws = state.scene.random.draws();
    const auto old = std::find_if(
        state.scene.world.world.facilities.begin(), state.scene.world.world.facilities.end(),
        [](const auto &entry) { return entry.second.placement.definition_id == 33; });
    require(old != state.scene.world.world.facilities.end(), "real startup bun shop33 is absent");
    const auto old_id = old->first;
    const auto old_placement = old->second.placement;
    state.scripts.user_flags |= 32U; // Explicit callsite gate; no claimed natural first-star event.
    accepted(simulation::begin_startup_world_edit(state, true), "cannot enter source move mode6");
    accepted(simulation::confirm_startup_world_edit(state, old_placement.anchor,
                                                    old_placement.orientation),
             "cannot select real bun shop");
    require(state.build_mode == 7 && state.build_moving_facility == old_id,
            "source selection did not retain its real moving instance");

    // Pick the closest genuinely empty legal footprint through the existing advisory view.
    // It neither edits the map nor substitutes a cheaper definition to obtain a screenshot.
    std::optional<rules::Position> target;
    int nearest = std::numeric_limits<int>::max();
    const auto &map = state.scene.world.world.map;
    for (int y = 0; y < map.height; ++y)
        for (int x = 0; x < map.width; ++x) {
            const int distance =
                std::abs(x - old_placement.anchor.x) + std::abs(y - old_placement.anchor.y);
            if (distance < nearest &&
                desktop::world_build_preview(state, 33, {x, y}, old_placement.orientation)
                    .valid()) {
                target = rules::Position{x, y};
                nearest = distance;
            }
        }
    require(target.has_value(), "real map has no legal vacant moving destination");
    const auto ghost = desktop::world_build_preview(state, 33, *target, old_placement.orientation);
    require(ghost.graphic_visible && ghost.cost == 300 && !ghost.graphic.frames.empty(),
            "mode7 must show its source ghost and fixed300G fee");
    const std::filesystem::path output = std::filesystem::path(ARK_TEST_OUTPUT) / "map-editing";
    std::filesystem::create_directories(output);
    desktop::WorldCameraView camera{{}, desktop::world_viewport({720, 600}, 1)};
    camera.camera = {
        15.F * (old_placement.anchor.x + old_placement.anchor.y + target->x + target->y) + 30,
        7.5F * (old_placement.anchor.y - old_placement.anchor.x + target->y - target->x)};
    EditFixtureWindow window;
    desktop::Sprites sprites(ARK_TEST_ASSETS);
    desktop::Text text(ARK_TEST_FONT, "移动哪个去哪里中止从开始撤除铺呢");
    ui::Skin skin(sprites, text);
    const auto capture = [&](const char *name, bool show_ghost) {
        const auto before = state;
        for (int frame = 0; frame < 4; ++frame) {
            BeginDrawing();
            ClearBackground({145, 211, 247, 255});
            desktop::draw_world_scene(state, sprites, 1, nullptr, 1, &camera);
            const auto editing = desktop::world_edit_view(state, target);
            if (show_ghost)
                desktop::draw_world_build_preview(ghost, camera, 1, sprites);
            desktop::draw_world_edit_controls(editing, {720, 600}, skin, true,
                                              show_ghost ? "300G  Enter移动" : "移动完毕");
            DrawRectangle(0, 0, 720, 54, {52, 55, 41, 255});
            DrawText("Rendering callsite fixture / flags32 supplied / no world updates", 8, 8, 16,
                     WHITE);
            DrawText(
                TextFormat("%s / cash %lldG / old (%d,%d) -> new (%d,%d)",
                           show_ghost ? "mode7 ghost" : "committed move",
                           static_cast<long long>(state.scene.world.world.ai.accounting.funds()),
                           old_placement.anchor.x, old_placement.anchor.y, target->x, target->y),
                8, 31, 15, YELLOW);
            text.flush(1, {});
            EndDrawing();
        }
        const auto file = output / name;
        auto image = LoadImageFromScreen();
        const bool saved = image.data && ExportImage(image, file.string().c_str());
        if (image.data)
            UnloadImage(image);
        require(saved && std::filesystem::is_regular_file(file) && std::filesystem::file_size(file),
                "could not save real rendered capture");
        require(same_world_clock(state, before) &&
                    state.scene.scene_counter == before.scene.scene_counter &&
                    state.scene.random.draws() == before.scene.random.draws() &&
                    state.scene.world.world.ai.accounting.funds() ==
                        before.scene.world.world.ai.accounting.funds(),
                "drawing advanced the source counters or changed cash/random");
        std::cout << "Map-edit rendering callsite fixture / flags32 supplied: " << name
                  << " frames=4 cash=" << state.scene.world.world.ai.accounting.funds()
                  << " screenshot=" << file.string() << '\n';
    };
    capture("callsite-mode7-ghost.png", true);
    const auto moved =
        simulation::confirm_startup_world_edit(state, *target, old_placement.orientation);
    accepted(moved, "real source move could not commit its previously legal destination");
    require(moved.created && !state.scene.world.world.facilities.count(old_id) &&
                state.scene.world.world.facilities.at(*moved.created).placement.anchor == *target &&
                state.scene.world.world.facilities.at(*moved.created).placement.orientation ==
                    old_placement.orientation &&
                state.scene.world.world.ai.accounting.funds() == cash - 300 &&
                state.scene.random.draws() == draws && same_world_clock(state, initial),
            "source move did not preserve orientation/time/random and charge exactly300G");
    capture("callsite-moved-map.png", false);
}
// One explicit static-page callsite batch, with MAX, rank/commerce gate and stock supplied.
// Source consumers create/initialize the actual pages; no natural unlock or gameplay claim.
void world_facility_static_render_fixture() {
    namespace sim = simulation;
    namespace ui = desktop::ui;
    const auto top_page = [](const auto &state) -> const sim::rules::WorldScriptPage & {
        const auto page = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                       [](const auto &p) { return p.lifecycle != 4; });
        require(page != state.scripts.pages.rend(), "static fixture lost its source top page");
        return *page;
    };
    const auto open_bun = [&]() {
        auto state = initial_world();
        const auto bun = std::find_if(
            state.scene.world.world.facilities.begin(), state.scene.world.world.facilities.end(),
            [](const auto &f) { return f.second.placement.definition_id == 33; });
        require(bun != state.scene.world.world.facilities.end(),
                "static fixture has no real bun shop");
        require(sim::open_startup_world_facility_page(state, bun->first) ==
                    sim::StartupWorldRuntimeError::none,
                "source cannot open real raw74");
        return state;
    };
    auto ordinary = open_bun();
    auto second = ordinary;
    require(sim::act_startup_world_facility_page(second, top_page(second).id,
                                                 sim::StartupFacilityPageAction::next) ==
                sim::StartupWorldRuntimeError::none,
            "source cannot turn real raw74 second page");
    auto maximum = ordinary;
    maximum.scene.world.world.facility_uses.at(33).level = maximum.scripts.facilities.at(33).level =
        5;
    auto items = open_bun();
    for (int n = 0; n < 6; ++n) {
        const int id = items.rules->items.at(n).identity;
        items.items.at(id).inventory = 2 + n;
        items.catalog.at({0, id}) = items.items.at(id);
    }
    require(sim::open_startup_world_facility_items(items, top_page(items).id) ==
                    sim::StartupWorldRuntimeError::none &&
                sim::initialize_startup_world_facility_item_pages(items),
            "source cannot initialize supplied-stock raw75");
    auto preview = initial_world();
    preview.scripts.user_flags |= 16U;
    preview.rank = 5; // Explicit eligibility fixture, never a claimed natural rank/commerce unlock.
    // Source open_commerce schedules the first-visit97 introduction above the selection
    // chain when unseen. This static-page callsite fixture supplies its already-seen gate,
    // rather than clearing that real modal stack or pretending to have acknowledged it.
    preview.scripts.event_calls[97] = 1;
    require(sim::open_startup_world_commerce(preview) == sim::StartupWorldRuntimeError::none,
            "source open83 failed with supplied rank5/flag16/seen97 gates");
    require(top_page(preview).legacy_page == 83,
            "supplied seen97 gate did not expose the actual83 selection page");
    require(sim::initialize_startup_world_commerce_pages(preview), "source initialize83 failed");
    auto commerce_page = top_page(preview).id;
    require(sim::act_startup_world_commerce_page(preview, commerce_page,
                                                 sim::StartupCommerceAction::select,
                                                 2) == sim::StartupWorldRuntimeError::none,
            "source select83 facilities option failed");
    require(sim::act_startup_world_commerce_page(preview, commerce_page,
                                                 sim::StartupCommerceAction::confirm) ==
                sim::StartupWorldRuntimeError::none,
            "source confirm83 facilities option failed");
    require(top_page(preview).legacy_page == 85, "source confirm83 did not create actual85");
    require(sim::initialize_startup_world_commerce_pages(preview), "source initialize85 failed");
    commerce_page = top_page(preview).id;
    const auto offers = sim::inspect_startup_world_commerce_page(preview, commerce_page);
    require(offers.has_value(), "source lost facilities85 offers");
    int equipment_index = -1;
    for (std::size_t index = 0; index < offers->entries.size(); ++index) {
        const auto definition =
            std::find_if(preview.rules->facilities.begin(), preview.rules->facilities.end(),
                         [&](const auto &d) { return d.id == offers->entries[index]; });
        if (definition != preview.rules->facilities.end() &&
            (definition->detail == 1 || definition->detail == 4 || definition->detail == 5)) {
            equipment_index = static_cast<int>(index);
            break;
        }
    }
    require(equipment_index >= 0, "supplied rank has no actual equipment offer");
    require(sim::act_startup_world_commerce_page(
                preview, commerce_page, sim::StartupCommerceAction::select, equipment_index) ==
                    sim::StartupWorldRuntimeError::none &&
                sim::act_startup_world_commerce_page(preview, commerce_page,
                                                     sim::StartupCommerceAction::inspect) ==
                    sim::StartupWorldRuntimeError::none,
            "source cannot open real equipment definition preview");
    const auto output = std::filesystem::path(ARK_TEST_OUTPUT) / "facility-details";
    std::filesystem::create_directories(output);
    EditFixtureWindow window;
    desktop::Sprites sprites(ARK_TEST_ASSETS);
    std::string glyphs = "距离下个等级还有人周围设施暂无来源入住希望者商品种类维护费品质魅力住宅"
                         "设施情报使用道具设施强化建设返回关闭使用确定价格升级";
    for (const auto &facility : items.rules->facilities)
        glyphs += facility.name;
    for (const auto &item : items.rules->items)
        glyphs += item.name;
    desktop::Text text(ARK_TEST_FONT, glyphs);
    ui::Skin skin(sprites, text);
    const auto capture = [&](const auto &state, const char *filename, bool narrow) {
        const auto before = state;
        const desktop::Extent extent =
            narrow ? desktop::Extent{240, 256} : desktop::Extent{720, 600};
        SetWindowSize(static_cast<int>(extent.width), static_cast<int>(extent.height));
        const auto &page = top_page(state);
        for (int frame = 0; frame < 4; ++frame) {
            BeginDrawing();
            ClearBackground({145, 211, 247, 255});
            if (page.legacy_page == 75)
                ui::draw_world_facility_items(ui::world_facility_items_view(state, page),
                                              ui::world_facility_items_layout(extent), skin, true);
            else
                ui::draw_world_building(ui::world_building_view(state, page),
                                        ui::world_building_layout(extent, 74), skin, {}, true);
            text.flush(1, {});
            EndDrawing();
        }
        const auto file = output / filename;
        auto image = LoadImageFromScreen();
        const bool saved = image.data && ExportImage(image, file.string().c_str());
        if (image.data)
            UnloadImage(image);
        require(saved && std::filesystem::is_regular_file(file), "static fixture capture failed");
        require(same_world_clock(state, before) &&
                    state.scene.random.draws() == before.scene.random.draws() &&
                    state.scene.world.world.ai.accounting.funds() ==
                        before.scene.world.world.ai.accounting.funds() &&
                    state.page_counters == before.page_counters &&
                    state.page_phases == before.page_phases,
                "static rendering changed cash/random/time or source page phase");
        std::cout
            << "Static facility callsite fixture / MAX, rank5/flag16/seen97 gates, stock supplied: "
            << filename << " frames=4 screenshot=" << file.string() << '\n';
    };
    capture(ordinary, "raw74-ordinary.png", false);
    capture(second, "raw74-second.png", false);
    capture(maximum, "raw74-max.png", false);
    capture(preview, "raw74-equipment-preview.png", false);
    capture(items, "raw75-minimum.png", true);
}
} // namespace ark::test
