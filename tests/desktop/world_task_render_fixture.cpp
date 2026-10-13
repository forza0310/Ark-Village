// Optional actual-window task callsites. Conditions are explicit, not natural unlock evidence.
#include "../../src/desktop/scene/projection.hpp"
#include "../../src/desktop/scene/world_canvas.hpp"
#include "../../src/desktop/ui/common/skin.hpp"
#include "../../src/desktop/ui/tasks/world_tasks.hpp"
#include "../support/world_fixture.hpp"
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"
#include <algorithm>
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
                         "给我!多指教来了!我会加油的!我要参加!好期待";
    for (const auto &t : listing.rules->tasks)
        glyphs += t.name;
    for (const auto &p : listing.rules->humans)
        glyphs += p.name;
    Text text(ARK_TEST_FONT, glyphs);
    ui::Skin skin(sprites, text);
    WorldCanvas canvas;
    const auto output = std::filesystem::path(ARK_TEST_OUTPUT) / "tasks";
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
                auto v = ui::world_task_view(s, top(s));
                require(v.initialized, "Task conditional view unavailable");
                ui::prepare_world_task(v, s, skin);
                ui::draw_world_task_selection(v, ui::world_task_layout(extent, v.raw), skin, true);
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
