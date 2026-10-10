#include "world_management.hpp"
#include "ui/skin.hpp"
#include <algorithm>

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
        building_ = {};
        facility_items_ = {};
        award_ = {};
        rank_ = 0;
        feedback_.clear();
    }
    blocked = blocked || pending();
    if (keyboard_event)
        building_.marked_definition.reset();
    const auto point = click ? mouse : std::nullopt;
    const bool enter = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
                       (ui::world_facility_items_page(page) && IsKeyPressed(KEY_SPACE));
    const bool escape = back;
    const bool up = IsKeyPressed(KEY_UP), down = IsKeyPressed(KEY_DOWN);
    if (ui::world_magic_pot_page(page)) {
        const auto view = ui::world_magic_pot_view(state, page);
        const auto layout = ui::world_commerce_layout(extent);
        const ui::WorldCommerceInput input{
            point,
            enter,
            escape,
            up,
            down,
            IsKeyPressed(KEY_LEFT),
            IsKeyPressed(KEY_RIGHT),
            false,
            hit(mouse, layout.rows) ? -static_cast<int>(GetMouseWheelMove() * 2) : 0};
        if (const auto intent = ui::world_magic_pot_input(view, layout, input, blocked))
            queued(session.act_magic_pot(page.id, intent->action, intent->selection));
        return true;
    }
    if (ui::world_facility_catalog_page(page)) {
        const auto view = ui::world_facility_catalog_view(state, page);
        const auto layout = ui::world_facility_catalog_layout(extent);
        const ui::WorldFacilityCatalogInput input{
            point,
            enter,
            escape,
            up,
            down,
            IsKeyPressed(KEY_LEFT),
            IsKeyPressed(KEY_RIGHT),
            IsKeyPressed(KEY_I),
            hit(mouse, layout.rows) ? -static_cast<int>(GetMouseWheelMove() * 2) : 0};
        if (const auto intent = ui::world_facility_catalog_input(view, layout, input, blocked))
            queued(session.act_facility_catalog(page.id, intent->action, intent->selection));
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
        const auto layout = ui::world_facility_items_layout(extent);
        const ui::WorldFacilityItemsInput input{
            point,         enter,
            escape,        up,
            down,          hit(mouse, layout.rows) ? -static_cast<int>(GetMouseWheelMove() * 2) : 0,
            keyboard_event};
        if (const auto intent =
                ui::world_facility_items_input(view, layout, facility_items_, input, blocked))
            queued(session.act_facility_item(page.id, intent->action, intent->selection));
        return true;
    }
    if (ui::world_village_activity_page(page)) {
        const auto view = ui::world_village_activity_view(state, page);
        const auto layout = ui::world_village_activity_layout(extent);
        const ui::WorldVillageActivityInput input{
            point,  up,
            down,   enter,
            escape, hit(mouse, layout.rows) ? -static_cast<int>(GetMouseWheelMove() * 2) : 0};
        if (const auto intent = ui::world_village_activity_input(view, layout, input, blocked))
            queued(session.act_village_activity(page.id, intent->action, intent->selection));
        return true;
    }
    if (ui::world_script_reward_page(page)) {
        const auto view = ui::world_script_reward_view(state, page);
        if (!blocked && view.initialized &&
            (enter || hit(point, ui::world_page_layout(page, extent).confirm)))
            queued(session.ack_page(page.id));
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
        if (const auto intent = ui::world_human_input(view, layout, input, blocked))
            queued(session.act_human(page.id, intent->action, intent->selection));
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
        const auto layout = ui::world_progression_layout(extent);
        if (const auto intent = ui::world_progression_input(
                view, layout, rank_, {point, enter, escape, up, down}, blocked))
            queued(intent->rank_action
                       ? session.act_rank(page.id, intent->selection, intent->cancel)
                       : session.ack_page(page.id));
        return true;
    }
    return false;
}
bool WorldManagement::draw_page(const State &state, const Page &page, Extent extent,
                                const ui::Skin &skin, bool enabled) const {
    enabled = enabled && !pending();
    if (ui::world_magic_pot_page(page)) {
        ui::draw_world_magic_pot(ui::world_magic_pot_view(state, page),
                                 ui::world_commerce_layout(extent), skin, enabled, feedback_);
    } else if (ui::world_facility_catalog_page(page)) {
        ui::draw_world_facility_catalog(ui::world_facility_catalog_view(state, page),
                                        ui::world_facility_catalog_layout(extent), skin, enabled,
                                        feedback_);
    } else if (ui::world_commerce_page(page)) {
        const auto view = ui::world_commerce_view(state, page);
        ui::draw_world_commerce(view, ui::world_commerce_layout(extent), skin, enabled, feedback_,
                                receipt_page_ == page.id && view.feedback_counter > 0
                                    ? commerce_amount_
                                    : std::nullopt);
    } else if (ui::world_facility_items_page(page)) {
        ui::draw_world_facility_items(ui::world_facility_items_view(state, page),
                                      ui::world_facility_items_layout(extent), skin, enabled,
                                      feedback_);
    } else if (ui::world_village_activity_page(page)) {
        ui::draw_world_village_activity(ui::world_village_activity_view(state, page),
                                        ui::world_village_activity_layout(extent), skin, enabled,
                                        feedback_);
    } else if (ui::world_script_reward_page(page)) {
        ui::draw_world_script_reward(ui::world_script_reward_view(state, page),
                                     ui::world_page_layout(page, extent), skin, enabled);
    } else if (ui::world_human_page(page)) {
        ui::draw_world_human(ui::world_human_view(state, page), ui::world_human_layout(extent),
                             skin, enabled, feedback_);
    } else if (ui::world_tax_page(page)) {
        ui::draw_world_tax(ui::world_tax_view(state, page), ui::world_tax_layout(extent), skin,
                           enabled);
    } else if (ui::world_building_page(page)) {
        ui::draw_world_building(ui::world_building_view(state, page),
                                ui::world_building_layout(extent, page.legacy_page), skin,
                                building_, enabled, feedback_);
    } else if (page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
               page.legacy_page == 87) {
        const auto view = ui::world_award_view(state, page.id);
        ui::draw_world_award(
            view, ui::world_award_layout(extent, view.termination_pending || view.pending_human),
            skin, award_, enabled);
    } else if (ui::world_progression_page(page)) {
        ui::draw_world_progression(ui::world_progression_view(state, page),
                                   ui::world_progression_layout(extent), skin, rank_, enabled);
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
