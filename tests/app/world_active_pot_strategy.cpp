#include "world_active_pot_strategy.hpp"

#include <algorithm>
#include <istream>
#include <ostream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace ref = sim::rules;
using State = app::WorldState;
using Command = app::WorldCommand;
using Kind = app::WorldCommandKind;
using Pot = sim::StartupMagicPotAction;
using Commerce = sim::StartupCommerceAction;
using Human = sim::StartupHumanPageAction;
void require(bool ok, const std::string &message) {
    if (!ok)
        throw std::runtime_error("Active pot village: " + message);
}
const ref::WorldScriptPage &top(const State &s) {
    for (auto p = s.scripts.pages.rbegin(); p != s.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4)
            return *p;
    throw std::runtime_error("Active pot village lost its page");
}
int month(const State &s) { return s.scene.calendar.year * 12 + s.scene.calendar.month; }
int week(const State &s) { return month(s) * 4 + s.scene.calendar.subperiod; }
std::int64_t cash(const State &s) { return s.scene.world.world.ai.accounting.funds(); }
Command command(Kind kind, std::uint64_t page = 0) {
    Command c;
    c.kind = kind;
    c.page = page;
    return c;
}
Command pot(std::uint64_t page, Pot action, int selection = 0) {
    auto c = command(Kind::magic_pot_action, page);
    c.magic_pot_action = action;
    c.selection = selection;
    return c;
}
Command commerce(std::uint64_t page, Commerce action, int selection = 0) {
    auto c = command(Kind::commerce_action, page);
    c.commerce_action = action;
    c.selection = selection;
    return c;
}
Command human(std::uint64_t page, Human action, int selection = 0) {
    auto c = command(Kind::human_action, page);
    c.human_action = action;
    c.selection = selection;
    return c;
}
const ref::WorldMagicPotRecipeDefinition &recipe(const State &s) {
    const auto p =
        std::find_if(s.rules->magic_pot_recipes.begin(), s.rules->magic_pot_recipes.end(),
                     [](const auto &r) { return r.identity == 7; });
    require(p != s.rules->magic_pot_recipes.end(), "published medicine recipe absent");
    return *p;
}
const sim::StartupWorldItem &item(const State &s, int id) {
    const auto p = std::find_if(s.rules->items.begin(), s.rules->items.end(),
                                [id](const auto &d) { return d.identity == id; });
    require(p != s.rules->items.end(), "item definition absent");
    return *p;
}
bool enough_elements(const State &s) {
    const auto &r = recipe(s);
    for (int i = 0; i < 4; ++i)
        if (s.legacy_n[3 + i] < r.costs[i])
            return false;
    return true;
}
bool needs_material(const State &s) {
    return s.legacy_n[0] + s.legacy_n[1] < recipe(s).experience_required || !enough_elements(s);
}
// This is a purchasing policy, not an alternate pot formula. Prefer stock which
// covers current elemental deficits; retain medicine for the real HP consumer.
int material_score(const State &s, const sim::StartupWorldItem &d) {
    if (d.identity == 29 || d.recovery > 0)
        return 0;
    int score{}, pending_output{};
    for (int i = 0; i < 4; ++i) {
        score +=
            std::min(std::max(0, recipe(s).costs[i] - s.legacy_n[3 + i]), d.magic_pot_elements[i]) *
            10;
        pending_output += d.magic_pot_elements[i] / 2;
    }
    if (s.legacy_n[0] + s.legacy_n[1] < recipe(s).experience_required && pending_output > 0)
        score += 10;
    return score;
}
int owned_material(const State &s) {
    int best = -1, score{};
    for (const auto &d : s.rules->items)
        if (s.items.at(d.identity).inventory > 0 && material_score(s, d) > score) {
            best = d.identity;
            score = material_score(s, d);
        }
    return best;
}
std::int64_t reserve(const State &s) {
    std::int64_t fee{};
    for (const auto &[id, f] : s.scene.world.world.facilities) {
        (void)f;
        const auto q = sim::startup_world_facility_values(s, id);
        require(q.has_value(), "missing maintenance quote");
        fee += std::max<std::int64_t>(0, q->definition_attributes[3]);
    }
    for (const auto &[id, present] : s.human_presence)
        if (present) {
            const auto job = s.scene.world.world.ai.growth.at(id).definition.current_profession;
            const auto &j = s.rules->jobs.at(job);
            fee += std::max({0, j.fee[0], j.fee[1]});
        }
    return fee * 2;
}
int purchase_material(const State &s, const std::vector<int> &entries) {
    int best = -1, best_score{}, best_price{};
    for (int id : entries) {
        const auto &d = item(s, id);
        const auto score = material_score(s, d);
        if (score <= 0 || s.shop_item_stock.at(id).quantity <= 0 ||
            d.commerce_price > cash(s) - reserve(s))
            continue;
        if (best < 0 || std::int64_t(score) * std::max(1, best_price) >
                            std::int64_t(best_score) * std::max(1, d.commerce_price)) {
            best = id;
            best_score = score;
            best_price = d.commerce_price;
        }
    }
    return best;
}
bool stable(const State &s) {
    return top(s).kind == ref::WorldScriptPageKind::scene && s.scene.scene_state == 0 &&
           !s.build_definition && !s.active_task;
}
} // namespace

