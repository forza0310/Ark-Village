#include "world_human_inspection.hpp"
#include "ark/simulation/rules/human_management.hpp"
#include "ark/simulation/startup_world_building.hpp"
#include "ark/simulation/startup_world_human.hpp"
#include "ark/simulation/startup_world_tax.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Error = simulation::StartupWorldRuntimeError;
using Action = simulation::StartupHumanPageAction;
using Kind = simulation::rules::WorldScriptPageKind;
namespace rules = simulation::rules;
const rules::WorldScriptPage *top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                [](const auto &page) { return page.lifecycle != 4; });
    return p == s.scripts.pages.rend() ? nullptr : &*p;
}
void require(Error error, const char *action) {
    if (error != Error::none)
        throw std::runtime_error(std::string("Human inspection ") + action +
                                 " failed: source=" + std::to_string(static_cast<int>(error)));
}
bool tax_mode(const std::string &mode) {
    return mode == "world-tax" || mode == "world-tax-collected";
}
int human_tab(const std::string &mode) {
    if (mode == "world-human")
        return 0;
    if (mode == "world-human-attributes")
        return 1;
    if (mode == "world-human-equipment")
        return 2;
    if (mode == "world-human-spells")
        return 3;
    return -1;
}
int gift_slot(const std::string &mode) {
    if (mode == "world-gifts-armor")
        return 1;
    if (mode == "world-gifts-shield")
        return 2;
    if (mode == "world-gifts-accessory")
        return 3;
    return 0;
}
bool gift_catalogue(const std::string &mode) {
    return mode == "world-gifts" || mode == "world-gifts-armor" || mode == "world-gifts-shield" ||
           mode == "world-gifts-accessory";
}
bool profession_mode(const std::string &mode) {
    return mode == "world-professions" || mode == "world-profession-preview" ||
           mode == "world-profession-cancel" || mode == "world-profession-change" ||
           mode == "world-profession-complete";
}
bool profession_commit_mode(const std::string &mode) {
    return mode == "world-profession-change" || mode == "world-profession-complete";
}
bool profession_available(const State &s, int human, int job, bool commit = false) {
    const auto &j = s.rules->jobs.at(job);
    const auto gate = rules::check_human_profession_change(
        commit ? rules::HumanProfessionEntry::confirmation62
               : rules::HumanProfessionEntry::catalogue61,
        s.scene.world.world.ai.growth.at(human).definition.current_profession,
        s.human_calendar.at(human).celebrations, s.village_points,
        {job, s.scripts.professions.at(job).status, j.sort_order, j.script_extra, j.change_points,
         j.required_medals});
    return gate.error == rules::HumanGrowthError::none &&
           gate.denial == rules::HumanManagementDenial::none;
}
bool can_preview_profession(const State &s, int human, bool commit) {
    std::vector<rules::HumanManagementProfession> jobs;
    for (std::size_t n = 0; n < s.rules->jobs.size(); ++n) {
        const auto &j = s.rules->jobs[n];
        jobs.push_back({static_cast<int>(n), s.scripts.professions.at(static_cast<int>(n)).status,
                        j.sort_order, j.script_extra, j.change_points, j.required_medals});
    }
    const auto list = rules::catalogue_human_professions(s.rules->humans.at(human).sex, jobs);
    return list && std::any_of(list->begin(), list->end(), [&](int job) {
               return profession_available(s, human, job, commit);
           });
}
int affordable_gift(const State &s, std::uint64_t page, int human, int slot) {
    const auto &list = s.equipment_page_catalogs.at(page)[slot];
    const int kind = slot == 0 ? 1 : slot == 3 ? 3 : 2;
    for (std::size_t n = 0; n < list.size(); ++n) {
        const int item = list[n];
        if (s.shop_humans.at(human).equipment[slot] == item)
            continue;
        const auto d =
            std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(),
                         [&](const auto &e) { return e.shop.kind == kind && e.shop.id == item; });
        if (d == s.rules->equipment.end())
            continue;
        const auto &current = s.catalog.at({kind, item});
        const auto cost = rules::human_equipment_gift_cost(
            {item, slot, current.status, d->gift_order, d->reward_difficulty, d->shop.price,
             d->gift_rating, current.free_purchases, d->shop.combat});
        if (cost && (*cost == -1 || *cost <= s.scene.world.world.ai.accounting.funds()))
            return static_cast<int>(n);
    }
    throw std::runtime_error(
        "Human inspection has no affordable different equipment in real catalogue");
}
void act(State &s, std::uint64_t page, Action action, int selection = 0) {
    require(simulation::act_startup_world_human_page(s, page, action, selection), "player choice");
}
void capture_choice(const State &s, WorldHumanInspection &i) {
    i.before_choice = simulation::startup_world_human_details(s, *i.human);
    if (!i.before_choice)
        throw std::runtime_error("Human inspection lost chosen human before confirmation");
    i.cash_before_choice = s.scene.world.world.ai.accounting.funds();
    i.draws_before_choice = s.scene.random.draws();
    i.points_before_choice = s.village_points;
}
void verify_cancelled_choice(const State &s, const std::string &mode,
                             const WorldHumanInspection &i) {
    const auto after = simulation::startup_world_human_details(s, *i.human);
    if (!i.before_choice || !after)
        throw std::runtime_error("Human inspection cancellation lost human details");
    const auto &before = *i.before_choice;
    const bool unchanged =
        after->profession == before.profession && after->attributes == before.attributes &&
        after->combat == before.combat && after->equipment == before.equipment &&
        after->effort == before.effort && after->satisfaction == before.satisfaction &&
        s.village_points == i.points_before_choice &&
        s.scene.world.world.ai.accounting.funds() == i.cash_before_choice &&
        s.scene.random.draws() == i.draws_before_choice;
    const bool stock_unchanged =
        mode != "world-gift-cancel" ||
        (i.equipment && s.catalog.at({1, *i.equipment}).free_purchases == i.stock_before_choice);
    if (!unchanged || !stock_unchanged)
        throw std::runtime_error(
            "Human inspection cancelled confirmation changed actual human/resources/random");
}

