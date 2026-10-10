// 原raw14的文件请求由应用协调；原页时点见rules/PERSISTENCE.md，失败策略采用维护事务。
#include "dungeon_village_prototype/startup_application.hpp"
#include "dungeon_village_prototype/startup_world_save.hpp"
#include <exception>
#include <type_traits>

namespace dungeon_village_prototype {
std::string StartupApplication::update_save_page(std::uint64_t page) {
    if (!world_ || page_ != StartupApplicationPage::world || clear_)
        return "当前不能处理保存页";
    const auto view = inspect_startup_world_save_page(world_->state(), page);
    if (!view || view->stage != 1) return "保存请求状态非法";
    if (world_->state().scene.framework_paused) return {}; // 暂停不消费请求或推进页面。
    if (!audio_requests_.empty()) return "应用声音输出尚未消费，不能处理保存请求";
    try {
        auto exported = prepare_startup_world_save_export(world_->state(), page);
        if (!exported) return "保存页不满足稳定导出资格";
        auto success = complete_startup_world_save_page(world_->state(), page, true, {});
        if (!success || !update_startup_world_render_cache(*success) ||
            !success->sound_requests.empty()) return "保存完成结果候选拒绝";

        // 发布后只有无抛出的移动安装；提前准备完整Session和唯一稳定导出，
        // 不借普通档接口删除任意模态页，也不在落盘后再初始化结果页。
        auto saved_session = *world_;
        saved_session.state_ = std::move(*exported);
        auto success_session = *world_;
        success_session.state_ = std::move(*success);
        static_assert(std::is_nothrow_move_assignable_v<StartupApplicationStorageSnapshot>);
        static_assert(std::is_nothrow_move_assignable_v<std::optional<StartupWorldRuntimeSession>>);
        auto committed = save_startup_application_slot(paths_.root, storage_, storage_.records,
            draft_.slot, StartupSaveKind::manual, saved_session);
        if (!committed.snapshot) {
            // 文件尚未发布才生成失败文本。原有效目录、业务及随机保留；
            // 合法失败结果是一次完整页面提交，不能被调用方当作付款重试。
            auto failure = complete_startup_world_save_page(world_->state(), page, false,
                committed.error.empty() ? "保存存储拒绝" : committed.error);
            if (!failure || !update_startup_world_render_cache(*failure) ||
                !failure->sound_requests.empty()) return "保存失败结果候选拒绝";
            auto failure_session = *world_;
            failure_session.state_ = std::move(*failure);
            world_ = std::move(failure_session);
            return {};
        }
        storage_ = std::move(*committed.snapshot);
        cleanup_pending_ = committed.cleanup_pending;
        world_ = std::move(success_session);
        return {};
    } catch (const std::exception &error) {
        return error.what();
    }
}
} // namespace dungeon_village_prototype
