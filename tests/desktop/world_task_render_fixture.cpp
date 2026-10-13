// Optional actual-window task callsites. Conditions are explicit, not natural unlock evidence.
#include "../../src/desktop/scene/projection.hpp"
#include "../../src/desktop/scene/world_canvas.hpp"
#include "../../src/desktop/ui/actors/world_residence_result.hpp"
#include "../../src/desktop/ui/common/skin.hpp"
#include "../../src/desktop/ui/common/world_reports.hpp"
#include "../../src/desktop/ui/tasks/world_task_tracking.hpp"
#include "../../src/desktop/ui/tasks/world_tasks.hpp"
#include "../support/world_fixture.hpp"
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
namespace ark::test {
namespace {
namespace sim = simulation;
using State = sim::StartupWorldRuntimeState;
void require(bool ok, const char *why) {
    if (!ok)
        throw std::runtime_error(why);
}
const sim::rules::WorldScriptPage &top(const State &s) { return s.scripts.pages.back(); }
void ready(State &s) {
    for (int n = 0; n < 6 && top(s).lifecycle != 2; ++n) {
        auto r = sim::prepare_startup_world_runtime(s);
        require(bool(r.candidate), "Task renderer source Init rejected");
        s = std::move(*r.candidate);
    }
    require(top(s).lifecycle == 2, "Task renderer page never became live");
}
} // namespace
void world_task_render_fixture() {
    namespace ui = desktop::ui;
    using namespace desktop;
    auto listing = initial_world();
    const auto task =
        sim::rules::prepare_world_task_creation(sim::startup_world_runtime_factory(listing), 0);
    require(task.candidate &&
                sim::write_startup_world_runtime_factory(listing, task.candidate->state),
            "Renderer actual factory rejected");
    require(sim::open_startup_world_runtime_task_menu(listing) ==
                sim::StartupWorldRuntimeError::none,
            "Renderer task menu rejected");
    ready(listing);
    auto offer = listing;
    sim::StartupTaskPageInput input;
    input.confirm = true;
    require(sim::input_startup_world_runtime_task_page(offer, top(offer).id, input).error ==
                sim::StartupWorldRuntimeError::none,
            "Renderer actual offer rejected");
    ready(offer);
    auto recruitment = offer;
    require(sim::input_startup_world_runtime_task_page(recruitment, top(recruitment).id, input)
                .accepted,
            "Renderer actual payment rejected");
    ready(recruitment);
    std::vector<std::pair<std::string, State>> samples{
        {"task22", listing}, {"task23", offer}, {"task24-empty", recruitment}};
    auto tracking = listing;
    input = {};
    input.track = true;
    require(sim::input_startup_world_runtime_task_page(tracking, top(tracking).id, input).error ==
                    sim::StartupWorldRuntimeError::none &&
                tracking.scene.scene_state == 5,
            "Renderer source tracking rejected");
    samples.emplace_back("task-info-moving", tracking);
    const auto scene_id = top(tracking).id;
    const auto selected_task = tracking.task_page_lists.at(scene_id).front();
    const auto site = *tracking.tasks.at(selected_task).site;
    tracking.camera = {float((site.x + site.y) * 30 + 30), float((site.y - site.x) * 15 + 10)};
    const auto arrived = sim::update_startup_world_runtime_task_tracking_scene(tracking);
    require(arrived && arrived->scene.scene_state == 4, "Renderer source camera arrival rejected");
    tracking = *arrived;
    samples.emplace_back("task-info-single", tracking);
    tracking.task_order.push_back(selected_task);
    tracking.scene.frame_counter = 8;
    samples.emplace_back("task-info-arrows", tracking);
    tracking.scene.framework_paused = true;
    samples.emplace_back("task-info-paused", tracking);
    auto information = listing;
    auto &info = information.scripts.pages.back();
    info.legacy_page = 29;
    info.task_identity = selected_task;
    info.task_definition = information.tasks.at(selected_task).definition;
    samples.emplace_back("task-info-raw29", std::move(information));
    // Explicit complete display payloads; these do not certify natural success or residence.
    auto victory = listing;
    auto &result = victory.scripts.pages.back();
    result.legacy_page = 30;
    result.task_identity.reset(); // Exploration binds its summary rather than encounter fields.
    result.task_definition.reset();
    result.legacy_f = 347;
    victory.exploration_summaries.emplace(
        result.id, sim::rules::WorldExplorationSummary{
                       30, selected_task, victory.tasks.at(selected_task).definition, {}});
    victory.page_secondary_counters[result.id] = 8;
    victory.page_phases[result.id] = 0;
    victory.page_counters[result.id] = 5;
    for (int i = 0; i < 3; ++i)
        victory.participants.push_back(victory.rules->humans.at(i).identity);
    require(sim::valid_startup_world_runtime_task_victory_page(victory, result),
            "Conditional victory payload rejected");
    samples.emplace_back("task30-early", victory);
    for (int second : {10, 18, 25}) {
        auto s = victory;
        s.page_counters[result.id] = 40;
        s.page_secondary_counters[result.id] = second;
        samples.emplace_back("task30-jump" + std::to_string(second), std::move(s));
    }
    victory.page_counters[result.id] = 40;
    victory.page_phases[result.id] = 1;
    samples.emplace_back("task30-reputation", std::move(victory));
    auto house = listing;
    auto &complete = house.scripts.pages.back();
    complete.legacy_page = 96;
    const auto definition =
        std::find_if(house.rules->facilities.begin(), house.rules->facilities.end(),
                     [](const auto &d) { return d.kind == 12; });
    require(definition != house.rules->facilities.end(), "Missing source residence definition");
    complete.legacy_f = definition->id;
    house.page_human_bindings[complete.id] = house.rules->humans.front().identity;
    house.page_secondary_counters[complete.id] = 18;
    house.page_counters[complete.id] = 50;
    house.page_phases[complete.id] = 0;
    house.reward_display = {{{9, 18}, {11, 23}, {2, 5}}};
    require(sim::valid_startup_world_runtime_residence_result_page(house, complete),
            "Conditional residence payload rejected");
    samples.emplace_back("house96-complete", house);
    house.page_phases[complete.id] = 1;
    for (int counter : {0, 57, 77}) {
        house.page_counters[complete.id] = counter;
        samples.emplace_back("house96-reward" + std::to_string(counter), house);
    }
    auto long_list = listing;
    auto &ids = long_list.task_page_lists.at(top(long_list).id);
    const auto first = ids.front();
    ids.assign(7, first); // Published duplicate references are legal.
    long_list.page_phases.at(top(long_list).id) = 6;
    long_list.page_secondary_counters.at(top(long_list).id) = 6;
    samples.emplace_back("task22-tail", std::move(long_list));
    const int h = recruitment.rules->humans.front().identity;
    recruitment.human_presence.at(h) = 1;
    for (int frame : {0, 20, 54, 55, 79, 80, 109, 110, 129}) {
        auto s = recruitment;
        auto &a = s.task_recruitment_pages.at(top(s).id);
        a.entries = {{h, frame, 0}};
        a.portraits = {h};
        a.displayed_count = 1;
        s.page_counters.at(top(s).id) = frame;
        samples.emplace_back("task24-arrival" + std::to_string(frame), std::move(s));
    }
    InitWindow(480, 512, "Ark-Village task / explicit source callsites");
    struct Scope {
        ~Scope() { CloseWindow(); }
    } scope;
    Sprites sprites(ARK_TEST_ASSETS);
    std::string glyphs = "任务选择寻求！冒险者姓名费用请选择任务募集冒险者返回选择追踪加速合计人交"
                         "给我!多指教来了!我会加油的!我要参加!好期待攻略了解自宅完成的满意度努力"
                         "胜利！因为完成任务街道人气上升了!!显示全文确定";
    for (const auto &t : listing.rules->tasks)
        glyphs += t.name;
    for (const auto &p : listing.rules->humans)
        glyphs += p.name;
    Text text(ARK_TEST_FONT, glyphs);
    ui::Skin skin(sprites, text);
    WorldCanvas canvas;
    const auto *isolated_output = std::getenv("ARK_TEST_OUTPUT_DIR");
    const auto output =
        std::filesystem::path(isolated_output ? isolated_output : ARK_TEST_OUTPUT) / "tasks";
    std::filesystem::create_directories(output);
    for (const auto extent : {Extent{240, 256}, Extent{720, 600}})
        for (const auto &[name, s] : samples) {
            const auto before = s;
            SetWindowSize(extent.width, extent.height);
            for (int frame = 0; frame < 4; ++frame) {
                canvas.resize({GetRenderWidth(), GetRenderHeight()});
                const auto raster =
                    canvas_camera(viewport(canvas.size.width, canvas.size.height, extent), extent);
                text.prepare(raster.zoom);
                BeginTextureMode(canvas.texture);
                ClearBackground({145, 211, 247, 255});
                BeginMode2D(raster);
                if (top(s).legacy_page == 30) {
                    auto v = ui::world_victory_view(s, top(s));
                    ui::prepare_world_victory(v, s, skin);
                    require(bool(v.skin), "Victory Owner bridge unavailable");
                    ui::draw_world_victory(v, ui::world_victory_layout(extent), skin, true);
                } else if (top(s).legacy_page == 96) {
                    const auto plan = ui::prepare_world_residence_result(s, top(s), skin);
                    require(bool(plan), "Residence Owner bridge unavailable");
                    ui::draw_world_residence_result(plan, ui::world_victory_layout(extent), skin,
                                                    true);
                } else if (s.scene.scene_state == 4 || s.scene.scene_state == 5 ||
                           top(s).legacy_page == 29) {
                    auto v = top(s).legacy_page == 29 ? ui::world_task_information_view(s, top(s))
                                                      : *ui::world_task_tracking_view(s);
                    ui::prepare_world_task_tracking(v, s, skin);
                    const auto controls = ui::draw_world_task_tracking(
                        v, ui::world_task_tracking_layout(extent), skin, true);
                    require(bool(controls) == v.initialized,
                            "Information draws only at its source lifecycle");
                    if (controls) {
                        require(controls->geometry.confirm.width > 0 && controls->tasks == v.tasks,
                                "Information must bind actual painted button and tasks");
                        ui::WorldTaskInput click;
                        const auto r = controls->geometry.confirm;
                        click.click = Vector2{r.x + r.width / 2, r.y + r.height / 2};
                        const auto intent = ui::world_task_tracking_input(
                            v, ui::world_task_tracking_layout(extent), click, false, controls);
                        require(bool(intent) == v.ready, "Painted information input lifecycle");
                    }
                } else {
                    auto v = ui::world_task_view(s, top(s));
                    require(v.initialized, "Task conditional view unavailable");
                    ui::prepare_world_task(v, s, skin);
                    ui::draw_world_task_selection(v, ui::world_task_layout(extent, v.raw), skin,
                                                  true);
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
            require(saved, "Task screenshot failed");
            require(same_world_clock(s, before) && s.page_counters == before.page_counters &&
                        s.page_secondary_counters == before.page_secondary_counters &&
                        s.page_phases == before.page_phases &&
                        s.items.size() == before.items.size() &&
                        std::equal(s.items.begin(), s.items.end(), before.items.begin(),
                                   [](const auto &a, const auto &b) {
                                       return a.first == b.first &&
                                              a.second.inventory == b.second.inventory;
                                   }) &&
                        s.scene.random.draws() == before.scene.random.draws() &&
                        s.scene.world.world.ai.accounting.funds() ==
                            before.scene.world.world.ai.accounting.funds() &&
                        s.sound_requests == before.sound_requests,
                    "Task rendering changed state or outputs");
            std::cout << "Task conditional render: screenshot=" << file.string() << '\n';
        }
}
} // namespace ark::test
