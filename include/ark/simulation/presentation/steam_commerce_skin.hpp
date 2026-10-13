#pragma once

#include "ark/simulation/presentation/steam_human_skin.hpp"

namespace ark::simulation {
// Steam raw83的独立资源：私有SPCHARA槽2是南瓜人物，不能套公共角色ID2。
enum class SteamCommerceAsset { row_background, category_icon, private_pumpkin };
struct SteamCommerceResource {
    const char *group;
    int image{}, sprite{-1};
    const char *published_image;
    const char *published_sprite;
};
std::optional<SteamCommerceResource> steam_commerce_resource(SteamCommerceAsset asset);
struct SteamCommerceImage {
    SteamCommerceAsset asset;
    std::array<int,2> position{}; // SEB沿调用锚，内部offset只加一次。
    int frame{};
    std::optional<std::array<int,4>> crop;
};
struct SteamCommerceSkinInput {
    int selection{};
    int rank{}; // 原UserData.nowRank_，显示rank+1，不传当前资金或已加1的值。
    bool item_new{}, facility_new{}; // 原两个独立谓词结果；不作为目录/确认资格。
};
// 从已初始化栈顶83读取原NEW谓词；缺真实字段拒绝，不在查询内创建载荷。
std::optional<SteamCommerceSkinInput> steam_commerce_input(
    const StartupWorldRuntimeState &state,std::uint64_t page);
// 当前语言转换后的真实文字与两次标题实测宽；未提供时不猜字体或代入日文字面。
struct SteamCommerceTextInput {
    std::string title, welcome;
    std::array<std::string,3> names, explanations;
    std::optional<std::array<int,2>> title_widths;
};
struct SteamCommerceSkinOptions {
    int view_y{}, finger_frame{}; // 原共享finger_r当前帧由表现消费者显式提供。
    bool japanese{};
    SteamCommerceTextInput text;
};
using SteamCommerceOperation=std::variant<SteamStartupFrameDraw,SteamCommerceImage,
    SteamFacilityImage,SteamFacilityNumber,SteamHumanImage,SteamHumanOverviewText,
    SteamStartupTouch,SteamHumanClip>;
struct SteamCommerceSkinPlan {
    std::string title;
    std::array<int,2> soft_labels{0,2};
    std::vector<SteamCommerceOperation> operations;
};
// 有序纯皮肤：窗框/内框→欢迎/等级/私有裁剪人物→三行→选中说明。
// 不读Owner、不初始化子页、不改NEW/库存、页面计数或随机；仅覆盖raw83。
std::optional<SteamCommerceSkinPlan> steam_commerce_skin(
    const SteamCommerceSkinInput &input,const SteamCommerceSkinOptions &options);
} // namespace ark::simulation
