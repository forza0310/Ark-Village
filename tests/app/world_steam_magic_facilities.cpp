#include "world_steam_magic_facilities.hpp"
#include "../support/world_fixture.hpp"
#include "world_steam_strategy.hpp"
#include <algorithm>
#include <limits>
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
        throw std::runtime_error(std::string("Pot facility: ") + message);
}
const ref::WorldScriptPage &top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                [](const auto &page) { return page.lifecycle != 4; });
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
} // namespace

void SteamPotFacilityPlayer::reconcile(const State &s) {
    require(s.rules && s.rank >= 2 && (s.scripts.user_flags & 1U), "requires the real pot");
    if (recipe_ >= 0)
        return;
    int best = std::numeric_limits<int>::max();
    for (const auto &r : s.rules->magic_pot_recipes) {
        if (!(r.flags & 2U) || r.reward_kind != 4 ||
            s.magic_pot_recipes.at(r.identity).status != 1 ||
            s.facility_presence.at(r.reward_definition) != 0)
            continue;
        int missing{};
        for (int n = 0; n < 4; ++n)
            missing += std::max(0, r.costs[n] - s.legacy_n[3 + n]);
        if (missing < best) {
            recipe_ = r.identity;
            definition_ = r.reward_definition;
            best = missing;
        }
    }
    require(recipe_ >= 0, "no known unowned facility recipe in the actual catalogue");
}

IncomeDecision SteamPotFacilityPlayer::next(const State &s, std::int64_t budget) {
    if (retired_)
        return {};
    const auto &p = top(s);
    if (p.lifecycle == 0)
        return {true, {}};
    if (!crafted_ || (p.legacy_page >= 41 && p.legacy_page <= 47))
        return next_steam_pot_command(s, recipe(s, recipe_), crafted_, budget, next_market_step_,
                                      leaving_market_);
    if (p.legacy_page == 93) {
        const auto view = sim::inspect_startup_world_commerce_page(s, p.id);
        if (!view)
            return {true, {}};
        require(p.id == reward_page_ && p.legacy_r == 3 && view->binding == definition_,
                "claim changed the queued source reward");
        app::WorldCommand c;
        c.kind = Kind::commerce_action;
        c.page = p.id;
        c.commerce_action = sim::StartupCommerceAction::confirm;
        return {true, c}; // The first early source confirmation only advances to40.
    }
    if (p.kind == ref::WorldScriptPageKind::scene && s.scene.scene_state == 0) {
        require(claimed_, "cannot leave the pot flow without its actual93 claim");
        retired_ = true;
    }
    return {}; // The original player owns107/dialogues and subsequent construction.
}

void SteamPotFacilityPlayer::observe(const State &before, const app::WorldCommand &c,
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
        for (const auto &p : after.scripts.pages)
            if (p.legacy_page == 93 && p.legacy_r == 3 && p.legacy_s == definition_ &&
                std::none_of(before.scripts.pages.begin(), before.scripts.pages.end(),
                             [&](const auto &old) { return old.id == p.id; }))
                reward_page_ = p.id;
        require(reward_page_ != 0, "paid recipe lacks its queued93");
    }
    if (!crafted_ || c.kind != Kind::commerce_action ||
        c.commerce_action != sim::StartupCommerceAction::confirm || top(before).legacy_page != 93)
        return;
    const auto view = sim::inspect_startup_world_commerce_page(before, c.page);
    require(view && c.page == reward_page_ && view->binding == definition_,
            "claim lost its initialized recipe reward binding");
    if (view->counter < 40) {
        require(before.facility_presence == after.facility_presence &&
                    before.facility_free_builds == after.facility_free_builds,
                "early93 confirmation granted a blueprint");
        return;
    }
    require(!claimed_ && before.facility_presence.at(definition_) == 0 &&
                after.facility_presence.at(definition_) == 2 &&
                after.facility_free_builds.at(definition_) ==
                    std::min(99, before.facility_free_builds.at(definition_) + 1) &&
                before.legacy_n == after.legacy_n &&
                before.village_points == after.village_points &&
                before.scene.world.world.ai.accounting.funds() ==
                    after.scene.world.world.ai.accounting.funds() &&
                top(after).id != c.page,
            "claim must open the paid recipe once without another resource charge");
    claimed_ = true;
    if (receipts)
        receipts(facility_transaction_receipt(CampaignReceiptKind::facility_blueprint_claimed,
                                              definition_, before, after, c.page));
}
bool SteamPotFacilityPlayer::complete(const State &s) const {
    return crafted_ && claimed_ && retired_ && s.facility_presence.at(definition_) == 2 &&
           top(s).kind == ref::WorldScriptPageKind::scene && s.scene.scene_state == 0;
}
std::string SteamPotFacilityPlayer::diagnose() const {
    std::ostringstream out;
    out << "recipe=" << recipe_ << " facility=" << definition_ << " queued=" << crafted_
        << " claimed=" << claimed_ << " reward_page=" << reward_page_ << " purchases=" << purchases_
        << " deposits=" << deposits_;
    return out.str();
}

