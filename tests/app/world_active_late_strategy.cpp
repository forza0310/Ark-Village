#include "world_active_late_strategy.hpp"

#include "ark/simulation/actors/rules/human_management.hpp"
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
using Commerce = sim::StartupCommerceAction;
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
Command commerce(std::uint64_t page, Commerce action, int selection = 0) {
    auto c = command(Kind::commerce_action, page);
    c.commerce_action = action;
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
bool has_definition(const State &s, int definition) {
    return std::any_of(
        s.scene.world.world.facilities.begin(), s.scene.world.world.facilities.end(),
        [&](const auto &entry) { return entry.second.placement.definition_id == definition; });
}
bool ready(const State &s, int target) {
    if (target == 3)
        return s.rank == 2 && s.popularity >= 1500 && s.maximum_income >= 35000 &&
               s.events_held >= 15 && has_definition(s, 40);
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
std::optional<ref::Position> free_roadside(const State &s, int definition = 35,
                                           bool reserve_school = false,
                                           std::size_t *candidate_count = nullptr) {
    const auto &m = s.scene.world.world.map;
    const auto bounds = s.rules->fences.at(s.fence_level);
    std::vector<ref::FootprintCell> reserved;
    if (reserve_school && !has_definition(s, 63)) {
        const auto place = free_roadside(s, 63);
        if (!place)
            return {};
        reserved = ref::facility_footprint(
                       static_cast<ref::FacilityShape>(s.rules->facilities.at(63).shape),
                       ref::FacilityOrientation::first, *place, m.width, m.height)
                       .cells;
    }
    const auto paths = ref::search_legacy_map(m, sim::startup_evidence().spawn_points.at(0));
    require(paths.field.has_value(), "roadside plan has no current distance field");
    std::optional<ref::Position> first;
    for (int y = bounds[1].y + 1; y < bounds[0].y; ++y)
        for (int x = bounds[0].x + 1; x < bounds[1].x; ++x) {
            const auto &cell = m.cells.at(y * m.width + x);
            if (cell.facility || cell.legacy_state != 4)
                continue;
            const auto footprint = ref::facility_footprint(
                static_cast<ref::FacilityShape>(s.rules->facilities.at(definition).shape),
                ref::FacilityOrientation::first, {x, y}, m.width, m.height);
            if (footprint.error != ref::GeometryError::none ||
                std::any_of(footprint.cells.begin(), footprint.cells.end(), [&](const auto &part) {
                    const auto pos = part.position;
                    if (pos.x <= bounds[0].x || pos.x >= bounds[1].x || pos.y >= bounds[0].y ||
                        pos.y <= bounds[1].y)
                        return true;
                    const auto &tile = m.cells.at(pos.y * m.width + pos.x);
                    return tile.facility || tile.legacy_state != 4 ||
                           std::any_of(reserved.begin(), reserved.end(),
                                       [&](const auto &slot) { return slot.position == pos; });
                }))
                continue;
            for (const auto d : {ref::Position{1, 0}, {-1, 0}, {0, 1}, {0, -1}}) {
                const int nx = x + d.x, ny = y + d.y;
                if (nx < 0 || ny < 0 || nx >= m.width || ny >= m.height)
                    continue;
                const auto index = ny * m.width + nx;
                if (m.cells.at(index).category == ref::RouteCategory::road &&
                    paths.field->distances.at(index)) {
                    if (!candidate_count)
                        return ref::Position{x, y};
                    ++*candidate_count;
                    if (!first)
                        first = ref::Position{x, y};
                    break;
                }
            }
        }
    return first;
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

ActiveLateVillageStrategy::ActiveLateVillageStrategy(int target_rank) {
    require(target_rank == 2 || target_rank == 3, "unsupported target rank");
    stats_.target_rank = target_rank;
}

void ActiveLateVillageStrategy::reconcile(const State &s) {
    require(s.rules && s.rank >= 1, "late route requires a real first-star player save");
    require(stats_.target_rank != 3 || (s.scripts.user_flags & 16U) != 0,
            "third-star prefix must already have the real chamber-entry qualification");
    if (!initialized_) {
        stats_.minimum_cash = cash(s);
        stats_.initial_successes = s.task_progress.successes;
        require(s.rank == stats_.target_rank - 1 && !s.active_task,
                "new late route must start at the preceding stable star");
        for (const auto &[id, f] : s.scene.world.world.facilities)
            if (f.placement.definition_id == 40) {
                stats_.western = id;
                stats_.western_initial_sales = f.sales;
            }
        initialized_ = true;
    }
    require(s.task_progress.successes >= stats_.initial_successes + stats_.task_successes,
            "task successes lost across player reload");
    require(stats_.promoted_month < 0 || s.rank >= stats_.target_rank,
            "promoted star lost across reload");
    require(stats_.pot_month < 0 || activity_count(s, 30) > 0, "pot import lost across reload");
    require(!stats_.western_unlock_claimed || s.facility_presence.at(40) == 2,
            "claimed restaurant qualification lost across reload");
    if (stats_.layout_old_shop) {
        require(stats_.layout_complete && stats_.layout_road_cells == 6 &&
                    stats_.layout_moved_shop &&
                    !s.scene.world.world.facilities.count(stats_.layout_old_shop) &&
                    s.scene.world.world.facilities.at(stats_.layout_moved_shop).placement.anchor ==
                        ref::Position{8, 7} &&
                    !(s.facility_flags.at(stats_.layout_moved_shop) & 1U),
                "road-expansion business checkpoint lost its completed connected moved shop");
    }
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
    if (stats_.target_rank == 3 && stats_.layout_old_shop && !stats_.layout_complete &&
        s.scene.scene_state == 1 && s.build_mode != 0) {
        auto c = command(Kind::confirm_edit);
        c.orientation = ref::FacilityOrientation::first;
        c.selection = s.build_mode;
        c.definition = s.build_definition.value_or(-1);
        c.edit_anchor = s.build_anchor;
        c.facility = s.build_moving_facility.value_or(0);
        if (s.build_mode == 6) {
            if (stats_.layout_moved_shop) {
                c.kind = Kind::cancel_edit;
                return c;
            }
            c.anchor = {10, 7};
        } else if (s.build_mode == 7) {
            c.anchor = {8, 7};
        } else if (s.build_mode == 1) {
            if (stats_.layout_road_cells == 6) {
                c.kind = Kind::cancel_edit;
                return c;
            }
            c.anchor = stats_.layout_road_cells == 0 ? ref::Position{10, 7} : ref::Position{9, 3};
        } else if (s.build_mode == 2) {
            c.anchor = stats_.layout_road_cells == 0 ? ref::Position{10, 7} : ref::Position{9, 7};
        } else {
            require(false, "unexpected edit mode in road-expansion route");
        }
        return c;
    }
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
        const bool third = stats_.target_rank == 3;
        if (third && !stats_.layout_complete && (stats_.layout_old_shop || !free_roadside(s, 40))) {
            const auto road_quote = sim::startup_world_build_quote(s, 18);
            require(road_quote.has_value(), "road expansion lacks current construction quote");
            if (cash(s) < reserve(s) + 300 + 6 * road_quote->construction_cost)
                return {};
            if (!stats_.layout_old_shop) {
                require(s.fence_level == 0 && (s.scripts.user_flags & 32U) != 0,
                        "road expansion requires the observed fence and real move qualification");
                const auto &map = s.scene.world.world.map;
                const auto &cell = map.cells.at(7 * map.width + 10);
                require(cell.facility.has_value(), "observed branch-entry shop is absent");
                const auto id = cell.facility->instance_id.value;
                const auto &shop = s.scene.world.world.facilities.at(id);
                require(shop.kind == 3 && shop.status == 1 &&
                            shop.placement.shape == ref::FacilityShape::single &&
                            shop.placement.definition_id == 45 &&
                            shop.placement.anchor == ref::Position{10, 7} &&
                            map.cells.at(7 * map.width + 11).category == ref::RouteCategory::road,
                        "unknown branch-entry layout; refuse automatic relocation");
                for (const auto point : {ref::Position{8, 7},
                                         {9, 3},
                                         {9, 4},
                                         {9, 5},
                                         {9, 6},
                                         {9, 7},
                                         {7, 3},
                                         {8, 3},
                                         {7, 4},
                                         {8, 4}}) {
                    const auto &tile = map.cells.at(point.y * map.width + point.x);
                    require(!tile.facility && tile.legacy_state == 4,
                            "observed branch, destination or school reservation is occupied");
                }
                stats_.layout_old_shop = id;
                build_definition_ = -2;
            } else {
                require(stats_.layout_moved_shop && stats_.layout_road_cells < 6,
                        "incomplete road expansion has no next action");
                build_definition_ = 18;
            }
            return command(Kind::open_build_menu);
        }
        const bool needs_western = third && s.rank == 2 && s.facility_presence.at(40) != 2;
        const int western_reserve = needs_western ? s.rules->facility_initial.at(40).capacity : 0;
        if ((!third && stats_.pot_month >= 0) || (third && stats_.school_activity_month >= 0))
            return {}; // After import, let the town actually trade for a whole month.
        // Rank two unlocks the chamber offer, not the construction catalogue. Preserve
        // its real point quote before ordinary activities and pay through 85 then 93.
        if (needs_western && s.village_points >= western_reserve &&
            stats_.ticks >= next_management_tick_) {
            next_management_tick_ = stats_.ticks + 300;
            return command(Kind::open_commerce);
        }
        if (s.quarter_counter > 0 && stats_.ticks >= next_activity_tick_ &&
            s.village_points - western_reserve >=
                (third         ? (s.rank == 3 ? s.rules->activities.at(7).parameters[4] : 20)
                 : s.rank >= 2 ? s.rules->activities.at(30).parameters[4]
                               : 20)) {
            next_activity_tick_ = stats_.ticks + 300;
            return command(Kind::open_village_activities);
        }
        if (third && stats_.ticks >= next_management_tick_) {
            next_management_tick_ = stats_.ticks + 300;
            const int mandatory = s.rank == 3 ? 63 : 40;
            const auto catalog = sim::startup_world_build_catalog(s);
            int selected = -1;
            std::optional<ref::Position> anchor;
            std::int64_t best = std::numeric_limits<std::int64_t>::max();
            if (catalog)
                for (const auto &group : *catalog)
                    for (const int id : group) {
                        const auto &d = s.rules->facilities.at(id);
                        const bool required = !has_definition(s, mandatory);
                        // Bound expansion by the real income deficit and keep one of each
                        // available shop. This is a budgeted player heuristic, not an optimum.
                        if (required ? id != mandatory
                                     : (s.rank != 2 || s.maximum_income >= 35000 ||
                                        count(s, false) >= 16 || has_definition(s, id) ||
                                        (d.kind != 3 && d.kind != 9) || d.shape != 0))
                            continue;
                        const auto q = sim::startup_world_build_quote(s, id);
                        const auto place = free_roadside(s, id, id != 63);
                        require(q.has_value(), "third-star catalog building lacks quote");
                        const auto capital = q->construction_cost + 2 * q->definition_attributes[3];
                        if (!place || cash(s) < reserve(s) + capital || capital >= best)
                            continue;
                        best = capital;
                        selected = id;
                        anchor = place;
                    }
            if (selected >= 0) {
                build_definition_ = selected;
                build_anchor_ = *anchor;
                build_committed_ = false;
                return command(Kind::open_build_menu);
            }
        }
        if (!third && stats_.ticks >= next_management_tick_ && s.rank == 1) {
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
        if ((s.rank == stats_.target_rank - 1 || (third && s.rank == 3)) && !s.active_task &&
            month(s) >= next_task_month_ &&
            (third ? (!stats_.task_successes || s.popularity < 1500 || s.village_points < 200)
                   : (s.task_progress.successes < 12 || s.popularity < 800)) &&
            std::any_of(s.task_order.begin(), s.task_order.end(),
                        [&](auto id) { return affordable_task(s, id); }))
            return command(Kind::open_task_menu);
        return {};
    }
    const int raw = p.legacy_page;
    if (stats_.target_rank == 3 && (raw == 83 || raw == 85 || raw == 93)) {
        const auto view = sim::inspect_startup_world_commerce_page(s, p.id);
        if (!view)
            return {};
        if (raw == 83)
            return commerce(p.id,
                            s.facility_presence.at(40) == 2 ? Commerce::cancel
                            : view->selection == 2          ? Commerce::confirm
                                                            : Commerce::select,
                            2);
        if (raw == 93) {
            require(view->binding == 40 && stats_.western_unlock_paid,
                    "unexpected facility claim in restaurant route");
            return commerce(p.id, Commerce::confirm);
        }
        const auto found = std::find(view->entries.begin(), view->entries.end(), 40);
        require(found != view->entries.end(), "rank-two chamber must offer locked restaurant40");
        const int index = static_cast<int>(found - view->entries.begin());
        return commerce(p.id, view->selection == index ? Commerce::confirm : Commerce::select,
                        index);
    }
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
                if ((stats_.target_rank == 3
                         ? (s.rank == 3 ? a.identity == 7 : a.parameters[2] <= 2)
                     : s.rank >= 2 ? a.identity == 30
                                   : a.parameters[2] <= 2) &&
                    a.parameters[4] <=
                        s.village_points -
                            (stats_.target_rank == 3 && s.facility_presence.at(40) != 2
                                 ? s.rules->facility_initial.at(40).capacity
                                 : 0) &&
                    s.quarter_counter > 0) {
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
        c.cancel = !ready(s, stats_.target_rank);
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
    stats_.trade.observe(before, after);
    stats_.minimum_cash = std::min(stats_.minimum_cash, cash(after));
    require(cash(after) >= 0, "cash became negative; " + diagnose(after));
    for (const auto &[id, f] : after.scene.world.world.facilities) {
        if (stats_.target_rank == 3 && f.placement.definition_id == 40 && !stats_.western)
            stats_.western = id;
        if (stats_.target_rank == 3 && stats_.buildings.count(id) &&
            f.placement.definition_id == 63) {
            stats_.school = id;
            stats_.school_sales = f.sales;
        }
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
        require(before.rank == stats_.target_rank - 1 && after.rank == stats_.target_rank &&
                    ready(before, stats_.target_rank) &&
                    (stats_.target_rank == 3 ? stats_.third_star_conditions
                                             : stats_.second_star_conditions),
                "promotion bypassed genuine conditions/application");
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
    if (stats_.target_rank == 3 && c.kind == Kind::confirm_edit && before.build_mode == 7) {
        const auto old_id = stats_.layout_old_shop;
        require(result.created && old_id && !stats_.layout_moved_shop,
                "relocation lacks unique source/destination receipt");
        const auto new_id = *result.created;
        const auto &old_shop = before.scene.world.world.facilities.at(old_id);
        const auto &new_shop = after.scene.world.world.facilities.at(new_id);
        require(
            !after.scene.world.world.facilities.count(old_id) &&
                old_shop.placement.definition_id == new_shop.placement.definition_id &&
                new_shop.placement.anchor == ref::Position{8, 7} &&
                old_shop.status == new_shop.status && old_shop.sales == new_shop.sales &&
                before.facility_monthly_cash.at(old_id) == after.facility_monthly_cash.at(new_id) &&
                before.facility_ordinals.at(old_id) == after.facility_ordinals.at(new_id) &&
                before.facility_original_ids.at(old_id) == after.facility_original_ids.at(new_id) &&
                cash(before) - cash(after) == 300,
            "real relocation must preserve shop identity/receipts and charge exactly 300G");
        for (std::size_t n = 0; n < before.scene.world.world.map.cells.size(); ++n)
            if (before.scene.world.world.map.cells[n].category == ref::RouteCategory::road)
                require(after.scene.world.world.map.cells[n].category == ref::RouteCategory::road,
                        "relocation changed an existing road");
        stats_.layout_moved_shop = new_id;
        if (stats_.buildings.erase(old_id))
            stats_.buildings.insert(new_id);
        stats_.layout_cost += 300;
    }
    if (stats_.target_rank == 3 && c.kind == Kind::confirm_edit && before.build_mode == 2 &&
        stats_.layout_old_shop && !stats_.layout_complete) {
        const auto quote = sim::startup_world_build_quote(before, 18);
        require(quote.has_value(), "road receipt has no current price");
        const auto &old_map = before.scene.world.world.map;
        const auto &new_map = after.scene.world.world.map;
        int changed{};
        for (int y = 0; y < old_map.height; ++y)
            for (int x = 0; x < old_map.width; ++x) {
                const auto n = y * old_map.width + x;
                if (old_map.cells[n].category == new_map.cells[n].category)
                    continue;
                require(((x == 10 && y == 7) || (x == 9 && y >= 3 && y <= 7)) &&
                            old_map.cells[n].category == ref::RouteCategory::ground &&
                            new_map.cells[n].category == ref::RouteCategory::road &&
                            !old_map.cells[n].facility && !new_map.cells[n].facility,
                        "road input changed an unplanned cell or occupied facility");
                ++changed;
            }
        require(changed == (stats_.layout_road_cells == 0 ? 1 : 5) &&
                    cash(before) - cash(after) == changed * quote->construction_cost,
                "road must charge current quote for exactly its actual changed cells");
        stats_.layout_road_cells += changed;
        stats_.layout_cost += cash(before) - cash(after);
    }
    if (stats_.target_rank == 3 && c.kind == Kind::cancel_edit && before.build_mode == 1 &&
        stats_.layout_road_cells == 6) {
        require(after.scene.scene_state == 0 && !after.build_definition &&
                    !(after.facility_flags.at(stats_.layout_moved_shop) & 1U) &&
                    free_roadside(after, 63).has_value() &&
                    free_roadside(after, 40, true).has_value(),
                "completed road branch must connect the moved shop, school and restaurant sites");
        stats_.layout_complete = true;
    }
    if (stats_.target_rank == 3 && c.kind == Kind::commerce_action &&
        c.commerce_action == Commerce::confirm) {
        const auto view = sim::inspect_startup_world_commerce_page(before, c.page);
        require(view.has_value(), "commerce receipt lacks initialized page");
        if (view->raw == 85) {
            const int price = before.rules->facility_initial.at(40).capacity;
            require(!stats_.western_unlock_paid && before.rank == 2 &&
                        view->entries.at(view->selection) == 40 &&
                        before.village_points - after.village_points == price &&
                        cash(before) == cash(after) && top(after).legacy_page == 93 &&
                        top(after).legacy_s == 40 &&
                        after.facility_presence.at(40) == before.facility_presence.at(40),
                    "restaurant chamber payment must charge points once before claim");
            stats_.western_unlock_points = price;
            stats_.western_unlock_paid = true;
        }
        if (view->raw == 93 && view->counter >= 40) {
            require(stats_.western_unlock_paid && !stats_.western_unlock_claimed &&
                        view->binding == 40 && after.facility_presence.at(40) == 2 &&
                        after.facility_free_builds.at(40) ==
                            std::min(99, before.facility_free_builds.at(40) + 1) &&
                        before.village_points == after.village_points &&
                        cash(before) == cash(after),
                    "restaurant claim must unlock once without a second charge");
            stats_.western_unlock_claimed = true;
        }
    }
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
        require(ready(before, stats_.target_rank), "premature rank application");
        if (stats_.target_rank == 3)
            stats_.third_star_conditions = true;
        else
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
        if (stats_.target_rank == 3 && v->raw == 52 && v->activity && *v->activity == 7) {
            require(before.rank == 3 && stats_.school &&
                        before.scene.world.world.facilities.at(stats_.school).status == 1 &&
                        before.scripts.activities.at(7).status != 0 &&
                        before.village_points - after.village_points ==
                            before.rules->activities.at(7).parameters[4] &&
                        activity_count(after, 7) == activity_count(before, 7) + 1,
                    "school activity must follow real completed construction and payment");
            stats_.school_activity_paid = true;
        }
        if (v->raw == 53 && v->counter >= 120 && top(after).id != c.page) {
            ++stats_.activities;
            if (v->activity && *v->activity == 30) {
                require(stats_.pot_paid, "pot import completed without payment");
                stats_.pot_month = month(after);
                stats_.pot_income = stats_.facility_income;
            }
            if (stats_.target_rank == 3 && v->activity && *v->activity == 7) {
                require(stats_.school_activity_paid, "school activity completed without payment");
                stats_.school_activity_month = month(after);
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
    const bool business =
        stats_.target_rank == 3
            ? stats_.western &&
                  s.scene.world.world.facilities.at(stats_.western).sales >
                      stats_.western_initial_sales &&
                  stats_.task_successes > 0
            : stats_.admissions > 0 && stats_.new_shop_income > 0 && stats_.task_successes > 0;
    return business &&
           (!stats_.layout_old_shop ||
            (stats_.layout_complete && !(s.facility_flags.at(stats_.layout_moved_shop) & 1U))) &&
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
    if (stats_.target_rank == 3)
        return stats_.third_star_conditions && stats_.promoted_month >= 0 && s.rank == 3 &&
               stats_.school && stats_.school_sales > 0 && stats_.school_activity_paid &&
               stats_.school_activity_month >= 0 && activity_count(s, 7) > 0 &&
               s.scene.world.world.facilities.at(stats_.school).status == 1 &&
               stats_.trade.full_month_after(stats_.school_activity_month, month(s)) &&
               checkpoint(s);
    return stats_.second_star_conditions && stats_.promoted_month >= 0 && stats_.pot_paid &&
           stats_.pot_month >= 0 && s.rank == 2 && count(s, true, true) >= 4 &&
           count(s, false, true) >= 10 && s.task_progress.successes >= 12 &&
           month(s) >= stats_.pot_month + 2 &&
           stats_.trade.full_month_after(stats_.pot_month, month(s)) &&
           stats_.facility_income > stats_.pot_income && checkpoint(s);
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
        << " pot_month=" << stats_.pot_month << " target_rank=" << stats_.target_rank
        << " income_record=" << s.maximum_income << " events_held=" << s.events_held
        << " western=" << stats_.western << " school=" << stats_.school
        << " school_sales=" << stats_.school_sales
        << " school_activity_month=" << stats_.school_activity_month
        << " western_presence=" << s.facility_presence.at(40)
        << " western_unlock_points=" << stats_.western_unlock_points
        << " western_unlock_claimed=" << stats_.western_unlock_claimed
        << " moved_shop=" << stats_.layout_old_shop << "->" << stats_.layout_moved_shop
        << " road_cells=" << stats_.layout_road_cells << " layout_cost=" << stats_.layout_cost
        << " layout_complete=" << stats_.layout_complete;
    return out.str();
}
std::string ActiveLateVillageStrategy::diagnose_construction(const State &s) const {
    const auto catalog = sim::startup_world_build_catalog(s);
    const auto &map = s.scene.world.world.map;
    const auto bounds = s.rules->fences.at(s.fence_level);
    std::ostringstream out;
    out << "READ_ONLY_BUILD_INPUT cash=" << cash(s) << " reserve=" << reserve(s)
        << " rank=" << s.rank << " fence_level=" << s.fence_level << " bounds=" << bounds[0].x
        << ',' << bounds[0].y << ':' << bounds[1].x << ',' << bounds[1].y;
    for (const int definition : {40, 63}) {
        bool listed{};
        if (catalog)
            for (const auto &group : *catalog)
                listed = listed || std::find(group.begin(), group.end(), definition) != group.end();
        std::size_t source_sites{}, roadside{}, reserved{};
        for (int y = bounds[1].y + 1; y < bounds[0].y; ++y)
            for (int x = bounds[0].x + 1; x < bounds[1].x; ++x) {
                const auto footprint = ref::facility_footprint(
                    static_cast<ref::FacilityShape>(s.rules->facilities.at(definition).shape),
                    ref::FacilityOrientation::first, {x, y}, map.width, map.height);
                if (footprint.error != ref::GeometryError::none)
                    continue;
                if (std::all_of(footprint.cells.begin(), footprint.cells.end(),
                                [&](const auto &part) {
                                    const auto p = part.position;
                                    if (p.x <= bounds[0].x || p.x >= bounds[1].x ||
                                        p.y >= bounds[0].y || p.y <= bounds[1].y)
                                        return false;
                                    const auto &tile = map.cells.at(p.y * map.width + p.x);
                                    return !tile.facility && tile.legacy_state != 1 &&
                                           tile.legacy_state != 2 && tile.legacy_state != 10;
                                }))
                    ++source_sites;
            }
        const auto place = free_roadside(s, definition, false, &roadside);
        free_roadside(s, definition, definition != 63, &reserved);
        out << " definition=" << definition << " shape=" << s.rules->facilities.at(definition).shape
            << " presence=" << s.facility_presence.at(definition) << " in_catalog=" << listed
            << " source_footprint_sites=" << source_sites << " strategy_roadside_sites=" << roadside
            << " after_school_reservation=" << reserved;
        if (place)
            out << " first_site=" << place->x << ',' << place->y;
    }
    for (int y = bounds[1].y + 1; y < bounds[0].y; ++y) {
        out << "\nMAP_ROW y=" << y;
        for (int x = bounds[0].x + 1; x < bounds[1].x; ++x) {
            const auto &cell = map.cells.at(y * map.width + x);
            out << " x" << x << '=';
            if (cell.facility)
                out << 'F' << cell.facility->instance_id.value;
            else
                out << 'S' << cell.legacy_state;
        }
    }
    return out.str();
}
void ActiveLateVillageStrategy::encode(std::ostream &out) const {
    const auto &v = stats_;
    out << "ARK_ACTIVE_LATE_2\n"
        << v.commands << ' ' << v.ticks << ' ' << v.departed_task << ' ' << v.minimum_cash << ' '
        << v.facility_income << ' ' << v.new_shop_income << ' ' << v.construction_cost << ' '
        << v.gift_cost << ' ' << v.initial_successes << ' ' << v.task_successes << ' '
        << v.task_departures << ' ' << v.activities << ' ' << v.gifts << ' ' << v.admissions << ' '
        << v.promoted_month << ' ' << v.pot_month << ' ' << v.pot_income << ' '
        << v.second_star_conditions << ' ' << v.pot_paid << ' ' << requested_task_ << ' '
        << next_management_tick_ << ' ' << next_activity_tick_ << ' ' << next_task_month_ << ' '
        << initialized_ << '\n'
        << v.target_rank << ' ' << v.school_activity_month << ' ' << v.western << ' ' << v.school
        << ' ' << v.western_initial_sales << ' ' << v.school_sales << ' ' << v.third_star_conditions
        << ' ' << v.school_activity_paid << '\n';
    const auto write = [&](const auto &values) {
        out << values.size();
        for (const auto id : values)
            out << ' ' << id;
        out << '\n';
    };
    write(v.buildings);
    write(v.successful_tasks);
    write(v.residents);
    v.trade.encode(out);
    out << "WESTERN_UNLOCK_1 " << v.western_unlock_points << ' ' << v.western_unlock_paid << ' '
        << v.western_unlock_claimed << '\n';
    out << "ROAD_BRANCH_1 " << v.layout_old_shop << ' ' << v.layout_moved_shop << ' '
        << v.layout_road_cells << ' ' << v.layout_cost << ' ' << v.layout_complete << '\n';
    require(bool(out), "cannot encode strategy evidence");
}
ActiveLateVillageStrategy ActiveLateVillageStrategy::decode(std::istream &in) {
    ActiveLateVillageStrategy result;
    auto &v = result.stats_;
    std::string magic;
    in >> magic;
    require(magic == "ARK_ACTIVE_LATE_2", "unknown late strategy evidence");
    in >> v.commands >> v.ticks >> v.departed_task >> v.minimum_cash >> v.facility_income >>
        v.new_shop_income >> v.construction_cost >> v.gift_cost >> v.initial_successes >>
        v.task_successes >> v.task_departures >> v.activities >> v.gifts >> v.admissions >>
        v.promoted_month >> v.pot_month >> v.pot_income >> v.second_star_conditions >> v.pot_paid >>
        result.requested_task_ >> result.next_management_tick_ >> result.next_activity_tick_ >>
        result.next_task_month_ >> result.initialized_;
    in >> v.target_rank >> v.school_activity_month >> v.western >> v.school >>
        v.western_initial_sales >> v.school_sales >> v.third_star_conditions >>
        v.school_activity_paid;
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
    v.trade.decode(in);
    // Keep the already verified target-two prefix readable. Only target-three
    // evidence requires the new, independent restaurant purchase receipts.
    in >> std::ws;
    if (in.peek() != std::char_traits<char>::eof()) {
        std::string extension;
        in >> extension >> v.western_unlock_points >> v.western_unlock_paid >>
            v.western_unlock_claimed;
        require(extension == "WESTERN_UNLOCK_1", "unknown restaurant receipt extension");
        in >> std::ws;
        if (in.peek() != std::char_traits<char>::eof()) {
            in >> extension >> v.layout_old_shop >> v.layout_moved_shop >> v.layout_road_cells >>
                v.layout_cost >> v.layout_complete;
            require(extension == "ROAD_BRANCH_1", "unknown road branch receipt extension");
        } else {
            require(v.target_rank == 2, "third-star evidence lacks road branch receipts");
            in.clear();
        }
    } else {
        require(v.target_rank == 2, "third-star evidence lacks restaurant receipt extension");
        in.clear();
    }
    require(bool(in) && (v.target_rank == 2 || v.target_rank == 3) && v.minimum_cash >= 0 &&
                v.gifts >= 0 && v.admissions >= 0 && v.western_unlock_points >= 0 &&
                (!v.western_unlock_claimed || v.western_unlock_paid) &&
                (v.western_unlock_paid == (v.western_unlock_points > 0)) &&
                v.layout_road_cells >= 0 && v.layout_road_cells <= 6 && v.layout_cost >= 0 &&
                (!v.layout_complete ||
                 (v.layout_old_shop && v.layout_moved_shop && v.layout_road_cells == 6)) &&
                v.task_successes == static_cast<int>(v.successful_tasks.size()) &&
                v.admissions == static_cast<int>(v.residents.size()),
            "invalid late evidence totals");
    return result;
}
} // namespace ark::test
