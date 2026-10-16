#include "world_steam_professions.hpp"
#include "ark/simulation/actors/rules/human_management.hpp"
#include "ark/simulation/actors/startup_world_profile.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
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
        throw std::runtime_error(std::string("Steam professions: ") + message);
}
const ref::WorldScriptPage *top(const State &s) {
    for (auto p = s.scripts.pages.rbegin(); p != s.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4)
            return &*p;
    return nullptr;
}
const ref::WorldScriptPage *page(const State &s, std::uint64_t id) {
    const auto p = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                [&](const auto &p) { return p.id == id; });
    return p == s.scripts.pages.end() ? nullptr : &*p;
}
app::WorldCommand input(std::uint64_t id, Human action, int selection = 0) {
    app::WorldCommand c;
    c.kind = Kind::human_action;
    c.page = id;
    c.human_action = action;
    c.selection = selection;
    return c;
}
std::vector<ref::HumanManagementProfession> catalogue(const State &s) {
    std::vector<ref::HumanManagementProfession> result;
    for (std::size_t n = 0; n < s.rules->jobs.size(); ++n) {
        const auto &j = s.rules->jobs[n];
        result.push_back({int(n), s.scripts.professions.at(int(n)).status, j.sort_order,
                          j.script_extra, j.change_points, j.required_medals});
    }
    return result;
}
bool affordable(const State &s, int human, int job, int reserved_points,
                 std::int64_t cash_reserve) {
    const auto &g = s.scene.world.world.ai.growth.at(human);
    const auto definitions = catalogue(s);
    const auto profile = sim::startup_world_human_profile(s, human);
    const auto allowed = profile ? ref::catalogue_human_professions(profile->sex, definitions)
                                 : std::nullopt;
    if (!allowed || std::find(allowed->begin(), allowed->end(), job) == allowed->end())
        return false;
    const auto &target = definitions.at(job);
    const auto gate = ref::check_human_profession_change(
        ref::HumanProfessionEntry::confirmation62, g.definition.current_profession,
        s.human_calendar.at(human).celebrations, s.village_points, target);
    if (gate.error != ref::HumanGrowthError::none || gate.denial != ref::HumanManagementDenial::none ||
        reserved_points < 0 || std::clamp(s.village_points - target.village_points, 0, 999) <
                                   reserved_points)
        return false;
    // Two-month endpoint buffer is the existing player's policy, not a duplicated fee formula.
    // Account for the target's future wages before opening a points-only transaction.
    const auto &old_fee = s.rules->jobs.at(g.definition.current_profession).fee;
    const auto &new_fee = s.rules->jobs.at(job).fee;
    const auto extra = std::max<std::int64_t>(
        0, 2LL * (std::max({0, new_fee[0], new_fee[1]}) -
                  std::max({0, old_fee[0], old_fee[1]})));
    return cash_reserve >= 0 && cash_reserve <= std::numeric_limits<std::int64_t>::max() - extra &&
           s.scene.world.world.ai.accounting.funds() >= cash_reserve + extra;
}
bool same_stats(const ref::HumanDerivedStats &a, const ref::HumanDerivedStats &b) {
    return a.growth == b.growth && a.attributes == b.attributes && a.combat == b.combat &&
           a.available_spells == b.available_spells;
}
} // namespace

