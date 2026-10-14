// UI owns current-owner projection and action routing. Frozen rule/runtime suites own
// profession gates, gift rewards and tax posting; those transactions are not duplicated here.
#include "../../src/desktop/inspection/world_human_inspection.hpp"
#include "../../src/desktop/ui/actors/world_human.hpp"
#include "../../src/desktop/ui/actors/world_monster.hpp"
#include "../../src/desktop/ui/village/world_progression.hpp"
#include "../../src/desktop/ui/village/world_tax.hpp"
#include "../support/checks.hpp"
#include "../support/world_fixture.hpp"
#include "ark/simulation/facilities/startup_world_commerce.hpp"
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"
#include <algorithm>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace ui = desktop::ui;
using Page = sim::rules::WorldScriptPage;
Page human_page(sim::StartupWorldRuntimeState &state, int raw, bool ready = true) {
    Page page;
    page.kind = sim::rules::WorldScriptPageKind::raw_page;
    page.legacy_page = raw;
    page.id = 901;
    page.lifecycle = 1;
    state.scripts.pages.push_back(page);
    state.page_human_bindings[page.id] = 0;
    // Scene/house/task details carry source0 even in a read-only UI fixture;
    // the new runtime rejects missing context rather than inventing tracking access.
    if (raw == 60)
        state.human_detail_contexts.emplace(page.id, sim::StartupHumanDetailContext{});
    state.page_phases[page.id] = state.page_counters[page.id] =
        state.human_page_selections[page.id] = 0;
    if (ready)
        state.human_pages_initialized.insert(page.id);
    return page;
}
Vector2 middle(Rectangle r) { return {r.x + r.width / 2, r.y + r.height / 2}; }
bool inside(Rectangle outer, Rectangle inner) {
    return inner.x >= outer.x && inner.y >= outer.y &&
           inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}
// Input-only geometry fixture. GPU/font/source geometry is verified by the renderer case.
ui::WorldHumanPaintedControls painted_rows(const ui::WorldHumanView &v,
                                           const ui::WorldHumanLayout &l) {
    ui::WorldHumanPaintedControls p;
    p.page = v.page;
    p.raw = v.raw;
    p.phase = v.phase;
    p.selection = v.selection;
    p.human = v.human;
    p.choice = v.choice ? v.choice->identity : -1;
    p.extent = {240, 256};
    p.first = v.first_visible;
    p.count = int(v.rows.size());
    const int visible = v.raw == 60 ? 4 : v.visible_rows;
    for (int n = 0; n < visible && (v.raw == 60 || n + p.first < p.count); ++n) {
        const int index = p.first + n;
        p.rows.push_back({index,
                          v.raw == 60 ? index : v.rows[index].identity,
                          {l.rows.x, l.rows.y + n * l.row_height, l.rows.width, l.row_height}});
    }
    return p;
}
// Monster92 transport/layout uses one real entry; shared growth and numeric oracle stay below UI.
void monster_detail_ui(Checks &check) {
    sim::rules::EncounterCreationInput request;
    request.kind = 0;
    request.center = {10, 18};
    request.year_index = 0;
    request.month_index = 3;
    const auto spawned = sim::prepare_startup_world_runtime_encounter(initial_world(), request);
    check(spawned && !spawned->scene.world.world.ai.monster_order.empty(),
          "Monster UI fixture obtains an actual current encounter actor");
    auto owner = *spawned;
    const auto actor = owner.scene.world.world.ai.monster_order.front();
    check(sim::open_startup_world_monster_detail_page(owner, actor) ==
              sim::StartupWorldRuntimeError::none,
          "Monster UI fixture opens92 through the source actor entry");
    const auto pending = owner.scripts.pages.back();
    sim::SteamMonsterSkinOptions options;
    options.view_y = 23;
    options.title_widths = std::array<int, 2>{48, 48};
    options.name_width = 48; // Explicit measurement fixture, never a character-count estimate.
    check(ui::world_monster_detail_page(pending) &&
              !ui::world_monster_detail_input(owner, pending, true, false, false) &&
              !sim::steam_monster_detail_skin(owner, pending.id, options) &&
              !owner.page_counters.count(pending.id),
          "Pending92 drawing/input cannot initialize the source clock");
    const auto ready = sim::prepare_startup_world_runtime(owner);
    check(ready.candidate && ready.candidate->scripts.pages.back().lifecycle == 2,
          "Actual framework Init admits92 before the desktop can close it");
    owner = *ready.candidate;
    owner.page_counters.at(pending.id) = 0; // A zero-clock ready92 is explicitly source-qualified.
    const auto page = owner.scripts.pages.back();
    const auto before = owner;
    using Action = ui::WorldMonsterDetailAction;
    const auto confirm = ui::world_monster_detail_input(owner, page, true, false, false);
    const auto back = ui::world_monster_detail_input(owner, page, false, true, false);
    check(confirm && *confirm == Action::confirm && back && *back == Action::cancel &&
              !ui::world_monster_detail_input(owner, page, false, false, false) &&
              !ui::world_monster_detail_input(owner, page, true, true, true),
          "Ready92 immediately maps Enter/Back and has no coordinate-free mouse confirmation");
    const auto plan = sim::steam_monster_detail_skin(owner, page.id, options);
    const auto repeated = sim::steam_monster_detail_skin(owner, page.id, options);
    check(plan && repeated && plan->title == "怪物情报" &&
              plan->soft_labels == std::array<int, 2>{0, 2} &&
              plan->operations.size() == repeated->operations.size() &&
              owner.page_counters == before.page_counters && same_world_clock(owner, before) &&
              owner.sound_requests == before.sound_requests &&
              owner.scene.random.draws() == before.scene.random.draws() &&
              owner.scene.world.world.ai.accounting.funds() ==
                  before.scene.world.world.ai.accounting.funds(),
          "Repeated92 plan/input queries preserve clocks, cash, random and one-shot sound");
    auto paused = owner;
    paused.scene.framework_paused = true;
    check(!ui::world_monster_detail_input(paused, page, true, false, false),
          "Paused92 refuses Enter even without a separate blocked input flag");
    auto broken = owner;
    broken.page_counters.erase(page.id);
    check(!ui::world_monster_detail_input(broken, page, true, false, false) &&
              !sim::steam_monster_detail_skin(broken, page.id, options) &&
              !broken.page_counters.count(page.id),
          "Ready92 missing its clock refuses input/drawing without reconstructing it");
    auto covered = owner;
    covered.scripts.executing_page = page.id;
    Page overlay;
    overlay.kind = sim::rules::WorldScriptPageKind::raw_page;
    overlay.legacy_page = 16;
    overlay.legacy_l = 2;
    const auto inserted =
        sim::rules::prepare_world_script_page(sim::startup_world_runtime_scripts(covered), overlay);
    check(inserted.candidate &&
              sim::write_startup_world_runtime_scripts(covered, inserted.candidate->state) &&
              !ui::world_monster_detail_input(covered, page, false, true, false) &&
              !sim::steam_monster_detail_skin(covered, page.id, options),
          "Covered92 cannot close the newly allocated top page using an old input identity");
}
} // namespace

