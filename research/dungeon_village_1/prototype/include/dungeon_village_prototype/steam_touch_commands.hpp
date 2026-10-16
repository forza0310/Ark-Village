#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace dungeon_village_prototype {
// GameView组件命中和顶页hook之后的局部请求；不是OS事件、按键状态或业务操作。
enum class SteamTouchAdmission { unverified, ready, intercepted };
// 原事件component的SetPaint字段与GetKey位型，非命中矩形或实体引用。
struct SteamTouchArrowPaint { int x{}, y{}, frame{}; std::uint64_t component_key{}; };
struct SteamTouchCommandInput {
    SteamTouchAdmission admission{SteamTouchAdmission::unverified};
    int component{}, event_type{}; // 本接口仅0–2、4–7、9、18–22、24；事件沿原0..13，非Android action。
    std::int32_t value{}, delta_x{}; // delta_x仅19/CELL_DRAG消费，单位由原事件产生者提供。
    // option.keys非空时的首值，0及0xffffffff均原样保留；不是物理keyCode。
    std::optional<std::uint32_t> first_key;
    std::vector<std::int32_t> extended_values;
    std::optional<SteamTouchArrowPaint> arrow_paint; // 仅1/18的DOWN实际消费。
};
struct SteamTouchKeyClick { std::uint32_t bits{}; };
struct SteamTouchKeyDown { std::uint32_t bits{}; bool pulse{}; };
struct SteamTouchKeyUp { std::uint32_t bits{}; };
// 一次Set的全部写请求使用该次重新取得的同一顶页accessor；type只保留高16位位型。
// 扩展type0仍保留，不能合并重复项或用页号替换原accessor资格。
struct SteamTouchValueWrite { std::uint32_t type{}; std::int32_t value{}; };
// 请求AddEffect(id0,paintX,paintY,texture_row,同一事件component)，再对返回对象写vx。
// texture_row索引GameView.TEXTURES，不是common图片ID或最终PNG绑定。
// 不创建效果池／Image，也不把效果创建失败当成成功；原KeyDown在此前已执行且不回滚。
struct SteamTouchArrowEffect { SteamTouchArrowPaint paint; int texture_row{}; float velocity_x{}; };
using SteamTouchCommand = std::variant<SteamTouchKeyClick,SteamTouchKeyDown,
    SteamTouchKeyUp,SteamTouchValueWrite,SteamTouchArrowEffect>;
struct SteamTouchCommandPlan { std::vector<SteamTouchCommand> commands; };

// 顺序见ui/STEAM_INTERACTIONS.md。拦截返回空计划并不读组件；未核准入/未知ID/坏事件拒绝。
// 不执行Canvas、SetTouchValue、表单、随机或声音；逻辑键仍需原focus/DecideKeyState/guard。
// 写请求交唯一Owner联合提交是维护策略，原Set主项及扩展项并不具备失败回滚。
// 消费的扩展项最多4096是维护预算，非原版上限；未走Set分支不校验它。
std::optional<SteamTouchCommandPlan> steam_touch_commands(const SteamTouchCommandInput &input);

