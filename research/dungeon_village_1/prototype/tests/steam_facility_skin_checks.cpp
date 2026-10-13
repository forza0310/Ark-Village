#include "dungeon_village_prototype/steam_facility_skin.hpp"
#include "dungeon_village_prototype/steam_facility_catalog_skin.hpp"
#include "dungeon_village_prototype/steam_facility_reputation_skin.hpp"
#include "dungeon_village_prototype/steam_facility_effect_skin.hpp"
#include "dungeon_village_prototype/startup_world_facility_catalog.hpp"
#include "dungeon_village_prototype/startup_world_commerce.hpp"
#include "dungeon_village_prototype/startup.hpp"
#include "dungeon_village_prototype/startup_world_projection.hpp"
#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "support/world_fixture.hpp"
#include "dungeon_village_tools/sprite.hpp"
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
    void operator()(bool ok,const char *why) {
        ++count;if(!ok)throw std::runtime_error(std::string("Steam81皮肤：")+why);
    }
};
template<class T> std::vector<T> parts(const SteamFacilitySkinPlan &p) {
    std::vector<T> result;
    for(const auto &draw:p.draws)if(const auto *value=std::get_if<T>(&draw))result.push_back(*value);
    return result;
}
SteamFacilityUpgradeSkinInput input() {
    // 冻结显示载荷条件，不代表自然建筑35已升级，也不调用业务Owner制造升级。
    SteamFacilityUpgradeSkinInput in;
    in.definition=35;in.mapchip=35;in.level=2;in.frame2=20;
    in.title_widths=std::array<int,2>{80,82};
    in.notice_widths=std::array<int,4>{12,30,4,34};
    in.attributes={{{100,110,10},{10,11,1},{20,18,-2}}};in.limits={110,20,40};
    return in;
}
std::array<int,3> values(const SteamFacilitySkinPlan &p) {
    std::array<int,3> result{};
    for(const auto &n:parts<SteamFacilityNumber>(p))
        if(n.parameter>=0&&n.kind!=SteamFacilityNumberKind::plus_value)result.at(n.parameter)=n.value;
    return result;
}
std::size_t images(const SteamFacilitySkinPlan &p,SteamFacilityAsset asset) {
    const auto entries=parts<SteamFacilityImage>(p);
    return std::count_if(entries.begin(),entries.end(),[=](const auto &v){return v.asset==asset;});
}
std::vector<std::uint8_t> bytes(const std::filesystem::path &p) {
    std::ifstream in(p,std::ios::binary);
    if(!in)throw std::runtime_error("Steam81源资源缺失："+p.string());
    return {std::istreambuf_iterator<char>(in),{}};
}
template<class T> std::vector<T> residence_parts(const SteamHumanOverviewPlan &p) {
    std::vector<T> out;
    for(const auto &op:p.operations)if(const auto *v=std::get_if<T>(&op))out.push_back(*v);
    return out;
}
void residence_checks(Checks &check) {
    // 原Update2四行／Draw2_3五行的独立坐标例，不用被测helper生成期望。
    struct CursorCase { SteamResidenceCursor before;int count;bool up,down;SteamResidenceCursor after; };
    for(const auto &v:std::array<CursorCase,6>{{
        {{0,0},7,true,false,{6,3}},{{6,3},7,false,true,{0,0}},
        {{3,0},6,false,true,{4,1}},{{4,1},6,true,true,{4,1}},
        {{4,0},6,false,false,{4,1}},{{0,0},1,true,true,{0,0}}}}) {
        const auto got=steam_residence_cursor(v.before,v.count,v.up,v.down);
        check(got&&got->selection==v.after.selection&&got->first==v.after.first,
              "80上后下、循环与四行首行修正保留源顺序");
    }
    check(!steam_residence_cursor({0,0},0,false,false)&&
          !steam_residence_cursor({7,0},7,false,false)&&
          !steam_residence_cursor({0,-1},7,false,false),"80空目录与越界游标helper拒绝");
    SteamResidenceInput in;in.cursor={3,0};in.order={5,1,4,2,0,3};
    in.options.title_widths=std::array<int,2>{60,60};in.options.bottom_width=84;
    for(int id=0;id<6;++id)in.humans.push_back({id,0,id%2,100+id,"人物"+std::to_string(id),id%2==0});
    const auto plan=steam_residence_skin(in);check(bool(plan),"80显式冻结顺序与当前人数据纯计划");
    check(plan->title=="选择入住者"&&plan->soft_labels==std::array<int,2>{0,2},"80独立标题与软返回");
    const auto rows=residence_parts<SteamHumanEquipmentRow>(*plan);
    const auto portraits=residence_parts<SteamHumanPortrait>(*plan);
    const auto fees=residence_parts<SteamFacilityNumber>(*plan);
    check(rows.size()==5&&portraits.size()==5&&fees.size()==5,"80四行更新仍输出五行，不能少末行");
    constexpr std::array<int,5> ys{{97,116,135,154,173}};
    constexpr std::array<int,5> values{{105,101,104,102,100}};
    for(int i=0;i<5;++i) {
        check(rows[i].touch.component==11&&rows[i].touch.value==(0x20000|i)&&rows[i].touch.option==16&&
              rows[i].touch.rectangle==std::optional<std::array<int,4>>{{23,ys[i]-2,191,16}}&&
              rows[i].margin==std::array<int,4>{29,29,0,0},"80行热区marker16与双29 margin不套商品79");
        check(portraits[i].position==std::array<int,2>{37,ys[i]+20}&&
              portraits[i].clip==std::optional<std::array<int,4>>{{29,ys[i]-1,15,14}}&&
              portraits[i].actor.body.sprite==1,"80人物脚底与窄clip独立锚");
        check(fees[i].kind==SteamFacilityNumberKind::money&&fees[i].value==values[i]&&
              fees[i].position==std::array<int,2>{212,ys[i]+1}&&fees[i].asset==
              (i<2?SteamFacilityAsset::number12:SteamFacilityAsset::number08),
              "80费用沿冻结身份顺序，实时足额与不足色分别消费");
    }
    int selected=0;
    for(const auto &frame:residence_parts<SteamStartupFrameDraw>(*plan))if(const auto *r=std::get_if<StartupSkinRect>(&frame))
        if(r->rgb==std::array<int,3>{255,153,55})selected+=r->rect==std::array<int,4>{23,152,191,16};
    const auto pictures=residence_parts<SteamFacilityImage>(*plan);
    check(selected==1&&pictures.size()==1&&pictures[0].asset==SteamFacilityAsset::finger&&
          pictures[0].position==std::array<int,2>{21,162},"80仅第四行橙底与手形");
    const auto scroll=residence_parts<SteamHumanCatalogueScroll>(*plan);
    check(scroll.size()==1&&scroll[0].rectangles[1].rect==std::array<int,4>{220,85,5,93}&&
          scroll[0].arguments==std::array<int,3>{6,5,0x20000},"80滚条largeChange5、六人thumb93保留绘制合同");
    const auto texts=residence_parts<SteamHumanOverviewText>(*plan);
    check(!texts.empty()&&texts.back().rectangle==std::array<int,4>{70,197,100,16}&&texts.back().anchor==34&&texts.back().layout&&
          residence_parts<SteamStartupTouch>(*plan).empty(),"80底提示原TextLayout，不补Touch4或Touch2");
    const auto size=plan->operations.size();
    for(int i=0;i<12;++i)check(steam_residence_skin(in)->operations.size()==size,"80重复查询图元规模不累计");
    auto altered=in;altered.cursor={4,1};altered.options.scroll_first_touch=true;
    const auto moved=steam_residence_skin(altered);
    const auto moved_scroll=residence_parts<SteamHumanCatalogueScroll>(*moved);
    check(moved_scroll[0].rectangles[1].rect==std::array<int,4>{220,103,5,93}&&
          moved_scroll[0].rectangles[1].rgb==std::array<int,3>{246,129,0},"80显式首行1滚条位置103与触摸色");
    altered=in;altered.options.bottom_width=300;
    check(residence_parts<SteamHumanOverviewText>(*steam_residence_skin(altered)).back().rectangle==
          std::array<int,4>{12,197,216,16},"80单文字底宽按源夹200加16");
    altered=in;altered.order.clear();check(!steam_residence_skin(altered),"80原空X已关闭，不造活页");
    altered=in;altered.order.back()=999;check(!steam_residence_skin(altered),"80不可见末候选缺定义也拒绝");
    altered=in;altered.order.back()=altered.order.front();check(!steam_residence_skin(altered),"80定义遍历名单不接受重复候选");
    altered=in;altered.humans.push_back(altered.humans.front());check(!steam_residence_skin(altered),"80实时定义重复拒绝");
    altered=in;altered.cursor={4,0};check(!steam_residence_skin(altered),"80纯绘制不隐式修正first");
    altered=in;altered.humans.back().fee=-1;check(!steam_residence_skin(altered),"80负费拒绝");
    altered=in;altered.humans.back().sex=2;check(!steam_residence_skin(altered),"80不可见坏性别拒绝");
    altered=in;altered.options.bottom_width.reset();check(!steam_residence_skin(altered),"80缺真实底文测宽拒绝");
}
}
// 挂既有visuals套件；只验页面只读计划和资源绑定，不复制升级/声音/Owner状态机。
int check_steam_facility_skin(const std::filesystem::path &source_root) {
    Checks check;
    residence_checks(check);
    // 此层只补Owner到皮肤的绑定与矩阵方向；完整升级业务／暂停／恢复由既有套件主责。
    auto owner=test_support::world_fixture();
    const auto facility=std::uint64_t{4};
    const int definition=owner.scene.world.world.facilities.at(facility).placement.definition_id;
    auto &progress=owner.scene.world.world.facility_uses.at(definition);
    progress.upgrade_pending=true;progress.completed_uses=57; // 明确条件夹具，不代表自然升级。
    check(open_startup_world_facility_page(owner,facility)==StartupWorldRuntimeError::none,
          "真实升级入口提供页面绑定");
    const auto page=owner.scripts.pages.back().id;
    const SteamFacilityUpgradeSkinOptions options{0,false,std::array<int,2>{80,82},
                                                  std::array<int,4>{12,30,4,34}};
    check(!steam_facility_upgrade_skin(owner,page,options),"皮肤查询不代替页面初始化");
    auto tick=prepare_startup_world_runtime(owner);
    check(tick.candidate.has_value(),"升级页初始化候选");
    owner=std::move(*tick.candidate);
    check(acknowledge_startup_world_runtime_page(owner,page)==StartupWorldRuntimeError::none&&
          acknowledge_startup_world_runtime_page(owner,page)==StartupWorldRuntimeError::none,
          "真实确认切到属性页");
    const auto frozen=startup_world_state_digest(owner);
    const auto bound=steam_facility_upgrade_skin(owner,page,options);
    check(bound.has_value(),"已初始化绑定产生完整只读皮肤");
    const auto bound_values=values(*bound);
    for(std::size_t slot=0;slot<3;++slot)
        check(bound_values[slot]==owner.facility_upgrade_display[0][slot],
              "冻结矩阵按属性转置，phase1 frame0显示各属性前值");
    const auto bound_mini=parts<SteamFacilityImage>(*bound);
    check(owner.page_counters.at(page)==0&&owner.page_secondary_counters.at(page)==1&&
          std::any_of(bound_mini.begin(),bound_mini.end(),[](const auto &image) {
              return image.asset==SteamFacilityAsset::mini&&image.frame==5;
          }),"主计数换段清零，角色仍用独立frame2的跳跃帧");
    check(steam_facility_upgrade_skin(owner,page,options).has_value()&&
          startup_world_state_digest(owner)==frozen,"重绘不改变Owner／随机／输出");
    auto malformed=owner;malformed.page_secondary_counters.erase(page);
    check(!steam_facility_upgrade_skin(malformed,page,options),"缺独立计数不能猜主计数");
    check(!steam_facility_upgrade_skin(owner,page+1000,options),"未知页面不能借当前绑定绘图");
    malformed=owner;malformed.scripts.pages.back().lifecycle=4;
    check(!steam_facility_upgrade_skin(malformed,page,options),"已退休页面不再投影");
    using A=SteamFacilityAsset;using R=SteamFacilityTextRole;using N=SteamFacilityNumberKind;
    auto in=input();
    const auto original=steam_facility_upgrade_skin(in);
    check(original&&original->soft_labels==std::array<int,2>{0,0}&&original->touches.size()==1&&
          original->touches[0].component==2&&original->touches[0].option==2&&
          !original->touches[0].rectangle&&!original->touches[0].image_draw,
          "双空软标签与无矩形确认注册分开，不能猜桌面全屏热区或右返回");
    const auto rects=parts<StartupSkinRect>(*original);
    check(rects.size()==5&&rects[0].rect==std::array<int,4>{8,33,224,173}&&
          rects[1].rect==std::array<int,4>{9,34,222,171}&&
          rects[2].rect==std::array<int,4>{15,139,209,63},"复用Steam窗框半开几何而非APK整图缩放");
    const auto labels=parts<SteamFacilityText>(*original);
    check(labels.size()==5&&labels[0].role==R::upgrade_title&&labels[0].rectangle[0]==80&&
          labels[1].rectangle[0]==79&&labels[2].role==R::facility_notice&&labels[2].argument==35&&
          labels[2].rectangle==std::array<int,4>{120,166,0,0}&&
          labels[3].rectangle==std::array<int,4>{90,184,0,0}&&
          labels[4].rectangle==std::array<int,4>{150,184,0,0},
          "标题双测宽、非日文10/6比例及prefix/suffix布局独立");
    const auto numbers=parts<SteamFacilityNumber>(*original);
    check(numbers.size()==1&&numbers[0].kind==N::number&&numbers[0].asset==A::number09&&
          numbers[0].value==2&&numbers[0].position==std::array<int,2>{128,183}&&numbers[0].anchor==1,
          "第四次prefix宽34独立于第一次30，等级仍是SEB16请求");
    check(images(*original,A::upgrade_background)==1&&images(*original,A::mini_background)==1&&
          parts<SteamFacilityMapchip2>(*original).front().position==std::array<int,2>{120,105}&&
          parts<SteamFacilityMapchip2>(*original).front().orientation==0,
          "phase0仍画event背景与朝向0的Mapchip2，不以负frame跳过整个helper");
    for(const auto &sample:std::array<std::array<int,2>,5>{{{0,0},{5,0},{6,1},{39,46},{40,48}}}) {
        in.frame=sample[0];const auto plan=steam_facility_upgrade_skin(in);
        const auto clips=parts<SteamFacilityClip>(*plan);
        check(clips.size()==4&&clips[0].rectangle==std::array<int,4>{0,158,240,sample[1]}&&
              clips[1].kind==SteamFacilityClipKind::pop&&clips[2].rectangle==std::array<int,4>{80,74,80,64}&&
              clips[3].kind==SteamFacilityClipKind::pop,"滑入clip与大图clip各自平衡且原顺序固定");
    }
    for(const auto &sample:std::array<std::array<int,2>,6>{{{39,0},{40,0},{49,0},{50,1},{64,1},{65,0}}}) {
        in.frame=sample[0];check(images(*steam_facility_upgrade_skin(in),A::arrow)==std::size_t(sample[1]),
              "phase0确认40不等于箭头50，提示按模25前15显示");
    }
    in.phase=1;
    for(const auto &sample:std::array<std::array<int,4>,8>{{{1,100,10,20},{2,100,10,20},
        {31,100,10,20},{32,100,10,20},{33,100,10,20},{34,101,10,20},{48,109,10,20},{49,110,11,18}}}) {
        in.frame=sample[0];const auto plan=steam_facility_upgrade_skin(in);
        check(plan&&values(*plan)==std::array<int,3>{sample[1],sample[2],sample[3]},
              "三属性特殊countAnime保留小差与下降独立行为，非线性lerp");
        check(images(*plan,A::maximum)==(sample[0]>=49?1U:0U),"MAX读当前显示值而非提前读最终新值");
    }
    for(const auto change:std::array<std::array<int,3>,4>{{{10,10,0},{10,11,1},{10,9,-1},{10,8,-2}}}) {
        in.attributes[0]=change;in.frame=48;
        check(values(*steam_facility_upgrade_skin(in))[0]==10,"0与正负1及下降在48仍为old");
        in.frame=49;check(values(*steam_facility_upgrade_skin(in))[0]==change[1],"49外层切新值不由插值helper假推");
    }
    in.attributes[0]={10,12,2};
    for(const auto sample:std::array<std::array<int,2>,4>{{{34,11},{36,11},{37,12},{48,12}}}) {
        in.frame=sample[0];check(values(*steam_facility_upgrade_skin(in))[0]==sample[1],"较小正差在有效end37提前结束");
    }
    in=input();in.phase=1;
    for(const auto &sample:std::array<std::array<int,2>,5>{{{54,0},{55,1},{69,1},{70,0},{80,1}}}) {
        in.frame=sample[0];check(images(*steam_facility_upgrade_skin(in),A::arrow)==std::size_t(sample[1]),
              "phase1提示以frame55为模25起点，不使用65自动关闭");
    }
    in.frame=4;
    auto plan=steam_facility_upgrade_skin(in);
    const auto deltas=parts<SteamFacilityNumber>(*plan);
    const auto delta=std::find_if(deltas.begin(),deltas.end(),[](const auto &n){return n.kind==N::plus_value;});
    check(delta!=deltas.end()&&delta->parameter==0&&delta->position==std::array<int,2>{190,154}&&
          std::count_if(deltas.begin(),deltas.end(),[](const auto &n){return n.kind==N::plus_value;})==1,
          "差值按slot延迟6，TIME12原float32抛物高度向零为4");
    for(const auto &sample:std::array<std::array<int,3>,6>{{{0,1,0},{5,1,4},{19,1,0},{20,0,0},{29,0,0},{30,1,0}}}) {
        in.frame2=sample[0];plan=steam_facility_upgrade_skin(in);
        auto mini=parts<SteamFacilityImage>(*plan);
        mini.erase(std::remove_if(mini.begin(),mini.end(),[](const auto &v){return v.asset!=A::mini;}),mini.end());
        check(mini.size()==4&&mini[0].position==std::array<int,2>{52,144}&&mini[0].frame==sample[1]&&
              mini[1].position==std::array<int,2>{52,144-sample[2]}&&mini[1].frame==(sample[1]?5:3)&&
              mini[2].position==std::array<int,2>{188,144}&&mini[3].frame==(sample[1]?4:2),
              "独立frame2按左影左身右影右身，阶段计数不重启动作");
    }
    in=input();in.japanese=true;in.notice_widths.reset();
    check(steam_facility_upgrade_skin(in).has_value()&&
          parts<SteamFacilityText>(*steam_facility_upgrade_skin(in)).size()==6,
          "日文无需非日文四次宽度且保独立五段通知布局");
    in.phase=1;plan=steam_facility_upgrade_skin(in);
    const auto parameter=parts<SteamFacilityText>(*plan).back();
    check(parameter.role==R::parameter_name&&parameter.rectangle==std::array<int,4>{48,194,0,0}&&
          !parameter.anchor&&parameter.font_size==0,"标签用无anchor重载，日文保留当前字号");
    // 检查完整序列的clip栈和覆盖层序，分类提取不能证明背景或MAX没有盖错层。
    in=input();in.frame=50;plan=steam_facility_upgrade_skin(in);
    const auto index=[&](auto matches) {
        return std::size_t(std::find_if(plan->draws.begin(),plan->draws.end(),matches)-plan->draws.begin());
    };
    const auto is_image=[](A asset) {
        return [=](const SteamFacilityDraw &draw) {
            const auto *v=std::get_if<SteamFacilityImage>(&draw);return v&&v->asset==asset;
        };
    };
    int depth=0;bool balanced=true;std::size_t first_pop=plan->draws.size();
    for(std::size_t i=0;i<plan->draws.size();++i)if(const auto *c=std::get_if<SteamFacilityClip>(&plan->draws[i])) {
        if(c->kind==SteamFacilityClipKind::push_intersect)++depth;
        else {--depth;first_pop=std::min(first_pop,i);}
        balanced=balanced&&depth>=0;
    }
    check(balanced&&depth==0&&first_pop<index(is_image(A::arrow))&&
          index(is_image(A::arrow))<index(is_image(A::upgrade_background))&&
          index(is_image(A::upgrade_background))<index(is_image(A::mini)),
          "正文clip退栈后才画箭头，箭头先于背景，人物最后画，原序不可分组重排");
    in.phase=1;plan=steam_facility_upgrade_skin(in);
    const auto max_index=index(is_image(A::maximum));
    check(max_index>0&&max_index+1<plan->draws.size()&&
          std::holds_alternative<SteamFacilityText>(plan->draws[max_index-1])&&
          std::get<SteamFacilityText>(plan->draws[max_index-1]).role==R::parameter_name&&
          std::holds_alternative<SteamFacilityNumber>(plan->draws[max_index+1])&&
          std::get<SteamFacilityNumber>(plan->draws[max_index+1]).kind==N::money&&
          std::get<SteamFacilityNumber>(plan->draws[max_index+1]).position==std::array<int,2>{134,158},
          "MAX在标签与主金额之间，金额记录原134锚而非提前施加helper偏移");
    const auto draw_count=plan->draws.size();
    bool bounded=true;
    for(int repeat=0;repeat<32;++repeat) {
        const auto repeated=steam_facility_upgrade_skin(in);
        bounded=bounded&&repeated&&repeated->draws.size()==draw_count&&repeated->touches.size()==1;
    }
    check(bounded&&in.frame==50&&in.frame2==20&&in.attributes==input().attributes,
          "重复只读请求不消费计数或冻结属性，单帧图元及触摸注册规模不累加");
    for(int fault=0;fault<12;++fault) {
        auto bad=input();
        if(fault==0)bad.title_widths.reset();
        if(fault==1)bad.notice_widths.reset();
        if(fault==2)bad.attributes[1][2]=2;
        if(fault==3)bad.phase=2;
        if(fault==4)bad.frame=-1;
        if(fault==5)bad.frame2=-1;
        if(fault==6)(*bad.notice_widths)[0]=std::numeric_limits<int>::max();
        if(fault==7)bad.view_y=std::numeric_limits<int>::max();
        if(fault==8)bad.mapchip=-1;
        if(fault==9)bad.level=0;
        if(fault==10)bad.level=6;
        if(fault==11)(*bad.title_widths)[1]=-1;
        check(!steam_facility_upgrade_skin(bad),"缺测量、坏冻结关系/阶段/计数及算术范围显式拒绝");
    }
    const auto assets=source_root.parent_path();
    for(int id=0;id<=static_cast<int>(A::mini_background);++id) {
        const auto resource=steam_facility_resource(static_cast<A>(id));
        check(resource&&std::filesystem::is_regular_file(assets/resource->published_image),"具名Steam资源映射到实际已出版图片");
        if(resource->published_sprite)check(std::filesystem::is_regular_file(assets/resource->published_sprite),"SEB引用不复制另一份素材");
    }
    check(!steam_facility_resource(static_cast<A>(99)),"未知资源枚举拒绝");
    // 固定Steam85项映射已经逐资源交叉；DrawMapchip2中心偏移不同于建设helper。
    const std::array<std::vector<std::array<int,2>>,3> first_points{{{{90,105}},{{105,97},{75,112}},
        {{90,90},{60,105},{120,105},{90,120}}}};
    const std::array<std::vector<std::array<int,2>>,3> second_points{{{{90,105}},{{45,97},{75,112}},
        {{90,90},{120,105},{60,105},{90,120}}}};
    const auto &catalog=startup_evidence();
    const auto &definitions=startup_world_rules().facilities;
    std::size_t mapped=0;bool road_seen=false;
    for(const auto &art:catalog.displays) {
        const auto def=std::find_if(definitions.begin(),definitions.end(),
            [&](const auto &d){return d.id==art.definition_id;});
        check(def!=definitions.end(),"mapchip反查完整世界定义存在，不能读早期切片目录");
        for(int orientation=0;orientation<2;++orientation) {
            const auto draws=steam_facility_mapchip2_draws({art.id,{120,105},orientation});
            const auto &expected=(orientation?second_points:first_points).at(def->shape);
            bool matches=draws&&draws->size()==expected.size();
            if(matches)for(std::size_t i=0;i<expected.size();++i)
                matches=matches&&(*draws)[i].sprite==art.sprite&&(*draws)[i].position==expected[i]&&
                    (*draws)[i].frame==static_cast<int>(2*i)+orientation;
            check(matches,"两朝向pattern原序与中心偏移独立oracle，不整图翻转");
        }
        if(def->kind==6) {
            const auto draws=steam_facility_mapchip2_draws({art.id,{120,105},0});
            check(draws&&draws->front().frame==0,"Mapchip2道路直接pattern0，不套建设道路frame11");road_seen=true;
        }
        ++mapped;
    }
    check(mapped==85&&road_seen,"覆盖固定85项mapchip及道路分支");
    check(!steam_facility_mapchip2_draws({-1,{120,105},0})&&
          !steam_facility_mapchip2_draws({999,{120,105},0})&&
          !steam_facility_mapchip2_draws({catalog.displays.front().id,{120,105},2})&&
          !steam_facility_mapchip2_draws({catalog.displays.front().id,{std::numeric_limits<int>::min(),105},0}),
          "未知图块、朝向及中心算术溢出不返回局部图元");
    // 已核helper的请求序列oracle；数字步宽由SEB提供，金额/加号固定8且不测Font。
    SteamFacilityNumber numeric{N::number,A::number09,123,{100,20},1,0,-1};
    for(const auto anchor:std::array<std::array<int,2>,4>{{{0,100},{2,89},{4,77},{6,89}}}) {
        numeric.anchor=anchor[0];
        const auto draws=steam_facility_number_draws(numeric,7);
        check(draws&&draws->size()==3&&(*draws)[0].frame==1&&(*draws)[1].frame==2&&(*draws)[2].frame==3&&
              (*draws)[0].position==std::array<int,2>{anchor[1],20}&&
              (*draws)[2].position==std::array<int,2>{anchor[1]+16,20},
              "普通数字按SEB宽加padding且anchor6以居中优先");
    }
    numeric={N::money,A::number08,1234567,{100,20},0,0,0};
    auto number_draws=steam_facility_number_draws(numeric,0);
    const std::array<int,10> money_frames{1,2,10,3,4,5,10,6,7,20};
    const std::array<int,10> money_x{35,43,41,51,59,67,65,75,83,91};
    bool money_matches=number_draws&&number_draws->size()==money_frames.size();
    if(money_matches)for(std::size_t i=0;i<money_frames.size();++i)
        money_matches=money_matches&&(*number_draws)[i].frame==money_frames[i]&&
            (*number_draws)[i].position==std::array<int,2>{money_x[i],20};
    check(money_matches,"金额先dx-9，逗号在后一个数字之后绘制，货币共享右锚");
    numeric={N::plus_value,A::number05,-12,{190,40},0,0,1};
    number_draws=steam_facility_number_draws(numeric,0);
    check(number_draws&&number_draws->size()==4&&(*number_draws)[0].frame==-3&&
          (*number_draws)[0].position==std::array<int,2>{166,40}&&(*number_draws)[1].frame==1&&
          (*number_draws)[2].frame==2&&(*number_draws)[3].frame==14&&(*number_draws)[3].position==std::array<int,2>{166,40},
          "signed串负号请求-3且加号无条件后画，不能猜负帧最终像素");
    numeric.value=0;number_draws=steam_facility_number_draws(numeric,0);
    check(number_draws&&number_draws->size()==2&&(*number_draws)[0].frame==0&&
          (*number_draws)[0].position[0]==182&&(*number_draws)[1].frame==14&&(*number_draws)[1].position[0]==174,
          "helper自身对零也画加号，81差零隐藏属于外层");
    numeric={N::number,A::number09,-12,{100,20},0,2,-1};
    number_draws=steam_facility_number_draws(numeric,7);
    check(number_draws&&number_draws->size()==1&&number_draws->front().frame==-12&&number_draws->front().position[0]==97,
          "普通负值保持一位负商请求，不误用逗号helper的signed串");
    check(!steam_facility_number_draws(numeric,0),"普通数字缺实际SEB步宽拒绝");
    numeric.kind=N::money;numeric.position[0]=std::numeric_limits<int>::min();
    check(!steam_facility_number_draws(numeric,0),"金额dx-9溢出拒绝且无部分图元结果");
    numeric.position[0]=100;numeric.asset=A::mini;
    check(!steam_facility_number_draws(numeric,8),"不能用人物图集替代数字资源身份");
    // 39新SEB19只补资产准入与自身边界，不重复旧money完整逗号矩阵。
    const auto loss=steam_facility_resource(A::number12);
    check(loss && loss->image==109 && loss->sprite==19 &&
          std::string(loss->published_image)=="steam-common/number12.png" &&
          std::string(loss->published_sprite)=="original/common/number12.seb",
          "39负利润SEB19绑定Steam差异image109并复用同字节SEB");
    numeric={N::number,A::number12,12,{100,20},0,4,-1};
    number_draws=steam_facility_number_draws(numeric,8);
    check(number_draws && number_draws->size()==2 && number_draws->at(0).frame==1 &&
          number_draws->at(1).frame==2 && number_draws->at(0).position==std::array<int,2>{84,20} &&
          number_draws->at(1).position==std::array<int,2>{92,20},"SEB19普通数右锚实际8步宽");
    check(!steam_facility_number_draws(numeric,7),"SEB19拒绝错误步宽，不推测另一数字图集宽度");
    numeric.kind=N::money;numeric.value=std::numeric_limits<int>::min();
    number_draws=steam_facility_number_draws(numeric,8);
    check(number_draws && number_draws->size()==15 && number_draws->front().frame==-3 &&
          number_draws->front().position==std::array<int,2>{3,20} && number_draws->back().frame==20 &&
          number_draws->back().position==std::array<int,2>{91,20},
          "SEB19金额INT_MIN保留signed串负帧和逗号/单位，不声称负帧已知像素");
    numeric.kind=N::plus_value;
    check(!steam_facility_number_draws(numeric,8),"SEB19没有加号调用资格，不能借旧资产允许plus");
    const auto loss_seb=dungeon_village_tools::parse_legacy_seb(bytes(assets/loss->published_sprite));
    for(const int frame:{0,9,10,20}) {
        const auto &parts=loss_seb.layers.at(0).parts;
        const auto part=std::find_if(parts.begin(),parts.end(),[=](const auto &p){return p.frame==frame;});
        check(part!=parts.end() && part->image_index==109 && part->source_x>=0 && part->source_y>=0 &&
              part->source_x+part->width<=100 && part->source_y+part->height<=21,
              "SEB19普通数字/逗号/单位实际帧裁片均落在Steam100×21图内");
    }
    // HUD普通SEB10与既有等级SEB11共享PNG但不是同一源行/资源身份。
    numeric={N::count,A::number05,1234,{210,20},0,4,-1};
    number_draws=steam_facility_number_draws(numeric,8);
    check(number_draws&&number_draws->size()==6&&number_draws->front().position[0]==168&&
          number_draws->at(2).frame==10&&number_draws->at(2).position[0]==174&&
          number_draws->back().asset==A::inventory_unit&&number_draws->back().frame==2&&
          number_draws->back().position[0]==200,
          "Draw_count retains comma order then independent SEB76 unit at dx-10");
    numeric.kind=N::person_count;numeric.value=0;
    number_draws=steam_facility_number_draws(numeric,8);
    check(number_draws&&number_draws->size()==2&&number_draws->front().frame==0&&
          number_draws->front().position[0]==192&&number_draws->back().frame==4,
          "Japanese Draw_count4 zero uses separate person unit, not multiplied stock");
    numeric.kind=N::multi_value;numeric.value=1234;
    number_draws=steam_facility_number_draws(numeric,8);
    check(number_draws&&number_draws->size()==6&&number_draws->front().position[0]==178&&
          number_draws->back().asset==A::number05&&number_draws->back().frame==16&&
          number_draws->back().position[0]==169,
          "Draw_multiValue emits comma digits before multiplication marker at dx-8*digits-9");
    numeric.value=-1;
    check(!steam_facility_number_draws(numeric,8),"catalogue count domain rejects negative inventory");
    numeric.value=0;numeric.position[0]=std::numeric_limits<int>::min();
    check(!steam_facility_number_draws(numeric,8),"multiplication marker overflow rejects whole plan");
    const auto hud=steam_facility_resource(A::number03_hud),level=steam_facility_resource(A::number03);
    check(hud && level && hud->image==102 && hud->sprite==10 && level->sprite==11 &&
          std::string(hud->published_image)=="original/common/number03.png" &&
          std::string(hud->published_sprite)=="original/common/number03.seb" &&
          std::string(level->published_sprite)=="original/common/number04.seb","SEB10/11独立身份不能因同PNG合并");
    const auto hud_seb=dungeon_village_tools::parse_legacy_seb(bytes(assets/hud->published_sprite));
    for(const int frame:{0,9}) {
        const auto &parts=hud_seb.layers.at(0).parts;
        const auto part=std::find_if(parts.begin(),parts.end(),[=](const auto &p){return p.frame==frame;});
        check(part!=parts.end() && part->image_index==102 && part->source_x==8*frame && part->source_y==0 &&
              part->width==8 && part->height==12 && part->offset_x==0 && part->offset_y==0,
              "HUD SEB10实际首末帧为8×12/sourceY0，不能误用SEB11的Y12");
    }
    numeric={N::number,A::number03_hud,12,{100,20},0,2,-1};
    number_draws=steam_facility_number_draws(numeric,8);
    check(number_draws && number_draws->size()==2 && number_draws->at(0).position[0]==92 &&
          number_draws->at(1).position[0]==100 && number_draws->at(0).frame==1 && number_draws->at(1).frame==2,
          "HUD普通数字保留8步宽与原居中锚");
    check(!steam_facility_number_draws(numeric,7),"HUD普通数字拒绝错误字格宽");
    for(const auto kind:{N::money,N::plus_value,N::comma_number}) {
        numeric.kind=kind;
        check(!steam_facility_number_draws(numeric,8),"HUD SEB10只开放已核普通数字，不借单位/Comma资格");
    }
    struct CommaCase {int value,half_width;std::vector<int> frames,x;};
    const std::vector<CommaCase> commas{
        {0,4,{0},{92}},
        {123,12,{1,2,3},{76,84,92}},
        {1234,16,{1,2,10,3,4},{68,76,74,84,92}},
        {1234567,28,{1,2,10,3,4,5,10,6,7},{44,52,50,60,68,76,74,84,92}},
        {-1234,20,{-3,1,2,10,3,4},{60,68,76,74,84,92}},
        {std::numeric_limits<int>::min(),44,{-3,2,1,10,4,7,4,10,8,3,6,10,4,8},
            {12,20,28,26,36,44,52,50,60,68,76,74,84,92}}
    };
    for(const auto asset:{A::number05,A::number11})for(const auto &sample:commas)for(const int anchor:{2,4}) {
        const SteamFacilityNumber request{N::comma_number,asset,sample.value,{100,20},0,anchor,-1};
        const auto draws=steam_facility_number_draws(request,8);
        bool matches=draws && draws->size()==sample.frames.size();
        if(matches)for(std::size_t i=0;i<sample.frames.size();++i)
            matches=matches&&draws->at(i).asset==asset&&draws->at(i).frame==sample.frames[i]&&
                draws->at(i).position==std::array<int,2>{sample.x[i]+(anchor==2?sample.half_width:0),20};
        check(matches,"Comma零/3/4/7位/负数/INT_MIN使用独立原序与2/4锚，无money偏移或单位");
    }
    numeric={N::comma_number,A::number11,1234,{100,20},0,4,-1};
    const auto comma_seb=dungeon_village_tools::parse_legacy_seb(bytes(assets/steam_facility_resource(A::number11)->published_sprite));
    const auto &comma_parts=comma_seb.layers.at(0).parts;
    check(comma_parts.size()==10 && std::none_of(comma_parts.begin(),comma_parts.end(),[](const auto &p){return p.frame==10;}) &&
          steam_facility_number_draws(numeric,8)->at(2).frame==10,
          "SEB20实际缺逗号帧但Comma计划仍保留frame10/原位置，由已核下游空绘制消费");
    for(int fault=0;fault<5;++fault) {
        auto invalid=numeric;int width=8;
        if(fault==0)invalid.anchor=6;
        if(fault==1)invalid.padding=1;
        if(fault==2)width=7;
        if(fault==3)invalid.asset=A::number08;
        if(fault==4)invalid.position[0]=std::numeric_limits<int>::min();
        check(!steam_facility_number_draws(invalid,width),"Comma拒绝位合并锚/额外padding/错误步宽/未核资产/坐标溢出");
    }
    const auto blue=steam_facility_resource(A::number08),orange=steam_facility_resource(A::number05);
    check(blue->image==105&&blue->sprite==15&&orange->image==103&&orange->sprite==12&&
          bytes(assets/blue->published_image)!=bytes(source_root/"common/number08.png")&&
          bytes(assets/orange->published_image)!=bytes(source_root/"common/number05.png"),
          "同号SEB显式绑定Steam差异图片，不alias APK默认common");
    const auto mini=dungeon_village_tools::parse_legacy_seb(bytes(source_root/"common/chara_mini.seb"));
    const std::array<std::array<int,6>,6> oracle{{{2,51,13,2,-7,-1},{21,51,11,2,-6,-1},
        {1,0,15,24,-8,-24},{1,24,15,25,-8,-25},{17,0,19,25,-10,-25},{17,24,19,26,-10,-26}}};
    for(int frame=0;frame<6;++frame) {
        const auto &p=mini.layers.at(0).parts.at(frame);
        check(p.image_index==93&&std::array<int,6>{p.source_x,p.source_y,p.width,p.height,p.offset_x,p.offset_y}==oracle[frame],
              "mini六个实际使用帧裁片/偏移独立源oracle");
    }
    // raw74沿现visuals套件：重点验证语言重载、触摸交错、五行与Owner绑定。
    SteamFacilityDetailSkinInput detail;
    detail.definition=36;detail.mapchip=65;detail.icon=2;detail.name="咖啡厅";
    detail.values={490,21,25,30};detail.limits={1000,50,50};detail.remaining=18;
    detail.effects={{{0,4}}};detail.title_widths=std::array<int,2>{80,80};detail.bottom_width=48;
    using D=SteamFacilityDetailText;using DR=SteamFacilityDetailTextRole;
    const auto detail_labels=[](const SteamFacilityDetailSkinPlan &p,DR role) {
        std::vector<D> result;
        for(const auto &op:p.operations)if(const auto *t=std::get_if<D>(&op))if(t->role==role)result.push_back(*t);
        return result;
    };
    auto details=steam_facility_detail_skin(detail);
    check(details&&details->soft_labels==std::array<int,2>{0,2},"74左空右返回，不增确定软键");
    auto names=detail_labels(*details,DR::name);
    check(names.size()==1&&names[0].value=="咖啡厅"&&names[0].rectangle==std::array<int,4>{41,69,85,11}&&
          names[0].anchor==32&&names[0].font_size==11&&names[0].method==SteamFacilityTextMethod::text_layout,
          "Steam非日文名称限框，第一页不拼来源实例编号");
    const auto buttons=std::count_if(details->operations.begin(),details->operations.end(),[](const auto &v){
        const auto *t=std::get_if<SteamStartupTouch>(&v);return t&&t->component==4;
    });
    check(buttons==1&&detail_labels(*details,DR::bottom)[0].rectangle==std::array<int,4>{88,197,64,16},
          "底部使用道具按实测宽+16，不把200当最终宽");
    // 普通定义预览复用第一页内容，但不能用无实例载荷伪造两页/加成/道具按钮。
    auto preview=detail;preview.definition_preview=true;preview.bottom_width.reset();preview.limits={0,0,0};
    const auto preview_images=[](const SteamFacilityDetailSkinPlan &p,SteamFacilityAsset asset) {
        int count=0;for(const auto &op:p.operations)if(const auto *d=std::get_if<SteamFacilityDraw>(&op))
            if(const auto *image=std::get_if<SteamFacilityImage>(d))if(image->asset==asset)++count;
        return count;
    };
    for(bool japanese:{false,true}) {
        preview.japanese=japanese;const auto plan=steam_facility_detail_skin(preview);
        check(plan&&plan->soft_labels==std::array<int,2>{0,2},"74普通定义预览无需无底钮测宽，左空右返回保持");
        const auto title=detail_labels(*plan,DR::title);const auto name=detail_labels(*plan,DR::name);
        check(title.size()==2&&title[0].argument==0&&title[1].argument==0&&
            name.size()==1&&name[0].value=="咖啡厅"&&name[0].font_size==(japanese?0:11)&&
            detail_labels(*plan,DR::bottom).empty()&&detail_labels(*plan,DR::bonus_title).empty(),
            "74预览纯标题没有/2；保留源两语言名称，不显示实例邻接页或使用道具提示");
        check(std::none_of(plan->operations.begin(),plan->operations.end(),[](const auto &op) {
            return std::holds_alternative<SteamFacilityArrowTouch>(op)||std::holds_alternative<SteamFacilityScrollRequest>(op);
        })&&preview_images(*plan,SteamFacilityAsset::maximum)==0&&preview_images(*plan,SteamFacilityAsset::finger)==0,
            "74预览不画双箭头/实例MAX/手形，不能用0限值补出MAX");
        const auto *confirm=std::get_if<SteamStartupTouch>(&plan->operations.back());
        check(confirm&&confirm->component==2&&confirm->option==2&&!confirm->rectangle&&
            std::count_if(plan->operations.begin(),plan->operations.end(),[](const auto &op){return std::holds_alternative<SteamStartupTouch>(op);})==1,
            "74预览末仅原Touch2 option2，无猜测矩形或购买/使用道具Touch4");
        std::vector<SteamFacilityNumber> numbers;std::vector<SteamFacilityMapchip2> mapchips;
        for(const auto &op:plan->operations)if(const auto *draw=std::get_if<SteamFacilityDraw>(&op)) {
            if(const auto *number=std::get_if<SteamFacilityNumber>(draw))numbers.push_back(*number);
            if(const auto *map=std::get_if<SteamFacilityMapchip2>(draw))mapchips.push_back(*map);
        }
        check(numbers.size()==5&&numbers[0].value==490&&numbers[1].value==1&&numbers[2].value==21&&
            numbers[3].value==25&&numbers[4].value==18&&mapchips.size()==1&&mapchips[0].mapchip==65&&
            mapchips[0].position==std::array<int,2>{72,125}&&mapchips[0].orientation==0&&
            preview_images(*plan,SteamFacilityAsset::number08)==4,
            "74预览保留显式定义价/品质/魅力、共享等级/剩余及四个效果点，不从实例或邻接再计算");
    }
    preview.level=5;const auto max_preview=steam_facility_detail_skin(preview);
    check(max_preview&&preview_images(*max_preview,SteamFacilityAsset::maximum)==1&&
        detail_labels(*max_preview,DR::maximum_level).size()==1&&detail_labels(*max_preview,DR::remaining).empty(),
        "74预览保留共享最高等级标记，区别于被禁用的三属性实例MAX");
    preview.level=1;
    for(int fault=0;fault<4;++fault) {
        auto bad=preview;
        if(fault==0)bad.page=1;
        if(fault==1)bad.bonuses.push_back({71,0,6,2,"槙树",{20,25}});
        if(fault==2)bad.cursor.first=1;
        if(fault==3)bad.title_widths.reset();
        check(!steam_facility_detail_skin(bad),"74定义预览拒第二页/假邻接/孤立游标/缺标题测宽");
    }
    for(const auto &definition:startup_world_rules().facilities)
        if(definition.kind==2||definition.kind==12||definition.detail==1||definition.detail==4||definition.detail==5||definition.detail==6) {
            auto bad=preview;bad.definition=definition.id;bad.mapchip=definition.display_id;
            check(!steam_facility_detail_skin(bad),"74本批明确拒特殊模板，不借普通布局伪造住宅/装备/入住/植物");
        }
    detail.page=1;details=steam_facility_detail_skin(detail);
    check(details&&detail_labels(*details,DR::no_bonus).size()==1&&detail_labels(*details,DR::name).empty()&&
          std::count_if(details->operations.begin(),details->operations.end(),[](const auto &v){return std::holds_alternative<SteamFacilityScrollRequest>(v);})==1,
          "空Y有无奖励文字且仍注册滚动；第二页不复用第一名称及道具按钮");
    for(int i=0;i<7;++i)detail.bonuses.push_back({71,i,6,2,"槙树",{20,i==6?-3:25}});
    const auto wrap=steam_facility_bonus_cursor(7,{0,0},true,false);
    check(wrap&&wrap->selection==6&&wrap->first==2,"七行上循环到末行、追踪末五行");
    const auto double_direction=steam_facility_bonus_cursor(7,{0,0},true,true);
    check(double_direction&&double_direction->selection==0&&double_direction->first==0,"同时上下各执行一次");
    detail.cursor=*wrap;detail.frame=8;details=steam_facility_detail_skin(detail);
    names=detail_labels(*details,DR::bonus_name);
    const auto bonus_values=detail_labels(*details,DR::bonus_value);
    check(names.size()==5&&names.front().value=="槙树3"&&names.back().value=="槙树7"&&
          bonus_values.size()==10&&bonus_values.back().value=="+-3"&&
          bonus_values.front().rectangle==std::array<int,4>{130,99,35,9}&&bonus_values.front().anchor==32&&
          bonus_values.front().method==SteamFacilityTextMethod::text_layout,
          "末五行原序、ordinal+1及signed字面串；植物值是独立TextLayout");
    bool ordered=false;int rows_seen=0;std::size_t scroll_at=details->operations.size();
    for(std::size_t i=0;i<details->operations.size();++i) {
        if(const auto *touch=std::get_if<SteamStartupTouch>(&details->operations[i])) {
            if(touch->component==11) {
                ++rows_seen;
                check(touch->value==(0x20000|(2+rows_seen-1))&&touch->option==0&&
                      touch->rectangle==std::array<int,4>{-1,95+19*(rows_seen-1),231,18},"SLIST保持绝对索引与无marker16注册");
                if(rows_seen==5&&i>0) {
                    const auto *draw=std::get_if<SteamFacilityDraw>(&details->operations[i-1]);
                    ordered=draw&&std::holds_alternative<SteamFacilityImage>(*draw)&&
                        std::get<SteamFacilityImage>(*draw).asset==A::finger;
                }
            }
            check(touch->component!=4,"第二页不注册使用道具按钮");
        }
        if(const auto *scroll=std::get_if<SteamFacilityScrollRequest>(&details->operations[i])) {
            scroll_at=i;
            check(scroll->count==7&&scroll->visible==5&&scroll->rectangle==std::array<int,4>{221,85,3,110},
                  "滚动只读全部七行计数、原组件基矩形，不生成自拟滑块");
        }
    }
    check(rows_seen==5&&ordered&&scroll_at+1<details->operations.size()&&
          std::get<D>(details->operations[scroll_at+1]).role==DR::bottom,
          "选中手形先于行touch，滚动请求后才底部说明");
    // 独立GameView入口oracle，不从页面布局函数反算期望；原短表也显示整根蓝条。
    SteamFacilityScrollRequest scroll_request;
    for(const int count:{0,1,2,5}) {
        scroll_request.count=count;
        const auto bar=steam_facility_scroll_skin(scroll_request,0,true);
        check(bar&&bar->rectangles[0].rect==std::array<int,4>{220,85,5,110}&&
            bar->rectangles[0].rgb==std::array<int,3>{7,5,78}&&
            bar->rectangles[1].rect==std::array<int,4>{220,85,5,111}&&
            bar->rectangles[1].rgb==std::array<int,3>{48,160,255}&&
            bar->thumb_touch==std::array<int,4>{221,85,5,110},
            "空至五行仍绘蓝滑块，短表忽略触摸，基命中区不混用绘图区");
    }
    scroll_request.count=7;
    const auto top_bar=steam_facility_scroll_skin(scroll_request,0);
    const auto bottom_bar=steam_facility_scroll_skin(scroll_request,2,true);
    check(top_bar&&bottom_bar&&top_bar->rectangles[1].rect==std::array<int,4>{220,85,5,79}&&
        bottom_bar->rectangles[1].rect==std::array<int,4>{220,116,5,79}&&
        bottom_bar->rectangles[1].rgb==std::array<int,3>{246,129,0}&&
        bottom_bar->thumb_touch==std::array<int,4>{221,116,5,78},
        "七行顶底原整数比例，首次触摸橙色及注册高度独立");
    check(!steam_facility_scroll_skin(scroll_request,-1)&&!steam_facility_scroll_skin(scroll_request,3),
        "越界滚动显式拒绝，不夹到可用图来掩盖坏输入");
    scroll_request.prefix=0;
    check(!steam_facility_scroll_skin(scroll_request,0),"未知组件请求不冒用raw74滚动皮肤");
    const auto absolute_scroll=steam_facility_bonus_scroll(7,{4,0},SteamFacilityScrollEvent::absolute,2);
    const auto relative_scroll=steam_facility_bonus_scroll(7,{6,2},SteamFacilityScrollEvent::relative,-3);
    const auto unchanged_scroll=steam_facility_bonus_scroll(7,{4,0},SteamFacilityScrollEvent::relative,0);
    const auto short_scroll=steam_facility_bonus_scroll(2,{1,0},SteamFacilityScrollEvent::absolute,10);
    check(absolute_scroll&&absolute_scroll->first==2&&absolute_scroll->selection==6&&
        relative_scroll&&relative_scroll->first==0&&relative_scroll->selection==4&&
        unchanged_scroll&&unchanged_scroll->first==0&&unchanged_scroll->selection==4&&
        short_scroll&&short_scroll->first==0&&short_scroll->selection==1,
        "组件12绝对/相对事件按实际夹后delta同步选择，无变化及短表不移选择");
    check(!steam_facility_bonus_scroll(7,{6,0},SteamFacilityScrollEvent::absolute,1)&&
        !steam_facility_bonus_scroll(7,{4,0},SteamFacilityScrollEvent::relative,std::numeric_limits<int>::max())&&
        !steam_facility_bonus_scroll(7,{4,0},static_cast<SteamFacilityScrollEvent>(9),1),
        "坏当前光标、INT_MAX事件哨兵及未知事件显式拒绝");
    const auto extreme_scroll=steam_facility_bonus_scroll(7,{6,2},SteamFacilityScrollEvent::relative,
        std::numeric_limits<int>::min());
    check(extreme_scroll&&extreme_scroll->first==0&&extreme_scroll->selection==4,
        "极端相对负量不溢出，先夹scroll再使用实际delta");
    for(int fault=0;fault<7;++fault) {
        auto bad=detail;
        if(fault==0)bad.bonuses[0].values.clear(); // 隐藏行也必须核验。
        if(fault==1)bad.title_widths.reset();
        if(fault==2)bad.bottom_width.reset();
        if(fault==3)bad.cursor.selection=7;
        if(fault==4)bad.finger_frame=6;
        if(fault==5)bad.effects={{{0,std::numeric_limits<int>::max()}}};
        if(fault==6)bad.view_y=std::numeric_limits<int>::max();
        const auto diagnostic="74拒绝场景"+std::to_string(fault)+"：隐藏坏载荷/缺测宽/越界选择/输出或坐标超限";
        check(!steam_facility_detail_skin(bad),diagnostic.c_str());
    }
    auto detail_owner=test_support::world_fixture();
    check(open_startup_world_facility_page(detail_owner,4)==StartupWorldRuntimeError::none,"74真实实例入口");
    const auto detail_page=detail_owner.scripts.pages.back().id;
    SteamFacilityDetailSkinOptions detail_options;detail_options.title_widths=detail.title_widths;detail_options.bottom_width=48;
    const auto before_detail=startup_world_state_digest(detail_owner);
    const auto live_plan=steam_facility_detail_skin(detail_owner,detail_page,detail_options);
    check(live_plan.has_value(),"已初始化普通实例由Owner产生第一页");
    for(int repeat=0;repeat<24;++repeat) {
        const auto same=steam_facility_detail_skin(detail_owner,detail_page,detail_options);
        check(same&&same->operations.size()==live_plan->operations.size(),"重复计划规模不累计");
    }
    check(startup_world_state_digest(detail_owner)==before_detail,"绘制不抽随机、不写声音/计数/选择/经济");
    auto incomplete=detail_owner;incomplete.facility_page_neighbours.erase(detail_page);
    check(!steam_facility_detail_skin(incomplete,detail_page,detail_options),"缺初始化Y显式拒绝");
    for(const int value:{-1,2}) {
        auto unsupported=detail_owner;unsupported.scripts.pages.back().legacy_g=value;
        check(!steam_facility_detail_skin(unsupported,detail_page,detail_options),"普通实例只接受源value1=0，不把坏模板当实例皮肤");
    }
    check(act_startup_world_facility_page(detail_owner,detail_page,StartupFacilityPageAction::cancel)==StartupWorldRuntimeError::none&&
          !steam_facility_detail_skin(detail_owner,detail_page,detail_options),"关闭后皮肤引用退休");
    // 85真实详情消费者产生单页74，定义34尚无实例；只检验皮肤桥的接线/引用边界。
    auto preview_owner=test_support::page_fixture(85);preview_owner.rank=5;
    const auto parent_id=preview_owner.scripts.pages.back().id;
    check(initialize_startup_world_commerce_pages(preview_owner),"74预览Owner条件夹具真实初始化85");
    const auto &preview_list=preview_owner.commerce_page_lists.at(parent_id);
    const auto chosen=std::find(preview_list.begin(),preview_list.end(),34);
    check(chosen!=preview_list.end(),"74普通定义34在真实p0冻结商会目录中");
    const int preview_selection=static_cast<int>(chosen-preview_list.begin());
    check(act_startup_world_commerce_page(preview_owner,parent_id,StartupCommerceAction::select,preview_selection)==StartupWorldRuntimeError::none&&
        act_startup_world_commerce_page(preview_owner,parent_id,StartupCommerceAction::inspect)==StartupWorldRuntimeError::none,
        "85真实选择/详情生成74预览，不付点或构造设施实例");
    const auto preview_page=preview_owner.scripts.pages.back().id;
    const auto &preview_source=preview_owner.scripts.pages.back();
    check(preview_source.legacy_page==74&&preview_source.legacy_f==34&&preview_source.legacy_g==1&&
        preview_owner.facility_definition_page_bindings.at(preview_page)==34&&
        !preview_owner.facility_page_bindings.count(preview_page)&&!preview_owner.facility_page_neighbours.count(preview_page)&&
        std::none_of(preview_owner.scene.world.world.facilities.begin(),preview_owner.scene.world.world.facilities.end(),
            [](const auto &f){return f.second.placement.definition_id==34;}),
        "74预览保持共享定义引用，无实例/初始化Y，不能为绘制伪造世界建筑");
    SteamFacilityDetailSkinOptions preview_options;preview_options.title_widths=std::array<int,2>{48,50};
    const auto shared=startup_world_facility_definition_values(preview_owner,34);
    check(shared.has_value(),"74定义共享数值帮助器可读未建设定义");
    const auto before_preview=startup_world_state_digest(preview_owner);
    const auto preview_plan=steam_facility_definition_skin(preview_owner,preview_page,preview_options);
    check(preview_plan.has_value()&&!steam_facility_detail_skin(preview_owner,preview_page,preview_options),
        "74预览严格使用新共享桥，旧实例API仍拒无实例页");
    for(const auto &op:preview_plan->operations)if(const auto *draw=std::get_if<SteamFacilityDraw>(&op))
        if(const auto *number=std::get_if<SteamFacilityNumber>(draw))if(number->parameter>=0)
            check(number->value==shared->definition_attributes[number->parameter],
                "74预览上层只投影共享定义属性，计算oracle继续由经济套件主责");
    const auto same_preview=steam_facility_definition_skin(preview_owner,preview_page,preview_options);
    check(same_preview&&same_preview->operations.size()==preview_plan->operations.size()&&
        startup_world_state_digest(preview_owner)==before_preview,"74预览重复查询规模稳定，无资金/村点/随机/页面或一次性输出修改");
    for(int fault=0;fault<11;++fault) {
        auto bad=preview_owner;
        if(fault==0)bad.page_counters.erase(preview_page);
        if(fault==1)bad.facility_definition_page_bindings.erase(preview_page);
        if(fault==2)for(auto &p:bad.scripts.pages)if(p.id==parent_id)p.legacy_page=83;
        if(fault==3)bad.page_phases.at(preview_page)=1;
        if(fault==4)bad.scripts.pages.back().legacy_g=0;
        if(fault==5)bad.facility_page_bindings[preview_page]=4;
        if(fault==6)bad.facility_page_neighbours[preview_page]={};
        if(fault==7)bad.page_counters.erase(parent_id);
        if(fault==8)bad.page_counters.at(parent_id)=-1;
        if(fault==9)bad.commerce_page_data.at(parent_id)[3]=static_cast<int>(bad.commerce_page_lists.at(parent_id).size());
        if(fault==10)bad.commerce_page_lists.at(parent_id).back()=bad.commerce_page_lists.at(parent_id).front();
        const auto before=startup_world_state_digest(bad);
        check(!steam_facility_definition_skin(bad,preview_page,preview_options)&&startup_world_state_digest(bad)==before,
            "74预览拒自身坏载荷及父85缺/负计数、越界首行或重复目录，不修复坏档");
    }
    for(const auto action:{StartupFacilityPageAction::confirm,StartupFacilityPageAction::cancel}) {
        auto closed=preview_owner;
        check(act_startup_world_facility_page(closed,preview_page,action)==StartupWorldRuntimeError::none&&
            !steam_facility_definition_skin(closed,preview_page,preview_options)&&
            closed.commerce_page_data.at(parent_id)[2]==preview_selection,
            "74预览关闭即不再绘制，父85真实选择保留；业务组合沿pages套件主责");
    }
    for(int id=static_cast<int>(A::facility_icon);id<=static_cast<int>(A::people);++id) {
        const auto resource=steam_facility_resource(static_cast<A>(id));
        check(resource&&std::filesystem::is_regular_file(assets/resource->published_image),"74复用已出版Steam资源，不读Unity/work");
    }
    // 商品目录条件夹具：开放六个定义用于四行窗口，不认证自然获取。
    auto goods=test_support::page_fixture(79);const auto goods_id=goods.scripts.pages.back().id;
    goods.scripts.executing_page.reset();goods.scripts.pages.back().legacy_f=1;goods.scripts.pages.back().lifecycle=0;
    for(auto &entry:goods.catalog)if(entry.first.first==1)entry.second.status=entry.first.second<6?1:0;
    check(initialize_startup_world_facility_catalog_pages(goods),"79真实初始化已有目录载荷");
    SteamHumanOverviewOptions go;go.title_widths=std::array<int,2>{64,64};go.bottom_widths[0]=96;
    auto gp=steam_facility_commodity_skin(goods,goods_id,go);
    check(gp&&gp->title=="商品 1/2"&&gp->soft_labels==std::array<int,2>{7,2},"79双页及详情/返回标签");
    int rows=0,prices=0;bool first_row=false,first_price=false,first_finger=false;
    for(const auto &op:gp->operations) {
        if(const auto *r=std::get_if<SteamHumanEquipmentRow>(&op)) {
            ++rows;first_row|=r->touch.rectangle==std::optional<std::array<int,4>>({1,96,233,16})&&r->margin==std::array<int,4>{0,-20,0,0};
        }
        if(const auto *n=std::get_if<SteamFacilityNumber>(&op)) {
            ++prices;first_price|=n->kind==SteamFacilityNumberKind::money&&n->asset==A::number08&&n->position==std::array<int,2>{211,99};
        }
        if(const auto *i=std::get_if<SteamFacilityImage>(&op))first_finger|=i->asset==A::finger&&i->position==std::array<int,2>{19,106};
    }
    check(rows==4&&prices==4&&first_row&&first_price&&first_finger,"79四行坐标/无marker行触摸/基础售价/手形");
    const auto goods_before=startup_world_state_digest(goods);
    for(int n=0;n<12;++n)check(steam_facility_commodity_skin(goods,goods_id,go)->operations.size()==gp->operations.size(),"商品计划输出不累计");
    check(startup_world_state_digest(goods)==goods_before,"商品绘制不清NEW/收费/抽随机/推进页面");
    check(act_startup_world_facility_catalog_page(goods,goods_id,StartupFacilityCatalogAction::select,5)==StartupWorldRuntimeError::none&&
          act_startup_world_facility_catalog_page(goods,goods_id,StartupFacilityCatalogAction::next_tab)==StartupWorldRuntimeError::none,"目录末项与属性页接线");
    gp=steam_facility_commodity_skin(goods,goods_id,go);rows=0;int columns=0;
    for(const auto &op:gp->operations) {
        rows+=std::holds_alternative<SteamHumanEquipmentRow>(op);
        if(const auto *i=std::get_if<SteamHumanImage>(&op))if(i->asset==SteamHumanAsset::combat_icon) {
            ++columns;check(i->frame==1||i->frame==3,"武器页正攻击/魔法列，不能套防具列");
        }
    }
    check(rows==4&&columns==2&&gp->title=="商品 2/2","末端滚动保持四行与两属性列");
    goods.facility_catalog_page_data.at(goods_id)[2]=5;
    StartupFacilityCatalogInput combined;combined.up=combined.down=combined.left=combined.right=true;
    check(input_startup_world_facility_catalog_page(goods,goods_id,combined)==StartupWorldRuntimeError::none&&
          inspect_startup_world_facility_catalog_page(goods,goods_id)->first_visible==5&&
          inspect_startup_world_facility_catalog_page(goods,goods_id)->selection==5&&
          inspect_startup_world_facility_catalog_page(goods,goods_id)->phase==1,"79双方向先合成，不把合法单行尾窗逐方向改掉");
    const auto input_before=startup_world_state_digest(goods);
    combined.selection=99;
    check(input_startup_world_facility_catalog_page(goods,goods_id,combined)==StartupWorldRuntimeError::invalid_page&&
          startup_world_state_digest(goods)==input_before,"同轮坏选择与方向失败无部分页/NEW修改");
    combined={};combined.scroll=StartupFacilityCatalogInput::Scroll{true,0};
    check(input_startup_world_facility_catalog_page(goods,goods_id,combined)==StartupWorldRuntimeError::none&&
          inspect_startup_world_facility_catalog_page(goods,goods_id)->selection==0,"滚动以实际delta同步选择");
    check(act_startup_world_facility_catalog_page(goods,goods_id,StartupFacilityCatalogAction::select,5)==StartupWorldRuntimeError::none,"恢复详情预选末项");
    auto broken=goods;broken.facility_catalog_page_data.erase(goods_id);
    check(!steam_facility_commodity_skin(broken,goods_id,go),"已初始化缺目录字段显式拒绝");
    check(act_startup_world_facility_catalog_page(goods,goods_id,StartupFacilityCatalogAction::inspect)==StartupWorldRuntimeError::none&&
          initialize_startup_world_facility_catalog_pages(goods),"79信息入口真实创建72");
    const auto child=goods.scripts.pages.back().id;
    const auto goods_detail=steam_facility_commodity_detail_skin(goods,child,go);
    check(goods_detail&&goods_detail->title=="装备信息 6/6"&&!steam_facility_commodity_skin(goods,goods_id,go),"72读取浏览序号且父页覆盖时不冒充顶页");
    broken=goods;broken.facility_catalog_page_parents.erase(child);
    check(!steam_facility_commodity_detail_skin(broken,child,go),"72错父与孤立载荷拒绝");
    auto single=test_support::page_fixture(79);single.scripts.executing_page.reset();single.scripts.pages.back().legacy_f=1;single.scripts.pages.back().lifecycle=0;
    for(auto &entry:single.catalog)if(entry.first.first==1)entry.second.status=entry.first.second==0?1:0;
    const auto single_id=single.scripts.pages.back().id;
    check(initialize_startup_world_facility_catalog_pages(single)&&
          act_startup_world_facility_catalog_page(single,single_id,StartupFacilityCatalogAction::inspect)==StartupWorldRuntimeError::none&&
          initialize_startup_world_facility_catalog_pages(single),"单条商品实际进入信息页");
    const auto single_detail=steam_facility_commodity_detail_skin(single,single.scripts.pages.back().id,go);
    int single_arrows=0;for(const auto &op:single_detail->operations)single_arrows+=std::holds_alternative<SteamFacilityArrowTouch>(op);
    check(single_detail->title=="装备信息 1/1"&&single_arrows==2,"72单条仍有序号/双箭头，不复用73单条隐藏策略");
    check(act_startup_world_facility_catalog_page(goods,child,StartupFacilityCatalogAction::cancel)==StartupWorldRuntimeError::none&&
          !steam_facility_commodity_detail_skin(goods,child,go),"信息页退休后不可继续出图");
    // 设施改良只读皮肤：条件载荷与独立像素坐标，不复制库存/改良状态机。
    const auto effect_parts=[](const SteamFacilityEffectPlan &p,auto tag) {
        using T=decltype(tag);std::vector<T> result;
        for(const auto &op:p.operations)if(const auto *v=std::get_if<T>(&op))result.push_back(*v);
        return result;
    };
    const auto item_icon=startup_world_item_icon_draws(owner,0);
    check(item_icon&&item_icon->size()==2,"纯皮肤夹具复用已证道具底框与前景");
    SteamFacilityItemSkinInput item_input;item_input.title_widths=std::array<int,2>{80,82};
    item_input.cursor={6,6};
    for(int i=0;i<7;++i)item_input.items.push_back({i,1,"测试道具",*item_icon,{49,3,6}});
    auto item_plan=steam_facility_item_skin(item_input);
    check(item_plan.has_value(),"75合法单行尾窗不套74的count减五上限");
    auto item_rows=effect_parts(*item_plan,SteamFacilityEffectTouch{});
    check(item_rows.size()==1&&item_rows[0].touch.component==11&&item_rows[0].touch.value==(0x20000|6)&&
          item_rows[0].touch.option==16&&item_rows[0].touch.rectangle==std::optional<std::array<int,4>>({23,95,191,16})&&
          item_rows[0].margin==std::array<int,4>{29,29,0,0},"75尾窗行marker16与原margin，不借商品行触摸");
    const auto item_scroll=effect_parts(*item_plan,SteamFacilityEffectScroll{});
    check(item_scroll.size()==1&&item_scroll[0].touch.component==12&&item_scroll[0].touch.value==0x40000&&
          item_scroll[0].touch.rectangle==std::optional<std::array<int,4>>({221,85,3,110})&&
          item_scroll[0].arguments==std::array<int,3>{7,5,0x20000}&&
          item_scroll[0].skin.rectangles[1].rect==std::array<int,4>{220,179,5,79}&&
          item_scroll[0].track_touch.component==25&&item_scroll[0].track_touch.value==0&&
          item_scroll[0].track_touch.option==4,"75滚动基矩形/绑定/参数与尾窗几何原序保留");
    const auto net_cursor=steam_facility_item_cursor(7,{6,6},true,true);
    const auto wrapped_cursor=steam_facility_item_cursor(7,{6,6},false,true);
    check(net_cursor&&net_cursor->selection==6&&net_cursor->first==6&&wrapped_cursor&&
          wrapped_cursor->selection==0&&wrapped_cursor->first==0,"75双方向合成不逐次修窗，跨尾后回零");
    for(const auto cursor:std::array<SteamFacilityBonusCursor,4>{{{-1,0},{7,0},{4,5},{6,7}}}) {
        auto bad=item_input;bad.cursor=cursor;
        check(!steam_facility_item_skin(bad),"75负/越界/选项不在视窗的first显式拒绝");
    }
    auto item_texts=effect_parts(*item_plan,SteamFacilityEffectText{});
    check(std::any_of(item_texts.begin(),item_texts.end(),[](const auto &t){return t.value=="剩余"&&
          t.layout&&t.rectangle==std::array<int,4>{185,69,40,12}&&t.anchor==32;}),"Steam75剩余使用TextLayout而非APK重载");
    auto effect_icons=effect_parts(*item_plan,SteamFacilityImage{});
    std::vector<std::array<int,2>> effect_positions;
    for(const auto &im:effect_icons)if(im.asset==A::number08&&im.frame==14)effect_positions.push_back(im.position);
    check(effect_positions==std::vector<std::array<int,2>>{{60,200},{124,200},{132,200},{188,200},{196,200},{204,200}},
          "75原k分档1/2/3，SEB15加号间距8，不使用乘算后的实际增量");
    item_input.arrow_count=1;item_input.scroll_first_touch=true;
    item_plan=steam_facility_item_skin(item_input);item_rows=effect_parts(*item_plan,SteamFacilityEffectTouch{});
    check(item_rows.size()==3&&item_rows[1].touch.component==23&&item_rows[1].touch.value==17&&
          item_rows[2].touch.value==19&&item_rows[1].touch.image_draw&&item_rows[2].touch.image_draw&&
          item_rows[1].margin==std::array<int,4>{40,40,45,0}&&
          effect_parts(*item_plan,SteamFacilityEffectScroll{})[0].skin.rectangles[1].rgb==std::array<int,3>{246,129,0},
          "75箭头注册引用实际绘图节点，触摸长目录滑块变橙");
    for(const auto sample:std::array<std::array<int,2>,5>{{{0,0},{49,1},{50,2},{99,2},{100,3}}}) {
        auto threshold=item_input;threshold.arrow_count=0;threshold.items[6].improvements={sample[0],0,0};
        const auto plan=steam_facility_item_skin(threshold);int marks=0;
        for(const auto &im:effect_parts(*plan,SteamFacilityImage{}))marks+=im.asset==A::number08&&im.frame==14;
        check(marks==sample[1],"75价格0/49/50/99/100边界");
    }
    SteamFacilityImprovementSkinInput improvement;
    improvement.mapchip=0;improvement.profession=0;improvement.sex=0;improvement.item_icon=*item_icon;
    improvement.attributes={{{100,110,10},{10,11,1},{20,18,-2}}};improvement.limits={110,20,40};
    improvement.title_widths=std::array<int,2>{80,82};
    for(const auto sample:std::array<std::array<int,4>,5>{{{14,18,190,111},{15,26,190,112},
          {35,2,107,95},{36,2,-8,-16},{37,2,-8,-16}}}) {
        improvement.frame=sample[0];const auto plan=steam_facility_improvement_skin(improvement);
        check(plan.has_value(),"76投掷时段纯计划有效");
        const auto actors=effect_parts(*plan,SteamHumanPortrait{});
        const auto icons=effect_parts(*plan,SteamHumanImage{});
        check(actors.size()==1&&actors[0].position==std::array<int,2>{198,144}&&
              actors[0].actor.body.sprite==sample[1]&&actors[0].actor.body.frame==0&&
              !actors[0].actor.weapon&&icons.size()==2&&icons[1].asset==SteamHumanAsset::item_icon&&
              icons[1].position==std::array<int,2>{sample[2],sample[3]},"76动作8/10/0及投物边界，clip外请求不延长飞行");
        std::array<int,3> old{};
        for(const auto &n:effect_parts(*plan,SteamFacilityNumber{}))
            if(n.parameter>=0&&n.kind!=N::plus_value)old[n.parameter]=n.value;
        check(old==std::array<int,3>{100,10,20},"76三行始终显示冻结旧值，不提前显示77结果");
    }
    for(const auto sample:std::array<std::array<int,2>,6>{{{37,-1},{38,0},{41,0},{42,1},{46,2},{48,2}}}) {
        improvement.frame=sample[0];const auto plan=steam_facility_improvement_skin(improvement);
        const auto flashes=effect_parts(*plan,SteamFacilityEffectImage{});
        check(sample[1]<0?flashes.empty():flashes.size()==1&&flashes[0].asset==SteamFacilityEffectAsset::throw_flash&&
              flashes[0].position==std::array<int,2>{120,109}&&flashes[0].frame==sample[1],"76到达特效38起、固定20槽锚点与四计数帧");
        if(sample[1]>=0) {
            std::size_t flash=plan->operations.size(),person=flash;
            for(std::size_t i=0;i<plan->operations.size();++i) {
                if(std::holds_alternative<SteamFacilityEffectImage>(plan->operations[i]))flash=i;
                if(std::holds_alternative<SteamHumanPortrait>(plan->operations[i]))person=i;
            }
            check(flash<person,"76到达特效先于人物/投物clip，不能按图类型重排");
        }
    }
    improvement.frame=49;check(!steam_facility_improvement_skin(improvement),"76达到49后应换77，拒绝伪造76尾帧");
    improvement.raw_page=77;
    for(const auto response:std::array<std::array<int,2>,4>{{{0,1},{1,2},{2,0},{-1,3}}})for(int frame:{0,6}) {
        improvement.response=response[0];improvement.frame=frame;
        const auto plan=steam_facility_improvement_skin(improvement);
        const auto reaction=effect_parts(*plan,SteamFacilityEffectImage{});
        check(reaction.size()==1&&reaction[0].asset==SteamFacilityEffectAsset::response&&reaction[0].frame==response[1]&&
              reaction[0].position==std::array<int,2>{120,frame==0?166:149},"77四响应映射与0..6滑入范围");
        std::size_t at=0;while(at<plan->operations.size()&&!std::holds_alternative<SteamFacilityEffectImage>(plan->operations[at]))++at;
        check(at>0&&at+2<plan->operations.size()&&
              std::get<SteamFacilityClip>(plan->operations[at-1]).rectangle==std::array<int,4>{85,132,70,17}&&
              std::get<SteamFacilityClip>(plan->operations[at+1]).kind==SteamFacilityClipKind::pop&&
              std::holds_alternative<SteamFacilityEffectText>(plan->operations[at+2]),"77反应条在clip内且先于三属性行");
    }
    improvement.response=0;
    for(const auto sample:std::array<std::array<int,4>,4>{{{33,100,10,20},{34,101,10,20},{48,109,10,20},{49,110,11,18}}}) {
        improvement.frame=sample[0];const auto plan=steam_facility_improvement_skin(improvement);
        std::array<int,3> shown{};for(const auto &n:effect_parts(*plan,SteamFacilityNumber{}))
            if(n.parameter>=0&&n.kind!=N::plus_value)shown[n.parameter]=n.value;
        check(shown==std::array<int,3>{sample[1],sample[2],sample[3]},"77沿同Draw_tenantStrength计数转换，小差与下降不伪作线性");
        const auto actors=effect_parts(*plan,SteamHumanPortrait{});
        check(actors.size()==1&&actors[0].actor.body.sprite==2&&actors[0].actor.body.frame==0&&
              actors[0].position==std::array<int,2>{198,144},"77定义0人物固定动作0无武器，不借升级双mini");
    }
    improvement.frame=55;const auto final_effect=steam_facility_improvement_skin(improvement);
    effect_icons=effect_parts(*final_effect,SteamFacilityImage{});
    check(std::any_of(effect_icons.begin(),effect_icons.end(),[](const auto &i){return i.asset==A::arrow&&
          i.position==std::array<int,2>{212,204};}),"77满55继续箭头按归零段计数显示");
    auto invalid_effect=improvement;invalid_effect.response=3;check(!steam_facility_improvement_skin(invalid_effect),"77未知响应不套正常图签");
    invalid_effect=improvement;invalid_effect.attributes[0][2]=11;check(!steam_facility_improvement_skin(invalid_effect),"77冻结前后差坏载荷拒绝");
    const auto reaction_resource=steam_facility_effect_resource(SteamFacilityEffectAsset::response);
    check(reaction_resource&&reaction_resource->image==136&&reaction_resource->sprite==92&&
          bytes(assets/reaction_resource->published_image)!=bytes(assets/"original/common/eff_tenantStren.png"),
          "77使用已出版Steam根图，不能把APK同名图当像素别名");
    const auto response_seb=dungeon_village_tools::parse_legacy_seb(bytes(assets/reaction_resource->published_sprite));
    check(response_seb.frame_count==4&&response_seb.layers.size()==1&&response_seb.layers[0].parts.size()==4&&
          std::all_of(response_seb.layers[0].parts.begin(),response_seb.layers[0].parts.end(),[](const auto &p){return p.image_index==136;}),
          "77反应图SEB92四帧绑定136，不从PNG宽度猜帧");
    const auto flash_resource=steam_facility_effect_resource(SteamFacilityEffectAsset::throw_flash);
    const auto flash_seb=dungeon_village_tools::parse_legacy_seb(bytes(assets/flash_resource->published_sprite));
    check(flash_resource->image==6&&flash_resource->sprite==6&&flash_seb.frame_count==3&&
          flash_seb.layers.size()==1&&std::all_of(flash_seb.layers[0].parts.begin(),flash_seb.layers[0].parts.end(),
          [](const auto &p){return p.image_index==6;}),"76到达特效SEB6三帧绑定effect6");
    SteamFacilityReputationInput reputation;reputation.definition=33;reputation.mapchip=44;reputation.name="设施条件";
    reputation.participants={{0,0},{1,1},{2,2}};reputation.title_widths=std::array<int,2>{84,84};
    for(const auto sample:std::array<std::array<int,2>,4>{{{5,0},{6,1},{39,46},{40,48}}}) {
        reputation.frame=sample[0];const auto p=steam_facility_reputation_skin(reputation);
        check(p.has_value(),"82有限计数文本展开计划");
        bool clip=false,border=false,mini=false;for(const auto &op:p->operations) {
            if(const auto *c=std::get_if<SteamFacilityClip>(&op))clip|=c->kind==SteamFacilityClipKind::push_intersect&&c->rectangle==std::array<int,4>{0,158,240,sample[1]};
            if(const auto *f=std::get_if<SteamStartupFrameDraw>(&op))if(const auto *r=std::get_if<StartupSkinRect>(f))
                border|=r->outline&&r->rect==std::array<int,4>{30,66,180,82};
            if(const auto *image=std::get_if<SteamFacilityImage>(&op))
                mini|=image->asset==A::mini_background&&image->position==std::array<int,2>{77,71}&&
                      image->crop==std::optional<std::array<int,4>>{{0,0,86,70}};
        }
        check(clip&&border,"82源clip5/40边界，灰框是内描边不遮背景");
        check(mini,"82 event16按原86x70整张PNG消费，不向无SEB资源请求帧");
    }
    for(const auto sample:std::array<std::array<int,3>,5>{{{9,1,143},{10,9,143},{17,9,135},{25,9,143},{26,1,143}}}) {
        reputation.frame2=sample[0];const auto p=steam_facility_reputation_skin(reputation);
        int bodies=0;for(const auto &op:p->operations)if(const auto *h=std::get_if<SteamHumanPortrait>(&op)) {
            check(h->actor.body.sprite==sample[1]&&h->actor.body.frame==0&&h->position[1]==sample[2]&&
                  h->actor.shadow&&h->position[1]+h->actor.shadow->offset[1]==143&&!h->actor.weapon,
                  "82独立26槽身体跳起，阴影固定脚底而非随身体跳");++bodies;
        }
        check(bodies==3,"82只按冻结三人物原序，不造实体或抽随机");
    }
    reputation.phase=1;reputation.increase_suffix_width=24;
    for(const auto sample:std::array<std::array<int,2>,5>{{{40,1},{54,1},{55,0},{64,0},{65,1}}}) {
        reputation.frame=sample[0];const auto p=steam_facility_reputation_skin(reputation);int arrows=0;bool number=false,suffix=false;
        for(const auto &op:p->operations) {
            if(const auto *i=std::get_if<SteamHumanImage>(&op))arrows+=i->asset==SteamHumanAsset::continue_cursor;
            if(const auto *n=std::get_if<SteamFacilityNumber>(&op))number|=n->asset==A::number09&&n->value==20&&n->position==std::array<int,2>{97,188}&&n->anchor==1;
            if(const auto *t=std::get_if<SteamHumanOverviewText>(&op))suffix|=t->value=="上升!"&&t->rectangle[0]==119&&t->rectangle[1]==188;
        }
        check(arrows==sample[1]&&number&&suffix,"82箭头25槽及Steam数字宽20+2+实测后缀居中");
    }
    auto bad_reputation=reputation;bad_reputation.participants.push_back({3,3});
    check(!steam_facility_reputation_skin(bad_reputation),"82第四人明确拒绝，不截取或补示例");
    bad_reputation=reputation;bad_reputation.mapchip=34;
    check(!steam_facility_reputation_skin(bad_reputation),"82定义/地图块不一致拒绝");
    bad_reputation=reputation;bad_reputation.increase_suffix_width.reset();
    check(!steam_facility_reputation_skin(bad_reputation),"82第二段必须真实后缀测宽");
    return check.count;
}
