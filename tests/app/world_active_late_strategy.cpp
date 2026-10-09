#include "world_active_late_strategy.hpp"

#include "ark/simulation/rules/human_management.hpp"
#include <algorithm>
#include <istream>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace ref = sim::rules;
using State = app::WorldState;
using Command = app::WorldCommand;
using Kind = app::WorldCommandKind;
using Human = sim::StartupHumanPageAction;
using Activity = sim::StartupVillageActivityAction;
using Task = sim::StartupWorldTaskAction;
void require(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error("Active late village: " + message);
}
const ref::WorldScriptPage &top(const State &s) {
    for (auto p = s.scripts.pages.rbegin(); p != s.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4)
            return *p;
    throw std::runtime_error("Active late village lost its page");
}
int month(const State &s) { return s.scene.calendar.year * 12 + s.scene.calendar.month; }
std::int64_t cash(const State &s) { return s.scene.world.world.ai.accounting.funds(); }
Command command(Kind kind, std::uint64_t page = 0) {
    Command c;
    c.kind = kind;
    c.page = page;
    return c;
}
Command human(std::uint64_t page, Human action, int selection = 0) {
    auto c = command(Kind::human_action, page);
    c.human_action = action;
    c.selection = selection;
    return c;
}
Command activity(std::uint64_t page, Activity action, int selection = 0) {
    auto c = command(Kind::village_activity_action, page);
    c.village_activity_action = action;
    c.selection = selection;
    return c;
}
Command task(std::uint64_t page, Task action, int selection = 0) {
    auto c = command(Kind::task_action, page);
    c.task_action = action;
    c.selection = selection;
    return c;
}
int count(const State &s, bool houses, bool finished = false) {
    int n{};
    for (const auto &[id, f] : s.scene.world.world.facilities) {
        (void)id;
        if ((houses ? f.kind == 12 : f.kind == 3 || f.kind == 9) && (!finished || f.status == 1))
            ++n;
    }
    return n;
}
bool ready(const State &s) {
    return s.rank == 1 && s.popularity >= 800 && s.task_progress.successes >= 12 &&
           count(s, false, true) >= 10 && count(s, true, true) >= 4;
}
int activity_count(const State &s, int id) {
    const auto f = s.activity_counts.find(id);
    return f == s.activity_counts.end() ? 0 : f->second;
}
// A conservative player budget, not a replacement fee algorithm: current facility quotes
// plus the larger published job-fee endpoint for every present definition, for two months.
std::int64_t reserve(const State &s) {
    std::int64_t fee{};
    for (const auto &[id, f] : s.scene.world.world.facilities) {
        (void)f;
        const auto q = sim::startup_world_facility_values(s, id);
        require(q.has_value(), "facility reserve has no current quote");
        fee += std::max<std::int64_t>(0, q->definition_attributes[3]);
    }
    for (const auto &[id, present] : s.human_presence) {
        if (!present)
            continue;
        const auto &g = s.scene.world.world.ai.growth.at(id);
        const auto &j = s.rules->jobs.at(g.definition.current_profession);
        fee += std::max({0, j.fee[0], j.fee[1]});
    }
    return fee * 2;
}
std::optional<ref::Position> free_roadside(const State &s) {
    const auto &m = s.scene.world.world.map;
    const auto bounds = s.rules->fences.at(s.fence_level);
    const auto paths = ref::search_legacy_map(m, sim::startup_evidence().spawn_points.at(0));
    require(paths.field.has_value(), "roadside plan has no current distance field");
    for (int y = bounds[1].y + 1; y < bounds[0].y; ++y)
        for (int x = bounds[0].x + 1; x < bounds[1].x; ++x) {
            const auto &cell = m.cells.at(y * m.width + x);
            if (cell.facility || cell.legacy_state != 4)
                continue;
            for (const auto d : {ref::Position{1, 0}, {-1, 0}, {0, 1}, {0, -1}}) {
                const int nx = x + d.x, ny = y + d.y;
                if (nx < 0 || ny < 0 || nx >= m.width || ny >= m.height)
                    continue;
                const auto index = ny * m.width + nx;
                if (m.cells.at(index).category == ref::RouteCategory::road &&
                    paths.field->distances.at(index))
                    return ref::Position{x, y};
            }
        }
    return {};
}
bool homeless(const State &s, int id) {
    return s.human_presence.at(id) != 0 && s.human_homes.at(id)[2] == 0;
}
bool eligible(const State &s, int id) {
    return homeless(s, id) &&
           s.shop_humans.at(id).satisfaction >= s.rules->humans.at(id).residence_threshold;
}
std::optional<ref::HumanManagementEquipment> equipment(const State &s, int slot, int id) {
    const int kind = slot == 0 ? 1 : slot == 3 ? 3 : 2;
    const auto f =
        std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(),
                     [&](const auto &e) { return e.shop.kind == kind && e.shop.id == id; });
    const auto current = s.catalog.find({kind, id});
    if (f == s.rules->equipment.end() || current == s.catalog.end() ||
        (kind == 2 && (f->shop.type == 2 ? 1 : 2) != slot))
        return {};
    return ref::HumanManagementEquipment{id,
                                         slot,
                                         current->second.status,
                                         f->gift_order,
                                         f->reward_difficulty,
                                         f->shop.price,
                                         f->gift_rating,
                                         current->second.free_purchases,
                                         f->shop.combat};
}
struct Gift {
    int slot{}, index{}, cost{}, satisfaction{};
};
std::optional<Gift> gift(const State &s, std::uint64_t page, int person) {
    const auto &lists = s.equipment_page_catalogs.at(page);
    const auto &g = s.scene.world.world.ai.growth.at(person);
    const auto &shop = s.shop_humans.at(person);
    std::optional<Gift> best;
    const auto budget = cash(s) - reserve(s) - s.rules->humans.at(person).residence_fee;
    for (int slot = 0; slot < 4; ++slot)
        for (int index = 0; index < static_cast<int>(lists[slot].size()); ++index) {
            const auto target = equipment(s, slot, lists[slot][index]);
            const auto old =
                shop.equipment[slot] ? equipment(s, slot, *shop.equipment[slot]) : std::nullopt;
            if (!target || !target->availability ||
                (old && (target->grade < old->grade ||
                         !std::equal(
                             old->combat.begin(), old->combat.end(), target->combat.begin(),
                             [](int a, int b) { return a <= b; }))))
                continue; // Cultivation must not silently downgrade a combat loadout.
            const auto quote = ref::human_equipment_gift_cost(*target);
            const auto score = ref::human_equipment_gift_evaluation(
                old ? old->grade : 0, target->grade,
                s.rules->jobs.at(g.definition.current_profession).gift_profile, target->affinity);
            const auto reward = score ? ref::human_gift_rewards(*score) : std::nullopt;
            if (!quote || !reward || (*reward)[0] <= 0)
                continue;
            const int price = std::max(0, *quote);
            if (price > budget)
                continue;
            const Gift candidate{slot, index, price, (*reward)[0]};
            if (!best || static_cast<std::int64_t>(price) * best->satisfaction <
                             static_cast<std::int64_t>(best->cost) * candidate.satisfaction)
                best = candidate;
        }
    return best;
}
bool affordable_task(const State &s, std::uint64_t id) {
    const auto f = s.tasks.find(id);
    return f != s.tasks.end() &&
           s.rules->tasks.at(f->second.definition).recruitment_fee <= cash(s) - reserve(s);
}
} // namespace

