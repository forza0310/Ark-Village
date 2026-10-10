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
    case Kind::item_consumed:
        return {"item", "bound_use", 36, true};
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
    case Kind::weapon_reward_equipped:
        return {"weapon", "reward_equipped", 33, true, true, 25};
    case Kind::armour_reward_equipped:
        return {"armour", "reward_equipped", 50, true, true, 25};
    case Kind::accessory_reward_equipped:
        return {"accessory", "reward_equipped", 30, true, true, 25};
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

std::vector<CampaignReceipt> residence_tax_receipts(const State &before, const State &after) {
    const auto page = std::find_if(before.scripts.pages.rbegin(), before.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    if (page == before.scripts.pages.rend() || page->kind != ref::WorldScriptPageKind::raw_page ||
        page->legacy_page != 98)
        return {};
    const auto closed = std::find_if(after.scripts.pages.begin(), after.scripts.pages.end(),
                                    [&](const auto &p) { return p.id == page->id; });
    if (closed != after.scripts.pages.end() && closed->lifecycle != 4)
        return {}; // Paused, not initialized, or not yet consumed: no money receipt.
    const auto need = [](bool value) {
        if (!value)
            throw std::runtime_error("Residence tax lost its raw98 income/reset binding");
    };
    const auto &old_date = before.scene.calendar, &new_date = after.scene.calendar;
    need(before.rules && after.rules && old_date.month >= 0 && old_date.month < 12 &&
         std::tie(old_date.year, old_date.month, old_date.subperiod, old_date.units,
                  old_date.previous_units, old_date.month_ticks) ==
             std::tie(new_date.year, new_date.month, new_date.subperiod, new_date.units,
                      new_date.previous_units, new_date.month_ticks));
    const int month = old_date.month;
    const auto old_cash = before.scene.world.world.ai.accounting.funds();
    const auto new_cash = after.scene.world.world.ai.accounting.funds();
    std::int64_t total{};
    std::vector<int> residents;
    for (const auto &human : before.rules->humans) {
        const int id = human.identity;
        need(after.human_presence.at(id) == before.human_presence.at(id) &&
             after.human_homes.at(id) == before.human_homes.at(id));
        if (before.human_presence.at(id) != 0 && before.human_homes.at(id)[2] == 1) {
            total += before.human_calendar.at(id).legacy_G;
            residents.push_back(id);
        }
        // The source resets all definitions, including absent/nonresident humans.
        need(after.human_calendar.at(id).legacy_F == 0 &&
             after.human_calendar.at(id).legacy_G == 0 &&
             after.scene.world.world.ai.battle.humans.at(id).battle_reward_stat == 0);
    }
    const auto old_random = before.scene.random.snapshot(), new_random = after.scene.random.snapshot();
    need(!residents.empty() &&
         !(total > 0 && old_cash > std::numeric_limits<std::int64_t>::max() - total) &&
         !(total < 0 && old_cash < std::numeric_limits<std::int64_t>::min() - total) &&
         new_cash == old_cash + total &&
         std::int64_t(after.monthly_cash[month][4][0]) - before.monthly_cash[month][4][0] == total &&
         std::tie(old_random.engine_state, old_random.tape, old_random.cursor, old_random.tape_mode) ==
             std::tie(new_random.engine_state, new_random.tape, new_random.cursor, new_random.tape_mode) &&
         before.sound_requests == after.sound_requests &&
         before.village_points == after.village_points && before.popularity == after.popularity);
    for (int m = 0; m < 12; ++m)
        for (int category = 0; category < 5; ++category)
            for (int side = 0; side < 2; ++side)
                if (m != month || category != 4 || side != 0)
                    need(before.monthly_cash[m][category][side] == after.monthly_cash[m][category][side]);
    need(after.scripts.notices.size() == before.scripts.notices.size() + 1 &&
         after.scripts.notices.back().message == 28);
    std::vector<CampaignReceipt> receipts;
    for (const int id : residents)
        receipts.push_back({CampaignReceiptKind::human_home_tax_collected,
                            id, after.simulation_steps, page->id, 0, id,
                            {old_cash, before.human_calendar.at(id).legacy_G,
                             before.monthly_cash[month][4][0], month},
                            {new_cash, 0, after.monthly_cash[month][4][0], month}});
    return receipts;
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
        {CampaignReceiptKind::weapon_reward_equipped, "weapon", "reward_equipped", 33, 1, 24},
        {CampaignReceiptKind::armour_reward_equipped, "armour", "reward_equipped", 50, 1, 24},
        {CampaignReceiptKind::accessory_reward_equipped, "accessory", "reward_equipped", 30, 1, 24},
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
        {CampaignReceiptKind::accessory_task_loot_claimed, "accessory", "task_loot_claimed", 30,
         1},
        {CampaignReceiptKind::human_home_tax_collected, "human", "home_tax_collected", 25, 0, 24}};
    for (const auto &entry : cases) {
        CampaignReceipt receipt{
            entry.kind,      entry.definitions - 1, 2,           1, entry.subject,
            entry.recipient, {120, 0, 2, 0},        {0, 1, 2, 0}};
        if (entry.kind == CampaignReceiptKind::human_home_tax_collected)
            receipt.after[1] = 0;
        const auto start = log.str().size();
        coverage.record(receipt, log);
        const auto expected = std::string("COVERAGE_RECEIPT\t") + entry.category + '\t' +
                              std::to_string(receipt.definition) + '\t' + entry.stage + "\t2\t1\t" +
                              std::to_string(entry.recipient) + '\t' +
                              std::to_string(entry.subject) +
                              (entry.kind == CampaignReceiptKind::human_home_tax_collected
                                   ? "\t120\t0\t2\t0\t0\t0\t2\t0\n"
                                   : "\t120\t0\t2\t0\t0\t1\t2\t0\n");
        if (log.str().substr(start) != expected)
            throw std::runtime_error(
                "Campaign receipt lost its namespace, stage or numeric evidence");
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
            const bool human_definition = entry.kind == CampaignReceiptKind::human_medal_awarded ||
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