// The published natural_housing scenario's player policy: buy a recruitment plot,
// gift the original short sword while reserving the residence fee, then admit an eligible
// person. Construction, satisfaction, tax preparation and cash posting remain source updates.
bool housing_input(State &s, const rules::WorldScriptPage &page, WorldHumanInspection &i) {
    if (i.tax_pending && page.legacy_page != 98) {
        if (s.scene.world.world.ai.accounting.funds() != i.cash_before_tax + i.expected_tax ||
            s.monthly_cash.at(s.scene.calendar.month)[4][0] != i.income_before_tax + i.expected_tax)
            throw std::runtime_error(
                "Human inspection automatic tax posting differs from actual resident sum");
        i.tax_pending = false;
        i.tax_collected = true;
    }
    if (page.kind == Kind::scene && s.scene.scene_state == 0 && !s.scene.framework_paused &&
        !i.home && i.recruitment && s.scene.world.world.facilities.at(*i.recruitment).status == 1) {
        const auto eligible =
            std::find_if(s.rules->humans.begin(), s.rules->humans.end(), [&](const auto &h) {
                return s.human_presence.at(h.identity) != 0 &&
                       s.human_homes.at(h.identity)[2] == 0 &&
                       s.shop_humans.at(h.identity).satisfaction >= h.residence_threshold &&
                       s.scene.world.world.ai.accounting.funds() >= h.residence_fee;
            });
        if (eligible != s.rules->humans.end()) {
            i.resident = eligible->identity;
            require(simulation::open_startup_world_facility_page(s, *i.recruitment),
                    "open recruitment");
        } else if (s.human_presence.at(1) != 0 &&
                   s.shop_humans.at(1).satisfaction < s.rules->humans.at(1).residence_threshold &&
                   s.scene.world.world.ai.accounting.funds() >=
                       s.rules->humans.at(1).residence_fee + 150) {
            i.housing_gift = true;
            require(simulation::open_startup_world_human_page(s, 1),
                    "open actual housing gift recipient");
        }
        return true;
    }
    if (page.legacy_page == 60) {
        act(s, page.id, i.housing_gift ? Action::gifts : Action::cancel);
        return true;
    }
    if (page.legacy_page == 64 && i.housing_gift) {
        if (s.human_page_answers.count(page.id))
            return true;
        if (s.shop_humans.at(1).satisfaction >= s.rules->humans.at(1).residence_threshold ||
            s.scene.world.world.ai.accounting.funds() < s.rules->humans.at(1).residence_fee + 150) {
            act(s, page.id, Action::cancel);
            i.housing_gift = false;
        } else {
            const auto &list = s.equipment_page_catalogs.at(page.id)[0];
            const auto selected = std::find(list.begin(), list.end(), 0);
            if (selected == list.end())
                throw std::runtime_error(
                    "Human inspection lost original short sword from catalogue");
            const int choice = static_cast<int>(selected - list.begin());
            act(s, page.id,
                s.human_page_selections.at(page.id) == choice ? Action::confirm : Action::select,
                choice);
        }
        return true;
    }
    if (page.legacy_page == 65 && i.housing_gift) {
        act(s, page.id, Action::confirm);
        return true;
    }
    if (page.legacy_page == 74) {
        require(simulation::act_startup_world_facility_page(
                    s, page.id,
                    !i.home && s.facility_page_bindings.at(page.id) == i.recruitment
                        ? simulation::StartupFacilityPageAction::confirm
                        : simulation::StartupFacilityPageAction::cancel),
                "recruitment details");
        return true;
    }
    if (page.legacy_page == 80 && i.resident) {
        const auto admitted = simulation::act_startup_world_residence_page(s, page.id, *i.resident);
        require(admitted.error, "admit actual resident");
        if (admitted.denial != simulation::StartupBuildDenial::none || !admitted.created)
            throw std::runtime_error("Human inspection real residence command was denied");
        i.home = admitted.created;
        return true;
    }
    if (page.legacy_page == 90) {
        const auto view = simulation::inspect_startup_world_tax_page(s, page.id);
        if (!view)
            return true;
        if (!i.home || s.scene.world.world.facilities.at(*i.home).status != 1 || view->total <= 0)
            throw std::runtime_error(
                "Human inspection tax page has no naturally completed taxable residence");
        i.tax_confirmed = true;
        require(simulation::act_startup_world_tax_page(s, page.id,
                                                       simulation::StartupWorldTaxAction::confirm),
                "confirm real tax list");
        return true;
    }
    if (page.legacy_page == 98) {
        if (!i.tax_confirmed)
            throw std::runtime_error("Human inspection tax posting has no confirmed list");
        i.tax_pending = true;
        i.cash_before_tax = s.scene.world.world.ai.accounting.funds();
        i.income_before_tax = s.monthly_cash.at(s.scene.calendar.month)[4][0];
        i.tax_month = s.scene.calendar.year * 12 + s.scene.calendar.month;
        i.expected_tax = 0;
        for (const auto &h : s.rules->humans)
            if (s.human_presence.at(h.identity) != 0 && s.human_homes.at(h.identity)[2] == 1)
                i.expected_tax += s.human_calendar.at(h.identity).legacy_G;
        return true; // raw98 owns automatic posting; never supply confirmation.
    }
    return false;
}
} // namespace

