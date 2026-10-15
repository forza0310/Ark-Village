#include "world_steam_magic_pot.hpp"
#include "../support/world_fixture.hpp"
#include "world_steam_people.hpp"
#include "world_steam_pot_operations.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace ref = sim::rules;
using State = app::WorldState;
using Kind = app::WorldCommandKind;
using Pot = sim::StartupMagicPotAction;
using Commerce = sim::StartupCommerceAction;
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(std::string("Pot equipment: ") + message);
}
int month(const State &s) { return s.scene.calendar.year * 12 + s.scene.calendar.month; }
std::int64_t cash(const State &s) { return s.scene.world.world.ai.accounting.funds(); }
const ref::WorldScriptPage &top(const State &s) {
    for (auto p = s.scripts.pages.rbegin(); p != s.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4)
            return *p;
    throw std::runtime_error("Pot equipment lost its page");
}
const ref::WorldMagicPotRecipeDefinition &source_recipe(const State &s, int id) {
    const auto r =
        std::find_if(s.rules->magic_pot_recipes.begin(), s.rules->magic_pot_recipes.end(),
                     [&](const auto &v) { return v.identity == id; });
    require(r != s.rules->magic_pot_recipes.end(), "missing source recipe");
    return *r;
}
const sim::StartupWorldItem &item(const State &s, int id) {
    const auto i = std::find_if(s.rules->items.begin(), s.rules->items.end(),
                                [&](const auto &v) { return v.identity == id; });
    require(i != s.rules->items.end(), "missing source material");
    return *i;
}
int deficit(const State &s, const ref::WorldMagicPotRecipeDefinition &r) {
    int total{};
    for (int n = 0; n < 4; ++n)
        total += std::max(0, r.costs[n] - s.legacy_n[3 + n]);
    return total;
}
int contribution(const State &s, const ref::WorldMagicPotRecipeDefinition &r,
                 const sim::StartupWorldItem &i, int experience_goal) {
    if (i.recovery > 0 || i.effect == 9)
        return 0; // Keep medicine and books for their consumers.
    if (experience_goal > 0) {
        // Pieces advance knowledge; preserve their source half-elements as well.
        // The buyer compares pending half-elements per gold, not just piece price.
        const int pending_goal = std::max(1, experience_goal - s.legacy_n[0]);
        if (s.legacy_n[1] >= pending_goal)
            return 0;
        return std::max(
            0, std::accumulate(i.magic_pot_elements.begin(), i.magic_pot_elements.end(), 0,
                               [](int total, int element) { return total + element / 2; }));
    }
    int value{};
    for (int n = 0; n < 4; ++n)
        value += std::min(std::max(0, r.costs[n] - s.legacy_n[3 + n]), i.magic_pot_elements[n]);
    return value;
}
int material(const State &s, const ref::WorldMagicPotRecipeDefinition &r,
             const std::vector<int> &entries, bool purchase, std::int64_t budget,
             int experience_goal = 0) {
    int best = -1, best_value{}, best_cost{};
    for (int id : entries) {
        const auto &i = item(s, id);
        const auto value = contribution(s, r, i, experience_goal);
        const int cost = purchase ? i.commerce_price : 0;
        if (value <= 0 || (purchase ? s.shop_item_stock.at(id).quantity <= 0 || cost > budget
                                    : s.items.at(id).inventory <= 0))
            continue;
        if (best < 0 ||
            (purchase ? std::int64_t(value) * best_cost > std::int64_t(best_value) * cost
                      : value > best_value)) {
            best = id;
            best_value = value;
            best_cost = cost;
        }
    }
    return best;
}
std::vector<int> item_ids(const State &s) {
    std::vector<int> ids;
    for (const auto &i : s.rules->items)
        ids.push_back(i.identity);
    return ids;
}
app::WorldCommand command(Kind kind, std::uint64_t page = 0) {
    app::WorldCommand c;
    c.kind = kind;
    c.page = page;
    return c;
}
IncomeDecision pot(std::uint64_t page, Pot action, int selection = 0) {
    auto c = command(Kind::magic_pot_action, page);
    c.magic_pot_action = action;
    c.selection = selection;
    return {true, c};
}
IncomeDecision commerce(std::uint64_t page, Commerce action, int selection = 0) {
    auto c = command(Kind::commerce_action, page);
    c.commerce_action = action;
    c.selection = selection;
    return {true, c};
}
bool reward_attributes_applied(const State &s, int human, int kind, int definition, int slot) {
    const auto source =
        std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(),
                     [&](const auto &e) { return e.shop.kind == kind && e.shop.id == definition; });
    const auto details = sim::startup_world_human_details(s, human);
    return source != s.rules->equipment.end() && details &&
           s.scene.world.world.ai.growth.at(human).definition.equipment[slot] ==
               source->shop.combat &&
           (!details->live_actor ||
            s.scene.world.world.ai.battle.actors.at(*details->live_actor).capacity ==
                details->combat[0]);
}
std::optional<int> next_equipment_recipe(const State &s) {
    std::optional<int> selected;
    int best = std::numeric_limits<int>::max();
    for (const auto &r : s.rules->magic_pot_recipes) {
        if (!(r.flags & 2U) || r.reward_kind < 1 || r.reward_kind > 3 ||
            s.magic_pot_recipes.at(r.identity).status != 1 ||
            s.catalog.at({r.reward_kind, r.reward_definition}).status != 0 ||
            !steam_equipment_reward_usable(s, r.reward_kind, r.reward_definition))
            continue;
        const auto missing = deficit(s, r);
        if (missing < best) {
            selected = r.identity;
            best = missing;
        }
    }
    return selected;
}
} // namespace

