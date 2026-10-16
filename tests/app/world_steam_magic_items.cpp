#include "world_steam_magic_items.hpp"
#include "../support/world_fixture.hpp"
#include "ark/simulation/facilities/startup_world_facility_items.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace ref = sim::rules;
using State = app::WorldState;
using Kind = app::WorldCommandKind;
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(std::string("Pot facility item: ") + message);
}
const ref::WorldScriptPage &top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                [](const auto &v) { return v.lifecycle != 4; });
    require(p != s.scripts.pages.rend(), "missing live page");
    return *p;
}
const ref::WorldMagicPotRecipeDefinition &recipe(const State &s, int id) {
    require(s.rules != nullptr, "missing source definitions");
    const auto r = std::find_if(s.rules->magic_pot_recipes.begin(), s.rules->magic_pot_recipes.end(),
                                [&](const auto &v) { return v.identity == id; });
    require(r != s.rules->magic_pot_recipes.end(), "missing source recipe");
    return *r;
}
bool stable(const State &s) {
    return top(s).kind == ref::WorldScriptPageKind::scene && s.scene.scene_state == 0;
}
} // namespace

void SteamPotItemPlayer::reconcile(const State &s, int id) {
    require(s.rank >= 2 && (s.scripts.user_flags & 1U), "requires the real pot");
    require(recipe_ < 0 || recipe_ == id, "cannot replace an unfinished objective");
    if (recipe_ >= 0)
        return;
    const auto &r = recipe(s, id);
    require((r.flags & 2U) && r.reward_kind == 0 &&
                s.magic_pot_recipes.at(id).status == 1,
            "requires a known ordinary item recipe");
    const auto item = std::find_if(s.rules->items.begin(), s.rules->items.end(),
                                   [&](const auto &v) { return v.identity == r.reward_definition; });
    require(item != s.rules->items.end() && item->effect >= 0 && item->effect < 8 &&
                std::any_of(item->facility_improvements.begin(), item->facility_improvements.end(),
                            [](int value) { return value > 0; }) &&
                s.items.at(item->identity).inventory < 999,
            "requires strengthening stock below the cap; medicines and books are separate");
    recipe_ = id;
    item_ = item->identity;
}

IncomeDecision SteamPotItemPlayer::next(const State &s, std::int64_t reserve) {
    if (retired_)
        return {};
    require(reserve >= 0, "negative operating reserve");
    const auto &r = recipe(s, recipe_);
    const auto &p = top(s);
    if (!crafted_ || (p.legacy_page >= 41 && p.legacy_page <= 47)) {
        const auto available = std::max<std::int64_t>(
            0, s.scene.world.world.ai.accounting.funds() - reserve);
        return next_steam_pot_command(s, r, crafted_, std::min<std::int64_t>(10000, available),
                                      next_market_step_, leaving_market_);
    }
    if (!armed_) {
        if (!stable(s))
            return {}; // Original notices/dialogues retire before the new use begins.
        require(choose_income_investment(s, 0, {}, true, item_).has_value(),
                "crafted stock needs a real nonnegative, positive facility effect");
        use_.set_item_target(s, item_, reserve);
        armed_ = true;
    }
    // Do not substitute a second paid purchase if the newly crafted stock is lost.
    require(use_.uses() || s.items.at(item_).inventory > 0, "crafted inventory disappeared before use");
    const auto selected = use_.next(s, reserve, false);
    if (!use_.active() && use_.uses() == 1 && stable(s))
        retired_ = true;
    return selected;
}

void SteamPotItemPlayer::observe(const State &before, const app::WorldCommand &c,
                                  const app::WorldCommandResult &result, const State &after,
                                  const CampaignReceiptSink &receipts) {
    if (retired_ || result.outcome != app::WorldCommandOutcome::applied)
        return; // A source refusal/cancel must not mark manufacture or use as complete.
    const auto commit = observe_steam_pot_command(before, c, result, after,
                                                  recipe(before, recipe_), crafted_, receipts);
    purchases_ += commit.purchases;
    deposits_ += commit.deposits;
    if (commit.crafted) {
        crafted_ = true;
        paid_ = commit.paid;
        experience_ = commit.experience;
    }
    if (armed_)
        use_.observe(before, c, after, receipts);
}
void SteamPotItemPlayer::observe_tick(const State &before, const State &after,
                                       const CampaignReceiptSink &receipts) {
    if (armed_ && !retired_)
        use_.observe_tick(before, after, receipts);
}
bool SteamPotItemPlayer::complete(const State &s) const {
    return crafted_ && armed_ && retired_ && !use_.active() && use_.uses() == 1 &&
           use_.spent() == 0 && paid_ == recipe(s, recipe_).costs && stable(s);
}
std::string SteamPotItemPlayer::diagnose() const {
    std::ostringstream out;
    out << "recipe=" << recipe_ << " item=" << item_ << " crafted=" << crafted_
        << " uses=" << use_.uses() << " use_pending=" << use_.active() << " retired=" << retired_
        << " purchases=" << purchases_ << " deposits=" << deposits_ << " craft_exp=" << experience_;
    return out.str();
}

