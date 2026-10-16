// Optional actual-window task callsites. Conditions are explicit, not natural unlock evidence.
#include "../../src/desktop/scene/projection.hpp"
#include "../../src/desktop/scene/world_canvas.hpp"
#include "../../src/desktop/ui/actors/world_human_ceremony.hpp"
#include "../../src/desktop/ui/common/skin.hpp"
#include "../../src/desktop/ui/common/world_reports.hpp"
#include "../../src/desktop/ui/tasks/world_dungeon_footer.hpp"
#include "../../src/desktop/ui/tasks/world_task_tracking.hpp"
#include "../../src/desktop/ui/tasks/world_tasks.hpp"
#include "../../src/desktop/ui/village/world_progression.hpp"
#include "../support/world_fixture.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"
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
    if (std::getenv("ARK_RENDER_DUNGEON_FOOTER")) {
        InitWindow(480, 512, "Ark-Village dungeon footer / explicit conditions");
        struct WindowScope {
            ~WindowScope() { CloseWindow(); }
        } window;
        Sprites sprites(ARK_TEST_ASSETS);
        Text text(ARK_TEST_FONT);
        ui::Skin skin(sprites, text);
        const auto *configured = std::getenv("ARK_TEST_OUTPUT_DIR");
        const auto output =
            std::filesystem::path(configured ? configured : ARK_TEST_OUTPUT) / "footer";
        std::filesystem::create_directories(output);
        using Base = sim::SteamDungeonFooterBaseLanguage;
        using Language = sim::SteamDungeonFooterResourceLanguage;
        const std::array<sim::SteamDungeonFooterLanguageProfile, 4> languages{
            {{Base::root, Language::root, true},
             {Base::english, Language::english, true},
             {Base::english, Language::simplified_chinese, true},
             {Base::english, Language::traditional_chinese, true}}};
        WorldCanvas canvas;
        for (const auto extent : {Extent{240, 256}, Extent{540, 360}})
            for (std::size_t language = 0; language < languages.size(); ++language)
                for (const int age : {0, 4, 8}) {
                    sim::SteamDungeonFooterInput input;
                    input.view = {0, 0, extent.width, extent.height};
                    input.page = 1;
                    input.presentation.ordinal = 1;
                    input.presentation.pages = {1};
                    input.presentation.random_after = 1;
                    input.presentation.dungeon_jitters = {{1, 7, 1, -1}};
                    sim::SteamDungeonFooterTask task;
                    task.facility = 7;
                    task.background_row = 3;
                    task.counter = age == 0 ? 4 : 18;
                    task.progress = 42;
                    task.maximum = 100;
                    task.challenges = {{{18, 0, 3, 0, 0, 0}, -1, -1},
                                       {{64, 1, 1, 20, 0, 0}, 0, 42}};
                    task.humans = {{22, 0, 0, 0}, {50, 1, 13, 1}};
                    // Age8 is already retired by the actual logic consumer (runtime_tasks suite).
                    if (age < 8)
                        task.effects = {{{age, 85, extent.height - 14}}};
                    input.task = task;
                    const auto plan = sim::steam_dungeon_footer_skin(input);
                    require(plan && !plan->operations.empty(), "Footer conditional plan failed");
                    for (int frame = 0; frame < 4; ++frame) {
                        SetWindowSize(extent.width, extent.height);
                        canvas.resize({GetRenderWidth(), GetRenderHeight()});
                        const auto raster = canvas_camera(
                            viewport(canvas.size.width, canvas.size.height, extent), extent);
                        text.prepare(raster.zoom);
                        BeginTextureMode(canvas.texture);
                        ClearBackground({145, 211, 247, 255});
                        BeginMode2D(raster);
                        // A stripe behind the transparent source overflow makes cropping visible.
                        DrawRectangle(0, extent.height - 18, extent.width, 18,
                                      {220, 160, 100, 255});
                        ui::draw_world_dungeon_footer(*plan, skin, languages[language]);
                        EndMode2D();
                        text.flush(raster.zoom, raster.offset);
                        EndTextureMode();
                        BeginDrawing();
                        ClearBackground(BLACK);
                        DrawTexturePro(canvas.texture.texture,
                                       {0, 0, float(canvas.size.width), -float(canvas.size.height)},
                                       {0, 0, float(GetScreenWidth()), float(GetScreenHeight())},
                                       {0, 0}, 0, WHITE);
                        EndDrawing();
                    }
                    const auto file =
                        output / ("footer-" + std::to_string(extent.width) + "-language" +
                                  std::to_string(language) + "-age" + std::to_string(age) + ".png");
                    auto image = LoadImageFromScreen();
                    const bool saved = image.data && ExportImage(image, file.string().c_str());
                    if (image.data)
                        UnloadImage(image);
                    require(saved, "Footer screenshot failed");
                    std::cout << "Conditional footer screenshot=" << file.string() << '\n';
                }
        return;
    }
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
    // Conditional source-bound successors. These frozen payloads do not certify natural wins.
    auto team = listing;
    const auto team_id = top(team).id;
    auto &team_page = team.scripts.pages.back();
    team_page.legacy_page = 25;
    team_page.lifecycle = 0;
    team_page.task_identity = selected_task;
    team_page.task_definition = team.tasks.at(selected_task).definition;
    team.task_page_lists.erase(team_id);
    team.page_phases.erase(team_id);
    team.page_secondary_counters.erase(team_id);
    team.page_counters[team_id] = 8;
    team.participants = {1, 2, 3, 4, 5, 6};
    for (int id = 0; id < 9; ++id)
        team.human_presence.at(id) = 1;
    require(sim::initialize_startup_world_runtime_task_ui_page(team, team_id),
            "Conditional team Init rejected");
    team.scripts.pages.back().lifecycle = 2;
    team.task_page_ui.at(team_id).selection = 6;
    team.task_page_ui.at(team_id).first = 2;
    require(sim::valid_startup_world_runtime_task_page(team, top(team)),
            "Conditional frozen team rejected");
    samples.emplace_back("task25-tail", team);
    auto active = team;
    active.scripts.pages.back().legacy_page = 26;
    active.active_task = selected_task;
    samples.emplace_back("task26-active", active);
    auto extra = team;
    sim::StartupTaskPageInput additional;
    additional.confirm = true;
    require(sim::input_startup_world_runtime_task_page(extra, team_id, additional).error ==
                sim::StartupWorldRuntimeError::none,
            "Actual additional-member producer rejected");
    ready(extra);
    require(top(extra).legacy_page == 27, "Conditional extra page was not produced");
    samples.emplace_back("task27-fees", extra);
    extra.task_page_ui.at(top(extra).id).tab = 1;
    samples.emplace_back("task27-attributes", extra);
    auto departure = team;
    additional = {};
    additional.depart = true;
    require(sim::input_startup_world_runtime_task_page(departure, team_id, additional).error ==
                sim::StartupWorldRuntimeError::none,
            "Actual departure prompt producer rejected");
    ready(departure);
    require(top(departure).legacy_page == 28, "Conditional departure prompt was not produced");
    samples.emplace_back("task28-yes", departure);
    departure.task_page_ui.at(top(departure).id).selection = 1;
    samples.emplace_back("task28-no", departure);
    departure.task_page_ui.at(top(departure).id).selection = 0;
    departure.page_phases.at(top(departure).id) = 1;
    for (int counter : {0, 32, 70, 95}) {
        auto s = departure;
        s.page_counters.at(top(s).id) = counter;
        s.page_secondary_counters.at(top(s).id) = counter / 3;
        samples.emplace_back("task28-depart" + std::to_string(counter), std::move(s));
    }
    auto crew = team;
    auto &crew_page = crew.scripts.pages.back();
    crew_page.legacy_page = 31;
    crew_page.lifecycle = 0;
    crew.task_page_ui.clear();
    crew.page_counters.erase(team_id);
    require(sim::initialize_startup_world_runtime_task_summary_ui_page(crew, team_id),
            "Actual crew result Init rejected");
    crew.scripts.pages.back().lifecycle = 2;
    samples.emplace_back("task31-crew", crew);
    auto treasure = team;
    auto &treasure_page = treasure.scripts.pages.back();
    treasure_page.legacy_page = 32;
    treasure_page.lifecycle = 0;
    treasure_page.legacy_f = 1;
    treasure_page.legacy_g = 3;
    treasure.task_page_ui.clear();
    treasure.page_counters.erase(team_id);
    treasure.exploration_summaries.emplace(
        team_id, sim::rules::WorldExplorationSummary{
                     32, selected_task, treasure.tasks.at(selected_task).definition, {}});
    auto &rewards = treasure.exploration_summaries.at(team_id).rewards;
    const auto &equipment = treasure.rules->equipment.front();
    for (int row = 0; row < 9; ++row)
        rewards.push_back(row % 2 ? std::array<int, 2>{equipment.shop.kind, equipment.shop.id}
                                  : std::array<int, 2>{0, treasure.rules->items.front().identity});
    require(sim::initialize_startup_world_runtime_task_summary_ui_page(treasure, team_id),
            "Treasure UI Init rejected");
    treasure.scripts.pages.back().lifecycle = 2;
    samples.emplace_back("task32-rewards", treasure);
    treasure.task_page_ui.at(team_id).first = 4;
    samples.emplace_back("task32-tail", treasure);
    auto deadline = team;
    auto &deadline_page = deadline.scripts.pages.back();
    deadline_page.legacy_page = 33;
    deadline_page.lifecycle = 0;
    deadline.task_page_ui.clear();
    deadline.page_counters.erase(team_id);
    deadline.active_task = selected_task;
    require(sim::initialize_startup_world_runtime_deadline_page(deadline, team_id),
            "Deadline UI Init rejected");
    deadline.scripts.pages.back().lifecycle = 2;
    samples.emplace_back("task33-continue", deadline);
    for (int answer : {0, 1}) {
        auto s = deadline;
        s.task_page_ui.at(team_id).selection = answer;
        s.page_phases.at(team_id) = 1;
        s.deadline_returns[team_id] = answer;
        for (int counter : {0, 25, 50}) {
            s.page_counters.at(team_id) = counter;
            samples.emplace_back(
                "task33-answer" + std::to_string(answer) + "-" + std::to_string(counter), s);
        }
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
    std::string glyphs =
        "任务选择寻求！冒险者姓名费用请选择任务募集冒险者返回选择追踪加速合计人交"
        "给我!多指教来了!我会加油的!我要参加!好期待攻略了解自宅完成的满意度努力"
        "胜利！因为完成任务街道人气上升了!!显示全文确定参加者名单参加委托追加队员"
        "任务开始立即重新考虑成果打倒数倒地下次数获得宝物！任务继续确认加油哦!遗憾…"
        "有些困难吧有点担心这些人真是了不起我觉得会好的值得期待能轻松通关他们做到"
        "再加把劲再加加油需要补充战力重新来过比较好应该撤退消费金继续中止贡献努力";
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
                    const auto plan = ui::prepare_world_human_ceremony(s, top(s), skin);
                    require(bool(plan), "Residence Owner bridge unavailable");
                    auto v = ui::world_progression_view(s, top(s));
                    ui::prepare_world_progression(v, s, skin);
                    require(bool(v.ceremony),
                            "Normal management progression must bind the residence ceremony");
                    ui::draw_world_progression(v, ui::world_progression_layout(extent, 96), skin,
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
                    if (v.raw <= 24)
                        ui::draw_world_task_selection(v, ui::world_task_layout(extent, v.raw), skin,
                                                      true);
                    else {
                        const auto controls = ui::draw_world_task_followup(
                            v, ui::world_task_layout(extent, v.raw), skin, true, {});
                        require(bool(controls), "Followup painted geometry missing");
                        require(controls->geometry.selection == v.selected &&
                                    controls->geometry.first == v.first,
                                "Followup geometry must tag the actual Owner cursor");
                        const auto frame = ui::world_task_layout(extent, v.raw).panel;
                        for (const auto &row : controls->geometry.rows)
                            require(row.rectangle.x >= frame.x && row.rectangle.y >= frame.y &&
                                        row.rectangle.x + row.rectangle.width <=
                                            frame.x + frame.width &&
                                        row.rectangle.y + row.rectangle.height <=
                                            frame.y + frame.height,
                                    "Followup PC rows cannot hit outside the painted source frame");
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
            require(saved, "Task screenshot failed");
            require(same_world_clock(s, before) && s.page_counters == before.page_counters &&
                        s.page_secondary_counters == before.page_secondary_counters &&
                        s.page_phases == before.page_phases &&
                        s.task_page_ui.size() == before.task_page_ui.size() &&
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