void SteamPotEquipmentPlayer::reconcile(const State &s) {
    require(s.rules && s.rank >= 2 && (s.scripts.user_flags & 1U),
            "requires the real imported pot");
    if (recipe_ < 0) {
        const auto selected = next_equipment_recipe(s);
        require(selected.has_value(), "no known unowned equipment recipe with a useful recipient");
        recipe_ = *selected;
        const auto &r = source_recipe(s, recipe_);
        kind_ = r.reward_kind;
        definition_ = r.reward_definition;
        for (const auto &e : s.rules->equipment)
            if (e.shop.kind == kind_ && e.shop.id == definition_)
                slot_ = kind_ == 1 ? 0 : kind_ == 3 ? 3 : e.shop.type == 2 ? 1 : 2;
        initial_month_ = month(s);
        initial_experience_ = s.legacy_n[0];
    }
    const auto &r = source_recipe(s, recipe_);
    require((r.flags & 2U) && r.reward_kind == kind_ && r.reward_definition == definition_ &&
                s.magic_pot_recipes.at(recipe_).status == 1,
            "recipe identity/knowledge changed");
    const auto equipment =
        std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(), [&](const auto &e) {
            return e.shop.kind == kind_ && e.shop.id == definition_;
        });
    require(equipment != s.rules->equipment.end(), "recipe reward has no source equipment");
    const int source_slot = kind_ == 1 ? 0 : kind_ == 3 ? 3 : equipment->shop.type == 2 ? 1 : 2;
    require(slot_ == source_slot, "recorded equipment slot differs from its source type");
    const auto &reward = s.catalog.at({kind_, definition_});
    require(crafted_ ? reward.status == 1 && reward.free_purchases == (equipped_ ? 0 : 1)
                     : reward.status == 0,
            "reward no longer supports recorded pot origin");
    if (crafted_)
        require(paid_ == r.costs, "recorded elements differ from source costs");
    require(next_market_step_ <= s.simulation_steps + 300,
            "market retry lies beyond its recorded player budget");
    leaving_market_ = false;
}

IncomeDecision next_steam_pot_command(const State &s, const ref::WorldMagicPotRecipeDefinition &r,
                                      bool crafted, std::int64_t budget,
                                      std::uint64_t &next_market_step, bool &leaving_market,
                                      int experience_goal) {
    const auto &p = top(s);
    if (p.lifecycle == 0)
        return {true, {}};
    const bool discovery = experience_goal > 0;
    const bool done = crafted || (discovery && s.magic_pot_recipes.at(r.identity).status == 1);
    const bool ready_to_craft = !discovery && deficit(s, r) == 0;
    const bool enough_pending =
        discovery && s.legacy_n[1] >= std::max(1, experience_goal - s.legacy_n[0]);
    const auto held_material = [&] {
        return material(s, r, item_ids(s), false, 0, experience_goal);
    };
    if (p.kind == ref::WorldScriptPageKind::scene) {
        if (s.scene.scene_state != 0)
            return {};
        if (done)
            return {}; // Existing People owns fairness, payment and equipment effects.
        if (ready_to_craft)
            return {true, command(Kind::open_magic_pot)};
        const auto capacity = ref::world_magic_pot_capacity(s.legacy_n[11]);
        require(capacity.has_value(), "invalid source pot capacity");
        if (s.legacy_n[1] >= *capacity || enough_pending || (discovery && s.legacy_n[1] > 0)) {
            const int week = month(s) * 4 + s.scene.calendar.subperiod;
            if (week > s.legacy_n[12])
                return {true, command(Kind::open_magic_pot)};
            if (s.legacy_n[1] >= *capacity || enough_pending)
                return {true, {}};
        }
        if (held_material() >= 0)
            return {true, command(Kind::open_magic_pot)};
        // An empty/unaffordable84 visit must return to real world updates so
        // income/restocking can recover. This is a player retry, not a source timer.
        if (s.simulation_steps < next_market_step)
            return {true, {}};
        next_market_step = s.simulation_steps + 300;
        leaving_market = false;
        return {true, command(Kind::open_commerce)};
    }
    if (p.legacy_page >= 41 && p.legacy_page <= 47) {
        const auto v = sim::inspect_startup_world_magic_pot_page(s, p.id);
        if (!v)
            return {true, {}};
        if (v->raw == 41) {
            if (done)
                return pot(p.id, Pot::cancel);
            const auto capacity = ref::world_magic_pot_capacity(s.legacy_n[11]);
            require(capacity.has_value(), "invalid source pot capacity");
            if (!ready_to_craft &&
                (enough_pending || s.legacy_n[1] >= *capacity || held_material() < 0))
                return pot(p.id, Pot::cancel);
            const int choice = ready_to_craft ? 1 : 0;
            return pot(p.id, v->selection == choice ? Pot::confirm : Pot::select, choice);
        }
        if (v->raw == 42) {
            const int id = material(s, r, v->entries, false, 0, experience_goal);
            if (id < 0 || ready_to_craft || done)
                return pot(p.id, Pot::cancel);
            const int index =
                int(std::find(v->entries.begin(), v->entries.end(), id) - v->entries.begin());
            return pot(p.id, v->selection == index ? Pot::confirm : Pot::select, index);
        }
        if (v->raw == 43) {
            if (done || discovery)
                return pot(p.id, Pot::cancel);
            const auto found = std::find(v->entries.begin(), v->entries.end(), r.identity);
            require(found != v->entries.end(), "known recipe missing from initialized43");
            const int index = int(found - v->entries.begin());
            return pot(p.id, v->selection == index ? Pot::confirm : Pot::select, index);
        }
        if ((v->raw == 44 && v->counter < 36) ||
            ((v->raw == 46 || v->raw == 47) && v->counter <= 6) ||
            (v->raw == 45 && v->counter >= 77 && v->counter < 83))
            return {true, {}};
        return pot(p.id, Pot::confirm);
    }
    if (!done && (p.legacy_page == 83 || p.legacy_page == 84)) {
        const auto v = sim::inspect_startup_world_commerce_page(s, p.id);
        if (!v)
            return {true, {}};
        if (v->raw == 83) {
            if (leaving_market)
                return commerce(p.id, Commerce::cancel);
            if (v->selection != 0)
                return commerce(p.id, Commerce::select, 0);
            leaving_market = true;
            return commerce(p.id, Commerce::confirm);
        }
        const int id = material(s, r, v->entries, true, budget, experience_goal);
        if (held_material() >= 0 || ready_to_craft || enough_pending || id < 0)
            return commerce(p.id, Commerce::cancel);
        if (v->feedback_counter > 0)
            return {true, {}};
        const int index =
            int(std::find(v->entries.begin(), v->entries.end(), id) - v->entries.begin());
        return commerce(p.id, v->selection == index ? Commerce::confirm : Commerce::select, index);
    }
    return {}; // Original dialogs, gift/reward and tax consumers remain in the primary player.
}

