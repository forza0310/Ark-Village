// Bind source task commits to campaign evidence. These queries never advance a task,
// award a catalog entry or retain an Owner; ambiguous co-frame origins stay unverified.
#include "world_campaign_coverage.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <stdexcept>
#include <tuple>

namespace ark::test {
namespace {
using State = app::WorldState;
namespace ref = simulation::rules;
} // namespace

std::optional<CampaignReceipt> task_victory_receipt(const State &before, const State &after,
                                                    std::uint64_t departed_task) {
    if (before.task_progress.successes == after.task_progress.successes)
        return {}; // Cold-loaded totals and queued results do not reconstruct a victory.
    const auto need = [](bool value) {
        if (!value)
            throw std::runtime_error(
                "Task victory lost its departure, completion or result binding");
    };
    need(departed_task && before.active_task == departed_task && !after.active_task &&
         !after.scene.world.world.ai.task_active && before.tasks.count(departed_task) &&
         after.tasks.count(departed_task) && !before.participants.empty() &&
         before.participants == after.participants &&
         std::int64_t(after.task_progress.successes) ==
             std::int64_t(before.task_progress.successes) + 1);
    const auto &task = before.tasks.at(departed_task);
    need(task.definition == after.tasks.at(departed_task).definition &&
         before.task_progress.definitions.size() == after.task_progress.definitions.size());
    for (const auto &[definition, progress] : before.task_progress.definitions) {
        const auto updated = after.task_progress.definitions.find(definition);
        need(updated != after.task_progress.definitions.end() &&
             std::int64_t(updated->second.completed) ==
                 std::int64_t(progress.completed) + (definition == task.definition ? 1 : 0));
    }
    // The source removes the first list reference; duplicate task references remain legal.
    const auto references = [&](const State &s) {
        return std::count(s.task_order.begin(), s.task_order.end(), departed_task);
    };
    need(references(before) == references(after) + 1);
    std::uint64_t page{};
    for (const auto &[id, summary] : after.exploration_summaries) {
        if (summary.legacy_page != 30 || summary.task != departed_task ||
            before.exploration_summaries.count(id))
            continue;
        const auto source_id = id;
        const auto source = std::find_if(after.scripts.pages.begin(), after.scripts.pages.end(),
                                         [source_id](const auto &p) { return p.id == source_id; });
        need(!page && summary.definition == task.definition &&
             source != after.scripts.pages.end() && source->lifecycle != 4 &&
             source->kind == ref::WorldScriptPageKind::raw_page &&
             // Exploration binds only its summary payload; encounter pages also carry
             // the optional duplicate references. Any present duplicate must agree.
             source->legacy_page == 30 &&
             (!source->task_identity || source->task_identity == departed_task) &&
             (!source->task_definition || source->task_definition == task.definition) &&
             std::none_of(before.scripts.pages.begin(), before.scripts.pages.end(),
                          [source_id](const auto &p) { return p.id == source_id; }));
        page = id;
    }
    need(page && before.task_progress.definitions.count(task.definition));
    const auto values = [&](const State &s) -> std::array<std::int64_t, 4> {
        return {s.task_progress.successes,
                s.task_progress.definitions.at(task.definition).completed,
                s.task_progress.ordinary_explorations, s.task_progress.task_pool_progress};
    };
    return CampaignReceipt{CampaignReceiptKind::task_victory_committed,
                           task.definition,
                           after.simulation_steps,
                           page,
                           departed_task,
                           -1,
                           values(before),
                           values(after)};
}

CampaignLootObservation task_loot_receipts(const State &before, const State &after) {
    if (!before.active_task || before.active_task != after.active_task)
        return {}; // A result page or a loaded inventory is not a direct challenge grant.
    const auto task_id = *before.active_task;
    const auto &task = before.tasks.at(task_id);
    if (!task.facility || !before.dungeon_facilities.count(*task.facility) ||
        !after.dungeon_facilities.count(*task.facility) ||
        before.scene.world.world.facilities.at(*task.facility).category != 5 ||
        before.scene.world.world.facilities.at(*task.facility).status != 1)
        return {};
    const auto &old_challenges = before.dungeon_facilities.at(*task.facility).challenges;
    const auto &new_challenges = after.dungeon_facilities.at(*task.facility).challenges;
    if (old_challenges == new_challenges)
        return {}; // Modal/paused frames retain age0; they cannot grant the same loot again.
    using Challenge = std::tuple<int, int, int>; // Threshold, reward kind, original definition.
    std::map<Challenge, int> eligible, granted;
    for (const auto &row : old_challenges)
        if (row[1] == 0 && row[2] == 0)
            ++eligible[{row[0], row[4], row[5]}];
    for (const auto &row : new_challenges)
        if (row[1] == 0 && row[2] == 2 && row[3] == 0)
            ++granted[{row[0], row[4], row[5]}];
    if (granted.empty())
        return {};
    std::map<std::pair<int, int>, int> counts;
    for (const auto &[key, count] : granted) {
        if (eligible[key] != count)
            return {{}, "challenge_transition_binding"};
        counts[{std::get<1>(key), std::get<2>(key)}] += count;
    }
    // Timed ground pickup grants before facilities in the same world round. Do not
    // attribute its overlapping catalog mutation to a dungeon challenge instead.
    int ground_items{};
    for (const auto &[id, object] : before.scene.world.world.ai.battle.objects) {
        const auto updated = after.scene.world.world.ai.battle.objects.find(id);
        if (object.state == 5 && object.counter == 19 &&
            updated != after.scene.world.world.ai.battle.objects.end() &&
            updated->second.state == 5 && updated->second.counter == 20 &&
            updated->second.kind == object.kind &&
            updated->second.definition == object.definition) {
            if (counts.count({object.kind, object.definition}))
                return {{}, "same_definition_ground_pickup"};
            if (object.kind == 0)
                ++ground_items; // Account the sharedE counter without claiming this ground loot.
        }
    }
    int ordinary{};
    for (const auto &[key, count] : counts)
        if (key.first == 0)
            ordinary += count;
    if ((std::int64_t(before.item_rewards) + ordinary + ground_items) %
            std::numeric_limits<int>::max() !=
        after.item_rewards)
        return {{}, "unaccounted_item_grant"};
    std::uint64_t page{};
    for (auto p = before.scripts.pages.rbegin(); p != before.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4) {
            page = p->id;
            break;
        }
    if (!page || after.tasks.at(task_id).definition != task.definition ||
        after.tasks.at(task_id).facility != task.facility)
        return {{}, "task_or_frame_binding"};
    const CampaignReceiptKind kinds[]{CampaignReceiptKind::item_task_loot_claimed,
                                      CampaignReceiptKind::weapon_task_loot_claimed,
                                      CampaignReceiptKind::armour_task_loot_claimed,
                                      CampaignReceiptKind::accessory_task_loot_claimed};
    CampaignLootObservation result;
    for (const auto &[key, count] : counts) {
        if (key.first < 0 || key.first > 3 || !before.catalog.count(key) ||
            !after.catalog.count(key))
            return {{}, "reward_definition_binding"};
        const auto &old = before.catalog.at(key), &now = after.catalog.at(key);
        const int expected_status = key.first == 0 && old.status != 0 ? old.status : 1;
        const int expected_quantity =
            key.first == 0 ? int(std::min<std::int64_t>(999, std::int64_t(old.inventory) + count))
            : old.status == 0 ? 1
                              : old.free_purchases;
        const int previous = key.first == 0 ? old.inventory : old.free_purchases;
        const int current = key.first == 0 ? now.inventory : now.free_purchases;
        if (now.status != expected_status || current != expected_quantity ||
            (key.first == 0 && after.items.at(key.second).inventory != now.inventory))
            return {{}, "catalog_effect_not_isolated"};
        result.receipts.push_back({kinds[key.first],
                                   key.second,
                                   after.simulation_steps,
                                   page,
                                   task_id,
                                   -1,
                                   {previous, old.status, count, task.definition},
                                   {current, now.status, count, task.definition}});
    }
    return result;
}

} // namespace ark::test
