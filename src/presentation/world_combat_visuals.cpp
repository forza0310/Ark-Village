#include "ark/presentation/world_combat_visuals.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace ark::desktop {
namespace {
namespace rules = simulation::rules;

void digits(OverlayPlan &plan, const std::string &sprite, int amount, float x, float y) {
    if (amount < 0)
        throw std::invalid_argument("Combat display requires a nonnegative amount");
    for (const char digit : std::to_string(amount)) {
        plan.emplace_back(OverlaySprite{sprite, digit - '0', x, y});
        x += 8;
    }
}

// The maintained growth consumer transfers floor(O*N/9) into L. Display its unconsumed
// complement, also used by prepare_delayed_reward, so paid XP plus shown remainder conserves N.
// This is a data-consistent presentation adaptation: the published UI map(O,0,9,N,0) does not
// specify its precise truncation order, which still requires a maintained research contract.
int remaining_experience(const rules::DelayedRewardState &pending) {
    if (pending.amount < 0)
        throw std::invalid_argument("Negative pending experience");
    return pending.amount - static_cast<int>(static_cast<std::int64_t>(pending.amount) *
                                             std::clamp(pending.counter, 0, 9) / 9);
}

void experience_bar(OverlayPlan &plan, const rules::RewardHumanDefinition &growth,
                    const std::vector<rules::HumanProfessionRule> &professions) {
    const auto &definition = growth.definition;
    const int job = definition.current_profession;
    if (job < 0 || static_cast<std::size_t>(job) >= definition.profession_levels.size() ||
        static_cast<std::size_t>(job) >= professions.size() || growth.experience < 0)
        throw std::invalid_argument("Invalid experience display definition");
    const int level = definition.profession_levels.at(job);
    const auto threshold = rules::human_growth_threshold(level, professions.at(job).difficulty);
    if (!threshold || *threshold <= 0)
        throw std::invalid_argument("Invalid experience display threshold");
    const int fill =
        level >= 10 ? 31
                    : static_cast<int>(std::clamp<std::int64_t>(
                          static_cast<std::int64_t>(growth.experience) * 31 / *threshold, 0, 31));
    plan.emplace_back(OverlayRectangle{-17, -24, 34, 5, {246, 246, 246}});
    plan.emplace_back(OverlayRectangle{-16, -23, 32, 3, {39, 53, 74}});
    if (fill > 0)
        plan.emplace_back(OverlayRectangle{-15, -22, static_cast<float>(fill), 2, {255, 109, 15}});
    if (fill < 31)
        plan.emplace_back(OverlayRectangle{-15.F + fill, -22, 31.F - fill, 2, {68, 100, 104}});
}
} // namespace

OverlayPlan world_actor_combat_visuals(const simulation::StartupWorldRuntimeState &state,
                                       rules::CharacterId id) {
    const auto &ai = state.scene.world.world.ai;
    const auto &actor = ai.battle.actors.at(id);
    OverlayPlan plan;
    if (actor.control.flags & 1U)
        return plan;
    if (actor.label_timer > 0 && !actor.miss_label) {
        const auto count = std::to_string(actor.damage_total).size();
        digits(plan, actor.kind == rules::ActorKind::human ? "number07.seb" : "number06.seb",
               actor.damage_total, -4.F * count, -38);
    }
    if (actor.kind != rules::ActorKind::human)
        return plan;
    const auto context = ai.contexts.find(id);
    if (context == ai.contexts.end())
        return plan;
    for (const auto &effect : context->second.effects.display) {
        if (effect.size() < 2)
            throw std::invalid_argument("Incomplete actor display record");
        const int count = effect[1];
        if (count < 0)
            continue;
        if (effect[0] == 14 && count < 48) {
            // ef_lvUp's own SEB (-20,-15) offset is applied by the sprite renderer once.
            plan.emplace_back(OverlaySprite{"ef_lvUp.seb", 0, 0, -23});
        } else if (effect[0] == 24 && count < 72) {
            const auto &growth = ai.growth.at(actor.definition);
            if (count < 55) {
                const int amount = remaining_experience(growth.pending);
                const float number_width = std::to_string(amount).size() * 8.F;
                const float width = number_width + 31;
                plan.emplace_back(OverlaySprite{"wnd_exp.seb", 0, -width / 2, -34});
                const float number_x = width / 2 - number_width;
                plan.emplace_back(OverlaySprite{"number05.seb", 14, number_x - 8, -35});
                digits(plan, "number05.seb", amount, number_x, -35);
            }
            experience_bar(plan, growth, ai.professions);
            // COMBAT_RENDER: shared definition P gates the pending badge. Its frame uses
            // cd24's own admitted counter, including the bar-only interval after tick55.
            // Reuse the current ordinary-body badge anchor; special bl offsets remain separate.
            if (growth.notice_pending)
                plan.emplace_back(OverlaySprite{"ef_lvUp.seb", (count % 12) / 6, 0, -23});
        }
    }
    return plan;
}

OverlayPlan world_cash_visuals(const std::vector<int> &effect) {
    OverlayPlan plan;
    if (effect.size() < 2 || (effect[0] != 2 && effect[0] != 3))
        return plan;
    if (effect.size() != 7 || effect[4] < 0)
        throw std::invalid_argument("Invalid world cash-display payload");
    const int count = effect[1];
    // startup_world_runtime_scene::effect_limits[2] and [3] are both 20; [4] is the separate
    // coin animation's 23. Both text kinds therefore expire at 20. Checking the
    // boundary also makes stale inspection records invisible without changing their lifetime.
    if (count < 0 || count >= 20)
        return plan;
    const float width = std::to_string(effect[4]).size() * 8.F + 9;
    float x = -width / 2;
    float y = -39;
    if (effect[0] == 2) {
        x += 28;
        const auto motion = (static_cast<std::int64_t>(effect[5]) * count +
                             static_cast<std::int64_t>(effect[6]) * count * (count + 1) / 2) /
                            1000;
        y = count < 6 ? -26.F + static_cast<float>(motion) : -36;
    }
    digits(plan, "number05.seb", effect[4], x, y);
    plan.emplace_back(OverlaySprite{"number05.seb", 20, x + width - 9, y});
    return plan;
}
} // namespace ark::desktop
