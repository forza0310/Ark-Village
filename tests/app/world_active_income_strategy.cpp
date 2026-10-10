#include "world_active_income_strategy.hpp"
#include "../support/world_fixture.hpp"
#include "ark/simulation/facilities/rules/facility_items.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace ref = sim::rules;
using State = app::WorldState;
using Kind = app::WorldCommandKind;
using Command = app::WorldCommand;
using Commerce = sim::StartupCommerceAction;
using Item = sim::StartupFacilityItemAction;
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(std::string("Income strategy: ") + message);
}
std::int64_t cash(const State &s) { return s.scene.world.world.ai.accounting.funds(); }
// Assemble source inputs only; do not reproduce price, affinity or cap arithmetic.
ref::FacilityEconomyInput input(const State &s, std::uint64_t id) {
    const int d = s.scene.world.world.facilities.at(id).placement.definition_id;
    const auto &use = s.scene.world.world.facility_uses.at(d);
    ref::FacilityEconomyInput in;
    in.level = use.level;
    in.completed_definition_uses = use.completed_uses;
    in.definition_improvements = s.scripts.facilities.at(d).improvements;
    const auto &neighbours = s.neighbourhood.at(id);
    std::copy(neighbours.begin(), neighbours.end(), in.instance_modifiers.begin());
    for (const auto &h : s.rules->humans)
        if (s.human_presence.at(h.identity)) {
            const int job =
                s.scene.world.world.ai.growth.at(h.identity).definition.current_profession;
            ++in.legacy_job_counts.at(s.rules->jobs.at(job).type);
        }
    return in;
}
std::int64_t last_income(const State &s, std::uint64_t id) {
    // The annual buckets reset in January; do not treat old/partial data as a full month.
    const int month = s.scene.calendar.month;
    const auto bucket = s.facility_monthly_cash.find(id);
    return month > 0 && bucket != s.facility_monthly_cash.end() ? bucket->second[month - 1][0] : 0;
}
Command command(Kind kind, std::uint64_t page = 0) {
    Command c;
    c.kind = kind;
    c.page = page;
    return c;
}
Command commerce(std::uint64_t page, Commerce action, int selection = 0) {
    auto c = command(Kind::commerce_action, page);
    c.commerce_action = action;
    c.selection = selection;
    return c;
}
Command item(std::uint64_t page, Item action, int selection = -1) {
    auto c = command(Kind::facility_item_action, page);
    c.facility_item_action = action;
    c.selection = selection;
    return c;
}
} // namespace

