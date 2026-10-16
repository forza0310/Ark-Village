#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace dungeon_village_prototype {
// 真实Font对象的接口；四层引用分别是入场、可选本地化、工作及逐块冻结副本。
// 字号是原usedSize/size字段，不是像素包围盒或按字符数推导的宽度。
class SteamLayoutFont {
public:
    virtual ~SteamLayoutFont()=default;
    virtual std::int32_t used_size() const=0;
    virtual std::int32_t request_size() const=0;
    virtual std::int32_t style() const=0;
};
using SteamLayoutFontRef=std::shared_ptr<SteamLayoutFont>;

// _makeTextBlocks的实际外部消费者。所有字符串均按UTF-16代码单元；
// 文化相关比较/Trim/整数解析由宿主实际提供，不在此替成浏览器富文本或ASCII算法。
class SteamTextLayoutBackend {
public:
    virtual ~SteamTextLayoutBackend()=default;
    virtual void begin_sample()=0;
    virtual void end_sample()=0;
    virtual void push_auto_text_layout(bool)=0;
    virtual void pop_auto_text_layout()=0;
    virtual bool japanese_style_text_layout()=0;
    virtual std::u16string language_code()=0;
    virtual void push_localize(bool)=0;
    virtual void pop_localize()=0;
    virtual bool needs_localize(const SteamLayoutFontRef &)=0;
    virtual SteamLayoutFontRef copy_font(const SteamLayoutFontRef &)=0;
    virtual std::int32_t localized_request(const SteamLayoutFontRef &)=0;
    virtual void install_layout_font(const SteamLayoutFontRef &)=0;
    virtual void set_size(const SteamLayoutFontRef &,std::int32_t,bool preset)=0;
    virtual void set_style(const SteamLayoutFontRef &,std::int32_t)=0;
    virtual void adjust_font_size(const SteamLayoutFontRef &,float ratio)=0;
    virtual void set_layout_disabled(bool)=0;
    virtual std::u16string translate_text(const std::u16string &,bool)=0;
    virtual std::u16string unicode_to_chanakya(const std::u16string &)=0;
    virtual void set_chanakya_enabled(bool)=0;
    virtual float string_width_f(const SteamLayoutFontRef &,const std::u16string &)=0;
    virtual int compare_to(const std::u16string &,const std::u16string &)=0;
    virtual bool starts_with(const std::u16string &,const std::u16string &)=0;
    virtual bool trim_is_empty(char16_t)=0;
    // 仅原两次局部ParseInt捕获；其它后端异常应传播，不能一律伪装解析失败。
    virtual std::optional<std::int32_t> try_parse_integer(const std::u16string &,int radix)=0;
    virtual std::int32_t parse_decimal(const std::u16string &)=0;
    // StringUtil.Split默认trim=true/excludeZeroLength=false/escape=false：
    // 首尾仅裁<=0x20；非空输入保空片，空输入返回零长数组。
    virtual std::vector<std::u16string> split_parameters(const std::u16string &)=0;
    virtual std::int32_t rgb_color(std::int32_t rgb)=0;
    // 实际注册查找->参数frame/X->SEB包围盒/Transform->原舍入链。
    // 只有真实查不到key时宽0；缺资源提供者不能假装一次成功的空注册查找。
    virtual std::int32_t pictograph_layout_width(const std::vector<std::u16string> &,
                                                const SteamLayoutFontRef &)=0;
};

enum class SteamLayoutBlockKind { ordinary, pictograph, button };
struct SteamLayoutBlock {
    SteamLayoutBlockKind kind{SteamLayoutBlockKind::ordinary};
    SteamLayoutFontRef font;
    std::u16string text;
    std::vector<std::u16string> parameters;
    float x{},y{},width{};
    std::int32_t color{},blink{},border_color{-1},start{-1},length{};
    bool bold{};
};
struct SteamTextLayoutInput {
    std::u16string text;
    SteamLayoutFontRef font;
    float width{-1},height{-1},scale_ratio{1},previous_text_width{};
    std::int32_t color{},anchor{0x11},line_space{},request_font_size{-1};
    bool multi_line{true},localize_font_enable{true},auto_scale{},format{};
};
struct SteamTextLayoutResult {
    std::vector<SteamLayoutBlock> blocks;
    float text_width{};
    std::int32_t request_font_size{-1};
    bool over_area{};
};

// 完整_make正常域消费者：入口/14TAG/整词和组合/行提交/源范围重建/正常清理。
// 不代替上游SetText/autoScale循环，不改渐显length/pos/时钟，也不实际绘图。
// 文本<=4096 UTF-16单元，块<=8192，有限坐标/测宽绝对值<=2^24；字号0..65536。
// FF零除、越界源索引、不前进及预算超限拒绝，不夹限或补字宽。输入保持不变。
// 返回候选才可安装；后端副作用不可回滚。异常/维护拒绝时尽力执行已取得的清理，
// 属维护资源策略，不认证原异常前缀有事务性；后端抛错原样传播。
std::optional<SteamTextLayoutResult> steam_make_text_blocks(
    const SteamTextLayoutInput &,SteamTextLayoutBackend &);
} // namespace dungeon_village_prototype
