// 固定Steam TextLayout._makeTextBlocks RVA7CD750的维护消费者。
// 后端提供真实字体/语言；这里保持UTF-16扫描、重复测宽及源范围重建的原顺序。
#include "dungeon_village_prototype/steam_text_layout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <limits>
#include <utility>

namespace dungeon_village_prototype {
namespace {
static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559);
constexpr std::size_t text_budget=4096,block_budget=8192;
struct Rejected {};
float f32(float value) {volatile float out=value;return out;}
float bounded(float value) {
    if(!std::isfinite(value)||std::abs(value)>16777216.0f)throw Rejected{};
    return value;
}
std::int32_t signed32(std::uint32_t bits) {
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):
        static_cast<std::int32_t>(static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::int32_t add32(std::int32_t a,std::int32_t b) {
    return signed32(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));
}
std::int32_t sub32(std::int32_t a,std::int32_t b) {
    return signed32(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));
}
std::int32_t integer(float value) {
    if(!std::isfinite(value)||static_cast<double>(value)<std::numeric_limits<std::int32_t>::min()||
       static_cast<double>(value)>std::numeric_limits<std::int32_t>::max())throw Rejected{};
    return static_cast<std::int32_t>(value);
}
int used(const SteamLayoutFontRef &font) {
    if(!font)throw Rejected{};
    const auto size=font->used_size();
    if(size<0||size>65536)throw Rejected{};
    return size;
}
bool prefix_at(const std::u16string &text,std::size_t index,const std::u16string &prefix) {
    return index<=text.size()&&text.compare(index,prefix.size(),prefix)==0;
}
const std::array<std::u16string,14> tags{u"<co=>",u"</co>",u"<si=>",u"</si>",
    u"<st=>",u"</st>",u"<bl=>",u"</bl>",u"<po=>",u"<pic=>",u"<bo=>",u"</bo>",
    u"<btn=>",u"<ff>"};

// 正常尾按原序清理。异常时尽力收齐本消费者已取得的栈，保留首个后端错误；
// 这是维护策略，不把原来的部分赋值或后端图形/语言副作用描述为事务回滚。
struct Scope {
    SteamTextLayoutBackend &backend;
    SteamLayoutFontRef entry;
    bool sample{},chanakya_tail{},auto_layout{},localize_off{},localized_off{},installed{},disabled{};
    void inner() {
        if(disabled) {disabled=false;backend.set_layout_disabled(false);}
        if(localized_off) {localized_off=false;backend.pop_localize();}
        if(installed) {installed=false;backend.install_layout_font(entry);}
        if(localize_off) {localize_off=false;backend.pop_localize();}
        if(auto_layout) {auto_layout=false;backend.pop_auto_text_layout();}
    }
    void outer() {
        if(chanakya_tail) {chanakya_tail=false;backend.set_chanakya_enabled(true);}
        if(sample) {sample=false;backend.end_sample();}
    }
    void unwind() noexcept {
        // 单项清理失败也继续收齐其它已取得资源；不得重放失败操作。
        auto attempt=[](auto call) {try {call();}catch(...) {}};
        if(disabled) {disabled=false;attempt([&]{backend.set_layout_disabled(false);});}
        if(localized_off) {localized_off=false;attempt([&]{backend.pop_localize();});}
        if(installed) {installed=false;attempt([&]{backend.install_layout_font(entry);});}
        if(localize_off) {localize_off=false;attempt([&]{backend.pop_localize();});}
        if(auto_layout) {auto_layout=false;attempt([&]{backend.pop_auto_text_layout();});}
        if(chanakya_tail) {chanakya_tail=false;attempt([&]{backend.set_chanakya_enabled(true);});}
        if(sample) {sample=false;attempt([&]{backend.end_sample();});}
    }
};

class Scanner {
public:
    Scanner(const SteamTextLayoutInput &input,SteamTextLayoutBackend &backend,
            SteamLayoutFontRef base,SteamTextLayoutResult &result):
        in(input),out(backend),base_font(std::move(base)),result(result) {}