std::optional<IncomeInvestment> choose_income_investment(const State &s, std::int64_t available,
                                                         std::optional<std::uint64_t> new_shop,
                                                         bool inventory_only) {
    std::optional<IncomeInvestment> best;
    std::optional<IncomeInvestment> amenities;
    std::optional<std::tuple<std::int64_t, std::int64_t>> amenities_priority;
    std::int64_t best_introduction_score{};
    for (const auto &[id, f] : s.scene.world.world.facilities) {
        if (new_shop && id != *new_shop)
            continue;
        const auto &d = s.rules->facilities.at(f.placement.definition_id);
        if (f.status != 1 || (d.kind != 3 && d.kind != 9) || d.detail == 1 || d.detail == 4 ||
            d.detail == 5 || d.detail == 6 || (s.facility_flags.at(id) & 1U))
            continue;
        const auto in = input(s, id);
        for (const auto &i : s.rules->items) {
            const bool buy = s.items.at(i.identity).inventory == 0;
            if (inventory_only && buy)
                continue; // Dungeon rewards are consumed as held inventory, never replenished by
                          // purchase.
            const auto cost = buy ? i.commerce_price : 0;
            if (cost < 0 || cost > available ||
                (buy && s.shop_item_stock.at(i.identity).quantity <= 0))
                continue;
            const auto r = ref::prepare_facility_improvement(
                {d.id, d.legacy_icon, s.rules->facility_initial.at(d.id).item_affinities,
                 d.economy},
                {i.identity, i.category, i.facility_improvements}, in);
            if (!r.candidate)
                continue;
            const auto &deltas = r.candidate->visible_deltas;
            const bool nonnegative =
                std::all_of(deltas.begin(), deltas.end(), [](auto delta) { return delta >= 0; });
            if (inventory_only && !nonnegative)
                continue;
            // Quality feeds satisfaction; charm feeds source selection weights.
            // Neither is converted to invented money. Consider this free-stock
            // fallback only after the ordinary historical-income choices fail.
            if (inventory_only && !new_shop && (deltas[1] > 0 || deltas[2] > 0)) {
                const auto priority = std::make_tuple(last_income(s, id), deltas[1] + deltas[2]);
                if (!amenities_priority || priority > *amenities_priority) {
                    amenities_priority = priority;
                    amenities = IncomeInvestment{id, d.id,  i.identity, 0,    deltas[0],
                                                 0,  false, false,      true, deltas};
                }
            }
            if (new_shop ? deltas[2] <= 0 : deltas[0] <= 0)
                continue;
            // New shops have no revenue history. Charm20 is a bounded player goal,
            // using inventory or at most500G per purchase, never invented income.
            if (new_shop) {
                if (r.candidate->before.definition_attributes[2] >= 20 || cost > 500)
                    continue;
                const auto score =
                    r.candidate->visible_deltas[2] * 1000 + r.candidate->visible_deltas[0];
                if (score > best_introduction_score) {
                    best_introduction_score = score;
                    best = IncomeInvestment{id, d.id, i.identity, cost,  deltas[0],
                                            0,  buy,  true,       false, deltas};
                }
                continue;
            }
            std::int64_t gain{};
            // Same-definition instances share improvements but retain their own adjacency.
            for (const auto &[other_id, other] : s.scene.world.world.facilities) {
                if (other.placement.definition_id != d.id || other.status != 1 || other.price <= 0)
                    continue;
                const auto prediction = ref::prepare_facility_improvement(
                    {d.id, d.legacy_icon, s.rules->facility_initial.at(d.id).item_affinities,
                     d.economy},
                    {i.identity, i.category, i.facility_improvements}, input(s, other_id));
                if (prediction.candidate)
                    gain += last_income(s, other_id) * prediction.candidate->visible_deltas[0] /
                            other.price;
            }
            // A four-month simple payback bound is a player policy, not a revenue guarantee.
            if (gain <= 0 || cost > 4 * gain)
                continue;
            IncomeInvestment candidate{id,   d.id, i.identity, cost,  deltas[0],
                                       gain, buy,  false,      false, deltas};
            if (!best || gain * std::max<std::int64_t>(1, best->cost) >
                             best->monthly_gain * std::max<std::int64_t>(1, cost))
                best = candidate;
        }
    }
    return best ? best : amenities;
}

void inspect_income(const State &s, std::ostream &out) {
    out << "INCOME_INPUT month=" << s.scene.calendar.year * 12 + s.scene.calendar.month
        << " cash=" << cash(s) << " record=" << s.maximum_income << '\n';
    for (const auto &[id, f] : s.scene.world.world.facilities)
        if (f.kind == 3 || f.kind == 9)
            out << "SHOP id=" << id << " definition=" << f.placement.definition_id
                << " anchor=" << f.placement.anchor.x << ',' << f.placement.anchor.y
                << " status=" << f.status << " flags=" << s.facility_flags.at(id)
                << " category=" << f.category << " shared_charm="
                << sim::startup_world_facility_values(s, id)->definition_attributes[2]
                << " price=" << f.price << " level="
                << s.scene.world.world.facility_uses.at(f.placement.definition_id).level
                << " previous_month_income=" << last_income(s, id) << " sales=" << f.sales << '\n';
    const auto best = choose_income_investment(s, 10000);
    for (const auto &d : s.rules->facilities)
        if ((d.kind == 3 || d.kind == 9) && d.unlock_rank >= 0 && d.unlock_rank <= s.rank &&
            s.facility_presence.at(d.id) == 0) {
            const auto quote = sim::startup_world_build_quote(s, d.id);
            if (quote)
                out << "CHAMBER definition=" << d.id << " rank=" << d.unlock_rank
                    << " points=" << s.rules->facility_initial.at(d.id).capacity
                    << " price=" << quote->definition_attributes[0]
                    << " construction=" << quote->construction_cost
                    << " maintenance=" << quote->definition_attributes[3] << " shape=" << d.shape
                    << '\n';
        }
    if (best)
        out << "INVESTMENT facility=" << best->facility << " item=" << best->item
            << " cost=" << best->cost << " price_delta=" << best->price_delta
            << " forecast_monthly_gain=" << best->monthly_gain << '\n';
    else
        out << "INVESTMENT none (no positive observed-price payback within budget)\n";
    // Read-only ordinary-shop candidate membership from each present human's current
    // cell. This is opportunity evidence, not a forced activity or a simulated visit.
    const auto adapter = sim::startup_world_runtime_adapter();
    for (const auto actor : s.scene.world.world.ai.human_order) {
        const auto input = adapter.actors.decision(s, actor);
        if (!input || !input->landing_departure)
            continue;
        auto catalogue = input->landing_departure->catalogue;
        catalogue.legacy_activity = 0;
        const auto cell = s.scene.world.world.ai.contexts.at(actor).cell;
        const auto field = ref::prepare_world_departure_field(s.scene.world.world.map, cell,
                                                              std::numeric_limits<int>::max());
        if (!field.field)
            continue;
        const auto candidates = ref::collect_activity_candidates(*field.field, catalogue);
        out << "CUSTOMER actor=" << actor.value << " cell=" << cell.x << ',' << cell.y
            << " snapshot_error=" << int(candidates.error);
        if (candidates.snapshot) {
            std::map<int, int> counts;
            for (const auto &c : candidates.snapshot->cells)
                if (c.instance && c.instance->legacy_phase == 1)
                    ++counts[c.instance->definition_id];
            for (const auto &d : s.rules->facilities)
                if (d.unlock_rank == 2 && (d.kind == 3 || d.kind == 9))
                    out << " definition" << d.id << "_cells=" << counts[d.id];
        }
        out << '\n';
    }
}

