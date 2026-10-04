#include "character_animation.hpp"
#include <array>
#include <cmath>
#include <stdexcept>

namespace ark::desktop {
namespace {
std::array<int, 2> ground_projection(world::WorldPosition p) {
    // CHARACTERS u/v and prototype startup_world_raw_projection: float multiply/divide,
    // then truncate each sum. This is before camera, screen Y inversion, zoom and DPI.
    return {static_cast<int>(p.x * 30.0F / 100.0F + p.z * 30.0F / 100.0F),
            static_cast<int>(p.x * -15.0F / 100.0F + p.z * 15.0F / 100.0F)};
}
} // namespace
const char *walking_sprite(int facing) {
    constexpr std::array<const char *, 4> names{"walk00.seb", "walk01.seb", "walk02.seb",
                                                "walk03.seb"};
    return names.at(static_cast<std::size_t>(facing));
}
void CharacterAnimation::reset() {
    previous_.reset();
    tick_ = walking_ticks_ = 0;
    frame_ = 0;
    facing_ = control_facing_ = 0;
}
void CharacterAnimation::observe(std::optional<world::WorldPosition> position,
                                 std::uint64_t admitted_tick, int control_facing, int actor_state) {
    if (!position) {
        reset();
        return;
    }
    if (control_facing < 0 || control_facing > 3)
        throw std::invalid_argument("Invalid character facing");
    if (!previous_ || admitted_tick < tick_) {
        reset();
        previous_ = position;
        tick_ = admitted_tick;
        facing_ = control_facing_ = control_facing;
        return;
    }
    if (admitted_tick == tick_)
        return;
    // The finite product model writes facing on departure/control4, but has no u/v render
    // cache. Keep the movement-derived display direction until an actual control change.
    if (control_facing != control_facing_ || actor_state == 4 || actor_state == 20)
        facing_ = control_facing;
    if (actor_state != 4 && actor_state != 20) {
        const auto old = ground_projection(*previous_), now = ground_projection(*position);
        // An unchanged integer axis must preserve j, including subpixel movement.
        if (now[0] != old[0] && now[1] != old[1])
            facing_ = now[1] > old[1] ? (now[0] > old[0] ? 0 : 3) : (now[0] > old[0] ? 1 : 2);
    }
    control_facing_ = control_facing;
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
