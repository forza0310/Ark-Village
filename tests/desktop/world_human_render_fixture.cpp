// Explicit renderer fixture, never a natural-world or mastery-reward acceptance result.
// This case is invoked manually and is deliberately absent from standard CTest registration.
#include "../../src/desktop/resources/resources.hpp"
#include "../../src/desktop/ui/actors/world_human.hpp"
#include "../../src/desktop/ui/common/skin.hpp"
#include "../support/world_fixture.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace ark::test {
namespace {
class FixtureWindow {
  public:
    FixtureWindow() {
        InitWindow(720, 600, "Ark-Village UI fixture / synthetic human presentation");
        if (!IsWindowReady())
            throw std::runtime_error("Synthetic UI fixture could not create a window");
        SetTargetFPS(60);
    }
    ~FixtureWindow() { CloseWindow(); }
    FixtureWindow(const FixtureWindow &) = delete;
    FixtureWindow &operator=(const FixtureWindow &) = delete;
};
void capture(const std::filesystem::path &file) {
    auto image = LoadImageFromScreen();
    const bool saved = image.data && ExportImage(image, file.string().c_str());
    UnloadImage(image);
    if (!saved || !std::filesystem::is_regular_file(file) || std::filesystem::file_size(file) == 0)
        throw std::runtime_error("Human fixture failed to write its screenshot");
}
} // namespace