void ActiveLateVillageStrategy::reconcile(const State &s) {
    require(s.rules && s.rank >= 1, "late route requires a real first-star player save");
    if (!initialized_) {
        stats_.minimum_cash = cash(s);
        stats_.initial_successes = s.task_progress.successes;
        require(s.rank == 1 && !s.active_task, "new late route must start at stable first star");
        initialized_ = true;
    }
    require(s.task_progress.successes >= stats_.initial_successes + stats_.task_successes,
            "task successes lost across player reload");
    require(stats_.promoted_month < 0 || s.rank >= 2, "second star lost across reload");
    require(stats_.pot_month < 0 || activity_count(s, 30) > 0, "pot import lost across reload");
    for (const auto id : stats_.residents)
        require(s.human_homes.at(id)[2] != 0, "admitted resident lost across reload");
    for (const auto id : stats_.buildings)
        require(s.scene.world.world.facilities.count(id) != 0, "built shop lost across reload");
    require(!s.build_definition, "late checkpoint contains unfinished placement input");
    build_definition_ = -1;
    build_committed_ = false;
    gifting_ = false;
}

std::optional<Command> ActiveLateVillageStrategy::next(const State &s) {
    if (!initialized_)
        reconcile(s);
    const auto &p = top(s);
    if (p.lifecycle == 0)
        return {};
    if (s.build_definition) {
        require(*s.build_definition == build_definition_, "unexpected placement definition");
        auto c = command(build_committed_ ? Kind::cancel_build : Kind::confirm_build);
        c.definition = build_definition_;
        c.anchor = build_anchor_;
        c.orientation = ref::FacilityOrientation::first;
        return c;
    }
    if (p.kind == ref::WorldScriptPageKind::scene) {
        if (s.scene.scene_state != 0)
            return {};
        for (const auto id : s.scene.world.facility_order)
            if (s.scene.world.world.facility_uses
                    .at(s.scene.world.world.facilities.at(id).placement.definition_id)
                    .upgrade_pending) {
                auto c = command(Kind::open_facility);
                c.facility = id;
                return c;
            }
        if (stats_.pot_month >= 0)
            return {}; // After import, let the town actually trade for a whole month.
        if (s.quarter_counter > 0 && stats_.ticks >= next_activity_tick_ &&
            s.village_points >= (s.rank >= 2 ? s.rules->activities.at(30).parameters[4] : 20)) {
            next_activity_tick_ = stats_.ticks + 300;
            return command(Kind::open_village_activities);
        }
        if (stats_.ticks >= next_management_tick_ && s.rank == 1) {
            next_management_tick_ = stats_.ticks + 100;
            recruitment_ = 0;
            for (const auto &[id, f] : s.scene.world.world.facilities)
                if (f.placement.definition_id == 24) {
                    recruitment_ = id;
                    break;
                }
            if (count(s, true) < 4) {
                recipient_ = -1;
                int deficit = std::numeric_limits<int>::max();
                for (const auto &h : s.rules->humans) {
                    if (!homeless(s, h.identity) || cash(s) < reserve(s) + h.residence_fee)
                        continue;
                    const auto d = std::max(0, h.residence_threshold -
                                                   s.shop_humans.at(h.identity).satisfaction);
                    if (d < deficit) {
                        deficit = d;
                        recipient_ = h.identity;
                    }
                }
                if (recipient_ >= 0 && eligible(s, recipient_) && recruitment_ &&
                    s.scene.world.world.facilities.at(recruitment_).status == 1) {
                    auto c = command(Kind::open_facility);
                    c.facility = recruitment_;
                    return c;
                }
                if (recipient_ >= 0 && !eligible(s, recipient_)) {
                    const auto info = sim::startup_world_human_details(s, recipient_);
                    if (info && info->live_actor &&
                        s.scene.world.world.ai.battle.actors.at(*info->live_actor).control.state !=
                            4) {
                        gifting_ = true;
                        auto c = command(Kind::open_human);
                        c.actor = *info->live_actor;
                        c.definition = recipient_;
                        return c;
                    }
                }
            }
            // One real, roadside, single-cell build at a time. Prefer distinct shop
            // definitions, then low two-month capital cost, while reserving residence cash.
            const auto anchor = free_roadside(s);
            const auto catalog = sim::startup_world_build_catalog(s);
            if (anchor && catalog) {
                int selected = -1;
                std::int64_t best = std::numeric_limits<std::int64_t>::max();
                const bool housing = count(s, true) < 4 && !recruitment_ && recipient_ >= 0 &&
                                     eligible(s, recipient_);
                for (const auto &group : *catalog)
                    for (const int id : group) {
                        const auto &d = s.rules->facilities.at(id);
                        if (d.shape != 0 ||
                            (housing ? id != 24
                                     : ((d.kind != 3 && d.kind != 9) || count(s, false) >= 10)))
                            continue;
                        const auto q = sim::startup_world_build_quote(s, id);
                        require(q.has_value(), "catalog building lacks quote");
                        const auto housing_cash =
                            housing ? s.rules->humans.at(recipient_).residence_fee : 0;
                        const auto capital = q->construction_cost + 2 * q->definition_attributes[3];
                        if (cash(s) < reserve(s) + capital + housing_cash)
                            continue;
                        const bool exists =
                            std::any_of(s.scene.world.world.facilities.begin(),
                                        s.scene.world.world.facilities.end(), [&](const auto &f) {
                                            return f.second.placement.definition_id == id;
                                        });
                        const auto score = capital + (exists ? 1000000 : 0);
                        if (score < best) {
                            best = score;
                            selected = id;
                        }
                    }
                if (selected >= 0) {
                    build_definition_ = selected;
                    build_anchor_ = *anchor;
                    build_committed_ = false;
                    return command(Kind::open_build_menu);
                }
            }
        }
        if (s.rank == 1 && !s.active_task && month(s) >= next_task_month_ &&
            (s.task_progress.successes < 12 || s.popularity < 800) &&
            std::any_of(s.task_order.begin(), s.task_order.end(),
                        [&](auto id) { return affordable_task(s, id); }))
            return command(Kind::open_task_menu);
        return {};
    }
    const int raw = p.legacy_page;
    if (raw == 21) {
        auto c = command(Kind::select_build_menu, p.id);
        c.definition = build_definition_;
        return c;
    }
    if (raw >= 51 && raw <= 54) {
        const auto view = sim::inspect_startup_world_village_activity_page(s, p.id);
        if (!view || s.activity_page_answers.count(p.id))
            return {};
        if (raw == 51) {
            int chosen = -1;
            for (int n = 0; n < static_cast<int>(view->entries.size()); ++n) {
                const auto &a = s.rules->activities.at(view->entries[n]);
                if ((s.rank >= 2 ? a.identity == 30 : a.parameters[2] <= 2) &&
                    a.parameters[4] <= s.village_points && s.quarter_counter > 0) {
                    if (chosen < 0 || a.parameters[2] == 2)
                        chosen = n;
                }
            }
            if (chosen < 0)
                return activity(p.id, Activity::cancel);
            return activity(p.id, view->selection == chosen ? Activity::confirm : Activity::select,
                            chosen);
        }
        return raw == 53 && view->counter < 120
                   ? std::nullopt
                   : std::optional<Command>{activity(p.id, Activity::confirm)};
    }
    if (raw == 60 || raw == 64 || raw == 65 || raw == 66 || raw == 68 || raw == 69 || raw == 70) {
        if (!sim::startup_world_human_page_ready(s, p.id))
            return {};
        if (raw == 60)
            return human(p.id, gifting_ ? Human::gifts : Human::cancel);
        if (raw == 64) {
            if (s.human_page_answers.count(p.id))
                return {};
            const auto choice = recipient_ >= 0 && !eligible(s, recipient_)
                                    ? gift(s, p.id, recipient_)
                                    : std::nullopt;
            if (!gifting_ || !choice) {
                gifting_ = false;
                return human(p.id, Human::cancel);
            }
            if (s.page_phases.at(p.id) != choice->slot)
                return human(p.id, Human::equipment_slot, choice->slot);
            return human(p.id,
                         s.human_page_selections.at(p.id) == choice->index ? Human::confirm
                                                                           : Human::select,
                         choice->index);
        }
        if (raw == 70 && s.page_phases.at(p.id) == 2 && s.human_page_selections.at(p.id) != 1)
            return human(p.id, Human::select, 1);
        return human(p.id, Human::confirm);
    }
    if (raw == 22) {
        const auto f = s.task_page_lists.find(p.id);
        if (f == s.task_page_lists.end())
            return {};
        const auto choice = std::find_if(f->second.begin(), f->second.end(),
                                         [&](auto id) { return affordable_task(s, id); });
        return choice == f->second.end()
                   ? task(p.id, Task::cancel)
                   : task(p.id, Task::confirm, static_cast<int>(choice - f->second.begin()));
    }
    if (raw == 23)
        return task(p.id, Task::confirm);
    if (raw == 25)
        return task(p.id, Task::depart);
    if (raw == 28)
        return s.page_phases.at(p.id) == 0 ? std::optional<Command>{task(p.id, Task::confirm)}
                                           : std::nullopt;
    if (raw == 33)
        return task(p.id, Task::confirm, cash(s) - reserve(s) >= p.legacy_f ? 0 : 1);
    if (raw == 48) {
        auto c = command(Kind::rank_action, p.id);
        c.cancel = !ready(s);
        return c;
    }
    if (raw == 74) {
        auto c = command(Kind::facility_action, p.id);
        c.facility_action = s.facility_page_bindings.at(p.id) == recruitment_ && recipient_ >= 0 &&
                                    eligible(s, recipient_)
                                ? sim::StartupFacilityPageAction::confirm
                                : sim::StartupFacilityPageAction::cancel;
        return c;
    }
    if (raw == 80) {
        auto c = command(Kind::residence_action, p.id);
        const auto &list = s.residence_page_candidates.at(p.id);
        c.selection = recipient_;
        c.cancel = std::find(list.begin(), list.end(), recipient_) == list.end() ||
                   cash(s) < reserve(s) + s.rules->humans.at(recipient_).residence_fee;
        return c;
    }
    if (raw == 82)
        return s.facility_catalog_pages_initialized.count(p.id)
                   ? std::optional<Command>{command(Kind::facility_catalog_action, p.id)}
                   : std::nullopt;
    if (raw == 87) {
        auto c = command(Kind::award_action, p.id);
        const auto f = s.award_termination_pending.find(p.id);
        c.award_action = f != s.award_termination_pending.end() && f->second
                             ? ref::WorldAwardAction::confirm_termination
                             : ref::WorldAwardAction::request_termination;
        return c;
    }
    if (raw == 83)
        return command(Kind::cancel_page, p.id);
    if (raw == 90)
        return s.tax_page_residents.count(p.id)
                   ? std::optional<Command>{command(Kind::tax_action, p.id)}
                   : std::nullopt;
    const std::set<int> automatic{16, 24, 56, 57, 97, 98};
    if (automatic.count(raw))
        return {};
    const std::set<int> ordinary{0,  1,  11, 15, 30, 31, 32, 49, 50, 59,
                                 67, 81, 88, 89, 94, 95, 96, 99, 100};
    require(ordinary.count(raw) != 0, "uncovered decision page raw=" + std::to_string(raw));
    return command(Kind::acknowledge_page, p.id);
}