IncomeDecision SteamPotEquipmentPlayer::next(const State &s, std::int64_t budget,
                                             bool wait_for_trade,
                                             const ActiveTradeEvidence &trade) {
    const auto &p = top(s);
    if (p.lifecycle == 0)
        return {true, {}};
    if (p.kind == ref::WorldScriptPageKind::scene && s.scene.scene_state == 0 && equipped_) {
        certify(s, trade);
        return {wait_for_trade && !complete(s, trade), {}};
    }
    return next_steam_pot_command(s, source_recipe(s, recipe_), crafted_, budget, next_market_step_,
                                  leaving_market_);
}

SteamPotCommit observe_steam_pot_command(const State &before, const app::WorldCommand &c,
                                         const app::WorldCommandResult &result, const State &after,
                                         const ref::WorldMagicPotRecipeDefinition &r,
                                         bool already_crafted,
                                         const CampaignReceiptSink &receipts) {
    require(result.outcome == app::WorldCommandOutcome::applied, "intervention command rejected");
    SteamPotCommit commit;
    const int raw = top(before).legacy_page;
    if (c.kind == Kind::open_magic_pot && after.legacy_n[0] > before.legacy_n[0]) {
        commit.processed = before.legacy_n[1] - after.legacy_n[1];
        require(commit.processed > 0 && after.legacy_n[2] == commit.processed &&
                    after.legacy_n[0] == std::min(999, before.legacy_n[0] + commit.processed) &&
                    cash(before) == cash(after),
                "processing must consume real pending pieces and preserve cash");
        std::cout << "POT_PROCESS experience=" << before.legacy_n[0] << "->" << after.legacy_n[0]
                  << " processed=" << commit.processed << " pending=" << after.legacy_n[1] << '\n';
    }
    if (raw == 46) {
        const int id = top(before).legacy_g;
        if (before.magic_pot_recipes.at(id).status == 0 &&
            after.magic_pot_recipes.at(id).status == 1) {
            const auto &discovered = source_recipe(before, id);
            require(c.kind == Kind::magic_pot_action && c.magic_pot_action == Pot::confirm &&
                        c.page == top(before).id && before.page_counters.at(c.page) > 6 &&
                        (discovered.flags & 2U) &&
                        before.legacy_n[0] >= discovered.experience_required &&
                        after.magic_pot_recipes.at(id).pending_notice &&
                        before.legacy_n == after.legacy_n && cash(before) == cash(after) &&
                        before.scene.random.draws() == after.scene.random.draws(),
                    "discovery lacks its bound source46 confirmation or changes resources");
            if (receipts)
                receipts({CampaignReceiptKind::recipe_discovered,
                          id,
                          after.simulation_steps,
                          c.page,
                          0,
                          -1,
                          {before.magic_pot_recipes.at(id).status, before.legacy_n[0],
                           before.magic_pot_recipes.at(id).pending_notice},
                          {after.magic_pot_recipes.at(id).status, after.legacy_n[0],
                           after.magic_pot_recipes.at(id).pending_notice}});
        }
    }
    if (!already_crafted && c.kind == Kind::commerce_action &&
        c.commerce_action == Commerce::confirm && raw == 84 && result.commerce_amount) {
        const auto v = sim::inspect_startup_world_commerce_page(before, c.page);
        require(v && v->mode == 0, "purchase lacks actual material catalogue");
        const int id = v->entries.at(v->selection), cost = item(before, id).commerce_price;
        require(after.items.at(id).inventory == before.items.at(id).inventory + 1 &&
                    after.shop_item_stock.at(id).quantity + 1 ==
                        before.shop_item_stock.at(id).quantity &&
                    cash(before) - cash(after) == cost && *result.commerce_amount == cost,
                "material purchase lost stock or its source cash quote");
        ++commit.purchases;
        if (receipts)
            receipts({CampaignReceiptKind::item_commerce_bought,
                      id,
                      after.simulation_steps,
                      c.page,
                      0,
                      -1,
                      {before.items.at(id).inventory, before.shop_item_stock.at(id).quantity,
                       cash(before)},
                      {after.items.at(id).inventory, after.shop_item_stock.at(id).quantity,
                       cash(after)}});
    }
    if (!already_crafted && c.kind == Kind::magic_pot_action &&
        c.magic_pot_action == Pot::confirm && raw == 42) {
        const auto v = sim::inspect_startup_world_magic_pot_page(before, c.page);
        require(v && !v->entries.empty(), "deposit lost source42 inventory");
        const int id = v->entries.at(v->selection);
        require(before.items.at(id).inventory == after.items.at(id).inventory + 1 &&
                    after.legacy_n[1] == before.legacy_n[1] + 1 && cash(before) == cash(after),
                "deposit did not consume one actual item");
        ++commit.deposits;
        std::cout << "POT_MATERIAL item=" << id << " inventory=" << before.items.at(id).inventory
                  << "->" << after.items.at(id).inventory << " pending=" << after.legacy_n[1]
                  << '\n';
    }
    if (!already_crafted && c.kind == Kind::magic_pot_action &&
        c.magic_pot_action == Pot::confirm && raw == 47) {
        const auto key = std::make_pair(r.reward_kind, r.reward_definition);
        require(top(before).legacy_g == r.identity &&
                    before.magic_pot_recipes.at(r.identity).status == 1 &&
                    cash(before) == cash(after) && before.legacy_n[0] == after.legacy_n[0],
                "craft must use its known recipe without cash or experience changes");
        if (r.reward_kind == 0) {
            require(before.items.at(r.reward_definition).inventory < 999 &&
                        after.items.at(r.reward_definition).inventory ==
                            before.items.at(r.reward_definition).inventory + 1 &&
                        after.catalog.at(key).inventory ==
                            after.items.at(r.reward_definition).inventory &&
                        after.items.at(r.reward_definition).status == 1,
                    "item craft must grant exactly one actual published reward");
        } else if (r.reward_kind == 4) {
            require(before.facility_presence.at(r.reward_definition) == 0 &&
                        after.facility_presence.at(r.reward_definition) == 0 &&
                        before.facility_free_builds == after.facility_free_builds &&
                        before.scene.world.world.facilities.size() ==
                            after.scene.world.world.facilities.size(),
                    "facility craft must queue93 without opening or constructing its reward");
        } else {
            require(before.catalog.at(key).status == 0 && after.catalog.at(key).status == 1 &&
                        before.catalog.at(key).free_purchases == 0 &&
                        after.catalog.at(key).free_purchases == 1,
                    "equipment craft must grant exactly one first free equipment");
        }
        for (int n = 0; n < 4; ++n) {
            commit.paid[n] = before.legacy_n[3 + n] - after.legacy_n[3 + n];
            require(commit.paid[n] == r.costs[n], "craft did not pay exact published elements");
        }
        std::uint64_t reward_page{};
        if (r.reward_kind == 4) {
            for (const auto &p : after.scripts.pages)
                if (p.legacy_page == 93 && p.legacy_r == 3 && p.legacy_s == r.reward_definition &&
                    std::none_of(before.scripts.pages.begin(), before.scripts.pages.end(),
                                 [&](const auto &old) { return old.id == p.id; })) {
                    require(!reward_page, "facility recipe queued multiple rewards");
                    reward_page = p.id;
                }
            require(reward_page != 0, "facility recipe paid without its bound93 request");
        }
        commit.crafted = true;
        commit.experience = before.legacy_n[0];
        if (receipts)
            receipts(
                {CampaignReceiptKind::recipe_crafted,
                 r.identity,
                 after.simulation_steps,
                 c.page,
                 std::uint64_t(r.reward_definition),
                 -1,
                 {before.legacy_n[3], before.legacy_n[4], before.legacy_n[5], before.legacy_n[6]},
                 {after.legacy_n[3], after.legacy_n[4], after.legacy_n[5], after.legacy_n[6]}});
        if (r.reward_kind == 4 && receipts)
            receipts({CampaignReceiptKind::facility_pot_queued,
                      r.reward_definition,
                      after.simulation_steps,
                      c.page,
                      reward_page,
                      -1,
                      {r.identity, before.facility_presence.at(r.reward_definition),
                       before.facility_free_builds.at(r.reward_definition), 0},
                      {r.identity, after.facility_presence.at(r.reward_definition),
                       after.facility_free_builds.at(r.reward_definition),
                       static_cast<std::int64_t>(reward_page)}});
        if (r.reward_kind == 0 && receipts)
            receipts({CampaignReceiptKind::item_obtained,
                      r.reward_definition,
                      after.simulation_steps,
                      c.page,
                      std::uint64_t(r.identity),
                      -1,
                      {before.items.at(r.reward_definition).inventory},
                      {after.items.at(r.reward_definition).inventory}});
    }
    return commit;
}

