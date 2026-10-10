#pragma once

#include "ark/simulation/steam_startup_skin.hpp"
#include <array>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ark::simulation {
// Steam资源身份独立于APK同号槽。路径相对已出版assets目录，不读取work或Unity容器。
enum class SteamFacilityAsset {
    wood, title_bar, corner, arrow, mini, number05, number08, number09, maximum,
    upgrade_background, mini_background
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
enum class SteamFacilityNumberKind { number, money, plus_value };
struct SteamFacilityNumber {
    SteamFacilityNumberKind kind;
    SteamFacilityAsset asset;
    int value{};
    std::array<int,2> position{};
    // 仅number使用padding/anchor；money/plus保留原具名helper，position为调用实参。
    // money内的dx−9与plus内的加号定位留给helper消费者，不能在接线时重复偏移。
    int padding{}, anchor{};
    int parameter{-1}; // -1当前等级；0/1/2对应冻结属性行，不重算业务值。
};
// 展开实际SEB请求；普通数字的步宽来自所选SEB frame0/line0的SP_W，不能用Font字宽。
// money/plus沿源固定8步宽及逗号原序，digit_width不参与这两类计算。
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
} // namespace ark::simulation
