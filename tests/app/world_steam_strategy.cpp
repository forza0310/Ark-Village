#include "world_steam_strategy.hpp"
#include "ark/simulation/village/startup_world_village_activity.hpp"
#include "support/world_fixture.hpp"
#include "world_steam_budget.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace ref = sim::rules;
using State = app::WorldState;
using Command = app::WorldCommand;
using Kind = app::WorldCommandKind;
using Position = ref::Position;
using Activity = sim::StartupVillageActivityAction;
using Commerce = sim::StartupCommerceAction;
using Task = sim::StartupWorldTaskAction;
void require(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error("Steam village: " + message);
}
const ref::WorldScriptPage &top(const State &s) {
    for (auto p = s.scripts.pages.rbegin(); p != s.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4)
            return *p;
    throw std::runtime_error("Steam village lost its page");
}
int month(const State &s) { return s.scene.calendar.year * 12 + s.scene.calendar.month; }
std::int64_t cash(const State &s) { return s.scene.world.world.ai.accounting.funds(); }
Command cmd(Kind kind, std::uint64_t page = 0) {
    Command c;
    c.kind = kind;
    c.page = page;
    return c;
}
Command task(std::uint64_t page, Task action, int selection = 0) {
    auto c = cmd(Kind::task_action, page);
    c.task_action = action;
    c.selection = selection;
    return c;
}
Command activity(std::uint64_t page, Activity action, int selection = 0) {
    auto c = cmd(Kind::village_activity_action, page);
    c.village_activity_action = action;
    c.selection = selection;
    return c;
}
Command commerce(std::uint64_t page, Commerce action, int selection = 0) {
    auto c = cmd(Kind::commerce_action, page);
    c.commerce_action = action;
    c.selection = selection;
    return c;
}
std::int64_t reserve(const State &s) { return steam_operating_reserve(s); }
bool outside_targets(const State &s) {
    return std::any_of(steam_village_layout().begin(), steam_village_layout().end(),
                       [&](const auto &t) { return !steam_layout_inside_fence(s, t); });
}
int matching(const State &s, bool finished) {
    int count{};
    for (const auto &t : steam_village_layout()) {
        const auto match = steam_layout_match(s, t);
        if (!match)
            continue;
        const auto &f = s.scene.world.world.facilities.at(*match);
        if (finished &&
            (f.status != 1 || (s.facility_flags.at(*match) & 1U) ||
             (t.role == SteamLayoutRole::residence && s.facility_residents.at(*match) < 0)))
            continue;
        ++count;
    }
    return count;
}
bool reserved_cell(const State &s, Position cell) {
    for (const auto &t : steam_village_layout())
        for (const auto &part : steam_layout_footprint(s, t).cells)
            if (part.position == cell)
                return true;
    return false;
}
bool affordable_task(const State &s, std::uint64_t id) { return steam_task_affordable(s, id); }
bool buildable(const State &s, int definition) {
    const auto catalog = sim::startup_world_build_catalog(s);
    require(catalog.has_value(), "source construction catalogue missing");
    for (const auto &group : *catalog)
        if (std::find(group.begin(), group.end(), definition) != group.end())
            return true;
    return false;
}
int held(const State &s, int id) {
    const auto f = s.activity_counts.find(id);
    return f == s.activity_counts.end() ? 0 : f->second;
}
} // namespace

void SteamVillageStrategy::reconcile(const State &s) {
    require(s.rules && s.rank >= 1, "requires a verified first-or-later-star business prefix");
    require(validate_steam_village_layout(s) == ref::GeometryError::none,
            "Steam blueprint conflicts with current product footprint geometry");
    require(!s.build_definition && s.build_mode == 0, "cannot reload a partial layout transaction");
    require(operation_ == Operation::none && !people_.active() && !income_.active(),
            "controller has unfinished actions at player restore");
    if (stats_.initial_month < 0) {
        stats_.initial_month = month(s);
        stats_.minimum_cash = cash(s);
        departed_task_ =
            s.active_task.value_or(0); // A live task is part of the verified business prefix.
    }
    for (const auto &target : steam_village_layout())
        if (target.role == SteamLayoutRole::shop)
            if (const auto id = steam_layout_match(s, target))
                income_.register_new_shop(*id);
}

