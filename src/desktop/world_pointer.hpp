#pragma once

// Window-point gestures are desktop input state, never world state. Small left clicks
// resolve on release; a captured drag cannot later select a cell or press a UI button.
#include "raylib.h"

namespace ark::desktop {
struct WorldPointerResult {
    bool click{};
    Vector2 pan{};
};
class WorldPointerGesture {
  public:
    void cancel() { *this = {}; }
    WorldPointerResult sample(Vector2 position, bool pressed, bool down, bool released,
                              bool can_pan, bool enabled) {
        if (!enabled) {
            cancel();
            return {};
        }
        if (pressed) {
            active_ = true;
            moved_ = false;
            can_pan_ = can_pan;
            origin_ = previous_ = position;
        }
        if (!active_)
            return {};
        const Vector2 from_origin{position.x - origin_.x, position.y - origin_.y};
        const bool was_moved = moved_;
        // Four window points tolerate hand jitter and do not depend on framebuffer DPI.
        moved_ = moved_ || from_origin.x * from_origin.x + from_origin.y * from_origin.y >= 16.F;
        WorldPointerResult result;
        if (moved_ && can_pan_ && (down || released))
            result.pan = was_moved ? Vector2{position.x - previous_.x, position.y - previous_.y}
                                   : from_origin;
        previous_ = position;
        if (released) {
            result.click = !moved_ && can_pan_ == can_pan;
            cancel();
        } else if (!down) {
            cancel(); // Missing release/focus interruption cannot leave a held gesture.
        }
        return result;
    }

  private:
    bool active_{}, moved_{}, can_pan_{};
    Vector2 origin_{}, previous_{};
};
} // namespace ark::desktop
