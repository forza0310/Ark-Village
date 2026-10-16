#pragma once

#include "dungeon_village_prototype/steam_manual_skin.hpp"

namespace dungeon_village_prototype {
// raw15只接真实TextLayout观测；泰语短路时原代码不读取字号。
std::optional<int> steam_news_rows(bool thai, std::optional<int> rendering_font_size);
// MakeNews 315750的逐步有符号整数式；合法0页原样返回，不补成1页。
// 非负高度、正lineStep及无int溢出是维护拒绝边界，不是原游戏额外保护。
std::optional<int> steam_news_page_count(int layout_height, int line_step, int rows);

struct SteamNewsUpdateInput {
    int page{}, pages{}, main{}, secondary{};
    bool left_repeat{}, right_repeat{}, confirm_pulse{}, back_soft_label{};
    bool debug_command{}, debug_held{}; // DEBUG资格与held观测分开，非正常用户按钮。
};
struct SteamNewsUpdatePlan {
    int page{}, main{}, secondary{};
    // left Repeat、right Repeat、confirm Pulse、DEBUG held、soft2的实际检查。
    std::array<bool,5> checked_keys{};
    bool request_pop{}; // 只报告请求；真实FormManager.Pop返回结果尚未消费。
};
// 已准入且wait=0的一整轮Update；0页可等待/确认/返回，实际命中左右取模则拒绝除零。
std::optional<SteamNewsUpdatePlan> steam_news_update(const SteamNewsUpdateInput &input);

enum class SteamNewsFontAction { set_size11, reset };
struct SteamNewsFont { SteamNewsFontAction action; };
enum class SteamNewsColorRole { window_blue }; // SubForm.SC_WINDOW_BLUE，不编未知RGB。
struct SteamNewsColor { SteamNewsColorRole role{SteamNewsColorRole::window_blue}; };
struct SteamNewsNameDraw {
    std::string text;
    std::array<int,2> position{120,69};
    int anchor{2};
};
struct SteamNewsLogoDraw {
    int common_image{125};
    const char *published_image{"original/common/wnd_newsLogo.png"};
    std::array<int,2> position{29,62}; // 原DrawImage整图，不造裁片尺寸。
};
// 对调用方已配置/Flush的同一个TextLayout实例执行Draw；不创建字形或重排正文。
struct SteamNewsBodyDraw { std::array<int,2> position{41,106}; int scroll_v{}; };
using SteamNewsDraw=std::variant<SteamStartupFrameDraw,SteamNewsLogoDraw,SteamNewsColor,SteamNewsFont,
    SteamNewsNameDraw,SteamManualArrowRequest,SteamNewsBodyDraw,SteamStartupTouch>;
struct SteamNewsSkinInput {
    int width{240}, height{240}, view_y{}, wait{}, pages{}, page{}, main{};
    bool japanese{}, english{}, thai{}, on_top{true}, until_active_hide{};
    bool configured_text_layout{};
    std::optional<int> rendering_font_size, line_step;
    std::optional<std::string> window_title, name; // 真实语言题头(含页码)与当前name_，允许空串。
    std::optional<std::array<int,2>> title_widths; // 原两次真实StringWidth，不按字符串长度估算。
    std::array<bool,2> arrow_selected{}; // CheckTouch(1,16/18)，不以悬停代替。
};
struct SteamNewsSkinPlan {
    std::array<int,2> origin{};
    std::string title;
    std::vector<SteamNewsDraw> draws;
};
// MakeNews x/y=0正常入口；窗口/内框helper各只消费一次VIEW_Y。
// 维护预算：尺寸1..65536、文字及测宽<=4096、main/page<=INT_MAX-1；不是原版限制。
// pages=0仍可画原静态画面，但不认证可导航；绘制不更新Owner、随机、系统槽或布局。
std::optional<SteamNewsSkinPlan> steam_news_skin(const SteamNewsSkinInput &input);
} // namespace dungeon_village_prototype
