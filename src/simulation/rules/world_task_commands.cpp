#include "ark/simulation/rules/world_task_commands.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation::rules {
namespace {
using State = WorldTaskCommandState;
using Candidate = TaskCommandCandidate;
using CommandError = TaskCommandError;
using Effect = TaskCommandEffectKind;
TaskCommandResult fail(CommandError error) { return {error, {}}; }
bool fits(std::int64_t value) {
    return value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max();
}
std::optional<int> scale(int value, int from, int to, int low, int high, bool clamp = true) {
    if (clamp) {
        if (value < from)
            return low;
        if (value > to)
            return high;
    }
    const auto product = (static_cast<std::int64_t>(value) - from) * (high - low);
    if (!fits(product))
        return {};
    const auto result = low + product / (to - from);
    return fits(result) ? std::optional<int>{static_cast<int>(result)} : std::nullopt;
}
TaskCommandHuman *human(State &s, int id) {
    const auto found = std::find_if(s.humans.begin(), s.humans.end(),
                                    [id](const auto &h) { return h.identity == id; });
    return found == s.humans.end() ? nullptr : &*found;
}
const TaskCommandHuman *human(const State &s, int id) {
    const auto found = std::find_if(s.humans.begin(), s.humans.end(),
                                    [id](const auto &h) { return h.identity == id; });
    return found == s.humans.end() ? nullptr : &*found;
}
bool valid(const State &s) {
    if (s.humans.size() > 1000000 || s.page_counter < 0 ||
        s.page_counter == std::numeric_limits<int>::max() || s.secondary_counter < 0 ||
        s.secondary_counter == std::numeric_limits<int>::max() || s.clock_parameter <= 0 ||
        s.active_task_updates < 0 || s.task_subperiods < 0)
        return false;
    std::set<int> identities;
    for (const auto &h : s.humans) {
        if (h.identity < 0 || !identities.insert(h.identity).second || h.status < 0 ||
            h.legacy_u < 0 || h.event_participations < 0 || h.extra_fee < 0 || h.absence < 0 ||
            h.profession_levels.empty() || h.profession_levels.size() > 1000 ||
            !s.finish.human_definition_flags.count(h.identity) ||
            !s.finish.dungeon.world.ai.battle.humans.count(h.identity))
            return false;
        for (const int level : h.profession_levels)
            if (level < 1 || level > 10)
                return false;
    }
    for (const auto id : s.finish.participants)
        if (!human(s, id))
            return false;
    return true;
}
const DungeonFinishTask *task(const State &s, std::uint64_t id) {
    const auto found = s.finish.tasks.find(id);
    if (!id || found == s.finish.tasks.end() || found->second.identity != id ||
        std::find(s.finish.task_order.begin(), s.finish.task_order.end(), id) ==
            s.finish.task_order.end())
        return nullptr;
    return &found->second;
}
const TaskCommandDefinition *definition(const State &s, std::uint64_t id) {
    const auto *t = task(s, id);
    if (!t)
        return nullptr;
    const auto found = s.definitions.find(t->definition);
    const auto progress = s.finish.task_progress.definitions.find(t->definition);
    if (found == s.definitions.end() || found->second.identity != t->definition ||
        found->second.recruitment_fee < 0 || found->second.strength < 0 ||
        progress == s.finish.task_progress.definitions.end())
        return nullptr;
    return &found->second;
}
bool emit(Candidate &c, TaskCommandEffect effect, const TaskCommandConsumer &consumer,
          CommandError &error) {
    if (!consumer) {
        error = CommandError::missing_consumer;
        return false;
    }
    std::optional<State> consumed;
    try {
        consumed = consumer(c.state, effect);
    } catch (...) {
        error = CommandError::consumer_failed;
        return false;
    }
    if (!consumed || !valid(*consumed)) {
        error = CommandError::consumer_failed;
        return false;
    }
    c.state = std::move(*consumed);
    c.effects.push_back(std::move(effect));
    return true;
}
bool request(Candidate &c, Effect kind, int first, const TaskCommandConsumer &consumer,
             CommandError &error, std::string text = {}, std::optional<int> id = {}) {
    return emit(c, {kind, first, 0, std::move(text), c.state.selected_task, id}, consumer, error);
}
std::optional<int> draw(Candidate &c, int bound) {
    c.random_bounds.push_back(bound);
    const auto result = c.state.random.draw(bound);
    return result.error == WorldRandomError::none ? std::optional<int>{result.ticket}
                                                  : std::nullopt;
}
bool expense(Candidate &c, int amount) {
    auto &ai = c.state.finish.dungeon.world.ai;
    if (!ai.next_cash_id || ai.next_cash_id == std::numeric_limits<std::uint64_t>::max())
        return false;
    if (ai.accounting.post_cash({ai.next_cash_id, c.state.cash_period, CashCategory::other,
                                 CashDirection::expense, amount}) != AccountingError::none)
        return false;
    ++ai.next_cash_id;
    return true;
}
bool reset_human(State &s, int id) {
    const auto found = s.finish.dungeon.world.ai.battle.humans.find(id);
    if (!human(s, id) || found == s.finish.dungeon.world.ai.battle.humans.end())
        return false;
    found->second.task_kills = found->second.participant_downs = 0; // e.o：仅H/I，不清J/K。
    return true;
}
bool initialize_recruitment(Candidate &c) {
    std::vector<int> remaining;
    for (const auto &h : c.state.humans)
        if (h.status != 0)
            remaining.push_back(h.identity);
    // 逐原序与全列表抽号交换，不是Fisher-Yates；单人仍实际消费draw1。
    for (std::size_t n = 0; n < remaining.size(); ++n) {
        const auto ticket = draw(c, static_cast<int>(remaining.size()));
        if (!ticket)
            return false;
        std::swap(remaining[n], remaining[static_cast<std::size_t>(*ticket)]);
    }
    std::vector<int> selected;
    while (!remaining.empty() && selected.size() < 3) {
        selected.push_back(remaining.back());
        remaining.pop_back();
    }
    for (std::size_t n = remaining.size(); n > 0; --n) {
        const auto id = remaining[n - 1];
        const auto &h = *human(c.state, id);
        const int threshold = *scale(static_cast<int>(selected.size()), 3, 8, 0, -50) +
                              *scale(h.legacy_u, 0, 100, 15, 40) +
                              *scale(h.event_participations, 0, 3, 0, 20);
        const auto ticket = draw(c, 100);
        if (!ticket)
            return false;
        if (*ticket < threshold) {
            selected.push_back(id);
            if (selected.size() >= 9)
                break;
        }
    }
    auto &animation = c.state.recruitment;
    animation = {};
    animation.arrival_extent = *scale(static_cast<int>(selected.size()), 0, 20, 100, 200);
    animation.completion_tick = animation.arrival_extent + 130;
    const int interval =
        std::max(animation.arrival_extent / (static_cast<int>(selected.size()) + 1), 3);
    const int jitter = std::max(interval - 3, 1);
    int previous_phrase = -1;
    for (std::size_t n = 0; n < selected.size(); ++n) {
        const auto offset = draw(c, jitter);
        if (!offset)
            return false;
        const int delay = (static_cast<int>(n) + 1) * interval + *offset - jitter / 2;
        if (previous_phrase == -1) {
            const auto probability = draw(c, 100);
            if (!probability)
                return false;
            if (*probability < 35) {
                const auto phrase = draw(c, 6);
                if (!phrase)
                    return false;
                previous_phrase = *phrase;
            }
        } else
            previous_phrase = -1; // 两项不能连续有发言，第二项也不抽概率号。
        animation.entries.push_back({selected[n], -delay, previous_phrase});
    }
    c.state.page_counter = c.state.secondary_counter = 0;
    c.state.phase = TaskCommandPhase::recruiting;
    return true;
}
bool increment(int &counter) {
    if (counter == std::numeric_limits<int>::max())
        return false;
    ++counter;
    return true;
}
} // namespace

