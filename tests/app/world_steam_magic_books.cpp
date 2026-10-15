#include "world_steam_magic_books.hpp"
#include "../support/world_fixture.hpp"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace ref = sim::rules;
using State = app::WorldState;
using Kind = app::WorldCommandKind;
using Human = sim::StartupHumanPageAction;
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(std::string("Pot spell book: ") + message);
}
const ref::WorldScriptPage &top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                [](const auto &v) { return v.lifecycle != 4; });
    require(p != s.scripts.pages.rend(), "missing live page");
    return *p;
}
const ref::WorldMagicPotRecipeDefinition &recipe(const State &s, int id) {
    const auto r =
        std::find_if(s.rules->magic_pot_recipes.begin(), s.rules->magic_pot_recipes.end(),
                     [&](const auto &v) { return v.identity == id; });
    require(r != s.rules->magic_pot_recipes.end(), "missing source recipe");
    return *r;
}
std::optional<int> recipient(const State &s, int spell) {
    std::optional<int> best;
    const auto score = [&](int human) {
        const auto &definition = s.scene.world.world.ai.growth.at(human).definition;
        const int learned =
            std::count(definition.learned_spells.begin(), definition.learned_spells.end(), true);
        const auto spent = s.scene.world.world.human_spending.find(human);
        return std::make_tuple(
            learned,
            -std::int64_t(spent == s.scene.world.world.human_spending.end() ? 0 : spent->second),
            human);
    };
    for (const auto &[human, presence] : s.human_presence) {
        if (!presence ||
            s.scene.world.world.ai.growth.at(human).definition.learned_spells.at(spell))
            continue;
        const auto details = sim::startup_world_human_details(s, human);
        if (!details || !details->live_actor ||
            s.scene.world.world.ai.battle.actors.at(*details->live_actor).control.state == 4)
            continue;
        if (!best || score(human) < score(*best))
            best = human;
    }
    return best;
}
IncomeDecision human_command(std::uint64_t page, Human action, int selection = 0) {
    app::WorldCommand c;
    c.kind = Kind::human_action;
    c.page = page;
    c.human_action = action;
    c.selection = selection;
    return {true, c};
}
} // namespace

void SteamPotSpellBookPlayer::reconcile(const State &s) {
    require(s.rules && s.rank >= 2 && (s.scripts.user_flags & 1U), "requires the real pot");
    if (recipe_ >= 0)
        return;
    std::optional<std::tuple<bool, int, int>> best;
    for (const auto &r : s.rules->magic_pot_recipes) {
        if (!(r.flags & 2U) || r.reward_kind != 0 || s.magic_pot_recipes.at(r.identity).status != 1)
            continue;
        const auto item =
            std::find_if(s.rules->items.begin(), s.rules->items.end(),
                         [&](const auto &i) { return i.identity == r.reward_definition; });
        if (item == s.rules->items.end() || item->effect != 9 || item->spell < 0 ||
            item->spell >= 4 || s.items.at(item->identity).inventory != 0 ||
            !recipient(s, item->spell))
            continue;
        int missing{};
        for (int n = 0; n < 4; ++n)
            missing += std::max(0, r.costs[n] - s.legacy_n[3 + n]);
        // Cover a new book definition before repeating a cheap book for another
        // recipient. Definition status is a choice hint, not a crafting receipt.
        const auto priority = std::make_tuple(s.items.at(item->identity).status != 0,
                                              missing, r.identity);
        if (!best || priority < *best) {
            recipe_ = r.identity;
            item_ = item->identity;
            spell_ = item->spell;
            best = priority;
        }
    }
    require(recipe_ >= 0, "no known book recipe with an unlearned recipient and empty inventory");
}

IncomeDecision SteamPotSpellBookPlayer::next(const State &s, std::int64_t budget) {
    if (retired_)
        return {}; // The original player now owns all subsequent human pages.
    const auto &p = top(s);
    if (p.lifecycle == 0)
        return {true, {}};
    if (!crafted_ || (p.legacy_page >= 41 && p.legacy_page <= 47))
        return next_steam_pot_command(s, recipe(s, recipe_), crafted_, budget, next_market_step_,
                                      leaving_market_);
    if (p.kind == ref::WorldScriptPageKind::scene) {
        if (consumed_) {
            retired_ = true;
            return {};
        }
        if (s.scene.scene_state != 0)
            return {};
        const auto selected = recipient(s, spell_);
        require(selected.has_value(), "crafted book lost its unlearned recipient");
        human_ = *selected;
        const auto details = sim::startup_world_human_details(s, human_);
        actor_ = details->live_actor->value;
        app::WorldCommand c;
        c.kind = Kind::open_human;
        c.definition = human_;
        c.actor = *details->live_actor;
        return {true, c};
    }
    if (p.legacy_page != 60 && p.legacy_page != 64 && p.legacy_page != 66)
        return {};
    if (!sim::startup_world_human_page_ready(s, p.id))
        return {true, {}};
    require(s.page_human_bindings.at(p.id) == human_, "gift changed the recorded human");
    if (p.legacy_page == 60)
        return human_command(p.id, consumed_ ? Human::cancel : Human::gifts);
    if (p.legacy_page == 64) {
        if (consumed_)
            return human_command(p.id, Human::cancel);
        if (s.page_phases.at(p.id) != 4)
            return human_command(p.id, Human::equipment_slot, 4);
        const auto &list = s.equipment_page_catalogs.at(p.id)[4];
        const auto entry = std::find(list.begin(), list.end(), item_);
        require(entry != list.end(), "crafted book missing from actual fifth gift catalogue");
        const int index = int(entry - list.begin());
        return human_command(
            p.id, s.human_page_selections.at(p.id) == index ? Human::confirm : Human::select,
            index);
    }
    const int counter = s.page_counters.at(p.id);
    return counter < 40 || counter >= 85 ? human_command(p.id, Human::confirm)
                                         : IncomeDecision{true, {}};
}