void inspect_steam_professions(const State &s, int points, std::int64_t cash, std::ostream &out) {
    require(s.rules != nullptr, "inspection needs actual profession definitions");
    const auto definitions = catalogue(s);
    out << "PROFESSION_BUDGET points=" << s.village_points << " reserved_points=" << points
        << " cash_reserve=" << cash << " definitions=" << definitions.size() << '\n';
    for (const auto &job : definitions) {
        std::ostringstream eligible, funded, nonmaster;
        for (const auto &[human, present] : s.human_presence) {
            const auto info = present ? sim::startup_world_human_details(s, human) : std::nullopt;
            const auto profile = info ? sim::startup_world_human_profile(s, human) : std::nullopt;
            if (!info || !profile || !info->live_actor ||
                s.scene.world.world.ai.battle.actors.at(*info->live_actor).control.state == 4)
                continue;
            const auto choices = ref::catalogue_human_professions(profile->sex, definitions);
            const auto &g = s.scene.world.world.ai.growth.at(human).definition;
            const auto gate = ref::check_human_profession_change(
                ref::HumanProfessionEntry::confirmation62, g.current_profession,
                s.human_calendar.at(human).celebrations, s.village_points, job);
            if (!choices || std::find(choices->begin(), choices->end(), job.definition) == choices->end() ||
                gate.error != ref::HumanGrowthError::none ||
                gate.denial != ref::HumanManagementDenial::none)
                continue;
            eligible << human << ',';
            if (affordable(s, human, job.definition, points, cash)) {
                funded << human << ',';
                if (g.profession_levels.at(g.current_profession) != 10)
                    nonmaster << human << ',';
            }
        }
        out << "PROFESSION_DEFINITION definition=" << job.definition
            << " availability=" << job.availability << " quote=" << job.village_points
            << " medals=" << job.medals << " source_live=" << eligible.str()
            << " funded=" << funded.str() << " funded_nonmaster=" << nonmaster.str() << '\n';
    }
}