void ActivePotVillageStrategy::reconcile(const State &s) {
    require(s.rules && s.rank >= 2 && (s.scripts.user_flags & 1U),
            "route requires a real second star and imported pot");
    const auto &r = recipe(s);
    require(r.reward_kind == 0 && r.reward_definition == 29 && r.experience_required == 9 &&
                r.costs == std::array<std::int32_t, 4>{10, 10, 10, 0} && item(s, 29).recovery == 50,
            "medicine recipe or recovery differs from approved published route");
    if (!initialized_) {
        require(stable(s) && s.legacy_n[0] == 0 && s.legacy_n[1] == 0 &&
                    s.magic_pot_recipes.at(7).status == 0,
                "new pot route must start before genuine processing/discovery");
        stats_.minimum_cash = cash(s);
        initialized_ = true;
    }
    craft_allowed_ = decoded_evidence_ && stats_.discoveries == 1;
    require(stats_.discoveries == 0 || s.magic_pot_recipes.at(7).status == 1,
            "medicine discovery lost across reload");
    require(!s.build_definition, "checkpoint retains construction input");
    leaving_market_ = false;
}

std::optional<Command> ActivePotVillageStrategy::next(const State &s) {
    if (!initialized_)
        reconcile(s);
    const auto &p = top(s);
    if (p.lifecycle == 0)
        return {};
    if (p.kind == ref::WorldScriptPageKind::scene) {
        if (s.scene.scene_state != 0)
            return {};
        if (stats_.used)
            return {};
        if (stats_.crafted) {
            for (const auto id : s.scene.world.world.ai.human_order) {
                const auto &a = s.scene.world.world.ai.battle.actors.at(id);
                if (a.hp.target <= 0 || a.hp.target >= a.capacity || a.control.state == 4 ||
                    a.control.action == 7 || a.rescue)
                    continue;
                stats_.recipient = a.definition;
                stats_.healed_actor = id.value;
                auto c = command(Kind::open_human);
                c.actor = id;
                c.definition = a.definition;
                return c;
            }
            return {}; // Wait for actual ordinary combat damage, never manufacture injury.
        }
        if (s.magic_pot_recipes.at(7).status == 1 && enough_elements(s))
            return craft_allowed_ ? std::optional<Command>{command(Kind::open_magic_pot)}
                                  : std::nullopt;
        if (s.legacy_n[1] > 0 && week(s) - s.legacy_n[12] >= s.legacy_n[1])
            return command(Kind::open_magic_pot);
        if (needs_material(s) && s.legacy_n[1] < 10 && owned_material(s) >= 0)
            return command(Kind::open_magic_pot);
        if (needs_material(s) && s.legacy_n[1] < 10 && stats_.ticks >= next_market_tick_) {
            next_market_tick_ = stats_.ticks + 300;
            leaving_market_ = false;
            return command(Kind::open_commerce);
        }
        return {};
    }
    const int raw = p.legacy_page;
    if (raw >= 41 && raw <= 47) {
        const auto v = sim::inspect_startup_world_magic_pot_page(s, p.id);
        if (!v)
            return {};
        if (raw == 41) {
            if (!craft_allowed_ && s.magic_pot_recipes.at(7).status == 1 && enough_elements(s))
                return pot(p.id, Pot::cancel);
            if (stats_.crafted || s.legacy_n[1] >= 10 ||
                (s.magic_pot_recipes.at(7).status != 1 && !needs_material(s)) ||
                (needs_material(s) && owned_material(s) < 0))
                return pot(p.id, Pot::cancel);
            const int choice = s.magic_pot_recipes.at(7).status == 1 && enough_elements(s) ? 1 : 0;
            return pot(p.id, v->selection == choice ? Pot::confirm : Pot::select, choice);
        }
        if (raw == 42) {
            const int id = needs_material(s) ? owned_material(s) : -1;
            if (id < 0)
                return pot(p.id, Pot::cancel);
            const auto entry = std::find(v->entries.begin(), v->entries.end(), id);
            require(entry != v->entries.end(), "owned material absent from actual pot catalogue");
            const int index = static_cast<int>(entry - v->entries.begin());
            return pot(p.id, v->selection == index ? Pot::confirm : Pot::select, index);
        }
        if (raw == 43) {
            if (!craft_allowed_ || stats_.crafted)
                return pot(p.id, Pot::cancel);
            const auto entry = std::find(v->entries.begin(), v->entries.end(), 7);
            require(entry != v->entries.end(), "medicine absent from actual recipe catalogue");
            const int index = static_cast<int>(entry - v->entries.begin());
            return pot(p.id, v->selection == index ? Pot::confirm : Pot::select, index);
        }
        if ((raw == 46 || raw == 47) && v->counter <= 6)
            return {};
        if (raw == 47 && !craft_allowed_)
            return pot(p.id, Pot::cancel);
        return pot(p.id, Pot::confirm);
    }
    if (raw == 83 || raw == 84) {
        const auto v = sim::inspect_startup_world_commerce_page(s, p.id);
        if (!v)
            return {};
        if (raw == 83) {
            if (leaving_market_ || owned_material(s) >= 0 || !needs_material(s))
                return commerce(p.id, Commerce::cancel);
            if (v->selection != 0)
                return commerce(p.id, Commerce::select, 0);
            // Even an empty buy catalogue closes itself through event13. Remember
            // that this visit was attempted so its parent does not immediately
            // reopen it and freeze natural restocking forever.
            leaving_market_ = true;
            return commerce(p.id, Commerce::confirm);
        }
        if (owned_material(s) >= 0 || !needs_material(s))
            return commerce(p.id, Commerce::cancel);
        const int id = purchase_material(s, v->entries);
        if (id < 0) {
            leaving_market_ = true;
            return commerce(p.id, Commerce::cancel);
        }
        if (v->feedback_counter > 0)
            return {};
        const int index = static_cast<int>(std::find(v->entries.begin(), v->entries.end(), id) -
                                           v->entries.begin());
        return commerce(p.id, v->selection == index ? Commerce::confirm : Commerce::select, index);
    }
    if (raw == 60 || raw == 64 || raw == 66 || raw == 68 || raw == 69 || raw == 70) {
        if (!sim::startup_world_human_page_ready(s, p.id))
            return {};
        if (raw == 60)
            return human(p.id, stats_.used ? Human::cancel : Human::gifts);
        if (raw == 64) {
            if (s.human_page_answers.count(p.id))
                return {};
            if (stats_.used)
                return human(p.id, Human::cancel);
            if (s.page_phases.at(p.id) != 4)
                return human(p.id, Human::equipment_slot, 4);
            const auto &list = s.equipment_page_catalogs.at(p.id)[4];
            const auto entry = std::find(list.begin(), list.end(), 29);
            require(entry != list.end() && s.items.at(29).inventory > 0,
                    "crafted medicine missing from real gift catalogue");
            const int index = static_cast<int>(entry - list.begin());
            return human(p.id,
                         s.human_page_selections.at(p.id) == index ? Human::confirm : Human::select,
                         index);
        }
        // Let the genuine HP consumer at count75 run; early confirmation must not
        // be mistaken for healing merely because the inventory was already consumed.
        if (raw == 66 && stats_.used && !stats_.healed)
            return {};
        if (raw == 70 && s.page_phases.at(p.id) == 2 && s.human_page_selections.at(p.id) != 1)
            return human(p.id, Human::select, 1);
        return human(p.id, Human::confirm);
    }
    if (raw == 48) {
        auto c = command(Kind::rank_action, p.id);
        c.cancel = true;
        return c;
    }
    if (raw == 87) {
        auto c = command(Kind::award_action, p.id);
        const auto pending = s.award_termination_pending.find(p.id);
        c.award_action = pending != s.award_termination_pending.end() && pending->second
                             ? ref::WorldAwardAction::confirm_termination
                             : ref::WorldAwardAction::request_termination;
        return c;
    }
    if (raw == 90)
        return s.tax_page_residents.count(p.id)
                   ? std::optional<Command>{command(Kind::tax_action, p.id)}
                   : std::nullopt;
    if (raw == 82)
        return s.facility_catalog_pages_initialized.count(p.id)
                   ? std::optional<Command>{command(Kind::facility_catalog_action, p.id)}
                   : std::nullopt;
    if (raw == 93)
        return s.commerce_pages_initialized.count(p.id)
                   ? std::optional<Command>{commerce(p.id, Commerce::confirm)}
                   : std::nullopt;
    const std::set<int> automatic{16, 24, 56, 57, 86, 97, 98};
    if (automatic.count(raw))
        return {};
    const std::set<int> ordinary{0,  1,  11, 15, 30, 31, 32, 49, 50, 59,
                                 67, 81, 88, 89, 94, 95, 96, 99, 100};
    require(ordinary.count(raw) != 0, "uncovered decision page raw=" + std::to_string(raw));
    return command(Kind::acknowledge_page, p.id);
}

