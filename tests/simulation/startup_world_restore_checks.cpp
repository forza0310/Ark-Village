#include "startup_world_restore_checks.hpp"

#include "../../src/simulation/persistence/startup_world_restore_validation.hpp"
#include "ark/simulation/facilities/startup_world_facility_catalog.hpp"
#include "ark/simulation/facilities/startup_world_magic_pot.hpp"

#include <algorithm>

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
    auto profile = baseline; // 合法定义覆盖夹具；不创建定义0实例、不改原表。
    profile.human_profiles.emplace(0, p::StartupWorldHumanProfile{"恢复姓名", 1, true});
    profile.scripts.humans.at(0).name = "恢复姓名";
    expect(profile, true, "profile without main character instance");
    auto profile_bad = profile;
    profile_bad.human_profiles.emplace(1, p::StartupWorldHumanProfile{"越权", 0, false});
    expect(profile_bad, false, "unsupported profile stable definition");
    profile_bad = profile;
    profile_bad.human_profiles.at(0).sex = 2;
    expect(profile_bad, false, "profile invalid sex");
    profile_bad = profile;
    profile_bad.human_profiles.at(0).name = std::string("a\0b", 3);
    expect(profile_bad, false, "profile invalid name");
    profile_bad = profile;
    profile_bad.scripts.humans.at(0).name = "旧缓存";
    expect(profile_bad, false, "profile stale script-name reference");
    // 存活主场景有合法身份，也不能接受其它页型的附属载荷。
    for (int domain = 0; domain < 3; ++domain) {
        auto damaged = baseline;
        const auto id = damaged.scripts.pages.front().id;
        if (domain == 0) damaged.facility_catalog_page_data[id] = {1, 0, 0, -1};
        if (domain == 1) damaged.facility_catalog_page_lists[id] = {0};
        if (domain == 2) damaged.facility_catalog_page_parents[id] = id;
        expect(damaged, false, "catalogue payload attached to valid wrong page kind");
    }
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
    {
        // 页面结构夹具；目录、人物、库存和随机均来自已跑到稳定主场景的真实baseline。
        auto catalogue = baseline;
        r::WorldScriptPage goods;
        goods.id = catalogue.scripts.next_page_id++;
        goods.kind = r::WorldScriptPageKind::raw_page;
        goods.legacy_page = 79;
        goods.lifecycle = 0; // 与prepare_world_script_page新插页一致，未初始化不能伪装更新态。
        goods.legacy_f = 1;
        catalogue.scripts.pages.push_back(goods);
        expect(catalogue, true, "uninitialized79 valid source");
        if (!p::initialize_startup_world_facility_catalog_pages(catalogue))
            throw std::runtime_error("restore fixture cannot initialize79");
        catalogue.scripts.pages.back().lifecycle = 2;
        expect(catalogue, true, "initialized79 complete source");
        const auto reject = [&](const p::StartupWorldRuntimeState &valid,
                                const char *scenario, const auto &damage) {
            auto broken = valid;
            damage(broken);
            expect(broken, false, scenario);
        };
        reject(catalogue, "initialized79 missing data", [&](auto &v) {
            v.facility_catalog_page_data.erase(goods.id);
        });
        reject(catalogue, "initialized79 missing list", [&](auto &v) {
            v.facility_catalog_page_lists.erase(goods.id);
        });
        reject(catalogue, "initialized79 missing counter", [&](auto &v) {
            v.page_counters.erase(goods.id);
        });
        reject(catalogue, "initialized79 missing phase", [&](auto &v) {
            v.page_phases.erase(goods.id);
        });
        if (p::act_startup_world_facility_catalog_page(
                catalogue, goods.id, p::StartupFacilityCatalogAction::inspect) !=
            p::StartupWorldRuntimeError::none ||
            !p::initialize_startup_world_facility_catalog_pages(catalogue))
            throw std::runtime_error("restore fixture cannot open real79 information72");
        const auto info = catalogue.scripts.pages.back().id;
        catalogue.scripts.pages.back().lifecycle = 2;
        expect(catalogue, true, "real79 information72 source");
        reject(catalogue, "information72 missing parent", [&](auto &v) {
            v.facility_catalog_page_parents.erase(info);
        });
        reject(catalogue, "information72 wrong parent", [&](auto &v) {
            v.facility_catalog_page_parents.at(info) = v.scripts.pages.front().id;
        });
        reject(catalogue, "information72 missing equipment binding", [&](auto &v) {
            v.facility_catalog_page_data.at(info)[3] = std::numeric_limits<int>::max();
        });
        reject(catalogue, "information72 wrong parent category", [&](auto &v) {
            v.scripts.pages[v.scripts.pages.size() - 2].legacy_f = 5;
        });
        auto retired = baseline;
        const auto stale = retired.scripts.next_page_id++;
        reject(retired, "retired catalogue initialized identity", [&](auto &v) {
            v.facility_catalog_pages_initialized.insert(stale);
        });
        reject(retired, "retired catalogue data map", [&](auto &v) {
            v.facility_catalog_page_data[stale] = {1, 0, 0, -1};
        });
        reject(retired, "retired catalogue list map", [&](auto &v) {
            v.facility_catalog_page_lists[stale] = {0};
        });
        reject(retired, "retired catalogue parent map", [&](auto &v) {
            v.facility_catalog_page_parents[stale] = goods.id;
        });
        auto praise = baseline;
        const auto d = std::find_if(praise.rules->facilities.begin(), praise.rules->facilities.end(),
                                   [](const auto &v) { return v.legacy_icon == 2; });
        if (d == praise.rules->facilities.end())
            throw std::runtime_error("restore fixture fixed catalogue has no icon2 source");
        r::WorldScriptPage animation;
        animation.id = praise.scripts.next_page_id++;
        animation.kind = r::WorldScriptPageKind::raw_page;
        animation.legacy_page = 82;
        animation.lifecycle = 0;
        animation.legacy_f = 0;
        animation.facility_definition = d->id;
        praise.scripts.pages.push_back(animation);
        expect(praise, true, "uninitialized82 defined source");
        if (!p::initialize_startup_world_facility_catalog_pages(praise))
            throw std::runtime_error("restore fixture cannot initialize82");
        praise.scripts.pages.back().lifecycle = 2;
        expect(praise, true, "initialized82 complete source");
        reject(praise, "animation82 missing facility definition", [&](auto &v) {
            v.scripts.pages.back().facility_definition.reset();
        });
        reject(praise, "animation82 invalid source definition", [&](auto &v) {
            v.scripts.pages.back().facility_definition = std::numeric_limits<int>::max();
            v.facility_catalog_page_data.at(animation.id)[3] = std::numeric_limits<int>::max();
        });
        reject(praise, "animation82 wrong icon for source mode", [&](auto &v) {
            v.scripts.pages.back().legacy_f = 1;
            v.facility_catalog_page_data.at(animation.id)[0] = 1;
        });
        reject(praise, "animation82 invalid f", [&](auto &v) {
            v.scripts.pages.back().legacy_f = 2;
            v.facility_catalog_page_data.at(animation.id)[0] = 2;
        });
        reject(praise, "animation82 missing human reference", [&](auto &v) {
            v.facility_catalog_page_lists.at(animation.id).front() =
                std::numeric_limits<int>::max();
        });
    }
    {
        // 壶资格/库存是明确条件组合；页面载荷只准备最小结构，不宣称自然新局解锁。
        auto pot = baseline;
        pot.scripts.user_flags |= 1U;
        pot.items.at(0).inventory = 2;
        pot.catalog.at({0, 0}) = pot.items.at(0);
        const auto add = [&](auto &state, int raw, int binding = -1,
                             std::optional<std::uint64_t> parent = {}) {
            r::WorldScriptPage p;
            p.id = state.scripts.next_page_id++;
            p.kind = r::WorldScriptPageKind::raw_page;
            p.legacy_page = raw;
            p.legacy_g = binding;
            state.scripts.pages.push_back(p);
            if (parent) state.magic_pot_page_parents.emplace(p.id, *parent);
            return p.id;
        };
        const auto root = add(pot, 41);
        // 41已看过首壶说明后的稳定结构：不注入脚本seen计数或重执初始化。
        pot.magic_pot_pages_initialized.insert(root);
        pot.magic_pot_page_data[root] = {0, 0, -1};
        pot.magic_pot_page_lists[root] = {};
        pot.page_phases[root] = 0;
        pot.page_counters[root] = 0;
        pot.scripts.pages.back().lifecycle = 2;
        expect(pot, true, "magic41 complete structural fixture");
        const auto reject = [&](const p::StartupWorldRuntimeState &source, const char *scenario,
                                const auto &damage) {
            auto broken = source;
            damage(broken);
            expect(broken, false, scenario);
        };
        reject(pot, "initialized magic41 missing data", [&](auto &v) { v.magic_pot_page_data.erase(root); });
        reject(pot, "initialized magic41 missing list", [&](auto &v) { v.magic_pot_page_lists.erase(root); });
        reject(pot, "initialized magic41 missing counter", [&](auto &v) { v.page_counters.erase(root); });
        reject(pot, "initialized magic41 missing phase", [&](auto &v) { v.page_phases.erase(root); });
        reject(pot, "magic payload on wrong raw page", [&](auto &v) { v.scripts.pages.back().legacy_page = 11; });
        reject(pot, "magic recipe missing definition progress", [&](auto &v) { v.magic_pot_recipes.erase(1); });
        reject(pot, "magic recipe identity mismatch", [&](auto &v) { v.magic_pot_recipes.at(1).identity = 2; });
        reject(pot, "magic recipe status2 unsupported", [&](auto &v) { v.magic_pot_recipes.at(1).status = 2; });
        reject(pot, "magic recipe reward catalog missing", [&](auto &v) { v.catalog.erase({0, 5}); });
        reject(pot, "magic level outside original range", [&](auto &v) { v.legacy_n[11] = 4; });
        reject(pot, "magic pending exceeds current capacity", [&](auto &v) { v.legacy_n[1] = 11; });
        reject(pot, "magic current element negative", [&](auto &v) { v.legacy_n[3] = -1; });
        reject(pot, "magic future processing timestamp", [&](auto &v) { v.legacy_n[12] = std::numeric_limits<int>::max(); });
        reject(pot, "magic display negative", [&](auto &v) { v.magic_pot_display[2][0] = -1; });
        reject(pot, "magic output negative", [&](auto &v) { v.magic_pot_output[0] = -1; });
        auto deposit = pot;
        const auto second = add(deposit, 42, -1, root);
        if (!p::initialize_startup_world_magic_pot_pages(deposit))
            throw std::runtime_error("restore fixture cannot initialize actual42 catalogue");
        deposit.scripts.pages.back().lifecycle = 2;
        expect(deposit, true, "magic42 inventory source and real parent41");
        reject(deposit, "magic42 missing parent", [&](auto &v) { v.magic_pot_page_parents.erase(second); });
        reject(deposit, "magic42 wrong parent", [&](auto &v) { v.magic_pot_page_parents.at(second) = v.scripts.pages.front().id; });
        const auto animation = add(deposit, 44, 0, second);
        if (!p::initialize_startup_world_magic_pot_pages(deposit))
            throw std::runtime_error("restore fixture cannot initialize44 binding");
        deposit.scripts.pages.back().lifecycle = 2;
        expect(deposit, true, "magic44 real item binding");
        reject(deposit, "magic44 invalid item binding", [&](auto &v) {
            v.scripts.pages.back().legacy_g = std::numeric_limits<int>::max();
            v.magic_pot_page_data.at(animation)[2] = std::numeric_limits<int>::max();
        });
        auto recipes = pot;
        recipes.magic_pot_recipes.at(1).status = 1; // 47只绑定已发现的配方条件夹具。
        const auto third = add(recipes, 43, -1, root);
        if (!p::initialize_startup_world_magic_pot_pages(recipes))
            throw std::runtime_error("restore fixture cannot initialize43 catalogue");
        recipes.scripts.pages.back().lifecycle = 2;
        const auto confirmation = add(recipes, 47, 1, third);
        if (!p::initialize_startup_world_magic_pot_pages(recipes))
            throw std::runtime_error("restore fixture cannot initialize47 binding");
        recipes.scripts.pages.back().lifecycle = 2;
        expect(recipes, true, "magic47 recipe binding and real parent43");
        reject(recipes, "magic47 wrong parent type", [&](auto &v) { v.magic_pot_page_parents.at(confirmation) = root; });
        reject(recipes, "magic47 invalid recipe binding", [&](auto &v) {
            v.scripts.pages.back().legacy_g = std::numeric_limits<int>::max();
            v.magic_pot_page_data.at(confirmation)[2] = std::numeric_limits<int>::max();
        });
        for (int raw : {45, 46}) {
            auto result = baseline;
            const auto id = add(result, raw, raw == 46 ? 1 : -1);
            if (!p::initialize_startup_world_magic_pot_pages(result))
                throw std::runtime_error("restore fixture cannot initialize45/46 result");
            result.scripts.pages.back().lifecycle = 2;
            expect(result, true, raw == 45 ? "magic45 queued result" : "magic46 undiscovered recipe");
            reject(result, "magic result page cannot acquire fake parent", [&](auto &v) {
                v.magic_pot_page_parents[id] = v.scripts.pages.front().id;
            });
        }
        auto retired = baseline;
        const auto stale = retired.scripts.next_page_id++;
        reject(retired, "retired magic initialized identity", [&](auto &v) { v.magic_pot_pages_initialized.insert(stale); });
        reject(retired, "retired magic data map", [&](auto &v) { v.magic_pot_page_data[stale] = {0, 0, -1}; });
        reject(retired, "retired magic list map", [&](auto &v) { v.magic_pot_page_lists[stale] = {}; });
        reject(retired, "retired magic parent map", [&](auto &v) { v.magic_pot_page_parents[stale] = root; });
    }
    return checks;
}
