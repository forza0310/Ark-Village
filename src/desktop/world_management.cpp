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
    if (!pending_)
        return;
    const auto result = std::find_if(frame.command_results.begin(), frame.command_results.end(),
                                     [&](const auto &r) { return r.serial == pending_; });
    if (result != frame.command_results.end()) {
        if (result->outcome == app::WorldCommandOutcome::rejected)
            feedback_ = refusal(result->build_denial);
        if (result->created)
            anchor_.reset();
        pending_ = 0;
    } else if (frame.last_command_serial >= pending_) {
        pending_ = 0; // Ordinary animation-page acknowledgements use the shared serial fence.
    }
}
bool WorldManagement::input_page(const State &state, const Page &page, Extent extent,
                                 std::optional<Vector2> mouse, bool click, bool blocked,
                                 app::WorldSession &session) {
    if (page_ != page.id) {
        page_ = page.id;
        building_ = {};
        award_ = {};
        rank_ = 0;
        feedback_.clear();
    }
    blocked = blocked || pending();
    const auto point = click ? mouse : std::nullopt;
    const bool enter = IsKeyPressed(KEY_ENTER), escape = IsKeyPressed(KEY_ESCAPE);
    const bool up = IsKeyPressed(KEY_UP), down = IsKeyPressed(KEY_DOWN);
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
    if (ui::world_village_activity_page(page)) {
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
                                  std::optional<Vector2> mouse, bool click, float zoom,
                                  bool blocked, app::WorldSession &session) {
    const bool placing = state.scene.scene_state == 1 && state.build_definition.has_value();
    if (definition_ != state.build_definition || !placing) {
        definition_ = state.build_definition;
        anchor_.reset();
        orientation_ = simulation::rules::FacilityOrientation::first;
    }
    if (blocked || pending())
        return placing;
    if (placing) {
        const auto intent =
            world_build_input(world_build_controls(extent),
                              {click ? mouse : std::nullopt, IsKeyPressed(KEY_ENTER),
                               IsKeyPressed(KEY_ESCAPE), IsKeyPressed(KEY_R)},
                              blocked || pending());
        if (!intent)
            return true;
        switch (intent->action) {
        case WorldBuildAction::cancel:
            queued(session.cancel_build(*definition_));
            break;
        case WorldBuildAction::rotate:
            if (!world_build_preview(state, *definition_,
                                     anchor_.value_or(simulation::rules::Position{0, 0}),
                                     orientation_)
                     .rotation_hint)
                return true;
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
    if (IsKeyPressed(KEY_X) && state.active_task) {
        queued(session.open_task_control_menu());
        return true;
    }
    if (click && hit(mouse, ui::Layout(extent).scene)) {
        if (const auto actor = world_pick_human(state, view, *mouse, zoom)) {
            queued(session.open_human(
                *actor, state.scene.world.world.ai.battle.actors.at(*actor).definition));
            return true;
        }
        if (const auto cell = world_pick_cell(state, view, *mouse, zoom)) {
            const auto &map = state.scene.world.world.map;
            const auto facility =
                map.cells.at(static_cast<std::size_t>(cell->y * map.width + cell->x)).facility;
            const auto id = facility ? std::optional{facility->instance_id.value} : std::nullopt;
            const auto &facilities = state.scene.world.world.facilities;
            if (id && facilities.count(*id) && facilities.at(*id).status != 0) {
                queued(session.open_facility(*id));
                return true;
            }
        }
    }
    return false;
}
void WorldManagement::draw_footprint(const State &state, const WorldCameraView &view, Extent extent,
                                     std::optional<Vector2> mouse, float zoom,
                                     Sprites &sprites) const {
    if (state.scene.scene_state != 1 || !state.build_definition)
        return;
    const auto cell = anchor_ ? anchor_
                      : hit(mouse, ui::Layout(extent).scene)
                          ? world_pick_cell(state, view, *mouse, zoom)
                          : std::nullopt;
    if (cell)
        draw_world_build_preview(
            world_build_preview(state, *state.build_definition, *cell, orientation_), view, zoom,
            sprites);
}
void WorldManagement::inspect_placement(int definition, simulation::rules::Position anchor,
                                        simulation::rules::FacilityOrientation orientation) {
    definition_ = definition;
    anchor_ = anchor;
    orientation_ = orientation;
}
void WorldManagement::draw_placement(const State &state, const WorldCameraView &view, Extent extent,
                                     std::optional<Vector2> mouse, float zoom, const ui::Skin &skin,
                                     bool enabled) const {
    if (state.scene.scene_state != 1 || !state.build_definition)
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
