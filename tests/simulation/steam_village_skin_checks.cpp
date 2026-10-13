#include "ark/simulation/presentation/steam_village_activity_skin.hpp"
#include "ark/simulation/village/startup_world_village_activity.hpp"
#include "ark/simulation/persistence/startup_world_persistence.hpp"
#include "support/world_fixture.hpp"
#include "ark/assets/sprite.hpp"
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
    void operator()(bool ok,const char *why) {++count;if(!ok)throw std::runtime_error(std::string("Steam村办皮肤：")+why);}
};
template<class T> std::vector<T> parts(const SteamVillageActivityPlan &p) {
    std::vector<T> result;for(const auto &d:p.operations)if(const auto *v=std::get_if<T>(&d))result.push_back(*v);return result;
}
SteamVillageActivitySkinInput input(int raw) {
    // 定向显示夹具，选择/点数/计数不表示自然经营或实际开展过活动。
    SteamVillageActivitySkinInput in;in.raw=raw;in.points=100;in.quarter_remaining=3;
    in.options.title_widths=std::array<int,2>{60,62};
    in.options.description_width=220;in.options.button_widths={20,100};
    in.options.increase_suffix_width=36;in.options.attribute_widths={30,24,18};
    in.activity=SteamVillageActivityDefinition{23,1,0,0,0,50,5,"活动条件","详情","说明",0,true};
    for(int i=0;i<6;++i) {auto d=*in.activity;d.identity=i;d.cost=100+i;in.activities.push_back(d);
        in.humans.push_back({i,0,0,25+i,20,"人物条件"});}
    in.display_humans={in.humans[0],in.humans[1]};return in;
}
std::vector<std::uint8_t> bytes(const std::filesystem::path &p) {
    std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("村办源资源缺失："+p.string());
    return {std::istreambuf_iterator<char>(f),{}};
}
}
// 现visuals套件主责绘制顺序/热区/资源和只读桥，不复制村办业务条件矩阵。
int check_steam_village_activity_skins(const std::filesystem::path &root) {
    Checks check;
    for(auto a:{SteamVillageActivityAsset::season,SteamVillageActivityAsset::activity_icon,
                SteamVillageActivityAsset::activity_picture,SteamVillageActivityAsset::picture_background}) {
        const auto r=steam_village_activity_resource(a);
        check(r&&std::filesystem::exists(root/std::filesystem::path(r->published_image).lexically_relative("original")),"四资源映射均有正式出版文件");
        if(r->published_sprite) {const auto seb=ark::assets::parse_legacy_seb(bytes(root/std::filesystem::path(r->published_sprite).lexically_relative("original")));
            check(seb.frame_count==4,"季节SEB24四帧，不按PNG猜帧");}
    }
    auto in=input(51);in.selection=5;in.first=5;
    auto p=steam_village_activity_skin(in);check(p.has_value(),"51允许合法尾窗只剩一行");
    auto rows=parts<SteamHumanEquipmentRow>(*p);
    check(rows.size()==1&&rows[0].touch.component==11&&rows[0].touch.value==(0x20000|5)&&
        rows[0].touch.rectangle==std::optional<std::array<int,4>>({3,95,231,16})&&
        rows[0].margin==std::array<int,4>{0,-20,0,0},"51行绝对索引、宽触摸和Steam独立margin");
    auto n=parts<SteamFacilityNumber>(*p);
    check(n.size()==2&&n[1].asset==SteamFacilityAsset::number12&&n[1].value==105,"点数不足价格用Steam number12");
    auto bars=parts<SteamHumanCatalogueScroll>(*p);
    check(bars.size()==1&&bars[0].touch.rectangle==std::optional<std::array<int,4>>({221,85,3,111})&&
        bars[0].arguments==std::array<int,3>{6,5,0x20000},"51/54原111高滑条保留原参数");
    auto t=parts<SteamVillageActivityText>(*p);
    check(std::any_of(t.begin(),t.end(),[](const auto &v){return v.rich&&v.font_size==11&&
        v.value=="冒险者的<co=0065FF>满足度</co>上升";}),"仅原DrawString2属性词指定蓝色");
    in.selection=4;check(!steam_village_activity_skin(in),"选择不在当前窗显式拒绝");
    in=input(51);in.activities[1].identity=in.activities[0].identity;
    check(!steam_village_activity_skin(in),"目录重复身份拒绝");
    in=input(51);in.activities[0].picture=16;check(!steam_village_activity_skin(in),"图片裁片超出正式160×220画集拒绝");
    in=input(51);in.activities[0].icon=7;check(!steam_village_activity_skin(in),"图标裁片超出正式112×16画集拒绝");
    in=input(51);in.options.title_widths.reset();check(!steam_village_activity_skin(in),"窗标题缺实际测宽拒绝");
    in=input(51);in.options.thai=true;p=steam_village_activity_skin(in);
    n=parts<SteamFacilityNumber>(*p);check(n[0].position==std::array<int,2>{129,68},"Steam th分支季度数位置独立于其它非日文");
    in.options.thai=false;p=steam_village_activity_skin(in);n=parts<SteamFacilityNumber>(*p);
    check(n[0].position==std::array<int,2>{100,68},"其它非日文季度数位置");
    in=input(52);p=steam_village_activity_skin(in);check(p.has_value(),"52完整确认计划");
    auto touch=parts<SteamStartupTouch>(*p);
    check(touch.size()==2&&touch[0].component==3&&touch[0].rectangle==std::optional<std::array<int,4>>({42,166,76,56})&&
        touch[1].rectangle==std::optional<std::array<int,4>>({102,166,116,56}),"BTMT六行margin与clamp60决定实际触摸，不复用橙底");
    t=parts<SteamVillageActivityText>(*p);
    check(t[0].layout&&t[0].rectangle==std::array<int,4>{17,162,206,16}&&t[0].anchor==34,
        "52说明真实测宽夹190再加16，沿TextLayout");
    const auto pictures=parts<SteamVillageActivityImage>(*p);
    check(pictures.size()==2&&pictures[1].crop==std::optional<std::array<int,4>>({40,0,40,55}),"52展示图片原40×55裁片");
    in.options.button_widths[1].reset();check(!steam_village_activity_skin(in),"按钮缺真实测宽拒绝");
    in=input(53);in.counter=60;p=steam_village_activity_skin(in);check(p.has_value(),"53进度完成而结果尚未揭示");
    check(parts<SteamFacilityClip>(*p).empty(),"53 counter60仍是进度分支");
    in.counter=70;p=steam_village_activity_skin(in);
    check(parts<SteamFacilityClip>(*p).front().rectangle==std::array<int,4>{0,158,240,0},"53恰70结果mask从0开始");
    in.counter=75;p=steam_village_activity_skin(in);auto sprites=parts<SteamFacilityImage>(*p);
    check(sprites.size()==4&&sprites[0].frame==1&&sprites[1].frame==5&&sprites[1].position==std::array<int,2>{48,136}&&
        sprites[3].frame==4,"counter75局部跳跃和左右type5/6帧分别映射");
    for(int counter:{119,120,140,150}) {
        in.counter=counter;p=steam_village_activity_skin(in);auto images=parts<SteamHumanImage>(*p);
        check(images.size()==static_cast<std::size_t>(counter>=120&&counter%25<15),"53继续箭头遵从120与25/15闪烁");
        if(!images.empty())check(images[0].position==std::array<int,2>{212,204},"共享继续箭头真实锚点");
    }
    for(int kind:{0,1,2,3,5,6}) {in=input(53);in.counter=110;in.activity->kind=kind;
        check(steam_village_activity_skin(in).has_value(),"已接六种活动结果均有纯计划");}
    in.activity->kind=4;check(!steam_village_activity_skin(in),"未定义类型4不能假装演出成功");
    in=input(53);in.activity->kind=1;in.counter=110;in.options.original_language=true;
    in.options.attribute_widths[1].reset();check(!steam_village_activity_skin(in),"原语言属性组合缺测宽拒绝");
    in.options.original_language=false;check(!steam_village_activity_skin(in),"53所有语言仍组属性句，不误沿51非原语言detail分支");
    in=input(51);in.activities[0].kind=1;in.options.attribute_widths[1].reset();
    check(steam_village_activity_skin(in).has_value(),"51非原语言沿活动detail，不依赖原语言测宽");
    in=input(54);p=steam_village_activity_skin(in);check(p.has_value(),"54冻结结果列表");
    touch=parts<SteamStartupTouch>(*p);
    check(touch.size()==6&&touch[0].rectangle==std::optional<std::array<int,4>>({3,94,231,18})&&
        parts<SteamHumanEquipmentRow>(*p).empty(),"54原无TouchOption行不沿51margin");
    n=parts<SteamFacilityNumber>(*p);
    check(n.size()==10&&n[0].kind==SteamFacilityNumberKind::plus_value&&n[0].value==5&&n[1].value==25,
        "54读取冻结previous差值与当前值，不使用活动magnitude");
    in.humans[0].current=std::numeric_limits<int>::max();in.humans[0].previous=-1;
    check(!steam_village_activity_skin(in),"结果差值int32超界拒绝");
    auto owner=test_support::page_fixture(51);const auto id=owner.scripts.pages.back().id;
    owner.scripts.event_calls[100]=1;SteamVillageActivityOptions o;o.title_widths=std::array<int,2>{60,62};
    check(!steam_village_activity_skin(owner,id,o),"未初始化Owner绘制拒绝且不代执行Init");
    check(initialize_startup_world_village_activity_pages(owner),"使用现真实Owner初始化显示载荷");
    p=steam_village_activity_skin(owner,id,o);check(p.has_value(),"真实初始化51到纯计划只读桥");
    const auto digest=startup_world_state_digest(owner);
    const auto size=p->operations.size();
    for(int i=0;i<8;++i) {const auto again=steam_village_activity_skin(owner,id,o);
        check(again&&again->operations.size()==size&&startup_world_state_digest(owner)==digest,
            "重复读取规模稳定，不修改世界、页面、随机、NEW或声音");}
    auto broken=owner;broken.activity_page_scroll.erase(id);
    check(!steam_village_activity_skin(broken,id,o),"已初始化缺首行显式拒绝");
    broken=owner;broken.activity_page_display_humans.erase(id);
    check(!steam_village_activity_skin(broken,id,o),"已初始化缺角色引用显式拒绝");
    auto result=test_support::page_fixture(54);const auto result_id=result.scripts.pages.back().id;
    const auto activity=std::find_if(result.rules->activities.begin(),result.rules->activities.end(),
        [](const auto &d){return d.parameters[2]==0;});
    check(activity!=result.rules->activities.end(),"结果夹具绑定真实满足度活动");
    check(install_startup_world_main_character(result,{"研究姓名",1,true}),"沿已确认唯一Owner安装主角名字/性别覆盖");
    result.activity_page_bindings[result_id]=activity->identity;
    result.activity_page_display_humans[result_id]={0,0};result.human_presence.at(0)=1;
    check(initialize_startup_world_village_activity_pages(result),"结果夹具从真实Init冻结人物名单");
    p=steam_village_activity_skin(result,result_id,o);check(p.has_value(),"54只读Owner覆盖姓名及性别");
    t=parts<SteamVillageActivityText>(*p);const auto actors=parts<SteamHumanPortrait>(*p);
    const auto expected=startup_title_actor_skin(result.scene.world.world.ai.growth.at(0).definition.current_profession,1,-1,0,1,false);
    check(std::any_of(t.begin(),t.end(),[](const auto &v){return v.value=="研究姓名";})&&
        expected&&!actors.empty()&&actors[0].actor.body.image==expected->body.image,
        "结果不回退冻结原表默认姓名/性别");
    return check.count;
}
