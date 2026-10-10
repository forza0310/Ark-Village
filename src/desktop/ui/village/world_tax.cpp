#include "world_tax.hpp"
#include "ark/simulation/actors/startup_world_profile.hpp"
#include "ark/simulation/presentation/startup_world_visuals.hpp"
#include "../common/skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
bool hit(std::optional<Vector2> p, Rectangle r) {
    return p && p->x >= r.x && p->y >= r.y && p->x < r.x + r.width && p->y < r.y + r.height;
}
} // namespace
bool world_tax_page(const simulation::rules::WorldScriptPage &page) {
    return page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
           (page.legacy_page == 90 || page.legacy_page == 98);
}
WorldTaxView world_tax_view(const simulation::StartupWorldRuntimeState &state,
                            const simulation::rules::WorldScriptPage &page) {
    if (!state.rules || !world_tax_page(page))
        throw std::invalid_argument("Tax view requires a supported page and catalogue");
    WorldTaxView view;
    view.raw = page.legacy_page;
    view.page = page.id;
    // raw98 is an automatic consumer, never a second user confirmation or a payment preview.
    if (view.raw == 98 || !state.tax_page_residents.count(page.id))
        return view;
    const auto tax = simulation::inspect_startup_world_tax_page(state, page.id);
    if (!tax)
        return view;
    view.total = tax->total;
    view.selection = tax->selection;
    view.first_visible = tax->first_visible;
    for (const auto &row : tax->rows) {
        const auto human =
            std::find_if(state.rules->humans.begin(), state.rules->humans.end(),
                         [&](const auto &h) { return h.identity == row.definition; });
        const auto portrait = simulation::startup_world_portrait(state, row.definition);
        if (human == state.rules->humans.end() || !portrait)
            throw std::invalid_argument("Tax page references a missing resident portrait");
        view.rows.push_back(
            {row.definition, row.amount, portrait->image,
             simulation::startup_world_human_profile(state, human->identity).value().name});
    }
    view.initialized = true;
    return view;
}
WorldTaxLayout world_tax_layout(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Tax page requires the supported logical viewport");
    const float width = std::min(300.F, extent.width - 16.F);
    const float height = std::min(250.F, extent.height - 40.F);
    WorldTaxLayout out;
    out.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto p = out.panel;
    out.rows = {p.x + 12, p.y + 33, width - 24, height - 93};
    out.row_height = out.rows.height / 5;
    out.total = {p.x + 12, p.y + height - 55, width - 24, 20};
    out.confirm = {p.x + width - 70, p.y + height - 28, 58, 20};
    return out;
}
std::optional<WorldTaxIntent> world_tax_input(const WorldTaxView &view,
                                              const WorldTaxLayout &layout,
                                              const WorldHumanInput &input, bool blocked) {
    using Action = simulation::StartupWorldTaxAction;
    if (blocked || !view.initialized || view.raw != 90)
        return {};
    const auto intent = [&](Action action, int selection = 0) {
        return WorldTaxIntent{action, view.page, selection};
    };
    if (input.enter || hit(input.click, layout.confirm))
        return intent(Action::confirm);
    if (view.rows.empty())
        return {};
    if (input.up || input.down)
        return intent(input.up ? Action::previous : Action::next);
    if (input.wheel_rows)
        return intent(Action::select, std::clamp(view.selection + input.wheel_rows, 0,
                                                 static_cast<int>(view.rows.size()) - 1));
    for (int row = 0; row < 5 && row + view.first_visible < static_cast<int>(view.rows.size());
         ++row)
        if (hit(input.click, {layout.rows.x, layout.rows.y + row * layout.row_height,
                              layout.rows.width, layout.row_height}))
            return intent(Action::select, row + view.first_visible);
    return {}; // Escape has no tax consumer and cannot silently acknowledge the report.
}
void draw_world_tax(const WorldTaxView &view, const WorldTaxLayout &layout, const Skin &skin,
                    bool enabled) {
    if (view.raw == 98)
        return;
    skin.window(layout.panel, "住宅税收");
    skin.content(layout.rows);
    if (view.initialized) {
        for (int row = 0; row < 5 && row + view.first_visible < static_cast<int>(view.rows.size());
             ++row) {
            const int index = row + view.first_visible;
            const auto &resident = view.rows[index];
            const float y = layout.rows.y + row * layout.row_height;
            if (index == view.selection)
                DrawRectangleRec({layout.rows.x, y, layout.rows.width, layout.row_height},
                                 {255, 236, 174, 255});
            skin.sprites.human_image(resident.portrait_image, {1, 27, 15, 14},
                                     {layout.rows.x + 3, y + 3, 15, 14});
            const float size = std::min(11.F, 12.F * (layout.rows.width - 100) /
                                                  std::max(1.F, skin.text.width(resident.name)));
            skin.text.draw(resident.name, layout.rows.x + 23, y + 4, ink, size);
            skin.right(std::to_string(resident.amount) + "G", layout.rows.x + layout.rows.width - 4,
                       y + 4, blue, 10);
        }
        skin.text.draw("合计", layout.total.x, layout.total.y + 3);
        skin.right(std::to_string(view.total) + "G", layout.total.x + layout.total.width,
                   layout.total.y + 3);
    }
    skin.button(layout.confirm, "确定", enabled && view.initialized);
}
} // namespace ark::desktop::ui
