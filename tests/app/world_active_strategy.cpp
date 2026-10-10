#include "world_active_strategy.hpp"

#include <algorithm>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>

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
        throw std::runtime_error("Active village: " + message);
}
const ref::WorldScriptPage &top(const State &s) {
    for (auto p = s.scripts.pages.rbegin(); p != s.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4)
            return *p;
    throw std::runtime_error("Active village lost its current page");
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
int activity_count(const State &s, int id) {
    const auto it = s.activity_counts.find(id);
    return it == s.activity_counts.end() ? 0 : it->second;
}
bool first_star_ready(const State &s) {
    return s.popularity >= 300 && s.maximum_income >= 5000 && s.events_held >= 2 &&
           std::any_of(
               s.scene.world.world.facilities.begin(), s.scene.world.world.facilities.end(),
               [](const auto &entry) { return entry.second.placement.definition_id == 35; });
}
bool affordable(const State &s, std::uint64_t id) {
    const auto found = s.tasks.find(id);
    return found != s.tasks.end() &&
           s.rules->tasks.at(found->second.definition).recruitment_fee <= cash(s);
}
} // namespace

void ActiveVillageStrategy::reconcile(const State &s) {
    require(s.rules != nullptr, "missing source rules");
    if (!initialized_) {
        stats_.minimum_cash = cash(s);
        initialized_ = true;
    }
    const auto &facilities = s.scene.world.world.facilities;
    if (stats_.bakery) {
        const auto found = facilities.find(stats_.bakery);
        require(found != facilities.end() && found->second.placement.definition_id == 35 &&
                    found->second.placement.anchor == ref::Position{12, 6},
                "saved P1 bakery identity/location differs from evidence");
        require(!stats_.bakery_completed || found->second.status == 1,
                "completed bakery reverted on player load");
        require(found->second.sales >= stats_.bakery_income, "bakery sales lost on player load");
    }
    require(s.task_progress.successes >= stats_.task_successes,
            "task success total lost on player load");
    require(stats_.promoted_month < 0 || s.rank >= 1, "first star lost on player load");
    require(stats_.exhibition_month < 0 || activity_count(s, 16) > 0,
            "consumed exhibition lost on player load");
    require(!stats_.gifts || (recipient_ >= 0 && s.human_presence.count(recipient_)),
            "gift recipient lost on player load");
    if (stats_.departed_task && s.active_task)
        require(*s.active_task == stats_.departed_task,
                "active adventure differs from submitted departure");
}

