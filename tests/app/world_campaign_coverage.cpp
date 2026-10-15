#include "world_campaign_coverage.hpp"
#include "support/world_fixture.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace ark::test {
namespace {
using State = app::WorldState;
namespace ref = simulation::rules;
const char *equipment_kind(int kind) {
    return kind == 1 ? "weapon" : kind == 2 ? "armour" : "accessory";
}
std::string clean(std::string text) {
    for (auto &c : text)
        if (c == '\t' || c == '\n' || c == '\r')
            c = ' ';
    return text;
}
struct ReceiptIdentity {
    const char *category, *stage;
    int definitions;
    bool bound_human;
    bool bound_instance{};
    int recipient_limit{};
};
ReceiptIdentity receipt_identity(CampaignReceiptKind kind) {
    using Kind = CampaignReceiptKind;
    switch (kind) {
    case Kind::activity_points_paid:
        return {"activity", "points_paid", 31, false};
    case Kind::activity_effect_queued:
        return {"activity", "effect_queued", 31, false};
    case Kind::activity_satisfaction_applied:
        return {"activity", "satisfaction_applied", 31, true, false, 25};
    case Kind::activity_attribute_applied:
        return {"activity", "attribute_applied_to_human", 31, true, false, 25};
    case Kind::activity_popularity_queued:
        return {"activity", "popularity_queued", 31, false};
    case Kind::item_consumed:
        return {"item", "bound_use", 36, true};
    case Kind::item_spell_learned:
        return {"item", "permanent_spell_learned", 36, true, true, 25};
    case Kind::item_attribute_applied:
        return {"item", "attribute_applied", 36, true};
    case Kind::item_obtained:
        return {"item", "obtained", 36, false};
    case Kind::recipe_discovered:
        return {"recipe", "discovered", 40, false};
    case Kind::recipe_crafted:
        return {"recipe", "elements_paid_and_crafted", 40, false};
    case Kind::item_hp_restored:
        return {"item", "hp_restored", 36, true, true};
    case Kind::facility_blueprint_paid:
        return {"facility", "blueprint_points_paid", 85, false};
    case Kind::facility_blueprint_claimed:
        return {"facility", "blueprint_claimed", 85, false};
    case Kind::facility_construction_paid:
        return {"facility", "construction_paid", 85, false, true};
    case Kind::activity_pot_opened:
        return {"activity", "pot_opened", 31, false};
    case Kind::activity_pot_expanded:
        return {"activity", "pot_expanded", 31, false};
    case Kind::weapon_reward_equipped:
        return {"weapon", "reward_equipped", 33, true, true, 25};
    case Kind::armour_reward_equipped:
        return {"armour", "reward_equipped", 50, true, true, 25};
    case Kind::accessory_reward_equipped:
        return {"accessory", "reward_equipped", 30, true, true, 25};
    case Kind::weapon_paid_equipped:
        return {"weapon", "paid_equipped", 33, true, true, 25};
    case Kind::armour_paid_equipped:
        return {"armour", "paid_equipped", 50, true, true, 25};
    case Kind::accessory_paid_equipped:
        return {"accessory", "paid_equipped", 30, true, true, 25};
    case Kind::weapon_combat_applied:
        return {"weapon", "combat_attributes_applied", 33, true, true, 25};
    case Kind::armour_combat_applied:
        return {"armour", "combat_attributes_applied", 50, true, true, 25};
    case Kind::accessory_combat_applied:
        return {"accessory", "combat_attributes_applied", 30, true, true, 25};
    case Kind::human_medal_awarded:
        return {"human", "medal_awarded", 25, true, false, 25};
    case Kind::human_home_admitted:
        return {"human", "home_admitted", 25, true, true, 25};
    case Kind::human_home_tax_collected:
        return {"human", "home_tax_collected", 25, true, false, 25};
    case Kind::monster_retirement_cash:
        return {"monster", "retirement_cash_committed", 36, false, true};
    case Kind::monster_retirement_cash_month_reset:
        return {"monster", "retirement_cash_month_reset", 36, false, true};
    case Kind::rank_conditions_promoted:
        return {"rank", "conditions_promoted", 6, false};
    case Kind::task_victory_committed:
        return {"task", "victory_committed", 81, false, true};
    case Kind::item_commerce_bought:
        return {"item", "commerce_bought", 36, false};
    case Kind::item_facility_consumed:
        return {"item", "facility_bound_use", 36, false, true};
    case Kind::facility_item_improved:
        return {"facility", "item_improvement_applied", 85, false, true};
    case Kind::item_task_loot_claimed:
        return {"item", "task_loot_claimed", 36, false, true};
    case Kind::weapon_task_loot_claimed:
        return {"weapon", "task_loot_claimed", 33, false, true};
    case Kind::armour_task_loot_claimed:
        return {"armour", "task_loot_claimed", 50, false, true};
    case Kind::accessory_task_loot_claimed:
        return {"accessory", "task_loot_claimed", 30, false, true};
    case Kind::profession_points_paid:
        return {"profession", "points_paid", 23, true, true, 25};
    case Kind::profession_set:
        return {"profession", "set", 23, true, true, 25};
    case Kind::profession_stats_applied:
        return {"profession", "stats_applied", 23, true, true, 25};
    case Kind::spell_available:
        return {"spell", "available", 4, true, true, 25};
    }
    throw std::runtime_error("Unknown campaign receipt kind");
}
} // namespace

