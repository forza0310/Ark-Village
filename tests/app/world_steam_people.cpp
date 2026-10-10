#include "world_steam_people.hpp"
#include "../support/world_fixture.hpp"
#include "ark/simulation/actors/rules/human_management.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace ref = sim::rules;
using State = app::WorldState;
using Command = app::WorldCommand;
using Kind = app::WorldCommandKind;
using Human = sim::StartupHumanPageAction;
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(std::string("Steam people: ") + message);
}
const ref::WorldScriptPage *top(const State &s) {
    for (auto p = s.scripts.pages.rbegin(); p != s.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4)
            return &*p;
    return nullptr;
}
std::int64_t cash(const State &s) { return s.scene.world.world.ai.accounting.funds(); }
int spending(const State &s, int human) {
    const auto i = s.scene.world.world.human_spending.find(human);
    return i == s.scene.world.world.human_spending.end() ? 0 : i->second;
}
Command human_command(std::uint64_t page, Human action, int selection = 0) {
    Command c;
    c.kind = Kind::human_action;
    c.page = page;
    c.human_action = action;
    c.selection = selection;
    return c;
}
int kind(int slot) { return slot == 0 ? 1 : slot == 3 ? 3 : 2; }
std::optional<ref::HumanManagementEquipment> equipment(const State &s, int slot, int id) {
    const auto f =
        std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(),
                     [&](const auto &e) { return e.shop.kind == kind(slot) && e.shop.id == id; });
    const auto current = s.catalog.find({kind(slot), id});
    if (f == s.rules->equipment.end() || current == s.catalog.end() ||
        (kind(slot) == 2 && (f->shop.type == 2 ? 1 : 2) != slot))
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
bool useful(const State &s, int person, const ref::HumanManagementEquipment &target) {
    if (!target.availability || target.stock <= 0)
        return false;
    const auto old_id = s.shop_humans.at(person).equipment[target.slot];
    if (old_id == target.definition)
        return false; // Do not spend a one-use reward just to give the same equipment again.
    const auto old = old_id ? equipment(s, target.slot, *old_id) : std::nullopt;
    if (old_id && !old)
        return false;
    const std::array<int, 4> prior = old ? old->combat : std::array<int, 4>{};
    return (!old || target.grade >= old->grade) &&
           std::equal(prior.begin(), prior.end(), target.combat.begin(),
                      [](int a, int b) { return a <= b; });
}
// Source raw87 contribution ranking is an input list, not the medal priority.
// e.E is the same persisted field exposed as StartupHumanDetails::medals.
std::optional<int> award_index(const State &s, const std::vector<int> &ranked) {
    std::optional<int> best;
    for (int i = 0; i < static_cast<int>(ranked.size()); ++i) {
        const int h = ranked[i];
        const auto present = s.human_presence.find(h);
        if (present == s.human_presence.end() || !present->second || !s.human_calendar.count(h))
            continue;
        if (!best ||
            std::make_tuple(s.human_calendar.at(h).celebrations, -std::int64_t(spending(s, h)), h) <
                std::make_tuple(s.human_calendar.at(ranked[*best]).celebrations,
                                -std::int64_t(spending(s, ranked[*best])), ranked[*best]))
            best = i;
    }
    return best;
}
} // namespace