    void run() {
        work=clone(base_font);
        out.adjust_font_size(work,in.scale_ratio);
        (void)used(work);
        row_height=used(base_font); // 初始高来自activeBase，之后换行才取工作Font。
        color=in.color;horizontal=in.anchor&7;vertical=in.anchor&0x70;
        text=out.translate_text(in.text,true);
        if(text.size()>text_budget)throw Rejected{};
        text=out.unicode_to_chanakya(text);
        if(text.size()>text_budget)throw Rejected{};
        out.set_chanakya_enabled(false);
        for(std::size_t at=0;(at=text.find(u"<br>",at))!=std::u16string::npos;++at)
            text.replace(at,4,u"\n");
        result.text_width=0;
        scan();
        finish();
    }
    bool word_enabled{}; // 入口一次真实IsJapaneseStyle/原串空格查询的结果。

private:
    const SteamTextLayoutInput &in;
    SteamTextLayoutBackend &out;
    SteamLayoutFontRef base_font,work;
    SteamTextLayoutResult &result;
    std::u16string text;
    std::optional<std::size_t> current;
    std::size_t row_start{};
    float row_width{},y{};
    std::int32_t row_height{},color{},blink{},border{-1};
    int horizontal{},vertical{};
    bool new_block{true},word_head{true},pending_ff{},bold{};