CampaignReceipt facility_transaction_receipt(CampaignReceiptKind kind, int definition,
                                             const State &before, const State &after,
                                             std::uint64_t page, std::uint64_t instance) {
    return {kind,
            definition,
            after.simulation_steps,
            page,
            instance,
            -1,
            {before.village_points, before.facility_presence.at(definition),
             before.facility_free_builds.at(definition),
             before.scene.world.world.ai.accounting.funds()},
            {after.village_points, after.facility_presence.at(definition),
             after.facility_free_builds.at(definition),
             after.scene.world.world.ai.accounting.funds()}};
}

CampaignReceipt residence_transaction_receipt(int human, const State &before, const State &after,
                                              std::uint64_t page, std::uint64_t facility) {
    const auto fee = before.rules->humans.at(human).residence_fee;
    return {CampaignReceiptKind::human_home_admitted,
            human,
            after.simulation_steps,
            page,
            facility,
            human,
            {before.scene.world.world.ai.accounting.funds(), before.human_homes.at(human)[2],
             before.shop_humans.at(human).satisfaction, fee},
            {after.scene.world.world.ai.accounting.funds(), after.human_homes.at(human)[2],
             after.shop_humans.at(human).satisfaction, fee}};
}

void CampaignCoverage::mark(const char *kind, int id, const char *event) {
    observations_[{kind, id}].insert(event);
}
void CampaignCoverage::record(const CampaignReceipt &receipt, std::ostream &log) {
    const auto identity = receipt_identity(receipt.kind);
    // The fourth value links the item-use row to a facility definition, and the
    // improvement row to an item. Both namespaces remain independently validated.
    const bool task_loot = receipt.kind == CampaignReceiptKind::item_task_loot_claimed ||
                           receipt.kind == CampaignReceiptKind::weapon_task_loot_claimed ||
                           receipt.kind == CampaignReceiptKind::armour_task_loot_claimed ||
                           receipt.kind == CampaignReceiptKind::accessory_task_loot_claimed;
    const int related_limit = task_loot                                                     ? 81
                              : receipt.kind == CampaignReceiptKind::item_facility_consumed ? 85
                              : receipt.kind == CampaignReceiptKind::facility_item_improved ? 36
                                                                                            : 0;
    if (receipt.definition < 0 || receipt.definition >= identity.definitions || !receipt.page ||
        (identity.bound_human &&
         (receipt.recipient < 0 ||
          (identity.recipient_limit && receipt.recipient >= identity.recipient_limit))) ||
        (identity.bound_instance && !receipt.subject) ||
        ((receipt.kind == CampaignReceiptKind::human_medal_awarded ||
          receipt.kind == CampaignReceiptKind::human_home_admitted ||
          receipt.kind == CampaignReceiptKind::human_home_tax_collected) &&
         receipt.definition != receipt.recipient))
        throw std::runtime_error("Campaign receipt lacks its source definition/page/human binding");
    if (related_limit &&
        (receipt.recipient != -1 || receipt.before[3] < 0 || receipt.before[3] >= related_limit ||
         receipt.after[3] != receipt.before[3]))
        throw std::runtime_error("Campaign receipt lost its related source definition");
    if (task_loot && (receipt.before[2] <= 0 || receipt.before[2] != receipt.after[2]))
        throw std::runtime_error("Task loot receipt lost its actual challenge grant count");
    if (receipt.kind == CampaignReceiptKind::human_home_tax_collected &&
        (receipt.before[3] < 0 || receipt.before[3] >= 12 ||
         receipt.before[3] != receipt.after[3] || receipt.after[1] != 0))
        throw std::runtime_error("Residence tax receipt lost its month/reset binding");
    if (receipt.kind == CampaignReceiptKind::monster_retirement_cash ||
        receipt.kind == CampaignReceiptKind::monster_retirement_cash_month_reset) {
        const auto &old = receipt.before, &now = receipt.after;
        if (receipt.recipient != -1 || old[0] < 0 || old[1] < 0 || old[2] != 0 || old[3] <= 0 ||
            old[0] > std::numeric_limits<int>::max() || old[1] > std::numeric_limits<int>::max() ||
            old[3] > std::numeric_limits<int>::max() || now[0] > std::numeric_limits<int>::max() ||
            now[1] > std::numeric_limits<int>::max() || now[2] < 0 || now[3] != old[3] ||
            now[0] != (receipt.kind == CampaignReceiptKind::monster_retirement_cash_month_reset
                           ? 0
                           : old[0] + old[3]) ||
            now[1] != old[1] + old[3])
            throw std::runtime_error("Monster retirement receipt lost its counters/cash/batch");
    }
    if (receipt.kind == CampaignReceiptKind::rank_conditions_promoted) {
        const auto &old = receipt.before, &now = receipt.after;
        if (receipt.definition < 1 || receipt.definition > 5 || receipt.recipient != -1 ||
            receipt.subject != 0 || old[0] != receipt.definition - 1 ||
            now[0] != receipt.definition || old[1] != 15 || now[1] != 15 || old[2] < 0 ||
            now[2] < 0 || old[3] < 0 || now[3] < 0)
            throw std::runtime_error("Rank receipt lost its transition or four-condition mask");
    }
    if (receipt.kind == CampaignReceiptKind::activity_satisfaction_applied ||
        receipt.kind == CampaignReceiptKind::activity_attribute_applied) {
        const auto &old = receipt.before, &now = receipt.after;
        const bool satisfaction =
            receipt.kind == CampaignReceiptKind::activity_satisfaction_applied;
        if (receipt.subject != 0 || old[2] != now[2] || old[3] != now[3] || old[3] < 0 ||
            old[3] >= 23 ||
            (satisfaction ? old[2] != -1 || old[0] != old[1] || now[0] != now[1] || old[0] < 0 ||
                                old[0] > 100 || now[0] < 0 || now[0] > 100
                          : old[2] < 0 || old[2] > 5))
            throw std::runtime_error(
                "Activity receipt lost its human/attribute/profession binding");
    }
    if (receipt.kind == CampaignReceiptKind::profession_points_paid) {
        const auto &old = receipt.before, &now = receipt.after;
        if (old[0] < 0 || old[0] > std::numeric_limits<int>::max() || now[0] < 0 || now[0] > 999 ||
            now[0] > old[0] || old[1] < 0 ||
            old[1] >= std::numeric_limits<int>::max() || now[1] != old[1] + 1 || old[2] < 0 ||
            old[2] >= 23 || old[2] == receipt.definition || now[2] != old[2] || old[3] < 0 ||
            old[3] > std::numeric_limits<int>::max() || now[3] != old[3])
            throw std::runtime_error("Profession payment lost its points/history/current job/medals");
    }
    if (receipt.kind == CampaignReceiptKind::profession_set) {
        const auto &old = receipt.before, &now = receipt.after;
        if (old[0] < 0 || old[0] >= 23 || old[0] == receipt.definition || old[1] != old[0] ||
            now[0] != receipt.definition || now[1] != receipt.definition || old[2] != 54 ||
            now[2] != 55 || old[3] <= 0 || old[3] > std::numeric_limits<int>::max() ||
            now[3] != old[3])
            throw std::runtime_error("Profession setter lost its actual55 transition or actor binding");
    }
    if (receipt.kind == CampaignReceiptKind::profession_stats_applied) {
        const auto valid_combat = [](std::int64_t value) {
            return value >= std::numeric_limits<int>::min() && value <= 9999;
        };
        if (!std::all_of(receipt.before.begin(), receipt.before.end(), valid_combat) ||
            !std::all_of(receipt.after.begin(), receipt.after.end(), valid_combat))
            throw std::runtime_error("Profession stats receipt lost its four actual combat values");
    }
    if (receipt.kind == CampaignReceiptKind::spell_available) {
        const auto &old = receipt.before, &now = receipt.after;
        constexpr int spell_professions[]{5, 12, 18, 6};
        if (old[0] != 0 || now[0] != 1 || old[1] < 0 || old[1] > 1 || now[1] != old[1] ||
            old[2] < 0 || old[2] >= 23 || now[2] != old[2] || old[3] < 1 || old[3] > 10 ||
            now[3] != old[3] ||
            (now[1] != 1 && now[2] != spell_professions[receipt.definition] && now[3] != 10))
            throw std::runtime_error("Spell availability lost its unchanged learned bit/job/level");
    }
    // Numeric evidence remains in the existing phase log. Only a fixed stage set is retained.
    log << "COVERAGE_RECEIPT\t" << identity.category << '\t' << receipt.definition << '\t'
        << identity.stage << '\t' << receipt.step << '\t' << receipt.page << '\t'
        << receipt.recipient << '\t' << receipt.subject;
    for (const auto value : receipt.before)
        log << '\t' << value;
    for (const auto value : receipt.after)
        log << '\t' << value;
    log << '\n';
    receipts_[{identity.category, receipt.definition}].insert(identity.stage);
}
void CampaignCoverage::observe(const State &before, const State &after) {
    if (segment_.empty()) {
        segment_ = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        first_step_ = before.simulation_steps;
    }
    last_step_ = after.simulation_steps;
    for (const auto &[id, value] : after.human_definition_state) {
        const auto old = before.human_definition_state.find(id);
        if (old != before.human_definition_state.end() && value != old->second)
            mark("human", id, "definition_state_changed");
    }
    for (const auto &[id, value] : after.human_presence) {
        const auto old = before.human_presence.find(id);
        if (old != before.human_presence.end() && value != old->second)
            mark("human", id, "presence_changed"); // Not an arrival/admission proof by itself.
    }
    for (const auto &[id, facility] : after.scene.world.world.facilities) {
        const auto old = before.scene.world.world.facilities.find(id);
        const int definition = facility.placement.definition_id;
        if (old == before.scene.world.world.facilities.end())
            mark("facility", definition,
                 "instance_created"); // Can also be a task site/move; not construction proof.
        else if (facility.sales > old->second.sales)
            mark("facility", definition, "revenue_received");
    }
    for (const auto &[id, value] : after.scripts.facilities) {
        const auto old = before.scripts.facilities.find(id);
        if (old == before.scripts.facilities.end())
            continue;
        if (value.level > old->second.level)
            mark("facility", id, "level_increased");
        if (value.improvements != old->second.improvements)
            mark("facility", id, "shared_improvements_changed");
    }
    for (const auto &[id, value] : after.items) {
        const auto old = before.items.find(id);
        if (old == before.items.end())
            continue;
        if (value.status != old->second.status)
            mark("item", id, "status_changed");
        if (value.inventory > old->second.inventory)
            mark("item", id, "inventory_increased");
        if (value.inventory < old->second.inventory)
            mark("item", id, "inventory_decreased");
    }
    for (const auto &[id, value] : after.catalog) {
        const auto old = before.catalog.find(id);
        if (old != before.catalog.end() && value.status != old->second.status)
            mark(equipment_kind(id.first), id.second, "status_changed");
    }
    for (const auto &[id, value] : after.shop_humans) {
        const auto old = before.shop_humans.find(id);
        if (old == before.shop_humans.end())
            continue; // Initial loadout is not a new purchase.
        for (int slot = 0; slot < 4; ++slot)
            if (value.equipment[slot] && value.equipment[slot] != old->second.equipment[slot])
                mark(equipment_kind(slot == 0   ? 1
                                    : slot == 3 ? 3
                                                : 2),
                     *value.equipment[slot], "equipped");
    }
    for (const auto &[id, value] : after.activity_counts) {
        const auto old = before.activity_counts.find(id);
        if (value > (old == before.activity_counts.end() ? 0 : old->second))
            mark("activity", id, "started"); // Effects happen later, not at the payment.
    }
    for (const auto &[id, value] : after.task_progress.definitions) {
        const auto old = before.task_progress.definitions.find(id);
        if (old != before.task_progress.definitions.end() &&
            value.completed > old->second.completed)
            mark("task", id, "completion_increased");
    }
    for (const auto &[id, value] : after.magic_pot_recipes) {
        const auto old = before.magic_pot_recipes.find(id);
        if (old != before.magic_pot_recipes.end() && value.status != old->second.status)
            mark("recipe", id, "status_changed"); // Discovery is not a paid craft.
    }
    if (after.rank > before.rank)
        mark("rank", after.rank, "promoted");
}

