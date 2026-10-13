// Read frozen producer payloads; drawing never awards a medal, stats or a house.
#include "world_human_ceremony.hpp"
#include "../common/skin.hpp"
#include "../common/world_list_controls.hpp"
#include "world_human_detail.hpp"
#include <stdexcept>
namespace ark::desktop::ui {
std::optional<simulation::SteamHumanOverviewPlan>
prepare_world_human_ceremony(const simulation::StartupWorldRuntimeState &s,
                             const simulation::rules::WorldScriptPage &p, const Skin &skin) {
    if (p.kind != simulation::rules::WorldScriptPageKind::raw_page ||
        (p.legacy_page != 67 && p.legacy_page != 88 && p.legacy_page != 96))
        throw std::invalid_argument("Human ceremony requires raw67/88/96");
    if (p.lifecycle != 2)
        return {};
    simulation::SteamHumanOverviewOptions o;
    o.view_y = 23;
    const std::string title = p.legacy_page == 67   ? "努力奖励"
                              : p.legacy_page == 88 ? "勋章授予式"
                                                    : "自宅完成";
    const int width = int(skin.text.width(title));
    o.title_widths = std::array<int, 2>{width, width};
    o.character_frame = simulation::steam_human_shared_character_frame(s);
    o.ceremony_message_widths = {int(skin.text.width("恭喜!")), int(skin.text.width("太好了!"))};
    const auto result = p.legacy_page == 67 ? simulation::steam_human_effort_result_skin(s, p.id, o)
                        : p.legacy_page == 88
                            ? simulation::steam_human_award_result_skin(s, p.id, o)
                            : simulation::steam_human_residence_result_skin(s, p.id, o);
    if (!result)
        throw std::invalid_argument("Invalid bound human ceremony plan");
    return result;
}
void draw_world_human_ceremony(const std::optional<simulation::SteamHumanOverviewPlan> &plan,
                               const WorldPageLayout &l, const Skin &skin, bool enabled,
                               const std::string &title) {
    if (!plan) {
        skin.window(l.panel, title);
        return;
    }
    draw_world_human_skin(*plan, world_list_skin_origin(l.panel), skin);
    skin.choice(l.confirm, "确定", enabled);
}
} // namespace ark::desktop::ui