// MAINMENU先正序寻找formID4/raw3。identity是调用方稳定观察身份，不是地址、raw页号或目录索引。
struct SteamTouchFormObservation {
    std::uint64_t identity{};
    int form_id{};
    std::optional<int> subform_type; // 仅formID4被实际扫描时要求已解析。
};
struct SteamTouchMainMenuInput {
    SteamTouchAdmission admission{SteamTouchAdmission::unverified};
    int event_type{};
    std::int32_t value{};
    std::optional<std::vector<SteamTouchFormObservation>> ordered_forms;
    std::optional<bool> has_submenu_component; // 本轮实际GetComponentId(9)观察，不是猜父页类型。
    std::optional<std::int32_t> main_menu_value; // 仅有9分支读取首raw3的指定高16位属性。
    // nullopt未查询；0表示实际GetTopForm为空。Pop仍保留空请求，不能用0作属性写目标。
    std::optional<std::uint64_t> top_form;
    std::vector<std::int32_t> extended_values;
};
enum class SteamTouchMainMenuProbeKind { scan_form, submenu_component, main_menu_value, top_form };
struct SteamTouchMainMenuProbe {
    SteamTouchMainMenuProbeKind kind{};
    std::uint64_t form{};
    std::uint32_t type{};
};
struct SteamTouchFormPop { std::uint64_t form{}; }; // 0保留原Pop(null)调用意图，不认证其后端成功。
struct SteamTouchAccessorWrite {
    std::uint64_t accessor{};
    SteamTouchValueWrite write;
};
using SteamTouchMainMenuCommand = std::variant<SteamTouchFormPop,SteamTouchAccessorWrite,SteamTouchKeyClick>;
struct SteamTouchMainMenuPlan {
    std::optional<std::uint64_t> main_menu;
    // 已消费观测的顺序轨迹，不是待执行查询；禁止照此再次查询而重复消费。
    std::vector<SteamTouchMainMenuProbe> probes;
    std::vector<SteamTouchMainMenuCommand> commands;
};
// 有9且改值：先请求Pop当时顶页，再向已经绑定的首raw3写主项/扩展；原Pop bool未检查。
// 消费者不能Pop后重找accessor；维护Owner的联合提交不由本纯候选承诺。
// 最多实际扫描4096项；已短路的尾部观察、未消费的顶页/扩展均不校验。
std::optional<SteamTouchMainMenuPlan> steam_touch_main_menu(const SteamTouchMainMenuInput &input);

struct SteamTouchUnmarkerInput {
    SteamTouchAdmission admission{SteamTouchAdmission::unverified};
    int event_type{};
    std::int64_t marker_key{-1}; // 仅-1是原无标记；其它负值保留原位型。
    // 本轮实际查询观察；DOWN观察时点在原frame归0之后，非UI上一轮缓存。
    std::array<std::optional<bool>,3> scrollbar_events{}; // 当前事件的ID12/14/15查询结果。
    std::optional<bool> marker_event; // 当前事件的GetTouchEventKey(marker_key,type)。
};
enum class SteamTouchUnmarkerProbeKind { component, marker_key };
struct SteamTouchUnmarkerProbe {
    SteamTouchUnmarkerProbeKind kind{};
    int component{}, event_type{};
    std::int64_t key{};
};
struct SteamTouchMarkerFrameReset {}; // 只将markComponentFrame设0。
struct SteamTouchMarkerClear {}; // ClearMarker：key=-1且frame=0。
using SteamTouchUnmarkerCommand = std::variant<SteamTouchMarkerFrameReset,SteamTouchMarkerClear>;
struct SteamTouchUnmarkerPlan {
    // 已消费观测的轨迹，不可执行第二遍。两向量各保序，不冒充跨查询/动作的完整执行trace。
    std::vector<SteamTouchUnmarkerProbe> probes;
    std::vector<SteamTouchUnmarkerCommand> commands;
};
// DOWN先重置frame，再按12→14→15→旧key短路；UP只在有旧key时查同序；DRAG仅查旧key。
// 已分发事件及实际surface查询由调用者提供；不模拟OS命中、组件注册或保存表现状态。
std::optional<SteamTouchUnmarkerPlan> steam_touch_unmarker(const SteamTouchUnmarkerInput &input);

