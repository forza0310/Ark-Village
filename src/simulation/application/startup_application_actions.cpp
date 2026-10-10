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
