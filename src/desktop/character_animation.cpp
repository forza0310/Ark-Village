#include "character_animation.hpp"
#include <cmath>

namespace ark::desktop {
void CharacterAnimation::reset() {
    previous_.reset();
    tick_ = walking_ticks_ = 0;
    frame_ = 0;
}
void CharacterAnimation::observe(std::optional<world::WorldPosition> position,
                                 std::uint64_t admitted_tick) {
    if (!position) {
        reset();
        return;
    }
    if (!previous_ || admitted_tick < tick_) {
        reset();
        previous_ = position;
        tick_ = admitted_tick;
        return;
    }
    if (admitted_tick == tick_)
        return;
    const bool moved = std::abs(position->x - previous_->x) > 0.0001F ||
                       std::abs(position->z - previous_->z) > 0.0001F;
    // Six eligible moving ticks per pose is an explicit desktop policy. STARTUP confirms the
    // 0/1/2/3 records (neutral/step/neutral/other step), but not the original playback clock.
    if (moved) {
        walking_ticks_ = (walking_ticks_ + (admitted_tick - tick_) % 24) % 24;
        frame_ = static_cast<int>(walking_ticks_ / 6);
    } else {
        walking_ticks_ = 0;
        frame_ = 0;
    }
    tick_ = admitted_tick;
    previous_ = position;
}
} // namespace ark::desktop
