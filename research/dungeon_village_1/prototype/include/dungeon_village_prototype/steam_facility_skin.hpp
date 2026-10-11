#pragma once

#include "dungeon_village_prototype/steam_startup_skin.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace dungeon_village_prototype {
// Steam资源身份独立于APK同号槽。路径相对已出版assets目录，不读取work或Unity容器。
enum class SteamFacilityAsset {
    wood, title_bar, corner, arrow, mini, number05, number08, number09, maximum,
    upgrade_background, mini_background, number03, number11, number12, number03_hud,
    facility_icon, effect_icon, level_label, finger, title_arrows, people
};
struct SteamFacilityResource {
    const char *group;
    int image{}, sprite{-1};
    const char *published_image;
    const char *published_sprite; // nullptr表示直接图片裁片。
};
std::optional<SteamFacilityResource> steam_facility_resource(SteamFacilityAsset asset);
struct SteamFacilityImage {
    SteamFacilityAsset asset;
    std::array<int,2> position{};
    int frame{};
    std::optional<std::array<int,4>> crop; // 空值沿已核SEB；内部offset只加一次。
};
enum class SteamFacilityTextRole { upgrade_title, facility_notice, level_prefix, level_suffix,
                                  level_completed, parameter_name };
struct SteamFacilityText {
    SteamFacilityTextRole role;
    int argument{}; // notice为定义ID，parameter为0/1/2；实际翻译由文本消费者提供。
    std::array<int,4> rectangle{};
    std::optional<int> anchor;
    std::array<int,3> rgb{};
    int font_size{}; // 0保留当前字体；9是原临时请求，画后恢复。
    bool japanese{}; // notice的名称后缀/翻译分支，不能猜为同一串字。
};
enum class SteamFacilityClipKind { push_intersect, pop };
struct SteamFacilityClip {
    SteamFacilityClipKind kind;
    std::array<int,4> rectangle{};
};
// 页面计划保留具名helper请求；数字可经下方独立函数展开，Mapchip2执行器仍另接。
struct SteamFacilityMapchip2 {
    int mapchip{};
    std::array<int,2> position{};
    int orientation{}; // 81固定0；消费者使用Mapchip2居中pattern，不套普通建设锚点。
};
struct SteamFacilityMapchipDraw {
    std::string sprite; // image组内已出版SEB文件名，Steam固定85映射与其字节逐项已核。
    int frame{};
    std::array<int,2> position{}; // 包含Mapchip2居中偏移，不含SEB内部offset。
};
// 固定已核mapchip目录；不借地图可建/旋转资格限制只读helper，也不改道路请求帧。
std::optional<std::vector<SteamFacilityMapchipDraw>> steam_facility_mapchip2_draws(
    const SteamFacilityMapchip2 &request);
enum class SteamFacilityNumberKind { number, money, plus_value, comma_number };
struct SteamFacilityNumber {
    SteamFacilityNumberKind kind;
    SteamFacilityAsset asset;
    int value{};
    std::array<int,2> position{};
    // number使用原padding/位锚；comma_number只支持已核padding0与精确anchor2/4。
    // money/plus保留原具名helper，position为调用实参。
    // money内的dx−9与plus内的加号定位留给helper消费者，不能在接线时重复偏移。
    int padding{}, anchor{};
    int parameter{-1}; // -1当前等级；0/1/2对应冻结属性行，不重算业务值。
};
// 展开实际SEB请求；普通数字的步宽来自所选SEB frame0/line0的SP_W，不能用Font字宽。
// money/plus沿源固定8步宽及逗号原序，digit_width不参与这两类计算。
// number03_hud独立绑定SEB10/sourceY0；35的number03绑定SEB11/sourceY12，二者普通步宽8。
// number11绑定SEB20，普通字格步宽7；与number05均支持独立comma_number固定8步宽。
// comma_number保留frame10请求；SEB20缺该帧的空绘制由已核后端消费，不借别图补逗号。
// 39负利润number12绑定Steam image109/SEB19，支持普通数/money，不扩为plus消费者。
// 源负数可能请求负frame；保留请求事实，不能当作已认证的负帧像素/减号映射。
std::optional<std::vector<SteamFacilityImage>> steam_facility_number_draws(
    const SteamFacilityNumber &number, int digit_width);
using SteamFacilityDraw=std::variant<StartupSkinRect,SteamFacilityImage,SteamFacilityText,
                                    SteamFacilityClip,SteamFacilityMapchip2,SteamFacilityNumber>;
struct SteamFacilitySkinPlan {
    std::vector<SteamFacilityDraw> draws; // 必须原序执行，不能把文本/图/clip分别重排。
    std::array<int,2> soft_labels{0,0};
    std::vector<SteamStartupTouch> touches; // 确认组件2 option2，原无矩形重载不猜物理热区。
};
struct SteamFacilityUpgradeSkinInput {
    int definition{}, mapchip{}, level{1};
    int phase{}, frame{}, frame2{}, view_y{};
    bool japanese{};
    std::array<std::array<int,3>,3> attributes{}; // 各slot的[前,后,差]，来自Owner冻结载荷。
    std::array<int,3> limits{};
    std::optional<std::array<int,2>> title_widths; // DrawWindow阴影/正文各一次真实StringWidth。
    // phase0非日文的数字文本、prefix、suffix、再次prefix四次实际整数测宽。
    std::optional<std::array<int,4>> notice_widths;
};
// Steam81独立纯计划；输入必须为已初始化页面事实。无Owner、计数推进、声音、随机或存档。
// int32算术超界与坏冻结差额拒绝是维护安全约束，不复制原运行时异常/回绕。
std::optional<SteamFacilitySkinPlan> steam_facility_upgrade_skin(
    const SteamFacilityUpgradeSkinInput &input);
