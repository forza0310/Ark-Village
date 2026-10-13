#include "ark/simulation/presentation/steam_human_skin.hpp"
#include "ark/simulation/world/startup.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include "ark/simulation/persistence/startup_world_persistence.hpp"
#include "support/world_fixture.hpp"
#include "ark/assets/sprite.hpp"
#include <raylib.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

using namespace ark::simulation;
namespace {
struct Checks {
    int count{};
    void operator()(bool ok,const char *why) {
        ++count;if(!ok)throw std::runtime_error(std::string("Steam人物皮肤：")+why);
    }
};
template<class T> std::vector<T> parts(const SteamHumanSkinPlan &plan) {
    std::vector<T> result;
    for(const auto &draw:plan.draws)if(const auto *value=std::get_if<T>(&draw))result.push_back(*value);
    return result;
}
template<class T> std::vector<T> parts(const SteamHumanOverviewPlan &plan) {
    std::vector<T> result;
    for(const auto &op:plan.operations)if(const auto *value=std::get_if<T>(&op))result.push_back(*value);
    return result;
}
std::vector<std::uint8_t> bytes(const std::filesystem::path &path) {
    std::ifstream in(path,std::ios::binary);
    if(!in)throw std::runtime_error("人物素材缺失："+path.string());
    return {std::istreambuf_iterator<char>(in),{}};
}
void power_checks(Checks &check) {
    const auto plan=steam_human_power_skin({75,121},1,0,SteamHumanHpInput{800,309,1000});
    check(plan.has_value(),"合法HP图元");
    const auto r=parts<StartupSkinRect>(*plan);
    const auto images=parts<SteamHumanImage>(*plan);
    check(r.size()==4 && r[0].rect==std::array<int,4>{64,95,22,6} && r[0].outline &&
          r[1].rect==std::array<int,4>{65,96,20,4} && r[1].outline &&
          r[2].rect==std::array<int,4>{66,97,14,2} && !r[2].outline &&
          r[3].rect==std::array<int,4>{80,97,4,2} && !r[3].outline,
          "NOW控制长度，DrawRect加1为半开边框，FillRect保留18总宽");
    check(r[0].rgb==std::array<int,3>{246,246,246} && r[1].rgb==std::array<int,3>{39,53,74} &&
          r[2].rgb==std::array<int,3>{83,255,0} && r[3].rgb==std::array<int,3>{68,100,104},
          "血条四层原颜色与顺序");
    check(images.size()==1 && images[0].asset==SteamHumanAsset::pinch && images[0].frame==0 &&
          images[0].position==std::array<int,2>{61,93},"AFTER整数30.9%仍触发危险帧0");
    check(parts<SteamHumanImage>(*steam_human_power_skin({75,121},1,0,SteamHumanHpInput{0,310,1000})).empty(),
          "NOW为0但AFTER31%不触发危险");
    const auto right=parts<SteamHumanImage>(*steam_human_power_skin({98,121},2,5,SteamHumanHpInput{80,30,100}));
    check(right.size()==1 && right[0].position==std::array<int,2>{103,93},"direction2原偏移5/-28");
    check(parts<SteamHumanImage>(*steam_human_power_skin({75,121},1,6,SteamHumanHpInput{80,30,100})).empty(),
          "12槽闪烁后半段不绘危险标");
    check(steam_human_power_skin({75,121},1,0,{})->draws.empty(),"无实例不补满HP或危险图标");
    for(const auto sample:std::array<std::array<int,2>,4>{{{{-1,0}},{{0,0}},{{100,18}},{{101,18}}}}) {
        const auto bar=parts<StartupSkinRect>(*steam_human_power_skin({75,121},1,6,SteamHumanHpInput{sample[0],0,100}));
        check(bar[2].rect[2]==sample[1],"越界HP按原端点夹限，不修改HP槽");
    }
    check(steam_human_power_skin({75,121},1,6,SteamHumanHpInput{std::numeric_limits<int>::max(),
              std::numeric_limits<int>::max(),100}).has_value(),"端点夹限和隐藏闪烁不进行无关溢出乘法");
    check(!steam_human_power_skin({75,121},1,-1,{}) &&
          !steam_human_power_skin({75,121},4,0,{}) &&
          !steam_human_power_skin({75,121},1,0,SteamHumanHpInput{0,0,0}) &&
          !steam_human_power_skin({std::numeric_limits<int>::min(),121},1,0,SteamHumanHpInput{0,0,100}),
          "坏方向/计数/最大HP/坐标显式拒绝");
}
void medal_bubble_checks(Checks &check) {
    check(steam_human_medal_skin(0)->draws.empty(),"0枚不绘");
    const auto four=parts<SteamHumanImage>(*steam_human_medal_skin(4));
    check(four.size()==4,"1至4只绘重复奖章");
    for(int i=0;i<4;++i)
        check(four[i].position==std::array<int,2>{103-10*i,70} &&
              four[i].crop==std::optional<std::array<int,4>>({14,0,14,14}),"mode6/index1向左排列，原14×14裁片");
    const auto five=parts<SteamHumanImage>(*steam_human_medal_skin(5));
    check(five.size()==3 && five[0].position==std::array<int,2>{83,70} &&
          five[1].asset==SteamHumanAsset::number05 && five[1].frame==16 &&
          five[1].position==std::array<int,2>{95,74} && five[2].frame==5 &&
          five[2].position==std::array<int,2>{105,74},"5枚切换乘号SEB16及右对齐单数字");
    const auto many=parts<SteamHumanImage>(*steam_human_medal_skin(123));
    check(many.size()==5 && many[0].position==std::array<int,2>{67,70} &&
          many[1].position==std::array<int,2>{79,74} && many[1].frame==16 &&
          many[2].position==std::array<int,2>{89,74} && many[2].frame==1 &&
          many[3].position==std::array<int,2>{97,74} && many[3].frame==2 &&
          many[4].position==std::array<int,2>{105,74} && many[4].frame==3,
          "多位数字8步宽，奖章宽14不参与数字宽度");
    const auto small=steam_human_down_bubble_skin(60),large=steam_human_down_bubble_skin(61);
    const auto a=parts<SteamHumanImage>(*small),b=parts<SteamHumanImage>(*large);
    check(a.size()==3 && a[0].crop==std::optional<std::array<int,4>>({6,0,60,21}) &&
          a[0].position==std::array<int,2>{45,79} && a[1].frame==0 && a[1].position==std::array<int,2>{39,79} &&
          a[2].frame==1 && a[2].position==std::array<int,2>{105,79},"W60保留中段尾部与两帽");
    check(b.size()==6 && b[0].crop==std::optional<std::array<int,4>>({6,0,27,21}) &&
          b[1].position==std::array<int,2>{72,79} && b[2].crop==std::optional<std::array<int,4>>({6,0,7,21}) &&
          b[2].position==std::array<int,2>{99,79} && b[3].crop==std::optional<std::array<int,4>>({33,0,5,21}) &&
          b[3].position==std::array<int,2>{73,79} && b[4].frame==0 && b[5].frame==1,
          "W61铺27/27/7后只补一次中心尾部，再绘端帽");
    const auto text=parts<SteamHumanText>(*large);
    check(text.size()==1 && text[0].position==std::array<int,2>{75,82} && text[0].anchor==2 &&
          text[0].rgb==std::array<int,3>{0,0,0},"倒下文字原中心锚及黑色，不改字体字号");
    check(!steam_human_medal_skin(-1) && !steam_human_down_bubble_skin(-1) &&
          !steam_human_down_bubble_skin(4097),"负奖章/宽度与维护输出预算拒绝");
}
std::filesystem::path packaged(const std::filesystem::path &root,const char *published) {
    const std::filesystem::path file(published);
    const auto relative=file.generic_string();
    if(relative.rfind("original/",0)==0)return root/file.lexically_relative("original");
    if(relative=="steam-build-common/original/common/number05.png" ||
       relative=="steam-facility-common/original/common/number08.png" ||
       relative=="steam-facility-common/original/common/icon_param00.png" ||
       relative=="steam-human-common/original/common/wnd_ato.png")
        return root/"steam_common"/file.filename();
    throw std::runtime_error("Unsupported published Steam fixture path: "+relative);
}
void resources(Checks &check,const std::filesystem::path &source_root) {
    const auto assets=source_root;
    for(const auto asset:{SteamHumanAsset::medal,SteamHumanAsset::pinch,SteamHumanAsset::bubble,SteamHumanAsset::number05,
        SteamHumanAsset::portrait_background,SteamHumanAsset::home_background,SteamHumanAsset::experience_bar,
        SteamHumanAsset::experience_label,SteamHumanAsset::experience_remaining,SteamHumanAsset::smile,
        SteamHumanAsset::combat_icon,SteamHumanAsset::equipment_background,SteamHumanAsset::weapon_icon,
        SteamHumanAsset::armour_icon,SteamHumanAsset::accessory_icon,SteamHumanAsset::magic_icon,
        SteamHumanAsset::ceremony_background,SteamHumanAsset::curtain,SteamHumanAsset::continue_cursor,SteamHumanAsset::result_icon,
        SteamHumanAsset::president,SteamHumanAsset::gift_box,SteamHumanAsset::gift_face,SteamHumanAsset::item_icon,
        SteamHumanAsset::gift_flash,SteamHumanAsset::recovery_effect,SteamHumanAsset::recovery_number}) {
        const auto resource=steam_human_resource(asset);
        check(resource.has_value(),"具名资源身份存在");
        Image image=LoadImage((packaged(assets,resource->published_image)).string().c_str());
        check(image.data!=nullptr,"正式PNG可解码");
        if(resource->published_sprite) {
            const auto seb=ark::assets::parse_legacy_seb(bytes(packaged(assets,resource->published_sprite)));
            for(const auto &layer:seb.layers)for(const auto &p:layer.parts) {
                if(p.width<=0 || p.height<=0)continue;
                if(asset==SteamHumanAsset::president&&(p.frame==10||p.frame==11)) {
                    // 原PNG宽75，未消费帧10/11指向x76；不改原资源或认证未消费帧。
                    check(p.image_index==resource->image&&p.source_x==76&&p.width==13&&image.width==75&&
                          p.source_x+p.width>image.width,"原总统SEB10/11越界事实保留，66只消费1/9，不将异常改成通过");
                    continue;
                }
                check(p.image_index==resource->image && p.source_x>=0 && p.source_y>=0 &&
                      p.source_x+p.width<=image.width && p.source_y+p.height<=image.height,
                      "实际SEB图片身份与裁片在正式PNG内");
                if(asset==SteamHumanAsset::number05 && p.frame==16)
                    check(p.source_x==61 && p.source_y==10 && p.width==9 && p.height==10,
                          "乘号原裁片61/10/9/10");
            }
        } else if(asset==SteamHumanAsset::medal)check(image.width>=28 && image.height>=14,"奖章index1裁片实际存在");
        UnloadImage(image);
    }
    const auto number=steam_human_resource(SteamHumanAsset::number05);
    check(bytes(packaged(assets,number->published_image))!=bytes(source_root/"common/number05.png"),
          "Steam数字PNG不静默替换为APK同名图");
    check(!steam_human_resource(static_cast<SteamHumanAsset>(99)),"未知资源枚举拒绝");
}
void overview_checks(Checks &check) {
    // 概览边界夹具；不认证自然等级10、住宅或恢复道具取得。
    auto owner=test_support::page_fixture(60);
    const auto id=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=1;owner.scripts.executing_page.reset();
    owner.page_human_bindings[id]=1;owner.page_phases[id]=0;owner.page_counters[id]=0;
    owner.human_page_selections[id]=0;owner.human_pages_initialized.insert(id);
    owner.shop_humans.at(1).equipment={0,{},{},{}};
    for(auto &stock:owner.items)stock.second.inventory=0;
    SteamHumanOverviewOptions options;
    options.title_widths=std::array<int,2>{80,80};options.profession_width=52;
    options.bottom_widths={48,96};options.down_text_width=36;
    auto plan=steam_human_overview_skin(owner,id,options);
    check(plan&&plan->soft_labels==std::array<int,2>{0,2}&&!plan->recovery_gift,"概览source0左空右返回且无W不补回复资格");
    const auto texts=parts<SteamHumanOverviewText>(*plan);
    check(std::all_of(texts.begin(),texts.end(),[](const auto &t){return t.rgb==std::array<int,3>{92,51,31};}),
          "概览正文读取固定SC_WINDOW_BLACK92/51/31，不借其它窗口棕色");
    const auto job=std::find_if(texts.begin(),texts.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::profession;});
    check(job!=texts.end()&&job->font_size==9&&job->rectangle[1]==88,"职业首次测宽52选择原9号字体");
    options.profession_width=53;
    check(!steam_human_overview_skin(owner,id,options),"首次53时必须提供原改字号前第二次测宽");
    options.profession_repeat_width=52;plan=steam_human_overview_skin(owner,id,options);
    auto labels=parts<SteamHumanOverviewText>(*plan);
    check(std::find_if(labels.begin(),labels.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::profession;})->font_size==8,
          "第二次52用8号，不自行按缩小字体重新试排");
    options.profession_repeat_width=53;plan=steam_human_overview_skin(owner,id,options);
    labels=parts<SteamHumanOverviewText>(*plan);
    check(std::find_if(labels.begin(),labels.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::profession;})->font_size==7,
          "两次53进入7号分支");
    const auto bars=parts<SteamHumanOverviewScroll>(*plan);
    check(bars.size()==1&&bars[0].rectangles[1].rect==std::array<int,4>{220,69,5,32}&&
        bars[0].thumb_touch==std::array<int,4>{221,69,5,31},"概览仍保留四页比例与独立注册矩形");
    auto &growth=owner.scene.world.world.ai.growth.at(1);
    const int profession=growth.definition.current_profession;
    check(startup_world_human_details(owner,1)->threshold==12,"夹具原初级经验门槛12");
    growth.experience=1;plan=steam_human_overview_skin(owner,id,options);
    const auto partial=parts<SteamHumanImage>(*plan);
    check(std::any_of(partial.begin(),partial.end(),[](const auto &i){return i.asset==SteamHumanAsset::experience_bar&&
        i.crop==std::optional<std::array<int,4>>({2,5,6,3});}),"经验1/12截断为6而非舍入7");
    const auto remaining=parts<SteamFacilityNumber>(*plan);
    check(std::any_of(remaining.begin(),remaining.end(),[](const auto &n){return n.position==std::array<int,2>{201,110}&&n.value==11;}),
          "尚需经验读取12减1，不显示当前经验或总经验");
    growth.definition.profession_levels[profession]=10;
    growth.experience=0;plan=steam_human_overview_skin(owner,id,options);
    const auto images=parts<SteamHumanImage>(*plan);
    check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::experience_bar&&
        i.crop==std::optional<std::array<int,4>>({2,5,79,3});}),"等级恰10经验条强制79");
    const auto numbers=parts<SteamFacilityNumber>(*plan);
    check(numbers.size()==6&&std::none_of(numbers.begin(),numbers.end(),[](const auto &n){return n.position==std::array<int,2>{201,110};}),
          "大师仅满意/努力和四战斗缓存六数，不绘等级/剩余经验数");
    const auto recovery=std::find_if(owner.rules->items.begin(),owner.rules->items.end(),[](const auto &i){return i.recovery>0;});
    check(recovery!=owner.rules->items.end(),"源定义确有回复道具");
    owner.items.at(recovery->identity).inventory=1;
    ref::BattleActorRecord actor;actor.id={900};actor.kind=ref::ActorKind::human;actor.definition=1;
    actor.control.state=0;actor.hp.displayed=80;actor.hp.target=50;actor.capacity=999;
    owner.scene.world.world.ai.battle.actors.emplace(actor.id,actor);
    owner.scene.world.world.ai.human_order={actor.id};growth.derived.combat[0]=100;
    const auto combat=parts<SteamFacilityNumber>(*steam_human_overview_skin(owner,id,options));
    check(std::any_of(combat.begin(),combat.end(),[](const auto &n){return n.position==std::array<int,2>{203,128}&&n.value==100;}),
          "概览HP读取共享最大缓存100，不用实例显示80/目标50/旧capacity999");
    check(!steam_human_overview_skin(owner,id,options)->recovery_gift,"恰一半目标HP不切回复，不用显示HP或旧capacity");
    owner.scene.world.world.ai.battle.actors.at(actor.id).hp.target=49;
    check(steam_human_overview_skin(owner,id,options)->recovery_gift,"有库存且目标HP低于一半才切回复");
    owner.items.at(recovery->identity).inventory=0;
    owner.scene.world.world.ai.battle.actors.at(actor.id).control.state=7;
    check(!steam_human_overview_skin(owner,id,options)->recovery_gift,"倒下仍要求持有回复道具");
    owner.items.at(recovery->identity).inventory=1;
    check(steam_human_overview_skin(owner,id,options)->recovery_gift,"倒下且有道具进入回复提示");
    const auto before=startup_world_state_digest(owner);
    const auto size=steam_human_overview_skin(owner,id,options)->operations.size();
    for(int repeat=0;repeat<24;++repeat) {
        const auto same=steam_human_overview_skin(owner,id,options);
        check(same&&same->operations.size()==size&&startup_world_state_digest(owner)==before,"重复只读概览输出规模稳定，不改Owner/HP/随机/声音");
    }
    auto bad=owner;bad.items.erase(recovery->identity);
    check(!steam_human_overview_skin(bad,id,options),"缺必需库存依赖显式拒绝");
    bad=owner;bad.shop_humans.at(1).equipment[1]=std::numeric_limits<int>::max();
    check(!steam_human_overview_skin(bad,id,options),"错误当前防具引用不作为空槽容错");
    bad=owner;bad.page_phases[id]=1;
    check(!steam_human_overview_skin(bad,id,options),"页1不冒认本批概览皮肤");
    bad=owner;bad.scripts.pages.back().lifecycle=4;
    check(!steam_human_overview_skin(bad,id,options),"退休概览不能继续出图");
}
void owner_checks(Checks &check) {
    // 页面/HP/实例均为表现条件夹具，不宣称新局自然出现两名同定义冒险者。
    auto owner=test_support::page_fixture(60);
    auto &page=owner.scripts.pages.back();const auto id=page.id;
    page.lifecycle=1;owner.scripts.executing_page.reset();
    owner.page_human_bindings[id]=1;owner.page_phases[id]=0;owner.page_counters[id]=0;
    owner.human_page_selections[id]=0;owner.human_pages_initialized.insert(id);
    owner.human_homes.at(1)[2]=1;
    owner.shop_humans.at(1).equipment[0].reset();
    check(steam_human_detail_skin(owner,id).has_value(),"初始化条件页60产生只读皮肤");
    auto plan=*steam_human_detail_skin(owner,id);
    check(plan.after_portrait.draws.empty() && !plan.portrait.weapon && !plan.portrait.shadow,
          "无实例保留定义肖像，但不补HP/危险/标题阴影");
    check(plan.portrait_position==std::array<int,2>{75,121} && plan.portrait.body.sprite==1 &&
          plan.portrait.body.frame==0,"frame0住宅肖像原锚与方向");
    owner.page_counters[id]=100;plan=*steam_human_detail_skin(owner,id);
    check(plan.portrait_position==std::array<int,2>{75,121} && plan.portrait.body.frame==1,
          "位移100槽回原点，步行16槽仍取完整frame，不能一并清零");
    owner.page_counters[id]=50;plan=*steam_human_detail_skin(owner,id);
    check(plan.portrait_position==std::array<int,2>{98,121} && plan.portrait.body.sprite==2,
          "第4段固定98/方向2");
    owner.human_homes.at(1)[2]=0;plan=*steam_human_detail_skin(owner,id);
    check(plan.portrait_position==std::array<int,2>{86,121},"非居民肖像x减12");
    owner.scene.world.world.ai.growth.at(1).definition.current_profession=2;
    owner.shop_humans.at(1).equipment[0]=0;
    plan=*steam_human_detail_skin(owner,id);
    check(plan.portrait.body.image==owner.rules->jobs.at(2).sprites.at(owner.rules->humans.at(1).sex) &&
          plan.portrait.weapon.has_value(),"消费当前职业和当前主武器而非初始人物缓存");
    auto &ai=owner.scene.world.world.ai;
    ref::BattleActorRecord first;first.id={900};first.kind=ref::ActorKind::human;first.definition=1;
    first.control.state=0;first.hp.displayed=80;first.hp.target=90;first.capacity=999;
    auto second=first;second.id={901};second.hp.displayed=0;second.hp.target=0;
    ai.battle.actors.emplace(first.id,first);ai.battle.actors.emplace(second.id,second);
    ai.human_order={second.id,first.id};ai.growth.at(1).derived.combat[0]=100;
    owner.page_counters[id]=0;
    plan=*steam_human_detail_skin(owner,id);
    check(parts<StartupSkinRect>(plan.after_portrait)[2].rect[2]==0 &&
          parts<SteamHumanImage>(plan.after_portrait).size()==1,"按名单原序首实例读取HP，不按ID排序");
    ai.human_order={first.id,second.id};plan=*steam_human_detail_skin(owner,id);
    check(parts<StartupSkinRect>(plan.after_portrait)[2].rect[2]==14 &&
          parts<SteamHumanImage>(plan.after_portrait).empty(),"HP最大取共享定义100，不借旧实例capacity999");
    auto bound=owner;
    bound.actor_metadata.emplace(second.id,StartupWorldActorMetadata{1,3,0,{}});
    bound.human_detail_contexts.at(id)={1,second.id}; // 镜头确认来源的明确W条件，不宣称自然双实例。
    const auto bound_plan=steam_human_detail_skin(bound,id);
    check(bound_plan && parts<StartupSkinRect>(bound_plan->after_portrait)[2].rect[2]==0 &&
          parts<SteamHumanImage>(bound_plan->after_portrait).size()==1,
          "source1显式W优先于同定义首实例，不能误显示另一人的HP");
    ai.battle.actors.at(first.id).control.state=7;
    check(!steam_human_detail_skin(owner,id),"倒下缺实际测宽不猜字宽");
    plan=*steam_human_detail_skin(owner,id,60);
    check(plan.portrait_position==std::array<int,2>{75,121} && plan.portrait.body.sprite==13 &&
          plan.portrait.body.frame==0 && !plan.portrait.weapon &&
          parts<StartupSkinRect>(plan.after_portrait).empty() && parts<SteamHumanText>(plan.after_portrait).size()==1,
          "倒下走固定身体13/0和气泡，不复用普通位移/HP/武器");
    owner.scripts.pages.back().lifecycle=3;
    ref::WorldScriptPage child;child.id=owner.scripts.next_page_id++;child.kind=ref::WorldScriptPageKind::raw_page;
    child.legacy_page=61;child.lifecycle=1;owner.scripts.pages.push_back(child);
    const auto before=startup_world_state_digest(owner);
    check(steam_human_detail_skin(owner,id,60).has_value() && startup_world_state_digest(owner)==before,
          "被子页覆盖的父60生命周期3仍可只读绘制，不推进Owner/随机/输出");
    for(int missing=0;missing<5;++missing) {
        auto bad=owner;
        if(missing==0)bad.page_human_bindings.erase(id);
        if(missing==1)bad.page_phases.erase(id);
        if(missing==2)bad.page_counters.erase(id);
        if(missing==3)bad.human_page_selections.erase(id);
        if(missing==4)bad.human_pages_initialized.erase(id);
        check(!steam_human_detail_skin(bad,id,60),"已初始化但缺必需键显式拒绝，不抛map.at");
    }
}
void attributes_checks(Checks &check) {
    // 缓存字段／无W为表现条件，不认证自然培养到这些属性。
    auto owner=test_support::page_fixture(60);const auto id=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=1;owner.scripts.executing_page.reset();
    owner.page_human_bindings[id]=1;owner.page_phases[id]=1;owner.page_counters[id]=0;
    owner.human_page_selections[id]=0;owner.human_pages_initialized.insert(id);
    auto &growth=owner.scene.world.world.ai.growth.at(1);
    growth.derived.attributes={11,22,33,44,55,66};growth.derived.combat={101,202,303,404};
    growth.definition.extra={7,7,7,7,7,7}; // 与缓存故意不同，绘制不能重算或再加一遍。
    SteamHumanOverviewOptions options;options.title_widths=std::array<int,2>{80,80};
    options.profession_width=52;options.profession_change_width=24;
    const auto plan=steam_human_attributes_skin(owner,id,options);
    check(plan&&plan->soft_labels==std::array<int,2>{0,2}&&!plan->recovery_gift,"六属性保留source0空左软标签，不改成转职");
    constexpr std::array<std::array<int,3>,6> expected{{{29,134,0},{29,153,16},{29,172,32},
        {121,134,48},{121,153,64},{121,172,96}}};
    int row{};
    for(std::size_t n=0;n<plan->operations.size();++n) {
        const auto *t=std::get_if<SteamHumanOverviewText>(&plan->operations[n]);
        if(!t||t->role!=SteamHumanOverviewTextRole::attribute)continue;
        check(row<6&&n>=3&&n+1<plan->operations.size(),"每属性均有完整五图元层序");
        const auto &e=expected[row];
        const auto *fill=std::get_if<SteamStartupFrameDraw>(&plan->operations[n-3]);
        const auto *line=std::get_if<SteamStartupFrameDraw>(&plan->operations[n-2]);
        const auto *image=std::get_if<SteamHumanImage>(&plan->operations[n-1]);
        const auto *value=std::get_if<SteamFacilityNumber>(&plan->operations[n+1]);
        check(fill&&line&&std::get<StartupSkinRect>(*fill).rect==std::array<int,4>{e[0]-4,e[1]-2,93,20}&&
            !std::get<StartupSkinRect>(*fill).outline&&std::get<StartupSkinRect>(*fill).rgb==std::array<int,3>{208,242,249}&&
            std::get<StartupSkinRect>(*line).rect==std::get<StartupSkinRect>(*fill).rect&&
            std::get<StartupSkinRect>(*line).outline&&std::get<StartupSkinRect>(*line).rgb==std::array<int,3>{149,206,217},
            "先原FillRect再DrawRect半开93×20，不双加1");
        check(image&&image->asset==SteamHumanAsset::combat_icon&&image->position==std::array<int,2>{e[0],e[1]}&&
            image->crop==std::optional<std::array<int,4>>({e[2],16,16,16}),"mode7直接裁图，运气原x96，不套SEB98");
        check(t->argument==row&&t->rectangle==std::array<int,4>{e[0]+18,e[1]+2,0,0}&&t->font_size==0&&t->rgb==std::array<int,3>{0,100,255}&&
            value&&value->value==11*(row+1)&&value->position==std::array<int,2>{e[0]+83,e[1]+3}&&
            value->asset==SteamFacilityAsset::number08,"六字段原序读共享缓存，默认字号和SEB15右锚");
        ++row;
    }
    check(row==6,"两列各三属性，不遗漏或追加战斗值");
    const auto bars=parts<SteamHumanOverviewScroll>(*plan);
    check(bars.size()==1&&bars[0].rectangles[1].rect==std::array<int,4>{220,100,5,32}&&
        bars[0].thumb_touch==std::array<int,4>{221,100,5,31},"页1保持四页蓝条比例与原有符号截断位置");
    const auto texts=parts<SteamHumanOverviewText>(*plan);
    check(std::count_if(texts.begin(),texts.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::profession_change;})==1&&
        std::none_of(texts.begin(),texts.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::gift;}),"页1唯一底部转职，不绘概览赠礼");
    const auto before=startup_world_state_digest(owner);
    for(int repeat=0;repeat<24;++repeat)
        check(steam_human_attributes_skin(owner,id,options)->operations.size()==plan->operations.size()&&
            startup_world_state_digest(owner)==before,"重复六属性输出规模稳定，不重算缓存/随机/输出");
    auto missing=options;missing.profession_change_width.reset();
    check(!steam_human_attributes_skin(owner,id,missing),"缺底部测宽显式拒绝");
    auto local=owner;local.items.clear();
    check(steam_human_attributes_skin(local,id,options).has_value(),"页1不查不消费页0回复库存");
    local=owner;local.page_phases[id]=2;
    check(!steam_human_attributes_skin(local,id,options),"页2仍显式拒绝，不冒认完整四页");
    local=owner;local.scripts.pages.back().lifecycle=4;
    check(!steam_human_attributes_skin(local,id,options),"退休六属性页不继续出图");
}
void equipment_magic_checks(Checks &check) {
    auto owner=test_support::page_fixture(60);const auto id=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=1;owner.scripts.executing_page.reset();
    owner.page_human_bindings[id]=1;owner.page_phases[id]=2;owner.page_counters[id]=0;
    owner.human_page_selections[id]=2;owner.human_pages_initialized.insert(id);
    owner.shop_humans.at(1).equipment={0,{},{},{}};
    SteamHumanOverviewOptions options;options.title_widths=std::array<int,2>{80,80};options.profession_width=52;
    options.equipment_hint_width=96;options.magic_hint_width=120;
    auto plan=steam_human_information_skin(owner,id,options);
    check(plan.has_value(),"四装备读取已有选择2，不退回默认武器");
    const auto rows=parts<SteamHumanEquipmentRow>(*plan);
    check(rows.size()==4,"四行均保留Touch10原请求");
    for(int row=0;row<4;++row)
        check(rows[row].touch.component==10&&rows[row].touch.value==(0x20000|row)&&
              rows[row].touch.rectangle==std::optional<std::array<int,4>>({5,128+17*row,225,18})&&
              rows[row].touch.option==0&&rows[row].margin==std::array<int,4>{0,-20,0,0},"普通装备行无marker且保持负margin");
    auto labels=parts<SteamHumanOverviewText>(*plan);
    check(std::count_if(labels.begin(),labels.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::equipment_empty&&!t.layout;})==3,
          "三空槽普通DrawString，不套有装备名的TextLayout");
    check(parts<SteamStartupTouch>(*plan).empty(),"空装备底文false/false不注册确认条");
    const auto blue=parts<SteamHumanOverviewScroll>(*plan);
    check(blue[0].rectangles[1].rect==std::array<int,4>{220,132,5,32},"装备页四页比例蓝块top132");
    owner.human_page_selections[id]=0;plan=steam_human_information_skin(owner,id,options);
    const auto defs=std::find_if(owner.rules->equipment.begin(),owner.rules->equipment.end(),[](const auto &d){return d.shop.kind==1&&d.shop.id==0;});
    check(defs!=owner.rules->equipment.end()&&defs->shop.combat[1]>0,"原短剑确有攻击加值");
    const auto values=parts<SteamFacilityNumber>(*plan);
    check(std::any_of(values.begin(),values.end(),[&](const auto &n){return n.kind==SteamFacilityNumberKind::plus_value&&
          n.asset==SteamFacilityAsset::number05&&n.value==defs->shop.combat[1]&&n.position==std::array<int,2>{108,205};}),
          "所选武器底文用装备定义加值/SEB12，不用人物战斗缓存");
    owner.page_phases[id]=3;owner.scene.world.world.ai.growth.at(1).derived.available_spells={false,false,false,false};
    plan=steam_human_information_skin(owner,id,options);
    labels=parts<SteamHumanOverviewText>(*plan);
    check(std::count_if(labels.begin(),labels.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::magic_empty&&t.anchor==34&&t.rgb==std::array<int,3>{0,100,255};})==4&&
          parts<SteamStartupTouch>(*plan).empty(),"未学四格蓝色---且不创建赠礼/转职触摸");
    check(parts<SteamHumanOverviewScroll>(*plan)[0].rectangles[1].rect==std::array<int,4>{220,164,5,32},"魔法页蓝块top164");
    for(int mask=1;mask<16;++mask) {
        auto &spells=owner.scene.world.world.ai.growth.at(1).derived.available_spells;
        int count{};for(int i=0;i<4;++i){spells[i]=(mask&(1<<i))!=0;if(spells[i])++count;}
        plan=steam_human_information_skin(owner,id,options);const auto images=parts<SteamHumanImage>(*plan);
        int found{};for(const auto &img:images)if(img.asset==SteamHumanAsset::magic_icon) {
            check(img.crop&&(*img.crop)[1]==0&&(*img.crop)[2]==16&&(*img.crop)[3]==16&&(*img.crop)[0]!=48,
                  "回复魔法使用index4而非第四列index3");++found;
        }
        labels=parts<SteamHumanOverviewText>(*plan);
        check(found==count&&std::any_of(labels.begin(),labels.end(),[=](const auto &t){return t.role==SteamHumanOverviewTextRole::magic_status&&
              t.argument==count&&t.rectangle==std::array<int,4>{120,204,0,0};}),"魔法所有组合按四可用位及计数正文，不消费永久学习位代替");
    }
    const auto before=startup_world_state_digest(owner);
    const auto size=plan->operations.size();
    for(int i=0;i<24;++i)check(steam_human_information_skin(owner,id,options)->operations.size()==size&&
        startup_world_state_digest(owner)==before,"完整四页计划无Owner/随机/声音写入且输出稳定");
    owner.human_page_selections[id]=4;
    check(!steam_human_information_skin(owner,id,options),"选择4严格拒绝，不映射到某个合法槽");
}
void equipment_detail_checks(Checks &check) {
    // 展示目录条件，未认证真实玩家已经开放第二武器。
    auto owner=test_support::page_fixture(73);const auto id=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=1;owner.scripts.executing_page.reset();
    owner.page_human_bindings[id]=1;owner.page_phases[id]=0;owner.page_counters[id]=0;
    owner.human_page_selections[id]=0;owner.human_pages_initialized.insert(id);
    owner.equipment_page_catalogs[id][0]={0,1};owner.human_equipment_choices[id]={0,0};
    SteamHumanOverviewOptions options;options.title_widths=std::array<int,2>{90,90};
    const auto plan=steam_human_equipment_detail_skin(owner,id,options);
    check(plan&&plan->soft_labels==std::array<int,2>{2,2}&&plan->title=="装备信息 1/2","原73双返回及真实目录序号");
    const auto arrows=parts<SteamFacilityArrowTouch>(*plan);
    check(arrows.size()==2&&arrows[0].position==std::array<int,2>{51,77}&&arrows[0].frame==3&&arrows[0].value==16&&
          arrows[1].position==std::array<int,2>{189,77}&&arrows[1].frame==0&&arrows[1].value==18,
          "TitleArrow170/125原y77，注册SEB请求后委托真实按钮纹理");
    const auto numbers=parts<SteamFacilityNumber>(*plan);
    check(std::any_of(numbers.begin(),numbers.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::money&&
          n.asset==SteamFacilityAsset::number08&&n.position==std::array<int,2>{189,92}&&n.value==400;}),
          "短剑原基础价400而非赠礼600或render_image50");
    const auto text=parts<SteamHumanOverviewText>(*plan);
    check(std::any_of(text.begin(),text.end(),[](const auto &t){return t.value=="--"&&t.rectangle==std::array<int,4>{173,119,0,0}&&
          t.anchor==34&&t.rgb==std::array<int,3>{92,51,31};}),"非正HP原--173/119居中，不猜---184/115");
    const auto before=startup_world_state_digest(owner);
    for(int i=0;i<12;++i)check(steam_human_equipment_detail_skin(owner,id,options)->operations.size()==plan->operations.size()&&
        startup_world_state_digest(owner)==before,"73读取不收费/装配/清NEW/抽随机，输出稳定");
    owner.human_page_selections[id]=1;
    const auto second=steam_human_equipment_detail_skin(owner,id,options);
    const auto names=parts<SteamHumanOverviewText>(*second);
    const auto expected=std::find_if(owner.rules->equipment.begin(),owner.rules->equipment.end(),[](const auto &d){return d.shop.kind==1&&d.shop.id==1;});
    check(second->title=="装备信息 2/2"&&std::any_of(names.begin(),names.end(),[&](const auto &t){return t.role==SteamHumanOverviewTextRole::equipment_name&&t.value==expected->name;}),
          "73切目录读取当前选择1，不固定初始choice0");
    owner.equipment_page_catalogs[id][0]={0};owner.human_page_selections[id]=0;
    check(parts<SteamFacilityArrowTouch>(*steam_human_equipment_detail_skin(owner,id,options)).empty(),"原单元素目录没有TitleArrow");
    owner.human_page_selections[id]=1;
    check(!steam_human_equipment_detail_skin(owner,id,options),"73越界目录选择显式拒绝");
    owner.human_page_selections[id]=0;owner.human_equipment_choices.erase(id);
    check(!steam_human_equipment_detail_skin(owner,id,options),"73缺初始化来源拒绝而非容错补默认");
}
void gift_confirmation_checks(Checks &check) {
    auto owner=test_support::page_fixture(64);const auto parent=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=3;owner.scripts.executing_page.reset();
    owner.page_human_bindings[parent]=1;owner.page_phases[parent]=0;owner.page_counters[parent]=0;
    owner.human_page_selections[parent]=0;owner.human_pages_initialized.insert(parent);
    owner.equipment_page_catalogs[parent][0]={1};owner.human_equipment_choices[parent]={0,1};
    ref::WorldScriptPage child;child.id=owner.scripts.next_page_id++;child.kind=ref::WorldScriptPageKind::raw_page;
    child.legacy_page=65;child.lifecycle=1;owner.scripts.pages.push_back(child);const auto id=child.id;
    owner.page_human_bindings[id]=1;owner.page_phases[id]=0;owner.page_counters[id]=0;
    owner.human_page_selections[id]=0;owner.human_pages_initialized.insert(id);
    owner.human_equipment_choices[id]={0,1};owner.human_page_parents[id]=parent;
    owner.shop_humans.at(1).equipment[0]=0;owner.scene.world.world.ai.growth.at(1).derived.combat={100,999,888,777};
    SteamHumanOverviewOptions options;options.title_widths=std::array<int,2>{48,48};
    options.comparison_name_widths={20,20};options.gift_accept_width=48;
    auto plan=steam_human_gift_confirmation_skin(owner,id,options);
    check(plan&&plan->soft_labels==std::array<int,2>{0,2}&&plan->title=="赠送礼物","65固定赠礼标题，左空右返回");
    owner.scripts.pages[owner.scripts.pages.size()-2].lifecycle=2;
    check(steam_human_gift_confirmation_skin(owner,id,options).has_value(),"真实通用open的父64仍life2可绘制，不伪定必须life3");
    const auto numbers=parts<SteamFacilityNumber>(*plan);
    check(numbers.size()==4&&numbers[0].value==5&&numbers[0].asset==SteamFacilityAsset::number08&&
          numbers[1].value==8&&numbers[1].asset==SteamFacilityAsset::number05&&numbers[2].value==0&&
          numbers[3].value==0&&numbers[3].asset==SteamFacilityAsset::number08,"短剑5与铁剑8为装备定义参数，不用人物缓存999；升值12同值15");
    check(std::none_of(numbers.begin(),numbers.end(),[](const auto &n){return n.kind!=SteamFacilityNumberKind::number;}),
          "确认65没有价格/库存/加号，不提前报价或兑现");
    const auto images=parts<SteamFacilityImage>(*plan);
    check(std::count_if(images.begin(),images.end(),[](const auto &i){return i.asset==SteamFacilityAsset::title_arrows;})==3&&
          images[0].frame==3&&images[1].frame==1&&images[2].frame==1,"counter0三箭头首项frame3，其余1");
    auto width=options;width.comparison_name_widths[0]=std::numeric_limits<int>::max();
    check(steam_human_gift_confirmation_skin(owner,id,width).has_value(),"名称测宽先Clamp90，不能int+20溢出");
    owner.shop_humans.at(1).equipment[0]=1;owner.human_equipment_choices[parent]=owner.human_equipment_choices[id]={0,0};
    owner.equipment_page_catalogs[parent][0]={0};owner.page_counters[id]=16;
    plan=steam_human_gift_confirmation_skin(owner,id,options);const auto decreased=parts<SteamFacilityNumber>(*plan);
    check(decreased[1].value==5&&decreased[1].asset==SteamFacilityAsset::number12,"下降用原SEB19而非普通/红字体数字");
    const auto before=startup_world_state_digest(owner);const auto size=plan->operations.size();
    for(int i=0;i<12;++i)check(steam_human_gift_confirmation_skin(owner,id,options)->operations.size()==size&&
        startup_world_state_digest(owner)==before,"65重复绘制不装配/扣款/答案/声音/随机变更");
    auto bad=owner;bad.page_human_bindings.erase(parent);
    check(!steam_human_gift_confirmation_skin(bad,id,options),"65父绑定缺失显式拒绝");
    bad=owner;bad.equipment_page_catalogs.erase(parent);
    check(!steam_human_gift_confirmation_skin(bad,id,options),"65父目录缺载荷不能从候选装备补默认");
    bad=owner;bad.scripts.pages[bad.scripts.pages.size()-2].lifecycle=4;
    check(!steam_human_gift_confirmation_skin(bad,id,options),"65退休父64不能冒认有效候选来源");
    bad=owner;bad.page_phases.erase(id);
    check(!steam_human_gift_confirmation_skin(bad,id,options),"已初始化65缺phase不能靠绘制补键");
    bad=owner;bad.human_equipment_choices.at(parent)[1]=1;
    check(!steam_human_gift_confirmation_skin(bad,id,options),"65错误父候选拒绝");
    auto missing=options;missing.comparison_name_widths[0].reset();
    check(!steam_human_gift_confirmation_skin(owner,id,missing),"缺实际字号10测宽拒绝");
}
StartupWorldRuntimeState profession_fixture(int raw) {
    auto owner=test_support::page_fixture(61);const auto parent=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=2;owner.scripts.executing_page.reset();
    owner.page_human_bindings[parent]=1;owner.page_phases[parent]=0;owner.page_counters[parent]=0;
    owner.human_page_selections[parent]=0;owner.human_pages_initialized.insert(parent);owner.human_page_catalogs[parent]={0,1};
    ref::WorldScriptPage page;page.id=owner.scripts.next_page_id++;page.kind=ref::WorldScriptPageKind::raw_page;
    page.legacy_page=raw;page.lifecycle=1;owner.scripts.pages.push_back(page);
    owner.page_human_bindings[page.id]=1;owner.page_phases[page.id]=0;owner.page_counters[page.id]=0;
    owner.human_page_selections[page.id]=1;owner.human_pages_initialized.insert(page.id);
    owner.human_page_catalogs[page.id]={0,1};owner.page_job_bindings[page.id]=1;owner.human_page_parents[page.id]=parent;
    owner.human_gift_messages[page.id]="谢谢";
    // 仅表现条件，不声称真实人物已转职、升级或领取奖励。
    owner.scene.world.world.ai.growth.at(1).definition.current_profession=0;
    owner.scene.world.world.ai.growth.at(1).definition.profession_levels[1]=10;
    owner.human_attribute_display={std::array<int,6>{100,100,100,100,100,100},
        std::array<int,6>{99,100,101,100,100,100},std::array<int,6>{-1,0,1,0,0,0}};
    owner.reward_display={std::array<int,2>{999,9},std::array<int,2>{999,10},std::array<int,2>{5,1}};
    owner.village_points=1000;
    return owner;
}
void profession_skin_checks(Checks &check) {
    auto owner=profession_fixture(62);const auto id=owner.scripts.pages.back().id;
    SteamHumanOverviewOptions o;o.title_widths=std::array<int,2>{80,80};o.profession_change_width=48;
    o.insufficient_points_width=72;o.message_width=24;o.character_frame=4;
    const auto preview=steam_human_profession_preview_skin(owner,id,o);
    check(preview&&preview->soft_labels==std::array<int,2>{0,2},"62右返回，读取真实父61与目录");
    const auto values=parts<SteamFacilityNumber>(*preview);
    const auto negative=std::find_if(values.begin(),values.end(),[](const auto &v){return v.parameter==0;});
    check(negative!=values.end()&&negative->kind==SteamFacilityNumberKind::minus_value&&negative->asset==SteamFacilityAsset::number12&&
          negative->value==1&&negative->position==std::array<int,2>{112,137},"62只有at[2]差额，负差SEB19正幅值右112/137");
    check(std::count_if(values.begin(),values.end(),[](const auto &v){return v.kind==SteamFacilityNumberKind::plus_value&&v.value==0;})==4,
          "62零差也画+0，不额外画before/after数值");
    const auto minus=steam_facility_number_draws(*negative,8);
    check(minus&&minus->back().frame==15&&minus->back().position==std::array<int,2>{96,137},"原minus先数字再减号，符号右锚减8*(位数+1)");
    auto bad_number=*negative;bad_number.value=-1;
    check(!steam_facility_number_draws(bad_number,8),"minus不自动abs负输入");
    bad_number=*negative;bad_number.kind=SteamFacilityNumberKind::plus_value;
    check(!steam_facility_number_draws(bad_number,8),"SEB19仍拒绝未证plus消费者");
    const auto portraits=parts<SteamHumanPortrait>(*preview);
    check(portraits.size()==4&&portraits[0].clip==std::optional<std::array<int,4>>({38,68,15,14})&&portraits[0].image_clip&&
          portraits[0].position==std::array<int,2>{46,89}&&std::none_of(portraits.begin(),portraits.end(),[](const auto &p){return p.actor.weapon.has_value();}),
          "62 clip/ImageClip并存且四身体不带武器");
    const auto images=parts<SteamFacilityImage>(*preview);
    check(std::any_of(images.begin(),images.end(),[](const auto &p){return p.asset==SteamFacilityAsset::maximum&&p.position==std::array<int,2>{171,118};}),
          "目标职业读取已有等级10并绘MAX，不固定预览等级1");
    auto no_points=owner;no_points.village_points=-1;
    const auto denied=steam_human_profession_preview_skin(no_points,id,o);
    const auto denied_images=parts<SteamHumanImage>(*denied);
    const auto denied_touches=parts<SteamStartupTouch>(*denied);
    check(std::none_of(denied_touches.begin(),denied_touches.end(),[](const auto &t){return t.component==4;})&&
          std::any_of(denied_images.begin(),denied_images.end(),[](const auto &i){return i.asset==SteamHumanAsset::result_icon;}),
          "不足点显示原result icon，不伪造支付热区");
    auto bad=owner;bad.human_page_parents.erase(id);
    check(!steam_human_profession_preview_skin(bad,id,o),"62缺父来源拒绝");
    bad=owner;bad.human_attribute_display[2][0]=2;
    check(!steam_human_profession_preview_skin(bad,id,o),"冻结差额失配拒绝，不绘制重算");
    const auto before=startup_world_state_digest(owner);
    for(int i=0;i<12;++i)check(steam_human_profession_preview_skin(owner,id,o)->operations.size()==preview->operations.size()&&
        startup_world_state_digest(owner)==before,"62只读稳定，不扣点/勋章/NEW/随机");
    for(const auto sample:std::vector<std::array<int,3>>{{0,0,999},{1,9,999},{1,10,999},{1,47,999},{1,57,999},{1,67,999},{1,77,999}}) {
        const auto plan=steam_human_reward_skin(owner,1,sample[0],sample[1],o);
        const auto nums=parts<SteamFacilityNumber>(*plan);
        check(nums[0].value==sample[2],"满意封顶主值不由请求量5增加到1004");
        if(sample[0]&&sample[1]>=10)check(std::any_of(nums.begin(),nums.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::plus_value&&n.value==5;}),
            "封顶仍显示原请求+5而不是实际差0");
        const auto imgs=parts<SteamHumanImage>(*plan);
        check(std::any_of(imgs.begin(),imgs.end(),[](const auto &i){return i.asset==SteamHumanAsset::continue_cursor;})==(sample[0]==1&&sample[1]>=77),
            "共用奖励箭头只page1满77");
    }
    auto missing=o;missing.character_frame.reset();
    check(!steam_human_reward_skin(owner,1,1,77,missing),"共享人物表现帧缺失拒绝，不用结果counter补默认");
    owner=profession_fixture(63);owner.human_page_selections[id]=0;
    for(const auto sample:std::vector<std::array<int,2>>{{25,-1},{26,1},{30,2},{40,3},{46,4},{52,3},{54,3},{55,3},{70,2},{80,1},{84,-1},{109,-1},{110,-1},{196,-1},{197,-1}}) {
        owner.page_counters[id]=sample[0];
        const auto plan=steam_human_profession_result_skin(owner,id,o);
        check(plan&&plan->soft_labels==std::array<int,2>{0,0},"63只读演出无软返回");
        const auto imgs=parts<SteamHumanImage>(*plan);
        check(std::count_if(imgs.begin(),imgs.end(),[&](const auto &i){return i.asset==SteamHumanAsset::curtain&&i.frame>0;})==(sample[1]>=0?1:0),
            "63幕布边界及退休，无额外帧");
        if(sample[1]>=0)check(std::any_of(imgs.begin(),imgs.end(),[&](const auto &i){return i.asset==SteamHumanAsset::curtain&&i.frame==sample[1];}),
            "63幕布40起局部12槽交替3/4");
        check(std::any_of(imgs.begin(),imgs.end(),[](const auto &i){return i.asset==SteamHumanAsset::continue_cursor;})==(sample[0]==197),
            "63满197才有继续箭头，196无箭头");
        const auto actor=parts<SteamHumanPortrait>(*plan).front();
        check(actor.actor.body.sprite==22&&actor.actor.body.frame==0&&!actor.actor.weapon,"63action4/dir2固定human22/frame0无武器");
        check(actor.actor.body.image==owner.rules->jobs[sample[0]<55?0:1].sprites[0],"63前55当前职业，55起绑定目标，无持久旧职业字段");
    }
    const auto result_before=startup_world_state_digest(owner);
    const auto result_size=steam_human_profession_result_skin(owner,id,o)->operations.size();
    for(int i=0;i<12;++i)check(steam_human_profession_result_skin(owner,id,o)->operations.size()==result_size&&
        startup_world_state_digest(owner)==result_before,"63纯绘制不改职业/重算/发声/重抽冻结文案");
    owner.human_gift_messages.erase(id);
    check(!steam_human_profession_result_skin(owner,id,o),"63缺冻结文案拒绝，不由绘制随机生成");
}
void gift_result_skin_checks(Checks &check) {
    auto owner=profession_fixture(66);const auto id=owner.scripts.pages.back().id;
    owner.human_page_selections[id]=0;owner.human_equipment_choices[id]={0,1};owner.human_gift_scores[id]=24;
    SteamHumanOverviewOptions o;o.title_widths=std::array<int,2>{48,48};o.message_width=24;o.character_frame=0;
    for(int frame:{0,9,10,13,16,23,25,34,35,44,45,47,48,53,54,60,74,75,84,85,161,162}) {
        owner.page_counters[id]=frame;owner.page_phases[id]=frame>=135?1:0;
        const auto plan=steam_human_gift_result_skin(owner,id,o);
        check(plan&&plan->soft_labels==std::array<int,2>{0,0},"66礼物只读结果、无返回软标签");
        const auto images=parts<SteamHumanImage>(*plan);
        const auto people=parts<SteamHumanPortrait>(*plan);
        const auto president=std::find_if(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::president;});
        check(president!=images.end()&&president->frame==(frame>=10&&frame<20?9:1),"spChara2是总统，10..19行动3/direction1=frame9");
        check(people.front().actor.body.sprite==(frame>=25&&frame<35?25:1)&&people.front().actor.body.frame==0&&
              !people.front().actor.weapon,"上部25..34为buy01 SEB25/frame0，35无基础动作残留且无武器");
        const auto box=std::find_if(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::gift_box;});
        check(box!=images.end()&&box->frame==(frame<45?0:1),"45才开礼盒，不靠PNG数猜时序");
        if(frame==0)check(box->position==std::array<int,2>{126,137},"礼盒13前初始126/137");
        if(frame==13)check(box->position==std::array<int,2>{126,137},"13起抛物线局部0");
        if(frame==16)check(box->position==std::array<int,2>{125,134},"礼盒局部3：单精度抛物线向零截断3，横整数125");
        if(frame==23)check(box->position==std::array<int,2>{120,131},"礼盒局部10顶点6，横120");
        if(frame>=35)check(box->position==std::array<int,2>{114,137},"20槽抛物线结束114/137");
        const auto icon=std::find_if(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::weapon_icon;});
        check((icon!=images.end())==(frame>=45&&frame<75),"商品只45..74出现");
        if(frame==45)check(icon->position==std::array<int,2>{105,117},"商品45起105/117");
        if(frame==60)check(icon->position==std::array<int,2>{105,92},"60夹到92而非继续飞出");
        check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::gift_flash;})==(frame>=45&&frame<54),
              "dropItem00仅三个3槽帧，54消失，无绘制发声");
        if(frame>=75)check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::gift_face&&i.frame==1&&i.position==std::array<int,2>{63,157};}),
              "低评价使用icon_nico00 frame1固定y157，不使用满意smile00");
        check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::continue_cursor;})==(frame==162),
              "66奖励page1局部77（总162）才有继续箭头，不混Owner提前确认门槛");
    }
    const auto before=startup_world_state_digest(owner);
    const auto size=steam_human_gift_result_skin(owner,id,o)->operations.size();
    for(int i=0;i<12;++i)check(steam_human_gift_result_skin(owner,id,o)->operations.size()==size&&startup_world_state_digest(owner)==before,
        "66重复绘制不扣款/发声/随机/奖励/装配/HP更新");
    owner.page_counters[id]=79;owner.page_phases[id]=0;
    for(int score:{24,25,69,70}) {
        owner.human_gift_scores[id]=score;
        const auto images=parts<SteamHumanImage>(*steam_human_gift_result_skin(owner,id,o));
        check(std::any_of(images.begin(),images.end(),[=](const auto &i){return i.asset==SteamHumanAsset::gift_face&&
            i.frame==(score<25?1:score<70?0:2)&&i.position==std::array<int,2>{63,score<25?157:156};}),
            "评价24/25/69/70三档真实帧及中高档8槽跳动");
    }
    const auto ordinary=std::find_if(owner.rules->items.begin(),owner.rules->items.end(),[](const auto &i){return i.recovery==0&&i.effect>=0&&i.effect<6;});
    check(ordinary!=owner.rules->items.end(),"固定表有普通属性道具");
    owner.human_equipment_choices[id]={4,ordinary->identity};owner.page_counters[id]=45;
    auto ordinary_plan=steam_human_gift_result_skin(owner,id,o);
    const auto ordinary_images=parts<SteamHumanImage>(*ordinary_plan);
    check(std::any_of(ordinary_images.begin(),ordinary_images.end(),[](const auto &i){return i.asset==SteamHumanAsset::item_icon&&i.position==std::array<int,2>{106,117};}),
          "普通道具沿真实图标裁片，商品锚106/117不同装备105/117");
    const auto learning=std::find_if(owner.rules->items.begin(),owner.rules->items.end(),[](const auto &i){return i.effect==9;});
    check(learning!=owner.rules->items.end(),"固定表有魔法学习道具");
    owner.human_equipment_choices[id]={4,learning->identity};owner.page_counters[id]=75;
    const auto learned=parts<SteamHumanOverviewText>(*steam_human_gift_result_skin(owner,id,o));
    check(std::any_of(learned.begin(),learned.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::result_message&&t.value.find("习得")!=std::string::npos;}),
          "学习道具气泡取魔法学习文案，不直接用冻结普通评价");
    const auto recovery=std::find_if(owner.rules->items.begin(),owner.rules->items.end(),[](const auto &i){return i.recovery>0;});
    check(recovery!=owner.rules->items.end(),"固定表有真实回复道具");
    owner.human_equipment_choices[id]={4,recovery->identity};owner.page_counters[id]=75;owner.page_phases[id]=0;
    owner.scene.world.world.ai.human_order.clear();
    auto plan=steam_human_gift_result_skin(owner,id,o);
    const auto rectangles=[&](const SteamHumanOverviewPlan &p) {
        std::vector<StartupSkinRect> result;
        for(const auto &op:p.operations)if(const auto *f=std::get_if<SteamStartupFrameDraw>(&op))
            if(const auto *r=std::get_if<StartupSkinRect>(f))result.push_back(*r);
        return result;
    };
    auto rects=rectangles(*plan);
    check(std::any_of(rects.begin(),rects.end(),[](const auto &r){return r.rect==std::array<int,4>{97,115,18,2}&&r.rgb==std::array<int,3>{83,255,0};}),
          "66无W原特殊恢复展示满18，不修改60无W不画条");
    for(int frame:{75,78,79,82,83,86,87,90,91,114,115}) {
        owner.page_counters[id]=frame;plan=steam_human_gift_result_skin(owner,id,o);
        const auto images=parts<SteamHumanImage>(*plan);
        const auto effect=std::find_if(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::recovery_effect;});
        check((effect!=images.end())==(frame<91),"回复effect按4/8/12/16四门槛结束，不循环");
        if(effect!=images.end())check(effect->frame==(frame-75)/4,"回复四帧门槛向右独立");
        check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::recovery_number;})==(frame<115),
              "回复数字SEB18仅75..114，115改HP回复气泡");
    }
    owner.page_counters[id]=75;
    ref::BattleActorRecord actor;actor.id={510};actor.definition=1;actor.kind=ref::ActorKind::human;
    actor.hp.displayed=actor.hp.target=0;owner.scene.world.world.ai.battle.actors[actor.id]=actor;
    owner.scene.world.world.ai.human_order.push_back(actor.id);
    rects=rectangles(*steam_human_gift_result_skin(owner,id,o));
    check(std::any_of(rects.begin(),rects.end(),[](const auto &r){return r.rect==std::array<int,4>{97,115,0,2}&&r.rgb==std::array<int,3>{83,255,0};}),
          "真实实例HP0取NOW空条，不用缺W的满条fallback");
    owner.human_gift_scores.erase(id);
    check(!steam_human_gift_result_skin(owner,id,o),"66缺评价冻结载荷拒绝，不在绘制重评礼物");
    owner.human_gift_scores[id]=30;owner.scripts.pages.back().lifecycle=4;
    check(!steam_human_gift_result_skin(owner,id,o),"66退休页面不再输出人物/礼盒/HP表现引用");
}
}
// 复用既有visuals执行文件；本层只验表现边界，不重测人物成长/战斗/职业业务。
int check_steam_human_skin(const std::filesystem::path &source_root) {
    Checks check;power_checks(check);medal_bubble_checks(check);resources(check,source_root);owner_checks(check);overview_checks(check);attributes_checks(check);equipment_magic_checks(check);equipment_detail_checks(check);gift_confirmation_checks(check);profession_skin_checks(check);gift_result_skin_checks(check);
    return check.count;
}