IncomeDecision SteamProfessionStrategy::next(const State &s, int reserved_points,
                                             std::int64_t cash_reserve, bool start) {
    const auto *p = top(s);
    if (!p || !s.rules || s.scene.framework_paused)
        return {};
    if (!flow_) {
        if (!start || p->kind != ref::WorldScriptPageKind::scene || s.scene.scene_state != 0 ||
            s.build_definition || s.build_mode)
            return {};
        const bool directed = target_pending(s);
        std::optional<std::tuple<bool, std::int64_t, int, int>> best;
        for (const auto &[human, presence] : s.human_presence) {
            if (target_ && (directed ? human != target_->first : human == target_->first))
                continue;
            const auto details = sim::startup_world_human_details(s, human);
            if (!presence || !details || !details->live_actor ||
                s.scene.world.world.ai.battle.actors.at(*details->live_actor).control.state == 4)
                continue;
            const auto &g = s.scene.world.world.ai.growth.at(human);
            if (!directed && g.definition.profession_levels.at(g.definition.current_profession) != 10)
                continue;
            const auto old = ref::derive_human_stats(g.definition, s.scene.world.world.ai.professions);
            const auto profile = sim::startup_world_human_profile(s, human);
            const auto choices = profile ? ref::catalogue_human_professions(profile->sex, catalogue(s))
                                         : std::nullopt;
            require(old.candidate && choices, "mastered human has invalid source stats or catalogue");
            for (const int job : *choices) {
                if ((directed ? job != target_->second : g.definition.profession_levels.at(job) >= 10) ||
                    !affordable(s, human, job, reserved_points, cash_reserve))
                    continue;
                const auto preview = ref::preview_human_profession_change(
                    g.definition, s.scene.world.world.ai.professions, job);
                auto target = g.definition;
                target.current_profession = job;
                const auto stats = ref::derive_human_stats(target, s.scene.world.world.ai.professions);
                require(preview && stats.candidate, "legal target lacks its source preview");
                const auto gain = std::accumulate(stats.candidate->combat.begin(),
                                                   stats.candidate->combat.end(), std::int64_t{}) -
                                  std::accumulate(old.candidate->combat.begin(),
                                                  old.candidate->combat.end(), std::int64_t{});
                bool new_spell{};
                for (std::size_t n = 0; n < 4; ++n)
                    new_spell |= !old.candidate->available_spells[n] &&
                                 stats.candidate->available_spells[n];
                if (!directed && gain <= 0 && !new_spell)
                    continue;
                // Source order breaks exact ties; neither ID, quote nor directory index is rank.
                const auto priority = std::make_tuple(!new_spell, -gain, human, job);
                if (!best || std::tie(std::get<0>(priority), std::get<1>(priority)) <
                                 std::tie(std::get<0>(*best), std::get<1>(*best))) {
                    best = priority;
                    flow_ = Flow{human, job, g.definition.current_profession,
                                 s.rules->jobs.at(job).change_points, *details->live_actor};
                }
            }
        }
        if (!flow_)
            return {};
        app::WorldCommand c;
        c.kind = Kind::open_human;
        c.actor = flow_->actor;
        c.definition = flow_->human;
        return {true, c};
    }
    auto &f = *flow_;
    if (p->kind == ref::WorldScriptPageKind::scene) {
        require(f.cancelled || (f.paid && f.changed && f.finished),
                "profession flow returned without its source completion");
        flow_.reset();
        return {true, {}};
    }
    const int raw = p->legacy_page;
    if (raw != 60 && raw != 61 && raw != 62 && raw != 63)
        return {}; // Existing reward/dialogue/67/70 consumers remain responsible for their pages.
    if (!sim::startup_world_human_page_ready(s, p->id))
        return {true, {}};
    require(s.page_human_bindings.at(p->id) == f.human, "profession page changed its human");
    if (raw == 60)
        return {true, input(p->id, f.cancelled || f.finished ? Human::cancel : Human::professions)};
    if (raw == 61) {
        if (s.human_page_answers.count(p->id) || f.paid)
            return {true, {}}; // The source resumes K0 and closes its parent, without another input.
        if (f.cancelled)
            return {true, input(p->id, Human::cancel)};
        const auto &list = s.human_page_catalogs.at(p->id);
        const auto found = std::find(list.begin(), list.end(), f.job);
        if (found == list.end()) {
            f.cancelled = true;
            return {true, input(p->id, Human::cancel)};
        }
        const int selected = int(found - list.begin());
        return {true, input(p->id, s.human_page_selections.at(p->id) == selected ? Human::confirm
                                                                             : Human::select,
                            selected)};
    }
    if (raw == 62) {
        if (f.cancelled || !affordable(s, f.human, f.job, reserved_points, cash_reserve)) {
            f.cancelled = true;
            return {true, input(p->id, Human::cancel)};
        }
        const auto &list = s.human_page_catalogs.at(p->id);
        const auto found = std::find(list.begin(), list.end(), f.job);
        if (found == list.end()) {
            f.cancelled = true;
            return {true, input(p->id, Human::cancel)};
        }
        const int selected = int(found - list.begin());
        return {true, input(p->id, s.page_job_bindings.at(p->id) == f.job ? Human::confirm
                                                                     : Human::select,
                            selected)};
    }
    require(p->id == f.animation && f.paid, "animation lacks its paid source transaction");
    return {true, s.page_counters.at(p->id) >= 197
                      ? std::optional<app::WorldCommand>{input(p->id, Human::confirm)}
                      : std::nullopt};
}

void SteamProfessionStrategy::set_target(const State &s, int human, int job) {
    require(!flow_ && s.rules && s.human_presence.count(human) &&
                human >= 0 && job >= 0 && std::size_t(job) < s.rules->jobs.size(),
            "explicit target cannot replace an active flow or use an unknown definition");
    require(!target_ || !target_pending(s) || *target_ == std::make_pair(human, job),
            "cannot replace an unfinished explicit profession target");
    const auto profile = sim::startup_world_human_profile(s, human);
    const auto choices = profile ? ref::catalogue_human_professions(profile->sex, catalogue(s))
                                 : std::nullopt;
    require(choices && std::find(choices->begin(), choices->end(), job) != choices->end(),
            "explicit target is not in the source gender/opening catalogue");
    target_ = std::make_pair(human, job);
}

bool SteamProfessionStrategy::target_pending(const State &s) const {
    return target_ && s.scene.world.world.ai.growth.at(target_->first).definition.current_profession !=
                          target_->second;
}

