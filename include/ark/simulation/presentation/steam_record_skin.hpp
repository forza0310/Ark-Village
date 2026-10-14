#pragma once

#include "ark/simulation/presentation/steam_manual_skin.hpp"
#include <cstdint>

namespace ark::simulation {
// 独立RankForm的正常state0主体，不是SubForm raw页；可翻译文案由调用方明确提供。
struct SteamRecordLabels { std::string english_title, translated_title, topic, bottom; };
struct SteamRecordSkinInput {
    int view_width{}, view_height{}, page{}, frame{};
    bool japanese{}, english{};
    std::int64_t value{}; // 当前页对应的系统纪录，不是世界钱包；维护合法域非负。
    std::string village;
    SteamRecordLabels labels;
    std::optional<int> topic_width, value_width;
    std::optional<float> village_width; // 原StringWidthF，不能先截为整数。
    std::array<bool,2> arrow_selected{};
    int definition_count{};
    std::vector<int> frozen_definitions; // Init后的原序；fallback可能超过5人，不在Draw截断。
};
struct SteamRecordWindow { SteamStartupWindow source; std::string title; };
struct SteamRecordText { std::string value; std::array<float,2> position{}; std::array<int,3> rgb{}; };
using SteamRecordDraw=std::variant<SteamRecordWindow,SteamRecordText,StartupSkinDraw,
    SteamManualActorRequest,SteamManualArrowRequest,SteamStartupTouch>;
struct SteamRecordSkinPlan {
    std::array<int,2> origin{}, final_origin{}; // 正常/已核异常收尾清0，不恢复旧origin。
    std::array<int,2> soft_labels{0,2};
    std::vector<SteamRecordDraw> draws;
};
// 仅Rank实际P/全角Ｇ专支；其它币种、万/k换算及负数不混入此接口。
std::optional<std::string> steam_record_value_text(int page,std::int64_t value,bool japanese);
// 维护尺寸/定义目录上界65536/4096，字符串/测宽4096；不声称原限制。
// frame必须显式提供纪录钟；不借标题钟、不抽人、不改Owner/快照或创建第二份人物。
std::optional<SteamRecordSkinPlan> steam_record_skin(const SteamRecordSkinInput &input);

struct SteamRecordUpdateInput {
    int page{}, frame{};
    bool state_zero{true}; // 原state==0资格；非0仍推进独立frame，但不检查输入。
    bool left_repeat{}, right_repeat{}, back_soft_label{}; // 最后一项是soft2，不猜物理Esc/确认。
};
struct SteamRecordUpdatePlan {
    int page{}, frame{};
    std::array<bool,3> checked_keys{}; // 原检查顺序Left Repeat、Right Repeat、soft2。
    bool request_pop{}; // 不代表Pop成功；原返回值由实际管理器提供。
};
// 一次已准入的Rank.Update纯候选。只返回页/钟及请求；无新Owner或持久字段。
std::optional<SteamRecordUpdatePlan> steam_record_update(const SteamRecordUpdateInput &input);
} // namespace ark::simulation