IncomeDecision SteamPeopleStrategy::next(const State &s, int reserved_points,
                                         std::int64_t cash_reserve, bool start) {
    (void)reserved_points; // No paid cultivation or speculative profession change in this policy.
    (void)cash_reserve; // Reward equipment has a verified zero charge; housing reserves separately.
    const auto *p = top(s);
    if (!p || s.scene.framework_paused)
        return {};
    if (p->legacy_page == 87 && p->kind == ref::WorldScriptPageKind::raw_page) {
        const auto list = s.award_rankings.find(p->id);
        if (list == s.award_rankings.end())
            return {true, {}}; // Wait for real page initialization; do not synthesize its ranking.
        Command c;
        c.kind = Kind::award_action;
        c.page = p->id;
        if (s.award_pending_humans.count(p->id))
            c.award_action = ref::WorldAwardAction::confirm_award;
        else if (s.award_termination_pending.at(p->id))
            c.award_action = ref::WorldAwardAction::confirm_termination;
        else if (const auto selected = award_index(s, list->second);
                 s.medal_count > 0 && selected) {
            c.award_action = ref::WorldAwardAction::request_award;
            c.selection = *selected;
        } else
            c.award_action = ref::WorldAwardAction::request_termination;
        return {true, c};
    }
    if (p->kind == ref::WorldScriptPageKind::scene) {
        if (gift_) {
            require(consumed_, "gift flow returned to world without consuming its reward");
            gift_.reset();
            consumed_ = false;
        }
        if (!start || s.scene.scene_state != 0 || s.build_definition || s.build_mode)
            return {};
        std::optional<std::tuple<int, std::int64_t, int, int, int>> priority;
        for (const auto &[person, presence] : s.human_presence) {
            if (!presence)
                continue;
            const auto info = sim::startup_world_human_details(s, person);
            if (!info || !info->live_actor ||
                s.scene.world.world.ai.battle.actors.at(*info->live_actor).control.state == 4)
                continue;
            const auto count = equipment_gifts_.find(person);
            const int received = count == equipment_gifts_.end() ? 0 : count->second;
            for (int slot = 0; slot < 4; ++slot)
                for (const auto &[key, entry] : s.catalog) {
                    if (key.first != kind(slot) || entry.free_purchases <= 0)
                        continue;
                    const auto target = equipment(s, slot, key.second);
                    if (!target || !useful(s, person, *target))
                        continue;
                    const auto quote = ref::human_equipment_gift_cost(*target);
                    if (!quote || *quote != -1)
                        continue;
                    const auto value = std::make_tuple(received, -std::int64_t(spending(s, person)),
                                                       person, slot, key.second);
                    if (!priority || value < *priority) {
                        priority = value;
                        gift_ = Gift{person, slot, key.second};
                    }
                }
        }
        if (!gift_)
            return {};
        consumed_ = false;
        Command c;
        c.kind = Kind::open_human;
        c.definition = gift_->human;
        c.actor = *sim::startup_world_human_details(s, gift_->human)->live_actor;
        return {true, c};
    }
    if (!gift_)
        return {};
    const int raw = p->legacy_page;
    if (raw != 60 && raw != 64 && raw != 65 && raw != 66 && raw != 68 && raw != 69 && raw != 70)
        return {}; // Script/effort subpages retain their existing automatic-player consumer.
    if (!sim::startup_world_human_page_ready(s, p->id))
        return {true, {}};
    require(s.page_human_bindings.at(p->id) == gift_->human, "gift page changed recipient");
    if (raw == 60)
        return {true, human_command(p->id, consumed_ ? Human::cancel : Human::gifts)};
    if (raw == 64) {
        if (s.human_page_answers.count(p->id))
            return {true,
                    {}}; // Confirmation only queues the answer; the next owner tick equips it.
        if (consumed_)
            return {true, human_command(p->id, Human::cancel)};
        const auto target = equipment(s, gift_->slot, gift_->definition);
        require(target && useful(s, gift_->human, *target),
                "selected reward is no longer available");
        const auto &list = s.equipment_page_catalogs.at(p->id)[gift_->slot];
        const auto i = std::find(list.begin(), list.end(), gift_->definition);
        require(i != list.end(), "reward equipment missing from actual gift catalogue");
        const int index = static_cast<int>(i - list.begin());
        if (s.page_phases.at(p->id) != gift_->slot)
            return {true, human_command(p->id, Human::equipment_slot, gift_->slot)};
        return {true, human_command(p->id,
                                    s.human_page_selections.at(p->id) == index ? Human::confirm
                                                                               : Human::select,
                                    index)};
    }
    if (raw == 70 && s.page_phases.at(p->id) == 2 && s.human_page_selections.at(p->id) != 1)
        return {true, human_command(p->id, Human::select, 1)};
    return {true, human_command(p->id, Human::confirm)};
}

void SteamPeopleStrategy::observe_world(const State &before, const State &after) {
    if (!gift_ || consumed_)
        return;
    const auto &g = *gift_;
    const auto key = std::make_pair(kind(g.slot), g.definition);
    if (before.catalog.at(key).free_purchases == after.catalog.at(key).free_purchases)
        return;
    require(before.catalog.at(key).free_purchases == after.catalog.at(key).free_purchases + 1 &&
                after.shop_humans.at(g.human).equipment[g.slot] == g.definition &&
                cash(before) == cash(after),
            "reward must consume one qualification and equip without charge");
    const auto old = sim::startup_world_human_details(before, g.human);
    const auto now = sim::startup_world_human_details(after, g.human);
    require(old && now &&
                std::equal(old->combat.begin(), old->combat.end(), now->combat.begin(),
                           [](int a, int b) { return a <= b; }),
            "gift reduced recipient combat attributes");
    consumed_ = true;
    ++equipment_gifts_[g.human];
    std::cout << "PEOPLE gift human=" << g.human << " slot=" << g.slot
              << " equipment=" << g.definition << " received=" << equipment_gifts_.at(g.human)
              << std::endl;
}
void SteamPeopleStrategy::observe(const State &before, const Command &c,
                                  const app::WorldCommandResult &result, const State &after) {
    if (result.outcome != app::WorldCommandOutcome::applied)
        return;
    if (c.kind == Kind::award_action && c.award_action == ref::WorldAwardAction::confirm_award &&
        before.medal_count != after.medal_count) {
        const int person = before.award_pending_humans.at(c.page);
        require(before.medal_count == after.medal_count + 1 &&
                    before.human_calendar.at(person).celebrations + 1 ==
                        after.human_calendar.at(person).celebrations,
                "award must consume one real medal and reward the bound person once");
        ++awarded_;
        std::cout << "PEOPLE medal human=" << person
                  << " medals=" << after.human_calendar.at(person).celebrations
                  << " spending=" << spending(before, person) << std::endl;
    }
    observe_world(before, after);
}
void SteamPeopleStrategy::observe_tick(const State &before, const State &after) {
    observe_world(before, after);
}

