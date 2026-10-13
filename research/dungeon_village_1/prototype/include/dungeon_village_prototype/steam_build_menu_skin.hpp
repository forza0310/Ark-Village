#pragma once

#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/steam_human_skin.hpp"

namespace dungeon_village_prototype {
// raw21专属已出版common资源；固定槽身份不借设施定义ID或PNG文件名猜测。
enum class SteamBuildMenuAsset { demolition, movement, resident, new_tag };
std::optional<SteamFacilityResource> steam_build_menu_resource(SteamBuildMenuAsset asset);
struct SteamBuildMenuImage {
    SteamBuildMenuAsset asset;
    std::array<int,2> position{};
    int frame{};
    std::optional<std::array<int,4>> crop;
};
// 保留源DrawLine端点与线宽；当前宽1轴向线已核含终点，后端据此转换像素覆盖。
struct SteamBuildMenuLine {
    std::array<int,4> endpoints{};
    int width{1};
    std::array<int,3> rgb{};
};
enum class SteamBuildMenuClipKind { set, clear };
struct SteamBuildMenuClip {
    SteamBuildMenuClipKind kind;
    std::array<int,4> rectangle{}; // 原SetClip／ClearClip，不是push交集与pop恢复。
};
struct SteamBuildMenuTouch {
    SteamStartupTouch touch;
    std::array<int,4> margin{}; // 原left/right/top/bottom，保留负值。
    std::optional<std::array<int,3>> arguments;
    int flags{}; // AddTouch下层新增的TouchOption位，不混为组件号或原option参数。
    std::optional<float> drag_grid_y{}; // 只有长表实际firstTouch非null分支安装的float32网格。
};
struct SteamBuildMenuArrowTouch {
    std::array<int,2> position{};
    int frame{}, value{};
    std::array<int,4> margin{}; // 组件18由SEB3取bounds，helper实际绘surface箭头，不重复绘SEB。
    bool selected{}; // 原箭头触摸选中事实；组件号不等于CheckTouch查询域，不写输入或Owner。
};
// 只提供当前可见实际行的源事实，不传地图实体；负ID仅原撤除−1／移动−2。
struct SteamBuildMenuRow {
    int definition{}, mapchip{-1}, kind{}, remaining{}, quote{};
    bool build_now{}, resident{}; // 原isBuildNow和type13使用的任一定义isResident谓词。
    std::string name, price_text; // 实际译文；price_text必须为当次YEN("Ｇ",quote)结果。
    std::optional<int> name_width; // 原当前字号实际StringWidth，不按字数估算。
};
struct SteamBuildMenuSkinInput {
    std::array<std::vector<int>,3> catalogs;
    StartupBuildMenuCursor cursor;
    std::vector<SteamBuildMenuRow> visible_rows; // 从scroll开始的原序实际行，最多5条。
    std::array<bool,3> category_new{};
    std::array<std::string,3> category_names; // 当前语言转换后的原三标签。
    std::array<int,2> origin{}; // 已按VIEW与Graphics原点求出的J／K，恰一次消费。
    std::int64_t funds{}; // 当前系统所持资金，只用于价格色，不执行预检／收费。
    int finger_frame{}, frame2{}; // 公共手形当前帧与页副钟，不能由主钟猜帧。
    bool japanese{}, scroll_first_touch{};
    std::array<bool,2> arrow_selected{}; // 原左右CheckTouch结果，独立于类目选择。
};
using SteamBuildMenuOperation=std::variant<SteamStartupFrameDraw,SteamBuildMenuImage,
    SteamBuildMenuLine,SteamBuildMenuClip,SteamFacilityImage,SteamFacilityNumber,
    SteamFacilityMapchipDraw,SteamHumanOverviewText,SteamBuildMenuTouch,
    SteamBuildMenuArrowTouch>;
struct SteamBuildMenuSkinPlan {
    std::vector<SteamBuildMenuOperation> operations;
    std::array<int,2> soft_labels{0,2}; // 当前合法type2／3／13为7／2；空行／特殊条目为0／2。
};
// 原raw21有序只读皮肤；不读写Owner、不推进源Draw中的frame、不安装目录或抽随机。
// 明确输入资格及资源边界；完整Owner游标／绘制准入／恢复与OS投递仍另行接线。
std::optional<SteamBuildMenuSkinPlan> steam_build_menu_skin(const SteamBuildMenuSkinInput &input);
} // namespace dungeon_village_prototype