void ActivePotVillageStrategy::observe_world(const State &before, const State &after) {
    stats_.trade.observe(before, after);
    stats_.minimum_cash = std::min(stats_.minimum_cash, cash(after));
    require(cash(after) >= 0, "cash became negative; " + diagnose(after));
    for (const auto &[id, f] : after.scene.world.world.facilities) {
        const auto old = before.scene.world.world.facilities.find(id);
        if (old != before.scene.world.world.facilities.end() && f.sales > old->second.sales)
            stats_.facility_income += f.sales - old->second.sales;
    }
    if (stats_.used && !stats_.healed && top(before).legacy_page == 66 &&
        before.page_human_bindings.at(top(before).id) == stats_.recipient &&
        before.human_equipment_choices.at(top(before).id)[1] == 29) {
        const auto actor = ref::CharacterId{stats_.healed_actor};
        const auto &a = before.scene.world.world.ai.battle.actors.at(actor);
        const auto &b = after.scene.world.world.ai.battle.actors.at(actor);
        if (b.hp.target > a.hp.target) {
            require(b.hp.target == std::min(a.capacity, a.hp.target + 50) && a.hp.target > 0 &&
                        a.capacity == b.capacity && cash(before) == cash(after),
                    "medicine must restore actual HP without changing capacity/cash");
            stats_.hp_before = a.hp.target;
            stats_.hp_after = b.hp.target;
            stats_.healed = 1;
        }
    }
}

