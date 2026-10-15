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
std::int64_t paid_budget(const State &s, std::int64_t reserve) {
    // Actual source category2 includes gifts, profession fees and admissions. Cap
    // their combined monthly spend for this player; do not add save-side counters
    // or subtract reconstructed fees from the authoritative ledger.
    const int spent = s.monthly_cash.at(s.scene.calendar.month)[2][1];
    if (cash(s) < reserve || spent < 0 || spent >= 10000)
        return 0;
    return std::min<std::int64_t>(10000 - spent, cash(s) - reserve);
}
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
bool gift_confirm_has_effect(const State &s, int raw, std::uint64_t page) {
    const int counter = s.page_counters.at(page);
    if (raw == 66)
        return counter < 40 || counter >= 85; // Preserve the source's first early fast-forward.
    if (raw == 68) {
        const auto plan =
            ref::prepare_world_effort_display(counter, true, s.equipment_attribute_display[2]);
        require(plan.has_value(), "equipment display has invalid source timing or deltas");
        return plan->closed || plan->counter != counter;
    }
    return true;
}
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
bool useful(const State &s, int person, const ref::HumanManagementEquipment &target,
            bool paid = false) {
    if (!target.availability || target.stock < 0 || (!paid && target.stock == 0))
        return false;
    const auto old_id = s.shop_humans.at(person).equipment[target.slot];
    if (old_id == target.definition)
        return false; // Do not spend a one-use reward just to give the same equipment again.
    const auto old = old_id ? equipment(s, target.slot, *old_id) : std::nullopt;
    if (old_id && !old)
        return false;
    const std::array<int, 4> prior = old ? old->combat : std::array<int, 4>{};
    return (!paid || !old || target.combat != prior) && (!old || target.grade >= old->grade) &&
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
    const auto *p = top(s);
    if (!p || s.scene.framework_paused)
        return {};
    if (professions_.active())
        return professions_.next(s, reserved_points, cash_reserve, false);
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
        const auto cultivation_budget = paid_budget(s, cash_reserve);
        std::optional<std::tuple<bool, int, std::int64_t, int, int, int>> priority;
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
                    if (key.first != kind(slot))
                        continue;
                    const auto target = equipment(s, slot, key.second);
                    const bool paid = entry.free_purchases == 0;
                    if (!target || !useful(s, person, *target, paid))
                        continue;
                    const auto quote = ref::human_equipment_gift_cost(*target);
                    if (!quote ||
                        (paid ? *quote <= 0 || *quote > cultivation_budget : *quote != -1))
                        continue;
                    const auto value =
                        std::make_tuple(paid, received, -std::int64_t(spending(s, person)), person,
                                        slot, key.second);
                    if (!priority || value < *priority) {
                        priority = value;
                        gift_ = Gift{person, slot, key.second, paid ? *quote : 0, target->stock};
                    }
                }
        }
        // Free acquired equipment keeps its old precedence. A legal surplus-point profession
        // flow can precede a paid gift, without changing gift fairness or serialized history.
        if (!gift_ || gift_->charge > 0) {
            const auto training = professions_.next(s, reserved_points, cash_reserve, true);
            if (training.handled) {
                gift_.reset();
                return training;
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
        require(target && useful(s, gift_->human, *target, gift_->charge > 0) &&
                    target->stock == gift_->stock,
                "selected equipment is no longer available");
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
    if ((raw == 66 || raw == 68) && !gift_confirm_has_effect(s, raw, p->id))
        return {true, {}}; // Keep the next real update; avoid a known no-op input transaction.
    return {true, human_command(p->id, Human::confirm)};
}

bool steam_equipment_reward_usable(const State &s, int equipment_kind, int definition) {
    const auto source =
        std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(), [&](const auto &e) {
            return e.shop.kind == equipment_kind && e.shop.id == definition;
        });
    if (source == s.rules->equipment.end())
        return false;
    const int slot = equipment_kind == 1      ? 0
                     : equipment_kind == 3    ? 3
                     : source->shop.type == 2 ? 1
                                              : 2;
    auto target = equipment(s, slot, definition);
    if (!target)
        return false;
    target->availability = 1;
    target->stock = 1;
    for (const auto &[human, presence] : s.human_presence) {
        const auto details = presence ? sim::startup_world_human_details(s, human) : std::nullopt;
        if (details && details->live_actor &&
            s.scene.world.world.ai.battle.actors.at(*details->live_actor).control.state != 4 &&
            useful(s, human, *target))
            return true;
    }
    return false;
}

