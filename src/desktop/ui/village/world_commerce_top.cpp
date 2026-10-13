// Steam83 keeps its ordered private character, category icons and no-marker source navigation.
#include "world_commerce_top.hpp"
#include "../common/steam_plan_draw.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <type_traits>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
Rectangle translated(const std::array<int, 4> &r, Vector2 origin) {
    return {origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
}
Rectangle clipped(Rectangle r, Extent extent) {
    const float x = std::max(0.F, r.x), y = std::max(0.F, r.y);
    return {x, y, std::max(0.F, std::min(float(extent.width), r.x + r.width) - x),
            std::max(0.F, std::min(float(extent.height), r.y + r.height) - y)};
}
bool hit(std::optional<Vector2> point, Rectangle r) {
    return point && r.width > 0 && r.height > 0 && point->x >= r.x && point->y >= r.y &&
           point->x < r.x + r.width && point->y < r.y + r.height;
}
} // namespace
WorldCommerceTopView world_commerce_top_view(const sim::StartupWorldRuntimeState &state,
                                             const sim::rules::WorldScriptPage &page) {
    if (!world_commerce_page(page) || page.legacy_page != 83)
        throw std::invalid_argument("Commerce top requires raw83");
    WorldCommerceTopView view;
    view.page = page.id;
    view.initialized = state.commerce_pages_initialized.count(page.id) != 0;
    if (!view.initialized)
        return view;
    view.source = sim::steam_commerce_input(state, page.id);
    if (!view.source)
        throw std::invalid_argument("Initialized commerce top lost its source payload");
    view.interactive = !state.scene.framework_paused;
    return view;
}
void prepare_world_commerce_top(WorldCommerceTopView &view, const Skin &skin) {
    view.skin.reset();
    if (!view.source)
        return;
    sim::SteamCommerceSkinOptions options;
    options.view_y = 23;
    options.finger_frame = 0; // Existing explicit static PC hand; no render-frame clock.
    options.text.title = "南瓜商会";
    options.text.welcome = "欢迎光临";
    options.text.names = {"购买道具", "出售道具", "购买设施"};
    options.text.explanations = options.text.names;
    options.text.title_widths = std::array<int, 2>{int(skin.text.width(options.text.title)),
                                                   int(skin.text.width(options.text.title))};
    view.skin = sim::steam_commerce_skin(*view.source, options);
    if (!view.skin)
        throw std::invalid_argument("Commerce top cannot build its measured source plan");
}
WorldCommerceTopControls world_commerce_top_controls(const WorldCommerceTopView &view,
                                                     Extent extent) {
    if (!view.source || !view.skin || extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Commerce top controls require a measured supported source");
    WorldCommerceTopControls out;
    out.page = view.page;
    out.selection = view.source->selection;
    out.extent = extent;
    out.origin = {float((extent.width - 240) / 2), float((extent.height - 256) / 2)};
    out.cancel = {float(extent.width - 58), float(extent.height - 25), 54, 21};
    std::array<bool, 3> rows{};
    bool frame{};
    for (const auto &operation : view.skin->operations)
        if (const auto *part = std::get_if<sim::SteamStartupFrameDraw>(&operation)) {
            if (!frame)
                if (const auto *r = std::get_if<sim::StartupSkinRect>(part)) {
                    out.panel = translated(r->rect, out.origin);
                    frame = true;
                }
        } else if (const auto *touch = std::get_if<sim::SteamStartupTouch>(&operation)) {
            const int row = touch->value & 0xffff;
            if (touch->component != 11 || !touch->rectangle || row >= 3 || rows[row])
                throw std::invalid_argument("Commerce top row Touch identity is invalid");
            out.rows[row] = clipped(translated(*touch->rectangle, out.origin), extent);
            rows[row] = true;
        }
    if (!frame || !std::all_of(rows.begin(), rows.end(), [](bool present) { return present; }))
        throw std::invalid_argument("Commerce top plan lost its source frame or rows");
    return out;
}
std::optional<sim::StartupCommerceTopInput>
world_commerce_top_input(const WorldCommerceTopView &view, Extent extent,
                         const std::optional<WorldCommerceTopControls> &painted,
                         const WorldCommerceInput &input, bool blocked) {
    if (blocked || input.focus_lost || !view.initialized || !view.interactive || !view.source)
        return {};
    sim::StartupCommerceTopInput out{input.up, input.down, input.enter, input.escape, {}};
    if (input.click) {
        if (!painted || painted->page != view.page ||
            painted->selection != view.source->selection || painted->extent.width != extent.width ||
            painted->extent.height != extent.height)
            return {};
        if (hit(input.click, painted->cancel))
            out.cancel = true;
        else
            for (int row = 0; row < 3; ++row)
                if (hit(input.click, painted->rows[row])) {
                    out.selection = row;
                    out.confirm = true; // No marker: one release selects and opens only the child.
                    break;
                }
    }
    if (out.up || out.down || out.confirm || out.cancel || out.selection)
        return out;
    return {};
}
void draw_world_commerce_plan(const sim::SteamCommerceSkinPlan &plan, Vector2 origin,
                              const Skin &skin) {
    SteamPlanDraw draw(origin, skin, plan.title);
    for (const auto &operation : plan.operations)
        std::visit(
            [&](const auto &part) {
                using T = std::decay_t<decltype(part)>;
                if constexpr (std::is_same_v<T, sim::SteamStartupFrameDraw>)
                    draw.frame(part);
                else if constexpr (std::is_same_v<T, sim::SteamCommerceImage>) {
                    const auto resource = sim::steam_commerce_resource(part.asset);
                    if (!resource)
                        throw std::invalid_argument("Commerce top image has no source binding");
                    if (part.asset == sim::SteamCommerceAsset::category_icon) {
                        // Common2 has its own image table. Its SEB1 embeds the actual image2.
                        if (!resource->published_sprite || part.crop)
                            throw std::invalid_argument(
                                "Commerce category requires its common2 SEB");
                        skin.sprites.draw(
                            std::filesystem::path(resource->published_sprite).filename().string(),
                            part.frame, draw.point(part.position), WHITE,
                            Sprites::Binding::common2);
                    } else
                        draw.published(resource->published_image, resource->published_sprite,
                                       part.position, part.frame, part.crop);
                } else if constexpr (std::is_same_v<T, sim::SteamFacilityImage>) {
                    const auto resource = sim::steam_facility_resource(part.asset);
                    if (!resource)
                        throw std::invalid_argument("Commerce top label has no source binding");
                    if (!resource->published_sprite && !part.crop)
                        draw.whole_image(resource->published_image, part.position);
                    else
                        draw.facility_image(part);
                } else if constexpr (std::is_same_v<T, sim::SteamFacilityNumber>)
                    draw.number(part);
                else if constexpr (std::is_same_v<T, sim::SteamFacilityMapchip2>)
                    draw.mapchip(part);
                else if constexpr (std::is_same_v<T, sim::SteamHumanCatalogueScroll>) {
                    for (const auto &rectangle : part.rectangles)
                        draw.rectangle(rectangle);
                } else if constexpr (std::is_same_v<T, sim::SteamHumanImage>) {
                    const auto resource = sim::steam_human_resource(part.asset);
                    if (!resource)
                        throw std::invalid_argument("Commerce top NEW has no source binding");
                    if (!resource->published_sprite && !part.crop)
                        draw.whole_image(resource->published_image, part.position);
                    else
                        draw.human_image(part);
                } else if constexpr (std::is_same_v<T, sim::SteamHumanOverviewText>)
                    draw.literal(part.value, part.rectangle, part.anchor, part.font_size,
                                 part.layout, part.rgb, 200);
                else if constexpr (std::is_same_v<T, sim::SteamHumanClip>)
                    draw.clip({part.kind == sim::SteamHumanClipKind::pop
                                   ? sim::SteamFacilityClipKind::pop
                                   : sim::SteamFacilityClipKind::push_intersect,
                               part.rectangle});
                // Touch carries layout metadata only; source input is submitted once by the FIFO.
            },
            operation);
    draw.finish();
}
std::optional<WorldCommerceTopControls> draw_world_commerce_top(const WorldCommerceTopView &view,
                                                                Extent extent, const Skin &skin,
                                                                bool enabled,
                                                                const std::string &feedback) {
    if (!view.initialized)
        return {};
    const auto controls = world_commerce_top_controls(view, extent);
    draw_world_commerce_plan(*view.skin, controls.origin, skin);
    skin.button(controls.cancel, "返回", enabled && view.interactive);
    if (!feedback.empty())
        skin.text.draw(feedback, controls.panel.x, controls.panel.y + controls.panel.height + 2,
                       MAROON, 9);
    return controls;
}
} // namespace ark::desktop::ui