void ActivePotVillageStrategy::observe(const State &before, const Command &c,
                                       const app::WorldCommandResult &result, const State &after) {
    require(result.outcome == app::WorldCommandOutcome::applied &&
                result.runtime_error == sim::StartupWorldRuntimeError::none &&
                result.denial == ref::TaskCommandDenial::none,
            "command rejected kind=" + std::to_string(static_cast<int>(c.kind)) + "; " +
                diagnose(before));
    ++stats_.commands;
    const int raw = top(before).legacy_page;
    if (c.kind == Kind::magic_pot_action && c.magic_pot_action == Pot::confirm && raw == 42) {
        const auto v = sim::inspect_startup_world_magic_pot_page(before, c.page);
        require(v && !v->entries.empty(), "deposit missing selected inventory");
        const int id = v->entries.at(v->selection);
        require(after.items.at(id).inventory + 1 == before.items.at(id).inventory &&
                    after.legacy_n[1] == before.legacy_n[1] + 1 && cash(before) == cash(after),
                "deposit must consume one real item and keep cash");
        ++stats_.deposits;
    }
    if (c.kind == Kind::open_magic_pot && after.legacy_n[0] > before.legacy_n[0]) {
        require(week(before) > before.legacy_n[12] && after.legacy_n[0] - before.legacy_n[0] ==
                                                          before.legacy_n[1] - after.legacy_n[1],
                "processing lacks natural elapsed week and actual pending items");
        stats_.processed += after.legacy_n[0] - before.legacy_n[0];
    }
    if (before.magic_pot_recipes.at(7).status == 0 && after.magic_pot_recipes.at(7).status == 1) {
        require(c.kind == Kind::magic_pot_action && raw == 46 && stats_.processed >= 9 &&
                    after.legacy_n[0] >= 9,
                "medicine discovered without genuine processing");
        ++stats_.discoveries;
    }
    if (c.kind == Kind::magic_pot_action && c.magic_pot_action == Pot::confirm && raw == 47) {
        require(craft_allowed_ && top(before).legacy_g == 7 && !stats_.crafted &&
                    enough_elements(before) && before.magic_pot_recipes.at(7).status == 1 &&
                    after.items.at(29).inventory == before.items.at(29).inventory + 1 &&
                    cash(before) == cash(after),
                "craft must grant exactly one real medicine");
        for (int i = 0; i < 4; ++i)
            require(before.legacy_n[i + 3] - after.legacy_n[i + 3] == recipe(before).costs[i],
                    "craft must pay exact published elemental cost once");
        stats_.crafted = 1;
        stats_.craft_month = month(after);
    }
    if (c.kind == Kind::commerce_action && c.commerce_action == Commerce::confirm && raw == 84 &&
        result.commerce_amount) {
        const auto v = sim::inspect_startup_world_commerce_page(before, c.page);
        require(v && v->mode == 0, "material purchase has no buy catalogue binding");
        const int id = v->entries.at(v->selection);
        const auto price = item(before, id).commerce_price;
        require(before.items.at(id).inventory + 1 == after.items.at(id).inventory &&
                    before.shop_item_stock.at(id).quantity ==
                        after.shop_item_stock.at(id).quantity + 1 &&
                    cash(before) - cash(after) == price && *result.commerce_amount == price,
                "material purchase must consume real stock and quoted cash");
        ++stats_.purchases;
        stats_.purchase_cost += price;
    }
    if (c.kind == Kind::human_action && c.human_action == Human::confirm && raw == 64 &&
        before.page_phases.at(c.page) == 4) {
        require(stats_.crafted == 1 && !stats_.used && stats_.recipient >= 0 &&
                    before.items.at(29).inventory == after.items.at(29).inventory + 1 &&
                    cash(before) == cash(after),
                "medicine use must consume the crafted stock");
        stats_.used = 1;
        stats_.use_month = month(after);
        stats_.use_income = stats_.facility_income;
    }
    observe_world(before, after);
}

