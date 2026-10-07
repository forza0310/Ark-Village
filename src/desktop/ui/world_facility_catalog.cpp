#include "world_facility_catalog.hpp"
#include "skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using Action = simulation::StartupFacilityCatalogAction;
void fitted(const Skin &skin, const std::string &text, Rectangle box, Color color = ink) {
    skin.text.draw(text, box.x, box.y, color,
                   std::min(11.F, 12.F * box.width / std::max(1.F, skin.text.width(text))));
}
WorldFacilityCatalogRow equipment(const simulation::StartupWorldRuntimeState &state, int kind,
                                  int id) {
    const auto found = std::find_if(
        state.rules->equipment.begin(), state.rules->equipment.end(),
        [=](const auto &entry) { return entry.shop.kind == kind && entry.shop.id == id; });
    if (found == state.rules->equipment.end())
        throw std::invalid_argument("Facility catalogue lost equipment definition");
    return {id, found->shop.price, found->name, found->shop.combat,
            state.catalog.at({kind, id}).newly_unlocked};
}
} // namespace
bool world_facility_catalog_page(const simulation::rules::WorldScriptPage &page) {
    return page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
           (page.legacy_page == 79 || page.legacy_page == 72 || page.legacy_page == 82);
}
WorldFacilityCatalogView
world_facility_catalog_view(const simulation::StartupWorldRuntimeState &state,
                            const simulation::rules::WorldScriptPage &page) {
    if (!state.rules || !world_facility_catalog_page(page))
        throw std::invalid_argument("Unsupported facility catalogue view");
    WorldFacilityCatalogView out;
    out.page = page.id;
    out.raw = page.legacy_page;
    out.title = out.raw == 79 ? "商品" : out.raw == 72 ? "装备情报" : "设施口碑";
    if (!state.facility_catalog_pages_initialized.count(page.id))
        return out;
    const auto source = simulation::inspect_startup_world_facility_catalog_page(state, page.id);
    if (!source || source->raw != out.raw)
        throw std::invalid_argument("Invalid initialized facility catalogue payload");
    out.phase = source->phase;
    out.counter = source->counter;
    out.selection = source->selection;
    out.first_visible = source->first_visible;
    if (out.raw == 82) {
        const auto facility =
            std::find_if(state.rules->facilities.begin(), state.rules->facilities.end(),
                         [&](const auto &entry) { return entry.id == source->binding; });
        if (facility == state.rules->facilities.end())
            throw std::invalid_argument("Facility publicity lost definition binding");
        out.title = facility->name;
        for (int id : source->entries) {
            const auto human =
                std::find_if(state.rules->humans.begin(), state.rules->humans.end(),
                             [id](const auto &entry) { return entry.identity == id; });
            if (human == state.rules->humans.end())
                throw std::invalid_argument("Facility publicity lost shared human definition");
            out.participants.push_back(human->name);
        }
    } else {
        const int kind = out.raw == 79 ? (source->mode == 1   ? 1
                                          : source->mode == 4 ? 2
                                                              : 3)
                                       : (source->mode == 0   ? 1
                                          : source->mode == 3 ? 3
                                                              : 2);
        if (out.raw == 79) {
            for (int id : source->entries)
                out.rows.push_back(equipment(state, kind, id));
            out.choice = out.rows.at(out.selection);
        } else {
            out.choice = equipment(state, kind, source->binding);
        }
    }
    out.initialized = true;
    return out;
}
WorldFacilityCatalogLayout world_facility_catalog_layout(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Facility catalogue requires supported logical viewport");
    WorldFacilityCatalogLayout out;
    const float width = std::min(264.F, extent.width - 16.F);
    const float height = std::min(246.F, extent.height - 58.F);
    out.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto p = out.panel;
    out.rows = {p.x + 12, p.y + 45, width - 24, height - 104};
    out.row_height = out.rows.height / 4;
    out.status = {p.x + 12, p.y + height - 53, width - 24, 14};
    out.cancel = {p.x + width - 58, p.y + height - 27, 48, 20};
    out.confirm = {p.x + 10, p.y + height - 27, 48, 20};
    out.inspect = out.confirm;
    out.previous = {p.x + 10, p.y + 25, 24, 16};
    out.next = {p.x + width - 34, p.y + 25, 24, 16};
    return out;
}
std::optional<WorldFacilityCatalogIntent>
world_facility_catalog_input(const WorldFacilityCatalogView &view,
                             const WorldFacilityCatalogLayout &layout,
                             const WorldFacilityCatalogInput &input, bool blocked) {
    if (blocked || !view.initialized)
        return {};
    const auto hit = [&](Rectangle box) {
        return input.click && CheckCollisionPointRec(*input.click, box);
    };
    const auto intent = [](Action action, int selection = 0) {
        return WorldFacilityCatalogIntent{action, selection};
    };
    if (input.escape || hit(layout.cancel))
        return intent(Action::cancel);
    if (view.raw != 82) {
        if (input.left || hit(layout.previous))
            return intent(Action::previous_tab);
        if (input.right || hit(layout.next))
            return intent(Action::next_tab);
    }
    if (view.raw == 79) {
        if (input.up || input.down)
            return intent(input.up ? Action::previous : Action::next);
        if (input.wheel_rows)
            return intent(Action::select, std::clamp(view.selection + input.wheel_rows, 0,
                                                     static_cast<int>(view.rows.size()) - 1));
        for (int n = 0; n < 4 && n + view.first_visible < static_cast<int>(view.rows.size()); ++n)
            if (hit({layout.rows.x, layout.rows.y + n * layout.row_height, layout.rows.width,
                     layout.row_height}))
                return intent(Action::select, n + view.first_visible);
        if (input.inspect || hit(layout.inspect))
            return intent(Action::inspect);
    }
    if (input.enter || (view.raw != 79 && hit(layout.confirm)))
        return intent(Action::confirm);
    return {};
}
void draw_world_facility_catalog(const WorldFacilityCatalogView &view,
                                 const WorldFacilityCatalogLayout &layout, const Skin &skin,
                                 bool enabled, const std::string &feedback) {
    skin.window(layout.panel, view.raw == 79
                                  ? view.title + " " + std::to_string(view.phase + 1) + "/2"
                                  : view.title);
    skin.content(layout.rows);
    const bool active = enabled && view.initialized;
    if (view.raw != 82) {
        skin.centered("<", layout.previous, active ? blue : GRAY);
        skin.centered(">", layout.next, active ? blue : GRAY);
    }
    if (view.initialized && view.raw == 79) {
        skin.centered(view.phase == 0 ? "名称                 价格" : "名称                 能力",
                      {layout.panel.x + 36, layout.panel.y + 25, layout.panel.width - 72, 16}, ink,
                      10);
        for (int n = 0; n < 4 && n + view.first_visible < static_cast<int>(view.rows.size()); ++n) {
            const int index = n + view.first_visible;
            const auto &row = view.rows[index];
            const Rectangle box{layout.rows.x + 4, layout.rows.y + n * layout.row_height + 2,
                                layout.rows.width - 8, layout.row_height - 4};
            if (index == view.selection)
                skin.sprites.draw("finger_r.seb", 0, {box.x - 4, box.y + box.height / 2}, WHITE,
                                  Sprites::Binding::common);
            const float y = view.phase == 0 ? box.y + (box.height - 12) / 2 : box.y;
            fitted(skin, row.name, {box.x + 10, y, box.width - (view.phase == 0 ? 79 : 39), 12});
            if (row.fresh)
                skin.sprites.image("wnd_new.png", {0, 0, 20, 9},
                                   {box.x + box.width - 23, box.y + 1, 20, 9});
            const auto value = view.phase == 0 ? std::to_string(row.price) + "G"
                                               : "HP" + std::to_string(row.combat[0]) + " 攻" +
                                                     std::to_string(row.combat[1]) + " 防" +
                                                     std::to_string(row.combat[2]) + " 魔" +
                                                     std::to_string(row.combat[3]);
            skin.right(value, box.x + box.width - 3, view.phase == 0 ? y : box.y + box.height - 12,
                       blue, 10);
        }
        const Rectangle bar{layout.rows.x + layout.rows.width - 3, layout.rows.y, 3,
                            layout.rows.height};
        DrawRectangleRec(bar, {24, 34, 109, 255});
        const float total = static_cast<float>(view.rows.size());
        DrawRectangleRec({bar.x, bar.y + bar.height * view.first_visible / total, bar.width,
                          bar.height * std::min(4.F, total - view.first_visible) / total},
                         {77, 167, 236, 255});
        fitted(skin, "正在销售" + std::to_string(view.rows.size()) + "种商品", layout.status);
        skin.button(layout.inspect, "情报", active);
    } else if (view.initialized && view.raw == 72 && view.choice) {
        const auto &row = *view.choice;
        fitted(skin, row.name, {layout.rows.x + 6, layout.rows.y + 5, layout.rows.width - 12, 16});
        const char *labels[] = {"最大HP", "攻击", "防御", "魔法"};
        const float step = (layout.rows.height - 25) / 4;
        for (int n = 0; n < 4; ++n) {
            const float y = layout.rows.y + 24 + n * step;
            skin.text.draw(labels[n], layout.rows.x + 10, y, ink, 11);
            skin.right(std::to_string(row.combat[n]), layout.rows.x + layout.rows.width - 10, y,
                       blue, 11);
        }
        fitted(skin, std::to_string(row.price) + "G", layout.status, blue);
    } else if (view.initialized && view.raw == 82) {
        fitted(skin, "设施口碑 " + std::to_string(view.phase + 1) + "/2",
               {layout.rows.x + 6, layout.rows.y + 5, layout.rows.width - 12, 16});
        for (std::size_t n = 0; n < view.participants.size(); ++n)
            fitted(skin, view.participants[n],
                   {layout.rows.x + 10, layout.rows.y + 30 + static_cast<float>(n) * 22,
                    layout.rows.width - 20, 18});
        // Precise choreography is not delivered. Do not fabricate animation or earned points.
        fitted(skin, view.counter < 40 ? "确认可快进" : "确认继续", layout.status, blue);
    }
    if (view.raw != 79)
        skin.button(layout.confirm, "确定", active);
    skin.button(layout.cancel, "返回", active);
    if (!feedback.empty())
        fitted(skin, feedback, layout.status, MAROON);
}
} // namespace ark::desktop::ui