IncomeDecision ActiveIncomeStrategy::next(const State &s, std::int64_t reserve, bool start) {
    // Moving or replacing a building creates a new instance identity. Retire old
    // introductions instead of dereferencing an instance from a previous layout.
    for (auto i = new_shops_.begin(); i != new_shops_.end();) {
        if (!s.scene.world.world.facilities.count(*i))
            i = new_shops_.erase(i);
        else
            ++i;
    }
    const ref::WorldScriptPage *page{};
    for (auto p = s.scripts.pages.rbegin(); p != s.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4) {
            page = &*p;
            break;
        }
    if (!page || page->lifecycle == 0)
        return {active(), {}};
    const auto &p = *page;
    if (p.kind == ref::WorldScriptPageKind::scene && s.scene.scene_state == 0) {
        if (choice_ && improved_)
            choice_.reset();
        if (choice_ && !s.scene.world.world.facilities.count(choice_->facility)) {
            require(!consumed_, "consumed investment lost its target before effect verification");
            choice_.reset();
        }
        if (!choice_ && start &&
            (reward_only_ || (uses_ < 24 && s.rank == 2 && s.maximum_income < 35000)) &&
            s.scene.calendar.year * 12 + s.scene.calendar.month < stop_month_) {
            // A free held item needs no cash reservation. Paid experiments retain
            // their original total budget, two-star boundary and 24-use limit.
            const auto available = reward_only_ ? 0 : std::min(10000 - spent_, cash(s) - reserve);
            bool constructing{};
            for (const auto id : new_shops_) {
                if (s.scene.world.world.facilities.at(id).status != 1) {
                    constructing = true;
                    continue;
                }
                choice_ = choose_income_investment(s, available, id, reward_only_);
                if (choice_)
                    break;
            }
            if (!choice_ && reward_only_)
                for (const auto &[id, facility] : s.scene.world.world.facilities) {
                    (void)facility;
                    if (last_income(s, id) > 0 || new_shops_.count(id))
                        continue;
                    choice_ = choose_income_investment(s, available, id, true);
                    if (choice_)
                        break;
                }
            if (!choice_ && (!constructing || reward_only_))
                choice_ = choose_income_investment(s, available, {}, reward_only_);
            purchased_ = consumed_ = improved_ = false;
            if (choice_)
                std::cout << "INVEST plan facility=" << choice_->facility
                          << " item=" << choice_->item << " cost=" << choice_->cost
                          << " introduction=" << choice_->introduction
                          << " amenities=" << choice_->amenities
                          << " quality_delta=" << choice_->visible_deltas[1]
                          << " charm_delta=" << choice_->visible_deltas[2]
                          << " forecast_gain=" << choice_->monthly_gain << std::endl;
        }
        if (!choice_)
            return {};
        if (choice_->purchase && !purchased_)
            return {true, command(Kind::open_commerce)};
        auto c = command(Kind::open_facility);
        c.facility = choice_->facility;
        return {true, c};
    }
    if (!choice_)
        return {};
    if (p.legacy_page >= 83 && p.legacy_page <= 86) {
        const auto view = sim::inspect_startup_world_commerce_page(s, p.id);
        if (!view)
            return {true, {}};
        if (view->raw == 86)
            return {true, commerce(p.id, Commerce::confirm)};
        if (purchased_)
            return {true, commerce(p.id, Commerce::cancel)};
        if (view->raw == 83)
            return {true,
                    commerce(p.id, view->selection == 0 ? Commerce::confirm : Commerce::select)};
        require(view->raw == 84 && view->mode == 0, "unexpected commerce page");
        const auto found = std::find(view->entries.begin(), view->entries.end(), choice_->item);
        require(found != view->entries.end(), "selected stock disappeared");
        const int index = static_cast<int>(found - view->entries.begin());
        return {
            true,
            commerce(p.id, view->selection == index ? Commerce::confirm : Commerce::select, index)};
    }
    if (p.legacy_page == 74 && s.facility_page_bindings.at(p.id) == choice_->facility) {
        auto c = command(Kind::facility_action, p.id);
        c.facility_action = consumed_ ? sim::StartupFacilityPageAction::cancel
                                      : sim::StartupFacilityPageAction::confirm;
        return {true, c};
    }
    if (p.legacy_page >= 75 && p.legacy_page <= 77) {
        if (!s.facility_item_pages_initialized.count(p.id))
            return {true, {}};
        if (p.legacy_page == 76)
            return {true, {}};
        if (p.legacy_page == 77) {
            const int count = s.page_counters.at(p.id);
            return {true, count < 49 || count >= 55
                              ? std::optional<Command>(item(p.id, Item::confirm))
                              : std::nullopt};
        }
        if (consumed_)
            return {true, item(p.id, Item::cancel)};
        const auto &list = s.facility_item_page_lists.at(p.id);
        const auto found = std::find(list.begin(), list.end(), choice_->item);
        require(found != list.end(), "purchased item is absent from facility inventory");
        const int index = static_cast<int>(found - list.begin());
        return {true, item(p.id,
                           s.facility_item_page_selections.at(p.id) == index ? Item::confirm
                                                                             : Item::select,
                           index)};
    }
    return {}; // Source scripts/upgrade pages keep their existing player handlers.
}

