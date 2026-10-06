// Presentation contracts use sparse page/definition identities and explicit minimal page caches.
// Rule arithmetic/transactions stay in frozen building tests; this suite owns mapping and input.
#include "support/checks.hpp"
#include "support/world_fixture.hpp"
#include "ui/world_building.hpp"
#include "world_build_placement.hpp"
#include <algorithm>
#include <utility>

namespace ark::test {
void world_building() {
    Checks check{"world_building"};
    namespace ui = desktop::ui;
    namespace sim = simulation;
    namespace rules = sim::rules;
    using Action = ui::WorldBuildingAction;
    using Denial = sim::StartupBuildDenial;
    const auto middle = [](Rectangle r) { return Vector2{r.x + r.width / 2, r.y + r.height / 2}; };
    auto state = initial_world();
    auto catalogue = *state.rules;
    state.rules = &catalogue;
    // Real asset metadata covers the single-frame plot that the inn-only artwork examples
    // missed. Preview and installed surfaces both resolve through this CPU-only boundary.
    desktop::Sprites sprites(ARK_TEST_ASSETS);
    check(sprites.map_frame("t_resident00.seb", 0) == 0 &&
              sprites.map_frame("t_resident00.seb", 1) == 0 &&
              sprites.map_frame("t_myhome03.seb", 1) == 0,
          "Single-frame resident/house artwork remains visible in either logical orientation");
    for (const auto &item : catalogue.facilities) {
        if (!(item.flags & 4) && item.kind != 12)
            continue;
        if (item.kind == 6)
            continue; // Roads select adjacency masks, not facility orientation fragments.
        for (const auto orientation :
             {rules::FacilityOrientation::first, rules::FacilityOrientation::second}) {
            const auto graphic = desktop::world_build_graphic(item, orientation);
            for (const auto &part : graphic.frames)
                check(sprites.map_frame(graphic.sprite, part.first) >= 0,
                      "Every buildable/house fragment resolves against shipped assets: " +
                          std::to_string(item.id));
        }
    }
    for (const auto &invalid : {std::pair{"t_resident00.seb", -1}, std::pair{"t_resident00.seb", 2},
                                std::pair{"tenant10.seb", 2}, std::pair{"t_inn00.seb", 4}}) {
        bool rejected = false;
        try {
            (void)sprites.map_frame(invalid.first, invalid.second);
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        check(rejected, "Frame accommodation must not clamp corrupt or multi-fragment indices");
    }
    rules::WorldScriptPage page;
    page.kind = rules::WorldScriptPageKind::raw_page;
    page.legacy_page = 21;
    page.id = 903;
    check(!ui::world_building_view(state, page).initialized && state.build_page_catalogs.empty(),
          "Rendering cannot initialize the build menu's source catalogue");
    const int definition = catalogue.facilities.at(2).id;
    const int other = catalogue.facilities.at(5).id;
    state.build_page_catalogs[page.id][0] = {other, definition, other};
    state.build_page_catalogs[page.id][2] = {definition};
    auto view = ui::world_building_view(state, page);
    check(view.page == 903 && view.initialized && view.catalogs[0].size() == 3 &&
              view.catalogs[0][0].identity == other && view.catalogs[0][1].identity == definition &&
              view.catalogs[0][2].identity == other && view.catalogs[1].empty(),
          "Build tabs preserve published page order, duplicates and actual definition identities");
    check(view.catalogs[0][1].cost ==
              sim::startup_world_build_quote(state, definition)->construction_cost,
          "Displayed cost uses current source economy rather than definition's initial cost");
    check(!view.catalogs[0][1].graphic.sprite.empty() &&
              view.catalogs[0][1].graphic.frames.front().first == 0,
          "Catalogue carries source artwork in initial orientation instead of text-only rows");
    // FACILITIES shape/fragment tables and SPRITE_BINDINGS independently specify these
    // examples. In particular the two inn fragments come from different PNGs in one SEB.
    struct GraphicCase {
        int definition;
        const char *sprite;
        std::vector<std::pair<int, Vector2>> first, second;
    };
    for (const auto &example : std::vector<GraphicCase>{
             {24, "t_resident00.seb", {{0, {0, 0}}}, {{1, {0, 0}}}},
             {28, "tenant10.seb", {{0, {0, 0}}}, {{1, {0, 0}}}},
             {29, "t_inn00.seb", {{0, {30, -15}}, {2, {0, 0}}}, {{1, {-30, -15}}, {3, {0, 0}}}},
             {55,
              "t_circus.seb",
              {{0, {0, -30}}, {2, {-30, -15}}, {4, {30, -15}}, {6, {0, 0}}},
              {{1, {0, -30}}, {3, {30, -15}}, {5, {-30, -15}}, {7, {0, 0}}}}}) {
        const auto item =
            std::find_if(catalogue.facilities.begin(), catalogue.facilities.end(),
                         [&](const auto &value) { return value.id == example.definition; });
        check(item != catalogue.facilities.end(), "Published graphic fixture definition exists");
        for (const auto orientation :
             {rules::FacilityOrientation::first, rules::FacilityOrientation::second}) {
            const auto context = "definition=" + std::to_string(example.definition) +
                                 " orientation=" + std::to_string(static_cast<int>(orientation));
            const auto graphic = desktop::world_build_graphic(*item, orientation);
            const auto &expected =
                orientation == rules::FacilityOrientation::first ? example.first : example.second;
            check(graphic.sprite == example.sprite && graphic.frames.size() == expected.size(),
                  "Complete source tenant graphic: " + context);
            for (std::size_t i = 0; i < expected.size(); ++i)
                check(graphic.frames[i].first == expected[i].first &&
                          graphic.frames[i].second.x == expected[i].second.x &&
                          graphic.frames[i].second.y == expected[i].second.y,
                      "Source frame and raster anchor: " + context +
                          " fragment=" + std::to_string(i));
            if (example.definition != 24)
                for (const auto &part : graphic.frames)
                    check(sprites.map_frame(graphic.sprite, part.first) == part.first,
                          "Authored orientation frames are preserved: " + context);
        }
    }
    for (const auto extent :
         {desktop::Extent{240, 256}, desktop::Extent{540, 360}, desktop::Extent{960, 640}}) {
        const auto layout = ui::world_building_layout(extent);
        check(layout.panel.x >= 0 && layout.panel.y >= 0 &&
                  layout.panel.x + layout.panel.width <= extent.width &&
                  layout.panel.y + layout.panel.height <= extent.height &&
                  layout.rows.y + ui::world_building_visible_rows(layout) * layout.row_height <=
                      layout.cancel.y &&
                  !CheckCollisionRecs(layout.previous, layout.cancel) &&
                  !CheckCollisionRecs(layout.next, layout.confirm),
              "Responsive page rows and navigation remain on-screen without button overlaps");
        ui::WorldBuildingSelection selection;
        ui::WorldBuildingInput input;
        input.down = true;
        (void)ui::world_building_input(view, layout, selection, input, false);
        input = {};
        input.enter = true;
        auto intent = ui::world_building_input(view, layout, selection, input, false);
        check(intent && intent->action == Action::select_build && intent->page == 903 &&
                  intent->selection == definition,
              "Keyboard selection submits stable definition and source page IDs");
        input.click = middle(layout.tabs[2]);
        input.enter = false;
        (void)ui::world_building_input(view, layout, selection, input, false);
        check(selection.tab == 2 && selection.selected == 0 && selection.first_row == 0,
              "Changing category resets desktop list cursor without changing source catalogue");
        input.click = middle(layout.tabs[1]);
        (void)ui::world_building_input(view, layout, selection, input, false);
        input = {};
        input.enter = true;
        check(!ui::world_building_input(view, layout, selection, input, false),
              "An empty source category never sends a fictitious build definition");
        input.escape = true;
        check(ui::world_building_input(view, layout, selection, input, false)->action ==
                  Action::cancel_build,
              "Back cancels the source build menu even when its tab is empty");
        input.right = true;
        check(!ui::world_building_input(view, layout, selection, input, true) && selection.tab == 1,
              "Pause/pending barrier blocks page actions and cursor mutation together");
    }
    const auto layout = ui::world_building_layout({540, 360});
    ui::WorldBuildingSelection selection;
    ui::WorldBuildingInput input;
    page.legacy_page = 80;
    state.residence_page_candidates[page.id] = {catalogue.humans.at(3).identity,
                                                catalogue.humans.at(1).identity};
    catalogue.humans.at(3).residence_fee = 901;
    view = ui::world_building_view(state, page);
    check(view.residents[0].identity == catalogue.humans.at(3).identity &&
              view.residents[0].name == catalogue.humans.at(3).name &&
              view.residents[0].cost == 901,
          "Residence candidates use actual human identity and residence fee, not build price");
    input.click = middle({layout.rows.x, layout.rows.y, layout.rows.width, layout.row_height});
    auto intent = ui::world_building_input(view, layout, selection, input, false);
    check(intent && intent->action == Action::residence_select &&
              intent->selection == catalogue.humans.at(3).identity,
          "Residence click sends the selected human ID rather than list index");
    state.residence_page_candidates.at(page.id).clear();
    view = ui::world_building_view(state, page);
    input = {};
    input.enter = true;
    check(!view.can_confirm && !ui::world_building_input(view, layout, selection, input, false),
          "Empty residence pages cannot confirm a stale previous candidate");

    const auto instance = state.scene.world.world.facilities.begin()->first;
    page.legacy_page = 74;
    page.legacy_f = state.scene.world.world.facilities.at(instance).placement.definition_id;
    state.facility_page_bindings[page.id] = instance;
    state.facility_page_neighbours[page.id] = {};
    state.page_phases[page.id] = 0;
    state.facility_monthly_cash.at(instance).at(state.scene.calendar.month)[0] = 714;
    view = ui::world_building_view(state, page);
    check(view.facility == instance && view.income == 714 && view.neighbours == 0 &&
              view.attributes ==
                  sim::startup_world_facility_values(state, instance)->instance_attributes,
          "Facility view binds source instance, live economy and actual monthly cash");
    // Action gating must use current view phase/count, not assume every facility has two pages.
    view.page_count = 1;
    view.can_confirm = false;
    input.right = true;
    check(!ui::world_building_input(view, layout, selection, input, false),
          "Unsupported next/confirm cannot fabricate shop or booster consumers");
    view.page_count = 2;
    intent = ui::world_building_input(view, layout, selection, input, false);
    check(intent && intent->action == Action::facility_next && intent->page == page.id,
          "Two-page navigation submits source action instead of local phase mutation");
    input = {};
    input.escape = true;
    check(ui::world_building_input(view, layout, selection, input, false)->action ==
              Action::facility_cancel,
          "Facility back closes its own bound page");
    page.legacy_page = 81;
    check(!ui::world_building_view(state, page).initialized,
          "Upgrade display cannot initialize or apply an unconsumed source upgrade");
    state.facility_upgrade_initialized.insert(page.id);
    state.facility_upgrade_display = {{{11, 12, 13}, {14, 15, 16}, {3, 3, 3}}};
    view = ui::world_building_view(state, page);
    check(view.initialized && view.upgrade[0][2] == 13 && view.upgrade[1][0] == 14,
          "Upgrade display preserves the source old/new/delta matrix without recalculation");
    check(!ui::world_building_input(view, layout, selection, input, false),
          "Escape does not skip the upgrade consumer's required confirmation phases");
    input = {};
    input.enter = true;
    check(ui::world_building_input(view, layout, selection, input, false)->action ==
              Action::confirm_upgrade,
          "Upgrade sends generic source confirmation, not a second upgrade transaction");

    // Input arbitration is independent of Owner and commit eligibility. A previously locked
    // anchor A must survive until the controller consumes choose(B), never confirm(A) first.
    for (const auto extent :
         {desktop::Extent{240, 256}, desktop::Extent{540, 360}, desktop::Extent{960, 640}}) {
        const auto controls = desktop::world_build_controls(extent);
        using BuildAction = desktop::WorldBuildAction;
        desktop::WorldBuildInput request;
        const Vector2 newly_clicked{extent.width / 2.F, 90};
        request.click = newly_clicked;
        request.enter = true;
        auto selected = desktop::world_build_input(controls, request, false);
        check(selected && selected->action == BuildAction::choose && selected->point &&
                  selected->point->x == newly_clicked.x && selected->point->y == newly_clicked.y,
              "A new map click plus Enter only selects B; it cannot submit the previous anchor A");
        request.click = middle(controls.confirm);
        selected = desktop::world_build_input(controls, request, false);
        check(
            selected && selected->action == BuildAction::confirm && !selected->point,
            "Confirm button plus Enter emits one confirmation, never an additional map selection");
        for (const auto &button : {std::pair{controls.cancel, BuildAction::cancel},
                                   std::pair{controls.rotate, BuildAction::rotate}}) {
            request.click = middle(button.first);
            selected = desktop::world_build_input(controls, request, false);
            check(selected && selected->action == button.second && !selected->point,
                  "Cancel/rotate buttons inside the scene consume their click without choosing a "
                  "cell");
            check(!desktop::world_build_input(controls, request, true),
                  "Blocked pending/paused inputs cannot cancel, rotate, select or submit");
        }
        request = {};
        request.enter = true;
        check(desktop::world_build_input(controls, request, false)->action == BuildAction::confirm,
              "A later separate Enter submits the controller's currently locked selection");
        request = {};
        request.click = Vector2{extent.width / 2.F, 2};
        check(!desktop::world_build_input(controls, request, false),
              "HUD clicks outside scene and placement controls produce no world selection");
        request.click = newly_clicked;
        request.escape = true;
        check(desktop::world_build_input(controls, request, false)->action == BuildAction::cancel,
              "Explicit Escape wins over a simultaneous map click");
    }

    // Picking uses independently specified projected ground centres across pan and zoom.
    for (float zoom : {.5F, 1.F, 2.F}) {
        desktop::WorldCameraView camera{{23.5F, -17.F}, {3, 19, 539, 299}};
        for (const rules::Position cell : {rules::Position{2, 3}, {11, 14}, {22, 22}}) {
            const float px = zoom * (271 + 30 * (cell.x + cell.y) + 30 - 23.5F);
            const float py = zoom * (159 - 15 * (cell.y - cell.x) - 17.F);
            const auto pick = desktop::world_pick_cell(state, camera, {px, py}, zoom);
            check(pick && pick->x == cell.x && pick->y == cell.y,
                  "Ground-centre picking agrees with canonical raster camera and zoom");
        }
        check(!desktop::world_pick_cell(state, camera, {-10000, -10000}, zoom),
              "Picking outside map returns no cell rather than clamping to an edge building");
    }
    // Minimal placement fixture: no actor/world updates and no duplicate transaction algorithm.
    auto &map = state.scene.world.world.map;
    for (auto &tile : map.cells) {
        tile.legacy_state = 0;
        tile.facility.reset();
    }
    catalogue.facilities.at(2).shape = 2;
    catalogue.facilities.at(2).economy.construction_cost = 0;
    catalogue.fences.at(state.fence_level) = {{{0, map.height - 1}, {map.width - 1, 0}}};
    const auto draws = state.scene.random.draws();
    const auto funds = state.scene.world.world.ai.accounting.funds();
    auto preview =
        desktop::world_build_preview(state, definition, {8, 8}, rules::FacilityOrientation::first);
    check(preview.valid() && preview.cells.size() == 4 && preview.cells[0].position.x == 7 &&
              preview.cells[0].position.y == 9 && preview.graphic.frames.size() == 4,
          "Construction ghost includes the entire rotated source footprint");
    const auto pair_first =
        desktop::world_build_preview(state, 29, {8, 8}, rules::FacilityOrientation::first);
    const auto pair_second =
        desktop::world_build_preview(state, 29, {8, 8}, rules::FacilityOrientation::second);
    check(pair_first.valid() && pair_second.valid() && pair_first.graphic.sprite == "t_inn00.seb" &&
              pair_second.graphic.sprite == pair_first.graphic.sprite &&
              pair_first.cells[0].position.x == 8 && pair_first.cells[0].position.y == 9 &&
              pair_second.cells[0].position.x == 7 && pair_second.cells[0].position.y == 8 &&
              pair_first.graphic.frames[0].first == 0 && pair_first.graphic.frames[1].first == 2 &&
              pair_second.graphic.frames[0].first == 1 && pair_second.graphic.frames[1].first == 3,
          "Rotated placement changes both the full source tenant artwork and its occupied cells");
    map.cells.at(9 * map.width + 7).legacy_state = 10;
    preview =
        desktop::world_build_preview(state, definition, {8, 8}, rules::FacilityOrientation::first);
    check(preview.denial == Denial::occupied && preview.graphic.frames.size() == 4,
          "Invalid placement keeps its full ghost artwork while rejecting an occupied outer cell");
    map.cells.at(9 * map.width + 7).legacy_state = 0;
    check(desktop::world_build_preview(state, definition, {1, 8}, rules::FacilityOrientation::first)
                      .denial == Denial::outside_town &&
              desktop::world_build_preview(state, definition, {0, 8},
                                           rules::FacilityOrientation::first)
                      .denial == Denial::outside_map,
          "Whole footprint distinguishes town fence crossing from map crossing");
    check(state.scene.random.draws() == draws &&
              state.scene.world.world.ai.accounting.funds() == funds && !state.build_definition,
          "Preview never spends, consumes random, installs facilities or begins construction mode");
}
} // namespace ark::test
