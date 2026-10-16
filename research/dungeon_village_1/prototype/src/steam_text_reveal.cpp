// Steam TextLayout RVA7CAC70/7CACD0/7CB640/7CBA00/7CC0B0的数值消费者。
// 显式复现位宽回卷，避免把原机器算术变成C++ signed溢出或过强的clamp。
#include "dungeon_village_prototype/steam_text_reveal.hpp"

#include <limits>
#include <algorithm>
#include <cmath>

namespace dungeon_village_prototype {
namespace {
static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559);
std::int32_t signed32(std::uint32_t bits) {
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):
        static_cast<std::int32_t>(static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::int64_t signed64(std::uint64_t bits) {
    return bits<=0x7fffffffffffffffULL?static_cast<std::int64_t>(bits):
        -static_cast<std::int64_t>(~bits)-1;
}
std::int32_t length_units(std::int32_t length) {
    return signed32(static_cast<std::uint32_t>(length)*1000U);
}
} // namespace

SteamTextRevealFramePlan steam_text_reveal_frame(const SteamTextRevealState &in,
        const std::vector<SteamTextBlinkState> &blocks,SteamTextRevealSource &source) {
    SteamTextRevealFramePlan plan{in,blocks};
    auto &s=plan.candidate;
    if(s.begin_time==0)
        s.begin_time=signed64(static_cast<std::uint64_t>(source.current_time_millis())-50U);
    if(static_cast<std::int64_t>(source.total_frames())==s.last_app_frame)
        s.time_offset=signed32(static_cast<std::uint32_t>(s.time_offset)+50U);
    s.last_app_frame=source.total_frames();
    const auto now=source.current_time_millis();
    const auto elapsed=static_cast<std::uint64_t>(now)-static_cast<std::uint64_t>(s.begin_time)+
        static_cast<std::uint64_t>(static_cast<std::int64_t>(s.time_offset));
    auto pos=signed64(elapsed*static_cast<std::uint64_t>(static_cast<std::int64_t>(s.speed)))/50;
    if(pos<0)pos=0;
    const auto limit=length_units(s.length);
    if(pos>limit)pos=limit;
    s.pos=static_cast<std::int32_t>(pos); // 上述原序比较后必在int32范围内，含负limit。
    for(auto &block:plan.blocks)if(block.period>0) {
        const auto next=signed32(static_cast<std::uint32_t>(block.counter)+1U);
        const auto period=signed32(static_cast<std::uint32_t>(block.period)*2U);
        // 正period的双倍为非零偶数，除数既不为0也不为-1。
        block.counter=next%period;
    }
    if(source.common_frames_update()&&s.frame<std::numeric_limits<std::int32_t>::max())++s.frame;
    return plan;
}
SteamTextFlushResult steam_text_reveal_flush(const SteamTextRevealState &in,SteamTextRevealSource &source) {
    auto s=in;
    s.pos=length_units(s.length);
    const auto now=source.current_time_millis();
    if(s.speed==0)return SteamTextRevealFault::flush_divide_by_zero;
    // pos是int32，乘1000后不可能达到int64 MIN；这里两次除法均无MIN/-1风险。
    const auto elapsed=(static_cast<std::int64_t>(s.pos)*1000/s.speed)/20;
    s.begin_time=signed64(static_cast<std::uint64_t>(now)-static_cast<std::uint64_t>(elapsed));
    return s;
}
bool steam_text_reveal_is_end(const SteamTextRevealState &s) {
    return s.pos==-1||s.pos>=length_units(s.length);
}
SteamTextRevealState steam_text_reveal_set_speed(const SteamTextRevealState &in,
        std::int32_t speed,bool japanese) {
    auto s=in;s.speed=speed;
    if(!japanese)s.speed=signed32(static_cast<std::uint32_t>(speed)*3U)/2;
    return s;
}
SteamTextRevealState steam_text_reveal_set_text_tail(const SteamTextRevealState &in,
        std::int32_t length) {
    auto s=in;s.length=length;s.pos=0;s.begin_time=0;s.time_offset=0;return s;
}
SteamTextRevealState steam_text_reveal_reset(const SteamTextRevealState &in) {
    auto s=in;s.pos=0;s.begin_time=0;s.time_offset=0;s.last_app_frame=0;return s;
}
// 普通TextBlock.Draw 107DA37C–43D的预算/资格；实际矩阵/染色/Font绘制不在这里执行。
std::optional<SteamTextPlainBudgetPlan> steam_text_plain_draw_budget(const SteamTextPlainDrawInput &in) {
    if(in.budget<0||in.utf16_length<0)return {};
    SteamTextPlainBudgetPlan plan{std::min(in.budget,in.utf16_length),SteamTextPlainSkip::none,{}};
    if(in.blink_period>0&&in.blink_counter>=in.blink_period) {
        plan.skip=SteamTextPlainSkip::blink;return plan;
    }
    if(in.budget==0) {plan.skip=SteamTextPlainSkip::zero_budget;return plan;}
    if(in.box_height!=-1.0f) {
        const auto coordinate=[](float x) {return std::isfinite(x)&&std::abs(x)<=65536.0f;};
        if(!coordinate(in.box_height)||!coordinate(in.block_y)||!coordinate(in.dy)||
           !coordinate(in.box_y)||!in.font_used_size||*in.font_used_size<0||*in.font_used_size>65536)return {};
        // 对应源addss各自落回float32，保留边缘相接，不引入横向裁剪。
        const auto rounded=[](float x) {volatile float value=x;return value;};
        const auto y=rounded(in.block_y+in.dy);
        if(in.box_y>rounded(y+static_cast<float>(*in.font_used_size))||
           y>rounded(in.box_y+in.box_height)) {
            plan.skip=SteamTextPlainSkip::outside_viewport;return plan;
        }
    }
    plan.text=SteamTextPlainRun{plan.consumed,in.budget<in.utf16_length};return plan;
}
// TextBlock.StringWidth 107DB1A9–221的普通分支，不复用Draw的隐藏资格。
std::optional<SteamTextPlainBudgetPlan> steam_text_plain_width_budget(
        std::int32_t budget,std::int32_t length) {
    if(budget<0||length<0)return {};
    SteamTextPlainBudgetPlan plan{std::min(budget,length),SteamTextPlainSkip::none,{}};
    if(budget==0)plan.skip=SteamTextPlainSkip::zero_budget;
    else plan.text=SteamTextPlainRun{plan.consumed,budget<length};
    return plan;
}
} // namespace dungeon_village_prototype
