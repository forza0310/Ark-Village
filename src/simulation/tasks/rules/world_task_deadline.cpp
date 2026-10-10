#include "ark/simulation/tasks/rules/world_task_deadline.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation::rules {
WorldTaskDeadlinePageResult prepare_world_task_deadline_page(const WorldTaskDeadlinePageState &s,
                                                             const WorldTaskDeadlinePageInput &i) {
    if (s.phase < 0 || s.phase > 1 || s.counter < 0 || s.cursor < 0 || s.cursor > 1 ||
        s.returned < -1 || s.returned > 1 || s.grade < 0 || s.grade > 4 || i.initial_grade < 0 ||
        i.initial_grade > 4 || i.selection < 0 || i.selection > 1 ||
        (i.action != WorldTaskDeadlinePageAction::initialize &&
         i.action != WorldTaskDeadlinePageAction::update &&
         i.action != WorldTaskDeadlinePageAction::confirm))
        return {WorldTaskDeadlineError::invalid_input, {}};
    WorldTaskDeadlinePageCandidate c{s, false, false};
    if (!c.state.initialized) {
        c.state.grade = i.initial_grade;
        c.state.initialized = true;
    }
    if (i.action == WorldTaskDeadlinePageAction::update) {
        // 页面通用b()用模INTMAX递增，动画本身并无50以外的计数限幅。
        c.state.counter = static_cast<int>((static_cast<std::int64_t>(c.state.counter) + 1) %
                                           std::numeric_limits<int>::max());
        c.closed = c.state.phase == 1 && c.state.counter >= 50;
    } else if (i.action == WorldTaskDeadlinePageAction::confirm) {
        if (c.state.phase == 1)
            c.state.counter = 50;
        else {
            c.state.cursor = i.selection;
            if (i.selection == 0 && i.funds < i.displayed_fee)
                c.insufficient_funds = true;
            else {
                c.state.returned = i.selection;
                c.state.phase = 1;
                c.state.counter = 0;
            }
        }
    }
    return {WorldTaskDeadlineError::none, c};
}
WorldTaskDeadlineResult
prepare_world_task_deadline_result(const WorldCalendarTasksState &s, int returned,
                                   std::uint64_t period, const WorldTaskDeadlineConsumer &consumer,
                                   const std::optional<WorldScriptPage> &closed_page) {
    const auto fail = [](WorldTaskDeadlineError e) { return WorldTaskDeadlineResult{e, {}}; };
    if (returned < 0 || returned > 1 || !period || !s.deadline_page)
        return fail(WorldTaskDeadlineError::invalid_input);
    const auto page = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                   [&](const auto &p) { return p.id == *s.deadline_page; });
    // aI仍持有已出栈的真实页面；它不是需要重新插入框架栈的假页面。
    const auto *original =
        page == s.scripts.pages.end() ? (closed_page ? &*closed_page : nullptr) : &*page;
    if (!original || original->id != *s.deadline_page || original->legacy_page != 33 ||
        original->kind != WorldScriptPageKind::raw_page || original->lifecycle != 4)
        return fail(WorldTaskDeadlineError::stale_page);
    WorldTaskDeadlineCandidate c{s, {}, 0, false, false};
    c.state.deadline_page.reset(); // 原aI先清，不在最后成功通知时才清。
    std::int64_t total{};
    for (int id : c.state.finish.participants) {
        const auto h = c.state.human_details.find(id);
        if (h == c.state.human_details.end())
            return fail(WorldTaskDeadlineError::missing_human);
        total += h->second.continuation_cost;
        if (total < std::numeric_limits<int>::min() || total > std::numeric_limits<int>::max())
            return fail(WorldTaskDeadlineError::overflow);
    }
    c.current_fee = static_cast<int>(total);
    const auto effect = [&](WorldTaskDeadlineEffect e) {
        if (!consumer)
            return WorldTaskDeadlineError::missing_consumer;
        try {
            const auto next = consumer(c.state, e);
            if (!next || next->random.draws() < c.state.random.draws())
                return WorldTaskDeadlineError::consumer_failed;
            c.state = *next;
            c.effects.push_back(e);
            return WorldTaskDeadlineError::none;
        } catch (...) {
            return WorldTaskDeadlineError::consumer_failed;
        }
    };
    const auto event = [&](int id) {
        const auto before = c.state.scripts.event_calls.find(id);
        const int count = before == c.state.scripts.event_calls.end() ? 0 : before->second;
        const auto error = effect({WorldTaskDeadlineEffectKind::event, id});
        if (error != WorldTaskDeadlineError::none)
            return error;
        const auto after = c.state.scripts.event_calls.find(id);
        return after != c.state.scripts.event_calls.end() && after->second > count
                   ? WorldTaskDeadlineError::none
                   : WorldTaskDeadlineError::consumer_failed;
    };
    if (returned == 0 && c.state.finish.dungeon.world.ai.accounting.funds() >= total) {
        c.state.task_subperiods = 0;
        auto &ai = c.state.finish.dungeon.world.ai;
        if (total != 0) {
            if (!ai.next_cash_id || ai.next_cash_id == std::numeric_limits<std::uint64_t>::max())
                return fail(WorldTaskDeadlineError::accounting_failed);
            const auto direction = total > 0 ? CashDirection::expense : CashDirection::income;
            // 原an仅上限3000；负费用也不补零下限，g(负数,4)实际增加现金。
            if (ai.accounting.post_cash({ai.next_cash_id, period, CashCategory::other, direction,
                                         total > 0 ? total : -total}) != AccountingError::none)
                return fail(WorldTaskDeadlineError::accounting_failed);
            ++ai.next_cash_id;
            // 同步脚本投影，避免真实外层回写时用扣费前finance重新退款或覆盖月分类统计。
            if (c.state.scripts.finance) {
                auto &finance = *c.state.scripts.finance;
                if (finance.month < 0 || finance.month >= 12)
                    return fail(WorldTaskDeadlineError::invalid_input);
                finance.cash = ai.accounting.funds();
                auto &slot = finance.monthly_totals[finance.month][4][total > 0 ? 1 : 0];
                const auto sum = static_cast<std::int64_t>(slot) + (total > 0 ? total : -total);
                if (sum < std::numeric_limits<int>::min() || sum > std::numeric_limits<int>::max())
                    return fail(WorldTaskDeadlineError::overflow);
                slot = static_cast<int>(sum);
                if (total < 0 && finance.legacy_flags14 == 0 && finance.cash > finance.cash_peak) {
                    finance.cash_peak = finance.cash;
                    finance.cash_peak_village = c.state.scripts.village_name;
                }
            }
        }
        c.renewed = true;
        const auto error = effect({WorldTaskDeadlineEffectKind::notice, 25});
        if (error != WorldTaskDeadlineError::none)
            return fail(error);
    } else {
        if (returned == 0) {
            const auto error = event(11);
            if (error != WorldTaskDeadlineError::none)
                return fail(error);
        }
        const auto abort = effect({WorldTaskDeadlineEffectKind::abort_task, 0});
        if (abort != WorldTaskDeadlineError::none)
            return fail(abort);
        c.aborted = true;
        const auto ended = event(80);
        if (ended != WorldTaskDeadlineError::none)
            return fail(ended);
        auto &pending = c.state.finish.dungeon.world.ai.pending_completion;
        if (pending < std::numeric_limits<int>::min() + 10)
            return fail(WorldTaskDeadlineError::overflow);
        pending -= 10; // c/n.e(-10)只累加f215e，不即时写人气或I。
        c.state.scripts.pending_completion = pending;
        if (!world_script_seen(c.state.scripts, 162)) {
            const auto first = event(162);
            if (first != WorldTaskDeadlineError::none)
                return fail(first);
        }
        const auto notice = effect({WorldTaskDeadlineEffectKind::notice, 26});
        if (notice != WorldTaskDeadlineError::none)
            return fail(notice);
    }
    return {WorldTaskDeadlineError::none, std::move(c)};
}
} // namespace ark::simulation::rules
