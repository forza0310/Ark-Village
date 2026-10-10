#pragma once

// Desktop-only slot selection/confirmation. The worker owns file I/O and the world freeze.
#include "ark/app/session/world_session.hpp"
#include "../ui/common/layout.hpp"

namespace ark::desktop {
namespace ui {
class Skin;
}
struct WorldSaveMenuInput {
    std::optional<Vector2> click;
    bool up{}, down{}, left{}, right{}, enter{}, escape{};
};
struct WorldSaveMenuLayout {
    Rectangle panel{}, back{}, confirm{}, cancel{}, message{}, records{};
    std::array<Rectangle, 2> slots{}, save{}, load{};
};
WorldSaveMenuLayout world_save_menu_layout(Extent extent);

class WorldSaveMenu {
  public:
    void observe(const app::WorldFrame &frame);
    bool pending() const { return pending_ != 0; }
    void input(const app::WorldFrame &frame, Extent extent, const WorldSaveMenuInput &input,
               app::WorldSession &session);
    void draw(const app::WorldFrame &frame, Extent extent, const ui::Skin &skin) const;

  private:
    enum class Confirmation { none, overwrite, load };
    int selected_{};
    bool records_open_{};
    int record_page_{};
    bool load_selected_{};
    bool opened_{};
    Confirmation confirmation_{Confirmation::none};
    std::uint64_t pending_{};
    std::string feedback_;
};
} // namespace ark::desktop
