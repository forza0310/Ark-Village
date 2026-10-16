#pragma once

// 商会83—86与购买设施领取93：使用唯一世界Owner的金币、点数、库存和页面。
#include "dungeon_village_prototype/startup_world_runtime.hpp"

namespace dungeon_village_prototype {
enum class StartupCommerceAction {
    confirm,
    cancel,
    previous,
    next,
    select,
    previous_tab,
    next_tab,
    inspect
};
struct StartupCommerceView {
    int raw{};
    int mode{}; // 84/86：0买入，1出售。
    int tab{};  // 买入目录的价格/持有数量展示，不改变交易方向。
    int selection{};
    int first_visible{};
    int binding{-1};
    int feedback_counter{};
    int counter{};
    std::vector<int> entries;
    std::optional<int> feedback_amount{}; // 84上一笔实际成交价；无反馈时无值，不从当前选择重算。
};
// Steam83的一轮已解析输入；设备选择先安装，再按上→下→确认优先返回消费。
// 仅作用于主页，不提前定义84/85的反馈、滚动和marker契约。
struct StartupCommerceTopInput {
    bool up{}, down{}, confirm{}, cancel{};
    std::optional<int> selection;
};
StartupWorldRuntimeError input_startup_world_commerce_top(
    StartupWorldRuntimeState &state,std::uint64_t page,const StartupCommerceTopInput &input);
// Steam85：组件滚动与设备选择先安装，上→下后修三行窗，确认优先于返回和详情。
// Scroll只描述本轮已解析输入，首行仍复用原commerce六槽，不新增持久字段。
struct StartupCommerceFacilityInput {
    bool up{}, down{}, confirm{}, cancel{}, inspect{};
    std::optional<int> selection;
    struct Scroll { bool absolute{}; int value{}; };
    std::optional<Scroll> scroll;
};
StartupWorldRuntimeError input_startup_world_commerce_facility(
    StartupWorldRuntimeState &state,std::uint64_t page,const StartupCommerceFacilityInput &input);
StartupWorldRuntimeError open_startup_world_commerce(StartupWorldRuntimeState &state);
bool initialize_startup_world_commerce_pages(StartupWorldRuntimeState &state);
// 已初始化商会载荷的只读校验；允许挂起的父页，不要求栈顶，也不补字段或初始化。
bool valid_startup_world_commerce_page(const StartupWorldRuntimeState &state,const ref::WorldScriptPage &page);
// 84待Finish可有合法耗尽的空目录；仍须保留完整反馈/成交金额配对，不能借退休掩盖缺键。
bool valid_startup_world_commerce_feedback(const StartupWorldRuntimeState &state,const ref::WorldScriptPage &page);
// raw93领取前后沿同一交易载荷；关闭待退休不能通过删除字段伪装未初始化。
bool valid_startup_world_commerce_claim_page(const StartupWorldRuntimeState &state,const ref::WorldScriptPage &page);
std::optional<StartupCommerceView>
inspect_startup_world_commerce_page(const StartupWorldRuntimeState &state, std::uint64_t page);
StartupWorldRuntimeError act_startup_world_commerce_page(StartupWorldRuntimeState &state,
                                                         std::uint64_t page,
                                                         StartupCommerceAction action,
                                                         int selection = 0);
std::optional<StartupWorldRuntimeState>
update_startup_world_commerce_page(const StartupWorldRuntimeState &state, std::uint64_t page,
                                 const StartupCommerceItemInput *input = nullptr);
} // namespace dungeon_village_prototype
