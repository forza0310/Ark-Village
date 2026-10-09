// UI owns source identity/currency/selection projection. Frozen commerce suites own transactions.
#include "support/checks.hpp"
#include "support/world_fixture.hpp"
#include "ui/world_commerce.hpp"
#include "ui/world_facility_catalog.hpp"
#include "ui/world_magic_pot.hpp"
#include <algorithm>

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
} // namespace
void world_commerce() {
    Checks check{"world_commerce"};
    {
        auto state = initial_world();
        state.scripts.user_flags |= 3U;
        state.scripts.event_calls[101] = 1;
        check(sim::open_startup_world_magic_pot(state, sim::StartupMagicPotEntry::main_menu) ==
                  sim::StartupWorldRuntimeError::none,
              "Explicit pot gate opens actual source41");
        auto page = state.scripts.pages.back();
        const auto layout = ui::world_commerce_layout({240, 256});
        ui::WorldCommerceInput input;
        input.enter = true;
        auto view = ui::world_magic_pot_view(state, page);
        check(!view.initialized && !ui::world_magic_pot_input(view, layout, input, false),
              "Pot rendering does not initialize or act on an unpublished payload");
        check(sim::initialize_startup_world_magic_pot_pages(state), "Source initializes pot41");
        const auto before = state;
        view = ui::world_magic_pot_view(state, page);
        check(view.rows.size() == 2 && view.rows[0].name == "投入道具" &&
                  view.rows[1].name == "开发" &&
                  ui::world_magic_pot_input(view, layout, input, false)->action ==
                      sim::StartupMagicPotAction::confirm &&
                  !ui::world_magic_pot_input(view, layout, input, true) &&
                  same_world_clock(state, before) &&
                  state.scene.random.draws() == before.scene.random.draws(),
              "Pot UI keeps choice order, pending barrier and read-only drawing");
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
        check(ui::world_facility_catalog_input(view, layout, input, false)->action ==
                  CatalogAction::select,
              "Commodity row click selects without clearing NEW or purchasing");
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
    auto page = commerce_page(state, 83);
    state.commerce_pages_initialized.erase(page.id);
    ui::WorldCommerceInput input;
    input.enter = true;
    const auto unopened = state;
    auto view = ui::world_commerce_view(state, page);
    check(!view.initialized && !ui::world_commerce_input(view, layout, input, false) &&
              state.commerce_pages_initialized.empty() && same_world_clock(state, unopened),
          "Drawing an uninitialized commerce page cannot initialize or acknowledge it");
    state.commerce_pages_initialized.insert(page.id);
    view = ui::world_commerce_view(state, page);
    check(view.rows.size() == 3 && view.rows[0].name == "购买道具" &&
              view.rows[1].name == "出售道具" && view.rows[2].name == "购买设施",
          "Commerce menu preserves source choice order");
    input = {};
    input.click = center({layout.rows.x, layout.rows.y + layout.rows.height * 2 / 3,
                          layout.rows.width, layout.rows.height / 3});
    auto intent = ui::world_commerce_input(view, layout, input, false);
    check(intent && intent->action == Action::select && intent->selection == 2,
          "Menu row clicks select the source index without executing a purchase");

    page = commerce_page(state, 84);
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
    page = commerce_page(state, 93);
    page.legacy_s = state.scripts.pages.back().legacy_s = id;
    page.legacy_r = state.scripts.pages.back().legacy_r = 3;
    state.commerce_page_data[page.id][4] = id;
    for (int count : {0, 39, 40}) {
        state.page_counters[page.id] = count;
        view = ui::world_commerce_view(state, page);
        input = {};
        input.escape = true;
        check(!view.can_cancel && !ui::world_commerce_input(view, layout, input, false),
              "Facility receipt cannot cancel the already paid exchange");
        input = {};
        input.enter = true;
        check(
            ui::world_commerce_input(view, layout, input, false)->action == Action::confirm &&
                view.counter == count && view.choice->identity == id,
            "Receipt confirmation preserves the source counter for fast-forward/receive handling");
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
} // namespace ark::test
