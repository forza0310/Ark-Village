// 完整排块的功能短测。文化比较、字体与非加法字宽均为具名夹具，不是平台字形认证。
#include "dungeon_village_prototype/steam_text_layout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace dungeon_village_prototype;
namespace {
struct Checks {
    int count{};
    void operator()(bool ok,const char *message) {
        ++count;
        if(!ok)throw std::runtime_error(std::string("Steam完整排块：")+message);
    }
};
struct FixtureFont final:SteamLayoutFont {
    int identity{},used{12},request{12},font_style{};
    std::function<void(const FixtureFont &)> used_probe;
    std::int32_t used_size() const override {if(used_probe)used_probe(*this);return used;}
    std::int32_t request_size() const override {return request;}
    std::int32_t style() const override {return font_style;}
};
struct LayoutBackendFailure {std::string operation;};
class FixtureBackend final:public SteamTextLayoutBackend {
public:
    std::vector<std::string> calls;
    std::vector<std::string> *shared_trace{}; // 外层Draw与排块共享一次原序轨迹，不新建持久记录器。
    std::vector<std::pair<int,int>> copies;
    std::vector<std::pair<int,int>> outside_copies; // SetText首排后备份，不能混同样本内工作/块复制。
    struct SizeCall {int identity{},request{};bool preset{};};
    std::vector<SizeCall> sizes;
    std::map<int,int> explicit_used; // 具名setter oracle：请求值与后继used可不同。
    std::vector<std::pair<int,std::u16string>> widths;
    std::vector<SteamLayoutFontRef> fonts,installed;
    std::optional<std::u16string> translation;
    std::optional<std::u16string> language;
    std::map<std::u16string,float> width_overrides; // 明确的组合测宽夹具，不模拟Unicode字形算法。
    std::string fail_operation;
    int fail_occurrence{1};
    bool fail_cleanup{},fail_install{},japanese{true},localize{},disabled{},chanakya{true};
    int samples{},auto_depth{},localize_depth{},next_identity{2};
    static std::shared_ptr<FixtureFont> font(const SteamLayoutFontRef &value) {
        const auto out=std::dynamic_pointer_cast<FixtureFont>(value);
        if(!out)throw std::runtime_error("排块夹具收到非夹具Font");
        return out;
    }
    void begin_sample() override {call("begin");++samples;}
    void end_sample() override {call("end");--samples;}
    void push_auto_text_layout(bool value) override {call(value?"autoPush1":"autoPush0");++auto_depth;}
    void pop_auto_text_layout() override {call("autoPop");--auto_depth;}
    bool japanese_style_text_layout() override {call("japanese");return japanese;}
    std::u16string language_code() override {call("language");return language?*language:(japanese?u"ja":u"en");}
    void push_localize(bool value) override {call(value?"localPush1":"localPush0");++localize_depth;}
    void pop_localize() override {
        call("localPop");--localize_depth;
        if(fail_cleanup)throw LayoutBackendFailure{"cleanup"};
    }
    bool needs_localize(const SteamLayoutFontRef &) override {
        call("needsLocalize");return localize&&localize_depth==0;
    }
    SteamLayoutFontRef copy_font(const SteamLayoutFontRef &from) override {
        call("copy");if(samples==0)call("outsideCopy");
        auto out=std::make_shared<FixtureFont>(*font(from));
        out->identity=next_identity++;copies.emplace_back(font(from)->identity,out->identity);
        if(samples==0)outside_copies.emplace_back(font(from)->identity,out->identity);
        fonts.push_back(out);return out;
    }
    std::int32_t localized_request(const SteamLayoutFontRef &) override {call("localizedRequest");return 10;}
    void install_layout_font(const SteamLayoutFontRef &value) override {
        call("install");if(fail_install)throw LayoutBackendFailure{"install"};installed.push_back(value);
    }
    void set_size(const SteamLayoutFontRef &value,std::int32_t request,bool preset) override {
        call("setSize");auto out=font(value);out->request=request;
        sizes.push_back({out->identity,request,preset});
        if(preset&&request>=0&&request<3)out->used=std::array<int,3>{12,24,30}[request];
        else {
            const auto observed=explicit_used.find(request);
            out->used=observed==explicit_used.end()?request:observed->second;
        }
    }
    void set_style(const SteamLayoutFontRef &value,std::int32_t style) override {
        call("setStyle");font(value)->font_style=style;
    }
    void adjust_font_size(const SteamLayoutFontRef &value,float ratio) override {
        call("adjust");font(value)->used=static_cast<int>(static_cast<float>(font(value)->used)*ratio);
    }
    void set_layout_disabled(bool value) override {call(value?"disabled1":"disabled0");disabled=value;}
    std::u16string translate_text(const std::u16string &text,bool) override {
        call("translate");return translation?*translation:text;
    }
    std::u16string unicode_to_chanakya(const std::u16string &text) override {call("unicode");return text;}
    void set_chanakya_enabled(bool value) override {call(value?"chanakya1":"chanakya0");chanakya=value;}
    float string_width_f(const SteamLayoutFontRef &value,const std::u16string &text) override {
        call("width");widths.emplace_back(font(value)->identity,text);
        const auto explicit_width=width_overrides.find(text);
        if(explicit_width!=width_overrides.end())return explicit_width->second;
        // 明确的非加法oracle：字号12时A/B各6，AB整串9。其余每单元半字号仅为夹具。
        if(text==u"AB")return static_cast<float>(font(value)->used)*0.75F;
        return static_cast<float>(text.size())*static_cast<float>(font(value)->used)*0.5F;
    }
    int compare_to(const std::u16string &a,const std::u16string &b) override {
        call("compare");return a.compare(b);
    }
    bool starts_with(const std::u16string &a,const std::u16string &b) override {
        call("starts");return a.size()>=b.size()&&a.compare(0,b.size(),b)==0;
    }
    bool trim_is_empty(char16_t value) override {call("trim");return value<=0x20;}
    std::optional<std::int32_t> try_parse_integer(const std::u16string &text,int radix) override {
        call(radix==16?"tryHex":"tryDecimal");
        try {return parse_fixture(text,radix);} catch(const std::invalid_argument &) {return {};}
        catch(const std::out_of_range &) {return {};}
    }
    std::int32_t parse_decimal(const std::u16string &text) override {
        call("decimal");return parse_fixture(text,10);
    }
    std::vector<std::u16string> split_parameters(const std::u16string &text) override {
        call("split");
        if(text.empty())return {};
        std::vector<std::u16string> result;
        std::size_t begin=0;
        do {
            auto end=text.find(u',',begin);if(end==std::u16string::npos)end=text.size();
            auto left=begin,right=end;
            while(left<right&&text[left]<=0x20)++left;
            while(right>left&&text[right-1]<=0x20)--right;
            result.push_back(text.substr(left,right-left));
            if(end==text.size())break;
            begin=end+1;
        } while(true);
        return result;
    }
    std::int32_t rgb_color(std::int32_t rgb) override {call("rgb");return rgb;}
    std::int32_t pictograph_layout_width(const std::vector<std::u16string> &parameters,
                                        const SteamLayoutFontRef &) override {
        call("pictograph");
        if(parameters!=std::vector<std::u16string>{u"fixture"})
            throw std::runtime_error("排块夹具图标参数不符");
        return 17;
    }
    bool cleaned() const {return samples==0&&auto_depth==0&&localize_depth==0&&!disabled&&chanakya;}
private:
    void call(const char *name) {
        calls.emplace_back(name);
        if(shared_trace)shared_trace->emplace_back(name);
        if(fail_operation==name&&--fail_occurrence==0) {
            fail_operation.clear();throw LayoutBackendFailure{name};
        }
    }
    // 限定ASCII样本的测试文化适配；不作为原Int32.Parse/CompareTo的生产替代。
    static std::int32_t parse_fixture(const std::u16string &text,int radix) {
        std::string ascii;
        for(const auto c:text) {
            if(c>0x7f)throw std::invalid_argument("fixture non-ASCII integer");
            ascii.push_back(static_cast<char>(c));
        }
        std::size_t used=0;const auto value=std::stoll(ascii,&used,radix);
        if(used!=ascii.size())throw std::invalid_argument("fixture integer suffix");
        if(value<std::numeric_limits<std::int32_t>::min()||value>std::numeric_limits<std::int32_t>::max())
            throw std::out_of_range("fixture integer range");
        return static_cast<std::int32_t>(value);
    }
};
SteamTextLayoutInput input(std::u16string text) {
    auto f=std::make_shared<FixtureFont>();f->identity=1;
    SteamTextLayoutInput out;out.text=std::move(text);out.font=f;out.color=0x123456;return out;
}
std::size_t position(const std::vector<std::string> &calls,const char *operation) {
    const auto found=std::find(calls.begin(),calls.end(),operation);
    return static_cast<std::size_t>(found-calls.begin());
}
void ordinary_and_source_ranges(Checks &check) {
    auto in=input(u"AB");FixtureBackend backend;
    const auto p=steam_make_text_blocks(in,backend);
    check(p&&p->blocks.size()==1&&p->blocks[0].text==u"AB"&&p->blocks[0].width==9.F&&
              p->text_width==9.F&&p->blocks[0].start==0&&p->blocks[0].length==2&&
              backend.widths.size()==4&&backend.widths[0].second==u"A"&&
              backend.widths[1].second==u"A"&&backend.widths[2].second==u"AB"&&
              backend.widths[3].second==u"AB"&&
              position(backend.calls,"translate")<position(backend.calls,"unicode")&&
              position(backend.calls,"unicode")<position(backend.calls,"width")&&backend.cleaned()&&
              in.text==u"AB"&&FixtureBackend::font(in.font)->used==12,
          "实际普通测宽按预测/追加各读一次整串，不按单字相加；翻译先于转换及测宽，输入与后端清理保持");
    struct RangeCase {std::u16string source,expected;float width;};
    for(const auto &c:std::array<RangeCase,3>{{{u"A\rB",u"A\rB",9.F},
                                              {u"\rA",u"A",6.F},{u"A\r",u"A",6.F}}}) {
        FixtureBackend b;const auto result=steam_make_text_blocks(input(c.source),b);
        check(result&&result->blocks.size()==1&&result->blocks[0].text==c.expected&&
                  result->blocks[0].width==c.width&&b.cleaned(),
              "CR不追加临时串，最终源范围仅保留被后继接受单元包住的CR，不重测最终字串");
    }
    auto narrow=input(u"A");narrow.width=3.F;FixtureBackend nb;
    const auto n=steam_make_text_blocks(narrow,nb);
    check(n&&n->blocks.size()==1&&n->blocks[0].y==12.F&&n->blocks[0].width==6.F,
          "超窄首字符先推进空行再接受超宽字符，不循环试fit或删除字符");
    auto lf=input(u"A\nB");lf.multi_line=false;FixtureBackend lb;
    const auto lines=steam_make_text_blocks(lf,lb);
    check(lines&&lines->blocks.size()==2&&lines->blocks[0].text==u"A"&&
              lines->blocks[1].text==u"B"&&lines->blocks[1].y==12.F,
          "LF独立换行，不被multiLine=false或未定宽豁免，也不进入可见串");
    FixtureBackend trailing_backend;
    const auto trailing=steam_make_text_blocks(input(u"A\n"),trailing_backend);
    check(trailing&&trailing->blocks.size()==1&&trailing->blocks[0].text==u"A",
          "末尾LF不补普通空块或第二个可见空行");
    FixtureBackend unknown_backend;
    const auto unknown=steam_make_text_blocks(input(u"<zz=1>"),unknown_backend);
    check(unknown&&unknown->blocks.size()==1&&unknown->blocks[0].text==u"<zz=1>"&&
              unknown->blocks[0].width==36.F,
          "未知标签仍为普通字面，不吞括号范围或强行套HTML解析");
    auto empty=input(u"");empty.previous_text_width=42.F;FixtureBackend eb;
    const auto e=steam_make_text_blocks(empty,eb);
    check(e&&e->blocks.empty()&&e->text_width==42.F&&eb.cleaned(),
          "原始空串早退保留旧textWidth，不能当成翻译后空串重排路径");
}
void fonts_and_tags(Checks &check) {
    auto localized=input(u"A");FixtureBackend l;l.localize=true;
    const auto lp=steam_make_text_blocks(localized,l);
    check(lp&&lp->blocks.size()==1&&l.copies.size()==3&&
              l.copies[0].first==1&&l.copies[1].first==l.copies[0].second&&
              l.copies[2].first==l.copies[1].second&&
              FixtureBackend::font(lp->blocks[0].font)->identity==l.copies[2].second&&
              FixtureBackend::font(lp->blocks[0].font)->used==10&&l.installed.size()==2&&
              l.installed.front()!=localized.font&&l.installed.back()==localized.font&&
              FixtureBackend::font(localized.font)->used==12&&l.cleaned(),
          "入场/本地化/工作/逐块四层Font身份分开，正常尾恢复入场字体并收齐本地化栈");
    const auto frozen=lp->blocks[0].font;
    FixtureBackend::font(l.fonts[1])->used=99;
    check(FixtureBackend::font(frozen)->used==10,
          "已交块Font是冻结副本，后来修改工作Font不得倒改块字号");
    FixtureBackend sizes;const auto sp=steam_make_text_blocks(input(u"A<si=24>B</si>C"),sizes);
    check(sp&&sp->blocks.size()==2&&sp->blocks[0].text==u"A"&&sp->blocks[1].text==u"B"&&
              FixtureBackend::font(sp->blocks[0].font)->used==12&&
              FixtureBackend::font(sp->blocks[1].font)->used==24&&
              sp->blocks[0].y==6.F&&sp->blocks[1].y==0.F&&sp->text_width==18.F,
          "关闭si保留旧B指针，当前12字号测BC宽12减旧B宽12为零，C不建块/追加，行高仍24");
    FixtureBackend restored;restored.width_overrides={{u"BC",18.F}};
    const auto rp=steam_make_text_blocks(input(u"A<si=24>B</si>C"),restored);
    check(rp&&rp->blocks.size()==3&&rp->blocks[2].text==u"C"&&
              FixtureBackend::font(rp->blocks[0].font)->used==12&&
              FixtureBackend::font(rp->blocks[1].font)->used==24&&
              FixtureBackend::font(rp->blocks[2].font)->used==12&&
              rp->blocks[0].y==6.F&&rp->blocks[1].y==0.F&&rp->blocks[2].y==6.F,
          "具名BC宽18夹具使关闭si后的D为正才创建C；恢复基字号12且旧块字体及行高冻结");
    auto switched=input(u"A<si=30>B");switched.width=22.F;switched.anchor=2;FixtureBackend sb;
    const auto wrap=steam_make_text_blocks(switched,sb);
    check(wrap&&wrap->blocks.size()==2&&wrap->blocks[0].x==8.F&&wrap->blocks[0].y==9.F&&
              wrap->blocks[1].x==3.5F&&wrap->blocks[1].y==30.F&&wrap->blocks[1].width==15.F&&
              wrap->text_width==15.F&&sb.widths.size()==4&&sb.widths[2].second==u"AB"&&
              sb.widths[3].second==u"B",
          "待新块仍用旧块串预测D16.5触发断行，旧行按旧宽6居中，新块再测B宽15且不重试fit");
    FixtureBackend tags;
    const auto tp=steam_make_text_blocks(input(u"<co=FF0000><st=1><bl=2><bo=00FF00>A</bo></bl></st></co>B"),tags);
    check(tp&&tp->blocks.size()==2&&tp->blocks[0].text==u"A"&&tp->blocks[1].text==u"B"&&
              tp->blocks[0].color==0xff0000&&tp->blocks[0].blink==2&&tp->blocks[0].bold&&
              tp->blocks[0].border_color==0x00ff00&&FixtureBackend::font(tp->blocks[0].font)->font_style==1&&
              tp->blocks[1].color==0x123456&&tp->blocks[1].blink==0&&!tp->blocks[1].bold&&
              FixtureBackend::font(tp->blocks[1].font)->font_style==0&&tags.cleaned(),
          "多标签分别冻结色/style/blink/描边，关闭回基值而非嵌套栈，连续标签不额外产空块");
    auto aligned=input(u"<po=cm>AB");aligned.width=40.F;aligned.height=40.F;FixtureBackend ab;
    const auto ap=steam_make_text_blocks(aligned,ab);
    check(ap&&ap->blocks.size()==1&&ap->blocks[0].x==15.5F&&ap->blocks[0].y==14.F,
          "po按真实非加法末行宽9居中，再对全部块按总高12纵向居中，不重复加行距");
    for(const bool picture:std::array<bool,2>{false,true}) {
        FixtureBackend b;const auto p=steam_make_text_blocks(input(picture?u"<pic=fixture>":u"<btn=1,, 2, >"),b);
        check(p&&p->blocks.size()==2&&p->blocks[0].kind==(picture?SteamLayoutBlockKind::pictograph:
                  SteamLayoutBlockKind::button)&&p->blocks[0].width==(picture?17.F:14.F)&&
                  p->blocks[1].kind==SteamLayoutBlockKind::ordinary&&p->blocks[1].text.empty()&&
                  p->blocks[1].width==0.F&&p->blocks[1].x==(picture?17.F:14.F)&&
                  (picture||p->blocks[0].parameters==std::vector<std::u16string>{u"1",u"",u"2",u""}),
              "pic/btn用具名宿主宽和保留空片的真实参数分隔合同，随后实际建立普通尾空块");
    }
}
void combination_words_and_empty(Checks &check) {
    const std::u16string one=u"\u00c0",pair=u"\u00c0\u00c1",triple=u"\u00c0\u00c1\u00c2";
    for(const auto &language:std::array<std::u16string,3>{u"hi",u"ja",u"HI"}) {
        FixtureBackend b;b.language=language;b.japanese=language==u"ja";
        b.width_overrides={{one,6.F},{pair,6.F}};
        const auto p=steam_make_text_blocks(input(pair),b);
        const bool combines=language==u"hi";
        check(p&&p->blocks.size()==1&&p->blocks[0].text==(combines?pair:one)&&
                  p->blocks[0].length==(combines?2:1)&&p->blocks[0].width==6.F&&
                  b.widths.size()==(combines?4U:3U)&&b.widths[2].second==pair&&
                  (!combines||b.widths[3].second==one)&&b.cleaned(),
              "hi精确语言码将阈值降128，真实pair/前单元等宽才扩范围；其他语言和HI不按代理对或大小写猜组合");
    }
    FixtureBackend three;three.language=u"hi";three.japanese=false;
    three.width_overrides={{one,6.F},{pair,6.F},{triple,6.F}};
    const auto t=steam_make_text_blocks(input(triple),three);
    check(t&&t->blocks.size()==1&&t->blocks[0].text==triple&&t->blocks[0].length==3&&
              t->blocks[0].width==6.F&&three.widths.size()==6&&
              three.widths[2].second==pair&&three.widths[3].second==one&&
              three.widths[4].second==triple&&three.widths[5].second==one,
          "三单元续体另测triple/前单元后跳过第三次扫描，最终范围含三单元而布局宽仍6");
    auto words=input(u"A BC");words.width=18.F;FixtureBackend wb;wb.japanese=false;
    const auto w=steam_make_text_blocks(words,wb);
    check(w&&w->blocks.size()==2&&w->blocks[0].text==u"A "&&w->blocks[0].width==12.F&&
              w->blocks[1].text==u"BC"&&w->blocks[1].width==12.F&&w->blocks[1].y==12.F,
          "已有非空前缀时整词前瞻将BC整体移行，B本身恰能放入旧行也不得先追加");
    auto first_word=input(u"BC D");first_word.width=6.F;FixtureBackend fw;fw.japanese=false;
    const auto f=steam_make_text_blocks(first_word,fw);
    check(f&&f->blocks.size()==3&&f->blocks[0].text==u"B"&&f->blocks[0].y==0.F&&
              f->blocks[1].text==u"C"&&f->blocks[1].y==12.F&&
              f->blocks[2].text==u"D"&&f->blocks[2].y==24.F,
          "首词超宽且无可移前缀时关闭词前瞻，退化逐单元拆行，不给首字新增空行");
    const auto candidate=std::u16string(u"<")+std::u16string(33,u'x')+u">";
    for(const bool prefix:std::array<bool,2>{false,true}) {
        const auto raw=(prefix?std::u16string(u"A"):std::u16string{})+candidate;
        FixtureBackend b;const auto p=steam_make_text_blocks(input(raw),b);
        const auto expected=prefix?raw:candidate.substr(1);
        check(p&&p->blocks.size()==1&&p->blocks[0].text==expected&&
                  p->blocks[0].start==(prefix?0:1)&&p->blocks[0].width==(prefix?210.F:204.F),
              "超过32的候选只跳起始括号，已有块的后续源范围可带回括号但不重测最终串宽");
    }
    auto untranslated=input(u"A");untranslated.previous_text_width=42.F;
    FixtureBackend empty;empty.translation=u"";empty.localize=true;
    const auto e=steam_make_text_blocks(untranslated,empty);
    check(e&&e->blocks.empty()&&e->text_width==0.F&&e->request_font_size==10&&
              untranslated.request_font_size==-1&&FixtureBackend::font(untranslated.font)->used==12&&
              position(empty.calls,"translate")<position(empty.calls,"unicode")&&
              std::count(empty.calls.begin(),empty.calls.end(),"width")==0&&empty.cleaned(),
          "非空原串翻译为空仍走本地化/工作字体与入口清理，清宽为0并返回新requestFontSize10，不能走原始空串早退");
}
void boundaries_and_cleanup(Checks &check) {
    auto area=input(u"A");area.width=100.F;area.height=-1.F;FixtureBackend b;
    const auto a=steam_make_text_blocks(area,b);
    check(a&&a->over_area,"overArea在有限宽且非autoScale时仍比较高度负一，不当作高度豁免");
    area.auto_scale=true;FixtureBackend auto_backend;
    const auto scaled=steam_make_text_blocks(area,auto_backend);
    check(scaled&&!scaled->over_area,"autoScale独立跳过overArea检查，不改变已排块作为凑通过");
    auto ff=input(u"A<ff>\nB");ff.height=24.F;ff.line_space=-12;FixtureBackend fb;
    const auto failed=steam_make_text_blocks(ff,fb);
    check(!failed&&fb.cleaned()&&ff.text==u"A<ff>\nB"&&ff.line_space==-12&&
              FixtureBackend::font(ff.font)->used==12,
          "FF行高加行距为零显式拒绝并清理，不补演示页高或修改输入");
    auto indices=input(u" ");indices.width=6.F;FixtureBackend ib;
    ib.japanese=false;ib.translation=u"A BC";
    const auto invalid_indices=steam_make_text_blocks(indices,ib);
    check(!invalid_indices&&ib.cleaned()&&indices.text==u" ",
          "整词前瞻混用原串/译串索引时越界拒绝，不伪造索引映射或跳过检查");
    for(const bool cleanup_failure:std::array<bool,2>{false,true}) {
        auto original=input(u"A");original.localize_font_enable=false;
        FixtureBackend failing;failing.fail_operation="width";failing.fail_cleanup=cleanup_failure;
        bool original_error=false;
        try {(void)steam_make_text_blocks(original,failing);}
        catch(const LayoutBackendFailure &e) {original_error=e.operation=="width";}
        check(original_error&&std::count(failing.calls.begin(),failing.calls.end(),"width")==1&&
                  position(failing.calls,"localPop")<failing.calls.size()&&
                  position(failing.calls,"autoPop")<failing.calls.size()&&
                  !failing.calls.empty()&&failing.calls.back()=="end"&&
                  original.text==u"A"&&FixtureBackend::font(original.font)->used==12&&
                  (cleanup_failure||failing.cleaned()),
              "后端测宽异常只执行一次，清理故障仍尽力退休其余已取得资源并传播首错，不宣称宿主副作用回滚");
    }
    auto original=input(u"A");FixtureBackend restore;restore.fail_operation="chanakya1";
    bool restore_error=false;
    try {(void)steam_make_text_blocks(original,restore);}
    catch(const LayoutBackendFailure &e) {restore_error=e.operation=="chanakya1";}
    check(restore_error&&std::count(restore.calls.begin(),restore.calls.end(),"chanakya1")==1&&
              std::count(restore.calls.begin(),restore.calls.end(),"end")==1&&
              restore.calls.back()=="end"&&restore.samples==0&&restore.auto_depth==0&&
              original.text==u"A"&&FixtureBackend::font(original.font)->used==12,
          "末尾Chanakya恢复抛错仍恰执行一次EndSample，不重放失败恢复或用宿主回滚掩盖错误");
}

// SetText外层只消费宿主已准备的text_；各字号/宽/调用次数为独立夹具oracle。
void prepared_text_outer(Checks &check) {
    for(const auto c:std::array<std::array<int,2>,2>{{{150,5},{50,48}}}) {
        SteamTextSetInput in;in.layout=input(u"A");in.layout.request_font_size=99;
        in.request_font_scale=c[0];
        FixtureBackend b;const auto p=steam_set_prepared_text(in,b);
        check(p&&p->adjust_font_size&&p->layout.request_font_size==c[1]&&
                  FixtureBackend::font(in.layout.font)->used==c[1]&&p->font!=in.layout.font&&
                  FixtureBackend::font(p->font)->used==c[1]&&b.sizes.size()==1&&
                  b.sizes[0].identity==1&&b.sizes[0].request==c[1]&&!b.sizes[0].preset&&
                  b.outside_copies.size()==1&&b.outside_copies[0].first==1&&
                  position(b.calls,"setSize")<position(b.calls,"begin")&&
                  position(b.calls,"end")<position(b.calls,"outsideCopy")&&
                  b.installed==std::vector<SteamLayoutFontRef>{p->font}&&b.cleaned(),
              "两次乘100/比例分别截零：150%得5、50%得48；首排之后才备份，非autoScale也换回该新引用");
    }
    struct ScaleCase {bool adjust,multi;std::optional<std::int32_t> scale;bool accepted;};
    for(const auto &c:std::array<ScaleCase,5>{{{true,true,{},true},{false,false,0,true},
             {true,false,0,true},{false,true,{},false},{false,true,0,false}}}) {
        SteamTextSetInput in;in.layout=input(u"A");in.layout.multi_line=c.multi;
        in.adjust_font_size=c.adjust;in.request_font_scale=c.scale;
        FixtureBackend b;const auto p=steam_set_prepared_text(in,b);
        check(bool(p)==c.accepted&&b.sizes.empty()&&FixtureBackend::font(in.layout.font)->used==12&&
                  (c.accepted?b.outside_copies.size()==1:b.calls.empty())&&b.cleaned(),
              "比例只在未adjust且多行时消费；未消费的缺值/0不拒绝，实际零除先拒绝且不发setter");
    }
    struct SingleCase {float height,width;bool auto_scale;};
    for(const auto c:std::array<SingleCase,5>{{{-1.F,3.F,true},{0.F,3.F,true},
             {1.F,0.F,true},{1.F,-1.F,true},{1.F,3.F,false}}}) {
        SteamTextSetInput in;in.layout=input(u"A");in.layout.multi_line=false;
        in.layout.auto_scale=c.auto_scale;in.layout.height=c.height;in.layout.width=c.width;
        in.adjust_font_size=true;
        FixtureBackend b;const auto p=steam_set_prepared_text(in,b);
        check(p&&b.sizes.empty()&&p->height==c.height&&p->layout.text_width==6.F&&
                  p->layout.request_font_size==12&&b.outside_copies.size()==1&&b.cleaned(),
              "单行仍须autoScale且高/宽均正才缩字，其余保留首排宽和框高而不省略备份");
    }
    SteamTextSetInput empty;empty.layout=input(u"");empty.layout.previous_text_width=42.F;
    empty.layout.request_font_size=99;empty.layout.multi_line=false;
    empty.layout.auto_scale=true;empty.layout.height=1.F;empty.layout.width=1.F;
    FixtureBackend::font(empty.layout.font)->used=3;
    FixtureBackend eb;const auto e=steam_set_prepared_text(empty,eb,3);
    check(e&&e->layout.blocks.empty()&&e->layout.text_width==42.F&&e->layout.request_font_size==-1&&
              e->reveal.length==0&&FixtureBackend::font(empty.layout.font)->used==1&&
              FixtureBackend::font(e->font)->used==3&&eb.sizes.size()==2&&
              eb.sizes[0].request==2&&eb.sizes[1].request==1&&
              std::count(eb.calls.begin(),eb.calls.end(),"begin")==0&&eb.outside_copies.size()==1&&eb.cleaned(),
          "空串每次早退保留旧宽42仍触发缩字；request首次清-1且保持，旧Font降至1而返回首排后字号3备份");
    SteamTextSetInput stop;stop.layout=input(u"A");stop.layout.multi_line=false;
    stop.layout.auto_scale=true;stop.layout.height=1.F;stop.layout.width=0.5F;
    stop.adjust_font_size=true;FixtureBackend::font(stop.layout.font)->used=2;
    FixtureBackend sb;sb.explicit_used.emplace(1,7); // 请求1的后继字号7，独立检测停机条件。
    const auto s=steam_set_prepared_text(stop,sb,2);
    check(s&&s->layout.text_width==3.5F&&s->layout.request_font_size==2&&
              FixtureBackend::font(stop.layout.font)->used==7&&FixtureBackend::font(s->font)->used==2&&
              FixtureBackend::font(s->layout.blocks.at(0).font)->used==7&&sb.sizes.size()==1&&
              sb.sizes[0].request==1&&!sb.sizes[0].preset&&
              std::count(sb.calls.begin(),sb.calls.end(),"begin")==2&&sb.cleaned(),
          "退出依据减一实参恰1而非后继used=7或宽已适配；首排request2沿用，最终块保留末次Font而旧别名不回滚");
    struct MultiCase {float height,expected;int reads,passes;};
    for(const auto c:std::array<MultiCase,3>{{{11.F,12.F,3,1},{12.F,12.F,2,1},{10.9F,11.F,4,2}}}) {
        SteamTextSetInput in;in.layout=input(u"A");in.layout.auto_scale=true;
        in.layout.height=c.height;in.adjust_font_size=true;
        FixtureBackend b;std::vector<int> height_fonts;
        FixtureBackend::font(in.layout.font)->used_probe=[&](const FixtureFont &font) {
            if(b.samples==0&&font.identity!=1) {
                height_fonts.push_back(font.identity);b.calls.emplace_back("heightRead");
            }
        };
        const auto p=steam_set_prepared_text(in,b);
        check(p&&p->height==c.expected&&p->layout.request_font_size==12&&
                  height_fonts.size()==static_cast<std::size_t>(c.reads)&&
                  std::count(b.calls.begin(),b.calls.end(),"begin")==c.passes&&
                  position(b.calls,"outsideCopy")<position(b.calls,"heightRead")&&
                  position(b.calls,"heightRead")<position(b.calls,"install")&&
                  b.sizes.size()==static_cast<std::size_t>(c.passes-1)&&b.cleaned(),
              "多行先比框高+1；离阈值内不缩仍可扩框，扩高赋值再独立GetHeight，不缓存重复Font读取");
        if(c.passes==2)
            check(b.sizes[0].request==11&&height_fonts.front()!=height_fonts.back()&&
                      height_fonts[1]==height_fonts[2]&&height_fonts[2]==height_fonts[3]&&
                      FixtureBackend::font(in.layout.font)->used==11&&FixtureBackend::font(p->font)->used==12,
                  "多行首排高度读旧块一次，重排后新块再读三次；备份仍为首排字号12");
    }
    SteamTextSetInput mixed;mixed.layout=input(u"A\U0001F600<pic=fixture><btn=0>");
    mixed.adjust_font_size=true;mixed.reveal={77,123,25,6,4,9,5};
    FixtureBackend mb;const auto m=steam_set_prepared_text(mixed,mb);
    std::ostringstream mixed_diagnostic;
    mixed_diagnostic<<"最终长度按普通UTF16三单元+pic/btn各1，相邻特殊标签之间和末尾的普通空块均保留；"
                    <<"SetText仅清位置/两时间值，保留speed/frame/lastAppFrame；实际";
    if(m) {
        mixed_diagnostic<<"length="<<m->reveal.length<<", blocks="<<m->layout.blocks.size();
        for(std::size_t i=0;i<m->layout.blocks.size();++i) {
            const auto &block=m->layout.blocks[i];
            mixed_diagnostic<<" ["<<i<<": kind="<<static_cast<int>(block.kind)
                            <<", start="<<block.start<<", length="<<block.length<<", UTF16=";
            for(const auto unit:block.text)mixed_diagnostic<<std::hex<<static_cast<unsigned>(unit)<<' ';
            mixed_diagnostic<<std::dec<<']';
        }
    } else mixed_diagnostic<<"拒绝候选";
    check(m&&m->layout.blocks.size()==5&&m->layout.blocks[0].text==u"A\U0001F600"&&
              m->layout.blocks[1].kind==SteamLayoutBlockKind::pictograph&&
              m->layout.blocks[2].kind==SteamLayoutBlockKind::ordinary&&m->layout.blocks[2].text.empty()&&
              m->layout.blocks[3].kind==SteamLayoutBlockKind::button&&
              m->layout.blocks[4].kind==SteamLayoutBlockKind::ordinary&&m->layout.blocks[4].text.empty()&&
              m->reveal.length==5&&m->reveal.pos==0&&m->reveal.begin_time==0&&m->reveal.time_offset==0&&
              m->reveal.speed==25&&m->reveal.frame==4&&m->reveal.last_app_frame==5&&mb.cleaned(),
          mixed_diagnostic.str().c_str());
    SteamTextSetInput special;special.layout=input(u"<pic=fixture><btn=0>");
    special.layout.auto_scale=true;special.layout.height=1.F;special.adjust_font_size=true;
    FixtureBackend pb;int special_height_reads=0;
    FixtureBackend::font(special.layout.font)->used_probe=[&](const FixtureFont &font) {
        if(pb.samples==0&&font.identity!=1)++special_height_reads;
    };
    const auto pictures=steam_set_prepared_text(special,pb,1);
    check(pictures&&pictures->reveal.length==2&&pictures->height==1.F&&pb.sizes.empty()&&
              special_height_reads==0&&pb.cleaned(),
          "特殊块长度各1，但GetHeight仍按实际空text跳过；不以特殊块类型补高或读取其Font字号");
}

void prepared_text_failures(Checks &check) {
    struct FailureCase {const char *operation;int used;};
    for(const auto c:std::array<FailureCase,4>{{{"setSize",12},
             {"width",5},{"outsideCopy",5},{"install",5}}}) {
        SteamTextSetInput in;in.layout=input(u"A");in.layout.localize_font_enable=false;
        in.request_font_scale=150;
        FixtureBackend b;b.fail_operation=c.operation;
        bool exact=false;
        try {(void)steam_set_prepared_text(in,b,3);}
        catch(const LayoutBackendFailure &failure) {exact=failure.operation==c.operation;}
        check(exact&&FixtureBackend::font(in.layout.font)->used==c.used&&b.cleaned()&&
                  std::count(b.calls.begin(),b.calls.end(),c.operation)==1&&
                  (std::string(c.operation)!="install"||b.installed.empty()),
              "准备/首排/备份/末尾换回失败传播原错误，清理不重试失败调用，也不回滚已完成的原Font修改");
    }
    for(const bool cleanup_fails:std::array<bool,2>{false,true}) {
        SteamTextSetInput in;in.layout=input(u"A");in.layout.multi_line=false;
        in.layout.auto_scale=true;in.layout.height=1.F;in.layout.width=1.F;
        in.layout.localize_font_enable=false;in.adjust_font_size=true;
        FixtureBackend b;b.fail_operation="width";b.fail_occurrence=3;b.fail_install=cleanup_fails;
        bool exact=false;
        try {(void)steam_set_prepared_text(in,b,3);}
        catch(const LayoutBackendFailure &failure) {exact=failure.operation=="width";}
        check(exact&&b.cleaned()&&FixtureBackend::font(in.layout.font)->used==11&&
                  b.outside_copies.size()==1&&std::count(b.calls.begin(),b.calls.end(),"begin")==2&&
                  std::count(b.calls.begin(),b.calls.end(),"end")==2&&
                  std::count(b.calls.begin(),b.calls.end(),"install")==1&&
                  b.installed.size()==(cleanup_fails?0U:1U),
              "第二次排块测宽失败先清本轮栈，再尽力换回备份一次；换回也失败仍保留测宽首错与旧别名11");
    }
    SteamTextSetInput metric;metric.layout=input(u"A");metric.layout.auto_scale=true;
    metric.layout.height=10.F;metric.adjust_font_size=true;
    FixtureBackend hb;int reads=0;
    FixtureBackend::font(metric.layout.font)->used_probe=[&](const FixtureFont &font) {
        if(hb.samples==0&&font.identity!=1) {++reads;throw LayoutBackendFailure{"height"};}
    };
    bool height_error=false;
    try {(void)steam_set_prepared_text(metric,hb,3);}
    catch(const LayoutBackendFailure &failure) {height_error=failure.operation=="height";}
    check(height_error&&reads==1&&hb.cleaned()&&hb.outside_copies.size()==1&&hb.installed.size()==1,
          "首排已清理后读取块Font高度抛错，不重放GetHeight，仍尽力释放本次备份安装义务");
    for(const std::uint32_t budget:std::array<std::uint32_t,2>{1,2}) {
        SteamTextSetInput in;in.layout=input(u"A");in.layout.multi_line=false;
        in.layout.auto_scale=true;in.layout.height=1.F;in.layout.width=1.F;in.adjust_font_size=true;
        FixtureBackend b;if(budget==2)b.explicit_used.emplace(11,12); // 每次请求11仍used12，不保证自然停机。
        const auto p=steam_set_prepared_text(in,b,budget);
        check(!p&&b.cleaned()&&b.outside_copies.size()==1&&b.installed.size()==1&&
                  b.sizes.size()==budget&&std::count(b.calls.begin(),b.calls.end(),"begin")==static_cast<int>(budget)&&
                  FixtureBackend::font(in.layout.font)->used==(budget==1?11:12),
              "小重排预算含首排；耗尽前已执行的SetSize不回滚，停滞不靠夹字号或默认65536轮凑通过");
    }
    for(const std::uint32_t budget:std::array<std::uint32_t,2>{0,65537}) {
        SteamTextSetInput in;in.layout=input(u"A");FixtureBackend b;
        check(!steam_set_prepared_text(in,b,budget)&&b.calls.empty()&&
                  FixtureBackend::font(in.layout.font)->used==12,
              "非法预算在任何后端调用前拒绝，不建立备份或成功候选");
    }
}

// 只记录本次访问的真实候选key；返回值按具名用例供给，不复制被测剥标签/归一算法。
class ConsoleTextFixture final:public SteamConsoleTextBackend {
public:
    bool table{true},shown{true};
    std::int32_t count{1};
    std::vector<std::string> calls;
    std::vector<std::u16string> keys,position_queries;
    std::vector<SteamConsoleTextLookup> answers;
    std::string fail_operation;
    int fail_occurrence{1};
    bool has_table() override {call("table");return table;}
    std::int32_t table_count() override {call("count");return count;}
    bool joystick_is_shown() override {call("shown");return shown;}
    bool starts_with_position_tag(const std::u16string &text) override {
        call("position");position_queries.push_back(text);
        // 仅这些具名样本的区分大小写文化结果；不冒充任意.NET文化比较。
        return text.size()>=4&&text.compare(0,4,u"<po=")==0;
    }
    SteamConsoleTextLookup lookup(const std::u16string &key) override {
        call("lookup");keys.push_back(key);
        if(keys.size()>answers.size())throw std::runtime_error("Console夹具收到多余查询");
        return answers[keys.size()-1];
    }
private:
    void call(const char *name) {
        calls.emplace_back(name);
        if(fail_operation==name&&--fail_occurrence==0)throw LayoutBackendFailure{name};
    }
};

void console_text_consumer(Checks &check) {
    struct Gate {bool table;int count;bool shown;std::vector<std::string> calls;};
    for(const auto &c:std::vector<Gate>{{false,1,true,{"table"}},
             {true,0,true,{"table","count"}},
             {true,1,false,{"table","count","shown"}}}) {
        ConsoleTextFixture backend;backend.table=c.table;backend.count=c.count;backend.shown=c.shown;
        const auto p=steam_translate_console_text({},backend);
        check(p&&!p->translated&&!p->out_text&&backend.calls==c.calls&&backend.keys.empty(),
              "console资格原序短路，禁用路径可保留null，不读取后续表/Joystick/位置或key");
    }
    ConsoleTextFixture negative;negative.count=-1;
    check(!steam_translate_console_text(u"A",negative)&&negative.calls==std::vector<std::string>{"table","count"},
          "坏Count拒绝，不伪装成空表或继续Joystick查询");
    ConsoleTextFixture null_enabled;
    check(!steam_translate_console_text({},null_enabled)&&
              null_enabled.calls==std::vector<std::string>{"table","count","shown"},
          "启用路径null在位置查询前拒绝，区别于禁用路径原样null");
    struct Case {
        const char *name;
        std::u16string input;
        std::vector<std::u16string> keys;
        std::vector<SteamConsoleTextLookup> answers;
        bool translated;
        std::optional<std::u16string> output;
    };
    // key/输出直接按固定VA各分支登记，不调用维护替换/Regex方法生成期望。
    const std::vector<Case> cases{
        {"空key两查不Last",u"",{u"",u""},{{false,{}},{false,{}}},false,u""},
        {"空key真命中",u"",{u""},{{true,u"T"}},true,u"T"},
        {"合法prefix",u"<po=x>A</po>",{u"A"},{{true,u"T"}},true,u"<po=x>T"},
        {"非开头不Regex",u"A<po=x>B</po>",{u"A<po=x>B</po>"},{{true,u"T"}},true,u"T"},
        {"缺结束符不prefix",u"<po=x",{u"<po=x"},{{true,u"T"}},true,u"T"},
        {"大写标签不门控",u"<PO=x>A</PO>",{u"<PO=x>A</PO>"},{{true,u"T"}},true,u"T"},
        {"大写值不剥开标签",u"<po=X>A</po>",{u"<po=X>A"},{{true,u"T"}},true,u"<po=X>T"},
        {"数字值不剥开标签",u"<po=1>A</po>",{u"<po=1>A"},{{true,u"T"}},true,u"<po=1>T"},
        {"Unicode值不剥开标签",u"<po=甲>A</po>",{u"<po=甲>A"},{{true,u"T"}},true,u"<po=甲>T"},
        {"空值不剥开标签",u"<po=>A</po>",{u"<po=>A"},{{true,u"T"}},true,u"<po=>T"},
        {"门内移除全部匹配",u"<po=x>A<po=y>B</po></po>",{u"AB"},{{true,u"T"}},true,u"<po=x>T"},
        {"CR裁后命中",u"A\r\r",{u"A\r\r",u"A"},{{false,{}},{true,u"T"}},true,u"T"},
        {"第三查保留原CR键",u"A\n\r\r",{u"A\n\r\r",u"A\n",u"A<br>\r\r"},
             {{false,{}},{false,{}},{true,u"T"}},true,u"T"},
        {"第四查只裁尾CR",u"A\r\n\r\r",{u"A\r\n\r\r",u"A\r\n",u"A<br>\r\r",u"A<br>"},
             {{false,{}},{false,{}},{false,{}},{true,u"T"}},true,u"T"},
        {"全miss保原串",u"<po=x>A\n</po>",{u"A\n",u"A<br>"},
             {{false,{}},{false,{}}},false,u"<po=x>A\n</po>"},
        {"无prefix命中null",u"A",{u"A"},{{true,{}}},true,{}},
        {"有prefix命中null",u"<po=x>A</po>",{u"A"},{{true,{}}},true,u"<po=x>"},
        {"无prefix命中空",u"A",{u"A"},{{true,u""}},true,u""},
        {"有prefix命中空",u"<po=x>A</po>",{u"A"},{{true,u""}},true,u"<po=x>"}
    };
    for(const auto &c:cases) {
        ConsoleTextFixture backend;backend.answers=c.answers;
        const std::optional<std::u16string> source=c.input;
        const auto p=steam_translate_console_text(source,backend);
        if(!p||p->translated!=c.translated||p->out_text!=c.output||backend.keys!=c.keys||
           backend.position_queries!=std::vector<std::u16string>{c.input}||source!=c.input)
            throw std::runtime_error(std::string("Console具名oracle：")+c.name);
        check(backend.calls.size()==4+c.keys.size()&&
                  std::vector<std::string>(backend.calls.begin(),backend.calls.begin()+4)==
                      std::vector<std::string>{"table","count","shown","position"}&&
                  std::count(backend.calls.begin(),backend.calls.end(),"lookup")==static_cast<int>(c.keys.size()),
              "console原序资格与实际候选key逐项一致，无多余查询或持久化副作用");
    }
    ConsoleTextFixture expansion;expansion.answers={{true,u"T"}};
    const std::u16string linefeeds(1025,u'\n');
    check(!steam_translate_console_text(linefeeds,expansion)&&expansion.keys==std::vector<std::u16string>{linefeeds},
          "已命中仍执行换行归一，1025个LF扩为4100超过维护预算必须拒绝，不能直接交T");
    ConsoleTextFixture large_value;large_value.answers={{true,std::u16string(4097,u'T')}};
    check(!steam_translate_console_text(u"A",large_value),"实际命中值超4096拒绝，不截短或发布原输入");
    ConsoleTextFixture prefix_budget;prefix_budget.answers={{true,std::u16string(4096,u'T')}};
    check(!steam_translate_console_text(u"<po=x>A</po>",prefix_budget),"prefix与value各自合法但Concat总长超限也拒绝");
    ConsoleTextFixture oversized;oversized.table=false;
    check(!steam_translate_console_text(std::u16string(4097,u'A'),oversized)&&oversized.calls.empty(),
          "维护原输入预算先拒绝，不借禁用表吞掉超限输入");
    for(const auto *operation:{"table","count","shown","position","lookup"}) {
        ConsoleTextFixture backend;backend.answers={{false,{}},{true,u"T"}};backend.fail_operation=operation;
        bool exact=false;
        try {(void)steam_translate_console_text(u"A\r",backend);}
        catch(const LayoutBackendFailure &failure) {exact=failure.operation==operation;}
        check(exact&&backend.calls.back()==operation&&
                  std::count(backend.calls.begin(),backend.calls.end(),operation)==1,
              "真实供给首错原样传播且无后继访问或重放，不能包装成未翻译成功");
    }
    ConsoleTextFixture later;later.answers={{false,{}},{true,u"T"}};
    later.fail_operation="lookup";later.fail_occurrence=2;bool exact=false;
    try {(void)steam_translate_console_text(u"A\r",later);}
    catch(const LayoutBackendFailure &failure) {exact=failure.operation=="lookup";}
    check(exact&&later.keys==std::vector<std::u16string>{u"A\r"}&&
              std::count(later.calls.begin(),later.calls.end(),"lookup")==2,
          "CR后继查询故障保留真实前缀，仅首查询有完成的key记录，不重试失败查表");
}

// 这里只对ASCII样本提供具名文化夹具；hash是宿主给定回执，不实现.NET散列。
class TextInputFixture final:public SteamTextInputBackend {
public:
    struct Query {
        std::string operation;
        std::u16string text,needle;
        SteamTextComparison comparison;
        bool operator==(const Query &other) const {
            return operation==other.operation&&text==other.text&&needle==other.needle&&comparison==other.comparison;
        }
    };
    explicit TextInputFixture(std::u16string expected):hash_input(std::move(expected)) {}
    const std::u16string hash_input;
    static constexpr std::int32_t receipt=-2000000077;
    std::optional<SteamConsoleTextResult> console;
    SteamConsoleTextBackend *actual_console{}; // 一条真实消费者接线，仍使用显式运行表夹具。
    std::vector<std::string> calls;
    std::vector<Query> queries;
    std::string fail_operation;
    std::int32_t hash_code(const std::u16string &text) override {
        call("hash");
        if(text!=hash_input)throw std::runtime_error("公开文字夹具：hash必须读取原序Replace完成的串");
        return receipt;
    }
    SteamConsoleTextResult translate_console_text(const std::u16string &text) override {
        call("console");
        if(text!=hash_input)throw std::runtime_error("公开文字夹具：console必须读取原序Replace完成的串");
        if(actual_console) {
            const auto result=steam_translate_console_text(text,*actual_console);
            if(!result)throw std::runtime_error("公开文字夹具：真实console供给拒绝");
            return *result;
        }
        return console?*console:SteamConsoleTextResult{false,text};
    }
    bool contains(const std::u16string &text,const std::u16string &needle,SteamTextComparison comparison) override {
        query("contains",text,needle,comparison);
        return compared(text,comparison).find(compared(needle,comparison))!=std::u16string::npos;
    }
    bool starts_with(const std::u16string &text,const std::u16string &needle,SteamTextComparison comparison) override {
        query("starts",text,needle,comparison);
        const auto a=compared(text,comparison),b=compared(needle,comparison);
        return a.size()>=b.size()&&a.compare(0,b.size(),b)==0;
    }
    bool ends_with(const std::u16string &text,const std::u16string &needle,SteamTextComparison comparison) override {
        query("ends",text,needle,comparison);
        const auto a=compared(text,comparison),b=compared(needle,comparison);
        return a.size()>=b.size()&&a.compare(a.size()-b.size(),b.size(),b)==0;
    }
private:
    void call(const char *operation) {
        calls.emplace_back(operation);
        if(fail_operation==operation)throw LayoutBackendFailure{operation};
    }
    void query(const char *operation,const std::u16string &text,const std::u16string &needle,
               SteamTextComparison comparison) {
        call(operation);queries.push_back({operation,text,needle,comparison});
    }
    static std::u16string compared(std::u16string text,SteamTextComparison comparison) {
        if(comparison==SteamTextComparison::current_culture_ignore_case)
            for(auto &unit:text)if(unit>=u'A'&&unit<=u'Z')unit=static_cast<char16_t>(unit+(u'a'-u'A'));
        return text;
    }
};

void console_preparation_bridge(Checks &check) {
    ConsoleTextFixture table;table.answers={{true,u"B<ff>C"}};
    TextInputFixture backend(u"A");backend.actual_console=&table;
    const auto prepared=steam_prepare_text(u"source",{{u"source",u"A"}},backend);
    check(prepared&&prepared->text==u"B<br><ff><br>C"&&prepared->hash==TextInputFixture::receipt&&
              table.keys==std::vector<std::u16string>{u"A"}&&
              backend.calls[0]=="hash"&&backend.calls[1]=="console"&&
              backend.queries.front().text==u"B<ff>C",
          "真实console消费者接公开原序Replace/hash，再把实际out交ff重组，不重算处理串hash");
    ConsoleTextFixture null_table;null_table.answers={{true,{}}};
    TextInputFixture null_backend(u"A");null_backend.actual_console=&null_table;
    check(!steam_prepare_text(u"A",{},null_backend)&&null_table.keys==std::vector<std::u16string>{u"A"}&&
              null_backend.calls==std::vector<std::string>{"hash","console"},
          "console无prefix命中null本身成功，随后公开准备才因实际out为null拒绝，不调用ff或伪造空串");
    // 同一供给再次调用只依赖本次input/当前表；清的是夹具观测，不是被测长期状态。
    table.calls.clear();table.keys.clear();table.position_queries.clear();
    const auto repeated=steam_prepare_text(u"source",{{u"source",u"A"}},backend);
    check(repeated&&repeated->text==u"B<br><ff><br>C"&&
              repeated->hash==TextInputFixture::receipt&&table.keys==std::vector<std::u16string>{u"A"},
          "重复准备不积累隐式已翻译状态或省略当前表查询，没有新增跨调用持久历史");
}

void public_text_preparation(Checks &check) {
    const auto ignore=SteamTextComparison::current_culture_ignore_case;
    const auto culture=SteamTextComparison::current_culture;
    TextInputFixture null_backend(u"");const auto empty=steam_prepare_text({}, {},null_backend);
    check(empty&&empty->text.empty()&&empty->hash==TextInputFixture::receipt&&
              null_backend.calls==std::vector<std::string>{"hash","console","contains"}&&
              null_backend.queries==std::vector<TextInputFixture::Query>{{"contains",u"",u"<ff>",ignore}},
          "公开null先归空，仍读取实际hash、console和ff查询；空文案不是省略上游的成功");
    struct ReplaceCase {
        std::u16string input,expected;
        std::vector<SteamTextReplaceWord> words;
    };
    const std::vector<ReplaceCase> replacements{
        {u"ab",u"Y",{{u"ab",u"X"},{u"X",u"Y"}}},
        {u"ab",u"X",{{u"X",u"Y"},{u"ab",u"X"}}},
        {u"AAAA",u"AAAAAAAA",{{u"A",u"AA"}}},
        {u"aAA",u"axx",{{u"A",u"x"}}},
        {u"A",u"",{{u"A",u""}}},
        {u"A",u"A",{{{},std::u16string(4097,u'x')},{std::u16string(4097,u'x'),{}},{u"",{}}}}
    };
    for(const auto &c:replacements) {
        TextInputFixture backend(c.expected);const auto p=steam_prepare_text(c.input,c.words,backend);
        check(p&&p->text==c.expected&&p->hash==TextInputFixture::receipt&&
                  backend.calls==std::vector<std::string>{"hash","console","contains"},
              "替换按实际枚举原序跨项串联，单项不递归新串、Ordinal区分大小写、允许删空且null键值先跳过");
    }
    TextInputFixture converted(u"after-replace");converted.console=SteamConsoleTextResult{false,u"console-out"};
    const auto result=steam_prepare_text(u"before",{{u"before",u"after-replace"}},converted);
    check(result&&result->text==u"console-out"&&result->hash==TextInputFixture::receipt&&
              converted.calls==std::vector<std::string>{"hash","console","contains"}&&
              converted.queries.front().text==u"console-out",
          "hash先于console且保留Replace串回执；console返回false仍实际消费out，不给最终串重算hash");
    struct FfCase {std::u16string input,expected;};
    const std::array<FfCase,7> ff{{
        {u"<ff>A<FF>",u"A"},{u"<ff><FF>",u""},
        {u"A<ff><ff>B",u"A<br><ff><br>B"},{u"A<Ff>B",u"A<Ff>B"},
        {u" <ff> ",u" <br><ff><br> "},
        {u"A<ff>B<ff>C",u"A<br><ff><br>B<br><ff><br>C"},
        {u"<Ff><ff>A<FF><fF>",u"<Ff><br><ff><br>A<br><ff><br><fF>"}
    }};
    for(const auto &c:ff) {
        TextInputFixture backend(c.input);const auto p=steam_prepare_text(c.input,{},backend);
        check(p&&p->text==c.expected&&p->hash==TextInputFixture::receipt&&
                  backend.queries.front()==TextInputFixture::Query{"contains",c.input,u"<ff>",ignore},
              "ff入口ignorecase、分隔却只认精确ff/FF；首尾连续空片移除，不Trim，混合大小写片保留原文");
    }
    struct EdgeCase {
        std::u16string input,expected;
        std::vector<TextInputFixture::Query> queries;
    };
    const std::array<EdgeCase,3> edges{{
        {u"A<BR><ff><bR>B",u"A<BR><ff><bR>B",{
            {"contains",u"A<BR><ff><bR>B",u"<ff>",ignore},
            {"ends",u"A<BR>",u"<br>",ignore},{"starts",u"<bR>B",u"<br>",ignore}}},
        {u"A\n<ff>\nB",u"A\n<ff>\nB",{
            {"contains",u"A\n<ff>\nB",u"<ff>",ignore},
            {"ends",u"A\n",u"<br>",ignore},{"ends",u"A\n",u"\n",culture},
            {"starts",u"\nB",u"<br>",ignore},{"starts",u"\nB",u"\n",culture}}},
        {u"A<br> <ff> <br>B",u"A<br> <br><ff><br> <br>B",{
            {"contains",u"A<br> <ff> <br>B",u"<ff>",ignore},
            {"ends",u"A<br> ",u"<br>",ignore},{"ends",u"A<br> ",u"\n",culture},
            {"starts",u" <br>B",u"<br>",ignore},{"starts",u" <br>B",u"\n",culture}}}
    }};
    for(const auto &c:edges) {
        TextInputFixture backend(c.input);const auto p=steam_prepare_text(c.input,{},backend);
        check(p&&p->text==c.expected&&backend.queries==c.queries,
              "片尾先于后片开头查询：br忽略大小写命中即短路LF，否则LF为CurrentCulture；空格不Trim或跨过");
    }
}

void public_text_rejection_and_faults(Checks &check) {
    TextInputFixture null_out(u"A");null_out.console=SteamConsoleTextResult{true,{}};
    check(!steam_prepare_text(u"A",{},null_out)&&null_out.calls==std::vector<std::string>{"hash","console"},
          "console真值不能将null out伪装成功，拒绝前已消费hash与console且不继续文化查询");
    TextInputFixture large_out(u"A");large_out.console=SteamConsoleTextResult{false,std::u16string(4097,u'x')};
    check(!steam_prepare_text(u"A",{},large_out)&&large_out.calls==std::vector<std::string>{"hash","console"},
          "console实际out超4096拒绝，不在超限串上继续ff查询");
    const std::vector<std::vector<SteamTextReplaceWord>> rejected_words{
        {{u"",u"x"}},{{u"A",std::u16string(4097,u'x')}},
        {{std::u16string(4097,u'x'),u"A"}},std::vector<SteamTextReplaceWord>(4097)
    };
    for(const auto &words:rejected_words) {
        TextInputFixture backend(u"A");
        check(!steam_prepare_text(u"A",words,backend)&&backend.calls.empty(),
              "实际空旧词及已消费超限键值、超限表在后端前拒绝，不夹限或返回原文凑成功");
    }
    TextInputFixture oversized(u"");
    check(!steam_prepare_text(std::u16string(4097,u'A'),{},oversized)&&oversized.calls.empty(),
          "原输入超过4096在后端前拒绝");
    TextInputFixture growth(u"");
    check(!steam_prepare_text(std::u16string(4096,u'A'),{{u"A",u"AA"}},growth)&&growth.calls.empty(),
          "替换途中增长超预算即拒绝，不能截成4096再给hash");
    for(const int entries:std::array<int,2>{16,17}) {
        const std::u16string text(4096,u'A'),expected(4096,static_cast<char16_t>(u'A'+entries));
        TextInputFixture backend(expected);std::vector<SteamTextReplaceWord> words;
        for(int i=0;i<entries;++i)
            words.push_back({std::u16string(1,static_cast<char16_t>(u'A'+i)),
                             std::u16string(1,static_cast<char16_t>(u'A'+i+1))});
        const auto p=steam_prepare_text(text,words,backend);
        check(entries==16?(p&&p->text==expected&&p->hash==TextInputFixture::receipt):(!p&&backend.calls.empty()),
              "真实唯一旧key枚举A到B到C串联：65536次边界通过，第65537次在后端前拒绝");
    }
    std::u16string expanded;for(int i=0;i<512;++i)expanded+=u"A<ff>";expanded+=u"A";
    TextInputFixture ff_budget(expanded);
    check(!steam_prepare_text(expanded,{},ff_budget)&&
              ff_budget.calls.front()=="hash"&&ff_budget.calls[1]=="console"&&ff_budget.queries.size()>1,
          "ff插入换行后的最终串同样受4096预算约束，失败不能交部分重组候选");
    for(const auto *operation:{"hash","console","contains","ends","starts"}) {
        TextInputFixture backend(u"A<ff>B");backend.fail_operation=operation;
        bool exact=false;
        try {(void)steam_prepare_text(u"A<ff>B",{},backend);}
        catch(const LayoutBackendFailure &failure) {exact=failure.operation==operation;}
        check(exact&&std::count(backend.calls.begin(),backend.calls.end(),operation)==1,
              "真实hash/console/文化查询抛错原样传播且不重放，不把平台失败包装成成功原文");
    }
}

void draw_scale_reflow(Checks &check) {
    auto same=input(u"A");same.font={};same.scale_ratio=1.F;
    FixtureBackend untouched;const auto unchanged=steam_reflow_text_for_draw(same,1.F,untouched);
    check(unchanged&&unchanged->scale_ratio==1.F&&!unchanged->replacement&&untouched.calls.empty(),
          "同Graphics比例不读取缺失Font或调用排版后端，返回无替换而非伪造新空块");
    for(const bool negative:std::array<bool,2>{false,true}) {
        auto zero=input(u"A");zero.font={};zero.scale_ratio=negative?-0.F:0.F;
        FixtureBackend backend;const auto p=steam_reflow_text_for_draw(zero,negative?0.F:-0.F,backend);
        check(p&&!p->replacement&&std::signbit(p->scale_ratio)==negative&&backend.calls.empty(),
              "正负零相等不重排，也不把旧scale符号覆盖成当前Graphics的零符号");
    }
    auto changed=input(u"A");changed.scale_ratio=1.F;changed.request_font_size=99;
    changed.previous_text_width=42.F;changed.auto_scale=true;changed.height=1.F;
    FixtureBackend changed_backend;const auto next=steam_reflow_text_for_draw(changed,2.F,changed_backend);
    check(next&&next->scale_ratio==2.F&&next->replacement&&
              next->replacement->blocks.size()==1&&next->replacement->text_width==12.F&&
              next->replacement->request_font_size==99&&
              FixtureBackend::font(next->replacement->blocks[0].font)->used==24&&
              std::count(changed_backend.calls.begin(),changed_backend.calls.end(),"begin")==1&&
              changed_backend.sizes.empty()&&changed_backend.outside_copies.empty()&&
              changed_backend.installed.empty()&&changed_backend.cleaned()&&
              changed.scale_ratio==1.F&&changed.previous_text_width==42.F&&changed.height==1.F&&
              changed.request_font_size==99&&FixtureBackend::font(changed.font)->used==12,
          "变比例仅_make一次：块字号24/宽12，即便autoScale高1也不缩字、不备份换Font或重置首次request99；输入框高保持");
    auto empty=input(u"");empty.font={};empty.scale_ratio=1.F;
    empty.previous_text_width=42.F;empty.request_font_size=91;
    FixtureBackend empty_backend;const auto empty_next=steam_reflow_text_for_draw(empty,2.F,empty_backend);
    check(empty_next&&empty_next->replacement&&empty_next->replacement->blocks.empty()&&
              empty_next->replacement->text_width==42.F&&empty_next->replacement->request_font_size==91&&
              empty_next->scale_ratio==2.F&&empty_backend.calls.empty(),
          "变比例空原串沿_make早退，缺Font仍可交空块并保旧宽42/request91，不误调prepared SetText备份");
    auto unordered=input(u"A");unordered.scale_ratio=std::numeric_limits<float>::quiet_NaN();
    FixtureBackend nan_backend;const auto finite=steam_reflow_text_for_draw(unordered,1.F,nan_backend);
    check(finite&&finite->replacement&&finite->scale_ratio==1.F&&
              std::isnan(unordered.scale_ratio)&&
              std::count(nan_backend.calls.begin(),nan_backend.calls.end(),"begin")==1&&nan_backend.cleaned(),
          "旧NaN与有限新比例无序仍触发一次重排，候选用新比例且不回写原输入");
    for(const float scale:std::array<float,3>{std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::infinity(),16777218.F}) {
        FixtureBackend backend;
        check(!steam_reflow_text_for_draw(changed,scale,backend)&&backend.calls.empty()&&
                  changed.scale_ratio==1.F&&FixtureBackend::font(changed.font)->used==12,
              "实际新比例非有限或超过维护域时先拒绝，无新块成功、后端调用或旧Font修改");
    }
    FixtureBackend failure;failure.fail_operation="width";bool exact=false;
    try {(void)steam_reflow_text_for_draw(changed,2.F,failure);}
    catch(const LayoutBackendFailure &error) {exact=error.operation=="width";}
    check(exact&&std::count(failure.calls.begin(),failure.calls.end(),"width")==1&&failure.cleaned()&&
              failure.outside_copies.empty()&&changed.text==u"A"&&changed.scale_ratio==1.F&&
              changed.height==1.F&&changed.previous_text_width==42.F&&
              changed.request_font_size==99&&FixtureBackend::font(changed.font)->used==12,
          "变比例排块测宽失败只传播一次原错并清理，没有候选安装；原输入和入场Font不改，不宣称后端副作用回滚");
}

class BlockSequenceFixture final:public SteamTextBlockDrawBackend {
public:
    std::vector<std::int32_t> receipts;
    std::vector<const SteamLayoutBlock *> seen;
    std::vector<SteamTextBlockDrawRequest> requests;
    std::size_t fail_at{};
    std::int32_t draw_block(const SteamLayoutBlock &block,const SteamTextBlockDrawRequest &request) override {
        seen.push_back(&block);requests.push_back(request);
        if(fail_at==seen.size())throw LayoutBackendFailure{"blockDraw"};
        if(seen.size()>receipts.size())throw std::runtime_error("逐块夹具缺少已声明的实际消费回执");
        return receipts[seen.size()-1];
    }
};

void draw_block_sequence(Checks &check) {
    const auto nan=std::numeric_limits<float>::quiet_NaN();
    std::vector<SteamLayoutBlock> one(1);
    struct PositionCase {std::int32_t pos,budget;};
    for(const auto c:std::array<PositionCase,8>{{
             {std::numeric_limits<std::int32_t>::min(),-2147483},{-1001,-1},{-999,0},{0,0},
             {999,0},{1000,1},{1999,1},{std::numeric_limits<std::int32_t>::max(),2147483}}}) {
        BlockSequenceFixture backend;backend.receipts={0};
        const auto p=steam_draw_text_block_sequence(one,c.pos,c.budget<=0?nan:1.25F,2.5F,0,-1.F,backend);
        check(p&&*p==c.budget&&backend.seen.size()==(c.budget>0?1U:0U)&&
                  (c.budget<=0||backend.requests[0].budget==c.budget),
              "逐块入口按signed pos/1000截零，负值及INT边界不取绝对值/夹length，预算非正不消费未用坏坐标");
    }
    BlockSequenceFixture no_blocks;
    const auto empty=steam_draw_text_block_sequence({},1000,nan,nan,17,nan,no_blocks);
    check(empty&&*empty==1&&no_blocks.seen.empty(),
          "空块目录即使正预算也不读取无用坐标/盒高或调用后端，不伪造消费");
    struct ScrollCase {float dy;std::int32_t scroll;float text_y;};
    for(const auto c:std::array<ScrollCase,3>{{{0.F,16777217,-16777216.F},
             {16777216.F,16777217,0.F},{1.F,-16777217,16777216.F}}}) {
        BlockSequenceFixture backend;backend.receipts={1};
        const auto p=steam_draw_text_block_sequence(one,1000,1.25F,c.dy,c.scroll,-1.F,backend);
        check(p&&*p==0&&backend.requests.size()==1&&backend.seen[0]==&one[0]&&
                  backend.requests[0].dx==1.25F&&backend.requests[0].dy==c.text_y&&
                  backend.requests[0].box_y==c.dy&&backend.requests[0].box_height==-1.F,
              "scroll先转float32再减：16777217先成16777216；文字y变而boxY保持原dy，分数x及盒高-1不舍整");
    }
    // 回执分别来自已隐藏普通AB、图标、普通空块、按钮和普通C；执行器不再猜类型消费量。
    std::vector<SteamLayoutBlock> blocks(5);
    blocks[0].text=u"AB";blocks[0].blink=2;
    blocks[1].kind=SteamLayoutBlockKind::pictograph;
    blocks[3].kind=SteamLayoutBlockKind::button;
    blocks[4].text=u"C";
    BlockSequenceFixture mixed;mixed.receipts={2,1,0,1,1};
    const auto result=steam_draw_text_block_sequence(blocks,5999,3.5F,9.75F,2,16.F,mixed);
    check(result&&*result==0&&mixed.seen==std::vector<const SteamLayoutBlock *>{
                  &blocks[0],&blocks[1],&blocks[2],&blocks[3],&blocks[4]}&&
              mixed.requests.size()==5&&mixed.requests[0].budget==5&&mixed.requests[1].budget==3&&
              mixed.requests[2].budget==2&&mixed.requests[3].budget==2&&mixed.requests[4].budget==1&&
              std::all_of(mixed.requests.begin(),mixed.requests.end(),[](const auto &request) {
                  return request.dx==3.5F&&request.dy==7.75F&&request.box_y==9.75F&&request.box_height==16.F;
              }),
          "逐块以原对象引用和真实回执推进：隐藏普通仍消费2，pic/btn各1，普通空块回0仍前进且不重画本块");
    struct ReceiptCase {
        std::vector<std::int32_t> receipts,budgets;
        std::int32_t remaining;
    };
    const std::vector<ReceiptCase> unusual{
        {{2},{1},-1},{{-2,1},{1,3},2},
        {{std::numeric_limits<std::int32_t>::min()},{1},-2147483647},
        {{std::numeric_limits<std::int32_t>::max()},{1},-2147483646},
        {{-2147483646,-2},{1,2147483647},-2147483647}
    };
    for(const auto &c:unusual) {
        BlockSequenceFixture backend;backend.receipts=c.receipts;
        const std::vector<SteamLayoutBlock> candidates(c.receipts.size()+(c.remaining<=0?1U:0U));
        const auto p=steam_draw_text_block_sequence(candidates,1000,0,0,0,-1.F,backend);
        bool budgets=backend.requests.size()==c.budgets.size();
        for(std::size_t i=0;budgets&&i<c.budgets.size();++i)budgets=backend.requests[i].budget==c.budgets[i];
        check(p&&*p==c.remaining&&budgets,
              "故障后端的过量/负回执仍按源int32减法回卷，不min修补；新预算非正立即停止，正预算才走后继");
    }
    for(const bool bad_height:std::array<bool,2>{false,true}) {
        BlockSequenceFixture backend;backend.receipts={1};
        check(!steam_draw_text_block_sequence(one,1000,bad_height?0.F:nan,0,0,bad_height?nan:-1.F,backend)&&
                  backend.seen.empty(),
              "实际访问块前拒绝已消费的坏坐标或盒高，不发后端请求或返回假成功剩余量");
    }
    BlockSequenceFixture count_limit;
    check(!steam_draw_text_block_sequence(std::vector<SteamLayoutBlock>(8193),0,nan,nan,0,nan,count_limit)&&
              count_limit.seen.empty(),
          "目录超过8192个块在调用前明确拒绝，不以零预算将超限布局登记为成功");
    blocks[0].font=input(u"").font;blocks[0].x=4.25F;
    BlockSequenceFixture failure;failure.receipts={0};failure.fail_at=2;bool exact=false;
    try {(void)steam_draw_text_block_sequence(blocks,1000,3.5F,9.75F,2,16.F,failure);}
    catch(const LayoutBackendFailure &error) {exact=error.operation=="blockDraw";}
    check(exact&&failure.seen==std::vector<const SteamLayoutBlock *>{&blocks[0],&blocks[1]}&&
              failure.requests[0].budget==1&&failure.requests[1].budget==1&&
              blocks.size()==5&&blocks[0].text==u"AB"&&blocks[0].blink==2&&blocks[0].x==4.25F&&
              FixtureBackend::font(blocks[0].font)->used==12&&blocks[1].kind==SteamLayoutBlockKind::pictograph,
          "晚期后端错误传播一次：前块回0后访问第二块失败，不重试或继续尾块；原布局/Font不改，已画图元不声称回滚");
}
class InstanceDrawFixture final:public SteamTextDrawBackend {
    struct Rectangle final:SteamTextDrawRectangle {
        InstanceDrawFixture &host;
        explicit Rectangle(InstanceDrawFixture &value):host(value) {}
        float width() override {host.call("draw:rectWidth");return host.rect_width;}
        float height() override {host.call("draw:rectHeight");return host.rect_height;}
    };
public:
    explicit InstanceDrawFixture(std::vector<std::string> &value):trace(value) {}
    std::vector<std::string> &trace;
    bool matrix{},jp{},visible{},null_rectangle{};
    std::vector<float> widths{101.F,20.F,10.F};
    std::vector<std::int32_t> sizes{16,8,12},receipts{2};
    std::size_t width_at{},size_at{};
    float rect_width{30.F},rect_height{12.F},scale{1.F},local_x{7.25F},local_y{8.5F};
    std::vector<std::string> failures;
    std::vector<float> matrix_scales;
    std::vector<std::array<float,2>> rectangle_inputs,coordinate_inputs;
    std::vector<std::array<float,4>> rectangles,clips;
    std::vector<const SteamLayoutBlock *> blocks;
    std::vector<SteamTextBlockDrawRequest> requests;
    std::array<std::int32_t,3> color{-7,-8,-9};
    std::vector<std::array<std::int32_t,3>> colors;
    int matrix_depth{},coordinate_depth{},clip_depth{};
    bool check_local_matrix() override {call("draw:checkMatrix");return matrix;}
    float layout_font_width() override {
        call("draw:fontWidth");
        if(width_at==widths.size())throw std::runtime_error("Draw夹具缺少原序Font宽观测");
        return widths[width_at++];
    }
    std::int32_t layout_font_used_size() override {
        call("draw:fontUsed");
        if(size_at==sizes.size())throw std::runtime_error("Draw夹具缺少原序Font字号观测");
        return sizes[size_at++];
    }
    std::shared_ptr<SteamTextDrawRectangle> reference_rectangle(float width,float height) override {
        call("draw:refRect");rectangle_inputs.push_back({width,height});
        if(null_rectangle)return {};
        return std::make_shared<Rectangle>(*this);
    }
    void push_horizontal_scale(float value) override {
        call("draw:pushMatrix");matrix_scales.push_back(value);++matrix_depth;
    }
    void pop_matrix() override {call("draw:popMatrix");--matrix_depth;}
    void begin_local_coordination(float &x,float &y) override {
        call("draw:beginCoords");coordinate_inputs.push_back({x,y});
        x=local_x;y=local_y;++coordinate_depth;
    }
    void end_local_coordination() override {call("draw:endCoords");--coordinate_depth;}
    float graphics_scale() override {call("draw:scale");return scale;}
    bool japanese() override {call("draw:japanese");return jp;}
    bool render_rect_visible() override {call("draw:visible");return visible;}
    void set_color_rgb(std::int32_t r,std::int32_t g,std::int32_t b) override {
        call("draw:color");color={r,g,b};colors.push_back(color);
    }
    void draw_rectangle(float x,float y,float width,float height) override {
        call("draw:rectangle");rectangles.push_back({x,y,width,height});
    }
    void push_clip(float x,float y,float width,float height) override {
        call("draw:clipPush");clips.push_back({x,y,width,height});++clip_depth;
    }
    void pop_clip() override {call("draw:clipPop");--clip_depth;}
    std::int32_t draw_block(const SteamLayoutBlock &block,const SteamTextBlockDrawRequest &request) override {
        blocks.push_back(&block);requests.push_back(request);call("draw:block");
        if(blocks.size()>receipts.size())throw std::runtime_error("Draw夹具缺少真实块消费回执");
        return receipts[blocks.size()-1];
    }
    void call(const char *operation) {
        trace.emplace_back(operation);
        const auto failure=std::find(failures.begin(),failures.end(),operation);
        if(failure!=failures.end()) {failures.erase(failure);throw LayoutBackendFailure{operation};}
    }
};

SteamTextLayoutResult drawable_fixture(std::vector<std::string> &trace) {
    SteamTextLayoutResult current;current.request_font_size=77;current.text_width=9.F;current.over_area=true;
    SteamLayoutBlock block;block.font=input(u"").font;block.text=u"AB";
    FixtureBackend::font(block.font)->used=8;
    FixtureBackend::font(block.font)->used_probe=[&trace](const FixtureFont &) {trace.emplace_back("draw:firstUsed");};
    current.blocks.push_back(std::move(block));return current;
}
SteamTextLayoutInput framed_fixture() {
    auto layout=input(u"A");layout.width=40.F;layout.height=12.F;layout.auto_scale=true;return layout;
}
std::vector<std::string> draw_cleanup_calls(const std::vector<std::string> &trace) {
    std::vector<std::string> result;
    for(const auto &operation:trace)
        if(operation=="localPop"||operation=="draw:endCoords"||operation=="draw:popMatrix"||
           operation=="autoPop"||operation=="draw:clipPop")result.push_back(operation);
    return result;
}

void instance_draw_normal_order(Checks &check) {
    const std::vector<std::string> expected{
        "draw:checkMatrix","draw:fontWidth","draw:fontWidth","draw:fontUsed","draw:refRect",
        "draw:rectWidth","draw:fontWidth","draw:rectHeight","draw:fontUsed","draw:pushMatrix",
        "draw:beginCoords","autoPush0","draw:scale","localPush0","draw:japanese","draw:visible",
        "draw:color","draw:rectangle","draw:firstUsed","draw:fontUsed","draw:color","draw:rectangle",
        "draw:rectangle","draw:clipPush","draw:block","localPop","draw:endCoords","draw:popMatrix",
        "autoPop","draw:clipPop"
    };
    for(const bool zero_budget:std::array<bool,2>{false,true}) {
        std::vector<std::string> trace;auto current=drawable_fixture(trace);const auto layout=framed_fixture();
        FixtureBackend text;text.shared_trace=&trace;InstanceDrawFixture draw(trace);draw.matrix=true;draw.visible=true;
        SteamTextInstanceDrawInput in;in.dx=100.F;in.dy=200.F;in.scroll_v=3;in.pos=zero_budget?0:2000;
        in.draw=std::numeric_limits<std::int32_t>::max()-1;in.text_check_size=10;
        const auto result=steam_draw_text_layout(layout,current,in,text,draw);
        auto expected_trace=expected;
        if(zero_budget)expected_trace.erase(std::find(expected_trace.begin(),expected_trace.end(),"draw:block"));
        check(result&&result->draw==std::numeric_limits<std::int32_t>::max()&&!result->reflow.replacement&&
                  trace==expected_trace&&draw.matrix_scales==std::vector<float>{0.5F}&&
                  draw.rectangle_inputs==std::vector<std::array<float,2>>{{20.F,16.F}}&&
                  draw.coordinate_inputs==std::vector<std::array<float,2>>{{100.F,200.F}}&&
                  draw.rectangles==std::vector<std::array<float,4>>{{5.25F,6.5F,44.F,16.F},
                      {5.25F,6.5F,44.F,16.F},{4.25F,5.5F,46.F,18.F}}&&
                  draw.clips==std::vector<std::array<float,4>>{{7.25F,8.5F,40.F,14.F}}&&
                  draw.colors==std::vector<std::array<std::int32_t,3>>{{0,255,0},{255,0,0}}&&
                  draw.color==std::array<std::int32_t,3>{255,0,0}&&draw.blocks.size()==(zero_budget?0U:1U)&&
                  text.cleaned()&&draw.matrix_depth==0&&draw.coordinate_depth==0&&draw.clip_depth==0,
              "完整Draw原序：三宽两字号/Rect分时读取、.5横补偿、局部坐标后绿红框、非JP高+2 clip、正常尾原序清理；预算0仍画框且颜色不回滚");
        if(!zero_budget)
            check(draw.blocks[0]==&current.blocks[0]&&draw.requests[0].dx==7.25F&&draw.requests[0].dy==5.5F&&
                      draw.requests[0].box_y==8.5F&&draw.requests[0].box_height==12.F&&draw.requests[0].budget==2,
                  "同scale直接消费旧块引用，scroll仅移正文y，clip补高不倒改实际boxH或渐显预算");
    }
    std::vector<std::string> trace;FixtureBackend text;text.shared_trace=&trace;
    InstanceDrawFixture draw(trace);draw.matrix=true;draw.widths={99.F,11.F,3.F};draw.sizes={13,5};
    draw.rect_width=2.F;draw.rect_height=1.F;
    const auto precise=steam_draw_text_layout(input(u""),{}, {},text,draw);
    check(precise&&draw.matrix_scales==std::vector<float>{0.29999998211860657F}&&
              draw.rectangle_inputs==std::vector<std::array<float,2>>{{11.F,13.F}}&&draw.width_at==3&&draw.size_at==2&&
              trace[5]=="draw:rectWidth"&&trace[6]=="draw:fontWidth"&&trace[7]=="draw:rectHeight"&&
              trace[8]=="draw:fontUsed"&&text.cleaned()&&draw.matrix_depth==0,
          "逐次float32除法oracle为bits3E999999；合并有理数会成3E99999A，不能复用第二次宽/首次字号或提前读Rect高");
    struct RatioCase {float first_width,height;std::vector<float> pushed;};
    for(const auto &c:std::vector<RatioCase>{
             {std::numeric_limits<float>::quiet_NaN(),12.F,{0.5F}},
             {101.F,-12.F,{-0.5F}},{101.F,0.F,{0.F}},{101.F,24.F,{}},{101.F,36.F,{}}}) {
        std::vector<std::string> ratio_trace;FixtureBackend ratio_text;
        InstanceDrawFixture ratio_draw(ratio_trace);ratio_draw.matrix=true;
        ratio_draw.widths={c.first_width,20.F,10.F};ratio_draw.sizes={16,8};ratio_draw.rect_height=c.height;
        const auto p=steam_draw_text_layout(input(u""),{}, {},ratio_text,ratio_draw);
        check(p&&ratio_draw.width_at==3&&ratio_draw.size_at==2&&ratio_draw.matrix_scales==c.pushed&&
                  std::count(ratio_trace.begin(),ratio_trace.end(),"draw:popMatrix")==static_cast<int>(c.pushed.size())&&
                  ratio_draw.matrix_depth==0&&ratio_text.cleaned(),
              "首次GetWidth值丢弃故NaN不拒绝；有限负/零ratio原样压矩阵，恰1或更大不压亦不Pop，不补默认正比例");
    }
    for(const auto c:std::array<std::array<std::int32_t,2>,3>{{{-2,-1},
             {std::numeric_limits<std::int32_t>::max()-1,std::numeric_limits<std::int32_t>::max()},
             {std::numeric_limits<std::int32_t>::max(),std::numeric_limits<std::int32_t>::max()}}}) {
        std::vector<std::string> count_trace;FixtureBackend count_text;InstanceDrawFixture count_draw(count_trace);
        SteamTextInstanceDrawInput in;in.draw=c[0];const auto p=steam_draw_text_layout(input(u""),{},in,count_text,count_draw);
        check(p&&p->draw==c[1]&&count_draw.matrix_scales.empty()&&count_draw.width_at==0&&count_draw.size_at==0,
              "不在局部矩阵中跳过全部宽/字号查询；draw负值照加、MAX封顶，空正文仍有外层生命周期");
    }
}

void instance_draw_gates_and_reflow(Checks &check) {
    struct RedCase {bool empty;int first;std::vector<std::int32_t> sizes;std::size_t red;};
    for(const auto &c:std::vector<RedCase>{{true,8,{12},0},{false,-1,{12},0},
             {false,8,{3,12},0},{false,8,{12,8},0},{false,8,{12,12},2}}) {
        std::vector<std::string> trace;auto current=drawable_fixture(trace);
        if(c.empty)current.blocks.clear();else FixtureBackend::font(current.blocks[0].font)->used=c.first;
        current.over_area=false;FixtureBackend text;text.shared_trace=&trace;InstanceDrawFixture draw(trace);draw.sizes=c.sizes;
        SteamTextInstanceDrawInput in;in.text_check_size=-1;
        const auto p=steam_draw_text_layout(framed_fixture(),current,in,text,draw);
        check(p&&draw.size_at==c.sizes.size()&&draw.rectangles.size()==c.red&&text.cleaned()&&
                  position(trace,"draw:fontUsed")<position(trace,"draw:firstUsed"),
              "checkSize=-1先取当前Font阈值，即使块为空；首块字号-1短路，之后基Font仍先于阈值比较且不能偷用阈值读数");
    }
    struct Gate {bool automatic;float width,height;int check_size;};
    for(const auto c:std::array<Gate,4>{{{false,40,12,10},{true,-1,12,10},
             {true,40,-1,10},{true,40,12,0}}}) {
        std::vector<std::string> trace;auto current=drawable_fixture(trace);current.over_area=false;current.blocks[0].font={};
        auto layout=framed_fixture();layout.auto_scale=c.automatic;layout.width=c.width;layout.height=c.height;
        FixtureBackend text;InstanceDrawFixture draw(trace);draw.sizes.clear();draw.visible=true;
        SteamTextInstanceDrawInput in;in.text_check_size=c.check_size;
        const auto p=steam_draw_text_layout(layout,current,in,text,draw);
        check(p&&draw.size_at==0&&draw.rectangles.size()==(c.width!=-1&&c.height!=-1?1U:0U)&&text.cleaned(),
              "红框须check非0/autoScale/两维非-1，失败不访问坏首Font；绿色独立沿visible与两维资格且无DEBUG门");
    }
    for(const bool jp:std::array<bool,2>{false,true}) {
        std::vector<std::string> trace;auto current=drawable_fixture(trace);current.blocks.clear();
        auto layout=framed_fixture();layout.height=-1.F;FixtureBackend text;InstanceDrawFixture draw(trace);draw.jp=jp;
        const auto p=steam_draw_text_layout(layout,current,{},text,draw);
        check(p&&draw.clips==std::vector<std::array<float,4>>{{7.25F,8.5F,40.F,-1.F}},
              "overArea单独决定clip；盒高-1不增非JP的2，也不擅自豁免整个clip请求");
    }
    std::vector<std::string> trace;auto current=drawable_fixture(trace);current.text_width=42.F;
    auto layout=framed_fixture();layout.height=1.F;layout.multi_line=false;layout.request_font_size=999;
    FixtureBackend text;text.shared_trace=&trace;text.translation=u"ABCDE";
    InstanceDrawFixture draw(trace);draw.scale=2.F;draw.receipts={1};
    SteamTextInstanceDrawInput in;in.pos=1000;in.draw=9;in.scroll_v=3;
    const auto p=steam_draw_text_layout(layout,current,in,text,draw);
    check(p&&p->reflow.replacement&&p->reflow.replacement->request_font_size==77&&
              p->reflow.replacement->text_width==60.F&&p->reflow.replacement->blocks[0].text==u"ABCDE"&&
              FixtureBackend::font(p->reflow.replacement->blocks[0].font)->used==24&&p->draw==10&&
              draw.blocks.size()==1&&draw.requests[0].budget==1&&draw.clips.empty()&&
              text.outside_copies.empty()&&text.sizes.empty()&&text.cleaned()&&
              current.text_width==42.F&&current.request_font_size==77&&current.blocks[0].text==u"AB"&&
              layout.height==1.F&&layout.scale_ratio==1.F&&in.pos==1000&&in.draw==9&&
              position(trace,"draw:scale")<position(trace,"begin")&&
              position(trace,"end")<position(trace,"localPush0"),
          "变比例只_make一次并用新overArea/块，旧current宽/request覆盖输入历史；不缩字备份、不重算渐显或改旧输入");
}

void instance_draw_fault_obligations(Checks &check) {
    const std::vector<std::string> matrix_only{"draw:popMatrix"};
    const std::vector<std::string> coordinates{"draw:endCoords","draw:popMatrix"};
    const std::vector<std::string> automatic{"draw:endCoords","draw:popMatrix","autoPop"};
    const std::vector<std::string> localized{"localPop","draw:endCoords","draw:popMatrix","autoPop"};
    const std::vector<std::string> clipped{"localPop","draw:endCoords","draw:popMatrix","autoPop","draw:clipPop"};
    struct FaultCase {const char *operation;std::vector<std::string> cleanup;};
    const std::vector<FaultCase> cases{
        {"draw:checkMatrix",{}},{"draw:fontWidth",{}},{"draw:fontUsed",{}},{"draw:refRect",{}},
        {"draw:rectWidth",{}},{"draw:rectHeight",{}},{"draw:pushMatrix",{}},
        {"draw:beginCoords",matrix_only},{"autoPush0",coordinates},{"draw:scale",automatic},
        {"localPush0",automatic},{"draw:japanese",localized},{"draw:visible",localized},
        {"draw:color",localized},{"draw:rectangle",localized},{"draw:clipPush",localized},
        {"draw:block",clipped},{"localPop",clipped},{"draw:endCoords",clipped},
        {"draw:popMatrix",clipped},{"autoPop",clipped},{"draw:clipPop",clipped}
    };
    for(const auto &c:cases) {
        std::vector<std::string> trace;auto current=drawable_fixture(trace);const auto layout=framed_fixture();
        FixtureBackend text;text.shared_trace=&trace;InstanceDrawFixture draw(trace);draw.matrix=true;draw.visible=true;
        const std::string operation=c.operation;
        if(operation.rfind("draw:",0)==0)draw.failures={operation};else text.fail_operation=operation;
        SteamTextInstanceDrawInput in;in.pos=2000;in.draw=99;in.text_check_size=10;bool exact=false;
        try {(void)steam_draw_text_layout(layout,current,in,text,draw);}
        catch(const LayoutBackendFailure &error) {exact=error.operation==operation;}
        check(exact&&draw_cleanup_calls(trace)==c.cleanup&&
                  std::count(trace.begin(),trace.end(),operation)==1&&in.draw==99&&in.pos==2000&&
                  current.blocks[0].text==u"AB"&&FixtureBackend::font(current.blocks[0].font)->used==8,
              "Draw后端故障只清成功取得的义务，首错原样传播、失败清理不重试、无draw/输入安装");
    }
    for(const bool missing_rectangle:std::array<bool,2>{false,true}) {
        std::vector<std::string> rejected_trace;FixtureBackend rejected_text;
        InstanceDrawFixture rejected_draw(rejected_trace);rejected_draw.matrix=true;
        rejected_draw.null_rectangle=missing_rectangle;rejected_draw.sizes={16,0};
        const auto p=steam_draw_text_layout(input(u""),{}, {},rejected_text,rejected_draw);
        check(!p&&draw_cleanup_calls(rejected_trace).empty()&&rejected_text.calls.empty()&&
                  rejected_draw.coordinate_inputs.empty()&&rejected_draw.matrix_scales.empty()&&
                  rejected_draw.size_at==(missing_rectangle?1U:2U),
              "实际Rect缺失或第二字号0分母在取得矩阵/坐标前拒绝，不造参考矩形/默认字号或清理未取得的义务");
    }
    {
        std::vector<std::string> coordinate_trace;FixtureBackend coordinate_text;
        InstanceDrawFixture coordinate_draw(coordinate_trace);coordinate_draw.matrix=true;
        coordinate_draw.local_x=std::numeric_limits<float>::quiet_NaN();
        const auto p=steam_draw_text_layout(input(u""),{}, {},coordinate_text,coordinate_draw);
        check(!p&&draw_cleanup_calls(coordinate_trace)==coordinates&&coordinate_text.calls.empty()&&
                  coordinate_draw.matrix_depth==0&&coordinate_draw.coordinate_depth==0&&
                  std::count(coordinate_trace.begin(),coordinate_trace.end(),"draw:scale")==0,
              "BeginCoords成功取得义务后回传NaN需拒绝，只清坐标/先前矩阵，不Pop尚未取得的自动或本地化栈");
    }
    {
        std::vector<std::string> make_trace;auto old=drawable_fixture(make_trace);
        FixtureBackend make_text;make_text.shared_trace=&make_trace;make_text.fail_operation="width";
        InstanceDrawFixture make_draw(make_trace);make_draw.matrix=true;make_draw.scale=2.F;
        bool original=false;
        try {(void)steam_draw_text_layout(framed_fixture(),old,{},make_text,make_draw);}
        catch(const LayoutBackendFailure &error) {original=error.operation=="width";}
        check(original&&draw_cleanup_calls(make_trace)==std::vector<std::string>{
                      "autoPop","draw:endCoords","draw:popMatrix","autoPop"}&&
                  std::count(make_trace.begin(),make_trace.end(),"begin")==1&&
                  std::count(make_trace.begin(),make_trace.end(),"end")==1&&
                  std::count(make_trace.begin(),make_trace.end(),"localPop")==0&&make_text.cleaned()&&
                  make_draw.coordinate_depth==0&&make_draw.matrix_depth==0&&old.blocks[0].text==u"AB",
              "嵌套_make在外层localize取得前失败，先释放其自身样本/自动栈，再清外层坐标/矩阵/自动栈，不误Pop尚未取得的本地化义务");
    }
    std::vector<std::string> trace;auto current=drawable_fixture(trace);FixtureBackend text;text.shared_trace=&trace;
    text.fail_operation="localPop";InstanceDrawFixture draw(trace);draw.matrix=true;draw.visible=true;
    draw.failures={"draw:block","draw:endCoords","draw:clipPop"};
    SteamTextInstanceDrawInput in;in.pos=2000;in.text_check_size=10;bool first=false;
    try {(void)steam_draw_text_layout(framed_fixture(),current,in,text,draw);}
    catch(const LayoutBackendFailure &error) {first=error.operation=="draw:block";}
    check(first&&draw_cleanup_calls(trace)==clipped&&
              std::count(trace.begin(),trace.end(),"localPop")==1&&
              std::count(trace.begin(),trace.end(),"draw:clipPop")==1&&
              draw.matrix_depth==0&&text.auto_depth==0&&draw.color==std::array<std::int32_t,3>{255,0,0},
          "块首错后多个清理同时失败仍尝试其余已取得义务，PopClip失败恰一次，已写红色不回滚为入场色");
}
} // namespace

int steam_text_layout_checks() {
    Checks check;ordinary_and_source_ranges(check);fonts_and_tags(check);
    combination_words_and_empty(check);boundaries_and_cleanup(check);
    prepared_text_outer(check);prepared_text_failures(check);
    console_text_consumer(check);console_preparation_bridge(check);
    public_text_preparation(check);public_text_rejection_and_faults(check);
    draw_scale_reflow(check);draw_block_sequence(check);
    instance_draw_normal_order(check);instance_draw_gates_and_reflow(check);instance_draw_fault_obligations(check);
    // 逐字面代入固定RVA8E8770算式的独立oracle，不使用被测helper构造期望。
    struct HashCase {std::u16string text;std::int32_t hash;};
    for(const auto &c:std::vector<HashCase>{{u"",371857150},{u"A",372029405},
             {u"AB",-331290873},{u"ABC",-325606582},{u"中文",1842450162},
             {u"\U0001F600",1126268081},{std::u16string{u'A',0,u'B'},372029405},
             {std::u16string{0,u'A'},371857150}})
        check(steam_text_string_hash(c.text)==c.hash,
              "固定Steam hash保留双累加器回卷、Unicode单元和内嵌零终止，不改成std::hash或按length继续");
    return check.count;
}