std::optional<Command> ActiveVillageStrategy::next(const State &s) {
    if (!initialized_)
        reconcile(s);
    const auto &p = top(s);
    // Initialization is a source update, never an input-side/render-side shortcut.
    if (p.lifecycle == 0)
        return {};
    if (s.build_definition) {
        require(*s.build_definition == 35, "unexpected selected building");
        auto c = command(stats_.bakery ? Kind::cancel_build : Kind::confirm_build);
        c.definition = 35;
        c.anchor = {12, 6}; // Approved P1, beside the existing road and sunflower.
        c.orientation = ref::FacilityOrientation::first;
        return c;
    }
    if (p.kind == ref::WorldScriptPageKind::scene) {
        if (s.scene.scene_state != 0)
            return {};
        const auto pending = std::find_if(
            s.scene.world.facility_order.begin(), s.scene.world.facility_order.end(), [&](auto id) {
                return s.scene.world.world.facility_uses
                    .at(s.scene.world.world.facilities.at(id).placement.definition_id)
                    .upgrade_pending;
            });
        if (pending != s.scene.world.facility_order.end()) {
            auto c = command(Kind::open_facility);
            c.facility = *pending;
            return c;
        }
        if (!stats_.bakery) {
            const auto quote = sim::startup_world_build_quote(s, 35);
            require(quote.has_value(), "initial bakery has no genuine quote");
            if (cash(s) >= quote->construction_cost)
                return command(Kind::open_build_menu);
            return {};
        }
        // The initial potato is a real consumable: the command must consume inventory
        // and raise this recipient's actual extra[0], not merely display growth text.
        if (!stats_.gifts && !s.scene.world.world.ai.human_order.empty()) {
            auto c = command(Kind::open_human);
            c.actor = s.scene.world.world.ai.human_order.front();
            c.definition = s.scene.world.world.ai.battle.actors.at(c.actor).definition;
            recipient_ = c.definition;
            return c;
        }
        const bool exhibition = s.rank >= 1 && stats_.exhibition_month < 0;
        const int cost = exhibition ? s.rules->activities.at(16).parameters[4] : 20;
        if (stats_.exhibition_month < 0 && s.quarter_counter > 0 && s.village_points >= cost &&
            stats_.ticks >= next_activity_tick_) {
            next_activity_tick_ = stats_.ticks + 300;
            return command(Kind::open_village_activities);
        }
        // Adventure costs are genuine quotes. Leave a complete calendar month between
        // tasks for recovery/business, as in the published natural-progression player.
        if (!s.active_task && month(s) >= next_task_month_ &&
            (s.task_progress.successes == 0 || (s.rank == 0 && s.popularity < 300)) &&
            std::any_of(s.task_order.begin(), s.task_order.end(),
                        [&](auto id) { return affordable(s, id); }))
            return command(Kind::open_task_menu);
        return {};
    }
    const int raw = p.legacy_page;
    if (raw == 11) {
        // Published event-message consumer only advances its counter to40 or closes it.
        // Its instruction22 continuation/reward belongs to the source script, never this
        // player: do not replay message_commands or treat this as a purchase prompt.
        return command(Kind::acknowledge_page, p.id);
    }
    if (raw == 21) {
        auto c = command(Kind::select_build_menu, p.id);
        c.definition = 35;
        return c;
    }
    if (raw >= 51 && raw <= 54) {
        const auto view = sim::inspect_startup_world_village_activity_page(s, p.id);
        if (!view)
            return {};
        if (raw == 51) {
            if (s.activity_page_answers.count(p.id))
                return {};
            int selected = -1;
            for (int n = 0; n < static_cast<int>(view->entries.size()); ++n) {
                const auto &a = s.rules->activities.at(view->entries[n]);
                if (s.rank >= 1 && a.identity != 16)
                    continue;
                if (stats_.exhibition_month < 0 && a.parameters[2] <= 2 &&
                    a.parameters[4] <= s.village_points && s.quarter_counter > 0) {
                    selected = n;
                    if (a.identity == 16)
                        break;
                }
            }
            if (selected < 0)
                return activity(p.id, Activity::cancel);
            return activity(
                p.id, view->selection == selected ? Activity::confirm : Activity::select, selected);
        }
        if (raw == 53 && view->counter < 120)
            return {};
        return activity(p.id, Activity::confirm);
    }
    if (raw == 60 || raw == 64 || raw == 66 || raw == 68 || raw == 69 || raw == 70) {
        if (!sim::startup_world_human_page_ready(s, p.id))
            return {};
        if (raw == 60)
            return human(p.id, stats_.gifts ? Human::cancel : Human::gifts);
        // A naturally mastered profession offers a transfer. This first-star player
        // keeps the current profession; it must explicitly choose No at the real prompt.
        if (raw == 70 && s.page_phases.at(p.id) == 2 && s.human_page_selections.at(p.id) != 1)
            return human(p.id, Human::select, 1);
        if (raw == 66 || raw == 68 || raw == 69 || raw == 70)
            return human(p.id, Human::confirm);
        if (s.human_page_answers.count(p.id))
            return {};
        if (stats_.gifts)
            return human(p.id, Human::cancel);
        if (s.page_phases.at(p.id) != 4)
            return human(p.id, Human::equipment_slot, 4);
        const auto &list = s.equipment_page_catalogs.at(p.id)[4];
        const auto found = std::find(list.begin(), list.end(), 0);
        require(found != list.end() && s.items.at(0).inventory > 0,
                "real starting potato is unavailable for planned cultivation");
        const int selected = static_cast<int>(found - list.begin());
        return human(p.id,
                     s.human_page_selections.at(p.id) == selected ? Human::confirm : Human::select,
                     selected);
    }
    if (raw == 22) {
        const auto found = s.task_page_lists.find(p.id);
        if (found == s.task_page_lists.end())
            return {};
        const auto &list = found->second;
        const auto choice =
            std::find_if(list.begin(), list.end(), [&](auto id) { return affordable(s, id); });
        require(choice != list.end(), "opened task list has no affordable adventure");
        return task(p.id, Task::confirm, static_cast<int>(choice - list.begin()));
    }
    if (raw == 23)
        return task(p.id, Task::confirm);
    if (raw == 25)
        return task(p.id, Task::depart);
    if (raw == 28)
        return s.page_phases.at(p.id) == 0 ? std::optional<Command>{task(p.id, Task::confirm)}
                                           : std::nullopt;
    if (raw == 33)
        return task(p.id, Task::confirm, cash(s) >= p.legacy_f ? 0 : 1);
    if (raw == 48) {
        auto c = command(Kind::rank_action, p.id);
        c.cancel = s.rank != 0 || !first_star_ready(s);
        return c;
    }
    if (raw == 74) {
        auto c = command(Kind::facility_action, p.id);
        c.facility_action = sim::StartupFacilityPageAction::cancel;
        return c;
    }
    if (raw == 82) {
        if (!s.facility_catalog_pages_initialized.count(p.id))
            return {};
        return command(Kind::facility_catalog_action, p.id);
    }
    if (raw == 87) {
        auto c = command(Kind::award_action, p.id);
        const auto pending = s.award_termination_pending.find(p.id);
        c.award_action = pending != s.award_termination_pending.end() && pending->second
                             ? ref::WorldAwardAction::confirm_termination
                             : ref::WorldAwardAction::request_termination;
        return c;
    }
    if (raw == 83)
        return command(Kind::cancel_page, p.id);
    if (raw == 90) {
        if (!s.tax_page_residents.count(p.id))
            return {};
        return command(Kind::tax_action, p.id);
    }
    const std::set<int> automatic{16, 24, 56, 57, 97, 98};
    if (automatic.count(raw))
        return {};
    const std::set<int> ordinary{0,  1,  15, 30, 31, 32, 49, 50, 59,
                                 67, 81, 88, 89, 94, 95, 96, 99, 100};
    require(ordinary.count(raw) != 0, "uncovered decision page raw=" + std::to_string(raw));
    return command(Kind::acknowledge_page, p.id);
}

