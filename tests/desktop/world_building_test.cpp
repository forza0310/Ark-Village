// Presentation contracts use sparse page/definition identities and explicit minimal page caches.
// Rule arithmetic/transactions stay in frozen building tests; this suite owns mapping and input.
#include "support/checks.hpp"
#include "support/world_fixture.hpp"
#include "ui/world_building.hpp"
#include "world_build_placement.hpp"
#include "world_editing.hpp"
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
    // 2b479f6 PAGES "fixed APK candidate buildings": missing map layers return null.
    // This supersedes the earlier documented desktop frame1->frame0 accommodation.
    desktop::Sprites sprites(ARK_TEST_ASSETS);
    check(sprites.map_frame("t_resident00.seb", 0) == 0 &&
              sprites.map_frame("t_resident00.seb", 1) == 1 &&
              sprites.map_frame("t_myhome03.seb", 1) == 1,
          "Single-frame resident/house requests keep orientation1 instead of remapping to frame0");
    for (const auto &absent : {std::pair{"t_resident00.seb", 1}, std::pair{"t_resident00.seb", 2},
                               std::pair{"tenant10.seb", 2}, std::pair{"t_inn00.seb", 4}}) {
        // No GPU context exists: attempting the old fallback image would load a texture and
        // fail. Missing maps must return before texture access in each actual draw path.
        sprites.draw(absent.first, absent.second, {});
        sprites.thumbnail(absent.first, absent.second, {0, 0, 64, 32});
        check(sprites.map_image_height(absent.first, absent.second) == 0,
              "Missing map frames have no drawable image or height");
    }
    for (const auto &item : catalogue.facilities) {
        if (!(item.flags & 4) && item.kind != 12)
            continue;
        if (item.kind == 6)
            continue; // Roads select adjacency masks, not facility orientation fragments.
        for (const auto orientation :
             {rules::FacilityOrientation::first, rules::FacilityOrientation::second}) {
            const auto graphic = desktop::world_build_graphic(item, orientation);
            for (const auto &part : graphic.frames)
                check(sprites.map_frame(graphic.sprite, part.first) == part.first,
                      "Buildable/house fragments preserve source requests without clamping: " +
                          std::to_string(item.id));
        }
    }
    for (const auto &invalid : {std::pair{"t_resident00.seb", -1}, std::pair{"tenant10.seb", -1},
                                std::pair{"t_inn00.seb", -1}}) {
        bool rejected = false;
        try {
            (void)sprites.map_frame(invalid.first, invalid.second);
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        check(rejected, "Negative map requests remain invalid");
    }
    bool rejected_non_map{};
    try {
        sprites.draw("chara_hishoko01.seb", 999, {}, WHITE, desktop::Sprites::Binding::secretary);
    } catch (const std::runtime_error &) {
        rejected_non_map = true;
    }
    check(rejected_non_map, "Missing-map behavior does not weaken non-map source frame checks");
    // Crop actual source pixels rather than rescaling the building to its catalogue box.
    for (const auto &[flip_x, flip_y] : {std::pair{false, false}, std::pair{true, false},
                                         std::pair{false, true}, std::pair{true, true}}) {
        const auto clipped = desktop::clip_sprite_blit(
            {{10, 20, flip_x ? -80.F : 80.F, flip_y ? -40.F : 40.F}, {100, 200, 80, 40}},
            {120, 205, 30, 20});
        check(clipped && clipped->source.x == (flip_x ? 40 : 30) &&
                  clipped->source.y == (flip_y ? 35 : 25) &&
                  clipped->source.width == (flip_x ? -30 : 30) &&
                  clipped->source.height == (flip_y ? -20 : 20) && clipped->destination.x == 120 &&
                  clipped->destination.y == 205 && clipped->destination.width == 30 &&
                  clipped->destination.height == 20,
              "Logical clipping preserves the selected source pixels and both sprite flips");
    }
    check(!desktop::clip_sprite_blit({{10, 20, 80, 40}, {100, 200, 80, 40}}, {180, 200, 8, 8}) &&
              !desktop::clip_sprite_blit({{10, 20, 80, 40}, {100, 200, 80, 40}}, {110, 205, 0, 8}),
          "Disjoint and empty catalogue crops produce no sprite blit");
    const auto scaled_crop =
        desktop::clip_sprite_blit({{10, 20, 80, 40}, {100, 200, 160, 80}}, {120, 210, 60, 40});
    check(scaled_crop && scaled_crop->source.x == 20 && scaled_crop->source.y == 25 &&
              scaled_crop->source.width == 30 && scaled_crop->source.height == 20,
          "Clipping respects an existing draw scale without fitting or recentering the sprite");
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
    state.facility_presence.at(18) = 0; // Isolate cached building order from road availability.
    state.scripts.user_flags &= ~32U;
    auto view = ui::world_building_view(state, page);
    check(view.page == 903 && view.initialized && view.catalogs[0].size() == 4 &&
              view.catalogs[0][0].identity == other && view.catalogs[0][1].identity == definition &&
              view.catalogs[0][2].identity == other && view.catalogs[1].empty(),
          "Build tabs preserve published page order, duplicates and actual definition identities");
    check(view.catalogs[0].back().identity == -1 && view.catalogs[0].back().cost == 0 &&
              view.catalogs[0].back().common_image == "destruct00.png" &&
              view.catalogs[0].back().image_source.width == 60 &&
              view.catalogs[0].back().image_source.height == 29 &&
              view.catalogs[0].back().image_offset.x == 4 &&
              view.catalogs[0].back().image_offset.y == 3,
          "Removal is an independent stable identity with its published PNG crop and zero quote");
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
    const auto road = std::find_if(catalogue.facilities.begin(), catalogue.facilities.end(),
                                   [](const auto &item) { return item.id == 18; });
    check(road != catalogue.facilities.end() && road->kind == 6,
          "Published road fixture keeps its source definition and kind");
    const auto road_first = desktop::world_build_graphic(*road, rules::FacilityOrientation::first);
    const auto road_second =
        desktop::world_build_graphic(*road, rules::FacilityOrientation::second);
    check(road_first.frames.size() == 1 && road_first.frames[0].first == 11 &&
              road_second.frames.size() == 1 && road_second.frames[0].first == 1,
          "Source road candidate uses masks11/1 rather than regular building fragment0/1");
    {
        auto editing = state;
        editing.facility_presence.at(18) = 2;
        editing.scripts.user_flags |= 32U;
        const auto cache = editing.build_page_catalogs;
        const auto cash = editing.scene.world.world.ai.accounting.funds();
        const auto random = editing.scene.random.draws();
        auto catalogue_view = ui::world_building_view(editing, page);
        const auto &first_tab = catalogue_view.catalogs[0];
        const int moving_row = 2;
        check(first_tab.size() == 6 && first_tab.front().identity == 18 &&
                  first_tab[1].identity == -1 && first_tab[2].identity == -2 &&
                  first_tab[3].identity == other && first_tab[4].identity == definition &&
                  first_tab[5].identity == other,
              "S057 places editing tools after roads while base order and duplicates stay intact");
        check(first_tab[2].cost == 300 && first_tab[2].common_image == "moveTenant.png" &&
                  first_tab[2].image_source.width == 63 && first_tab[2].image_source.height == 32 &&
                  first_tab[2].image_offset.x == 1 && first_tab[2].image_offset.y == 0 &&
                  first_tab.front().cost ==
                      sim::startup_world_build_quote(editing, 18)->construction_cost,
              "Moving uses its actual fixed quote/PNG and road uses current economy quote");
        editing.build_page_catalogs[page.id][0] = {25};
        editing.facility_free_builds.at(25) = 3;
        catalogue_view = ui::world_building_view(editing, page);
        const auto house =
            std::find_if(catalogue_view.catalogs[0].begin(), catalogue_view.catalogs[0].end(),
                         [](const auto &row) { return row.identity == 25; });
        check(house != catalogue_view.catalogs[0].end() && house->residence_qualifications == 3 &&
                  house->cost == sim::startup_world_build_quote(editing, 25)->construction_cost,
              "Residential H is displayed separately and does not erase the real gold price");
        editing.build_page_catalogs = cache;
        for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{540, 360}}) {
            const auto geometry = ui::world_building_layout(extent);
            ui::WorldBuildingSelection cursor{0, moving_row, 0};
            ui::WorldBuildingInput request;
            request.enter = true;
            const auto action = ui::world_building_input(ui::world_building_view(editing, page),
                                                         geometry, cursor, request, false);
            check(
                action && action->selection == -2 && action->action == Action::select_build,
                "Special catalogue rows keep stable negative identities through responsive input");
        }
        check(editing.build_page_catalogs == cache && editing.scene.random.draws() == random &&
                  editing.scene.world.world.ai.accounting.funds() == cash,
              "Catalogue augmentation never rewrites cache, consumes random or spends money");
        editing.facility_presence.at(18) = 0;
        editing.scripts.user_flags &= ~32U;
        catalogue_view = ui::world_building_view(editing, page);
        check(catalogue_view.catalogs[0].size() == 4 &&
                  catalogue_view.catalogs[0].back().identity == -1,
              "Road absence and flag32 independently gate road and moving rows");
    }
    for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{384, 256},
                              desktop::Extent{540, 360}, desktop::Extent{960, 640}}) {
        const auto layout = ui::world_building_layout(extent);
        check(layout.row_height == 37 && ui::world_building_visible_rows(layout) <= 5 &&
                  (extent.height < 360 || ui::world_building_visible_rows(layout) == 5),
              "Raw21 uses source row pitch and five rows when the desktop height permits");
        check(layout.panel.y >= 24 && layout.panel.y + layout.panel.height <= extent.height - 29 &&
                  layout.rows.y + ui::world_building_visible_rows(layout) * layout.row_height <=
                      layout.cancel.y,
              "Catalogue clears the HUD and playback footer at the actual widescreen canvas size");
        for (int row = 0; row < ui::world_building_visible_rows(layout); ++row) {
            const auto icon = ui::world_building_icon(layout, row);
            check(icon.clip.width == 64 && icon.clip.height == 32 &&
                      icon.clip.y == layout.rows.y + row * 37 + 2 &&
                      icon.anchor.x == icon.clip.x + 2 && icon.anchor.y == icon.clip.y + 10 &&
                      icon.background.r == 190 && icon.background.g == 242 &&
                      icon.background.b == 230,
                  "Raw21 preserves the published crop, source anchor and background");
        }
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
    {
        auto details = initial_world();
        auto sources = *details.rules;
        details.rules = &sources;
        const auto bun =
            std::find_if(details.scene.world.world.facilities.begin(),
                         details.scene.world.world.facilities.end(),
                         [](const auto &f) { return f.second.placement.definition_id == 33; });
        check(bun != details.scene.world.world.facilities.end(), "Actual ordinary bun shop exists");
        auto detail_page = page;
        detail_page.legacy_f = 33;
        details.facility_page_bindings[detail_page.id] = bun->first;
        details.facility_page_neighbours[detail_page.id] = {};
        details.page_phases[detail_page.id] = 0;
        const auto source = std::find_if(sources.facilities.begin(), sources.facilities.end(),
                                         [](const auto &d) { return d.id == 33; });
        source->economy.upgrade_uses = {100, 100}; // Independent display fixture: remaining100-7.
        auto &progress = details.scene.world.world.facility_uses.at(33);
        progress.level = details.scripts.facilities.at(33).level = 2;
        progress.completed_uses = 7;
        const auto untouched = details;
        auto detail = ui::world_building_view(details, detail_page);
        const auto query = app::query_world_facility_detail(details, bun->first);
        check(query.detail && detail.detail_type == app::WorldFacilityTemplate::ordinary &&
                  detail.level == 2 && detail.remaining_uses == 93 &&
                  detail.attributes == query.detail->attributes && !detail.graphic.frames.empty() &&
                  detail.source_names.empty(),
              "Ordinary instance details project actual economy, shared level, remaining uses and "
              "artwork");
        check(details.scene.random.draws() == untouched.scene.random.draws() &&
                  same_world_clock(details, untouched) &&
                  details.scene.world.world.ai.accounting.funds() ==
                      untouched.scene.world.world.ai.accounting.funds() &&
                  details.scene.world.world.facility_uses.at(33).completed_uses == 7 &&
                  details.scripts.facilities.at(33).level == 2,
              "Raw74 detail queries cannot advance time, charge, increment uses or upgrade the "
              "definition");
        progress.level = details.scripts.facilities.at(33).level = 5;
        detail = ui::world_building_view(details, detail_page);
        check(detail.level == 5 && !detail.remaining_uses,
              "Shared level5 displays MAX without inventing a sixth-level remaining count");
        const auto maximum_maintenance = detail.attributes[3];
        // Explicit initialized-page display fixture. These source identities/names are real;
        // this test does not claim their positions form a naturally produced reward list.
        const auto a = details.scene.world.facility_order.front();
        const auto b = details.scene.world.facility_order.back();
        const int a_definition = details.scene.world.world.facilities.at(a).placement.definition_id;
        const int b_definition = details.scene.world.world.facilities.at(b).placement.definition_id;
        details.facility_page_neighbours[detail_page.id] = {{rules::BuildingId{b}, b_definition},
                                                            {rules::BuildingId{a}, a_definition}};
        details.page_phases[detail_page.id] = 1;
        detail = ui::world_building_view(details, detail_page);
        const auto a_source =
            std::find_if(sources.facilities.begin(), sources.facilities.end(),
                         [a_definition](const auto &d) { return d.id == a_definition; });
        const auto b_source =
            std::find_if(sources.facilities.begin(), sources.facilities.end(),
                         [b_definition](const auto &d) { return d.id == b_definition; });
        check(detail.source_names == std::vector<std::string>{b_source->name, a_source->name} &&
                  detail.neighbours == 2 && detail.attributes[3] == maximum_maintenance,
              "Second-page names preserve initialized source order while maintenance stays the "
              "effective instance value");
        for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{384, 256},
                                  desktop::Extent{540, 360}, desktop::Extent{960, 640}}) {
            const auto frame = ui::world_building_layout(extent, 74);
            check(frame.previous.y < frame.body.y && frame.next.y < frame.body.y &&
                      frame.previous.x + frame.previous.width < frame.next.x,
                  "S043/S054 facility page navigation is in the title bar, clear of body controls");
            const auto ordinary =
                ui::world_building_detail_layout(frame, app::WorldFacilityTemplate::ordinary);
            const auto special =
                ui::world_building_detail_layout(frame, app::WorldFacilityTemplate::equipment);
            check(ordinary.picture.width == 97 && ordinary.picture.height == 74 &&
                      ordinary.picture.y + ordinary.picture.height <=
                          frame.body.y + frame.body.height &&
                      ordinary.remaining.y + ordinary.remaining.height <=
                          frame.body.y + frame.body.height &&
                      !CheckCollisionRecs(ordinary.name, ordinary.price) &&
                      !CheckCollisionRecs(ordinary.picture, ordinary.values) &&
                      special.picture.x == frame.body.x + (frame.body.width - 97) / 2 &&
                      ordinary.sources.y + ordinary.sources.height <= frame.cancel.y,
                  "Static raw74 big-picture/value/remaining/source regions remain inside narrow "
                  "and large windows");
            for (const auto type :
                 {app::WorldFacilityTemplate::equipment, app::WorldFacilityTemplate::home,
                  app::WorldFacilityTemplate::recruitment, app::WorldFacilityTemplate::booster}) {
                const auto template_layout = ui::world_building_detail_layout(frame, type);
                check(
                    template_layout.picture.x == frame.body.x + (frame.body.width - 97) / 2 &&
                        template_layout.remaining.y + template_layout.remaining.height <=
                            frame.body.y + frame.body.height,
                    "Equipment/home/recruitment/booster layouts use the centered special template");
            }
            auto scrolling = detail;
            scrolling.source_names.assign(8, a_source->name); // Presentation-only long list input.
            ui::WorldBuildingSelection cursor;
            ui::WorldBuildingInput navigation;
            navigation.wheel_rows = 99;
            check(!ui::world_building_input(scrolling, frame, cursor, navigation, false) &&
                      cursor.first_row > 0 && cursor.first_row < 8,
                  "Long source names can scroll locally without sending a facility transaction");
            const int first = cursor.first_row;
            navigation.up = true;
            check(!ui::world_building_input(scrolling, frame, cursor, navigation, true) &&
                      cursor.first_row == first,
                  "Pending/paused input blocks local source scrolling as well as source actions");
        }
    }
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

    // The existing inn is an eligible two-page facility, while a definition preview has no
    // instance.
    const auto inn = std::find_if(
        state.scene.world.world.facilities.begin(), state.scene.world.world.facilities.end(),
        [](const auto &entry) { return entry.second.placement.definition_id == 28; });
    check(inn != state.scene.world.world.facilities.end(), "Published initial inn instance exists");
    page.legacy_page = 74;
    page.legacy_f = 28;
    state.facility_page_bindings[page.id] = inn->first;
    state.page_phases[page.id] = 0;
    view = ui::world_building_view(state, page);
    input = {};
    input.enter = true;
    check(view.can_use_items && view.can_confirm &&
              ui::world_building_input(view, layout, selection, input, false)->action ==
                  Action::facility_confirm,
          "Ordinary instance details open the maintained item consumer through facility confirm");
    state.page_phases[page.id] = 1;
    view = ui::world_building_view(state, page);
    check(!view.can_use_items && !view.can_confirm &&
              !ui::world_building_input(view, layout, selection, input, false),
          "Neighbour/income second page cannot open item use");

    {
        auto preview_state = initial_world();
        const auto offer = std::find_if(preview_state.rules->facilities.begin(),
                                        preview_state.rules->facilities.end(), [](const auto &d) {
                                            return (d.flags & 4) && d.unlock_rank >= 0 &&
                                                   d.detail != 1 && d.detail != 4 && d.detail != 5;
                                        });
        check(offer != preview_state.rules->facilities.end(),
              "Published ordinary facility offer exists");
        rules::WorldScriptPage commerce;
        commerce.id = 777;
        commerce.kind = rules::WorldScriptPageKind::raw_page;
        commerce.legacy_page = 85;
        commerce.lifecycle = 1;
        preview_state.scripts.pages.clear();
        preview_state.scripts.pages.push_back(commerce);
        preview_state.commerce_pages_initialized.insert(commerce.id);
        preview_state.commerce_page_lists[commerce.id] = {offer->id};
        preview_state.commerce_page_data[commerce.id] = {0, 0, 0, 0, -1, 0};
        auto preview = commerce;
        preview.id = 778;
        preview.legacy_page = 74;
        preview.legacy_f = offer->id;
        preview.legacy_g = 1;
        preview_state.scripts.pages.push_back(preview);
        preview_state.rank = std::max(preview_state.rank, offer->unlock_rank);
        preview_state.facility_presence.at(offer->id) = 0;
        preview_state.facility_definition_page_bindings[preview.id] = offer->id;
        preview_state.page_phases[preview.id] = preview_state.page_counters[preview.id] = 0;
        preview_state.scripts.facilities.at(offer->id).attributes = {411, 22, 33, 44};
        const auto preview_before = preview_state;
        view = ui::world_building_view(preview_state, preview);
        check(
            view.definition_preview && !view.facility && view.page_count == 1 && view.phase == 0 &&
                view.attributes == std::array<std::int64_t, 4>{411, 22, 33, 44} &&
                !view.can_use_items && view.income == 0 && view.neighbours == 0 &&
                !view.graphic.frames.empty(),
            "Definition preview reads shared attributes and artwork with no instance economics or "
            "second page");
        input = {};
        input.enter = true;
        check(ui::world_building_input(view, layout, selection, input, false)->action ==
                  Action::facility_confirm,
              "Definition preview confirm only returns its existing parent through source facility "
              "action");
        input = {};
        input.escape = true;
        check(ui::world_building_input(view, layout, selection, input, false)->action ==
                      Action::facility_cancel &&
                  preview_state.village_points == preview_before.village_points &&
                  preview_state.facility_free_builds == preview_before.facility_free_builds &&
                  preview_state.scene.random.draws() == preview_before.scene.random.draws() &&
                  same_world_clock(preview_state, preview_before),
              "Preview input cannot pay, grant construction qualifications or consume world/random "
              "updates");
        input = {};
        input.right = true;
        check(!ui::world_building_input(view, layout, selection, input, false),
              "Definition preview has no fabricated instance second page");
        preview_state.facility_page_bindings[preview.id] = inn->first;
        bool rejected = false;
        try {
            (void)ui::world_building_view(preview_state, preview);
        } catch (const std::invalid_argument &) {
            rejected = true;
        }
        check(rejected, "A definition preview carrying an instance binding is rejected explicitly");
        preview_state.facility_page_bindings.erase(preview.id);
        const auto shop = std::find_if(
            preview_state.rules->facilities.begin(), preview_state.rules->facilities.end(),
            [](const auto &d) { return d.detail == 4 && d.unlock_rank >= 0; });
        check(shop != preview_state.rules->facilities.end(), "Published armor-shop offer exists");
        preview.legacy_f = shop->id;
        preview_state.scripts.pages.back().legacy_f = shop->id;
        preview_state.facility_definition_page_bindings[preview.id] = shop->id;
        preview_state.commerce_page_lists[commerce.id] = {shop->id};
        preview_state.facility_presence.at(shop->id) = 0;
        preview_state.rank = std::max(preview_state.rank, shop->unlock_rank);
        for (auto &entry : preview_state.catalog)
            if (entry.first.first == 2)
                entry.second.status = 0;
        preview_state.catalog.at({2, 0}).status = 1;
        preview_state.catalog.at({2, 1}).status = 2;
        view = ui::world_building_view(preview_state, preview);
        check(view.definition_preview && view.product_count == 2 && !view.facility &&
                  !view.can_use_items &&
                  view.detail_type == app::WorldFacilityTemplate::equipment &&
                  view.source_names.empty() && view.income == 0,
              "Armor-shop preview counts only already-open merchandise without an instance or "
              "item-use command");
    }

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
        auto selected = desktop::world_build_input(controls, request, false, true);
        check(selected && selected->action == BuildAction::choose && selected->point &&
                  selected->point->x == newly_clicked.x && selected->point->y == newly_clicked.y,
              "A new map click plus Enter only selects B; it cannot submit the previous anchor A");
        request.click = middle(controls.confirm);
        selected = desktop::world_build_input(controls, request, false, true);
        check(
            selected && selected->action == BuildAction::confirm && !selected->point,
            "Confirm button plus Enter emits one confirmation, never an additional map selection");
        for (const auto &button : {std::pair{controls.cancel, BuildAction::cancel},
                                   std::pair{controls.rotate, BuildAction::rotate}}) {
            request.click = middle(button.first);
            selected = desktop::world_build_input(controls, request, false, true);
            check(selected && selected->action == button.second && !selected->point,
                  "Cancel/rotate buttons inside the scene consume their click without choosing a "
                  "cell");
            check(!desktop::world_build_input(controls, request, true, true),
                  "Blocked pending/paused inputs cannot cancel, rotate, select or submit");
        }
        for (const bool enter : {false, true}) {
            request = {};
            request.click = middle(controls.rotate);
            request.enter = enter;
            request.rotate = true;
            selected = desktop::world_build_input(controls, request, false, false);
            check(selected && selected->action == BuildAction::choose && selected->point &&
                      selected->point->x == request.click->x &&
                      selected->point->y == request.click->y,
                  "Hidden rotation region selects its map cell, including simultaneous Enter/R");
            check(!desktop::world_build_input(controls, request, true, false),
                  "Hidden rotation region cannot bypass the pending/paused barrier");
        }
        request = {};
        request.rotate = true;
        for (const bool enter : {false, true}) {
            request.enter = enter;
            check(!desktop::world_build_input(controls, request, false, false),
                  "Unavailable keyboard rotation remains rejected without confirming a lock");
        }
        request = {};
        request.enter = true;
        check(desktop::world_build_input(controls, request, false, true)->action ==
                  BuildAction::confirm,
              "A later separate Enter submits the controller's currently locked selection");
        request = {};
        request.click = Vector2{extent.width / 2.F, 2};
        check(!desktop::world_build_input(controls, request, false, true),
              "HUD clicks outside scene and placement controls produce no world selection");
        request.click = newly_clicked;
        request.escape = true;
        check(desktop::world_build_input(controls, request, false, true)->action ==
                  BuildAction::cancel,
              "Explicit Escape wins over a simultaneous map click");
    }

    // Picking uses independently specified projected ground centres across pan and zoom.
    {
        auto editing = state;
        editing.scene.scene_state = 1;
        editing.build_mode = 2;
        editing.build_anchor = rules::Position{8, 8};
        const auto random = editing.scene.random.draws();
        const auto cash = editing.scene.world.world.ai.accounting.funds();
        auto &tiles = editing.scene.world.world.map;
        for (int y = 8; y <= 10; ++y)
            tiles.cells.at(static_cast<std::size_t>(y * tiles.width + 8)).legacy_state = y - 7;
        auto edit = desktop::world_edit_view(editing, rules::Position{10, 10});
        check(edit.active && edit.mode == 2 && edit.segment.size() == 3 && edit.segment[0].x == 8 &&
                  edit.segment[0].y == 8 && edit.segment[2].x == 8 && edit.segment[2].y == 10 &&
                  edit.road_preview.size() == 2 && edit.road_preview[0].y == 9 &&
                  edit.road_preview[1].y == 10 && edit.current_cell->x == 10 &&
                  edit.current_cell->y == 10 && edit.cancel_label == "中止" &&
                  edit.caption == "铺到哪里呢" && !edit.rotate_allowed,
              "Road tie projects the vertical segment, skips state1 alone and preserves raw cursor "
              "T");
        edit = desktop::world_edit_view(editing, rules::Position{11, 9});
        check(
            edit.segment.size() == 4 && edit.segment[3].x == 11 && edit.segment[3].y == 8 &&
                edit.current_cell->y == 9,
            "Longer horizontal axis retains source ascending order without snapping current cell");
        check(!desktop::world_edit_view(editing, rules::Position{-1, 8}).current_cell &&
                  desktop::world_edit_view(editing, std::nullopt).segment.empty(),
              "Missing/outside pointer produces no stale road or cursor preview");
        for (const int mode : {1, 3, 5, 6, 7}) {
            editing.build_mode = mode;
            edit = desktop::world_edit_view(editing, rules::Position{10, 10});
            check(edit.active && edit.road_preview.empty() &&
                      edit.cancel_label == (mode == 5 || mode == 7 ? "中止" : "返回"),
                  "Editing modes expose source cancellation phase without invented road graphics");
        }
        editing.build_mode = 7;
        editing.build_definition = 2;
        for (const bool rotate : {false, true}) {
            const auto flags = catalogue.facilities.at(2).flags;
            catalogue.facilities.at(2).flags = rotate ? flags | 32U : flags & ~32U;
            edit = desktop::world_edit_view(editing, rules::Position{10, 10});
            check(edit.rotate_allowed == rotate,
                  "Moving rotation control reads actual selected definition flag32");
            for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{540, 360}}) {
                desktop::WorldBuildInput request;
                request.click = middle(desktop::world_build_controls(extent).rotate);
                request.enter = true;
                const auto action = desktop::world_edit_input(edit, extent, request, false);
                check(action &&
                          action->action == (rotate ? desktop::WorldBuildAction::rotate
                                                    : desktop::WorldBuildAction::choose) &&
                          action->point.has_value() != rotate,
                      "Moving rotation region consumes visible controls and selects through hidden "
                      "ones");
            }
            catalogue.facilities.at(2).flags = flags;
        }
        editing.build_mode = 2;
        edit = desktop::world_edit_view(editing, rules::Position{10, 10});
        for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{540, 360}}) {
            desktop::WorldBuildInput request;
            request.click = Vector2{extent.width / 2.F, 90};
            request.enter = true;
            const auto action = desktop::world_edit_input(edit, extent, request, false);
            check(action && action->action == desktop::WorldBuildAction::choose && action->point,
                  "Editing selection plus Enter cannot submit the previous locked endpoint");
            for (const int mode : {1, 2, 3, 5, 6}) {
                editing.build_mode = mode;
                const auto hidden_rotation =
                    desktop::world_edit_view(editing, rules::Position{10, 10});
                request.click = middle(desktop::world_build_controls(extent).rotate);
                const auto selected =
                    desktop::world_edit_input(hidden_rotation, extent, request, false);
                check(selected && selected->action == desktop::WorldBuildAction::choose &&
                          selected->point,
                      "Road/removal/move-selection modes allow map locking under hidden rotation: "
                      "mode=" +
                          std::to_string(mode));
            }
            editing.build_mode = 2;
            request = {};
            request.rotate = true;
            check(!desktop::world_edit_input(edit, extent, request, false),
                  "Road mode rejects a hidden rotate action");
            request.enter = true;
            check(!desktop::world_edit_input(edit, extent, request, true),
                  "Pending editing input remains blocked through shared input arbitration");
        }
        check(editing.scene.random.draws() == random &&
                  editing.scene.world.world.ai.accounting.funds() == cash &&
                  editing.build_anchor->x == 8 && editing.build_anchor->y == 8,
              "Editing projection and input never mutate canonical anchor, random or funds");
    }
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
    state.scene.scene_state = 1;
    state.build_mode = 0;
    const auto draws = state.scene.random.draws();
    const auto funds = state.scene.world.world.ai.accounting.funds();
    for (const auto &[counter, visible] :
         {std::pair{0, true}, std::pair{9, true}, std::pair{10, false}, std::pair{19, false},
          std::pair{20, true}}) {
        state.scene.scene_counter = counter;
        for (const bool paused : {false, true}) {
            state.scene.framework_paused = paused;
            for (const int speed : {0, 1}) {
                state.scene.speed_setting = speed;
                for (int repaint = 0; repaint < 2; ++repaint) {
                    const auto phase = desktop::world_build_preview(
                        state, definition, {8, 8}, rules::FacilityOrientation::first);
                    check(phase.cursor_in_map && phase.graphic_visible == visible &&
                              state.scene.scene_counter == counter,
                          "Candidate blink reads logical counter without advancing on repaint: " +
                              std::to_string(counter));
                }
            }
        }
    }
    state.scene.framework_paused = false;
    state.scene.speed_setting = 0;
    state.scene.scene_counter = 0;
    for (const auto &[mode, visible] :
         {std::pair{0, true}, std::pair{1, false}, std::pair{6, false}, std::pair{7, true}}) {
        state.build_mode = mode;
        check(desktop::world_build_preview(state, definition, {8, 8},
                                           rules::FacilityOrientation::first)
                      .graphic_visible == visible,
              "Normal/moving placement alone selects the source candidate building: mode=" +
                  std::to_string(mode));
    }
    state.build_mode = 0;
    const auto original_flags = catalogue.facilities.at(2).flags;
    for (const bool enabled : {false, true}) {
        catalogue.facilities.at(2).flags = enabled ? original_flags | 32 : original_flags & ~32;
        check(desktop::world_build_preview(state, definition, {8, 8},
                                           rules::FacilityOrientation::first)
                      .rotation_hint == enabled,
              "Rotation prompt follows source definition bit32");
    }
    catalogue.facilities.at(2).flags = original_flags;
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
    check(preview.denial == Denial::occupied && preview.graphic.frames.size() == 4 &&
              preview.graphic_visible,
          "Invalid placement keeps its full ghost artwork while rejecting an occupied outer cell");
    map.cells.at(9 * map.width + 7).legacy_state = 0;
    catalogue.facilities.at(2).economy.construction_cost = 100000000;
    preview =
        desktop::world_build_preview(state, definition, {8, 8}, rules::FacilityOrientation::first);
    check(preview.denial == Denial::insufficient_funds && preview.graphic_visible,
          "Unaffordable source candidate remains visible during the visible logical phase");
    state.build_mode = 7;
    const auto moving =
        desktop::world_build_preview(state, definition, {8, 8}, rules::FacilityOrientation::first);
    check(funds >= 300 && moving.valid() && moving.cost == 300 && moving.graphic_visible &&
              moving.cells.size() == 4 && moving.graphic.frames.size() == 4 &&
              state.scene.random.draws() == draws &&
              state.scene.world.world.ai.accounting.funds() == funds,
          "Moving to a free destination quotes fixed300G with the full ghost, independent of "
          "unaffordable fresh construction and without consuming cash/random");
    map.cells.at(9 * map.width + 7).legacy_state = 10;
    const auto blocked_move =
        desktop::world_build_preview(state, definition, {8, 8}, rules::FacilityOrientation::first);
    check(blocked_move.denial == Denial::occupied && blocked_move.graphic_visible &&
              blocked_move.graphic.frames.size() == 4,
          "Moving preview keeps nonoverlap checks and full ghost despite the fixed fee");
    map.cells.at(9 * map.width + 7).legacy_state = 0;
    state.build_mode = 0;
    catalogue.facilities.at(2).economy.construction_cost = 0;
    check(desktop::world_build_preview(state, definition, {1, 8}, rules::FacilityOrientation::first)
                      .denial == Denial::outside_town &&
              desktop::world_build_preview(state, definition, {0, 8},
                                           rules::FacilityOrientation::first)
                      .denial == Denial::outside_map,
          "Whole footprint distinguishes town fence crossing from map crossing");
    preview =
        desktop::world_build_preview(state, definition, {0, 8}, rules::FacilityOrientation::first);
    check(preview.denial == Denial::outside_map && preview.cells.empty() && preview.cursor_in_map &&
              preview.graphic_visible,
          "A footprint crossing the map edge does not hide an in-map cursor's candidate");
    preview =
        desktop::world_build_preview(state, definition, {-1, 8}, rules::FacilityOrientation::first);
    check(!preview.cursor_in_map && !preview.graphic_visible,
          "An out-of-map cursor hides the candidate independently of placement eligibility");
    check(state.scene.random.draws() == draws &&
              state.scene.world.world.ai.accounting.funds() == funds && !state.build_definition,
          "Preview never spends, consumes random, installs facilities or begins construction mode");
}
} // namespace ark::test
