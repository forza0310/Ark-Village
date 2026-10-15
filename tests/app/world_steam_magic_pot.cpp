#include "world_steam_magic_pot.hpp"
#include "../support/world_fixture.hpp"
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
                 const sim::StartupWorldItem &i) {
    if (i.recovery > 0 || i.effect == 9)
        return 0; // Keep medicine and books for their consumers.
    int value{};
    for (int n = 0; n < 4; ++n)
        value += std::min(std::max(0, r.costs[n] - s.legacy_n[3 + n]), i.magic_pot_elements[n]);
    return value;
}
int material(const State &s, const ref::WorldMagicPotRecipeDefinition &r,
             const std::vector<int> &entries, bool purchase, std::int64_t budget) {
    int best = -1, best_value{}, best_cost{};
    for (int id : entries) {
        const auto &i = item(s, id);
        const auto value = contribution(s, r, i);
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

IncomeDecision SteamPotEquipmentPlayer::next(const State &s, std::int64_t budget,
                                             bool controller_ready,
                                             const ActiveTradeEvidence &trade) {
    const auto &p = top(s);
    if (p.lifecycle == 0)
        return {true, {}};
    const auto &r = source_recipe(s, recipe_);
    if (p.kind == ref::WorldScriptPageKind::scene) {
        if (s.scene.scene_state != 0)
            return {};
        if (equipped_)
            return {controller_ready && !complete(s, trade), {}};
        if (crafted_)
            return {}; // Existing People owns fairness, payment and equipment effects.
        if (deficit(s, r) == 0)
            return {true, command(Kind::open_magic_pot)};
        const auto capacity = ref::world_magic_pot_capacity(s.legacy_n[11]);
        require(capacity.has_value(), "invalid source pot capacity");
        if (s.legacy_n[1] >= *capacity) {
            const int week = month(s) * 4 + s.scene.calendar.subperiod;
            return {true, week > s.legacy_n[12]
                              ? std::optional<app::WorldCommand>{command(Kind::open_magic_pot)}
                              : std::nullopt};
        }
        if (material(s, r, item_ids(s), false, 0) >= 0)
            return {true, command(Kind::open_magic_pot)};
        // An empty/unaffordable84 visit must return to real world updates so
        // income/restocking can recover. This is a player retry, not a source timer.
        if (s.simulation_steps < next_market_step_)
            return {true, {}};
        next_market_step_ = s.simulation_steps + 300;
        leaving_market_ = false;
        return {true, command(Kind::open_commerce)};
    }
    if (p.legacy_page >= 41 && p.legacy_page <= 47) {
        const auto v = sim::inspect_startup_world_magic_pot_page(s, p.id);
        if (!v)
            return {true, {}};
        if (v->raw == 41) {
            if (crafted_)
                return pot(p.id, Pot::cancel);
            const auto capacity = ref::world_magic_pot_capacity(s.legacy_n[11]);
            require(capacity.has_value(), "invalid source pot capacity");
            if (deficit(s, r) > 0 &&
                (s.legacy_n[1] >= *capacity || material(s, r, item_ids(s), false, 0) < 0))
                return pot(p.id, Pot::cancel);
            const int choice = deficit(s, r) == 0 ? 1 : 0;
            return pot(p.id, v->selection == choice ? Pot::confirm : Pot::select, choice);
        }
        if (v->raw == 42) {
            const int id = material(s, r, v->entries, false, 0);
            if (id < 0 || deficit(s, r) == 0)
                return pot(p.id, Pot::cancel);
            const int index =
                int(std::find(v->entries.begin(), v->entries.end(), id) - v->entries.begin());
            return pot(p.id, v->selection == index ? Pot::confirm : Pot::select, index);
        }
        if (v->raw == 43) {
            if (crafted_)
                return pot(p.id, Pot::cancel);
            const auto found = std::find(v->entries.begin(), v->entries.end(), recipe_);
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
    if (!crafted_ && (p.legacy_page == 83 || p.legacy_page == 84)) {
        const auto v = sim::inspect_startup_world_commerce_page(s, p.id);
        if (!v)
            return {true, {}};
        if (v->raw == 83) {
            if (leaving_market_)
                return commerce(p.id, Commerce::cancel);
            if (v->selection != 0)
                return commerce(p.id, Commerce::select, 0);
            leaving_market_ = true;
            return commerce(p.id, Commerce::confirm);
        }
        const int id = material(s, r, v->entries, true, budget);
        if (material(s, r, item_ids(s), false, 0) >= 0 || deficit(s, r) == 0 || id < 0)
            return commerce(p.id, Commerce::cancel);
        if (v->feedback_counter > 0)
            return {true, {}};
        const int index =
            int(std::find(v->entries.begin(), v->entries.end(), id) - v->entries.begin());
        return commerce(p.id, v->selection == index ? Commerce::confirm : Commerce::select, index);
    }
    return {}; // Original dialogs, gift/reward and tax consumers remain in the primary player.
}

void SteamPotEquipmentPlayer::observe(const State &before, const app::WorldCommand &c,
                                      const app::WorldCommandResult &result, const State &after,
                                      const CampaignReceiptSink &receipts) {
    require(result.outcome == app::WorldCommandOutcome::applied, "intervention command rejected");
    const bool already_crafted = crafted_ != 0;
    const auto &r = source_recipe(before, recipe_);
    const int raw = top(before).legacy_page;
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
    if (!crafted_ && c.kind == Kind::commerce_action && c.commerce_action == Commerce::confirm &&
        raw == 84 && result.commerce_amount) {
        const auto v = sim::inspect_startup_world_commerce_page(before, c.page);
        require(v && v->mode == 0, "purchase lacks actual material catalogue");
        const int id = v->entries.at(v->selection), cost = item(before, id).commerce_price;
        require(after.items.at(id).inventory == before.items.at(id).inventory + 1 &&
                    after.shop_item_stock.at(id).quantity + 1 ==
                        before.shop_item_stock.at(id).quantity &&
                    cash(before) - cash(after) == cost && *result.commerce_amount == cost,
                "material purchase lost stock or its source cash quote");
        ++purchases_;
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
    if (!crafted_ && c.kind == Kind::magic_pot_action && c.magic_pot_action == Pot::confirm &&
        raw == 42) {
        const auto v = sim::inspect_startup_world_magic_pot_page(before, c.page);
        require(v && !v->entries.empty(), "deposit lost source42 inventory");
        const int id = v->entries.at(v->selection);
        require(before.items.at(id).inventory == after.items.at(id).inventory + 1 &&
                    after.legacy_n[1] == before.legacy_n[1] + 1 && cash(before) == cash(after),
                "deposit did not consume one actual item");
        ++deposits_;
        std::cout << "POT_MATERIAL item=" << id << " inventory=" << before.items.at(id).inventory
                  << "->" << after.items.at(id).inventory << " pending=" << after.legacy_n[1]
                  << '\n';
    }
    if (!crafted_ && c.kind == Kind::magic_pot_action && c.magic_pot_action == Pot::confirm &&
        raw == 47) {
        const auto key = std::make_pair(kind_, definition_);
        require(top(before).legacy_g == recipe_ &&
                    before.magic_pot_recipes.at(recipe_).status == 1 &&
                    before.catalog.at(key).status == 0 && after.catalog.at(key).status == 1 &&
                    before.catalog.at(key).free_purchases == 0 &&
                    after.catalog.at(key).free_purchases == 1 && cash(before) == cash(after) &&
                    before.legacy_n[0] == after.legacy_n[0],
                "craft must use known recipe and grant exactly one first free equipment");
        for (int n = 0; n < 4; ++n) {
            paid_[n] = before.legacy_n[3 + n] - after.legacy_n[3 + n];
            require(paid_[n] == r.costs[n], "craft did not pay exact published elements");
        }
        crafted_ = 1;
        craft_experience_ = before.legacy_n[0];
        if (receipts)
            receipts(
                {CampaignReceiptKind::recipe_crafted,
                 recipe_,
                 after.simulation_steps,
                 c.page,
                 std::uint64_t(definition_),
                 -1,
                 {before.legacy_n[3], before.legacy_n[4], before.legacy_n[5], before.legacy_n[6]},
                 {after.legacy_n[3], after.legacy_n[4], after.legacy_n[5], after.legacy_n[6]}});
    }
    // The first0->1 grant belongs to47; a later1->0 gift belongs to the primary
    // People consumer. Never classify the grant itself as equipment consumption.
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
bool SteamPotEquipmentPlayer::begin_next(const State &s, const ActiveTradeEvidence &trade) {
    if (!complete(s, trade))
        return false;
    verify(s, trade);
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
        << " human=" << human_ << " actor=" << actor_ << " equipped_month=" << equipped_month_;
    return out.str();
}
void SteamPotEquipmentPlayer::encode(std::ostream &out) const {
    out << "STEAM_POT_EQUIPMENT_1\n"
        << recipe_ << ' ' << kind_ << ' ' << definition_ << ' ' << slot_ << ' ' << initial_month_
        << ' ' << initial_experience_ << ' ' << purchases_ << ' ' << deposits_ << ' ' << crafted_
        << ' ' << equipped_ << ' ' << human_ << ' ' << actor_ << ' ' << equipped_month_ << ' '
        << craft_experience_ << ' ' << next_market_step_;
    for (int value : paid_)
        out << ' ' << value;
    out << '\n';
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
    require(bool(in) && magic == "STEAM_POT_EQUIPMENT_1" && v.recipe_ >= 0 && v.recipe_ < 40 &&
                v.kind_ >= 1 && v.kind_ <= 3 && v.definition_ >= 0 && v.slot_ >= 0 && v.slot_ < 4 &&
                v.initial_month_ >= 0 && v.initial_experience_ >= 0 &&
                v.initial_experience_ <= 999 && v.purchases_ >= 0 && v.deposits_ >= 0 &&
                v.crafted_ >= 0 && v.crafted_ <= 1 && v.equipped_ >= 0 &&
                v.equipped_ <= v.crafted_ &&
                (v.equipped_ ? v.human_ >= 0 && v.human_ < 25 && v.actor_ &&
                                   v.equipped_month_ >= v.initial_month_
                             : v.human_ == -1 && !v.actor_ && v.equipped_month_ == -1) &&
                (v.crafted_ ? v.craft_experience_ >= 0 && v.craft_experience_ <= 999
                            : v.craft_experience_ == -1 && v.paid_ == std::array<int, 4>{}),
            "invalid equipment controller evidence");
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
    for (const auto &invalid : std::vector<std::pair<int, std::string>>{
             {2, "4"}, {4, "4"}, {4, "1"}, {9, "1"}, {10, "1"}}) {
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
