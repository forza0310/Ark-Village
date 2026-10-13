// UI owns source identity/currency/selection projection. Frozen commerce suites own transactions.
#include "../../src/desktop/ui/common/skin.hpp"
#include "../../src/desktop/ui/facilities/world_facility_catalog.hpp"
#include "../../src/desktop/ui/village/world_award.hpp"
#include "../../src/desktop/ui/village/world_commerce.hpp"
#include "../../src/desktop/ui/village/world_commerce_top.hpp"
#include "../../src/desktop/ui/village/world_magic_pot.hpp"
#include "../../src/desktop/ui/village/world_progression.hpp"
#include "../support/checks.hpp"
#include "../support/world_fixture.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace ui = desktop::ui;
using Page = sim::rules::WorldScriptPage;
Page commerce_page(sim::StartupWorldRuntimeState &state, int raw, int mode = 0) {
    state.scripts.pages.clear();
    Page page;
    page.id = 901;
    page.kind = sim::rules::WorldScriptPageKind::raw_page;
    page.legacy_page = raw;
    page.legacy_f = mode;
    page.lifecycle = 1;
    state.scripts.next_page_id = std::max(state.scripts.next_page_id, page.id + 1);
    state.scripts.pages.push_back(page);
    state.commerce_pages_initialized.insert(page.id);
    state.commerce_page_data[page.id] = {mode, 0, 0, 0, -1, 0};
    state.page_counters[page.id] = 0;
    return page;
}
Vector2 center(Rectangle box) { return {box.x + box.width / 2, box.y + box.height / 2}; }
sim::StartupWorldRuntimeState source_reward(int raw, int kind = 3) {
    auto state = initial_world();
    Page page;
    page.kind = sim::rules::WorldScriptPageKind::raw_page;
    page.legacy_page = raw;
    page.legacy_r = kind;
    if (kind == 3) {
        const auto facility =
            std::find_if(state.rules->facilities.begin(), state.rules->facilities.end(),
                         [&](const auto &d) { return state.facility_presence.at(d.id) == 0; });
        if (facility == state.rules->facilities.end())
            throw std::runtime_error("Source reward fixture requires an unopened facility");
        page.legacy_s = facility->id;
    } else if (kind >= 5 && kind <= 7) {
        const auto equipment =
            std::find_if(state.rules->equipment.begin(), state.rules->equipment.end(),
                         [&](const auto &d) { return d.shop.kind == kind - 4; });
        if (equipment == state.rules->equipment.end())
            throw std::runtime_error("Source reward fixture requires its equipment namespace");
        page.legacy_s = equipment->shop.id;
    } else if (kind == 8)
        page.legacy_s = state.rules->items.front().identity;
    const auto inserted =
        sim::rules::prepare_world_script_page(sim::startup_world_runtime_scripts(state), page);
    if (!inserted.candidate ||
        !sim::write_startup_world_runtime_scripts(state, inserted.candidate->state))
        throw std::runtime_error("Source reward fixture insertion failed");
    if (raw == 94)
        state.page_human_bindings[state.scripts.pages.back().id] = 1;
    if (!(raw == 93 ? sim::initialize_startup_world_commerce_pages(state)
                    : sim::initialize_startup_world_gift_notice_pages(state)))
        throw std::runtime_error("Source reward fixture initialization failed");
    state.scripts.pages.back().lifecycle = 2; // Explicit published presentation callsite.
    return state;
}
void commerce_top_ui(Checks &check) {
    auto state = initial_world();
    const auto page = commerce_page(state, 83);
    state.commerce_pages_initialized.erase(page.id);
    ui::WorldCommerceInput input;
    input.enter = true;
    const auto before = state;
    auto view = ui::world_commerce_top_view(state, page);
    check(!view.initialized && !ui::world_commerce_top_input(view, {240, 256}, {}, input, false) &&
              state.commerce_pages_initialized.empty() && same_world_clock(state, before),
          "Drawing an uninitialized commerce page cannot initialize or acknowledge it");
    state.commerce_pages_initialized.insert(page.id);
    view = ui::world_commerce_top_view(state, page);
    check(view.initialized && view.source && view.source->selection == 0,
          "Commerce top reads its committed source selection without a desktop cursor");
    sim::SteamCommerceSkinOptions options;
    options.view_y = 23;
    options.text.title = "南瓜商会";
    options.text.welcome = "欢迎光临";
    options.text.names = {"购买道具", "出售道具", "购买设施"};
    options.text.explanations = options.text.names;
    options.text.title_widths = std::array<int, 2>{48, 48};
    view.skin = sim::steam_commerce_skin(*view.source, options);
    check(view.skin.has_value(), "Commerce top uses the published three-choice source skin");
    for (const auto extent :
         {desktop::Extent{240, 256}, desktop::Extent{540, 360}, desktop::Extent{541, 361}}) {
        const auto painted = ui::world_commerce_top_controls(view, extent);
        check(painted.origin.x == (extent.width - 240) / 2 &&
                  painted.origin.y == (extent.height - 256) / 2,
              "Commerce top keeps one integer-centred source canvas on odd and ordinary viewports");
        for (int row = 0; row < 3; ++row) {
            const auto &box = painted.rows[row];
            check(box.x >= 0 && box.y >= 0 && box.x + box.width <= extent.width &&
                      box.y + box.height <= extent.height &&
                      !CheckCollisionRecs(box, painted.cancel),
                  "Raw83 source Touch and PC return have separate visible hit regions");
            input = {};
            input.click = Vector2{box.x + 1, box.y + 1}; // Wider than the painted 134px row bar.
            const auto intent = ui::world_commerce_top_input(view, extent, painted, input, false);
            check(intent && intent->selection == row && intent->confirm && !intent->cancel,
                  "No-marker menu release selects the source row and opens its child once");
            auto opened = state;
            check(
                sim::input_startup_world_commerce_top(opened, page.id, *intent) ==
                        sim::StartupWorldRuntimeError::none &&
                    opened.scripts.pages.back().legacy_page == (row == 2 ? 85 : 84) &&
                    opened.scripts.pages.back().legacy_f == (row == 1 ? 1 : 0) &&
                    opened.commerce_page_data.at(page.id)[2] == row &&
                    opened.scene.world.world.ai.accounting.funds() ==
                        state.scene.world.world.ai.accounting.funds() &&
                    opened.village_points == state.village_points &&
                    opened.scene.random.draws() == state.scene.random.draws() &&
                    opened.item_commerce_read == state.item_commerce_read &&
                    opened.facility_commerce_read == state.facility_commerce_read &&
                    std::all_of(state.items.begin(), state.items.end(),
                                [&](const auto &item) {
                                    return opened.items.at(item.first).inventory ==
                                               item.second.inventory &&
                                           opened.shop_item_stock.at(item.first).quantity ==
                                               state.shop_item_stock.at(item.first).quantity;
                                }),
                "The source-selected child preserves parent choice without paying or clearing NEW");
        }
        input = {};
        input.click = Vector2{painted.rows[0].x - 1, painted.rows[0].y + 1};
        check(!ui::world_commerce_top_input(view, extent, painted, input, false),
              "A release outside source row margins cannot activate a catalogue");
        input.click = center(painted.rows[1]);
        for (int fault = 0; fault < 4; ++fault) {
            auto stale = painted;
            if (fault == 0)
                ++stale.page;
            if (fault == 1)
                ++stale.selection;
            if (fault == 2)
                ++stale.extent.width;
            if (fault == 3)
                ++stale.extent.height;
            check(!ui::world_commerce_top_input(view, extent, stale, input, false),
                  "A stale page, selection or resized commerce paint cannot consume a release");
        }
        check(!ui::world_commerce_top_input(view, extent, {}, input, false) &&
                  !ui::world_commerce_top_input(view, extent, painted, input, true),
              "Missing paint and a pending FIFO command both reject repeated pointer input");
        input.focus_lost = true;
        check(!ui::world_commerce_top_input(view, extent, painted, input, false),
              "Lost focus cannot turn a commerce release into navigation");
        input = {};
        input.click = center(painted.cancel);
        const auto cancel = ui::world_commerce_top_input(view, extent, painted, input, false);
        check(cancel && cancel->cancel && !cancel->confirm && !cancel->selection,
              "The single PC return maps to source soft2 without an independent confirm button");
    }
    input = {};
    input.up = input.down = input.enter = input.escape = true;
    const auto keys = ui::world_commerce_top_input(view, {240, 256}, {}, input, false);
    check(keys && keys->up && keys->down && keys->confirm && keys->cancel && !keys->selection,
          "One keyboard frame retains both directions and source confirm-before-return priority");
    input = {};
    input.left = input.right = input.inspect = true;
    input.wheel_rows = 1;
    check(!ui::world_commerce_top_input(view, {240, 256}, {}, input, false),
          "Raw83 does not invent a tab, inspection or scrolling consumer");
    state.scene.framework_paused = true;
    const auto paused = ui::world_commerce_top_view(state, page);
    input = {};
    input.enter = true;
    check(!ui::world_commerce_top_input(paused, {240, 256}, {}, input, false),
          "Framework pause keeps commerce navigation closed");
}
} // namespace
void world_commerce() {
    Checks check{"world_commerce"};
    commerce_top_ui(check);
    {
        auto state = initial_world();
        state.scripts.user_flags |= 3U;
        state.scripts.event_calls[101] = 1;
        check(sim::open_startup_world_magic_pot(state, sim::StartupMagicPotEntry::main_menu) ==
                  sim::StartupWorldRuntimeError::none,
              "Explicit pot gate opens actual source41");
        auto page = state.scripts.pages.back();
        const auto layout = ui::world_magic_pot_layout({240, 256});
        ui::WorldMagicPotInput input;
        input.enter = true;
        auto view = ui::world_magic_pot_view(state, page);
        check(!view.initialized && !ui::world_magic_pot_input(view, layout, input, false),
              "Pot rendering does not initialize or act on an unpublished payload");
        check(sim::initialize_startup_world_magic_pot_pages(state), "Source initializes pot41");
        state.scripts.pages.back().lifecycle = 1; // Explicit visible callsite after Init.
        const auto before = state;
        view = ui::world_magic_pot_view(state, page);
        check(view.rows.size() == 2 && view.rows[0].name == "放入道具" &&
                  view.rows[1].name == "查看调和书" &&
                  ui::world_magic_pot_input(view, layout, input, false)->confirm &&
                  !ui::world_magic_pot_input(view, layout, input, true) &&
                  same_world_clock(state, before) &&
                  state.scene.random.draws() == before.scene.random.draws(),
              "Pot UI keeps choice order, pending barrier and read-only drawing");
        auto inventory = state;
        check(sim::act_startup_world_magic_pot_page(inventory, page.id,
                                                    sim::StartupMagicPotAction::confirm) ==
                      sim::StartupWorldRuntimeError::none &&
                  sim::initialize_startup_world_magic_pot_pages(inventory),
              "Pot source opens42 for device contract checks");
        const auto list_page = inventory.scripts.pages.back();
        auto list = ui::world_magic_pot_view(inventory, list_page);
        const auto pot_layout = ui::world_magic_pot_layout({240, 256}, 42);
        ui::WorldFacilityPaintedControls controls;
        controls.page = list.page;
        controls.raw = 42;
        controls.selection = list.selection;
        controls.first = list.first_visible;
        controls.count = int(list.rows.size());
        controls.choice = list.binding;
        controls.extent = pot_layout.extent;
        controls.rows.push_back({0, list.rows.front().identity, {25, 95, 190, 16}});
        std::optional<ui::WorldFacilityPaintedControls> painted = controls;
        ui::WorldFacilityGesture gesture;
        input = {};
        input.click = Vector2{50, 100};
        const auto first =
            ui::world_magic_pot_input(list, pot_layout, input, false, painted, &gesture);
        check(first && first->selection == 0 && !first->confirm && gesture.marked_row == 0,
              "42 first release marks without consuming inventory");
        check(!ui::world_magic_pot_input(list, pot_layout, input, true, painted, &gesture),
              "Pending input cannot turn marker into a repeated payment");
        const auto second =
            ui::world_magic_pot_input(list, pot_layout, input, false, painted, &gesture);
        check(second && second->confirm && second->selection == 0,
              "42 second release confirms only the committed source row");
        painted->extent.width = 241;
        check(!ui::world_magic_pot_input(list, pot_layout, input, false, painted, &gesture),
              "Resized painted geometry cannot consume a click");
        painted = controls;
        input.keyboard_event = true;
        const auto reset =
            ui::world_magic_pot_input(list, pot_layout, input, false, painted, &gesture);
        check(reset && !reset->confirm, "Keyboard resets42 marker before a new release");
        input = {};
        input.up = input.down = input.enter = input.escape = true;
        const auto combined =
            ui::world_magic_pot_input(list, pot_layout, input, false, painted, &gesture);
        check(combined && combined->up && combined->down && combined->confirm && combined->cancel,
              "Simultaneous directions and both actions enter one Owner transaction");
        list.raw = 43;
        list.rows.front().known = true;
        painted->raw = 43;
        input = {};
        input.click = Vector2{50, 100};
        const auto recipe_click =
            ui::world_magic_pot_input(list, pot_layout, input, false, painted, &gesture);
        check(recipe_click && recipe_click->selection == 0 && recipe_click->confirm,
              "43 unmarked recipe release selects and confirms in one frame");
        list.rows.front().known = false;
        const auto unknown =
            ui::world_magic_pot_input(list, pot_layout, input, false, painted, &gesture);
        check(unknown && unknown->selection == 0 && !unknown->confirm,
              "Unknown recipe can be selected without entering creation");
        input = {};
        input.down = input.enter = true;
        list.can_confirm = false;
        const auto next_recipe =
            ui::world_magic_pot_input(list, pot_layout, input, false, painted, &gesture);
        check(next_recipe && next_recipe->down && next_recipe->confirm,
              "Old unknown selection cannot swallow same-frame direction and Enter");
        list.raw = 42;
        painted->raw = 42;
        list.rows.front().known = true;
        list.rows.resize(8, list.rows.front());
        painted->count = 8;
        painted->scroll = Rectangle{220, 85, 6, 110};
        painted->scroll_visible = 5;
        input = {};
        input.pointer = Vector2{223, 90};
        input.pointer_pressed = input.pointer_down = true;
        check(ui::world_magic_pot_input(list, pot_layout, input, false, painted, &gesture)
                      ->scroll->value == 0,
              "Source track starts an absolute scroll request");
        input = {};
        input.pointer = Vector2{223, 120};
        input.pointer_released = true;
        input.click = Vector2{50, 100};
        const auto dragged =
            ui::world_magic_pot_input(list, pot_layout, input, false, painted, &gesture);
        check(dragged && dragged->scroll && dragged->scroll->absolute && !dragged->selection &&
                  !dragged->confirm,
              "Consumed scroll release cannot spend an item");
        input = {};
        input.pointer = Vector2{223, 90};
        input.pointer_pressed = input.pointer_down = true;
        ui::world_magic_pot_input(list, pot_layout, input, false, painted, &gesture);
        input = {};
        input.pointer_released = true;
        input.click = Vector2{50, 100};
        check(!ui::world_magic_pot_input(list, pot_layout, input, true, painted, &gesture) &&
                  !gesture.scrolling && !gesture.consumed,
              "Pending drag release clears device capture without a business command");
        // Explicit processing45 callsite: test source counters through the real view rather
        // than constructing can_confirm booleans that merely repeat the input implementation.
        Page processing;
        processing.kind = sim::rules::WorldScriptPageKind::raw_page;
        processing.legacy_page = 45;
        const auto inserted = sim::rules::prepare_world_script_page(
            sim::startup_world_runtime_scripts(state), processing);
        check(inserted.candidate &&
                  sim::write_startup_world_runtime_scripts(state, inserted.candidate->state) &&
                  sim::initialize_startup_world_magic_pot_pages(state),
              "Source initializes processing-result callsite");
        state.scripts.pages.back().lifecycle = 1;
        page = state.scripts.pages.back();
        for (const int count : {0, 76, 77, 82, 83}) {
            state.page_counters.at(page.id) = count;
            const auto gate = ui::world_magic_pot_view(state, page);
            input = {};
            input.escape = true;
            check(!gate.can_cancel && !ui::world_magic_pot_input(gate, layout, input, false),
                  "Pot Back cannot retire a result page without a source cancel consumer");
            input = {};
            input.enter = true;
            check(ui::world_magic_pot_input(gate, layout, input, false).has_value() ==
                          (count < 77 || count >= 83) &&
                      state.page_counters.at(page.id) == count,
                  "Processing45 shows fast-forward, timed hold and finish from immutable source "
                  "count");
        }
    }
    {
        using CatalogAction = sim::StartupFacilityCatalogAction;
        auto source = initial_world();
        Page page;
        page.kind = sim::rules::WorldScriptPageKind::raw_page;
        page.legacy_page = 79;
        page.legacy_f = 1;
        const auto opened =
            sim::rules::prepare_world_script_page(sim::startup_world_runtime_scripts(source), page);
        check(opened.candidate &&
                  sim::write_startup_world_runtime_scripts(source, opened.candidate->state),
              "Catalogue UI fixture creates its page through the source stack");
        page = source.scripts.pages.back();
        const auto layout = ui::world_facility_catalog_layout({240, 256});
        ui::WorldFacilityCatalogInput input;
        input.enter = true;
        auto view = ui::world_facility_catalog_view(source, page);
        check(!view.initialized && !ui::world_facility_catalog_input(view, layout, input, false),
              "Catalogue rendering cannot initialize or act on pending source payload");
        check(sim::initialize_startup_world_facility_catalog_pages(source),
              "Source initializes commodity catalogue");
        const auto before = source;
        view = ui::world_facility_catalog_view(source, page);
        const auto entries =
            sim::inspect_startup_world_facility_catalog_page(source, page.id)->entries;
        check(view.rows.size() == entries.size() && view.rows.front().identity == entries.front() &&
                  same_world_clock(source, before) &&
                  source.scene.random.draws() == before.scene.random.draws(),
              "Read-only commodity view retains source order and does not consume a notification "
              "or RNG");
        for (const auto &row : view.rows) {
            const auto definition =
                std::find_if(source.rules->equipment.begin(), source.rules->equipment.end(),
                             [&](const auto &item) {
                                 return item.shop.kind == 1 && item.shop.id == row.identity;
                             });
            check(definition != source.rules->equipment.end() &&
                      row.price == definition->shop.price && row.combat == definition->shop.combat,
                  "Catalogue uses store price and real combat values, not gift quote or screenshot "
                  "amounts");
            check(row.icon_image == 12 && row.icon == definition->shop.type,
                  "Weapon catalogue binds column2 icon instead of the sparse map PNG index");
        }
        check(!ui::world_facility_catalog_input(view, layout, input, true),
              "Pending catalogue blocks repeated input");
        input = {};
        input.click = center({layout.rows.x, layout.rows.y, layout.rows.width, layout.row_height});
        check(!ui::world_facility_catalog_input(view, layout, input, false),
              "Commodity click requires the last-painted controls");
        ui::WorldFacilityPaintedControls painted;
        painted.page = view.page;
        painted.raw = view.raw;
        painted.phase = view.phase;
        painted.selection = view.selection;
        painted.first = view.first_visible;
        painted.count = static_cast<int>(view.rows.size());
        painted.choice = view.choice->identity;
        painted.extent = layout.extent;
        painted.rows.push_back(
            {0,
             entries.front(),
             {layout.rows.x, layout.rows.y, layout.rows.width, layout.row_height}});
        const auto clicked =
            ui::world_facility_catalog_input(view, layout, input, false, painted, layout.extent);
        check(clicked && clicked->input && clicked->input->selection == 0 &&
                  clicked->input->confirm && !clicked->input->inspect,
              "Source79 row release selects and confirms in one frame without a purchase action");
        check(!ui::world_facility_catalog_input(view, layout, input, true, painted, layout.extent),
              "Pending catalogue cannot confirm its last-painted row");
        auto stale = painted;
        ++stale.phase;
        check(!ui::world_facility_catalog_input(view, layout, input, false, stale, layout.extent),
              "Commodity click rejects geometry from a different phase");
        auto confirmed = source;
        confirmed.catalog.at({1, entries.front()}).newly_unlocked = true;
        const auto stock = confirmed.catalog.at({1, entries.front()}).inventory;
        check(sim::input_startup_world_facility_catalog_page(confirmed, page.id, *clicked->input) ==
                      sim::StartupWorldRuntimeError::none &&
                  confirmed.scripts.pages.back().lifecycle == 4 &&
                  !confirmed.catalog.at({1, entries.front()}).newly_unlocked &&
                  confirmed.catalog.at({1, entries.front()}).inventory == stock &&
                  confirmed.scene.world.world.ai.accounting.funds() ==
                      source.scene.world.world.ai.accounting.funds() &&
                  confirmed.scene.random.draws() == source.scene.random.draws(),
              "Commodity confirmation closes and clears NEW without payment, stock use or RNG");
        input = {};
        input.inspect = true;
        check(ui::world_facility_catalog_input(view, layout, input, false)->action ==
                      CatalogAction::inspect &&
                  sim::act_startup_world_facility_catalog_page(source, page.id,
                                                               CatalogAction::inspect) ==
                      sim::StartupWorldRuntimeError::none &&
                  sim::initialize_startup_world_facility_catalog_pages(source),
              "Information key opens source72 with its actual parent");
        view = ui::world_facility_catalog_view(source, source.scripts.pages.back());
        check(view.raw == 72 && view.choice && view.choice->identity == entries.front(),
              "Information view binds the selected equipment rather than a human item page");
        input = {};
        input.escape = true;
        check(ui::world_facility_catalog_input(view, layout, input, false)->action ==
                  CatalogAction::cancel,
              "Right-click/escape mapping preserves the dedicated information return");
        for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{540, 360}}) {
            const auto frame = ui::world_facility_catalog_layout(extent);
            check(frame.panel.y >= 24 && frame.panel.y + frame.panel.height <= extent.height - 29 &&
                      frame.rows.height == frame.row_height * 4 &&
                      !CheckCollisionRecs(frame.cancel, frame.inspect),
                  "Four source rows and small soft keys fit minimum and ordinary viewports");
        }
    }
    using Action = sim::StartupCommerceAction;
    auto state = initial_world();
    const auto layout = ui::world_commerce_layout({240, 256});
    ui::WorldCommerceInput input;
    ui::WorldCommerceView view;
    std::optional<ui::WorldCommerceIntent> intent;
    auto page = commerce_page(state, 84);
    std::vector<int> ids;
    for (int n = 0; n < 6; ++n) {
        const int id = state.rules->items.at(5 - n).identity;
        ids.push_back(id);
        state.shop_item_stock.at(id).quantity = 17 + n;
        state.items.at(id).inventory = 3 + n;
        state.catalog.at({0, id}) = state.items.at(id);
        state.item_commerce_read.at(id) = n != 1;
    }
    state.commerce_page_lists[page.id] = ids;
    state.commerce_page_data[page.id] = {0, 0, 5, 1, -1, 20};
    const auto before = state;
    view = ui::world_commerce_view(state, page);
    check(view.rows.front().identity == 5 && view.rows.front().icon.size() == 2 &&
              view.rows.front().icon[1].image == 9 &&
              view.rows.front().icon[1].crop == std::array<int, 4>{112, 16, 16, 16},
          "Commerce uses original item5 icon22 regardless of price/stock sorting");
    check(view.rows[0].identity == ids[0] && view.rows[1].identity == ids[1] &&
              view.rows[1].fresh && !view.rows[0].fresh && view.rows[1].remaining == 18 &&
              view.rows[1].owned == 4 && view.selection == 5 && view.first_visible == 1 &&
              view.feedback_counter == 20,
          "Source order, separate A/z inventories, B notices and five-row scroll are preserved");
    input = {};
    input.right = true;
    intent = ui::world_commerce_input(view, layout, input, false);
    check(intent && intent->action == Action::next_tab && view.mode == 0,
          "Purchase tabs switch the display without switching buy/sell direction");
    state.commerce_page_data[page.id][1] = 1;
    view = ui::world_commerce_view(state, page);
    input = {};
    input.enter = true;
    check(ui::world_commerce_input(view, layout, input, false)->action == Action::confirm &&
              !ui::world_commerce_input(view, layout, input, true),
          "Feedback clearing stays a source confirm and pending FIFO blocks a second command");
    check(state.scene.world.world.ai.accounting.funds() ==
                  before.scene.world.world.ai.accounting.funds() &&
              state.items.at(ids[0]).inventory == before.items.at(ids[0]).inventory &&
              state.scene.random.draws() == before.scene.random.draws() &&
              same_world_clock(state, before),
          "Commerce projection/input cannot spend, alter stock, consume random or advance world");
    state.commerce_page_data[page.id] = {1, 0, 0, 0, -1, 0};
    state.scripts.pages.back().legacy_f = page.legacy_f = 1;
    view = ui::world_commerce_view(state, page);
    check(view.mode == 1 && view.rows[0].price == state.rules->items.at(5).commerce_price / 2,
          "Sale price uses the frozen item price half, not construction or free-equipment cost");
    input = {};
    input.right = true;
    check(!ui::world_commerce_input(view, layout, input, false),
          "Selling has no purchase/owned display tabs");

    check(sim::act_startup_world_commerce_page(state, page.id, Action::confirm) ==
              sim::StartupWorldRuntimeError::none,
          "The source sale creates its real post-payment86 before a framework update");
    const auto receipt = state.scripts.pages.back();
    check(receipt.legacy_page == 86 && !state.commerce_pages_initialized.count(receipt.id),
          "The actual transaction receipt is fresh before its automatic consumer");
    const auto paid = state;
    view = ui::world_commerce_view(state, receipt);
    check(view.initialized && view.raw == 86 && view.mode == 1 && view.choice &&
              view.choice->identity == ids[0] &&
              view.choice->price == state.rules->items.at(5).commerce_price / 2 &&
              !view.can_confirm && !view.can_cancel,
          "A real uninitialized86 projects its bound sold item and half price without player "
          "commands");
    input = {};
    input.enter = input.escape = input.up = true;
    check(!ui::world_commerce_input(view, layout, input, false) &&
              !state.commerce_pages_initialized.count(receipt.id) &&
              state.commerce_page_data == paid.commerce_page_data &&
              state.commerce_page_lists == paid.commerce_page_lists &&
              state.page_counters == paid.page_counters &&
              state.items.at(ids[0]).inventory == paid.items.at(ids[0]).inventory &&
              state.scene.world.world.ai.accounting.funds() ==
                  paid.scene.world.world.ai.accounting.funds() &&
              same_world_clock(state, paid) &&
              state.scene.random.draws() == paid.scene.random.draws(),
          "Fresh receipt projection creates no counters/cache and cannot repay, advance or close "
          "the source page");
    for (const auto invalid : {2, -1}) {
        auto bad = receipt;
        bad.legacy_f = invalid;
        bool rejected = false;
        try {
            (void)ui::world_commerce_view(state, bad);
        } catch (const std::invalid_argument &) {
            rejected = true;
        }
        check(rejected, "Fresh receipts reject invalid buy/sell direction");
    }
    auto bad = receipt;
    bad.legacy_s = -1;
    bool receipt_rejected = false;
    try {
        (void)ui::world_commerce_view(state, bad);
    } catch (const std::invalid_argument &) {
        receipt_rejected = true;
    }
    check(receipt_rejected, "Fresh receipts reject missing or mismatched item binding");

    page = commerce_page(state, 85);
    const auto definition = std::find_if(
        state.rules->facilities.begin(), state.rules->facilities.end(),
        [&](const auto &d) { return d.unlock_rank >= 0 && d.unlock_rank <= state.rank; });
    check(definition != state.rules->facilities.end(),
          "Published facility exchange fixture exists");
    const int id = definition->id;
    state.facility_presence.at(id) = 0;
    state.commerce_page_lists[page.id] = {id};
    state.village_points = 87;
    state.facility_commerce_read.at(id) = false;
    view = ui::world_commerce_view(state, page);
    check(view.points == 87 &&
              view.rows[0].price == state.rules->facility_initial.at(id).capacity &&
              view.rows[0].fresh && !view.rows[0].graphic.frames.empty(),
          "Facility commerce uses village points and the actual definition graphic");
    input = {};
    input.inspect = true;
    check(ui::world_commerce_input(view, layout, input, false)->action == Action::inspect,
          "Definition inspection is a separate source action, not a purchase");
    // These source receipts now share the ordered town plan; commerce owns93's actual claim.
    for (const int raw : {93, 94})
        for (int kind = 3; kind <= (raw == 93 ? 3 : 8); ++kind) {
            auto reward = source_reward(raw, kind);
            const auto reward_page = reward.scripts.pages.back();
            check(ui::world_progression_page(reward_page) &&
                      !ui::world_commerce_page(reward_page) &&
                      !ui::world_page_regular_confirmation(reward_page),
                  "Source rewards have one UI route, without commerce/generic confirmation races");
            const auto frame = ui::world_progression_layout({240, 256}, raw);
            for (const int count : {0, 39, 40}) {
                reward.page_counters.at(reward_page.id) = count;
                const auto before_reward = reward;
                const auto projection = ui::world_progression_view(reward, reward_page);
                ui::WorldProgressionInput reward_input;
                reward_input.escape = true;
                check(!ui::world_progression_input(projection, frame, reward_input, false),
                      "Facility/character receipt cannot cancel its committed source reward");
                reward_input = {};
                reward_input.enter = true;
                const auto confirm =
                    ui::world_progression_input(projection, frame, reward_input, false);
                check(confirm && !confirm->rank_action && !confirm->input &&
                          projection.counter == count && projection.reward_kind == kind &&
                          projection.reward_value == reward_page.legacy_s &&
                          (raw == 93 ? projection.giver == -1 : projection.giver == 1),
                      "Receipt confirmation preserves source counter, namespace and real giver");
                check(!ui::world_progression_input(projection, frame, reward_input, true) &&
                          reward.page_counters == before_reward.page_counters &&
                          reward.facility_presence == before_reward.facility_presence &&
                          reward.scene.random.draws() == before_reward.scene.random.draws() &&
                          same_world_clock(reward, before_reward),
                      "Receipt projection cannot claim, draw random or bypass pending input");
            }
            auto incomplete = reward;
            incomplete.page_counters.erase(reward_page.id);
            check(!ui::world_progression_view(incomplete, reward_page).initialized,
                  "Reward projection refuses missing Owner clock rather than initializing it");
            incomplete = reward;
            incomplete.scripts.pages.back().lifecycle = 0;
            check(!ui::world_progression_view(incomplete, incomplete.scripts.pages.back())
                       .initialized,
                  "Reward projection waits for the real initialized and published source page");
            if (raw == 94) {
                incomplete = reward;
                incomplete.page_human_bindings.erase(reward_page.id);
                check(!ui::world_progression_view(incomplete, reward_page).initialized,
                      "Character notice cannot substitute a generic actor for a missing giver");
            }
            auto unsupported = reward_page;
            unsupported.legacy_r = raw == 93 ? 4 : 2;
            check(!ui::world_progression_page(unsupported),
                  "Published reward skin does not claim unsupported source templates");
        }
    page = commerce_page(state, 86, 1);
    page.legacy_s = state.scripts.pages.back().legacy_s = ids[0];
    state.commerce_page_data[page.id][4] = ids[0];
    view = ui::world_commerce_view(state, page);
    input = {};
    input.enter = input.escape = input.up = input.inspect = true;
    check(
        view.initialized && !view.can_confirm && !view.can_cancel &&
            !ui::world_commerce_input(view, layout, input, false),
        "Transaction result86 is automatic and cannot repeat payment through generic confirmation");
    state.commerce_page_data.erase(page.id);
    bool rejected = false;
    try {
        (void)ui::world_commerce_view(state, page);
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected, "Initialized commerce pages with missing payload fail explicitly");
    for (const auto extent :
         {desktop::Extent{240, 256}, desktop::Extent{540, 360}, desktop::Extent{960, 640}}) {
        const auto frame = ui::world_commerce_layout(extent);
        check(frame.panel.y >= 24 && frame.panel.y + frame.panel.height <= extent.height - 29 &&
                  frame.rows.y + frame.rows.height <= frame.status.y &&
                  !CheckCollisionRecs(frame.cancel, frame.inspect) &&
                  !CheckCollisionRecs(frame.inspect, frame.confirm),
              "Commerce controls and rows clear the date/footer across supported viewports");
    }
}
// Optional raw83 draw snapshots; injected presentation conditions are not natural unlock evidence.
void world_commerce_top_render_fixture() {
    Checks check{"world_commerce_top_render_fixture"};
    const auto output = std::filesystem::path(ARK_TEST_OUTPUT) / "commerce-top";
    std::filesystem::create_directories(output);
    struct Window {
        Window() {
            InitWindow(720, 600, "Ark-Village UI fixture / commerce top");
            if (!IsWindowReady())
                throw std::runtime_error("Commerce top fixture could not create its window");
            SetTargetFPS(60);
        }
        ~Window() { CloseWindow(); }
    } window;
    desktop::Sprites sprites(ARK_TEST_ASSETS);
    desktop::Text text(ARK_TEST_FONT);
    ui::Skin skin(sprites, text);
    for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{720, 600}}) {
        SetWindowSize(extent.width, extent.height);
        for (int fresh = 0; fresh < 2; ++fresh)
            for (int selected = 0; selected < 3; ++selected) {
                auto state = initial_world();
                const auto page = commerce_page(state, 83);
                state.rank = 4; // Explicit eligible-NEW snapshot, never a claimed earned star.
                state.commerce_page_data.at(page.id)[2] = selected;
                for (auto &[id, read] : state.item_commerce_read) {
                    read = !fresh;
                    state.shop_item_stock.at(id).quantity = fresh ? 1 : 0;
                }
                for (auto &[id, read] : state.facility_commerce_read) {
                    (void)id;
                    read = !fresh;
                }
                const auto before = state;
                auto view = ui::world_commerce_top_view(state, page);
                check(view.source && view.source->item_new == bool(fresh) &&
                          view.source->facility_new == bool(fresh),
                      "Commerce fixture uses both actual source NEW predicates");
                ui::prepare_world_commerce_top(view, skin);
                std::optional<ui::WorldCommerceTopControls> painted;
                for (int frame = 0; frame < 8; ++frame) {
                    check(!WindowShouldClose(), "Commerce top fixture remains open for capture");
                    BeginDrawing();
                    ClearBackground({145, 211, 247, 255});
                    DrawText("CONDITIONAL commerce UI", 4, 4, 12, BLACK);
                    painted = ui::draw_world_commerce_top(view, extent, skin, true);
                    text.flush(1, {});
                    EndDrawing();
                }
                check(painted && painted->selection == selected &&
                          painted->extent.width == extent.width,
                      "Actual ordered drawing publishes the same page and selected row geometry");
                const auto file =
                    output / ("raw83-" + std::to_string(extent.width) + "-row" +
                              std::to_string(selected) + "-new" + std::to_string(fresh) + ".png");
                auto image = LoadImageFromScreen();
                const bool exported = image.data && ExportImage(image, file.string().c_str());
                UnloadImage(image);
                check(exported && std::filesystem::file_size(file) > 0,
                      "Commerce top writes its actual common/common2/private-character screenshot");
                check(state.item_commerce_read == before.item_commerce_read &&
                          state.facility_commerce_read == before.facility_commerce_read &&
                          state.commerce_page_data == before.commerce_page_data &&
                          state.page_counters == before.page_counters &&
                          state.village_points == before.village_points &&
                          state.scene.world.world.ai.accounting.funds() ==
                              before.scene.world.world.ai.accounting.funds() &&
                          same_world_clock(state, before),
                      "Repeated commerce rendering preserves NEW, selection, funds and source "
                      "clocks");
                std::cout << "UI fixture / conditional raw83 selection=" << selected
                          << " NEW=" << fresh << " extent=" << extent.width << 'x' << extent.height
                          << " frames=8 screenshot=" << file.string() << '\n';
            }
    }
    std::cout << "Commerce top rendering: " << check.count() << " checks\n";
}
} // namespace ark::test