void SteamPotEquipmentPlayer::observe(const State &before, const app::WorldCommand &c,
                                      const app::WorldCommandResult &result, const State &after,
                                      const CampaignReceiptSink &receipts) {
    const bool already_crafted = crafted_ != 0;
    const auto commit = observe_steam_pot_command(
        before, c, result, after, source_recipe(before, recipe_), already_crafted, receipts);
    purchases_ += commit.purchases;
    deposits_ += commit.deposits;
    if (commit.crafted) {
        crafted_ = 1;
        paid_ = commit.paid;
        craft_experience_ = commit.experience;
    }
    // A grant belongs to47; only a subsequent1->0 gift is consumption.
    if (already_crafted)
        observe_equipment(before, after);
}

void SteamPotEquipmentPlayer::observe_equipment(const State &before, const State &after) {
    if (!crafted_ || equipped_)
        return;
    const auto key = std::make_pair(kind_, definition_);
    if (before.catalog.at(key).free_purchases == after.catalog.at(key).free_purchases)
        return;
    require(before.catalog.at(key).free_purchases == 1 &&
                after.catalog.at(key).free_purchases == 0 && cash(before) == cash(after),
            "recipe equipment must use its real free qualification");
    for (const auto &[human, h] : after.shop_humans)
        if (h.equipment[slot_] == definition_ &&
            before.shop_humans.at(human).equipment[slot_] != definition_) {
            require(human_ < 0, "one free equipment cannot be gifted to multiple humans");
            const auto details = sim::startup_world_human_details(after, human);
            require(details && details->live_actor, "equipped reward lost its live human");
            require(reward_attributes_applied(after, human, kind_, definition_, slot_),
                    "reward attributes/capacity not applied to its real recipient");
            human_ = human;
            actor_ = details->live_actor->value;
        }
    require(human_ >= 0, "free count changed without the bound equipment setter");
    equipped_ = 1;
    equipped_month_ = month(after);
    std::cout << "POT_EQUIPPED " << diagnose() << '\n';
}
void SteamPotEquipmentPlayer::observe_tick(const State &before, const State &after) {
    observe_equipment(before, after);
}
bool SteamPotEquipmentPlayer::complete(const State &s, const ActiveTradeEvidence &trade) const {
    return crafted_ && equipped_ && month(s) >= equipped_month_ + 2 &&
           trade.full_month_after(equipped_month_, month(s));
}
void SteamPotEquipmentPlayer::verify(const State &s, const ActiveTradeEvidence &trade) const {
    require(complete(s, trade) && s.catalog.at({kind_, definition_}).status == 1 &&
                s.catalog.at({kind_, definition_}).free_purchases == 0 &&
                s.shop_humans.at(human_).equipment[slot_] == definition_ &&
                paid_ == source_recipe(s, recipe_).costs &&
                reward_attributes_applied(s, human_, kind_, definition_, slot_),
            "cold endpoint lost equipment or full following trade");
}
void SteamPotEquipmentPlayer::certify(const State &s, const ActiveTradeEvidence &trade) {
    if (certified_) {
        require(complete(s, trade) && paid_ == source_recipe(s, recipe_).costs &&
                    s.catalog.at({kind_, definition_}).status == 1 &&
                    s.catalog.at({kind_, definition_}).free_purchases == 0,
                "certified history lost its source payment, reward or following trade");
    } else if (complete(s, trade)) {
        verify(s, trade); // Keep every original live-equipment/attribute assertion.
        certified_ = true;
    }
}
bool SteamPotEquipmentPlayer::begin_next(const State &s, const ActiveTradeEvidence &trade) {
    if (!complete(s, trade))
        return false;
    certify(s, trade);
    if (!next_equipment_recipe(s))
        return false;
    SteamPotEquipmentPlayer candidate;
    candidate.reconcile(s);
    *this = std::move(candidate);
    return true;
}
std::string SteamPotEquipmentPlayer::diagnose() const {
    std::ostringstream out;
    out << "recipe=" << recipe_ << " kind=" << kind_ << " reward=" << definition_
        << " slot=" << slot_ << " initial_exp=" << initial_experience_
        << " craft_exp=" << craft_experience_ << " purchases=" << purchases_
        << " deposits=" << deposits_ << " crafted=" << crafted_ << " equipped=" << equipped_
        << " human=" << human_ << " actor=" << actor_ << " equipped_month=" << equipped_month_
        << " certified=" << certified_;
    return out.str();
}
void SteamPotEquipmentPlayer::encode(std::ostream &out) const {
    out << "STEAM_POT_EQUIPMENT_2\n"
        << recipe_ << ' ' << kind_ << ' ' << definition_ << ' ' << slot_ << ' ' << initial_month_
        << ' ' << initial_experience_ << ' ' << purchases_ << ' ' << deposits_ << ' ' << crafted_
        << ' ' << equipped_ << ' ' << human_ << ' ' << actor_ << ' ' << equipped_month_ << ' '
        << craft_experience_ << ' ' << next_market_step_;
    for (int value : paid_)
        out << ' ' << value;
    out << ' ' << certified_ << '\n';
    require(bool(out), "equipment controller write failed");
}
SteamPotEquipmentPlayer SteamPotEquipmentPlayer::decode(std::istream &in) {
    SteamPotEquipmentPlayer v;
    std::string magic;
    in >> magic >> v.recipe_ >> v.kind_ >> v.definition_ >> v.slot_ >> v.initial_month_ >>
        v.initial_experience_ >> v.purchases_ >> v.deposits_ >> v.crafted_ >> v.equipped_ >>
        v.human_ >> v.actor_ >> v.equipped_month_ >> v.craft_experience_ >> v.next_market_step_;
    for (int &value : v.paid_)
        in >> value;
    int certified{};
    if (magic == "STEAM_POT_EQUIPMENT_2")
        in >> certified;
    require(bool(in) && (magic == "STEAM_POT_EQUIPMENT_1" || magic == "STEAM_POT_EQUIPMENT_2") &&
                certified >= 0 && certified <= 1 && (!certified || v.equipped_) && v.recipe_ >= 0 &&
                v.recipe_ < 40 && v.kind_ >= 1 && v.kind_ <= 3 && v.definition_ >= 0 &&
                v.slot_ >= 0 && v.slot_ < 4 && v.initial_month_ >= 0 &&
                v.initial_experience_ >= 0 && v.initial_experience_ <= 999 && v.purchases_ >= 0 &&
                v.deposits_ >= 0 && v.crafted_ >= 0 && v.crafted_ <= 1 && v.equipped_ >= 0 &&
                v.equipped_ <= v.crafted_ &&
                (v.equipped_ ? v.human_ >= 0 && v.human_ < 25 && v.actor_ &&
                                   v.equipped_month_ >= v.initial_month_
                             : v.human_ == -1 && !v.actor_ && v.equipped_month_ == -1) &&
                (v.crafted_ ? v.craft_experience_ >= 0 && v.craft_experience_ <= 999
                            : v.craft_experience_ == -1 && v.paid_ == std::array<int, 4>{}),
            "invalid equipment controller evidence");
    v.certified_ = certified != 0;
    in >> std::ws;
    require(in.eof(), "trailing equipment controller evidence");
    return v;
}