void ActiveLateVillageStrategy::observe_world(const State &before, const State &after) {
    stats_.minimum_cash = std::min(stats_.minimum_cash, cash(after));
    require(cash(after) >= 0, "cash became negative; " + diagnose(after));
    for (const auto &[id, f] : after.scene.world.world.facilities) {
        const auto old = before.scene.world.world.facilities.find(id);
        if (old != before.scene.world.world.facilities.end() && f.sales > old->second.sales) {
            stats_.facility_income += f.sales - old->second.sales;
            if (stats_.buildings.count(id))
                stats_.new_shop_income += f.sales - old->second.sales;
        }
    }
    if (!before.active_task && after.active_task) {
        require(requested_task_ == *after.active_task && !after.participants.empty(),
                "unbound departure");
        stats_.departed_task = *after.active_task;
        requested_task_ = 0;
        ++stats_.task_departures;
    }
    if (after.task_progress.successes > before.task_progress.successes) {
        require(after.task_progress.successes == before.task_progress.successes + 1 &&
                    stats_.departed_task &&
                    stats_.successful_tasks.insert(stats_.departed_task).second,
                "task success has no unique submitted departure");
        ++stats_.task_successes;
    }
    if (before.active_task && !after.active_task)
        next_task_month_ = month(after) + 1;
    if (after.rank > before.rank) {
        require(before.rank == 1 && after.rank == 2 && ready(before) &&
                    stats_.second_star_conditions,
                "second star bypassed genuine conditions/application");
        stats_.promoted_month = month(after);
    }
    // Equipment is consumed by the parent update after65 returns. Validate at that commit,
    // not at the click on65, and retain the real stock/charge receipt.
    if (top(before).legacy_page == 64 && before.human_page_answers.count(top(before).id) &&
        before.human_page_answers.at(top(before).id) == 0 && top(after).legacy_page == 66) {
        const auto id = top(before).id;
        const int person = before.page_human_bindings.at(id);
        const auto choice = before.human_equipment_choices.at(id);
        const auto e = equipment(before, choice[0], choice[1]);
        const auto cost = e ? ref::human_equipment_gift_cost(*e) : std::nullopt;
        require(cost && cash(before) - cash(after) == std::max(0, *cost) &&
                    after.shop_humans.at(person).satisfaction >
                        before.shop_humans.at(person).satisfaction,
                "gift must charge its current quote and actually cultivate its recipient");
        if (*cost == -1) {
            const int kind = choice[0] == 0 ? 1 : choice[0] == 3 ? 3 : 2;
            require(before.catalog.at({kind, choice[1]}).free_purchases ==
                        after.catalog.at({kind, choice[1]}).free_purchases + 1,
                    "stock gift must consume one real inventory unit");
        }
        stats_.gift_cost += std::max(0, *cost);
        ++stats_.gifts;
    }
}