void ActiveVillageStrategy::observe_world(const State &before, const State &after) {
    stats_.trade.observe(before, after);
    stats_.minimum_cash = std::min(stats_.minimum_cash, cash(after));
    require(cash(after) >= 0, "cash became negative; " + diagnose(after));
    for (const auto &[id, f] : after.scene.world.world.facilities) {
        const auto old = before.scene.world.world.facilities.find(id);
        if (old != before.scene.world.world.facilities.end() && f.sales > old->second.sales)
            stats_.facility_income += f.sales - old->second.sales;
    }
    if (stats_.bakery) {
        const auto &f = after.scene.world.world.facilities.at(stats_.bakery);
        stats_.bakery_completed = stats_.bakery_completed || f.status == 1;
        stats_.bakery_income = f.sales;
    }
    for (const auto &[definition, progress] : after.scene.world.world.facility_uses) {
        const auto old = before.scene.world.world.facility_uses.find(definition);
        if (old != before.scene.world.world.facility_uses.end() &&
            progress.level > old->second.level)
            stats_.upgraded_definitions.insert(definition);
    }
    if (!before.active_task && after.active_task) {
        require(requested_task_ && *after.active_task == requested_task_ &&
                    !after.participants.empty(),
                "actual departure must match the explicitly confirmed team and task");
        stats_.departed_task = *after.active_task;
        ++stats_.task_departures;
        requested_task_ = 0;
    }
    if (after.task_progress.successes > before.task_progress.successes) {
        require(after.task_progress.successes == before.task_progress.successes + 1 &&
                    stats_.departed_task != 0 &&
                    stats_.successful_tasks.insert(stats_.departed_task).second,
                "success must belong to one submitted departure and commit exactly once");
        ++stats_.task_successes;
    }
    if (before.active_task && !after.active_task) {
        next_task_month_ = month(after) + 2;
        if (!stats_.successful_tasks.count(*before.active_task))
            ++stats_.task_failures;
    }
    if (after.rank > before.rank) {
        require(before.rank == 0 && after.rank == 1 && first_star_ready(before),
                "promotion bypassed one of the four genuine first-star conditions");
        require(stats_.first_star_conditions && after.scripts.activities.at(16).status == 1,
                "first star needs player application and actual exhibition unlock");
        stats_.promoted_month = month(after);
    }
}

