#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ark::simulation {
// SteamInputManager平台按钮，不占用游戏common图片命名空间。
enum class SteamPlatformButtonKind { controller_binding, keyboard_guide };
struct SteamPlatformButtonResource {
    const char *image_path;
    const char *sprite_path;
    int image{}, sprite{};
    std::array<int,4> crop{};
    std::array<int,2> offset{};
};
std::optional<SteamPlatformButtonResource> steam_platform_button_resource(SteamPlatformButtonKind kind);
struct SteamPlatformButtonInput {
    SteamPlatformButtonKind kind{};
    std::array<int,2> anchor{};
    int component{};
    bool desktop{}, xbox{}, touched{}; // 真实平台资格及CheckTouch，不从悬停猜测。
    std::optional<float> scale_ratio; // 仅controller附加glyph消费GetScaleRatio(false)，缺值拒绝。
};
struct SteamPlatformBlend {
    int mode{};
    std::array<int,4> rgba{};
};
struct SteamPlatformJoystickRequest {
    int button{}, registration{}, scale{};
    std::array<int,2> anchor{};
};
struct SteamPlatformButtonBody {
    SteamPlatformButtonResource resource;
    std::array<float,4> destination{};
    std::optional<SteamPlatformBlend> blend; // 空值继承原Graphics混色；高亮BLEND_COLOR1。
    int component{}, value{};
    std::array<int,4> touch_rectangle{}, margin{15,15,15,15};
    std::optional<SteamPlatformJoystickRequest> joystick;
    bool reset_origin{true}; // Draw/注册前PushOrigin(0,0,false)，结束恢复，不叠表单原点。
};
struct SteamPlatformButtonPlan {
    std::optional<SteamPlatformButtonBody> body; // 平台资格不满足是合法隐藏，不生成热区。
};
// 纯资源/几何/注册请求；不打开网页或绑定面板，不改Steam、图形栈、Owner或随机。
// 维护预算：坐标±65536、component0..65535、controller比例(0,10000]；不宣称原规则限值。
std::optional<SteamPlatformButtonPlan> steam_platform_button_skin(const SteamPlatformButtonInput &input);

// Kairolib._drawButtons未取得动态图像时的原SEB后备；不属于平台绑定/键盘入口按钮。
struct SteamControllerButtonResource {
    const char *image_path;
    const char *sprite_path;
    int image{}, sprite{}; // managed kairolib组的原INF索引，不是游戏common索引。
};
struct SteamControllerButtonFallbackInput {
    int input_type{}, frame{}, layer{}; // input_type为原Steam ESteamInputType值，未匹配者用Switch。
    bool auto_key_reverse{};
    std::optional<int> okay_frame, cancel_frame; // 仅原frame0/1实际消费对应Canvas映射时必填。
};
struct SteamControllerButtonFallbackPlan {
    SteamControllerButtonResource resource;
    int frame{}, layer{};
};
// 先按原frame互斥反转，再仅对原layer1的16..19映射32..35；所有层最终归0。
// 已出版SEB的维护范围为frame0..43，坏请求拒绝；不发现设备、不调用Steam或推测当前映射。
// 调用者须先完成源动态图像优先分支；frame7无控制器时的right_click路径不是此后备。
std::optional<SteamControllerButtonFallbackPlan> steam_controller_button_fallback(
    const SteamControllerButtonFallbackInput &input);

// 调用者已完成USE_STEAM/设备/frame分流并取得有效Image；这里仅还原Draw调用几何。
enum class SteamButtonImageKind { dynamic_glyph, right_click };
struct SteamButtonImageInput {
    SteamButtonImageKind kind{};
    int width{}, height{};
    std::array<int,2> anchor{};
    bool local_matrix{};
};
struct SteamButtonImagePlan {
    std::array<int,2> draw_anchor{};
    std::array<float,2> scaled_size{};
    std::optional<std::array<float,2>> translation; // TranslateTemporary→PushMatrix，不合并进anchor。
    bool recheck_matrix_before_pop{}; // Draw之后重新检查真实图形状态，不能无条件Pop。
};
// 尺寸1..65536、锚±65536为维护预算；不取得SDK图片、不操作图形栈或注册输入。
std::optional<SteamButtonImagePlan> steam_button_image_layout(const SteamButtonImageInput &input);
// frame7、USE_STEAM且HasController=false的原整图；不是SEB裁片或控制器后备。
inline constexpr const char *steam_right_click_image_path="steam-platform/right_click.png";

