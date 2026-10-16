// 完整排块的功能短测。文化比较、字体与非加法字宽均为具名夹具，不是平台字形认证。
#include "dungeon_village_prototype/steam_text_layout.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
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
    std::int32_t used_size() const override {return used;}
    std::int32_t request_size() const override {return request;}
    std::int32_t style() const override {return font_style;}
};
struct LayoutBackendFailure {std::string operation;};
class FixtureBackend final:public SteamTextLayoutBackend {
public:
    std::vector<std::string> calls;
    std::vector<std::pair<int,int>> copies;
    std::vector<std::pair<int,std::u16string>> widths;
    std::vector<SteamLayoutFontRef> fonts,installed;
    std::optional<std::u16string> translation;
    std::optional<std::u16string> language;
    std::map<std::u16string,float> width_overrides; // 明确的组合测宽夹具，不模拟Unicode字形算法。
    std::string fail_operation;
    bool fail_cleanup{},japanese{true},localize{},disabled{},chanakya{true};
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
        call("copy");auto out=std::make_shared<FixtureFont>(*font(from));
        out->identity=next_identity++;copies.emplace_back(font(from)->identity,out->identity);
        fonts.push_back(out);return out;
    }
    std::int32_t localized_request(const SteamLayoutFontRef &) override {call("localizedRequest");return 10;}
    void install_layout_font(const SteamLayoutFontRef &value) override {call("install");installed.push_back(value);}
    void set_size(const SteamLayoutFontRef &value,std::int32_t request,bool preset) override {
        call("setSize");auto out=font(value);out->request=request;
        if(preset&&request>=0&&request<3)out->used=std::array<int,3>{12,24,30}[request];
        else out->used=request;
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
        if(fail_operation==name) {fail_operation.clear();throw LayoutBackendFailure{name};}
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
} // namespace

int steam_text_layout_checks() {
    Checks check;ordinary_and_source_ranges(check);fonts_and_tags(check);
    combination_words_and_empty(check);boundaries_and_cleanup(check);
    return check.count;
}
