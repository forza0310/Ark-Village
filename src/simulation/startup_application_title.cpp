#include "ark/simulation/startup_application.hpp"
#include <type_traits>

namespace ark::simulation {
StartupTitleApplyResult StartupApplication::advance_title_background(StartupTitleUpdateRequest request) {
    if (!error_.empty()) return {error_, false, false, 0};
    if (mode_ != StartupApplicationMode::title_presentation || page_ != StartupApplicationPage::title)
        return {"只有具名标题表现模式的活动标题接受背景更新", false, false, 0};
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
} // namespace ark::simulation
