#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace dungeon_village_prototype {
using SteamGuiTextColor=std::array<float,4>;
enum class SteamGuiTextAdmission { unverified, prepared_ordinary_gui };
struct SteamGuiTextInput {
    SteamGuiTextAdmission admission{SteamGuiTextAdmission::unverified};
    std::string translated_text; // 已通过上游翻译/TextFormat/临时字号；Chanakya转换仍在本链内。
    float x{}, y{}; // BeginRenderMatrix后坐标，不再次平移原始世界坐标。
    std::int32_t anchor{0x11}, used_size{}, font_scale{};
    bool font_scale_offset{}, font_keep{};
    std::array<std::int32_t,2> clip_origin{};
    std::int32_t render_operator{}, render_src_ratio{255};
};

// 单一实际Graphics上下文的后端，不持世界Owner。每步返回后才执行下一步；
// 默认字体/矩阵/GUI对象及字符转换必须来自宿主真实消费者，不由接口补假值。
class SteamGuiTextBackend {
public:
    virtual ~SteamGuiTextBackend()=default;
    virtual float string_width_f(const std::string &)=0;
    // 原基准Rect(0,0,width,height)经GetTransRectGUI后的实际宽/高。
    virtual std::array<float,2> reference_rect_size(float width,float height)=0;
    virtual void copy_font_to_gui_style()=0; // 当前Font.GetFont -> GUIStyle.font。
    virtual void copy_font_style_to_gui_style()=0; // GetFontStyle -> GUIStyle.fontStyle。
    virtual void set_font_keep()=0;
    virtual void set_gui_font_size(std::int32_t)=0;
    virtual void synchronize_skin_font()=0; // 实际相等查询；不等才设为当前style.font。
    virtual std::array<float,2> map_screen_point(float x,float y)=0;
    virtual SteamGuiTextColor normal_text_color()=0;
    virtual void set_normal_text_color(const SteamGuiTextColor &)=0;
    virtual std::string unicode_to_chanakya(const std::string &)=0;
    virtual void update_gui_group()=0;
    virtual void label(const std::array<float,4> &rectangle,const std::string &)=0;
    virtual void clear_skin_font_if_nonnull()=0; // 原尾部清null，不能恢复入场前font。
};
struct SteamGuiTextResult {
    std::array<float,4> rectangle{};
    std::int32_t gui_font_size{};
};
// 执行Graphics.DrawString正常GUI局部链，不是全部DrawString入口或世界事务。
// 准入要求实际isGUI=true/useFontTexture=false/glRender=false及上游已准备完成。
// 正常域：U1..65536，输入及转换后文本各<=4096字节；消费的x/y、映射点有限且
// 绝对值<=65536，测宽0..65536、参考尺寸(0,65536]，实际比率/中间值有限且sy>0。
// clip原点允许全int32；最终Label坐标不受65536限制，高度2B可至131072。
// TOP字号补偿才要求F非0，保留int32回卷与整数除法。退化/越界显式拒绝，不夹分母。
// 后端可能已改style/font或绘制；拒绝/异常不回滚它们，不可将失败整段盲重放。
// 后端异常原样传播；EndRenderMatrix/本次本地化锁/请求字号恢复仍归外层调用者。
std::optional<SteamGuiTextResult> steam_draw_ordinary_gui_text(
    const SteamGuiTextInput &,SteamGuiTextBackend &);
} // namespace dungeon_village_prototype
