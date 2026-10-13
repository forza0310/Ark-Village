#include "world_management.hpp"
#include "../ui/common/skin.hpp"
#include "../ui/system/world_menu.hpp"
#include "world_save_menu.hpp"
#include <algorithm>
#include <cmath>

namespace ark::desktop {
namespace {
using State = app::WorldState;
using Page = simulation::rules::WorldScriptPage;
using Denial = simulation::StartupBuildDenial;
bool hit(std::optional<Vector2> point, Rectangle box) {
    return point && CheckCollisionPointRec(*point, box);
}
std::string refusal(Denial denial) {
    switch (denial) {
    case Denial::insufficient_funds:
        return "资金不足";
    case Denial::outside_map:
        return "超出地图";
    case Denial::outside_town:
        return "请建在村庄范围内";
    case Denial::occupied:
        return "位置已被占用";
    case Denial::unavailable:
        return "当前不可建设";
    default:
        return "页面或选择已变化，请重试";
    }
}
} // namespace
void WorldManagement::queued(std::uint64_t serial) {
    pending_ = serial;
    feedback_.clear();
}
void WorldManagement::observe(const app::WorldFrame &frame) {
    if (generation_ != frame.generation) {
        generation_ = frame.generation;
        receipt_page_ = 0;
        commerce_amount_.reset();
        building_ = {};
        manual_layout_.reset();
        commerce_top_controls_.reset();
        commerce_facility_controls_.reset();
        commerce_facility_gesture_ = {};
        human_controls_.reset();
        human_gesture_ = {};
        facility_controls_.reset();
        facility_items_ = {};
        facility_catalog_gesture_ = {};
        village_controls_.reset();
        village_gesture_ = {};
        magic_controls_.reset();
        magic_gesture_ = {};
    }
    if (!pending_)
        return;
    const auto result = std::find_if(frame.command_results.begin(), frame.command_results.end(),
                                     [&](const auto &r) { return r.serial == pending_; });
    if (result != frame.command_results.end()) {
        if (result->commerce_amount) {
            commerce_amount_ = result->commerce_amount;
            receipt_page_ = result->page;
        }
        if (result->outcome == app::WorldCommandOutcome::rejected)
            feedback_ = refusal(result->build_denial);
        if (result->created || (result->kind == app::WorldCommandKind::confirm_edit &&
                                result->outcome == app::WorldCommandOutcome::applied))
            anchor_.reset();
        pending_ = 0;
    } else if (frame.last_command_serial >= pending_) {
        pending_ = 0; // Ordinary animation-page acknowledgements use the shared serial fence.
    }
}
bool WorldManagement::input_page(const State &state, const Page &page, Extent extent,
                                 std::optional<Vector2> mouse, bool click, bool back,
                                 bool keyboard_event, bool blocked, app::WorldSession &session) {
    if (page_ != page.id) {
        page_ = page.id;
        manual_layout_.reset();
        commerce_top_controls_.reset();
        commerce_facility_controls_.reset();
        commerce_facility_gesture_ = {};
        human_controls_.reset();
        human_gesture_ = {};
        building_ = {};
        facility_controls_.reset();
        facility_items_.marked_row.reset();
        facility_items_.scrolling = false;
        facility_catalog_gesture_.scrolling = false;
        award_ = {};
        village_controls_.reset();
        village_gesture_.scrolling = false;
        magic_controls_.reset();
        magic_gesture_ = {};
        feedback_.clear();
    }
    blocked = blocked || pending();
    if (keyboard_event) {
        building_.marked_definition.reset();
        commerce_facility_gesture_.marked_row.reset();
    }
    const auto point = click ? mouse : std::nullopt;
    const bool enter = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
                       (ui::world_facility_items_page(page) && IsKeyPressed(KEY_SPACE));
    const bool escape = back;
    const bool up = IsKeyPressed(KEY_UP), down = IsKeyPressed(KEY_DOWN);
    if (ui::world_manual_page(page)) {
        const auto view = ui::world_manual_view(state, page);
        const ui::WorldManualInput input{point, enter, IsKeyPressed(KEY_RIGHT),
                                         IsKeyPressed(KEY_LEFT), escape};
        if (const auto action =
                ui::world_manual_input(view, extent, manual_layout_, input, blocked))
            queued(session.act_manual(page.id, *action));
        return true;
    }
    if (page.kind == simulation::rules::WorldScriptPageKind::raw_page && page.legacy_page == 14) {
        if (!blocked && world_source_save_ready(state, page.id) &&
            (enter || escape || hit(point, world_source_save_confirm(extent))))
            queued(session.ack_page(page.id));
        return true;
    }
    if (ui::world_source_menu_page(page)) {
        const auto view = simulation::inspect_startup_world_menu_page(state, page.id);
        if (!view)
            return true;
        const auto layout = ui::world_source_menu_layout(*view, extent);
        const ui::WorldMenuInput input{point, false, escape, up, down, enter};
        if (const auto action = ui::world_source_menu_input(
                *view, layout, input, IsKeyPressed(KEY_LEFT), IsKeyPressed(KEY_RIGHT),
                blocked || page.lifecycle != 2 || state.scene.framework_paused)) {
            if (view->raw == 10 && !action->up && !action->down &&
                (action->confirm || action->right) && view->tags.at(view->selection) == 20)
                queued(session.open_source_save_menu(page.id));
            else
                queued(session.act_menu(page.id, *action));
        }
        return true;
    }
    if (ui::world_information_page(page)) {
        const auto view = ui::world_information_view(state, page);
        const auto layout =
            ui::world_information_layout(extent, page.legacy_page, view.menu_position);
        const ui::WorldInformationInput input{
            point,
            up,
            down,
            IsKeyPressed(KEY_LEFT),
            IsKeyPressed(KEY_RIGHT),
            enter,
            escape,
            hit(mouse, page.legacy_page == 9
                           ? Rectangle{layout.menu_rows[0].x, layout.menu_rows[0].y, 90, 140}
                           : layout.rows)
                ? -static_cast<int>(GetMouseWheelMove() * 2)
                : 0};
        if (const auto action = ui::world_information_input(view, layout, input, blocked))
            queued(session.act_information(page.id, *action));
        return true;
    }
    if (ui::world_magic_pot_page(page)) {
        const auto view = ui::world_magic_pot_view(state, page);
        const auto layout = ui::world_magic_pot_layout(extent, view.raw);
        ui::WorldMagicPotInput input{
            point,
            enter,
            escape,
            up,
            down,
            IsKeyPressed(KEY_LEFT),
            IsKeyPressed(KEY_RIGHT),
            hit(mouse, layout.rows) ? -static_cast<int>(GetMouseWheelMove() * 2) : 0,
            keyboard_event};
        input.pointer = mouse;
        input.pointer_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        input.pointer_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        input.pointer_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
        if (const auto intent = ui::world_magic_pot_input(view, layout, input, blocked,
                                                          magic_controls_, &magic_gesture_))
            queued(session.input_magic_pot(page.id, *intent));
        return true;
    }
    if (ui::world_facility_catalog_page(page)) {
        const auto view = ui::world_facility_catalog_view(state, page);
        const auto layout = ui::world_facility_catalog_layout(extent, view.raw);
        ui::WorldFacilityCatalogInput input{
            point,
            enter,
            escape,
            up,
            down,
            IsKeyPressed(KEY_LEFT),
            IsKeyPressed(KEY_RIGHT),
            IsKeyPressed(KEY_I),
            hit(mouse, layout.rows) ? -static_cast<int>(GetMouseWheelMove() * 2) : 0};
        input.pointer = mouse;
        input.pointer_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        input.pointer_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        input.pointer_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
        if (const auto intent =
                ui::world_facility_catalog_input(view, layout, input, blocked, facility_controls_,
                                                 extent, &facility_catalog_gesture_))
            queued(intent->input
                       ? session.input_facility_catalog(page.id, *intent->input)
                       : session.act_facility_catalog(page.id, intent->action, intent->selection));
        return true;
    }
    if (ui::world_commerce_page(page) && page.legacy_page == 83) {
        const auto view = ui::world_commerce_top_view(state, page);
        ui::WorldCommerceInput input;
        input.click = point;
        input.enter = enter;
        input.escape = escape;
        input.up = up;
        input.down = down;
        input.focus_lost = !IsWindowFocused();
        if (input.focus_lost)
            commerce_top_controls_.reset();
        if (const auto action =
                ui::world_commerce_top_input(view, extent, commerce_top_controls_, input, blocked))
            queued(session.input_commerce_top(page.id, *action));
        return true;
    }
    if (ui::world_commerce_page(page) && page.legacy_page == 85) {
        const auto view = ui::world_commerce_view(state, page);
        ui::WorldCommerceInput input;
        input.click = point;
        input.enter = enter;
        input.escape = escape;
        input.up = up;
        input.down = down;
        input.inspect = IsKeyPressed(KEY_I);
        input.keyboard_event = keyboard_event;
        input.focus_lost = !IsWindowFocused();
        input.pointer = mouse;
        input.pointer_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        input.pointer_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        input.pointer_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
        if (commerce_facility_controls_ &&
            (hit(mouse, commerce_facility_controls_->scroll.value_or(Rectangle{})) ||
             std::any_of(commerce_facility_controls_->rows.begin(),
                         commerce_facility_controls_->rows.end(),
                         [&](const auto &row) { return hit(mouse, row.rectangle); })))
            input.wheel_rows = -static_cast<int>(GetMouseWheelMove() * 2);
        if (input.focus_lost)
            commerce_facility_controls_.reset();
        if (const auto action =
                ui::world_commerce_facility_input(view, extent, commerce_facility_controls_,
                                                  commerce_facility_gesture_, input, blocked))
            queued(session.input_commerce_facility(page.id, *action));
        return true;
    }
    if (ui::world_commerce_page(page)) {
        const auto view = ui::world_commerce_view(state, page);
        const auto layout = ui::world_commerce_layout(extent);
        const ui::WorldCommerceInput input{
            point,
            enter,
            escape,
            up,
            down,
            IsKeyPressed(KEY_LEFT),
            IsKeyPressed(KEY_RIGHT),
            IsKeyPressed(KEY_I),
            hit(mouse, layout.rows) ? -static_cast<int>(GetMouseWheelMove() * 2) : 0};
        if (const auto intent = ui::world_commerce_input(view, layout, input, blocked))
            queued(session.act_commerce(page.id, intent->action, intent->selection));
        return true;
    }
    if (ui::world_facility_items_page(page)) {
        const auto view = ui::world_facility_items_view(state, page);
        const auto layout = ui::world_facility_items_layout(extent, view.raw);
        ui::WorldFacilityItemsInput input{
            point,         enter,
            escape,        up,
            down,          hit(mouse, layout.rows) ? -static_cast<int>(GetMouseWheelMove() * 2) : 0,
            keyboard_event};
        input.pointer = mouse;
        input.pointer_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        input.pointer_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        input.pointer_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
        if (const auto intent = ui::world_facility_items_input(view, layout, facility_items_, input,
                                                               blocked, facility_controls_, extent))
            queued(intent->input
                       ? session.input_facility_item(page.id, *intent->input)
                       : session.act_facility_item(page.id, intent->action, intent->selection));
        return true;
    }
    if (ui::world_village_activity_page(page)) {
        const auto view = ui::world_village_activity_view(state, page);
        const auto layout = ui::world_village_activity_layout(extent, view.raw);
        ui::WorldVillageActivityInput input{
            point,  up,
            down,   enter,
            escape, hit(mouse, layout.rows) ? -static_cast<int>(GetMouseWheelMove() * 2) : 0};
        input.pointer = mouse;
        input.pointer_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        input.pointer_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        input.pointer_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
        if (const auto intent = ui::world_village_activity_input(
                view, layout, input, blocked, village_controls_, &village_gesture_))
            queued(session.input_village_activity(page.id, *intent->input));
        return true;
    }
    if (ui::world_human_page(page)) {
        const auto view = ui::world_human_view(state, page);
        const auto layout = ui::world_human_layout(extent);
        ui::WorldHumanInput input;
        input.click = point;
        input.enter = enter;
        input.escape = escape;
        input.up = up;
        input.down = down;
        input.left = IsKeyPressed(KEY_LEFT);
        input.right = IsKeyPressed(KEY_RIGHT);
        input.wheel_rows = -static_cast<int>(GetMouseWheelMove() * 2);
        input.professions = IsKeyPressed(KEY_P);
        input.gifts = IsKeyPressed(KEY_G);
        input.inspect = IsKeyPressed(KEY_I);
        input.pointer = mouse;
        input.pointer_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        input.pointer_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        input.pointer_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
        if (keyboard_event)
            human_gesture_ = {};
        if (const auto intent = ui::world_human_input(view, layout, input, blocked, human_controls_,
                                                      extent, &human_gesture_)) {
            if (intent->input)
                queued(session.input_human(page.id, *intent->input));
            else
                queued(session.act_human(page.id, intent->action, intent->selection));
        }
        return true;
    }
    if (ui::world_tax_page(page)) {
        const auto view = ui::world_tax_view(state, page);
        const auto layout = ui::world_tax_layout(extent);
        ui::WorldHumanInput input;
        input.click = point;
        input.enter = enter;
        input.up = up;
        input.down = down;
        input.wheel_rows = -static_cast<int>(GetMouseWheelMove() * 2);
        if (const auto intent = ui::world_tax_input(view, layout, input, blocked))
            queued(session.act_tax(page.id, intent->action, intent->selection));
        return true;
    }
    if (ui::world_building_page(page)) {
        const auto view = ui::world_building_view(state, page);
        const auto layout = ui::world_building_layout(extent, page.legacy_page);
        ui::WorldBuildingInput input{
            point,
            enter,
            escape,
            up,
            down,
            IsKeyPressed(KEY_LEFT),
            IsKeyPressed(KEY_RIGHT),
            hit(mouse, layout.rows) ? -static_cast<int>(GetMouseWheelMove() * 2) : 0};
        const auto intent = ui::world_building_input(view, layout, building_, input, blocked);
        if (!intent)
            return true;
        using Action = ui::WorldBuildingAction;
        using Facility = simulation::StartupFacilityPageAction;
        switch (intent->action) {
        case Action::select_build:
            queued(session.select_build_menu(page.id, intent->selection));
            break;
        case Action::cancel_build:
            queued(session.cancel_build_menu(page.id));
            break;
        case Action::facility_previous:
            queued(session.act_facility(page.id, Facility::previous));
            break;
        case Action::facility_next:
            queued(session.act_facility(page.id, Facility::next));
            break;
        case Action::facility_confirm:
            queued(session.act_facility(page.id, Facility::confirm));
            break;
        case Action::facility_cancel:
            queued(session.act_facility(page.id, Facility::cancel));
            break;
        case Action::residence_select:
            queued(session.act_residence(page.id, intent->selection));
            break;
        case Action::residence_cancel:
            queued(session.act_residence(page.id, 0, true));
            break;
        case Action::confirm_upgrade:
            queued(session.ack_page(page.id));
            break;
        }
        return true;
    }
    if (page.kind == simulation::rules::WorldScriptPageKind::raw_page && page.legacy_page == 87) {
        const auto view = ui::world_award_view(state, page.id);
        const auto layout =
            ui::world_award_layout(extent, view.termination_pending || view.pending_human);
        ui::WorldAwardInput input{
            point,
            enter,
            escape,
            up,
            down,
            IsKeyPressed(KEY_LEFT),
            IsKeyPressed(KEY_RIGHT),
            hit(mouse, layout.rows) ? -static_cast<int>(GetMouseWheelMove() * 2) : 0};
        if (const auto intent = ui::world_award_input(view, layout, award_, input, blocked))
            queued(session.act_award(page.id, intent->action, intent->selection));
        return true;
    }
    if (ui::world_progression_page(page)) {
        const auto view = ui::world_progression_view(state, page);
        const auto layout = ui::world_progression_layout(extent, view.raw);
        if (const auto intent = ui::world_progression_input(
                view, layout, {point, enter, escape, up, down}, blocked, village_controls_))
            queued(intent->input ? session.input_rank(page.id, *intent->input)
                   : view.raw == 93
                       ? session.act_commerce(page.id, simulation::StartupCommerceAction::confirm)
                       : session.ack_page(page.id));
        return true;
    }
    return false;
}
bool WorldManagement::draw_page(const State &state, const Page &page, Extent extent,
                                const ui::Skin &skin, bool enabled) const {
    enabled = enabled && !pending();
    if (ui::world_manual_page(page)) {
        manual_layout_ =
            ui::draw_world_manual(state, ui::world_manual_view(state, page), extent, skin, enabled);
        return true;
    }
    if (page.kind == simulation::rules::WorldScriptPageKind::raw_page && page.legacy_page == 14) {
        draw_world_source_save(state, page.id, extent, skin);
    } else if (ui::world_source_menu_page(page)) {
        ui::draw_world_source_menus(state, extent, skin, enabled, feedback_);
    } else if (ui::world_information_page(page)) {
        if (page.legacy_page == 9)
            ui::draw_world_source_menus(state, extent, skin, enabled);
        const auto view = ui::world_information_view(state, page);
        ui::draw_world_information(
            state, view, ui::world_information_layout(extent, page.legacy_page, view.menu_position),
            skin, enabled, feedback_);
    } else if (ui::world_magic_pot_page(page)) {
        auto view = ui::world_magic_pot_view(state, page);
        ui::prepare_world_magic_pot(view, state, skin, magic_gesture_.scrolling);
        magic_controls_ = ui::draw_world_magic_pot(
            view, ui::world_magic_pot_layout(extent, view.raw), skin, enabled, feedback_);
    } else if (ui::world_facility_catalog_page(page)) {
        auto view = ui::world_facility_catalog_view(state, page);
        ui::prepare_world_facility_catalog(view, state, skin, facility_catalog_gesture_.scrolling);
        facility_controls_ = ui::draw_world_facility_catalog(
            view, ui::world_facility_catalog_layout(extent, view.raw), skin, enabled, feedback_);
    } else if (ui::world_commerce_page(page) && page.legacy_page == 83) {
        auto view = ui::world_commerce_top_view(state, page);
        ui::prepare_world_commerce_top(view, skin);
        const auto controls = ui::draw_world_commerce_top(view, extent, skin, enabled, feedback_);
        commerce_top_controls_ = IsWindowFocused() ? controls : std::nullopt;
    } else if (ui::world_commerce_page(page) && page.legacy_page >= 85) {
        auto view = ui::world_commerce_view(state, page);
        ui::prepare_world_commerce_source(view, state, skin, commerce_facility_gesture_.scrolling);
        const auto controls =
            ui::draw_world_commerce_source(view, extent, skin, enabled, feedback_);
        commerce_facility_controls_ = IsWindowFocused() ? controls : std::nullopt;
    } else if (ui::world_commerce_page(page)) {
        const auto view = ui::world_commerce_view(state, page);
        ui::draw_world_commerce(view, ui::world_commerce_layout(extent), skin, enabled, feedback_,
                                receipt_page_ == page.id && view.feedback_counter > 0
                                    ? commerce_amount_
                                    : std::nullopt);
    } else if (ui::world_facility_items_page(page)) {
        auto view = ui::world_facility_items_view(state, page);
        ui::prepare_world_facility_items(view, state, skin, facility_items_.scrolling);
        facility_controls_ = ui::draw_world_facility_items(
            view, ui::world_facility_items_layout(extent, view.raw), skin, enabled, feedback_);
    } else if (ui::world_village_activity_page(page)) {
        auto view = ui::world_village_activity_view(state, page);
        ui::prepare_world_village_activity(view, state, skin, village_gesture_.scrolling);
        village_controls_ = ui::draw_world_village_activity(
            view, ui::world_village_activity_layout(extent, view.raw), skin, enabled, feedback_);
    } else if (ui::world_human_page(page)) {
        const auto down_width =
            static_cast<int>(std::ceil(skin.text.width(ui::world_human_down_text)));
        auto view = ui::world_human_view(state, page, down_width);
        ui::prepare_world_human_detail(view, state, skin, human_gesture_.scrolling);
        human_controls_ =
            ui::world_human_painted_controls(view, ui::world_human_layout(extent), extent, skin);
        ui::draw_world_human(view, ui::world_human_layout(extent), skin, enabled, feedback_);
    } else if (ui::world_tax_page(page)) {
        ui::draw_world_tax(ui::world_tax_view(state, page), ui::world_tax_layout(extent), skin,
                           enabled);
    } else if (ui::world_building_page(page)) {
        auto view = ui::world_building_view(state, page);
        ui::prepare_world_building_detail(view, state, building_, skin);
        ui::draw_world_building(view, ui::world_building_layout(extent, page.legacy_page), skin,
                                building_, enabled, feedback_);
    } else if (page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
               page.legacy_page == 87) {
        const auto view = ui::world_award_view(state, page.id);
        ui::draw_world_award(
            view, ui::world_award_layout(extent, view.termination_pending || view.pending_human),
            skin, award_, enabled);
    } else if (ui::world_progression_page(page)) {
        auto view = ui::world_progression_view(state, page);
        ui::prepare_world_progression(view, state, skin);
        village_controls_ = ui::draw_world_progression(
            view, ui::world_progression_layout(extent, view.raw), skin, enabled);
    } else {
        return false;
    }
    return true;
}
bool WorldManagement::input_scene(const State &state, const WorldCameraView &view, Extent extent,
                                  std::optional<Vector2> mouse, bool click, float zoom, bool back,
                                  bool blocked, app::WorldSession &session,
                                  const SpritePickMap &picks) {
    const bool editing = world_edit_view(state, {}).active;
    const bool placing =
        state.scene.scene_state == 1 && state.build_mode == 0 && state.build_definition.has_value();
    if (definition_ != state.build_definition || edit_mode_ != state.build_mode ||
        (!placing && !editing)) {
        definition_ = state.build_definition;
        edit_mode_ = state.build_mode;
        anchor_.reset();
        orientation_ = simulation::rules::FacilityOrientation::first;
        if (editing && state.build_mode == 7 && state.build_moving_facility)
            orientation_ = state.scene.world.world.facilities.at(*state.build_moving_facility)
                               .placement.orientation;
    }
    if (blocked || pending())
        return placing || editing;
    if (state.scene.scene_state == 6) {
        if (state.scripts.selected_actor &&
            (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
             (click && hit(mouse, ui::world_tracking_confirm(extent))))) {
            const auto page = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                           [](const auto &p) { return p.lifecycle != 4; });
            if (page != state.scripts.pages.rend())
                queued(session.confirm_tracking(
                    page->id, simulation::rules::CharacterId{*state.scripts.selected_actor}));
        }
        return true; // Right-click and map dragging never invent a chase cancellation.
    }
    if (editing) {
        const auto intent = world_edit_input(
            world_edit_view(state, anchor_), extent,
            {click ? mouse : std::nullopt, IsKeyPressed(KEY_ENTER), back, IsKeyPressed(KEY_R)},
            false);
        if (!intent)
            return true;
        switch (intent->action) {
        case WorldBuildAction::cancel:
            queued(session.cancel_edit(state));
            break;
        case WorldBuildAction::choose:
            anchor_ = world_pick_cell(state, view, *intent->point, zoom);
            feedback_.clear();
            break;
        case WorldBuildAction::rotate:
            orientation_ = orientation_ == simulation::rules::FacilityOrientation::first
                               ? simulation::rules::FacilityOrientation::second
                               : simulation::rules::FacilityOrientation::first;
            break;
        case WorldBuildAction::confirm:
            if (anchor_)
                queued(session.confirm_edit(state, *anchor_, orientation_));
            break;
        }
        return true;
    }
    if (placing) {
        const WorldBuildInput input{click ? mouse : std::nullopt, IsKeyPressed(KEY_ENTER), back,
                                    IsKeyPressed(KEY_R)};
        const bool rotation_allowed =
            (input.click || input.rotate) &&
            world_build_preview(state, *definition_,
                                anchor_.value_or(simulation::rules::Position{0, 0}), orientation_)
                .rotation_hint;
        const auto intent = world_build_input(world_build_controls(extent), input,
                                              blocked || pending(), rotation_allowed);
        if (!intent)
            return true;
        switch (intent->action) {
        case WorldBuildAction::cancel:
            queued(session.cancel_build(*definition_));
            break;
        case WorldBuildAction::rotate:
            orientation_ = orientation_ == simulation::rules::FacilityOrientation::first
                               ? simulation::rules::FacilityOrientation::second
                               : simulation::rules::FacilityOrientation::first;
            break;
        case WorldBuildAction::choose:
            anchor_ = world_pick_cell(state, view, *intent->point, zoom);
            feedback_.clear();
            break;
        case WorldBuildAction::confirm:
            if (anchor_) {
                const auto preview =
                    world_build_preview(state, *definition_, *anchor_, orientation_);
                if (preview.valid())
                    queued(session.confirm_build(*definition_, *anchor_, orientation_));
                else
                    feedback_ = refusal(preview.denial);
            }
            break;
        }
        return true;
    }
    if (state.scene.scene_state != 0)
        return false;
    if (IsKeyPressed(KEY_B)) {
        queued(session.open_build_menu());
        return true;
    }
    if (IsKeyPressed(KEY_C) && (state.scripts.user_flags & 16U)) {
        queued(session.open_commerce());
        return true;
    }
    if (IsKeyPressed(KEY_K) && (state.scripts.user_flags & 1U)) {
        queued(session.open_magic_pot());
        return true;
    }
    if (IsKeyPressed(KEY_X) && state.active_task) {
        queued(session.open_task_control_menu());
        return true;
    }
    if (click && hit(mouse, ui::Layout(extent).scene)) {
        const auto target = picks.pick(*mouse);
        if (target && target->kind == SpritePickTarget::Kind::human) {
            const simulation::rules::CharacterId id{target->id};
            const auto &actors = state.scene.world.world.ai.battle.actors;
            if (actors.count(id)) {
                queued(session.open_human(id, actors.at(id).definition));
                return true;
            }
        }
        if (target && target->kind == SpritePickTarget::Kind::facility) {
            const auto &facilities = state.scene.world.world.facilities;
            if (facilities.count(target->id) && facilities.at(target->id).status != 0) {
                queued(session.open_facility(target->id));
                return true;
            }
        }
    }
    return false;
}
bool WorldManagement::pointer_on_control(const State &state, Extent extent, Vector2 mouse) const {
    const auto edit = world_edit_view(state, {});
    const bool placing =
        state.scene.scene_state == 1 && state.build_mode == 0 && state.build_definition.has_value();
    if (!edit.active && !placing)
        return false;
    const bool rotate =
        edit.active ? edit.rotate_allowed
                    : world_build_preview(state, *state.build_definition, {0, 0}, orientation_)
                          .rotation_hint;
    const auto intent = world_build_input(world_build_controls(extent),
                                          {mouse, false, false, false}, false, rotate);
    return intent && intent->action != WorldBuildAction::choose;
}
void WorldManagement::draw_footprint(const State &state, const WorldCameraView &view, Extent extent,
                                     std::optional<Vector2> mouse, float zoom,
                                     Sprites &sprites) const {
    if (state.scene.scene_state != 1)
        return;
    const auto cell = anchor_ ? anchor_
                      : hit(mouse, ui::Layout(extent).scene)
                          ? world_pick_cell(state, view, *mouse, zoom)
                          : std::nullopt;
    const auto edit = world_edit_view(state, cell);
    if (edit.active)
        draw_world_edit_preview(edit, view, zoom, sprites);
    if (cell && state.build_definition && (state.build_mode == 0 || state.build_mode == 7))
        draw_world_build_preview(
            world_build_preview(state, *state.build_definition, *cell, orientation_), view, zoom,
            sprites);
}
void WorldManagement::inspect_placement(int definition, simulation::rules::Position anchor,
                                        simulation::rules::FacilityOrientation orientation) {
    definition_ = definition;
    anchor_ = anchor;
    orientation_ = orientation;
    edit_mode_ = 0;
}
void WorldManagement::inspect_edit(const State &state, simulation::rules::Position position) {
    definition_ = state.build_definition;
    anchor_ = position;
    edit_mode_ = state.build_mode;
    if (state.build_moving_facility)
        orientation_ = state.scene.world.world.facilities.at(*state.build_moving_facility)
                           .placement.orientation;
}
void WorldManagement::draw_placement(const State &state, const WorldCameraView &view, Extent extent,
                                     std::optional<Vector2> mouse, float zoom, const ui::Skin &skin,
                                     bool enabled) const {
    if (state.scene.scene_state == 6) {
        skin.button(ui::world_tracking_confirm(extent), "人物详情", enabled && !pending());
        return;
    }
    if (state.scene.scene_state != 1)
        return;
    auto edit = world_edit_view(state, anchor_);
    if (edit.active) {
        if (state.build_feedback_counter > 0)
            edit.caption = state.build_feedback_message;
        else if ((state.build_mode == 1 || state.build_mode == 2) && state.build_definition) {
            if (const auto quote =
                    simulation::startup_world_build_quote(state, *state.build_definition))
                edit.caption += "  " + std::to_string(quote->construction_cost) + "G/格";
        } else if (state.build_mode == 6 || state.build_mode == 7) {
            edit.caption += "  300G";
        }
        // A hovering preview is independent of the separately locked confirmation cell.
        draw_world_edit_controls(edit, extent, skin, enabled && !pending(), feedback_);
        return;
    }
    if (!state.build_definition)
        return;
    const auto cell = anchor_ ? anchor_
                      : hit(mouse, ui::Layout(extent).scene)
                          ? world_pick_cell(state, view, *mouse, zoom)
                          : std::nullopt;
    bool valid{};
    const bool rotation =
        world_build_preview(state, *state.build_definition,
                            cell.value_or(simulation::rules::Position{0, 0}), orientation_)
            .rotation_hint;
    std::string caption = rotation ? "点击选择位置，R旋转，Enter建设" : "点击选择位置，Enter建设";
    if (cell) {
        const auto preview =
            world_build_preview(state, *state.build_definition, *cell, orientation_);
        valid = preview.valid();
        caption = valid ? std::to_string(preview.cost) + "G  " + caption : refusal(preview.denial);
    }
    skin.text.draw(feedback_.empty() ? caption : feedback_, 6, extent.height - 72.F,
                   feedback_.empty() && valid ? ui::ink : MAROON, 10);
    const auto controls = world_build_controls(extent);
    skin.button(controls.cancel, "返回", enabled && !pending());
    if (rotation)
        skin.button(controls.rotate, "旋转", enabled && !pending());
    skin.button(controls.confirm, "建设", enabled && !pending() && valid && anchor_.has_value());
}
} // namespace ark::desktop
