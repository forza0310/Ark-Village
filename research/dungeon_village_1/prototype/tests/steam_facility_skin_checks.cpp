#include "dungeon_village_prototype/steam_facility_skin.hpp"
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
}
// 挂既有visuals套件；只验页面只读计划和资源绑定，不复制升级/声音/Owner状态机。
int check_steam_facility_skin(const std::filesystem::path &source_root) {
    Checks check;
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
    return check.count;
}
