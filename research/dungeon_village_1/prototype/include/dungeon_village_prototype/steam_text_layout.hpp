#pragma once

#include "dungeon_village_prototype/steam_text_reveal.hpp"

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

struct SteamTextSetInput {
    SteamTextLayoutInput layout; // 已完成公开Replace／TranslateConsoleText的真实text_。
    SteamTextRevealState reveal;
    std::optional<std::int32_t> request_font_scale; // 仅!adjust且多行时消费，不能猜平台比例。
    bool adjust_font_size{};
};
struct SteamTextSetResult {
    SteamTextLayoutResult layout;
    SteamLayoutFontRef font; // 首排后复制的备份；最终块仍持有末次重排的冻结Font。
    SteamTextRevealState reveal;
    float height{-1}; // 多行自动适配可能扩大框高；写回后不再重排。
    bool adjust_font_size{};
};
// 私有SetText在真实text_准备后的字号准备→排块/自适应→长度→Font换回。
// layout.font必须是宿主本次私有呈现候选：会原序修改该Font对象，旧别名不回滚；
// 不额外复制候选来伪装源调用。仅成功结果可安装，已执行后端操作无事务保证。
// relayout_budget是维护调用预算（1..65536，含首排），不是原版循环上限。
// 失败时尽力换回已经取得的备份，不重试失败清理；首排前失败可能没有备份。
// 不执行公开Replace/平台文案转换、不创建Owner/存档，也不代替实例Draw重排。
std::optional<SteamTextSetResult> steam_set_prepared_text(
    const SteamTextSetInput &,SteamTextLayoutBackend &,std::uint32_t relayout_budget=65536);

struct SteamTextReplaceWord {
    std::optional<std::u16string> old_word,new_word;
};
enum class SteamTextComparison { current_culture,current_culture_ignore_case };
struct SteamConsoleTextResult {
    bool translated{}; // 私有SetText忽略该返回值，实际消费out_text。
    std::optional<std::u16string> out_text;
};
// 固定Steam String.GetHashCode RVA8E8770：双5381累加器交替取UTF-16，遇NUL止，
// 低32位回卷后按有符号返回；不外推其它.NET/Unity版本，也不是文件身份摘要。
std::int32_t steam_text_string_hash(const std::u16string &);
class SteamTextInputBackend {
public:
    virtual ~SteamTextInputBackend()=default;
    virtual std::int32_t hash_code(const std::u16string &text) {return steam_text_string_hash(text);}
    virtual SteamConsoleTextResult translate_console_text(const std::u16string &)=0;
    virtual bool contains(const std::u16string &,const std::u16string &,SteamTextComparison)=0;
    virtual bool starts_with(const std::u16string &,const std::u16string &,SteamTextComparison)=0;
    virtual bool ends_with(const std::u16string &,const std::u16string &,SteamTextComparison)=0;
};
struct SteamPreparedText {
    std::u16string text;
    std::int32_t hash{}; // Replace结果的hash，不是console/ff最终串的hash。
};
// 公开null归空→当前字典枚举原序的ordinal替换→真实hash→console-out→ff重组。
// words必须是当前替换表的实际稳定枚举，不得因未找到writer就假设为空；
// 本接口不伪造原字典并发修改行为或安装共享text_/hash_，成功后交prepared SetText。
// 各串<=4096 UTF-16单元、表<=4096项、替换<=65536次为维护预算；
// helper原null键/值不替换，实际消费的空旧词会不前进，明确拒绝而非返回假成功。
// console的bool忽略，null out明确拒绝；后端异常原样传播，已执行后端副作用不回滚。
std::optional<SteamPreparedText> steam_prepare_text(
    const std::optional<std::u16string> &,const std::vector<SteamTextReplaceWord> &words,
    SteamTextInputBackend &);

struct SteamTextDrawReflow {
    float scale_ratio{};
    std::optional<SteamTextLayoutResult> replacement; // 空表示保持原块，不是清空块数组。
};
// 实例Draw的比例重排门。相等（含正负零）不调用后端，保留旧比例；
// 不等时先用Graphics比例准备一次_make，空串也保留原前宽/request资格。
// 只返回实际替换的块等字段；原渐显长度/钟、adjust、框高及draw计数由调用者保留。
// 实际更换的新比例NaN/无限或维护预算外明确拒绝，原版先改比例再排、异常不保证回滚。
// 不是完整实例Draw/GPU，也不调用SetText的缩字循环或末尾复位。
std::optional<SteamTextDrawReflow> steam_reflow_text_for_draw(
    const SteamTextLayoutInput &current,float graphics_scale,SteamTextLayoutBackend &);
} // namespace dungeon_village_prototype
