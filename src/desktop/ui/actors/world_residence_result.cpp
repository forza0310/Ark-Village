// No effects or clocks are advanced here. The source producer has already awarded the house.
#include "world_residence_result.hpp"
#include "../common/skin.hpp"
#include "../common/world_list_controls.hpp"
#include "world_human_detail.hpp"
#include <stdexcept>
namespace ark::desktop::ui {
std::optional<simulation::SteamHumanOverviewPlan>
prepare_world_residence_result(const simulation::StartupWorldRuntimeState &s,
                               const simulation::rules::WorldScriptPage &p, const Skin &skin) {
    if (p.kind != simulation::rules::WorldScriptPageKind::raw_page || p.legacy_page != 96)
        throw std::invalid_argument("Residence ceremony requires raw96");
    if (p.lifecycle != 2)
        return {};
    simulation::SteamHumanOverviewOptions o;
    o.view_y = 23;
    o.title_widths =
        std::array<int, 2>{int(skin.text.width("自宅完成")), int(skin.text.width("自宅完成"))};
    o.character_frame = simulation::steam_human_shared_character_frame(s);
    const auto result = simulation::steam_human_residence_result_skin(s, p.id, o);
    if (!result)
        throw std::invalid_argument("Invalid bound residence result plan");
    return result;
}
void draw_world_residence_result(const std::optional<simulation::SteamHumanOverviewPlan> &plan,
                                 const WorldPageLayout &l, const Skin &skin, bool enabled) {
    if (!plan) {
        skin.window(l.panel, "自宅完成");
        return;
    }
    draw_world_human_skin(*plan, world_list_skin_origin(l.panel), skin);
    skin.button(l.confirm, "确定", enabled);
}
} // namespace ark::desktop::ui
