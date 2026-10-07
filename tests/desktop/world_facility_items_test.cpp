// Presentation owns page bindings and source display bins; source suites own shared improvement
// math.
#include "support/checks.hpp"
#include "support/world_fixture.hpp"
#include "ui/world_facility_items.hpp"
#include <algorithm>

namespace ark::test {
void world_facility_items() {
    Checks check{"world_facility_items"};
    namespace sim = simulation;
    namespace ui = desktop::ui;
    using Page = sim::rules::WorldScriptPage;
    using Action = sim::StartupFacilityItemAction;
    auto state = initial_world();
    auto catalogue = *state.rules;
    state.rules = &catalogue;
    // STARTUP's loaded map contains inn28. Use its actual placement and occupied map cells:
    // changing only a placement definition breaks the parent's runtime target validation.
    const auto source = std::find_if(catalogue.facilities.begin(), catalogue.facilities.end(),
                                     [](const auto &d) { return d.id == 28; });
    const auto inn = std::find_if(
        state.scene.world.world.facilities.begin(), state.scene.world.world.facilities.end(),
        [](const auto &entry) { return entry.second.placement.definition_id == 28; });
    check(source != catalogue.facilities.end() && inn != state.scene.world.world.facilities.end(),
          "Published initial inn28 definition and instance exist");
    const auto instance = inn->first;
    Page parent;
    parent.id = 900;
    parent.kind = sim::rules::WorldScriptPageKind::raw_page;
    parent.legacy_page = 74;
    parent.legacy_f = source->id;
    parent.lifecycle = 1;
    state.scripts.pages.clear();
    state.scripts.pages.push_back(parent);
    state.facility_page_bindings[parent.id] = instance;
    state.facility_page_neighbours[parent.id] = {};
    state.page_phases[parent.id] = state.page_counters[parent.id] = 0;
    Page page = parent;
    page.id = 901;
    page.legacy_page = 75;
    state.scripts.pages.push_back(page);
    state.facility_page_bindings[page.id] = instance;
    state.page_phases[page.id] = state.page_counters[page.id] = 0;
    const auto layout = ui::world_facility_items_layout({240, 256});
    ui::WorldFacilityItemsInput input;
    input.enter = true;
    auto view = ui::world_facility_items_view(state, page);
    check(!view.initialized && !ui::world_facility_items_input(view, layout, input, false),
          "Uninitialized item pages cannot consume inventory through presentation");
    state.facility_item_pages_initialized.insert(page.id);
    std::vector<int> ids;
    const std::array<std::array<int, 3>, 6> values{
        {{0, 0, 0}, {49, 2, 2}, {50, 3, 3}, {99, 5, 5}, {100, 6, 6}, {-1, -1, -1}}};
    const std::array<int, 6> bins{0, 1, 2, 2, 3, 1};
    for (int n = 0; n < 6; ++n) {
        auto &item = catalogue.items.at(n);
        item.facility_improvements = values[n];
        ids.push_back(item.identity);
        state.items.at(item.identity).inventory = 11 + n;
        state.catalog.at({0, item.identity}) = state.items.at(item.identity);
    }
    state.facility_item_page_lists[page.id] = ids;
    state.facility_item_page_selections[page.id] = 5;
    const auto before = state;
    view = ui::world_facility_items_view(state, page);
    check(
        view.facility == instance && view.selection == 5 && view.first_visible == 1 &&
            view.rows[5].identity == ids[5] && view.rows[5].owned == 16,
        "Facility item rows retain source inventory, binding and the selected row's scroll window");
    for (int n = 0; n < 6; ++n)
        check(view.rows[n].hint == std::array<int, 3>{bins[n], bins[n], bins[n]},
              "Displayed plus bins use original k values at boundaries, including negative nonzero "
              "values");
    input = {};
    input.click = Vector2{layout.rows.x + 3, layout.rows.y + layout.row_height / 2};
    const auto selected = ui::world_facility_items_input(view, layout, input, false);
    check(selected && selected->action == Action::select && selected->selection == 1,
          "Clicking the first visible row selects its source index without using an item");
    input = {};
    input.enter = true;
    check(ui::world_facility_items_input(view, layout, input, false)->action == Action::confirm &&
              !ui::world_facility_items_input(view, layout, input, true),
          "Using an item is one source confirmation blocked while FIFO is pending");
    check(state.items.at(ids[5]).inventory == before.items.at(ids[5]).inventory &&
              state.facility_item_confirmations == before.facility_item_confirmations &&
              same_world_clock(state, before) &&
              state.scene.random.draws() == before.scene.random.draws(),
          "Read-only item projection cannot consume stock, increment event counts or update the "
          "world");
    page.legacy_page = state.scripts.pages.back().legacy_page = 76;
    state.facility_item_page_items[page.id] = ids[5];
    state.facility_upgrade_display = {{{11, 12, 13}, {15, 12, 9}, {4, 0, -4}}};
    view = ui::world_facility_items_view(state, page);
    input.enter = input.escape = input.up = true;
    check(
        !view.can_confirm && !view.can_cancel && !view.graphic.frames.empty() &&
            !ui::world_facility_items_input(view, layout, input, false),
        "Improvement76 displays the real bound facility and has no player-driven progress command");
    page.legacy_page = state.scripts.pages.back().legacy_page = 77;
    for (int count : {0, 48, 49, 54, 55}) {
        state.page_counters[page.id] = count;
        view = ui::world_facility_items_view(state, page);
        input = {};
        input.enter = true;
        const auto intent = ui::world_facility_items_input(view, layout, input, false);
        check(bool(intent) == (count < 49 || count >= 55) &&
                  view.attributes == state.facility_upgrade_display,
              "Result77 reads source before/after/delta and respects its 49-through54 confirmation "
              "gap");
        input = {};
        input.escape = true;
        check(!ui::world_facility_items_input(view, layout, input, false),
              "Improvement result cannot cancel its required source completion");
    }
    state.facility_item_page_items.erase(page.id);
    bool rejected = false;
    try {
        (void)ui::world_facility_items_view(state, page);
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected, "Initialized facility item page with missing item binding fails explicitly");
    for (const auto extent :
         {desktop::Extent{240, 256}, desktop::Extent{540, 360}, desktop::Extent{960, 640}}) {
        const auto frame = ui::world_facility_items_layout(extent);
        check(
            frame.panel.y >= 24 && frame.panel.y + frame.panel.height <= extent.height - 29 &&
                frame.row_height == 19 && frame.rows.height == 95 &&
                frame.rows.y + 5 * frame.row_height <= frame.hints.y &&
                frame.hints.y + frame.hints.height <= frame.feedback.y &&
                frame.feedback.y + frame.feedback.height <= frame.cancel.y &&
                !CheckCollisionRecs(frame.cancel, frame.confirm),
            "Five original19-pitch rows and effect hints remain clear of date/footer and commands");
        for (int row = 0; row < 5; ++row) {
            const auto highlighted = ui::world_facility_item_highlight(frame, row);
            check(highlighted.x == frame.rows.x && highlighted.y == frame.rows.y + 19 * row - 2 &&
                      highlighted.width == 191 && highlighted.height == 16,
                  "Original selected-item background keeps its191x16 region and y-minus2 offset");
        }
        const auto result_height = frame.panel.height - 118;
        check(frame.result.y == frame.panel.y + 47 && frame.result.height == result_height &&
                  frame.result.width == frame.panel.width - 20,
              "Result76/77 keep their existing responsive body separate from the fixed-pitch list");
        auto listing = ui::world_facility_items_view(before, before.scripts.pages.back());
        ui::WorldFacilityItemsInput row_input;
        row_input.click = Vector2{frame.rows.x + 3, frame.rows.y + 4 * 19 + 8};
        row_input.enter = true;
        const auto row_choice = ui::world_facility_items_input(listing, frame, row_input, false);
        check(row_choice && row_choice->action == Action::select && row_choice->selection == 5,
              "Fifth visible row plus Enter selects the actual scrolled row without using the old "
              "choice");
    }
}
} // namespace ark::test
