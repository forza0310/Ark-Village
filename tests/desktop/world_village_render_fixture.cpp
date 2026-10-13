// Conditional callsites for village UI. No natural unlock, payment or OS-input claim.
#include "../../src/desktop/scene/projection.hpp"
#include "../../src/desktop/scene/world_canvas.hpp"
#include "../../src/desktop/ui/common/skin.hpp"
#include "../../src/desktop/ui/village/world_progression.hpp"
#include "../../src/desktop/ui/village/world_village_activity.hpp"
#include "../support/world_fixture.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace ark::test {
namespace {
namespace sim = simulation;
using State = sim::StartupWorldRuntimeState;
using Page = sim::rules::WorldScriptPage;
void require(bool ok, const char *why) {
    if (!ok)
        throw std::runtime_error(why);
}
const Page &top(const State &s) { return s.scripts.pages.back(); }
State callsite(int raw, int reward = 0) {
    auto s = initial_world();
    s.scripts.pages.front().lifecycle = 3;
    s.rank = 1;
    s.village_points = 300;
    s.quarter_counter = 4;
    s.scripts.event_calls[100] = 1;
    for (int id = 0; id < 9; ++id)
        s.human_presence.at(id) = 1;
    for (auto &[id, progress] : s.scripts.activities)
        if (id != 27)
            progress.status = 1;
    Page p;
    p.kind = sim::rules::WorldScriptPageKind::raw_page;
    p.legacy_page = raw;
    p.lifecycle = 0;
    p.legacy_r = reward;
    p.legacy_s = reward == 0    ? 500
                 : reward == 1  ? 25
                 : reward == 3  ? 35
                 : reward == 4  ? 0
                 : reward == 11 ? 0
                                : 1;
    const auto page =
        sim::rules::prepare_world_script_page(sim::startup_world_runtime_scripts(s), p);
    require(page.candidate && sim::write_startup_world_runtime_scripts(s, page.candidate->state),
            "Village render callsite insertion failed");
    if ((raw >= 48 && raw <= 50) || raw == 95)
        require(sim::initialize_startup_world_town_progress_pages(s), "Town render Init failed");
    else
        require(sim::initialize_startup_world_village_activity_pages(s),
                "Village render Init failed");
    // Explicit presentation callsite: lifecycle1, with all actual Init payload already present.
    s.scripts.pages.back().lifecycle = 1;
    return s;
}
} // namespace
void world_village_render_fixture() {
    using namespace desktop;
    namespace ui = desktop::ui;
    std::vector<std::pair<std::string, State>> samples;
    auto listing = callsite(51);
    samples.emplace_back("activity51", listing);
    const auto list = sim::inspect_startup_world_village_activity_page(listing, top(listing).id);
    require(list && list->entries.size() > 5, "Activity renderer needs a real long directory");
    auto tail = listing;
    tail.activity_page_selections.at(top(tail).id) = int(list->entries.size()) - 1;
    tail.activity_page_scroll.at(top(tail).id) = int(list->entries.size()) - 1;
    samples.emplace_back("activity51-tail", std::move(tail));
    const auto chosen = std::find(list->entries.begin(), list->entries.end(), 4);
    require(chosen != list->entries.end(), "Published attribute activity4 missing");
    require(sim::act_startup_world_village_activity_page(
                listing, top(listing).id, sim::StartupVillageActivityAction::select,
                int(chosen - list->entries.begin())) == sim::StartupWorldRuntimeError::none &&
                sim::act_startup_world_village_activity_page(
                    listing, top(listing).id, sim::StartupVillageActivityAction::confirm) ==
                    sim::StartupWorldRuntimeError::none &&
                sim::initialize_startup_world_village_activity_pages(listing),
            "Real52 render child failed");
    listing.scripts.pages.back().lifecycle = 1;
    samples.emplace_back("activity52", listing);
    require(sim::act_startup_world_village_activity_page(
                listing, top(listing).id, sim::StartupVillageActivityAction::confirm) ==
                    sim::StartupWorldRuntimeError::none &&
                sim::initialize_startup_world_village_activity_pages(listing),
            "Real53 render child failed");
    listing.scripts.pages.back().lifecycle = 1;
    for (int frame : {0, 60, 69, 70, 110, 120}) {
        auto s = listing;
        s.page_counters.at(top(s).id) = frame;
        samples.emplace_back("activity53-" + std::to_string(frame), std::move(s));
    }
    listing.page_counters.at(top(listing).id) = 120;
    require(sim::act_startup_world_village_activity_page(
                listing, top(listing).id, sim::StartupVillageActivityAction::confirm) ==
                    sim::StartupWorldRuntimeError::none &&
                sim::initialize_startup_world_village_activity_pages(listing),
            "Real54 render child failed");
    listing.scripts.pages.back().lifecycle = 1;
    samples.emplace_back("activity54", listing);
    const auto last = int(listing.activity_page_lists.at(top(listing).id).size()) - 1;
    listing.activity_page_selections.at(top(listing).id) = last;
    listing.activity_page_scroll.at(top(listing).id) = last;
    samples.emplace_back("activity54-tail", std::move(listing));
    samples.emplace_back("rank48", callsite(48));
    samples.emplace_back("rank49", callsite(49));
    for (int phase = 0; phase < 3; ++phase)
        for (int frame : {0, 35, 100, 140}) {
            auto s = callsite(50);
            s.page_phases.at(top(s).id) = phase;
            s.page_counters.at(top(s).id) = frame;
            s.page_secondary_counters.at(top(s).id) = 19;
            samples.emplace_back("celebration50-" + std::to_string(phase) + "-" +
                                     std::to_string(frame),
                                 std::move(s));
        }
    for (int kind : {0, 1, 3, 4, 9, 10, 11})
        for (int frame : {0, 40}) {
            auto s = callsite(95, kind);
            s.page_counters.at(top(s).id) = frame;
            samples.emplace_back("gift95-" + std::to_string(kind) + "-" + std::to_string(frame),
                                 std::move(s));
        }
    InitWindow(480, 512, "Ark-Village village / explicit UI callsites");
    struct WindowScope {
        ~WindowScope() { CloseWindow(); }
    } window;
    Sprites sprites(ARK_TEST_ASSETS);
    std::string glyphs =
        "街道升级等级上升恭喜太好了万岁请活动获得满足上升了辛苦春夏秋冬还有价格姓名的条件申请了解居"
        "住数建设设施任务完成人气举办实施中开展不品质力器用丈夫体力魔运冒险者们结束完毕魔法壶配置更"
        "替勋章可以转职街道点数Ｇ○×返回领取继续确定";
    for (const auto &a : samples.front().second.rules->activities)
        glyphs += a.name + a.detail + a.description;
    for (const auto &f : samples.front().second.rules->facilities)
        glyphs += f.name;
    for (const auto &h : samples.front().second.rules->humans)
        glyphs += h.name;
    for (const auto &j : samples.front().second.rules->jobs)
        glyphs += j.name;
    Text text(ARK_TEST_FONT, glyphs);
    ui::Skin skin(sprites, text);
    WorldCanvas canvas;
    const auto output = std::filesystem::path(ARK_TEST_OUTPUT) / "village";
    std::filesystem::create_directories(output);
    for (const auto extent : {Extent{240, 256}, Extent{720, 600}})
        for (const auto &[name, owner] : samples) {
            const auto before = owner;
            SetWindowSize(extent.width, extent.height);
            for (int frame = 0; frame < 4; ++frame) {
                canvas.resize({GetRenderWidth(), GetRenderHeight()});
                const auto raster =
                    canvas_camera(viewport(canvas.size.width, canvas.size.height, extent), extent);
                text.prepare(raster.zoom);
                BeginTextureMode(canvas.texture);
                ClearBackground({145, 211, 247, 255});
                BeginMode2D(raster);
                if (ui::world_village_activity_page(top(owner))) {
                    auto view = ui::world_village_activity_view(owner, top(owner));
                    ui::prepare_world_village_activity(view, owner, skin);
                    require(view.skin.has_value(), "Bound activity plan missing");
                    ui::draw_world_village_activity(
                        view, ui::world_village_activity_layout(extent, view.raw), skin, true);
                } else {
                    auto view = ui::world_progression_view(owner, top(owner));
                    ui::prepare_world_progression(view, owner, skin);
                    require(view.skin.has_value(), "Bound town plan missing");
                    const auto controls = ui::draw_world_progression(
                        view, ui::world_progression_layout(extent, view.raw), skin, true);
                    if (view.raw == 49) {
                        require(controls && controls->confirm.width > 0,
                                "49 source acknowledge area missing");
                        ui::WorldProgressionInput click;
                        click.click = Vector2{controls->confirm.x + controls->confirm.width / 2,
                                              controls->confirm.y + controls->confirm.height / 2};
                        const auto input = ui::world_progression_input(
                            view, ui::world_progression_layout(extent, 49), click, false, controls);
                        require(input && !input->rank_action,
                                "49 drawn acknowledge must close, never promote");
                    }
                }
                EndMode2D();
                text.flush(raster.zoom, raster.offset);
                EndTextureMode();
                BeginDrawing();
                ClearBackground(BLACK);
                DrawTexturePro(canvas.texture.texture,
                               {0, 0, float(canvas.size.width), -float(canvas.size.height)},
                               {0, 0, float(GetScreenWidth()), float(GetScreenHeight())}, {0, 0}, 0,
                               WHITE);
                EndDrawing();
            }
            const auto file = output / (name + "-" + std::to_string(extent.width) + ".png");
            auto image = LoadImageFromScreen();
            const bool saved = image.data && ExportImage(image, file.string().c_str());
            if (image.data)
                UnloadImage(image);
            require(saved, "Village callsite screenshot failed");
            require(same_world_clock(owner, before) &&
                        owner.page_counters == before.page_counters &&
                        owner.page_secondary_counters == before.page_secondary_counters &&
                        owner.page_phases == before.page_phases &&
                        owner.scene.random.draws() == before.scene.random.draws() &&
                        owner.village_points == before.village_points &&
                        owner.scene.world.world.ai.accounting.funds() ==
                            before.scene.world.world.ai.accounting.funds() &&
                        owner.sound_requests == before.sound_requests &&
                        owner.events_held == before.events_held &&
                        owner.quarter_counter == before.quarter_counter,
                    "Village drawing changed clocks, money, events or random");
            std::cout << "Village conditional render: screenshot=" << file.string() << '\n';
        }
}
} // namespace ark::test