void SteamProfessionStrategy::observe(const State &before, const app::WorldCommand &c,
                                      const app::WorldCommandResult &result, const State &after,
                                      const CampaignReceiptSink &receipts) {
    if (!flow_)
        return;
    auto &f = *flow_;
    if (result.outcome != app::WorldCommandOutcome::applied) {
        if (!f.paid && (c.kind == Kind::open_human || c.kind == Kind::human_action))
            f.cancelled = true;
        return;
    }
    if (c.kind != Kind::human_action || c.human_action != Human::confirm)
        return;
    const auto *p = page(before, c.page);
    if (!p || !before.page_human_bindings.count(c.page) ||
        before.page_human_bindings.at(c.page) != f.human)
        return;
    const auto &old = before.scene.world.world.ai.growth.at(f.human);
    const auto &now = after.scene.world.world.ai.growth.at(f.human);
    if (p->legacy_page == 62) {
        const int prior = before.human_profession_changes.at(f.human).at(f.job);
        if (after.human_profession_changes.at(f.human).at(f.job) == prior) {
            f.cancelled = true; // A real denial keeps feedback; do not retry its payment.
            return;
        }
        require(!f.paid && before.page_job_bindings.at(c.page) == f.job &&
                    after.village_points == std::clamp(before.village_points - f.charge, 0, 999) &&
                    after.human_profession_changes.at(f.human).at(f.job) == prior + 1 &&
                    old.definition.current_profession == f.old_job &&
                    now.definition.current_profession == f.old_job &&
                    before.human_calendar.at(f.human).celebrations ==
                        after.human_calendar.at(f.human).celebrations &&
                    before.scene.world.world.ai.accounting.funds() ==
                        after.scene.world.world.ai.accounting.funds(),
                "62 did not commit exactly the source points/R transaction with the old profession");
        const int satisfaction = prior == 0 ? 3 : prior == 1 ? 2 : 1;
        const int effort = prior == 0 ? 3 : prior == 1 ? 1 : 0;
        require(after.shop_humans.at(f.human).satisfaction ==
                        std::min(100, before.shop_humans.at(f.human).satisfaction + satisfaction) &&
                    now.definition.legacy_u == std::min(100, old.definition.legacy_u + effort) &&
                    std::int64_t(after.scene.world.world.ai.pending_completion) ==
                        std::int64_t(before.scene.world.world.ai.pending_completion) + satisfaction,
                "profession rewards lost the prior-entry requests or their real caps");
        for (const auto &child : after.scripts.pages)
            if (child.legacy_page == 63 && child.lifecycle != 4 &&
                after.page_human_bindings.count(child.id) &&
                after.page_human_bindings.at(child.id) == f.human) {
                require(!f.animation, "paid profession has multiple animation pages");
                f.animation = child.id;
            }
        require(f.animation != 0, "payment did not insert its real63 successor");
        f.paid = true;
        if (receipts)
            receipts({CampaignReceiptKind::profession_points_paid, f.job, after.simulation_steps,
                      c.page, f.actor.value, f.human,
                      {before.village_points, prior, old.definition.current_profession,
                       before.human_calendar.at(f.human).celebrations},
                      {after.village_points, prior + 1, now.definition.current_profession,
                       after.human_calendar.at(f.human).celebrations}});
        std::cout << "PEOPLE profession_paid human=" << f.human << " target=" << f.job
                  << " quote=" << f.charge << " prior_changes=" << prior << '\n';
    } else if (p->legacy_page == 63) {
        require(c.page == f.animation && f.paid && f.changed && !f.finished &&
                    before.page_counters.at(c.page) >= 197 && page(after, c.page) &&
                    page(after, c.page)->lifecycle == 4 &&
                    now.definition.current_profession == f.job,
                "profession finish bypassed the real197 confirmation");
        const auto derived = ref::derive_human_stats(now.definition, after.scene.world.world.ai.professions);
        require(derived.candidate && same_stats(now.derived, *derived.candidate) &&
                    now.experience == 0 && now.pending.amount == 0 &&
                    now.pending.counter == old.pending.counter &&
                    before.village_points == after.village_points &&
                    before.human_profession_changes.at(f.human) ==
                        after.human_profession_changes.at(f.human),
                "197 did not install the source derived stats, preserve O and clear L/N once");
        for (const auto &[id, actor] : after.scene.world.world.ai.battle.actors) {
            (void)id;
            if (actor.kind == ref::ActorKind::human && actor.definition == f.human)
                require(actor.capacity == now.derived.combat[0], "finish lost live HP capacity");
        }
        for (const auto &[id, actor] : after.scene.world.world.ai.retired_actors) {
            (void)id;
            if (actor.kind == ref::ActorKind::human && actor.definition == f.human)
                require(actor.capacity == now.derived.combat[0], "finish lost retained HP capacity");
        }
        f.finished = true;
        if (receipts) {
            receipts({CampaignReceiptKind::profession_stats_applied, f.job, after.simulation_steps,
                      c.page, f.actor.value, f.human,
                      {old.derived.combat[0], old.derived.combat[1], old.derived.combat[2], old.derived.combat[3]},
                      {now.derived.combat[0], now.derived.combat[1], now.derived.combat[2], now.derived.combat[3]}});
            for (std::size_t slot = 0; slot < 4; ++slot)
                if (!old.derived.available_spells[slot] && now.derived.available_spells[slot]) {
                    require(slot < now.definition.spell_professions.size(), "new spell lacks its profession source");
                    const int source = now.definition.spell_professions[slot];
                    receipts({CampaignReceiptKind::spell_available, int(slot), after.simulation_steps,
                              c.page, f.actor.value, f.human,
                              {old.derived.available_spells[slot], old.definition.learned_spells[slot],
                               old.definition.current_profession, old.definition.profession_levels.at(source)},
                              {now.derived.available_spells[slot], now.definition.learned_spells[slot],
                               now.definition.current_profession, now.definition.profession_levels.at(source)}});
                }
        }
        std::cout << "PEOPLE profession_finished human=" << f.human << " target=" << f.job << '\n';
    }
}

