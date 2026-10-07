#include "startup_world_restore_checks.hpp"

#include "startup_world_restore_validation.hpp"

#include <limits>
#include <stdexcept>

int check_startup_world_restore_contracts(
    const ark::simulation::StartupWorldRuntimeState &baseline) {
    namespace p = ark::simulation;
    namespace r = ark::simulation::rules;
    int checks{};
    const auto expect = [&](const p::StartupWorldRuntimeState &state, bool valid,
                            const char *scenario) {
        std::string reason;
        ++checks;
        if (p::persistence_detail::validate_restored_state(state, reason) != valid)
            throw std::runtime_error(std::string("restore fixture ") + scenario + ": " + reason);
    };
    expect(baseline, true, "natural baseline");
    {
        auto damaged = baseline;
        damaged.scripts.pending_completion = 1;
        expect(damaged, false, "duplicate authoritative completion");
    }
    {
        auto damaged = baseline;
        damaged.focus_actor.actor.position.x = std::numeric_limits<float>::infinity();
        expect(damaged, false, "nonfinite independent focus actor");
    }
    {
        auto damaged = baseline;
        damaged.surface.pop_back();
        expect(damaged, false, "partial surface");
    }
    // 明确夹具：期限页允许无任务，以及初始化后尚未作答的-1；不推进业务修补字段。
    auto deadline = baseline;
    r::WorldScriptPage page;
    page.id = deadline.scripts.next_page_id++;
    page.kind = r::WorldScriptPageKind::raw_page;
    page.legacy_page = 33;
    page.lifecycle = 2;
    deadline.scripts.pages.push_back(page);
    deadline.deadline_page = page.id;
    deadline.deadline_initialized.insert(page.id);
    deadline.deadline_grades[page.id] = 4;
    deadline.deadline_returns[page.id] = -1;
    deadline.page_counters[page.id] = 0;
    deadline.page_phases[page.id] = 0;
    expect(deadline, true, "deadline null task and unanswered");
    {
        auto damaged = deadline;
        damaged.deadline_grades[page.id] = 5;
        expect(damaged, false, "deadline grade out of range");
    }
    {
        auto damaged = deadline;
        damaged.deadline_returns.erase(page.id);
        expect(damaged, false, "initialized deadline missing answer");
    }
    {
        auto damaged = deadline;
        damaged.page_phases[page.id] = 2;
        expect(damaged, false, "deadline phase out of range");
    }
    // 关页后可由主场景延迟消费：初始化/答案仍保留，通用页面载荷已退休。
    deadline.deadline_returns[page.id] = 1;
    page.lifecycle = 4;
    deadline.deadline_closed_page = page;
    deadline.scripts.pages.pop_back();
    deadline.page_counters.erase(page.id);
    deadline.page_phases.erase(page.id);
    expect(deadline, true, "retired deadline awaiting main scene");
    {
        auto build = baseline;
        r::WorldScriptPage menu;
        menu.id = build.scripts.next_page_id++;
        menu.kind = r::WorldScriptPageKind::raw_page;
        menu.legacy_page = 21;
        menu.lifecycle = 2;
        build.scripts.pages.push_back(menu);
        build.build_page_catalogs.try_emplace(menu.id);
        build.page_phases[menu.id] = 0;
        build.page_counters[menu.id] = 0;
        expect(build, true, "build menu structural fixture");
        auto damaged = build;
        damaged.build_page_catalogs.erase(menu.id);
        expect(damaged, false, "build menu missing catalogue");
        damaged = build;
        damaged.page_phases[menu.id] =
            static_cast<int>(build.build_page_catalogs.at(menu.id).size());
        expect(damaged, false, "build menu tab overflow");
    }
    {
        auto residence = baseline;
        r::WorldScriptPage menu;
        menu.id = residence.scripts.next_page_id++;
        menu.kind = r::WorldScriptPageKind::raw_page;
        menu.legacy_page = 80;
        menu.lifecycle = 2;
        residence.scripts.pages.push_back(menu);
        residence.residence_page_candidates[menu.id] = {baseline.rules->humans.front().identity};
        residence.facility_page_bindings[menu.id] = baseline.scene.world.facility_order.front();
        expect(residence, true, "residence menu structural fixture");
        auto damaged = residence;
        damaged.residence_page_candidates.erase(menu.id);
        expect(damaged, false, "residence menu missing candidates");
        damaged = residence;
        damaged.facility_page_bindings.erase(menu.id);
        expect(damaged, false, "residence menu missing facility binding");
    }
    {
        // 明确源码合同夹具：world_dungeon_finish::restore_site与runtime_deadline::restore_site
        // 会移除活跃实体/探索进度/邻接，但不清以下九表；期限路径还保留sites。
        // 复制已证真实条目作为旧实例载荷，保留与活跃实例重复的可复用raw/ordinal。
        auto history = baseline;
        const auto live = history.scene.world.facility_order.front();
        const auto retired = history.next_facility_identity++;
#define RETAIN(field) history.field.emplace(retired, history.field.at(live))
        RETAIN(facility_original_ids);
        RETAIN(facility_ordinals);
        RETAIN(facility_residents);
        RETAIN(facility_difficulties);
        RETAIN(facility_flags);
        RETAIN(facility_monthly_cash);
        RETAIN(facility_month_age);
        RETAIN(facility_item_confirmations);
        RETAIN(facility_details);
#undef RETAIN
        history.sites[retired].occupied_cells = {
            history.scene.world.world.facilities.at(live).placement.anchor};
        expect(history, true, "task retirement preserves nine auxiliary histories and old site");
        const auto reject = [&](const char *scenario, const auto &damage) {
            auto broken = history;
            damage(broken);
            expect(broken, false, scenario);
        };
        reject("live facility missing original identity",
               [&](auto &s) { s.facility_original_ids.erase(live); });
        reject("live facility missing details", [&](auto &s) { s.facility_details.erase(live); });
        reject("live facility missing monthly ledger",
               [&](auto &s) { s.facility_monthly_cash.erase(live); });
        reject("retained facility identity outside allocator",
               [&](auto &s) { s.facility_original_ids.emplace(s.next_facility_identity, 0); });
        reject("retained original identity negative",
               [&](auto &s) { s.facility_original_ids.at(retired) = -1; });
        reject("retained residence definition missing", [&](auto &s) {
            s.facility_residents.at(retired) = std::numeric_limits<int>::max();
        });
        reject("retained site outside map", [&](auto &s) {
            s.sites.at(retired).occupied_cells.front().x = s.scene.world.world.map.width;
        });
        reject("retired facility cannot stay in active dungeon progress", [&](auto &s) {
            s.dungeon_facilities.emplace(retired, s.dungeon_facilities.at(live));
        });
        reject("retired facility cannot stay in active neighbourhood", [&](auto &s) {
            s.neighbourhood.emplace(retired, s.neighbourhood.at(live));
            s.neighbourhood_details.emplace(retired, s.neighbourhood_details.at(live));
        });
        reject("active neighbourhood source cannot reference retired facility", [&](auto &s) {
            s.neighbourhood_details.at(live).sources.push_back(
                {{retired}, s.scene.world.world.facilities.at(live).placement.definition_id});
        });
        if (!history.shops.empty()) {
            const auto shop = history.shops.begin()->first;
            reject("active shop missing record", [&](auto &s) { s.shops.erase(shop); });
            reject("shop order references retired facility",
                   [&](auto &s) { s.shop_order.push_back(retired); });
            reject("retired facility cannot stay in shop records", [&](auto &s) {
                s.shops.emplace(retired, s.shops.at(shop));
                s.shop_order.push_back(retired);
            });
        }
    }
    return checks;
}