TaskCommandResult prepare_world_task_offer(const State &s, std::uint64_t id, TaskCommandInput input,
                                           const TaskCommandConsumer &consumer) {
    if (!valid(s) || s.phase != TaskCommandPhase::offer || (input.confirm && input.cancel))
        return fail(CommandError::invalid_input);
    const auto *d = definition(s, id);
    if (!d)
        return fail(CommandError::stale_task);
    Candidate c;
    c.state = s;
    c.state.selected_task = id;
    CommandError error{};
    if (input.cancel) {
        c.state.phase = TaskCommandPhase::closed;
        if (!request(c, Effect::close_page, 23, consumer, error))
            return fail(error);
    } else if (input.confirm) {
        if (s.finish.dungeon.world.ai.accounting.funds() < d->recruitment_fee) {
            c.denial = TaskCommandDenial::insufficient_funds;
            if (!request(c, Effect::event, 11, consumer, error))
                return fail(error);
            return {CommandError::none, c};
        }
        if (!expense(c, d->recruitment_fee))
            return fail(CommandError::accounting_failed);
        if (!request(c, Effect::reset_main_scene, 0, consumer, error))
            return fail(error);
        if (!initialize_recruitment(c))
            return fail(CommandError::random_failed);
        if (!request(c, Effect::open_page, 24, consumer, error) ||
            !request(c, Effect::close_page, 23, consumer, error))
            return fail(error);
        c.accepted = true;
    }
    return {CommandError::none, c};
}

