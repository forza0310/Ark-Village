#pragma once

#include <optional>
#include <string>

namespace ark::simulation {
struct StartupWorldRuntimeState;
struct StartupWorldRules;
// 可变人物定义覆盖；身份由Owner map的稳定定义ID保存，不改冻结原表。
struct StartupWorldHumanProfile {
    std::string name;
    int sex{};
    bool custom_name{}; // 原e.x：自定义后切性别不替换名字，标题层负责选择默认姓名。
};
// 所有世界姓名/性别消费者共用此只读解析。非法覆盖/定义返回空，不抛map.at。
std::optional<StartupWorldHumanProfile>
startup_world_human_profile(const StartupWorldRuntimeState &state, int definition);
// 世界Owner建立前的原始新局投影仅能解析冻结默认值；安装后消费者使用Owner重载。
std::optional<StartupWorldHumanProfile>
startup_world_human_profile(const StartupWorldRules &rules, int definition);
bool valid_startup_world_human_profiles(const StartupWorldRuntimeState &state);
bool valid_startup_world_human_profile(const StartupWorldHumanProfile &profile);
// 只安装在私有真实新局候选：不创建定义0实例、不改资金/随机/日期。
// 拒绝运行中世界及已有覆盖；恢复使用校验API，不能以本函数修补坏档。
bool install_startup_world_main_character(StartupWorldRuntimeState &candidate,
                                         const StartupWorldHumanProfile &profile);
} // namespace ark::simulation