std::optional<Command> SteamVillageStrategy::layout(const State &s) {
    const auto funds = cash(s) - reserve(s);
    // First relocate an unmatched existing building to its exact source goal. Existing
    // residents are preserved by the production move transaction, never evicted to make room.
    for (const auto &t : steam_village_layout()) {
        if (!steam_layout_inside_fence(s, t) || steam_layout_match(s, t) ||
            !steam_layout_blockers(s, t).empty())
            continue;
        if ((s.scripts.user_flags & 32U) && funds >= 300)
            for (const auto &[id, f] : s.scene.world.world.facilities) {
                const bool same = f.placement.definition_id == t.definition ||
                                  (t.role == SteamLayoutRole::residence && f.kind == 12);
                if (!same || f.status != 1 || steam_layout_slot(s, id) ||
                    (f.kind != 2 && f.kind != 3 && f.kind != 12 && f.kind != 13))
                    continue;
                operation_ = Operation::move;
                moving_ = id;
                target_ = t;
                target_.definition = f.placement.definition_id;
                operation_committed_ = false;
                return cmd(Kind::open_build_menu);
            }
    }
    // Build only goals whose definitions are genuinely available. Houses first use a
    // recruitment plot and the normal admission consumer; definition25 is not injected.
    for (const auto &t : steam_village_layout()) {
        if (!steam_layout_inside_fence(s, t) || steam_layout_match(s, t) ||
            !steam_layout_blockers(s, t).empty())
            continue;
        int definition = t.definition;
        if (t.role == SteamLayoutRole::residence)
            continue;
        if (!buildable(s, definition))
            continue;
        const auto quote = sim::startup_world_build_quote(s, definition);
        if (!quote || funds < quote->construction_cost + 2 * quote->definition_attributes[3])
            continue;
        operation_ = Operation::build;
        target_ = t;
        target_.definition = definition;
        operation_committed_ = false;
        return cmd(Kind::open_build_menu);
    }
    // Purchase the actual chamber offer, once, before optional activities spend its points.
    if (s.scripts.user_flags & 16U) {
        for (const auto &t : steam_village_layout()) {
            if (t.role == SteamLayoutRole::residence || steam_layout_match(s, t) ||
                !steam_layout_inside_fence(s, t) || buildable(s, t.definition) ||
                s.rules->facilities.at(t.definition).unlock_rank < 0 ||
                s.rules->facilities.at(t.definition).unlock_rank > s.rank)
                continue;
            const auto quote = sim::startup_world_build_quote(s, t.definition);
            const int points = s.rules->facility_initial.at(t.definition).capacity;
            if (!quote || cash(s) < reserve(s) + quote->construction_cost ||
                s.village_points < points)
                continue;
            unlock_ = t.definition;
            return cmd(Kind::open_commerce);
        }
    }
    // Build a connected road frontier in the complement of the copied footprints. The
    // road choice is a PC test-player adaptation, not a claim to have decoded Steam roads.
    const auto &m = s.scene.world.world.map;
    const auto &bounds = s.rules->fences.at(s.fence_level);
    const auto paths = ref::search_legacy_map(m, sim::startup_evidence().spawn_points.at(0));
    require(paths.field.has_value(), "layout route field missing");
    const auto road_quote = sim::startup_world_build_quote(s, 18);
    if (road_quote && funds >= road_quote->construction_cost)
        for (int y = std::max(bounds[1].y + 1, 3); y <= std::min(bounds[0].y - 1, 10); ++y)
            for (int x = std::max(bounds[0].x + 1, 3); x <= std::min(bounds[1].x - 1, 20); ++x) {
                const Position cell{x, y};
                const auto &tile = m.cells.at(y * m.width + x);
                if (tile.facility || tile.legacy_state != 4 || reserved_cell(s, cell))
                    continue;
                bool connected{};
                for (const auto d : {Position{1, 0}, {-1, 0}, {0, 1}, {0, -1}}) {
                    const int nx = x + d.x, ny = y + d.y;
                    if (nx < 0 || ny < 0 || nx >= m.width || ny >= m.height)
                        continue;
                    const int index = ny * m.width + nx;
                    connected |= m.cells[index].category == ref::RouteCategory::road &&
                                 paths.field->distances[index].has_value();
                }
                if (!connected)
                    continue;
                operation_ = Operation::road;
                target_ = {18, cell, ref::FacilityOrientation::first, SteamLayoutRole::plant};
                operation_committed_ = false;
                return cmd(Kind::open_build_menu);
            }
    // Only retire an unmatched ordinary shop/plant that blocks an available replacement.
    // Keep at least ten operating shops and never remove a resident or unique goal instance.
    const auto shops =
        std::count_if(s.scene.world.world.facilities.begin(), s.scene.world.world.facilities.end(),
                      [](const auto &v) { return v.second.kind == 3 && v.second.status == 1; });
    for (const auto &t : steam_village_layout()) {
        if (t.role == SteamLayoutRole::residence || !steam_layout_inside_fence(s, t) ||
            !buildable(s, t.definition) || steam_layout_match(s, t))
            continue;
        const auto quote = sim::startup_world_build_quote(s, t.definition);
        if (!quote || funds < quote->construction_cost + 2 * quote->definition_attributes[3])
            continue;
        for (const auto id : steam_layout_blockers(s, t)) {
            const auto &f = s.scene.world.world.facilities.at(id);
            if (steam_layout_slot(s, id) || f.status != 1 ||
                (f.kind != 2 && f.kind != 13 && (f.kind != 3 || shops <= 10)))
                continue;
            // Preserve a same-definition building that can eventually be relocated to a goal.
            const bool needed =
                std::any_of(steam_village_layout().begin(), steam_village_layout().end(),
                            [&](const auto &goal) {
                                return goal.definition == f.placement.definition_id &&
                                       !steam_layout_match(s, goal);
                            });
            if (needed)
                continue;
            operation_ = Operation::remove;
            moving_ = id;
            target_ = t;
            operation_committed_ = false;
            return cmd(Kind::open_build_menu);
        }
    }
    return {};
}