// 列表与公共滚动的临时续体：后续读取依赖实际Set及扩展的结果，不能预填未来观测。
struct SteamTouchFlowInput {
    SteamTouchAdmission admission{SteamTouchAdmission::unverified};
    int component{}, event_type{}; // 仅3/10/11及12/13/14/15。
    std::int32_t value{}; // 原派发调用的v；不提前冻结事件对象的dy或组件ID。
    // event_identity是维护方为本次事件分配的非0唯一标识，非原TouchEvent字段。
    // binding指本叶入口实际e.component，使用期间的存活／身份由外层事务保证。
    std::uint64_t event_identity{}, component_binding{};
};
enum class SteamTouchFlowOperation {
    done, arguments, get_current, set_current, marker_flag, marker_key,
    component_key, component_id, event_component_key, event_first_key, event_delta_y,
    write_marker, click_key
};
struct SteamTouchFlowRequest {
    SteamTouchFlowOperation operation{SteamTouchFlowOperation::done};
    std::uint64_t event_identity{}, sequence{}, component_binding{};
    std::uint32_t type{};
    std::int32_t value{};
    std::uint64_t key{};
};
struct SteamTouchFlowArguments { std::optional<std::vector<std::int32_t>> values; };
struct SteamTouchFlowValue { std::int32_t value{}; };
struct SteamTouchFlowFlag { bool enabled{}; };
struct SteamTouchFlowKey { std::uint64_t bits{}; };
struct SteamTouchFlowFirstKey { std::optional<std::uint32_t> bits; };
struct SteamTouchFlowCompleted { bool success{}; }; // 整个请求真实完成才true，生成计划不算完成。
using SteamTouchFlowResult=std::variant<SteamTouchFlowArguments,SteamTouchFlowValue,
    SteamTouchFlowFlag,SteamTouchFlowKey,SteamTouchFlowFirstKey,SteamTouchFlowCompleted>;
struct SteamTouchFlowReply {
    std::uint64_t event_identity{}, sequence{};
    SteamTouchFlowResult result;
};
class SteamTouchFlow {
public:
    static std::optional<SteamTouchFlow> begin(const SteamTouchFlowInput &input);
    const SteamTouchFlowRequest &request() const noexcept { return request_; }
    // 坏类型、错事件/序号、未完成请求及完成后回复拒绝且保留旧续体。
    bool resume(const SteamTouchFlowReply &reply);
private:
    enum class Phase {
        done, list_set, list_flag, list_marker, list_compare, list_replace,
        list_write, list_first_key, list_click, arguments, bar_candidate,
        bar_relative_dy, bar_absolute_dy, bar_compare, bar_delta, bar_write,
        bar_selection, bar_component_id, finish_write,
        button_dec_check, button_inc_check, button_dec_read, button_inc_read,
        button_move_write, bottom_selection, bottom_first, bottom_read,
        bottom_write, top_selection, top_first, top_read, top_write,
        tail_positive, tail_fits, tail_read, tail_write
    };
    SteamTouchFlow()=default;
    void emit(Phase phase,SteamTouchFlowOperation operation,std::int32_t type=0,
              std::int32_t value=0,std::uint64_t key=0,std::uint64_t binding=0);
    void get(Phase phase,std::int32_t type);
    void set(Phase phase,std::int32_t type,std::int32_t value);
    void done();
    void bar_candidate(std::int32_t value);
    bool consume(const SteamTouchFlowResult &result);
    SteamTouchFlowInput input_;
    SteamTouchFlowRequest request_;
    Phase phase_{Phase::done};
    std::int32_t count_{}, visible_{}, selection_type_{-1}, wanted_{}, delta_{}, selection_{};
    std::uint64_t old_marker_{};
};
// get_current/set_current每次在消费时重新GetTopForm；type0的Get也调用真实getter。
// 一次Set主项和所有扩展绑定本次accessor，含type0/重复项/原setter副作用，结束才回执。
// 输入component_binding是本叶入口捕获对象，区别于顶页hook之前的派发组件；component_key/
// marker_flag/component_id及arguments使用这个固定对象，但内容在请求到达时才读取。
// event_component_key/event_first_key重新读取当时e.component；event_delta_y在该步读取e.dy。
// 列表set_current绑定入口对象；滚动set_current的binding为0，消费时先重读e.component、
// 再GetTopForm；不能把0当成空组件或提前省略其扩展。
// 列表marker_key/write_marker指派发接收者实例，不凭静态ClearMarker猜成同一对象。
// 调用方在私有Owner/表现候选的一次事件事务内完整消费，失败弃候选；不保存/跨轮悬挂。
// 本类固定大小，每次只推进一个请求，不自动执行尾循环；原循环不保证终止。
} // namespace dungeon_village_prototype