void steam_magic_item_contract() {
    // Callsite fixtures exercise coordination. They do not certify natural acquisition.
    auto s = initial_world();
    s.rank = 2;
    s.scripts.user_flags |= 1U;
    s.magic_pot_recipes.at(2).status = s.magic_pot_recipes.at(5).status = 1;
    const auto rejected = [](const auto &action) {
        try { action(); } catch (const std::runtime_error &) { return true; }
        return false;
    };
    for (const int invalid : {-1, 0, 7, 10, 14, 40}) {
        auto known = s;
        if (known.magic_pot_recipes.count(invalid))
            known.magic_pot_recipes.at(invalid).status = 1;
        SteamPotItemPlayer player;
        require(rejected([&] { player.reconcile(known, invalid); }),
                "unknown/flags0/equipment/medicine/book recipe must reject");
    }
    for (int reason = 0; reason < 3; ++reason) {
        auto unavailable = s;
        auto rules = *s.rules;
        unavailable.rules = &rules;
        if (reason == 0)
            unavailable.magic_pot_recipes.at(2).status = 0;
        if (reason == 1)
            unavailable.items.at(11).inventory = unavailable.catalog.at({0, 11}).inventory = 999;
        if (reason == 2)
            rules.magic_pot_recipes.at(2).flags = 0;
        SteamPotItemPlayer player;
        require(rejected([&] { player.reconcile(unavailable, 2); }),
                "unopened/flagless/full-stock goals must reject before configuration");
    }
    SteamPotItemPlayer missing;
    missing.reconcile(s, 2);
    const auto digest = sim::startup_world_state_digest(s);
    const auto absent = missing.next(s, s.scene.world.world.ai.accounting.funds());
    require(absent.handled && !missing.complete(s) &&
                sim::startup_world_state_digest(s) == digest &&
                rejected([&] { missing.reconcile(s, 5); }),
            "missing materials must retain the same goal without changing Owner/random");

    // A frozen source84 row can remain affordable overall yet exceed this
    // player's per-material ceiling. Neither planning nor cancellation pays it.
    auto shop = s;
    auto prices = *s.rules;
    shop.rules = &prices;
    shop.scene.world.world.ai.accounting = ref::PeriodAccounting(30000);
    for (auto &[id, value] : shop.items)
        value.inventory = shop.catalog.at({0, id}).inventory = 0;
    shop.items.at(24).status = shop.catalog.at({0, 24}).status = 1;
    shop.shop_item_stock.at(24).quantity = 1;
    shop.scripts.pages.resize(1);
    auto &offer = shop.scripts.pages.front();
    offer.kind = ref::WorldScriptPageKind::raw_page;
    offer.legacy_page = 84;
    offer.legacy_f = 0;
    offer.lifecycle = 3;
    shop.page_counters[offer.id] = shop.page_phases[offer.id] = 0;
    shop.commerce_pages_initialized.insert(offer.id);
    shop.commerce_page_data[offer.id] = {0, 0, 0, 0, -1, 0};
    shop.commerce_page_lists[offer.id] = {24};
    for (const auto &terms : std::array<std::array<int, 3>, 4>{{
             {10000, 10000, 1}, {10001, 10000, 0}, {20000, 10000, 0}, {10000, 20001, 0}}}) {
        prices.items.at(24).commerce_price = terms[0]; // Explicit boundary fixture, not original data.
        const auto original = sim::startup_world_state_digest(shop);
        require(sim::inspect_startup_world_commerce_page(shop, offer.id).has_value(),
                "budget fixture must retain the source84 payload contract");
        SteamPotItemPlayer player;
        player.reconcile(shop, 2);
        const auto selected = player.next(shop, terms[1]);
        require(selected.command && selected.command->kind == Kind::commerce_action &&
                    selected.command->commerce_action ==
                        (terms[2] ? sim::StartupCommerceAction::confirm
                                  : sim::StartupCommerceAction::cancel) &&
                    sim::startup_world_state_digest(shop) == original && !player.complete(shop),
                "material purchases must satisfy both the ceiling and the operating reserve");
    }

    // Both approved ordinary rewards use the existing47 observer and75/76 consumers.
    for (const int id : {2, 5}) {
        auto before = s;
        const auto &r = recipe(before, id);
        const int reward = r.reward_definition;
        before.items.at(reward).inventory = before.catalog.at({0, reward}).inventory = 0;
        before.scripts.pages.resize(1);
        auto &page = before.scripts.pages.front();
        page.kind = ref::WorldScriptPageKind::raw_page;
        page.legacy_page = 47;
        page.legacy_g = id;
        page.lifecycle = 3;
        for (int n = 0; n < 4; ++n)
            before.legacy_n[3 + n] = r.costs[n];
        SteamPotItemPlayer player;
        player.reconcile(before, id);
        auto crafted = before;
        for (int n = 0; n < 4; ++n)
            crafted.legacy_n[3 + n] = 0;
        crafted.items.at(reward).inventory = crafted.catalog.at({0, reward}).inventory = 1;
        crafted.items.at(reward).status = crafted.catalog.at({0, reward}).status = 1;
        app::WorldCommand make;
        make.kind = Kind::magic_pot_action;
        make.page = page.id;
        make.magic_pot_action = sim::StartupMagicPotAction::confirm;
        app::WorldCommandResult result;
        result.outcome = app::WorldCommandOutcome::applied;
        std::vector<CampaignReceipt> receipts;
        const CampaignReceiptSink collect = [&](const auto &v) { receipts.push_back(v); };
        auto refusal = result;
        refusal.outcome = app::WorldCommandOutcome::rejected;
        const auto pending = player.diagnose();
        player.observe(before, make, refusal, before, collect);
        require(player.diagnose() == pending && receipts.empty(),
                "a source refusal must preserve pending work without receipts");
        player.observe(before, make, result, crafted, collect);
        require(receipts.size() == 2 && receipts[0].kind == CampaignReceiptKind::recipe_crafted &&
                    receipts[1].kind == CampaignReceiptKind::item_obtained && !player.complete(crafted),
                "manufacturing must pay once and cannot certify facility effects");
        player.observe(crafted, make, result, crafted, collect);
        require(receipts.size() == 2, "a repeated observed confirmation cannot grant twice");
        auto use = crafted; // Source page-retirement callsite, not a natural timeline.
        use.scripts.pages.front().kind = ref::WorldScriptPageKind::scene;
        use.scripts.pages.front().legacy_page = 0;
        use.scene.scene_state = 0;
        const auto reserve = use.scene.world.world.ai.accounting.funds();
        const auto open = player.next(use, reserve);
        require(open.command && open.command->kind == Kind::open_facility &&
                    !player.complete(use), "crafted stock must enter facility use without buying");
        require(sim::open_startup_world_facility_page(use, open.command->facility) ==
                    sim::StartupWorldRuntimeError::none &&
                    sim::open_startup_world_facility_items(use, use.scripts.pages.back().id) ==
                    sim::StartupWorldRuntimeError::none &&
                    sim::initialize_startup_world_facility_item_pages(use),
                "crafted stock must reach the source inventory page");
        use.scripts.pages.back().lifecycle = 2;
        const auto choose = player.next(use, reserve);
        require(choose.command.has_value(), "facility inventory needs a bound source input");
        if (choose.command->facility_item_action == sim::StartupFacilityItemAction::select) {
            require(sim::act_startup_world_facility_item_page(use, choose.command->page,
                        choose.command->facility_item_action, choose.command->selection) ==
                        sim::StartupWorldRuntimeError::none, "source item selection failed");
        }
        const auto confirm = player.next(use, reserve);
        require(confirm.command && confirm.command->facility_item_action ==
                    sim::StartupFacilityItemAction::confirm, "source use requires confirmation");
        const auto old = use;
        require(sim::act_startup_world_facility_item_page(use, confirm.command->page,
                    confirm.command->facility_item_action, confirm.command->selection) ==
                    sim::StartupWorldRuntimeError::none, "source75 consumption failed");
        player.observe(old, *confirm.command, result, use, collect);
        require(!player.complete(use), "consumption before76 is incomplete");
        const auto consumed = use;
        require(sim::initialize_startup_world_facility_item_pages(use), "source76 effect failed");
        player.observe_tick(consumed, use, collect);
        require(receipts.size() == 4 &&
                    receipts[2].kind == CampaignReceiptKind::item_facility_consumed &&
                    receipts[3].kind == CampaignReceiptKind::facility_item_improved &&
                    use.items.at(reward).inventory == 0 && !player.complete(use),
                "75/76 must consume once and verify actual effects before retirement");
        use.scripts.pages.resize(1); // Main-scene return fixture; live route must actually retire77.
        require(!player.next(use, reserve).handled && player.complete(use),
                "only completed use and return permits a checkpoint");
    }
}
} // namespace ark::test