void ActiveLateVillageStrategy::observe(const State &before, const Command &c,
                                        const app::WorldCommandResult &result, const State &after) {
    require(result.outcome == app::WorldCommandOutcome::applied &&
                result.runtime_error == sim::StartupWorldRuntimeError::none &&
                result.denial == ref::TaskCommandDenial::none &&
                result.build_denial == sim::StartupBuildDenial::none,
            "command rejected kind=" + std::to_string(static_cast<int>(c.kind)) + "; " +
                diagnose(before));
    ++stats_.commands;
    if (c.kind == Kind::confirm_build) {
        const auto q = sim::startup_world_build_quote(before, build_definition_);
        require(result.created && q && cash(before) - cash(after) == q->construction_cost,
                "construction must charge once at current quote");
        const auto id = *result.created;
        require(after.scene.world.world.facilities.at(id).placement.anchor == build_anchor_ &&
                    !(after.facility_flags.at(id) & 1U),
                "planned roadside building is disconnected");
        stats_.construction_cost += q->construction_cost;
        if (build_definition_ == 24)
            recruitment_ = id;
        else
            require(stats_.buildings.insert(id).second, "duplicate construction receipt");
        build_committed_ = true;
    }
    if (c.kind == Kind::residence_action && !c.cancel) {
        require(result.created && eligible(before, recipient_) &&
                    cash(before) - cash(after) ==
                        before.rules->humans.at(recipient_).residence_fee &&
                    after.human_homes.at(recipient_)[2] == 1 &&
                    !after.scene.world.world.facilities.count(recruitment_),
                "invalid real residence replacement");
        require(stats_.residents.insert(recipient_).second, "same resident admitted twice");
        ++stats_.admissions;
        recruitment_ = 0;
        recipient_ = -1;
    }
    if (c.kind == Kind::task_action && c.task_action == Task::confirm &&
        top(before).legacy_page == 28 && before.page_phases.at(c.page) == 0) {
        require(top(before).task_identity.has_value(), "task confirmation has no identity");
        requested_task_ = *top(before).task_identity;
    }
    if (c.kind == Kind::rank_action && !c.cancel) {
        require(ready(before), "premature second-star application");
        stats_.second_star_conditions = true;
    }
    if (c.kind == Kind::village_activity_action && c.village_activity_action == Activity::confirm) {
        const auto v = sim::inspect_startup_world_village_activity_page(before, c.page);
        require(v.has_value(), "activity lacks initialized page");
        if (v->raw == 52 && v->activity && *v->activity == 30) {
            require(before.rank == 2 && before.scripts.activities.at(30).status != 0 &&
                        before.village_points - after.village_points ==
                            before.rules->activities.at(30).parameters[4] &&
                        activity_count(after, 30) == activity_count(before, 30) + 1,
                    "pot import must be genuinely unlocked and paid");
            stats_.pot_paid = true;
        }
        if (v->raw == 53 && v->counter >= 120 && top(after).id != c.page) {
            ++stats_.activities;
            if (v->activity && *v->activity == 30) {
                require(stats_.pot_paid, "pot import completed without payment");
                stats_.pot_month = month(after);
                stats_.pot_income = stats_.facility_income;
            }
        }
    }
    observe_world(before, after);
}