TaskCommandResult prepare_world_task_recruitment(const State &s, bool held,
                                                 const TaskCommandConsumer &consumer) {
    if (!valid(s) || s.phase != TaskCommandPhase::recruiting || !s.selected_task ||
        !definition(s, *s.selected_task) || s.recruitment.completion_tick < 0 ||
        s.recruitment.portrait_timer < -1 || s.recruitment.displayed_count < 0)
        return fail(CommandError::invalid_input);
    Candidate c;
    c.state = s;
    auto &next = c.state;
    if (!increment(next.page_counter) || !increment(next.secondary_counter) ||
        (held && (!increment(next.page_counter) || !increment(next.secondary_counter))))
        return fail(CommandError::numeric_overflow);
    auto &animation = next.recruitment;
    for (auto &entry : animation.entries) {
        if (!human(next, entry.human) || entry.phrase < -1 || entry.phrase >= 6)
            return fail(CommandError::stale_human);
        const auto previous = entry.counter;
        if (!increment(entry.counter) || (held && !increment(entry.counter)))
            return fail(CommandError::numeric_overflow);
        if (previous < 0 && entry.counter >= 0) {
            animation.portraits.push_back(entry.human);
            if (animation.displayed_count == 0) {
                ++animation.displayed_count;
                animation.portrait_timer = 10;
            }
        }
    }
    if (animation.portrait_timer > 0) {
        animation.portrait_timer -= held ? 2 : 1;
        if (animation.portrait_timer <= 0) {
            if (animation.portraits.size() > 1) {
                animation.portraits.erase(animation.portraits.begin());
                if (!increment(animation.displayed_count))
                    return fail(CommandError::numeric_overflow);
            }
            if (animation.portraits.size() > 1)
                animation.portrait_timer = 10;
        }
    } else if (animation.portraits.size() > 1)
        animation.portrait_timer = 10;
    if (next.page_counter < animation.completion_tick)
        return {CommandError::none, c};
    CommandError error{};
    if (!request(c, Effect::event, next.event9_seen ? 8 : 9, consumer, error,
                 std::to_string(animation.displayed_count)))
        return fail(error);
    // 真实消息先于m/H/I写入；回调后读候选，不能恢复事件之前的旧世界。
    next.finish.participants.clear();
    for (const auto &entry : next.recruitment.entries) {
        if (!reset_human(next, entry.human))
            return fail(CommandError::stale_human);
        next.finish.participants.push_back(entry.human);
    }
    next.finish.dungeon.world.ai.battle.participants = next.finish.participants;
    next.phase = TaskCommandPhase::team;
    next.page_counter = next.secondary_counter = 0;
    if (!request(c, Effect::open_page, 25, consumer, error) ||
        !request(c, Effect::close_page, 24, consumer, error))
        return fail(error);
    return {CommandError::none, c};
}

