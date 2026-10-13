#include "dungeon_village_prototype/startup_application.hpp"
#include <exception>
#include <type_traits>

namespace dungeon_village_prototype {
StartupTitleApplyResult StartupApplication::advance_title_background(StartupTitleUpdateRequest request) {
    if (!error_.empty()) return {error_, false, false, 0};
    if (mode_ != StartupApplicationMode::title_presentation || page_ != StartupApplicationPage::title)
        return {"只有具名标题表现模式的活动标题接受背景更新", false, false, 0};
    if (title_menu_.save_menu || title_menu_.confirmation || title_menu_.external)
        return {"标题子页尚未退休，父标题背景冻结", false, false, 0};
    auto prepared = prepare_startup_title_update(title_, random_, request);
    if (!prepared.candidate) return {prepared.error, false, false, 0};
    auto &candidate = *prepared.candidate;
    StartupTitleApplyResult result{{}, candidate.confirm_consumed, candidate.menu_confirm_ready,
                                   candidate.random_draws};
    static_assert(std::is_nothrow_move_assignable_v<ref::WorldRandomStream>);
    static_assert(std::is_nothrow_copy_assignable_v<StartupTitlePresentation>);
    title_ = candidate.state;
    random_ = std::move(candidate.random);
    return result;
}
StartupTitleApplyResult StartupApplication::input_title_frame(
    std::uint64_t id, const StartupTitleFrameInput &input) {
    if (!error_.empty()) return {error_, false, false, 0};
    if (mode_ != StartupApplicationMode::title_presentation || page_ != StartupApplicationPage::title || clear_)
        return {"只有具名标题表现模式的活动根标题接受同轮输入", false, false, 0};
    if (title_menu_.save_menu || title_menu_.confirmation || title_menu_.external)
        return {"标题子页尚未退休，不能消费根标题同轮输入", false, false, 0};
    if (id == 0 || id != startup_title_menu_top_id(title_menu_) || draft_.slot < 0 || draft_.slot > 1)
        return {"根标题身份或唯一档位非法", false, false, 0};
    try {
        auto prepared = prepare_startup_title_update(title_, random_, {input.admission, input.keys.confirm});
        if (!prepared.candidate) return {prepared.error, false, false, 0};
        auto candidate = *this;
        const auto &background = *prepared.candidate;
        candidate.title_ = background.state;
        candidate.random_ = std::move(prepared.candidate->random);
        if (candidate.title_.l >= 100) {
            StartupTitleMenuRequest request;
            request.keys = input.keys;
            request.keys.confirm = background.menu_confirm_ready;
            const auto error = candidate.apply_title_request_candidate(id, request);
            if (!error.empty()) return {error, false, false, 0};
        }
        // 统计本命令的标题流；读档沿既有文件消费者，不把世界载荷随机误算成标题抽数。
        StartupTitleApplyResult result{{}, background.confirm_consumed, background.menu_confirm_ready,
                                      candidate.random_.draws() - random_.draws()};
        static_assert(std::is_nothrow_move_assignable_v<StartupApplication>);
        *this = std::move(candidate);
        return result;
    } catch (const std::exception &e) { return {e.what(), false, false, 0}; }
}
} // namespace dungeon_village_prototype
