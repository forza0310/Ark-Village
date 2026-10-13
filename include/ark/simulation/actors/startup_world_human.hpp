#pragma once

#include "ark/simulation/world/startup_world_runtime.hpp"

namespace ark::simulation {
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
// raw67持原旧/新努力、共享四战斗显示及构造主钟；查询不补齐半初始化页。
bool valid_startup_world_runtime_effort_result_page(const StartupWorldRuntimeState &state,
                                                    const ref::WorldScriptPage &page);
// raw88年度授勋结果持人物、主钟和两段状态；共享奖励值不等同人物当前值。
bool valid_startup_world_runtime_award_result_page(const StartupWorldRuntimeState &state,
                                                   const ref::WorldScriptPage &page);
// raw70原构造双钟；阶段切换只归零主钟，不能由绘制补建缺失副钟。
bool valid_startup_world_human_mastery_page(const StartupWorldRuntimeState &state,
                                          const ref::WorldScriptPage &page);
// 原页0回复入口资格：每次实时按人物原序取首个同定义W，缺来源返回空。
std::optional<bool> startup_world_human_recovery_gift(const StartupWorldRuntimeState &state,int human);
struct StartupHumanPresentationLive {
    ref::CharacterId id;
    int state{};
    int hp_now{};   // 原PB_NOW=1，只供血条；不与目标HP合并。
    int hp_after{}; // 原PB_AFTER=3，供危险图标条件。
    int hp_max{};   // 当前共享定义派生上限，不使用创建时初值。
};
struct StartupHumanPresentationView {
    std::uint64_t page_id{};
    int tab{};
    int frame{};
    StartupHumanDetails human;
    std::optional<StartupHumanPresentationLive> live;
    bool tracking_available{}; // 只读来源资格：source1显示追踪；输入另查栈顶/生命周期/暂停。
    int equipment_selection{}; // raw60原select_，0..3；离开装备页保留，下次进入按方向重置。
};
// 投影指定已初始化raw60，含被子页遮住的挂起父页；可绘制不代表可交互。
// 坏载荷返回空，不初始化页面、不推进帧或修改HP。
std::optional<StartupHumanPresentationView>
inspect_startup_world_human_presentation(const StartupWorldRuntimeState &state,
                                       std::uint64_t page);
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
    equipment_slot,
    track,
    scroll_relative,
    scroll_absolute
};
struct StartupHumanCatalogueInput {
    bool up{},down{},left{},right{},confirm{},cancel{},inspect_equipment{};
    std::optional<int> selection;
    struct Scroll { bool absolute{};int value{}; };
    std::optional<Scroll> scroll; // 已核组件12事件8/9；物理坐标由平台转换，不存设备状态。
};
struct StartupHumanCatalogueCursor { int selection{},first{},count{},visible{}; };
// 只读真实已初始化61/64；不能从选择派生或补建缺失scroll，不投影退休页。
std::optional<StartupHumanCatalogueCursor> inspect_startup_world_human_catalogue_cursor(
    const StartupWorldRuntimeState &state,std::uint64_t page);
// 按原同轮次序合成上下/左右，最后确认优先；失败保留整个旧Owner及随机/输出。
StartupWorldRuntimeError input_startup_world_human_catalogue_page(StartupWorldRuntimeState &state,
    std::uint64_t page,const StartupHumanCatalogueInput &input);
// raw60组件滚动先同步page，再按进入本轮的页分支合成上下；最终页决定确认路由。
// 复用非持久DTO；不监听左右/装备信息，追踪仍由既有独立动作处理。
StartupWorldRuntimeError input_startup_world_human_detail_page(StartupWorldRuntimeState &state,
    std::uint64_t page,const StartupHumanCatalogueInput &input);
// raw70同轮选择/左右后确认；失败不留下选择改变，副钟不受早确认/切段影响。
StartupWorldRuntimeError input_startup_world_human_mastery_page(StartupWorldRuntimeState &state,
    std::uint64_t page,const StartupHumanCatalogueInput &input);
// 仅对事务候选调用：合法场景/35入口创建raw60并绑定追踪来源，不复制Owner。
std::optional<std::uint64_t> append_startup_world_human_detail_page(
    StartupWorldRuntimeState &candidate, int human, int chase_mode,
    std::optional<ref::CharacterId> actor = {});
// 缺失来源不能由初始化补默认；可选原W必须与详情定义一致。
bool valid_startup_world_human_detail_context(const StartupWorldRuntimeState &state,
                                             std::uint64_t page);
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
} // namespace ark::simulation