void ActivePotVillageStrategy::observe_tick(const State &before, const State &after) {
    ++stats_.ticks;
    require(month(after) >= month(before) && month(after) <= month(before) + 1 &&
                ref::valid_world_calendar_state(after.scene.calendar) &&
                after.scene.random.draws() >= before.scene.random.draws(),
            "calendar/random discontinuity");
    observe_world(before, after);
}
bool ActivePotVillageStrategy::checkpoint(const State &s) const {
    return stable(s) && stats_.deposits >= 9 && stats_.processed >= 9 && stats_.discoveries == 1 &&
           s.magic_pot_recipes.at(7).status == 1 && (stats_.crafted || enough_elements(s));
}
bool ActivePotVillageStrategy::complete(const State &s) const {
    return checkpoint(s) && stats_.crafted == 1 && stats_.used == 1 && stats_.healed == 1 &&
           stats_.hp_after > stats_.hp_before && month(s) >= stats_.use_month + 2 &&
           stats_.trade.full_month_after(stats_.use_month, month(s)) &&
           stats_.facility_income > stats_.use_income;
}
std::string ActivePotVillageStrategy::diagnose(const State &s) const {
    std::ostringstream out;
    out << "raw=" << top(s).legacy_page << " month=" << month(s) << " week=" << week(s)
        << " ticks=" << stats_.ticks << " cash=" << cash(s) << " reserve=" << reserve(s)
        << " exp=" << s.legacy_n[0] << "/9 pending=" << s.legacy_n[1]
        << " elements=" << s.legacy_n[3] << ',' << s.legacy_n[4] << ',' << s.legacy_n[5]
        << " purchases=" << stats_.purchases << " deposits=" << stats_.deposits
        << " processed=" << stats_.processed << " discovered=" << stats_.discoveries
        << " crafted=" << stats_.crafted << " used=" << stats_.used << " healed=" << stats_.healed
        << " recipient=" << stats_.recipient << " owned_material=" << owned_material(s);
    return out.str();
}
void ActivePotVillageStrategy::encode(std::ostream &out) const {
    const auto &v = stats_;
    out << "ARK_ACTIVE_POT_2\n"
        << v.commands << ' ' << v.ticks << ' ' << v.healed_actor << ' ' << v.minimum_cash << ' '
        << v.facility_income << ' ' << v.purchase_cost << ' ' << v.use_income << ' ' << v.purchases
        << ' ' << v.deposits << ' ' << v.processed << ' ' << v.discoveries << ' ' << v.crafted
        << ' ' << v.used << ' ' << v.healed << ' ' << v.recipe << ' ' << v.item << ' '
        << v.recipient << ' ' << v.hp_before << ' ' << v.hp_after << ' ' << v.craft_month << ' '
        << v.use_month << ' ' << next_market_tick_ << ' ' << initialized_ << '\n';
    v.trade.encode(out);
    require(bool(out), "cannot encode pot evidence");
}
ActivePotVillageStrategy ActivePotVillageStrategy::decode(std::istream &in) {
    ActivePotVillageStrategy result;
    auto &v = result.stats_;
    std::string magic;
    in >> magic;
    require(magic == "ARK_ACTIVE_POT_2", "unknown pot strategy evidence");
    in >> v.commands >> v.ticks >> v.healed_actor >> v.minimum_cash >> v.facility_income >>
        v.purchase_cost >> v.use_income >> v.purchases >> v.deposits >> v.processed >>
        v.discoveries >> v.crafted >> v.used >> v.healed >> v.recipe >> v.item >> v.recipient >>
        v.hp_before >> v.hp_after >> v.craft_month >> v.use_month >> result.next_market_tick_ >>
        result.initialized_;
    v.trade.decode(in);
    require(bool(in) && v.minimum_cash >= 0 && v.facility_income >= 0 && v.purchase_cost >= 0 &&
                v.purchases >= 0 && v.deposits >= 0 && v.processed >= 0 &&
                v.processed <= v.deposits && v.discoveries >= 0 && v.discoveries <= 1 &&
                v.crafted >= 0 && v.crafted <= v.discoveries && v.used >= 0 &&
                v.used <= v.crafted && v.healed >= 0 && v.healed <= v.used && v.recipe == 7 &&
                v.item == 29 &&
                (!v.healed || (v.hp_before > 0 && v.hp_after > v.hp_before && v.healed_actor)),
            "invalid pot evidence totals");
    result.decoded_evidence_ = true;
    return result;
}
} // namespace ark::test
