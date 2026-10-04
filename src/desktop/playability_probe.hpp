#pragma once

// Optional bounded diagnostic using the same logical-coordinate controller as mouse input.
// It observes real normal-game updates; it never creates an actor, goal, income or logical tick.
#include "ark/app/game.hpp"
#include "ui/layout.hpp"
#include "ui/state.hpp"
#include <iosfwd>
#include <map>

namespace ark::desktop {
class PlayabilityProbe {
  public:
    void drive(app::Game &game, ui::State &view, const ui::Layout &layout);
    void report(std::ostream &output, const app::Game &game) const;
    bool passed(const app::Game &game) const;

  private:
    enum class Stage {
        arrival,
        menu,
        catalog,
        tab,
        row,
        placement,
        place,
        close,
        pause,
        resume,
        run
    };
    Stage stage_{Stage::arrival};
    int pause_frames_{};
    bool moved_{}, income_{}, occupied_{}, released_{}, pause_verified_{}, resumed_{};
    int last_completions_{};
    std::vector<facilities::InstanceId> last_occupied_;
    std::optional<world::WorldPosition> last_position_;
    std::optional<world::Cell> build_cell_;
    std::optional<facilities::InstanceId> built_;
    std::optional<app::State> frozen_;
    void observe(const app::Game &game);
};
} // namespace ark::desktop
