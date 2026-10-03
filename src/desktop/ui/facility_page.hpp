#pragma once

// Page74 template selection is a view concern; its values are read-only application queries.
#include "layout.hpp"
#include "skin.hpp"
#include "state.hpp"
namespace ark::desktop::ui {
enum class FacilityTemplate { ordinary, equipment, recruitment, home, booster, terrain };
FacilityTemplate facility_template(const facilities::Definition &definition);
const facilities::Definition *shown_facility(const app::Game &game, const State &view);
int facility_page_count(const app::Game &game, const State &view);
void draw_facility_page(const app::Game &game, const State &view, const Layout &layout,
                        const Skin &skin);
} // namespace ark::desktop::ui