void steam_pot_equipment_contract() {
    auto s = initial_world();
    const auto arrival = sim::startup_world_runtime_adapter().arrival;
    for (int n = 0, count = s.arrival_counter;
         n < count && s.scene.world.world.ai.human_order.empty(); ++n) {
        auto next = arrival(s);
        require(next.has_value(), "fixture source arrival failed");
        s = std::move(*next);
    }
    // Explicit decision qualifications only; actual acquisition is a separate real-prefix run.
    s.rank = 2;
    s.scripts.user_flags |= 1U;
    s.legacy_n[0] = 9;
    s.legacy_n[3] = 9;
    s.legacy_n[4] = 25;
    s.legacy_n[5] = 6;
    s.legacy_n[6] = 1;
    for (auto &[key, entry] : s.catalog) {
        (void)key;
        entry.status = 0;
        entry.free_purchases = 0;
    }
    const auto random = s.scene.random.draws();
    const auto funds = cash(s);
    SteamPotEquipmentPlayer player;
    player.reconcile(s);
    require(player.recipe() == 22 && source_recipe(s, 22).experience_required == 10 &&
                s.legacy_n[0] == 9,
            "known equipment must not invent a discovery-experience crafting gate");
    std::ostringstream encoded;
    player.encode(encoded);
    std::istringstream input(encoded.str());
    auto copy = SteamPotEquipmentPlayer::decode(input);
    copy.reconcile(s);
    std::ostringstream again;
    copy.encode(again);
    require(encoded.str() == again.str() && !copy.complete(s, {}) && cash(s) == funds &&
                s.scene.random.draws() == random && s.catalog.at({2, 13}).status == 0,
            "prospective qualification must roundtrip without granting equipment or trade");
    require(!copy.begin_next(s, {}), "an unfinished recipe must retain its original controller");
    again.str({});
    copy.encode(again);
    require(again.str() == encoded.str(), "unfinished advancement changed the recorded objective");
    auto owned = s;
    owned.catalog.at({2, 13}).status = 1; // Selection fixture only, not observed crafting.
    SteamPotEquipmentPlayer successor;
    successor.reconcile(owned);
    const auto &next = source_recipe(owned, successor.recipe());
    require(successor.recipe() != 22 &&
                owned.catalog.at({next.reward_kind, next.reward_definition}).status == 0 &&
                (next.flags & 2U) && owned.magic_pot_recipes.at(next.identity).status == 1,
            "a successor must use a known unowned recipe, never recraft the recorded reward");
    std::vector<std::string> fields;
    std::istringstream tokens(encoded.str());
    for (std::string value; tokens >> value;)
        fields.push_back(value);
    auto legacy_fields = fields;
    legacy_fields.front() = "STEAM_POT_EQUIPMENT_1";
    legacy_fields.pop_back();
    std::ostringstream legacy_wire;
    for (const auto &value : legacy_fields)
        legacy_wire << value << ' ';
    std::istringstream legacy_input(legacy_wire.str());
    auto legacy = SteamPotEquipmentPlayer::decode(legacy_input);
    require(!legacy.certified(), "legacy evidence must not invent a historical certification");
    for (const auto &invalid : std::vector<std::pair<int, std::string>>{
             {2, "4"}, {4, "4"}, {4, "1"}, {9, "1"}, {10, "1"}, {20, "1"}, {20, "2"}}) {
        auto changed = fields;
        changed.at(invalid.first) = invalid.second;
        std::ostringstream wire;
        for (const auto &value : changed)
            wire << value << ' ';
        std::istringstream malformed(wire.str());
        bool refused{};
        try {
            auto invalid_player = SteamPotEquipmentPlayer::decode(malformed);
            invalid_player.reconcile(s);
        } catch (const std::runtime_error &) {
            refused = true;
        }
        require(refused, "invalid namespace/slot or unobserved craft/gift must be refused");
    }
    s.scripts.pages.resize(1);
    s.scripts.pages.front().lifecycle = 3;
    s.scene.scene_state = 0;
    const auto first = copy.next(s, 0, true, {});
    const auto retry = copy.next(s, 0, true, {});
    require(first.handled && first.command && first.command->kind == Kind::open_commerce &&
                retry.handled && !retry.command && s.scene.random.draws() == random,
            "unavailable materials must not reopen commerce without real world progress");
    s.legacy_n[1] = *ref::world_magic_pot_capacity(s.legacy_n[11]);
    s.legacy_n[12] = month(s) * 4 + s.scene.calendar.subperiod;
    const auto full = copy.next(s, 0, true, {});
    ++s.scene.calendar.subperiod; // Explicit next-week decision fixture, not business evidence.
    const auto process = copy.next(s, 0, true, {});
    require(full.handled && !full.command && process.handled && process.command &&
                process.command->kind == Kind::open_magic_pot,
            "a full pot must allow natural date progress before trying source processing");
    auto empty = initial_world(); // Initialized-page decision fixture, not an acquired item.
    empty.scripts.pages.resize(1);
    auto &empty_page = empty.scripts.pages.front();
    empty_page.kind = ref::WorldScriptPageKind::raw_page;
    empty_page.legacy_page = 41;
    empty_page.lifecycle = 3;
    empty.legacy_n[1] = 0;
    for (auto &[id, i] : empty.items) {
        i.inventory = 0;
        empty.catalog.at({0, id}).inventory = 0;
    }
    empty.magic_pot_pages_initialized.insert(empty_page.id);
    empty.magic_pot_page_data[empty_page.id] = {0, 0, -1};
    empty.magic_pot_page_lists[empty_page.id] = {};
    empty.page_counters[empty_page.id] = 0;
    empty.page_phases[empty_page.id] = 0;
    const auto leave_empty = copy.next(empty, 0, true, {});
    require(leave_empty.handled && leave_empty.command &&
                leave_empty.command->kind == Kind::magic_pot_action &&
                leave_empty.command->magic_pot_action == Pot::cancel,
            "a material deficit with empty inventory must exit41 instead of looping empty42");
    // Initialized purchase-page projection only. Fixed item0 costs400G for one
    // pending element; item25 costs1000G for63. Decisions neither buy nor draw.
    auto shopping = empty;
    auto &shop_page = shopping.scripts.pages.front();
    shop_page.legacy_page = 84;
    shop_page.legacy_f = 0;
    shopping.commerce_pages_initialized.insert(shop_page.id);
    shopping.commerce_page_data[shop_page.id] = {0, 0, 0, 0, -1, 0};
    shopping.commerce_page_lists[shop_page.id] = {0, 25};
    shopping.shop_item_stock.at(0).quantity = 1;
    shopping.shop_item_stock.at(25).quantity = 1;
    const auto shopping_random = shopping.scene.random.draws();
    for (const int budget : {1000, 999, 399}) {
        std::uint64_t retry_at{};
        bool leaving{};
        const auto chosen = next_steam_pot_command(shopping, source_recipe(shopping, 1), false,
                                                   budget, retry_at, leaving, 60);
        const auto expected = budget == 1000  ? Commerce::select
                              : budget == 999 ? Commerce::confirm
                                              : Commerce::cancel;
        if (!chosen.command || chosen.command->kind != Kind::commerce_action ||
            chosen.command->commerce_action != expected ||
            chosen.command->selection != (budget == 1000 ? 1 : 0))
            throw std::runtime_error(
                "Material choice budget=" + std::to_string(budget) + " view=" +
                std::to_string(
                    bool(sim::inspect_startup_world_commerce_page(shopping, shop_page.id))) +
                " action=" +
                std::to_string(chosen.command ? int(chosen.command->commerce_action) : -1) +
                " selection=" + std::to_string(chosen.command ? chosen.command->selection : -1));
    }
    require(shopping.scene.random.draws() == shopping_random &&
                shopping.shop_item_stock.at(25).quantity == 1 &&
                shopping.items.at(25).inventory == 0 && cash(shopping) == cash(empty),
            "material planning cannot buy, consume or change the world/random stream");
    // Partial observation fixture only; actual payment/grant/gift remains covered
    // independently by the real business-prefix acceptance.
    auto &p = s.scripts.pages.front();
    p.kind = ref::WorldScriptPageKind::raw_page;
    p.legacy_page = 47;
    p.legacy_g = 22;
    s.legacy_n[3] = 10;
    auto granted = s;
    for (int n = 0; n < 4; ++n)
        granted.legacy_n[3 + n] -= source_recipe(s, 22).costs[n];
    granted.catalog.at({2, 13}).status = 1;
    granted.catalog.at({2, 13}).free_purchases = 1;
    auto craft = command(Kind::magic_pot_action, p.id);
    craft.magic_pot_action = Pot::confirm;
    app::WorldCommandResult result;
    result.outcome = app::WorldCommandOutcome::applied;
    copy.observe(s, craft, result, granted);
    require(copy.diagnose().find("crafted=1 equipped=0") != std::string::npos,
            "a free grant must not pretend its equipment was already consumed");
    require(!copy.retained_slot(), "an unconsumed reward cannot retain a person's worn slot");
    // Qualification fixture for a historical endpoint, not a natural gift/trade
    // claim. The ordinary source gift and real following month are checked above
    // by separate suites and by the isolated business route.
    auto equipped = granted;
    const auto actor = equipped.scene.world.world.ai.human_order.front();
    const int human = equipped.scene.world.world.ai.battle.actors.at(actor).definition;
    const auto wear = [&](State &state, int definition) {
        const auto source = std::find_if(
            state.rules->equipment.begin(), state.rules->equipment.end(),
            [&](const auto &e) { return e.shop.kind == 2 && e.shop.id == definition; });
        require(source != state.rules->equipment.end(),
                "historical fixture lost its source armour");
        auto &growth = state.scene.world.world.ai.growth.at(human);
        growth.definition.equipment[2] = source->shop.combat;
        const auto derived =
            ref::derive_human_stats(growth.definition, state.scene.world.world.ai.professions);
        require(derived.candidate.has_value(), "historical fixture source stats failed");
        growth.derived = *derived.candidate;
        state.shop_humans.at(human).equipment[2] = definition;
        state.scene.world.world.ai.battle.actors.at(actor).capacity = growth.derived.combat[0];
    };
    wear(equipped, 13);
    equipped.catalog.at({2, 13}).free_purchases = 0;
    copy.observe(granted, craft, result, equipped);
    ActiveTradeEvidence trade;
    trade.revenue[month(equipped) + 1] = 100;
    copy.certify(equipped, trade);
    require(!copy.certified(), "a gift cannot archive an unfinished following month");
    const auto retained = copy.retained_slot();
    auto business = equipped; // Main-scene decision fixture, not a naturally drained gift page.
    business.scripts.pages.resize(1);
    business.scripts.pages.front().kind = ref::WorldScriptPageKind::scene;
    business.scripts.pages.front().legacy_page = 0;
    business.scripts.pages.front().lifecycle = 2;
    business.scene.scene_state = 0;
    require(retained && retained->human == human && retained->slot == 2 &&
                !copy.next(business, 0, false, trade).handled,
            "observed equipment permits primary business while only its worn slot is retained");
    equipped.scene.calendar.month += 2;
    copy.certify(equipped, trade);
    require(copy.certified(), "strict live endpoint must archive its successful checks");
    std::ostringstream certificate;
    copy.encode(certificate);
    std::istringstream archived_input(certificate.str());
    auto archived = SteamPotEquipmentPlayer::decode(archived_input);
    auto replaced = equipped;
    wear(replaced, 15);
    archived.reconcile(replaced);
    archived.certify(replaced, trade);
    bool live_refused{};
    try {
        archived.verify(replaced, trade);
    } catch (const std::runtime_error &) {
        live_refused = true;
    }
    require(live_refused && archived.certified(),
            "later replacement retains history but must not pass the original live endpoint");
    require(archived.begin_next(replaced, trade) && !archived.certified(),
            "an explicit new objective must reset the prior archived certification");
    // Partial observer fixtures: the source consumer has its own page/rollback
    // suite. A real-prefix rerun separately verifies acquisition and read-only observation.
    auto discovery = s;
    auto &notice = discovery.scripts.pages.front();
    notice.legacy_page = 46;
    notice.legacy_g = 15;
    discovery.page_counters[notice.id] = 7;
    discovery.legacy_n[0] = 14;
    auto found = discovery;
    found.magic_pot_recipes.at(15).status = 1;
    found.magic_pot_recipes.at(15).pending_notice = true;
    auto confirm = command(Kind::magic_pot_action, notice.id);
    confirm.magic_pot_action = Pot::confirm;
    std::vector<CampaignReceipt> discoveries;
    const CampaignReceiptSink collect = [&](const auto &receipt) {
        discoveries.push_back(receipt);
    };
    player.observe(discovery, confirm, result, found, collect);
    require(discoveries.size() == 1 &&
                discoveries.front().kind == CampaignReceiptKind::recipe_discovered &&
                discoveries.front().definition == 15 && discoveries.front().before[0] == 0 &&
                discoveries.front().after[0] == 1,
            "source46 must bind its actual discovered definition, not the current craft target");
    player.observe(found, confirm, result, found, collect);
    require(discoveries.size() == 1, "already-known recipes must not produce a discovery receipt");
    for (const int invalid : {0, 1, 2, 3}) {
        auto old = discovery;
        auto changed = found;
        if (invalid == 0)
            old.page_counters[notice.id] = 6;
        else if (invalid == 1)
            old.legacy_n[0] = changed.legacy_n[0] = 13;
        else if (invalid == 2)
            ++changed.legacy_n[3];
        else
            changed.magic_pot_recipes.at(15).pending_notice = false;
        bool refused{};
        try {
            player.observe(old, confirm, result, changed, collect);
        } catch (const std::runtime_error &) {
            refused = true;
        }
        require(refused && discoveries.size() == 1,
                "early/under-experience/resource-changing or missing-NEW discoveries must not "
                "publish success");
    }
}
} // namespace ark::test