std::optional<Command> SteamVillageStrategy::next(const State &s) {
    if (stats_.initial_month < 0)
        reconcile(s);
    const auto &p = top(s);
    if (p.lifecycle == 0)
        return {};
    const auto people = people_.next(s, 0, reserve(s), false);
    if (people.handled)
        return people.command;
    if (income_.active()) {
        const auto value = income_.next(s, reserve(s), false);
        if (value.handled)
            return value.command;
    }
    if (operation_ != Operation::none && p.kind == ref::WorldScriptPageKind::scene) {
        if (s.scene.scene_state == 0) {
            require(operation_committed_, "layout returned without a committed action");
            operation_ = Operation::none;
        } else if (s.scene.scene_state == 1) {
            if (operation_ == Operation::build) {
                auto c = cmd(operation_committed_ ? Kind::cancel_build : Kind::confirm_build);
                c.definition = target_.definition;
                c.anchor = target_.anchor;
                c.orientation = target_.orientation;
                return c;
            }
            auto c = cmd(operation_committed_ ? Kind::cancel_edit : Kind::confirm_edit);
            c.selection = s.build_mode;
            c.definition = s.build_definition.value_or(-1);
            c.facility = s.build_moving_facility.value_or(0);
            c.edit_anchor = s.build_anchor;
            c.orientation = target_.orientation;
            c.anchor =
                !operation_committed_ && ((operation_ == Operation::move && s.build_mode == 6) ||
                                          operation_ == Operation::remove)
                    ? s.scene.world.world.facilities.at(moving_).placement.anchor
                    : target_.anchor;
            return c;
        }
    }
    if (p.kind == ref::WorldScriptPageKind::scene) {
        if (s.scene.scene_state != 0)
            return {};
        if ((checkpoint_phase_ && checkpoint(s)) || complete(s))
            return {};
        for (const auto id : s.scene.world.facility_order)
            if (s.scene.world.world.facility_uses
                    .at(s.scene.world.world.facilities.at(id).placement.definition_id)
                    .upgrade_pending) {
                auto c = cmd(Kind::open_facility);
                c.facility = id;
                return c;
            }
        if (stats_.ticks >= next_management_) {
            next_management_ = stats_.ticks + 100;
            if (const auto person =
                    SteamPeopleStrategy::eligible_homeless(s, steam_residence_reserve(s))) {
                const bool destination =
                    std::any_of(steam_village_layout().begin(), steam_village_layout().end(),
                                [&](const auto &t) {
                                    return t.role == SteamLayoutRole::residence &&
                                           steam_layout_inside_fence(s, t) &&
                                           !steam_layout_match(s, t) &&
                                           steam_layout_blockers(s, t).empty();
                                });
                for (const auto &[id, f] : s.scene.world.world.facilities) {
                    if (!destination || f.placement.definition_id != 24 || f.status != 1)
                        continue;
                    const auto plot_anchor = f.placement.anchor;
                    const bool home_goal =
                        std::any_of(steam_village_layout().begin(), steam_village_layout().end(),
                                    [&](const auto &t) {
                                        return t.role == SteamLayoutRole::recruitment &&
                                               t.anchor == plot_anchor;
                                    });
                    if (!home_goal)
                        continue;
                    recruitment_ = id;
                    resident_ = *person;
                    auto c = cmd(Kind::open_facility);
                    c.facility = id;
                    return c;
                }
            }
            const auto value = people_.next(s, 0, reserve(s), true);
            if (value.handled)
                return value.command;
            const auto invest = income_.next(s, reserve(s), true);
            if (invest.handled)
                return invest.command;
        }
        if (stats_.ticks >= next_layout_) {
            next_layout_ = stats_.ticks + 40;
            if (const auto value = layout(s))
                return value;
        }
        if (!s.active_task && month(s) >= next_task_month_ &&
            std::any_of(s.task_order.begin(), s.task_order.end(),
                        [&](auto id) { return affordable_task(s, id); }))
            return cmd(
                Kind::open_task_menu); // Victories keep producing the actual improvement stock.
        if (s.quarter_counter > 0 && stats_.ticks >= next_activity_ && s.village_points >= 20) {
            next_activity_ = stats_.ticks + 300;
            return cmd(Kind::open_village_activities);
        }
        return {};
    }
    const int raw = p.legacy_page;
    if (raw == 21) {
        auto c = cmd(Kind::select_build_menu, p.id);
        c.definition = operation_ == Operation::move     ? -2
                       : operation_ == Operation::remove ? -1
                                                         : target_.definition;
        return c;
    }
    if (raw == 83 || raw == 85 || raw == 93) {
        const auto v = sim::inspect_startup_world_commerce_page(s, p.id);
        if (!v)
            return {};
        if (raw == 93) {
            require(unlock_ >= 0 && v->binding == unlock_, "chamber reward binding changed");
            return commerce(p.id, Commerce::confirm);
        }
        if (unlock_ < 0 || buildable(s, unlock_)) {
            if (raw == 83)
                unlock_ = -1;
            return commerce(p.id, Commerce::cancel);
        }
        if (raw == 83)
            return commerce(p.id, v->selection == 2 ? Commerce::confirm : Commerce::select, 2);
        const auto found = std::find(v->entries.begin(), v->entries.end(), unlock_);
        require(found != v->entries.end(), "planned chamber building is not actually offered");
        const int index = static_cast<int>(found - v->entries.begin());
        return commerce(p.id, v->selection == index ? Commerce::confirm : Commerce::select, index);
    }
    if (raw >= 51 && raw <= 54) {
        const auto v = sim::inspect_startup_world_village_activity_page(s, p.id);
        if (!v || s.activity_page_answers.count(p.id))
            return {};
        if (raw == 51) {
            int selected = -1, best = std::numeric_limits<int>::min();
            int points_reserve{};
            for (const auto &t : steam_village_layout())
                if (t.role != SteamLayoutRole::residence && steam_layout_inside_fence(s, t) &&
                    !buildable(s, t.definition) &&
                    s.rules->facilities.at(t.definition).unlock_rank >= 0 &&
                    s.rules->facilities.at(t.definition).unlock_rank <= s.rank)
                    points_reserve = std::max(points_reserve,
                                              s.rules->facility_initial.at(t.definition).capacity);
            // An unaffordable expansion must accumulate its budget. Merely giving
            // it a high score lets cheaper optional events spend those points first.
            // Reserve only an expansion currently offered by the source catalogue.
            if (outside_targets(s)) {
                std::optional<int> expansion_cost;
                for (const int id : v->entries) {
                    const auto &a = s.rules->activities.at(id);
                    if (a.parameters[2] == 3 &&
                        (!expansion_cost || a.parameters[4] < *expansion_cost))
                        expansion_cost = a.parameters[4];
                }
                if (expansion_cost)
                    points_reserve = std::max(points_reserve, *expansion_cost);
            }
            for (int n = 0; n < static_cast<int>(v->entries.size()); ++n) {
                const auto &a = s.rules->activities.at(v->entries[n]);
                const bool expansion = a.parameters[2] == 3 && outside_targets(s);
                if (a.parameters[2] == 3 && !expansion)
                    continue;
                if (!expansion && !steam_optional_activity_affordable(s))
                    continue;
                if (a.parameters[4] > s.village_points - (expansion ? 0 : points_reserve))
                    continue;
                const auto rank_terms = ref::fixed_calendar_task_rank_terms();
                const auto terms = rank_terms.find(s.rank);
                const bool needs_popularity =
                    terms != rank_terms.end() &&
                    std::any_of(terms->second.begin(), terms->second.end(), [&](const auto &term) {
                        return term.type == 5 && s.popularity < term.threshold;
                    });
                const int score = (expansion ? 100000 : 0) +
                                  (a.parameters[2] == 2 && needs_popularity ? 1000 : 0) -
                                  held(s, a.identity) * 10;
                if (score > best) {
                    best = score;
                    selected = n;
                }
            }
            if (selected < 0)
                return activity(p.id, Activity::cancel);
            return activity(p.id, v->selection == selected ? Activity::confirm : Activity::select,
                            selected);
        }
        return raw == 53 && v->counter < 120
                   ? std::nullopt
                   : std::optional<Command>{activity(p.id, Activity::confirm)};
    }
    if (raw == 22) {
        const auto list = s.task_page_lists.find(p.id);
        if (list == s.task_page_lists.end())
            return {};
        int selected = -1, fewest = std::numeric_limits<int>::max();
        for (int n = 0; n < static_cast<int>(list->second.size()); ++n) {
            const auto id = list->second[n];
            if (!affordable_task(s, id))
                continue;
            const int d = s.tasks.at(id).definition;
            const int done = s.task_progress.definitions.at(d).completed;
            if (done < fewest) {
                fewest = done;
                selected = n;
            }
        }
        return task(p.id, selected < 0 ? Task::cancel : Task::confirm, std::max(0, selected));
    }
    if (raw == 23)
        return task(p.id, p.task_identity && affordable_task(s, *p.task_identity) ? Task::confirm
                                                                                  : Task::cancel);
    if (raw == 25)
        return task(p.id, Task::depart);
    if (raw == 28)
        return s.page_phases.at(p.id) == 0 ? std::optional<Command>{task(p.id, Task::confirm)}
                                           : std::nullopt;
    if (raw == 33)
        return task(p.id, Task::confirm, steam_task_renewable(s, p.legacy_f) ? 0 : 1);
    if (raw == 48) {
        auto c = cmd(Kind::rank_action, p.id);
        c.cancel = !std::all_of(s.rank_met.begin(), s.rank_met.end(), [](bool met) { return met; });
        return c;
    }
    if (raw == 74) {
        auto c = cmd(Kind::facility_action, p.id);
        c.facility_action =
            recruitment_ && s.facility_page_bindings.at(p.id) == recruitment_ && resident_ >= 0
                ? sim::StartupFacilityPageAction::confirm
                : sim::StartupFacilityPageAction::cancel;
        return c;
    }
    if (raw == 80) {
        auto c = cmd(Kind::residence_action, p.id);
        c.selection = resident_;
        const auto &list = s.residence_page_candidates.at(p.id);
        c.cancel = std::find(list.begin(), list.end(), resident_) == list.end();
        return c;
    }
    if (raw == 82)
        return s.facility_catalog_pages_initialized.count(p.id)
                   ? std::optional<Command>{cmd(Kind::facility_catalog_action, p.id)}
                   : std::nullopt;
    if (raw == 90)
        return s.tax_page_residents.count(p.id)
                   ? std::optional<Command>{cmd(Kind::tax_action, p.id)}
                   : std::nullopt;
    if (raw == 60 || raw == 64) {
        auto c = cmd(Kind::human_action, p.id);
        c.human_action = sim::StartupHumanPageAction::cancel;
        return c;
    }
    if (raw == 70) {
        if (!sim::startup_world_human_page_ready(s, p.id))
            return {};
        auto c = cmd(Kind::human_action, p.id);
        c.human_action = sim::StartupHumanPageAction::confirm;
        if (s.page_phases.at(p.id) == 2 && s.human_page_selections.at(p.id) != 1) {
            c.human_action = sim::StartupHumanPageAction::select;
            c.selection = 1;
        }
        return c;
    }
    if (std::set<int>{16, 24, 56, 57, 97, 98}.count(raw))
        return {};
    require(std::set<int>{0, 1, 11, 15, 30, 31, 32, 49, 50, 59, 67, 81, 88, 89, 94, 95, 96, 99, 100}
                .count(raw),
            "uncovered decision page raw=" + std::to_string(raw));
    return cmd(Kind::acknowledge_page, p.id);
}

