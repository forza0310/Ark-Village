#include "ark/simulation/application/startup_application.hpp"

namespace ark::simulation {
namespace {
// 只检查最后一个未退休页，不把历史页或同号父页当作当前输入目标。
bool active_raw_page(const StartupWorldRuntimeSession &world, std::uint64_t id, int raw) {
    const auto &pages = world.state().scripts.pages;
    for (auto page = pages.rbegin(); page != pages.rend(); ++page)
        if (page->lifecycle != 4)
            return page->id == id && page->kind == ref::WorldScriptPageKind::raw_page &&
                   page->legacy_page == raw;
    return false;
}
std::string command_error(StartupWorldRuntimeError error) {
    return error == StartupWorldRuntimeError::none ? std::string{} :
        "世界命令拒绝：" + std::to_string(static_cast<int>(error));
}
} // namespace

StartupApplicationPresentationResult StartupApplication::present_world(
    const StartupPresentationRequest &request) {
    if (!error_.empty()) return {error_, {}};
    if (!world_ || page_ != StartupApplicationPage::world)
        return {"当前没有可表现的世界页面", {}};
    // 表现允许世界暂停及raw17；普通动作的资格函数会拒绝计分页，不能复用。
    const bool scoring = clear_.has_value();
    if (clear_rows_.has_value() != scoring || clear_id_.has_value() != scoring)
        return {"计分三元组不完整", {}};
    const auto &state = world_->state();
    const ref::WorldScriptPage *top{};
    for (auto p = state.scripts.pages.rbegin(); p != state.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4) { top = &*p; break; }
    const auto is_clear = [](const ref::WorldScriptPage &p) {
        return p.kind == ref::WorldScriptPageKind::raw_page && p.legacy_page == 17;
    };
    if (scoring) {
        if (!top || !is_clear(*top) || top->id != *clear_id_ || top->lifecycle != 2 ||
            !valid_startup_world_clear_page(state, *top))
            return {"计分控制器页面引用失效", {}};
        const auto counter = state.page_counters.find(top->id);
        const auto phase = state.page_phases.find(top->id);
        if (counter == state.page_counters.end() || phase == state.page_phases.end() ||
            counter->second != clear_->counter || phase->second != clear_->stage ||
            clear_->captured_high_score != storage_.records.high_score ||
            validate_startup_clear_score_page(*clear_rows_, *clear_) != StartupClearScoreError::none)
            return {"世界计分页与应用计分状态不同", {}};
    } else if (top && is_clear(*top) &&
               (top->lifecycle != 0 || !valid_startup_world_clear_page(state, *top))) {
        return {"已初始化计分页缺应用控制器", {}};
    }
    for (const auto &page : state.scripts.pages)
        if (is_clear(page) && page.lifecycle != 0 && page.lifecycle != 4 &&
            (!scoring || !top || top->id != page.id))
            return {"计分页存在孤立应用引用", {}};
    auto candidate = *world_;
    auto result = candidate.present(request);
    const auto error = command_error(result.error);
    if (!error.empty()) return {error, {}};
    if (!result.candidate || !result.plan) return {"世界表现缺完整候选或计划", {}};
    const auto committed = commit_world(std::move(candidate));
    if (!committed.empty()) return {committed, {}};
    return {{}, std::move(result.plan)};
}

// 每项玩家动作独立提交；年度终止请求与确认不可被合并为一次隐式确认。
std::string StartupApplication::act_award_page(std::uint64_t id, ref::WorldAwardAction action,
                                               int selection) {
    if (!error_.empty()) return error_;
    if (!world_ || page_ != StartupApplicationPage::world || clear_ || clear_rows_ || clear_id_)
        return "当前没有可操作的普通世界页面";
    if (!active_raw_page(*world_, id, 87)) return "授勋操作目标不是当前87页";
    auto candidate = *world_;
    const auto error = command_error(candidate.act_award_page(id, action, selection));
    return error.empty() ? commit_world(std::move(candidate)) : error;
}

std::string StartupApplication::return_rank_page(std::uint64_t id) {
    if (!error_.empty()) return error_;
    if (!world_ || page_ != StartupApplicationPage::world || clear_ || clear_rows_ || clear_id_)
        return "当前没有可操作的普通世界页面";
    if (!active_raw_page(*world_, id, 48)) return "晋级返回目标不是当前48页";
    auto candidate = *world_;
    const auto error = command_error(candidate.act_rank_page(id, 0, true));
    return error.empty() ? commit_world(std::move(candidate)) : error;
}

std::string StartupApplication::leave_commerce_page(std::uint64_t id) {
    if (!error_.empty()) return error_;
    if (!world_ || page_ != StartupApplicationPage::world || clear_ || clear_rows_ || clear_id_)
        return "当前没有可操作的普通世界页面";
    if (!active_raw_page(*world_, id, 83)) return "商会离开目标不是当前83页";
    auto candidate = *world_;
    // 限定83后复用原取消消费者；不会确认购买，也不会进入84/85商品页。
    const auto error = command_error(candidate.cancel_page(id));
    return error.empty() ? commit_world(std::move(candidate)) : error;
}
} // namespace ark::simulation