void steam_magic_facility_contract(const CampaignCommandConsumer &apply) {
    auto s = initial_world();
    // Menu/resources are explicit qualification fixtures, never a natural save.
    // Actual discovery, payment, claim and business use a separate legal prefix.
    s.rank = 2;
    s.scripts.user_flags |= 1U;
    s.magic_pot_recipes.at(35).status = 1;
    for (int n = 0; n < 4; ++n)
        s.legacy_n[3 + n] = recipe(s, 35).costs[n];
    SteamPotFacilityPlayer player;
    SteamVillageStrategy primary;
    player.reconcile(s);
    primary.reconcile(s);
    std::vector<CampaignReceipt> receipts;
    bool early{}, dialogue_before_claim{};
    std::optional<State> paid_before, paid_after, claim_before, claim_after;
    std::optional<SteamPotFacilityPlayer> before_payment, before_claim;
    app::WorldCommand payment, claim;
    app::WorldCommandResult payment_result, claim_result;
    for (int step = 0; step < 500 && !player.complete(s); ++step) {
        const auto input = player.next(s, 0);
        auto c = input.command;
        if (!c && !input.handled && top(s).kind == ref::WorldScriptPageKind::dialogue) {
            app::WorldCommand close;
            close.kind = Kind::acknowledge_page;
            close.page = top(s).id;
            c = close;
            if (paid_after && !claim_after)
                dialogue_before_claim = true;
        }
        if (c) {
            const auto old = s;
            const auto old_player = player;
            const int raw = top(s).legacy_page;
            const auto result = apply(s, *c);
            require(result.outcome == app::WorldCommandOutcome::applied,
                    "actual facility pot input rejected");
            primary.observe(old, *c, result, s); // External93 is not a chamber payment.
            player.observe(old, *c, result, s, [&](const auto &r) { receipts.push_back(r); });
            if (raw == 47 && s.legacy_n != old.legacy_n) {
                paid_before = old;
                paid_after = s;
                before_payment = old_player;
                payment = *c;
                payment_result = result;
                require(s.facility_presence.at(44) == 0 && s.facility_free_builds.at(44) == 0,
                        "paid recipe granted before93");
            }
            if (raw == 93) {
                if (old.page_counters.at(c->page) < 40)
                    early = true;
                else {
                    claim_before = old;
                    claim_after = s;
                    before_claim = old_player;
                    claim = *c;
                    claim_result = result;
                }
            }
        }
        if (!player.complete(s)) {
            auto update = sim::prepare_startup_world_runtime(s);
            require(update.candidate && sim::update_startup_world_render_cache(*update.candidate),
                    "facility fixture source update failed");
            s = std::move(*update.candidate);
        }
    }
    require(player.complete(s) && early && dialogue_before_claim && paid_before && paid_after &&
                claim_before && claim_after && before_payment && before_claim &&
                receipts.size() == 3 && receipts[0].kind == CampaignReceiptKind::recipe_crafted &&
                receipts[1].kind == CampaignReceiptKind::facility_pot_queued &&
                receipts[1].definition == 44 &&
                receipts[2].kind == CampaignReceiptKind::facility_blueprint_claimed &&
                s.facility_presence.at(44) == 2 && s.facility_free_builds.at(44) == 1,
            "facility requires separate cost, queued93, early confirmation and actual claim");
    for (int fault = 0; fault < 5; ++fault) {
        auto bad = *paid_after;
        if (fault == 0)
            bad.facility_presence.at(44) = 2;
        if (fault == 1)
            ++bad.facility_free_builds.at(44);
        if (fault == 2)
            ++bad.legacy_n[3];
        if (fault == 3)
            for (auto &p : bad.scripts.pages)
                if (p.legacy_page == 93)
                    p.legacy_s = 56;
        if (fault == 4)
            for (auto &p : bad.scripts.pages)
                if (p.legacy_page == 93)
                    p.legacy_r = 2;
        auto observer = *before_payment;
        std::vector<CampaignReceipt> rejected;
        bool refused{};
        try {
            observer.observe(*paid_before, payment, payment_result, bad,
                             [&](const auto &r) { rejected.push_back(r); });
        } catch (const std::exception &) {
            refused = true;
        }
        require(refused && rejected.empty(), "bad cost or premature/wrong reward certified");
    }
    for (int fault = 0; fault < 3; ++fault) {
        auto old = *claim_before, bad = *claim_after;
        if (fault == 0)
            old.page_counters.at(claim.page) = 39;
        if (fault == 1)
            ++bad.facility_free_builds.at(44);
        if (fault == 2)
            ++bad.village_points;
        auto observer = *before_claim;
        std::vector<CampaignReceipt> rejected;
        bool refused{};
        try {
            observer.observe(old, claim, claim_result, bad,
                             [&](const auto &r) { rejected.push_back(r); });
        } catch (const std::exception &) {
            refused = true;
        }
        require(refused && rejected.empty(), "early/double/charged claim certified");
    }
}
} // namespace ark::test