void ActiveVillageStrategy::observe(const State &before, const Command &c,
                                    const app::WorldCommandResult &result, const State &after) {
    require(result.outcome == app::WorldCommandOutcome::applied &&
                result.runtime_error == sim::StartupWorldRuntimeError::none &&
                result.denial == ref::TaskCommandDenial::none &&
                result.build_denial == sim::StartupBuildDenial::none,
            "command rejected kind=" + std::to_string(static_cast<int>(c.kind)) +
                " runtime=" + std::to_string(static_cast<int>(result.runtime_error)) +
                " task=" + std::to_string(static_cast<int>(result.denial)) + " build=" +
                std::to_string(static_cast<int>(result.build_denial)) + "; " + diagnose(before));
    ++stats_.commands;
    if (c.kind == Kind::confirm_build) {
        const auto quote = sim::startup_world_build_quote(before, 35);
        require(!stats_.bakery && result.created && quote &&
                    cash(before) - cash(after) == quote->construction_cost,
                "bakery must charge its current quote exactly once");
        stats_.bakery = *result.created;
        stats_.construction_cost = quote->construction_cost;
        require(after.scene.world.world.facilities.at(stats_.bakery).placement.anchor ==
                    ref::Position{12, 6},
                "construction did not install approved P1 position");
    }
    if (c.kind == Kind::human_action && c.human_action == Human::confirm &&
        top(before).legacy_page == 64 && before.page_phases.at(c.page) == 4) {
        require(!stats_.gifts && recipient_ >= 0 &&
                    before.items.at(0).inventory == after.items.at(0).inventory + 1 &&
                    after.scene.world.world.ai.growth.at(recipient_).definition.extra[0] ==
                        before.scene.world.world.ai.growth.at(recipient_).definition.extra[0] + 4 &&
                    cash(before) == cash(after),
                "potato must consume inventory and apply its published growth without cash");
        ++stats_.gifts;
    }
    if (c.kind == Kind::task_action && c.task_action == Task::confirm &&
        top(before).legacy_page == 28 && before.page_phases.at(c.page) == 0) {
        require(top(before).task_identity.has_value() && c.selection == 0,
                "departure confirmation needs its source task identity");
        requested_task_ = *top(before).task_identity;
    }
    if (c.kind == Kind::rank_action && !c.cancel && first_star_ready(before))
        stats_.first_star_conditions = true;
    if (c.kind == Kind::village_activity_action && c.village_activity_action == Activity::confirm) {
        const auto view = sim::inspect_startup_world_village_activity_page(before, c.page);
        require(view.has_value(), "activity input has no observed initialized page");
        if (view->raw == 52 && view->activity && *view->activity == 16) {
            require(before.rank >= 1 && before.scripts.activities.at(16).status == 1 &&
                        before.village_points - after.village_points ==
                            before.rules->activities.at(16).parameters[4] &&
                        activity_count(after, 16) == activity_count(before, 16) + 1,
                    "new exhibition must actually consume its published point quote");
            stats_.exhibition_paid = true;
        }
        if (view->raw == 53 && view->counter >= 120 && top(after).id != c.page) {
            ++stats_.activities;
            if (view->activity && *view->activity == 16 && stats_.exhibition_month < 0) {
                require(stats_.exhibition_paid, "exhibition completed without paid application");
                stats_.exhibition_month = month(after);
                stats_.exhibition_income = stats_.facility_income;
            }
        }
    }
    observe_world(before, after);
}

void ActiveVillageStrategy::observe_tick(const State &before, const State &after) {
    ++stats_.ticks;
    require(month(after) >= month(before) && month(after) <= month(before) + 1 &&
                ref::valid_world_calendar_state(after.scene.calendar),
            "calendar discontinuity");
    require(after.scene.random.draws() >= before.scene.random.draws(),
            "random stream rewound within one process");
    observe_world(before, after);
}

bool ActiveVillageStrategy::construction_checkpoint(const State &s) const {
    return stats_.bakery && stats_.bakery_completed && stats_.bakery_income > 0 && stats_.gifts &&
           top(s).kind == ref::WorldScriptPageKind::scene && s.scene.scene_state == 0 &&
           !s.build_definition;
}
bool ActiveVillageStrategy::complete(const State &s) const {
    return construction_checkpoint(s) && stats_.task_successes > 0 &&
           stats_.first_star_conditions && stats_.promoted_month >= 0 && stats_.activities >= 3 &&
           stats_.exhibition_paid && stats_.exhibition_month >= 0 &&
           month(s) >= stats_.exhibition_month + 2 &&
           stats_.trade.full_month_after(stats_.exhibition_month, month(s)) &&
           stats_.facility_income > stats_.exhibition_income &&
           !stats_.upgraded_definitions.empty() && !s.active_task &&
           s.activity_pages_initialized.empty();
}

