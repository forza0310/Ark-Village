// Conditional callsites for village UI. No natural unlock, payment or OS-input claim.
#include "../../src/desktop/scene/projection.hpp"
#include "../../src/desktop/scene/world_canvas.hpp"
#include "../../src/desktop/ui/common/skin.hpp"
#include "../../src/desktop/ui/system/world_clear_score.hpp"
#include "../../src/desktop/ui/village/world_commerce.hpp"
#include "../../src/desktop/ui/village/world_progression.hpp"
#include "../../src/desktop/ui/village/world_village_activity.hpp"
#include "../support/checks.hpp"
#include "../support/world_fixture.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
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
    if (raw == 93) {
        // A paid blueprint receipt always binds a still-unopened definition. Initial
        // bakery35 is already available and cannot stand in for this source callsite.
        const auto unopened =
            std::find_if(s.rules->facilities.begin(), s.rules->facilities.end(),
                         [&](const auto &d) { return s.facility_presence.at(d.id) == 0; });
        require(unopened != s.rules->facilities.end(), "Receipt needs an unopened facility");
        p.legacy_s = unopened->id;
    }
    if (reward >= 5 && reward <= 7) {
        const auto equipment =
            std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(),
                         [&](const auto &d) { return d.shop.kind == reward - 4; });
        require(equipment != s.rules->equipment.end(), "Reward namespace fixture missing");
        p.legacy_s = equipment->shop.id;
    } else if (reward == 8)
        p.legacy_s = s.rules->items.front().identity;
    const auto page =
        sim::rules::prepare_world_script_page(sim::startup_world_runtime_scripts(s), p);
    require(page.candidate && sim::write_startup_world_runtime_scripts(s, page.candidate->state),
            "Village render callsite insertion failed");
    if (raw == 94) {
        s.page_human_bindings[top(s).id] = 1;
        require(sim::initialize_startup_world_gift_notice_pages(s), "Character gift Init failed");
    } else if (raw == 93)
        require(sim::initialize_startup_world_commerce_pages(s), "Facility claim Init failed");
    else if ((raw >= 48 && raw <= 50) || raw == 95)
        require(sim::initialize_startup_world_town_progress_pages(s), "Town render Init failed");
    else
        require(sim::initialize_startup_world_village_activity_pages(s),
                "Village render Init failed");
    // Explicit presentation callsite, with all actual Init payload already present.
    s.scripts.pages.back().lifecycle = raw == 93 || raw == 94 ? 2 : 1;
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
    for (int raw : {93, 94})
        for (int kind = 3; kind <= (raw == 93 ? 3 : 8); ++kind)
            for (int frame : {0, 39, 40}) {
                auto s = callsite(raw, kind);
                s.page_counters.at(top(s).id) = frame;
                samples.emplace_back("gift" + std::to_string(raw) + "-" + std::to_string(kind) +
                                         "-" + std::to_string(frame),
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
        "替勋章可以转职街道点数Ｇ○×返回领取继续确定注目因为的介绍被了可为";
    for (const auto &a : samples.front().second.rules->activities)
        glyphs += a.name + a.detail + a.description;
    for (const auto &f : samples.front().second.rules->facilities)
        glyphs += f.name;
    for (const auto &h : samples.front().second.rules->humans)
        glyphs += h.name;
    for (const auto &j : samples.front().second.rules->jobs)
        glyphs += j.name;
    for (const auto &e : samples.front().second.rules->equipment)
        glyphs += e.name;
    for (const auto &i : samples.front().second.rules->items)
        glyphs += i.name;
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
                    } else if (view.raw == 93 || view.raw == 94) {
                        require(
                            controls && controls->confirm.width > 0 &&
                                !ui::world_commerce_page(top(owner)),
                            "93/94 must use one source-reward UI route and its painted soft key");
                        ui::WorldProgressionInput click;
                        click.click = Vector2{controls->confirm.x + controls->confirm.width / 2,
                                              controls->confirm.y + controls->confirm.height / 2};
                        const auto layout = ui::world_progression_layout(extent, view.raw);
                        require(
                            ui::world_progression_input(view, layout, click, false, controls)
                                    .has_value() &&
                                !ui::world_progression_input(view, layout, click, true, controls),
                            "93/94 painted soft key forwards one command and preserves pending");
                        auto stale = controls;
                        ++stale->choice;
                        require(!ui::world_progression_input(view, layout, click, false, stale),
                                "Reward hot area must reject changed source reward identity");
                        stale = controls;
                        ++stale->count;
                        require(!ui::world_progression_input(view, layout, click, false, stale),
                                "Reward hot area must reject changed real giver binding");
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

namespace {
// Explicit raw17 application callsite; this exercises the source bridge, not natural completion.
std::pair<State, app::WorldSystemState> clear_callsite() {
    auto state = initial_world();
    state.scripts.pages.front().lifecycle = 3;
    Page page;
    page.kind = sim::rules::WorldScriptPageKind::raw_page;
    page.legacy_page = 17;
    page.lifecycle = 0; // The factory also enforces the natural pending Init state.
    const auto opened =
        sim::rules::prepare_world_script_page(sim::startup_world_runtime_scripts(state), page);
    require(opened.candidate &&
                sim::write_startup_world_runtime_scripts(state, opened.candidate->state),
            "Clear-score application callsite failed");
    app::WorldSystemState system;
    require(app::advance_world_clear(state, system, false).empty() && system.clear.has_value(),
            "Clear-score source initialization failed");
    return {std::move(state), std::move(system)};
}
} // namespace
void world_clear_score_ui() {
    namespace ui = desktop::ui;
    Checks check("World clear-score UI");
    const auto [state, system] = clear_callsite();
    const auto &clear = *system.clear;
    const auto input = ui::world_clear_score_input(state, clear);
    check(input && input->main_counter == 2 && input->stage_counter == 1 &&
              input->captured_high_score == clear.score.captured_high_score,
          "Clear bridge uses independent Owner clocks and frozen previous record");
    auto changed = state;
    changed.page_secondary_counters.at(clear.page) = 40;
    const auto changed_clock = ui::world_clear_score_input(changed, clear);
    check(changed_clock && changed_clock->main_counter == 40 && changed_clock->stage_counter == 1,
          "Changing the real main-clock mirror never replaces the stage clock");
    changed = state;
    changed.page_secondary_counters.erase(clear.page);
    check(!ui::world_clear_score_input(changed, clear),
          "Missing main clock cannot be repaired by a presentation fallback");
    changed = state;
    ++changed.page_counters.at(clear.page);
    check(!ui::world_clear_score_input(changed, clear),
          "Stale application stage clock refuses a mixed publication");
    changed = state;
    ++changed.page_phases.at(clear.page);
    check(!ui::world_clear_score_input(changed, clear),
          "Stale application stage mirror refuses a mixed publication");
    changed = state;
    changed.page_secondary_counters.at(clear.page) = std::numeric_limits<int>::max();
    check(!ui::world_clear_score_input(changed, clear),
          "Unwrapped main clock is invalid instead of becoming a desktop animation clock");
    changed = state;
    changed.scripts.pages.back().lifecycle = 3;
    check(!ui::world_clear_score_input(changed, clear),
          "Suspended clear page cannot use active-page paint qualification");
    changed.scripts.pages.back().lifecycle = 4;
    check(!ui::world_clear_score_input(changed, clear),
          "Retired page cannot paint an old clear controller");
    auto stale = clear;
    ++stale.page;
    check(!ui::world_clear_score_input(state, stale),
          "Application controller identity must match the actual top page");
    stale = clear;
    ++stale.score.sum;
    check(!ui::world_clear_score_input(state, stale),
          "Score-row association is checked without re-running the score consumer");
    changed = state;
    changed.scene.framework_paused = true;
    check(ui::world_clear_score_input(changed, clear).has_value(),
          "Paused rendering can read its unchanged initialized Owner payload");
    const auto before_draws = state.scene.random.draws();
    const auto before_sound = state.sound_requests;
    for (int query = 0; query < 8; ++query)
        check(ui::world_clear_score_input(state, clear).has_value(),
              "Repeated clear-score bridge remains read-only");
    check(state.scene.random.draws() == before_draws && state.sound_requests == before_sound &&
              state.page_secondary_counters.at(clear.page) == 2 && clear.score.counter == 1,
          "Bridge does not advance clocks, random or audio outputs");
    std::cout << "World clear-score UI: " << check.count() << " checks\n";
}
void world_clear_score_render_fixture() {
    using namespace desktop;
    namespace ui = desktop::ui;
    auto [owner, system] = clear_callsite();
    std::vector<std::pair<std::string, sim::SteamClearScoreInput>> samples;
    std::optional<sim::SteamClearScoreInput> first, last;
    for (int update = 0; update < 3000 && system.clear; ++update) {
        const auto input = ui::world_clear_score_input(owner, *system.clear);
        require(input.has_value(), "Clear render application binding failed");
        const int stage = input->stage, count = input->stage_counter;
        if (!first)
            first = input;
        last = input;
        const bool selected =
            (stage == 0 && (count == 1 || count == 14 || count == 15 || count == 44 ||
                            count == 45 || count == 75)) ||
            (stage == 1 &&
             ((input->row == 0 && (count == 14 || count == 15 || count == 44)) || count == 45)) ||
            (stage == 2 && input->row == 0 && (count == 14 || count == 15)) ||
            (stage == 3 && ((input->row == 0 && (count == 14 || count == 45)) || count == 15)) ||
            (stage == 4 && count == 0) ||
            (stage == 5 && input->row == 1 && (count == 14 || count == 15)) ||
            (stage == 6 &&
             (count == 14 || count == 15 || count == 39 || count == 40 || count == 64));
        if (selected)
            samples.emplace_back("score17-stage" + std::to_string(stage) + "-row" +
                                     std::to_string(input->row) + "-phase" + std::to_string(count),
                                 *input);
        require(app::advance_world_clear(owner, system, count == 75 || (stage == 3 && count == 45))
                    .empty(),
                "Clear conditional source progression failed");
    }
    require(!system.clear && first && last && last->stage == 6,
            "Clear callsite must complete once through the application consumer");
    // Actor/blink boundary conditions are explicit paint inputs, never synthetic world updates.
    for (int stage : {0, 3, 6})
        for (int main : {20, 21}) {
            auto input = stage == 0 ? *first : *last;
            input.stage = stage;
            input.stage_counter = stage == 0 ? 75 : stage == 3 ? 45 : 65;
            input.row = stage == 6 ? 6 : 0;
            input.main_counter = main;
            samples.emplace_back("score17-arrow-stage" + std::to_string(stage) + "-main" +
                                     std::to_string(main),
                                 std::move(input));
        }
    for (int main : {0, 4}) {
        auto input = *last;
        input.stage = 4;
        input.main_counter = main;
        samples.emplace_back("score17-celebration-main" + std::to_string(main), std::move(input));
    }
    auto terminal = *last;
    terminal.stage = 7;
    terminal.stage_counter = 0;
    terminal.rows.reset();
    samples.emplace_back("score17-terminal7-no-seventh-row", terminal);
    auto large = *first;
    large.stage = 3;
    large.stage_counter = 15;
    large.sum = large.captured_high_score = std::numeric_limits<std::int64_t>::max();
    (*large.rows)[0].count = (*large.rows)[0].score = large.sum;
    samples.emplace_back("score17-conditional-long-number", std::move(large));
    auto count = *first;
    count.stage = 1;
    count.stage_counter = 45;
    (*count.rows)[0].count = 1234567;
    samples.emplace_back("score17-conditional-comma-count", std::move(count));

    InitWindow(480, 512, "Ark-Village clear-score / explicit application callsites");
    struct WindowScope {
        ~WindowScope() { CloseWindow(); }
    } window;
    Sprites sprites(ARK_TEST_ASSETS);
    Text text(ARK_TEST_FONT,
              "通关点数发表那么赶紧开始计算吧街道人气任务完成设施发现冒险者全员LV合计自宅努力度总和"
              "追加排行榜更新接下来就是这样当前最高分0123456789,!…:");
    ui::Skin skin(sprites, text);
    WorldCanvas canvas;
    const auto output = std::filesystem::path(ARK_TEST_OUTPUT) / "clear-score";
    std::filesystem::create_directories(output);
    const auto before = owner;
    for (const auto extent : {Extent{240, 256}, Extent{720, 600}})
        for (const auto &[name, input] : samples) {
            SetWindowSize(extent.width, extent.height);
            for (int frame = 0; frame < 4; ++frame) {
                canvas.resize({GetRenderWidth(), GetRenderHeight()});
                const auto raster =
                    canvas_camera(viewport(canvas.size.width, canvas.size.height, extent), extent);
                text.prepare(raster.zoom);
                const auto plan = ui::prepare_world_clear_score(input, skin);
                require(plan.has_value(), "Measured clear-score plan failed");
                require(plan->soft_labels == std::array<int, 2>{0, 0} &&
                            std::holds_alternative<sim::SteamStartupTouch>(plan->operations.back()),
                        "Clear-score skin must keep empty soft labels and the global source touch");
                BeginTextureMode(canvas.texture);
                ClearBackground({145, 211, 247, 255});
                BeginMode2D(raster);
                ui::draw_world_clear_score(*plan, extent, skin);
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
            require(saved, "Clear-score callsite screenshot failed");
            std::cout << "Clear-score conditional render: screenshot=" << file.string() << '\n';
        }
    require(same_world_clock(owner, before) && owner.page_counters == before.page_counters &&
                owner.page_phases == before.page_phases &&
                owner.page_secondary_counters == before.page_secondary_counters &&
                owner.scene.random.draws() == before.scene.random.draws() &&
                owner.sound_requests == before.sound_requests &&
                owner.scene.world.world.ai.accounting.funds() ==
                    before.scene.world.world.ai.accounting.funds(),
            "Score drawing changed source clocks, funds, random or sound outputs");
    std::cout << "PASS clear-score conditional painting; no natural completion or OS-input claim\n";
}
} // namespace ark::test
