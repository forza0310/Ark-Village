#include "world_commerce.hpp"
#include "skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
using Action = simulation::StartupCommerceAction;
bool hit(const WorldCommerceInput &input, Rectangle box) {
    return input.click && CheckCollisionPointRec(*input.click, box);
}
void fitted(const Skin &skin, const std::string &value, Rectangle box, Color color = ink) {
    skin.text.draw(value, box.x, box.y, color,
                   std::min(11.F, 12.F * box.width / std::max(1.F, skin.text.width(value))));
}
WorldCommerceRow item(const State &state, int id, bool sale) {
    const auto found = std::find_if(state.rules->items.begin(), state.rules->items.end(),
                                    [id](const auto &d) { return d.identity == id; });
    if (found == state.rules->items.end())
        throw std::invalid_argument("Commerce view lost its item definition");
    WorldCommerceRow row;
    row.identity = id;
    row.name = found->name;
    row.price = found->commerce_price / (sale ? 2 : 1);
    row.remaining = state.shop_item_stock.at(id).quantity;
    row.owned = state.items.at(id).inventory;
    row.fresh = !state.item_commerce_read.at(id);
    return row;
}
WorldCommerceRow facility(const State &state, int id) {
    const auto found = std::find_if(state.rules->facilities.begin(), state.rules->facilities.end(),
                                    [id](const auto &d) { return d.id == id; });
    if (found == state.rules->facilities.end())
        throw std::invalid_argument("Commerce view lost its facility definition");
    WorldCommerceRow row;
    row.identity = id;
    row.name = found->name;
    row.price = state.rules->facility_initial.at(id).capacity;
    row.fresh = !state.facility_commerce_read.at(id);
    row.graphic =
        world_build_graphic(state, found->id, simulation::rules::FacilityOrientation::first);
    return row;
}
} // namespace
bool world_commerce_page(const Page &page) {
    return page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
           ((page.legacy_page >= 83 && page.legacy_page <= 86) || page.legacy_page == 93);
}
WorldCommerceView world_commerce_view(const State &state, const Page &page) {
    if (!state.rules || !world_commerce_page(page))
        throw std::invalid_argument("Commerce view requires a supported source page");
    WorldCommerceView out;
    out.page = page.id;
    out.raw = page.legacy_page;
    out.title = out.raw == 85 ? "购买设施" : out.raw == 93 ? "获得设施" : "南瓜商会";
    if (!state.commerce_pages_initialized.count(page.id) && out.raw == 86) {
        const auto live = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                       [](const auto &p) { return p.lifecycle != 4; });
        if (page.legacy_f < 0 || page.legacy_f > 1 || live == state.scripts.pages.rend() ||
            live->id != page.id || live->kind != page.kind || live->legacy_page != 86 ||
            live->legacy_f != page.legacy_f || live->legacy_s != page.legacy_s)
            throw std::invalid_argument("Commerce receipt has invalid live transaction binding");
        // 84 already committed the transaction. 86's first update initializes and closes it;
        // its intervening snapshot can display the bound receipt without creating page caches.
        out.mode = page.legacy_f;
        out.choice = item(state, page.legacy_s, out.mode == 1);
        out.points = state.village_points;
        out.funds = state.scene.world.world.ai.accounting.funds();
        out.initialized = true;
        return out;
    }
    if (!state.commerce_pages_initialized.count(page.id))
        return out;
    const auto source = simulation::inspect_startup_world_commerce_page(state, page.id);
    if (!source || source->raw != out.raw)
        throw std::invalid_argument("Commerce view has invalid initialized source payload");
    out.mode = source->mode;
    out.tab = source->tab;
    out.selection = source->selection;
    out.first_visible = source->first_visible;
    out.counter = source->counter;
    out.feedback_counter = source->feedback_counter;
    out.points = state.village_points;
    out.funds = state.scene.world.world.ai.accounting.funds();
    if (out.raw == 83) {
        for (const char *name : {"购买道具", "出售道具", "购买设施"}) {
            WorldCommerceRow row;
            row.name = name;
            out.rows.push_back(std::move(row));
        }
    } else if (out.raw == 84 || out.raw == 85) {
        out.title = out.raw == 85 ? "购买设施" : out.mode == 0 ? "购买道具" : "出售道具";
        for (const int entry : source->entries)
            out.rows.push_back(out.raw == 85 ? facility(state, entry)
                                             : item(state, entry, out.mode == 1));
    } else {
        out.choice = out.raw == 93 ? facility(state, source->binding)
                                   : item(state, source->binding, out.mode == 1);
    }
    if (!out.rows.empty())
        out.choice = out.rows.at(out.selection);
    out.can_cancel = out.raw == 83 || out.raw == 84 || out.raw == 85;
    out.can_confirm = out.raw != 86;
    out.can_inspect = out.raw == 85;
    out.initialized = true;
    return out;
}
WorldCommerceLayout world_commerce_layout(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Commerce page requires supported logical viewport");
    WorldCommerceLayout out;
    const float width = std::min(320.F, extent.width - 16.F);
    const float height = std::min(270.F, extent.height - 58.F);
    out.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto p = out.panel;
    out.heading = {p.x + 10, p.y + 27, width - 20, 14};
    out.rows = {p.x + 10, p.y + 65, width - 20, height - 128};
    out.row_height = out.rows.height / 5;
    for (int n = 0; n < 2; ++n)
        out.tabs[n] = {out.rows.x + n * out.rows.width / 2, p.y + 44, out.rows.width / 2, 18};
    out.status = {p.x + 10, p.y + height - 60, width - 20, 14};
    out.feedback = {p.x + 10, p.y + height - 43, width - 20, 12};
    out.cancel = {p.x + 10, p.y + height - 26, 48, 20};
    out.inspect = {p.x + width / 2 - 24, p.y + height - 26, 48, 20};
    out.confirm = {p.x + width - 58, p.y + height - 26, 48, 20};
    return out;
}
std::optional<WorldCommerceIntent> world_commerce_input(const WorldCommerceView &view,
                                                        const WorldCommerceLayout &layout,
                                                        const WorldCommerceInput &input,
                                                        bool blocked) {
    if (blocked || !view.initialized || view.raw == 86)
        return {};
    const auto intent = [&](Action action, int selection = 0) {
        return WorldCommerceIntent{action, view.page, selection};
    };
    if (view.can_cancel && (input.escape || hit(input, layout.cancel)))
        return intent(Action::cancel);
    if (view.raw == 84 && view.mode == 0) {
        if (input.left || input.right)
            return intent(input.left ? Action::previous_tab : Action::next_tab);
        for (int n = 0; n < 2; ++n)
            if (n != view.tab && hit(input, layout.tabs[n]))
                return intent(Action::next_tab);
    }
    if (!view.rows.empty()) {
        if (input.up || input.down)
            return intent(input.up ? Action::previous : Action::next);
        if (input.wheel_rows)
            return intent(Action::select, std::clamp(view.selection + input.wheel_rows, 0,
                                                     static_cast<int>(view.rows.size()) - 1));
        const int visible = view.raw == 85 || view.raw == 83 ? 3 : 5;
        const float height = layout.rows.height / visible;
        for (int n = 0; n < visible && n + view.first_visible < static_cast<int>(view.rows.size());
             ++n)
            if (hit(input, {layout.rows.x, layout.rows.y + n * height, layout.rows.width, height}))
                return intent(Action::select, n + view.first_visible);
    }
    if (view.can_inspect && (input.inspect || hit(input, layout.inspect)))
        return intent(Action::inspect);
    if (view.can_confirm && (input.enter || hit(input, layout.confirm)))
        return intent(Action::confirm);
    return {};
}
void draw_world_commerce(const WorldCommerceView &view, const WorldCommerceLayout &layout,
                         const Skin &skin, bool enabled, const std::string &feedback,
                         std::optional<std::int64_t> last_amount) {
    skin.window(layout.panel, view.title);
    skin.content(
        {layout.rows.x - 2, layout.rows.y - 2, layout.rows.width + 4, layout.rows.height + 4});
    if (view.initialized) {
        fitted(skin,
               view.raw == 85 || view.raw == 93 ? std::to_string(view.points) + "点"
                                                : std::to_string(view.funds) + "G",
               layout.heading, blue);
        if (view.raw == 84 && view.mode == 0)
            for (int n = 0; n < 2; ++n) {
                if (view.tab == n)
                    DrawRectangleRec(layout.tabs[n], {210, 229, 195, 255});
                skin.centered(n == 0 ? "价格" : "持有", layout.tabs[n], ink, 10);
            }
        if (!view.rows.empty()) {
            const int visible = view.raw == 85 || view.raw == 83 ? 3 : 5;
            const float height = layout.rows.height / visible;
            for (int n = 0;
                 n < visible && n + view.first_visible < static_cast<int>(view.rows.size()); ++n) {
                const int index = n + view.first_visible;
                const auto &row = view.rows[index];
                Rectangle box{layout.rows.x, layout.rows.y + n * height, layout.rows.width, height};
                if (index == view.selection)
                    DrawRectangleRec(box, {255, 153, 55, 255});
                float x = box.x + 3;
                if (!row.graphic.frames.empty()) {
                    skin.sprites.thumbnail(row.graphic.sprite, row.graphic.frames,
                                           {box.x + 2, box.y + 2, 42, height - 4});
                    x += 44;
                }
                if (view.raw == 84 && row.fresh) {
                    skin.sprites.image("wnd_new.png", {0, 0, 20, 9}, {x, box.y + 3, 20, 9});
                    x += 24;
                }
                fitted(skin, row.name,
                       {x, box.y + 2, box.x + box.width - x - (view.raw == 84 ? 65 : 35), height});
                if (view.raw != 83) {
                    if (view.raw == 85 && row.fresh)
                        skin.sprites.image("wnd_new.png", {0, 0, 20, 9},
                                           {box.x + box.width - 24, box.y + 2, 20, 9});
                    const auto value = view.raw == 85  ? std::to_string(row.price) + "点"
                                       : view.tab == 1 ? std::to_string(row.owned) + "个"
                                                       : std::to_string(row.price) + "G";
                    skin.right(value, box.x + box.width - 3,
                               view.raw == 84 ? box.y + 2 : box.y + height - 12, blue, 10);
                }
            }
        } else if (view.choice) {
            if (!view.choice->graphic.frames.empty())
                skin.sprites.thumbnail(view.choice->graphic.sprite, view.choice->graphic.frames,
                                       {layout.rows.x + 5, layout.rows.y + 5,
                                        layout.rows.width - 10, layout.rows.height - 30});
            skin.centered(
                view.choice->name,
                {layout.rows.x, layout.rows.y + layout.rows.height - 22, layout.rows.width, 18});
        }
        // A sold-out item can change selection. Only a FIFO success receipt supplies that amount.
        if (view.feedback_counter > 0)
            fitted(skin,
                   "多谢惠顾!" + (last_amount ? " " + std::to_string(*last_amount) + "G" : ""),
                   layout.status);
        else if (view.raw == 86 && view.choice)
            fitted(skin,
                   (view.mode == 0 ? "购买 " : "出售 ") + std::to_string(view.choice->price) + "G",
                   layout.status);
        else if (view.raw == 84 && view.choice)
            fitted(skin,
                   (view.mode == 1 || view.tab == 1 ? "持有 " : "剩余 ") +
                       std::to_string(view.mode == 1 || view.tab == 1 ? view.choice->owned
                                                                      : view.choice->remaining),
                   layout.status);
    }
    const bool active = enabled && view.initialized;
    if (view.can_cancel)
        skin.button(layout.cancel, "返回", active);
    if (view.can_inspect)
        skin.button(layout.inspect, "情报", active);
    if (view.raw != 86)
        skin.button(layout.confirm, view.raw == 84 ? (view.mode == 0 ? "购买" : "出售") : "确定",
                    active && view.can_confirm);
    if (!feedback.empty())
        fitted(skin, feedback, layout.feedback, MAROON);
}
} // namespace ark::desktop::ui