TaskCommandResult prepare_world_task_extra_candidates(const State &s,
                                                      const TaskCommandConsumer &consumer) {
    if (!valid(s) ||
        (s.phase != TaskCommandPhase::team && s.phase != TaskCommandPhase::active_team))
        return fail(CommandError::invalid_input);
    Candidate c;
    c.state = s;
    CommandError error{};
    if (s.finish.participants.size() >= 9) {
        c.denial = TaskCommandDenial::team_full;
        if (!request(c, Effect::event, 63, consumer, error))
            return fail(error);
        return {CommandError::none, c};
    }
    c.state.extra_candidates.clear();
    for (auto &h : c.state.humans) {
        if (h.status == 0 || std::find(s.finish.participants.begin(), s.finish.participants.end(),
                                       h.identity) != s.finish.participants.end())
            continue;
        std::int64_t fee = 200;
        for (const int level : h.profession_levels)
            fee += (level - 1) * 50;
        if (!fits(fee))
            return fail(CommandError::numeric_overflow);
        h.extra_fee = std::min(static_cast<int>(fee), 3000);
        c.state.extra_candidates.push_back(h.identity);
    }
    if (!request(c, Effect::open_page, 27, consumer, error))
        return fail(error);
    if (c.state.extra_candidates.empty()) {
        c.denial = TaskCommandDenial::no_extra_candidates;
        if (!request(c, Effect::event, 63, consumer, error) ||
            !request(c, Effect::close_page, 27, consumer, error))
            return fail(error);
    } else
        c.state.phase = TaskCommandPhase::extra;
    return {CommandError::none, c};
}

TaskCommandResult prepare_world_task_hire(const State &s, int id, TaskCommandInput input,
                                          const TaskCommandConsumer &consumer) {
    if (!valid(s) || s.phase != TaskCommandPhase::extra || (input.confirm && input.cancel))
        return fail(CommandError::invalid_input);
    Candidate c;
    c.state = s;
    CommandError error{};
    if (input.cancel) {
        c.state.phase =
            s.finish.active_task ? TaskCommandPhase::active_team : TaskCommandPhase::team;
        if (!request(c, Effect::close_page, 27, consumer, error))
            return fail(error);
        return {CommandError::none, c};
    }
    const auto *h = human(s, id);
    if (!h || h->status == 0 ||
        std::find(s.extra_candidates.begin(), s.extra_candidates.end(), id) ==
            s.extra_candidates.end() ||
        std::find(s.finish.participants.begin(), s.finish.participants.end(), id) !=
            s.finish.participants.end())
        return fail(CommandError::stale_human);
    if (!input.confirm)
        return {CommandError::none, c};
    if (s.finish.dungeon.world.ai.accounting.funds() < h->extra_fee) {
        c.denial = TaskCommandDenial::insufficient_funds;
        if (!request(c, Effect::event, 11, consumer, error))
            return fail(error);
        return {CommandError::none, c};
    }
    if (!expense(c, h->extra_fee))
        return fail(CommandError::accounting_failed);
    if (!reset_human(c.state, id))
        return fail(CommandError::stale_human);
    c.state.finish.participants.push_back(id);
    c.state.finish.dungeon.world.ai.battle.participants = c.state.finish.participants;
    c.state.finish.human_definition_flags.at(id) |= 2U;
    if (!emit(c, {Effect::event, 64, 1, {}, c.state.selected_task, id}, consumer, error))
        return fail(error);
    c.state.phase = s.finish.active_task ? TaskCommandPhase::active_team : TaskCommandPhase::team;
    if (!request(c, Effect::close_page, 27, consumer, error))
        return fail(error);
    c.accepted = true;
    return {CommandError::none, c};
}

