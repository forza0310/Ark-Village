#pragma once

// Desktop selection/preview and FIFO wiring for management pages. Business state stays in
// WorldSession; this controller owns no map, ledger, actor roster or simulation clock.
#include "ark/app/world_session.hpp"
#include "ui/world_award.hpp"
#include "ui/world_building.hpp"
#include "ui/world_commerce.hpp"
#include "ui/world_facility_items.hpp"
#include "ui/world_human.hpp"
#include "ui/world_panels.hpp"
#include "ui/world_progression.hpp"
#include "ui/world_tax.hpp"
#include "ui/world_village_activity.hpp"
#include "world_build_placement.hpp"
#include "world_editing.hpp"

namespace ark::desktop {
class WorldManagement {
  public:
    void observe(const app::WorldFrame &frame);
    bool pending() const { return pending_ != 0; }
    bool input_page(const app::WorldState &state, const simulation::rules::WorldScriptPage &page,
                    Extent extent, std::optional<Vector2> mouse, bool click, bool back,
                    bool blocked, app::WorldSession &session);
    bool draw_page(const app::WorldState &state, const simulation::rules::WorldScriptPage &page,
                   Extent extent, const ui::Skin &skin, bool enabled) const;
    // Called only for an unobstructed main scene. A click selects a cell; confirmation is a
    // separate input, preventing one physical click from both choosing and buying a building.
    bool input_scene(const app::WorldState &state, const WorldCameraView &view, Extent extent,
                     std::optional<Vector2> mouse, bool click, float zoom, bool back, bool blocked,
                     app::WorldSession &session);
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
    ui::WorldAwardSelection award_;
    int rank_{};
    int edit_mode_{-1};
    std::optional<int> definition_;
    std::optional<simulation::rules::Position> anchor_;
    simulation::rules::FacilityOrientation orientation_{};
};
} // namespace ark::desktop
