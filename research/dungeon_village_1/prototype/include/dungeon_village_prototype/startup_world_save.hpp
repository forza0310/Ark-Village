#pragma once

#include "dungeon_village_prototype/startup_world_runtime.hpp"

namespace dungeon_village_prototype {
// raw14保留在唯一Owner中；文件发布由应用完成，不在世界规则回调访问磁盘。
struct StartupWorldSavePageView {
    std::uint64_t page{};
    int stage{}, counter{};
    std::optional<bool> saved;
    std::string text;
};
StartupWorldRuntimeError open_startup_world_save_page(StartupWorldRuntimeState &state);
bool valid_startup_world_save_page(const StartupWorldRuntimeState &state,std::uint64_t page);
bool initialize_startup_world_save_pages(StartupWorldRuntimeState &candidate);
std::optional<StartupWorldSavePageView> inspect_startup_world_save_page(
    const StartupWorldRuntimeState &state,std::uint64_t page);
// 首次Update只置stage1；stage1须由应用保存桥消费，单独更新显式缺源。
std::optional<StartupWorldRuntimeState> update_startup_world_save_page(
    const StartupWorldRuntimeState &state,std::uint64_t page);
// 确认/取消共用：stage0/1无变化，stage2才Pop；暂停不执行输入。
StartupWorldRuntimeError act_startup_world_save_page(StartupWorldRuntimeState &state,std::uint64_t page);
// 只准备结果候选。成功/失败均推进一次公共counter；错误文本含前缀最多1024字节。
std::optional<StartupWorldRuntimeState> complete_startup_world_save_page(
    const StartupWorldRuntimeState &state,std::uint64_t page,bool saved,const std::string &error = {});
// 仅真实已初始化stage1可导出；严格scene3+14，不隐式清理旧退休页或未消费输出。
// 导出候选仍须经现normal完整校验；源Owner保留raw14以承接成功/失败结果。
std::optional<StartupWorldRuntimeState> prepare_startup_world_save_export(
    const StartupWorldRuntimeState &state,std::uint64_t page);
} // namespace dungeon_village_prototype
