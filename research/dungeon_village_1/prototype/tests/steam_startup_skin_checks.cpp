#include "dungeon_village_prototype/steam_startup_skin.hpp"
#include "dungeon_village_prototype/steam_main_menu_skin.hpp"
#include "dungeon_village_prototype/steam_manual_skin.hpp"
#include "dungeon_village_prototype/steam_record_skin.hpp"
#include "dungeon_village_prototype/steam_font_metrics.hpp"
#include "dungeon_village_prototype/steam_dialogue_skin.hpp"
#include "dungeon_village_prototype/steam_event_skin.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_prototype/steam_news_skin.hpp"
#include "dungeon_village_prototype/steam_touch_commands.hpp"
#include "dungeon_village_prototype/startup_world_projection.hpp"
#include "support/world_fixture.hpp"
#include "dungeon_village_tools/sprite.hpp"
#include <raylib.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

using namespace dungeon_village_prototype;
namespace {
struct Checks {
    int count{};
    void operator()(bool ok, const char *message) {
        ++count;
        if(!ok) throw std::runtime_error(std::string("Steam启动皮肤：")+message);
    }
};
template<class T, class Plan> std::vector<T> parts(const Plan &p) {
    std::vector<T> result;
    for(const auto &draw:p.draws) if(const auto *item=std::get_if<T>(&draw)) result.push_back(*item);
    return result;
}
auto sprite(const std::filesystem::path &p) {
    std::ifstream input(p,std::ios::binary);
    if(!input) throw std::runtime_error("Steam皮肤公共资源缺失："+p.string());
    return dungeon_village_tools::parse_legacy_seb({std::istreambuf_iterator<char>(input),{}});
}
void touch_commands(Checks &check) {
    SteamTouchCommandInput in;in.admission=SteamTouchAdmission::ready;
    for(int id:std::array<int,3>{0,2,9})for(int event=0;event<14;++event) {
        in.component=id;in.event_type=event;in.value=0x28000;in.extended_values={7};
        const auto plan=steam_touch_commands(in);
        const std::size_t count=event==2?2:event==1?(id==0?1:3):0;
        check(plan&&plan->commands.size()==count,"标题与菜单事件分母：仅ENTER/UP各有原分工");
        if(event==2||(event==1&&id!=0)) {
            const auto &write=std::get<SteamTouchValueWrite>(plan->commands[0]);
            check(write.type==(id==0?0x30000U:0x20000U)&&write.value==(id==0?0x28000:-32768)&&
                      std::get<SteamTouchValueWrite>(plan->commands[1]).type==0,
                  "标题ENTER不拆完整int值；菜单先signed short主写再type0扩展");
        }
        if(event==1)check(std::get<SteamTouchKeyClick>(plan->commands.back()).bits==0x100000U,
                         "标题UP不Set，菜单UP在Set之后按缺省确认键请求");
    }
    in.extended_values.assign(4097,0);
    for(const auto key:std::array<std::uint32_t,3>{0,0xffffffffU,0x80000010U}) {
        in.component=0;in.event_type=1;in.first_key=key;
        auto p=steam_touch_commands(in);
        check(p&&p->commands.size()==1&&std::get<SteamTouchKeyClick>(p->commands[0]).bits==key,
              "标题UP原样消费首自定义键，不读超预算未消费扩展");
        in.extended_values.clear();in.component=9;in.value=0;
        p=steam_touch_commands(in);
        check(p&&p->commands.size()==1&&std::get<SteamTouchKeyClick>(p->commands[0]).bits==key,
              "菜单值0跳主写并保持自定义键，不套KEYCLICK默认键号20");
        in.extended_values.assign(4097,0);
    }
    in.component=2;
    check(!steam_touch_commands(in),"菜单UP必须消费Set，扩展超预算时不得泄出确认");
    in.component=0;in.event_type=2;
    check(!steam_touch_commands(in),"标题ENTER必须消费Set，扩展超预算整体拒绝");
    in.extended_values.clear();in.first_key.reset();
    for(int id:std::array<int,2>{1,18})for(int event=0;event<14;++event) {
        in.component=id;in.event_type=event;in.value=16;
        const auto component_key=(static_cast<std::uint64_t>(id)<<32)|16U;
        in.arrow_paint=SteamTouchArrowPaint{37,-12,3,component_key};
        const auto p=steam_touch_commands(in);
        const std::size_t count=event==0?2:event==6||event==10?1:0;
        check(p&&p->commands.size()==count,"DLGPAGE/ARROW仅DOWN建效果，LOST/CANCEL才释放");
        if(event==0) {
            const auto &down=std::get<SteamTouchKeyDown>(p->commands[0]);
            const auto &effect=std::get<SteamTouchArrowEffect>(p->commands[1]);
            check(down.bits==0x10000U&&down.pulse&&effect.paint.x==37&&effect.paint.y==-12&&
                      effect.paint.component_key==component_key&&effect.texture_row==2&&effect.velocity_x==-10.f,
                  "箭头先Down再id0效果，取实际paint而非命中坐标，原value16向左");
        } else if(count)check(std::get<SteamTouchKeyUp>(p->commands[0]).bits==0x10000U,
                              "箭头释放沿同一逻辑位，无额外Set或效果");
    }
    in.component=1;in.event_type=0;in.value=0;in.arrow_paint->frame=4;
    in.arrow_paint->component_key=0x100000000ULL;
    auto arrow=steam_touch_commands(in);
    check(arrow&&std::get<SteamTouchKeyDown>(arrow->commands[0]).bits==1&&
              std::get<SteamTouchArrowEffect>(arrow->commands[1]).texture_row==3&&
              std::get<SteamTouchArrowEffect>(arrow->commands[1]).velocity_x==10.f,
          "箭头值0不默认20，paintFrame非3改图3，方向不按键名猜");
    in.value=16;in.first_key=0xffffffffU;in.arrow_paint->component_key=0x100000010ULL;
    arrow=steam_touch_commands(in);
    check(arrow&&std::get<SteamTouchKeyDown>(arrow->commands[0]).bits==0xffffffffU&&
              std::get<SteamTouchArrowEffect>(arrow->commands[1]).velocity_x==-10.f,
          "自定义负一仍保留后继效果请求，方向只读原value而非custom键");
    in.arrow_paint.reset();check(!steam_touch_commands(in),"箭头DOWN缺真实paint拒绝整个候选");
    in.event_type=10;in.extended_values.assign(4097,0);
    check(steam_touch_commands(in)->commands.size()==1,"箭头取消不消费paint或Set扩展");
    in.extended_values.clear();in.first_key.reset();
    // 固定DLL跳表及各具名入口的触发事件；不从被测函数生成期望资格。
    const std::array<std::array<int,2>,6> triggers{{{4,5},{5,0},{6,0},{7,1},{20,5},{21,5}}};
    for(const auto &entry:triggers)for(int event=0;event<14;++event) {
        in.component=entry[0];in.event_type=event;in.value=16;
        const auto plan=steam_touch_commands(in);
        const bool release=in.component==5&&(event==6||event==10);
        check(plan&&plan->commands.size()==static_cast<std::size_t>(event==entry[1]||release),
              "公共键组件各有真实触发事件，不按KEYUP名称合成释放");
        if(plan->commands.empty())continue;
        const auto &command=plan->commands.front();
        if(release)check(std::get<SteamTouchKeyUp>(command).bits==0x10000U,
                         "KEYPRESS仅LOST/CANCEL释放原方向逻辑位");
        else if(in.component==4||in.component==20||in.component==21)
            check(std::get<SteamTouchKeyClick>(command).bits==0x10000U,
                  "KEYCLICK和PLUS/MINUS保留相同Click请求，不擅自加减数值");
        else {
            const auto &down=std::get<SteamTouchKeyDown>(command);
            check(down.bits==0x10000U&&down.pulse==(in.component!=5),
                  "KEYDOWN及触摸UP触发的KEYUP均KeyDown(true)，KEYPRESS为false");
        }
    }
    in.component=7;in.event_type=1;
    for(const auto values:std::array<std::array<std::uint32_t,2>,5>{{
            {0,0x100000U},{20,0x100000U},{31,0x80000000U},{32,1},{65535,0x80000000U}}}) {
        in.value=static_cast<std::int32_t>(values[0]);
        const auto plan=steam_touch_commands(in);
        check(plan&&plan->commands.size()==1&&std::get<SteamTouchKeyDown>(plan->commands[0]).bits==values[1],
              "值0默认键号20，移位按低五位且bit31无有符号溢出");
    }
    for(const auto key:std::array<std::uint32_t,3>{0,0xffffffffU,0x80000010U}) {
        in.first_key=key;
        const auto plan=steam_touch_commands(in);
        check(plan&&std::get<SteamTouchKeyDown>(plan->commands[0]).bits==key,
              "首自定义键0/负一/复合位原样请求，Canvas实际资格由后继执行器处理");
    }
    in.first_key=0;in.value=0x20004;
    in.extended_values={0x20002,7,-32768,-1};
    auto plan=steam_touch_commands(in);
    const std::array<SteamTouchValueWrite,5> expected{{{0x20000,4},{0x20000,2},{0,7},{0xffff0000U,-32768},{0xffff0000U,-1}}};
    check(plan&&plan->commands.size()==expected.size(),"属性主项及扩展按原序，不合并同类型或丢零类型");
    for(std::size_t n=0;n<expected.size();++n) {
        const auto &write=std::get<SteamTouchValueWrite>(plan->commands[n]);
        check(write.type==expected[n].type&&write.value==expected[n].value,
              "属性低16位按有符号short，类型保留高16位位型");
    }
    in.component=5;in.event_type=6;
    check(steam_touch_commands(in)->commands.empty(),"KEYPRESS属性请求的LOST不写扩展或发KeyUp");
    in.component=24;in.event_type=2;
    plan=steam_touch_commands(in);
    check(plan&&plan->commands.size()==5&&std::holds_alternative<SteamTouchValueWrite>(plan->commands.back()),
          "RECT的ENTER写属性及扩展但不确认");
    in.event_type=1;in.first_key=0;
    plan=steam_touch_commands(in);
    check(plan&&plan->commands.size()==6&&std::get<SteamTouchKeyClick>(plan->commands.back()).bits==0x100000,
          "RECT的UP先完整写值再固定确认，不消费自定义键或标记资格");
    in.component=22;in.event_type=2;
    check(steam_touch_commands(in)->commands.empty(),"OVER不按ENTER冒充悬停确认");
    in.event_type=13;
    check(steam_touch_commands(in)->commands.size()==5,"OVER仅ON输出Set及扩展");
    in.component=19;in.event_type=9;in.value=0x80001;in.delta_x=-123;
    plan=steam_touch_commands(in);
    check(plan&&plan->commands.size()==5,"TRACKBAR有效主类型和扩展产生完整写序列");
    const auto &track=std::get<SteamTouchValueWrite>(plan->commands[0]);
    check(track.type==0x80000&&track.value==-123,
          "TRACKBAR的Set仍掩主类型高16位，事件dx不拆短数/夹范围/换百分比");
    in.extended_values.clear();in.value=7;
    check(steam_touch_commands(in)->commands.empty(),"TRACKBAR纯低位在Set后主type0跳写，不能补默认键");
    in.value=0;
    check(steam_touch_commands(in)->commands.empty(),"TRACKBAR值0不能补默认键号20");
    in.extended_values={7};
    check(steam_touch_commands(in)->commands.size()==1&&
              std::get<SteamTouchValueWrite>(steam_touch_commands(in)->commands.front()).type==0,
          "主type0也不阻止扩展type0按原序调用");
    in.extended_values.assign(4097,0);in.component=24;in.event_type=1;
    check(!steam_touch_commands(in),"消费扩展超过维护预算拒绝整个候选，不留半份属性或确认输出");
    in.extended_values.resize(4096);
    check(steam_touch_commands(in)->commands.size()==4097,"维护预算边界仍保留全部零类型扩展及末确认");
    in.extended_values.emplace_back(0);
    in.component=6;in.event_type=0;in.value=20;
    check(steam_touch_commands(in)->commands.size()==1,"键分支不消费扩展，预算不误拒未用数据");
    in.admission=SteamTouchAdmission::intercepted;in.component=-1;in.event_type=99;
    check(steam_touch_commands(in)->commands.empty(),"顶页截获先短路，通用组件与坏事件均不再读取");
    in.admission=SteamTouchAdmission::unverified;
    check(!steam_touch_commands(in),"未证surface/顶页准入不能生成通用请求");
    in.admission=SteamTouchAdmission::ready;in.component=8;in.event_type=1;
    check(!steam_touch_commands(in),"主菜单有独立消费者，不套成公共键叶");
    in.component=16;
    check(!steam_touch_commands(in),"解除标记有独立生命周期，不套成公共键叶");
    in.component=6;in.event_type=14;
    check(!steam_touch_commands(in),"未知事件显式拒绝");
}
void touch_menu_and_marker(Checks &check) {
    SteamTouchMainMenuInput menu;
    menu.admission=SteamTouchAdmission::ready;
    menu.value=0x20004;menu.top_form=900;menu.has_submenu_component=false;
    menu.ordered_forms=std::vector<SteamTouchFormObservation>{{100,9,{}},{200,4,3},{300,4,3},{0,4,{}}};
    for(int event=0;event<14;++event) {
        menu.event_type=event;
        const auto p=steam_touch_main_menu(menu);
        check(p&&p->main_menu==200&&p->probes[0].form==100&&p->probes[1].form==200,
              "MAINMENU先正序绑定第一个真实raw3，短路后坏尾项不消费");
        if(event!=1&&event!=2) {
            check(p->probes.size()==2&&p->commands.empty(),"非ENTER/UP仅扫描，不查询组件或顶页");
            continue;
        }
        check(p->probes.size()==4&&p->probes[2].kind==SteamTouchMainMenuProbeKind::submenu_component&&
                  p->probes[3].kind==SteamTouchMainMenuProbeKind::top_form,
              "无9时先查组件存在再取得当前顶页accessor");
        const auto &write=std::get<SteamTouchAccessorWrite>(p->commands[0]);
        check(write.accessor==900&&write.write.type==0x20000&&write.write.value==4&&
                  p->commands.size()==static_cast<std::size_t>(event==1?2:1),
              "无9向当时顶页而非扫描到的父菜单写行，只有UP附确认");
        if(event==1)check(std::get<SteamTouchKeyClick>(p->commands.back()).bits==0x100000U,
                         "MAINMENU的UP固定确认逻辑位，不消费自定义keys");
    }
    menu.event_type=1;menu.has_submenu_component=true;menu.main_menu_value=0;
    menu.extended_values={0x20002,7,-32768,-1};
    auto p=steam_touch_main_menu(menu);
    check(p&&p->probes.size()==5&&p->probes[3].kind==SteamTouchMainMenuProbeKind::main_menu_value&&
              p->probes[3].form==200&&p->probes[3].type==0x20000&&
              p->probes[4].kind==SteamTouchMainMenuProbeKind::top_form&&p->commands.size()==6&&
              std::get<SteamTouchFormPop>(p->commands.front()).form==900,
          "有9时先读既定父菜单，变化才取顶页并请求Pop，随后不发确认");
    const std::array<SteamTouchValueWrite,5> expected{{{0x20000,4},{0x20000,2},{0,7},{0xffff0000U,-32768},{0xffff0000U,-1}}};
    for(std::size_t i=0;i<expected.size();++i) {
        const auto &write=std::get<SteamTouchAccessorWrite>(p->commands[i+1]);
        check(write.accessor==200&&write.write.type==expected[i].type&&write.write.value==expected[i].value,
              "Pop后的主项/扩展仍绑定原首主菜单，重复属性和type0保留原序");
    }
    menu.top_form=0;menu.event_type=2;
    p=steam_touch_main_menu(menu);
    check(p&&std::get<SteamTouchFormPop>(p->commands[0]).form==0&&
              std::get<SteamTouchAccessorWrite>(p->commands[1]).accessor==200&&p->commands.size()==6,
          "有9的ENTER保留原Pop空目标请求及既定父写，不据此承诺弹栈成功或发确认");
    menu.event_type=1;
    menu.value=0x2ffff;menu.main_menu_value=65535;menu.top_form.reset();
    menu.extended_values.assign(4097,0);
    p=steam_touch_main_menu(menu);
    check(p&&p->commands.empty()&&p->probes.size()==4,
          "有9的相等比较取低16位无符号，短路不读缺顶页或超预算扩展");
    menu.main_menu_value=-1;menu.top_form=900;menu.extended_values.clear();
    p=steam_touch_main_menu(menu);
    check(p&&std::get<SteamTouchAccessorWrite>(p->commands[1]).write.value==-1,
          "65535比较与真正Set的short负一保持不同语义");
    menu.ordered_forms=std::vector<SteamTouchFormObservation>{{100,9,{}}};
    check(!steam_touch_main_menu(menu),"有9却没有真实父raw3时拒绝，不以顶页替代");
    menu.has_submenu_component=false;
    check(steam_touch_main_menu(menu)->commands.size()==2,"无9不要求父raw3存在，只使用已查询顶页");
    menu.value=0;menu.top_form=0;
    check(steam_touch_main_menu(menu)->commands.size()==1,
          "实际空顶页的type0无扩展Set仍可只请求UP确认，不造属性目标");
    menu.extended_values={0};
    check(!steam_touch_main_menu(menu),"空accessor不能吞掉type0扩展写");
    menu.top_form=900;menu.value=0x20004;menu.extended_values.assign(4097,0);
    check(!steam_touch_main_menu(menu),"超预算Set整体拒绝，不泄出Pop或确认半计划");
    menu.extended_values.resize(4096);
    check(steam_touch_main_menu(menu)->commands.size()==4098,"扩展维护预算边界保留主写/全部扩展/确认");
    menu.event_type=5;menu.ordered_forms=std::vector<SteamTouchFormObservation>(4096,{100,9,{}});
    check(steam_touch_main_menu(menu)->probes.size()==4096,"无父菜单的扫描维护预算边界可完成");
    menu.ordered_forms->push_back({200,4,3});
    check(!steam_touch_main_menu(menu),"实际首父位于扫描预算之外明确拒绝");
    menu.ordered_forms->front()={200,4,3};
    check(steam_touch_main_menu(menu)->probes.size()==1,"早找到首父不校验大列表未扫描的尾部");
    menu.ordered_forms->front()={200,4,{}};
    check(!steam_touch_main_menu(menu),"已扫formID4缺真实子页类型明确拒绝");
    menu.admission=SteamTouchAdmission::intercepted;menu.event_type=99;menu.ordered_forms.reset();
    check(steam_touch_main_menu(menu)->commands.empty(),"主菜单顶页截获不再读坏事件/缺表单观察");
    menu.admission=SteamTouchAdmission::ready;
    check(!steam_touch_main_menu(menu),"主菜单未获有效事件与扫描观察不得成功");

    SteamTouchUnmarkerInput marker;marker.admission=SteamTouchAdmission::ready;marker.marker_key=-2;
    // 来源按12/14/15/旧key短路，期望查询顺序独立固定，不根据被测计划生成。
    for(int event:std::array<int,2>{0,1})for(int hit=0;hit<5;++hit) {
        marker.event_type=event;marker.scrollbar_events={false,false,false};marker.marker_event=false;
        if(hit<3) {
            marker.scrollbar_events[hit]=true;
            for(int i=hit+1;i<3;++i)marker.scrollbar_events[i].reset();
            marker.marker_event.reset();
        } else if(hit==3)marker.marker_event=true;
        const auto q=steam_touch_unmarker(marker);
        const std::size_t count=hit<3?static_cast<std::size_t>(hit+1):4;
        check(q&&q->probes.size()==count&&q->commands.size()==static_cast<std::size_t>((event==0?1:0)+(hit==4?1:0)),
              "解除标记按命中短路，不读取后续缺观察；DOWN重置和最终清除分别输出");
        const std::array<int,3> ids{12,14,15};
        for(std::size_t i=0;i<std::min<std::size_t>(3,count);++i)
            check(q->probes[i].kind==SteamTouchUnmarkerProbeKind::component&&
                      q->probes[i].component==ids[i]&&q->probes[i].event_type==event,
                  "解除查询固定12→14→15，事件边沿不替换");
        if(count==4)check(q->probes.back().kind==SteamTouchUnmarkerProbeKind::marker_key&&q->probes.back().key==-2,
                          "除-1外的负标记键原位型交真实key查询");
        if(event==0)check(std::holds_alternative<SteamTouchMarkerFrameReset>(q->commands.front()),
                         "DOWN无论后续是否保留标记，第一条动作始终只重置frame");
        if(hit==4)check(std::holds_alternative<SteamTouchMarkerClear>(q->commands.back()),
                       "当前surface均无匹配事件才请求ClearMarker");
    }
    marker.marker_key=-1;marker.scrollbar_events={false,false,false};marker.marker_event.reset();
    for(int event=0;event<14;++event) {
        marker.event_type=event;const auto q=steam_touch_unmarker(marker);
        check(q&&q->probes.size()==static_cast<std::size_t>(event==0?3:0)&&
                  q->commands.size()==static_cast<std::size_t>(event==0?2:0),
              "无标记仅DOWN查询三种滚动组件并清除；UP/DRAG和其它事件无查询");
    }
    marker.event_type=7;marker.marker_key=std::numeric_limits<std::int64_t>::min();
    marker.scrollbar_events={};marker.marker_event=true;
    auto q=steam_touch_unmarker(marker);
    check(q&&q->commands.empty()&&q->probes.size()==1&&q->probes[0].key==marker.marker_key,
          "DRAG只查旧key，保留64位最低负边界且不触及滚动查询");
    marker.marker_event=false;q=steam_touch_unmarker(marker);
    check(q&&q->commands.size()==1&&std::holds_alternative<SteamTouchMarkerClear>(q->commands[0]),
          "DRAG无旧key对应事件则清除，不额外重置动作或发键");
    marker.marker_event.reset();
    check(!steam_touch_unmarker(marker),"必须查询的marker事件缺观测显式拒绝");
    marker.event_type=0;
    check(!steam_touch_unmarker(marker),"DOWN第一条必需滚动查询缺失时不返回部分重置计划");
    marker.admission=SteamTouchAdmission::intercepted;marker.event_type=99;
    check(steam_touch_unmarker(marker)->probes.empty(),"顶页截获后解除层不读事件或查询");
    marker.admission=SteamTouchAdmission::unverified;
    check(!steam_touch_unmarker(marker),"未证准入不能制造清除标记请求");
    marker.admission=SteamTouchAdmission::ready;marker.event_type=14;
    check(!steam_touch_unmarker(marker),"解除层拒绝原14事件之外的编号");
}
// raw0按原调用常量验证局部组合；不重复公共木框、人物scratch或TextLayout后端测试。
void dialogue_page(Checks &check) {
    SteamDialogueSkinInput in;
    in.width=239;in.height=261;in.view_y=23;in.main=197;
    in.title="会话";in.speaker_name="当前姓名";in.title_widths=std::array<int,2>{24,24};
    for(const auto ids:std::array<std::array<int,3>,3>{{{-1,126,87},{2,173,38},{3,176,95}}}) {
        in.speaker_index=ids[0];
        for(bool english:{false,true}) {
            in.english=english;
            const auto p=steam_dialogue_skin(in);
            check(p&&p->origin==std::array<int,2>{0,10}&&p->title=="会话",
                  "raw0外层共同居中条件保留负奇数向零；VIEW_Y不重复加原点");
            const auto images=parts<StartupSkinDraw>(*p);
            const auto names=parts<SteamDialogueNameDraw>(*p);
            const auto fonts=parts<SteamDialogueFont>(*p);
            check(images.size()==2&&images[0].image==143&&images[0].sprite==-1&&
                  images[0].crop==std::array<int,4>{0,0,7,7}&&images[0].offset==std::array<int,2>{174,169}&&
                  images[1].image==ids[1]&&images[1].sprite==ids[2]&&images[1].frame==1&&
                  images[1].offset==std::array<int,2>{193,189},
                  "point整图后接秘书/南瓜/大臣原SEB frame1；不能按main猜头像动画");
            check(names.size()==1&&names[0].text=="当前姓名"&&names[0].rectangle==
                  std::array<int,4>{53,english?175:174,120,11}&&names[0].anchor==34&&names[0].line_space==0&&
                  parts<SteamDialogueBodyDraw>(*p).size()==1&&
                  parts<SteamDialogueBodyDraw>(*p)[0].position==std::array<int,2>{37,100},
                  "Steam姓名float位型x53独立oracle，不能套APK84；正文保留实例Draw");
            check(fonts.size()==(english?2U:0U)&&(!english||
                  (fonts[0].action==SteamDialogueFontAction::set_size10&&fonts[1].action==SteamDialogueFontAction::reset))&&
                  std::holds_alternative<SteamStartupTouch>(p->draws.back())&&
                  parts<SteamStartupTouch>(*p)[0].component==2&&!parts<SteamStartupTouch>(*p)[0].rectangle,
                  "仅English临时字体10再恢复，最后无矩形AddTouch2不造按钮或取消热区");
            const auto body=std::find_if(p->draws.begin(),p->draws.end(),[](const auto &v){return std::holds_alternative<SteamDialogueBodyDraw>(v);});
            const auto avatar=std::find_if(p->draws.begin(),p->draws.end(),[](const auto &v){
                const auto *image=std::get_if<StartupSkinDraw>(&v);return image&&image->sprite>=0;});
            check(body<avatar&&(!english||std::holds_alternative<SteamDialogueFont>(*(body+1))),
                  "正文在临时字体与头像之前，字形后端不能重排图像/文字层");
        }
    }
    in.english=false;in.speaker_kind=1;in.speaker_index=9;
    check(!steam_dialogue_skin(in),"真实人物缺当前外观不回退出生表");
    in.human=SteamDialogueHuman{9,28,5};
    const auto human=steam_dialogue_skin(in);
    const auto sets=parts<SteamDialogueHumanSet>(*human);
    const auto humans=parts<SteamDialogueHumanDraw>(*human);
    check(sets.size()==1&&sets[0].definition==9&&sets[0].body_sprite==28&&sets[0].action==0&&
          sets[0].step==0&&sets[0].direction==2&&sets[0].main==197&&humans.size()==1&&humans[0].weapon==5&&
          humans[0].step==0&&humans[0].position==std::array<int,2>{193,189},
          "人物当前身体/武器显式传递，Set/Draw各原参数，不执行共享scratch");
    in.human->definition=8;check(!steam_dialogue_skin(in),"错人物外观绑定拒绝");
    in.speaker_kind=2;in.speaker_index=4;
    check(!steam_dialogue_skin(in),"怪物缺真实bit4结果拒绝");
    for(bool bit:{false,true}) {
        in.monster_flag4=bit;const auto p=steam_dialogue_skin(in);
        const auto monsters=parts<SteamDialogueMonsterDraw>(*p);
        check(monsters.size()==1&&monsters[0].definition==4&&monsters[0].direction==2&&
              monsters[0].position==std::array<int,2>{bit?188:177,188},"怪物bit4两头像锚独立原常量");
    }
    in.speaker_kind=1;in.speaker_index=-1;
    check(parts<SteamDialogueNameDraw>(*steam_dialogue_skin(in)).empty(),"原人物负索引不画姓名头像，不补默认秘书");
    in.speaker_kind=2;in.speaker_index=-1;
    check(!steam_dialogue_skin(in),"可见怪物负索引拒绝");
    for(int gate=0;gate<3;++gate) {
        auto hidden=in;hidden.title_widths.reset();hidden.speaker_name=std::string(4097,'x');
        hidden.on_top=gate!=0;hidden.until_active_hide=gate==1;hidden.wait=gate==2?1:0;
        check(steam_dialogue_skin(hidden)&&steam_dialogue_skin(hidden)->draws.empty(),
              "覆盖/untilActiveHide/wait各自隐藏且不消费缺失头像或测宽");
    }
    using R=SteamDialogueUpdateRequest;
    SteamDialogueUpdateInput u;u.paragraphs={"首段<br>原标签","第二段"};
    auto p=steam_dialogue_update(u);
    check(p&&p->requests==std::vector<R>{R::frame,R::confirm_pulse,R::debug_gate}&&p->paragraph==0&&
          !p->next_text&&p->normal_return==true,"无确认不消费IsEnd，Frame先于键查询且无取消分支");
    u.debug_held=true;
    check(steam_dialogue_update(u)->requests==p->requests,"DEBUG未开不查询Held，不让Held偷过资格");
    u.debug_enabled=true;
    check(!steam_dialogue_update(u),"确认资格已成立却缺Frame之后IsEnd真实观测应拒绝");
    u.ended_after_frame=false;p=steam_dialogue_update(u);
    check(p&&p->requests==std::vector<R>{R::frame,R::confirm_pulse,R::debug_gate,R::debug_held,R::is_end,R::flush}&&
          p->paragraph==0&&!p->next_text,"DEBUG合格只补完本段，不用字符串长度或main计数猜逐字结束");
    u.confirm_pulse=true;u.ended_after_frame=true;p=steam_dialogue_update(u);
    check(p&&p->requests==std::vector<R>{R::frame,R::confirm_pulse,R::is_end,R::set_text}&&
          p->paragraph==1&&p->next_text==std::optional<std::string>{"第二段"},
          "Pulse优先跳过DEBUG查询，展开完才增段并SetText，不额外LT或关闭");
    u.paragraph=1;p=steam_dialogue_update(u);
    check(p&&p->paragraph==2&&p->requests.back()==R::pop&&!p->normal_return&&!p->next_text,
          "末段确认请求Pop并保持其返回未知，不把候选当成功关闭");
    u.next_talk=3;p=steam_dialogue_update(u);
    check(p&&p->requests.back()==R::unsupported_next_talk&&!p->normal_return,
          "未支持续记录显式输出，不能套四字段原表或假装Pop成功");
    u.paragraph=-1;check(!steam_dialogue_update(u),"负段号拒绝");
    u.paragraph=2;check(!steam_dialogue_update(u),"越界段号拒绝");
    u.paragraphs.clear();u.paragraph=0;check(!steam_dialogue_update(u),"空正文拒绝，不补演示句");
}
// 事件卡主责原序、独立数组采样和严格只读桥；奖励/声音时点仍由pages套件负责。
void event_card(Checks &check,const std::filesystem::path &root) {
    SteamEventSkinInput in{0,0,"正文<br><co=0064FF>原标签</co>"};
    SteamEventSkinOptions options;options.title="事件";options.title_widths=std::array<int,2>{24,24};
    options.width=239;options.height=261;options.view_y=23;
    auto p=steam_event_skin(in,options);
    check(p&&p->origin==std::array<int,2>{0,10}&&p->soft_labels==std::array<int,2>{0,0},
          "11两软标签皆0且标准零位页共同居中，VIEW_Y不重复进origin");
    const auto first_image=std::find_if(p->draws.begin(),p->draws.end(),[](const auto &v){return std::holds_alternative<StartupSkinDraw>(v);});
    check(first_image!=p->draws.end(),"11背景请求存在");
    const auto at=static_cast<std::size_t>(first_image-p->draws.begin());
    const auto &background=std::get<StartupSkinDraw>(p->draws.at(at));
    check(background.package==StartupSkinPackage::event&&background.image==9&&background.sprite==-1&&
          background.crop==std::array<int,4>{0,0,180,80}&&background.offset==std::array<int,2>{30,67}&&
          std::get<SteamEventClip>(p->draws.at(at+1)).rectangle==std::array<int,4>{0,159,240,0}&&
          std::get<SteamEventTextLayout>(p->draws.at(at+2)).text==in.text&&
          std::get<SteamEventTextLayout>(p->draws.at(at+2)).rectangle==std::array<int,4>{28,159,182,46}&&
          std::get<SteamEventTextLayout>(p->draws.at(at+2)).anchor==34&&
          std::get<SteamEventTextLayout>(p->draws.at(at+2)).line_space==6&&
          std::get<SteamEventClip>(p->draws.at(at+3)).kind==SteamEventClipKind::pop&&
          std::get<SteamEventClip>(p->draws.at(at+4)).rectangle==std::array<int,4>{0,67,240,79},
          "11窗/box后背景→真正文clip/标签TextLayout/恢复→人物clip，零高不漏掉裁剪");
    const auto actors=parts<StartupSkinDraw>(*p);
    check(actors.size()==5&&actors[1].image==1&&actors[1].sprite==0&&actors[1].frame==0&&
          actors[1].offset==std::array<int,2>{65,161}&&actors[2].offset==std::array<int,2>{120,161}&&
          actors[3].offset==std::array<int,2>{175,161}&&actors[4].image==7&&
          actors[4].crop==std::array<int,4>{0,0,180,24}&&actors[4].offset==std::array<int,2>{30,67}&&
          std::get<SteamEventClip>(p->draws.at(p->draws.size()-2)).kind==SteamEventClipKind::pop&&
          std::get<SteamStartupTouch>(p->draws.back()).component==2&&
          !std::get<SteamStartupTouch>(p->draws.back()).rectangle,
          "11Happy三人x65/120/175与OY首13，三人后烟花裁片，恢复clip后无矩形Touch2");
    struct Sample {int main,happy,sad,reveal;bool marker;};
    constexpr std::array<Sample,10> samples{{{4,158,146,0,false},{5,158,146,0,false},
        {6,157,146,1,false},{10,154,147,6,false},{22,148,151,23,false},
        {39,151,147,46,false},{40,151,146,48,false},{52,148,148,48,false},
        {60,148,148,48,false},{61,148,148,48,true}}};
    for(const auto &sample:samples)for(int command:{0,1}) {
        in.main=sample.main;in.command=command;p=steam_event_skin(in,options);
        const auto images=parts<StartupSkinDraw>(*p);const auto clips=parts<SteamEventClip>(*p);
        const auto actor=std::find_if(images.begin(),images.end(),[](const auto &v){return v.package==StartupSkinPackage::event&&v.sprite>=0;});
        const auto marker=std::find_if(images.begin(),images.end(),[](const auto &v){return v.package==StartupSkinPackage::common;});
        check(actor!=images.end()&&actor->offset==std::array<int,2>{65,command==0?sample.happy:sample.sad}&&
              actor->sprite==(command==0?0:3)&&actor->frame==0&&clips[0].rectangle[3]==sample.reveal&&
              (marker!=images.end())==sample.marker,"11独立数组样点/RateConvert边界/恰60不画而61画继续标");
        if(sample.marker)check(marker->image==72&&marker->sprite==2&&marker->frame==0&&
            marker->offset==std::array<int,2>{213,204},"11继续SEB2不是标题AddTouch动态箭头");
        if(command==0)check(images.back().image==7&&images.back().crop==
            std::array<int,4>{0,(sample.main%20<10)?0:24,180,24},"11烟花两24高裁片按20槽切换，不画整48高");
    }
    options.japanese=true;in.main=61;in.command=2;p=steam_event_skin(in,options);
    const auto text=parts<SteamEventTextLayout>(*p);
    check(text.size()==1&&text[0].rectangle==std::array<int,4>{28,159,182,49}&&text[0].anchor==2&&
          text[0].rgb==std::array<int,3>{92,51,31}&&parts<StartupSkinDraw>(*p).size()==2&&
          parts<StartupSkinDraw>(*p)[0].image==10,"11命令2沿非0背景而无三人/烟花，JP正文49高anchor2");
    for(int command:{-7,99}) {
        in.command=command;
        check(steam_event_skin(in,options)&&parts<StartupSkinDraw>(*steam_event_skin(in,options)).size()==2,
              "11源通用else接受其它int，不凭当前原表只0/1造新业务禁令");
    }
    for(int gate=0;gate<3;++gate) {
        auto hidden=options;hidden.title_widths.reset();hidden.on_top=gate!=0;
        hidden.until_active_hide=gate==1;hidden.wait=gate==2?1:0;
        check(steam_event_skin(in,hidden)&&steam_event_skin(in,hidden)->draws.empty(),"11三种共同隐藏门槛均不消费缺失测宽");
    }
    auto owner=test_support::page_fixture(11);const auto id=owner.scripts.pages.back().id;
    auto &page=owner.scripts.pages.back();page.lifecycle=0;page.message_commands={0,1};page.paragraphs={"实际首段","实际次段"};
    check(!steam_event_input(owner,id)&&initialize_startup_world_event_pages(owner),"11未Init桥拒绝，通过真实初始化创建已有phase/main");
    owner.page_phases.at(id)=1;owner.page_counters.at(id)=52;
    const auto before=startup_world_state_digest(owner);const auto read=steam_event_input(owner,id);
    check(read&&read->command==1&&read->main==52&&read->text=="实际次段"&&startup_world_state_digest(owner)==before,
          "11桥按真实phase读取配对命令/正文，不推进主钟/页面/奖励/随机或输出");
    for(int fault=0;fault<6;++fault) {
        auto bad=owner;
        if(fault==0)bad.page_phases.erase(id);
        if(fault==1)bad.page_counters.erase(id);
        if(fault==2)bad.scripts.pages.back().paragraphs.pop_back();
        if(fault==3)bad.page_phases.at(id)=2;
        if(fault==4)bad.scripts.pages.back().lifecycle=4;
        if(fault==5)bad.scripts.pages.push_back(bad.scripts.pages.back());
        const auto frozen=startup_world_state_digest(bad);
        check(!steam_event_input(bad,id)&&startup_world_state_digest(bad)==frozen,
              "11桥拒缺字段/数组失配/越界/退休/重复ID，不用默认0或map[]补建");
    }
    for(int image:{1,2,3,4,5,6,7,9,10}) {
        const auto source=steam_event_resource(StartupSkinPackage::event,image);
        check(source&&std::filesystem::is_regular_file(root.parent_path()/source->image_path)&&
              (!source->sprite_path||std::filesystem::is_regular_file(root.parent_path()/source->sprite_path)),
              "11原图/SEB已有明确出版路径，资源表不猜同名重提取");
    }
    check(steam_event_resource(StartupSkinPackage::common,72)->sprite==2&&
          !steam_event_resource(StartupSkinPackage::event,72)&&!steam_event_resource(StartupSkinPackage::title,9),
          "11素材映射保留包身份，未知包/索引拒绝");

    // raw59共用事件窗，主责当前定义外观、入场/气泡/正文/页脚原序；不重复Owner70领取测试。
    SteamHumanUnlockSkinInput visitor{1,4,0,0,"当前职业","当前姓名"};
    options.japanese=false;options.human_unlock_text={"多指教","的","可以","到访了!"};
    struct VisitorSample {int main,x,step;bool greeting,marker;};
    constexpr std::array<VisitorSample,10> visitor_samples{{{0,220,0,false,false},{1,219,0,false,false},
        {39,155,1,false,false},{59,122,2,false,false},{60,120,3,true,false},
        {70,120,1,true,false},{75,120,2,true,true},{89,120,2,true,true},
        {90,120,2,true,false},{100,120,1,true,true}}};
    for(const auto &sample:visitor_samples) {
        visitor.main=sample.main;options.greeting_width=sample.greeting?std::optional<int>{61}:std::nullopt;
        p=steam_human_unlock_skin(visitor,options);
        check(p&&p->origin==std::array<int,2>{0,10}&&p->soft_labels==std::array<int,2>{0,0},
              "59沿共同零位窗及零soft标签，宽高居中不重复VIEW_Y");
        const auto people=parts<SteamHumanPortrait>(*p);
        const auto clips=parts<SteamEventClip>(*p);
        const auto images=parts<StartupSkinDraw>(*p);const auto texts=parts<SteamHumanOverviewText>(*p);
        check(people.size()==1&&people[0].position==std::array<int,2>{sample.x,144}&&
              people[0].actor.body.sprite==2&&people[0].actor.body.frame==sample.step&&
              people[0].actor.weapon&&!people[0].actor.shadow&&!people[0].clip&&
              clips.size()==4&&clips[0].rectangle==std::array<int,4>{30,67,180,80}&&
              clips[1].kind==SteamEventClipKind::pop&&clips[2].rectangle[1]==158&&
              clips[3].kind==SteamEventClipKind::pop,
              "59整数负乘积插值样点、左向四步、当前武器，无标题阴影；人物与正文分别裁剪");
        check(images.size()==(sample.marker?2U:1U)&&images[0].image==9&&images[0].package==StartupSkinPackage::event&&
              images[0].offset==std::array<int,2>{30,67}&&images[0].crop==std::array<int,4>{0,0,180,80}&&
              (sample.marker?images[1].image==72&&images[1].offset==std::array<int,2>{212,204}:true)&&
              std::get<SteamStartupTouch>(p->draws.back()).component==2&&
              std::get<SteamStartupTouch>(p->draws.back()).option==2&&
              !std::get<SteamStartupTouch>(p->draws.back()).rectangle,
              "59event9/70后25槽前15槽页脚与Create2触摸，不能套11周期/锚/option");
        const std::size_t first=sample.greeting?1:0;
        check(texts.size()==first+3&&texts[first].value=="当前职业的"&&texts[first].rectangle==std::array<int,4>{120,158,0,0}&&
              texts[first+1].value=="当前姓名可以"&&texts[first+1].rectangle==std::array<int,4>{120,176,0,0}&&
              texts[first+2].value=="到访了!"&&texts[first+2].rectangle==std::array<int,4>{120,194,0,0}&&
              !texts[first].layout&&!texts[first].rich&&texts[first].anchor==2&&
              (!sample.greeting||(texts[0].value=="多指教"&&texts[0].rectangle==std::array<int,4>{120,107,0,0}&&
                  texts[0].rgb==std::array<int,3>{0,0,0}&&parts<SteamHumanImage>(*p).size()==6)),
              "59三次普通DrawString不合段重排；60后气泡长串复用原27裁片/尾/端点及黑字");
    }
    visitor.main=60;options.greeting_width.reset();
    check(!steam_human_unlock_skin(visitor,options),"59可见气泡缺真实测宽拒绝，不按字数估算");
    visitor.main=59;check(steam_human_unlock_skin(visitor,options).has_value(),"59未到60不索取气泡测宽");
    for(int gate=0;gate<3;++gate) {
        auto hidden=options;hidden.title_widths.reset();hidden.on_top=gate!=0;
        hidden.until_active_hide=gate==1;hidden.wait=gate==2?1:0;
        check(steam_human_unlock_skin(visitor,hidden)&&steam_human_unlock_skin(visitor,hidden)->draws.empty(),
              "59三种隐藏资格短路，不消费标题/气泡测宽或生成图元");
    }
    auto unlock=test_support::page_fixture(59);const auto unlock_id=unlock.scripts.pages.back().id;
    unlock.scripts.pages.back().legacy_f=0;unlock.scripts.pages.back().lifecycle=2;
    unlock.page_counters[unlock_id]=73;
    unlock.human_profiles[0]={"覆盖姓名",1,true};
    unlock.scene.world.world.ai.growth.at(0).definition.current_profession=4;
    unlock.shop_humans.at(0).equipment[0]=0;
    const auto frozen=startup_world_state_digest(unlock);const auto source=steam_human_unlock_input(unlock,unlock_id);
    check(source&&source->main==73&&source->profession==4&&source->sex==1&&source->weapon==0&&source->name=="覆盖姓名"&&
          source->profession_name==unlock.rules->jobs.at(4).name&&startup_world_state_digest(unlock)==frozen,
          "59严格桥读取当前职业/主武器及Owner姓名性别覆盖，不创建到访实体或推进随机/输出");
    for(int fault=0;fault<9;++fault) {
        auto bad=unlock;
        if(fault==0)bad.page_counters.erase(unlock_id);
        if(fault==1)bad.page_phases[unlock_id]=0;
        if(fault==2)bad.page_secondary_counters[unlock_id]=0;
        if(fault==3)bad.scripts.pages.back().legacy_page=11;
        if(fault==4)bad.scripts.pages.back().lifecycle=4;
        if(fault==5)bad.scripts.pages.push_back(bad.scripts.pages.back());
        if(fault==6)bad.scene.world.world.ai.growth.erase(0);
        if(fault==7)bad.shop_humans.erase(0);
        if(fault==8)bad.shop_humans.at(0).equipment[0]=std::numeric_limits<int>::max();
        const auto before_read=startup_world_state_digest(bad);
        check(!steam_human_unlock_input(bad,unlock_id)&&startup_world_state_digest(bad)==before_read,
              "59桥拒缺钟/孤立副载荷/错类型/退休/重复页/缺当前外观及非法装备，不补默认值");
    }
}
// 字体请求数值与本地化准入独立于字形后端，不以渲染截图生成这些期望值。
void font_metrics(Checks &check) {
    struct Size {int request,scale;bool preset;int used;};
    constexpr std::array<Size,13> sizes{{
        {0,100,true,12},{1,100,true,24},{2,100,true,30},{3,100,true,3},
        {0,100,false,0},{1,100,false,1},{2,100,false,2},{11,98,false,10},
        {0,98,true,11},{11,125,false,13},{1,0,true,0},{1,1,false,0},
        {1,std::numeric_limits<int>::max(),false,21474836}}};
    for(const auto &sample:sizes) {
        const auto result=steam_font_size(sample.request,sample.scale,sample.preset);
        check(result&&result->request==sample.request&&result->scale==sample.scale&&
            result->use_preset==sample.preset&&result->used_size==sample.used,
            "Steam字体源preset12/24/30与实例整数缩放，显式0及scale0不造最小字号");
    }
    for(const auto bad:std::array<Size,4>{{{-1,100,true,0},{1,-1,false,0},
        {std::numeric_limits<int>::max(),2,false,0},{0,std::numeric_limits<int>::max(),true,0}}})
        check(!steam_font_size(bad.request,bad.scale,bad.preset),
            "Steam字体维护拒负/32位乘法溢出，不依赖C++ UB或暗中夹值");
    struct Localize {int used,normal,asia;};
    // 原float位型逐阶段推导的独立整数oracle，含可检出改用double/融合计算的边界。
    constexpr std::array<Localize,12> local{{
        {0,0,0},{6,5,5},{9,8,8},{10,8,9},{11,9,10},{12,10,11},
        {24,20,23},{30,25,29},{130,108,127},{630,522,617},{105,87,103},{1,0,1}}};
    for(const auto &sample:local)for(bool asia:{false,true})for(bool language:{false,true}) {
        const auto result=steam_font_localized_request(sample.used,asia,language);
        check(result&&*result==(asia&&language?sample.asia:sample.normal),
            "Steam本地化采用源float32乘加次序/<=9下限，亚洲须两个独立资格都成立");
    }
    const auto first=steam_font_size(12,98,true);
    const auto request=steam_font_localized_request(first->used_size,true,true);
    const auto final=steam_font_size(*request,98,true);
    check(first->used_size==11&&request==10&&final&&final->used_size==9&&
        (first->used_size-final->used_size)*0.5F==1.F,
        "Steam本地化返回新请求，须再过实例scale；顶锚字号差补偿1不能合并舍入");
    for(const auto top:std::array<std::optional<bool>,3>{std::nullopt,false,true})
        for(bool config:{false,true})for(bool locked:{false,true})for(bool japanese:{false,true}) {
            SteamFontLocalizeGate in{6,top,config,locked,japanese};
            const auto result=steam_font_needs_localize(in);
            check(result&&*result==(top.value_or(config)&&!locked&&!japanese),
                "Steam本地化栈顶覆盖全局，false不回退；锁/日文各自抑制");
        }
    check(steam_font_needs_localize({5,std::nullopt,true,false,false})==std::optional<bool>(false)&&
        !steam_font_needs_localize({-1,std::nullopt,true,false,false})&&
        !steam_font_localized_request(-1,true,true),
        "Steam字号5不准入，本地化helper不偷偷执行守卫；负维护状态拒绝");
    check(steam_font_localized_request(1,false,false)==0,
        "Steam派生字号函数独立于准入，合法小字号按原式计算，不夹到6");
}
// RankForm独立页面合同；共用窗框和人物SEB继续由各自主责套件验证。
void record_page(Checks &check) {
    SteamRecordSkinInput input;
    input.view_width=479;input.view_height=481;input.page=0;input.frame=19;
    input.english=true;input.value=1234;input.village="纪录村";
    input.labels={"High Score","已翻译纪录","最高点数","点数"};
    input.topic_width=65;input.value_width=23;input.village_width=65.5F;
    input.arrow_selected={true,false};input.definition_count=12;
    input.frozen_definitions={9,2,7,0,11,4};
    const auto plan=steam_record_skin(input);
    check(plan&&plan->origin==std::array<int,2>{119,133}&&plan->final_origin==std::array<int,2>{0,0}&&
          plan->soft_labels==std::array<int,2>{0,2}&&plan->draws.size()==17,
          "Rank奇数尺寸分别向零居中再加13，末清零而非恢复入口，六人保留17请求");
    const auto windows=parts<SteamRecordWindow>(*plan);
    check(windows.size()==2&&windows[0].title=="High Score 1/2"&&windows[1].title=="已翻译纪录 1/2"&&
          std::holds_alternative<SteamRecordWindow>(plan->draws[0])&&
          std::holds_alternative<SteamRecordWindow>(plan->draws[1]),
          "两次窗口标题的翻译来源不同，原序连续保留而非去重为一次");
    for(const auto &window:windows)
        check(window.source.width==200&&window.source.height==170&&window.source.vertical==0&&window.source.style==-12,
              "Rank两次200x170窗口均使用原offset与v实参，不重复验证底层木纹");
    const auto &left=std::get<SteamManualArrowRequest>(plan->draws[2]);
    const auto &left_touch=std::get<SteamStartupTouch>(plan->draws[3]);
    const auto &right=std::get<SteamManualArrowRequest>(plan->draws[4]);
    const auto &right_touch=std::get<SteamStartupTouch>(plan->draws[5]);
    check(left.position==std::array<int,2>{41,42}&&left.bounds_frame==3&&left.selected&&
          right.position==std::array<int,2>{196,42}&&right.bounds_frame==0&&!right.selected&&
          left_touch.component==1&&left_touch.value==16&&left_touch.image_draw==2&&!left_touch.rectangle&&
          right_touch.component==1&&right_touch.value==18&&right_touch.image_draw==4&&!right_touch.rectangle&&
          left_touch.option==0&&right_touch.option==0,
          "每箭头之后紧接对应touch索引，selected独立传递，不用资源frame造最终箭头贴图");
    const auto texts=parts<SteamRecordText>(*plan);
    check(texts.size()==4&&texts[0].value=="最高点数"&&texts[0].position==std::array<float,2>{88.F,59.F}&&
          texts[1].value=="纪录村"&&texts[1].position==std::array<float,2>{87.25F,166.F}&&
          texts[2].position==std::array<float,2>{46.F,186.F}&&texts[3].value=="1,234"&&
          texts[3].position==std::array<float,2>{175.F,186.F}&&
          texts[0].rgb==std::array<int,3>{60,100,200}&&texts[1].rgb==std::array<int,3>{0,0,60}&&
          texts[2].rgb==texts[0].rgb&&texts[3].rgb==std::array<int,3>{200,100,60},
          "主题宽先整数除2、村名保持float半宽，非JP底值独立右齐且P不补入");
    const auto images=parts<StartupSkinDraw>(*plan);
    const auto actors=parts<SteamManualActorRequest>(*plan);
    check(images.size()==1&&images[0].package==StartupSkinPackage::event&&images[0].image==9&&
          images[0].crop==std::array<int,4>{0,0,180,80}&&images[0].offset==std::array<int,2>{30,78}&&
          std::holds_alternative<SteamRecordText>(plan->draws[6])&&
          std::holds_alternative<StartupSkinDraw>(plan->draws[7])&&actors.size()==6,
          "主题→event9整图→人物助手原序，不另插标题阴影或将新局裁片冒充纪录背景");
    constexpr std::array<int,6> actor_ids{9,2,7,0,11,4};
    constexpr std::array<int,6> actor_x{68,94,120,146,172,198};
    for(std::size_t i=0;i<actors.size();++i)
        check(actors[i].definition==actor_ids[i]&&actors[i].position==std::array<int,2>{actor_x[i],153}&&
              actors[i].seb==0&&actors[i].anime_index==0&&actors[i].direction==1&&actors[i].update_counter==19,
              "冻结名单不重排，六人fallback不截五；仅中心计算夹五，原action0/dir1/纪录钟独立");

    struct FormatCase {std::int64_t value;const char *number;const char *points;const char *gold;};
    for(const auto &sample:std::array<FormatCase,3>{{
        {0,"0","0P","0Ｇ"},{1234,"1,234","1,234P","1,234Ｇ"},
        {std::numeric_limits<std::int64_t>::max(),"9,223,372,036,854,775,807","9,223,372,036,854,775,807P","9,223,372,036,854,775,807Ｇ"}}}) {
        for(int page=0;page<2;++page)for(bool japanese:{false,true}) {
            const auto formatted=steam_record_value_text(page,sample.value,japanese);
            check(formatted&&*formatted==(page?sample.gold:japanese?sample.points:sample.number),
                  "两页非负long独立格式oracle：P只限JP，Ｇ两语言保留，零与最大值不缩写");
        }
    }
    check(!steam_record_value_text(-1,0,false)&&!steam_record_value_text(2,0,true)&&
          !steam_record_value_text(0,-1,false)&&!steam_record_value_text(1,std::numeric_limits<std::int64_t>::min(),true),
          "坏页及负纪录拒绝，不用绝对值或当前钱包补造");
    input.page=1;input.japanese=true;input.english=false;
    const auto japanese=steam_record_skin(input);
    check(bool(japanese),"JP资金纪录合法候选可绘制");
    const auto jptexts=parts<SteamRecordText>(*japanese);
    check(parts<SteamRecordWindow>(*japanese)[0].title=="ハイスコア 2/2"&&
          parts<SteamRecordWindow>(*japanese)[1].title=="已翻译纪录 2/2"&&
          jptexts[2].value=="ﾊｲｽｺｱ"&&jptexts[2].position==std::array<float,2>{58.F,186.F}&&
          jptexts[3].position==std::array<float,2>{163.F,186.F}&&jptexts[3].value=="1,234Ｇ",
          "JP第一遍标题保留固定日文，第二遍仍LT；底签固定半角原串，资金页不改单位或两底锚");
    auto unused_text=input;
    unused_text.labels.english_title=std::string(4097,'x');unused_text.labels.bottom=std::string(4097,'y');
    check(bool(steam_record_skin(unused_text)),"JP不消费English专用标题或非JP底签，不能让未使用文本阻止画面");
    struct FrameCase {int frame,step,left,right;};
    for(const auto sample:std::array<FrameCase,6>{{{0,0,44,193},{15,3,41,196},{16,0,41,196},
            {19,0,41,196},{20,1,44,193},{std::numeric_limits<int>::max()-1,3,43,194}}}) {
        input.frame=sample.frame;
        const auto p=steam_record_skin(input);
        check(p&&parts<SteamManualActorRequest>(*p)[0].anime_index==sample.step&&
              parts<SteamManualActorRequest>(*p)[0].update_counter==sample.frame&&
              parts<SteamManualArrowRequest>(*p)[0].position[0]==sample.left&&
              parts<SteamManualArrowRequest>(*p)[1].position[0]==sample.right&&input.frozen_definitions==
                  std::vector<int>({9,2,7,0,11,4}),
              "人物16槽和箭头20槽独立oracle，最大合法钟不溢出且查询不改名单");
    }
    input.view_width=239;input.view_height=237;input.definition_count=0;input.frozen_definitions.clear();
    input.village.clear();input.village_width=0.F;input.value=0;
    const auto empty=steam_record_skin(input);
    check(empty&&empty->origin==std::array<int,2>{0,12}&&empty->final_origin==std::array<int,2>{0,0}&&
          empty->draws.size()==11&&parts<SteamManualActorRequest>(*empty).empty()&&
          parts<SteamRecordText>(*empty)[1].value.empty()&&parts<SteamRecordText>(*empty)[3].value=="0Ｇ",
          "小于240的负奇数偏移向零，空名单/空村名/零纪录均保留，不补默认人名村名");
    input.view_width=65536;input.view_height=65536;input.definition_count=4096;
    for(int id=0;id<4096;++id)input.frozen_definitions.push_back(id);
    const auto maximum=steam_record_skin(input),repeat=steam_record_skin(input);
    check(maximum&&repeat&&maximum->draws.size()==4107&&repeat->draws.size()==4107&&
          parts<SteamManualActorRequest>(*maximum).back().position==std::array<int,2>{106538,153},
          "最多definition_count加11请求，合法长fallback保留原排列而不因超窗截人或积累输出");
    input.definition_count=12;input.frozen_definitions={9,2,7,0,11,4};
    for(int fault=0;fault<28;++fault) {
        auto bad=input;
        if(fault==0)bad.page=-1;if(fault==1)bad.page=2;if(fault==2)bad.value=-1;
        if(fault==3)bad.frame=-1;if(fault==4)bad.frame=std::numeric_limits<int>::max();
        if(fault==5)bad.view_width=0;if(fault==6)bad.view_height=65537;
        if(fault==7)bad.topic_width.reset();if(fault==8)bad.value_width.reset();if(fault==9)bad.village_width.reset();
        if(fault==10)bad.village_width=std::numeric_limits<float>::quiet_NaN();
        if(fault==11)bad.village_width=std::numeric_limits<float>::infinity();
        if(fault==12)bad.topic_width=-1;if(fault==13)bad.value_width=4097;
        if(fault==14)bad.village_width=-0.5F;if(fault==15)bad.village_width=4096.5F;
        if(fault==16)bad.definition_count=-1;if(fault==17)bad.definition_count=4097;
        if(fault==18)bad.frozen_definitions[0]=-1;if(fault==19)bad.frozen_definitions[0]=12;
        if(fault==20)bad.frozen_definitions[1]=9;if(fault==21)bad.definition_count=5;
        if(fault==22)bad.village=std::string(4097,'x');if(fault==23)bad.english=true;
        if(fault==24)bad.labels.translated_title=std::string(4097,'x');
        if(fault==25)bad.labels.topic=std::string(4097,'x');
        if(fault==26){bad.japanese=false;bad.labels.bottom=std::string(4097,'x');}
        if(fault==27){bad.japanese=false;bad.english=true;bad.labels.english_title=std::string(4097,'x');}
        check(!steam_record_skin(bad),"坏纪录页/钟/测宽/目录及重复引用整体拒绝，不返回部分画面");
    }
}
// raw15只测排版观测的消费、原输入顺序及局部计划；不复制TextLayout字形算法。
void news_page(Checks &check) {
    check(steam_news_rows(false,11)==6&&steam_news_rows(false,10)==7&&
          steam_news_rows(false,-1)==7&&steam_news_rows(true,{})==7&&
          !steam_news_rows(false,{}),"NEWS字号阈值及泰语短路，不猜缺失字体");
    check(steam_news_page_count(102,18,6)==1&&steam_news_page_count(120,18,6)==2&&
          steam_news_page_count(120,18,7)==1&&steam_news_page_count(0,18,6)==0,
          "MakeNews真实高度/行距的两次整数除法，合法0页不钳成1");
    check(!steam_news_page_count(-1,18,6)&&!steam_news_page_count(10,0,6)&&
          !steam_news_page_count(10,18,5)&&
          !steam_news_page_count(std::numeric_limits<int>::max(),18,6)&&
          !steam_news_page_count(std::numeric_limits<int>::max()-6,1,7),
          "NEWS非法观测及原32位加法溢出显式拒绝，不以宽整数改变源结果");
    SteamNewsUpdateInput u;u.pages=3;u.secondary=197;
    auto updated=steam_news_update(u);
    check(updated&&updated->main==2&&updated->secondary==198&&updated->page==0&&
          updated->checked_keys==std::array<bool,5>{true,true,true,false,true},
          "NEWS整轮主钟先共同加1再本页加1，独立副钟只加1");
    u.left_repeat=true;u.right_repeat=true;u.confirm_pulse=true;u.back_soft_label=true;
    updated=steam_news_update(u);
    check(updated&&updated->page==1&&updated->request_pop&&updated->checked_keys[4],
          "NEWS左右独立依次循环，确认未越尾仍消费soft2");
    u.page=2;u.left_repeat=false;u.right_repeat=false;
    updated=steam_news_update(u);
    check(updated&&updated->page==3&&updated->request_pop&&!updated->checked_keys[4],
          "NEWS确认越尾只发Pop请求并保留越尾候选，不假装Pop已成功");
    u.page=0;u.confirm_pulse=false;u.back_soft_label=false;u.debug_held=true;
    check(steam_news_update(u)->page==0&&!steam_news_update(u)->checked_keys[3],
          "DEBUG held没有DEBUG_CMD资格不成为正常确认按钮");
    u.debug_command=true;
    check(steam_news_update(u)->page==1&&steam_news_update(u)->checked_keys[3],
          "DEBUG_CMD有资格且无Pulse时才检查调试held");
    u.confirm_pulse=true;
    check(!steam_news_update(u)->checked_keys[3],"正常确认短路调试held观测");
    u.confirm_pulse=false;u.debug_command=false;u.main=std::numeric_limits<int>::max()-1;
    u.secondary=u.main;
    check(steam_news_update(u)->main==1&&steam_news_update(u)->secondary==0,
          "NEWS先共同INT_MAX模再本页模20，不错误整体加2后模20");
    u.pages=0;
    check(steam_news_update(u)&&!steam_news_update(u)->request_pop,
          "0页没有左右输入仍可正常等待，不把合法计算0改成演示1页");
    u.confirm_pulse=true;
    check(steam_news_update(u)->page==1&&steam_news_update(u)->request_pop,
          "0页确认仍按原递增和越尾比较请求Pop");
    u.confirm_pulse=false;u.right_repeat=true;
    check(!steam_news_update(u),"0页实际执行左右取模才显式拒绝除零");

    SteamNewsSkinInput in;in.width=239;in.height=261;in.view_y=23;in.pages=3;in.page=2;in.main=19;
    in.japanese=true;in.configured_text_layout=true;in.rendering_font_size=11;in.line_step=18;
    in.window_title="通信 3/3";in.title_widths=std::array<int,2>{45,47};in.name="当前文章名";
    in.arrow_selected={true,false};
    auto skin=steam_news_skin(in);
    check(skin&&skin->origin==std::array<int,2>{0,10}&&skin->title=="通信 3/3"&&
          parts<SteamNewsLogoDraw>(*skin)[0].position==std::array<int,2>{29,62}&&
          parts<SteamNewsLogoDraw>(*skin)[0].common_image==125,
          "NEWS共同居中保留负奇数向零，图125整图实际坐标不借内框坐标");
    const auto arrows=parts<SteamManualArrowRequest>(*skin);
    check(arrows.size()==2&&arrows[0].position==std::array<int,2>{41,46}&&
          arrows[0].bounds_frame==3&&arrows[0].selected&&
          arrows[1].position==std::array<int,2>{196,46}&&arrows[1].bounds_frame==0&&!arrows[1].selected,
          "NEWS真实按下选态交同AddTouch箭头helper，不直接绘common箭头SEB");
    const auto body=parts<SteamNewsBodyDraw>(*skin);
    const auto touches=parts<SteamStartupTouch>(*skin);
    check(body.size()==1&&body[0].position==std::array<int,2>{41,106}&&body[0].scroll_v==216&&
          touches.size()==3&&touches[0].value==16&&touches[1].value==18&&
          touches[2].component==2&&touches[2].option==2&&!touches[2].rectangle,
          "NEWS页2按6行与真实18行距滚动216，末尾登记无矩形touch2");
    const auto &last=skin->draws.back();
    check(std::holds_alternative<SteamStartupTouch>(last)&&parts<SteamNewsFont>(*skin).empty()&&
          parts<SteamNewsColor>(*skin)[0].role==SteamNewsColorRole::window_blue,
          "NEWS日本语不改字号，标题颜色保留具名源Role");
    in.japanese=false;in.english=true;in.rendering_font_size=10;
    skin=steam_news_skin(in);
    const auto fonts=parts<SteamNewsFont>(*skin);
    check(fonts.size()==2&&fonts[0].action==SteamNewsFontAction::set_size11&&
          fonts[1].action==SteamNewsFontAction::reset&&parts<SteamNewsBodyDraw>(*skin)[0].scroll_v==252,
          "NEWSEnglish只包围name改字号再恢复；正文仍用原实例7行观测");
    in.english=false;in.window_title.reset();in.title_widths.reset();
    skin=steam_news_skin(in);
    check(skin&&skin->title.empty()&&parts<SteamNewsLogoDraw>(*skin).size()==1&&
          parts<SteamNewsBodyDraw>(*skin).size()==1&&parts<SteamNewsFont>(*skin).empty(),
          "Jp/En均假只跳外框题头，仍画报纸内框正文及touch，不要求不消费的测宽");
    in.pages=0;in.page=0;
    check(steam_news_skin(in)&&parts<SteamManualArrowRequest>(*steam_news_skin(in)).empty(),
          "合法0页静态画面保留而不产生左右按钮，不冒充可导航");
    in.line_step.reset();check(!steam_news_skin(in),"缺真实TextLayout行距拒绝");
    in.on_top=false;
    check(steam_news_skin(in)->draws.empty(),"隐藏NEWS不消费尚无的正文布局观测");
    in.on_top=true;in.line_step=std::numeric_limits<int>::max();in.page=std::numeric_limits<int>::max()-1;
    check(!steam_news_skin(in),"NEWS滚动乘法先检查源int边界，不触发宿主宽整数溢出");
}
void record_update(Checks &check) {
    struct Case {bool left,right,back;int page;std::array<bool,3> checked;bool pop;};
    for(const auto c:std::array<Case,6>{{
        {false,false,false,0,{true,true,true},false},
        {true,false,false,1,{true,false,false},false},
        {false,true,false,1,{true,true,false},false},
        {true,true,true,1,{true,false,false},false},
        {false,true,true,1,{true,true,false},false},
        {false,false,true,0,{true,true,true},true}}}) {
        const SteamRecordUpdateInput in{0,19,true,c.left,c.right,c.back};
        const auto out=steam_record_update(in);
        check(out&&out->page==c.page&&out->frame==20&&out->checked_keys==c.checked&&out->request_pop==c.pop&&
              in.page==0&&in.frame==19,"Rank先钟后Left/Right短路，翻页压过返回且只翻一次；候选不写输入");
    }
    const auto wrap=steam_record_update({1,std::numeric_limits<int>::max()-1,true,false,true,true});
    check(wrap&&wrap->page==0&&wrap->frame==0&&!wrap->request_pop,
          "独立页钟到INT_MAX回0、第二页方向回第一页；不在翻页另清钟或同时请求Pop");
    const auto inactive=steam_record_update({1,7,false,true,true,true});
    check(inactive&&inactive->page==1&&inactive->frame==8&&!inactive->request_pop&&
          inactive->checked_keys==std::array<bool,3>{false,false,false},
          "非state0仍先加纪录钟，但不检查方向/soft2；不是由绘制推进");
    for(const auto bad:std::array<SteamRecordUpdateInput,4>{{{-1,0},{2,0},{0,-1},{0,std::numeric_limits<int>::max()}}})
        check(!steam_record_update(bad),"非法页或不可达独立钟拒绝，不补0或借标题钟");
}
void manual_page(Checks &check,const std::filesystem::path &root) {
    SteamManualSkinInput input;
    input.width=240;input.height=320;input.body_pages=29;input.page=0;input.frame=19;
    input.body_text="Owner已缓存的说明正文";input.localize_text=true;
    // 正文不读取末页演员；旧缓存可以尚未准备，不能扩大只读查询资格。
    input.definition_count=-1;input.frozen_definitions={-1,99,99,99,99};
    const auto body=steam_manual_skin(input);
    check(body && body->origin==std::array<int,2>{0,40} && body->draws.size()==8,
          "raw13正文保留SubForm共同居中，不消费未使用人物名单");
    const auto &window=std::get<SteamManualWindow>(body->draws[0]);
    const auto &left=std::get<SteamManualArrowRequest>(body->draws[1]);
    const auto &left_touch=std::get<SteamStartupTouch>(body->draws[2]);
    const auto &right=std::get<SteamManualArrowRequest>(body->draws[3]);
    const auto &right_touch=std::get<SteamStartupTouch>(body->draws[4]);
    const auto &box=std::get<SteamStartupBox>(body->draws[5]);
    const auto &text=std::get<SteamManualText>(body->draws[6]);
    const auto &tail=std::get<SteamStartupTouch>(body->draws[7]);
    check(window.source.width==220 && window.source.height==166 && window.source.vertical==0 &&
          window.page_number==1 && window.total_pages==30 && box.left==17 && box.top==60 &&
          box.right==223 && box.bottom==197,
          "非JP说明窗220x166、内框端点和1/30语义来自独立机器码实参");
    check(left.position==std::array<int,2>{21,56} && left.bounds_frame==3 && !left.selected &&
          right.position==std::array<int,2>{216,56} && right.bounds_frame==0 && !right.selected &&
          left_touch.component==1 && left_touch.value==16 && left_touch.image_draw==1 && !left_touch.rectangle &&
          right_touch.component==1 && right_touch.value==18 && right_touch.image_draw==3 &&
          tail.component==2 && tail.option==2 && !tail.rectangle && !tail.image_draw,
          "箭头helper请求后登记原触摸，bounds帧不充当绘制帧，共尾不捏造确定按钮");
    check(text.role==SteamManualTextRole::body && text.body==*input.body_text &&
          text.rectangle==std::array<int,4>{28,80,186,124} && text.font_size==10 &&
          text.line_space==8 && text.rgb==std::array<int,3>{30,30,30} &&
          !text.anchor && text.localize_text,
          "Owner缓存原样投影，非JP正文10号字与8行距，不重复LT或用白框居中");
    input.japanese=true;input.page=28;input.frame=20;input.localize_text=false;
    const auto japanese=steam_manual_skin(input);
    const auto jptext=parts<SteamManualText>(*japanese).front();
    check(std::get<SteamManualWindow>(japanese->draws[0]).source.width==180 &&
          std::get<SteamManualWindow>(japanese->draws[0]).page_number==29 &&
          jptext.rectangle==std::array<int,4>{48,80,146,124} && jptext.font_size==0 &&
          jptext.line_space==6 && !jptext.localize_text &&
          std::get<SteamManualArrowRequest>(japanese->draws[1]).position==std::array<int,2>{44,56},
          "JP保留当前字号，换页后的直接SetText标记与20帧箭头复位独立");
    input.page=29;input.body_text.reset();input.definition_count=25;
    input.frozen_definitions={24,0,8,3};input.frame=31;input.trial_version=true;
    const auto about=steam_manual_skin(input);
    check(about && about->draws.size()==20 &&
          std::get<SteamManualWindow>(about->draws[0]).page_number==30,
          "关于页30/30不要求旧正文缓存，四人输出大小有限");
    const auto labels=parts<SteamManualText>(*about);
    check(labels.size()==3 && labels[0].role==SteamManualTextRole::about &&
          labels[0].rectangle==std::array<int,4>{120,80,0,0} && !labels[0].localize_text &&
          labels[1].role==SteamManualTextRole::game_name && labels[1].localize_text && labels[1].trial_version &&
          labels[1].rectangle==std::array<int,4>{120,166,0,0} &&
          labels[2].role==SteamManualTextRole::copyright &&
          labels[2].rectangle==std::array<int,4>{120,182,0,0} && labels[2].anchor==2,
          "关于/游戏名/版权按原序，trial仅游戏名语义，不默认为正式版");
    check(std::get<SteamManualClip>(about->draws[9]).push &&
          std::get<SteamManualClip>(about->draws[9]).rectangle==std::array<int,4>{43,101,154,56} &&
          !std::get<SteamManualClip>(about->draws[17]).push &&
          std::get<StartupSkinRect>(about->draws[18]).rect==std::array<int,4>{43,101,153,55} &&
          std::get<StartupSkinRect>(about->draws[18]).rgb==std::array<int,3>{43,116,190} &&
          std::get<SteamStartupTouch>(about->draws[19]).component==2,
          "先clip再背景人物，pop恢复旧clip后画蓝描边，最后共尾触摸");
    const auto crops=parts<StartupSkinDraw>(*about);
    check(crops.size()==3 && crops[0].image==39 && crops[0].sprite==-1 &&
          crops[0].crop==std::array<int,4>{0,0,93,82} && crops[0].offset==std::array<int,2>{30,90} &&
          crops[1].crop==std::array<int,4>{1,0,91,82} && crops[1].offset==std::array<int,2>{109,90} &&
          crops[2].crop==std::array<int,4>{1,0,20,82} && crops[2].offset==std::array<int,2>{189,90},
          "mp_back39三个源裁片保持sx1而非镜像，不替换整张截图");
    const auto actors=parts<SteamManualActorRequest>(*about);
    const std::array<int,4> positions{80,106,133,160};
    for(std::size_t i=0;i<actors.size();++i)
        check(actors[i].definition==input.frozen_definitions[i] &&
              actors[i].position==std::array<int,2>{positions[i],148} && actors[i].seb==0 &&
              actors[i].anime_index==3 && actors[i].direction==2 && actors[i].update_counter==31,
              "冻结人物原序及完整SetDispPlayerData参数不消费随机或重排少人数位置");
    input.frozen_definitions={8};input.frame=32;input.trial_version=false;
    const auto single=steam_manual_skin(input);
    check(parts<SteamManualActorRequest>(*single).front().position==std::array<int,2>{80,148} &&
          parts<SteamManualActorRequest>(*single).front().anime_index==0 &&
          !parts<SteamManualText>(*single)[1].trial_version,
          "一人仍取原位置80，16帧步态独立复位，正式版不携试玩文字");
    input.frozen_definitions.clear();input.definition_count=0;
    check(steam_manual_skin(input) && parts<SteamManualActorRequest>(*steam_manual_skin(input)).empty(),
          "合法空人物池不制造后备人或表现请求");
    for(const auto &ids:std::vector<std::vector<int>>{{-1},{25},{1,1},{0,1,2,3,4}}) {
        auto invalid=input;invalid.definition_count=25;invalid.frozen_definitions=ids;
        check(!steam_manual_skin(invalid),"末页拒绝负/越界/重复定义和超过四人载荷");
        invalid.on_top=false;
        check(steam_manual_skin(invalid)->draws.empty(),"隐藏页不消费非法未用人物名单");
    }
    auto missing=input;missing.page=0;
    check(!steam_manual_skin(missing),"可见正文缺缓存明确拒绝");
    missing.body_text="";
    check(steam_manual_skin(missing).has_value(),"已提供空正文与缺缓存不同");
    missing.body_text.reset();
    for(int gate=0;gate!=3;++gate) {
        auto hidden=missing;
        if(gate==0)hidden.on_top=false;
        if(gate==1)hidden.until_active_hide=true;
        if(gate==2)hidden.wait=1;
        check(steam_manual_skin(hidden)->draws.empty(),"非顶/隐藏/等待阻挡正文和触摸请求");
    }
    for(int fault=0;fault!=7;++fault) {
        auto invalid=input;
        if(fault==0)invalid.body_pages=0;
        if(fault==1)invalid.body_pages=std::numeric_limits<int>::max();
        if(fault==2)invalid.page=30;
        if(fault==3)invalid.frame=-1;
        if(fault==4)invalid.wait=-1;
        if(fault==5)invalid.width=0;
        if(fault==6)invalid.origin[1]=std::numeric_limits<int>::max();
        check(!steam_manual_skin(invalid),"说明页原始维度/页号/计数/居中溢出拒绝");
    }
    const auto arrows=sprite(root/"common/arrow02.seb");
    check(arrows.frame_count==6 && arrows.layers.size()==1 && arrows.layers[0].parts.size()==6,
          "原arrow02实际六帧，不能从raw13只用0/3推成四帧");
    for(int frame:{0,3}) {
        const auto &part=arrows.layers[0].parts[frame];
        check(part.frame==frame && part.image_index==74 && part.offset_x==0 && part.offset_y==-3 &&
              part.width==4 && part.height==8 && part.source_x==(frame==0?4:8),
              "raw13所取SEB bounds源记录独立核验，原PNG箭头不是最终显示贴图");
    }
    const std::array<int,4> bounds{0,-3,4,8};
    // 沿视觉套件已有raylib依赖解码正式图；不为PNG头检查另链接完整归档工具。
    auto button_image=LoadImage((root.parent_path()/"steam-common/buttoneffect.png").string().c_str());
    const bool button_decoded=button_image.data!=nullptr;
    const int button_width=button_image.width, button_height=button_image.height;
    UnloadImage(button_image);
    check(button_decoded && button_width==128 && button_height==128,
          "正式Steam独立buttoneffect纹理必须存在且为已发布128x128 PNG，不借APK同名文件");
    for(int frame:{3,0})for(bool selected:{false,true}) {
        const auto part=steam_manual_arrow_skin({{120,56},frame,selected},bounds,
                                                button_width);
        check(part && part->crop[0]>=0 && part->crop[1]>=0 && part->crop[2]>0 && part->crop[3]>0 &&
              part->crop[0]+part->crop[2]<=button_width &&
              part->crop[1]+part->crop[3]<=button_height &&
              part->crop[2]==(selected?20:15) && part->crop[3]==(selected?26:20),
              "四种箭头均实际消费正式PNG宽度，裁片尺寸及二维边界完整可用");
    }
    const auto arrow_left=steam_manual_arrow_skin({{21,56},3,false},bounds,128);
    const auto arrow_right=steam_manual_arrow_skin({{216,56},0,false},bounds,128);
    check(arrow_left && arrow_right && arrow_left->texture_index==0 && arrow_right->texture_index==1 &&
          arrow_left->crop==std::array<int,4>{0,57,15,20} &&
          arrow_right->crop==std::array<int,4>{15,57,15,20} &&
          arrow_left->destination==std::array<float,4>{17.25F,50,11.25F,15} &&
          arrow_right->destination==std::array<float,4>{213,50,11.25F,15} &&
          arrow_left->touch_rectangle==std::array<int,4>{17,50,11,15} &&
          arrow_left->paint_origin==std::array<int,2>{21,56} && arrow_left->paint_frame==3,
          "默认左右取buttoneffect0/1，0.75与居中重配准后绘制/触摸/SetPaint不同");
    const auto active_left=steam_manual_arrow_skin({{21,56},3,true},bounds,256);
    const auto active_right=steam_manual_arrow_skin({{216,56},0,true},bounds,256);
    check(active_left && active_right && active_left->texture_index==2 && active_right->texture_index==3 &&
          active_left->crop==std::array<int,4>{62,114,40,52} &&
          active_right->crop==std::array<int,4>{104,114,40,52} &&
          active_left->destination==std::array<float,4>{15.25F,47,15,19.5F} &&
          active_right->destination==std::array<float,4>{210.5F,47,15,19.5F} &&
          active_left->paint_origin==std::array<int,2>{22,56} &&
          active_right->paint_origin==std::array<int,2>{215,56} &&
          active_right->touch_rectangle==std::array<int,4>{210,47,15,19},
          "选中用2/3独立大裁片，纹理密度2不改变逻辑0.75，配准向零截断");
    input.arrow_selected={true,false};
    check(std::get<SteamManualArrowRequest>(steam_manual_skin(input)->draws[1]).selected &&
          !std::get<SteamManualArrowRequest>(steam_manual_skin(input)->draws[3]).selected,
          "调用方CheckTouch资格分别传入，不由皮肤查询鼠标或重建选择");
    check(!steam_manual_arrow_skin({{0,0},2,false},bounds,128) &&
          !steam_manual_arrow_skin({{0,0},3,false},{0,0,0,8},128) &&
          !steam_manual_arrow_skin({{0,0},3,false},bounds,0) &&
          !steam_manual_arrow_skin({{std::numeric_limits<int>::max(),0},3,true},bounds,128),
          "本页未证bounds帧、缺尺寸和配准溢出显式拒绝，不用猜帧或补纹理");
}
void save_page(Checks &check) {
    using Role=SteamStartupTextRole;
    SteamSavePageSkinInput input;
    for(int stage:{0,1}) {
        input.stage=stage;
        // 保存中根本不读取字体测宽，故缺值及未消费的坏值均不影响正文。
        for(const auto width:std::vector<std::optional<float>>{{},-1.0F,
                std::numeric_limits<float>::quiet_NaN()}) {
            input.measured_button_width=width;
            const auto plan=steam_save_page_skin(input);
            check(plan && plan->origin==std::array<int,2>{0,0} && plan->draws.size()==3 &&
                  plan->touches.empty(),"raw14阶段0/1只画正文框，不提前要求了解按钮测宽或注册触摸");
            check(std::holds_alternative<SteamStartupWindow>(plan->draws[0]) &&
                  std::holds_alternative<SteamStartupBox>(plan->draws[1]) &&
                  std::holds_alternative<SteamStartupText>(plan->draws[2]),
                  "raw14保留窗口、内框、正文的原调用顺序");
            const auto &window=std::get<SteamStartupWindow>(plan->draws[0]);
            const auto &box=std::get<SteamStartupBox>(plan->draws[1]);
            const auto &text=std::get<SteamStartupText>(plan->draws[2]);
            check(window.width==210 && window.height==110 && window.vertical==0 && window.style==0 &&
                  window.title==Role::message_title && window.title_value==0 &&
                  box.left==26 && box.top==90 && box.right==213 && box.bottom==148,
                  "raw14正式窗口和DrawBox端点不冒充宽高或手动选栏皮肤");
            check(text.role==Role::message_body && text.rectangle==std::array<double,4>{26,115,187,32} &&
                  text.anchor==34 && text.rgb==std::array<int,3>{92,51,31} &&
                  text.line_space==6 && text.font_size==0 && text.placement==SteamStartupTextPlacement::layout,
                  "raw14正文沿原TextLayout锚点、行距、棕色和当前字号");
        }
    }
    input.stage=2;input.measured_button_width=31.5F;
    const auto completed=steam_save_page_skin(input);
    check(completed && completed->draws.size()==5 && completed->touches.size()==1 &&
          std::holds_alternative<SteamStartupFill>(completed->draws[3]) &&
          std::holds_alternative<SteamStartupText>(completed->draws[4]),
          "raw14完成页追加一个高亮和一个了解，不复用raw1双答案或raw20三行");
    const auto &fill=std::get<SteamStartupFill>(completed->draws[3]);
    const auto &answer=std::get<SteamStartupText>(completed->draws[4]);
    const auto outer=steam_startup_window_skin(std::get<SteamStartupWindow>(completed->draws[0]),23);
    const auto inner=steam_startup_box_skin(std::get<SteamStartupBox>(completed->draws[1]),23);
    check(outer && inner && parts<StartupSkinDraw>(*outer)[0].offset==std::array<int,2>{15,76} &&
          parts<StartupSkinRect>(*inner)[0].rect==std::array<int,4>{26,101,187,58},
          "GameForm原静态VIEW_Y23只移动两种窗框，不能把0条件例图当源默认");
    check(fill.rectangle==std::array<double,4>{98.25,164,43.5,17} &&
          fill.rgb==std::array<int,3>{255,153,55} && fill.source_ratio==256 &&
          answer.role==Role::answer && answer.row==0 &&
          answer.rectangle==std::array<double,4>{120,167,0,0} && answer.anchor==2 &&
          answer.rgb==std::array<int,3>{92,51,31} && answer.font_size==0,
          "实测31.5的单按钮保留float半像素高亮和中心120正文");
    const auto &touch=completed->touches.front();
    check(touch.component==3 && touch.value==0x20000 &&
          touch.rectangle==std::array<int,4>{-1,64,243,217} && !touch.image_draw && touch.option==0,
          "raw14了解触摸使用原ID3与负数向零截断，不添加全局KEYCLICK或option");
    input.measured_button_width=0.0F;
    const auto zero=steam_save_page_skin(input);
    check(zero && zero->touches.front().rectangle==std::array<int,4>{14,64,212,217},
          "零宽仍是合法实际测宽，按钮补2且保留触摸额外210");
    input.width=200;input.height=260;
    check(steam_save_page_skin(input)->origin==std::array<int,2>{-20,10},
          "raw14任一维超过240时共同居中，窄维保留负偏移");
    input.width=200;input.height=200;
    check(steam_save_page_skin(input)->origin==std::array<int,2>{0,0},
          "raw14两维均小于240时不自行居中或缩放窗口");
    for(const auto width:std::vector<std::optional<float>>{{},-1.0F,
            std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::max(),static_cast<float>(std::numeric_limits<int>::max())}) {
        input.measured_button_width=width;
        check(!steam_save_page_skin(input),"raw14可见结果页缺测宽或转换越界必须显式拒绝");
        for(int gate=0;gate<3;++gate) {
            auto hidden=input;
            if(gate==0)hidden.on_top=false;
            if(gate==1)hidden.until_active_hide=true;
            if(gate==2)hidden.wait=1;
            const auto plan=steam_save_page_skin(hidden);
            check(plan && plan->draws.empty() && plan->touches.empty(),
                  "raw14非顶、隐藏或等待先阻挡整块绘制，未消费的按钮测宽不额外拒绝");
        }
    }
    for(int fault=0;fault<5;++fault) {
        auto bad=SteamSavePageSkinInput{};
        if(fault==0)bad.stage=-1;
        if(fault==1)bad.stage=3;
        if(fault==2)bad.width=0;
        if(fault==3)bad.height=-1;
        if(fault==4)bad.wait=-1;
        check(!steam_save_page_skin(bad),"raw14非法阶段、维度和等待值拒绝");
        bad.on_top=false;
        check(!steam_save_page_skin(bad),"隐藏资格不能修补非法原始几何或阶段");
    }
}
void main_menu_notices(Checks &check) {
    // 仅准备实际消费的Owner字段；故意缺少短路后的数据，防止查询扩大为全世界校验。
    StartupWorldRules rules;
    StartupWorldRuntimeState state;
    check(!steam_main_menu_notices(state),"NEW查询缺规则源显式拒绝");
    state.rules=&rules;
    state.build_category_new={false,true,false};
    const auto initial=steam_main_menu_notices(state);
    check(initial && initial->construction && !initial->quest && !initial->equipment &&
          !initial->activities && !initial->adventurers,"建设NEW直接消费原缓存，不重建建设列表");
    StartupWorldEquipment armour,weapon,accessory;
    armour.shop.kind=2;armour.shop.id=4;armour.shop.type=2;
    weapon.shop.kind=1;weapon.shop.id=9;
    accessory.shop.kind=3;accessory.shop.id=2;
    rules.equipment={armour,accessory,weapon}; // 混排不应改变原武器→全防具→饰品查询次序。
    state.task_order={1,2};state.next_task_identity=3;
    state.tasks[1].identity=1;state.tasks[1].newly_available=true;
    auto notice=steam_main_menu_notices(state);
    check(notice && notice->quest && !notice->equipment,
          "首任务NEW短路后续缺任务和全装备来源");
    state.active_task=1;
    check(!steam_main_menu_notices(state),"已有选中任务时继续读装备，缺实际武器源拒绝");
    state.catalog[{1,9}].status=-1;state.catalog[{1,9}].newly_unlocked=true;
    notice=steam_main_menu_notices(state);
    check(notice && notice->quest && notice->selected_quest && notice->equipment,
          "装备p非零而非正数，零flags/库存仍有GET；首武器命中不读防具饰品");
    SteamMainMenuSkinInput input;input.frame=3;input.selection=1;input.notices=notice;
    const auto plan=steam_main_menu_skin(input,{});
    check(plan.has_value(),"Owner通知可接稳定raw3绘制");
    const auto images=parts<SteamMainMenuImage>(*plan);
    check(std::any_of(images.begin(),images.end(),[](const auto &part){return part.image==148&&part.position[1]==44;}),
          "Owner选中任务屏蔽NEW后实际绘制装备GET");
    state.tasks[1].newly_available=false;
    check(!steam_main_menu_notices(state),"未命中首任务时后续悬空引用拒绝");
    state.task_order.clear();state.active_task.reset();state.catalog[{1,9}].status=0;
    check(!steam_main_menu_notices(state),"武器p0不命中后检查防具实际来源");
    state.catalog[{2,4}].status=1;state.catalog[{2,4}].newly_unlocked=true;
    notice=steam_main_menu_notices(state);
    check(notice && notice->equipment,"全防具查询不复用Steam装备页flags0排除");
    state.catalog[{2,4}].newly_unlocked=false;
    check(!steam_main_menu_notices(state),"防具无NEW后读取饰品，缺源拒绝");
    state.catalog[{3,2}].status=2;state.catalog[{3,2}].newly_unlocked=true;
    check(steam_main_menu_notices(state)->equipment,"饰品p2仍命中GET");

    StartupWorldItem item;item.identity=5;rules.items={item};
    StartupDefinition facility;facility.id=7;facility.unlock_rank=0;rules.facilities={facility};
    StartupWorldActivity activity;activity.identity=3;rules.activities={activity};
    state.scripts.activities[3].status=0;
    check(steam_main_menu_notices(state).has_value(),"商会flag16关不读取商会源，活动status0不读flags/count");
    state.scripts.user_flags=16;
    check(!steam_main_menu_notices(state),"商会开启后缺库存来源拒绝");
    state.shop_item_stock[5].quantity=0;state.facility_presence[7]=2;
    check(steam_main_menu_notices(state).has_value(),"商品零库存与设施p2跳过各自已阅字段");
    state.shop_item_stock[5].quantity=1;
    check(!steam_main_menu_notices(state),"正库存实际需要已阅来源");
    state.item_commerce_read[5]=false;state.scripts.activities.clear();state.facility_presence.clear();
    notice=steam_main_menu_notices(state);
    check(notice && notice->commerce_items && !notice->commerce_facilities && !notice->activities,
          "商会首商品命中跳过设施与活动缺源");
    state.item_commerce_read[5]=true;
    check(!steam_main_menu_notices(state),"商品已阅后需要设施presence来源");
    state.facility_presence[7]=1;
    check(!steam_main_menu_notices(state),"原设施p1仍进入商会NEW已阅检查");
    state.facility_commerce_read[7]=false;
    notice=steam_main_menu_notices(state);
    check(notice && notice->commerce_facilities && !notice->activities,
          "设施NEW使用p不等于2而非维护商会p0策略，命中后不读活动");
    state.scripts.activities[3].status=0;state.facility_commerce_read.clear();
    for(int required:{-1,1}) {
        rules.facilities[0].unlock_rank=required;
        check(steam_main_menu_notices(state).has_value(),"未开放等级或高于当前星级不读取设施已阅");
    }
    rules.facilities[0].unlock_rank=0;state.facility_presence[7]=2;
    state.scripts.activities[3].status=1;state.scripts.activities[3].pending_notice=true;
    check(!steam_main_menu_notices(state),"status1活动缺flags拒绝");
    state.activity_flags[3]=0;
    check(steam_main_menu_notices(state)->activities,"未设bit2不读count，NEW不检查点数季度或效果支持");
    state.activity_flags[3]=6;
    check(!steam_main_menu_notices(state),"活动bit2计数在bit4前读取，不能先用bit4跳过缺count");
    state.activity_counts[3]=0;
    check(!steam_main_menu_notices(state)->activities,"活动bit4屏蔽NEW");
    state.activity_flags[3]=2;state.activity_counts[3]=1;
    check(!steam_main_menu_notices(state)->activities,"限次活动已举办屏蔽NEW");
    state.activity_counts[3]=-1;
    check(steam_main_menu_notices(state)->activities,"原计数小于等于0判断不改成等于0");
    StartupWorldActivity later_activity;later_activity.identity=8;rules.activities.push_back(later_activity);
    check(steam_main_menu_notices(state)->activities,"首活动NEW跳过后续缺活动状态");

    StartupWorldHuman human;human.identity=4;rules.humans={human};
    check(!steam_main_menu_notices(state),"人物目录缺presence拒绝");
    state.human_presence[4]=0;
    check(!steam_main_menu_notices(state)->adventurers,"人物p0不读取脚本通知");
    state.human_presence[4]=-1;
    check(!steam_main_menu_notices(state),"人物p非零实际消费脚本通知");
    state.scripts.humans[4].pending_notice=true;
    StartupWorldHuman later_human;later_human.identity=9;rules.humans.push_back(later_human);
    check(steam_main_menu_notices(state)->adventurers,"首人物NEW跳过后续缺presence来源");
    for(unsigned flags:{0U,1U,2U,3U}) {
        state.scripts.user_flags=flags;
        check(steam_main_menu_notices(state)->magic_pot==(flags==3U),"壶NEW只在flag1可见时消费flag2");
    }
    check(state.build_category_new==std::array<bool,3>{false,true,false} &&
          state.catalog.at({3,2}).newly_unlocked && state.item_commerce_read.at(5) &&
          state.scripts.activities.at(3).pending_notice && state.scripts.humans.at(4).pending_notice &&
          state.scripts.user_flags==3U && state.activity_counts.at(3)==-1 && state.task_order.empty(),
          "查询不刷新缓存、不清NEW/已阅、不改计数或任务目录");
}
void main_menu(Checks &check,const std::filesystem::path &root) {
    SteamMainMenuSkinInput input;input.selection=1;
    SteamMainMenuSkinOptions options;
    constexpr std::array<int,4> widths{0,22,45,68},heights{0,9,19,29},icon_sizes{0,5,11,18};
    for(int frame=0;frame<4;++frame) {
        input.frame=frame;
        input.notices=frame==3?std::optional<SteamMainMenuNotices>{SteamMainMenuNotices{}}:std::nullopt;
        const auto plan=steam_main_menu_skin(input,options);
        check(plan && plan->touches.size()==5 && plan->origin==options.origin,"主菜单五项包含全尺寸frame0触摸");
        const auto images=parts<SteamMainMenuImage>(*plan);
        const auto labels=parts<SteamMainMenuText>(*plan);
        std::vector<SteamMainMenuImage> backgrounds,icons;
        for(const auto &part:images) {
            if(part.image==25)backgrounds.push_back(part);
            if(part.image==168)icons.push_back(part);
        }
        check(images.size()==(frame==0?0U:frame==3?11U:10U) && labels.size()==(frame==3?5U:0U),
              "主菜单原展开先缩源底图/图标，文字与手形等待3");
        constexpr std::array<int,5> tags{0,1,2,5,6};
        for(int row=0;row<5;++row) {
            const auto &touch=plan->touches[row];
            check(touch.component==8 && touch.value==(0x20000|row) && touch.option==0 &&
                  touch.rectangle==std::array<int,4>{0,28*row,91,28},"ID8原整行与tag4不入目录");
            if(frame)check(backgrounds.at(row).crop==std::array<int,4>{0,row==1?0:29,widths[frame],heights[frame]} &&
                  icons.at(row).crop==std::array<int,4>{18*tags[row],0,icon_sizes[frame],icon_sizes[frame]},
                  "主菜单68宽底图与18格图标使用已核源裁片，不按91宽拉伸");
            if(frame==3)check(labels[row].tag==tags[row] && labels[row].position==std::array<int,2>{28,28*row+9} &&
                  labels[row].rgb==(row==1?std::array<int,3>{76,58,50}:std::array<int,3>{255,242,220}),
                  "主菜单文字取MENU_STR真实tag与选中颜色");
        }
    }
    input.magic_unlocked=true;input.selection=3;input.magic_period=StartupMagicPotMenuInformation{2,3};
    input.notices=SteamMainMenuNotices{true,true,false,true,true,true,true,true,true,true};
    const auto full=*steam_main_menu_skin(input,options);
    check(full.touches.size()==6 && parts<SteamMainMenuText>(full)[3].tag==3,
          "flag1只在2与5之间插开发，仍无tag4");
    const auto period=std::find_if(full.draws.begin(),full.draws.end(),[](const auto &part) {
        const auto *image=std::get_if<SteamMainMenuImage>(&part);
        return image&&image->package==SteamMainMenuPackage::common2;
    });
    check(period!=full.draws.end() && std::get<SteamMainMenuImage>(*period).position==std::array<int,2>{69,88} &&
          std::get<SteamFacilityNumber>(*(period+1)).asset==SteamFacilityAsset::number08 &&
          std::get<SteamFacilityNumber>(*(period+1)).value==2 &&
          std::get<SteamFacilityNumber>(*(period+1)).position==std::array<int,2>{104,94} &&
          std::get<SteamMainMenuImage>(*(period+2)).crop==std::array<int,4>{47,0,7,10} &&
          std::get<SteamFacilityNumber>(*(period+3)).value==3 &&
          std::get<SteamMainMenuImage>(*(period+4)).sprite==22,
          "NEW后期间底图/SEB15数字/斜线/待件分母/手形保持原序");
    // 原冒险NEW优先；选中任务会屏蔽任务NEW，但不会屏蔽装备GET。
    for(const auto scenario:std::array<std::array<int,4>,5>{{{1,0,1,147},{1,1,1,148},{0,0,1,148},{1,1,0,-1},{0,0,0,-1}}}) {
        input.notices->quest=scenario[0];input.notices->selected_quest=scenario[1];input.notices->equipment=scenario[2];
        const auto plan=*steam_main_menu_skin(input,options);
        const auto images=parts<SteamMainMenuImage>(plan);
        const auto mark=std::find_if(images.begin(),images.end(),[](const auto &p){return (p.image==147||p.image==148)&&p.position[1]==44;});
        check(scenario[3]<0?mark==images.end():mark!=images.end()&&mark->image==scenario[3],
              "任务NEW和装备GET按源优先级，不能两个标签一起画");
    }
    input.notices->quest=false;input.notices->equipment=true;
    for(const auto scenario:std::array<std::array<int,4>,4>{{{0,1,0,0},{0,1,1,1},{1,1,0,1},{1,0,0,0}}}) {
        input.notices->commerce_open=scenario[0];input.notices->commerce_items=scenario[1];
        input.notices->commerce_facilities=false;input.notices->activities=scenario[2];
        const auto images=parts<SteamMainMenuImage>(*steam_main_menu_skin(input,options));
        const bool shown=std::any_of(images.begin(),images.end(),[](const auto &p){return p.image==147&&p.position[1]==72;});
        check(shown==(scenario[3]!=0),"村办活动NEW独立于商会flag16，未阅商会须同时开放");
    }
    for(const auto language:std::array<std::array<int,6>,4>{{{0,0,91,28,44,69},{0,1,91,26,80,97},
                                                         {1,0,63,28,55,79},{1,1,63,26,55,79}}}) {
        options.japanese=language[0];options.english=language[1];
        const auto plan=*steam_main_menu_skin(input,options);const auto images=parts<SteamMainMenuImage>(plan);
        check(plan.touches[0].rectangle->at(2)==language[2] && parts<SteamMainMenuText>(plan)[0].position[0]==language[3] &&
              std::find_if(images.begin(),images.end(),[&](const auto &p){return p.image==148&&p.position[0]==language[4];})!=images.end() &&
              std::find_if(images.begin(),images.end(),[&](const auto &p){return p.package==SteamMainMenuPackage::common2&&p.position[0]==language[5];})!=images.end(),
              "JP覆盖NEW/期间附加偏移，En文字减2仍独立，不使用raw9测宽规则");
    }
    options={};options.origin={200,200};options.safe_left=10;
    const auto overflow=*steam_main_menu_skin(input,options);
    check(overflow.touches[0].rectangle==std::array<int,4>{-51,-133,91,28},"六行右底修正覆盖safe-left，允许负局部原点");
    options={};options.on_top=false;
    check(steam_main_menu_skin(input,options)->touches.size()==6,"touch资格是栈顶raw3，不与手形IsTopForm合并");
    const auto covered_images=parts<SteamMainMenuImage>(*steam_main_menu_skin(input,options));
    check(std::none_of(covered_images.begin(),covered_images.end(),[](const auto &p){return p.image==70;}),
          "不是本页栈顶不绘制手形");
    options.on_top=true;options.top_is_main_menu=false;
    check(steam_main_menu_skin(input,options)->touches.empty(),"非raw3栈顶不登记ID8，手形资格独立");
    options.covered_by_nonmenu_subform=true;input.notices.reset();input.magic_period.reset();
    const auto hidden=steam_main_menu_skin(input,options);
    check(hidden&&hidden->draws.empty()&&hidden->touches.empty(),"非菜单覆盖不读取未绘制NEW/期间源");
    options={};
    check(!steam_main_menu_skin(input,options),"可见稳定菜单缺NEW或期间查询结果显式拒绝");
    input.notices=SteamMainMenuNotices{};input.magic_period=StartupMagicPotMenuInformation{4,3};
    check(!steam_main_menu_skin(input,options),"期间超待件分母拒绝");
    input.magic_period=StartupMagicPotMenuInformation{0,3};input.selection=6;
    check(!steam_main_menu_skin(input,options),"六行选择越界拒绝，不靠数组异常");
    input.selection=0;options.safe_left=std::numeric_limits<int>::max();
    check(!steam_main_menu_skin(input,options),"图元锚点溢出拒绝");
    check(steam_main_menu_child_position({0,25},3,false,false)==std::array<int,2>{68,109} &&
          !steam_main_menu_child_position({0,25},3,true,false) &&
          steam_main_menu_child_position({0,25},4,true,true)==std::array<int,2>{96,137} &&
          !steam_main_menu_child_position({std::numeric_limits<int>::max(),25},1,false,false),
          "子页定位用父存储位置/行号/En，不按tag迁移，不取绘制safe-left");
    const auto source=sprite(root/"common/menu.seb");
    check(source.layers[0].parts[0].width==68 && source.layers[0].parts[1].source_y==29,
          "主菜单底图独立SEB源oracle");
    const auto icons=sprite(root/"common/wnd_menuIcon.seb");
    check(icons.frame_count==7 && icons.layers[0].parts[6].image_index==168 &&
          icons.layers[0].parts[6].source_x==108 && icons.layers[0].parts[6].width==18,
          "七标签图集独立源oracle不等于七项均可选");
    const auto path=steam_main_menu_image(SteamMainMenuPackage::common2,0);
    check(path=="steam-common/mpot_event.png" && std::filesystem::file_size(root.parent_path()/std::string(*path))==744 &&
          !steam_main_menu_image(SteamMainMenuPackage::common2,168),"Steam期间差异原图独立出版，组域不能混用");
}
}
// 挂既有visuals套件：局部合同与源资源oracle，不建立新target或窗口状态机。
int check_steam_startup_skin(const std::filesystem::path &root) {
    Checks check;
    using Asset=SteamStartupAsset;
    using Role=SteamStartupTextRole;
    SteamTitleBackdropInput backdrop{479,481,301,277,9,{600,380},{}};
    const auto early=steam_title_backdrop_skin(backdrop);
    check(early&&early->operations.size()==12&&early->final_origin==std::array<int,2>{119,0},
          "标题前段按W/Wv独立使用，frame9无Logo也留下中心原点");
    const auto &sky=std::get<SteamStartupFill>(early->operations[0]);
    check(sky.rectangle==std::array<double,4>{0,0,301,277}&&sky.rgb==std::array<int,3>{41,198,255},
          "首操作直接填view尺寸继承入口原点，不能先补Set0");
    check(std::get<SteamTitleSetOrigin>(early->operations[1]).value==std::array<int,2>{30,0}&&
          std::get<SteamTitleWholeImage>(early->operations[2]).position==std::array<int,2>{-180,101}&&
          std::get<SteamTitleSetOrigin>(early->operations[3]).value==std::array<int,2>{0,0}&&
          std::get<SteamTitleWholeImage>(early->operations[4]).image==6&&
          std::get<SteamTitleWholeImage>(early->operations[5]).position==std::array<int,2>{240,0}&&
          std::get<SteamTitleSetOrigin>(early->operations[6]).value==std::array<int,2>{119,0}&&
          std::holds_alternative<SteamTitleActors>(early->operations[7])&&
          std::get<SteamTitleSetOrigin>(early->operations[8]).value==std::array<int,2>{0,0}&&
          std::get<SteamTitleWholeImage>(early->operations[9]).position==std::array<int,2>{0,450}&&
          std::get<SteamTitleWholeImage>(early->operations[10]).position==std::array<int,2>{240,450}&&
          std::get<SteamTitleSetOrigin>(early->operations[11]).value==early->final_origin,
          "整图背景→upper→实际演员插入点→草→无条件中心原点，尾片不裁短或拉伸");
    backdrop.logo=SteamTitleLogoVariant::original;
    for(const auto [frame,y]:std::array<std::array<int,2>,9>{{
        {10,-61},{73,117},{74,120},{100,120},{101,118},{105,114},{106,114},{112,120},
        {2147483646,117}}}) {
        backdrop.title_frame=frame;
        const auto plan=steam_title_backdrop_skin(backdrop);
        check(plan&&plan->operations.size()==14,"Logo门槛后增加恰一幅原图与独立无矩形确认注册");
        const auto &logo=std::get<SteamTitleWholeImage>(plan->operations[12]);
        const auto &touch=std::get<SteamTitleConfirmRegistration>(plan->operations[13]);
        check(logo.image==2&&logo.position==std::array<int,2>{2,y}&&touch.component==2&&touch.flags==2&&
              std::string(logo.published_image)=="steam-startup/original/title/title_logo.png",
              "Logo入场/负余数/弹跳/饱和钟的独立整数oracle，不附加APK英语偏移");
    }
    backdrop.title_frame=100;backdrop.logo=SteamTitleLogoVariant::english;
    const auto english=steam_title_backdrop_skin(backdrop);
    check(english&&std::string(std::get<SteamTitleWholeImage>(english->operations[12]).published_image)==
          "steam-startup/original/title/English.lproj/title_logo.png"&&
          std::get<SteamTitleWholeImage>(english->operations[12]).position==std::array<int,2>{2,120},
          "实际Logo选择显式传入，英语变体不改变本体锚也不从中文UI猜资源");
    struct BackdropAsset {const char *name;int width,height;};
    for(const auto &asset:std::array<BackdropAsset,5>{{{"title00.png",600,380},{"upper.png",240,9},
        {"title_grass.png",240,18},{"title_logo.png",236,115},{"English.lproj/title_logo.png",236,132}}}) {
        const auto path=root.parent_path()/"steam-startup/original/title"/asset.name;
        const auto image=LoadImage(path.string().c_str());
        const bool valid=image.data&&image.width==asset.width&&image.height==asset.height;
        if(image.data)UnloadImage(image);
        check(valid,"背景/顶部/草边/两Logo消费真正Steam出版整图，不借APK同名图");
    }
    auto narrow=backdrop;narrow.width=239;narrow.view_width=239;narrow.background_size={599,379};
    const auto odd=steam_title_backdrop_skin(narrow);
    check(odd&&odd->final_origin==std::array<int,2>{0,0}&&
          std::get<SteamTitleWholeImage>(odd->operations[2]).position==std::array<int,2>{-179,102},
          "纯几何条件覆盖负奇数向零除2，背景坐标来自实际Image尺寸而非固定600假设");
    auto wide=backdrop;wide.width=65536;
    const auto bounded=steam_title_backdrop_skin(wide);
    check(bounded&&bounded->operations.size()==558&&steam_title_backdrop_skin(wide)->operations.size()==558,
          "维护最大宽度的平铺输出有界，重复查询不累计图元或推进演员");
    for(int fault=0;fault<6;++fault) {
        auto bad=backdrop;
        if(fault==0)bad.width=0;if(fault==1)bad.view_height=65537;if(fault==2)bad.background_size[0]=-1;
        if(fault==3)bad.title_frame=std::numeric_limits<int>::max();if(fault==4)bad.logo.reset();
        if(fault==5)bad.logo=static_cast<SteamTitleLogoVariant>(-1);
        check(!steam_title_backdrop_skin(bad),"已消费标题尺寸/钟/Logo身份错误明确拒绝");
    }
    // Steam窗口helper独立oracle：变换前几何，不以真实字体或物理像素认证代替。
    SteamStartupWindow frame{198,146,70,-12,Role::slot_title,2};
    const auto expanded=steam_startup_window_skin(frame,0,std::array<int,2>{41,44});
    check(expanded && expanded->draws.size()==10,"标准窗边线、五木纹块、标题带、双文字保持原序");
    const auto frame_rects=parts<StartupSkinRect>(*expanded);
    const auto frame_images=parts<StartupSkinDraw>(*expanded);
    const auto frame_texts=parts<SteamStartupText>(*expanded);
    check(frame_rects.size()==2 && frame_rects[0].rect==std::array<int,4>{20,103,200,149} &&
          frame_rects[0].rgb==std::array<int,3>{89,103,91} && frame_rects[0].outline &&
          frame_rects[1].rect==std::array<int,4>{21,104,198,147} &&
          frame_rects[1].rgb==std::array<int,3>{239,239,221},"Steam DrawRect双环已转半开边界不可再加1");
    check(frame_images.size()==6 && frame_images[0].image==28 &&
          frame_images[4].crop==std::array<int,4>{0,0,38,146} &&
          frame_images[4].offset==std::array<int,2>{181,105} &&
          frame_images[5].image==29 && frame_images[5].crop==std::array<int,4>{22,0,196,17},
          "木纹按40裁尾，标题带按全窗横坐标裁取");
    check(frame_texts.size()==2 && frame_texts[0].rectangle==std::array<double,4>{100,108,0,0} &&
          frame_texts[1].rectangle==std::array<double,4>{98,107,0,0} &&
          frame_texts[0].rgb==std::array<int,3>{44,54,105} &&
          frame_texts[1].rgb==std::array<int,3>{247,253,247} &&
          frame_texts[0].role==Role::slot_title && frame_texts[1].row==2 &&
          !frame_texts[0].anchor && frame_texts[0].font_size==0,
          "标题两次实测宽分别使用，保留角色值而不猜字体或隐式anchor");
    check(std::holds_alternative<StartupSkinRect>(expanded->draws[1]) &&
          std::holds_alternative<StartupSkinDraw>(expanded->draws[2]) &&
          std::holds_alternative<SteamStartupText>(expanded->draws[8]),"完整展开保持层次而非按后端重排");
    check(parts<SteamStartupText>(*steam_startup_window_skin(frame,0)).empty() &&
          parts<SteamStartupText>(*steam_startup_window_skin(frame,0,std::array<int,2>{0,0})).size()==2,
          "null标题和实测零宽标题有不同原调用数量");
    frame={120,216,0,0,Role::message_title,0};
    const auto negative=steam_startup_window_skin(frame,-241);
    check(negative && parts<StartupSkinDraw>(*negative)[0].offset==std::array<int,2>{60,-108} &&
          parts<StartupSkinDraw>(*negative)[3].crop==std::array<int,4>{0,0,0,216},
          "负奇数VIEW_Y向零截断且整40宽保留零宽尾请求");
    frame.height=217; frame.style=std::numeric_limits<int>::max();
    check(parts<StartupSkinDraw>(*steam_startup_window_skin(frame,std::numeric_limits<int>::max()))[0]
              .offset==std::array<int,2>{60,12},"高于216不使用VIEW_Y及style，不能无条件加偏移");
    const SteamStartupBox box{26,90,213,148};
    const auto box_plan=steam_startup_box_skin(box,-3,1);
    const auto box_rects=parts<StartupSkinRect>(*box_plan);
    const auto corners=parts<StartupSkinDraw>(*box_plan);
    check(box_plan->draws.size()==7 && box_rects[0].rect==std::array<int,4>{26,89,187,58} &&
          !box_rects[0].outline && box_rects[1].rect==box_rects[0].rect && box_rects[1].outline &&
          box_rects[2].rect==std::array<int,4>{27,90,185,56},"Box边界转尺寸，VIEW_Y先独立除2");
    check(corners.size()==4 && corners[0].image==121 && corners[0].sprite==6 &&
          corners[0].offset==std::array<int,2>{26,89} &&
          corners[2].offset==std::array<int,2>{213,147} && corners[3].frame==3,
          "白角替换图片121而保留SEB6及四边界锚点");
    check(parts<StartupSkinDraw>(*steam_startup_box_skin(box,0))[0].image==30 &&
          steam_startup_box_skin(box,0,2)->draws.size()==3 &&
          steam_startup_box_skin(box,0,-1)->draws.size()==3,"标准角与其他mode无角不能混为默认回退");
    const auto wood_only=steam_startup_window2_skin(80,40,5,3);
    check(wood_only && wood_only->draws.size()==3 &&
          parts<StartupSkinDraw>(*wood_only)[0].offset==std::array<int,2>{80,106} &&
          parts<StartupSkinDraw>(*wood_only)[2].crop==std::array<int,4>{0,0,0,40},
          "Window2只画木纹，不能用标准窗null标题代替");
    const auto explicit_window=steam_startup_window3_skin(81,40,7,9);
    check(explicit_window && explicit_window->draws.size()==5 &&
          parts<StartupSkinRect>(*explicit_window)[0].rect==std::array<int,4>{6,8,83,43} &&
          parts<StartupSkinRect>(*explicit_window)[1].rect==std::array<int,4>{7,9,81,41} &&
          parts<StartupSkinDraw>(*explicit_window)[2].crop==std::array<int,4>{0,0,1,41},
          "Window3显式坐标与H加1裁高独立于标准窗");
    check(!steam_startup_window_skin({2,17,0,0,Role::slot_title,0},0) &&
          !steam_startup_window_skin({240,16,0,0,Role::slot_title,0},0) &&
          !steam_startup_window_skin({240,240,0,0,Role::slot_title,0},0,std::array<int,2>{0,-1}) &&
          !steam_startup_window_skin({240,216,0,0,Role::slot_title,0},std::numeric_limits<int>::max()) &&
          !steam_startup_window3_skin(81,240,7,9) &&
          !steam_startup_window3_skin(81,40,std::numeric_limits<int>::min(),9) &&
          !steam_startup_box_skin({26,90,29,148},0) &&
          !steam_startup_box_skin({0,0,10,std::numeric_limits<int>::max()},4),
          "不可裁片尺寸、负测宽与坐标溢出作为维护拒绝，不伪造原保护");
    const auto corner_sprite=sprite(root/"common/wnd_conner.seb");
    check(corner_sprite.frame_count==4 && corner_sprite.layers.size()==1 &&
          corner_sprite.layers[0].parts.size()==4,"原窗角SEB为四帧单层");
    const std::array<std::array<int,4>,4> corner_oracle{{{0,0,0,0},{4,0,-4,0},
                                                       {4,4,-4,-4},{0,4,0,-4}}};
    for (int n=0;n<4;++n) {
        const auto &part=corner_sprite.layers[0].parts[n];
        check(part.image_index==30 && part.width==4 && part.height==4 &&
              part.source_x==corner_oracle[n][0] && part.source_y==corner_oracle[n][1] &&
              part.offset_x==corner_oracle[n][2] && part.offset_y==corner_oracle[n][3],
              "白角只替换图片，原四裁片偏移仍由SEB解读一次");
    }
    const auto title=steam_title_menu_skin(600,380,75,1);
    check(title && title->origin==std::array<int,2>{180,0},"Steam标题逻辑原点按宽度居中");
    const auto title_images=parts<SteamStartupImage>(*title);
    const auto title_text=parts<SteamStartupText>(*title);
    check(title_images.size()==2 && title_images[0].asset==Asset::title_menu &&
          title_images[0].position==std::array<int,2>{137,308} &&
          title_images[1].position==std::array<int,2>{127,329},"菜单底图与选中第二行剑形指示");
    check(title_text.size()==2 && title_text[0].rectangle==std::array<double,4>{157,320,63,16} &&
          title_text[1].rectangle==std::array<double,4>{157,342,63,16} &&
          title_text[0].anchor==0x22,"Steam标题用TextLayout矩形，不用APK中心文字");
    check(title->touches.size()==2 && title->touches[1].component==0 &&
          title->touches[1].rectangle==std::array<int,4>{138,339,100,22},"标题两行原触摸矩形");
    check(steam_title_menu_skin(240,240,74,0)->draws.empty() &&
          steam_title_menu_skin(239,240,75,0)->origin[0]==0,"75准入及负奇数除2向零截断");
    check(!steam_title_menu_skin(0,240,75,0) && !steam_title_menu_skin(240,240,-1,0) &&
          !steam_title_menu_skin(240,240,75,2),"坏尺寸计数选择拒绝");

    SteamSaveSelectorSkinInput selected;
    selected.width=600; selected.height=380; selected.frame=19; selected.slot=1;
    selected.present={false,true};
    auto selector=steam_save_selector_skin(selected);
    check(selector && selector->touches.size()==3,"空中断不注册行，保留两箭头和手动行");
    const auto windows=parts<SteamStartupWindow>(*selector);
    check(windows.size()==1 && windows[0].width==198 && windows[0].height==146 &&
          windows[0].vertical==70 && windows[0].style==-12 && windows[0].title_value==2,
          "选档窗口保留原helper尺寸/偏移及槽号加1");
    const auto save_images=parts<SteamStartupImage>(*selector);
    check(save_images[0].frame==3 && save_images[0].position==std::array<int,2>{31,124} &&
          save_images[1].frame==0 && save_images[1].position==std::array<int,2>{206,124},
          "选档箭头位移读取frame%20，不自行推进");
    check(save_images[2].asset==Asset::save_icon && save_images[2].crop==std::array<int,4>{2,23,25,20} &&
          save_images[2].position==std::array<int,2>{35,167} &&
          save_images[3].crop==std::array<int,4>{6,3,16,16} &&
          save_images[3].position==std::array<int,2>{39,207},"空中断和非空手动分别使用独立裁片与偏移");
    const auto fills=parts<SteamStartupFill>(*selector);
    check(fills.size()==2 && fills[0].source_ratio==128 &&
          fills[0].rectangle==std::array<double,4>{29,154,181,41} &&
          fills[1].rectangle==std::array<double,4>{75,207,102,14},"空中断遮罩和非日文选中区不能混用");
    const auto slot_texts=parts<SteamStartupText>(*selector);
    check(slot_texts[0].anchor==2 && slot_texts[0].rgb==std::array<int,3>{60,100,200} &&
          slot_texts[1].anchor==2 && slot_texts[1].rgb==std::array<int,3>{30,30,30} &&
          slot_texts[2].anchor==0x22 && slot_texts[2].rgb==std::array<int,3>{30,30,30} &&
          !slot_texts[3].anchor && slot_texts[3].rgb==std::array<int,3>{30,30,30} &&
          slot_texts[4].anchor==4 && slot_texts[4].rgb==std::array<int,3>{30,30,30},
          "Steam类型、空栏及现有档案各自显式样式，日期不猜测重载锚点");
    check(selector->touches[0].image_draw.has_value() && !selector->touches[0].rectangle &&
          selector->touches[2].rectangle==std::array<int,4>{0,196,240,51},"箭头保留helper注册，行保留基矩形");
    selected.japanese=true;
    check(parts<SteamStartupFill>(*steam_save_selector_skin(selected))[1].rectangle==
          std::array<double,4>{79,207,94,14},"日文只改变选中背景宽度");
    selected.title_on_top=false;
    selector=steam_save_selector_skin(selected);
    check(selector->touches.size()==1 && parts<SteamStartupImage>(*selector).size()==4,
          "菜单覆盖时保留选档但不注册箭头或画选中手形");
    selected.covered_by_dialog=true;
    check(steam_save_selector_skin(selected)->draws.empty(),"询问覆盖选档整块隐藏");
    selected.covered_by_dialog=false; selected.row=0;
    check(!steam_save_selector_skin(selected),"空中断不得作为有效选择由纯查询静默矫正");
    selected.present={true,false}; selected.title_on_top=true;
    selector=steam_save_selector_skin(selected);
    check(selector && selector->touches.size()==4,"有效中断和空手动均注册行");
    selected.row=1;
    selector=steam_save_selector_skin(selected);
    const auto empty_fills=parts<SteamStartupFill>(*selector);
    check(empty_fills.size()==1 && empty_fills[0].rectangle==std::array<double,4>{85,215,82,14} &&
          empty_fills[0].rgb==std::array<int,3>{200,200,255},
          "空手动选中保留原独立底色，不沿用非空村名区域");
    selected.title_frame=74;
    check(steam_save_selector_skin(selected)->draws.empty(),"选档同样受标题75绘制门槛");

    SteamSaveMenuSkinInput menu;
    menu.frame=3; menu.selection=1; menu.measured_text_widths={80,81,12};
    auto plan=steam_save_menu_skin(menu);
    check(plan && plan->origin==std::array<int,2>{136,144},"raw20原点是136/144");
    const auto images=parts<SteamStartupImage>(*plan);
    const auto texts=parts<SteamStartupText>(*plan);
    check(images.size()==4 && images[0].frame==3 && images[1].frame==2 && images[3].frame==3 &&
          images[2].asset==Asset::finger_left && images[2].position==std::array<int,2>{96,41},
          "raw20共用menu SEB2/3与当前手形，保持穿插绘制顺序");
    check(texts.size()==3 && texts[0].font_size==0 && texts[1].font_size==11 &&
          texts[1].rectangle==std::array<double,4>{7,37,0,0},"真实测宽超过m-10才临时请求11号字");
    check(plan->touches.size()==4 && plan->touches[0].rectangle==std::array<int,4>{40,0,50,28} &&
          plan->touches[3].component==4 && plan->touches[3].value==22 && plan->touches[3].option==2 &&
          !plan->touches[3].rectangle,"原行窄矩形与KEYCLICK helper分别保留");
    menu.width=200; menu.height=260;
    check(steam_save_menu_skin(menu)->origin==std::array<int,2>{116,154},
          "SubForm任一维大于240即同时加两维偏移，窄维也产生负修正");
    menu.width=200; menu.height=200; menu.frame=2;
    plan=steam_save_menu_skin(menu);
    check(parts<SteamStartupText>(*plan).empty() && parts<SteamStartupImage>(*plan).size()==3 &&
          parts<SteamStartupImage>(*plan)[0].position==std::array<int,2>{-25,-32} &&
          parts<SteamStartupImage>(*plan)[0].scale==666,"越界菜单平移、未满3无文字而图块保留原缩放");
    menu.frame=3; menu.japanese=true; menu.measured_text_widths={74,75,12};
    plan=steam_save_menu_skin(menu);
    check(plan->touches[0].rectangle==std::array<int,4>{20,-33,44,28} &&
          parts<SteamStartupText>(*plan)[0].font_size==0 && parts<SteamStartupText>(*plan)[1].font_size==11,
          "日文宽84与资源宽89分开，阈值74独立验证");
    menu.covered_by_dialog=true;
    check(steam_save_menu_skin(menu)->draws.empty(),"raw1覆盖raw20不透明叠画旧菜单");
    menu.covered_by_dialog=false; menu.frame=4;
    check(parts<SteamStartupImage>(*steam_save_menu_skin(menu))[0].scale==1333,
          "DrawMenu2纯查询不擅自套FrameMenu夹3；不据此声称frame4自然可达");
    menu.frame=std::numeric_limits<int>::max();
    check(parts<SteamStartupImage>(*steam_save_menu_skin(menu))[0].scale==-333,
          "原imul32回卷后有符号除3，极值不触发C++未定义溢出");

    const auto dialog=steam_save_confirmation_skin(600,380,1,{22.5,31.5});
    check(dialog && dialog->origin==std::array<int,2>{180,70},"raw1单独SubForm居中原点");
    check(steam_save_confirmation_skin(200,260,1,{22.5,31.5})->origin==std::array<int,2>{-20,10},
          "询问在混合宽高窗口保留两维共同居中条件");
    const auto dialog_texts=parts<SteamStartupText>(*dialog);
    check(dialog_texts.size()==3 && dialog_texts[0].role==Role::message_body &&
          dialog_texts[0].rectangle==std::array<double,4>{26,115,187,32} &&
          dialog_texts[0].line_space==6 && dialog_texts[1].anchor==2 &&
          dialog_texts[1].rgb==std::array<int,3>{92,51,31} &&
          dialog_texts[2].rgb==std::array<int,3>{92,51,31},
          "正文保留TextLayout/换行间隔，两横向按钮独立锚点");
    const auto highlight=parts<SteamStartupFill>(*dialog);
    check(highlight.size()==1 && highlight[0].rectangle==std::array<double,4>{134.25,164,43.5,17},
          "共同按钮宽取最大实测值加2，默认否选中来自显式selection1");
    check(dialog->touches[0].rectangle==std::array<int,4>{-37,64,243,217} &&
          dialog->touches[1].rectangle==std::array<int,4>{34,64,243,217},"宽按钮原基矩形及负数向零截断");
    check(!steam_save_confirmation_skin(240,240,2,{0,0}) &&
          !steam_save_confirmation_skin(240,240,1,{-1,0}) &&
          !steam_save_confirmation_skin(240,240,1,{std::numeric_limits<float>::infinity(),0}) &&
          !steam_save_confirmation_skin(240,240,1,{std::numeric_limits<float>::quiet_NaN(),0}),
          "非法选择与不可用字体测宽显式拒绝");

    // 独立SEB源oracle：避免“生成计划再用同一公式验证自己”。现默认common字节已跨版核同。
    const auto menu_sprite=sprite(root/"common/menu.seb");
    check(menu_sprite.frame_count==4 && menu_sprite.layers.size()==1 &&
          menu_sprite.layers[0].parts.size()==4,"原menu四帧单层");
    const auto &m2=menu_sprite.layers[0].parts[2], &m3=menu_sprite.layers[0].parts[3];
    check(m2.image_index==25 && m2.source_x==68 && m2.source_y==0 && m2.width==89 && m2.height==29 &&
          m3.source_x==68 && m3.source_y==29 && m3.width==89 && m3.height==29,
          "原menu2/3裁片89x29，不用90布局宽改图");
    const auto left=steam_startup_resource(Asset::finger_left);
    check(left && left->image==70 && left->sprite==22 && std::string(left->image_name)=="finger_r.png" &&
          sprite(root/"common/finger_l.seb").layers[0].parts[0].image_index==70,
          "左手SEB真实引用右手图片，不能虚构finger_l.png");
    const auto icon=steam_startup_resource(Asset::save_icon);
    check(icon && icon->image==177 && icon->sprite==-1 && icon->language_variant,
          "saveload177保留语言变体资格，不能由本函数猜中文fallback");
    check(!steam_startup_resource(static_cast<Asset>(999)),"未知资源枚举拒绝");
    main_menu_notices(check);
    main_menu(check,root);
    save_page(check);
    manual_page(check,root);
    record_page(check);
    record_update(check);
    font_metrics(check);
    touch_commands(check);
    touch_menu_and_marker(check);
    dialogue_page(check);
    event_card(check,root);
    news_page(check);
    return check.count;
}
