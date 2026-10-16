#pragma once

#include <cstdint>
#include <variant>
#include <vector>

namespace dungeon_village_prototype {
// TextLayout渐显的数值投影；不是完整排版器、第二个Owner或新增存档格式。
// length必须来自最终排版块：普通文字按UTF-16单元，pic/btn各1，累计int32回卷。
struct SteamTextRevealState {
    std::int32_t length{}, pos{}, speed{1000}, time_offset{}, frame{};
    std::int64_t begin_time{}, last_app_frame{};
};
struct SteamTextBlinkState { std::int32_t period{}, counter{}; };

// 宿主提供真实获准调用的外部观测；函数按源序逐次读取，不合并两次TotalFrames。
// 可用记录源回放；不得自行将世界tick、FPS或本机时间当作已认证原平台观测。
class SteamTextRevealSource {
public:
    virtual ~SteamTextRevealSource()=default;
    virtual std::int64_t current_time_millis()=0;
    virtual std::int32_t total_frames()=0;
    virtual bool common_frames_update()=0;
};
struct SteamTextRevealFramePlan {
    SteamTextRevealState candidate;
    std::vector<SteamTextBlinkState> blocks;
};
// 渐显->按序blink->末尾共同门槛/动画钟。外部读取抛错则传播，不修改输入。
// 候选仅在整次成功后交调用者安装；原对象入口可能留下部分修改，不宣称原版回滚。
SteamTextRevealFramePlan steam_text_reveal_frame(const SteamTextRevealState &,
    const std::vector<SteamTextBlinkState> &,SteamTextRevealSource &);

enum class SteamTextRevealFault { flush_divide_by_zero };
using SteamTextFlushResult=std::variant<SteamTextRevealState,SteamTextRevealFault>;
// 速度0也先取得一次墙钟，随后返回具名故障，无成功候选。原入口此前已写pos。
SteamTextFlushResult steam_text_reveal_flush(const SteamTextRevealState &,SteamTextRevealSource &);
bool steam_text_reveal_is_end(const SteamTextRevealState &);
SteamTextRevealState steam_text_reveal_set_speed(const SteamTextRevealState &,
    std::int32_t speed,bool japanese);
// 仅实际SetText完成排块后的数值尾；不代替翻译/替换/排块，不复位旧动画计数。
SteamTextRevealState steam_text_reveal_set_text_tail(const SteamTextRevealState &,
    std::int32_t final_block_length);
SteamTextRevealState steam_text_reveal_reset(const SteamTextRevealState &);
} // namespace dungeon_village_prototype
