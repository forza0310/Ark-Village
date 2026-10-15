#include "world_steam_magic_discovery.hpp"
#include "../support/world_fixture.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace ark::test {
namespace {
namespace ref = simulation::rules;
using State = app::WorldState;
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(std::string("Pot discovery: ") + message);
}
const ref::WorldMagicPotRecipeDefinition &recipe(const State &s, int id) {
    const auto found =
        std::find_if(s.rules->magic_pot_recipes.begin(), s.rules->magic_pot_recipes.end(),
                     [&](const auto &r) { return r.identity == id; });
    require(found != s.rules->magic_pot_recipes.end(), "missing source definition");
    return *found;
}
const ref::WorldScriptPage &top(const State &s) {
    const auto page = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                   [](const auto &p) { return p.lifecycle != 4; });
    require(page != s.scripts.pages.rend(), "missing live page");
    return *page;
}
} // namespace

void SteamPotDiscoveryPlayer::reconcile(const State &s) {
    require(s.rules && s.rank >= 2 && (s.scripts.user_flags & 1U), "requires the real pot");
    if (recipe_ >= 0)
        return;
    for (const auto &r : s.rules->magic_pot_recipes)
        if ((r.flags & 2U) && s.magic_pot_recipes.at(r.identity).status == 0 &&
            (recipe_ < 0 || r.experience_required < recipe(s, recipe_).experience_required))
            recipe_ = r.identity;
    require(recipe_ >= 0, "no undiscovered recipe with a source discovery path");
}

IncomeDecision SteamPotDiscoveryPlayer::next(const State &s, std::int64_t budget) {
    if (retired_)
        return {};
    const auto &r = recipe(s, recipe_);
    if (s.magic_pot_recipes.at(recipe_).status == 1 &&
        top(s).kind == ref::WorldScriptPageKind::scene && s.scene.scene_state == 0) {
        retired_ = true;
        return {}; // Actual46 committed; ordinary business resumes after retirement.
    }
    auto input = next_steam_pot_command(s, r, false, budget, next_market_step_, leaving_market_,
                                        r.experience_required);
    if (input.command && input.command->kind == app::WorldCommandKind::open_magic_pot &&
        top(s).kind == ref::WorldScriptPageKind::scene) {
        const int week =
            (s.scene.calendar.year * 12 + s.scene.calendar.month) * 4 + s.scene.calendar.subperiod;
        if (last_open_week_ == week)
            return {true, {}}; // Rounded-zero processing must allow actual calendar progress.
        last_open_week_ = week;
    }
    return input;
}

void SteamPotDiscoveryPlayer::observe(const State &before, const app::WorldCommand &command,
                                      const app::WorldCommandResult &result, const State &after,
                                      const CampaignReceiptSink &receipts) {
    if (retired_)
        return;
    const auto commit = observe_steam_pot_command(before, command, result, after,
                                                  recipe(before, recipe_), false, receipts);
    require(!commit.crafted, "discovery cannot masquerade as manufacturing");
    purchases_ += commit.purchases;
    deposits_ += commit.deposits;
    processed_ += commit.processed;
}

std::string SteamPotDiscoveryPlayer::diagnose(const State &s) const {
    std::ostringstream out;
    out << "recipe=" << recipe_ << " required=" << recipe(s, recipe_).experience_required
        << " experience=" << s.legacy_n[0] << " pending=" << s.legacy_n[1]
        << " discovered=" << s.magic_pot_recipes.at(recipe_).status << " purchases=" << purchases_
        << " deposits=" << deposits_ << " processed=" << processed_;
    return out.str();
}

void steam_magic_discovery_contract() {
    auto s = initial_world();
    // Decision/observer qualifications only; real discovery and cold pending
    // restoration use a legal business prefix in the explicit monthly route.
    s.rank = 2;
    s.scripts.user_flags |= 1U;
    for (int id : {7, 10, 15})
        s.magic_pot_recipes.at(id).status = 1;
    s.legacy_n[0] = 18;
    s.legacy_n[1] = 3;
    s.legacy_n[12] =
        (s.scene.calendar.year * 12 + s.scene.calendar.month) * 4 + s.scene.calendar.subperiod;
    s.scripts.pages.resize(1);
    s.scripts.pages.front().lifecycle = 3;
    s.scene.scene_state = 0;
    const auto random = s.scene.random.draws();
    SteamPotDiscoveryPlayer player;
    player.reconcile(s);
    const auto waiting = player.next(s, 0);
    require(waiting.handled && !waiting.command && s.scene.random.draws() == random,
            "enough pending pieces must wait for source date rather than reopen or craft");
    ++s.scene.calendar.subperiod;
    const auto process = player.next(s, 0);
    require(process.handled && process.command &&
                process.command->kind == app::WorldCommandKind::open_magic_pot,
            "a later source date must process real pending pieces");
    auto processed = s;
    processed.legacy_n[0] = 19;
    processed.legacy_n[1] = 2;
    processed.legacy_n[2] = 1;
    processed.legacy_n[12] = s.legacy_n[12] + 1;
    app::WorldCommandResult result;
    result.outcome = app::WorldCommandOutcome::applied;
    player.observe(s, *process.command, result, processed);
    auto malformed = processed;
    malformed.legacy_n[2] = 0;
    bool refused{};
    try {
        player.observe(s, *process.command, result, malformed);
    } catch (const std::runtime_error &) {
        refused = true;
    }
    require(refused, "processing without its exact consumed-piece count must be refused");
    // A cold process needs only the durable source pending count, not a sidecar.
    SteamPotDiscoveryPlayer cold;
    cold.reconcile(processed);
    const auto cold_wait = cold.next(processed, 0);
    require(cold_wait.handled && !cold_wait.command,
            "restored pending pieces must still wait for their real next source date");
    ++processed.scene.calendar.subperiod;
    const auto cold_process = cold.next(processed, 0);
    require(cold_process.command &&
                cold_process.command->kind == app::WorldCommandKind::open_magic_pot,
            "partial source pending work must remain actionable after reconstruction");
    require(!cold.next(processed, 0).command,
            "rounded-zero processing must not repeatedly reopen within the same source week");
    auto learned = processed;
    learned.magic_pot_recipes.at(25).status = 1;
    require(!cold.next(learned, 0).handled && !cold.active(),
            "committed discovery must return control to ordinary business");
    for (auto &[id, progress] : learned.magic_pot_recipes)
        if (id != 0 && id != 38)
            progress.status = 1;
    refused = false;
    try {
        SteamPotDiscoveryPlayer excluded;
        excluded.reconcile(learned);
    } catch (const std::runtime_error &) {
        refused = true;
    }
    require(refused && learned.magic_pot_recipes.at(0).status == 0 &&
                learned.magic_pot_recipes.at(38).status == 0,
            "dummy/no-discovery flags must not become player targets");
}
} // namespace ark::test
