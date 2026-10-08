#pragma once

#include "dungeon_village_prototype/startup_world_runtime.hpp"

namespace dungeon_village_prototype {
struct StartupHumanDetails {
    int definition{};
    int profession{};
    int level{};
    int experience{};
    int threshold{};
    int satisfaction{};
    int effort{};
    int medals{};
    bool resident{};
    std::array<int, 6> attributes{};
    std::array<int, 4> combat{};
    std::array<std::optional<int>, 4> equipment;
    std::array<bool, 4> spells{};
    std::optional<ref::CharacterId> live_actor;
    std::string name; // 统一Owner定义覆盖解析结果，未到访定义也可查询。
    int sex{};
};
// raw60只读当前共享定义/装备；不使用创建时的metadata替代玩家变更。
std::optional<StartupHumanDetails>
startup_world_human_details(const StartupWorldRuntimeState &state, int human);
// 原Character.h()实时读共享w0；重算后同步活跃/保留实例的上限缓存，不回血或夹当前HP。
bool synchronize_startup_world_human_capacity(StartupWorldRuntimeState &candidate, int human);
enum class StartupHumanPageAction {
    previous,
    next,
    select,
    confirm,
    cancel,
    professions,
    gifts,
    inspect_equipment,
    view_tab,
    equipment_slot
};
StartupWorldRuntimeError open_startup_world_human_page(StartupWorldRuntimeState &state, int human);
// 新建生命周期0页尚无目录/显示载荷；表现与输入等待真正框架初始化，不由绘制补推进。
bool startup_world_human_page_ready(const StartupWorldRuntimeState &state, std::uint64_t page);
// 框架入口按原页栈顺序初始化全部新人物页，包括被结果页遮住的63；只对候选Owner调用。
bool initialize_startup_world_human_pages(StartupWorldRuntimeState &candidate);
StartupWorldRuntimeError act_startup_world_human_page(StartupWorldRuntimeState &state,
                                                      std::uint64_t page,
                                                      StartupHumanPageAction action,
                                                      int selection = 0);
// 初始化/恢复父页答案/计数演出在唯一Owner内提交；不由绘制推进。
std::optional<StartupWorldRuntimeState>
update_startup_world_human_page(const StartupWorldRuntimeState &state, std::uint64_t page);

struct StartupWorldResourceUsage {
    std::size_t live_actors{}, retired_actors{}, live_encounters{}, retired_encounters{};
    std::size_t facilities{}, pending_tasks{}, retained_tasks{}, pages{}, page_payloads{};
    std::size_t sound_outputs{}, effects{}, continuations{};
};
// 规模诊断不是业务守卫；合法历史/引用不能为满足固定数量而删除。
StartupWorldResourceUsage startup_world_resource_usage(const StartupWorldRuntimeState &state);
} // namespace dungeon_village_prototype
