#pragma once

#include <cstdint>
#include <optional>
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

// 仅普通TextBlock；pic/btn须在上游独立分流。预算/长度按UTF-16单元，不截UTF-8字节。
struct SteamTextPlainRun { std::int32_t units{}; bool prefix{}; };
enum class SteamTextPlainSkip { none, blink, zero_budget, outside_viewport };
struct SteamTextPlainBudgetPlan {
    std::int32_t consumed{};
    SteamTextPlainSkip skip{SteamTextPlainSkip::none};
    std::optional<SteamTextPlainRun> text; // 有值即请求真实绘制/测宽，空串也可有请求。
};
struct SteamTextPlainDrawInput {
    std::int32_t budget{}, utf16_length{}, blink_period{}, blink_counter{};
    float block_y{}, dy{}, box_y{}, box_height{-1};
    std::optional<int> font_used_size{}; // 仅实际走纵视口判断才读取，不能按字符数推测。
};
// 源序：消费量->blink->budget恰0->纵视口->SetColor及真实文字请求。
// 维护正常域预算/长度非负；实际消费的坐标有限且绝对值<=65536、字号0..65536。
// 短路分支不校验未用视口/字号；盒高恰-1免纵裁剪。边缘相接仍可绘制。
std::optional<SteamTextPlainBudgetPlan> steam_text_plain_draw_budget(const SteamTextPlainDrawInput &);
// 普通块StringWidth不查blink/视口；请求前缀或整串的真实Font.StringWidth。
std::optional<SteamTextPlainBudgetPlan> steam_text_plain_width_budget(
    std::int32_t budget,std::int32_t utf16_length);

// 已排块的真实投影。null文字不能伪装成空串；由上游拒绝null块本体。
// 特殊块也提供实际text.Length，本组高度查询不按pic/btn旗标改为长度1。
struct SteamTextBlockMetrics {
    std::optional<std::int32_t> utf16_length{};
    float y{};
    std::optional<std::int32_t> font_used_size{};
};
// GetHeight取非空块float32最大高度，末尾加0.9F再截零；不读渐显/钟/间距。
// 实际消费的y有限且|y|<=65536，字号0..65536；空文字不读取这两项。
std::optional<std::int32_t> steam_text_content_height(const std::vector<SteamTextBlockMetrics> &);
// GetNextHeight有自己的signed32墙钟差/乘法，不复用Frame的int64位置或offset。
// begin为0时取两次墙钟，局部减50且不写回；按完整块长度扣预算，逐块截整高度。
// 输入及source失败规则沿上文；未遍历块不校验，不读取TotalFrames/共同门槛。
std::optional<std::int32_t> steam_text_next_height(const SteamTextRevealState &,
    const std::vector<SteamTextBlockMetrics> &,SteamTextRevealSource &);
struct SteamTextLineHeightFallback {}; // 空块目录须走真实Font复制/本地化/缩放，不能补0。
using SteamTextLineHeight=std::variant<std::int32_t,SteamTextLineHeightFallback>;
// 非空目录只取首块实际字号，不读该块文字/y或取其余块最大值。
std::optional<SteamTextLineHeight> steam_text_line_height(const std::vector<SteamTextBlockMetrics> &);
// 传实际GetLineHeight结果，保持原int32加法；不代替上述fallback执行。
std::int32_t steam_text_line_step(std::int32_t observed_line_height,std::int32_t line_space);
} // namespace dungeon_village_prototype