void ActiveLateVillageStrategy::observe_tick(const State &before, const State &after) {
    ++stats_.ticks;
    require(month(after) >= month(before) && month(after) <= month(before) + 1 &&
                ref::valid_world_calendar_state(after.scene.calendar) &&
                after.scene.random.draws() >= before.scene.random.draws(),
            "calendar/random discontinuity");
    observe_world(before, after);
}
bool ActiveLateVillageStrategy::checkpoint(const State &s) const {
    return stats_.admissions > 0 && stats_.new_shop_income > 0 && stats_.task_successes > 0 &&
           top(s).kind == ref::WorldScriptPageKind::scene && s.scene.scene_state == 0 &&
           !s.build_definition && !s.active_task && s.activity_pages_initialized.empty() &&
           std::all_of(s.scene.world.world.facilities.begin(), s.scene.world.world.facilities.end(),
                       [](const auto &f) {
                           return (f.second.kind != 12 && f.second.kind != 3 &&
                                   f.second.kind != 9) ||
                                  f.second.status == 1;
                       });
}
bool ActiveLateVillageStrategy::complete(const State &s) const {
    return stats_.second_star_conditions && stats_.promoted_month >= 0 && stats_.pot_paid &&
           stats_.pot_month >= 0 && s.rank == 2 && count(s, true, true) >= 4 &&
           count(s, false, true) >= 10 && s.task_progress.successes >= 12 &&
           month(s) >= stats_.pot_month + 2 && stats_.facility_income > stats_.pot_income &&
           checkpoint(s);
}
std::string ActiveLateVillageStrategy::diagnose(const State &s) const {
    std::ostringstream out;
    out << "raw=" << top(s).legacy_page << " month=" << month(s) << " ticks=" << stats_.ticks
        << " cash=" << cash(s) << " reserve=" << reserve(s) << " rank=" << s.rank
        << " popularity=" << s.popularity << "/800 shops=" << count(s, false, true)
        << "/10 homes=" << count(s, true, true) << "/4 successes=" << s.task_progress.successes
        << "/12 gifts=" << stats_.gifts << " admissions=" << stats_.admissions
        << " active_task=" << s.active_task.value_or(0) << " points=" << s.village_points
        << " quarter_slots=" << s.quarter_counter << " promoted_month=" << stats_.promoted_month
        << " pot_month=" << stats_.pot_month;
    return out.str();
}
void ActiveLateVillageStrategy::encode(std::ostream &out) const {
    const auto &v = stats_;
    out << "ARK_ACTIVE_LATE_1\n"
        << v.commands << ' ' << v.ticks << ' ' << v.departed_task << ' ' << v.minimum_cash << ' '
        << v.facility_income << ' ' << v.new_shop_income << ' ' << v.construction_cost << ' '
        << v.gift_cost << ' ' << v.initial_successes << ' ' << v.task_successes << ' '
        << v.task_departures << ' ' << v.activities << ' ' << v.gifts << ' ' << v.admissions << ' '
        << v.promoted_month << ' ' << v.pot_month << ' ' << v.pot_income << ' '
        << v.second_star_conditions << ' ' << v.pot_paid << ' ' << requested_task_ << ' '
        << next_management_tick_ << ' ' << next_activity_tick_ << ' ' << next_task_month_ << ' '
        << initialized_ << '\n';
    const auto write = [&](const auto &values) {
        out << values.size();
        for (const auto id : values)
            out << ' ' << id;
        out << '\n';
    };
    write(v.buildings);
    write(v.successful_tasks);
    write(v.residents);
    require(bool(out), "cannot encode strategy evidence");
}
ActiveLateVillageStrategy ActiveLateVillageStrategy::decode(std::istream &in) {
    ActiveLateVillageStrategy result;
    auto &v = result.stats_;
    std::string magic;
    in >> magic;
    require(magic == "ARK_ACTIVE_LATE_1", "unknown late strategy evidence");
    in >> v.commands >> v.ticks >> v.departed_task >> v.minimum_cash >> v.facility_income >>
        v.new_shop_income >> v.construction_cost >> v.gift_cost >> v.initial_successes >>
        v.task_successes >> v.task_departures >> v.activities >> v.gifts >> v.admissions >>
        v.promoted_month >> v.pot_month >> v.pot_income >> v.second_star_conditions >> v.pot_paid >>
        result.requested_task_ >> result.next_management_tick_ >> result.next_activity_tick_ >>
        result.next_task_month_ >> result.initialized_;
    const auto read = [&](auto &values) {
        std::size_t size{};
        in >> size;
        require(bool(in) && size <= 10000, "invalid evidence collection size");
        for (std::size_t n = 0; n < size; ++n) {
            typename std::decay_t<decltype(values)>::value_type id{};
            in >> id;
            require(id >= 0 && values.insert(id).second, "duplicate/invalid evidence identity");
        }
    };
    read(v.buildings);
    read(v.successful_tasks);
    read(v.residents);
    require(bool(in) && v.minimum_cash >= 0 && v.gifts >= 0 && v.admissions >= 0 &&
                v.task_successes == static_cast<int>(v.successful_tasks.size()) &&
                v.admissions == static_cast<int>(v.residents.size()),
            "invalid late evidence totals");
    return result;
}
} // namespace ark::test
