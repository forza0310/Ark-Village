#pragma once

#include "dungeon_village_prototype/steam_startup_skin.hpp"
#include <string>

namespace dungeon_village_prototype {
// Steam raw0：现成TextLayout实例/人物scratch均由调用方拥有，本层只描述原序请求。
struct SteamDialogueHuman {
    int definition{-1}, body_sprite{-1}, weapon{-1}; // 当前职业/性别外观与当前武器的真实查询结果。
};
struct SteamDialogueSkinInput {
    int width{240}, height{240}, view_y{}, main{}, wait{};
    bool on_top{true}, until_active_hide{}, english{}; // 前两项是原IsTopForm/隐藏字段；english是Language.English结果。
    int speaker_kind{}, speaker_index{-1};
    std::optional<SteamDialogueHuman> human;
    std::optional<bool> monster_flag4;
    std::string title, speaker_name; // 已经当前语言适配的标题/姓名；不从静态表补当前姓名。
    std::optional<std::array<int,2>> title_widths; // 两次真实Font.StringWidth。
};
struct SteamDialogueBodyDraw { std::array<int,2> position{37,100}; }; // 原已配置实例.Draw，不重新SetText。
enum class SteamDialogueFontAction { set_size10, reset };
struct SteamDialogueFont { SteamDialogueFontAction action; };
struct SteamDialogueWindowColor {}; // Graphics.SetColor(SC_WINDOW_BLACK)，正文实例自有颜色另保留。
struct SteamDialogueNameDraw {
    std::string text;
    std::array<int,4> rectangle{};
    int anchor{34}, line_space{}; // 当前Graphics color；不由字符数代替TextLayout。
};
struct SteamDialogueHumanSet {
    int definition{-1}, action{}, body_sprite{-1}, step{}, direction{2}, main{};
};
struct SteamDialogueHumanDraw {
    std::array<int,2> position{193,189};
    int weapon{-1}, step{};
};
struct SteamDialogueMonsterDraw {
    int definition{-1};
    std::array<int,2> position{};
    int direction{2};
};
using SteamDialogueDraw = std::variant<SteamStartupFrameDraw,StartupSkinDraw,
    SteamDialogueWindowColor,SteamDialogueBodyDraw,SteamDialogueFont,SteamDialogueNameDraw,
    SteamDialogueHumanSet,SteamDialogueHumanDraw,SteamDialogueMonsterDraw,SteamStartupTouch>;
struct SteamDialogueSkinPlan {
    std::array<int,2> origin{};
    std::string title;
    std::vector<SteamDialogueDraw> draws;
};
// 仅MakeTalkForm构造x/y=0的标准raw0内容；不认证其它非零位置writer。
// 外层应用origin一次；VIEW_Y只交Window/Box。隐藏页不消费正文/头像载荷。
// 维护预算：画布1..65536，main0..INT_MAX-1，文字/测宽<=4096。未知speaker保持原不画头像。
std::optional<SteamDialogueSkinPlan> steam_dialogue_skin(const SteamDialogueSkinInput &input);

enum class SteamDialogueUpdateRequest { frame, confirm_pulse, debug_gate, debug_held,
    is_end, flush, set_text, pop, unsupported_next_talk };
struct SteamDialogueUpdateInput {
    std::vector<std::string> paragraphs;
    int paragraph{}, next_talk{-1};
    bool confirm_pulse{}, debug_enabled{}, debug_held{};
    // 只能提供本轮真实Frame之后的IsEnd观测；没有确认资格时不消费/校验此值。
    std::optional<bool> ended_after_frame;
};
struct SteamDialogueUpdatePlan {
    std::vector<SteamDialogueUpdateRequest> requests;
    int paragraph{};
    std::optional<std::string> next_text;
    // 请求正常完成后的源返回；Pop须转交真实返回值，未支持的后继也不冒称true。
    std::optional<bool> normal_return{true};
};
// 这是带实际查询观测的有序合同，不是可直接执行的单次事务或已完成动作回执。
// Frame先于查询；调用者为取得ended_after_frame已执行Frame时，禁止重复执行trace中的Frame。
// 不推导墙钟/逐字量，不读Owner/随机，不直接acknowledge；next非-1仅在段尽时显式未支持。
std::optional<SteamDialogueUpdatePlan> steam_dialogue_update(const SteamDialogueUpdateInput &input);
} // namespace dungeon_village_prototype