void SteamPotSpellBookPlayer::observe(const State &before, const app::WorldCommand &c,
                                      const app::WorldCommandResult &result, const State &after,
                                      const CampaignReceiptSink &receipts) {
    if (retired_)
        return;
    const auto commit = observe_steam_pot_command(before, c, result, after, recipe(before, recipe_),
                                                  crafted_, receipts);
    purchases_ += commit.purchases;
    deposits_ += commit.deposits;
    if (commit.crafted) {
        crafted_ = true;
        paid_ = commit.paid;
        experience_ = commit.experience;
    }
    if (!crafted_ || consumed_ || c.kind != Kind::human_action ||
        c.human_action != Human::confirm || top(before).legacy_page != 64 ||
        before.page_phases.at(c.page) != 4)
        return;
    const auto &list = before.equipment_page_catalogs.at(c.page)[4];
    require(list.at(before.human_page_selections.at(c.page)) == item_ &&
                before.page_human_bindings.at(c.page) == human_,
            "actual book selection lost its binding");
    const auto &old = before.scene.world.world.ai.growth.at(human_);
    const auto &now = after.scene.world.world.ai.growth.at(human_);
    auto expected = old.definition.learned_spells;
    require(!expected[spell_], "already learned is not first learning evidence");
    expected[spell_] = true;
    require(before.items.at(item_).inventory == 1 && after.items.at(item_).inventory == 0 &&
                after.catalog.at({0, item_}).inventory == 0 &&
                now.definition.learned_spells == expected &&
                before.scene.world.world.ai.accounting.funds() ==
                    after.scene.world.world.ai.accounting.funds(),
            "book must consume actual inventory, teach its slot and preserve cash/other learning");
    consumed_ = true;
    if (receipts) {
        receipts({CampaignReceiptKind::item_consumed,
                  item_,
                  after.simulation_steps,
                  c.page,
                  actor_,
                  human_,
                  {1},
                  {0}});
        receipts({CampaignReceiptKind::item_spell_learned,
                  item_,
                  after.simulation_steps,
                  c.page,
                  actor_,
                  human_,
                  {1, 0, old.derived.available_spells[spell_], spell_},
                  {0, 1, now.derived.available_spells[spell_], spell_}});
    }
    // The source sets the permanent flag after its shared reward recalculation;
    // Q may remain stale here. A later recalculation/cold restore is independent.
    std::cout << "POT_BOOK " << diagnose()
              << " available_after64=" << now.derived.available_spells[spell_] << '\n';
}
bool SteamPotSpellBookPlayer::complete(const State &s) const {
    return crafted_ && consumed_ && retired_ && paid_ == recipe(s, recipe_).costs &&
           s.scene.world.world.ai.growth.at(human_).definition.learned_spells.at(spell_) &&
           top(s).kind == ref::WorldScriptPageKind::scene && s.scene.scene_state == 0;
}
std::string SteamPotSpellBookPlayer::diagnose() const {
    std::ostringstream out;
    out << "recipe=" << recipe_ << " item=" << item_ << " spell=" << spell_ << " human=" << human_
        << " actor=" << actor_ << " crafted=" << crafted_ << " learned=" << consumed_
        << " purchases=" << purchases_ << " deposits=" << deposits_ << " craft_exp=" << experience_;
    return out.str();
}