    SteamLayoutFontRef clone(const SteamLayoutFontRef &font) {
        auto copy=out.copy_font(font);
        if(!copy||copy==font)throw Rejected{};
        (void)used(copy);
        return copy;
    }
    float width(const std::u16string &value) {
        return bounded(out.string_width_f(work,value));
    }
    void append_block() {
        if(result.blocks.size()>=block_budget)throw Rejected{};
        SteamLayoutBlock block;
        block.font=clone(work);block.color=color;block.blink=blink;
        block.bold=bold;block.border_color=border;
        result.blocks.push_back(std::move(block));current=result.blocks.size()-1;
    }
    void increase(float delta) {
        row_width=bounded(f32(row_width+delta));
        if(row_width>result.text_width)result.text_width=row_width;
    }
    float row_x(float center_width) const {
        if(horizontal==2)return in.width==-1?-f32(center_width*0.5f):
            bounded(f32(f32(in.width*0.5f)-f32(center_width*0.5f)));
        if(horizontal==4)return in.width==-1?-row_width:bounded(f32(in.width-row_width));
        return 0;
    }
    void place_row(float center_width) {
        auto x=row_x(center_width);
        for(auto i=row_start;i<result.blocks.size();++i) {
            auto &block=result.blocks[i];block.x=x;
            x=bounded(f32(x+block.width));
            const auto half=sub32(row_height,used(block.font))/2;
            block.y=bounded(f32(y+static_cast<float>(half)));
        }
    }
    void advance_row() {
        const auto step=add32(row_height,in.line_space);
        const auto c=static_cast<float>(step);
        if(pending_ff&&!in.auto_scale) {
            if(step==0)throw Rejected{};
            const auto n=integer(f32(f32(in.height+static_cast<float>(in.line_space))/c));
            if(n==0)throw Rejected{};
            auto remainder=[&] {
                const auto value=integer(f32(f32(y/c)+0.1f));
                // MIN/-1的原signed除法异常不在C++里执行未定义运算。
                if(value==std::numeric_limits<std::int32_t>::min()&&n==-1)throw Rejected{};
                return value%n;
            };
            std::size_t attempts=0;
            while(remainder()!=0) {
                const auto next=bounded(f32(y+c));
                if(next==y||++attempts>65536)throw Rejected{};
                y=next;
            }
            y=bounded(f32(y-c));y=bounded(f32(y+c));
        } else y=bounded(f32(y+c));
        new_block=true;row_height=used(work);pending_ff=false;
    }
    bool tag(std::size_t &index,char16_t character,SteamLayoutBlockKind &kind,
             std::int32_t &special,std::vector<std::u16string> &parameters) {
        if(character!=u'<')return false;
        const auto end=text.find(u'>',index);
        if(end==std::u16string::npos)return false;
        if(end-index>32) {++index;return true;}
        const auto candidate=text.substr(index,end-index+1);
        std::size_t number=0;
        for(;number<tags.size();++number) {
            const auto &pattern=tags[number];
            if(out.compare_to(pattern,candidate)==0)break;
            const auto equals=pattern.find(u'=');
            if(equals!=std::u16string::npos&&candidate.find(u'=')!=std::u16string::npos&&
               out.starts_with(candidate,pattern.substr(0,equals+1)))break;
        }
        // CDD5F直接跳CE224；未知TAG仍从原'<'逐单元走普通路径。
        if(number==tags.size())return false;
        index=end;
        std::u16string parameter;
        std::int32_t decimal=0,hex=0;
        const auto equals=candidate.find(u'=');
        if(equals!=std::u16string::npos) {
            parameter=candidate.substr(equals+1,candidate.size()-equals-2);
            decimal=out.try_parse_integer(parameter,10).value_or(0);
            hex=out.try_parse_integer(parameter,16).value_or(0);
            if((number==0||number==10)&&hex==0&&parameter.find(u',')!=std::u16string::npos) {
                const auto values=out.split_parameters(parameter);
                std::uint32_t rgb=0;
                for(std::size_t i=0;i<values.size()&&i<3;++i)
                    rgb|=static_cast<std::uint32_t>(out.parse_decimal(values[i]))<<(16-8*i);
                hex=signed32(rgb);
            }
        }
        switch(number) {
        case 0:color=out.rgb_color(hex);break;
        case 1:color=in.color;break;
        case 2:out.set_size(work,decimal,true);row_height=std::max(row_height,used(work));break;
        case 3:out.set_size(work,base_font->request_size(),true);
               row_height=std::max(row_height,used(work));break;
        case 4:out.set_style(work,decimal);break;
        case 5:out.set_style(work,base_font->style());break;
        case 6:blink=decimal;break;
        case 7:blink=0;break;
        case 8:
            if(parameter.find(u'l')!=std::u16string::npos)horizontal=1;
            if(parameter.find(u'c')!=std::u16string::npos)horizontal=2;
            if(parameter.find(u'r')!=std::u16string::npos)horizontal=4;
            if(parameter.find(u't')!=std::u16string::npos)vertical=16;
            if(parameter.find(u'm')!=std::u16string::npos)vertical=32;
            if(parameter.find(u'b')!=std::u16string::npos)vertical=64;
            y=0;break;
        case 9:
            kind=SteamLayoutBlockKind::pictograph;
            parameters=out.split_parameters(parameter);
            special=out.pictograph_layout_width(parameters,work);break;
        case 10:border=out.rgb_color(hex);bold=true;break;
        case 11:bold=false;break;
        case 12:
            kind=SteamLayoutBlockKind::button;special=add32(used(work),2);
            parameters=out.split_parameters(parameter);break;
        case 13:pending_ff=true;break;
        }
        new_block=true;
        if(kind==SteamLayoutBlockKind::ordinary) {++index;return true;}
        return false; // 特殊块仍经同一溢出/整词/行提交；character保持最初的'<'。
    }
    bool word_prefix(std::size_t limit) {
        for(std::size_t i=0;i<limit;++i) {
            if(i>=in.text.size())throw Rejected{}; // 原串/处理串没有索引映射。
            const auto character=in.text[i];
            if(out.trim_is_empty(character))continue;
            if(character!=u'<')return true;
            const auto end=text.find(u'>',i+1);
            if(end==std::u16string::npos)return false;
            i=end;
        }
        return false;
    }
    void lookahead(std::size_t index,char16_t character,bool special_block,
                   std::int32_t special,bool &line_break) {
        if(!word_enabled||!word_head||character==u' ')return;
        const auto start=index+(special_block?1:0);
        auto end=start;
        while(end<text.size()&&text[end]!=u' '&&text[end]!=u'\n'&&!prefix_at(text,end,u"<br>"))++end;
        if(end>start) {
            float predicted=special_block?static_cast<float>(special):0;
            for(auto i=start;i<end;++i) {
                const auto character_to_measure=text[i];
                if(character_to_measure==u'<') {
                    const auto close=text.find(u'>',i+1);
                    if(close!=std::u16string::npos)i=close;
                    bool skip=false;
                    for(const auto *prefix:{u"<co=",u"<si=",u"<st=",u"<po=",u"<bl=",u"<bo=",u"<ff>",u"</",u"<br>"})
                        if(prefix_at(text,i,prefix)) {skip=true;break;}
                    if(skip)continue;
                    if(prefix_at(text,i,u"<pic=")||prefix_at(text,i,u"<btn="))
                        predicted=bounded(f32(predicted+static_cast<float>(special)));
                }
                predicted=bounded(f32(predicted+width(std::u16string(1,character_to_measure))));
            }
            if(in.width!=-1&&f32(row_width+predicted)>in.width) {
                if(word_prefix(start))line_break=true;
                else word_enabled=false;
            }
        }
        word_head=false;
    }
    void scan() {
        for(std::size_t index=0;index<text.size();) {
            const auto old_width=row_width;
            const auto character=text[index];
            auto kind=SteamLayoutBlockKind::ordinary;
            std::int32_t special=0;
            std::vector<std::u16string> parameters;
            if(tag(index,character,kind,special,parameters))continue;
            const bool special_block=kind!=SteamLayoutBlockKind::ordinary;
            const auto threshold=out.language_code()==u"hi"?128:255;
            bool combine=false,combine_three=false;
            if(index>0&&text[index-1]>threshold&&text[index]>threshold) {
                const auto pair=width(text.substr(index-1,2));
                const auto first=width(text.substr(index-1,1));
                combine=pair==first;
                if(combine&&index+1<text.size()&&text[index+1]>threshold) {
                    const auto triple=width(text.substr(index-1,3));
                    const auto again=width(text.substr(index-1,1));
                    combine_three=triple==again;
                }
            }
            float delta=0;
            if(special_block)delta=static_cast<float>(special);
            else if(character!=u'\n'&&character!=u'\r'&&!combine)
                delta=current?f32(width(result.blocks[*current].text+character)-result.blocks[*current].width):
                    width(std::u16string(1,character));
            bool line_break=character==u'\n';
            if(!line_break&&in.width!=-1&&!combine&&f32(row_width+delta)>in.width&&in.multi_line) {
                if(in.format&&(character==u'\u3001'||character==u'\u3002'))delta=0;
                else line_break=true;
            }
            lookahead(index,character,special_block,special,line_break);
            if(!out.japanese_style_text_layout()&&character==u' ') {
                word_head=true;
                if(line_break)delta=0;
            }
            const auto accept_delta=delta;
            if(line_break) {
                place_row(old_width);row_width=0;row_start=result.blocks.size();advance_row();
            }
            if(new_block&&(delta>0||combine||special_block)) {append_block();new_block=false;}
            if(special_block) {
                if(!current)throw Rejected{};
                auto &block=result.blocks[*current];block.kind=kind;block.parameters=std::move(parameters);
                block.bold=false;block.width=bounded(static_cast<float>(special));
                increase(block.width);append_block(); // 真正普通空块，不能仅设new_block或删除。
            } else if(accept_delta>0||combine) {
                if(!current)throw Rejected{};
                auto &block=result.blocks[*current];
                if(!combine)delta=f32(width(block.text+character)-block.width);
                if(block.start==-1)block.start=static_cast<std::int32_t>(index);
                block.length=static_cast<std::int32_t>(index)-block.start+1+(combine_three?1:0);
                if(!combine)block.text+=character;
                block.width=bounded(f32(block.width+delta));increase(delta);
            }
            index+=combine_three?2:1;
        }
    }
    void finish() {
        auto total=y;
        if(row_start<result.blocks.size()) {place_row(row_width);total=bounded(f32(y+static_cast<float>(row_height)));}
        float offset=0;
        if(vertical==32)offset=in.height==-1?-f32(total*0.5f):f32(f32(in.height-total)*0.5f);
        else if(vertical==64)offset=in.height==-1?-total:f32(in.height-total);
        if(in.height!=-1&&offset<0)offset=0;
        for(auto &block:result.blocks) {
            block.y=bounded(f32(block.y+offset));
            if(block.start!=-1) {
                const auto start=static_cast<std::size_t>(block.start),length=static_cast<std::size_t>(block.length);
                if(start>text.size()||length>text.size()-start)throw Rejected{};
                block.text=text.substr(start,length); // 不重测；中间CR/零宽单元可重新进入最终串。
            }
        }
    }
};
} // namespace