void SteamPeopleStrategy::observe_world(const State &before, const State &after,
                                        std::vector<CampaignReceipt> &pending) {
    if (!gift_ || consumed_)
        return;
    const auto &g = *gift_;
    const auto key = std::make_pair(kind(g.slot), g.definition);
    if (before.shop_humans.at(g.human).equipment[g.slot] ==
        after.shop_humans.at(g.human).equipment[g.slot])
        return;
    require(before.catalog.at(key).free_purchases == g.stock &&
                after.catalog.at(key).free_purchases == (g.charge > 0 ? g.stock : g.stock - 1) &&
                after.shop_humans.at(g.human).equipment[g.slot] == g.definition &&
                cash(before) - cash(after) == g.charge,
            "gift must consume its source qualification or pay its actual quote exactly once");
    const auto old = sim::startup_world_human_details(before, g.human);
    const auto now = sim::startup_world_human_details(after, g.human);
    const auto target = equipment(before, g.slot, g.definition);
    require(old && now && target && old->live_actor && old->live_actor == now->live_actor &&
                std::equal(old->combat.begin(), old->combat.end(), now->combat.begin(),
                           [](int a, int b) { return a <= b; }),
            "gift reduced recipient combat attributes");
    const auto quote = ref::human_equipment_gift_cost(*target);
    require(quote && (*quote == -1 ? 0 : *quote) == g.charge,
            "equipment charge differs from the real source gift quote");
    require(after.scene.world.world.ai.growth.at(g.human).definition.equipment[g.slot] ==
                    target->combat &&
                after.scene.world.world.ai.battle.actors.at(*now->live_actor).capacity ==
                    now->combat[0],
            "equipped source attributes or live capacity were not synchronized");
    std::uint64_t page{};
    for (const auto &p : before.scripts.pages)
        if (p.legacy_page == 64 && p.lifecycle != 4 && before.human_page_answers.count(p.id) &&
            before.page_human_bindings.at(p.id) == g.human) {
            require(!page, "reward observation has multiple queued gift pages");
            page = p.id;
        }
    require(page != 0, "equipped reward lacks its real queued source page");
    using Receipt = CampaignReceiptKind;
    const auto equipped = g.charge > 0 ? (kind(g.slot) == 1   ? Receipt::weapon_paid_equipped
                                          : kind(g.slot) == 2 ? Receipt::armour_paid_equipped
                                                              : Receipt::accessory_paid_equipped)
                                       : (kind(g.slot) == 1   ? Receipt::weapon_reward_equipped
                                          : kind(g.slot) == 2 ? Receipt::armour_reward_equipped
                                                              : Receipt::accessory_reward_equipped);
    const auto applied = kind(g.slot) == 1   ? Receipt::weapon_combat_applied
                         : kind(g.slot) == 2 ? Receipt::armour_combat_applied
                                             : Receipt::accessory_combat_applied;
    pending.push_back({equipped,
                       g.definition,
                       after.simulation_steps,
                       page,
                       now->live_actor->value,
                       g.human,
                       {before.catalog.at(key).free_purchases, cash(before), g.slot,
                        old->equipment[g.slot].value_or(-1)},
                       {after.catalog.at(key).free_purchases, cash(after), g.slot, g.definition}});
    pending.push_back({applied,
                       g.definition,
                       after.simulation_steps,
                       page,
                       now->live_actor->value,
                       g.human,
                       {old->combat[0], old->combat[1], old->combat[2], old->combat[3]},
                       {now->combat[0], now->combat[1], now->combat[2], now->combat[3]}});
    consumed_ = true;
    ++equipment_gifts_[g.human];
    std::cout << "PEOPLE gift human=" << g.human << " slot=" << g.slot
              << " equipment=" << g.definition << " charge=" << g.charge
              << " received=" << equipment_gifts_.at(g.human) << std::endl;
}
void SteamPeopleStrategy::observe(const State &before, const Command &c,
                                  const app::WorldCommandResult &result, const State &after,
                                  const CampaignReceiptSink &receipts) {
    professions_.observe(before, c, result, after, receipts);
    if (result.outcome != app::WorldCommandOutcome::applied)
        return;
    std::vector<CampaignReceipt> pending;
    if (c.kind == Kind::award_action && c.award_action == ref::WorldAwardAction::confirm_award &&
        before.medal_count != after.medal_count) {
        const int person = before.award_pending_humans.at(c.page);
        require(before.medal_count == after.medal_count + 1 &&
                    before.human_calendar.at(person).celebrations + 1 ==
                        after.human_calendar.at(person).celebrations,
                "award must consume one real medal and reward the bound person once");
        ++awarded_;
        pending.push_back({CampaignReceiptKind::human_medal_awarded,
                           person,
                           after.simulation_steps,
                           c.page,
                           0,
                           person,
                           {before.medal_count, before.human_calendar.at(person).celebrations,
                            cash(before), spending(before, person)},
                           {after.medal_count, after.human_calendar.at(person).celebrations,
                            cash(after), spending(after, person)}});
        std::cout << "PEOPLE medal human=" << person
                  << " medals=" << after.human_calendar.at(person).celebrations
                  << " spending=" << spending(before, person) << std::endl;
    }
    observe_world(before, after, pending);
    if (receipts)
        for (const auto &receipt : pending)
            receipts(receipt);
}
void SteamPeopleStrategy::observe_tick(const State &before, const State &after,
                                       const CampaignReceiptSink &receipts) {
    professions_.observe_tick(before, after, receipts);
    std::vector<CampaignReceipt> pending;
    observe_world(before, after, pending);
    if (receipts)
        for (const auto &receipt : pending)
            receipts(receipt);
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
    require(!active(), "cannot save a pending gift or profession flow");
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
    // A legal live record from the real producer, then explicit selection qualifications.
    // This is a short player-policy fixture, never a natural mastery or acquired-stock claim.
    auto training = s;
    const auto arrival = sim::startup_world_runtime_adapter().arrival;
    const int count = training.arrival_counter;
    for (int n = 0; n < count && training.scene.world.world.ai.human_order.empty(); ++n) {
        auto next = arrival(training);
        require(next.has_value(), "profession coordination needs the real source first arrival");
        training = std::move(*next);
    }
    require(!training.scene.world.world.ai.human_order.empty(),
            "profession fixture has no live actor");
    const auto actor = training.scene.world.world.ai.human_order.front();
    const int human = training.scene.world.world.ai.battle.actors.at(actor).definition;
    training.scripts.pages.resize(1); // Explicit stable-page qualification; original s stays cold.
    training.scene.scene_state = 0;
    auto &growth = training.scene.world.world.ai.growth.at(human);
    growth.definition.current_profession = 0;
    growth.definition.profession_levels.at(0) = 10;
    training.actor_metadata.at(actor).profession = 0;
    training.scripts.professions.at(5).status = 1;
    training.human_calendar.at(human).celebrations = 99;
    training.village_points = 999;
    training.scene.world.world.ai.accounting = ref::PeriodAccounting(100000);
    for (auto &[key, entry] : training.catalog)
        if (key.first != 0)
            entry.status = 0; // No competing equipment offer in this profession-only qualification.
    SteamPeopleStrategy deferred;
    require(!deferred.next(training, 1000, 0, true).handled && !deferred.active(),
            "unfunded reserved goals must prevent the player opening a profession flow");
    SteamPeopleStrategy cultivation;
    const auto proposed = cultivation.next(training, 0, 0, true);
    require(proposed.handled && proposed.command && proposed.command->kind == Kind::open_human &&
                proposed.command->definition == human && cultivation.active(),
            "legal surplus-point cultivation must start through the real human entry");
    bool pending_rejected{};
    std::ostringstream pending;
    try {
        cultivation.encode(pending);
    } catch (const std::runtime_error &) {
        pending_rejected = true;
    }
    require(pending_rejected && pending.str().empty() && training.village_points == 999 &&
                growth.definition.current_profession == 0 &&
                s.scene.world.world.ai.human_order.empty(),
            "pending profession cannot enter the unchanged sidecar or mutate the source new-world "
            "Owner");
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
    auto budget = s;
    budget.scene.world.world.ai.accounting = ref::PeriodAccounting(12000);
    budget.monthly_cash.at(budget.scene.calendar.month)[2][1] = 9700;
    require(
        paid_budget(budget, 11000) == 300 && paid_budget(budget, 11701) == 299,
        "gift decisions must retain both the monthly300 allowance and cash reserve differing by1G");
    budget.monthly_cash.at(budget.scene.calendar.month)[2][1] = 10000;
    require(paid_budget(budget, 11000) == 0,
            "paid cultivation must stop at the actual monthly staff/gift expense limit");
    // Selection tests use isolated input projections, not injected campaign rewards.
    auto target = ref::HumanManagementEquipment{};
    target.definition = 0;
    target.availability = target.stock = 1;
    target.combat = {1, 0, 0, 0};
    s.shop_humans.at(0).equipment[0].reset();
    require(useful(s, 0, target), "real qualification can fill an empty slot");
    target.stock = 0;
    require(!useful(s, 0, target), "unlocked equipment without reward qualification is not free");
    target.price = 200;
    require(useful(s, 0, target, true) && ref::human_equipment_gift_cost(target) == 300,
            "a paid weapon uses the source gift quote, not its shop price or a free qualification");
    target.availability = 0;
    require(!useful(s, 0, target, true),
            "paid gifts cannot bypass an unopened equipment definition");
    target.availability = 1;
    target.stock = 1;
    target.combat[0] = -1;
    require(!useful(s, 0, target), "free equipment must not lower a combat attribute");
    target.combat[0] = 1;
    s.shop_humans.at(0).equipment[0] = target.definition;
    require(!useful(s, 0, target), "do not waste a reward on an already equipped item");
    require(!useful(s, 0, target, true),
            "paid gifts cannot rebuy the same equipped item as an improvement");
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