std::optional<int> SteamPeopleStrategy::eligible_homeless(const State &s, std::int64_t reserve) {
    std::optional<int> best;
    for (const auto &h : s.rules->humans) {
        const int id = h.identity;
        if (!s.human_presence.at(id) || s.human_homes.at(id)[2] != 0 ||
            s.shop_humans.at(id).satisfaction < h.residence_threshold ||
            cash(s) - reserve < h.residence_fee)
            continue;
        if (!best || std::make_pair(-std::int64_t(spending(s, id)), id) <
                         std::make_pair(-std::int64_t(spending(s, *best)), *best))
            best = id;
    }
    return best;
}
void SteamPeopleStrategy::encode(std::ostream &out) const {
    require(!gift_, "cannot save a pending gift flow");
    out << "STEAM_PEOPLE_1 " << awarded_ << ' ' << equipment_gifts_.size() << '\n';
    for (const auto &[person, count] : equipment_gifts_)
        out << person << ' ' << count << '\n';
    require(bool(out), "failed to encode controller");
}
SteamPeopleStrategy SteamPeopleStrategy::decode(std::istream &in) {
    SteamPeopleStrategy s;
    std::string magic;
    int count{};
    require(bool(in >> magic >> s.awarded_ >> count) && magic == "STEAM_PEOPLE_1" &&
                s.awarded_ >= 0 && count >= 0 && count <= 1000,
            "invalid controller header");
    for (int i = 0; i < count; ++i) {
        int person{}, gifts{};
        require(bool(in >> person >> gifts) && person >= 0 && person < 1000 && gifts > 0 &&
                    s.equipment_gifts_.emplace(person, gifts).second,
                "invalid gift history");
    }
    return s;
}

void steam_people_contract() {
    auto s = initial_world();
    const std::vector<int> list{0, 1, 2};
    for (const int person : list) {
        s.human_presence.at(person) = 1;
        s.human_calendar.at(person).celebrations = 0;
        s.scene.world.world.human_spending.at(person) = (person + 1) * 100;
    }
    s.human_calendar.at(2).celebrations = 1;
    require(award_index(s, list) == 1, "medal fairness precedes higher spending");
    s.human_calendar.at(2).celebrations = 0;
    require(award_index(s, list) == 2, "equal medals prefer actual spending");
    s.human_presence.at(2) = 0;
    require(award_index(s, list) == 1, "absent definition cannot receive award");
    require(!SteamPeopleStrategy::eligible_homeless(s, cash(s) + 1),
            "housing must preserve reserved cash");
    // Selection tests use isolated input projections, not injected campaign rewards.
    auto target = ref::HumanManagementEquipment{};
    target.definition = 0;
    target.availability = target.stock = 1;
    target.combat = {1, 0, 0, 0};
    s.shop_humans.at(0).equipment[0].reset();
    require(useful(s, 0, target), "real qualification can fill an empty slot");
    target.stock = 0;
    require(!useful(s, 0, target), "unlocked equipment without reward qualification is not free");
    target.stock = 1;
    target.combat[0] = -1;
    require(!useful(s, 0, target), "free equipment must not lower a combat attribute");
    target.combat[0] = 1;
    s.shop_humans.at(0).equipment[0] = target.definition;
    require(!useful(s, 0, target), "do not waste a reward on an already equipped item");
    std::istringstream input("STEAM_PEOPLE_1 3 2\n0 2\n1 1\n");
    const auto restored = SteamPeopleStrategy::decode(input);
    std::ostringstream output;
    restored.encode(output);
    require(output.str() == "STEAM_PEOPLE_1 3 2\n0 2\n1 1\n",
            "save must retain recipient fairness history");
    bool rejected{};
    try {
        std::istringstream bad("STEAM_PEOPLE_1 0 2\n1 1\n1 1\n");
        (void)SteamPeopleStrategy::decode(bad);
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    require(rejected, "duplicate saved recipient must be rejected");
}
} // namespace ark::test