std::optional<SteamTextLayoutResult> steam_make_text_blocks(
        const SteamTextLayoutInput &in,SteamTextLayoutBackend &out) {
    if(in.text.size()>text_budget)return {};
    SteamTextLayoutResult result;result.text_width=in.previous_text_width;
    result.request_font_size=in.request_font_size;
    if(in.text.empty())return result; // 不读Font、不清旧宽、不压栈或切换全局开关。
    Scope scope{out,in.font};
    try {
        (void)bounded(in.width);(void)bounded(in.height);(void)bounded(in.scale_ratio);
        (void)used(in.font);
        out.begin_sample();scope.sample=true;scope.chanakya_tail=true;
        out.push_auto_text_layout(false);scope.auto_layout=true;
        const bool word_enabled=!out.japanese_style_text_layout()&&in.multi_line&&
            in.text.find(u' ')!=std::u16string::npos;
        if(!in.localize_font_enable) {out.push_localize(false);scope.localize_off=true;}
        auto base=in.font;
        if(out.needs_localize(in.font)) {
            base=out.copy_font(in.font);
            if(!base||base==in.font)throw Rejected{};
            out.install_layout_font(base);scope.installed=true;
            out.set_size(base,out.localized_request(base),true);
            out.push_localize(false);scope.localized_off=true;
        }
        if(result.request_font_size==-1)result.request_font_size=used(base);
        out.set_layout_disabled(true);scope.disabled=true;
        Scanner scanner(in,out,base,result);scanner.word_enabled=word_enabled;scanner.run();
        scope.inner();
        // 原尾只看右/下边；height=-1没有免检，空特殊后继也遍历。
        if(in.width!=-1&&!in.auto_scale)for(const auto &block:result.blocks)
            if(f32(block.x+block.width)>in.width||f32(block.y+static_cast<float>(used(block.font)))>in.height) {
                result.over_area=true;break;
            }
        scope.outer();
        return result;
    } catch(const Rejected &) {scope.unwind();return {};}
      catch(...) {scope.unwind();throw;}
}

