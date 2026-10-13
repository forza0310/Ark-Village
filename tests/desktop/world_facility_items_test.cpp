// Presentation owns page bindings and source display bins; source suites own shared improvement
// math.
#include "../../src/desktop/ui/facilities/world_facility_items.hpp"
#include "../../src/desktop/ui/facilities/world_facility_skin.hpp"
#include "../support/checks.hpp"
#include "../support/world_fixture.hpp"
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
    const auto painted = [](const ui::WorldFacilityItemsView &v,
                            const ui::WorldFacilityItemsLayout &l) {
        ui::WorldFacilityPaintedControls p;
        p.page = v.page;
        p.raw = v.raw;
        p.selection = v.selection;
        p.first = v.first_visible;
        p.count = int(v.rows.size());
        p.choice = v.choice ? v.choice->identity : -1;
        p.extent = l.extent;
        p.cancel = v.can_cancel ? l.cancel : Rectangle{};
        p.confirm = v.raw != 76 ? l.confirm : Rectangle{};
        const auto origin = ui::world_facility_skin_origin(l.panel);
        // Independent published row rectangles; GPU execution is checked by render fixtures.
        for (int row = v.first_visible; row < int(v.rows.size()) && row < v.first_visible + 5;
             ++row)
            p.rows.push_back(
                {row,
                 v.rows[row].identity,
                 {origin.x + 23, origin.y + 95 + 19 * (row - v.first_visible), 191, 16}});
        p.scroll = Rectangle{origin.x + 218, origin.y + 85, 6, 110};
        p.scroll_visible = 5;
        return p;
    };
    const auto input_of = [&](const auto &v, const auto &l, auto &g, const auto &i, bool blocked) {
        return ui::world_facility_items_input(v, l, g, i, blocked, painted(v, l), l.extent);
    };
    ui::WorldFacilityItemsInput input;
    ui::WorldFacilityItemsSelection marker;
    input.enter = true;
    auto view = ui::world_facility_items_view(state, page);
    check(!view.initialized && !input_of(view, layout, marker, input, false),
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
    state.facility_item_page_scroll[page.id] = 1;
    state.facility_monthly_cash.at(instance)[0] = {100, 7};
    state.facility_monthly_cash.at(instance)[1] = {20, 60};
    state.facility_monthly_cash.at(instance)[4] = {2000,
                                                   1}; // Future month is not part of the footer.
    const auto before = state;
    view = ui::world_facility_items_view(state, page);
    check(view.rows[0].identity == 0 && view.rows[0].icon.size() == 2 &&
              view.rows[0].icon[0].image == 24 && view.rows[0].icon[1].image == 9 &&
              view.rows[0].icon[1].crop == std::array<int, 4>{80, 0, 16, 16},
          "Item0 uses original icon5 foreground over category background, not identity0");
    check(view.title == "设施强化" && view.profit == 53,
          "Strengthening caption follows S044; footer sums actual income-cost only through month3");
    check(
        view.facility == instance && view.selection == 5 && view.first_visible == 1 &&
            view.rows[5].identity == ids[5] && view.rows[5].owned == 16,
        "Facility item rows retain source inventory, binding and the selected row's scroll window");
    for (int n = 0; n < 6; ++n)
        check(view.rows[n].hint == std::array<int, 3>{bins[n], bins[n], bins[n]},
              "Displayed plus bins use original k values at boundaries, including negative nonzero "
              "values");
    {
        auto tail = state;
        tail.facility_item_page_scroll.at(page.id) = 5;
        const auto tail_view = ui::world_facility_items_view(tail, page);
        check(tail_view.first_visible == 5 && tail_view.selection == 5,
              "Read-only projection retains the legal one-row tail instead of recomputing first");
        ui::WorldFacilityItemsInput keys;
        keys.up = keys.down = true;
        const auto combined = input_of(view, layout, marker, keys, false);
        check(combined && combined->input && combined->input->up && combined->input->down,
              "Desktop sends cancelling directions together for the source viewport consumer");
        auto stale = painted(view, layout);
        ++stale.page;
        ui::WorldFacilityItemsInput click;
        click.click = Vector2{layout.rows.x + 3, layout.rows.y + 8};
        check(!ui::world_facility_items_input(view, layout, marker, click, false, stale,
                                              layout.extent),
              "Retired painted geometry cannot select or use a new page");
        auto controls = painted(view, layout);
        ui::WorldFacilityItemsInput drag;
        drag.pointer = Vector2{controls.scroll->x + 1, controls.scroll->y + 5};
        drag.pointer_pressed = drag.pointer_down = true;
        const auto captured = ui::world_facility_items_input(view, layout, marker, drag, false,
                                                             controls, layout.extent);
        check(captured && captured->input && captured->input->scroll && marker.scrolling,
              "Painted scrollbar captures once and translates to the source absolute request");
        drag.pointer_pressed = drag.pointer_down = false;
        drag.pointer_released = true;
        drag.click = click.click;
        check(!ui::world_facility_items_input(view, layout, marker, drag, true, controls,
                                              layout.extent) &&
                  !marker.scrolling && !marker.consumed,
              "Pending release clears capture without replaying a row confirmation");
    }
    input = {};
    input.click = Vector2{layout.rows.x + 3, layout.rows.y + layout.row_height / 2};
    const auto selected = input_of(view, layout, marker, input, false);
    check(selected && selected->action == Action::select && selected->selection == 1,
          "Clicking the first visible row selects its source index without using an item");
    auto committed = view;
    committed.selection = 1;
    const auto confirmed = input_of(committed, layout, marker, input, false);
    check(confirmed && confirmed->action == Action::confirm,
          "User-approved row marker confirms a later click on the same committed absolute row");
    input.click->y += layout.row_height;
    check(input_of(committed, layout, marker, input, false)->action == Action::select &&
              input_of(committed, layout, marker, input, false)->action == Action::select,
          "A different row marks/selects; even a second click cannot use a stale committed row");
    check(!input_of(committed, layout, marker, input, true),
          "Pending barrier cannot use a marked row or submit duplicate selection");
    input = {};
    input.keyboard_event = true;
    check(!input_of(committed, layout, marker, input, false) && !marker.marked_row,
          "Any key-down clears mouse marker even when it has no list navigation action");
    input = {};
    input.click = Vector2{layout.rows.x + 3, layout.rows.y + 8};
    check(input_of(committed, layout, marker, input, false)->action == Action::select,
          "Click after keyboard reset starts a new marker without consuming stock");
    ++committed.page;
    check(input_of(committed, layout, marker, input, false)->action == Action::select,
          "Identical row coordinates on a different page never inherit a confirmation marker");
    input = {};
    input.enter = true;
    check(input_of(view, layout, marker, input, false)->action == Action::confirm &&
              !input_of(view, layout, marker, input, true),
          "Using an item is one source confirmation blocked while FIFO is pending");
    check(state.items.at(ids[5]).inventory == before.items.at(ids[5]).inventory &&
              state.facility_item_confirmations == before.facility_item_confirmations &&
              same_world_clock(state, before) &&
              state.scene.random.draws() == before.scene.random.draws(),
          "Read-only item projection cannot consume stock, increment event counts or update the "
          "world");
    state.facility_item_page_lists.erase(page.id);
    state.facility_item_page_selections.erase(page.id);
    state.facility_item_page_scroll.erase(page.id);
    page.legacy_page = state.scripts.pages.back().legacy_page = 76;
    state.facility_item_page_items[page.id] = ids[5];
    state.facility_upgrade_display = {{{11, 12, 13}, {15, 12, 9}, {4, 0, -4}}};
    view = ui::world_facility_items_view(state, page);
    input.enter = input.escape = input.up = true;
    check(
        !view.can_confirm && !view.can_cancel && !view.graphic.frames.empty() &&
            !input_of(view, layout, marker, input, false),
        "Improvement76 displays the real bound facility and has no player-driven progress command");
    page.legacy_page = state.scripts.pages.back().legacy_page = 77;
    for (int count : {0, 48, 49, 54, 55}) {
        state.page_counters[page.id] = count;
        view = ui::world_facility_items_view(state, page);
        input = {};
        input.enter = true;
        const auto intent = input_of(view, layout, marker, input, false);
        check(bool(intent) == (count < 49 || count >= 55) &&
                  view.attributes == state.facility_upgrade_display,
              "Result77 reads source before/after/delta and respects its 49-through54 confirmation "
              "gap");
        input = {};
        input.escape = true;
        check(!input_of(view, layout, marker, input, false),
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
                frame.hints.y + frame.hints.height <= frame.panel.y + frame.panel.height &&
                frame.cancel.y == frame.panel.y + frame.panel.height + 4 &&
                !CheckCollisionRecs(frame.cancel, frame.confirm),
            "Five original19-pitch rows and effect hints remain clear of date/footer and commands");
        for (int row = 0; row < 5; ++row) {
            const auto highlighted = ui::world_facility_item_highlight(frame, row);
            check(highlighted.x == frame.rows.x && highlighted.y == frame.rows.y + 19 * row - 2 &&
                      highlighted.width == 191 && highlighted.height == 16,
                  "Original selected-item background keeps its191x16 region and y-minus2 offset");
        }
        const auto result_frame = ui::world_facility_items_layout(extent, 77);
        const auto origin = ui::world_facility_skin_origin(result_frame.panel);
        // The old self-drawn result body is retired. Preserve its overlap/containment risk
        // checks against the actual maintained clip and all three number requests instead.
        sim::SteamFacilityImprovementSkinInput result;
        result.raw_page = 77;
        result.frame = 55;
        result.mapchip = source->display_id;
        result.title_widths = std::array<int, 2>{80, 80};
        result.limits = {100, 100, 100};
        result.attributes = {{{11, 15, 4}, {12, 12, 0}, {13, 9, -4}}};
        const auto plan = sim::steam_facility_improvement_skin(result);
        check(plan.has_value(), "Result geometry fixture uses the maintained plan");
        std::optional<std::array<int, 4>> picture;
        std::vector<std::array<int, 4>> clips;
        std::array<int, 3> number_y{};
        std::array<bool, 3> numbers{};
        for (const auto &op : plan->operations) {
            if (const auto *c = std::get_if<sim::SteamFacilityClip>(&op)) {
                if (c->kind == sim::SteamFacilityClipKind::pop) {
                    check(!clips.empty(), "Result source clip stack cannot underflow");
                    clips.pop_back();
                } else
                    clips.push_back(c->rectangle);
            } else if (std::holds_alternative<sim::SteamFacilityMapchip2>(op)) {
                check(!clips.empty(), "Actual facility picture remains inside its source clip");
                picture = clips.back();
            } else if (const auto *n = std::get_if<sim::SteamFacilityNumber>(&op);
                       n && n->parameter >= 0 &&
                       n->kind != sim::SteamFacilityNumberKind::plus_value) {
                numbers[n->parameter] = true;
                number_y[n->parameter] = n->position[1];
            }
        }
        check(
            picture == std::optional<std::array<int, 4>>({80, 74, 80, 64}) && clips.empty() &&
                numbers == std::array<bool, 3>{true, true, true} &&
                number_y == std::array<int, 3>{158, 176, 194} &&
                origin.y + (*picture)[1] >= result_frame.panel.y &&
                origin.x + (*picture)[0] >= result_frame.panel.x &&
                origin.x + (*picture)[0] + (*picture)[2] <=
                    result_frame.panel.x + result_frame.panel.width,
            "Result picture clip, balanced layers and all three rows keep the published geometry");
        auto listing = ui::world_facility_items_view(before, before.scripts.pages.back());
        ui::WorldFacilityItemsInput row_input;
        row_input.click = Vector2{frame.rows.x + 3, frame.rows.y + 4 * 19 + 8};
        row_input.enter = true;
        const auto row_choice = input_of(listing, frame, marker, row_input, false);
        check(row_choice && row_choice->action == Action::select && row_choice->selection == 5,
              "Fifth visible row plus Enter selects the actual scrolled row without using the old "
              "choice");
        check((*picture)[1] + (*picture)[3] < number_y[0] - 9 && number_y[1] - number_y[0] == 18 &&
                  number_y[2] - number_y[1] == 18 &&
                  origin.y + number_y[2] + 18 <= result_frame.panel.y + result_frame.panel.height &&
                  result_frame.panel.y >= 24 &&
                  result_frame.panel.y + result_frame.panel.height <= extent.height - 29,
              "S045 actual clipped picture and complete three-row result remain separate and "
              "contained at minimum size");
        row_input = {};
        row_input.click = Vector2{frame.rows.x + frame.rows.width, frame.rows.y + 8};
        check(!input_of(listing, frame, marker, row_input, false),
              "Researched row hit test excludes its right boundary");
    }
}
} // namespace ark::test
