#include "ark/simulation/presentation/steam_task_selection_skin.hpp"
#include "ark/simulation/presentation/steam_task_roster_skin.hpp"
#include "ark/simulation/persistence/startup_world_persistence.hpp"
#include "ark/assets/sprite.hpp"
#include "support/world_fixture.hpp"
#include <raylib.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>

using namespace ark::simulation;
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
    const auto root=source_root;
    const auto background=steam_task_selection_resource(SteamTaskSelectionAsset::recruitment_background);
    const auto treasure=steam_task_selection_resource(SteamTaskSelectionAsset::treasure);
    c(background&&background->image==0&&background->sprite==-1&&!background->published_sprite,
      "背景为event/image0整图，不能假造SEB");
    c(treasure&&treasure->image==6&&treasure->sprite==27&&treasure->published_sprite,
      "宝箱为common/image6/SEB27");
    for(const auto &r:{*background,*treasure}) {
        const auto path=root/std::filesystem::path(r.published_image).lexically_relative("original");
        c(std::filesystem::is_regular_file(path),"具名正式PNG存在");
        auto png=LoadImage(path.string().c_str());
        c(png.data!=nullptr,"正式PNG可解码");
        const int w=png.width,h=png.height;UnloadImage(png);
        c(w==(r.image==0?180:22)&&h==(r.image==0?80:95),"原背景180×80与宝箱22×95，不以截图推裁片");
        if(r.published_sprite) {
            const auto seb=ark::assets::parse_legacy_seb(bytes(root/std::filesystem::path(r.published_sprite).lexically_relative("original")));
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
    c(badge&&badge->image==147&&std::filesystem::is_regular_file(root/std::filesystem::path(badge->published_image).lexically_relative("original")),
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
    const auto root=source_root;
    for(const auto asset:{SteamTaskRosterAsset::departure_background,SteamTaskRosterAsset::secretary_face,SteamTaskRosterAsset::shadow}) {
        const auto r=steam_task_roster_resource(asset);c(bool(r)&&std::filesystem::is_regular_file(root/std::filesystem::path(r->published_image).lexically_relative("original")),"三资源正式路径存在");
        auto image=LoadImage((root/std::filesystem::path(r->published_image).lexically_relative("original")).string().c_str());c(image.data!=nullptr,"出发资源PNG可解码");
        const int w=image.width,h=image.height;UnloadImage(image);
        c(w==(asset==SteamTaskRosterAsset::departure_background?93:asset==SteamTaskRosterAsset::secretary_face?34:123)&&
          h==(asset==SteamTaskRosterAsset::departure_background?82:asset==SteamTaskRosterAsset::secretary_face?27:3),
          "原mp_back93×82/秘书34×27/阴影123×3，各裁片独立");
        if(asset==SteamTaskRosterAsset::shadow)c(r->image==3&&r->sprite==25&&r->published_sprite&&
          ark::assets::parse_legacy_seb(bytes(root/std::filesystem::path(r->published_sprite).lexically_relative("original"))).frame_count==4,"阴影SEB25真实四帧，phase1只请求帧0");
    }
    for(int raw:{25,26,27,28}) {
        const auto in=roster_input(raw);const auto plan=steam_task_roster_skin(in);c(bool(plan),"稳定只读计划");
        for(int repeat=0;repeat<4;++repeat) {
            const auto again=steam_task_roster_skin(in);
            c(again&&again->operations.size()==plan->operations.size(),"不变DTO重复绘制规模稳定，不保留跨查询池");
        }
        for(const auto &op:plan->operations)if(const auto *v=std::get_if<SteamTaskRosterImage>(&op)) {
            const auto resource=steam_task_roster_resource(v->asset);c(bool(resource),"每个新增资源图元都有消费映射");
            auto image=LoadImage((root/std::filesystem::path(resource->published_image).lexically_relative("original")).string().c_str());c(image.data!=nullptr,"图元资源能解码");
            if(v->crop) {const auto &r=*v->crop;c(r[0]>=0&&r[1]>=0&&r[2]>0&&r[3]>0&&
                r[0]+r[2]<=image.width&&r[1]+r[3]<=image.height,"背景三片及秘书16×16裁片不越原PNG");}
            UnloadImage(image);
        }
    }
}
} // namespace

// 仅扩现visuals套件；报名扣费、held推进、随机次序与回滚由pages主责。
int check_steam_task_selection_skins(const std::filesystem::path &source_root) {
    Checks c;directory(c);prompt(c);recruitment(c);owner(c);resources(c,source_root);
    rosters(c);departure(c);roster_resources(c,source_root);return c.count;
}