struct StartupWorldRuntimeState;
struct SteamFacilityUpgradeSkinOptions {
    int view_y{};
    bool japanese{};
    std::optional<std::array<int,2>> title_widths;
    std::optional<std::array<int,4>> notice_widths;
};
// 从真实已初始化raw81读取冻结值与独立计数；平台只提供语言、视口及实际测宽。
// 失效页面/绑定/载荷返回空，不初始化、不升级、不推进计数或领取声音。
std::optional<SteamFacilitySkinPlan> steam_facility_upgrade_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const SteamFacilityUpgradeSkinOptions &options);

// raw74普通实例两页。保留文字重载与绘制/触摸交错；不含住宅、预览和商品页。
enum class SteamFacilityDetailTextRole {
    title, name, price, parameter, remaining, maximum_level, no_effects,
    bonus_title, maintenance, no_bonus, bonus_name, bonus_value, bottom
};
enum class SteamFacilityTextMethod { draw_string, draw_string2, text_layout };
struct SteamFacilityDetailText {
    SteamFacilityDetailTextRole role;
    int argument{};
    // bonus_value由argument指定标签，消费者拼<co=0065FF>译文</co>后接signed串；
    // 标签蓝为(0,101,255)，不是SC_WINDOW_BLUE；数值仍为棕色。不执行业务。
    std::string value;
    std::array<int,4> rectangle{};
    std::optional<int> anchor;
    int font_size{};
    SteamFacilityTextMethod method{};
    std::array<int,3> rgb{};
};
// 此helper一并绘箭头与注册组件1；不得重复普通图片绘制或猜其OS热区。
struct SteamFacilityArrowTouch { std::array<int,2> position{}; int frame{}, value{}; };
// 原helper调用12后25；12在GameView注册下层立即绘轨道/滑块。count含隐藏行，空表仍调用。
struct SteamFacilityScrollRequest {
    std::array<int,4> rectangle{221,85,3,110};
    int count{}, visible{5}, prefix{0x20000};
    std::array<int,4> margin{3,20,0,0};
};
struct SteamFacilityScrollSkinPlan {
    std::array<StartupSkinRect,2> rectangles; // 原顺序：暗轨道，再蓝／触摸橙滑块。
    std::array<int,4> thumb_touch; // 组件12注册基矩形，与绘图区宽／高并不相同。
};
// 只展开raw74的组件12/type0；当前scroll来自调用者的本页光标，不写Owner或注册输入。
// 首次触摸只在超过五行时变橙；空／短表也绘整根蓝条。未知请求／越界scroll显式拒绝。
std::optional<SteamFacilityScrollSkinPlan> steam_facility_scroll_skin(
    const SteamFacilityScrollRequest &request,int first,bool first_touch=false);
using SteamFacilityDetailOperation=std::variant<SteamFacilityDraw,SteamFacilityDetailText,
    SteamStartupTouch,SteamFacilityArrowTouch,SteamFacilityScrollRequest>;
struct SteamFacilityDetailSkinPlan {
    std::vector<SteamFacilityDetailOperation> operations;
    std::array<int,2> soft_labels{0,2};
};
struct SteamFacilityDetailBonus {
    int definition{}, ordinal{}, icon{}, kind{};
    std::string name;
    std::vector<int> values; // 原y序，kind2两项、其余一项；不分摊累计邻接。
};
struct SteamFacilityBonusCursor { int selection{}, first{}; };
// 同轮先up再down，各自循环，scroll只跟随选择；纯状态值，不加入世界/存档。
std::optional<SteamFacilityBonusCursor> steam_facility_bonus_cursor(
    int count, SteamFacilityBonusCursor cursor, bool up, bool down);
struct SteamFacilityDetailSkinInput {
    int definition{}, mapchip{}, icon{}, level{1}, page{}, frame{}, view_y{};
    std::string name;
    bool japanese{}, english{};
    std::array<int,4> values{}; // 实例价格/品质/魅力/维护费，只读Owner计算结果。
    std::array<int,3> limits{};
    int remaining{}, finger_frame{}; // SEB当前帧由表现消费者提供，不按页面frame猜帧。
    std::vector<std::array<int,2>> effects; // 原z/A的[属性,加号数]原序。
    std::vector<SteamFacilityDetailBonus> bonuses; // 初始化Y原序，包括隐藏行。
    SteamFacilityBonusCursor cursor;
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> bottom_width; // 原Font.StringWidth的结果；不按汉字数估宽。
};
std::optional<SteamFacilityDetailSkinPlan> steam_facility_detail_skin(
    const SteamFacilityDetailSkinInput &input);
struct SteamFacilityDetailSkinOptions {
    int view_y{}, finger_frame{};
    bool japanese{}, english{};
    SteamFacilityBonusCursor cursor;
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> bottom_width;
};
// 原页面/实例/定义/完整Y核验后才出图。不初始化、不写页面选择、不消费输出。
std::optional<SteamFacilityDetailSkinPlan> steam_facility_detail_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const SteamFacilityDetailSkinOptions &options);
} // namespace dungeon_village_prototype