struct SteamLanguageButtonInput {
    bool switch_enabled{}, touched{}; // 当前Language配置的Switch资格，不用语言名或平台猜。
    int width{}, height{}, anchor{20}; // 原anchor位；标题RIGHT|TOP为20。
    std::optional<float> scale_ratio;
    int new_state{};
    std::optional<int> new_frame; // 仅new_state==2消费0..15，其余不读。
};
struct SteamLanguageNewText {
    std::array<float,2> anchor{};
    int next_frame{}; // 仅在一次获准Draw提交；重复展示计划不推进。
    int frame{8}, layer{-1};
    float scale{0.75F};
};
enum class SteamLanguageButtonStep {
    push_origin_zero, replace_callback, draw_button, draw_new,
    register_touch, register_joystick, pop_origin
};
struct SteamLanguageButtonPlan {
    std::optional<SteamPlatformButtonBody> body;
    std::optional<SteamLanguageNewText> new_text;
    int collision_mode{1}, screen_mode{1}; // 原TOUCHPARAMS[3]后两项；仅register_touch消费。
    // body各部分按steps分别消费，不在draw_button时提前登记输入/手柄。
    // body.reset_origin=false；两层origin均由steps配对，不额外插入第三层。
    std::vector<SteamLanguageButtonStep> steps;
};
// callback由调用方提供并按显式step替换；此纯计划不保存指针、不打开页面或更改语言。
// 尺寸1..65536、比例(0,10000]为维护预算；资格不满足合法隐藏，不读取其余输入。
std::optional<SteamLanguageButtonPlan> steam_language_button_skin(const SteamLanguageButtonInput &input);
struct SteamLanguageTouchResult { bool propagate{}, open_language{}; };
// 匹配组件3/value0x133A066只在UP1请求入页；其它匹配事件截断分发，不调callback。
SteamLanguageTouchResult steam_language_button_touch(int component,int value,int event_type);
// LanguageForm.Update普通结果；平台字符串解析由适配层完成，空字符串/坏格式不能当null取消。
enum class SteamLanguagePanelResultKind { null_result, parsed_selection };
struct SteamLanguagePanelResultInput {
    SteamLanguagePanelResultKind kind{};
    int selection{};
    std::vector<std::optional<std::string>> frozen_files; // 入页时复制的行[1]，保留null语义。
    bool current_file_resolved{};
    std::optional<std::string> current_file;
};
struct SteamLanguageInstallRequest {
    std::optional<std::string> file;
    bool save{true}; // 原SetLanguagePack(file,null,true)，不传CSV字节。
};
// 消费时只在前一安装返回true后读取并调用当前共享callback；不能冻结为入页私有回调。
struct SteamLanguageSuccessCallbackRequest {};
struct SteamLanguagePopRequest {};
using SteamLanguagePanelAction=std::variant<SteamLanguageInstallRequest,
    SteamLanguageSuccessCallbackRequest,SteamLanguagePopRequest>;
struct SteamLanguagePanelResultPlan { std::vector<SteamLanguagePanelAction> actions; };
// null/相同包仅Pop；新包按安装→成功条件回调→Pop。安装false仍Pop，异常不执行余项。
// 负结果的HTTP/模板/外链另属平台分支，本接口显式拒绝；不安装、不保存、不调用callback。
// 目录至多4096行、实际消费的字符串至多4096字节为维护预算；输出至多3项。
std::optional<SteamLanguagePanelResultPlan> steam_language_panel_result(
    const SteamLanguagePanelResultInput &input);
inline constexpr const char *steam_language_new_image_path="steam-platform/new.png";
inline constexpr const char *steam_language_new_sprite_path="steam-platform/new.seb";

// TitleForm._draw的调用资格；APPLI_TYPE与Platform独立，不能用是否连接控制器代替。
struct SteamTitlePlatformInput {
    int width{}, height{}, title_frame{}, state{}, application_type{};
    bool on_top{}, desktop{}, xbox{};
    bool controller_touched{}, keyboard_touched{}, language_touched{}, language_switch_enabled{};
    std::optional<float> scale_ratio;
    int new_state{};
    std::optional<int> new_frame;
};
struct SteamTitlePlatformPlan {
    SteamLanguageButtonPlan language; // Logo/确认注册之后、菜单75门槛之前。
    std::vector<SteamPlatformButtonBody> after_menu; // 菜单之后，绑定→键盘；最多2项。
};
// 不登记平台状态、替换callback、推进NEW或执行外链；调用者逐序消费显式请求。
// frame/state总是校验；其它字段仅在来源分支实际消费时验证，不为隐藏项补默认值。
std::optional<SteamTitlePlatformPlan> steam_title_platform_skin(const SteamTitlePlatformInput &input);

// 已由原TouchComponent分发到void回调；不是组件命中器，不另造propagate返回。
struct SteamPlatformButtonEventInput {
    SteamPlatformButtonKind kind{};
    int event_type{};
    bool debug{}, use_steam{}, initialized{};
    std::optional<std::uint64_t> controller_handle; // 实际消费时必填；已取得的0仍合法。
    bool language_code_resolved{}; // 区分尚未取值与原GetLanguageCode确实返回null。
    std::optional<std::string> language_code;
    // 只在DEBUG/ON13消费；两按钮共用同一downButtonTime，不是各自按住时长。
    std::optional<std::int64_t> now_millis, shared_down_millis;
};
struct SteamBindingPanelRequest { std::uint64_t controller_handle{}; };
struct SteamControlMapRequest { std::string url; };
struct SteamDebugTrophyRequest {};
// 必须在此动作执行时重新读钟并写共同字段，不复用事件判定时的now。
struct SteamCaptureSharedButtonTimeRequest {};
using SteamPlatformButtonAction=std::variant<SteamBindingPanelRequest,SteamControlMapRequest,
    SteamDebugTrophyRequest,SteamCaptureSharedButtonTimeRequest>;
struct SteamPlatformButtonEventPlan { std::vector<SteamPlatformButtonAction> actions; };
// 纯候选最多2动作；不调用SDK/浏览器/时钟或修改平台与Owner。
// 语言串至多4096字节为维护预算，按原Concat保留内容；只校验实际消费的输入。
std::optional<SteamPlatformButtonEventPlan> steam_platform_button_event(const SteamPlatformButtonEventInput &input);
} // namespace ark::simulation
