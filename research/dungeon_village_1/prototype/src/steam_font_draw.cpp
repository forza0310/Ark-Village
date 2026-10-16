// 固定Steam Graphics.DrawString RVA7663D0的正常GUI阶段10766804–107674D9。
// Source实际供给字体/矩阵/GUI及绘制；本层保持查询与副作用顺序，不模拟原生字形。
// 测宽、字符转换与Label均保留原UTF-16单元，不在此修补被渐显切开的代理对。
#include "dungeon_village_prototype/steam_font_draw.hpp"

#include <cmath>
#include <limits>

namespace dungeon_village_prototype {
namespace {
static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559);
float f32(float value) {volatile float out=value;return out;}
bool coordinate(float value) {return std::isfinite(value)&&std::abs(value)<=65536.0f;}
bool dimension(float value) {return std::isfinite(value)&&value>0&&value<=65536.0f;}
std::int32_t signed32(std::uint32_t bits) {
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):
        static_cast<std::int32_t>(static_cast<std::int64_t>(bits)-0x100000000LL);
}
bool valid_color(const SteamGuiTextColor &color) {
    for(const auto c:color)if(!std::isfinite(c))return false;
    return true;
}
} // namespace
std::optional<SteamGuiTextResult> steam_draw_ordinary_gui_text(
        const SteamGuiTextInput &in,SteamGuiTextBackend &out) {
    if(in.admission!=SteamGuiTextAdmission::prepared_ordinary_gui||
       in.translated_text.size()>4096||!coordinate(in.x)||!coordinate(in.y)||
       in.used_size<=0||in.used_size>65536)return {};
    float x=in.x,y=in.y;
    const auto u=f32(static_cast<float>(in.used_size));
    // 只有CENTER/RIGHT真实测宽；CENTER优先，不用字符数预测宽度。
    if(in.anchor&6) {
        const auto width=out.string_width_f(in.translated_text);
        if(!std::isfinite(width)||width<0||width>65536.0f)return {};
        x=f32(x-((in.anchor&2)?f32(width*0.5f):width));
    }
    if(in.anchor&32)y=f32(y-f32(u*0.5f));
    else if(in.anchor&64)y=f32(y-u);
    else if(in.font_scale_offset) {
        if(in.font_scale==0)return {};
        const auto product=signed32(static_cast<std::uint32_t>(in.used_size)*100U);
        // 当前正常域U上界使乘积不达MIN；仍显式保留原商/差的32位语义。
        const auto unscaled=product/in.font_scale;
        const auto diff=signed32(static_cast<std::uint32_t>(unscaled)-static_cast<std::uint32_t>(in.used_size));
        y=f32(y+f32(static_cast<float>(diff/2)));
    }
    const auto half=f32(u*0.5f);
    const auto extent=out.reference_rect_size(half,u);
    if(!dimension(extent[0])||!dimension(extent[1]))return {};
    const auto sx=f32(extent[0]/half),sy=f32(extent[1]/u);
    if(!std::isfinite(sx)||!std::isfinite(sy)||sx<=0||sy<=0)return {};
    const auto scale=sy>sx?sx:sy; // 正常有限域保持源比较次序，不处理NaN为正常输入。
    const auto scaled=f32(u*scale);
    const auto requested=f32(scaled+0.001f);
    if(!std::isfinite(requested)||requested<0||
       static_cast<double>(requested)>std::numeric_limits<std::int32_t>::max())return {};
    const auto size=static_cast<std::int32_t>(requested);
    if(!in.font_keep) {
        out.copy_font_to_gui_style();out.copy_font_style_to_gui_style();out.set_font_keep();
    }
    out.set_gui_font_size(size);
    // 以未截整scaled补偿非等比矩阵的Y，不能用刚写的整数GUI字号反推。
    y=f32(y+f32(f32(u-f32(scaled/sy))*0.5f));
    if(!coordinate(x)||!coordinate(y))return {};
    out.synchronize_skin_font();
    const auto mapped=out.map_screen_point(x,y);
    if(!coordinate(mapped[0])||!coordinate(mapped[1]))return {};
    const std::array<float,4> rectangle{
        f32(mapped[0]-f32(static_cast<float>(in.clip_origin[0]))),
        f32(mapped[1]-f32(static_cast<float>(in.clip_origin[1]))),1.0f,f32(2.0f*extent[1])};
    const auto saved=out.normal_text_color();
    if(!valid_color(saved))return {};
    if((in.render_operator==0||in.render_operator==1)&&in.render_src_ratio<255) {
        auto color=out.normal_text_color(); // 原有第二次读取，不改用先前保存值。
        if(!valid_color(color))return {};
        const auto ratio=f32(static_cast<float>(in.render_src_ratio));
        if(in.render_operator==0) {
            for(int i=0;i<3;++i)color[i]=f32(f32(ratio*color[i])/255.0f);
        } else color[3]=f32(f32(ratio*color[3])/255.0f);
        if(!valid_color(color))return {};
        out.set_normal_text_color(color);
    }
    const auto text=out.unicode_to_chanakya(in.translated_text);
    if(text.size()>4096)return {};
    out.update_gui_group();out.label(rectangle,text);
    out.set_normal_text_color(saved);
    out.clear_skin_font_if_nonnull();
    return SteamGuiTextResult{rectangle,size};
}
} // namespace dungeon_village_prototype