std::string ActiveVillageStrategy::diagnose(const State &s) const {
    std::ostringstream out;
    out << "raw=" << top(s).legacy_page << " month=" << month(s) << " ticks=" << stats_.ticks
        << " commands=" << stats_.commands << " cash=" << cash(s)
        << " minimum_cash=" << stats_.minimum_cash << " bakery=" << stats_.bakery
        << " bakery_income=" << stats_.bakery_income
        << " facility_income=" << stats_.facility_income << " rank=" << s.rank
        << " popularity=" << s.popularity << "/300 income_record=" << s.maximum_income
        << "/5000 events=" << s.events_held << "/2 points=" << s.village_points
        << " quarter_slots=" << s.quarter_counter << " tasks_available=" << s.task_order.size()
        << " task=" << s.active_task.value_or(0) << " departures=" << stats_.task_departures
        << " victories=" << stats_.task_successes << " failures=" << stats_.task_failures
        << " gifts=" << stats_.gifts << " activities=" << stats_.activities
        << " promoted_month=" << stats_.promoted_month
        << " exhibition_month=" << stats_.exhibition_month;
    return out.str();
}

void ActiveVillageStrategy::encode(std::ostream &out) const {
    const auto &v = stats_;
    out << "ARK_ACTIVE_VILLAGE_2\n"
        << v.commands << ' ' << v.ticks << ' ' << v.bakery << ' ' << v.departed_task << ' '
        << v.construction_cost << ' ' << v.minimum_cash << ' ' << v.facility_income << ' '
        << v.bakery_income << ' ' << v.activities << ' ' << v.gifts << ' ' << v.task_departures
        << ' ' << v.task_successes << ' ' << v.task_failures << ' ' << v.promoted_month << ' '
        << v.exhibition_month << ' ' << v.exhibition_income << ' ' << v.bakery_completed << ' '
        << v.first_star_conditions << ' ' << v.exhibition_paid << ' ' << next_activity_tick_ << ' '
        << requested_task_ << ' ' << next_task_month_ << ' ' << recipient_ << ' ' << initialized_
        << '\n';
    out << v.successful_tasks.size();
    for (const auto id : v.successful_tasks)
        out << ' ' << id;
    out << '\n' << v.upgraded_definitions.size();
    for (const auto id : v.upgraded_definitions)
        out << ' ' << id;
    out << '\n';
    v.trade.encode(out);
    require(bool(out), "cannot write strategy evidence");
}

ActiveVillageStrategy ActiveVillageStrategy::decode(std::istream &in) {
    ActiveVillageStrategy result;
    auto &v = result.stats_;
    std::string magic;
    in >> magic;
    require(magic == "ARK_ACTIVE_VILLAGE_2", "unknown strategy evidence format");
    in >> v.commands >> v.ticks >> v.bakery >> v.departed_task >> v.construction_cost >>
        v.minimum_cash >> v.facility_income >> v.bakery_income >> v.activities >> v.gifts >>
        v.task_departures >> v.task_successes >> v.task_failures >> v.promoted_month >>
        v.exhibition_month >> v.exhibition_income >> v.bakery_completed >>
        v.first_star_conditions >> v.exhibition_paid >> result.next_activity_tick_ >>
        result.requested_task_ >> result.next_task_month_ >> result.recipient_ >>
        result.initialized_;
    std::size_t size{};
    in >> size;
    require(bool(in) && size <= 10000, "invalid task evidence count");
    for (std::size_t n = 0; n < size; ++n) {
        std::uint64_t id{};
        in >> id;
        require(id && v.successful_tasks.insert(id).second, "duplicate successful task evidence");
    }
    in >> size;
    require(bool(in) && size <= 1000, "invalid upgrade evidence count");
    for (std::size_t n = 0; n < size; ++n) {
        int id{};
        in >> id;
        require(id >= 0 && v.upgraded_definitions.insert(id).second, "duplicate upgrade evidence");
    }
    v.trade.decode(in);
    require(bool(in) && v.gifts >= 0 && v.gifts <= 1 && v.activities >= 0 &&
                v.task_successes == static_cast<int>(v.successful_tasks.size()) &&
                v.task_departures >= v.task_successes && v.task_failures >= 0 &&
                v.minimum_cash >= 0 && v.bakery_income >= 0 && v.facility_income >= 0,
            "invalid strategy evidence totals");
    return result;
}
} // namespace ark::test
