#include "world_campaign_coverage.hpp"
#include "support/world_fixture.hpp"
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace ark::test {
namespace {
using State = app::WorldState;
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
        return {"item", "hp_restored", 36, true};
    case Kind::facility_blueprint_paid:
        return {"facility", "blueprint_points_paid", 85, false};
    case Kind::facility_blueprint_claimed:
        return {"facility", "blueprint_claimed", 85, false};
    case Kind::facility_construction_paid:
        return {"facility", "construction_paid", 85, false};
    case Kind::activity_pot_opened:
        return {"activity", "pot_opened", 31, false};
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

void CampaignCoverage::mark(const char *kind, int id, const char *event) {
    observations_[{kind, id}].insert(event);
}
void CampaignCoverage::record(const CampaignReceipt &receipt, std::ostream &log) {
    const auto identity = receipt_identity(receipt.kind);
    if (receipt.definition < 0 || receipt.definition >= identity.definitions || !receipt.page ||
        (identity.bound_human && receipt.recipient < 0) ||
        ((receipt.kind == CampaignReceiptKind::item_hp_restored ||
          receipt.kind == CampaignReceiptKind::facility_construction_paid) &&
         !receipt.subject))
        throw std::runtime_error("Campaign receipt lacks its source definition/page/human binding");
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
void campaign_coverage_contract() {
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
    };
    const Case cases[] = {
        {CampaignReceiptKind::facility_blueprint_paid, "facility", "blueprint_points_paid", 85, 0},
        {CampaignReceiptKind::facility_blueprint_claimed, "facility", "blueprint_claimed", 85, 0},
        {CampaignReceiptKind::facility_construction_paid, "facility", "construction_paid", 85, 1},
        {CampaignReceiptKind::activity_pot_opened, "activity", "pot_opened", 31, 0}};
    for (const auto &entry : cases) {
        const CampaignReceipt receipt{
            entry.kind, entry.definitions - 1, 2,           1, entry.subject,
            -1,         {120, 0, 2, 0},        {0, 1, 2, 0}};
        const auto start = log.str().size();
        coverage.record(receipt, log);
        const auto expected = std::string("COVERAGE_RECEIPT\t") + entry.category + '\t' +
                              std::to_string(receipt.definition) + '\t' + entry.stage +
                              "\t2\t1\t-1\t" + std::to_string(entry.subject) +
                              "\t120\t0\t2\t0\t0\t1\t2\t0\n";
        if (log.str().substr(start) != expected)
            throw std::runtime_error(
                "Facility/pot receipt lost its namespace, stage or numeric evidence");
        auto invalid = receipt;
        invalid.definition = -1;
        expect_rejected(invalid);
        invalid.definition = entry.definitions;
        expect_rejected(invalid);
        invalid = receipt;
        invalid.page = 0;
        expect_rejected(invalid);
        if (entry.kind == CampaignReceiptKind::facility_construction_paid) {
            invalid = receipt;
            invalid.subject = 0;
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