void ActiveIncomeStrategy::observe(const State &before, const Command &c, const State &after) {
    if (!choice_)
        return;
    const auto &v = *choice_;
    if (c.kind == Kind::commerce_action && c.commerce_action == Commerce::confirm &&
        after.items.at(v.item).inventory > before.items.at(v.item).inventory) {
        require(!purchased_ && cash(before) - cash(after) == v.cost &&
                    after.items.at(v.item).inventory == before.items.at(v.item).inventory + 1 &&
                    before.shop_item_stock.at(v.item).quantity -
                            after.shop_item_stock.at(v.item).quantity ==
                        1,
                "purchase must pay once and transfer one stock unit");
        purchased_ = true;
        spent_ += v.cost;
        std::cout << "INVEST paid=" << v.cost << " total=" << spent_ << std::endl;
    }
    if (c.kind == Kind::facility_item_action && c.facility_item_action == Item::confirm &&
        after.items.at(v.item).inventory < before.items.at(v.item).inventory) {
        require(!consumed_ &&
                    before.items.at(v.item).inventory - after.items.at(v.item).inventory == 1 &&
                    cash(before) == cash(after),
                "facility use consumes one item without a second charge");
        consumed_ = true;
        ++uses_;
    }
}
void ActiveIncomeStrategy::observe_tick(const State &before, const State &after) {
    if (!choice_ || !consumed_ || improved_)
        return;
    const auto &v = *choice_;
    if (before.scripts.facilities.at(v.definition).improvements ==
        after.scripts.facilities.at(v.definition).improvements)
        return;
    const auto delta = after.scene.world.world.facilities.at(v.facility).price -
                       before.scene.world.world.facilities.at(v.facility).price;
    require(delta == v.price_delta && (delta > 0 || v.introduction || v.amenities),
            "committed facility price differs from selected forecast");
    const auto old_values = sim::startup_world_facility_values(before, v.facility);
    const auto new_values = sim::startup_world_facility_values(after, v.facility);
    require(old_values && new_values, "investment target lost its economy projection");
    for (std::size_t slot = 0; slot < v.visible_deltas.size(); ++slot)
        require(new_values->instance_attributes[slot] - old_values->instance_attributes[slot] ==
                    v.visible_deltas[slot],
                "committed price/quality/charm differs from forecast");
    if (v.amenities)
        require(reward_only_ && !v.purchase && v.cost == 0 && v.monthly_gain == 0 &&
                    (v.visible_deltas[1] > 0 || v.visible_deltas[2] > 0),
                "quality/charm fallback must be a free effective improvement, not cash income");
    improved_ = true;
    std::cout << "INVEST improved facility=" << v.facility << " item=" << v.item
              << " actual_price_delta=" << delta << " actual_quality_delta=" << v.visible_deltas[1]
              << " actual_charm_delta=" << v.visible_deltas[2] << " uses=" << uses_ << std::endl;
}