void steam_magic_book_contract() {
    auto s = initial_world();
    const auto arrival = sim::startup_world_runtime_adapter().arrival;
    for (int n = 0, count = s.arrival_counter;
         n < count && s.scene.world.world.ai.human_order.empty(); ++n) {
        auto next = arrival(s);
        require(next.has_value(), "source arrival failed for book fixture");
        s = std::move(*next);
    }
    // Eligibility and observer fixtures only. Real source64 and persistence are
    // separately exercised from a legal business prefix, without injected stock.
    s.rank = 2;
    s.scripts.user_flags |= 1U;
    s.magic_pot_recipes.at(10).status = 1;
    s.legacy_n[3] = 50;
    auto choices = s; // Selection qualification, not a naturally obtained recipe or reward.
    for (auto &[id, value] : choices.magic_pot_recipes)
        value.status = 0;
    for (const int id : {10, 11, 12})
        choices.magic_pot_recipes.at(id).status = 1;
    choices.legacy_n[4] = 50;
    for (const int id : {32, 33})
        choices.items.at(id).status = choices.catalog.at({0, id}).status = 1;
    choices.items.at(34).status = choices.catalog.at({0, 34}).status = 0;
    SteamPotSpellBookPlayer first_definition;
    first_definition.reconcile(choices);
    require(first_definition.diagnose().find("recipe=12 item=34 ") == 0,
            "an unobtained known book must precede cheaper repeats with valid recipients");
    choices.items.at(34).status = choices.catalog.at({0, 34}).status = 1;
    SteamPotSpellBookPlayer repeated_gift;
    repeated_gift.reconcile(choices);
    require(repeated_gift.diagnose().find("recipe=10 item=32 ") == 0,
            "after new definitions, repeated gifts retain source deficits and stable ID order");
    s.scripts.pages.resize(1);
    auto &p = s.scripts.pages.front();
    p.kind = ref::WorldScriptPageKind::raw_page;
    p.legacy_page = 47;
    p.lifecycle = 3;
    p.legacy_g = 10;
    SteamPotSpellBookPlayer player;
    player.reconcile(s);
    auto granted = s;
    granted.legacy_n[3] = 0;
    granted.items.at(32).inventory = granted.catalog.at({0, 32}).inventory = 1;
    granted.items.at(32).status = granted.catalog.at({0, 32}).status = 1;
    app::WorldCommand make;
    make.kind = Kind::magic_pot_action;
    make.page = p.id;
    make.magic_pot_action = sim::StartupMagicPotAction::confirm;
    app::WorldCommandResult result;
    result.outcome = app::WorldCommandOutcome::applied;
    std::vector<CampaignReceipt> receipts;
    const CampaignReceiptSink collect = [&](const auto &v) { receipts.push_back(v); };
    player.observe(s, make, result, granted, collect);
    require(receipts.size() == 2 && receipts[0].kind == CampaignReceiptKind::recipe_crafted &&
                receipts[1].kind == CampaignReceiptKind::item_obtained && !player.complete(granted),
            "manufacturing is not permanent learning");
    auto &scene = granted.scripts.pages.front();
    scene.kind = ref::WorldScriptPageKind::scene;
    scene.legacy_page = 0;
    granted.scene.scene_state = 0;
    const auto open = player.next(granted, 0);
    require(open.handled && open.command && open.command->kind == Kind::open_human,
            "crafted book must select an actual unlearned live recipient");
    const int human = open.command->definition;
    auto gift = granted;
    auto &parent = gift.scripts.pages.front();
    parent.kind = ref::WorldScriptPageKind::raw_page;
    parent.legacy_page = 64;
    gift.page_human_bindings[parent.id] = human;
    gift.page_phases[parent.id] = 4;
    gift.equipment_page_catalogs[parent.id][4] = {32};
    gift.human_page_selections[parent.id] = 0;
    auto learned = gift;
    learned.items.at(32).inventory = learned.catalog.at({0, 32}).inventory = 0;
    learned.scene.world.world.ai.growth.at(human).definition.learned_spells[0] = true;
    app::WorldCommand confirm;
    confirm.kind = Kind::human_action;
    confirm.page = parent.id;
    confirm.human_action = Human::confirm;
    const auto pending = player;
    player.observe(gift, confirm, result, learned, collect);
    require(receipts.size() == 4 &&
                receipts.back().kind == CampaignReceiptKind::item_spell_learned &&
                receipts.back().recipient == human &&
                learned.scene.world.world.ai.growth.at(human).derived.available_spells ==
                    gift.scene.world.world.ai.growth.at(human).derived.available_spells &&
                !player.complete(learned),
            "ordinary64 learns without inventing immediate Q refresh or skipping page retirement");
    learned.scripts.pages.front().kind = ref::WorldScriptPageKind::scene;
    learned.scripts.pages.front().legacy_page = 0;
    require(!player.next(learned, 0).handled,
            "finished book flow must return to the original player");
    require(player.complete(learned), "completed learning must allow the original stable world");
    auto other = learned;
    other.scripts.pages.front().kind = ref::WorldScriptPageKind::raw_page;
    other.scripts.pages.front().legacy_page = 60;
    other.page_human_bindings[other.scripts.pages.front().id] = human == 0 ? 1 : 0;
    require(!player.next(other, 0).handled,
            "a retired book player must not intercept another human opened by the primary player");
    for (int invalid = 0; invalid < 3; ++invalid) {
        auto after = learned;
        if (invalid == 0)
            after.items.at(32).inventory = after.catalog.at({0, 32}).inventory = 1;
        if (invalid == 1)
            after.scene.world.world.ai.growth.at(human).definition.learned_spells[0] = false;
        if (invalid == 2)
            after.scene.world.world.ai.growth.at(human).definition.learned_spells[1] = true;
        auto rejected = pending;
        bool refused{};
        try {
            rejected.observe(gift, confirm, result, after, collect);
        } catch (const std::runtime_error &) {
            refused = true;
        }
        require(refused && receipts.size() == 4,
                "missing inventory/learning or an unrelated learned bit must not publish success");
    }
}
} // namespace ark::test