bool human_inspection_mode(const std::string &mode) {
    return human_tab(mode) >= 0 || gift_catalogue(mode) || tax_mode(mode) ||
           profession_mode(mode) || mode == "world-gift-confirm" || mode == "world-gift-cancel" ||
           mode == "world-equipment-info" || mode == "world-gift-result" ||
           mode == "world-equipment-change";
}
void begin_human_inspection(State &s, const std::string &mode, WorldHumanInspection &i) {
    if (!tax_mode(mode))
        return;
    const auto &map = s.scene.world.world.map;
    const auto bounds = s.rules->fences.at(s.fence_level);
    std::optional<rules::Position> anchor;
    for (int y = bounds[1].y + 1; y < bounds[0].y && !anchor; ++y)
        for (int x = bounds[0].x + 1; x < bounds[1].x && !anchor; ++x)
            if (!map.cells.at(y * map.width + x).facility)
                anchor = rules::Position{x, y};
    if (!anchor)
        throw std::runtime_error("Human inspection has no vacant recruitment site");
    require(simulation::open_startup_world_build_menu(s), "open actual building catalogue");
    const auto selected = simulation::select_startup_world_build_menu(s, top(s)->id, 24);
    require(selected.error, "select recruitment");
    const auto placed =
        simulation::confirm_startup_world_build(s, *anchor, rules::FacilityOrientation::first);
    require(placed.error, "build recruitment");
    if (placed.denial != simulation::StartupBuildDenial::none || !placed.created)
        throw std::runtime_error("Human inspection recruitment was denied by actual world");
    i.recruitment = placed.created;
    require(simulation::cancel_startup_world_build(s), "leave continuous placement");
}
bool human_inspection_ready(const State &s, const std::string &mode,
                            const WorldHumanInspection &i) {
    const auto *page = top(s);
    if (!page)
        return false;
    if (mode == "world-tax") {
        const auto view = simulation::inspect_startup_world_tax_page(s, page->id);
        return page->legacy_page == 90 && i.home && view && view->total > 0;
    }
    if (mode == "world-tax-collected")
        return i.tax_collected && page->kind == Kind::scene &&
               s.scene.calendar.year * 12 + s.scene.calendar.month > i.tax_month &&
               s.scene.calendar.units >= 27;
    if (mode == "world-profession-complete" && i.profession_completed &&
        page->kind == Kind::scene && s.scene.scene_state == 0) {
        const auto details = simulation::startup_world_human_details(s, *i.human);
        if (!details || !i.target_profession || details->profession != *i.target_profession ||
            details->experience != 0)
            throw std::runtime_error("Human inspection completed profession did not retain actual "
                                     "target/reset experience");
        return true;
    }
    if (!i.human || !simulation::startup_world_human_page_ready(s, page->id))
        return false;
    const auto binding = s.page_human_bindings.find(page->id);
    if (binding == s.page_human_bindings.end() || binding->second != *i.human)
        return false;
    const int raw = page->legacy_page;
    if ((mode == "world-gift-cancel" || mode == "world-profession-cancel") && i.cancelled &&
        raw == (mode == "world-gift-cancel" ? 64 : 61) && !s.human_page_answers.count(page->id)) {
        verify_cancelled_choice(s, mode, i);
        return true;
    }
    if (human_tab(mode) >= 0)
        return raw == 60 && s.page_phases.at(page->id) == human_tab(mode);
    if (gift_catalogue(mode))
        return raw == 64 && s.page_phases.at(page->id) == gift_slot(mode);
    if (mode == "world-professions")
        return raw == 61;
    if (mode == "world-profession-preview")
        return raw == 62;
    if (mode == "world-profession-change")
        return raw == 63 && i.profession_confirmed && s.page_counters.at(page->id) >= 197;
    if (mode == "world-gift-confirm")
        return raw == 65;
    if (mode == "world-equipment-info")
        return raw == 73;
    if (mode == "world-gift-result")
        return raw == 66 && i.gift_confirmed && s.page_counters.at(page->id) >= 135;
    return mode == "world-equipment-change" && raw == 68 && i.gift_confirmed &&
           s.page_counters.at(page->id) >= 40;
}
bool apply_human_inspection_input(State &s, const std::string &mode, WorldHumanInspection &i) {
    const auto *current = top(s);
    if (!current)
        throw std::runtime_error("Human inspection lost real page stack");
    const auto page = *current;
    // Do not let generic acknowledgements silently make a profession/mastery decision.
    if (page.legacy_page == 70 || (page.legacy_page == 63 && !profession_commit_mode(mode)))
        throw std::runtime_error(
            "Human inspection reached a profession/mastery decision outside its player policy");
    if (((page.legacy_page >= 60 && page.legacy_page <= 66) || page.legacy_page == 68 ||
         page.legacy_page == 73) &&
        !simulation::startup_world_human_page_ready(s, page.id))
        return true;
    if (tax_mode(mode) && housing_input(s, page, i))
        return true;
    if (!tax_mode(mode) && page.kind == Kind::scene) {
        // A scene page can still be running the source arrival camera transition. Wait for
        // the same ordinary-play gate as open_startup_world_human_page before choosing input.
        if (s.scene.scene_state != 0 || s.scene.framework_paused)
            return true;
        if (i.profession_completed)
            return true;
        if (!i.human)
            for (const auto id : s.scene.world.world.ai.human_order) {
                const auto &actor = s.scene.world.world.ai.battle.actors.at(id);
                if (actor.kind == rules::ActorKind::human && actor.control.state != 4 &&
                    s.human_presence.at(actor.definition) != 0 &&
                    simulation::startup_world_human_details(s, actor.definition)) {
                    i.human = actor.definition;
                    break;
                }
            }
        if (i.human && (!profession_mode(mode) || mode == "world-professions" ||
                        can_preview_profession(s, *i.human, profession_commit_mode(mode))))
            require(simulation::open_startup_world_human_page(s, *i.human), "open actual human");
        return true;
    }
    if (!tax_mode(mode) && page.legacy_page == 60) {
        if (i.profession_completed)
            act(s, page.id, Action::cancel);
        else if (human_tab(mode) >= 0)
            act(s, page.id, Action::view_tab, human_tab(mode));
        else
            act(s, page.id, profession_mode(mode) ? Action::professions : Action::gifts);
        return true;
    }
    if (!tax_mode(mode) && page.legacy_page == 61) {
        if (s.human_page_answers.count(page.id) || i.profession_confirmed || i.cancelled)
            return true;
        const auto &list = s.human_page_catalogs.at(page.id);
        const auto job = std::find_if(list.begin(), list.end(), [&](int value) {
            return profession_available(s, *i.human, value, profession_commit_mode(mode));
        });
        if (job == list.end())
            throw std::runtime_error("Human inspection has no affordable profession preview");
        const int choice = static_cast<int>(job - list.begin());
        if (s.human_page_selections.at(page.id) == choice) {
            capture_choice(s, i);
            i.target_profession = *job;
        }
        act(s, page.id,
            s.human_page_selections.at(page.id) == choice ? Action::confirm : Action::select,
            choice);
        return true;
    }
    if (!tax_mode(mode) && page.legacy_page == 62) {
        if (mode == "world-profession-cancel") {
            act(s, page.id, Action::cancel);
            i.cancelled = true;
        } else if (profession_commit_mode(mode)) {
            act(s, page.id, Action::confirm);
            i.profession_confirmed = true;
        }
        return true;
    }
    if (page.legacy_page == 63 && profession_commit_mode(mode)) {
        if (mode == "world-profession-complete" && s.page_counters.at(page.id) >= 197) {
            act(s, page.id, Action::confirm);
            i.profession_completed = true;
        }
        return true; // Natural animation reaches midpoint/final gate without fast-forward input.
    }
    if (!tax_mode(mode) && page.legacy_page == 64) {
        if (s.human_page_answers.count(page.id))
            return true;
        if (i.gift_confirmed || i.cancelled)
            return true;
        const int slot = gift_slot(mode);
        if (s.page_phases.at(page.id) != slot)
            act(s, page.id, Action::equipment_slot, slot);
        else if (!gift_catalogue(mode)) {
            const int choice = affordable_gift(s, page.id, *i.human, slot);
            i.equipment = s.equipment_page_catalogs.at(page.id)[slot][choice];
            if (s.human_page_selections.at(page.id) != choice)
                act(s, page.id, Action::select, choice);
            else {
                if (mode == "world-gift-cancel") {
                    capture_choice(s, i);
                    i.stock_before_choice = s.catalog.at({1, *i.equipment}).free_purchases;
                }
                act(s, page.id,
                    mode == "world-equipment-info" ? Action::inspect_equipment : Action::confirm);
            }
        }
        return true;
    }
    if (!tax_mode(mode) && page.legacy_page == 65) {
        if (mode == "world-gift-cancel") {
            act(s, page.id, Action::cancel);
            i.cancelled = true;
        } else {
            act(s, page.id, Action::confirm);
            i.gift_confirmed = true;
        }
        return true;
    }
    if (page.legacy_page == 66 || page.legacy_page == 68) {
        if ((mode == "world-gift-result" && page.legacy_page == 66) ||
            (mode == "world-equipment-change" && page.legacy_page == 68))
            return true;
        act(s, page.id, Action::confirm);
        return true;
    }
    if (page.legacy_page == 83) {
        require(simulation::cancel_startup_world_runtime_page(s, page.id), "close shop entry");
        return true;
    }
    if (page.legacy_page == 87) {
        const auto ranking = s.award_rankings.find(page.id);
        if (ranking == s.award_rankings.end())
            return true;
        using Award = rules::WorldAwardAction;
        if (tax_mode(mode) && s.medal_count > 0 && !ranking->second.empty())
            require(simulation::act_startup_world_runtime_award_page(
                        s, page.id,
                        s.award_pending_humans.count(page.id) ? Award::confirm_award
                                                              : Award::request_award,
                        0),
                    "annual housing award");
        else {
            const auto pending = s.award_termination_pending.find(page.id);
            if (pending != s.award_termination_pending.end())
                require(
                    simulation::act_startup_world_runtime_award_page(
                        s, page.id,
                        pending->second ? Award::confirm_termination : Award::request_termination),
                    "finish actual annual awards");
        }
        return true;
    }
    if (page.legacy_page == 98 || page.legacy_page == 62 || page.legacy_page == 73)
        return true;
    return false;
}
} // namespace ark::desktop
