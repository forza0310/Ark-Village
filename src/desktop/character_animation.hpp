#pragma once

// Published four-direction walking sprites; presentation cache independent of camera and FPS.
#include "ark/world/grid.hpp"
#include <cstdint>
#include <optional>

namespace ark::desktop {
// human/seb.inf entries 0..3 select walk00..03, each with its own four source poses.
const char *walking_sprite(int facing);
class CharacterAnimation {
  public:
    // Observe after admitted updates and renders. Equal ticks preserve pose while paused/modal;
    // an admitted tick without displacement returns to idle. Missing/reset actors clear state.
    // Control seeds the first pose and explicit turns. Movement uses the original integer
    // ground projection; states4/20 bypass that ordinary movement-facing override.
    void observe(std::optional<world::WorldPosition> position, std::uint64_t admitted_tick,
                 int control_facing = 0, int actor_state = 0);
    int frame() const { return frame_; }
    int facing() const { return facing_; }
    void reset();

  private:
    std::optional<world::WorldPosition> previous_;
    std::uint64_t tick_{}, walking_ticks_{};
    int frame_{};
    int facing_{}, control_facing_{};
};
} // namespace ark::desktop