void CampaignCoverage::write(std::ostream &out, const State &state) const {
    if (!state.rules)
        throw std::runtime_error("Coverage requires the actual source catalogue");
    out << "# Segment observations only; no full-campaign certificate. Loaded history is not "
           "inferred.\n"
        << "# All source IDs retained, including map/task-only facilities; scope and effects still "
           "require contracts.\n"
        << "# steps=" << first_step_ << ".." << last_step_ << " rank=" << state.rank
        << " maximum_income=" << state.maximum_income << '\n'
        << "category\tid\tname\tsource_kind\trank_hint\tobserved_in_segment\tverified_"
           "receipts\trequired_evidence\n";
    const auto row = [&](const char *kind, int id, const std::string &name, int source_kind,
                         int rank, const char *required) {
        out << kind << '\t' << id << '\t' << clean(name) << '\t' << source_kind << '\t' << rank
            << '\t';
        const auto evidence = observations_.find({kind, id});
        if (evidence == observations_.end())
            out << "UNOBSERVED";
        else {
            for (const auto &event : evidence->second)
                out << event << ';';
        }
        out << '\t';
        const auto receipts = receipts_.find({kind, id});
        if (receipts == receipts_.end())
            out << "UNVERIFIED";
        else
            for (const auto &stage : receipts->second)
                out << stage << ';';
        out << '\t' << required << '\n';
    };
    for (const auto &d : state.rules->facilities)
        row("facility", d.id, d.name, d.kind, d.unlock_rank,
            "legal_unlock/"
            "payment;build_complete;business_revenue_or_kind_effect;upgrade_and_adjacency_where_"
            "applicable");
    for (const auto &d : state.rules->items)
        row("item", d.identity, d.name, d.category, -1,
            "obtain;bound_use;stock_and_cost;actual_human_or_facility_effect;deposit_or_sale_not_"
            "use");
    for (const auto &d : state.rules->humans)
        row("human", d.identity, d.name, d.definition.current_profession, -1,
            "legal_unlock;actual_arrival;admission_where_applicable;growth;mastery_and_successor;"
            "restore");
    for (const auto &d : state.rules->equipment)
        row(equipment_kind(d.shop.kind), d.shop.id, d.name, d.shop.type, -1,
            "unlock;paid_or_valid_free_acquisition;equip;stats_and_combat_use");
    for (const auto &d : state.rules->activities)
        row("activity", d.identity, d.name, d.parameters[0], -1,
            "unlock;points_paid_once;complete;bound_effect;subsequent_trading");
    for (const auto &d : state.rules->tasks)
        row("task", d.factory.identity, d.name, d.factory.kind, -1,
            "naturally_generated;recruit_and_depart;identity_bound_victory;reward_once;successor");
    for (const auto &d : state.rules->monsters)
        row("monster", d.identity, d.name, -1, -1,
            "encounter;damage_and_death;rewards;boss_identity_from_source_not_appearance");
    for (const auto &d : state.rules->magic_pot_recipes)
        row("recipe", d.identity, d.name, d.reward_kind, -1,
            "deposit;natural_processing;discover;pay_elements;craft;consume_reward");
    for (std::size_t id = 0; id < state.rules->jobs.size(); ++id)
        row("profession", static_cast<int>(id), state.rules->jobs[id].name,
            state.rules->jobs[id].type, -1, "unlock;legal_change;growth;mastery_effect");
    // Four shared spell slots are explicit in HumanDefinitionStatsInput, distinct from recipe IDs.
    for (int id = 0; id < 4; ++id)
        row("spell", id, "spell slot", -1, -1,
            "learn_or_profession;actual_cast;damage_or_healing;timing_and_cost");
    for (int id = 1; id <= 5; ++id)
        row("rank", id, "village rank", -1, id,
            "all_real_conditions;apply;celebrate;new_unlock_consumption;full_month_business");
    row("campaign", 0, "natural clear", -1, -1,
        "active_business;natural_date;score;records;continue;inherit;restart");
    // These IDs belong to the test checklist, not the game's definition namespace.
    const char *functions[] = {
        "commerce:buy_and_sell;stock_and_money;new_building_points_and_claim",
        "map:roads;move;demolish;connectivity;continued_trading",
        "residence:eligibility;admit;build;tax_once;retired_references",
        "awards:ranked_candidate;grant_and_decline;finish;later_profession_eligibility",
        "expansion:all_legal_levels;cost_once;new_area_construction_and_use",
        "reports_and_events:natural_trigger;automatic_or_confirmed_lifecycle;rewards_once;resume",
        "combat_recovery:damage;down;rescue;rest;actual_hp_recovery;return_to_battle",
        ("player_save:business_checkpoints;fresh_process;continued_business;original_slot_"
         "preserved")};
    for (std::size_t id = 0; id < std::size(functions); ++id)
        row("function", static_cast<int>(id), "product acceptance checklist", -1, -1,
            functions[id]);
}
void CampaignCoverage::save(const std::filesystem::path &directory, const State &state) const {
    if (segment_.empty())
        return;
    std::ofstream out(directory / ("coverage-segment-" + segment_ + ".tsv"), std::ios::binary);
    out.exceptions(std::ios::badbit | std::ios::failbit);
    write(out, state);
}
namespace {
// Receipt-binding fixtures only. Victory and reward algorithms remain covered by the
// frozen task suites and the natural business route; no fabricated result certifies them.
void task_receipt_binding_contract() {
    auto before = initial_world();
    constexpr std::uint64_t task_id = 42, page_id = 77;
    ref::DungeonFinishTask task;
    task.identity = task_id;
    task.definition = 0;
    before.tasks.emplace(task_id, task);
    before.task_order = {task_id, task_id};
    before.active_task = task_id;
    before.scene.world.world.ai.task_active = true;
    before.participants = {0, 1, 0}; // Original ordered lists may contain duplicates.
    before.task_progress.successes = 20;
    before.task_progress.definitions.at(0).completed = 3;
    auto after = before;
    after.task_progress.successes = 21;
    after.task_progress.definitions.at(0).completed = 4;
    after.active_task.reset();
    after.scene.world.world.ai.task_active = false;
    after.task_order.erase(after.task_order.begin());
    ref::WorldScriptPage result;
    result.id = page_id;
    result.kind = ref::WorldScriptPageKind::raw_page;
    result.legacy_page = 30;
    result.task_identity = task_id;
    result.task_definition = 0;
    after.scripts.pages.push_back(result);
    after.exploration_summaries.emplace(page_id, ref::WorldExplorationSummary{30, task_id, 0, {}});
    const auto receipt = task_victory_receipt(before, after, task_id);
    if (!receipt || receipt->kind != CampaignReceiptKind::task_victory_committed ||
        receipt->definition != 0 || receipt->page != page_id || receipt->subject != task_id ||
        receipt->before[0] != 20 || receipt->after[0] != 21 || receipt->before[1] != 3 ||
        receipt->after[1] != 4 || task_victory_receipt(after, after, task_id))
        throw std::runtime_error("Task receipt failed binding or reconstructed loaded history");
    auto exploration = after;
    exploration.scripts.pages.back().task_identity.reset();
    exploration.scripts.pages.back().task_definition.reset();
    const auto exploration_receipt = task_victory_receipt(before, exploration, task_id);
    if (!exploration_receipt || exploration_receipt->page != receipt->page ||
        exploration_receipt->subject != receipt->subject ||
        exploration_receipt->definition != receipt->definition)
        throw std::runtime_error("Exploration summary binding required encounter-only duplicates");
    // Corrupt one external source binding at a time. No log is published on refusal.
    for (int fault = 0; fault < 19; ++fault) {
        auto old = before, changed = after;
        auto departed = task_id;
        switch (fault) {
        case 0:
            departed = task_id + 1;
            break;
        case 1:
            old.active_task.reset();
            break;
        case 2:
            changed.tasks.at(task_id).definition = 1;
            break;
        case 3:
            changed.exploration_summaries.at(page_id).definition = 1;
            break;
        case 4:
            changed.scripts.pages.back().task_definition = 1;
            break;
        case 5:
            changed.scripts.pages.back().task_identity = task_id + 1;
            break;
        case 6:
            changed.active_task = task_id;
            break;
        case 7:
            changed.scene.world.world.ai.task_active = true;
            break;
        case 8:
            ++changed.task_progress.successes;
            break;
        case 9:
            ++changed.task_progress.definitions.at(0).completed;
            break;
        case 10:
            ++changed.task_progress.definitions.at(1).completed;
            break;
        case 11:
            changed.task_order = old.task_order;
            break;
        case 12:
            changed.participants = {1, 0, 0};
            break;
        case 13:
            changed.exploration_summaries.clear();
            break;
        case 14:
            old.exploration_summaries = changed.exploration_summaries;
            break;
        case 15:
            changed.scripts.pages.pop_back();
            break;
        case 16:
            changed.scripts.pages.back().lifecycle = 4;
            break;
        case 17:
            old.scripts.pages.push_back(result);
            break;
        case 18:
            changed.scripts.pages.back().kind = ref::WorldScriptPageKind::scene;
            break;
        }
        bool rejected{};
        try {
            (void)task_victory_receipt(old, changed, departed);
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        if (!rejected)
            throw std::runtime_error("Invalid task receipt binding accepted: " +
                                     std::to_string(fault));
    }
}

} // namespace

void campaign_coverage_contract() {
    task_receipt_binding_contract();
    const auto state = initial_world();
    CampaignCoverage coverage;
    coverage.observe(state,
                     state); // A save/load snapshot is not evidence of performing its history.
    std::ostringstream out;
    coverage.write(out, state);
    const auto text = out.str();
    if (text.find("revenue_received;") != std::string::npos ||
        text.find("equipped;") != std::string::npos ||
        text.find("promoted;") != std::string::npos ||
        text.find("spell\t3\t") == std::string::npos ||
        text.find("campaign\t0\t") == std::string::npos ||
        text.find("human\t24\t") == std::string::npos ||
        text.find("verified_receipts") == std::string::npos ||
        text.find("\tbound_use;") != std::string::npos)
        throw std::runtime_error(
            "Coverage inferred consumption from a snapshot or omitted a required domain");
    std::ostringstream log;
    const CampaignReceipt queued{
        CampaignReceiptKind::activity_effect_queued, 16, 0, 1, 0, -1, {0, 0, 0, 0}, {10, 20, 1, 0}};
    coverage.record(queued, log);
    std::ostringstream recorded;
    coverage.write(recorded, state);
    if (recorded.str().find("effect_queued;") == std::string::npos ||
        recorded.str().find("effect_applied;") != std::string::npos ||
        recorded.str().find("pot_opened;") != std::string::npos ||
        log.str().find("COVERAGE_RECEIPT\tactivity\t16\teffect_queued\t") == std::string::npos)
        throw std::runtime_error("Queued popularity was certified as an actual activity effect");
    bool rejected{};
    const auto validated_log = log.str();
    try {
        auto invalid = queued;
        invalid.page = 0;
        coverage.record(invalid, log);
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    if (!rejected || log.str() != validated_log)
        throw std::runtime_error("Unbound campaign receipt was accepted or partially published");
    const auto expect_rejected = [&](const CampaignReceipt &invalid) {
        const auto previous_log = log.str();
        std::ostringstream previous_inventory;
        coverage.write(previous_inventory, state);
        bool refused{};
        try {
            coverage.record(invalid, log);
        } catch (const std::runtime_error &) {
            refused = true;
        }
        std::ostringstream current_inventory;
        coverage.write(current_inventory, state);
        if (!refused || log.str() != previous_log ||
            current_inventory.str() != previous_inventory.str())
            throw std::runtime_error(
                "Rejected campaign receipt changed its log or definition evidence");
    };
    struct Case {
        CampaignReceiptKind kind;
        const char *category, *stage;
        int definitions;
        std::uint64_t subject;
        int recipient{-1};
    };
    const Case cases[] = {
        {CampaignReceiptKind::facility_blueprint_paid, "facility", "blueprint_points_paid", 85, 0},
        {CampaignReceiptKind::facility_blueprint_claimed, "facility", "blueprint_claimed", 85, 0},
        {CampaignReceiptKind::facility_construction_paid, "facility", "construction_paid", 85, 1},
        {CampaignReceiptKind::activity_pot_opened, "activity", "pot_opened", 31, 0},
        {CampaignReceiptKind::activity_pot_expanded, "activity", "pot_expanded", 31, 0},
        {CampaignReceiptKind::activity_popularity_queued, "activity", "popularity_queued", 31, 0},
        {CampaignReceiptKind::activity_satisfaction_applied, "activity", "satisfaction_applied", 31,
         0, 24},
        {CampaignReceiptKind::activity_attribute_applied, "activity", "attribute_applied_to_human",
         31, 0, 24},
        {CampaignReceiptKind::weapon_reward_equipped, "weapon", "reward_equipped", 33, 1, 24},
        {CampaignReceiptKind::armour_reward_equipped, "armour", "reward_equipped", 50, 1, 24},
        {CampaignReceiptKind::accessory_reward_equipped, "accessory", "reward_equipped", 30, 1, 24},
        {CampaignReceiptKind::weapon_paid_equipped, "weapon", "paid_equipped", 33, 1, 24},
        {CampaignReceiptKind::armour_paid_equipped, "armour", "paid_equipped", 50, 1, 24},
        {CampaignReceiptKind::accessory_paid_equipped, "accessory", "paid_equipped", 30, 1, 24},
        {CampaignReceiptKind::weapon_combat_applied, "weapon", "combat_attributes_applied", 33, 1,
         24},
        {CampaignReceiptKind::armour_combat_applied, "armour", "combat_attributes_applied", 50, 1,
         24},
        {CampaignReceiptKind::accessory_combat_applied, "accessory", "combat_attributes_applied",
         30, 1, 24},
        {CampaignReceiptKind::human_medal_awarded, "human", "medal_awarded", 25, 0, 24},
        {CampaignReceiptKind::human_home_admitted, "human", "home_admitted", 25, 1, 24},
        {CampaignReceiptKind::task_victory_committed, "task", "victory_committed", 81, 1},
        {CampaignReceiptKind::item_commerce_bought, "item", "commerce_bought", 36, 0},
        {CampaignReceiptKind::item_facility_consumed, "item", "facility_bound_use", 36, 1},
        {CampaignReceiptKind::facility_item_improved, "facility", "item_improvement_applied", 85,
         1},
        {CampaignReceiptKind::item_task_loot_claimed, "item", "task_loot_claimed", 36, 1},
        {CampaignReceiptKind::weapon_task_loot_claimed, "weapon", "task_loot_claimed", 33, 1},
        {CampaignReceiptKind::armour_task_loot_claimed, "armour", "task_loot_claimed", 50, 1},
        {CampaignReceiptKind::accessory_task_loot_claimed, "accessory", "task_loot_claimed", 30, 1},
        {CampaignReceiptKind::human_home_tax_collected, "human", "home_tax_collected", 25, 0, 24},
        {CampaignReceiptKind::profession_points_paid, "profession", "points_paid", 23, 1, 24},
        {CampaignReceiptKind::profession_set, "profession", "set", 23, 1, 24},
        {CampaignReceiptKind::profession_stats_applied, "profession", "stats_applied", 23, 1, 24},
        {CampaignReceiptKind::spell_available, "spell", "available", 4, 1, 24}};
    for (const auto &entry : cases) {
        CampaignReceipt receipt{
            entry.kind,      entry.definitions - 1, 2,           1, entry.subject,
            entry.recipient, {120, 0, 2, 0},        {0, 1, 2, 0}};
        if (entry.kind == CampaignReceiptKind::human_home_tax_collected)
            receipt.after[1] = 0;
        std::string activity_values;
        if (entry.kind == CampaignReceiptKind::activity_satisfaction_applied) {
            receipt.before = {20, 20, -1, 22};
            receipt.after = {22, 22, -1, 22};
            activity_values = "\t20\t20\t-1\t22\t22\t22\t-1\t22\n";
        }
        if (entry.kind == CampaignReceiptKind::activity_attribute_applied) {
            receipt.before = {0, 108, 0, 22};
            receipt.after = {5, 113, 0, 22};
            activity_values = "\t0\t108\t0\t22\t5\t113\t0\t22\n";
        }
        std::string profession_values;
        if (entry.kind == CampaignReceiptKind::profession_points_paid) {
            receipt.before = {120, 0, 2, 5};
            receipt.after = {65, 1, 2, 5};
            profession_values = "\t120\t0\t2\t5\t65\t1\t2\t5\n";
        } else if (entry.kind == CampaignReceiptKind::profession_set) {
            receipt.before = {2, 2, 54, 1};
            receipt.after = {22, 22, 55, 1};
            profession_values = "\t2\t2\t54\t1\t22\t22\t55\t1\n";
        } else if (entry.kind == CampaignReceiptKind::spell_available) {
            receipt.before = {0, 0, 6, 1};
            receipt.after = {1, 0, 6, 1};
            profession_values = "\t0\t0\t6\t1\t1\t0\t6\t1\n";
        }
        const auto start = log.str().size();
        coverage.record(receipt, log);
        const auto expected = std::string("COVERAGE_RECEIPT\t") + entry.category + '\t' +
                              std::to_string(receipt.definition) + '\t' + entry.stage + "\t2\t1\t" +
                              std::to_string(entry.recipient) + '\t' +
                              std::to_string(entry.subject) +
                              (!profession_values.empty() ? profession_values
                               : !activity_values.empty() ? activity_values
                               : entry.kind == CampaignReceiptKind::human_home_tax_collected
                                   ? "\t120\t0\t2\t0\t0\t0\t2\t0\n"
                                   : "\t120\t0\t2\t0\t0\t1\t2\t0\n");
        if (log.str().substr(start) != expected)
            throw std::runtime_error(
                "Campaign receipt lost its namespace, stage or numeric evidence");
        if (!activity_values.empty()) {
            auto wrong_attribute = receipt;
            ++wrong_attribute.after[2];
            expect_rejected(wrong_attribute);
            auto wrong_job = receipt;
            ++wrong_job.after[3];
            expect_rejected(wrong_job);
            auto wrong_subject = receipt;
            wrong_subject.subject = 1;
            expect_rejected(wrong_subject);
        }
        auto invalid = receipt;
        invalid.definition = -1;
        expect_rejected(invalid);
        invalid.definition = entry.definitions;
        expect_rejected(invalid);
        invalid = receipt;
        invalid.page = 0;
        expect_rejected(invalid);
        if (entry.subject) {
            invalid = receipt;
            invalid.subject = 0;
            expect_rejected(invalid);
        }
        if (entry.recipient >= 0) {
            invalid = receipt;
            invalid.recipient = -1;
            expect_rejected(invalid);
            invalid.recipient = 25;
            expect_rejected(invalid);
            const bool human_definition =
                entry.kind == CampaignReceiptKind::human_medal_awarded ||
                entry.kind == CampaignReceiptKind::human_home_admitted ||
                entry.kind == CampaignReceiptKind::human_home_tax_collected;
            invalid = receipt;
            invalid.recipient = 0;
            if (human_definition) {
                expect_rejected(
                    invalid); // An otherwise valid person cannot claim another definition.
                invalid.definition = 0;
            }
            coverage.record(invalid,
                            log); // Both endpoints of the source person directory are valid.
        }
        const bool task_loot = std::string(entry.stage) == "task_loot_claimed";
        const int related_limit = task_loot                                                   ? 81
                                  : entry.kind == CampaignReceiptKind::item_facility_consumed ? 85
                                  : entry.kind == CampaignReceiptKind::facility_item_improved ? 36
                                                                                              : 0;
        if (related_limit) {
            for (const auto related : {-1, related_limit}) {
                invalid = receipt;
                invalid.before[3] = invalid.after[3] = related;
                expect_rejected(invalid);
            }
            invalid = receipt;
            invalid.before[3] = invalid.after[3] = related_limit - 1;
            coverage.record(invalid, log);
            invalid = receipt;
            invalid.after[3] = 1;
            expect_rejected(invalid);
            invalid = receipt;
            invalid.recipient = 0;
            expect_rejected(invalid);
        }
        if (task_loot) {
            invalid = receipt;
            invalid.before[2] = 0;
            expect_rejected(invalid);
            invalid = receipt;
            invalid.after[2] = 1;
            expect_rejected(invalid);
        }
        if (entry.kind == CampaignReceiptKind::human_home_tax_collected) {
            for (const int month : {-1, 12}) {
                invalid = receipt;
                invalid.before[3] = invalid.after[3] = month;
                expect_rejected(invalid);
            }
            invalid = receipt;
            invalid.after[3] = 1;
            expect_rejected(invalid);
            invalid = receipt;
            invalid.after[1] = 1;
            expect_rejected(invalid);
        }
        if (entry.kind == CampaignReceiptKind::profession_points_paid) {
            for (const auto values : {std::array<std::int64_t, 4>{-1, 1, 2, 5},
                                       {121, 1, 2, 5}, {1000, 1, 2, 5}, {65, 0, 2, 5},
                                       {65, 1, 22, 5}, {65, 1, 2, 4}}) {
                invalid = receipt;
                invalid.after = values;
                expect_rejected(invalid);
            }
            invalid = receipt;
            invalid.after[0] = invalid.before[0]; // Wire permits zero fees; the source flow owns the quote.
            coverage.record(invalid, log);
            invalid.before[0] = 1200;
            invalid.after[0] = 999; // The source clamps only the result, not the input.
            coverage.record(invalid, log);
        } else if (entry.kind == CampaignReceiptKind::profession_set) {
            for (const auto values : {std::array<std::int64_t, 4>{22, 2, 55, 1},
                                       {2, 22, 55, 1}, {22, 22, 54, 1}, {22, 22, 56, 1},
                                       {22, 22, 55, 2}}) {
                invalid = receipt;
                invalid.after = values;
                expect_rejected(invalid);
            }
        } else if (entry.kind == CampaignReceiptKind::profession_stats_applied) {
            invalid = receipt;
            invalid.after[0] = 10000;
            expect_rejected(invalid);
            invalid.after = invalid.before; // NewQ may be the gain even if all four combat values match.
            coverage.record(invalid, log);
        } else if (entry.kind == CampaignReceiptKind::spell_available) {
            for (const auto values : {std::array<std::int64_t, 4>{0, 0, 6, 1},
                                       {1, 1, 6, 1}, {1, 0, 5, 1}, {1, 0, 6, 10}}) {
                invalid = receipt;
                invalid.after = values;
                expect_rejected(invalid);
            }
            invalid = receipt;
            invalid.before[2] = invalid.after[2] = 5;
            expect_rejected(invalid); // Current5 grants slot0, not this slot3.
            invalid.before[1] = invalid.after[1] = 1;
            coverage.record(invalid, log); // Existing learning can enter Q at a later source recompute.
            invalid.after[1] = 0;
            expect_rejected(invalid); // Availability never certifies changing the permanent bit.
        }
    }
    const CampaignReceipt hp{
        CampaignReceiptKind::item_hp_restored, 29, 2, 1, 1, 0, {1, 0, 0, 0}, {51, 0, 0, 0}};
    auto invalid_hp = hp;
    invalid_hp.recipient = -1;
    expect_rejected(invalid_hp);
    invalid_hp = hp;
    invalid_hp.subject = 0;
    expect_rejected(invalid_hp);
    auto unknown = queued;
    unknown.kind = static_cast<CampaignReceiptKind>(-1);
    expect_rejected(unknown);
}
} // namespace ark::test
