#include "world_monster.hpp"
#include "../common/steam_plan_draw.hpp"
#include "ark/simulation/world/startup_world_runtime.hpp"
#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
// These windows consume the same ordered primitives. Their Owner qualifiers and input gates
// stay separate: a zero-clock92 Pop must never inherit the introduction's fast-forward rule.
void draw_monster_plan(const std::optional<sim::SteamMonsterSkinPlan> &plan, Extent extent,
                       const Skin &skin) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Monster window needs a supported viewport");
    if (!plan)
        return;
    SteamPlanDraw draw({float((extent.width - 240) / 2), float((extent.height - 256) / 2)}, skin,
                       plan->title);
    for (const auto &operation : plan->operations)
        std::visit(
            [&](const auto &part) {
                using T = std::decay_t<decltype(part)>;
                if constexpr (std::is_same_v<T, sim::SteamStartupFrameDraw>)
                    draw.frame(part);
                else if constexpr (std::is_same_v<T, sim::SteamHumanImage>)
                    draw.human_image(part);
                else if constexpr (std::is_same_v<T, sim::SteamHumanOverviewText>)
                    draw.text(part, part.value);
                else if constexpr (std::is_same_v<T, sim::SteamHumanClip>)
                    draw.clip({part.kind == sim::SteamHumanClipKind::push_intersect
                                   ? sim::SteamFacilityClipKind::push_intersect
                                   : sim::SteamFacilityClipKind::pop,
                               part.rectangle});
                else if constexpr (std::is_same_v<T, sim::SteamMonsterBody>)
                    draw.monster(part.sprite, part.image, part.frame, part.position);
                else if constexpr (std::is_same_v<T, sim::SteamFacilityImage>)
                    draw.facility_image(part);
                else if constexpr (std::is_same_v<T, sim::SteamFacilityNumber>)
                    draw.number(part);
                else if constexpr (std::is_same_v<T, sim::SteamInformationNumber>)
                    draw.information_number(part);
                else if constexpr (std::is_same_v<T, sim::SteamMonsterImage>) {
                    const auto resource = sim::steam_monster_resource(part.asset);
                    if (!resource)
                        throw std::invalid_argument("Monster image has no published resource");
                    draw.published(resource->published_image, resource->published_sprite,
                                   part.position, part.frame);
                } else if constexpr (std::is_same_v<T, sim::SteamCommerceImage>) {
                    const auto resource = sim::steam_commerce_resource(part.asset);
                    if (!resource)
                        throw std::invalid_argument(
                            "Monster point image has no published resource");
                    draw.published(resource->published_image, resource->published_sprite,
                                   part.position, part.frame, part.crop);
                } else {
                    static_assert(std::is_same_v<T, sim::SteamStartupTouch>,
                                  "Unhandled monster skin operation");
                    // Touch2 is metadata, with no inferred mouse area or added confirm button.
                    if (part.component != 2 || part.rectangle || part.option != 2)
                        throw std::invalid_argument(
                            "Monster window has an unsupported touch request");
                }
            },
            operation);
    draw.finish();
}
} // namespace
bool world_monster_intro_page(const simulation::rules::WorldScriptPage &page) {
    return page.kind == simulation::rules::WorldScriptPageKind::raw_page && page.legacy_page == 89;
}
bool world_monster_intro_confirm(const simulation::StartupWorldRuntimeState &state,
                                 const simulation::rules::WorldScriptPage &page, bool enter,
                                 bool blocked) {
    if (!enter || blocked || state.scene.framework_paused || !world_monster_intro_page(page) ||
        page.lifecycle != 2)
        return false;
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    return top != state.scripts.pages.rend() && top->id == page.id &&
           simulation::valid_startup_world_monster_intro_page(state, page);
}
std::optional<simulation::SteamMonsterSkinPlan>
prepare_world_monster_intro(const simulation::StartupWorldRuntimeState &state,
                            const simulation::rules::WorldScriptPage &page, const Skin &skin) {
    if (!world_monster_intro_page(page))
        throw std::invalid_argument("Monster introduction requires source89");
    if (page.lifecycle != 2)
        return {};
    simulation::SteamMonsterSkinOptions options;
    options.view_y = 23;
    // This non-Japanese title is the published fixed-APK wording, not a new Steam translation.
    options.title_widths =
        std::array<int, 2>{int(skin.text.width("新型怪物")), int(skin.text.width("新型怪物"))};
    const auto plan = simulation::steam_monster_introduction_skin(state, page.id, options);
    if (!plan)
        throw std::invalid_argument("Initialized monster introduction lost its source payload");
    return plan;
}
void draw_world_monster_intro(const std::optional<simulation::SteamMonsterSkinPlan> &plan,
                              Extent extent, const Skin &skin) {
    draw_monster_plan(plan, extent, skin);
}
bool world_monster_detail_page(const sim::rules::WorldScriptPage &page) {
    return page.kind == sim::rules::WorldScriptPageKind::raw_page && page.legacy_page == 92;
}
std::optional<WorldMonsterDetailAction>
world_monster_detail_input(const sim::StartupWorldRuntimeState &state,
                           const sim::rules::WorldScriptPage &page, bool enter, bool back,
                           bool blocked) {
    if ((!enter && !back) || blocked || state.scene.framework_paused ||
        !world_monster_detail_page(page) || page.lifecycle != 2)
        return {};
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    if (top == state.scripts.pages.rend() || top->id != page.id || top->lifecycle != 2 ||
        top->monster_definition != page.monster_definition ||
        !sim::valid_startup_world_monster_detail_page(state, *top))
        return {};
    return enter ? WorldMonsterDetailAction::confirm : WorldMonsterDetailAction::cancel;
}
std::optional<sim::SteamMonsterSkinPlan>
prepare_world_monster_detail(const sim::StartupWorldRuntimeState &state,
                             const sim::rules::WorldScriptPage &page, const Skin &skin) {
    if (!world_monster_detail_page(page))
        throw std::invalid_argument("Monster detail requires source92");
    if (page.lifecycle != 2)
        return {};
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    if (top == state.scripts.pages.rend() || top->id != page.id ||
        top->monster_definition != page.monster_definition ||
        !sim::valid_startup_world_monster_detail_page(state, *top))
        throw std::invalid_argument("Monster detail lost its bound shared definition");
    const auto definition =
        std::find_if(state.rules->monsters.begin(), state.rules->monsters.end(),
                     [&](const auto &d) { return d.identity == *top->monster_definition; });
    if (definition == state.rules->monsters.end())
        throw std::invalid_argument("Monster detail definition is missing");
    sim::SteamMonsterSkinOptions options;
    options.view_y = 23;
    options.title_widths =
        std::array<int, 2>{int(skin.text.width("怪物情报")), int(skin.text.width("怪物情报"))};
    options.name_width = int(skin.text.width(definition->name));
    // Omit statistic_labels deliberately: source92 keeps its static Japanese fallback.
    const auto plan = sim::steam_monster_detail_skin(state, page.id, options);
    if (!plan)
        throw std::invalid_argument("Initialized monster detail lost its source payload");
    return plan;
}
void draw_world_monster_detail(const std::optional<sim::SteamMonsterSkinPlan> &plan, Extent extent,
                               const Skin &skin) {
    draw_monster_plan(plan, extent, skin);
}
bool world_monster_notice_page(const sim::rules::WorldScriptPage &page) {
    return page.kind == sim::rules::WorldScriptPageKind::raw_page &&
           (page.legacy_page == 99 || page.legacy_page == 100);
}
bool world_monster_notice_confirm(const sim::StartupWorldRuntimeState &state,
                                  const sim::rules::WorldScriptPage &page, bool enter,
                                  bool blocked) {
    if (!enter || blocked || state.scene.framework_paused || !world_monster_notice_page(page) ||
        page.lifecycle != 2)
        return false;
    // Widths are qualification-only here. The source bridge checks the actual top page,
    // initialized clock, definition and outbreak payload without modifying any of them.
    sim::SteamMonsterSkinOptions options;
    options.title_widths = std::array<int, 2>{0, 0};
    return sim::steam_monster_notice_skin(state, page.id, options).has_value();
}
std::optional<sim::SteamMonsterSkinPlan>
prepare_world_monster_notice(const sim::StartupWorldRuntimeState &state,
                             const sim::rules::WorldScriptPage &page, const Skin &skin) {
    if (!world_monster_notice_page(page))
        throw std::invalid_argument("Monster notice requires source99/100");
    if (page.lifecycle != 2)
        return {};
    sim::SteamMonsterSkinOptions options;
    options.view_y = 23;
    const auto title = page.legacy_page == 99 ? "BOSS出现！" : "怪物大量出现";
    const int width = int(skin.text.width(title));
    options.title_widths = std::array<int, 2>{width, width};
    const auto plan = sim::steam_monster_notice_skin(state, page.id, options);
    if (!plan)
        throw std::invalid_argument("Initialized monster notice lost its source payload");
    return plan;
}
void draw_world_monster_notice(const std::optional<sim::SteamMonsterSkinPlan> &plan, Extent extent,
                               const Skin &skin) {
    draw_monster_plan(plan, extent, skin);
}
} // namespace ark::desktop::ui
