#pragma once

// Desktop playback of the four published walk00 frames; no AI or wall-clock dependencies.
#include "ark/world/grid.hpp"
#include <cstdint>
#include <optional>

namespace ark::desktop {
class CharacterAnimation {
  public:
    // Observe once per render. Equal admitted ticks preserve the pose while paused/in a modal;
    // an admitted tick without displacement returns to idle. Missing/reset actors clear state.
    void observe(std::optional<world::WorldPosition> position, std::uint64_t admitted_tick);
    int frame() const { return frame_; }
    void reset();

  private:
    std::optional<world::WorldPosition> previous_;
    std::uint64_t tick_{}, walking_ticks_{};
    int frame_{};
};
} // namespace ark::desktop
