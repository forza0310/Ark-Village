// UI owns current-owner projection and action routing. Frozen rule/runtime suites own
// profession gates, gift rewards and tax posting; those transactions are not duplicated here.
#include "ark/simulation/facilities/startup_world_commerce.hpp"
#include "../support/checks.hpp"
#include "../support/world_fixture.hpp"
#include "../../src/desktop/ui/actors/world_human.hpp"
#include "../../src/desktop/ui/village/world_tax.hpp"
#include "../../src/desktop/inspection/world_human_inspection.hpp"
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
} // namespace

void world_human() {
    Checks check{"world_human"};
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
    check(ui::world_human_input(view, layout, input, false)->action == Action::confirm &&
              !ui::world_human_input(view, layout, input, true),
          "Human confirmation is explicit and blocked while an earlier command is pending");
    input = {};
    input.escape = true;
    check(ui::world_human_input(view, layout, input, false)->action == Action::cancel,
          "Human detail Back is routed to source cancel");
    for (int tab = 0; tab < 4; ++tab) {
        input = {};
        input.click = middle(ui::world_human_detail_layout(layout).tabs[tab]);
        const auto intent = ui::world_human_input(view, layout, input, false);
        check(intent && intent->action == Action::view_tab && intent->selection == tab,
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
    view = ui::world_human_view(state, page);
    check(view.rows.size() == 3 && view.rows[0].identity == 2 && view.rows[1].identity == 0 &&
              view.choice->identity == 0,
          "Profession catalogue retains source ordering and selection separately from IDs");
    input = {};
    input.click = middle({layout.rows.x, layout.rows.y, layout.rows.width, layout.row_height});
    auto intent = ui::world_human_input(view, layout, input, false);
    check(intent && intent->action == Action::select && intent->selection == 0 &&
              state.human_page_selections.at(page.id) == 1,
          "Row click selects an index through FIFO without confirming or mutating the Owner");
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
        input.click = middle(layout.gift_tabs[slot]);
        intent = ui::world_human_input(view, layout, input, false);
        check(intent && intent->action == Action::equipment_slot && intent->selection == slot,
              "Gift tabs preserve four equipment namespaces and the separate ordinary-item "
              "namespace");
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
    page.legacy_page = state.scripts.pages.back().legacy_page = 70;
    state.page_phases[page.id] = 2;
    view = ui::world_human_view(state, page);
    input = {};
    input.click = middle({layout.body.x, layout.body.y + 3 * layout.body.height / 4,
                          layout.body.width, layout.body.height / 4});
    intent = ui::world_human_input(view, layout, input, false);
    check(intent && intent->action == Action::select && intent->selection == 1,
          "Mastery keep-current option selects before a separate final confirmation");
    page.legacy_page = 67;
    check(!ui::world_human_page(page),
          "Effort raw67 remains owned by the existing progression view");
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
        for (const auto box : frame.gift_tabs)
            check(inside(frame.panel, box), "All five gift tabs fit minimum and large viewports");
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
    using Action = sim::StartupWorldTaxAction;
    auto state = initial_world();
    auto page = human_page(state, 90);
    const auto layout = ui::world_tax_layout({240, 256});
    check(!ui::world_tax_view(state, page).initialized && state.tax_page_residents.empty(),
          "Tax drawing waits for source list initialization without freezing a list itself");
    state.tax_page_residents[page.id] = {5, 2, 4, 1, 0, 3};
    state.tax_page_selection[page.id] = 5;
    state.tax_page_scroll[page.id] = 1;
    for (int id = 0; id < 6; ++id)
        state.human_calendar.at(id).legacy_G = 10 + id;
    const auto before = state;
    auto view = ui::world_tax_view(state, page);
    check(
        view.initialized && view.rows.size() == 6 && view.rows[0].identity == 5 &&
            view.rows[1].identity == 2 && view.rows[0].amount == 15 && view.total == 75 &&
            view.first_visible == 1 && view.selection == 5,
        "Tax shows source frozen order, current G amounts, exact total and source five-row scroll");
    ui::WorldHumanInput input;
    input.escape = true;
    check(!ui::world_tax_input(view, layout, input, false),
          "Tax report has no cancel acknowledgement");
    input = {};
    input.click = middle({layout.rows.x, layout.rows.y, layout.rows.width, layout.row_height});
    const auto selected = ui::world_tax_input(view, layout, input, false);
    check(selected && selected->action == Action::select && selected->selection == 1,
          "Tax visible row maps to the source list position, not resident identity");
    input = {};
    input.enter = true;
    check(ui::world_tax_input(view, layout, input, false)->action == Action::confirm &&
              !ui::world_tax_input(view, layout, input, true),
          "Tax confirmation has no animation threshold and obeys pending-command blocking");
    check(state.human_calendar.at(5).legacy_G == 15 && same_world_clock(state, before) &&
              state.scene.world.world.ai.accounting.funds() ==
                  before.scene.world.world.ai.accounting.funds(),
          "Tax view/input never credits or clears pending taxes");
    page.legacy_page = state.scripts.pages.back().legacy_page = 98;
    view = ui::world_tax_view(state, page);
    check(ui::world_tax_page(page) && !view.initialized &&
              !ui::world_tax_input(view, layout, input, false),
          "Automatic raw98 has no user confirmation path");
    check(inside(layout.panel, layout.rows) && inside(layout.panel, layout.total) &&
              inside(layout.panel, layout.confirm),
          "Five tax rows and total fit the minimum viewport");

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
