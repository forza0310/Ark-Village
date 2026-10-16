#include "dungeon_village_prototype/steam_task_selection_skin.hpp"
#include "dungeon_village_prototype/steam_task_roster_skin.hpp"
#include "dungeon_village_prototype/steam_task_result_skin.hpp"
#include "dungeon_village_prototype/steam_task_deadline_skin.hpp"
#include "dungeon_village_prototype/steam_dungeon_footer_skin.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_tools/sprite.hpp"
#include "support/world_fixture.hpp"
#include <raylib.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_prototype;
namespace {
struct Checks {
    int count{};
    void operator()(bool ok,const char *why) {
        ++count;if(!ok)throw std::runtime_error(std::string("Steam任务选择皮肤：")+why);
    }
};
template<class T,class Plan> std::vector<T> parts(const Plan &p) {
    std::vector<T> out;
    for(const auto &op:p.operations)if(const auto *v=std::get_if<T>(&op))out.push_back(*v);
    return out;
}
SteamTaskSelectionInput input(int raw) {
    // 明确的表现组合夹具，不认证自然任务解锁、收费或人员募集。
    SteamTaskSelectionInput in;in.raw=raw;in.options.title_widths=std::array<int,2>{60,62};
    in.options.bottom_width=240;in.options.phrase_widths.fill(36);
    in.fee=321;in.affordable=false;
    for(int i=0;i<7;++i)in.rows.push_back({std::uint64_t(i+1),i%2,3+i,100+i,i==0,i%2==0,"任务条件"});
    if(raw==24) {
        in.recruitment=ref::TaskRecruitmentAnimation{};
        in.humans.push_back({1,0,0,-1,3,2,"报名者"});
    }
    return in;
}
void directory(Checks &c) {
    // SubForm._draw VA10353269–10353798：五行19槽、Touch11无marker、宝箱读treasureLv_。
    auto in=input(22);auto plan=steam_task_selection_skin(in);c(bool(plan),"22原五行计划");
    c(plan->title=="任务选择"&&plan->soft_labels==std::array<int,2>{6,2},"22左详情、右返回");
    const auto touches=parts<SteamStartupTouch>(*plan);
    c(touches.size()==5&&touches.front().component==11&&touches.front().value==0x20000&&
      touches.front().option==0&&touches.front().rectangle==std::optional<std::array<int,4>>{{10,97,230,16}}&&
      touches.back().rectangle==std::optional<std::array<int,4>>{{10,173,230,16}},"22原五行热区与绝对行号，不沿赠礼marker16");
    c(std::any_of(plan->operations.begin(),plan->operations.end(),[](const auto &op){
        const auto *frame=std::get_if<SteamStartupFrameDraw>(&op);
        const auto *r=frame?std::get_if<StartupSkinRect>(frame):nullptr;
        return r&&r->rect==std::array<int,4>{23,95,191,16}&&r->rgb==std::array<int,3>{255,153,55};
    }),"22选中橙底与源行坐标一致，不由触摸热区代替绘制框");
    const auto images=parts<SteamHumanImage>(*plan);
    c(images.size()==1&&images[0].asset==SteamHumanAsset::catalogue_new&&images[0].position==std::array<int,2>{10,99},
      "NEW来自任务实例已阅标志，原image147");
    const auto treasure=parts<SteamTaskSelectionImage>(*plan);
    c(treasure.size()==3&&treasure[0].asset==SteamTaskSelectionAsset::treasure&&treasure[0].frame==2&&
      treasure[0].position==std::array<int,2>{136,109},"kind0画SEB27宝箱frame2，kind1不画");
    const auto numbers=parts<SteamFacilityNumber>(*plan);
    c(numbers.size()==8&&numbers[0].value==3&&numbers[0].asset==SteamFacilityAsset::number03&&
      numbers[0].position==std::array<int,2>{156,97},"宝箱等级与报名费分开，不读怪物数");
    c(numbers[1].kind==SteamFacilityNumberKind::money&&numbers[1].asset==SteamFacilityAsset::number08&&
      numbers[1].value==100&&numbers[1].position==std::array<int,2>{214,98}&&
      numbers[2].asset==SteamFacilityAsset::number12&&numbers[2].value==101,"费用足够SEB15、不足SEB19");
    auto texts=parts<SteamTaskSelectionText>(*plan);
    c(texts[2].rectangle==std::array<int,4>{29,99,0,0}&&texts[2].font_size==9&&
      texts.back().rectangle==std::array<int,4>{12,197,216,16}&&texts.back().anchor==34,
      "非JP行名9号，底说明测宽夹200并保留布局锚点");
    const auto bars=parts<SteamHumanCatalogueScroll>(*plan);
    c(bars.size()==1&&bars[0].arguments==std::array<int,3>{7,5,0x20000}&&
      bars[0].touch.rectangle==std::optional<std::array<int,4>>{{221,85,3,111}}&&
      bars[0].rectangles[1].rect==std::array<int,4>{220,85,5,80},"22原111高轨道、真实人数与五可见行");
    in.first=in.selection=6;in.options.scroll_first_touch=true;plan=steam_task_selection_skin(in);
    c(plan&&parts<SteamStartupTouch>(*plan).size()==1&&parts<SteamStartupTouch>(*plan)[0].value==(0x20000|6),
      "合法尾窗不硬凑五行或改首绘行");
    const auto tail=parts<SteamHumanCatalogueScroll>(*plan);
    c(tail[0].rectangles[1].rect==std::array<int,4>{220,180,5,80}&&
      tail[0].rectangles[1].rgb==std::array<int,3>{246,129,0},"scroll原比例允许短尾，按真实触摸资格改色");
    in.options.japanese=true;plan=steam_task_selection_skin(in);texts=parts<SteamTaskSelectionText>(*plan);
    c(texts[2].rectangle==std::array<int,4>{29,98,0,0}&&texts[2].font_size==0,"JP行名默认字号与独立y偏移");
    for(int bad:{-1,7}) {auto broken=in;broken.selection=bad;c(!steam_task_selection_skin(broken),"选择越界显式拒绝");}
    auto broken=in;broken.first=7;c(!steam_task_selection_skin(broken),"首行不越过选择");
    broken=in;broken.rows[6].fee=-1;c(!steam_task_selection_skin(broken),"可见行非法费用拒绝");
    broken=in;broken.options.bottom_width.reset();c(!steam_task_selection_skin(broken),"说明缺实际测宽拒绝");
}
void prompt(Checks &c) {
    // VA10332B89–10332D59：费用是普通蓝色DrawString＋DrawMoney，不是富文本。
    for(bool affordable:{false,true})for(int selection:{0,1}) {
        auto in=input(23);in.affordable=affordable;in.selection=selection;
        const auto plan=steam_task_selection_skin(in);c(bool(plan),"23两种selection均只有同一募集按钮");
        c(plan->soft_labels==std::array<int,2>{0,2},"23独立返回取消，不增第二确认按钮");
        const auto touches=parts<SteamStartupTouch>(*plan);
        c(touches.size()==1&&touches[0].component==4&&touches[0].value==20&&touches[0].option==0&&
          touches[0].rectangle==std::optional<std::array<int,4>>{{-8,164,256,56}},"单BTMT按钮夹200，Touch4确认无marker");
        const auto words=parts<SteamTaskSelectionText>(*plan);
        c(words[0].value=="费用"&&words[0].rectangle==std::array<int,4>{46,164,0,0}&&
          !words[0].layout&&words[0].rgb==std::array<int,3>{0,100,255},"费用标签原蓝普通文字，不能补原源没有的富文本");
        const auto numbers=parts<SteamFacilityNumber>(*plan);
        c(numbers.size()==1&&numbers[0].value==321&&numbers[0].kind==SteamFacilityNumberKind::money&&
          numbers[0].asset==(affordable?SteamFacilityAsset::number08:SteamFacilityAsset::number12)&&
          numbers[0].position==std::array<int,2>{194,165},"23费用数读已绑定任务且保留不足颜色");
        const auto fingers=parts<SteamFacilityImage>(*plan);
        c(fingers.size()==1&&fingers[0].asset==SteamFacilityAsset::finger&&fingers[0].position==std::array<int,2>{8,193},
          "23手形为按钮left−4及y+9");
    }
    auto in=input(23);in.selection=2;c(!steam_task_selection_skin(in),"23越界选择拒绝");
    in=input(23);in.options.bottom_width.reset();c(!steam_task_selection_skin(in),"23缺按钮实际测宽拒绝");
}
void recruitment(Checks &c) {
    // VA10332116–10332B88：X入场三段整除位置，130及负计数隐藏；Y与as独立。
    constexpr std::array<std::array<int,3>,10> poses{{{-1,0,0},{0,220,142},{20,185,142},{79,82,142},
        {80,80,142},{95,71,132},{109,62,122},{110,61,121},{129,23,121},{130,0,0}}};
    for(const auto &pose:poses) {
        auto in=input(24);in.counter=12;in.recruitment->entries.push_back({1,pose[0],-1});
        in.recruitment->displayed_count=7;
        const auto plan=steam_task_selection_skin(in);c(bool(plan),"24合法到场分段");
        const auto actors=parts<SteamHumanPortrait>(*plan);
        c(actors.size()==std::size_t(pose[0]>=0&&pose[0]<130),"负延迟和到场结束不绘人物");
        if(!actors.empty())c(actors[0].position==std::array<int,2>{pose[1],pose[2]}&&actors[0].actor.body.frame==3&&actors[0].actor.body.sprite==2,
            "原三个RateConvert向零截断与(counter%16)/4步长");
        const auto numbers=parts<SteamFacilityNumber>(*plan);
        c(numbers.size()==1&&numbers[0].value==7&&numbers[0].asset==SteamFacilityAsset::number03_hud&&
          numbers[0].position==std::array<int,2>{188,174},"显示人数读as7，不以X.size1替代");
        const auto touch=parts<SteamStartupTouch>(*plan);
        c(touch.size()==1&&touch[0].component==5&&touch[0].value==20&&!touch[0].rectangle&&touch[0].option==2&&
          plan->soft_labels==std::array<int,2>{0,0},"24无矩形held请求，纯绘制不执行加速");
        const auto &ops=plan->operations;
        const auto begin=std::find_if(ops.begin(),ops.end(),[](const auto &op){const auto *v=std::get_if<SteamTaskSelectionClip>(&op);return v&&v->push;});
        c(begin!=ops.end()&&std::get<SteamTaskSelectionClip>(*begin).rectangle==std::array<int,4>{30,67,180,80},"募集背景原clip");
        const auto background=std::find_if(ops.begin(),begin,[](const auto &op){const auto *v=std::get_if<SteamTaskSelectionImage>(&op);return v&&!v->crop;});
        c(background!=begin&&std::get<SteamTaskSelectionImage>(*background).position==std::array<int,2>{30,67},
          "整幅背景先画，再clip入场人物");
        const auto end=std::find_if(begin,ops.end(),[](const auto &op){const auto *v=std::get_if<SteamTaskSelectionClip>(&op);return v&&!v->push;});
        c(end!=ops.end()&&std::holds_alternative<SteamTaskSelectionImage>(*(end-1))&&
          std::get<SteamTaskSelectionImage>(*(end-1)).position==std::array<int,2>{30,105}&&
          std::get<SteamTaskSelectionImage>(*(end-1)).crop==std::optional<std::array<int,4>>{{0,38,19,22}},
          "人物后补前景裁片，再恢复clip，禁止改变遮挡序");
    }
    for(int age:{19,20,54,55})for(int phrase:{-1,0,5}) {
        auto in=input(24);in.recruitment->entries.push_back({1,age,phrase});
        const auto plan=steam_task_selection_skin(in);c(bool(plan),"24使用冻结phrase，不重抽发言");
        const auto words=parts<SteamTaskSelectionText>(*plan);
        const bool visible=phrase>=0&&age>=20&&age<55;
        c(std::count_if(words.begin(),words.end(),[](const auto &t){return t.value=="交给我!"||t.value=="好期待";})==int(visible),
          "原发言仅20至54可见，−1不补句子");
        if(visible)c(words[0].value==(phrase==0?"交给我!":"好期待")&&words[0].rectangle[1]==100,
          "冻结原六句索引与气泡y142−45+3");
    }
    for(int medals:{0,4,5})for(int level:{3,10}) {
        auto in=input(24);in.recruitment->portraits={1};in.humans[0].medals=medals;in.humans[0].level=level;
        const auto plan=steam_task_selection_skin(in);c(bool(plan),"24Y首人物独立肖像");
        const auto actors=parts<SteamHumanPortrait>(*plan);
        c(actors.size()==1&&actors[0].position==std::array<int,2>{48,183}&&actors[0].actor.body.sprite==1,
          "Y肖像在背景clip恢复后绘48,183、独立facing1");
        const auto decorations=parts<SteamHumanImage>(*plan);
        c(decorations.size()==std::size_t(medals<5?medals:1),"少于五枚原序逐图，五枚起只一勋章");
        if(medals)c(decorations[0].asset==SteamHumanAsset::medal&&decorations[0].position==std::array<int,2>{69,157}&&
          decorations[0].crop==std::optional<std::array<int,4>>{{14,0,14,14}},"勋章源类别6裁片与肖像独立坐标");
        const auto marks=parts<SteamFacilityImage>(*plan);
        c(std::count_if(marks.begin(),marks.end(),[](const auto &v){return v.asset==SteamFacilityAsset::maximum&&v.position==std::array<int,2>{90,176};})==int(level==10),
          "十级显示MAX而非10数字");
        const auto numbers=parts<SteamFacilityNumber>(*plan);
        const auto names=parts<SteamTaskSelectionText>(*plan);
        c(names[0].value=="报名者"&&names[0].rectangle==std::array<int,4>{69,191,0,0}&&names[0].anchor==2,
          "Y首人物名原中心锚，与人数标签分开");
        if(level==3)c(std::any_of(numbers.begin(),numbers.end(),[](const auto &n){return n.value==3&&n.position==std::array<int,2>{105,174};}),
          "未满级只画人物真实等级number05右105/174");
        if(medals==5)c(std::any_of(marks.begin(),marks.end(),[](const auto &v){return v.asset==SteamFacilityAsset::number05&&v.frame==16&&v.position==std::array<int,2>{81,161};})&&
          std::any_of(numbers.begin(),numbers.end(),[](const auto &n){return n.value==5&&n.position==std::array<int,2>{91,161}&&n.anchor==1;}),
          "五枚勋章改原乘号16＋左锚数，不能沿人物概览反向展开");
    }
    auto bad=input(24);bad.recruitment.reset();c(!steam_task_selection_skin(bad),"24缺冻结募集载荷拒绝");
    bad=input(24);bad.recruitment->entries={{1,20,0}};bad.options.phrase_widths[0].reset();c(!steam_task_selection_skin(bad),"实际可见发言缺测宽拒绝");
    bad=input(24);bad.recruitment->entries={{999,0,-1}};c(!steam_task_selection_skin(bad),"可见到场人缺定义拒绝");
}
void owner(Checks &c) {
    constexpr std::uint64_t task_id=4201;
    for(int raw:{22,23,24}) {
        auto s=test_support::page_fixture(raw);const auto id=s.scripts.pages.back().id;
        const auto &definition=s.rules->tasks.front();
        s.tasks.emplace(task_id,ref::DungeonFinishTask{task_id,definition.factory.identity,3,0,{}, {}});
        s.next_task_identity=std::max(s.next_task_identity,task_id+1);
        s.scripts.pages.back().lifecycle=2;s.scripts.executing_page.reset();
        s.page_counters[id]=0;
        if(raw==22) {s.task_page_lists[id]={task_id};s.page_phases[id]=0;s.page_secondary_counters[id]=0;}
        else {
            s.scripts.pages.back().task_identity=task_id;
            s.scripts.pages.back().task_definition=definition.factory.identity;
            if(raw==23)s.page_phases[id]=0;
            else {s.task_recruitment_pages[id]={};s.page_secondary_counters[id]=0;}
        }
        const auto options=input(raw).options;
        const auto plan=steam_task_selection_skin(s,id,options);c(bool(plan),"真实规则定义的完整Owner只读桥");
        const auto before=startup_world_state_digest(s);
        const auto draws=s.scene.random.draws();
        const auto sounds=s.sound_requests;
        for(int repeat=0;repeat<4;++repeat) {
            const auto again=steam_task_selection_skin(s,id,options);
            c(again&&again->operations.size()==plan->operations.size()&&startup_world_state_digest(s)==before&&
              s.scene.random.draws()==draws&&s.sound_requests==sounds,
              "重复查询规模稳定且不收费、报名、随机、声音或改栈");
        }
        auto bad=s;bad.page_counters.erase(id);c(!steam_task_selection_skin(bad,id,options),"缺主钟拒绝，不用map.at异常");
        bad=s;bad.page_counters[id]=std::numeric_limits<int>::max();c(!steam_task_selection_skin(bad,id,options),"Owner禁止的溢出主钟不可经只读桥绕过");
        bad=s;bad.scripts.pages.back().lifecycle=4;c(!steam_task_selection_skin(bad,id,options),"退休不留绘制引用");
        bad=s;bad.tasks.erase(task_id);c(!steam_task_selection_skin(bad,id,options),"悬空实例身份拒绝，不用定义ID补实例");
        if(raw==22) {
            bad=s;bad.task_page_lists.erase(id);c(!steam_task_selection_skin(bad,id,options),"22缺冻结目录拒绝");
            bad=s;bad.page_phases.erase(id);c(!steam_task_selection_skin(bad,id,options),"22缺原选择拒绝");
            bad=s;bad.page_secondary_counters.erase(id);c(!steam_task_selection_skin(bad,id,options),"22缺独立首行拒绝");
            bad=s;bad.page_phases[id]=1;c(!steam_task_selection_skin(bad,id,options),"22越界选择不偷偷归零");
            bad=s;bad.task_recruitment_pages[id]={};c(!steam_task_selection_skin(bad,id,options),"22不得带24错型冻结载荷");
            const auto repeatable=std::find_if(s.rules->tasks.begin(),s.rules->tasks.end(),[](const auto &d){
                return (d.factory.flags&4U)!=0&&(d.factory.flags&2U)==0;
            });
            c(repeatable!=s.rules->tasks.end(),"固定表有独立复发倍率任务");
            auto repeated=s;repeated.tasks.at(task_id).definition=repeatable->factory.identity;
            repeated.task_progress.definitions.at(repeatable->factory.identity).completed=3;
            const auto named=steam_task_selection_skin(repeated,id,options);c(bool(named),"22合法复发任务名");
            const auto names=parts<SteamTaskSelectionText>(*named);
            c(std::any_of(names.begin(),names.end(),[&](const auto &v){return v.value==repeatable->name+"+6";}),
              "GetName(false)实际完结三次显示+6，不只复制原表基础名");
        } else {
            bad=s;bad.scripts.pages.back().task_definition.reset();c(!steam_task_selection_skin(bad,id,options),"23/24缺定义绑定拒绝");
            if(raw==23) {bad=s;bad.page_phases.erase(id);c(!steam_task_selection_skin(bad,id,options),"23缺按钮选择拒绝");}
            else {
                bad=s;bad.task_recruitment_pages.erase(id);c(!steam_task_selection_skin(bad,id,options),"24缺冻结X/Y拒绝");
                bad=s;bad.page_secondary_counters.erase(id);c(!steam_task_selection_skin(bad,id,options),"24缺held副钟拒绝，不借绘制补齐");
                bad=s;bad.task_recruitment_pages[id].entries={{9999,0,-1}};
                c(!steam_task_selection_skin(bad,id,options),"24当前可见人缺共享详情拒绝");
            }
        }
    }
}
std::vector<std::uint8_t> bytes(const std::filesystem::path &p) {
    std::ifstream in(p,std::ios::binary);if(!in)throw std::runtime_error("任务皮肤资源缺失："+p.string());
    return {std::istreambuf_iterator<char>(in),{}};
}
void resources(Checks &c,const std::filesystem::path &source_root) {
    const auto root=source_root.parent_path();
    const auto background=steam_task_selection_resource(SteamTaskSelectionAsset::recruitment_background);
    const auto treasure=steam_task_selection_resource(SteamTaskSelectionAsset::treasure);
    c(background&&background->image==0&&background->sprite==-1&&!background->published_sprite,
      "背景为event/image0整图，不能假造SEB");
    c(treasure&&treasure->image==6&&treasure->sprite==27&&treasure->published_sprite,
      "宝箱为common/image6/SEB27");
    for(const auto &r:{*background,*treasure}) {
        const auto path=root/r.published_image;
        c(std::filesystem::is_regular_file(path),"具名正式PNG存在");
        auto png=LoadImage(path.string().c_str());
        c(png.data!=nullptr,"正式PNG可解码");
        const int w=png.width,h=png.height;UnloadImage(png);
        c(w==(r.image==0?180:22)&&h==(r.image==0?80:95),"原背景180×80与宝箱22×95，不以截图推裁片");
        if(r.published_sprite) {
            const auto seb=dungeon_village_tools::parse_legacy_seb(bytes(root/r.published_sprite));
            c(seb.frame_count==6,"treasure00真实六帧，所用frame2在界内");
            bool frame2{};
            for(const auto &layer:seb.layers)for(const auto &p:layer.parts) {
                frame2|=p.frame==2;
                c(p.source_x>=0&&p.source_y>=0&&p.width>0&&p.height>0&&
                  p.source_x+p.width<=w&&p.source_y+p.height<=h,"真实宝箱SEB记录全部在PNG内");
            }
            c(frame2,"绘制frame2必须具有原资源记录，不能仅看帧数量");
        }
    }
    const auto badge=steam_human_resource(SteamHumanAsset::catalogue_new);
    c(badge&&badge->image==147&&std::filesystem::is_regular_file(root/badge->published_image),
      "任务NEW只复用正式image147，不能替换GET148");
}
SteamTaskRosterInput roster_input(int raw) {
    SteamTaskRosterInput in;in.raw=raw;in.participant_count=2;in.task_name="任务条件";
    in.options.title_widths=std::array<int,2>{60,62};in.options.prediction_width=40;
    in.options.button_widths={240,24};
    for(int i=0;i<6;++i) {
        SteamTaskRosterHuman h;h.identity=i;h.profession=0;h.name="参加者条件";h.level=i==0?10:3;
        h.effort=41;h.medals=7;h.fee=123;h.combat={101,202,303,404};h.affordable=i%2==0;
        in.humans.push_back(h);in.frozen_humans.push_back(i);
    }
    if(raw<=26)in.frozen_humans.push_back(-1);
    if(raw==28)in.participant_count=6;
    return in;
}
void rosters(Checks &c) {
    // Steam25/26 RVA331996；APK b/g:938–1001，冻结X末−1与实时m.size独立。
    for(int raw:{25,26}) {
        auto in=roster_input(raw);auto plan=steam_task_roster_skin(in);c(bool(plan),"25/26合法冻结名单");
        c(plan->soft_labels==std::array<int,2>{7,raw==25?12:2},"25开始脚标与26返回不混用");
        const auto words=parts<SteamTaskSelectionText>(*plan);
        c(words[0].value=="2名参加"&&words[0].rectangle==std::array<int,4>{74,69,0,0}&&
          words[1].rectangle==std::array<int,4>{161,69,60,11}&&words[1].anchor==34&&words[1].layout,
          "人数读实时2，不用六个冻结人重算；Steam加油用TextLayout");
        const auto touches=parts<SteamTaskRosterTouch>(*plan);
        c(touches.size()==5&&touches[0].touch.option==0&&touches[0].touch.value==0x20000&&
          touches[0].touch.rectangle==std::optional<std::array<int,4>>{{10,97,230,16}}&&
          touches[0].margin==std::array<int,4>{0,-22,0,0},"25/26五行无marker、右margin−22");
        const auto portraits=parts<SteamHumanPortrait>(*plan);
        c(portraits.size()==5&&portraits[0].position==std::array<int,2>{37,118}&&
          portraits[0].clip==std::optional<std::array<int,4>>{{29,96,15,14}},"25/26原Draw_chara2的y−1 clip与y+21锚");
        const auto numbers=parts<SteamFacilityNumber>(*plan);
        c(numbers.front().value==41&&numbers.front().position==std::array<int,2>{207,97}&&
          numbers.front().asset==SteamFacilityAsset::number05,"加油列真实努力41，满级行不画10数");
        const auto images=parts<SteamFacilityImage>(*plan);
        c(std::any_of(images.begin(),images.end(),[](const auto &v){return v.asset==SteamFacilityAsset::maximum&&
          v.position==std::array<int,2>{140,99};}),"名单十级MAX用原image129锚");
        c(parts<SteamHumanCatalogueScroll>(*plan)[0].arguments==std::array<int,3>{7,5,0x20000},
          "滚动count包含末行追加委托，不减哨兵");
        in.selection=in.first=6;plan=steam_task_roster_skin(in);c(bool(plan),"末哨兵单行尾窗");
        c(parts<SteamHumanPortrait>(*plan).empty()&&parts<SteamTaskRosterTouch>(*plan).size()==1&&
          parts<SteamTaskRosterTouch>(*plan)[0].touch.value==(0x20000|6),"末−1只画追加委托，不造人物");
        const auto tail=parts<SteamTaskSelectionText>(*plan);
        c(std::any_of(tail.begin(),tail.end(),[](const auto &v){return v.value=="追加参加委托"&&v.rectangle==std::array<int,4>{29,97,0,0};}),
          "末行复用首绘行y97与绝对选择6");
        if(raw==26) {
            in.ongoing=true;plan=steam_task_roster_skin(in);
            c(parts<SteamTaskSelectionText>(*plan).back().value=="任务条件实施中","26仅原任务实施资格显示动态名后缀");
            in.task_name.clear();c(!steam_task_roster_skin(in),"26实施中缺任务名拒绝");
        }
    }
    // Steam27 RVA3311DE；APK b/g:1002–1064，四战斗值和共享E勋章槽。
    auto in=roster_input(27);auto plan=steam_task_roster_skin(in);c(bool(plan),"27费用页");
    const auto row=parts<SteamTaskRosterTouch>(*plan);
    c(row[0].touch.option==16&&row[0].margin==std::array<int,4>{29,29,0,0}&&
      row[0].touch.rectangle==std::optional<std::array<int,4>>{{23,95,191,16}},"27原marker16及双侧29，不沿25 margin");
    const auto portraits=parts<SteamHumanPortrait>(*plan);
    c(portraits[0].position==std::array<int,2>{37,117}&&portraits[0].clip==std::optional<std::array<int,4>>{{29,96,15,14}},
      "27原Draw_chara调用y−1，clip与25同但锚少一行");
    auto numbers=parts<SteamFacilityNumber>(*plan);
    c(numbers[0].kind==SteamFacilityNumberKind::money&&numbers[0].position==std::array<int,2>{212,98}&&
      numbers[0].asset==SteamFacilityAsset::number08&&numbers[2].asset==SteamFacilityAsset::number12,
      "27费用按真实负担资格用SEB15/19，不以努力或勋章替代费用");
    in.tab=1;plan=steam_task_roster_skin(in);c(bool(plan),"27四战斗值页");
    numbers=parts<SteamFacilityNumber>(*plan);
    for(int i=0;i<4;++i)c(numbers[i].value==101*(i+1)&&numbers[i].position==std::array<int,2>{80+36*i,99}&&
      numbers[i].asset==SteamFacilityAsset::number08,"四战斗值按HP/攻击/防御/魔法源序，不套六属性");
    c(numbers[4].value==7&&numbers[4].position==std::array<int,2>{211,98},"原E共享勋章数，独立于四战斗值");
    const auto icons=parts<SteamHumanImage>(*plan);
    c(icons.size()==5&&icons[0].asset==SteamHumanAsset::combat_icon&&icons[3].frame==3&&
      icons[4].asset==SteamHumanAsset::medal&&icons[4].crop==std::optional<std::array<int,4>>{{14,0,14,14}},
      "四战斗图标及原类别6勋章裁片，第二页不加人物名");
    for(int raw:{25,26,27}) {
        auto bad=roster_input(raw);bad.frozen_humans.push_back(0);c(!steam_task_roster_skin(bad),"重复冻结身份拒绝");
        bad=roster_input(raw);bad.frozen_humans[0]=999;c(!steam_task_roster_skin(bad),"缺冻结人物定义拒绝");
        bad=roster_input(raw);bad.first=1;c(!steam_task_roster_skin(bad),"首行大于选择拒绝，不改游标");
        bad=roster_input(raw);bad.humans[0].combat[0]=-1;c(!steam_task_roster_skin(bad),"坏四战斗值拒绝");
    }
    in=roster_input(25);in.frozen_humans.pop_back();c(!steam_task_roster_skin(in),"25缺末−1不能补哨兵");
    in=roster_input(27);in.frozen_humans.push_back(-1);c(!steam_task_roster_skin(in),"27禁止追加委托哨兵");
}
void departure(Checks &c) {
    // Steam28 RVA330630；APK b/g:1274–1349与原bi/bh动作门槛，不读页面FPS。
    auto in=roster_input(28);in.frozen_humans={0};in.participant_count=1;
    auto plan=steam_task_roster_skin(in);c(bool(plan),"28单人不除零或补中心");
    const auto actor=parts<SteamTaskDepartureActor>(*plan);
    c(actor.size()==1&&actor[0].position==std::array<int,2>{80,134},"单人Rate同端点返回left80，不自拟120");
    const auto words=parts<SteamTaskSelectionText>(*plan);
    c(words[0].value=="有些困难吧"&&words[0].rectangle==std::array<int,4>{150,164,0,0}&&words[0].anchor==4,
      "评价原索引0及实际测宽40+20的右锚");
    const auto touch=parts<SteamStartupTouch>(*plan);
    c(touch.size()==2&&touch[0].component==3&&touch[0].option==0&&
      touch[0].rectangle==std::optional<std::array<int,4>>{{31,167,98,56}}&&
      touch[1].rectangle==std::optional<std::array<int,4>>{{121,167,98,56}},"原两个固定Touch3不随文字宽变化");
    c(std::any_of(plan->operations.begin(),plan->operations.end(),[](const auto &op){
        const auto *f=std::get_if<SteamStartupFrameDraw>(&op);const auto *r=f?std::get_if<StartupSkinRect>(f):nullptr;
        return r&&r->rect==std::array<int,4>{-48,187,256,16}&&r->rgb==std::array<int,3>{255,153,55};
    }),"dx/dy按钮测宽240+16，不套底说明夹200");
    in.options.prediction_width.reset();c(!steam_task_roster_skin(in),"phase0缺评价测宽拒绝");
    in=roster_input(28);in.frozen_humans={0};in.participant_count=1;in.phase=1;in.secondary=12;
    in.options.prediction_width.reset();in.options.button_widths={};
    constexpr std::array<std::array<int,3>,13> oracle{{{1,0,80},{2,4,80},{8,0,80},{12,4,80},{18,0,80},
        {22,4,80},{35,4,80},{36,0,80},{40,0,80},{50,0,80},{70,0,-10},{90,0,-100},{91,0,-100}}};
    for(const auto &sample:oracle) {
        in.counter=sample[0];plan=steam_task_roster_skin(in);c(bool(plan),"出发动作/横移独立钟合法");
        const auto actors=parts<SteamTaskDepartureActor>(*plan);
        c(actors.size()==1&&actors[0].action==sample[1]&&actors[0].step==(sample[1]==4?0:3)&&
          actors[0].position==std::array<int,2>{sample[2],134},"bi2/40/50/90及bh6/10/16/20/34原边界");
        const auto body=steam_task_departure_actor_skin(actors[0]);
        c(body&&body->body.sprite==(sample[1]==4?22:2)&&body->body.frame==(sample[1]==4?0:3)&&!body->shadow,
          "action4身体SEB22/帧0，阴影单列而非普通walk帧或重复内嵌");
        const auto &ops=plan->operations;
        const auto at=std::find_if(ops.begin(),ops.end(),[](const auto &op){return std::holds_alternative<SteamTaskDepartureActor>(op);});
        c(at!=ops.begin()&&std::holds_alternative<SteamTaskRosterImage>(*(at-1))&&
          std::get<SteamTaskRosterImage>(*(at-1)).asset==SteamTaskRosterAsset::shadow&&
          std::get<SteamTaskRosterImage>(*(at-1)).position==actors[0].position,"阴影紧邻且先于当前身体/武器");
        const auto touches=parts<SteamStartupTouch>(*plan);
        c(touches.size()==1&&touches[0].component==2&&touches[0].option==2&&!touches[0].rectangle,
          "出发phase1无矩形Touch2，不重用phase0按钮注册");
        const auto clips=parts<SteamTaskSelectionClip>(*plan);
        c(clips.size()==2&&clips[0].push&&clips[0].rectangle==std::array<int,4>{30,66,180,82}&&!clips[1].push,
          "背景框内人物clip成对退休");
    }
    in.counter=8;in.secondary=4;plan=steam_task_roster_skin(in);
    c(parts<SteamTaskDepartureActor>(*plan)[0].step==1,"页计数8不决定身体帧，真实副钟4给step1");
    const auto weapon=std::find_if(startup_world_rules().equipment.begin(),startup_world_rules().equipment.end(),
        [](const auto &v){return v.shop.kind==1&&v.render_style==0;});
    c(weapon!=startup_world_rules().equipment.end(),"固定表有style0武器");
    SteamTaskDepartureActor raised{0,0,weapon->shop.id,4,0,{80,134}};
    const auto skin=steam_task_departure_actor_skin(raised);
    c(skin&&skin->weapon&&skin->weapon->offset==std::array<int,2>{-19,-37}&&skin->weapon->frame==0,
      "action4武器a/p.A样本为−19/−37及bf帧0，不沿动作0偏移");
    raised.step=1;c(!steam_task_departure_actor_skin(raised),"未核action4额外步拒绝");
    raised.step=0;raised.action=3;c(!steam_task_departure_actor_skin(raised),"helper不冒认其它动作已覆盖");
    auto bad=in;bad.participant_count=2;c(!steam_task_roster_skin(bad),"28实际m人数须匹配真实顺序，不用25独立X规则");
    bad=in;bad.prediction=7;c(!steam_task_roster_skin(bad),"评价索引越界显式拒绝");
    bad=in;bad.secondary=-1;c(!steam_task_roster_skin(bad),"坏副钟不由页计数补齐");
}
void roster_resources(Checks &c,const std::filesystem::path &source_root) {
    const auto root=source_root.parent_path();
    for(const auto asset:{SteamTaskRosterAsset::departure_background,SteamTaskRosterAsset::secretary_face,SteamTaskRosterAsset::shadow}) {
        const auto r=steam_task_roster_resource(asset);c(bool(r)&&std::filesystem::is_regular_file(root/r->published_image),"三资源正式路径存在");
        auto image=LoadImage((root/r->published_image).string().c_str());c(image.data!=nullptr,"出发资源PNG可解码");
        const int w=image.width,h=image.height;UnloadImage(image);
        c(w==(asset==SteamTaskRosterAsset::departure_background?93:asset==SteamTaskRosterAsset::secretary_face?34:123)&&
          h==(asset==SteamTaskRosterAsset::departure_background?82:asset==SteamTaskRosterAsset::secretary_face?27:3),
          "原mp_back93×82/秘书34×27/阴影123×3，各裁片独立");
        if(asset==SteamTaskRosterAsset::shadow)c(r->image==3&&r->sprite==25&&r->published_sprite&&
          dungeon_village_tools::parse_legacy_seb(bytes(root/r->published_sprite)).frame_count==4,"阴影SEB25真实四帧，phase1只请求帧0");
    }
    for(int raw:{25,26,27,28}) {
        const auto in=roster_input(raw);const auto plan=steam_task_roster_skin(in);c(bool(plan),"稳定只读计划");
        for(int repeat=0;repeat<4;++repeat) {
            const auto again=steam_task_roster_skin(in);
            c(again&&again->operations.size()==plan->operations.size(),"不变DTO重复绘制规模稳定，不保留跨查询池");
        }
        for(const auto &op:plan->operations)if(const auto *v=std::get_if<SteamTaskRosterImage>(&op)) {
            const auto resource=steam_task_roster_resource(v->asset);c(bool(resource),"每个新增资源图元都有消费映射");
            auto image=LoadImage((root/resource->published_image).string().c_str());c(image.data!=nullptr,"图元资源能解码");
            if(v->crop) {const auto &r=*v->crop;c(r[0]>=0&&r[1]>=0&&r[2]>0&&r[3]>0&&
                r[0]+r[2]<=image.width&&r[1]+r[3]<=image.height,"背景三片及秘书16×16裁片不越原PNG");}
            UnloadImage(image);
        }
    }
}
SteamTaskResultInput result_input(int raw) {
    SteamTaskResultInput in;in.raw=raw;in.task_name="任务条件";in.difficulty=7;
    in.options.title_widths=std::array<int,2>{24,26};in.options.bottom_width=240;
    in.footer={SteamTaskResultHuman{40,0,0,""},SteamTaskResultHuman{41,0,1,""}};
    if(raw==31)for(int i=0;i<7;++i)in.crew.push_back({{i,0,0,"队员"+std::to_string(i)},100+i,10+i});
    else {
        // 图标来自独立既有来源查询；展示价格是明确条件值，不冒认自然取得与售价结算。
        const auto s=test_support::page_fixture(32);
        for(int i=0;i<6;++i) {
            const int kind=i%4;
            SteamTaskResultReward r;r.kind=kind;r.value=i==1?0:1234;
            if(kind==0) {
                const auto &d=s.rules->items.front();r.definition=d.identity;r.name=d.name;
                r.icon=*startup_world_item_icon_draws(s,d.identity);
            } else {
                const auto d=std::find_if(s.rules->equipment.begin(),s.rules->equipment.end(),
                    [=](const auto &v){return v.shop.kind==kind;});
                if(d==s.rules->equipment.end())throw std::runtime_error("缺任务奖励图标源定义");
                r.definition=d->shop.id;r.name="装备条件";
                r.icon=*startup_world_equipment_icon_draws(s,kind,r.definition);
            }
            in.rewards.push_back(std::move(r));
        }
    }
    return in;
}
void results(Checks &c,const std::filesystem::path &source_root) {
    // Steam31 VA1032F67A..1032FDAD：真实五行，并非33的前三人简表。
    auto in=result_input(31);in.first=2;in.selection=4;
    auto plan=steam_task_result_skin(in);c(bool(plan),"31七人滚动五行");
    auto portraits=parts<SteamHumanPortrait>(*plan);
    c(portraits.size()==7&&portraits[0].position==std::array<int,2>{37,118}&&
      portraits[0].clip==std::optional<std::array<int,4>>{{29,95,15,15}}&&
      portraits[4].position==std::array<int,2>{37,194},"31列表人物的原15×15 clip与19行距");
    c(portraits[5].position==std::array<int,2>{21,220}&&portraits[6].position==std::array<int,2>{218,220}&&
      portraits[5].clip==std::optional<std::array<int,4>>{{13,197,15,15}}&&
      portraits[6].clip==std::optional<std::array<int,4>>{{210,197,15,15}}&&
      portraits[5].actor.body.sprite==1&&portraits[6].actor.body.sprite==2,"31冻结f/g人物在两边独立clip与朝向");
    auto words=parts<SteamTaskSelectionText>(*plan);
    c(words[2].rectangle==std::array<int,4>{185,75,40,0}&&words[2].layout&&words[2].anchor==32&&
      words[3].value=="队员2"&&words[7].value=="队员6", "Steam下降标题为width-only文字；名单沿first原序");
    c(words.back().font_size==10&&words.back().rectangle==std::array<int,4>{120,200,0,0},
      "31非日文完成句字号10直接居中");
    auto numbers=parts<SteamFacilityNumber>(*plan);
    c(numbers.size()==10&&numbers[0].value==102&&numbers[0].position==std::array<int,2>{162,98}&&
      numbers[1].value==12&&numbers[1].position==std::array<int,2>{201,98}&&
      numbers[0].kind==SteamFacilityNumberKind::comma_number,"击杀/倒下计数分开，count2先扣dx9");
    auto images=parts<SteamFacilityImage>(*plan);
    c(images.size()==6&&images[0].asset==SteamFacilityAsset::number05&&images[0].frame==19&&
      images[0].position==std::array<int,2>{201,98}&&
      std::count_if(images.begin(),images.end(),[](const auto &v){return v.asset==SteamFacilityAsset::finger&&
        v.position==std::array<int,2>{21,143};})==1,"倒下补原SEB12帧19，唯一手形选择第4行");
    const auto bars=parts<SteamHumanCatalogueScroll>(*plan);
    c(bars[0].touch.rectangle==std::optional<std::array<int,4>>{{221,85,3,110}}&&
      bars[0].rectangles[1].rect==std::array<int,4>{220,116,5,79},"31原110轨道及七人首行2比例，不沿22的111");
    c(parts<SteamStartupTouch>(*plan).size()==1&&parts<SteamStartupTouch>(*plan)[0].component==2&&
      parts<SteamStartupTouch>(*plan)[0].option==2,"31仅末确认组件，没有未经证明的整行Touch11");
    in.options.japanese=true;plan=steam_task_result_skin(in);c(bool(plan),"日文count3分支");
    numbers=parts<SteamFacilityNumber>(*plan);images=parts<SteamFacilityImage>(*plan);
    c(numbers[0].position==std::array<int,2>{152,98}&&images[0].asset==SteamFacilityAsset::inventory_unit&&
      images[0].frame==1&&images[0].position==std::array<int,2>{152,98},"日文击杀另扣dx10并补SEB76帧1");
    c(parts<SteamTaskSelectionText>(*plan).back().rectangle==std::array<int,4>{12,197,216,16},
      "日文底说明真实测宽夹200，不沿非日文10号点文字");
    in.crew[3]=in.crew[2];plan=steam_task_result_skin(in);
    c(plan&&parts<SteamHumanPortrait>(*plan).size()==7,"31原合法重复X保留，不去重或误拒绝");
    auto broken=in;broken.crew[0].downs=-1;c(!steam_task_result_skin(broken),"隐藏行坏统计拒绝，不留部分计划");
    broken=in;broken.options.bottom_width.reset();c(!steam_task_result_skin(broken),"日文底说明缺测宽拒绝");
    broken.options.japanese=false;c(bool(steam_task_result_skin(broken)),"非日文31无未消费测宽依赖");
    broken=in;broken.selection=7;c(!steam_task_result_skin(broken),"31越界选择拒绝");
    in=result_input(32);in.first=1;plan=steam_task_result_skin(in);c(bool(plan),"32六奖励从1起五行");
    numbers=parts<SteamFacilityNumber>(*plan);images=parts<SteamFacilityImage>(*plan);
    c(numbers.size()==6&&numbers[0].asset==SteamFacilityAsset::number03&&numbers[0].value==7&&
      numbers[0].position==std::array<int,2>{208,69}&&numbers[1].kind==SteamFacilityNumberKind::money&&
      numbers[1].asset==SteamFacilityAsset::number08&&numbers[1].value==0&&
      numbers[1].position==std::array<int,2>{210,98},"宝箱难度与售价分量纲，价格0仍画money");
    c(images.empty()&&parts<SteamStartupTouch>(*plan).size()==1,"32无selection手形或行选择Touch11");
    portraits=parts<SteamHumanPortrait>(*plan);
    c(portraits.size()==2&&portraits[0].position==std::array<int,2>{42,220}&&
      portraits[1].position==std::array<int,2>{196,220}&&
      portraits[0].clip==std::optional<std::array<int,4>>{{34,197,15,15}}&&
      portraits[1].clip==std::optional<std::array<int,4>>{{188,197,15,15}},"32独立footer位置，不沿31");
    const auto icons=parts<SteamHumanImage>(*plan);
    c(icons.size()==10&&icons[0].asset==SteamHumanAsset::equipment_background&&
      icons[0].position==std::array<int,2>{30,94}&&icons[0].crop==std::optional<std::array<int,4>>{{54,0,18,18}}&&
      icons[1].asset==SteamHumanAsset::weapon_icon&&icons[3].asset==SteamHumanAsset::armour_icon&&
      icons[5].asset==SteamHumanAsset::accessory_icon&&icons[7].asset==SteamHumanAsset::item_icon&&
      icons[6].position==std::array<int,2>{29,150},"四奖励类型用独立原图标，道具底框偏−1且绘前");
    for(const auto &v:icons) {
        const auto r=steam_human_resource(v.asset);c(bool(r)&&bool(v.crop),"奖励每个图元有正式裁片映射");
        auto png=LoadImage((source_root.parent_path()/r->published_image).string().c_str());
        c(png.data!=nullptr,"奖励原图可解码");const auto box=*v.crop;
        const bool inside=box[0]>=0&&box[1]>=0&&box[0]+box[2]<=png.width&&box[1]+box[3]<=png.height;
        UnloadImage(png);c(inside,"四类正式裁片不越PNG，不能由绘制截断掩盖坏图标");
    }
    for(const auto &sample:std::array<std::pair<SteamFacilityAsset,int>,2>{{{SteamFacilityAsset::number05,19},
        {SteamFacilityAsset::inventory_unit,1}}}) {
        const auto r=steam_facility_resource(sample.first);c(r&&r->published_sprite,"31两类单位有已出版SEB");
        const auto seb=dungeon_village_tools::parse_legacy_seb(bytes(source_root.parent_path()/r->published_sprite));
        auto png=LoadImage((source_root.parent_path()/r->published_image).string().c_str());c(png.data!=nullptr,"统计单位原PNG");
        bool found{},inside=true;
        for(const auto &layer:seb.layers)for(const auto &v:layer.parts)if(v.frame==sample.second) {
            found=true;inside&=v.source_x>=0&&v.source_y>=0&&v.width>0&&v.height>0&&
                v.source_x+v.width<=png.width&&v.source_y+v.height<=png.height;
        }
        UnloadImage(png);c(found&&inside,"单位帧有实际源记录且在PNG内，不只核帧数");
    }
    broken=in;broken.rewards[0].icon[1].crop[0]=999;c(!steam_task_result_skin(broken),"32隐藏奖励坏裁片也拒绝");
    broken=in;broken.first=2;c(!steam_task_result_skin(broken),"32首行不得越过count−5，区别31短尾选择");
    broken=in;broken.options.bottom_width.reset();c(!steam_task_result_skin(broken),"32底说明缺实际测宽拒绝");
    for(int raw:{31,32}) {
        auto stable=result_input(raw);const auto first=steam_task_result_skin(stable);c(bool(first),"结果只读规模样本");
        for(int repeat=0;repeat<3;++repeat) {
            const auto again=steam_task_result_skin(stable);
            c(again&&again->operations.size()==first->operations.size(),"结果反复查询不积累跨帧图元");
        }
        stable.crew.clear();stable.rewards.clear();
        const auto empty=steam_task_result_skin(stable);c(empty&&parts<SteamHumanPortrait>(*empty).size()==2,
            "空结果保留f/g与确认，不能因无行而猜人物或剥掉返回");
    }
}
void victory(Checks &c) {
    // Steam30局部调用点：经验与人气是显示条件，不触发任何任务或奖励。
    SteamTaskVictoryInput in;in.task_name="任务条件";in.experience=123;in.reputation=20;
    in.options.title_widths=std::array<int,2>{24,24};in.options.reputation_suffix_width=48;
    in.humans={{0,0,0,0},{1,0,1,0},{0,0,0,0}};
    auto plan=steam_task_victory_skin(in);c(bool(plan),"30当前演员计划与合法重复参与者");
    bool border=false;for(const auto &op:plan->operations)if(const auto *f=std::get_if<SteamStartupFrameDraw>(&op))
        if(const auto *r=std::get_if<StartupSkinRect>(f))border|=r->outline&&r->rect==std::array<int,4>{30,66,180,82}&&r->rgb==std::array<int,3>{148,148,148};
    c(border,"30原DrawRect179/81含端点，半开计划180/82");
    const auto confirm=parts<SteamStartupTouch>(*plan);
    c(plan->title=="胜利！"&&confirm.size()==1&&confirm[0].component==2&&confirm[0].value==0&&
      !confirm[0].rectangle&&confirm[0].option==2,"30标题胜利且无条件注册无显式矩形Touch2/option2");
    auto actors=parts<SteamTaskVictoryActor>(*plan);
    c(actors.size()==3&&actors[0].position==std::array<int,2>{80,134}&&
      actors[1].position==std::array<int,2>{120,134}&&actors[2].position==std::array<int,2>{160,134},
      "30三人80/120/160，重复引用不去重");
    const auto numbers=parts<SteamFacilityNumber>(*plan);
    c(numbers.size()==1&&numbers[0].value==123&&numbers[0].position==std::array<int,2>{119,73}&&
      numbers[0].asset==SteamFacilityAsset::number05&&numbers[0].anchor==1,
      "30三位经验宽46、EXP左97、原数点119，不按Font测数字");
    for(const auto sample:std::array<std::array<int,3>,7>{{{9,0,134},{10,6,134},{11,6,132},{12,6,131},{17,6,126},{25,6,134},{26,0,134}}}) {
        in.secondary=sample[0];plan=steam_task_victory_skin(in);c(bool(plan),"30副钟动画边界");
        actors=parts<SteamTaskVictoryActor>(*plan);
        c(actors[0].action==sample[1]&&actors[0].position[1]==sample[2],"30 float32抛物线与26周期独立oracle");
        const auto skin=steam_task_victory_actor_skin(actors[0]);
        c(skin&&skin->body.sprite==(sample[1]==6?10:2)&&skin->body.frame==0,
          "30喜悦用action6方向2 SEB10帧0，不沿举手22或walk动画");
        const auto it=std::find_if(plan->operations.begin(),plan->operations.end(),[](const auto &v){return std::holds_alternative<SteamTaskVictoryActor>(v);});
        c(it!=plan->operations.begin()&&std::holds_alternative<SteamTaskRosterImage>(*(it-1))&&
          std::get<SteamTaskRosterImage>(*(it-1)).position[1]==134,"人物跳起阴影不随跳，且在身体武器前消费");
    }
    for(const auto sample:std::array<std::array<int,3>,5>{{{4,0,0},{5,0,0},{39,46,0},{40,48,0},{50,48,1}}}) {
        in.counter=sample[0];plan=steam_task_victory_skin(in);
        const auto clip=parts<SteamTaskSelectionClip>(*plan);
        c(clip.size()==2&&clip[0].push&&!clip[1].push&&clip[0].rectangle==std::array<int,4>{0,158,240,sample[1]},
          "30只裁底文，5..40线性展开后配对释放");
        const auto images=parts<SteamHumanImage>(*plan);
        const auto arrow=std::count_if(images.begin(),images.end(),[](const auto &v){return v.asset==SteamHumanAsset::continue_cursor;});
        c(arrow==sample[2],"30箭头同时受40门槛和25模15限制");
    }
    in.phase=1;plan=steam_task_victory_skin(in);const auto result=parts<SteamFacilityNumber>(*plan);
    c(result.size()==2&&result[1].value==20&&result[1].position==std::array<int,2>{85,194}&&
      result[1].asset==SteamFacilityAsset::number09,"30人气数两位20宽＋2＋48，锚85不是实际封顶差");
    auto invalid=in;invalid.options.reputation_suffix_width.reset();c(!steam_task_victory_skin(invalid),"30第二段缺后缀测宽拒绝");
    invalid.phase=0;c(bool(steam_task_victory_skin(invalid)),"第一段不要求未读取测宽");
    invalid=in;invalid.humans.back().profession=999;c(!steam_task_victory_skin(invalid),"30末演员坏源拒绝全部计划");
    invalid=in;invalid.secondary=-1;c(!steam_task_victory_skin(invalid),"30负副钟拒绝");
    invalid=in;invalid.phase=2;c(!steam_task_victory_skin(invalid),"30坏阶段拒绝");
    invalid=in;invalid.humans.resize(10,in.humans.front());c(!steam_task_victory_skin(invalid),"30拒绝超出九人的维护参加契约");
    const auto count=plan->operations.size();for(int i=0;i<8;++i)c(steam_task_victory_skin(in)->operations.size()==count,"30重复只读查询不积累图元");
    in.humans.clear();c(parts<SteamTaskVictoryActor>(*steam_task_victory_skin(in)).empty(),"原空实时m不造演员");
}
void victory_owner(Checks &c) {
    auto s=test_support::world_fixture();
    const auto made=ref::prepare_world_task_creation(startup_world_runtime_factory(s),0);
    c(made.candidate&&made.candidate->created_task&&write_startup_world_runtime_factory(s,made.candidate->state),"30源任务绑定准备");
    s.active_task=*made.candidate->created_task;
    c(consume_startup_world_runtime_task_encounter_request(s,{ref::EncounterRequestKind::page30,{},30,70}),"30实际战斗请求producer");
    s.active_task.reset();const auto id=s.scripts.pages.back().id;
    const auto tick=prepare_startup_world_runtime(s);c(bool(tick.candidate),"30进入真实框架");s=*tick.candidate;
    SteamTaskVictoryOptions options;options.title_widths=std::array<int,2>{24,24};options.reputation_suffix_width=48;
    s.participants={0,1,0}; // 明确实时m条件，绘制桥不要求重新报名。
    const auto before=startup_world_state_digest(s);const auto draws=s.scene.random.draws();
    auto p=steam_task_victory_skin(s,id,options);c(p&&parts<SteamTaskVictoryActor>(*p).size()==3,"30实际producer只读桥读取m原序");
    for(int i=0;i<8;++i)c(steam_task_victory_skin(s,id,options)->operations.size()==p->operations.size()&&
        startup_world_state_digest(s)==before&&s.scene.random.draws()==draws,"30桥不颁奖、发声音、抽随机或写页面");
    auto bad=s;bad.page_secondary_counters.erase(id);c(!steam_task_victory_skin(bad,id,options),"30缺副钟不默认置零");
    bad=s;bad.participants.back()=999;c(!steam_task_victory_skin(bad,id,options),"30实时m悬空拒绝");
    bad=s;bad.scripts.pages.back().lifecycle=1;c(!steam_task_victory_skin(bad,id,options),"30未就绪不绘制");
    bad=s;bad.scripts.pages.back().task_identity.reset();bad.scripts.pages.back().task_definition.reset();
    c(bool(steam_task_victory_skin(bad,id,options)),"30探索summary绑定合法，不强加战斗optional");
    bad.exploration_summaries.at(id).definition=999;c(!steam_task_victory_skin(bad,id,options),"30错summary显式拒绝");
    for(const auto flags:{2U,4U}) {
        const auto d=std::find_if(s.rules->tasks.begin(),s.rules->tasks.end(),[=](const auto &v){return (v.factory.flags&flags)&&(!(flags==4U)||!(v.factory.flags&2U));});
        c(d!=s.rules->tasks.end(),"原表提供两种成果名称分支");
        auto named=s;const auto task=named.exploration_summaries.at(id).task;
        named.exploration_summaries.at(id).definition=d->factory.identity;
        named.tasks.at(task).definition=d->factory.identity;named.scripts.pages.back().task_definition=d->factory.identity;
        if(flags==2U)named.scene.world.world.ai.monster_growth.at(d->factory.monster).growth=3;
        else named.task_progress.definitions.at(d->factory.identity).completed=3;
        const auto named_plan=steam_task_victory_skin(named,id,options);c(bool(named_plan),"30原已结算命名条件");
        const auto texts=parts<SteamTaskSelectionText>(*named_plan);
        c(std::any_of(texts.begin(),texts.end(),[&](const auto &t){return t.value==d->name+(flags==2U?"3":"+4");}),
          "GetName(true)使用已增进度：boss3或三次完成+4，不借目录的下一次后缀");
    }
}
SteamTaskDeadlineInput deadline_input() {
    SteamTaskDeadlineInput in;in.task_name="期限条件";in.progress=35;in.fee=100;in.affordable=true;
    in.participants={0,1,2,0};in.humans={{0,0,0,2},{1,0,0,1},{2,0,0,3}};
    in.options.title_widths=std::array<int,2>{72,72};in.options.down_number_width=6;
    in.options.prediction_width=72;in.options.button_widths={24,24};in.options.bubble_width=36;
    return in;
}
void deadline(Checks &c,const std::filesystem::path &source_root) {
    auto in=deadline_input();in.counter=4;in.active_humans={{0,7,30,100},{0,0,100,100},{1,0,31,100}};
    auto plan=steam_task_deadline_skin(in);c(bool(plan),"33期限只读输入");
    const auto people=parts<SteamHumanPortrait>(*plan);const auto images=parts<SteamHumanImage>(*plan);
    c(people.size()==3&&people[0].position==std::array<int,2>{49,145}&&people[2].position==std::array<int,2>{89,145}&&
      people[0].actor.body.sprite==13&&!people[0].actor.weapon,
      "33最多前三、首同定义实例动作7倒下无武器，不误读状态枚举");
    const auto male=startup_title_actor_skin(0,0,0,1,1,false);
    c(male&&people[1].actor.body.image==male->body.image,"33无sex参数固定职业男图");
    c(std::count_if(images.begin(),images.end(),[](const auto &v){return v.asset==SteamHumanAsset::pinch;})==1,
      "HPafter30%显示、31%不显示、无实例不造HP");
    const auto numbers=parts<SteamFacilityNumber>(*plan);
    c(std::any_of(numbers.begin(),numbers.end(),[](const auto &v){return v.kind==SteamFacilityNumberKind::comma_number&&v.value==8;}),
      "33倒下总数包含第四位重复m，不只可见三人");
    const auto touch=parts<SteamStartupTouch>(*plan);
    c(touch.size()==2&&touch[0].rectangle==std::optional<std::array<int,4>>{{31,175,98,56}}&&
      touch[1].rectangle==std::optional<std::array<int,4>>{{121,175,98,56}}&&touch[0].component==3&&touch[1].value==0x20001,
      "33独立DLGSEL扩张矩形，不沿28底按钮位置");
    in.active_humans={{0,0,150,100},{1,0,-1,100}};plan=steam_task_deadline_skin(in);
    c(bool(plan),"HP高于新max及负HP合法，不把显示钳回");
    in.phase=1;in.answer=0;in.counter=12;plan=steam_task_deadline_skin(in);
    const auto raised=parts<SteamHumanPortrait>(*plan);
    c(raised[0].actor.body.sprite==21&&raised[0].actor.body.frame==0&&parts<SteamStartupTouch>(*plan).size()==2,
      "33继续12帧举手方向1，与phase1仍注册按钮");
    in.answer=1;in.selection=1;plan=steam_task_deadline_skin(in);
    c(parts<SteamHumanPortrait>(*plan)[0].actor.body.frame==0,"33中止固定walk0");
    auto bad=in;bad.answer.reset();c(!steam_task_deadline_skin(bad),"33第二段缺答案拒绝");
    bad=in;bad.participants.back()=999;c(!steam_task_deadline_skin(bad),"33不可见第四人坏ID也拒绝");
    bad=in;bad.active_humans[0].hp_maximum=0;c(!steam_task_deadline_skin(bad),"33除零HP源拒绝");
    bad=in;bad.active_humans[0].hp_after=std::numeric_limits<int>::max();c(!steam_task_deadline_skin(bad),"33源HP乘100溢出拒绝");
    in=deadline_input();in.kind=1;in.progress.reset();in.remaining=3;in.main_monster=10;
    in.encounter_monsters={11,12};in.monsters={{10,2,0,2},{11,1,0,1},{12,3,0,3}};in.counter=12;
    plan=steam_task_deadline_skin(in);c(bool(plan),"33三个不同体型的条件编组");
    const auto monsters=parts<SteamTaskDeadlineMonster>(*plan);
    c(monsters.size()==3&&monsters[0].image==90&&monsters[0].position==std::array<int,2>{186,145}&&
      monsters[1].image==30&&monsters[1].position==std::array<int,2>{144,145}&&monsters[2].image==60&&monsters[2].position==std::array<int,2>{165,145},
      "33怪物逆2/1/0绘制、间距取主怪body2");
    c(monsters[0].requested_step==3&&monsters[0].frame==1&&monsters[1].frame==2&&monsters[2].frame==1,
      "传入末人step3与最终主钟/10、6、8生成帧1/2/1独立");
    in.participants.clear();plan=steam_task_deadline_skin(in);c(parts<SteamTaskDeadlineMonster>(*plan)[0].requested_step==0,
      "33空人物传入step0而仍使用主钟计算怪物动画");
    bad=in;bad.monsters[2].variant=6;c(!steam_task_deadline_skin(bad),"33大体型只有六图，拒绝虚构槽");
    in=deadline_input();const auto &facilities=startup_world_rules().facilities;
    const auto map=std::find_if(facilities.begin(),facilities.end(),[](const auto &f){return f.shape==2;});
    c(map!=facilities.end(),"固定源存在四片Mapchip");in.mapchip=map->display_id;
    plan=steam_task_deadline_skin(in);c(bool(plan),"33普通地图切片");
    const auto tiles=parts<SteamFacilityMapchipDraw>(*plan);
    c(tiles.size()==4&&tiles[0].position==std::array<int,2>{130,91}&&tiles[0].frame==0&&
      tiles[1].position==std::array<int,2>{100,106}&&tiles[1].frame==2&&tiles[2].position==std::array<int,2>{160,106}&&tiles[2].frame==4&&
      tiles[3].position==std::array<int,2>{130,121}&&tiles[3].frame==6,
      "33普通DrawMapchip四片不引入Mapchip2居中");
    const auto clips=parts<SteamTaskSelectionClip>(*plan);
    c(clips.size()==2&&clips[0].rectangle==std::array<int,4>{17,85,202,110}&&!clips[1].push,"33只裁普通设施并释放");
    const auto count=plan->operations.size();for(int i=0;i<8;++i)c(steam_task_deadline_skin(in)->operations.size()==count,"33重复只读规模稳定");
    const auto resource=steam_task_deadline_resource(SteamTaskDeadlineAsset::progress_label);c(bool(resource),"33进度/剩余资源映射");
    const auto root=source_root.parent_path();auto image=LoadImage((root/resource->published_image).string().c_str());
    c(image.data!=nullptr,"Steam menuRT00已出版图片可解码");UnloadImage(image);
    const auto seb=dungeon_village_tools::parse_legacy_seb(bytes(root/resource->published_sprite));
    c(resource->image==84&&resource->sprite==75&&seb.frame_count==4,"menuRT00 SEB75实际四帧、此页仅用0/1，不使用字体猜图标");
}
void result_owner(Checks &c) {
    SteamTaskResultOptions options;options.title_widths=std::array<int,2>{24,24};options.bottom_width=72;
    for(int raw:{31,32}) {
        auto s=test_support::page_fixture(raw);const auto id=s.scripts.pages.back().id;
        const auto made=ref::prepare_world_task_creation(startup_world_runtime_factory(s),0);
        c(made.candidate&&made.candidate->created_task&&write_startup_world_runtime_factory(s,made.candidate->state),
          "31/32桥绑定现真实任务工厂，摘要条件不认证自然结算");
        const auto task=*made.candidate->created_task;auto &page=s.scripts.pages.back();
        page.task_identity=task;page.task_definition=s.tasks.at(task).definition;
        page.legacy_f=0;page.legacy_g=1;page.lifecycle=2;
        s.task_page_ui.emplace(id,StartupTaskPageUi{});s.page_counters[id]=0;s.page_phases[id]=0;
        if(raw==31) {
            s.crew_summaries[id]={0,1,0};s.task_page_ui.at(id).selection=2;
        } else {
            const auto &item=s.rules->items.front();
            s.exploration_summaries.emplace(id,ref::WorldExplorationSummary{32,task,*page.task_definition,
                {{0,item.identity},{0,item.identity}}});
        }
        const auto before=startup_world_state_digest(s);const auto draws=s.scene.random.draws();
        const auto dto=steam_task_result_input(s,id);const auto plan=steam_task_result_skin(s,id,options);
        c(dto&&plan,"31/32严格Owner桥消费冻结源和真实字体选项");
        c(dto->footer[0].identity==0&&dto->footer[1].identity==1,"成果底部f/g独立于重复名单");
        if(raw==31)c(dto->crew.size()==3&&dto->crew[0].human.identity==dto->crew[2].human.identity,
          "31原重复X保序，不去重或重新结算H");
        else c(dto->rewards.size()==2&&dto->rewards[0].value==s.rules->items.front().commerce_price&&
          dto->rewards[0].definition==dto->rewards[1].definition,"32奖励原序价格/图标，重复领奖行不再次发放");
        for(int repeat=0;repeat<3;++repeat)c(steam_task_result_skin(s,id,options)->operations.size()==plan->operations.size()&&
            startup_world_state_digest(s)==before&&s.scene.random.draws()==draws,"成果重复投影不写随机/奖励/游标且规模稳定");
        auto bad=s;bad.task_page_ui.erase(id);c(!steam_task_result_skin(bad,id,options),"成果缺UI不得补默认游标");
        bad=s;bad.task_page_ui.at(id).parent=s.scripts.pages.front().id;
        c(!steam_task_result_input(bad,id),"成果不消费孤立父页载荷");
        bad=s;bad.scripts.pages.back().legacy_g=999;c(!steam_task_result_input(bad,id),"成果缺冻结底部人物拒绝");
        bad=s;bad.scripts.pages.back().lifecycle=1;c(!steam_task_result_skin(bad,id,options),"成果未获准页拒绝");
        bad=s;bad.task_page_ui.at(id).first=4;c(!steam_task_result_input(bad,id),"成果越界首行显式拒绝");
        bad=s;if(raw==31)bad.crew_summaries.at(id).back()=999;
        else bad.exploration_summaries.at(id).rewards.back()[1]=999;
        c(!steam_task_result_input(bad,id),"成果不可见冻结行坏引用也拒绝");
    }
}
void roster_owner(Checks &c) {
    SteamTaskRosterOptions options;options.title_widths=std::array<int,2>{72,72};
    options.prediction_width=72;options.button_widths={48,48};
    // 独立短条件桥：真实目录／factory任务，名单是已初始化UI条件，不冒称自然报名。
    for(int raw:{25,26,28}) {
        auto s=test_support::page_fixture(raw);const auto id=s.scripts.pages.back().id;
        const auto made=ref::prepare_world_task_creation(startup_world_runtime_factory(s),0);
        c(made.candidate&&made.candidate->created_task&&write_startup_world_runtime_factory(s,made.candidate->state),
          "25/26/28桥准备同一真实factory身份");
        const auto task=*made.candidate->created_task;auto &page=s.scripts.pages.back();
        page.task_identity=task;page.task_definition=s.tasks.at(task).definition;page.lifecycle=2;
        s.task_page_ui.emplace(id,StartupTaskPageUi{});s.page_counters[id]=12;
        if(raw<=26) {
            s.task_page_ui.at(id).member_rows={1,0,-1};s.participants={2};
            if(raw==26)s.active_task=task;
        } else {
            s.participants={1,0};s.page_phases[id]=1;s.page_secondary_counters[id]=7;
            s.task_page_predictions[id]=3;s.task_page_acceleration[id]=false;
        }
        const auto before=startup_world_state_digest(s);const auto draws=s.scene.random.draws();
        const auto plan=steam_task_roster_skin(s,id,options);c(bool(plan),"任务后继实际Owner桥");
        if(raw<=26) {
            const auto words=parts<SteamTaskSelectionText>(*plan);
            const auto one=startup_world_human_details(s,1),zero=startup_world_human_details(s,0);
            c(words.front().value=="1名参加"&&std::any_of(words.begin(),words.end(),[&](const auto &v){return v.value==one->name;})&&
              std::any_of(words.begin(),words.end(),[&](const auto &v){return v.value==zero->name;}),
              "25/26上方实时m人数1，冻结X两人保序不刷新成新m");
            const auto portraits=parts<SteamHumanPortrait>(*plan);
            const auto expected=startup_title_actor_skin(one->profession,one->sex,-1,0,1,false);
            c(portraits.size()==2&&expected&&portraits[0].actor.body.image==expected->body.image,
              "25/26首次肖像沿冻结原序，仅职业/性别实时");
        } else {
            const auto actors=parts<SteamTaskDepartureActor>(*plan);const auto h=startup_world_human_details(s,1);
            c(actors.size()==2&&actors.front().action==4&&actors.front().step==0&&actors.front().weapon==h->equipment[0].value_or(-1),
              "28桥读phase1主钟12举手及当前主武器，不用窗口副本");
            auto walking=s;walking.page_counters[id]=40;
            const auto walk=steam_task_roster_skin(walking,id,options);const auto walking_actors=walk?parts<SteamTaskDepartureActor>(*walk):std::vector<SteamTaskDepartureActor>{};
            c(walking_actors.size()==2&&walking_actors.front().action==0&&walking_actors.front().step==1,
              "28主钟40走路但副钟7给step1，两个钟不互代");
            auto bad=s;bad.page_secondary_counters.erase(id);c(!steam_task_roster_skin(bad,id,options),"28缺副钟显式拒绝");
            bad=s;bad.task_page_predictions.erase(id);c(!steam_task_roster_skin(bad,id,options),"28缺冻结评价不得临时重算");
        }
        for(int repeat=0;repeat<3;++repeat)c(steam_task_roster_skin(s,id,options)->operations.size()==plan->operations.size()&&
          startup_world_state_digest(s)==before&&s.scene.random.draws()==draws,"后继只读桥不收费、出发、推进、抽随机或积累图元");
        auto bad=s;bad.task_page_ui.erase(id);c(!steam_task_roster_skin(bad,id,options),"后继缺Owner UI不补临时游标");
        bad=s;bad.scripts.pages.back().lifecycle=1;c(!steam_task_roster_skin(bad,id,options),"后继未获准页不绘制");
        bad=s;bad.scripts.pages.back().task_definition.reset();c(!steam_task_roster_skin(bad,id,options),"后继坏任务绑定拒绝");
        if(raw==25) {
            // 27必须覆盖同任务父目录；独立候选为0，父X仍保留1/0/追加行。
            s.scripts.pages.back().lifecycle=3;ref::WorldScriptPage child;
            child.id=s.scripts.next_page_id++;child.kind=ref::WorldScriptPageKind::raw_page;
            child.legacy_page=27;child.lifecycle=2;child.task_identity=task;child.task_definition=*page.task_definition;
            const auto child_id=child.id;s.scripts.pages.push_back(child);
            StartupTaskPageUi ui;ui.parent=id;ui.tab=1;s.task_page_ui.emplace(child_id,std::move(ui));
            s.page_counters[child_id]=8;s.task_extra_pages[child_id]={0};
            const auto covered=startup_world_state_digest(s);const auto preview=steam_task_roster_skin(s,child_id,options);
            c(preview&&parts<SteamHumanPortrait>(*preview).size()==1&&s.task_page_ui.at(id).member_rows==std::vector<int>({1,0,-1}),
              "27严格覆盖父目录：候选tab1实时四战斗值，不改父冻结X");
            c(!steam_task_roster_skin(s,id,options)&&startup_world_state_digest(s)==covered,"被覆盖25不误画且查询只读");
            bad=s;bad.task_page_ui.at(child_id).parent=s.scripts.pages.front().id;
            c(!steam_task_roster_skin(bad,child_id,options),"27错父显式拒绝");
            bad=s;bad.task_extra_pages.erase(child_id);c(!steam_task_roster_skin(bad,child_id,options),"27候选缺失不复用实时m");
            bad=s;bad.task_page_ui.at(child_id).selection=1;c(!steam_task_roster_skin(bad,child_id,options),"27候选选择越界拒绝");
        }
    }
}
void deadline_owner(Checks &c) {
    auto s=test_support::page_fixture(33);const auto id=s.scripts.pages.back().id;
    // 无aa、但有真实q的原展示分支：不伪称玩家已在探索期限自然到达。
    const auto definition=std::find_if(s.rules->tasks.begin(),s.rules->tasks.end(),
        [](const auto &d){return d.factory.kind==0;});
    c(definition!=s.rules->tasks.end(),"33原表提供探索定义");
    auto &page=s.scripts.pages.back();page.lifecycle=2;page.task_definition=definition->factory.identity;page.legacy_f=100;
    s.task_page_ui.emplace(id,StartupTaskPageUi{});s.page_counters[id]=4;s.page_phases[id]=0;
    s.deadline_initialized.insert(id);s.deadline_grades[id]=3;s.deadline_returns[id]=-1;s.deadline_page=id;
    s.participants={0,1,0};
    const auto options=deadline_input().options;const auto before=startup_world_state_digest(s);
    const auto dto=steam_task_deadline_input(s,id);const auto plan=steam_task_deadline_skin(s,id,options);
    c(dto&&plan&&dto->progress==70&&dto->participants==s.participants,
      "33桥尊重原aa为空70分支，真实q/m与冻结grade独立");
    c(dto->humans.size()==2&&dto->active_humans.empty(),"定义可重复m，无bl不制造假实例/HP");
    for(int repeat=0;repeat<3;++repeat)c(steam_task_deadline_skin(s,id,options)->operations.size()==plan->operations.size()&&
        startup_world_state_digest(s)==before,"期限重复只读投影不发答案或声音，规模稳定");
    auto bad=s;bad.deadline_grades.erase(id);c(!steam_task_deadline_input(bad,id),"期限缺grade不补默认");
    bad=s;bad.task_page_ui.erase(id);c(!steam_task_deadline_input(bad,id),"期限缺UI不借窗口选择");
    bad=s;bad.task_page_ui.at(id).selection=2;c(!steam_task_deadline_input(bad,id),"期限选择越界显式拒绝");
    bad=s;bad.scripts.pages.back().task_identity=999;c(!steam_task_deadline_input(bad,id),"期限悬空aa不走合法空分支");
    bad=s;bad.scripts.pages.back().task_definition.reset();c(!steam_task_deadline_input(bad,id),"期限没有q不能猜定义0绘页");
    s.page_phases[id]=1;s.page_counters[id]=12;s.deadline_returns[id]=0;
    const auto answered=steam_task_deadline_skin(s,id,options);
    c(answered&&parts<SteamHumanPortrait>(*answered).front().actor.body.sprite==21,"期限第二段读唯一K答案并演出举手");
    bad=s;bad.deadline_returns.erase(id);c(!steam_task_deadline_input(bad,id),"期限已确认却缺K答案拒绝");
    // 原monster表第5列是独立shadow：软软0，蘑菇子1，不按body猜。
    const auto zero=std::find_if(s.rules->monsters.begin(),s.rules->monsters.end(),[](const auto &d){return d.identity==0;});
    const auto mushroom=std::find_if(s.rules->monsters.begin(),s.rules->monsters.end(),[](const auto &d){return d.identity==4;});
    c(zero!=s.rules->monsters.end()&&mushroom!=s.rules->monsters.end()&&zero->shadow==0&&mushroom->shadow==1,
      "33怪物固定列5独立oracle，不默填0或body");
}
void task_information(Checks &c) {
    // 两独立原helper：UserData RVA2F82B0(scene4)、SubForm RVA30BED0(raw29)。
    SteamTaskInfoInput in;in.name="任务详情条件";in.treasure_level=9;in.monster_rank=4;
    in.fee=432;in.available_tasks=2;in.frame=8;in.options.bottom_width=240;
    in.options.view_y=40;in.options.view_layout=std::array<int,4>{20,40,300,280};
    for(const auto source:{SteamTaskInfoSource::raw29,SteamTaskInfoSource::tracking_scene}) {
        const bool scene=source==SteamTaskInfoSource::tracking_scene;in.source=source;
        const auto plan=steam_task_information_skin(in);c(bool(plan),"29与scene4独立详情helper");
        c(plan->title.empty()&&plan->soft_labels==std::array<int,2>{scene?6:0,2},
          "任务地图上下蓝框而非普通标题窗，左右脚标按入口");
        const auto frames=parts<SteamStartupFrameDraw>(*plan);
        const auto *box=std::get_if<StartupSkinRect>(&frames.front());
        c(box&&box->rect==(scene?std::array<int,4>{57,100,202,20}:std::array<int,4>{17,60,202,20})&&
          box->rgb==std::array<int,3>{247,253,247},"scene中心原点与DrawBox2额外VIEW_Y/2各消费一次");
        const auto words=parts<SteamTaskSelectionText>(*plan);
        c(words.size()==3&&words[0].rectangle==(scene?std::array<int,4>{160,96,0,0}:std::array<int,4>{120,56,0,0})&&
          words[0].font_size==(scene?11:0)&&words[0].anchor==2,"scene标题11号，29保留当前字号及固定原点");
        c(words[1].rectangle==(scene?std::array<int,4>{196,212,0,0}:std::array<int,4>{136,172,0,0})&&
          words[1].anchor==(scene?4:0)&&words[1].rgb==std::array<int,3>{0,100,255},
          "费用scene右锚156、29点锚136，不能共享一种对齐");
        const auto numbers=parts<SteamFacilityNumber>(*plan);
        c(numbers.size()==2&&numbers[0].value==9&&numbers[1].value==432&&
          numbers[1].kind==SteamFacilityNumberKind::money&&numbers[1].asset==
            (scene?SteamFacilityAsset::number12:SteamFacilityAsset::number08),"藏宝等级9与费用432，缺钱仅scene用SEB19");
        const auto arrows=parts<SteamFacilityArrowTouch>(*plan);
        c(arrows.size()==std::size_t(scene?2:0),"仅scene4多任务注册真实箭头helper，29不补箭头");
        if(scene)c(arrows[0].position==std::array<int,2>{83,101}&&arrows[0].frame==3&&arrows[0].value==16&&
          arrows[1].position==std::array<int,2>{237,101}&&arrows[1].frame==0&&arrows[1].value==18,
          "frame8给左右三像素位移，Touch1值16/18原序");
        const auto touch=parts<SteamStartupTouch>(*plan);
        c(touch.size()==1&&touch[0].component==4&&touch[0].value==20&&touch[0].rectangle==
          std::optional<std::array<int,4>>(scene?std::array<int,4>{12,210,296,56}:std::array<int,4>{-8,170,256,56}),
          "scene按钮测宽240不夹，29夹200；Touch4包含原BTMT margin");
        c(scene?(words[2].value=="攻略"&&words[2].rectangle==std::array<int,4>{160,232,0,0}&&!words[2].layout):
          (words[2].value=="了解"&&words[2].rectangle==std::array<int,4>{12,190,216,16}&&words[2].layout),
          "scene中心点字与29TextLayout各按真实重载");
    }
    in.source=SteamTaskInfoSource::tracking_scene;in.kind=1;in.frame=7;in.affordable=true;
    auto plan=steam_task_information_skin(in);c(bool(plan),"讨伐任务详情");
    c(parts<SteamTaskSelectionImage>(*plan).empty()&&parts<SteamFacilityNumber>(*plan)[0].value==5&&
      parts<SteamFacilityNumber>(*plan)[1].asset==SteamFacilityAsset::number08&&
      parts<SteamFacilityArrowTouch>(*plan)[0].position==std::array<int,2>{86,101},
      "kind1用monster_rank+1，不借treasure_level；frame7尚未平移");
    in.available_tasks=1;plan=steam_task_information_skin(in);
    c(plan&&parts<SteamFacilityArrowTouch>(*plan).empty(),"单实时任务不补左右箭头");
    auto bad=in;bad.options.view_layout.reset();c(!steam_task_information_skin(bad),"scene缺真实VIEW布局拒绝");
    bad.source=SteamTaskInfoSource::raw29;c(bool(steam_task_information_skin(bad)),"29不读取scene专有VIEW布局");
    bad=in;bad.monster_rank=std::numeric_limits<int>::max();c(!steam_task_information_skin(bad),"rank+1溢出拒绝");
    bad=in;bad.options.bottom_width.reset();c(!steam_task_information_skin(bad),"原按钮缺实际测宽拒绝");
}
void task_information_owner(Checks &c) {
    auto s=test_support::world_fixture();
    const auto made=ref::prepare_world_task_creation(startup_world_runtime_factory(s),0);
    c(made.candidate&&made.candidate->created_task&&write_startup_world_runtime_factory(s,made.candidate->state),
      "任务详情桥使用真实factory身份与地图目标");
    const auto task=*made.candidate->created_task;s.task_order={task,task};
    c(open_startup_world_runtime_task_menu(s)==StartupWorldRuntimeError::none,"真实22入口准备跟踪");
    const auto menu=s.scripts.pages.back().id;s.scripts.pages.back().lifecycle=2;
    StartupTaskPageInput command;command.track=true;
    c(input_startup_world_runtime_task_page(s,menu,command).error==StartupWorldRuntimeError::none&&s.scene.scene_state==5,
      "22真实跟踪进入scene5，尚未显示详情");
    SteamTaskInfoOptions options;options.bottom_width=36;options.view_layout=std::array<int,4>{0,0,240,240};
    c(!steam_task_information_scene_skin(s,options),"scene5相机过渡不画scene4详情");
    const auto site=*s.tasks.at(task).site;
    s.camera={float((site.x+site.y)*30+30),float((site.y-site.x)*15+10)};
    const auto arrived=update_startup_world_runtime_task_tracking_scene(s);
    c(arrived&&arrived->scene.scene_state==4,"短条件相机到位由真实消费者进入4");s=*arrived;
    const auto scene=s.scripts.pages.back().id;s.scene.frame_counter=8;
    const auto digest=startup_world_state_digest(s);
    const auto sounds=s.sound_requests;
    const auto draws=s.scene.random.draws();
    const auto plan=steam_task_information_scene_skin(s,options);c(bool(plan),"已初始化scene4只读桥");
    const auto arrows=parts<SteamFacilityArrowTouch>(*plan);
    c(arrows.size()==2&&arrows[0].position==std::array<int,2>{43,61}&&arrows[1].position==std::array<int,2>{197,61},
      "桥消费真实GameForm frame_counter8而非页钟或日期");
    for(int repeat=0;repeat<3;++repeat) {
        const auto again=steam_task_information_scene_skin(s,options);
        c(again&&again->operations.size()==plan->operations.size()&&startup_world_state_digest(s)==digest&&
          s.scene.random.draws()==draws&&s.sound_requests==sounds,"场景重复投影不计费、推进、抽随机或积累图元");
    }
    auto bad=s;bad.task_page_lists.erase(scene);c(!steam_task_information_scene_skin(bad,options),"缺跟踪名单显式拒绝");
    bad=s;bad.page_phases[scene]=2;c(!steam_task_information_scene_skin(bad,options),"场景越界选择拒绝");
    bad=s;bad.scripts.pages.back().lifecycle=1;c(!steam_task_information_scene_skin(bad,options),"scene未获准轮不绘制");
    bad=s;bad.task_order={task};
    const auto single=steam_task_information_scene_skin(bad,options);
    c(single&&parts<SteamFacilityArrowTouch>(*single).empty(),"箭头看实时questList，不用跟踪冻结两条名单代替");
    // raw29尚无新动态入口，以下明确为已初始化绑定页的条件桥，不冒认自然打开。
    auto raw=test_support::page_fixture(29);const auto id=raw.scripts.pages.back().id;
    raw.tasks.emplace(task,s.tasks.at(task));raw.scripts.pages.back().task_identity=task;
    raw.scripts.pages.back().task_definition=s.tasks.at(task).definition;
    raw.scripts.pages.back().lifecycle=2;raw.page_counters[id]=0;
    const auto before=startup_world_state_digest(raw);
    const auto page=steam_task_information_skin(raw,id,options);
    c(page&&parts<SteamFacilityArrowTouch>(*page).empty()&&startup_world_state_digest(raw)==before,
      "29只读绑定桥与scene4分开，不复用箭头或创建页面");
    raw.scripts.pages.back().task_definition.reset();c(!steam_task_information_skin(raw,id,options),"29缺定义绑定拒绝");
    raw.scripts.pages.back().task_definition=s.tasks.at(task).definition;raw.page_counters.erase(id);
    c(!steam_task_information_skin(raw,id,options),"29缺初始化钟拒绝，不默认置零");
}
void dungeon_footer(Checks &c) {
    SteamDungeonFooterInput in;in.view={10,20,480,320};in.side_width=12;in.page=7;
    in.presentation.ordinal=5;in.presentation.pages={7};
    in.presentation.random_before=20;in.presentation.random_after=21;
    SteamDungeonFooterTask task;task.facility=4;task.background_row=3;task.counter=18;
    task.progress=7;task.maximum=10;
    // 原逆序：已开宝箱先画；随后dy=-18宝箱→继承dy怪物→抖动下降怪物。
    task.challenges={{{100,1,1,25,0,0},0,42},{{50,1,0,0,0,0},30,30},
                     {{25,0,1,5,0,0},-1,-1},{{75,0,3,0,1,0},-1,-1}};
    task.humans={{25,0,0,0},{75,1,15,1}};task.effects={{{3,8,111}},{{4,9,112}}};
    in.task=task;in.presentation.dungeon_jitters={{7,4,0,-1}};
    const auto p=steam_dungeon_footer_skin(in);c(bool(p),"探索底栏冻结调用点可投影");
    const auto images=parts<StartupSkinDraw>(*p);
    c(images.front().image==127&&images.front().crop==std::array<int,4>{-66,54,283,18}&&
      images.front().offset==std::array<int,2>{12,322},"Steam背景保留未消费ClampMax差异和非零VIEW原式");
    c(images[1].image==97&&images[1].sprite==27&&images[1].frame==5&&
      images[2].image==6&&images[2].frame==3,"宝箱先画状态3，第二遍不重复并保留帧3");
    const auto bodies=parts<SteamMonsterBody>(*p);
    c(bodies.size()==2&&bodies[0].image==30&&bodies[0].sprite==30&&
      bodies[0].position==std::array<int,2>{205,305}&&
      bodies[1].position==std::array<int,2>{285,332},"怪物继承前宝箱dy，下降者只取冻结-1抖动");
    c(images[3].image==24&&images[3].crop==std::array<int,4>{0,0,18,18},"怪物先原mode5背景垫片，不替成地表阴影");
    const auto clips=parts<SteamHumanClip>(*p);
    c(clips.size()==4&&clips[0].rectangle==std::array<int,4>{12,322,456,18}&&
      clips[1].rectangle==std::array<int,4>{12,322,283,18}&&
      clips[2].kind==SteamHumanClipKind::pop&&clips[3].kind==SteamHumanClipKind::pop,
      "嵌套外底18高、人物内clip及各自Pop");
    const auto humans=parts<SteamHumanPortrait>(*p);
    c(humans.size()==2&&humans[0].actor.body.image==1&&humans[0].actor.body.frame==0&&
      humans[0].actor.body.sprite==1&&!humans[0].actor.weapon&&!humans[0].actor.shadow&&
      humans[0].image_clip&&humans[0].clip==std::optional<std::array<int,4>>{{230,306,15,14}}&&
      humans[0].position==std::array<int,2>{238,327},"人物逆序、撤退位移及原独立ImageClip，非按aa行走");
    std::vector<StartupSkinDraw> effects,tiles;
    for(const auto &im:images) {
        if(im.image==141)effects.push_back(im);
        if(im.image==86)tiles.push_back(im);
    }
    c(effects.size()==2&&effects[0].frame==1&&effects[0].offset==std::array<int,2>{127,112}&&
      effects[1].frame==0&&effects[1].offset==std::array<int,2>{126,111},"aW逆序并保留独立y，不叠滑入Y");
    c(tiles.size()==9&&tiles.front().crop==std::array<int,4>{0,0,77,18}&&
      tiles.back().crop==std::array<int,4>{64,0,5,18},"77宽右段加13宽平铺，末片5不缩放");
    const auto numbers=parts<SteamFacilityNumber>(*p);
    c(numbers.size()==1&&numbers[0].asset==SteamFacilityAsset::number03_hud&&numbers[0].value==70&&
      numbers[0].position==std::array<int,2>{399,325}&&numbers[0].anchor==4,
      "百分比来自g/h，非日文独立右移6与SEB10");
    c(images.back().image==85&&images.back().offset==std::array<int,2>{400,328},"百分号独立SEB76调用");
    auto jp=in;jp.japanese=true;
    c(parts<SteamFacilityNumber>(*steam_dungeon_footer_skin(jp))[0].position[0]==393,"日文只改右段数字与百分号横移");
    auto zero=in;zero.task->maximum=0;
    c(parts<SteamFacilityNumber>(*steam_dungeon_footer_skin(zero))[0].value==0,"零分母沿原比例helper返回起点");
    auto bad=in;bad.presentation.dungeon_jitters.clear();c(!steam_dungeon_footer_skin(bad),"缺本次抖动拒绝，不重抽");
    bad=in;bad.presentation.dungeon_jitters.push_back({7,4,0,0});c(!steam_dungeon_footer_skin(bad),"重复随机事实拒绝");
    bad=in;bad.presentation.dungeon_jitters[0].facility=5;c(!steam_dungeon_footer_skin(bad),"错误实例抖动拒绝");
    bad=in;bad.presentation.dungeon_jitters[0].challenge=1;c(!steam_dungeon_footer_skin(bad),"错原索引拒绝");
    bad=in;bad.task->challenges[0].monster_sprite=0;c(!steam_dungeon_footer_skin(bad),"拒绝把身体SEB当脸图SEB");
    bad=in;bad.task->humans[0].body_image=32;c(!steam_dungeon_footer_skin(bad),"缺失human32不补透明占位");
    bad=in;bad.task->background_row=6;c(!steam_dungeon_footer_skin(bad),"背景六行之外拒绝");
    bad=in;bad.presentation.pages={8};c(!steam_dungeon_footer_skin(bad),"未获准页不绘制");
    bad=in;bad.presentation.cleared_task=1;c(!steam_dungeon_footer_skin(bad),"清理后的旧绑定不能继续画");
    bad=in;bad.task->challenges.resize(4097);c(!steam_dungeon_footer_skin(bad),"输出规模预算拒绝");
    for(int repeat=0;repeat<3;++repeat) {
        const auto again=steam_dungeon_footer_skin(in);
        c(again&&again->operations.size()==p->operations.size()&&
          in.presentation.random_before==20&&in.presentation.random_after==21&&
          in.presentation.dungeon_jitters[0].offset==-1&&in.task->counter==18,
          "冻结计划重复投影不推进、不累计输出、不消费共同随机");
    }
    in.task.reset();c(steam_dungeon_footer_skin(in)->operations.empty(),"无任务不画底栏");
    in.task=task;in.task->kind=1;c(steam_dungeon_footer_skin(in)->operations.empty(),"非探索任务不画底栏");
}
void dungeon_footer_owner(Checks &c) {
    auto s=test_support::world_fixture();
    const auto made=ref::prepare_world_task_creation(startup_world_runtime_factory(s),0);
    c(made.candidate&&made.candidate->created_task&&write_startup_world_runtime_factory(s,made.candidate->state),
      "底栏桥沿真实factory生成任务和绑定实例");
    s.active_task=*made.candidate->created_task;
    const auto facility=*s.tasks.at(*s.active_task).facility;
    s.dungeon_facilities.at(facility).challenges={{0,1,1,10,0,0}};
    s.dungeon_facilities.at(facility).progress=7;s.dungeon_facilities.at(facility).extent=10;
    s.dungeon_facilities.at(facility).percent=99; // 故意与g/h不同，不能误读缓存。
    s.scene.world.updates=9;s.scene.world.floating_notes={{3,8,111,999}};
    s.scripts.executing_page.reset();s.scripts.pages.front().lifecycle=3;
    const auto page=s.scripts.pages.front().id;
    StartupPresentationRequest request;request.ordinal=1;request.mode=StartupPresentationMode::full_redraw;
    const auto pages=startup_world_presentation_pages(s,request.mode);
    c(bool(pages),"真实场景名单可生成Owner表现请求");request.expected_pages=*pages;
    const auto presented=prepare_startup_world_presentation(s,request);
    c(presented.candidate&&presented.plan&&presented.plan->dungeon_jitters.size()==1,
      "Owner恰消费一次原挑战抖动，皮肤不代抽");s=*presented.candidate;
    const auto digest=startup_world_state_digest(s);
    const auto p=steam_dungeon_footer_skin(s,page,*presented.plan,{0,0,240,240},0,false);
    c(bool(p),"同次candidate/plan只读桥读取真实g/h、挑战、aW与页资格");
    c(parts<SteamFacilityNumber>(*p)[0].value==70&&
      parts<SteamHumanClip>(*p)[1].rectangle[1]==231,
      "进度不是99缓存，滑入从UserData.p=9映射为231");
    for(int repeat=0;repeat<3;++repeat)
        c(steam_dungeon_footer_skin(s,page,*presented.plan,{0,0,240,240},0,false)&&
          startup_world_state_digest(s)==digest&&s.scene.world.floating_notes[0].size()==4,
          "重复桥不改Owner、原aW尾槽或随机，短期输出不保留引用");
    auto bad=s;bad.scene.world.floating_notes={{0,8}};
    c(!steam_dungeon_footer_skin(bad,page,*presented.plan,{0,0,240,240},0,false),"aW不足三槽拒绝");
    bad=s;bad.dungeon_facilities.erase(facility);
    c(!steam_dungeon_footer_skin(bad,page,*presented.plan,{0,0,240,240},0,false),"缺实例进度拒绝");
    bad=s;bad.scene.world.world.facilities.erase(facility);
    c(!steam_dungeon_footer_skin(bad,page,*presented.plan,{0,0,240,240},0,false),"缺实体交回Owner清理，皮肤不修补");
    bad=s;bad.scene.world.world.facilities.at(facility).occupants={{999}};
    c(!steam_dungeon_footer_skin(bad,page,*presented.plan,{0,0,240,240},0,false),"失效人物引用不跳过或用参与者补位");
    bad=s;bad.scripts.pages.front().lifecycle=4;
    c(!steam_dungeon_footer_skin(bad,page,*presented.plan,{0,0,240,240},0,false),"退休场景不能复用旧计划");
    bad=s;bad.scripts.pages.push_back(bad.scripts.pages.front());
    c(!steam_dungeon_footer_skin(bad,page,*presented.plan,{0,0,240,240},0,false),"重复页身份拒绝");
    bad=s;bad.scripts.executing_page=page;
    c(!steam_dungeon_footer_skin(bad,page,*presented.plan,{0,0,240,240},0,false),"轮内执行中不可冒充轮末冻结绘制");
    auto stale=*presented.plan;++stale.random_after;
    c(!steam_dungeon_footer_skin(s,page,stale,{0,0,240,240},0,false),"错误随机交接拒绝");
    stale=*presented.plan;stale.random_before=stale.random_after;
    c(!steam_dungeon_footer_skin(s,page,stale,{0,0,240,240},0,false),"相同after但错误抽数仍拒绝");
    // 仅设置人物缓存/占用显示条件，非自然入场或完整保存认证；Java保留根不能被过滤。
    auto held=s;ref::BattleActorRecord actor;actor.id={901};actor.definition=1;
    held.scene.world.world.ai.retired_actors.emplace(actor.id,actor);
    held.dungeon_actors.emplace(actor.id,ref::DungeonActorProgress{});
    held.scene.world.world.facilities.at(facility).occupants={actor.id,actor.id};
    const auto retained=steam_dungeon_footer_skin(held,page,*presented.plan,{0,0,240,240},0,false);
    c(retained&&parts<SteamHumanPortrait>(*retained).size()==2,
      "设施保留根仍绘头像，重复占用按原引用数保留，不按活跃名单去重");
    held.scene.world.world.ai.battle.actors.emplace(actor.id,actor);
    c(!steam_dungeon_footer_skin(held,page,*presented.plan,{0,0,240,240},0,false),"活跃/保留双持有拒绝");
    held.scene.world.world.ai.battle.actors.clear();
    held.scene.world.world.ai.retired_actors.at(actor.id).id={902};
    c(!steam_dungeon_footer_skin(held,page,*presented.plan,{0,0,240,240},0,false),"人物槽键与实体身份不符拒绝");
}
void dungeon_footer_resources(Checks &c,const std::filesystem::path &root) {
    for(const int id:{6,24,84,85,86,97,102,127,141}) {
        const auto r=steam_dungeon_footer_resource(id);c(bool(r),"底栏每项common有明确路径映射");
        const auto file=root.parent_path()/r->published_image;
        Image im=LoadImage(file.string().c_str());c(im.data&&im.width>0&&im.height>0,"底栏真实PNG可读取");
        if(id==127)c(im.width==217&&im.height==108,"底栏真实背景六行18高");
        if(id==86)c(im.width==77&&im.height==18,"右段原图77宽，铺片不猜尺寸");
        if(r->published_sprite) {
            std::ifstream stream(root.parent_path()/r->published_sprite,std::ios::binary);
            const std::vector<std::uint8_t> data{std::istreambuf_iterator<char>(stream),{}};
            const auto seb=dungeon_village_tools::parse_legacy_seb(data);
            const int frame=id==6?3:id==97?5:0;
            bool found{};
            for(const auto &layer:seb.layers)for(const auto &part:layer.parts) {
                if(part.frame!=frame)continue;
                found=true;
                c(part.source_x>=0&&part.source_y>=0&&part.source_x+part.width<=im.width&&
                  part.source_y+part.height<=im.height,"具名SEB请求在实际PNG范围内，替图97独立验证");
            }
            c(found,"底栏实际SEB有被消费帧");
        }
        UnloadImage(im);
    }
    c(!steam_dungeon_footer_resource(-1)&&!steam_dungeon_footer_resource(103),"非底栏图片不猜映射");
}
} // namespace

// 仅扩现visuals套件；报名扣费、held推进、随机次序与回滚由pages主责。
int check_steam_task_selection_skins(const std::filesystem::path &source_root) {
    Checks c;directory(c);prompt(c);recruitment(c);owner(c);resources(c,source_root);
    rosters(c);departure(c);roster_resources(c,source_root);results(c,source_root);
    task_information(c);task_information_owner(c);victory(c);victory_owner(c);deadline(c,source_root);
    result_owner(c);deadline_owner(c);roster_owner(c);dungeon_footer(c);dungeon_footer_owner(c);
    dungeon_footer_resources(c,source_root);return c.count;
}
