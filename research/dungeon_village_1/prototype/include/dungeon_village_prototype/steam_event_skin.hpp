#pragma once

#include "dungeon_village_prototype/steam_startup_skin.hpp"
#include "dungeon_village_prototype/steam_human_skin.hpp"
#include <cstdint>
#include <string>

namespace dungeon_village_prototype {
struct StartupWorldRuntimeState;
// 固定Steam raw11局部素材；路径相对assets，未请求的包/图号显式拒绝。
struct SteamEventResource {
    const char *image_path;
    const char *sprite_path; // 整图无SEB；不按PNG列数猜帧。
    int width{}, height{}, sprite{-1};
};
std::optional<SteamEventResource> steam_event_resource(StartupSkinPackage package,int image);
struct SteamEventSkinInput {
    int main{}, command{}; // 原frame_与eventIds_[page_]；任意int命令沿源else，不限为0/1/2。
    std::string text; // 原已替换正文，保留br/co/po等标签，交实际TextLayout处理。
};
struct SteamEventSkinOptions {
    int width{240},height{240},view_y{},wait{};
    bool japanese{},on_top{true},until_active_hide{};
    std::string title; // 当前语言标题；原literal为“イベント”。
    std::optional<std::array<int,2>> title_widths; // 两次真实Font测宽。
    std::optional<int> greeting_width; // raw59获准显示气泡后，当前Font的真实StringWidth。
    // raw59四个已核literal：气泡／职业名后缀／姓名后缀／末行。调用方可显式提供有限翻译适配；
    // 这里不猜翻译，不改变原三次Graphics.DrawString的独立布局资格。
    std::array<std::string,4> human_unlock_text{"よろしく","の","さんが","訪れるようになりました!"};
};
struct SteamEventTextLayout {
    std::string text;
    std::array<int,4> rectangle{};
    std::array<int,3> rgb{92,51,31};
    int anchor{},line_space{6};
};
enum class SteamEventClipKind { push_intersect,pop };
struct SteamEventClip {
    SteamEventClipKind kind;
    std::array<int,4> rectangle{}; // 真实clip尺寸，可为0高；Pop恢复旧clip，不清成全屏。
};
using SteamEventDraw=std::variant<SteamStartupFrameDraw,StartupSkinDraw,
    SteamEventTextLayout,SteamEventClip,SteamStartupTouch,SteamHumanImage,SteamHumanPortrait,SteamHumanOverviewText>;
struct SteamEventSkinPlan {
    std::array<int,2> origin{};
    std::array<int,2> soft_labels{0,0};
    std::string title;
    std::vector<SteamEventDraw> draws;
};
// 标准MakeEvent x/y=0入口。VIEW_Y只交Window/Box，origin由外层恰一次安装。
// 保留原窗/背景→正文clip→继续SEB→人物clip/三人/烟花→AddTouch2原序。
// 维护预算：画布1..65536/main0..INT_MAX-1/可见文字及标题测宽<=4096；不推进世界/随机/声音。
std::optional<SteamEventSkinPlan> steam_event_skin(
    const SteamEventSkinInput &input,const SteamEventSkinOptions &options);
// 严格只读已初始化life1/2/3的真实11；阶段/主钟/命令数组缺失或孤立时拒绝，不默认0。
std::optional<SteamEventSkinInput> steam_event_input(const StartupWorldRuntimeState &state,std::uint64_t page);
struct SteamHumanUnlockSkinInput {
    int main{},profession{},sex{},weapon{-1};
    std::string profession_name,name; // 当前职业／Owner姓名；不是定义初始职业或已到访实例。
};
// raw59只读当前定义；life1/2/3及现main载荷须经Owner校验，无默认0／无创建实体。
std::optional<SteamHumanUnlockSkinInput> steam_human_unlock_input(
    const StartupWorldRuntimeState &state,std::uint64_t page);
// _draw2 1034ED99–1034F27E；60气泡、70页脚，页脚为25槽/前15槽、锚212/204。
// 不混用raw11的40槽/锚213；不提交main60的Steam成就，不调用共享scratch附效。
std::optional<SteamEventSkinPlan> steam_human_unlock_skin(
    const SteamHumanUnlockSkinInput &input,const SteamEventSkinOptions &options);
} // namespace dungeon_village_prototype