std::optional<SteamTextSetResult> steam_set_prepared_text(
        const SteamTextSetInput &in,SteamTextLayoutBackend &out,std::uint32_t budget) {
    if(!budget||budget>65536||in.layout.text.size()>text_budget)return {};
    auto current=in.layout;
    SteamTextSetResult result;
    result.height=current.height;result.adjust_font_size=in.adjust_font_size;
    SteamLayoutFontRef backup;
    bool restore_pending{};
    auto restore=[&] {
        if(restore_pending) {restore_pending=false;out.install_layout_font(backup);}
    };
    auto cleanup=[&]() noexcept {try {restore();}catch(...) {}};
    try {
        // 107CC4DE–538只在此分支读一次静态比例；两次乘法分别回卷、除法分别截零。
        if(!result.adjust_font_size&&current.multi_line) {
            if(!in.request_font_scale)throw Rejected{};
            const auto scale=*in.request_font_scale;
            auto size=used(current.font);
            for(int n=0;n<2;++n) {
                size=signed32(static_cast<std::uint32_t>(size)*100U);
                if(!scale||(size==std::numeric_limits<std::int32_t>::min()&&scale==-1))throw Rejected{};
                size/=scale;
            }
            out.set_size(current.font,size,false);
            result.adjust_font_size=true;
        }
        current.request_font_size=-1;
        auto layout=[&] {
            if(!budget)throw Rejected{};
            --budget;
            // 上次宽／首次请求已在current，旧块在新排块前退休，不保留每轮Font或字符串。
            result.layout={};
            auto next=steam_make_text_blocks(current,out);
            if(!next)throw Rejected{};
            result.layout=std::move(*next);
            current.previous_text_width=result.layout.text_width;
            current.request_font_size=result.layout.request_font_size;
        };
        layout();
        // 107CC551–576正常路径始终复制；空串首排也不能把Font缺失视作合法。
        if(!current.font)throw Rejected{};
        backup=out.copy_font(current.font);
        if(!backup||backup==current.font)throw Rejected{};
        restore_pending=true;
        // GetHeight每次重新遍历/读取非空块Font。与窄范围指标DTO分别校验，不裁成65536高。
        auto height=[&] {
            float maximum=0;
            for(const auto &block:result.layout.blocks)if(!block.text.empty())
                maximum=std::max(maximum,bounded(f32(block.y+static_cast<float>(used(block.font)))));
            return integer(f32(maximum+0.9F));
        };
        auto smaller=[&] {
            const auto request=sub32(used(current.font),1);
            out.set_size(current.font,request,false);
            layout();
            return request; // 原退出看SetSize实参，不看后继usedSize或首次请求字号。
        };
        if(current.auto_scale&&current.height>0) {
            (void)bounded(current.height);
            if(current.multi_line) {
                while(static_cast<float>(height())>f32(current.height+1.0F))
                    if(smaller()==1)break;
                if(static_cast<float>(height())>current.height)
                    result.height=static_cast<float>(height()); // 源再取一次，不复用比较值。
            } else if(current.width>0) {
                (void)bounded(current.width);
                while(result.layout.text_width>current.width)
                    if(smaller()==1)break;
            }
        }
        std::int32_t length=0;
        for(const auto &block:result.layout.blocks)
            length=add32(length,block.kind==SteamLayoutBlockKind::ordinary?
                static_cast<std::int32_t>(block.text.size()):1);
        result.reveal=steam_text_reveal_set_text_tail(in.reveal,length);
        result.font=backup;
        restore(); // 最后只换布局引用；旧Font对象及其外部别名已经可能被缩字。
        return result;
    } catch(const Rejected &) {cleanup();return {};}
      catch(...) {cleanup();throw;}
}

