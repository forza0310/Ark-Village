// Read-only catalogue/facility projections. Selection never charges or initializes source pages.
#include "ark/simulation/actors/startup_world_profile.hpp"
// Page21/74/80/81 fields and actions follow the frozen 2b479f6 maintained prototype.
// Drawing never initializes a page, charges money or changes a facility's shared level.
#include "../../scene/world_overlay_render.hpp"
#include "../common/skin.hpp"
#include "world_building.hpp"
#include "world_facility_upgrade.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
const simulation::StartupDefinition &definition(const State &state, int id) {
    const auto &items = state.rules->facilities;
    const auto found =
        std::find_if(items.begin(), items.end(), [id](const auto &item) { return item.id == id; });
    if (found == items.end())
        throw std::invalid_argument("Building page references an unknown definition");
    return *found;
}
app::WorldFacilityTemplate detail_type(const simulation::StartupDefinition &d) {
    using Type = app::WorldFacilityTemplate;
    // The same published priority applies to definition previews without an instance query.
    return d.kind == 12                                      ? Type::home
           : d.detail == 1 || d.detail == 4 || d.detail == 5 ? Type::equipment
           : d.detail == 6                                   ? Type::recruitment
           : d.kind == 2                                     ? Type::booster
                                                             : Type::ordinary;
}
std::optional<std::size_t> open_products(const State &state,
                                         const simulation::StartupDefinition &d) {
    const int kind = d.detail == 1 ? 1 : d.detail == 4 ? 2 : d.detail == 5 ? 3 : 0;
    if (kind == 0)
        return {};
    std::size_t count{};
    for (const auto &product : state.rules->equipment)
        if (product.shop.kind == kind && state.catalog.at({kind, product.shop.id}).status != 0)
            ++count;
    return count;
}
}
const std::vector<WorldBuildingRow> &world_building_rows(const WorldBuildingView &view, int tab) {
    return view.raw == 21 ? view.catalogs.at(std::clamp(tab, 0, 2)) : view.residents;
}
bool world_building_page(const Page &page) {
    return page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
           (page.legacy_page == 21 || page.legacy_page == 74 || page.legacy_page == 80 ||
            page.legacy_page == 81);
}
WorldBuildingView world_building_view(const State &state, const Page &page) {
    if (!world_building_page(page) || !state.rules)
        throw std::invalid_argument("Building view requires a supported page and catalogue");
    WorldBuildingView view;
    view.raw = page.legacy_page;
    view.page = page.id;
    view.title = view.raw == 21 ? "建设" : view.raw == 80 ? "入住" : "设施情报";
    if (view.raw == 21) {
        const auto catalog = state.build_page_catalogs.find(page.id);
        if (catalog == state.build_page_catalogs.end())
            return view;
        for (std::size_t tab = 0; tab < 3; ++tab)
            for (const int id : catalog->second[tab]) {
                const auto quote = simulation::startup_world_build_quote(state, id);
                if (!quote)
                    throw std::invalid_argument("Building page is missing its current quote");
                view.catalogs[tab].push_back(
                    {id,
                     definition(state, id).detail == 6 ? "募集入住" : definition(state, id).name,
                     quote->construction_cost,
                     world_build_graphic(state, id,
                                         simulation::rules::FacilityOrientation::first)});
            }
        // The maintained base catalogue contains buildings only. The desktop raw21
        // projection adds the published road/edit identities without changing that cache.
        std::vector<WorldBuildingRow> roads;
        for (const auto &item : state.rules->facilities) {
            const auto presence = state.facility_presence.find(item.id);
            if (item.kind != 6 || !(item.flags & 4) || presence == state.facility_presence.end() ||
                presence->second == 0)
                continue;
            const auto quote = simulation::startup_world_build_quote(state, item.id);
            if (!quote)
                throw std::invalid_argument("Road catalogue is missing its current quote");
            roads.push_back({item.id, "道路", quote->construction_cost,
                             world_build_graphic(state, item.id,
                                                 simulation::rules::FacilityOrientation::first)});
        }
        view.catalogs[0].insert(view.catalogs[0].begin(), roads.begin(), roads.end());
        std::vector<WorldBuildingRow> tools{
            {-1, "撤除", 0, {}, {}, "destruct00.png", {0, 0, 60, 29}, {4, 3}}};
        if (state.scripts.user_flags & 32U)
            tools.push_back(
                {-2, "交换位置", 300, {}, {}, "moveTenant.png", {0, 0, 63, 32}, {1, 0}});
        // S057 places editing entries directly after roads. Preserve the base building
        // order/duplicates and all command identities; road-absent behavior stays unchanged.
        const auto tools_position =
            roads.empty() ? view.catalogs[0].end() : view.catalogs[0].begin() + roads.size();
        view.catalogs[0].insert(tools_position, tools.begin(), tools.end());
        for (auto &tab : view.catalogs)
            for (auto &row : tab)
                if (row.identity >= 0 && definition(state, row.identity).kind == 12)
                    row.residence_qualifications = state.facility_free_builds.at(row.identity);
        view.can_confirm = true;
    } else if (view.raw == 80) {
        const auto list = state.residence_page_candidates.find(page.id);
        if (list == state.residence_page_candidates.end())
            return view;
        for (const int id : list->second) {
            const auto &people = state.rules->humans;
            const auto human = std::find_if(people.begin(), people.end(),
                                            [id](const auto &item) { return item.identity == id; });
            if (human == people.end())
                throw std::invalid_argument("Residence page references an unknown human");
            view.residents.push_back(
                {id, simulation::startup_world_human_profile(state, human->identity).value().name,
                 human->residence_fee});
        }
        view.can_confirm = !view.residents.empty();
    } else if (view.raw == 74 && state.facility_definition_page_bindings.count(page.id)) {
        if (!simulation::valid_startup_world_facility_page(state, page))
            throw std::invalid_argument("Facility definition preview has invalid source payload");
        const int id = state.facility_definition_page_bindings.at(page.id);
        const auto &item = definition(state, id);
        view.mapchip = item.display_id;
        view.definition_preview = true;
        view.title = item.name;
        view.page_count = 1;
        view.phase = state.page_phases.at(page.id);
        const auto &attributes = state.scripts.facilities.at(id).attributes;
        std::copy(attributes.begin(), attributes.end(), view.attributes.begin());
        view.graphic =
            world_build_graphic(state, item.id, simulation::rules::FacilityOrientation::first);
        view.detail_type = detail_type(item);
        const auto &progress = state.scene.world.world.facility_uses.at(id);
        view.level = progress.level;
        if (view.level != 5) {
            const auto quote = simulation::startup_world_build_quote(state, id);
            if (!quote)
                throw std::invalid_argument("Definition preview is missing its shared level quote");
            view.remaining_uses = quote->upgrade_uses - progress.completed_uses;
        }
        // The source shop template displays already-open merchandise, not an invented instance.
        view.product_count = open_products(state, item);
        view.can_confirm = true;
    } else {
        const auto binding = state.facility_page_bindings.find(page.id);
        if (binding == state.facility_page_bindings.end())
            return view;
        view.facility = binding->second;
        const auto &facility = state.scene.world.world.facilities.at(*view.facility);
        const auto &item = definition(state, facility.placement.definition_id);
        view.mapchip = item.display_id;
        view.title = item.name;
        const auto phase = state.page_phases.find(page.id);
        if (phase == state.page_phases.end())
            return view;
        view.phase = phase->second;
        if (view.raw == 81) {
            if (!state.facility_upgrade_initialized.count(page.id))
                return view;
            view.title = "设施升级";
            view.upgrade = state.facility_upgrade_display;
            view.facility_name = item.name;
            simulation::SteamFacilityUpgradeSkinInput input;
            input.definition = item.id;
            input.mapchip = item.display_id;
            input.level = state.scene.world.world.facility_uses.at(item.id).level;
            input.phase = view.phase;
            input.frame = state.page_counters.at(page.id);
            const auto secondary = state.page_secondary_counters.find(page.id);
            view.upgrade_secondary_available = secondary != state.page_secondary_counters.end();
            if (view.upgrade_secondary_available)
                input.frame2 = secondary->second;
            const auto checked = [](std::int64_t n) {
                if (n < std::numeric_limits<int>::min() || n > std::numeric_limits<int>::max())
                    throw std::invalid_argument("Upgrade display outside source integer range");
                return static_cast<int>(n);
            };
            for (int slot = 0; slot < 3; ++slot) {
                for (int phase = 0; phase < 3; ++phase)
                    input.attributes[slot][phase] = checked(view.upgrade[phase][slot]);
                input.limits[slot] = checked(std::int64_t(item.economy.attributes[slot].fifth) * 2);
            }
            view.upgrade_skin = input;
            view.can_confirm = true;
        } else {
            const auto values = simulation::startup_world_facility_values(state, *view.facility);
            if (!values)
                throw std::invalid_argument("Facility page is missing its economy projection");
            view.attributes = values->instance_attributes;
            const auto detail = app::query_world_facility_detail(state, *view.facility);
            if (!detail.detail)
                throw std::invalid_argument(
                    "Facility page is missing its validated read-only detail");
            view.detail_type = detail.detail->type;
            view.level = detail.detail->level;
            view.remaining_uses = detail.detail->remaining_uses;
            view.cumulative_profit = detail.detail->cumulative_profit;
            view.graphic = world_build_graphic(state, item.id, facility.placement.orientation);
            view.product_count = open_products(state, item);
            const auto bonuses = simulation::startup_world_facility_bonus_rows(state, page.id);
            if (!bonuses)
                throw std::invalid_argument("Facility page has invalid bonus rows");
            for (const auto &source : *bonuses) {
                const auto icon =
                    simulation::startup_world_facility_icon_draw(state, source.definition);
                if (!icon)
                    throw std::invalid_argument("Facility bonus row has invalid category icon");
                view.bonus_rows.push_back({source, *icon});
            }
            view.page_count = simulation::startup_world_facility_page_count(state, page);
            view.income =
                state.facility_monthly_cash.at(*view.facility).at(state.scene.calendar.month)[0];
            view.neighbours = state.facility_page_neighbours.at(page.id).size();
            view.can_use_items = view.phase == 0 && item.kind != 2 && item.kind != 12 &&
                                 item.detail != 1 && item.detail != 4 && item.detail != 5 &&
                                 item.detail != 6;
            view.can_view_products =
                view.phase == 0 && (item.detail == 1 || item.detail == 4 || item.detail == 5);
            view.can_confirm = (item.detail == 6 && view.phase == 0) || view.can_use_items ||
                               view.can_view_products;
        }
    }
    if (view.raw == 74) {
        const int id =
            view.definition_preview
                ? state.facility_definition_page_bindings.at(page.id)
                : state.scene.world.world.facilities.at(*view.facility).placement.definition_id;
        for (int n = 0; n < 3; ++n)
            view.attribute_limits[n] =
                std::int64_t(definition(state, id).economy.attributes[n].fifth) * 2;
        view.category_icon = simulation::startup_world_facility_icon_draw(state, id);
        if (!view.category_icon)
            throw std::invalid_argument("Facility detail has invalid category icon");
        if (view.detail_type == app::WorldFacilityTemplate::ordinary) {
            const auto effects = simulation::startup_world_facility_exit_effect_draws(state, id);
            if (!effects)
                throw std::invalid_argument("Facility detail has invalid exit effects");
            view.exit_effects = *effects;
        }
    }
    view.initialized = true;
    return view;
}
} // namespace ark::desktop::ui
