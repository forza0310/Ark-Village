#pragma once

// Desktop selection/preview and FIFO wiring for management pages. Business state stays in
// WorldSession; this controller owns no map, ledger, actor roster or simulation clock.
#include "../scene/world_build_placement.hpp"
#include "../scene/world_editing.hpp"
#include "../ui/actors/world_human.hpp"
#include "../ui/common/world_panels.hpp"
#include "../ui/facilities/world_building.hpp"
#include "../ui/facilities/world_build_menu.hpp"
#include "../ui/facilities/world_facility_catalog.hpp"
#include "../ui/facilities/world_facility_items.hpp"
#include "../ui/information/world_information.hpp"
#include "../ui/system/world_manual.hpp"
#include "../ui/village/world_award.hpp"
#include "../ui/village/world_commerce.hpp"
#include "../ui/village/world_commerce_top.hpp"
#include "../ui/village/world_magic_pot.hpp"
#include "../ui/village/world_progression.hpp"
#include "../ui/village/world_tax.hpp"
#include "../ui/village/world_village_activity.hpp"
#include "ark/app/session/world_session.hpp"

namespace ark::desktop {
class WorldManagement {
  public:
    void observe(const app::WorldFrame &frame);
    bool pending() const { return pending_ != 0; }
    // Local keyboard reset also applies while a FIFO command is pending; it changes no Owner.
    void clear_mouse_marker() {
        build_menu_gesture_.marked_row.reset();
        build_menu_gesture_.scrolling = false;
        facility_items_.marked_row.reset();
        facility_items_.scrolling = false;
        facility_catalog_gesture_.scrolling = false;
        human_gesture_ = {};
        magic_gesture_.marked_row.reset();
        magic_gesture_.scrolling = false;
    }
    bool input_page(const app::WorldState &state, const simulation::rules::WorldScriptPage &page,
                    Extent extent, std::optional<Vector2> mouse, bool click, bool back,
                    bool keyboard_event, bool blocked, app::WorldSession &session);
    bool draw_page(const app::WorldState &state, const simulation::rules::WorldScriptPage &page,
                   Extent extent, const ui::Skin &skin, bool enabled) const;
    // Covered21 is really painted before its child, but contributes no pointer controls.
    void draw_build_background(const app::WorldState &state,
                               const simulation::rules::WorldScriptPage &page, Extent extent,
                               const ui::Skin &skin) const;
    // Submit completed drawing facts only after EndDrawing, once per source epoch/page.
    void presented(const app::WorldFrame &frame, bool blocked, app::WorldSession &session);
    // Called only for an unobstructed main scene. A click selects a cell; confirmation is a
    // separate input, preventing one physical click from both choosing and buying a building.
    bool input_scene(const app::WorldState &state, const WorldCameraView &view, Extent extent,
                     std::optional<Vector2> mouse, bool click, float zoom, bool back, bool blocked,
                     app::WorldSession &session, const SpritePickMap &picks);
    // Only visible placement controls capture a pointer press; hidden controls stay map space.
    bool pointer_on_control(const app::WorldState &state, Extent extent, Vector2 mouse) const;
    void draw_placement(const app::WorldState &state, const WorldCameraView &view, Extent extent,
                        std::optional<Vector2> mouse, float zoom, const ui::Skin &skin,
                        bool enabled) const;
    // Draw inside the scene scissor; controls/text are a separate UI pass.
    void draw_footprint(const app::WorldState &state, const WorldCameraView &view, Extent extent,
                        std::optional<Vector2> mouse, float zoom, Sprites &sprites) const;
    // Explicit window diagnostic: fixes only desktop selection, never buys or edits the Owner.
    void inspect_placement(int definition, simulation::rules::Position anchor,
                           simulation::rules::FacilityOrientation orientation);
    void inspect_edit(const app::WorldState &state, simulation::rules::Position position);

  private:
    void queued(std::uint64_t serial);
    std::uint64_t page_{}, pending_{};
    std::uint64_t receipt_page_{}, generation_{};
    std::optional<std::int64_t> commerce_amount_;
    std::string feedback_;
    ui::WorldBuildingSelection building_;
    ui::WorldBuildMenuGesture build_menu_gesture_;
    mutable std::optional<ui::WorldBuildMenuControls> build_menu_controls_;
    struct BuildPresentation {
        std::uint64_t page{}, generation{}, epoch{};
        bool same(const BuildPresentation &other) const {
            return page == other.page && generation == other.generation && epoch == other.epoch;
        }
    };
    std::uint64_t presentation_epoch_{}, build_draw_pending_{};
    std::optional<BuildPresentation> build_draw_inflight_, build_draw_ready_;
    std::vector<BuildPresentation> build_draw_submitted_; // Bounded by the current page stack/epoch.
    mutable std::vector<BuildPresentation> build_pages_drawn_;
    ui::WorldFacilityItemsSelection facility_items_;
    ui::WorldFacilityGesture facility_catalog_gesture_;
    mutable std::optional<ui::WorldFacilityPaintedControls> facility_controls_;
    ui::WorldAwardSelection award_;
    ui::WorldFacilityGesture tax_gesture_;
    mutable std::optional<ui::WorldFacilityPaintedControls> tax_controls_;
    mutable std::optional<ui::WorldManualLayout> manual_layout_; // Last painted controls only.
    // Raw74 mouse actions use only the controls painted for this page and viewport.
    struct BuildingPaint {
        std::uint64_t page{};
        int phase{}, mapchip{};
        bool preview{};
        Extent extent;
        ui::WorldBuildingLayout layout;
    };
    mutable std::optional<BuildingPaint> building_detail_paint_;
    mutable std::optional<ui::WorldCommerceTopControls> commerce_top_controls_;
    mutable std::optional<ui::WorldFacilityPaintedControls> commerce_facility_controls_;
    ui::WorldFacilityGesture commerce_facility_gesture_;
    mutable std::optional<ui::WorldHumanPaintedControls> human_controls_;
    ui::WorldHumanGesture human_gesture_;
    mutable std::optional<ui::WorldFacilityPaintedControls> village_controls_;
    ui::WorldFacilityGesture village_gesture_;
    mutable std::optional<ui::WorldFacilityPaintedControls> magic_controls_;
    ui::WorldFacilityGesture magic_gesture_;
    int edit_mode_{-1};
    std::optional<int> definition_;
    std::optional<simulation::rules::Position> anchor_;
    simulation::rules::FacilityOrientation orientation_{};
};
} // namespace ark::desktop