void world_human() {
    Checks check{"world_human"};
    monster_detail_ui(check);
    using Action = sim::StartupHumanPageAction;
    const auto layout = ui::world_human_layout({240, 256});
    auto state = initial_world();
    auto page = human_page(state, 60, false);
    const auto before = state;
    auto view = ui::world_human_view(state, page);
    ui::WorldHumanInput input;
    input.enter = true;
    check(!view.initialized && !ui::world_human_input(view, layout, input, false) &&
              state.human_pages_initialized.empty() && same_world_clock(state, before),
          "Uninitialized human pages cannot initialize, tick or confirm through presentation");
    state.human_pages_initialized.insert(page.id);
    state.shop_humans.at(0).satisfaction = 43;
    state.human_calendar.at(0).celebrations = 7;
    state.scene.world.world.ai.growth.at(0).derived.attributes = {101, 102, 103, 104, 105, 106};
    state.scene.world.world.ai.growth.at(0).derived.combat = {301, 302, 303, 304};
    view = ui::world_human_view(state, page);
    check(view.initialized && view.page == 901 && view.details.satisfaction == 43 &&
              view.details.medals == 7 && view.details.attributes[2] == 103 &&
              view.details.combat[0] == 301,
          "Human detail projects current shared values instead of creation metadata");
    check(!view.can_track, "Ordinary source0 details never expose directory tracking");
    {
        // Display-only snapshots: source suites own cycle arithmetic and HP thresholds.
        // This boundary verifies that the desktop actually carries the complete source plan.
        auto portrait = state;
        portrait.page_counters.at(page.id) = 100;
        portrait.human_homes.at(0)[2] = 0;
        portrait.scene.world.world.ai.growth.at(0).definition.current_profession = 2;
        portrait.shop_humans.at(0).equipment[0] = 0;
        const auto walking = ui::world_human_view(portrait, page);
        check(walking.detail_skin &&
                  walking.detail_skin->portrait_position == std::array<int, 2>{63, 121} &&
                  walking.detail_skin->portrait.body.frame == 1 &&
                  walking.detail_skin->portrait.body.image ==
                      portrait.rules->jobs.at(2).sprites.at(walking.details.sex) &&
                  walking.detail_skin->portrait.weapon && !walking.detail_skin->portrait.shadow,
              "Desktop retains current job/sex/weapon, nonresident offset and independent cycles");
        check(walking.detail_skin->medals.draws.size() == 3 &&
                  std::get<sim::SteamHumanImage>(walking.detail_skin->medals.draws[1]).asset ==
                      sim::SteamHumanAsset::number05,
              "Seven medals carry the source image, multiplication frame and Steam number plan");
        auto &ai = portrait.scene.world.world.ai;
        sim::rules::BattleActorRecord live;
        live.id = {900};
        live.kind = sim::rules::ActorKind::human;
        live.definition = 0;
        live.hp.displayed = 80;
        live.hp.target = 0;
        ai.battle.actors.emplace(live.id, live);
        ai.human_order = {live.id};
        ai.growth.at(0).derived.combat[0] = 100;
        portrait.page_counters.at(page.id) = 0;
        const auto danger = ui::world_human_view(portrait, page);
        check(
            danger.live && danger.live->id == live.id && danger.detail_skin &&
                danger.detail_skin->after_portrait.draws.size() == 5 &&
                std::get<sim::StartupSkinRect>(danger.detail_skin->after_portrait.draws[2])
                        .rect[2] == 14 &&
                std::get<sim::SteamHumanImage>(danger.detail_skin->after_portrait.draws[4]).asset ==
                    sim::SteamHumanAsset::pinch,
            "Desktop preserves distinct display HP bar and target HP danger requests");
        ai.battle.actors.at(live.id).control.state = 7;
        const auto input_only = ui::world_human_view(portrait, page);
        check(input_only.initialized && !input_only.detail_skin,
              "Down input projection stays usable without guessing a font width");
        const auto down = ui::world_human_view(portrait, page, 36);
        check(down.detail_skin && down.detail_skin->portrait.body.sprite == 13 &&
                  down.detail_skin->portrait.body.frame == 0 &&
                  !down.detail_skin->portrait.weapon &&
                  std::holds_alternative<sim::SteamHumanText>(
                      down.detail_skin->after_portrait.draws.back()),
              "Measured down view replaces walk/weapon/HP with body13/frame0 and bubble text");
        check(portrait.page_counters.at(page.id) == 0 &&
                  ai.battle.actors.at(live.id).hp.displayed == 80 &&
                  ai.battle.actors.at(live.id).hp.target == 0 &&
                  same_world_clock(portrait, state) &&
                  portrait.scene.random.draws() == state.scene.random.draws(),
              "Repeated UI projection does not advance source counters, HP or random");
    }
    {
        auto tracking = view;
        ui::WorldHumanInput chase;
        chase.click = middle(ui::world_human_detail_layout(layout).track);
        check(!ui::world_human_input(view, layout, chase, false),
              "Invisible tracking control has no active source0 hit region");
        tracking.can_track = true; // Source eligibility itself belongs to the Owner suite.
        const auto action = ui::world_human_input(tracking, layout, chase, false);
        check(action && action->action == Action::track &&
                  !ui::world_human_input(tracking, layout, chase, true),
              "Source1 tracking emits one FIFO intent and blocks duplicates while pending");
    }
    check(ui::world_human_input(view, layout, input, false)->action == Action::gifts &&
              !ui::world_human_input(view, layout, input, true),
          "Overview Enter opens actual gifts and blocks input while an earlier command is pending");
    // Formal Steam page0/1 Enter routes to the inline command, not source confirm/close.
    for (int phase = 0; phase < 2; ++phase) {
        auto detail = view;
        detail.phase = phase;
        const auto expected = phase == 0 ? Action::gifts : Action::professions;
        for (bool pointer : {false, true}) {
            ui::WorldHumanInput command;
            command.enter = !pointer;
            if (pointer)
                command.click = middle(ui::world_human_detail_layout(layout).primary);
            const auto intent = ui::world_human_input(detail, layout, command, false);
            check(intent && intent->action == expected && intent->page == view.page &&
                      !ui::world_human_input(detail, layout, command, true),
                  "Inline/Enter command binds its current phase and rejects pending duplicate");
        }
        for (bool previous : {false, true}) {
            ui::WorldHumanInput navigation;
            navigation.up = previous;
            navigation.down = !previous;
            const auto intent = ui::world_human_input(detail, layout, navigation, false);
            check(intent && intent->action == (previous ? Action::previous : Action::next),
                  "Human vertical navigation uses source page actions without mutating the Owner");
        }
    }
    input = {};
    input.escape = true;
    check(ui::world_human_input(view, layout, input, false)->action == Action::cancel,
          "Human detail Back is routed to source cancel");
    {
        auto equipment = view;
        equipment.phase = 2;
        equipment.selection = 2;
        ui::WorldHumanInput enter;
        enter.enter = true;
        check(ui::world_human_input(equipment, layout, enter, false)->action == Action::gifts &&
                  !ui::world_human_input(equipment, layout, enter, true),
              "Equipment Enter opens Owner-selected gift slot and rejects pending duplicates");
        const auto rows = ui::world_human_detail_layout(layout).body;
        auto painted = painted_rows(equipment, layout);
        for (int slot = 0; slot < 4; ++slot)
            painted.rows[slot].rectangle = {rows.x, rows.y + slot * 17, rows.width, 17};
        for (int slot = 0; slot < 4; ++slot) {
            ui::WorldHumanInput click;
            click.click = middle({rows.x, rows.y + slot * 17, rows.width, 17});
            const auto intent =
                ui::world_human_input(equipment, layout, click, false, painted, {240, 256});
            check(intent && intent->action == Action::select && intent->selection == slot &&
                      !ui::world_human_input(equipment, layout, click, true),
                  "Visible equipment row selects its source slot without opening or spending");
        }
        ui::WorldHumanInput inspect;
        inspect.inspect = true;
        check(ui::world_human_input(equipment, layout, inspect, false)->selection == 2,
              "Explicit equipment inspection retains selected second-armour slot");
        equipment.phase = 3;
        check(!ui::world_human_input(equipment, layout, enter, false),
              "Magic Enter has no source action and cannot accidentally close the page");
    }
    for (int tab = 0; tab < 4; ++tab) {
        input = {};
        input.click = middle(ui::world_human_detail_layout(layout).tabs[tab]);
        const auto intent = ui::world_human_input(view, layout, input, false);
        check(intent && intent->input && intent->input->scroll && intent->input->scroll->absolute &&
                  intent->input->scroll->value == tab,
              "Each human detail tab targets its exact source phase");
    }
    input = {};
    input.professions = true;
    check(ui::world_human_input(view, layout, input, false)->action == Action::professions,
          "Profession shortcut opens the source profession page");
    input = {};
    input.gifts = true;
    check(ui::world_human_input(view, layout, input, false)->action == Action::gifts,
          "Gift shortcut opens the source equipment page");
    page.legacy_page = state.scripts.pages.back().legacy_page = 61;
    state.human_page_catalogs[page.id] = {2, 0, 1};
    state.human_page_selections[page.id] = 1;
    state.human_page_scroll[page.id] = 0;
    page.lifecycle = state.scripts.pages.back().lifecycle = 0;
    check(
        !ui::world_human_view(state, page).initialized &&
            state.human_pages_initialized.count(page.id),
        "Initialized catalogue payload is not drawable before the source page lifecycle advances");
    page.lifecycle = state.scripts.pages.back().lifecycle = 1;
    view = ui::world_human_view(state, page);
    check(view.rows.size() == 3 && view.rows[0].identity == 2 && view.rows[1].identity == 0 &&
              view.choice->identity == 0,
          "Profession catalogue retains source ordering and selection separately from IDs");
    input = {};
    input.click = middle({layout.rows.x, layout.rows.y, layout.rows.width, layout.row_height});
    auto intent =
        ui::world_human_input(view, layout, input, false, painted_rows(view, layout), {240, 256});
    check(intent && intent->input && intent->input->selection == 0 && intent->input->confirm &&
              state.human_page_selections.at(page.id) == 1,
          "Profession UP maps to one selected-row confirmation without mutating the Owner");
    state.human_page_answers[page.id] = 0;
    check(!ui::world_human_view(state, page).initialized,
          "Resumed catalogue waits until the Owner consumes its child answer");
    state.human_page_answers.clear();
    page.legacy_page = state.scripts.pages.back().legacy_page = 62;
    state.human_attribute_display[0] = {1, 2, 3, 4, 5, 6};
    state.human_attribute_display[1] = {7, 8, 9, 10, 11, 12};
    view = ui::world_human_view(state, page);
    input = {};
    input.right = true;
    check(view.attributes[0][5] == 6 && view.attributes[1][5] == 12 &&
              ui::world_human_input(view, layout, input, false)->action == Action::next,
          "Profession preview reads Owner comparison and permits source target cycling");
    page.legacy_page = state.scripts.pages.back().legacy_page = 63;
    state.page_job_bindings[page.id] = 1;
    state.human_gift_messages[page.id] = "谢谢";
    auto &definition = state.scene.world.world.ai.growth.at(0).definition;
    definition.current_profession = 0;
    definition.profession_levels.at(0) = 2;
    definition.profession_levels.at(1) = 7;
    state.page_counters[page.id] = 54;
    view = ui::world_human_view(state, page);
    check(view.profession == state.rules->jobs.at(0).name && view.details.level == 2 &&
              view.target_profession == state.rules->jobs.at(1).name &&
              view.profession != view.target_profession && definition.current_profession == 0,
          "Before profession setter 55, header pairs the old job with its level while body names "
          "the target");
    // Model only the committed snapshot after the source setter; drawing never performs it.
    definition.current_profession = 1;
    state.page_counters[page.id] = 55;
    view = ui::world_human_view(state, page);
    check(view.profession == state.rules->jobs.at(1).name && view.details.level == 7 &&
              view.target_profession == state.rules->jobs.at(1).name,
          "After the source setter, header switches job and its own level together");
    input = {};
    input.enter = true;
    state.page_counters[page.id] = 196;
    check(!ui::world_human_input(ui::world_human_view(state, page), layout, input, false),
          "Profession transition cannot close before source counter 197");
    state.page_counters[page.id] = 197;
    check(ui::world_human_input(ui::world_human_view(state, page), layout, input, false)->action ==
              Action::confirm,
          "Profession transition confirmation opens at source counter 197");
    page.legacy_page = state.scripts.pages.back().legacy_page = 64;
    state.page_phases[page.id] = 0;
    state.human_page_selections[page.id] = 0;
    state.equipment_page_catalogs[page.id][0] = {1, 0};
    state.catalog.at({1, 1}).free_purchases = 2;
    view = ui::world_human_view(state, page);
    check(view.rows[0].identity == 1 && view.rows[1].identity == 0 && view.rows[0].cost == -1 &&
              view.rows[0].stock == 2,
          "Equipment preserves source order and shared stock quote, never displays a negative cash "
          "price");
    for (int slot = 0; slot < 5; ++slot) {
        input = {};
        input.right = true;
        intent = ui::world_human_input(view, layout, input, false);
        check(intent && intent->input && intent->input->right && !intent->input->selection,
              "Gift tab input delegates five-namespace navigation to the source Owner");
    }
    const int ordinary = state.rules->items.at(5).identity;
    state.equipment_page_catalogs[page.id][4] = {ordinary};
    state.items.at(ordinary).inventory = 3;
    state.catalog.at({0, ordinary}) = state.items.at(ordinary);
    state.page_phases[page.id] = 4;
    const auto item_before = state;
    view = ui::world_human_view(state, page);
    check(view.choice && view.choice->icon.size() == 2 && view.choice->icon[1].image == 9 &&
              view.choice->icon[1].crop == std::array<int, 4>{112, 16, 16, 16},
          "Ordinary item5 keeps source icon22 in the gift list instead of equipment artwork");
    check(view.choice && view.choice->slot == 4 && view.choice->identity == ordinary &&
              view.choice->stock == 3 && view.choice->cost == -1 &&
              view.choice->name == state.rules->items.at(5).name,
          "Fifth gift tab reads owned item z, never equipment armor or free-purchase stock");
    {
        auto painted = painted_rows(view, layout);
        ui::WorldHumanGesture marker;
        ui::WorldHumanInput click;
        click.click = middle(painted.rows.front().rectangle);
        const auto first =
            ui::world_human_input(view, layout, click, false, painted, {240, 256}, &marker);
        const auto pending =
            ui::world_human_input(view, layout, click, true, painted, {240, 256}, &marker);
        const auto second =
            ui::world_human_input(view, layout, click, false, painted, {240, 256}, &marker);
        check(first && first->input && first->input->selection == 0 && !first->input->confirm &&
                  !pending && second && second->input->confirm,
              "Ordinary gift first UP marks, pending blocks, same committed key then confirms");
        ui::WorldHumanInput keys;
        keys.up = keys.down = keys.enter = keys.escape = true;
        const auto combined =
            ui::world_human_input(view, layout, keys, false, painted, {240, 256}, &marker);
        check(combined && combined->input->up && combined->input->down &&
                  combined->input->confirm && combined->input->cancel &&
                  marker.marked_identity == -1,
              "Simultaneous keys reach the source intact and clear pointer confirmation marker");
        ++painted.first;
        check(!ui::world_human_input(view, layout, click, false, painted, {240, 256}, &marker),
              "Stale catalogue scroll geometry cannot select or confirm a gift");
        auto empty = view;
        empty.rows.clear();
        empty.choice.reset();
        empty.can_confirm = false;
        ui::WorldHumanInput switch_and_confirm;
        switch_and_confirm.right = switch_and_confirm.enter = true;
        const auto next_tab = ui::world_human_input(empty, layout, switch_and_confirm, false);
        check(next_tab && next_tab->input->right && next_tab->input->confirm,
              "Empty old gift tab cannot suppress source confirmation after same-frame tab "
              "navigation");
        auto long_list = view;
        long_list.rows.clear();
        for (int n = 0; n < 8; ++n) {
            auto row = *view.choice;
            row.identity = state.rules->items.at(n).identity;
            long_list.rows.push_back(row);
        }
        auto track = painted_rows(long_list, layout);
        track.scroll = Rectangle{215, 85, 5, 110};
        track.scroll_count = 8;
        track.scroll_visible = 4;
        ui::WorldHumanInput drag;
        drag.pointer = Vector2{217, 194};
        drag.pointer_pressed = drag.pointer_down = true;
        const auto scrolled =
            ui::world_human_input(long_list, layout, drag, false, track, {240, 256}, &marker);
        check(scrolled && scrolled->input->scroll && scrolled->input->scroll->absolute &&
                  scrolled->input->scroll->value == 4 && marker.scrolling &&
                  long_list.first_visible == 0 && state.human_page_scroll.at(page.id) == 0,
              "Painted track maps to a source absolute-scroll input without a second cursor");
        drag.pointer_pressed = drag.pointer_down = false;
        drag.pointer_released = true;
        check(
            !ui::world_human_input(long_list, layout, drag, true, track, {240, 256}, &marker) &&
                !marker.scrolling,
            "Releasing the scrollbar during pending FIFO clears capture without submitting input");
    }
    input = {};
    input.inspect = true;
    check(!ui::world_human_input(view, layout, input, false),
          "Ordinary items cannot open equipment-only raw73");
    input = {};
    input.right = true;
    check(ui::world_human_input(view, layout, input, false)->selection == 0,
          "Gift keyboard navigation wraps from the fifth tab to weapon");
    input = {};
    input.enter = true;
    check(ui::world_human_input(view, layout, input, false)->action == Action::confirm &&
              state.items.at(ordinary).inventory == 3 &&
              state.scene.random.draws() == item_before.scene.random.draws() &&
              same_world_clock(state, item_before),
          "Ordinary gift confirmation emits an intent without consuming stock or drawing dialogue");
    page.legacy_page = state.scripts.pages.back().legacy_page = 66;
    state.human_equipment_choices[page.id] = {4, ordinary};
    state.human_gift_scores[page.id] = 51;
    state.human_gift_messages[page.id] = "谢谢";
    view = ui::world_human_view(state, page);
    check(view.choice && view.choice->slot == 4 &&
              view.choice->name == state.rules->items.at(5).name && view.gift_score == 51 &&
              !view.can_cancel,
          "Ordinary gift evaluation uses its committed item binding and source message");
    page.legacy_page = state.scripts.pages.back().legacy_page = 69;
    state.human_attribute_display = {
        {{10, 20, 30, 40, 50, 60}, {10, 22, 30, 40, 50, 60}, {0, 2, 0, 0, 0, 0}}};
    check(ui::world_human_page(page), "Ordinary attribute result69 belongs to the human UI");
    for (int counter : {0, 38, 39, 44, 45}) {
        state.page_counters[page.id] = counter;
        view = ui::world_human_view(state, page);
        input = {};
        input.enter = true;
        check(view.attributes == state.human_attribute_display &&
                  bool(ui::world_human_input(view, layout, input, false)) ==
                      (counter < 39 || counter >= 45),
              "Ordinary item result reads six attributes and respects the39-through44 confirmation "
              "gap");
        input = {};
        input.escape = true;
        check(!ui::world_human_input(view, layout, input, false),
              "Ordinary item attribute result has no cancel path");
    }
    page.legacy_page = state.scripts.pages.back().legacy_page = 73;
    check(!ui::world_human_view(state, page).initialized,
          "Equipment-only inspection refuses a fifth ordinary-item phase");
    state.page_phases[page.id] = 0;
    page.legacy_page = state.scripts.pages.back().legacy_page = 65;
    state.human_equipment_choices[page.id] = {0, 1};
    view = ui::world_human_view(state, page);
    input = {};
    input.escape = true;
    check(view.choice && view.choice->identity == 1 &&
              ui::world_human_input(view, layout, input, false)->action == Action::cancel &&
              state.catalog.at({1, 1}).free_purchases == 2,
          "Gift confirmation reads its bound equipment and cancel leaves stock untouched");
    page.legacy_page = state.scripts.pages.back().legacy_page = 66;
    state.human_gift_scores[page.id] = 73;
    state.human_gift_messages[page.id] = "太好了!";
    view = ui::world_human_view(state, page);
    check(view.gift_score == 73 && view.message == "太好了!" && !view.can_cancel,
          "Gift evaluation uses committed score and dialogue, without a redraw random draw");
    page.legacy_page = state.scripts.pages.back().legacy_page = 68;
    state.equipment_attribute_display[0] = {10, 20, 30, 40};
    state.equipment_attribute_display[1] = {11, 22, 33, 44};
    view = ui::world_human_view(state, page);
    check(view.combat[0][0] == 10 && view.combat[1][3] == 44 && !view.can_cancel,
          "Equipment comparison displays the committed four combat values, not six attributes");
    page.legacy_page = state.scripts.pages.back().legacy_page = 73;
    view = ui::world_human_view(state, page);
    input = {};
    input.enter = true;
    check(view.choice && view.choice->identity == 1 &&
              ui::world_human_input(view, layout, input, false)->action == Action::confirm,
          "Read-only equipment details close through their own source confirm action");
    input = {};
    input.gifts = true;
    check(!ui::world_human_input(view, layout, input, false),
          "Equipment inspection cannot use the gift shortcut to apply or buy equipment");
    {
        // Mastery has no gift parent/catalogue payload. A legitimate independent Owner
        // fixture verifies PC wiring; the source suite owns bonus and successor transactions.
        auto mastery = initial_world();
        auto master_page = human_page(mastery, 70);
        master_page.lifecycle = mastery.scripts.pages.back().lifecycle = 2;
        mastery.page_secondary_counters[master_page.id] = 23;
        mastery.page_phases[master_page.id] = 2;
        const auto master = ui::world_human_view(mastery, master_page);
        check(master.initialized && master.raw == 70 && master.selection == 0 &&
                  master.mastery_attribute ==
                      mastery.rules->jobs.at(master.details.profession).mastery_attribute &&
                  ui::world_human_source_child(master),
              "Mastery projects a legitimate bound Owner and uses the source child renderer");
        ui::WorldHumanPaintedControls painted;
        painted.page = master.page;
        painted.raw = master.raw;
        painted.phase = master.phase;
        painted.selection = master.selection;
        painted.human = master.human;
        painted.extent = {240, 256};
        painted.mastery_buttons = {Rectangle{30, 154, 88, 56}, Rectangle{104, 154, 112, 56}};
        for (int choice = 0; choice < 2; ++choice) {
            ui::WorldHumanInput click;
            click.click = middle(*painted.mastery_buttons[choice]);
            const auto command =
                ui::world_human_input(master, layout, click, false, painted, {240, 256});
            check(command && command->input && command->input->selection == choice &&
                      command->input->confirm && command->action == Action::confirm &&
                      !ui::world_human_input(master, layout, click, true, painted, {240, 256}) &&
                      !ui::world_human_input(master, layout, click, false, {}, {240, 256}),
                  "Mastery source button selects and confirms in one frame, rejecting "
                  "pending/unpainted hits");
        }
        ui::WorldHumanInput overlap;
        overlap.click = Vector2{110, 175};
        check(ui::world_human_input(master, layout, overlap, false, painted, {240, 256})
                      ->input->selection == 1,
              "Overlapping source touches retain the established last-registered PC hit order");
        for (int mismatch = 0; mismatch < 6; ++mismatch) {
            auto stale = painted;
            if (mismatch == 0)
                ++stale.page;
            else if (mismatch == 1)
                ++stale.phase;
            else if (mismatch == 2)
                ++stale.selection;
            else if (mismatch == 3)
                ++stale.human;
            else if (mismatch == 4)
                ++stale.extent.width;
            else
                ++stale.extent.height;
            check(!ui::world_human_input(master, layout, overlap, false, stale, {240, 256}),
                  "Mastery rejects a stale page, phase, selection, person or viewport");
        }
        ui::WorldHumanInput combined;
        combined.left = combined.right = combined.enter = true;
        const auto command = ui::world_human_input(master, layout, combined, false);
        check(command && command->input && command->input->left && command->input->right &&
                  command->input->confirm && !command->input->up && !command->input->cancel,
              "Mastery preserves simultaneous horizontal input and confirmation for the Owner");
        for (int phase = 0; phase < 2; ++phase) {
            auto introduction = master;
            introduction.phase = phase;
            auto drawn = painted;
            drawn.phase = phase;
            drawn.mastery_buttons = {};
            drawn.primary = Rectangle{180, 224, 50, 18};
            ui::WorldHumanInput click;
            click.click = middle(*drawn.primary);
            const auto confirm =
                ui::world_human_input(introduction, layout, click, false, drawn, {240, 256});
            check(confirm && confirm->input && confirm->input->confirm &&
                      !confirm->input->selection,
                  "Mastery introduction/reward footer confirms without inventing a choice");
        }
        for (int unsupported = 0; unsupported < 3; ++unsupported) {
            ui::WorldHumanInput key;
            key.escape = unsupported == 0;
            key.up = unsupported == 1;
            key.down = unsupported == 2;
            check(!ui::world_human_input(master, layout, key, false),
                  "Steam mastery does not infer Esc or vertical navigation from APK Back2");
        }
    }
    page.legacy_page = 67;
    check(!ui::world_human_page(page),
          "Effort raw67 remains owned by the existing progression view");
    for (int raw : {67, 88, 96}) {
        // Constructor timing only: clocks can be visible before the source callback is
        // admitted. This fixture does not stand in for earning the actual reward.
        auto owner = initial_world();
        Page ceremony;
        ceremony.id = owner.scripts.next_page_id++;
        ceremony.kind = sim::rules::WorldScriptPageKind::raw_page;
        ceremony.legacy_page = raw;
        owner.scripts.pages.push_back(ceremony);
        owner.page_human_bindings[ceremony.id] = 1;
        owner.page_counters[ceremony.id] = owner.page_phases[ceremony.id] = 0;
        for (int lifecycle : {0, 1, 2}) {
            auto &live = owner.scripts.pages.back();
            live.lifecycle = lifecycle;
            const auto v = ui::world_progression_view(owner, live);
            check(v.initialized == (lifecycle == 2),
                  "Human ceremonies wait for the source Start before accepting confirmation");
            ui::WorldProgressionInput enter;
            enter.enter = true;
            if (lifecycle != 2)
                check(!ui::world_progression_input(v, ui::world_progression_layout({240, 256}, raw),
                                                   enter, false),
                      "Early Enter cannot reach a ceremony callback before lifecycle2");
        }
    }
    {
        // Explicit input-boundary fixture: the real84 consumer creates86, whose first
        // update retires it. The diagnostic must recognize it before that update.
        auto purchase = initial_world();
        purchase.scripts.pages.clear();
        Page catalogue;
        catalogue.id = purchase.scripts.next_page_id++;
        catalogue.kind = sim::rules::WorldScriptPageKind::raw_page;
        catalogue.legacy_page = 84;
        catalogue.lifecycle = 1;
        purchase.scripts.pages.push_back(catalogue);
        purchase.shop_item_stock.at(0).quantity = 2;
        check(sim::initialize_startup_world_commerce_pages(purchase),
              "Receipt boundary fixture initializes real commerce catalogue");
        desktop::WorldHumanInspection diagnostic;
        check(!desktop::human_inspection_ready(purchase, "world-commerce-receipt", diagnostic),
              "Uncommitted purchase is not a captured receipt");
        check(desktop::apply_human_inspection_input(purchase, "world-commerce-receipt", diagnostic),
              "Receipt diagnostic issues the actual selected purchase command");
        check(diagnostic.commerce_bought &&
                  desktop::human_inspection_ready(purchase, "world-commerce-receipt", diagnostic),
              "Actual receipt is observable immediately after84 before source initialization");
        const auto receipt = purchase.scripts.pages.back().id;
        check(!purchase.commerce_pages_initialized.count(receipt),
              "Diagnostic does not initialize transient receipt to manufacture a display frame");
        const auto after = sim::update_startup_world_commerce_page(purchase, receipt);
        check(after &&
                  !desktop::human_inspection_ready(*after, "world-commerce-receipt", diagnostic),
              "First original86 update retires the receipt without a second purchase");
    }
    {
        // Only painted controls can emit pointer intents for source child windows.
        // Their geometry is exercised by the renderer fixture; this suite owns stale-input gates.
        ui::WorldHumanView child;
        child.raw = 65;
        child.page = 902;
        child.human = 0;
        child.initialized = child.can_cancel = child.can_confirm = true;
        child.choice = ui::WorldHumanRow{};
        child.choice->identity = 1;
        ui::WorldHumanPaintedControls painted;
        painted.page = child.page;
        painted.raw = child.raw;
        painted.human = child.human;
        painted.choice = 1;
        painted.extent = {240, 256};
        painted.primary = Rectangle{90, 180, 60, 16};
        painted.back = {180, 208, 50, 18};
        ui::WorldHumanInput pointer;
        pointer.click = middle(*painted.primary);
        const auto command =
            ui::world_human_input(child, layout, pointer, false, painted, {240, 256});
        check(command && command->page == child.page && command->action == Action::confirm &&
                  !ui::world_human_input(child, layout, pointer, true, painted, {240, 256}) &&
                  !ui::world_human_input(child, layout, pointer, false, {}, {240, 256}),
              "Gift confirmation requires the last painted control and blocks pending repeats");
        for (int mismatch = 0; mismatch < 8; ++mismatch) {
            auto stale = painted;
            switch (mismatch) {
            case 0:
                ++stale.page;
                break;
            case 1:
                ++stale.raw;
                break;
            case 2:
                ++stale.phase;
                break;
            case 3:
                ++stale.selection;
                break;
            case 4:
                ++stale.human;
                break;
            case 5:
                ++stale.choice;
                break;
            case 6:
                ++stale.extent.width;
                break;
            case 7:
                ++stale.extent.height;
                break;
            }
            check(!ui::world_human_input(child, layout, pointer, false, stale, {240, 256}),
                  "Stale page, slot, selection, person, candidate or viewport rejects pointer");
            ui::WorldHumanInput key;
            key.enter = true;
            check(ui::world_human_input(child, layout, key, false, stale, {240, 256})->action ==
                      Action::confirm,
                  "Keyboard confirmation remains independent of the previous draw cache");
        }
        pointer.click = {painted.primary->x - 1, painted.primary->y};
        check(!ui::world_human_input(child, layout, pointer, false, painted, {240, 256}),
              "Gift window background is not an extra confirmation button");
        child.raw = painted.raw = 73;
        child.rows = {*child.choice, *child.choice};
        painted.count = 2;
        painted.other_back = {10, 208, 50, 18};
        painted.previous = Rectangle{40, 65, 18, 18};
        painted.next = Rectangle{182, 65, 18, 18};
        for (const bool previous : {false, true}) {
            pointer.click = middle(previous ? *painted.previous : *painted.next);
            check(
                ui::world_human_input(child, layout, pointer, false, painted, {240, 256})->action ==
                    (previous ? Action::previous : Action::next),
                "Equipment title arrows emit catalogue navigation without selecting a slot");
        }
        for (const auto back : {painted.back, painted.other_back}) {
            pointer.click = middle(back);
            check(
                ui::world_human_input(child, layout, pointer, false, painted, {240, 256})->action ==
                    Action::cancel,
                "Both equipment soft-label returns close through source cancel");
        }
        ui::WorldHumanInput up;
        up.up = true;
        check(!ui::world_human_input(child, layout, up, false, painted, {240, 256}),
              "Equipment child does not inherit unsupported vertical list navigation");
        child.raw = painted.raw = 62;
        pointer.click = middle(*painted.next);
        check(ui::world_human_input(child, layout, pointer, false, painted, {240, 256})->action ==
                  Action::next,
              "Profession preview title arrow cycles the source candidate catalogue");
        painted.primary.reset();
        pointer.click = middle({90, 180, 60, 16});
        check(!ui::world_human_input(child, layout, pointer, false, painted, {240, 256}),
              "Current job or insufficient-points label is not a painted change button");
        for (const int raw : {63, 66}) {
            child.raw = painted.raw = raw;
            child.can_cancel = false;
            child.can_confirm = raw == 66;
            painted.primary = Rectangle{180, 208, 50, 18};
            pointer.click = middle(*painted.primary);
            check(bool(ui::world_human_input(child, layout, pointer, false, painted, {240, 256})) ==
                      child.can_confirm,
                  "Result footer obeys current Owner confirmation/fast-forward eligibility");
            child.can_confirm = true;
            check(
                ui::world_human_input(child, layout, pointer, false, painted, {240, 256})->action ==
                    Action::confirm,
                "Result footer confirms through its actual painted area once eligible");
        }
    }
    for (const auto extent :
         {desktop::Extent{240, 256}, desktop::Extent{384, 256}, desktop::Extent{1080, 720}}) {
        const auto frame = ui::world_human_layout(extent);
        check(
            inside({0, 24, static_cast<float>(extent.width), extent.height - 53.F}, frame.panel) &&
                frame.row_height >= 14 &&
                frame.body.y + frame.body.height <= frame.panel.y + frame.panel.height - 49,
            "Human panel clears HUD/footer with readable five-row lists and separate feedback");
        for (const auto box : {frame.body, frame.cancel, frame.confirm, frame.previous, frame.next,
                               frame.professions, frame.gifts, frame.inspect})
            check(inside(frame.panel, box), "Human controls fit minimum and large viewports");
        const auto detail = ui::world_human_detail_layout(frame);
        check(
            inside({0, 0, float(extent.width), float(extent.height)}, detail.panel) &&
                inside(detail.panel, detail.body),
            "Steam static human details preserve the source body inside every supported viewport");
        for (const auto box : detail.tabs)
            check(inside({0, 0, float(extent.width), float(extent.height)}, box),
                  "PC human page tabs stay inside the minimum viewport");
    }
}

