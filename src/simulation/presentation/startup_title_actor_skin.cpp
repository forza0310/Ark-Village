#include "ark/simulation/presentation/startup_title_actor_skin.hpp"
#include <algorithm>

namespace ark::simulation {
std::optional<StartupTitleActorSkin> startup_title_actor_skin(
    int profession, int sex, int weapon_definition, int step, int facing, bool include_shadow) {
    const auto &rules = startup_world_rules();
    if (profession < 0 || profession >= static_cast<int>(rules.jobs.size()) ||
        sex < 0 || sex > 1 || weapon_definition < -1 || step < 0 || step > 3 || facing < 0 || facing > 3)
        return {};
    const int body_image = rules.jobs[static_cast<std::size_t>(profession)].sprites[static_cast<std::size_t>(sex)];
    // human/img.inf为0..31、33..37，32缺失不是透明占位。
    if (body_image < 0 || body_image > 37 || body_image == 32)
        return {};
    StartupTitleActorSkin result;
    result.body = {body_image, facing, step}; // c/b.bd[0]=0、be[0]={0,1,2,3}。
    if (include_shadow)
        result.shadow = StartupVisualDraw{StartupVisualResource::common, 25, 3, 0, 0, {}, {0, 0}, {}};
    if (weapon_definition != -1) {
        const auto found = std::find_if(rules.equipment.begin(), rules.equipment.end(),
            [=](const auto &entry) { return entry.shop.kind == 1 && entry.shop.id == weapon_definition; });
        if (found == rules.equipment.end() || found->render_style < 0 || found->render_style > 3)
            return {};
        // a/p.x四类/四向/四步：只有纵坐标随奇偶步变化，横坐标从原表逐项保留。
        constexpr std::array<std::array<std::array<int, 2>, 4>, 4> anchors{{
            {{{-3,-28},{-3,-28},{-18,-28},{-18,-28}}},
            {{{-7,-22},{-7,-22},{-15,-22},{-15,-22}}},
            {{{-5,-28},{-5,-28},{-20,-28},{-20,-28}}},
            {{{-9,-38},{-9,-37},{-22,-37},{-23,-38}}}
        }};
        auto offset = anchors[static_cast<std::size_t>(found->render_style)][static_cast<std::size_t>(facing)];
        offset[1] += step % 2;
        // action0的bf[style][0][step]全为0；不是拿body step当武器SEB帧。
        result.weapon = StartupVisualDraw{StartupVisualResource::weapon,
            found->render_style * 4 + facing, found->render_image, 0, 0, {}, offset, {}};
    }
    return result;
}
} // namespace ark::simulation