void ActiveIncomeStrategy::encode(std::ostream &out) const {
    require(!choice_, "cannot save an active investment flow");
    out << "ACTIVE_INCOME_1 " << stop_month_ << ' ' << reward_only_ << ' ' << uses_ << ' ' << spent_
        << ' ' << new_shops_.size() << '\n';
    for (const auto id : new_shops_)
        out << id << '\n';
    require(bool(out), "failed to encode controller");
}
ActiveIncomeStrategy ActiveIncomeStrategy::decode(std::istream &in) {
    std::string magic;
    int stop{}, reward{}, uses{}, count{};
    std::int64_t spent{};
    require(bool(in >> magic >> stop >> reward >> uses >> spent >> count) &&
                magic == "ACTIVE_INCOME_1" && stop >= 0 && (reward == 0 || reward == 1) &&
                uses >= 0 && spent >= 0 && count >= 0 && count <= 8192 &&
                (reward ? spent == 0 : spent <= 10000 && uses <= 24),
            "invalid saved income controller");
    ActiveIncomeStrategy result(stop, reward != 0);
    result.uses_ = uses;
    result.spent_ = spent;
    for (int i = 0; i < count; ++i) {
        std::uint64_t id{};
        require(bool(in >> id) && id > 0 && result.new_shops_.insert(id).second,
                "invalid saved introduction identity");
    }
    return result; // next() validates each saved instance against the restored Owner.
}

void income_strategy_contract() {
    const auto s = initial_world();
    require(!choose_income_investment(s, -1), "negative budget must never spend inventory or cash");
    require(!choose_income_investment(s, 10000),
            "no observed full-month trade cannot justify a forecast");
    // The source reset copies item.s into held inventory (FACILITY_EFFECTS.md,
    // items). Quality/charm fallback can legitimately use that initial stock;
    // only the price experiment requires observed monthly business history.
    const auto initial_reward = choose_income_investment(s, 10000, {}, true);
    require(initial_reward && s.items.at(initial_reward->item).inventory > 0 &&
                initial_reward->amenities && !initial_reward->purchase &&
                initial_reward->cost == 0 && initial_reward->monthly_gain == 0,
            "initial held inventory may improve amenities without invented revenue");
    auto empty_inventory = s;
    for (auto &[id, item] : empty_inventory.items) {
        item.inventory = 0;
        empty_inventory.catalog.at({0, id}).inventory = 0;
    }
    require(!choose_income_investment(empty_inventory, 10000, {}, true),
            "empty inventory cannot be replenished by the reward-only selector");
    // Source item0 has price/quality/charm increments 0/2/0. This isolated
    // selection fixture gives one held unit, without inventing past business.
    auto quality = empty_inventory;
    quality.items.at(0).inventory = 1;
    quality.catalog.at({0, 0}).inventory = 1;
    const auto quality_choice = choose_income_investment(quality, 0, {}, true);
    require(quality_choice && quality_choice->item == 0 && quality_choice->amenities &&
                quality_choice->cost == 0 && !quality_choice->purchase &&
                quality_choice->price_delta == 0 && quality_choice->visible_deltas[1] > 0 &&
                quality_choice->monthly_gain == 0,
            "held quality-only improvement must be usable without invented income");
    require(!choose_income_investment(quality, 0),
            "legacy price-only experiment must keep its original decisions");
    for (auto &[definition, facility] : quality.scripts.facilities) {
        (void)definition;
        facility.improvements[1] =
            1000000; // Deliberately saturated fixture, not a campaign mutation.
    }
    require(!choose_income_investment(quality, 0, {}, true),
            "fully capped quality-only inventory must not be wasted");
    std::istringstream source("ACTIVE_INCOME_1 240 1 38 0 2\n123\n456\n");
    auto restored = ActiveIncomeStrategy::decode(source);
    require(restored.reward_only() && restored.uses() == 38 && restored.spent() == 0,
            "reward-only history may exceed the old experiment cap");
    restored.next(s, 0, false); // Saved instance IDs need not survive relocation.
    std::ostringstream saved;
    restored.encode(saved);
    require(saved.str() == "ACTIVE_INCOME_1 240 1 38 0 0\n",
            "removed instances must not survive controller reconciliation");
    bool rejected{};
    try {
        std::istringstream invalid("ACTIVE_INCOME_1 240 1 1 500 0\n");
        (void)ActiveIncomeStrategy::decode(invalid);
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    require(rejected, "reward-only controller cannot contain paid item purchases");
}
} // namespace ark::test