TaskCommandResult prepare_world_task_departure_page(const State &s,
                                                    const TaskCommandConsumer &consumer) {
    if (!valid(s) || s.phase != TaskCommandPhase::team || !s.selected_task)
        return fail(CommandError::invalid_input);
    const auto *d = definition(s, *s.selected_task);
    if (!d)
        return fail(CommandError::stale_task);
    std::int64_t sum{};
    for (const auto id : s.finish.participants)
        for (const int level : human(s, id)->profession_levels)
            sum += level;
    if (!fits(sum) ||
        s.finish.participants.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return fail(CommandError::numeric_overflow);
    const auto people = scale(static_cast<int>(s.finish.participants.size()), 3, 9, 0, 100);
    const auto levels = scale(static_cast<int>(sum), 10, 250, 0, 100, false);
    if (!people || !levels)
        return fail(CommandError::numeric_overflow);
    const int score =
        (std::clamp(*levels, 0, 200) * 70 + *people * 30) / 100 - (d->strength / 100 + 5);
    int prediction = score < -10  ? 0
                     : score < -3 ? 1
                     : score < 3  ? 2
                     : score < 10 ? 3
                     : score < 17 ? 4
                                  : 5;
    const int kind = s.finish.task_progress.definitions.at(d->identity).kind;
    if (prediction <= 1 && kind == 0)
        prediction = 6;
    Candidate c;
    c.state = s;
    c.state.predicted_result = prediction;
    c.state.phase = TaskCommandPhase::departure_prompt;
    c.state.page_counter = c.state.secondary_counter = 0;
    c.state.accelerate_departure = false;
    CommandError error{};
    if (!request(c, Effect::open_page, 28, consumer, error))
        return fail(error);
    return {CommandError::none, c};
}

TaskCommandResult prepare_world_task_departure(const State &s, TaskCommandInput input,
                                               const TaskCommandConsumer &consumer) {
    if (!valid(s) ||
        (s.phase != TaskCommandPhase::departure_prompt && s.phase != TaskCommandPhase::departing) ||
        !s.selected_task || (input.confirm && input.cancel))
        return fail(CommandError::invalid_input);
    const auto *d = definition(s, *s.selected_task);
    if (!d)
        return fail(CommandError::stale_task);
    Candidate c;
    c.state = s;
    auto &next = c.state;
    if (!increment(next.page_counter) || !increment(next.secondary_counter))
        return fail(CommandError::numeric_overflow);
    if (next.accelerate_departure && next.page_counter < 90) {
        if (next.page_counter > std::numeric_limits<int>::max() - 5)
            return fail(CommandError::numeric_overflow);
        next.page_counter += 5;
    }
    CommandError error{};
    if (next.phase == TaskCommandPhase::departing && next.page_counter >= 96) {
        if (!request(c, Effect::activate_main_scene, 0, consumer, error))
            return fail(error);
        const auto *current_task = task(next, *next.selected_task);
        if (!current_task)
            return fail(CommandError::stale_task);
        next.finish.active_task = *next.selected_task;   // n.a(k)先h，再p0；不触发AI或任务F。
        next.finish.dungeon.world.ai.task_active = true; // 从实际h派生，不另建任务归属。
        next.active_task_updates = 0;
        if (current_task->facility) {
            const auto id = *current_task->facility;
            const auto difficulty = next.facility_difficulties.find(id);
            auto facility = next.finish.dungeon.world.facilities.find(id);
            auto progress = next.finish.dungeon.facilities.find(id);
            if (difficulty == next.facility_difficulties.end() || difficulty->second < 0 ||
                facility == next.finish.dungeon.world.facilities.end() ||
                progress == next.finish.dungeon.facilities.end())
                return fail(CommandError::stale_task);
            const auto low = static_cast<std::int64_t>(next.clock_parameter) * 12 * 10;
            const auto high = static_cast<std::int64_t>(next.clock_parameter) * 12 * 2 * 10;
            if (!fits(low) || !fits(high))
                return fail(CommandError::numeric_overflow);
            const auto extent =
                scale(difficulty->second, 1, 9, static_cast<int>(low), static_cast<int>(high));
            if (!extent)
                return fail(CommandError::numeric_overflow);
            facility->second.status = 1;
            progress->second.updates = 0;
            progress->second.progress = 0;
            progress->second.extent = *extent;
        }
        for (auto &flags : next.finish.human_definition_flags)
            flags.second &= ~2U;
        for (const auto id : next.finish.participants) {
            human(next, id)->absence = 0;
            next.finish.human_definition_flags.at(id) |= 2U;
        }
        if (!request(c, Effect::notice15, 15, consumer, error, d->notice_title))
            return fail(error);
        next.task_subperiods = 0;
        next.phase = TaskCommandPhase::closed;
        if (!request(c, Effect::close_page, 28, consumer, error))
            return fail(error);
        c.departed = true;
        return {CommandError::none, c};
    }
    if (input.confirm) {
        if (next.phase == TaskCommandPhase::departure_prompt) {
            if (input.selection != 0) {
                next.phase = TaskCommandPhase::team;
                if (!request(c, Effect::close_page, 28, consumer, error))
                    return fail(error);
            } else {
                next.phase = TaskCommandPhase::departing;
                next.page_counter = 0;
            }
        } else if (next.page_counter >= 40)
            next.accelerate_departure = true;
    } else if (input.cancel && next.phase == TaskCommandPhase::departure_prompt) {
        next.phase = TaskCommandPhase::team;
        if (!request(c, Effect::close_page, 28, consumer, error))
            return fail(error);
    }
    return {CommandError::none, c};
}
} // namespace ark::simulation::rules