std::optional<SteamConsoleTextResult> steam_translate_console_text(
        const std::optional<std::u16string> &input,SteamConsoleTextBackend &out) {
    if(input&&input->size()>text_budget)return {};
    if(!out.has_table())return SteamConsoleTextResult{false,input};
    const auto count=out.table_count();
    if(count<0)return {}; // 原字典Count不能为负，不把坏供给当空表。
    if(!count||!out.joystick_is_shown())return SteamConsoleTextResult{false,input};
    if(!input)return {}; // 原启用路径在StartsWith前解引用，不能补空成功。
    auto key=*input;
    std::optional<std::u16string> prefix;
    if(out.starts_with_position_tag(key)) {
        const auto end=key.find(u'>');
        if(end!=std::u16string::npos) {
            prefix=key.substr(0,end+1);
            std::u16string stripped;
            // 无options的固定模式(<po=[a-z]+>|</po>)，只在上述资格内应用。
            for(std::size_t at=0;at<key.size();) {
                if(prefix_at(key,at,u"</po>")) {at+=5;continue;}
                if(prefix_at(key,at,u"<po=")) {
                    auto last=at+4;
                    while(last<key.size()&&key[last]>=u'a'&&key[last]<=u'z')++last;
                    if(last>at+4&&last<key.size()&&key[last]==u'>') {at=last+1;continue;}
                }
                stripped+=key[at++];
            }
            key=std::move(stripped);
        }
    }
    auto trim_cr=[](std::u16string value) {
        while(!value.empty()&&value.back()==u'\r')value.pop_back();
        return value;
    };
    auto hit=out.lookup(key);
    // 10820A74/10820B9E先查Length>0再Last<char>；空key仍查表但不会抛Last异常。
    if(!hit.found&&!key.empty()&&key.back()==u'\r')hit=out.lookup(trim_cr(key));
    // 10820B11–B47即使命中也执行。不得以已Trim的第二查实参覆盖原key。
    auto replace=[](const std::u16string &text,const std::u16string &old_word)
            ->std::optional<std::u16string> {
        std::u16string next;
        for(std::size_t at=0;at<text.size();) {
            if(prefix_at(text,at,old_word)) {next+=u"<br>";at+=old_word.size();}
            else next+=text[at++];
            if(next.size()>text_budget)return {};
        }
        return next;
    };
    auto normalized=replace(key,u"\r\n");
    if(!normalized)return {};
    normalized=replace(*normalized,u"\n");
    if(!normalized)return {};
    if(!hit.found) {
        hit=out.lookup(*normalized);
        if(!hit.found&&!normalized->empty()&&normalized->back()==u'\r')
            hit=out.lookup(trim_cr(*normalized));
    }
    if(!hit.found)return SteamConsoleTextResult{false,input};
    if(hit.value&&hit.value->size()>text_budget)return {};
    if(prefix) {
        // String.Concat(prefix,null)为prefix；无prefix时原out可保持null。
        if(hit.value&&hit.value->size()>text_budget-prefix->size())return {};
        if(hit.value)*prefix+=*hit.value;
        return SteamConsoleTextResult{true,std::move(prefix)};
    }
    return SteamConsoleTextResult{true,std::move(hit.value)};
}

std::int32_t steam_text_string_hash(const std::u16string &text) {
    std::uint32_t first=5381U,second=5381U;
    bool to_second=false;
    for(const auto unit:text) {
        if(!unit)break; // 108E8782/8799零终止，不以.NET String.Length截取内嵌零后缀。
        auto &accumulator=to_second?second:first;
        accumulator=(accumulator*33U)^static_cast<std::uint32_t>(unit);
        to_second=!to_second;
    }
    return signed32(first+second*0x5D588B65U);
}