void world_tax() {
    Checks check{"world_tax"};
    auto state = initial_world();
    auto page = human_page(state, 90);
    const auto layout = ui::world_tax_layout({240, 256});
    check(!ui::world_tax_view(state, page).initialized && state.tax_page_residents.empty(),
          "Tax drawing waits for source list initialization without freezing a list itself");
    state.tax_page_residents[page.id] = {5, 2, 4, 1, 0, 3};
    state.tax_page_selection[page.id] = 5;
    state.tax_page_scroll[page.id] = 1;
    page.lifecycle = state.scripts.pages.back().lifecycle = 2;
    for (int id = 0; id < 6; ++id)
        state.human_calendar.at(id).legacy_G = 10 + id;
    const auto before = state;
    auto view = ui::world_tax_view(state, page);
    check(
        view.initialized && view.rows.size() == 6 && view.rows[0].identity == 5 &&
            view.rows[1].identity == 2 && view.rows[0].amount == 15 && view.total == 75 &&
            view.first_visible == 1 && view.selection == 5,
        "Tax shows source frozen order, current G amounts, exact total and source five-row scroll");
    sim::SteamInformationSkinOptions options;
    options.view_y = 23;
    options.title_widths = std::array<int, 2>{48, 48};
    view.skin = sim::steam_tax_information_skin(state, page.id, options);
    auto controls = ui::world_tax_painted_controls(view, layout);
    check(
        view.skin && controls && controls->rows.size() == 5 && controls->scroll &&
            controls->confirm.width == 0 && controls->cancel.width == 0,
        "Source tax plan supplies five no-marker rows and scrollbar, without a fabricated button");
    ui::WorldFacilityGesture gesture;
    ui::WorldTaxInput input;
    input.escape = true;
    check(!ui::world_tax_input(view, layout, gesture, input, false, controls),
          "Tax report has no cancel acknowledgement");
    input = {};
    input.click = middle(controls->rows.front().rectangle);
    const auto selected = ui::world_tax_input(view, layout, gesture, input, false, controls);
    check(selected && selected->selection == 1 && selected->confirm,
          "No-marker tax release selects the source position and confirms in one frame");
    input.up = input.down = true;
    input.wheel_rows = -1;
    const auto combined = ui::world_tax_input(view, layout, gesture, input, false, controls);
    check(combined && combined->selection == 1 && combined->up && combined->down &&
              combined->confirm && combined->scroll && !combined->scroll->absolute &&
              combined->scroll->value == -1,
          "Tax preserves scroll, selection, both directions and confirmation for source ordering");
    for (int stale = 0; stale < 5; ++stale) {
        auto old = controls;
        if (stale == 0)
            ++old->page;
        if (stale == 1)
            old->selection = 0;
        if (stale == 2)
            old->first = 0;
        if (stale == 3)
            --old->extent.width;
        if (stale == 4)
            ++old->rows.front().identity;
        input = {};
        input.click = middle(controls->rows.front().rectangle);
        check(!ui::world_tax_input(view, layout, gesture, input, false, old),
              "Tax rejects mouse actions from stale page, selection, scroll, size or identity");
    }
    input.focus_lost = true;
    check(!ui::world_tax_input(view, layout, gesture, input, false, controls),
          "Focus loss rejects the last-painted tax row");
    input = {};
    input.click = Vector2{layout.panel.x + 4, layout.panel.y + 4};
    check(!ui::world_tax_input(view, layout, gesture, input, false, controls),
          "Coordinate-free Touch2 never becomes a full-screen acknowledgement");
    input = {};
    input.pointer = Vector2{controls->scroll->x + 1, controls->scroll->y + 60};
    input.pointer_pressed = input.pointer_down = true;
    const auto drag_start = ui::world_tax_input(view, layout, gesture, input, false, controls);
    check(drag_start && drag_start->scroll && !drag_start->confirm && gesture.scrolling,
          "Tax scrollbar press submits only an absolute scroll request");
    input.pointer->y -= 40;
    input.pointer_pressed = input.pointer_down = false;
    input.pointer_released = true;
    input.click = middle(controls->rows.front().rectangle);
    const auto drag_end = ui::world_tax_input(view, layout, gesture, input, false, controls);
    check(
        drag_end && drag_end->scroll && drag_end->scroll->absolute &&
            drag_end->scroll->value == 0 && !drag_end->selection && !drag_end->confirm &&
            !gesture.scrolling,
        "Consumed drag release scrolls to the first window without confirming an overlapping row");
    input = {};
    input.enter = true;
    check(ui::world_tax_input(view, layout, gesture, input, false, controls)->confirm &&
              !ui::world_tax_input(view, layout, gesture, input, true, controls),
          "Tax confirmation has no animation threshold and obeys pending-command blocking");
    check(state.human_calendar.at(5).legacy_G == 15 && same_world_clock(state, before) &&
              state.scene.world.world.ai.accounting.funds() ==
                  before.scene.world.world.ai.accounting.funds(),
          "Tax view/input never credits or clears pending taxes");
    page.legacy_page = state.scripts.pages.back().legacy_page = 98;
    view = ui::world_tax_view(state, page);
    check(ui::world_tax_page(page) && !view.initialized &&
              !ui::world_tax_input(view, layout, gesture, input, false, controls),
          "Automatic raw98 has no user confirmation path");
    check(inside({0, 0, 240, 256}, layout.rows) && inside(layout.panel, layout.total),
          "Source five-row touch window and total fit the minimum viewport");

    // A preceding event input can expose raw98 immediately. The next source update both
    // pays and closes it, so the diagnostic must capture its input before that update.
    auto automatic = initial_world();
    const auto tax = human_page(automatic, 98);
    automatic.human_presence.at(1) = 1;
    automatic.human_homes.at(1)[2] = 1;
    automatic.human_calendar.at(1).legacy_G = 100;
    desktop::WorldHumanInspection inspection;
    inspection.tax_confirmed = true;
    desktop::before_human_inspection_update(automatic, "world-tax-collected", inspection);
    check(inspection.tax_pending && inspection.expected_tax == 100,
          "Diagnostic captures an input-created automatic page before it disappears");
    const auto paid = sim::update_startup_world_tax_page(automatic, tax.id);
    check(paid.has_value(), "Actual automatic tax consumer closes the diagnostic boundary fixture");
    desktop::after_human_inspection_update(*paid, inspection);
    check(inspection.tax_collected && !inspection.tax_pending,
          "Diagnostic recognizes the actual payment without needing a post-update raw98 page");
}
} // namespace ark::test
