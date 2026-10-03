// Shared page74 context for input and renderer. No GPU calls; navigation tests use it directly.
#include "facility_page.hpp"
namespace ark::desktop::ui {
FacilityTemplate facility_template(const facilities::Definition &d) {
    if (d.kind == 12)
        return FacilityTemplate::home;
    if (d.activity_detail == 1 || d.activity_detail == 4 || d.activity_detail == 5)
        return FacilityTemplate::equipment;
    if (d.activity_detail == 6)
        return FacilityTemplate::recruitment;
    if (d.kind == 2)
        return FacilityTemplate::booster;
    return d.kind == 3 ? FacilityTemplate::ordinary : FacilityTemplate::terrain;
}
const facilities::Definition *shown_facility(const app::Game &game, const State &view) {
    if (view.page == Page::definition) {
        const auto items = catalog_items(view.tab);
        return view.row >= 0 && view.row < static_cast<int>(items.size()) ? items[view.row]
                                                                          : nullptr;
    }
    if (view.page == Page::facility && view.detail)
        return &game.definition(game.state().facilities.at(*view.detail).definition_id);
    return nullptr;
}
int facility_page_count(const app::Game &game, const State &view) {
    const auto *d = shown_facility(game, view);
    return d && view.page == Page::facility && facility_template(*d) == FacilityTemplate::ordinary
               ? 2
               : 1;
}
} // namespace ark::desktop::ui