void world_human_render_fixture() {
    namespace ui = desktop::ui;
    namespace rules = simulation::rules;
    const std::filesystem::path output{ARK_TEST_OUTPUT};
    std::filesystem::create_directories(output);
    auto state = initial_world();
    const int human = state.rules->humans.front().identity;
    auto &growth = state.scene.world.world.ai.growth.at(human);
    // Synthetic display input only: no experience, mastery reward or profession consumer runs.
    growth.definition.profession_levels.at(growth.definition.current_profession) = 10;
    rules::WorldScriptPage page;
    page.id = 901;
    page.kind = rules::WorldScriptPageKind::raw_page;
    page.legacy_page = 70;
    page.lifecycle = 2;

    FixtureWindow window;
    desktop::Sprites sprites(ARK_TEST_ASSETS);
    std::string glyphs = "职业大师体力力量灵活结实魔力运气魔法转职保持现状确定"
                         "获得了因为奖励上升了!器用丈夫火魔法冰魔法雷魔法回复魔法学会了";
    glyphs += state.rules->humans.at(human).name;
    glyphs += state.rules->jobs.at(growth.definition.current_profession).name;
    glyphs += state.rules->jobs.at(2).name;
    glyphs += ui::world_human_down_text;
    desktop::Text text(ARK_TEST_FONT, glyphs);
    ui::Skin skin(sprites, text);
    struct MasterySample {
        int phase, counter, secondary, selection, spell;
        const char *name;
    };
    constexpr MasterySample masters[]{
        {0, 5, 0, 0, -1, "opening"},
        {0, 40, 10, 0, -1, "jump"},
        {0, 75, 25, 0, -1, "land"},
        {1, 40, 1, 0, -1, "attribute"},
        {1, 40, 23, 0, 0, "fire"},
        {1, 40, 25, 0, 3, "recovery"},
        {2, 0, 10, 0, -1, "change"},
        {2, 0, 25, 1, -1, "keep"},
        {2, 40, std::numeric_limits<int>::max() - 1, 1, -1, "rollover-tail"}};
    for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{720, 600}}) {
        SetWindowSize(extent.width, extent.height);
        for (const auto &sample : masters) {
            auto owner = initial_world();
            const auto job = std::find_if(
                owner.rules->jobs.begin(), owner.rules->jobs.end(), [&](const auto &j) {
                    return sample.spell < 0 ? j.mastery_attribute >= 0 && j.mastery_attribute < 6
                                            : j.mastery_attribute == 10 + sample.spell;
                });
            if (job == owner.rules->jobs.end())
                throw std::runtime_error("Mastery fixture requires a published reward profession");
            auto &g = owner.scene.world.world.ai.growth.at(human);
            g.definition.current_profession = int(job - owner.rules->jobs.begin());
            g.definition.profession_levels.at(g.definition.current_profession) = 10;
            if (!text.include_text(job->name))
                throw std::runtime_error("Mastery profession name font glyph missing");
            owner.scripts.pages.push_back(page);
            owner.page_human_bindings[page.id] = human;
            owner.human_pages_initialized.insert(page.id);
            owner.page_phases[page.id] = sample.phase;
            owner.page_counters[page.id] = sample.counter;
            owner.page_secondary_counters[page.id] = sample.secondary;
            owner.human_page_selections[page.id] = sample.selection;
            const auto before = owner;
            const auto layout = ui::world_human_layout(extent);
            auto view = ui::world_human_view(owner, page);
            ui::prepare_world_human_detail(view, owner, skin);
            const auto controls = ui::world_human_painted_controls(view, layout, extent, skin);
            if (!view.initialized || view.details.level != 10 || !view.detail_page_skin ||
                !controls ||
                (sample.phase == 2 ? !controls->mastery_buttons[0] ||
                                         !controls->mastery_buttons[1] || bool(controls->primary)
                                   : !controls->primary || bool(controls->mastery_buttons[0]) ||
                                         bool(controls->mastery_buttons[1])))
                throw std::runtime_error("Synthetic raw70 fixture lost its source plan/controls");
            if (sample.phase == 2) {
                const auto panel = ui::world_human_subwindow_layout(layout, 70).panel;
                for (int choice = 0; choice < 2; ++choice) {
                    const auto r = *controls->mastery_buttons[choice];
                    if (r.x < panel.x || r.y < panel.y || r.x + r.width > panel.x + panel.width ||
                        r.y + r.height > panel.y + panel.height)
                        throw std::runtime_error("Mastery touch escapes its source window");
                    ui::WorldHumanInput click;
                    click.click = Vector2{r.x + r.width / 2, r.y + r.height / 2};
                    const auto action =
                        ui::world_human_input(view, layout, click, false, controls, extent);
                    if (!action || !action->input || action->input->selection != choice ||
                        !action->input->confirm)
                        throw std::runtime_error(
                            "Painted mastery choice lost select/confirm frame");
                }
            } else {
                const auto r = *controls->primary;
                ui::WorldHumanInput click;
                click.click = Vector2{r.x + r.width / 2, r.y + r.height / 2};
                const auto action =
                    ui::world_human_input(view, layout, click, false, controls, extent);
                if (!action || !action->input || !action->input->confirm ||
                    action->input->selection)
                    throw std::runtime_error(
                        "Mastery PC footer lost its source confirmation frame");
            }
            for (int frame = 0; frame < 8; ++frame) {
                if (WindowShouldClose())
                    throw std::runtime_error("Synthetic UI fixture was closed before all captures");
                BeginDrawing();
                ClearBackground({145, 211, 247, 255});
                DrawText("SYNTHETIC mastery UI", 4, 4, 12, BLACK);
                ui::draw_world_human(view, layout, skin, true);
                text.flush(1, {});
                EndDrawing();
            }
            const auto file = output / ("synthetic-raw70-" + std::to_string(extent.width) + "-" +
                                        sample.name + ".png");
            capture(file);
            if (!same_world_clock(owner, before) ||
                owner.scene.world.world.ai.accounting.funds() !=
                    before.scene.world.world.ai.accounting.funds() ||
                owner.scene.random.draws() != before.scene.random.draws() ||
                owner.page_counters != before.page_counters ||
                owner.page_secondary_counters != before.page_secondary_counters ||
                owner.page_phases != before.page_phases ||
                owner.human_page_selections != before.human_page_selections ||
                owner.reward_display != before.reward_display ||
                owner.sound_requests != before.sound_requests ||
                owner.scene.world.world.ai.growth.at(human).definition.extra !=
                    before.scene.world.world.ai.growth.at(human).definition.extra)
                throw std::runtime_error(
                    "Rendering synthetic mastery unexpectedly changed world state");
            std::cout << "UI fixture / synthetic raw70 sample=" << sample.name
                      << " frames=8 screenshot=" << file.string() << '\n';
        }
    }
    // Display snapshots cover all four pages, page0 recovery/down and long page1 text/numbers.
    // They deliberately do not claim natural combat, acquisitions or job progression.
    for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{720, 600}}) {
        SetWindowSize(extent.width, extent.height);
        for (int sample = 0; sample < 10; ++sample) {
            const int phase = sample < 4                   ? sample
                              : sample == 5                ? 1
                              : sample == 7 || sample == 9 ? 2
                              : sample == 8                ? 3
                                                           : 0;
            auto owner = initial_world();
            auto catalogue = *owner.rules;
            owner.rules = &catalogue;
            auto detail = page;
            detail.legacy_page = 60;
            owner.scripts.pages.push_back(detail);
            owner.page_human_bindings[detail.id] = human;
            owner.human_detail_contexts.emplace(detail.id, simulation::StartupHumanDetailContext{});
            owner.human_pages_initialized.insert(detail.id);
            owner.page_phases[detail.id] = phase;
            owner.page_counters[detail.id] = std::array<int, 4>{0, 100, 25, 0}[phase];
            owner.human_page_selections[detail.id] = 0;
            owner.human_homes.at(human)[2] = phase == 1 ? 0 : 1;
            owner.human_calendar.at(human).celebrations = std::array<int, 4>{4, 7, 7, 12}[phase];
            auto &ai = owner.scene.world.world.ai;
            ai.growth.at(human).definition.current_profession = 2;
            owner.shop_humans.at(human).equipment[0] = 0;
            if (sample == 7)
                owner.human_page_selections[detail.id] = 2;
            if (sample == 8)
                ai.growth.at(human).derived.available_spells = {true, false, false, true};
            if (sample == 9) {
                auto weapon =
                    std::find_if(catalogue.equipment.begin(), catalogue.equipment.end(),
                                 [](const auto &e) { return e.shop.kind == 1 && e.shop.id == 0; });
                weapon->name = "名称很长的测试武器";
                if (!text.include_text(weapon->name))
                    throw std::runtime_error("Long weapon font glyph missing");
            }
            if (sample == 5) {
                catalogue.humans.at(human).name = "名称较长的冒险人物";
                catalogue.jobs.at(2).name = "LongProfessionName";
                ai.growth.at(human).derived.attributes = {12345, 23456, 34567, 45678, 56789, 67890};
                if (!text.include_text(catalogue.humans.at(human).name))
                    throw std::runtime_error("Synthetic long name could not add font glyphs");
            }
            if (phase >= 2 || sample == 4 || sample == 6) {
                rules::BattleActorRecord actor;
                actor.id = {900};
                actor.kind = rules::ActorKind::human;
                actor.definition = human;
                actor.hp.displayed = 80;
                actor.hp.target = 0;
                actor.control.state = phase == 3 || sample == 6 ? 7 : 0;
                ai.battle.actors.emplace(actor.id, actor);
                ai.human_order = {actor.id};
                ai.growth.at(human).derived.combat[0] = 100;
                if (sample >= 4) {
                    const auto recovery =
                        std::find_if(catalogue.items.begin(), catalogue.items.end(),
                                     [](const auto &item) { return item.recovery > 0; });
                    if (recovery == catalogue.items.end())
                        throw std::runtime_error(
                            "Synthetic recovery needs a published recovery item");
                    owner.items.at(recovery->identity).inventory = 1;
                    owner.catalog.at({0, recovery->identity}).inventory = 1;
                }
            }
            const auto before = owner;
            const auto frame = ui::world_human_layout(extent);
            const int down_width =
                static_cast<int>(std::ceil(text.width(ui::world_human_down_text)));
            auto view = ui::world_human_view(owner, detail, down_width);
            ui::prepare_world_human_detail(view, owner, skin);
            if (!view.initialized || !view.detail_skin)
                throw std::runtime_error("Synthetic raw60 fixture has no complete portrait plan");
            if (!view.detail_page_skin)
                throw std::runtime_error(
                    "Synthetic raw60 did not bind the formal four-page measured plan");
            if ((sample == 4 || sample == 6) && !view.detail_page_skin->recovery_gift)
                throw std::runtime_error("Target HP/down fixture lost the actual recovery hint");
            for (int draw = 0; draw < 8; ++draw) {
                if (WindowShouldClose())
                    throw std::runtime_error("Synthetic raw60 fixture was closed before capture");
                BeginDrawing();
                ClearBackground({145, 211, 247, 255});
                DrawText("SYNTHETIC raw60", 4, 4, 12, BLACK);
                ui::draw_world_human(view, frame, skin, true);
                text.flush(1, {});
                EndDrawing();
            }
            const std::string suffix = sample < 4    ? "phase" + std::to_string(phase)
                                       : sample == 4 ? "recovery"
                                       : sample == 5 ? "long-attributes"
                                       : sample == 6 ? "down-overview"
                                       : sample == 7 ? "empty-slot2"
                                       : sample == 8 ? "mixed-magic"
                                                     : "long-equipment";
            const auto file = output / ("synthetic-raw60-" + std::to_string(extent.width) + "-" +
                                        suffix + ".png");
            capture(file);
            if (!same_world_clock(owner, before) || owner.page_counters != before.page_counters ||
                owner.scene.random.draws() != before.scene.random.draws() ||
                owner.scene.world.world.ai.accounting.funds() !=
                    before.scene.world.world.ai.accounting.funds())
                throw std::runtime_error("Rendering raw60 unexpectedly changed the Owner");
            std::cout << "UI fixture / synthetic raw60 phase=" << phase
                      << " frames=8 screenshot=" << file.string() << '\n';
        }
    }
    // Child pages retain a complete parent binding. These display conditions do not acquire,
    // pay for or equip anything; the existing runtime suites own those transactions.
    constexpr const char *samples[]{"single",   "multiple", "long-name",    "increase",
                                    "decrease", "same",     "empty-armour", "long-comparison"};
    for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{720, 600}}) {
        SetWindowSize(extent.width, extent.height);
        for (int sample = 0; sample < 8; ++sample) {
            auto owner = initial_world();
            auto catalogue = *owner.rules;
            owner.rules = &catalogue;
            const bool info = sample < 3;
            const int slot = sample == 6 ? 1 : 0;
            const int current = sample == 4 ? 1 : 0;
            const int candidate = sample == 3 || sample == 7 ? 1 : 0;
            owner.shop_humans.at(human).equipment[slot] =
                sample == 6 ? std::optional<int>{} : current;
            auto parent = page;
            parent.legacy_page = 64;
            parent.lifecycle = 3;
            if (!info) {
                owner.scripts.pages.push_back(parent);
                owner.page_human_bindings[parent.id] = human;
                owner.page_phases[parent.id] = slot;
                owner.page_counters[parent.id] = 0;
                owner.human_page_selections[parent.id] = 0;
                owner.human_page_scroll[parent.id] = 0;
                owner.human_pages_initialized.insert(parent.id);
                owner.equipment_page_catalogs[parent.id][slot] = {candidate};
                owner.human_equipment_choices[parent.id] = {slot, candidate};
            }
            auto child = page;
            child.id = 902;
            child.legacy_page = info ? 73 : 65;
            owner.scripts.pages.push_back(child);
            owner.page_human_bindings[child.id] = human;
            owner.page_phases[child.id] = slot;
            owner.page_counters[child.id] = 16 * (sample % 3);
            owner.human_page_selections[child.id] = 0;
            owner.human_pages_initialized.insert(child.id);
            owner.human_equipment_choices[child.id] = {slot, candidate};
            if (info)
                owner.equipment_page_catalogs[child.id][slot] =
                    sample == 0 ? std::vector<int>{0} : std::vector<int>{0, 1};
            else
                owner.human_page_parents[child.id] = parent.id;
            if (sample == 2 || sample == 7) {
                for (auto &definition : catalogue.equipment)
                    if (definition.shop.kind == 1)
                        definition.name = "很长的装备测试名称";
                if (!text.include_text("很长的装备测试名称"))
                    throw std::runtime_error("Long child equipment font glyph missing");
            }
            const auto before = owner;
            const auto frame = ui::world_human_layout(extent);
            auto view = ui::world_human_view(owner, child);
            ui::prepare_world_human_detail(view, owner, skin);
            const auto controls = ui::world_human_painted_controls(view, frame, extent, skin);
            if (!view.detail_page_skin || !controls ||
                (info && (bool(controls->previous) != (sample != 0) ||
                          bool(controls->next) != (sample != 0))) ||
                (!info && !controls->primary))
                throw std::runtime_error("Child plan did not bind its actual painted controls");
            // Feed the coordinates actually produced by the source renderer, not guessed boxes.
            ui::WorldHumanInput pointer;
            const auto target = info ? controls->back : *controls->primary;
            pointer.click = Vector2{target.x + target.width / 2, target.y + target.height / 2};
            const auto action =
                ui::world_human_input(view, frame, pointer, false, controls, extent);
            if (!action || action->page != child.id ||
                action->action != (info ? simulation::StartupHumanPageAction::cancel
                                        : simulation::StartupHumanPageAction::confirm))
                throw std::runtime_error("Painted child control did not emit its bound intent");
            for (int draw = 0; draw < 8; ++draw) {
                if (WindowShouldClose())
                    throw std::runtime_error("Child renderer fixture was closed before capture");
                BeginDrawing();
                ClearBackground({145, 211, 247, 255});
                DrawText("SYNTHETIC child UI", 4, 4, 12, BLACK);
                ui::draw_world_human(view, frame, skin, true);
                text.flush(1, {});
                EndDrawing();
            }
            const auto file =
                output / ("synthetic-raw" + std::to_string(child.legacy_page) + "-" +
                          std::to_string(extent.width) + "-" + samples[sample] + ".png");
            capture(file);
            if (!same_world_clock(owner, before) || owner.page_counters != before.page_counters ||
                owner.scene.random.draws() != before.scene.random.draws() ||
                owner.scene.world.world.ai.accounting.funds() !=
                    before.scene.world.world.ai.accounting.funds() ||
                owner.shop_humans.at(human).equipment != before.shop_humans.at(human).equipment ||
                owner.human_page_answers != before.human_page_answers)
                throw std::runtime_error(
                    "Rendering a child changed funds, equipment, answer or tick");
            std::cout << "UI fixture / synthetic raw=" << child.legacy_page
                      << " sample=" << samples[sample] << " frames=8 screenshot=" << file.string()
                      << '\n';
        }
    }
    // Synthetic directory/attribute conditions use complete Owner payloads. They prove
    // drawing and pointer wiring, not natural unlock, payment or equipment acquisition.
    for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{720, 600}}) {
        SetWindowSize(extent.width, extent.height);
        for (int sample = 0; sample < 14; ++sample) {
            auto owner = initial_world();
            auto catalogue = *owner.rules;
            owner.rules = &catalogue;
            auto current = page;
            current.legacy_page = sample < 2 ? 61 : sample < 8 ? 64 : sample < 11 ? 68 : 69;
            owner.scripts.pages.push_back(current);
            owner.page_human_bindings[current.id] = human;
            owner.human_pages_initialized.insert(current.id);
            const int phase = sample < 2 ? sample : sample < 7 ? sample - 2 : sample == 7 ? 4 : 0;
            owner.page_phases[current.id] = phase;
            owner.page_counters[current.id] =
                sample < 8 ? 16
                           : std::array<int, 3>{0, sample < 11 ? 52 : 24, sample < 11 ? 73 : 45}.at(
                                 (sample - 8) % 3);
            owner.human_page_selections[current.id] = 0;
            owner.human_page_scroll[current.id] = 0;
            if (current.legacy_page == 64)
                owner.equipment_page_catalogs[current.id] = {};
            if (current.legacy_page == 61) {
                for (int job = 0; job < 8; ++job) {
                    owner.human_page_catalogs[current.id].push_back(job);
                    owner.scripts.professions.at(job).status = 1;
                    owner.scene.world.world.ai.growth.at(human).definition.profession_levels.at(
                        job) = job + 1;
                }
                catalogue.jobs.at(6).name = "名称较长的测试职业";
            } else if (current.legacy_page == 64 && sample != 7) {
                auto &list = owner.equipment_page_catalogs[current.id][phase];
                if (phase == 4) {
                    for (const auto &item : catalogue.items) {
                        list.push_back(item.identity);
                        owner.items.at(item.identity).inventory = 3;
                        owner.catalog.at({0, item.identity}) = owner.items.at(item.identity);
                        if (list.size() == 8)
                            break;
                    }
                } else {
                    const int kind = phase == 0 ? 1 : phase == 3 ? 3 : 2;
                    for (const auto &item : catalogue.equipment) {
                        if (item.shop.kind != kind ||
                            (kind == 2 && (item.shop.type == 2 ? 1 : 2) != phase))
                            continue;
                        list.push_back(item.shop.id);
                        auto &stock = owner.catalog.at({kind, item.shop.id});
                        stock.status = 3;
                        stock.free_purchases = list.size() == 7 ? 2 : 0;
                        stock.newly_unlocked = true;
                        if (list.size() == 8)
                            break;
                    }
                }
            } else if (current.legacy_page == 68) {
                owner.human_equipment_choices[current.id] = {0, 1};
                owner.equipment_attribute_display = {
                    {{100, 20, 30, 40}, {90, 25, 30, 42}, {-10, 5, 0, 2}}};
            } else if (current.legacy_page == 69) {
                const auto item = std::find_if(catalogue.items.begin(), catalogue.items.end(),
                                               [](const auto &d) { return d.effect == 2; });
                if (item == catalogue.items.end())
                    throw std::runtime_error("Attribute item fixture missing");
                owner.human_equipment_choices[current.id] = {4, item->identity};
                owner.human_attribute_display = {
                    {{100, 20, 30, 40, 50, 60}, {100, 20, 34, 40, 50, 60}, {0, 0, 4, 0, 0, 0}}};
            }
            if (sample < 7) {
                const auto count =
                    current.legacy_page == 61
                        ? owner.human_page_catalogs.at(current.id).size()
                        : owner.equipment_page_catalogs.at(current.id).at(phase).size();
                if (count > 6) {
                    owner.human_page_selections[current.id] = 6;
                    owner.human_page_scroll[current.id] = 3;
                }
            }
            if (!text.include_text("名称较长的测试职业"))
                throw std::runtime_error("Catalogue fixture font missing");
            const auto before = owner;
            const auto frame = ui::world_human_layout(extent);
            auto view = ui::world_human_view(owner, current);
            ui::prepare_world_human_detail(view, owner, skin);
            const auto controls = ui::world_human_painted_controls(view, frame, extent, skin);
            if (!view.detail_page_skin || !controls ||
                ((current.legacy_page == 61 || current.legacy_page == 64) && !controls->scroll))
                throw std::runtime_error("Directory/result fixture lost its measured controls");
            for (const auto &row : controls->rows) {
                ui::WorldHumanInput click;
                click.click = Vector2{row.rectangle.x + row.rectangle.width / 2,
                                      row.rectangle.y + row.rectangle.height / 2};
                ui::WorldHumanGesture gesture;
                const auto input =
                    ui::world_human_input(view, frame, click, false, controls, extent, &gesture);
                if (!input || !input->input || input->input->selection != row.index ||
                    input->input->confirm != (current.legacy_page == 61 || phase != 4))
                    throw std::runtime_error(
                        "Source-painted catalogue row lost select/confirm mapping");
            }
            for (int draw = 0; draw < 8; ++draw) {
                if (WindowShouldClose())
                    throw std::runtime_error("Directory fixture closed before capture");
                BeginDrawing();
                ClearBackground({145, 211, 247, 255});
                DrawText("SYNTHETIC catalogue/result UI", 4, 4, 12, BLACK);
                ui::draw_world_human(view, frame, skin, true);
                text.flush(1, {});
                EndDrawing();
            }
            const auto file =
                output / ("synthetic-human-management-" + std::to_string(extent.width) + "-" +
                          std::to_string(sample) + ".png");
            capture(file);
            if (!same_world_clock(owner, before) ||
                owner.human_page_scroll != before.human_page_scroll ||
                owner.page_counters != before.page_counters ||
                owner.scene.random.draws() != before.scene.random.draws() ||
                owner.human_attribute_display != before.human_attribute_display ||
                owner.equipment_attribute_display != before.equipment_attribute_display)
                throw std::runtime_error(
                    "Catalogue/result drawing changed cursor, reward or clock");
            std::cout << "UI fixture / synthetic raw=" << current.legacy_page
                      << " sample=" << sample << " frames=8 screenshot=" << file.string() << '\n';
        }
    }
    struct ResultSample {
        int raw, counter, condition;
        const char *name;
    };
    const ResultSample results[]{{62, 0, 0, "preview"},
                                 {62, 8, 1, "insufficient"},
                                 {62, 0, 2, "current"},
                                 {62, 0, 3, "long-job"},
                                 {63, 40, 0, "curtain"},
                                 {63, 55, 0, "changed"},
                                 {63, 197, 0, "reward"},
                                 {66, 13, 0, "throw"},
                                 {66, 25, 0, "receive"},
                                 {66, 45, 0, "open"},
                                 {66, 75, 0, "evaluation"},
                                 {66, 162, 0, "reward"},
                                 {66, 80, 1, "recovery-hp"},
                                 {66, 90, 1, "recovery-effect"},
                                 {66, 115, 1, "recovery-message"},
                                 {66, 162, 2, "magic-message"}};
    for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{720, 600}}) {
        SetWindowSize(extent.width, extent.height);
        for (const auto &sample : results) {
            auto owner = initial_world();
            auto catalogue = *owner.rules;
            owner.rules = &catalogue;
            auto &ai = owner.scene.world.world.ai;
            ai.growth.at(human).definition.current_profession = 0;
            ai.growth.at(human).definition.profession_levels[0] = 2;
            ai.growth.at(human).definition.profession_levels[1] = 5;
            owner.village_points = sample.raw == 62 && sample.condition == 1 ? 0 : 1000;
            owner.reward_display = {{{999, 9}, {999, 10}, {5, 1}}};
            owner.human_attribute_display = {{{100, 100, 100, 100, 100, 100},
                                              {99, 100, 101, 100, 100, 100},
                                              {-1, 0, 1, 0, 0, 0}}};
            auto parent = page;
            parent.legacy_page = sample.raw == 66 ? 64 : 61;
            parent.lifecycle = 3;
            owner.scripts.pages.push_back(parent);
            owner.page_human_bindings[parent.id] = human;
            owner.page_phases[parent.id] = owner.page_counters[parent.id] = 0;
            owner.human_page_selections[parent.id] = 0;
            owner.human_page_scroll[parent.id] = 0;
            owner.human_pages_initialized.insert(parent.id);
            owner.human_page_catalogs[parent.id] = {0, 1};
            auto detail = page;
            detail.id = 902;
            detail.legacy_page = sample.raw;
            owner.scripts.pages.push_back(detail);
            owner.page_human_bindings[detail.id] = human;
            owner.human_pages_initialized.insert(detail.id);
            owner.page_phases[detail.id] = sample.raw == 66 && sample.counter >= 135 ? 1 : 0;
            owner.page_counters[detail.id] = sample.counter;
            owner.human_page_selections[detail.id] =
                sample.raw == 62 && sample.condition != 2 ? 1 : 0;
            owner.human_page_catalogs[detail.id] = {0, 1};
            owner.human_page_parents[detail.id] = parent.id;
            owner.page_job_bindings[detail.id] = sample.raw == 62 && sample.condition == 2 ? 0 : 1;
            owner.human_gift_messages[detail.id] = "谢谢";
            owner.human_gift_scores[detail.id] = 90;
            owner.human_equipment_choices[detail.id] = {0, 1};
            owner.shop_humans.at(human).equipment[0] = 1;
            if (sample.raw == 63 && sample.counter >= 55)
                ai.growth.at(human).definition.current_profession = 1;
            if (sample.raw == 62 && sample.condition == 3) {
                catalogue.jobs.at(1).name = "名称较长的测试职业";
                if (!text.include_text(catalogue.jobs.at(1).name))
                    throw std::runtime_error("Long job font glyph missing");
            }
            if (sample.raw == 66 && sample.condition != 0) {
                const auto item = std::find_if(
                    catalogue.items.begin(), catalogue.items.end(), [&](const auto &d) {
                        return sample.condition == 1 ? d.recovery > 0 : d.effect == 9;
                    });
                if (item == catalogue.items.end())
                    throw std::runtime_error("Result needs a published recovery/magic item");
                owner.human_equipment_choices[detail.id] = {4, item->identity};
                if (sample.condition == 1) {
                    rules::BattleActorRecord actor;
                    actor.id = {900};
                    actor.kind = rules::ActorKind::human;
                    actor.definition = human;
                    actor.hp.displayed = 50;
                    actor.hp.target = 70;
                    actor.capacity = 100;
                    ai.battle.actors.emplace(actor.id, actor);
                    ai.human_order = {actor.id};
                    ai.growth.at(human).derived.combat[0] = 100;
                }
            }
            const auto before = owner;
            const auto frame = ui::world_human_layout(extent);
            auto view = ui::world_human_view(owner, detail);
            ui::prepare_world_human_detail(view, owner, skin);
            const auto controls = ui::world_human_painted_controls(view, frame, extent, skin);
            if (!view.detail_page_skin || !controls ||
                (sample.raw == 62 &&
                 bool(controls->primary) != (sample.condition != 1 && sample.condition != 2)))
                throw std::runtime_error("Result skin lost its change/confirmation controls");
            for (int draw = 0; draw < 8; ++draw) {
                if (WindowShouldClose())
                    throw std::runtime_error("Result fixture closed before capture");
                BeginDrawing();
                ClearBackground({145, 211, 247, 255});
                DrawText("SYNTHETIC result UI", 4, 4, 12, BLACK);
                ui::draw_world_human(view, frame, skin, true);
                text.flush(1, {});
                EndDrawing();
            }
            const auto file = output / ("synthetic-raw" + std::to_string(sample.raw) + "-" +
                                        std::to_string(extent.width) + "-" + sample.name + ".png");
            capture(file);
            if (!same_world_clock(owner, before) || owner.page_counters != before.page_counters ||
                owner.scene.random.draws() != before.scene.random.draws() ||
                owner.scene.world.world.ai.accounting.funds() !=
                    before.scene.world.world.ai.accounting.funds() ||
                owner.reward_display != before.reward_display ||
                owner.shop_humans.at(human).equipment != before.shop_humans.at(human).equipment)
                throw std::runtime_error("Result drawing changed rewards, equipment or clock");
            std::cout << "UI fixture / synthetic raw=" << sample.raw << " sample=" << sample.name
                      << " frames=8 screenshot=" << file.string() << '\n';
        }
    }
}
} // namespace ark::test