void SteamProfessionStrategy::observe_tick(const State &before, const State &after,
                                           const CampaignReceiptSink &receipts) {
    if (!flow_ || !flow_->paid || flow_->changed)
        return;
    auto &f = *flow_;
    const auto &old = before.scene.world.world.ai.growth.at(f.human);
    const auto &now = after.scene.world.world.ai.growth.at(f.human);
    if (old.definition.current_profession == now.definition.current_profession)
        return;
    require(old.definition.current_profession == f.old_job &&
                now.definition.current_profession == f.job &&
                before.page_counters.at(f.animation) == 54 &&
                after.page_counters.at(f.animation) == 55 &&
                same_stats(old.derived, now.derived) &&
                before.human_profession_changes.at(f.human) == after.human_profession_changes.at(f.human),
            "profession setter did not occur exactly at55 with unchanged derived stats/R");
    for (const auto id : after.scene.world.world.ai.human_order)
        if (after.scene.world.world.ai.battle.actors.at(id).definition == f.human)
            require(after.actor_metadata.at(id).profession == f.job, "setter lost same-definition metadata");
    f.changed = true;
    if (receipts)
        receipts({CampaignReceiptKind::profession_set, f.job, after.simulation_steps,
                  f.animation, f.actor.value, f.human,
                  {old.definition.current_profession, before.actor_metadata.at(f.actor).profession,
                   before.page_counters.at(f.animation), before.human_profession_changes.at(f.human).at(f.job)},
                  {now.definition.current_profession, after.actor_metadata.at(f.actor).profession,
                   after.page_counters.at(f.animation), after.human_profession_changes.at(f.human).at(f.job)}});
}
} // namespace ark::test