void SteamVillageStrategy::observe_world(const State &before, const State &after) {
    stats_.trade.observe(before, after);
    stats_.minimum_cash = std::min(stats_.minimum_cash, cash(after));
    require(cash(after) >= 0, "town ran out of money");
    for (std::size_t slot = 0; slot < steam_village_layout().size(); ++slot) {
        const auto &goal = steam_village_layout()[slot];
        if (goal.role != SteamLayoutRole::shop)
            continue;
        const auto old = steam_layout_match(before, goal), now = steam_layout_match(after, goal);
        if (old && now && *old == *now &&
            after.scene.world.world.facilities.at(*now).sales >
                before.scene.world.world.facilities.at(*old).sales)
            traded_slots_.insert(slot);
    }
    if (!before.active_task && after.active_task)
        departed_task_ = *after.active_task;
    if (after.task_progress.successes > before.task_progress.successes) {
        require(departed_task_ &&
                    after.task_progress.successes == before.task_progress.successes + 1 &&
                    won_tasks_.insert(departed_task_).second,
                "victory lacks unique departure");
        ++stats_.victories;
    }
    if (before.active_task && !after.active_task)
        next_task_month_ = steam_task_recovery_month(after);
    if (after.events_held > before.events_held)
        stats_.activities += after.events_held - before.events_held;
    if (stats_.completed_month < 0 && after.rank >= 4 && matching(after, true) == 50)
        stats_.completed_month = month(after);
}
void SteamVillageStrategy::observe(const State &before, const Command &c,
                                   const app::WorldCommandResult &r, const State &after,
                                   const CampaignReceiptSink &receipts) {
    require(r.outcome == app::WorldCommandOutcome::applied &&
                r.runtime_error == sim::StartupWorldRuntimeError::none &&
                r.build_denial == sim::StartupBuildDenial::none &&
                r.denial == ref::TaskCommandDenial::none,
            "command rejected kind=" + std::to_string(int(c.kind)) +
                " build=" + std::to_string(int(r.build_denial)));
    // Keep the segment evidence private until every command/cross-domain oracle
    // passes. A cold load or a queued page answer does not certify a past effect.
    std::vector<CampaignReceipt> pending;
    ++stats_.commands;
    people_.observe(before, c, r, after,
                    [&](const CampaignReceipt &receipt) { pending.push_back(receipt); });
    income_.observe(before, c, after,
                    [&](const CampaignReceipt &receipt) { pending.push_back(receipt); });
    if (c.kind == Kind::confirm_build) {
        const auto quote = sim::startup_world_build_quote(before, target_.definition);
        require(r.created && quote && cash(before) - cash(after) == quote->construction_cost,
                "building charge mismatch");
        const auto &f = after.scene.world.world.facilities.at(*r.created);
        require(f.placement.definition_id == target_.definition &&
                    f.placement.anchor == target_.anchor &&
                    f.placement.orientation == target_.orientation,
                "construction did not match Steam goal");
        stats_.construction_cost += quote->construction_cost;
        ++stats_.builds;
        operation_committed_ = true;
        if (target_.role == SteamLayoutRole::shop)
            income_.register_new_shop(*r.created);
        pending.push_back(facility_transaction_receipt(
            CampaignReceiptKind::facility_construction_paid, target_.definition, before, after,
            top(before).id, *r.created));
    }
    if (c.kind == Kind::confirm_edit && operation_ == Operation::move && before.build_mode == 7) {
        require(r.created && cash(before) - cash(after) == 300 &&
                    !after.scene.world.world.facilities.count(moving_),
                "move must retire old instance and pay actual fee");
        const auto &f = after.scene.world.world.facilities.at(*r.created);
        const auto &old = before.scene.world.world.facilities.at(moving_);
        require(f.sales == old.sales && f.placement.anchor == target_.anchor &&
                    f.placement.orientation == target_.orientation &&
                    after.facility_residents.at(*r.created) ==
                        before.facility_residents.at(moving_),
                "move lost business/resident or layout");
        stats_.construction_cost += 300;
        ++stats_.moves;
        operation_committed_ = true;
        if (target_.role == SteamLayoutRole::shop)
            income_.register_new_shop(*r.created);
    }
    if (c.kind == Kind::confirm_edit && operation_ == Operation::road && before.build_mode == 2) {
        const auto &m = after.scene.world.world.map;
        require(m.cells.at(target_.anchor.y * m.width + target_.anchor.x).category ==
                    ref::RouteCategory::road,
                "road was not installed");
        stats_.construction_cost += cash(before) - cash(after);
        ++stats_.roads;
        operation_committed_ = true;
    }
    if (c.kind == Kind::confirm_edit && operation_ == Operation::remove && before.build_mode == 3) {
        require(!after.scene.world.world.facilities.count(moving_),
                "replacement did not retire old shop");
        ++stats_.removals;
        operation_committed_ = true;
    }
    if (c.kind == Kind::residence_action && !c.cancel) {
        require(r.created && resident_ >= 0 && before.human_homes.at(resident_)[2] == 0 &&
                    after.human_homes.at(resident_)[2] == 1 &&
                    after.scene.world.world.facilities.at(*r.created).kind == 12 &&
                    after.facility_residents.at(*r.created) == resident_ &&
                    cash(before) - cash(after) == before.rules->humans.at(resident_).residence_fee,
                "residence transaction mismatch");
        pending.push_back(
            residence_transaction_receipt(resident_, before, after, c.page, *r.created));
        ++stats_.admissions;
        recruitment_ = 0;
        resident_ = -1;
    }
    if (c.kind == Kind::commerce_action && c.commerce_action == Commerce::confirm) {
        const auto view = sim::inspect_startup_world_commerce_page(before, c.page);
        require(view.has_value(), "chamber observation lacks its initialized source page");
        if (view->raw == 85) {
            const int definition = view->entries.at(view->selection);
            const int price = before.rules->facility_initial.at(definition).capacity;
            require(definition == unlock_ && before.facility_presence.at(definition) == 0 &&
                        before.village_points - after.village_points == price &&
                        cash(before) == cash(after) && top(after).legacy_page == 93 &&
                        top(after).legacy_s == definition &&
                        before.facility_presence.at(definition) ==
                            after.facility_presence.at(definition),
                    "chamber must pay the selected blueprint before its later claim");
            stats_.village_points_spent += price;
            pending.push_back(facility_transaction_receipt(
                CampaignReceiptKind::facility_blueprint_paid, definition, before, after, c.page));
        }
        if (view->raw == 93 && view->counter >= 40) {
            const int definition = view->binding;
            require(definition == unlock_ && before.facility_presence.at(definition) == 0 &&
                        after.facility_presence.at(definition) == 2 &&
                        after.facility_free_builds.at(definition) ==
                            std::min(99, before.facility_free_builds.at(definition) + 1) &&
                        before.village_points == after.village_points &&
                        cash(before) == cash(after) && top(after).id != c.page,
                    "blueprint claim must open the paid definition once without another charge");
            pending.push_back(
                facility_transaction_receipt(CampaignReceiptKind::facility_blueprint_claimed,
                                             definition, before, after, c.page));
        }
    }
    if (c.kind == Kind::village_activity_action && c.village_activity_action == Activity::confirm) {
        const auto view = sim::inspect_startup_world_village_activity_page(before, c.page);
        require(view.has_value(), "activity observation lacks its initialized source page");
        if (view->raw == 52 && view->activity) {
            const int definition = *view->activity;
            require(before.village_points - after.village_points ==
                            before.rules->activities.at(definition).parameters[4] &&
                        held(after, definition) == held(before, definition) + 1,
                    "activity must pay its actual point quote and begin exactly once");
            pending.push_back({CampaignReceiptKind::activity_points_paid,
                               definition,
                               after.simulation_steps,
                               c.page,
                               0,
                               -1,
                               {before.village_points, held(before, definition)},
                               {after.village_points, held(after, definition)}});
        }
        if (view->raw == 53 && view->counter >= 120 && view->activity == 30 &&
            top(after).id != c.page && !(before.scripts.user_flags & 1U)) {
            require(after.scripts.user_flags & 1U, "completed import did not open the magic pot");
            pending.push_back({CampaignReceiptKind::activity_pot_opened,
                               30,
                               after.simulation_steps,
                               c.page,
                               0,
                               -1,
                               {before.scripts.user_flags},
                               {after.scripts.user_flags}});
        }
    }
    observe_world(before, after);
    if (const auto victory = task_victory_receipt(before, after, departed_task_))
        pending.push_back(*victory);
    if (receipts)
        for (const auto &receipt : pending)
            receipts(receipt);
}
void SteamVillageStrategy::observe_tick(const State &before, const State &after,
                                        const CampaignReceiptSink &receipts) {
    std::vector<CampaignReceipt> pending;
    people_.observe_tick(before, after,
                         [&](const CampaignReceipt &receipt) { pending.push_back(receipt); });
    income_.observe_tick(before, after,
                         [&](const CampaignReceipt &receipt) { pending.push_back(receipt); });
    ++stats_.ticks;
    observe_world(before, after);
    if (const auto victory = task_victory_receipt(before, after, departed_task_))
        pending.push_back(*victory);
    if (receipts)
        for (const auto &receipt : pending)
            receipts(receipt);
}
bool SteamVillageStrategy::checkpoint(const State &s) const {
    return stats_.initial_month >= 0 && month(s) > stats_.initial_month &&
           stats_.builds + stats_.moves > 0 && income_.uses() > 0 && stats_.victories > 0 &&
           operation_ == Operation::none && unlock_ < 0 && !people_.active() && !income_.active();
}
bool SteamVillageStrategy::complete(const State &s) const {
    return s.rank >= 4 && matching(s, true) == 50 && traded_slots_.size() == 25 &&
           stats_.trade.full_month_after(stats_.completed_month, month(s)) &&
           operation_ == Operation::none && unlock_ < 0 && !people_.active() && !income_.active();
}
std::string SteamVillageStrategy::diagnose(const State &s) const {
    std::ostringstream out;
    out << "raw=" << top(s).legacy_page << " month=" << month(s) << " ticks=" << stats_.ticks
        << " rank=" << s.rank << " cash=" << cash(s) << " reserve=" << reserve(s)
        << " points=" << s.village_points << " income_record=" << s.maximum_income
        << " quarter_slots=" << s.quarter_counter
        << " expansion_status=" << s.scripts.activities.at(25).status
        << " expansion_flags=" << s.activity_flags.at(25) << " expansion_count=" << held(s, 25)
        << " task=" << s.active_task.value_or(0) << " offers=" << s.task_order.size()
        << " task_resume_month=" << next_task_month_ << " popularity=" << s.popularity
        << " fence=" << s.fence_level << " layout=" << matching(s, true)
        << "/50 trading_slots=" << traded_slots_.size() << "/25 builds=" << stats_.builds
        << " moves=" << stats_.moves << " removals=" << stats_.removals << " roads=" << stats_.roads
        << " items=" << income_.uses() << " victories=" << stats_.victories
        << " activities=" << stats_.activities << " residents=" << stats_.admissions
        << " medals=" << people_.medals() << " equipment=" << people_.gifts();
    return out.str();
}
void SteamVillageStrategy::encode(std::ostream &out) const {
    require(operation_ == Operation::none && unlock_ < 0 && !people_.active() && !income_.active(),
            "only stable Steam decisions can be saved");
    const auto &v = stats_;
    out << "ARK_STEAM_PLAYER_1\n"
        << v.ticks << ' ' << v.commands << ' ' << v.builds << ' ' << v.moves << ' ' << v.removals
        << ' ' << v.roads << ' ' << v.admissions << ' ' << v.victories << ' ' << v.activities << ' '
        << v.initial_month << ' ' << v.completed_month << ' ' << v.minimum_cash << ' '
        << v.construction_cost << ' ' << v.village_points_spent << ' ' << next_task_month_ << ' '
        << departed_task_ << ' ' << checkpoint_phase_ << '\n';
    out << won_tasks_.size();
    for (auto id : won_tasks_)
        out << ' ' << id;
    out << '\n';
    out << traded_slots_.size();
    for (const auto slot : traded_slots_)
        out << ' ' << slot;
    out << '\n';
    v.trade.encode(out);
    people_.encode(out);
    income_.encode(out);
}
SteamVillageStrategy SteamVillageStrategy::decode(std::istream &in) {
    SteamVillageStrategy value;
    auto &v = value.stats_;
    std::string magic;
    in >> magic;
    require(magic == "ARK_STEAM_PLAYER_1", "unknown Steam decision format");
    in >> v.ticks >> v.commands >> v.builds >> v.moves >> v.removals >> v.roads >> v.admissions >>
        v.victories >> v.activities >> v.initial_month >> v.completed_month >> v.minimum_cash >>
        v.construction_cost >> v.village_points_spent >> value.next_task_month_ >>
        value.departed_task_ >> value.checkpoint_phase_;
    std::size_t count{};
    in >> count;
    require(in && count <= 10000, "invalid victory history");
    for (std::size_t n = 0; n < count; ++n) {
        std::uint64_t id{};
        in >> id;
        require(id && value.won_tasks_.insert(id).second, "duplicate victory");
    }
    require(in && v.victories == static_cast<int>(count) && v.minimum_cash >= 0 &&
                v.construction_cost >= 0 && v.village_points_spent >= 0,
            "invalid Steam business evidence");
    in >> count;
    require(in && count <= 25, "invalid target shop trade history");
    for (std::size_t n = 0; n < count; ++n) {
        std::size_t slot{};
        in >> slot;
        require(in && slot < steam_village_layout().size() &&
                    steam_village_layout()[slot].role == SteamLayoutRole::shop &&
                    value.traded_slots_.insert(slot).second,
                "invalid/duplicate traded goal");
    }
    v.trade.decode(in);
    value.people_ = SteamPeopleStrategy::decode(in);
    value.income_ = ActiveIncomeStrategy::decode(in);
    require(value.income_.reward_only(), "Steam improvement policy must consume owned loot");
    return value;
}
void steam_strategy_contract() {
    steam_layout_contract();
    steam_people_contract();
    steam_budget_contract();
    SteamVillageStrategy strategy;
    std::stringstream out;
    strategy.encode(out);
    const auto copy = SteamVillageStrategy::decode(out);
    require(copy.stats().ticks == 0, "empty strategy roundtrip lost counters");
    const auto s = initial_world();
    require(!strategy.checkpoint(s) && !strategy.complete(s),
            "untouched world cannot certify Steam business");
    auto available = s;
    available.facility_presence.at(28) = 1; // Source status1 is already a construction entry.
    require(buildable(available, 28), "status1 must not trigger repeated chamber purchases");
    available.facility_presence.at(28) = 0;
    require(!buildable(available, 28), "unavailable definitions must wait for actual unlock");

    // Explicit catalogue qualifications; normal Init builds the real offered list.
    // This tests a legal test-player decision, not natural activity unlocking.
    auto budget = initial_world();
    budget.rank = 2;
    for (auto &[id, activity] : budget.scripts.activities)
        activity.status = id == 23 || id == 25 ? 1 : 0;
    for (const auto &t : steam_village_layout())
        budget.facility_presence.at(t.definition) = 1;
    budget.quarter_counter = 1;
    budget.village_points = 99;
    ref::WorldScriptPage menu;
    menu.kind = ref::WorldScriptPageKind::raw_page;
    menu.legacy_page = 51;
    const auto inserted =
        ref::prepare_world_script_page(sim::startup_world_runtime_scripts(budget), menu);
    require(inserted.candidate &&
                sim::write_startup_world_runtime_scripts(budget, inserted.candidate->state),
            "activity budget fixture lost its source page");
    const auto page = budget.scripts.pages.back().id;
    require(sim::initialize_startup_world_village_activity_pages(budget),
            "activity budget fixture did not initialize its source catalogue");
    for (int step = 0; step < 30 && top(budget).id != page; ++step) {
        const auto &introduced = top(budget);
        if (introduced.kind == ref::WorldScriptPageKind::dialogue && introduced.lifecycle == 2)
            require(sim::acknowledge_startup_world_runtime_page(budget, introduced.id) ==
                        sim::StartupWorldRuntimeError::none,
                    "activity budget introduction did not close through its source consumer");
        const auto update = sim::prepare_startup_world_runtime(budget);
        require(update.candidate.has_value(), "activity budget introduction update failed");
        budget = *update.candidate;
    }
    const auto view = sim::inspect_startup_world_village_activity_page(budget, page);
    require(view && view->entries == std::vector<int>({23, 25}),
            "activity budget fixture lost its optional/expansion offers");
    SteamVillageStrategy player;
    auto choice = player.next(budget);
    require(choice && choice->kind == Kind::village_activity_action &&
                choice->village_activity_action == Activity::cancel,
            "optional event spent the points needed for an offered expansion");
    budget.village_points = 100;
    choice = player.next(budget);
    require(choice && choice->kind == Kind::village_activity_action &&
                choice->village_activity_action == Activity::select &&
                view->entries.at(choice->selection) == 25,
            "affordable expansion did not take priority over the optional event");
    budget.fence_level = 2; // Planning qualification only; no expansion effect certificate.
    budget.village_points = 20;
    choice = player.next(budget);
    require(choice && choice->kind == Kind::village_activity_action &&
                choice->village_activity_action == Activity::confirm && view->selection == 0,
            "an already-reachable layout unnecessarily reserved expansion points");
    budget.scene.world.world.ai.accounting = ref::PeriodAccounting(reserve(budget) - 1);
    choice = player.next(budget);
    require(choice && choice->village_activity_action == Activity::cancel,
            "low-liquidity optional event must wait without consuming points");
    budget.fence_level = s.fence_level;
    budget.village_points = 100;
    choice = player.next(budget);
    require(choice && choice->village_activity_action == Activity::select &&
                view->entries.at(choice->selection) == 25,
            "necessary point-only expansion must remain available below the cash buffer");

    // Explicit task-end observation: the existing sidecar retains the recovery
    // month across cold loading, including a calendar year boundary.
    auto before = initial_world();
    before.rank = 2;
    before.scene.calendar.year = 5;
    before.scene.calendar.month = 11;
    before.active_task = 1; // Task-end decision qualification, not a natural task grant.
    auto after = before;
    after.active_task.reset();
    SteamVillageStrategy recovery;
    recovery.observe_tick(before, after);
    std::stringstream saved;
    recovery.encode(saved);
    const auto resumed = SteamVillageStrategy::decode(saved);
    require(resumed.diagnose(after).find("task_resume_month=73") != std::string::npos,
            "cold load must preserve the complete business recovery month");
}
} // namespace ark::test