std::optional<SteamPreparedText> steam_prepare_text(
        const std::optional<std::u16string> &input,
        const std::vector<SteamTextReplaceWord> &words,SteamTextInputBackend &out) {
    if((input&&input->size()>text_budget)||words.size()>4096)return {};
    auto text=input.value_or(u"");
    std::uint32_t replacements{};
    for(const auto &word:words) {
        // StringUtil.Replace 1086A397–3A6：null参数原样返回，不读取未用串长。
        if(!word.old_word||!word.new_word)continue;
        if(word.old_word->empty()||word.old_word->size()>text_budget||word.new_word->size()>text_budget)return {};
        std::size_t start=0;
        for(;;) {
            const auto at=text.find(*word.old_word,start); // Ordinal=4，原UTF-16单元精确查找。
            if(at==std::u16string::npos)break;
            if(++replacements>65536||word.new_word->size()>text_budget-(text.size()-word.old_word->size()))return {};
            text.replace(at,word.old_word->size(),*word.new_word);
            start=at+word.new_word->size(); // 不从头递归替换刚写入的new_word。
        }
    }
    SteamPreparedText result;
    result.hash=out.hash_code(text); // 107CC1DD早于console；即使后续串改变也不重算。
    auto console=out.translate_console_text(text);
    if(!console.out_text||console.out_text->size()>text_budget)return {};
    text=std::move(*console.out_text);
    using Comparison=SteamTextComparison;
    if(!out.contains(text,u"<ff>",Comparison::current_culture_ignore_case)) {
        result.text=std::move(text);return result;
    }
    std::vector<std::u16string> parts;
    std::size_t begin=0;
    for(;;) {
        const auto lower=text.find(u"<ff>",begin),upper=text.find(u"<FF>",begin);
        const auto end=std::min(lower,upper); // 原精确string[] Split，不是ignore-case分隔。
        const auto length=(end==std::u16string::npos?text.size():end)-begin;
        if(length)parts.emplace_back(text.substr(begin,length)); // RemoveEmptyEntries，不Trim。
        if(end==std::u16string::npos)break;
        begin=end+4;
    }
    auto append=[&](const std::u16string &value) {
        if(value.size()>text_budget-result.text.size())return false;
        result.text+=value;return true;
    };
    for(std::size_t i=0;i<parts.size();++i) {
        auto &part=parts[i];
        if(i&&!out.starts_with(part,u"<br>",Comparison::current_culture_ignore_case)&&
              !out.starts_with(part,u"\n",Comparison::current_culture))part=u"<br>"+part;
        if(i+1<parts.size()&&!out.ends_with(part,u"<br>",Comparison::current_culture_ignore_case)&&
                              !out.ends_with(part,u"\n",Comparison::current_culture))part+=u"<br>";
        if(i&&!append(u"<ff>"))return {};
        if(!append(part))return {};
    }
    return result;
}

std::optional<SteamTextDrawReflow> steam_reflow_text_for_draw(
        const SteamTextLayoutInput &current,float graphics_scale,SteamTextLayoutBackend &out) {
    if(graphics_scale==current.scale_ratio)return SteamTextDrawReflow{current.scale_ratio,{}};
    // 原无序比较也会重排；维护拒绝不能实际消费的新非有限比例，不借空串早退伪装有效。
    if(!std::isfinite(graphics_scale)||std::abs(graphics_scale)>16777216.F)return {};
    auto next=current;
    next.scale_ratio=graphics_scale;
    auto layout=steam_make_text_blocks(next,out);
    if(!layout)return {};
    return SteamTextDrawReflow{graphics_scale,std::move(layout)};
}

std::optional<std::int32_t> steam_draw_text_block_sequence(
        const std::vector<SteamLayoutBlock> &blocks,std::int32_t pos,float dx,float dy,
        std::int32_t scroll_v,float box_height,SteamTextBlockDrawBackend &out) {
    if(blocks.size()>block_budget)return {};
    auto budget=pos/1000; // 107CA040–50 signed截零，不以length钳位或调用Frame。
    if(blocks.empty()||budget<=0)return budget;
    const auto scrolled_y=f32(dy-static_cast<float>(scroll_v));
    if(!std::isfinite(dx)||!std::isfinite(dy)||!std::isfinite(scrolled_y)||!std::isfinite(box_height))return {};
    for(const auto &block:blocks) {
        if(budget<=0)break;
        const auto consumed=out.draw_block(block,{dx,scrolled_y,dy,box_height,budget});
        budget=sub32(budget,consumed); // 源无min/非负修补；后继仍看signed预算。
    }
    return budget;
}
} // namespace dungeon_village_prototype
