#include "dungeon_village_prototype/startup_skin.hpp"
#include "dungeon_village_prototype/steam_clear_score_skin.hpp"
#include "dungeon_village_prototype/steam_new_game_skin.hpp"
#include "dungeon_village_prototype/steam_config_skin.hpp"
#include "dungeon_village_prototype/steam_commerce_skin.hpp"
#include "dungeon_village_prototype/startup_world_commerce.hpp"
#include "dungeon_village_prototype/steam_platform_button_skin.hpp"
#include "dungeon_village_prototype/steam_cursor_skin.hpp"
#include "dungeon_village_prototype/steam_joystick_prompts.hpp"
#include "dungeon_village_prototype/startup_information.hpp"
#include "dungeon_village_prototype/steam_information_skin.hpp"
#include "dungeon_village_prototype/steam_monster_skin.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "dungeon_village_prototype/startup_world_information.hpp"
#include "dungeon_village_prototype/startup_world_menu.hpp"
#include "dungeon_village_prototype/startup_world_tax.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_prototype/startup_world_projection.hpp"
#include "support/world_fixture.hpp"
#include "dungeon_village_prototype/startup_title_actor_skin.hpp"
#include "dungeon_village_tools/archive.hpp"
#include "dungeon_village_tools/sprite.hpp"
#include "dungeon_village_tools/table.hpp"
#include <raylib.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

int check_steam_startup_skin(const std::filesystem::path &source_root);
int check_steam_facility_skin(const std::filesystem::path &source_root);
int check_steam_human_skin(const std::filesystem::path &source_root);

namespace {
using namespace dungeon_village_prototype;
namespace tools = dungeon_village_tools;
using Package = StartupSkinPackage;
using Part = StartupSkinImage;
// 测试只读已发布素材，不为读文件引入完整归档／存档解码库。
std::vector<std::uint8_t> read_bytes(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("启动皮肤素材读取失败：" + path.string());
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
struct Checks {
    int count{};
    void operator()(bool value, const std::string &message) {
        ++count;
        if (!value)
            throw std::runtime_error("启动皮肤：" + message);
    }
};
void monster_introduction_skin(Checks &check,const std::filesystem::path &root) {
    SteamMonsterSkinOptions options;options.view_y=23;options.title_widths=std::array<int,2>{48,48};
    struct Case {int body,counter,frame,height;bool arrow;};
    for(const auto c:std::array<Case,10>{{{0,0,0,0,false},{0,5,0,0,false},{0,6,1,1,false},
        {1,39,2,46,false},{2,8,1,4,false},{2,31,3,35,false},{3,40,0,48,true},
        {3,54,1,48,true},{3,55,1,48,false},{3,65,2,48,true}}})for(bool japanese:{false,true}) {
        options.japanese=japanese;
        const auto plan=steam_monster_introduction_skin({c.counter,c.body,0,3,"原定义名"},options);
        check(plan&&plan->soft_labels==std::array<int,2>{0,0}&&
            plan->title==(japanese?"新種ﾓﾝｽﾀｰ":"新型怪物"),"89独立日文字面与APK中文适配，原soft0/0");
        int bodies=0,shadows=0,backgrounds=0,arrows=0,texts=0,clips=0,touches=0;
        std::size_t shadow_at=0,body_at=0,clip_at=0,pop_at=0;
        for(std::size_t i=0;i<plan->operations.size();++i) {
            const auto &op=plan->operations[i];
            if(const auto *v=std::get_if<SteamMonsterBody>(&op)) {
                ++bodies;body_at=i;check(v->image==c.body*30&&v->sprite==c.body*4+2&&v->frame==c.frame&&
                    v->position==std::array<int,2>{120,132},"89身体源身份、方向2与源速度帧oracle");
            } else if(const auto *v=std::get_if<SteamHumanImage>(&op)) {
                if(v->asset==SteamHumanAsset::shadow){++shadows;shadow_at=i;check(v->frame==3&&v->position==std::array<int,2>{120,133},"89影子读定义而非体型默认");}
                if(v->asset==SteamHumanAsset::residence_background)++backgrounds;
                if(v->asset==SteamHumanAsset::continue_cursor){++arrows;check(v->position==std::array<int,2>{212,204}&&v->frame==0,"89继续箭头原锚与帧");}
            } else if(const auto *v=std::get_if<SteamHumanClip>(&op)) {
                ++clips;if(v->kind==SteamHumanClipKind::push_intersect){clip_at=i;check(v->rectangle==std::array<int,4>{0,158,240,c.height},"89源整数RateConvert展开窗口");}else pop_at=i;
            } else if(const auto *v=std::get_if<SteamHumanOverviewText>(&op)) {
                check(v->value==(texts==0?std::string("原定义名")+(japanese?"が":""):(japanese?"出現しました!!":"出现了!!"))&&
                    v->rectangle[1]==(texts==0?165:187)&&v->anchor==2,"89名字模板/出现句及顺序");++texts;
            } else if(const auto *v=std::get_if<SteamStartupTouch>(&op)) {
                ++touches;check(i+1==plan->operations.size()&&v->component==2&&v->option==2&&!v->rectangle,"89末Touch2不造全屏热区");
            }
        }
        check(bodies==1&&shadows==1&&backgrounds==3&&arrows==int(c.arrow)&&texts==2&&clips==2&&touches==1&&
            shadow_at<body_at&&body_at<clip_at&&clip_at<pop_at,"89有序图元及固定输出规模");
    }
    constexpr std::array<const char *,4> sizes{"S","M","L","LL"};
    for(int body=0;body<4;++body) {
        const auto seb=tools::parse_legacy_seb(read_bytes(root/"monster"/(std::string("walk_")+sizes[body]+"_02.seb")));
        auto image=LoadImage((root/"monster"/(std::string("monster")+sizes[body]+"00.png")).string().c_str());
        check(image.data&&seb.frame_count==4,"89四体型方向2使用真实已出版四帧SEB/PNG");
        for(const auto &layer:seb.layers)for(const auto &part:layer.parts)
            check(part.source_x>=0&&part.source_y>=0&&part.width>0&&part.height>0&&
                part.source_x+part.width<=image.width&&part.source_y+part.height<=image.height,"89身体裁片没有越出真实图片");
        UnloadImage(image);
    }
    SteamMonsterIntroductionInput good{40,0,0,0,"原定义名"};
    for(int fault=0;fault<6;++fault){auto bad=good;if(fault==0)bad.body=4;if(fault==1)bad.variant=9;
        if(fault==2)bad.shadow=4;if(fault==3)bad.counter=-1;if(fault==4)bad.counter=std::numeric_limits<int>::max();if(fault==5)bad.name.clear();
        check(!steam_monster_introduction_skin(bad,options),"89坏资源/计数/文本明确拒绝");}
    auto state=consume_startup_world_runtime_encounter_request(test_support::world_fixture(),{ref::EncounterCreationRequestKind::page89,3});
    check(state.has_value(),"89皮肤用真实请求生产者");const auto id=state->scripts.pages.back().id;
    check(!steam_monster_introduction_skin(*state,id,options),"89皮肤不替pending执行Init");
    const auto ready=prepare_startup_world_runtime(*state);check(ready.candidate.has_value(),"89皮肤经真实框架初始化");state=ready.candidate;
    const auto before=startup_world_state_digest(*state);
    const auto first=steam_monster_introduction_skin(*state,id,options),again=steam_monster_introduction_skin(*state,id,options);
    check(first&&again&&first->operations.size()==again->operations.size()&&startup_world_state_digest(*state)==before,"89重复查询不消费页面/随机/声音/实体，输出规模稳定");
    auto bad=*state;bad.page_counters.erase(id);check(!steam_monster_introduction_skin(bad,id,options),"89Owner桥拒缺钟，不补默认");
    check(!steam_monster_introduction_skin(*state,id+1,options),"89Owner桥拒陈旧页身份");
    check(acknowledge_startup_world_runtime_page(*state,id)==StartupWorldRuntimeError::none&&
        acknowledge_startup_world_runtime_page(*state,id)==StartupWorldRuntimeError::none&&
        !steam_monster_introduction_skin(*state,id,options),"89已关闭页不能继续产生皮肤");
}
void monster_detail_skin(Checks &check,const std::filesystem::path &root) {
    SteamMonsterSkinOptions options;options.view_y=23;options.title_widths=std::array<int,2>{48,48};
    options.name_width=37;
    const SteamMonsterDetailInput input{31,2,7,3,200,12,3,12,175,4,"原怪物名","真实定义说明"};
    for(const int language:{0,1,2}) {
        options.japanese=language==0;
        options.statistic_labels=language==1?std::optional<std::array<std::string,2>>{{" 	 defeated third","lost heroes extra"}}:
            std::optional<std::array<std::string,2>>{{"no-space","split words"}};
        const auto plan=steam_monster_detail_skin(input,options);
        check(plan&&plan->soft_labels==std::array<int,2>{0,2},"92独立立即返回软标签，不沿89等40确认");
        int levels{},points{},counts{},bodies{},backgrounds{},exp{},money{};
        std::size_t shadow_at{},body_at{},first_count{},money_at{},description_at{};
        std::vector<SteamHumanOverviewText> labels;
        for(std::size_t i=0;i<plan->operations.size();++i) {
            const auto &op=plan->operations[i];
            if(const auto *v=std::get_if<SteamFacilityImage>(&op)) {
                if(v->asset==SteamFacilityAsset::level_label) {++levels;check(v->position==std::array<int,2>{68,70},"92等级标签读取实测名宽加31");}
            } else if(const auto *v=std::get_if<SteamCommerceImage>(&op)) {
                ++points;const auto r=steam_commerce_resource(v->asset);
                check(r&&r->image==31&&r->sprite==44&&std::string(r->published_image)=="steam-common/icon_result00.png"&&
                      v->frame==4&&v->position==std::array<int,2>{language==0?142:166,67},"92点数图必须消费Steam差异图和原frame4");
            } else if(const auto *v=std::get_if<SteamHumanImage>(&op)) {
                if(v->asset==SteamHumanAsset::residence_background){++backgrounds;check(v->position==std::array<int,2>{23,89}&&v->crop==std::array<int,4>{0,0,93,82},"92地板整图，不套89三段铺底");}
                if(v->asset==SteamHumanAsset::shadow){shadow_at=i;check(v->frame==3&&v->position==std::array<int,2>{70,158},"92原定义独立影子");}
                if(v->asset==SteamHumanAsset::experience_label){++exp;check(v->position==std::array<int,2>{42,178}&&v->frame==0,"92经验SEB60 frame0原锚");}
            } else if(const auto *v=std::get_if<SteamMonsterBody>(&op)) {
                ++bodies;body_at=i;check(v->image==67&&v->sprite==9&&v->frame==3&&v->position==std::array<int,2>{70,157},"92方向1与实际body速度，非89方向2");
            } else if(const auto *v=std::get_if<SteamInformationNumber>(&op)) {
                if(counts==0)first_count=i;
                check(v->value==(counts==0?200:12)&&v->kind==(counts==0?SteamInformationNumberKind::monster_count:SteamInformationNumberKind::person_count)&&
                      v->position==std::array<int,2>{210,counts==0?(language==1?104:110):(language==1?138:144)}&&v->japanese==(language==0),
                      "92两统计分段资格共同控制几何，非JP后备仍不加日文单位");
                ++counts;
            } else if(const auto *v=std::get_if<SteamFacilityNumber>(&op)) {
                if(v->position==std::array<int,2>{86,70})check(v->value==3&&v->anchor==1,"92等级数字与文字实测基准同源");
                if(v->position==std::array<int,2>{210,70})check(v->value==4&&v->kind==(language==0?SteamFacilityNumberKind::number:SteamFacilityNumberKind::plus_value),"92点数非JP仍无条件加号");
                if(v->position==std::array<int,2>{95,177})check(v->value==12&&v->kind==SteamFacilityNumberKind::number,"92经验使用普通数字");
                if(v->position==std::array<int,2>{200,177})check(v->value==175&&v->kind==SteamFacilityNumberKind::money,"92金币保留money内部dx减9");
            } else if(const auto *v=std::get_if<SteamMonsterImage>(&op)) {
                const auto resource=steam_monster_resource(v->asset);
                ++money;money_at=i;check(resource&&std::string(resource->group)=="common2"&&
                    resource->sprite==0&&resource->image==1&&v->frame==1&&
                    std::string(resource->published_image)=="original/common2/monster_money.png"&&
                    v->position==std::array<int,2>{147,189},"92原AppData.resCom2身份，不可误绑resWeapon");
            } else if(const auto *v=std::get_if<SteamHumanOverviewText>(&op)) {
                if(v->rectangle[0]==125)labels.push_back(*v);
                if(v->value==input.description){description_at=i;check(v->rectangle==std::array<int,4>{120,198,0,0}&&v->anchor==2&&!v->layout,"92说明为普通居中文字，不造文本框");}
            }
        }
        check(levels==1&&points==1&&counts==2&&bodies==1&&backgrounds==1&&exp==1&&money==1&&
              shadow_at<body_at&&body_at<first_count&&first_count<money_at&&money_at<description_at,"92图元原序及有限资源输出");
        check(labels.size()==(language==1?4U:2U),"92两句均至少两段才用双行布局");
        for(const auto &label:labels)check(label.anchor==0,"92普通DrawString保留源默认anchor0");
        if(language==1)check(labels[0].value.empty()&&labels[1].value.empty()&&labels[2].value=="lost"&&labels[3].value=="heroes"&&
              labels[0].font_size==10,"92Split保留前导/连续空段、trim控制字符、不消费第三段");
        else check(labels[0].value=="倒した数…"&&labels[1].value=="倒された人数…"&&labels[0].font_size==0,
                   "92非JP后备仍取静态日文，不借LT残片或APK中文替换源literal");
    }
    options.japanese=false;options.statistic_labels.reset();
    auto no_level=input;no_level.level=1;options.name_width.reset();
    check(steam_monster_detail_skin(no_level,options).has_value()&&!steam_monster_detail_skin(input,options),
          "92Lv1不消费名宽，Lv2起缺实测宽明确拒绝");
    options.name_width=37;
    const auto coin=tools::parse_legacy_seb(read_bytes(root/"common2/monster_money.seb"));
    auto money_image=LoadImage((root/"common2/monster_money.png").string().c_str());
    int coin_parts{};
    for(const auto &layer:coin.layers)for(const auto &part:layer.parts)if(part.frame==1&&part.image_index>=0) {
        ++coin_parts;check(money_image.data&&money_image.width==28&&money_image.height==18&&
            part.image_index==1&&part.source_x==14&&part.source_y==0&&part.width==14&&part.height==18&&
            part.offset_x==-7&&part.offset_y==-18&&part.source_x+part.width<=money_image.width&&part.height<=money_image.height,
            "92掉钱图原common2裁片及(-7,-18)内部offset独立oracle");
    }
    check(coin_parts==1,"92掉钱只消费已核frame1裁片");UnloadImage(money_image);
    auto state=test_support::page_fixture(92);const auto id=state.scripts.pages.back().id;
    state.scripts.pages.back().lifecycle=0;state.scripts.pages.back().monster_definition=3;
    check(!steam_monster_detail_skin(state,id,options),"92只读桥不初始化pending页");
    const auto initialized=prepare_startup_world_runtime(state);
    check(initialized.candidate.has_value(),"92调用点夹具经真实框架准入");state=*initialized.candidate;
    auto &growth=state.scene.world.world.ai.monster_growth.at(3);
    check(growth.base_death_reward==9&&growth.base_cash_reward==125,"92源表兔子R9/S125独立oracle");
    growth.growth=200;growth.defeats=1;state.scene.world.world.ai.battle.monsters.at(3).human_kills=7;
    const auto before=startup_world_state_digest(state);const auto plan=steam_monster_detail_skin(state,id,options);
    check(plan.has_value(),"92当前定义累计成长/击倒人数可只读投影");
    int values{};
    for(const auto &op:plan->operations) {
        if(const auto *v=std::get_if<SteamFacilityNumber>(&op)) {
            if(v->position==std::array<int,2>{95,177}){++values;check(v->value==12,"92累计200经验140%向零取整，不读实例旧奖励");}
            if(v->position==std::array<int,2>{200,177}){++values;check(v->value==175,"92累计200金币140%独立oracle");}
        } else if(const auto *v=std::get_if<SteamInformationNumber>(&op))
            check(v->value==(v->kind==SteamInformationNumberKind::monster_count?200:7),"92累计k.v与月计k.u分开，倒下人数读k.x");
    }
    check(values==2&&steam_monster_detail_skin(state,id,options)&&startup_world_state_digest(state)==before,
          "92重复皮肤查询不改变当前定义、页面、随机或输出");
    auto boss=state;boss.scripts.pages.back().monster_definition=30;
    boss.scene.world.world.ai.monster_growth.at(30).growth=4;
    const auto boss_plan=steam_monster_detail_skin(boss,id,options);check(boss_plan.has_value(),"92BOSS读取独立成长倍率");
    int boss_values{};
    for(const auto &op:boss_plan->operations)if(const auto *v=std::get_if<SteamFacilityNumber>(&op)) {
        if(v->position==std::array<int,2>{95,177}){++boss_values;check(v->value==144,"92BOSS累计4经验144而非普通成长100%");}
        if(v->position==std::array<int,2>{200,177}){++boss_values;check(v->value==1440,"92BOSS金币基础1000按144%独立oracle");}
        if(v->position==std::array<int,2>{86,70}){++boss_values;check(v->value==5,"92BOSS等级min(count+1,10)不同普通百次一档");}
    }
    check(boss_values==3,"92BOSS等级与两收益均由当前共享定义展示");
    for(int fault=0;fault<5;++fault) {
        auto bad=state;if(fault==0)bad.page_counters.erase(id);if(fault==1)bad.scene.world.world.ai.monster_growth.erase(3);
        if(fault==2)bad.scene.world.world.ai.battle.monsters.erase(3);if(fault==3)bad.scripts.pages.back().lifecycle=4;
        if(fault==4)bad.scene.world.world.ai.monster_growth.at(3).growth=-1;
        check(!steam_monster_detail_skin(bad,id,options),"92坏Owner来源/计数与退休页拒绝，不补演示数值");
    }
}
void monster_notice_skin(Checks &check,const std::filesystem::path &root) {
    SteamMonsterSkinOptions options;options.view_y=23;options.japanese=true;
    options.title_widths=std::array<int,2>{48,48};
    SteamMonsterNoticeInput boss;boss.legacy_page=99;boss.monster={0,0,2,3,"原怪物名"};
    // 独立期望沿PAGES的13段原表：检查起末边界及整数移动，不用被测插值生成oracle。
    struct Case {int counter,x,direction,frame;};
    const std::vector<Case> edges{{0,120,2,0},{4,120,2,0},{5,120,2,0},{6,117,2,1},
        {10,101,2,1},{16,78,2,2},{17,74,2,2},{21,74,2,3},{22,74,2,0},{33,74,2,0},
        {34,74,2,1},{43,74,2,3},{44,74,1,3},{46,81,1,3},{50,97,1,0},{67,162,1,3},
        {68,166,1,3},{72,166,1,0},{73,166,1,0},{84,166,1,0},{85,166,1,2},{94,166,1,3},
        {95,166,2,3},{96,163,2,0},{106,124,2,1},{107,120,2,1},{111,120,2,2},
        {112,120,2,0},{123,120,2,0},{124,120,2,0},{128,120,2,1},{129,120,2,0},{130,120,2,0}};
    for(const auto &c:edges) {
        boss.monster.counter=c.counter;const auto plan=steam_monster_notice_skin(boss,options);
        check(plan&&plan->title=="ボス出現！"&&plan->soft_labels==std::array<int,2>{0,0},"99通知标题与无软标签");
        int bodies{},shadows{};
        for(const auto &op:plan->operations) {
            if(const auto *v=std::get_if<SteamMonsterBody>(&op)) {
                ++bodies;check(v->image==2&&v->sprite==c.direction&&v->frame==c.frame&&
                    v->position==std::array<int,2>{c.x,138},"99十三段边界/129循环/整数移动与最终身体帧，counter="+std::to_string(c.counter));
            } else if(const auto *v=std::get_if<SteamHumanImage>(&op))if(v->asset==SteamHumanAsset::shadow) {
                ++shadows;check(v->frame==3&&v->position==std::array<int,2>{c.x,138},"99阴影沿真实定义且不跟随跳高");
            }
        }
        check(bodies==1&&shadows==1,"99每次只有一个身体及阴影");
    }
    // float32抛物helper的逐槽已核整数高度，三个跳阶段均停最终身体frame0。
    constexpr std::array<int,12> heights{0,2,4,6,7,8,8,7,6,4,2,0};
    for(const auto jump:std::array<std::array<int,3>,3>{{{22,74,2},{73,166,1},{112,120,2}}})
        for(int tick=0;tick<12;++tick) {
            boss.monster.counter=jump[0]+tick;const auto plan=steam_monster_notice_skin(boss,options);
            check(plan.has_value(),"99跳跃逐槽合法");
            int bodies{},shadows{};
            for(const auto &op:plan->operations) {
                if(const auto *v=std::get_if<SteamMonsterBody>(&op)) {
                    ++bodies;check(v->frame==0&&v->sprite==jump[2]&&v->position==std::array<int,2>{jump[1],138-heights[tick]},
                        "99跳跃使用单精度独立高度序列，最终帧停0");
                } else if(const auto *v=std::get_if<SteamHumanImage>(&op))if(v->asset==SteamHumanAsset::shadow) {
                    ++shadows;check(v->position==std::array<int,2>{jump[1],138},"99跳跃不改变地面阴影");
                }
            }
            check(bodies==1&&shadows==1,"99跳跃图元规模固定");
        }
    for(const auto gate:std::array<std::array<int,3>,5>{{{4,1,1},{0,1,0},{4,0,0},{8,5,0},{5,1,1}}})
        for(int aura=0;aura<4;++aura) {
            boss.flags=gate[0];boss.defeats=gate[1];boss.aura_size=aura;boss.monster.counter=27;
            const auto plan=steam_monster_notice_skin(boss,options);check(plan.has_value(),"99aura资格输入合法");
            int auras{};std::size_t aura_at{},shadow_at{},body_at{};
            for(std::size_t i=0;i<plan->operations.size();++i) {
                const auto &op=plan->operations[i];
                if(const auto *v=std::get_if<SteamMonsterImage>(&op)) {
                    ++auras;aura_at=i;const auto r=steam_monster_resource(v->asset);
                    check(r&&std::string(r->group)=="effect"&&r->sprite==13+aura&&r->image==14+aura&&
                        v->frame==1&&v->position==std::array<int,2>{74,132},"99aura用独立auraSize、页钟及跳高，而非累计击杀当图号");
                } else if(const auto *v=std::get_if<SteamHumanImage>(&op)) {
                    if(v->asset==SteamHumanAsset::shadow)shadow_at=i;
                } else if(std::holds_alternative<SteamMonsterBody>(op))body_at=i;
            }
            check(auras==gate[2]&&shadow_at<body_at&&(!auras||aura_at<shadow_at),"99flags4且累计击杀至少1才先aura后阴影身体");
        }
    boss.flags=0;boss.defeats=0;boss.aura_size=0;
    for(const int start:{34,85,124})for(const auto sample:std::array<std::array<int,2>,7>{{{0,0},{2,0},{3,1},{5,1},{6,2},{8,2},{9,2}}}) {
        if(start==124&&sample[0]>=5)continue;
        boss.monster.counter=start+sample[0];const auto plan=steam_monster_notice_skin(boss,options);
        check(plan.has_value(),"99烟效原行合法");int smokes{};std::size_t body_at{},smoke_at{},clip_at{};
        for(std::size_t i=0;i<plan->operations.size();++i) {
            const auto &op=plan->operations[i];
            if(std::holds_alternative<SteamMonsterBody>(op))body_at=i;
            else if(const auto *v=std::get_if<SteamMonsterImage>(&op)) {
                ++smokes;smoke_at=i;check(v->asset==SteamMonsterAsset::smoke&&v->frame==sample[1]&&
                    v->position==std::array<int,2>{start==34?74:start==85?166:120,142},"99烟效3/6/9门槛与末帧封顶");
            } else if(const auto *v=std::get_if<SteamHumanClip>(&op))if(v->kind==SteamHumanClipKind::push_intersect)clip_at=i;
        }
        check(smokes==1&&body_at<smoke_at&&smoke_at<clip_at,"99仅4/8/12段在身体后/文字clip前画烟");
    }
    for(int raw:{99,100})for(const auto timing:std::array<std::array<int,3>,6>{{{5,0,0},{6,1,0},{39,46,0},{40,48,1},{54,48,1},{55,48,0}}}) {
        auto in=boss;in.legacy_page=raw;in.monster.counter=timing[0];
        if(raw==100)in.outbreak=std::array<std::array<std::int32_t,9>,8>{};
        const auto plan=steam_monster_notice_skin(in,options);check(plan.has_value(),"99/100文字和箭头门槛输入");
        int lines{},arrows{},text_clips{},touches{};bool inside{};
        for(std::size_t i=0;i<plan->operations.size();++i) {
            const auto &op=plan->operations[i];
            if(const auto *v=std::get_if<SteamHumanClip>(&op)) {
                if(v->kind==SteamHumanClipKind::push_intersect&&v->rectangle[1]==158) {
                    ++text_clips;inside=true;check(v->rectangle==std::array<int,4>{0,158,240,timing[1]},"99/100独立展开文字clip");
                } else if(v->kind==SteamHumanClipKind::pop)inside=false;
            } else if(const auto *v=std::get_if<SteamHumanOverviewText>(&op)) {
                check(inside&&v->value==(lines==0?"原怪物名が":raw==99?"出現しました!!":"大量発生しています!!")&&
                    v->rectangle==std::array<int,4>{120,lines==0?164:188,0,0}&&v->anchor==2,"99/100两文字原序y164/188，不沿89的165/187");++lines;
            } else if(const auto *v=std::get_if<SteamHumanImage>(&op))if(v->asset==SteamHumanAsset::continue_cursor) {
                ++arrows;check(!inside&&v->position==std::array<int,2>{212,204}&&v->frame==0,"99/100箭头在clip恢复后");
            }
            if(const auto *v=std::get_if<SteamStartupTouch>(&op)) {
                ++touches;check(i+1==plan->operations.size()&&v->component==2&&v->option==2&&!v->rectangle,"99/100末Touch2原无矩形");
            }
        }
        check(lines==2&&text_clips==1&&arrows==timing[2]&&touches==1,"99/100文字/箭头/输出消费门槛");
    }
    SteamMonsterNoticeInput outbreak;outbreak.legacy_page=100;outbreak.monster={31,0,2,3,"原怪物名"};
    outbreak.outbreak=std::array<std::array<std::int32_t,9>,8>{};
    const std::array<int,8> px{-199,7777,199,-99,21999,8888,12000,22000};
    const std::array<int,8> directions{1,1,0,2,1,0,-4,1},states{1,0,2,1,2,0,-1,1};
    for(std::size_t i=0;i<8;++i) {
        auto &row=(*outbreak.outbreak)[i];row[0]=px[i];row[2]=directions[i];row[3]=int(i)*17;row[7]=states[i];
    }
    for(const auto clock:std::array<std::array<int,2>,2>{{{31,1},{36,2}}}) {
        outbreak.monster.counter=clock[0];const auto plan=steam_monster_notice_skin(outbreak,options);
        check(plan&&plan->title=="ﾓﾝｽﾀｰ大量発生","100独立大量发生标题");
        constexpr std::array<int,6> x{-1,1,0,219,120,220},dir{2,1,1,2,1,2};
        int bodies{},shadows{},clips{},effects{};bool clipped{};
        for(const auto &op:plan->operations) {
            if(const auto *v=std::get_if<SteamHumanClip>(&op)) {
                ++clips;if(v->kind==SteamHumanClipKind::push_intersect) {
                    check(!clipped,"100clip不可嵌套遗留");clipped=true;
                    if(clips==1)check(v->rectangle==std::array<int,4>{30,66,180,82},"100八行共用场景clip");
                } else {check(clipped,"100恢复必须匹配clip");clipped=false;}
            } else if(const auto *v=std::get_if<SteamHumanImage>(&op)) {
                if(v->asset==SteamHumanAsset::shadow) {
                    check(clipped&&shadows<6&&shadows==bodies&&v->frame==3&&v->position==std::array<int,2>{x[shadows],139},"100原序每行先阴影，STATE0隐藏且PX/100向零");++shadows;
                }
            } else if(const auto *v=std::get_if<SteamMonsterBody>(&op)) {
                check(clipped&&bodies<6&&shadows==bodies+1&&v->image==2&&v->sprite==dir[bodies]&&
                    v->frame==clock[1]&&v->position==std::array<int,2>{x[bodies],139},"100DIRECT恰1才dir2，最终步帧来自页钟而非各行CNT");++bodies;
            } else if(std::holds_alternative<SteamMonsterImage>(op))++effects;
        }
        check(bodies==6&&shadows==6&&clips==4&&!clipped&&effects==0,"100八行原序/双clip配对，无99的aura或烟");
    }
    for(int fault=0;fault<7;++fault) {
        auto bad=boss;bad.flags=4;bad.defeats=1;
        if(fault==0)bad.legacy_page=98;if(fault==1)bad.monster.counter=-1;
        if(fault==2)bad.monster.counter=std::numeric_limits<int>::max();if(fault==3)bad.aura_size=-1;
        if(fault==4)bad.aura_size=4;if(fault==5)bad.defeats=-1;if(fault==6)bad.legacy_page=100;
        check(!steam_monster_notice_skin(bad,options),"99/100坏页/钟/aura/累计值/缺数组明确拒绝");
    }
    // 仅新增effect资源主责检查；common/monster裁片复用既有89覆盖。
    struct Effect {SteamMonsterAsset asset;const char *name;int image,sprite,frames,w,h,ox,oy;};
    const std::array<Effect,5> effects{{{SteamMonsterAsset::aura0,"aura10",14,13,2,17,23,-8,-23},
        {SteamMonsterAsset::aura1,"aura11",15,14,2,17,31,-8,-31},{SteamMonsterAsset::aura2,"aura12",16,15,2,25,36,-12,-36},
        {SteamMonsterAsset::aura3,"aura13",17,16,2,40,60,-20,-60},{SteamMonsterAsset::smoke,"satSmoke02",8,8,3,38,14,-19,-14}}};
    for(const auto &e:effects) {
        const auto r=steam_monster_resource(e.asset);
        check(r&&std::string(r->group)=="effect"&&r->image==e.image&&r->sprite==e.sprite&&
            std::string(r->published_image)==std::string("original/effect/")+e.name+".png"&&
            std::string(r->published_sprite)==std::string("original/effect/")+e.name+".seb","99effect资源槽与真实出版路径");
        const auto seb=tools::parse_legacy_seb(read_bytes(root.parent_path()/r->published_sprite));
        auto image=LoadImage((root.parent_path()/r->published_image).string().c_str());
        check(image.data&&seb.frame_count==e.frames&&image.width==e.frames*e.w&&image.height==e.h,"99aura两帧/烟三帧真实图幅");
        int parts{};
        for(const auto &layer:seb.layers)for(const auto &part:layer.parts)if(part.image_index>=0) {
            ++parts;check(part.frame>=0&&part.frame<e.frames&&part.image_index==e.image&&
                part.source_x==part.frame*e.w&&part.source_y==0&&part.width==e.w&&part.height==e.h&&
                part.offset_x==e.ox&&part.offset_y==e.oy&&part.source_x+part.width<=image.width&&part.height<=image.height,
                "99真实effect裁片/内部offset与帧界独立oracle");
        }
        check(parts==e.frames,"99effect每帧仅一有效裁片");UnloadImage(image);
    }
    check(!steam_monster_resource(static_cast<SteamMonsterAsset>(99)),"怪物未知资源拒绝");
    for(int raw:{99,100}) {
        // 调用点夹具不伪装自然发生；通过真实确认与Update创建演出标记/计数和100数组。
        auto state=test_support::page_fixture(raw);const auto id=state.scripts.pages.back().id;
        state.scripts.pages.back().monster_definition=3;
        check(!steam_monster_notice_skin(state,id,options),"99/100只读桥不补初始化标记和主钟");
        check(acknowledge_startup_world_runtime_page(state,id)==StartupWorldRuntimeError::none,"99/100真实早确认初始化演出，不关闭");
        const auto ready=prepare_startup_world_runtime(state);check(ready.candidate.has_value(),"99/100真实框架Update准入");state=*ready.candidate;
        const auto before=startup_world_state_digest(state);const auto first=steam_monster_notice_skin(state,id,options);
        const auto again=steam_monster_notice_skin(state,id,options);
        check(first&&again&&first->operations.size()==again->operations.size()&&startup_world_state_digest(state)==before,
            "99/100重复只读不推进演出/世界/随机/输出，有限图元规模稳定");
        auto wrapped=state;wrapped.page_counters.at(id)=std::numeric_limits<int>::max()-1;
        const auto wrapped_draws=wrapped.scene.random.draws();const auto rollover=prepare_startup_world_runtime(wrapped);
        check(rollover.candidate&&rollover.candidate->page_counters.count(id)&&rollover.candidate->page_counters.at(id)==0&&
            rollover.candidate->task_display_initialized.count(id)&&rollover.candidate->scene.random.draws()==wrapped_draws&&
            steam_monster_notice_skin(*rollover.candidate,id,options),
            "99/100主钟MAX-1经真实Update回卷0，不重复Init演出或重抽19票");
        auto missing_clock=state;missing_clock.page_counters.erase(id);
        const auto missing_before=startup_world_state_digest(missing_clock);
        check(acknowledge_startup_world_runtime_page(missing_clock,id)!=StartupWorldRuntimeError::none&&
            !prepare_startup_world_runtime(missing_clock).candidate&&startup_world_state_digest(missing_clock)==missing_before,
            "99/100已Init却缺主钟时确认/Update均拒绝，不用map[]补0或留下部分状态");
        for(int fault=0;fault<9;++fault) {
            auto bad=state;if(fault==0)bad.page_counters.erase(id);if(fault==1)bad.task_display_initialized.erase(id);
            if(fault==2)bad.scripts.pages.back().monster_definition.reset();if(fault==3)bad.scripts.pages.back().monster_definition=999;
            if(fault==4)bad.scripts.pages.back().lifecycle=4;if(fault==5)bad.scripts.pages.back().legacy_page=98;
            if(fault==6)bad.page_counters.at(id)=-1;if(fault==7)bad.scripts.pages.back().lifecycle=0;
            if(fault==8)bad.scene.world.world.ai.monster_growth.erase(3);
            check(!steam_monster_notice_skin(bad,id,options),"99/100Owner桥拒缺钟/标记/定义、坏页/钟及未Init/退休");
        }
        if(raw==100) {
            auto bad=state;bad.task_display_table[0][2]=2;
            check(!steam_monster_notice_skin(bad,id,options),"100Owner拒坏方向，独立于纯Draw的恰1/其它方向谓词");
            bad=state;bad.task_display_table[0][7]=-1;
            check(!steam_monster_notice_skin(bad,id,options),"100Owner拒坏状态，独立于纯Draw的STATE非零可见谓词");
        }
        check(!steam_monster_notice_skin(state,id+1,options),"99/100Owner桥拒陈旧页身份");
        const auto draws=state.scene.random.draws();const auto actors=state.scene.world.world.ai.battle.actors.size();
        const auto order=state.scene.world.world.ai.monster_order;const auto table=state.task_display_table;
        state.page_counters.at(id)=40;
        check(acknowledge_startup_world_runtime_page(state,id)==StartupWorldRuntimeError::none&&
            !steam_monster_notice_skin(state,id,options)&&
            state.scene.random.draws()==draws&&state.scene.world.world.ai.battle.actors.size()==actors&&
            state.scene.world.world.ai.monster_order==order&&state.task_display_table==table,
            "99/100满40真实确认标记退休后皮肤拒绝，不抽票、不增怪物、不改全局演出历史");
        // ack标生命周期4；计数/标记由下一框架Finish释放，暂停父世界以隔离退休职责。
        state.scene.framework_paused=true;const auto finished=prepare_startup_world_runtime(state);
        check(finished.candidate&&!finished.candidate->task_display_initialized.count(id)&&
            !finished.candidate->page_counters.count(id)&&!steam_monster_notice_skin(*finished.candidate,id,options)&&
            finished.candidate->scene.random.draws()==draws&&finished.candidate->scene.world.world.ai.battle.actors.size()==actors&&
            finished.candidate->scene.world.world.ai.monster_order==order&&finished.candidate->task_display_table==table,
            "99/100真实框架Finish释放退休页计数与初始化引用，暂停父世界不留下演出副作用");
    }
}
void income_information(Checks &check) {
    // 条件夹具只检验raw36统计口径，不声称这些现金桶由自然经营获得。
    StartupInformationCash buckets{};
    buckets[2] = {{{1200,250},{9,50},{0,15},{1800,2000},{7,800}}};
    buckets[11] = {{{50,9},{5,6},{700,60},{10,90},{11,12}}};
    const auto before = buckets;
    const auto month = startup_income_information(buckets,2,0);
    const auto year = startup_income_information(buckets,2,1);
    check(month && year,"收支月页和年页接受Owner形状的只读桶");
    constexpr std::array<std::string_view,5> labels{{"设施","怪物","冒险者","商店","其它"}};
    constexpr std::array<std::array<int,2>,5> monthly{{{1200,250},{9,50},{0,15},{1800,2000},{7,800}}};
    constexpr std::array<std::array<int,2>,5> yearly{{{1250,259},{14,56},{700,75},{1810,2090},{18,812}}};
    for(std::size_t row=0;row<5;++row) {
        check(month->rows[row].label==labels[row] && year->rows[row].label==labels[row],
              "五类按原标签序，商店和其它不能借前三类月报省略");
        check(month->rows[row].income==monthly[row][0] && month->rows[row].expense==monthly[row][1],
              "月页仅当前月收入与支出，不混前月或净额");
        check(year->rows[row].income==yearly[row][0] && year->rows[row].expense==yearly[row][1],
              "年页读取全部12桶，不能只累加到当前月份");
    }
    check(month->profit==-99 && month->profit_text=="-99Ｇ" && year->profit==500 && year->profit_text=="500Ｇ",
          "利润独立oracle包含全部五类，负号与全角单位保留");
    check(month->rows[0].income_text=="1,200Ｇ" && month->rows[3].expense_text=="2,000Ｇ" &&
          month->rows[2].income_text=="0Ｇ","收入支出均千位分组，零金额仍显示数字");
    const auto empty = startup_income_information(buckets,0,0);
    check(empty && empty->profit==0 && empty->rows[0].income_text=="0Ｇ","未记账月按原桶显示零");
    for(const auto input:std::array<std::array<int,2>,6>{{{-1,0},{12,0},{-1,1},{12,1},{2,-1},{2,2}}})
        check(!startup_income_information(buckets,input[0],input[1]),"坏月份或页签显式拒绝，年页也校验月份");
    check(buckets==before,"查询与拒绝都不修改原120个桶，无副本回写");
    // 独立32位边界oracle；不调用实现的回卷/格式帮助函数生成期望。
    StartupInformationCash overflow{};
    overflow[0][0][0]=2147483647;
    overflow[11][0][0]=1;
    auto result = startup_income_information(overflow,0,1);
    check(result && result->rows[0].income==(-2147483647-1) && result->profit==(-2147483647-1) &&
          result->rows[0].income_text=="-2,147,483,648Ｇ" && result->profit_text=="-2,147,483,648Ｇ",
          "年度int回卷后安全扩到long显示，最小负值无C++取负溢出");
    overflow[0][1][1]=1;
    result=startup_income_information(overflow,0,1);
    check(result && result->profit==2147483647 && result->profit_text=="2,147,483,647Ｇ",
          "利润减支出亦按Java int回卷，不提升为64位年度业务合计");
    overflow[0][0][0]=0;
    overflow[11][0][0]=0;
    overflow[0][1][1]=(-2147483647-1);
    result=startup_income_information(overflow,0,0);
    check(result && result->rows[1].expense_text=="-2,147,483,648Ｇ" && result->profit==(-2147483647-1),
          "负桶边界只用于显示语义测试，不放宽世界业务写入校验");
}
void income_skin(Checks &check,const std::filesystem::path &source_root) {
    using Role=SteamInformationTextRole;
    // 只准备页面与现金桶表现条件；业务累计与金额回卷已有独立主责测试。
    auto owner=test_support::page_fixture(36);
    owner.scripts.pages.back().lifecycle=0; // 公共夹具默认ready；本场景须先走真实Init。
    const auto id=owner.scripts.pages.back().id;
    check(initialize_startup_world_information_pages(owner),"收支皮肤夹具使用实际初始化");
    owner.monthly_cash={};owner.monthly_cash[owner.scene.calendar.month][0]={1200,1300};
    owner.monthly_cash[(owner.scene.calendar.month+1)%12][1]={500,0};
    SteamInformationSkinOptions options{0,false,std::array<int,2>{80,82}};
    const auto before=startup_world_state_digest(owner);
    const auto result=steam_income_information_skin(owner,id,options);
    check(result && startup_world_state_digest(owner)==before,"完整收支计划保持Owner与输出不变");
    const auto &plan=*result;
    const auto labels=[&](const SteamInformationSkinPlan &p,Role role) {
        std::vector<const SteamInformationText*> out;
        for(const auto &draw:p.draws)
            if(const auto *label=std::get_if<SteamInformationText>(&draw);label && label->role==role)
                out.push_back(label);
        return out;
    };
    const auto titles=labels(plan,Role::title);
    check(titles.size()==2 && titles[0]->position==std::array<int,2>{80,35} &&
              titles[1]->position==std::array<int,2>{79,34} && titles[0]->period==0,
          "标题阴影与正文保留两次实际测宽和原位置，不能合并测宽");
    const auto period=labels(plan,Role::period),income_head=labels(plan,Role::income_header),
               expense_head=labels(plan,Role::expense_header);
    check(period.size()==1 && period[0]->position==std::array<int,2>{26,65} && period[0]->font_size==0 && !period[0]->anchor &&
              income_head.size()==1 && income_head[0]->position==std::array<int,2>{108,65} &&
              income_head[0]->font_size==10 && expense_head.size()==1 &&
              expense_head[0]->position==std::array<int,2>{178,65} && expense_head[0]->font_size==10,
          "非日文页签先用当前字体，仅收入支出表头临时字号10");
    int money_count{},category_count{},line_count{};
    for(std::size_t index=0;index<plan.draws.size();++index) {
        if(const auto *text=std::get_if<SteamInformationText>(&plan.draws[index])) {
            if(!text->value.empty())++money_count;
            if(text->role==Role::category) {
                const int row=category_count++,y=90+18*row;
                check(text->slot==row && text->position==std::array<int,2>{22,y},"五行分类原序与18步距");
                const std::size_t first_amount=index+(row<4?2:1);
                const auto *income=std::get_if<SteamInformationText>(&plan.draws.at(first_amount));
                const auto *expense=std::get_if<SteamInformationText>(&plan.draws.at(first_amount+1));
                check(income && expense && income->role==Role::income && expense->role==Role::expense &&
                          income->anchor==4 && expense->anchor==4 &&
                          income->position==std::array<int,2>{143,y} && expense->position==std::array<int,2>{216,y},
                      "标签/前四行线/收入/支出保持绘制次序与右锚");
            }
        } else if(const auto *line=std::get_if<SteamInformationLine>(&plan.draws[index])) {
            const int row=line_count++;
            const int y=row<4?104+18*row:180;
            check(line->from==std::array<int,2>{22,y} && line->to==std::array<int,2>{220,y} && line->width==1,
                  "只四条分类线与一条利润线，保留源端点/线宽");
        }
    }
    check(money_count==11 && category_count==5 && line_count==5,"11个金额全部Font文字，没有数字SEB或第六分类");
    const auto incomes=labels(plan,Role::income),expenses=labels(plan,Role::expense),profit=labels(plan,Role::profit_value);
    check(incomes[0]->value=="1,200Ｇ" && expenses[0]->value=="1,300Ｇ" && profit.size()==1 &&
              profit[0]->value=="-100Ｇ" && profit[0]->rgb==std::array<int,3>{255,14,1},
          "Owner格式金额直达计划，负利润保留负号与红色");
    const auto description=labels(plan,Role::description);
    check(description.size()==1 && description[0]->position==std::array<int,2>{120,204} &&
              description[0]->anchor==2 && plan.soft_labels==std::array<int,2>{0,2} && plan.touches.size()==2,
          "底部只居中文字，右软标签返回，仅两箭头注册触摸");
    for(std::size_t side=0;side<2;++side) {
        const auto &touch=plan.touches[side];
        check(touch.component==1 && touch.value==(side==0?16:18) && touch.image_draw && !touch.rectangle,
              "箭头保留原SEB帮助器注册，不猜物理热区");
        const auto &image=std::get<SteamManualArrowRequest>(plan.draws.at(*touch.image_draw));
        check(image.bounds_frame==(side==0?3:0) && !image.selected &&
                  image.position==std::array<int,2>{side==0?22:218,51},"箭头原SEB3 bounds帧/初始锚，实际贴图由GameView helper消费");
    }
    owner.page_phases.at(id)=1;owner.page_counters.at(id)=8;options.japanese=true;options.view_y=20;
    const auto year=*steam_income_information_skin(owner,id,options);
    check(labels(year,Role::period)[0]->position[0]==28 && labels(year,Role::income_header)[0]->position[0]==121 &&
              labels(year,Role::income_header)[0]->font_size==0 && labels(year,Role::expense_header)[0]->position[0]==194 &&
              labels(year,Role::profit_value)[0]->value=="400Ｇ" &&
              labels(year,Role::profit_value)[0]->rgb==std::array<int,3>{0,100,255},
          "日文分支原表头与年统计正利润，未借非日文临时字体");
    check(labels(year,Role::title)[0]->position[1]==45 && labels(year,Role::category)[0]->position[1]==90 &&
              std::get<SteamManualArrowRequest>(year.draws.at(*year.touches[0].image_draw)).position==std::array<int,2>{19,51} &&
              std::get<SteamManualArrowRequest>(year.draws.at(*year.touches[1].image_draw)).position==std::array<int,2>{221,51},
          "VIEW_Y只进入窗口/内框帮助器，正文与16槽箭头不重复叠偏移");
    owner.scripts.pages.back().lifecycle=3;
    check(steam_income_information_skin(owner,id,options).has_value(),"挂起年页可只读绘制");
    auto bad=owner;bad.page_phases.erase(id);
    check(!steam_income_information_skin(bad,id,options),"初始化页缺页签拒绝，不制造默认月页");
    options.title_widths.reset();check(!steam_income_information_skin(owner,id,options),"缺真实标题双测宽不猜字符宽");
    options.title_widths=std::array<int,2>{-1,80};
    check(!steam_income_information_skin(owner,id,options),"负测宽拒绝");
    options.title_widths=std::array<int,2>{80,80};options.view_y=std::numeric_limits<int>::max();
    check(!steam_income_information_skin(owner,id,options),"窗框中间算术溢出不发布部分图元");
    const auto seb=tools::parse_legacy_seb(read_bytes(source_root/"common/arrow02.seb"));
    int arrow_parts{};
    for(const auto &layer:seb.layers)for(const auto &part:layer.parts)if(part.frame==0||part.frame==3) {
        ++arrow_parts;
        check(part.image_index==74 && part.source_x==(part.frame==0?4:8) && part.source_y==0 &&
                  part.width==4 && part.height==8 && part.offset_x==0 && part.offset_y==-3,
              "已出版箭头实际裁片和内部offset与Steam调用独立对应");
    }
    check(arrow_parts==2,"两个实际消费的箭头帧都存在，不能空遍历通过");
}
// CPU图像持有者只负责本批分配，失败也释放，不创建窗口／纹理或保留静态缓存。
struct CpuImage {
    Image image{};
    explicit CpuImage(Image value) : image(value) {
        if (!image.data)
            throw std::runtime_error("启动皮肤CPU素材无法解码");
    }
    CpuImage(const CpuImage &) = delete;
    CpuImage &operator=(const CpuImage &) = delete;
    ~CpuImage() { UnloadImage(image); }
};
bool same(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
Rectangle rectangle(const std::array<int, 4> &r) {
    return {static_cast<float>(r[0]), static_cast<float>(r[1]),
            static_cast<float>(r[2]), static_cast<float>(r[3])};
}
std::map<int, std::filesystem::path> image_table(const std::filesystem::path &root,
                                               const char *package, Checks &check) {
    std::map<int, std::filesystem::path> images;
    const auto rows = tools::parse_tsv(read_bytes(root / package / "img.inf"));
    for (std::size_t n = 0; n < rows.size(); ++n) {
        const auto &row = rows[n];
        check(row.size() == (std::string(package) == "title" ? 1U : 2U), "原图片索引列数");
        const int id = row.size() == 1 ? static_cast<int>(n) : tools::parse_table_integer(row[0]);
        std::filesystem::path name(row.back());
        check(!name.empty() && !name.is_absolute() && !name.has_parent_path(), "原图片名称是单文件");
        name.replace_extension(".png");
        check(images.emplace(id, root / package / name).second, "原图片索引无重复");
    }
    return images;
}
struct ImageOracle {
    Part part;
    Package package;
    int id;
    const char *filename;
    std::array<int, 2> dimensions;
    std::array<int, 4> crop;
    std::array<int, 2> anchor;
    const char *sha256;
};
// 来自原img.inf、具名页面绘制及冻结PNG，不能用被测查询生成期望。
constexpr std::array<ImageOracle, 8> images{{
    {Part::title_background,Package::title,0,"title00.png",{240,330},{0,0,240,330},{0,0},"bfd506538ad546e671e33519a51b23941a09d6cb98c4398b688df5afab85f3a5"},
    {Part::title_menu,Package::title,1,"title_window.png",{98,68},{0,0,98,68},{0,0},"65e082c57f226bd516033390556e669eca2815bdc90f55bf7010054ec7dacdc4"},
    {Part::title_logo,Package::title,2,"title_logo.png",{236,115},{0,0,236,115},{0,0},"9e8e90d9b1ed405c2019d90555f8da275d54df61502e42c6c1de18b4ddf72d97"},
    {Part::title_cursor,Package::title,3,"title_cursor.png",{28,23},{0,0,28,23},{0,0},"09b2e1fb2e2d4747d74d1b4b6e1ee25a86af62f1d13f43f0d8cfc7480c5deb82"},
    {Part::title_grass,Package::title,4,"title_grass.png",{240,18},{0,0,240,18},{0,0},"ceacff087df46f74ccf116b54281ba0decbbc306dfb7dac0be6326d70a1c21b0"},
    {Part::record_background,Package::event,9,"event_backGlad.png",{180,80},{0,0,180,80},{30,78},"5f95316883cbc9a3aebcd990f48645edd73dbd156a668be0a2dafb9fe898f370"},
    {Part::new_game_background,Package::event,9,"event_backGlad.png",{180,80},{0,9,180,63},{30,65},"5f95316883cbc9a3aebcd990f48645edd73dbd156a668be0a2dafb9fe898f370"},
    {Part::clear_background,Package::event,13,"event_medelCelemony_back2.png",{180,80},{0,0,180,80},{30,105},"4f8d81b2bd13260c03cd2eaf0ed731e8eb8ed7d6f4ae9b2805f2907c00df1d52"}
}};
struct AssetTables {
    std::map<int, std::filesystem::path> title, event, common;
    std::vector<std::filesystem::path> sprites;
};
AssetTables tables(const std::filesystem::path &root, Checks &check) {
    AssetTables result;
    result.title = image_table(root,"title",check);
    result.event = image_table(root,"event",check);
    result.common = image_table(root,"common",check);
    for (const auto &row : tools::parse_tsv(read_bytes(root/"common/seb.inf"))) {
        check(row.size() == 1 && !std::filesystem::path(row[0]).has_parent_path(), "原SEB目录一行一个名称");
        result.sprites.push_back(root/"common"/row[0]);
    }
    return result;
}
void draw(Image &destination, const Image &source, const std::array<int,4> &crop, int x, int y) {
    ImageDraw(&destination,source,rectangle(crop),
              {static_cast<float>(x),static_cast<float>(y),static_cast<float>(crop[2]),
               static_cast<float>(crop[3])},WHITE);
}
void static_images(const AssetTables &assets, Checks &check, Image *contact_sheet) {
    const auto *table = &assets.title;
    for (std::size_t index=0;index<images.size();++index) {
        const auto &expected=images[index];
        table=expected.package==Package::title?&assets.title:&assets.event;
        const auto found=table->find(expected.id);
        check(found!=table->end() && found->second.filename()==expected.filename,
              "原包序映射到指定PNG文件，不能把event9当common9");
        check(tools::sha256_hex(read_bytes(found->second))==expected.sha256,
              "静态图片冻结SHA256");
        const auto plan=startup_skin_image(expected.part);
        check(plan && plan->package==expected.package && plan->image==expected.id &&
                  plan->sprite==-1 && plan->frame==0 && plan->layer==0 &&
                  plan->crop==expected.crop && plan->offset==expected.anchor,
              "八个原图请求保留身份、裁片和独立页面锚点");
        CpuImage source(LoadImage(found->second.string().c_str()));
        check(source.image.width==expected.dimensions[0] && source.image.height==expected.dimensions[1],
              "实际PNG尺寸符合独立原素材oracle");
        check(plan->crop[0]>=0 && plan->crop[1]>=0 && plan->crop[2]>0 && plan->crop[3]>0 &&
                  plan->crop[0]+plan->crop[2]<=source.image.width &&
                  plan->crop[1]+plan->crop[3]<=source.image.height,"静态裁片实际PNG边界");
        CpuImage cropped(ImageFromImage(source.image,rectangle(plan->crop)));
        check(cropped.image.width==expected.crop[2] && cropped.image.height==expected.crop[3],
              "CPU原尺寸裁剪不缩放");
        int opaque{};
        for(int y=0;y<cropped.image.height;++y)
            for(int x=0;x<cropped.image.width;++x) {
                const auto actual=GetImageColor(cropped.image,x,y);
                check(same(actual,GetImageColor(source.image,x+expected.crop[0],y+expected.crop[1])),
                      "静态图块每个RGBA像素保持实际源裁片");
                opaque+=actual.a>0;
            }
        check(opaque>0,"原图CPU裁片非空");
        if(contact_sheet) {
            // 素材拼图布局是研究展示坐标；不把它标成原窗口或完整原皮肤。
            constexpr std::array<std::array<int,2>,8> positions{{
                {12,12},{264,12},{264,92},{264,220},{264,258},{12,356},{204,356},{396,356}}};
            draw(*contact_sheet,cropped.image,{0,0,cropped.image.width,cropped.image.height},
                 positions[index][0],positions[index][1]);
        }
    }
    for(const int invalid : {-1,8,9999,std::numeric_limits<int>::max()})
        check(!startup_skin_image(static_cast<Part>(invalid)),"坏静态资源枚举显式拒绝");
}
void phases(Checks &check) {
    // 原阶段门槛、两角色原序、四步相位是literal oracle，不回读实现的threshold数组。
    constexpr std::array<int,8> limits{{75,75,45,45,65,45,65,9999}};
    constexpr std::array<std::array<int,3>,7> frames{{
        {0,5,4},{3,5,4},{4,7,6},{7,7,6},{8,5,4},{15,7,6},{std::numeric_limits<int>::max(),7,6}}};
    for(int stage=0;stage<8;++stage) {
        for(const auto &sample:frames) {
            const auto skin=startup_clear_skin(stage,0,sample[0]);
            check(skin && !skin->continue_marker,"合法阶段零计数不提前给继续标记");
            check(skin->background.package==Package::event && skin->background.image==13 &&
                      skin->background.crop==std::array<int,4>{0,0,180,80} &&
                      skin->background.offset==std::array<int,2>{30,105},"计分背景独立源身份");
            const auto &right=skin->actors[0], &left=skin->actors[1];
            check(right.package==Package::common && right.image==171 && right.sprite==46 && right.layer==0 &&
                      right.frame==(stage==4?sample[1]:1) && right.offset==std::array<int,2>{190,181} &&
                      left.package==Package::common && left.image==172 && left.sprite==31 && left.layer==0 &&
                      left.frame==(stage==4?sample[2]:0) && left.offset==std::array<int,2>{51,181},
                  "计分角色保持右大臣先于左秘书及页面计数四步换帧");
        }
        for(int age : {0,20,21,39,40,60,61,79,80,std::numeric_limits<int>::max()})
            for(int counter : {limits[stage]-1,limits[stage]}) {
                const auto skin=startup_clear_skin(stage,counter,age);
                const bool expected=(stage==0||stage==3||stage==6) && counter==limits[stage] &&
                                     (age==21||age==39||age==61||age==79);
                check(skin && skin->continue_marker.has_value()==expected,
                      "阶段恰满与页面20/21、39/40分开控制继续提示");
                if(expected) {
                    const auto &arrow=*skin->continue_marker;
                    check(arrow.package==Package::common && arrow.image==72 && arrow.sprite==2 &&
                              arrow.frame==0 && arrow.layer==0 &&
                              arrow.offset==std::array<int,2>{213,stage==6?167:179},
                          "仅三个原阶段满门槛给原frame0及6阶段独立Y锚");
                }
            }
        check(!startup_clear_skin(stage,-1,0) && !startup_clear_skin(stage,limits[stage]+1,0) &&
                  !startup_clear_skin(stage,0,-1) && !startup_clear_skin(stage,std::numeric_limits<int>::max(),21),
              "坏阶段计数在隐藏相位也拒绝，不夹到合法门槛");
    }
    for(int stage : {-1,8,std::numeric_limits<int>::max()})
        check(!startup_clear_skin(stage,0,0),"坏阶段枚举拒绝，不越界查门槛");
    const auto maximum=startup_clear_skin(4,65,std::numeric_limits<int>::max());
    check(maximum && maximum->actors[0].frame==7 && maximum->actors[1].frame==6 &&
              !maximum->continue_marker,"页面INT_MAX合法取模，无计数增长或溢出");
}
void steam_clear_score_skin_checks(Checks &check) {
    // 仅绘制条件夹具：六数量/六得分是独立字面，不构造自然通关或第二套计分消费者。
    StartupClearScoreRows rows{{{1234,5678},{2,20},{3,30},{4,40},{5,50},{6,60}}};
    SteamClearScoreInput in;in.rows=rows;in.sum=1234567890123LL;in.captured_high_score=9876543210LL;
    SteamClearScoreOptions options;options.title_widths=std::array<int,2>{84,86};options.score_widths={12,14};
    auto body=[](const SteamClearScorePlan &p) {
        std::vector<SteamHumanOverviewText> result;
        for(const auto &op:p.operations)if(const auto *t=std::get_if<SteamHumanOverviewText>(&op))
            if(t->rectangle[1]>=105&&t->rectangle[1]<=127)result.push_back(*t);
        return result;
    };
    auto images=[](const SteamClearScorePlan &p) {
        std::vector<SteamHumanImage> result;
        for(const auto &op:p.operations)if(const auto *v=std::get_if<SteamHumanImage>(&op))result.push_back(*v);
        return result;
    };
    auto words=[&](const SteamClearScorePlan &p) {
        std::vector<std::string> result;for(const auto &t:body(p))result.push_back(t.value);return result;
    };
    for(int stage=0;stage<8;++stage)for(int phase:{14,15,39,40,44,45}) {
        in.stage=stage;in.stage_counter=phase;in.main_counter=4;
        const auto plan=steam_clear_score_skin(in,options);check(bool(plan),"Steam17八阶段阈值条件可绘");
        std::vector<std::string> expected;
        if(stage==0){if(phase>=15)expected.push_back("那么赶紧开始计算");if(phase>=45)expected.push_back("通关点数吧。");}
        if(stage==1){if(phase>=15)expected.push_back("街道人气");if(phase>=45)expected.push_back("1,234");}
        if(stage==2&&phase>=15)expected.push_back("通关点数…");
        if(stage==3&&phase>=15)expected={"5,678","点追加!!"};
        if(stage==4)expected={"排行榜更新！"};
        if(stage==5&&phase>=15)expected.push_back("接下来…");
        if(stage==6){if(phase>=15)expected.push_back("通关点数的计算");if(phase>=40)expected.push_back("就是这样。");}
        check(words(*plan)==expected,"Steam17阶段钟独立决定正文，14/15、39/40、44/45不串阶段");
        for(const auto &t:body(*plan))check(!t.layout&&!t.rich&&t.font_size==0,
            "Steam17正文普通Font，数量空单位不变成SEB数字或TextLayout");
        const auto parts=images(*plan);
        check(parts.size()==3&&
            parts[0].asset==SteamHumanAsset::ceremony_background&&parts[0].position==std::array<int,2>{30,105}&&
            parts[0].crop==std::array<int,4>{0,0,180,80}&&
            parts[1].asset==SteamHumanAsset::president&&parts[1].position==std::array<int,2>{190,181}&&
            parts[1].frame==(stage==4?7:1)&&parts[2].asset==SteamHumanAsset::secretary&&
            parts[2].position==std::array<int,2>{51,181}&&parts[2].frame==(stage==4?6:0),
            "Steam17出版背景及大臣先秘书的资源锚，演员只读独立主钟");
        check(plan->soft_labels==std::array<int,2>{0,0},"Steam17源空软标签，不借管理页取消");
        const auto *touch=std::get_if<SteamStartupTouch>(&plan->operations.back());
        check(touch&&touch->component==2&&touch->value==0&&touch->option==2&&!touch->rectangle&&!touch->image_draw,
            "Steam17末尾全局Touch2保留option2，无自造按钮矩形");
    }
    // 底层phases已负责全面主钟取模；完整计划仅补角色/箭头的独立接线。
    for(const auto sample:std::array<std::array<int,4>,5>{{{0,74,21,0},{0,75,20,0},{0,75,21,1},{3,46,21,0},{6,65,39,1}}}) {
        in.stage=sample[0];in.stage_counter=sample[1];in.main_counter=sample[2];
        const auto plan=steam_clear_score_skin(in,options);check(bool(plan),"Steam17箭头接线条件绘制");
        const auto parts=images(*plan);const auto count=std::count_if(parts.begin(),parts.end(),[](const auto &p){
            return p.asset==SteamHumanAsset::continue_cursor;});
        check(count==sample[3],"Steam17箭头phase恰门槛且main余数大于20，不夹限或沿阶段钟闪烁");
        if(count)check(parts.back().position==std::array<int,2>{213,sample[0]==6?167:179}&&parts.back().frame==0,
            "Steam17第六阶段独立箭头167，其余179");
    }
    in.stage=3;in.stage_counter=15;in.main_counter=8;
    auto plan=*steam_clear_score_skin(in,options);const auto split=body(plan);
    check(split[0].rectangle==std::array<int,4>{90,117,0,0}&&split[1].rectangle==std::array<int,4>{104,117,0,0}&&
        split[0].anchor==0&&split[1].anchor==0&&split[0].rgb==std::array<int,3>{0,100,255}&&
        split[1].rgb==std::array<int,3>{92,51,31},"Steam17加分用固定留宽48，两次测宽12/14，先蓝数字再棕后缀");
    std::array<std::size_t,5> layers{};layers.fill(plan.operations.size());
    for(std::size_t index=0;index<plan.operations.size();++index) {
        const auto &op=plan.operations[index];
        if(const auto *frame=std::get_if<SteamStartupFrameDraw>(&op))if(const auto *r=std::get_if<StartupSkinRect>(frame)) {
            if(!r->outline&&r->rect==std::array<int,4>{17,81,207,96})layers[0]=index;
            if(!r->outline&&r->rect==std::array<int,4>{21,85,197,52})layers[2]=index;
        }
        if(const auto *image=std::get_if<SteamHumanImage>(&op)) {
            if(image->asset==SteamHumanAsset::ceremony_background)layers[1]=index;
            if(image->asset==SteamHumanAsset::president)layers[3]=index;
            if(image->asset==SteamHumanAsset::secretary)layers[4]=index;
        }
    }
    check(layers[4]<plan.operations.size()&&layers[0]<layers[1]&&layers[1]<layers[2]&&layers[2]<layers[3]&&layers[3]<layers[4],
        "Steam17外box→180×80背景→内box覆盖→大臣→秘书，不能把框层整体提前");
    for(const auto oracle:std::array<std::array<int,3>,4>{{{static_cast<int>(SteamHumanAsset::ceremony_background),13,-1},
        {static_cast<int>(SteamHumanAsset::president),171,46},{static_cast<int>(SteamHumanAsset::secretary),172,31},
        {static_cast<int>(SteamHumanAsset::continue_cursor),72,2}}}) {
        const auto resource=steam_human_resource(static_cast<SteamHumanAsset>(oracle[0]));
        check(resource&&resource->image==oracle[1]&&resource->sprite==oracle[2],"Steam17已出版图像/SEB独立身份，沿现资源表不造副本");
    }
    constexpr std::array<const char*,6> names{"街道人气","任务完成数","设施发现数","冒险者全员LV合计","自宅数","努力度总和"};
    in.stage=1;in.stage_counter=45;
    for(int row=0;row<6;++row){in.row=row;check(body(*steam_clear_score_skin(in,options))[0].value==names[row],
        "Steam17六类名称按固定定义原序，非当前世界统计口径");}
    in.row=6;in.stage=7;in.stage_counter=9999;in.rows.reset();options.score_widths={};
    for(bool english:{false,true}) {
        options.english=english;plan=*steam_clear_score_skin(in,options);
        std::vector<SteamHumanOverviewText> labels;
        for(const auto &op:plan.operations)if(const auto *t=std::get_if<SteamHumanOverviewText>(&op))labels.push_back(*t);
        check(body(plan).empty()&&labels.size()==4&&labels[0].rectangle==std::array<int,4>{english?33:93,78,0,0}&&
            labels[0].anchor==(english?1:4)&&labels[1].value=="1,234,567,890,123点"&&
            labels[1].rectangle==std::array<int,4>{213,78,0,0}&&labels[1].anchor==4&&
            labels[2].rectangle==std::array<int,4>{93,192,0,0}&&labels[2].rgb==std::array<int,3>{0,100,255}&&
            labels[3].value=="9,876,543,210点"&&labels[3].rectangle==std::array<int,4>{213,192,0,0},
            "Steam17活跃末阶段row6无第七行，English只换累计标签，普通long与捕获旧纪录独立");
    }
    check(steam_clear_score_number(0)=="0"&&steam_clear_score_number(999)=="999"&&
        steam_clear_score_number(1000)=="1,000"&&steam_clear_score_number(std::numeric_limits<std::int64_t>::max())==
        "9,223,372,036,854,775,807"&&!steam_clear_score_number(-1),"Steam17非负long普通千位分组边界独立oracle");
    in.stage=3;in.row=0;in.rows=rows;in.stage_counter=14;
    check(bool(steam_clear_score_skin(in,options)),"Steam17尚未显示score不要求其测宽");
    in.stage_counter=15;check(!steam_clear_score_skin(in,options),"Steam17显示score缺两次测宽拒绝");
    options.score_widths={12,14};
    for(int fault=0;fault<12;++fault) {
        auto bad=in;auto o=options;
        if(fault==0)bad.stage=8;if(fault==1)bad.stage_counter=-1;if(fault==2)bad.main_counter=std::numeric_limits<int>::max();
        if(fault==3)bad.row=6;if(fault==4)bad.rows.reset();if(fault==5)bad.sum=-1;if(fault==6)bad.captured_high_score=-1;
        if(fault==7)(*bad.rows)[5].count=-1;if(fault==8)(*bad.rows)[5].score=-1;if(fault==9)o.title_widths.reset();
        if(fault==10)o.score_widths[0]=-1;if(fault==11)o.score_widths[1]=4097;
        check(!steam_clear_score_skin(bad,o),"Steam17坏钟/行/负值/坏实测宽显式拒，不以补值过验");
    }
    const auto stable=*steam_clear_score_skin(in,options);const auto input_rows=*in.rows;
    for(int repeat=0;repeat<5;++repeat){const auto again=steam_clear_score_skin(in,options);
        check(again&&again->operations.size()==stable.operations.size()&&(*in.rows)[0].count==input_rows[0].count&&
            (*in.rows)[5].score==input_rows[5].score&&in.stage_counter==15&&in.main_counter==8&&
            in.sum==1234567890123LL&&in.captured_high_score==9876543210LL,
            "Steam17重复纯查询规模稳定，不推进钟/数量/累加分数/覆盖旧纪录");}
}
void steam_new_game_skin_checks(Checks &check) {
    // 原91调用点的显式草稿夹具，不开始新世界，也不把普通基础W当任意scratch的完整绘制。
    SteamNewGameInput in{"UI测试村","UI测试甲",0,0,4,0};
    SteamNewGameOptions o;o.title_widths=std::array<int,2>{60,62};o.start_width=240;o.finger_frame=5;
    o.arrow_selected={true,false};
    auto texts=[](const SteamNewGamePlan &p) {
        std::vector<SteamHumanOverviewText> out;
        for(const auto &op:p.operations)if(const auto *v=std::get_if<SteamHumanOverviewText>(&op))out.push_back(*v);
        return out;
    };
    auto portrait=[](const SteamNewGamePlan &p)->const SteamHumanPortrait& {
        for(const auto &op:p.operations)if(const auto *v=std::get_if<SteamHumanPortrait>(&op))return *v;
        throw std::runtime_error("启动皮肤：Steam91缺人物请求");
    };
    constexpr std::array<std::array<int,4>,3> touches{{{106,128,114,26},{112,150,102,26},{112,171,102,26}}};
    constexpr std::array<std::array<int,2>,4> hands{{{112,143},{118,165},{136,186},{-12,215}}};
    for(int selection=0;selection<4;++selection) {
        in.selection=selection;const auto p=steam_new_game_skin(in,o);check(bool(p),"Steam91四项显式选择可绘");
        check(p->title=="重新开始"&&p->soft_labels==std::array<int,2>{0,2},"Steam91独立标题/软标签，非标题菜单或计分页");
        std::vector<SteamStartupTouch> actual;std::vector<SteamFacilityImage> fingers;
        std::vector<StartupSkinRect> orange;
        for(const auto &op:p->operations) {
            if(const auto *t=std::get_if<SteamStartupTouch>(&op))actual.push_back(*t);
            if(const auto *f=std::get_if<SteamFacilityImage>(&op))fingers.push_back(*f);
            if(const auto *f=std::get_if<SteamStartupFrameDraw>(&op))if(const auto *r=std::get_if<StartupSkinRect>(f))
                if(r->rgb==std::array<int,3>{255,153,55})orange.push_back(*r);
        }
        check(actual.size()==4&&actual[0].component==3&&actual[0].value==0x20003&&
            actual[0].rectangle==std::array<int,4>{-28,186,296,56},"Steam91开始宽240+16不夹200，未选也注册component3");
        for(int row=0;row<3;++row)check(actual[row+1].component==11&&actual[row+1].value==(0x20000|row)&&
            actual[row+1].rectangle==touches[row]&&!actual[row+1].image_draw&&!actual[row+1].option,
            "Steam91三数组值宽94/82/82扩真实Touch，不用高亮矩形或新增marker");
        check(fingers.size()==1&&fingers[0].asset==SteamFacilityAsset::finger&&fingers[0].frame==5&&
            fingers[0].position==hands[selection],"Steam91当前SEB手形帧5不由main4推导，四选项独立锚");
        check(orange.size()==(selection==2?0U:1U),"Steam91性别选择无橙框，字段/开始各有唯一当前高亮");
        if(selection!=2)check(orange[0].rect==(selection==0?std::array<int,4>{116,133,94,16}:
            selection==1?std::array<int,4>{122,155,82,16}:std::array<int,4>{-8,206,256,16})&&!orange[0].outline,
            "Steam91按字段第三数与开始实测宽生成原高亮，非Touch热区");
        const auto tx=texts(*p);check(tx.size()==7&&tx.back().value=="开始游戏"&&tx.back().layout&&
            tx.back().anchor==34&&tx.back().rectangle==std::array<int,4>{-8,206,256,16},
            "Steam91开始正文是anchor34 TextLayout16高，不改成裸字y208");
        const auto &left=std::get<SteamNewGameArrow>(p->operations[p->operations.size()-2]);
        const auto &right=std::get<SteamNewGameArrow>(p->operations.back());
        check(left.component==18&&right.component==18&&left.value==16&&right.value==18&&
            left.source.position==std::array<int,2>{118,185}&&right.source.position==std::array<int,2>{210,185}&&
            left.source.bounds_frame==3&&right.source.bounds_frame==0&&left.source.selected&&!right.source.selected&&
            left.margin==std::array<int,4>{30,5,20,4}&&right.margin==std::array<int,4>{5,20,20,4},
            "Steam91末尾AddTouch箭头bounds/实测选择/下margin4，非APK下20或裸arrow02贴图");
        check(std::none_of(actual.begin(),actual.end(),[](const auto &t){return t.component==2;}),
            "Steam91本分支末尾返回，无计分页全局Touch2");
    }
    in.selection=0;
    for(const auto language:std::array<std::array<bool,2>,3>{{{false,false},{true,false},{false,true}}}) {
        o.english=language[0];o.japanese=language[1];const auto plan=*steam_new_game_skin(in,o);const auto tx=texts(plan);
        constexpr std::array<int,3> ys{135,157,178};constexpr std::array<const char*,3> labels{"城镇名称","冒险者姓名","性别"};
        for(int row=0;row<3;++row)check(tx[row].value==labels[row]&&tx[row].rectangle==
            std::array<int,4>{27,ys[row]+(language[0]?1:0),0,0}&&tx[row].font_size==(language[0]?9:0)&&
            tx[row].rgb==std::array<int,3>{0,100,255},"Steam91仅English改变标签9号/加1，非JP不是同一资格");
        check(tx[3].rectangle==std::array<int,4>{language[1]?62:70,178,0,0}&&tx[3].value=="男"&&
            tx[3].font_size==0&&tx[3].rgb==std::array<int,3>{92,51,31},"Steam91性别先重置字号，非JP x70不抄APK65");
        check(tx[4].value==in.village&&tx[5].value==in.name&&tx[4].rectangle==std::array<int,4>{163,135,0,0}&&
            tx[5].rectangle==std::array<int,4>{163,157,0,0}&&tx[4].anchor==2&&tx[5].anchor==2&&
            tx[4].font_size==(language[0]?11:0)&&tx[5].font_size==(language[0]?11:0),
            "Steam91草稿村名/定义0姓名居中，English-only11号，不用性别/首访名字补造");
    }
    o.english=false;o.japanese=false;o.view_y=23;
    for(const auto sample:std::array<std::array<int,3>,4>{{{0,0,0},{0,4,1},{1,12,3},{1,16,0}}}) {
        in.sex=sample[0];in.main=sample[1];const auto p=*steam_new_game_skin(in,o);const auto &actor=portrait(p);
        check(actor.position==std::array<int,2>{163,195}&&!actor.clip&&!actor.image_clip&&!actor.actor.shadow&&
            actor.actor.weapon&&actor.actor.weapon->resource==StartupVisualResource::weapon&&actor.actor.weapon->frame==0&&
            actor.actor.body.image==(sample[0]==0?14:15)&&actor.actor.body.sprite==2&&actor.actor.body.frame==sample[2],
            "Steam91原冒险者职业性别图14/15，weapon0必须存在，方向2/main16周期，无标题shadow或VIEW_Y重复");
        const auto tx=texts(p);check(tx[3].value==(sample[0]==0?"男":"女")&&tx[3].rgb==
            (sample[0]==0?std::array<int,3>{92,51,31}:std::array<int,3>{255,14,1}),"Steam91女值红色独立于标签蓝/男棕");
        std::size_t box=p.operations.size(),background=p.operations.size(),border=p.operations.size(),first_label=p.operations.size();
        for(std::size_t index=0;index<p.operations.size();++index) {
            const auto &op=p.operations[index];
            if(const auto *f=std::get_if<SteamStartupFrameDraw>(&op))if(const auto *r=std::get_if<StartupSkinRect>(f)) {
                if(r->rect==std::array<int,4>{17,59,202,145}&&!r->outline)box=index;
                if(r->rect==std::array<int,4>{30,65,180,63}&&r->outline&&r->rgb==std::array<int,3>{52,133,255})border=index;
            }
            if(const auto *d=std::get_if<StartupSkinDraw>(&op);d&&d->package==Package::event&&d->image==9) {
                background=index;check(d->crop==std::array<int,4>{0,9,180,63}&&d->offset==std::array<int,2>{30,65},
                    "Steam91 event9裁片0/9不是整图，背景不加VIEW_Y23");
            }
            if(const auto *t=std::get_if<SteamHumanOverviewText>(&op);t&&t->value=="城镇名称")first_label=index;
        }
        check(box<background&&background<border&&border<first_label,"Steam91框helper消费VIEW_Y一半，box→背景→蓝边→标签原序");
    }
    // 产品交回的合法维护草稿边界：村名/姓名可分别清空，纯皮肤不能要求表现占位再还原。
    for(int empty_fields=1;empty_fields<=3;++empty_fields) {
        auto draft=in;
        if(empty_fields&1)draft.village.clear();
        if(empty_fields&2)draft.name.clear();
        const auto plan=steam_new_game_skin(draft,o);
        check(bool(plan),"Steam91合法维护空草稿仍可绘制，不私加原版未证非空门槛");
        const auto tx=texts(*plan);
        check(tx[4].value==draft.village&&tx[5].value==draft.name&&
            tx[4].rectangle==std::array<int,4>{163,135,0,0}&&tx[5].rectangle==std::array<int,4>{163,157,0,0}&&
            portrait(*plan).actor.body.image==15,
            "Steam91空字段原样保留，文字锚与女角色不变，不补默认名或改变草稿");
    }
    const auto stable=*steam_new_game_skin(in,o);const auto original=in;
    for(int repeat=0;repeat<4;++repeat)check(steam_new_game_skin(in,o)->operations.size()==stable.operations.size()&&
        in.village==original.village&&in.name==original.name&&in.sex==original.sex&&in.main==16&&in.selection==0,
        "Steam91重复查询输出规模不增，不改草稿/性别/主钟或执行开始");
    for(int fault=0;fault<12;++fault) {
        auto bad=in;auto options=o;
        if(fault==0)bad.selection=4;if(fault==1)bad.sex=2;if(fault==2)bad.main=-1;
        if(fault==3)bad.main=std::numeric_limits<int>::max();if(fault==4)bad.adventure_profession=-1;
        if(fault==5)bad.adventure_profession=4;if(fault==6)options.finger_frame=-1;if(fault==7)options.title_widths.reset();
        if(fault==8)options.start_width.reset();if(fault==9)options.start_width=-1;
        if(fault==10){options.english=true;options.japanese=true;}if(fault==11)bad.name=std::string(4097,'a');
        check(!steam_new_game_skin(bad,options),"Steam91缺显式来源/错职业/越界/坏实测宽拒绝，不补默认W或猜资源");
    }
    // 同页Update2的独立控制流oracle；平台编辑、通用主钟和新世界安装由各自Owner负责。
    using Action=SteamNewGameAction;
    for(int row=0;row<4;++row)for(int sex=0;sex<2;++sex)for(bool custom:{false,true}) {
        constexpr std::array<std::array<bool,2>,4> directions{{{false,false},{true,false},{false,true},{true,true}}};
        constexpr std::array<bool,4> toggles{false,true,true,false};
        for(std::size_t n=0;n<directions.size();++n) {
            SteamNewGameUpdateInput input;input.selection=row;input.sex=sex;input.change_name=custom;
            input.left_repeat=directions[n][0];input.right_repeat=directions[n][1];
            const auto result=steam_new_game_update(input);
            check(result&&result->selection==row&&result->sex==(toggles[n]?1-sex:sex)&&
                result->reset_default_name==(toggles[n]&&!custom)&&result->action==Action::none&&
                result->immediate_return==std::optional<bool>(true)&&!result->editor_max_length&&!result->page_result&&
                std::all_of(result->checked_keys.begin(),result->checked_keys.end(),[](bool b){return b;}),
                "Steam91所有行左右独立切性别；双向不改名，自定义名保留，空输入正常true");
        }
    }
    for(int first=0;first<4;++first) {
        SteamNewGameUpdateInput input;input.selection=3;input.sex=1;
        input.up_pulse=input.down_pulse=input.left_repeat=input.right_repeat=input.confirm_pulse=input.back_soft_label=true;
        for(int n=first;n<4;++n)input.early_pulses[n]=true;
        const auto result=steam_new_game_update(input);
        check(result&&result->selection==3&&result->sex==1&&!result->reset_default_name&&
            result->action==Action::none&&result->immediate_return==std::optional<bool>(false)&&
            !result->editor_max_length&&!result->page_result,
            "Steam91四早脉冲任一命中返回false，不当作取消/Pop或执行后续输入");
        for(int n=0;n<10;++n)check(result->checked_keys[n]==(n<=first),
            "Steam91早脉冲精确短路，后续查询结果不能生效");
    }
    constexpr std::array<std::array<int,4>,7> moves{{
        {0,1,0,3},{3,0,1,0},{2,1,0,1},{1,0,1,2},{0,1,1,0},{3,1,1,3},{2,0,0,2}}};
    for(const auto &move:moves) {
        SteamNewGameUpdateInput input;input.selection=move[0];input.up_pulse=move[1]!=0;input.down_pulse=move[2]!=0;
        const auto result=steam_new_game_update(input);
        check(result&&result->selection==move[3],"Steam91上下Pulse各自执行且四行环绕，同轮相消");
    }
    constexpr std::array<Action,4> actions{Action::edit_village,Action::edit_name,Action::none,Action::start_new_game};
    for(int row=0;row<4;++row) {
        SteamNewGameUpdateInput input;input.selection=row;input.confirm_pulse=input.back_soft_label=true;
        const auto result=steam_new_game_update(input);
        check(result&&result->action==actions[row]&&result->sex==0&&!result->reset_default_name&&
            !result->checked_keys[9]&&!result->page_result&&
            result->immediate_return==(row==2?std::optional<bool>(true):std::nullopt)&&
            result->editor_max_length==(row<2?std::optional<int>(row==0?14:12):std::nullopt),
            "Steam91确认按最终行分派且压过soft2；性别行确认不切换，编辑14/12不充作字符算法");
    }
    SteamNewGameUpdateInput combined;combined.selection=0;combined.down_pulse=true;
    combined.left_repeat=true;combined.confirm_pulse=true;combined.back_soft_label=true;
    const auto edited=steam_new_game_update(combined);
    check(edited&&edited->selection==1&&edited->sex==1&&edited->reset_default_name&&
        edited->action==Action::edit_name&&edited->editor_max_length==12&&!edited->immediate_return,
        "Steam91同轮先导航/性别/默认名请求，再按最终行编辑；没有提前执行平台动作");
    combined.confirm_pulse=false;
    const auto cancelled=steam_new_game_update(combined);
    check(cancelled&&cancelled->selection==1&&cancelled->sex==1&&cancelled->reset_default_name&&
        cancelled->action==Action::pop&&cancelled->page_result==-1&&!cancelled->editor_max_length&&
        !cancelled->immediate_return&&cancelled->checked_keys[9],
        "Steam91返回也保留当轮导航/性别候选，先置result=-1再请求Pop，不冒称Pop成功或回滚草稿");
    for(int repeat=0;repeat<3;++repeat)check(steam_new_game_update(combined)->action==Action::pop&&
        combined.selection==0&&combined.sex==0&&combined.down_pulse&&combined.left_repeat,
        "Steam91纯更新重复计算不安装候选/消费设备脉冲或增长历史");
    for(const auto bad:std::array<std::array<int,2>,4>{{{-1,0},{4,0},{0,-1},{0,2}}}) {
        SteamNewGameUpdateInput input;input.selection=bad[0];input.sex=bad[1];input.early_pulses[0]=true;
        check(!steam_new_game_update(input),"Steam91非法选择/性别先拒绝，不能靠早返回掩盖坏状态");
    }
}
void steam_config_skin_checks(Checks &check) {
    // Steam DrawConfig_en及_addTouch的独立坐标；只测只读皮肤，不运行设置setter或保存。
    SteamConfigSkinInput in{{0,1,0,5},0,false};
    SteamConfigSkinOptions o;o.view_y=23;o.graphics_origin={10,20};o.steam_platform=true;o.on_top=true;
    o.arrow_selected={{{true,false},{false,true}}};
    o.text.title="设置";o.text.names={"画质","速度","BGM","效果音"};
    o.text.choices={{{"画质优先","速度优先"},{"高速","普通"}}};
    o.text.explanation={"说明甲","说明乙"};o.text.title_widths=std::array<int,2>{24,26};
    const auto touches=[](const SteamConfigSkinPlan &p) {
        std::vector<SteamConfigTouch> out;
        for(const auto &op:p.operations)if(const auto *t=std::get_if<SteamConfigTouch>(&op))out.push_back(*t);
        return out;
    };
    const auto texts=[](const SteamConfigSkinPlan &p) {
        std::vector<SteamHumanOverviewText> out;
        for(const auto &op:p.operations)if(const auto *t=std::get_if<SteamHumanOverviewText>(&op))out.push_back(*t);
        return out;
    };
    const auto rectangles=[](const SteamConfigSkinPlan &p) {
        std::vector<StartupSkinRect> out;
        for(const auto &op:p.operations)if(const auto *f=std::get_if<SteamStartupFrameDraw>(&op))
            if(const auto *r=std::get_if<StartupSkinRect>(f))out.push_back(*r);
        return out;
    };
    const auto has_rectangle=[&](const SteamConfigSkinPlan &p,std::array<int,4> rect,std::array<int,3> rgb) {
        const auto all=rectangles(p);return std::any_of(all.begin(),all.end(),[&](const auto &r){return r.rect==rect&&r.rgb==rgb&&!r.outline;});
    };
    const auto has_touch=[](const std::vector<SteamConfigTouch> &all,int component,int value) {
        return std::find_if(all.begin(),all.end(),[=](const auto &t){return t.touch.component==component&&t.touch.value==value;});
    };
    for(int selection=0;selection<4;++selection) {
        in.selection=selection;const auto result=steam_config_skin(in,o);check(bool(result),"Steam12四项显式选择可绘");
        const auto &p=*result;const auto all=touches(p);const auto tx=texts(p);
        check(p.title=="设置"&&p.soft_labels==std::array<int,2>{0,2}&&all.size()==13,
            "Steam12固定四项与软标签，J9锁定只减一个值组件，不补方向第五项");
        check(has_rectangle(p,{19,46,202,127},{247,253,247})&&
            has_rectangle(p,{28,56+24*selection,56,14},{159,189,255}),
            "Steam12窗口返回box47/174先减off12再加VIEW_Y半值11，行选择独立底色");
        for(int row=0;row<4;++row) {
            const auto t=has_touch(all,6,0x20000|row);
            check(t!=all.end()&&t->source_rectangle==std::array<int,4>{-120,51+24*row,179,24}&&
                t->touch.rectangle==t->source_rectangle&&!t->extended_value&&!t->drag,
                "Steam12所有行保留component6原负x大矩形，不以可见名称框替代热区");
            const auto name=std::find_if(tx.begin(),tx.end(),[&](const auto &v){return v.value==o.text.names[row];});
            check(name!=tx.end()&&name->rectangle==std::array<int,4>{29,57+24*row,row==3?40:0,row==3?11:0}&&
                name->anchor==(row==3?32:0)&&name->layout==(row==3)&&name->font_size==0,
                "Steam12前三项普通正文，SE名字单独40×11 TextLayout，字号不猜常量");
        }
        check(tx[tx.size()-2].rectangle==std::array<int,4>{34,178,0,0}&&tx[tx.size()-2].anchor==17&&
            tx.back().rectangle==(selection==1?std::array<int,4>{34,196,180,11}:std::array<int,4>{34,196,0,0})&&
            tx.back().anchor==(selection==1?32:17)&&tx.back().layout==(selection==1)&&
            tx.back().rgb==(selection==1?std::array<int,3>{200,0,0}:std::array<int,3>{30,30,30}),
            "Steam12当前说明两行固定锚，速度第二行红色限框并不因J9隐藏");
    }
    in.selection=3;
    for(int quality:{0,1})for(int speed:{0,1})for(bool unlocked:{false,true}) {
        in.values[0]=quality;in.values[1]=speed;in.speed_unlocked=unlocked;
        const auto p=*steam_config_skin(in,o);const auto all=touches(p);const auto tx=texts(p);
        check(has_rectangle(p,{quality==0?151:94,56,54,14},{159,189,255})&&
            has_rectangle(p,{speed==0?151:94,80,54,14},{159,189,255}),
            "Steam12前两行存储0显示右项、存储1显示左项，当前值底与当前选择行无关");
        for(int row=0;row<2;++row)for(int choice=0;choice<2;++choice) {
            const int value=(row==0?0x60000:0x70000)|(choice==0?1:0);
            const auto t=has_touch(all,6,value);const bool blocked=row==1&&choice==0&&!unlocked;
            check((t==all.end())==blocked,"Steam12 J9仅屏蔽高速左项组件，普通/画质组件仍存在");
            if(!blocked)check(t->touch.rectangle==
                (choice==0?std::array<int,4>{59,51+24*row,92,24}:std::array<int,4>{149,51+24*row,240,24})&&
                t->extended_value==(0x20000|row),"Steam12值组件编码实际存储与ExtendValues父行，矩形不缩成选项色块");
        }
        const auto fast=std::find_if(tx.begin(),tx.end(),[](const auto &t){return t.value=="高速";});
        check(fast!=tx.end()&&fast->rectangle==std::array<int,4>{121,81,0,0}&&fast->rgb==
            (unlocked?std::array<int,3>{30,30,30}:std::array<int,3>{128,128,128}),
            "Steam12锁定高速文字变灰，合法历史值的底色仍绘制，不在查询中夹回普通");
    }
    for(int volume=0;volume<6;++volume) {
        in.values[2]=volume;in.values[3]=5-volume;const auto p=*steam_config_skin(in,o);const auto all=touches(p);
        std::vector<SteamConfigImage> images;
        for(const auto &op:p.operations)if(const auto *image=std::get_if<SteamConfigImage>(&op))images.push_back(*image);
        check(images.size()==6,"Steam12音量两行各左/右/knob三个图像请求，无插帧资源增长");
        for(int row=0;row<2;++row) {
            const int dy=24*row,level=row==0?volume:5-volume;
            check(images[3*row].image==(row==0?6:4)&&images[3*row].anchor==std::array<int,2>{99,111+dy}&&
                images[3*row+1].image==(row==0?5:7)&&images[3*row+1].anchor==std::array<int,2>{201,111+dy}&&
                images[3*row+2].image==11&&images[3*row+2].anchor==std::array<int,2>{119+12*level,111+dy},
                "Steam12按下图4/6与5/7独立，六档knob整数步宽12且图元不重加graphics_origin");
            const int key=0x20002|row;const auto left=has_touch(all,21,key),right=has_touch(all,20,key);
            const auto drag=has_touch(all,19,row==0?0x80000:0x90000);
            check(left!=all.end()&&right!=all.end()&&drag!=all.end(),"Steam12音量减/加/拖动三组件分开");
            check(left->source_rectangle==std::array<int,4>{59,99+dy,55,24}&&left->extended_value==0x10010&&
                right->source_rectangle==std::array<int,4>{186,99+dy,240,24}&&right->extended_value==0x10012,
                "Steam12左右组件保留原不同大矩形与逻辑方向值，非点击直写音量");
            check(drag->source_rectangle==std::array<int,4>{119,99+dy,62,24}&&
                drag->touch.rectangle==std::array<int,4>{104+12*level,99+dy,32,24}&&drag->touch.option==4&&
                drag->extended_value==key&&drag->drag==std::array<int,6>{129,20,62,0,6,0},
                "Steam12拖动注册thumb32宽，原轨道62宽，绝对Drag只一次加原点且y不加行号");
            check(has_rectangle(p,{120,109+dy,60,4},{30,30,30})&&has_rectangle(p,{119,110+dy,62,2},{30,30,30}),
                "Steam12两层轨道实参独立于thumb触区和最终缩放图块");
        }
    }
    struct ImageOracle {int id;std::array<int,2> anchor;std::array<int,4> crop;std::array<float,4> destination;};
    const std::array<ImageOracle,5> images{{
        {4,{99,111},{1,85,22,21},{90.75F,102.75F,16.5F,15.75F}},
        {5,{201,111},{24,85,22,21},{192.75F,102.75F,16.5F,15.75F}},
        {6,{99,111},{47,85,28,28},{88.5F,100.5F,21.F,21.F}},
        {7,{201,111},{76,85,28,28},{190.5F,100.5F,21.F,21.F}},
        {11,{119,111},{73,57,9,23},{117.5F,101.25F,6.75F,17.25F}}}};
    for(const auto &sample:images)for(int density:{1,2}) {
        const auto image=steam_config_image_skin({sample.id,sample.anchor},128*density);
        const std::array<int,4> crop{sample.crop[0]*density,sample.crop[1]*density,sample.crop[2]*density,sample.crop[3]*density};
        check(image&&image->crop==crop&&image->destination==sample.destination,
            "Steam12独立metadata IMAGE_DATA裁片：128/256密度只改源crop，目标offset/size保持0.75");
    }
    for(int id:{-1,0,3,8,12})check(!steam_config_image_skin({id,{0,0}},128),"Steam12未闭合image索引不能借其它control图回落");
    for(int width:{-1,0,1,8193})check(!steam_config_image_skin({4,{0,0}},width),"Steam12错误密度/舍入空裁片/超过维护预算拒绝，不猜图宽");
    const auto largest=steam_config_image_skin({4,{99,111}},8192);
    check(largest&&largest->crop==std::array<int,4>{64,5440,1408,1344}&&
        largest->destination==std::array<float,4>{90.75F,102.75F,16.5F,15.75F},
        "Steam12维护图宽上界允许，源64倍仍保持独立目标大小与锚");
    for(bool platform:{false,true})for(bool top:{false,true}) {
        o.steam_platform=platform;o.on_top=top;const auto p=*steam_config_skin(in,o);
        const auto count=std::count_if(p.operations.begin(),p.operations.end(),[](const auto &op){return std::holds_alternative<SteamConfigPlatformKeyboard>(op);});
        check(count==(platform&&top?1:0),"Steam12末平台键盘请求仅Steam资格及当前顶页，不伪装第五项设置");
        if(count){const auto &keyboard=std::get<SteamConfigPlatformKeyboard>(p.operations.back());
            check(keyboard.anchor==std::array<int,2>{21,51}&&keyboard.component==2,"Steam12平台请求原锚与组件，保持在说明之后");}
    }
    o.text.explanation={"",""};check(bool(steam_config_skin(in,o)),"Steam12空说明行保留平台解析结果，不补演示句或无源拒绝");
    for(int fault=0;fault<14;++fault) {
        auto bad=in;auto options=o;
        if(fault==0)bad.selection=4;if(fault==1)bad.values[0]=-1;if(fault==2)bad.values[1]=2;
        if(fault==3)bad.values[2]=6;if(fault==4)bad.values[3]=-1;if(fault==5)options.text.title.clear();
        if(fault==6)options.text.names[3].clear();if(fault==7)options.text.choices[1][0].clear();
        if(fault==8)options.text.title_widths.reset();if(fault==9)options.text.title_widths=std::array<int,2>{-1,20};
        if(fault==10)options.text.title_widths=std::array<int,2>{20,4097};if(fault==11)options.graphics_origin[0]=65537;
        if(fault==12)options.view_y=-65537;if(fault==13)options.text.explanation[1]=std::string(4097,'a');
        check(!steam_config_skin(bad,options),"Steam12坏值/选择/缺文本测宽/维护预算显式拒，不放宽门控或补默认值");
    }
    const auto baseline=*steam_config_skin(in,o);const auto values=in.values;
    for(int repeat=0;repeat<4;++repeat)check(steam_config_skin(in,o)->operations.size()==baseline.operations.size()&&
        in.values==values&&in.selection==3&&o.graphics_origin==std::array<int,2>{10,20},
        "Steam12重复纯查询输出规模稳定，不写设置、选择或消费坐标，未新增Owner或平台任务");
    // EN四行键盘主体；原setter的音频/画质副作用不在此重复展开或执行。
    using Return=SteamConfigReturnRequest;
    constexpr std::array<int,6> left_volume{5,0,1,2,3,4},right_volume{1,2,3,4,5,0};
    for(int row=0;row<4;++row)for(int value=0;value<(row<2?2:6);++value)for(int direction=0;direction<4;++direction) {
        SteamConfigUpdateInput input;input.state={{0,0,0,0},row,true};input.state.values[row]=value;
        input.left_repeat=(direction&1)!=0;input.right_repeat=(direction&2)!=0;
        const auto result=steam_config_english_update(input);
        const int expected=direction==0||direction==3?value:row<2?1-value:
            direction==1?left_volume[value]:right_volume[value];
        auto expected_values=input.state.values;expected_values[row]=expected;
        check(result&&result->candidate.values==expected_values&&result->candidate.selection==row&&
            result->candidate.speed_unlocked&&std::all_of(result->checked_keys.begin(),result->checked_keys.end(),[](bool b){return b;}),
            "Steam12四行实际存储值与音量绕环；左右独立查询且双向净值不变");
        check(result->operations.size()==(expected==value?0U:1U),
            "Steam12仅净改值产生一次setter请求，无方向/确认声或双向试听");
        if(expected!=value) {
            const auto *request=std::get_if<SteamConfigSettingRequest>(&result->operations.front());
            check(request&&request->row==row&&request->before==value&&request->after==expected,
                "Steam12setter携原实际值，不把显示索引交给系统/播放器");
        }
    }
    for(int speed=0;speed<2;++speed) {
        SteamConfigUpdateInput input;input.state={{0,speed,2,3},1,false};
        input.left_repeat=input.right_repeat=input.back_soft_label=true;
        const auto result=steam_config_english_update(input);
        check(result&&result->candidate.values==input.state.values&&result->checked_keys==
            std::array<bool,5>{true,true,false,false,true}&&result->operations.size()==3,
            "Steam12速度锁跳过左右查询但仍返回，不偷改合法历史高速值");
        for(int n=0;n<3;++n)check(std::get<Return>(result->operations[n])==
            std::array<Return,3>{Return::save_system,Return::reset_offscreen,Return::pop}[n],
            "Steam12返回SaveSystem→ResetOffscreen→Pop有序，不代表实际保存/Pop成功");
    }
    SteamConfigUpdateInput mixed;mixed.state={{0,0,0,0},0,false};mixed.down_repeat=true;
    mixed.right_repeat=true;mixed.back_soft_label=true;
    const auto blocked=steam_config_english_update(mixed);
    check(blocked&&blocked->candidate.selection==1&&blocked->candidate.values==mixed.state.values&&
        !blocked->checked_keys[2]&&!blocked->checked_keys[3]&&blocked->operations.size()==3,
        "Steam12先换最终行再判断速度门控，不能修改旧画质行");
    mixed.state.selection=1;mixed.up_repeat=true;mixed.down_repeat=false;
    const auto changed=steam_config_english_update(mixed);
    check(changed&&changed->candidate.selection==0&&changed->candidate.values[0]==1&&changed->operations.size()==4&&
        std::get<SteamConfigSettingRequest>(changed->operations[0]).row==0,
        "Steam12导航回画质后先改值，再同轮保存；back不是撤销");
    for(int n=0;n<3;++n)check(std::get<Return>(changed->operations[n+1])==
        std::array<Return,3>{Return::save_system,Return::reset_offscreen,Return::pop}[n],
        "Steam12真实setter请求必须排在返回三请求之前");
    mixed.state.selection=0;mixed.up_repeat=mixed.down_repeat=true;
    const auto both=steam_config_english_update(mixed);
    check(both&&both->candidate.selection==0&&both->candidate.values[0]==1,
        "Steam12上下Repeat独立执行，四行环绕后相消再改最终行");
    for(int repeat=0;repeat<3;++repeat)check(steam_config_english_update(mixed)->operations.size()==4&&
        mixed.state.values==std::array<int,4>{0,0,0,0}&&mixed.state.selection==0,
        "Steam12纯请求重复计算不安装设置、不发声/写文件或追加平台任务");
    for(int fault=0;fault<10;++fault) {
        auto bad=mixed;
        if(fault<2)bad.state.selection=fault==0?-1:4;
        else {const int row=(fault-2)/2;bad.state.values[row]=(fault%2==0)?-1:(row<2?2:6);}
        check(!steam_config_english_update(bad),"Steam12非法行/实际值显式拒绝，不能靠返回或速度锁掩盖坏来源");
    }
    // setter计划独立于键盘净改值过滤：同值画质/速度仍写，音量同值才无后继。
    using Property=SteamConfigProperty;
    using SetterAction=SteamConfigSetterAction;
    const auto property_is=[](const SteamConfigSetterOperation &op,Property property,int index,int value) {
        const auto *write=std::get_if<SteamConfigPropertyWrite>(&op);
        return write&&write->property==property&&write->index==index&&write->value==value;
    };
    const auto quality=changed?steam_config_setting_effects(
        std::get<SteamConfigSettingRequest>(changed->operations.front())):std::nullopt;
    check(quality&&quality->operations.size()==3&&
        property_is(quality->operations[0],Property::world,12,1)&&
        std::get<SteamConfigLinearFilter>(quality->operations[1]).enabled&&
        std::get<SetterAction>(quality->operations[2])==SetterAction::draw_all_forms,
        "Steam12真实键盘请求接setter，先写P12再请求过滤与DrawAllForms，不代表GPU或绘制已执行");
    for(int value:{0,1}) {
        const auto same=steam_config_setting_effects({0,value,value});
        check(same&&same->operations.size()==3&&
            property_is(same->operations[0],Property::world,12,value)&&
            std::get<SteamConfigLinearFilter>(same->operations[1]).enabled==(value==1)&&
            std::get<SetterAction>(same->operations[2])==SetterAction::draw_all_forms,
            "Steam12画质同值仍依序写P12/请求过滤/重绘，不能复用音量同值优化");
        const auto speed=steam_config_setting_effects({1,value,value});
        check(speed&&speed->operations.size()==1&&
            property_is(speed->operations[0],Property::world,13,value),
            "Steam12速度同值也只写P13；setter不额外添加J9门控、音频或重绘");
    }
    for(int row:{2,3})for(const auto values:std::array<std::array<int,2>,2>{{{1,0},{0,5}}}) {
        const int channel=row-2,after=values[1];
        const auto sound=steam_config_setting_effects({row,values[0],after});
        check(sound&&sound->operations.size()==(row==2?4U:5U)&&
            property_is(sound->operations[0],Property::system,row+1,after)&&
            std::get<SteamConfigVolume>(sound->operations[1]).channel==channel&&
            std::get<SteamConfigVolume>(sound->operations[1]).volume==after*51&&
            std::get<SteamConfigMute>(sound->operations[2]).channel==channel&&
            std::get<SteamConfigMute>(sound->operations[2]).mute==(after==0)&&
            std::get<SetterAction>(sound->operations[3])==SetterAction::reflect_audio,
            "Steam12音量边界先写J3/J4，再SetVolume(0/255)/SetMute/Reflect，BGM与SE通道分开");
        if(row==3)check(sound&&sound->operations.size()==5&&
            std::get<SteamConfigResourceSound>(sound->operations[4]).index==7,
            "Steam12效果音改变后末请求resSound_[7]试听，归零仍保留该请求，不替换成固定音频字节");
    }
    for(int row:{2,3})for(int value:{0,5}) {
        const auto same=steam_config_setting_effects({row,value,value});
        check(same&&same->operations.empty(),"Steam12音量同值为空计划，不写系统属性、Reflect或试听");
    }
    for(int row:{-1,4})check(!steam_config_setting_effects({row,0,0}),"Steam12 setter未知行显式拒绝");
    for(int row=0;row<4;++row)for(int value:{-1,row<2?2:6}) {
        check(!steam_config_setting_effects({row,value,0})&&!steam_config_setting_effects({row,0,value}),
            "Steam12 setter前后值各自校验，不能以同值过滤或调用者门控掩盖坏来源");
    }
}
void steam_commerce_skin_checks(Checks &check,const std::filesystem::path &root) {
    // raw83纯输入条件夹具；NEW不是购买资格，rank不是现金，不运行Owner事务。
    SteamCommerceSkinInput in{0,2,true,true};
    SteamCommerceSkinOptions o;o.view_y=23;o.finger_frame=5;
    o.text.title="南瓜商会";o.text.welcome="欢迎光临";
    o.text.names={"购买道具","出售道具","购买设施"};
    o.text.explanations={"说明甲","说明乙","说明丙"};
    o.text.title_widths=std::array<int,2>{48,50};
    for(bool japanese:{false,true})for(int selection=0;selection<3;++selection) {
        in.selection=selection;o.japanese=japanese;const auto result=steam_commerce_skin(in,o);
        check(bool(result),"Steam83两语言与三选项的纯计划接受完整显式输入");
        const auto &p=*result;
        check(p.title=="南瓜商会"&&p.soft_labels==std::array<int,2>{0,2},"Steam83标题和软标签不借84/85目录页");
        std::vector<SteamHumanOverviewText> texts;std::vector<SteamFacilityNumber> numbers;
        std::vector<SteamFacilityImage> facilities;std::vector<std::size_t> bars,icons;
        std::vector<SteamStartupTouch> touches;std::vector<SteamHumanImage> new_marks;
        std::size_t pumpkin=p.operations.size();
        for(std::size_t i=0;i<p.operations.size();++i) {
            const auto &op=p.operations[i];
            if(const auto *t=std::get_if<SteamHumanOverviewText>(&op))texts.push_back(*t);
            if(const auto *n=std::get_if<SteamFacilityNumber>(&op))numbers.push_back(*n);
            if(const auto *f=std::get_if<SteamFacilityImage>(&op))facilities.push_back(*f);
            if(const auto *t=std::get_if<SteamStartupTouch>(&op))touches.push_back(*t);
            if(const auto *n=std::get_if<SteamHumanImage>(&op))new_marks.push_back(*n);
            if(const auto *image=std::get_if<SteamCommerceImage>(&op)) {
                if(image->asset==SteamCommerceAsset::row_background)bars.push_back(i);
                if(image->asset==SteamCommerceAsset::category_icon)icons.push_back(i);
                if(image->asset==SteamCommerceAsset::private_pumpkin) {
                    pumpkin=i;check(image->position==std::array<int,2>{30,94}&&image->frame==0&&!image->crop,
                        "Steam83私有南瓜SEB38沿原锚，不能套公共ID2大臣或二次补offset");
                }
            }
        }
        check(pumpkin>0&&pumpkin+1<p.operations.size(),"Steam83存在可消费的独立私有人物请求");
        const auto *before=std::get_if<SteamHumanClip>(&p.operations[pumpkin-1]);
        const auto *after=std::get_if<SteamHumanClip>(&p.operations[pumpkin+1]);
        check(before&&after&&before->kind==SteamHumanClipKind::push_intersect&&
            before->rectangle==std::array<int,4>{21,66,18,18}&&after->kind==SteamHumanClipKind::pop,
            "Steam83人物只显示上部18高且clip原序退休，不裁整张素材");
        check(numbers.size()==1&&numbers[0].asset==SteamFacilityAsset::number03_hud&&
            numbers[0].value==3&&numbers[0].position==std::array<int,2>{202,69}&&
            numbers[0].padding==0&&numbers[0].anchor==1&&facilities.size()==2&&
            facilities[0].asset==SteamFacilityAsset::level_label&&facilities[0].position==std::array<int,2>{181,71}&&
            facilities[1].asset==SteamFacilityAsset::finger&&facilities[1].frame==5&&
            facilities[1].position==std::array<int,2>{japanese?68:66,114+26*selection},
            "Steam83等级是rank+1/SEB10，手形保留调用者当前帧及语言偏移");
        check(bars.size()==3&&icons.size()==3&&touches.size()==3&&texts.size()==5&&new_marks.size()==2,
            "Steam83只有三行、欢迎和选中说明；NEW只在购道具/购设施，没有额外价签或滚动条");
        check(texts[0].value=="欢迎光临"&&texts[0].rectangle==std::array<int,4>{45,70,0,0}&&
            texts[0].anchor==1&&texts[4].value==o.text.explanations[selection]&&
            texts[4].rectangle==std::array<int,4>{120,198,0,0}&&texts[4].anchor==2,
            "Steam83欢迎/当前说明的普通DrawString锚和当前文本正确");
        for(int row=0;row<3;++row) {
            const int y=101+26*row;const auto &bar=std::get<SteamCommerceImage>(p.operations[bars[row]]);
            const auto &icon=std::get<SteamCommerceImage>(p.operations[icons[row]]);
            const auto *touch=std::get_if<SteamStartupTouch>(&p.operations[bars[row]+1]);
            const auto *name=std::get_if<SteamHumanOverviewText>(&p.operations[bars[row]+2]);
            check(bar.position==std::array<int,2>{53,y}&&bar.crop==std::array<int,4>{0,row==selection?0:26,134,26}&&
                touch&&name&&touch->component==11&&touch->value==(0x20000|row)&&
                touch->rectangle==std::array<int,4>{33,y,174,26}&&touch->option==0&&
                !touch->image_draw&&name->value==o.text.names[row],
                "Steam83逐行底图→组件11→名称，热区不套75 marker16或附加Margin");
            check(icon.frame==row&&icon.position==std::array<int,2>{japanese?77:75,y+4}&&
                !icon.crop&&icons[row]>bars[row]+2&&name->rgb==std::array<int,3>{92,51,31}&&
                name->rectangle==(japanese?std::array<int,4>{127,y+7,0,0}:std::array<int,4>{99,y+7,85,11})&&
                name->font_size==(japanese?0:11)&&name->anchor==(japanese?2:32)&&name->layout==!japanese,
                "Steam83类别图在文字/NEW之后，日文普通字与非日文11号限框分别消费");
        }
        check(new_marks[0].asset==SteamHumanAsset::catalogue_new&&new_marks[0].position==std::array<int,2>{164,115}&&
            new_marks[1].asset==SteamHumanAsset::catalogue_new&&new_marks[1].position==std::array<int,2>{164,167},
            "Steam83两NEW原坐标，出售行不得误画NEW");
    }
    for(bool items:{false,true})for(bool facilities:{false,true}) {
        in.item_new=items;in.facility_new=facilities;const auto p=*steam_commerce_skin(in,o);
        const auto count=std::count_if(p.operations.begin(),p.operations.end(),[](const auto &op) {
            return std::holds_alternative<SteamHumanImage>(op);
        });
        check(count==int(items)+int(facilities),"Steam83两个NEW独立控制输出，均空仍保留三可选入口");
    }
    for(int fault=0;fault<9;++fault) {
        auto bad=in;auto options=o;
        if(fault==0)bad.selection=-1;if(fault==1)bad.selection=3;
        if(fault==2)bad.rank=-1;if(fault==3)bad.rank=std::numeric_limits<int>::max();
        if(fault==4)options.finger_frame=6;if(fault==5)options.text.title_widths.reset();
        if(fault==6)options.text.names[1].clear();if(fault==7)(*options.text.title_widths)[1]=-1;
        if(fault==8)options.view_y=std::numeric_limits<int>::max();
        check(!steam_commerce_skin(bad,options),"Steam83坏选择/算术/字体输入显式拒绝，无默认值补齐");
    }
    for(const auto asset:{SteamCommerceAsset::row_background,SteamCommerceAsset::category_icon,
                          SteamCommerceAsset::private_pumpkin}) {
        const auto r=steam_commerce_resource(asset);check(bool(r),"Steam83三个资源提供者都有出版入口");
        CpuImage image(LoadImage((root.parent_path()/r->published_image).string().c_str()));
        check(image.image.data!=nullptr,"Steam83已出版PNG可真实解码，无新素材副本");
        if(asset==SteamCommerceAsset::row_background)
            check(r->image==19&&r->sprite==-1&&image.image.width==134&&image.image.height==52,
                "Steam83底图两裁片在原134×52图内");
        else {
            const auto seb=tools::parse_legacy_seb(read_bytes(root.parent_path()/r->published_sprite));
            bool found=false;
            for(const auto &layer:seb.layers)for(const auto &part:layer.parts) {
                if(part.image_index<0)continue;
                check(part.source_x>=0&&part.source_y>=0&&part.source_x+part.width<=image.image.width&&
                    part.source_y+part.height<=image.image.height,"Steam83 SEB引用裁片在对应PNG内");
                if(asset==SteamCommerceAsset::private_pumpkin&&part.frame==0) {
                    found=part.image_index==173&&part.source_x==18&&part.source_y==0&&part.width==18&&
                        part.height==28&&part.offset_x==-9&&part.offset_y==-28;
                }
            }
            check(asset==SteamCommerceAsset::category_icon?(r->image==2&&r->sprite==1&&seb.frame_count==3):
                (r->image==173&&r->sprite==38&&found),"Steam83类别三帧/私有南瓜源矩形及offset有真实SEB oracle");
        }
    }
    check(!steam_commerce_resource(static_cast<SteamCommerceAsset>(99)),"Steam83未知资源身份不借公共角色兜底");
    auto state=test_support::page_fixture(83);const auto id=state.scripts.pages.back().id;
    check(!steam_commerce_input(state,id),"Steam83纯查询不补未初始化载荷");
    check(initialize_startup_world_commerce_pages(state),"Steam83只读Owner夹具真实初始化");
    for(auto &read:state.item_commerce_read)read.second=true;
    for(auto &read:state.facility_commerce_read)read.second=true;
    state.shop_item_stock.at(0).quantity=1;state.item_commerce_read.at(0)=false;
    const auto eligible=std::find_if(state.rules->facilities.begin(),state.rules->facilities.end(),
        [](const auto &f){return f.unlock_rank>=0;});
    check(eligible!=state.rules->facilities.end(),"Steam83夹具从真实设施定义找可售卖星级");
    state.rank=eligible->unlock_rank;state.facility_presence.at(eligible->id)=1;
    state.facility_commerce_read.at(eligible->id)=false;
    const auto snapshot=state.commerce_page_data;const auto random=state.scene.random.draws();
    for(int repeat=0;repeat<3;++repeat) {
        const auto input=steam_commerce_input(state,id);
        check(input&&input->item_new&&input->facility_new&&input->rank==state.rank&&input->selection==0&&
            state.commerce_page_data==snapshot&&state.scene.random.draws()==random&&state.sound_requests.empty(),
            "Steam83真实NEW保留p1设施提示，不套p0交易策略；重复查询不推进页面或一次性输出");
    }
    state.facility_presence.at(eligible->id)=2;state.shop_item_stock.at(0).quantity=0;
    const auto no_new=steam_commerce_input(state,id);
    check(no_new&&!no_new->item_new&&!no_new->facility_new,"Steam83库存0/设施p2不产生NEW");
    for(int fault=0;fault<4;++fault) {
        auto invalid=state;
        if(fault==0)invalid.commerce_page_data.erase(id);
        if(fault==1)invalid.page_counters.erase(id);
        if(fault==2)invalid.commerce_page_lists[id]={0};
        if(fault==3)invalid.shop_item_stock.erase(0);
        check(!steam_commerce_input(invalid,id),"Steam83Owner只读桥拒缺键/错误页目录，不以默认值凑输出");
    }
}
void steam_commerce_result_skin_checks(Checks &check) {
    SteamCommerceResultOptions options;options.finger_frame=4;
    options.text.title="商店";options.text.cash_label="所持金";options.text.acknowledgement="了解";
    options.text.messages={"已购买","已出售"};options.text.title_widths=std::array<int,2>{24,26};
    // 调用点夹具绑定真实定义0；初始化不执行84交易，不把它声称为自然购买。
    for(int mode=0;mode<2;++mode) {
        auto state=test_support::page_fixture(86);const auto id=state.scripts.pages.back().id;
        state.scripts.pages.back().legacy_f=mode;state.scripts.pages.back().legacy_s=0;
        options.text.name_width=31;options.text.acknowledgement_width=mode==0?100:240;
        check(!steam_commerce_result_skin(state,id,options),"Steam86缺初始化载荷不能靠真实定义补成合法反馈");
        check(initialize_startup_world_commerce_pages(state),"Steam86真实绑定初始化条件夹具");
        const auto before=startup_world_state_digest(state);
        const auto item=std::find_if(state.rules->items.begin(),state.rules->items.end(),[](const auto &d){return d.identity==0;});
        check(item!=state.rules->items.end(),"Steam86使用原定义0商品，不补演示定义或价格");
        for(int sample=0;sample<3;++sample) {
            constexpr std::array<int,3> widths{0,31,400},half{10,25,210};
            options.text.name_width=widths[sample];const auto result=steam_commerce_result_skin(state,id,options);
            check(bool(result),"Steam86买卖两类与零/奇数/宽名称实际Font测宽输入");
            const auto &p=*result;std::vector<SteamFacilityNumber> numbers;
            std::vector<SteamFacilityImage> images;std::vector<SteamHumanImage> icons;
            std::vector<SteamHumanOverviewText> texts;std::vector<SteamStartupTouch> touches;
            for(const auto &op:p.operations) {
                if(const auto *n=std::get_if<SteamFacilityNumber>(&op))numbers.push_back(*n);
                if(const auto *i=std::get_if<SteamFacilityImage>(&op))images.push_back(*i);
                if(const auto *i=std::get_if<SteamHumanImage>(&op))icons.push_back(*i);
                if(const auto *t=std::get_if<SteamHumanOverviewText>(&op))texts.push_back(*t);
                if(const auto *t=std::get_if<SteamStartupTouch>(&op))touches.push_back(*t);
            }
            const auto digits=mode==1?SteamFacilityAsset::number05:SteamFacilityAsset::number12;
            check(p.soft_labels==std::array<int,2>{0,0}&&numbers.size()==1&&
                numbers[0].asset==digits&&numbers[0].kind==(mode==1?SteamFacilityNumberKind::plus_value:SteamFacilityNumberKind::minus_value)&&
                numbers[0].value==(mode==1?item->commerce_price/2:item->commerce_price)&&
                numbers[0].position==std::array<int,2>{184,92}&&images.size()==2&&images[0].asset==digits&&
                images[0].position==std::array<int,2>{185,92}&&images[0].frame==20,
                "Steam86买价SEB19 minus/卖价SEB12 plus显示单次差额，另画同图frame20单位");
            check(texts.size()==4&&texts[0].value=="所持金"&&texts[0].rectangle==std::array<int,4>{47,91,0,0}&&
                texts[1].value==item->name&&texts[1].rectangle==std::array<int,4>{136-half[sample],122,0,0}&&
                texts[2].value==options.text.messages[mode]&&texts[2].rectangle==std::array<int,4>{120,146,0,0}&&
                texts[2].anchor==2&&icons.size()==2&&icons[0].asset==SteamHumanAsset::equipment_background&&
                icons[0].position==std::array<int,2>{115-half[sample],118}&&icons[0].crop==std::array<int,4>{18,0,18,18}&&
                icons[1].asset==SteamHumanAsset::item_icon&&icons[1].position==std::array<int,2>{116-half[sample],119}&&
                icons[1].crop==std::array<int,4>{80,0,16,16},
                "Steam86真实item0 icon5/分类底图先于名称；W+20先整半，不截长名称或拿定义ID裁图");
            const int button_width=mode==0?116:216,left=mode==0?62:12;
            const auto *frame=std::get_if<SteamStartupFrameDraw>(&p.operations[p.operations.size()-4]);
            const auto *rectangle=frame?std::get_if<StartupSkinRect>(frame):nullptr;
            check(rectangle&&rectangle->rect==std::array<int,4>{left,175,button_width,16}&&
                rectangle->rgb==std::array<int,3>{255,153,55}&&touches.size()==1&&touches[0].component==4&&touches[0].value==20&&
                touches[0].rectangle==std::array<int,4>{left-20,155,button_width+40,56}&&
                texts[3].rectangle==std::array<int,4>{left,175,button_width,16}&&texts[3].anchor==34&&texts[3].layout&&
                images[1].asset==SteamFacilityAsset::finger&&images[1].frame==4&&images[1].position==std::array<int,2>{left-4,184},
                "Steam86原底部了解夹200后加16，原序橙底/Touch4/限框字/当前帧手形，不增加软确认标签");
            check(startup_world_state_digest(state)==before,"Steam86重复绘制不付款/发声/Pop或推进输出/随机");
        }
        for(int fault=0;fault<8;++fault) {
            auto bad=state;auto input=options;
            if(fault==0)bad.commerce_page_data.erase(id);
            if(fault==1)bad.page_counters.erase(id);
            if(fault==2)bad.commerce_page_lists[id]={0};
            if(fault==3)bad.commerce_page_data.at(id)[4]=1;
            if(fault==4)input.text.name_width.reset();
            if(fault==5)input.text.acknowledgement_width=-1;
            if(fault==6)input.finger_frame=6;
            if(fault==7)input.text.messages[1].clear();
            const auto digest=startup_world_state_digest(bad);
            check(!steam_commerce_result_skin(bad,id,input)&&startup_world_state_digest(bad)==digest,
                "Steam86错绑定/缺键/孤立目录/坏字体或手形输入显式拒绝且原Owner不变");
        }
    }
}
void steam_commerce_item_skin_checks(Checks &check) {
    SteamCommerceItemOptions options;options.finger_frame=4;
    options.text={"南瓜商会 1/2","道具","剩余","持有","价格","多谢惠顾!",std::array<int,2>{72,74}};
    options.item_explanation.item_explain_widths={24,36,48};
    const auto fixture=[](int mode) {
        auto s=test_support::page_fixture(84);s.scripts.pages.back().legacy_f=mode;
        for(auto &stock:s.shop_item_stock)stock.second.quantity=0;
        // 独立六行几何夹具同时限定出售目录，不能漏入真实新局的其它初始持有物。
        for(auto &owned:s.items) {owned.second.inventory=0;s.catalog.at({0,owned.first})=owned.second;}
        for(int id=0;id<6;++id) {
            s.shop_item_stock.at(id).quantity=2;
            s.items.at(id).inventory=mode==1?2:0;
            s.catalog.at({0,id})=s.items.at(id);
        }
        return s;
    };
    for(int mode=0;mode<2;++mode)for(int tab=0;tab<(mode==0?2:1);++tab) {
        auto s=fixture(mode);const auto id=s.scripts.pages.back().id;
        check(!steam_commerce_item_skin(s,id,options),"Steam84查询不补Init或目录");
        check(initialize_startup_world_commerce_pages(s),"Steam84条件目录沿真实商会Init");
        check(s.commerce_page_lists.at(id)==std::vector<int>({0,1,2,3,4,5}),
              "Steam84几何夹具冻结恰六行原序定义，不混入其它新局持有物");
        auto &cursor=s.commerce_page_data.at(id);cursor[1]=tab;cursor[2]=1;cursor[3]=1;
        s.page_counters.at(id)=8;s.item_commerce_read.at(1)=false;
        const auto before=startup_world_state_digest(s);
        const auto result=steam_commerce_item_skin(s,id,options);
        check(bool(result),"Steam84买两页/卖一页接受独立first与已核中文测宽");
        std::vector<SteamHumanEquipmentRow> rows;std::vector<SteamHumanImage> icons;
        std::vector<SteamFacilityNumber> numbers;std::vector<SteamHumanOverviewText> texts;
        std::vector<SteamFacilityArrowTouch> arrows;std::optional<SteamHumanCatalogueScroll> scroll;
        std::size_t selection=0,touch=0,new_icon=0,finger=0,first_icon=0,name=0;
        for(std::size_t n=0;n<result->operations.size();++n) {
            const auto &op=result->operations[n];
            if(const auto *r=std::get_if<SteamStartupFrameDraw>(&op))if(const auto *v=std::get_if<StartupSkinRect>(r))
                if(v->rect==std::array<int,4>{23,95,191,16})selection=n;
            if(const auto *r=std::get_if<SteamHumanEquipmentRow>(&op)){if(rows.empty())touch=n;rows.push_back(*r);}
            if(const auto *i=std::get_if<SteamHumanImage>(&op)) {
                icons.push_back(*i);
                if(i->asset==SteamHumanAsset::catalogue_new&&i->position==std::array<int,2>{10,99})new_icon=n;
                if(i->asset==SteamHumanAsset::equipment_background&&i->position==std::array<int,2>{29,94})first_icon=n;
            }
            if(const auto *i=std::get_if<SteamFacilityImage>(&op))
                if(i->asset==SteamFacilityAsset::finger)finger=n;
            if(const auto *v=std::get_if<SteamHumanOverviewText>(&op)) {
                texts.push_back(*v);if(v->rectangle==std::array<int,4>{50,94,85,17})name=n;
            }
            if(const auto *v=std::get_if<SteamFacilityNumber>(&op))numbers.push_back(*v);
            if(const auto *v=std::get_if<SteamFacilityArrowTouch>(&op))arrows.push_back(*v);
            if(const auto *v=std::get_if<SteamHumanCatalogueScroll>(&op))scroll=*v;
        }
        check(rows.size()==5&&rows[0].touch.value==(0x20000|1)&&
              rows[0].touch.rectangle==std::optional<std::array<int,4>>({23,95,191,16})&&
              rows[0].touch.option==16&&rows[0].margin==std::array<int,4>{29,0,0,0},
              "Steam84保留五行Touch11 marker16及Steam单侧Margin，不套APK双侧");
        check(selection<touch&&touch<new_icon&&new_icon<finger&&finger<first_icon&&first_icon<name,
              "Steam84选中底→行Touch→NEW→手形→两层图标→姓名沿真实绘制顺序");
        check(std::count_if(icons.begin(),icons.end(),[](const auto &v){return v.asset==SteamHumanAsset::item_icon;})==5,
              "Steam84图标helper类型1对买卖均绘实际道具图标");
        check((mode==0&&arrows.size()==2&&arrows[0].position==std::array<int,2>{23,55}&&
              arrows[0].frame==3&&arrows[0].value==16&&arrows[1].position==std::array<int,2>{217,55}&&
              arrows[1].frame==0&&arrows[1].value==18)||(mode==1&&arrows.empty()),
              "Steam84只有购买有原title箭头，phase8保留3像素位移");
        check(scroll&&scroll->arguments==std::array<int,3>{6,5,0x20000}&&
              scroll->touch.rectangle==std::optional<std::array<int,4>>({221,85,3,110})&&
              scroll->rectangles[1].rect==std::array<int,4>{220,103,5,92},
              "Steam84滚动条110高，真实first1与五行比例，不借79的111");
        check(std::any_of(texts.begin(),texts.end(),[&](const auto &v){return v.value==s.rules->items.at(1).name&&
              v.rectangle==std::array<int,4>{50,94,85,17}&&v.layout&&v.font_size==0;}),
              "Steam84非日文姓名继承字号，沿85×17TextLayout不套APK字号10");
        if(mode==0&&tab==1) {
            check(numbers.size()==5&&std::all_of(numbers.begin(),numbers.end(),[](const auto &v){
                      return v.kind==SteamFacilityNumberKind::count&&v.value==0&&v.position[0]==210;}),
                  "Steam84买入持有页只画合法零库存，不偷偷抬成1或显示售价");
            const auto zero=steam_facility_number_draws(numbers.front(),8);
            check(zero&&zero->size()==2&&zero->front().frame==0&&
                  zero->back().asset==SteamFacilityAsset::inventory_unit&&zero->back().frame==2,
                  "Steam84零数量真实展开0与SEB76单位帧2");
        } else check(numbers.size()==10&&numbers[0].kind==SteamFacilityNumberKind::count&&
                      numbers[0].position==std::array<int,2>{161,98}&&numbers[1].kind==SteamFacilityNumberKind::money&&
                      numbers[1].position==std::array<int,2>{210,98},
                      "Steam84买剩余/卖持有与钱数独立右锚161/210");
        for(int n=0;n<8;++n)check(steam_commerce_item_skin(s,id,options).has_value(),"Steam84重复查询有合法固定规模输出");
        check(startup_world_state_digest(s)==before,"Steam84有序计划不改游标/金额/库存/随机/页面");
    }
    auto s=fixture(0);const auto id=s.scripts.pages.back().id;
    check(initialize_startup_world_commerce_pages(s),"Steam84反馈测试Init真实目录");
    s.commerce_page_data.at(id)[5]=20;s.commerce_page_feedback_amount.emplace(id,123);
    auto feedback=options;feedback.item_explanation.item_explain_widths={};
    for(int mode=0;mode<2;++mode) {
        s.scripts.pages.back().legacy_f=mode;s.commerce_page_data.at(id)[0]=mode;
        if(mode==1)for(int item:s.commerce_page_lists.at(id)) {s.items.at(item).inventory=2;s.catalog.at({0,item})=s.items.at(item);}
        for(int selected:{0,4}) {
            s.commerce_page_data.at(id)[2]=selected;
            const auto plan=steam_commerce_item_skin(s,id,feedback);
            check(bool(plan),"Steam84正反馈不要求未消费的道具说明测宽");
            const auto &amount=std::get<SteamFacilityNumber>(plan->operations[plan->operations.size()-2]);
            const auto &unit=std::get<SteamFacilityImage>(plan->operations.back());
            check(amount.value==123&&amount.kind==(mode?SteamFacilityNumberKind::plus_value:SteamFacilityNumberKind::minus_value)&&
                  amount.asset==(mode?SteamFacilityAsset::number05:SteamFacilityAsset::number12)&&
                  amount.position==std::array<int,2>{201,200}&&unit.asset==amount.asset&&unit.frame==20&&
                  unit.position==amount.position,"Steam84冻结本笔123，不随选中商品重算；Steam钱数→单位同锚201/200");
            const auto &pumpkin=std::get<SteamCommerceImage>(plan->operations[plan->operations.size()-4]);
            check(pumpkin.asset==SteamCommerceAsset::private_pumpkin&&pumpkin.position==std::array<int,2>{30,223},
                  "Steam84反馈南瓜复用真实私有SEB与18×18底栏裁剪");
        }
    }
    s=fixture(0);check(initialize_startup_world_commerce_pages(s),"Steam84拒绝测试Init");
    s.commerce_page_data.at(id)[2]=5;s.commerce_page_data.at(id)[3]=5;
    const auto tail=steam_commerce_item_skin(s,id,options);
    check(tail&&std::count_if(tail->operations.begin(),tail->operations.end(),[](const auto &op){return std::holds_alternative<SteamHumanEquipmentRow>(op);})==1&&
          s.commerce_page_data.at(id)[3]==5,"Steam84尾部不足五项的独立首行合法，查询不夹到count−5");
    for(int fault=0;fault<7;++fault) {
        auto bad=s;auto o=options;
        if(fault==0)bad.commerce_page_data.erase(id);
        if(fault==1)bad.item_commerce_read.erase(5);
        if(fault==2)o.text.title_widths.reset();
        if(fault==3)o.item_explanation.item_explain_widths={};
        if(fault==4){bad.commerce_page_data.at(id)[5]=1;bad.commerce_page_feedback_amount.erase(id);}
        if(fault==5){bad.commerce_page_data.at(id)[5]=1;bad.commerce_page_feedback_amount[id]=-1;}
        if(fault==6)bad.commerce_page_feedback_amount[id]=400;
        const auto before=startup_world_state_digest(bad);
        check(!steam_commerce_item_skin(bad,id,o)&&startup_world_state_digest(bad)==before,
              "Steam84缺载荷/已读/标题或效果测宽/反馈金额配对拒绝且无修补");
    }
}
void steam_commerce_facility_skin_checks(Checks &check,const std::filesystem::path &root) {
    auto initial=test_support::page_fixture(85);const auto id=initial.scripts.pages.back().id;
    initial.rank=5; // 已知星级条件夹具，不宣称自然升星/解锁路线。
    SteamCommerceFacilityOptions options;
    options.text.title="购买设施";options.text.facility_header="设施";
    options.text.price_header="价格";options.text.explanation="使用城镇点数购买";
    options.text.title_widths=std::array<int,2>{48,50};options.text.price_width=17;
    check(!steam_commerce_facility_skin(initial,id,options),"Steam85纯查询不补初始化或目录");
    check(initialize_startup_world_commerce_pages(initial),"Steam85条件页真实Init生成维护p0冻结目录");
    const auto count=static_cast<int>(initial.commerce_page_lists.at(id).size());
    check(count>=3,"原设施目录提供至少三项，覆盖完整窗与短表不补定义");
    for(int sample=0;sample<3;++sample) {
        auto state=initial;options.japanese=sample==1;options.scroll_first_touch=sample!=2;
        options.text.price_width=sample==1?19:17;
        state.village_points=sample==0?0:999;
        auto &cursor=state.commerce_page_data.at(id);
        if(sample==1){cursor[2]=count-1;cursor[3]=count-3;}
        if(sample==2){state.commerce_page_lists.at(id).resize(1);cursor[2]=0;cursor[3]=0;}
        const auto &list=state.commerce_page_lists.at(id);
        for(auto &read:state.facility_commerce_read)read.second=true;
        state.facility_commerce_read.at(list[cursor[3]])=false;
        const auto digest=startup_world_state_digest(state);
        const auto result=steam_commerce_facility_skin(state,id,options);
        check(bool(result),"Steam85全语言/尾窗/短表沿冻结目录可绘，村点条件不筛目录");
        const auto &p=*result;std::vector<std::size_t> maps;
        std::vector<SteamHumanEquipmentRow> rows;std::vector<SteamFacilityNumber> numbers;
        std::vector<SteamHumanOverviewText> texts;std::vector<SteamHumanImage> news;
        std::vector<SteamHumanCatalogueScroll> scrolls;std::vector<StartupSkinRect> rectangles;
        for(std::size_t index=0;index<p.operations.size();++index) {
            const auto &op=p.operations[index];
            if(std::holds_alternative<SteamFacilityMapchip2>(op))maps.push_back(index);
            if(const auto *r=std::get_if<SteamHumanEquipmentRow>(&op))rows.push_back(*r);
            if(const auto *n=std::get_if<SteamFacilityNumber>(&op))numbers.push_back(*n);
            if(const auto *t=std::get_if<SteamHumanOverviewText>(&op))texts.push_back(*t);
            if(const auto *n=std::get_if<SteamHumanImage>(&op))news.push_back(*n);
            if(const auto *s=std::get_if<SteamHumanCatalogueScroll>(&op))scrolls.push_back(*s);
            if(const auto *f=std::get_if<SteamStartupFrameDraw>(&op))
                if(const auto *r=std::get_if<StartupSkinRect>(f))rectangles.push_back(*r);
        }
        check(std::none_of(p.operations.begin(),p.operations.end(),[](const auto &op) {
            return std::holds_alternative<SteamFacilityImage>(op);
        }),"Steam85本页没有手形或标题箭头");
        const int visible=sample==2?1:3;
        check(p.soft_labels==std::array<int,2>{7,2}&&int(maps.size())==visible&&int(rows.size())==visible&&
            int(numbers.size())==visible&&int(texts.size())==visible+3&&news.size()==1&&scrolls.size()==1,
            "Steam85三行原序、短表一行，详情/返回软标签与NEW输出规模正确");
        check(texts[0].rectangle==std::array<int,4>{51,69,0,0}&&
            texts[1].rectangle==std::array<int,4>{options.japanese?162:158,69,0,0},
            "Steam85设施/价格表头语言锚独立，不影响后续单位定位");
        const auto unit=std::find_if(p.operations.begin(),p.operations.end(),[](const auto &op) {
            const auto *image=std::get_if<SteamCommerceImage>(&op);return image&&image->asset==SteamCommerceAsset::point_unit;
        });
        check(unit!=p.operations.end()&&std::get<SteamCommerceImage>(*unit).position==
            std::array<int,2>{166+*options.text.price_width,67}&&std::get<SteamCommerceImage>(*unit).frame==4,
            "Steam85点单位两语言均166+实际LT价格字宽，不猜表头x+4");
        for(int row=0;row<visible;++row) {
            const int absolute=cursor[3]+row,y=89+34*row,definition=list[absolute];
            const auto d=std::find_if(state.rules->facilities.begin(),state.rules->facilities.end(),
                [&](const auto &v){return v.id==definition;});
            const auto &map=std::get<SteamFacilityMapchip2>(p.operations[maps[row]]);
            const auto &clip=std::get<SteamHumanClip>(p.operations[maps[row]-1]);
            const auto &pop=std::get<SteamHumanClip>(p.operations[maps[row]+1]);
            check(d!=state.rules->facilities.end()&&map.mapchip==d->display_id&&map.position==std::array<int,2>{65,y+11}&&
                map.orientation==0&&clip.kind==SteamHumanClipKind::push_intersect&&clip.rectangle==std::array<int,4>{22,y+1,88,33}&&
                pop.kind==SteamHumanClipKind::pop,"Steam85图块沿真实display_id/Mapchip2，不借实例朝向，交集clip立即退休");
            check(rows[row].touch.component==11&&rows[row].touch.value==(0x20000|absolute)&&rows[row].touch.option==16&&
                rows[row].touch.rectangle==std::array<int,4>{110,y,106,35}&&rows[row].margin==std::array<int,4>{29,0,0,0}&&
                texts[row+2].value==d->name&&texts[row+2].rectangle==std::array<int,4>{116,y+5,90,11}&&
                texts[row+2].anchor==32&&texts[row+2].layout&&texts[row+2].font_size==0,
                "Steam85右框热区marker16/单侧左29，全部语言仍TextLayout继承字号");
            const int cost=state.rules->facility_initial[definition].capacity;
            check(numbers[row].kind==SteamFacilityNumberKind::number&&numbers[row].value==cost&&
                numbers[row].position==std::array<int,2>{206,y+20}&&numbers[row].anchor==4&&numbers[row].padding==0&&
                numbers[row].asset==(state.village_points>=cost?SteamFacilityAsset::number11:SteamFacilityAsset::number12),
                "Steam85点价用capacity原cost，足额SEB20/不足19，金额没有G或逐行单位");
            for(const auto expected:{std::array<int,4>{21,y,90,35},std::array<int,4>{110,y,106,35}})
                check(std::any_of(rectangles.begin(),rectangles.end(),[&](const auto &r){return r.rect==expected&&r.outline&&r.rgb==std::array<int,3>{0,255,128};}),
                    "Steam85两个绿框端点转半开尺寸，未画绿色实心建筑底图");
        }
        const int selection_y=89+34*(cursor[2]-cursor[3]);
        check(std::count_if(rectangles.begin(),rectangles.end(),[](const auto &r){return !r.outline&&r.rgb==std::array<int,3>{255,153,55};})==1&&
            std::any_of(rectangles.begin(),rectangles.end(),[&](const auto &r){return r.rect==std::array<int,4>{111,selection_y+1,104,33}&&r.rgb==std::array<int,3>{255,153,55};})&&
            news[0].position==std::array<int,2>{22,114},"Steam85选择底色独立点价，NEW读isShopSee而非定义new_");
        const int denominator=std::max(int(list.size()),3),top=85+110*cursor[3]/denominator,height=330/denominator;
        const auto &scroll=scrolls[0];
        check(scroll.rectangles[0].rect==std::array<int,4>{220,85,5,110}&&
            scroll.rectangles[1].rect==std::array<int,4>{220,top,5,height+1}&&
            scroll.rectangles[1].rgb==(options.scroll_first_touch&&list.size()>3?std::array<int,3>{246,129,0}:std::array<int,3>{48,160,255})&&
            scroll.touch.component==12&&scroll.track_touch.component==25&&scroll.arguments==std::array<int,3>{int(list.size()),3,0x20000}&&
            scroll.touch.rectangle==std::array<int,4>{221,85,3,110},"Steam85原三行110高滚动及12→25，没有套79的111高");
        const auto &bottom=std::get<SteamHumanOverviewText>(p.operations[p.operations.size()-4]);
        const auto &clip=std::get<SteamHumanClip>(p.operations[p.operations.size()-3]);
        const auto &pumpkin=std::get<SteamCommerceImage>(p.operations[p.operations.size()-2]);
        check(bottom.value==options.text.explanation&&bottom.rectangle==std::array<int,4>{45,199,0,0}&&bottom.anchor==1&&
            clip.rectangle==std::array<int,4>{21,195,18,18}&&pumpkin.asset==SteamCommerceAsset::private_pumpkin&&
            pumpkin.position==std::array<int,2>{30,223}&&std::get<SteamHumanClip>(p.operations.back()).kind==SteamHumanClipKind::pop,
            "Steam85末说明之后私有南瓜，不创建购买/确认按钮或借公共ID2");
        check(startup_world_state_digest(state)==digest,"Steam85绘制不扣点/清已读/改冻结目录或消费随机输出");
    }
    for(int fault=0;fault<7;++fault) {
        auto state=initial;auto input=options;
        if(fault==0)state.commerce_page_data.erase(id);
        if(fault==1)state.commerce_page_lists.erase(id);
        if(fault==2)state.facility_commerce_read.erase(state.commerce_page_lists.at(id)[0]);
        if(fault==3)state.commerce_page_data.at(id)[2]=count;
        if(fault==4)input.text.price_width.reset();
        if(fault==5)input.text.price_width=-1;
        if(fault==6)input.text.title_widths.reset();
        check(!steam_commerce_facility_skin(state,id,input),"Steam85坏冻结载荷/缺已读/缺真实测宽显式拒绝，不重筛凑可画行");
    }
    const auto resource=steam_commerce_resource(SteamCommerceAsset::point_unit);
    check(resource&&resource->image==31&&resource->sprite==44&&std::string(resource->published_image)=="steam-common/icon_result00.png",
        "Steam85原单位具名绑定Steam图，不借APK同SEB不同像素");
    CpuImage image(LoadImage((root.parent_path()/resource->published_image).string().c_str()));
    const auto seb=tools::parse_legacy_seb(read_bytes(root.parent_path()/resource->published_sprite));
    check(image.image.width==63&&image.image.height==48&&std::any_of(seb.layers.begin(),seb.layers.end(),[](const auto &layer) {
        return std::any_of(layer.parts.begin(),layer.parts.end(),[](const auto &p){return p.frame==4&&p.image_index==31&&p.source_x==0&&
            p.source_y==16&&p.width==16&&p.height==16&&p.offset_x==0&&p.offset_y==0;});
    }),"Steam85真实Steam单位PNG解码与SEB44 frame4源裁有独立oracle");
}
void steam_platform_skin_checks(Checks &check,const std::filesystem::path &source_root) {
    // 原managed kairolib的显式seb23/img27及seb25/img29；与游戏common包不同。
    struct Oracle {const char *stem;int image,width,height,dx,dy;};
    constexpr std::array<Oracle,2> resources{{{"button_gamepad",27,64,60,-32,-30},
                                           {"button_keyboard",29,64,42,-32,-21}}};
    for(const auto &expected:resources) {
        const auto root=source_root.parent_path()/"steam-platform";
        CpuImage image(LoadImage((root/(std::string(expected.stem)+".png")).string().c_str()));
        check(image.image.width==expected.width&&image.image.height==expected.height,
              "Steam平台图像原尺寸真实解码，不借common或按截图画替代图");
        const auto seb=tools::parse_legacy_seb(read_bytes(root/(std::string(expected.stem)+".seb")));
        check(seb.frame_count==1&&seb.layers.size()==1&&seb.layers[0].parts.size()==1,
              "Steam平台按钮原SEB只有layer0/frame0一个实际裁片，不补高亮帧");
        const auto &part=seb.layers[0].parts[0];
        check(part.frame==0&&part.image_index==expected.image&&part.source_x==0&&part.source_y==0&&
              part.width==expected.width&&part.height==expected.height&&part.offset_x==expected.dx&&
              part.offset_y==expected.dy&&part.flip_x==0&&part.flip_y==0,
              "Steam平台SEB原img身份、完整裁片与不同居中偏移保持独立oracle");
        int opaque{};
        for(int y=0;y<image.image.height;++y)for(int x=0;x<image.image.width;++x)
            opaque+=GetImageColor(image.image,x,y).a>0;
        check(opaque>0,"Steam平台源裁片真实非空，不以目录存在替代图像消费检查");
    }
    using Kind=SteamPlatformButtonKind;
    const auto close=[](const std::array<float,4> &actual,const std::array<float,4> &expected) {
        for(std::size_t n=0;n<actual.size();++n)if(std::abs(actual[n]-expected[n])>0.00002F)return false;
        return true;
    };
    SteamPlatformButtonInput in{Kind::controller_binding,{20,20},2,true,false,false,100.F};
    for(const auto kind:{Kind::controller_binding,Kind::keyboard_guide})for(bool touched:{false,true}) {
        const bool keyboard=kind==Kind::keyboard_guide;
        in.kind=kind;in.anchor=keyboard?std::array<int,2>{21,51}:std::array<int,2>{20,20};in.touched=touched;
        const auto result=steam_platform_button_skin(in);
        check(result&&result->body,"Steam两平台按钮可发布独立纯计划，不执行外链或Steam面板");
        const auto &body=*result->body;const auto &resource=body.resource;
        check(resource.image==(keyboard?29:27)&&resource.sprite==(keyboard?25:23)&&
              resource.crop==(keyboard?std::array<int,4>{0,0,64,42}:std::array<int,4>{0,0,64,60})&&
              resource.offset==(keyboard?std::array<int,2>{-32,-21}:std::array<int,2>{-32,-30})&&
              std::string(resource.image_path)==(keyboard?"steam-platform/button_keyboard.png":"steam-platform/button_gamepad.png")&&
              std::string(resource.sprite_path)==(keyboard?"steam-platform/button_keyboard.seb":"steam-platform/button_gamepad.seb"),
              "Steam平台资源身份/路径均来自managed kairolib，不混Unity同名包或common ID");
        const auto expected=keyboard?(touched?std::array<float,4>{3.4F,39.45F,35.2F,23.1F}:
                                             std::array<float,4>{5.F,40.5F,32.F,21.F}):
                                    (touched?std::array<float,4>{2.4F,3.5F,35.2F,33.F}:
                                             std::array<float,4>{4.F,5.F,32.F,30.F});
        check(close(body.destination,expected)&&body.reset_origin,
              "Steam平台T锚/S尺寸/T偏移原序：锚不缩放且不再叠表单origin，高亮1.1只放大裁片");
        check(body.component==2&&body.value==(keyboard?0x13489A9:0x13465BA)&&
              body.touch_rectangle==(keyboard?std::array<int,4>{5,36,32,30}:std::array<int,4>{4,5,32,30})&&
              body.margin==std::array<int,4>{15,15,15,15},
              "Steam平台两热区固定gamepad23的半尺寸，键盘图高42和高亮都不改变普通30高热区");
        check(bool(body.blend)==touched,"Steam平台普通继承混色，高亮才带显式混色请求");
        if(touched)check(body.blend->mode==1&&body.blend->rgba==std::array<int,4>{255,255,255,128},
              "Steam平台高亮保留BLEND_COLOR1白128实参，不转换成未经核验的普通alpha混合");
        check(bool(body.joystick)==!keyboard,"Steam控制器额外glyph独立请求，键盘不虚构glyph");
        if(!keyboard)check(body.joystick->button==8&&body.joystick->registration==24&&
              body.joystick->scale==100&&body.joystick->anchor==std::array<int,2>{6,6},
              "Steam控制器glyph保留原button8/registration24/scale100，实际比例只缩放附加glyph锚");
    }
    in.touched=false;
    for(const auto kind:{Kind::controller_binding,Kind::keyboard_guide})for(bool desktop:{false,true})for(bool xbox:{false,true}) {
        in.kind=kind;in.desktop=desktop;in.xbox=xbox;
        const auto p=steam_platform_button_skin(in);
        check(p&&bool(p->body)==(desktop&&(kind==Kind::controller_binding||!xbox)),
              "Steam helper仅桌面；Xbox仅隐藏keyboard，外层顶页资格不冒充helper内部条件");
    }
    in.kind=Kind::keyboard_guide;in.desktop=true;in.xbox=false;in.scale_ratio.reset();
    check(steam_platform_button_skin(in)->body.has_value(),"Steam keyboard不消费仅controller需要的比例，缺值仍合法");
    in.scale_ratio=std::numeric_limits<float>::quiet_NaN();
    check(steam_platform_button_skin(in)->body.has_value(),"Steam keyboard不提前校验未消费glyph比例");
    in.kind=Kind::controller_binding;in.scale_ratio=75.F;in.anchor={-15,15};
    check(steam_platform_button_skin(in)->body->joystick->anchor==std::array<int,2>{-21,0},
          "Steam控制器glyph负乘积向零截断，不套floor或缩放按钮本体锚");
    in.scale_ratio=10000.F;in.anchor={65536,-65536};in.component=65535;
    const auto limit=steam_platform_button_skin(in);
    check(limit&&limit->body&&limit->body->component==65535&&
          limit->body->touch_rectangle==std::array<int,4>{65520,-65551,32,30},
          "Steam平台维护坐标/组件/比例上界合法，输出保留显式负坐标而不夹回屏幕");
    in={Kind::controller_binding,{20,20},2,true,false,false,100.F};
    for(int fault=0;fault<12;++fault) {
        auto bad=in;
        if(fault==0)bad.kind=static_cast<Kind>(2);if(fault==1)bad.anchor[0]=-65537;
        if(fault==2)bad.anchor[1]=65537;if(fault==3)bad.component=-1;if(fault==4)bad.component=65536;
        if(fault==5)bad.scale_ratio.reset();if(fault==6)bad.scale_ratio=0.F;if(fault==7)bad.scale_ratio=-1.F;
        if(fault==8)bad.scale_ratio=10001.F;if(fault==9)bad.scale_ratio=std::numeric_limits<float>::quiet_NaN();
        if(fault==10)bad.scale_ratio=std::numeric_limits<float>::infinity();
        if(fault==11)bad.scale_ratio=-std::numeric_limits<float>::infinity();
        check(!steam_platform_button_skin(bad),"Steam平台错kind/坐标/组件/缺实际比例/非法浮点预算显式拒绝，不生成部分请求");
    }
    check(!steam_platform_button_resource(static_cast<Kind>(2)),"Steam平台非法资源kind不能回落到keyboard");
    in.desktop=false;in.scale_ratio.reset();
    const auto hidden=steam_platform_button_skin(in);
    check(hidden&&!hidden->body,"Steam非桌面隐藏不提前消费controller比例或发布图像/热区/glyph");
    in.desktop=true;in.scale_ratio=100.F;
    const auto baseline=steam_platform_button_skin(in);
    for(int repeat=0;repeat<4;++repeat) {
        const auto p=steam_platform_button_skin(in);
        check(p&&p->body&&p->body->destination==baseline->body->destination&&
              p->body->touch_rectangle==baseline->body->touch_rectangle&&p->body->joystick&&
              in.anchor==std::array<int,2>{20,20}&&in.scale_ratio==100.F&&in.component==2,
              "Steam重复只读平台计划保持一个本体/一个glyph固定规模，不写输入或累积平台任务");
    }
    // 控制器SEB后备与平台入口按钮是不同资源；以下oracle来自managed原目录和SEB记录。
    struct ControllerOracle {int input_type,image,sprite;const char *stem;bool swapped;};
    constexpr std::array<ControllerOracle,3> controllers{{
        {0,15,13,"buttons_switch",false},{5,22,19,"buttons_ps4",false},{2,33,29,"buttons_xbox",true}}};
    const auto controller_root=source_root.parent_path()/"steam-platform";
    for(const auto &expected:controllers) {
        CpuImage image(LoadImage((controller_root/(std::string(expected.stem)+".png")).string().c_str()));
        const auto seb=tools::parse_legacy_seb(read_bytes(controller_root/(std::string(expected.stem)+".seb")));
        check(image.image.data&&image.image.width==512&&image.image.height==512&&seb.frame_count==44&&
                  seb.layers.size()==1&&seb.layers[0].parts.size()==44,
              "三套控制器PNG真实解码512方图，SEB仅layer0含44个独立关键帧");
        std::array<bool,44> seen{};
        for(const auto &part:seb.layers[0].parts) {
            check(part.frame>=0&&part.frame<44&&!seen[static_cast<std::size_t>(part.frame)]&&
                      part.image_index==expected.image&&part.width==64&&part.height==64&&
                      part.offset_x==-32&&part.offset_y==-32&&part.flip_x==0&&part.flip_y==0&&
                      part.source_x>=0&&part.source_y>=0&&part.source_x+64<=512&&part.source_y+64<=512,
                  "三套全部44帧保留真实图片槽、64裁片、居中偏移和无翻转，不按图列补帧");
            seen[static_cast<std::size_t>(part.frame)]=true;
            if(part.frame==0||part.frame==1)
                check(part.source_x==64*(expected.swapped?1-part.frame:part.frame)&&part.source_y==0,
                      "Xbox原frame0/1源裁片顺序交换，不借Switch或PS4图替代");
            if(part.frame>=32&&part.frame<=35)
                check(part.source_x==256+64*(part.frame-32)&&part.source_y==192,
                      "layer1方向提示重映射目标32..35有独立原裁片oracle");
            const auto plan=steam_controller_button_fallback({expected.input_type,part.frame,0,false,{},{}});
            check(plan&&plan->frame==part.frame&&plan->layer==0&&plan->resource.image==expected.image&&
                      plan->resource.sprite==expected.sprite&&
                      std::string(plan->resource.image_path)=="steam-platform/"+std::string(expected.stem)+".png"&&
                      std::string(plan->resource.sprite_path)=="steam-platform/"+std::string(expected.stem)+".seb",
                  "控制器后备全部44帧消费现出版资源及原目录槽，不生成动态图或账号设备事实");
        }
        check(std::all_of(seen.begin(),seen.end(),[](bool value){return value;}),
              "每套控制器44个实际帧均有主责资源消费，不以重复frame0冒充全集");
    }
    for(const auto [type,expected_image,expected_sprite]:std::array<std::array<int,3>,11>{{
            {1,33,29},{2,33,29},{3,33,29},{5,22,19},{13,22,19},{0,15,13},{4,15,13},{6,15,13},
            {-1,15,13},{std::numeric_limits<int>::min(),15,13},{std::numeric_limits<int>::max(),15,13}}}) {
        const auto plan=steam_controller_button_fallback({type,8,0,false,{},{}});
        check(plan&&plan->resource.image==expected_image&&plan->resource.sprite==expected_sprite&&plan->frame==8,
              "原InputType只识别1/2/3和5/13，其余任意有符号值走Switch后备");
    }
    // 源分支的具名映射表作为oracle，不在测试中重写被测范围／加16算法。
    constexpr std::array<std::array<int,2>,10> layer_frames{{
        {0,0},{1,1},{7,7},{8,8},{16,32},{17,33},{18,34},{19,35},{20,20},{43,43}}};
    for(const int layer:{std::numeric_limits<int>::min(),-1,0,1,2,std::numeric_limits<int>::max()})
        for(const auto [frame,layer_one_frame]:layer_frames) {
            const int expected_frame=layer==1?layer_one_frame:frame;
            const auto plan=steam_controller_button_fallback({0,frame,layer,false,-1,44});
            check(plan&&plan->frame==expected_frame&&plan->layer==0,
                  "原任意非零layer最终归0，仅原layer1的16..19重映射；未消费映射不校验");
        }
    // 原0/1反转互斥且先于layer转换：OK返回1不再触发Cancel，返回16才继续映射32。
    for(const auto &sample:std::array<std::array<int,4>,4>{{{0,1,1,1},{0,16,1,32},{1,17,1,33},{0,16,2,16}}}) {
        SteamControllerButtonFallbackInput input{0,sample[0],sample[2],true,{},{}};
        if(sample[0]==0){input.okay_frame=sample[1];input.cancel_frame=std::numeric_limits<int>::max();}
        else {input.cancel_frame=sample[1];input.okay_frame=std::numeric_limits<int>::min();}
        const auto result=steam_controller_button_fallback(input);
        check(result&&result->frame==sample[3]&&result->layer==0,
              "确认/取消仅消费原分支且在layer映射之前，OK变1不二次进入取消");
    }
    check(steam_controller_button_fallback({0,8,1,true,-1,44}).has_value(),
          "autoKeyReverse对非0/1不消费optional映射，即使未消费值超维护预算仍合法");
    for(int fault=0;fault<8;++fault) {
        SteamControllerButtonFallbackInput bad{0,0,1,true,0,1};
        if(fault==0)bad.frame=-1;if(fault==1)bad.frame=44;
        if(fault==2)bad.frame=std::numeric_limits<int>::max();
        if(fault==3)bad.okay_frame.reset();if(fault==4)bad.okay_frame=-1;if(fault==5)bad.okay_frame=44;
        if(fault==6){bad.frame=1;bad.cancel_frame.reset();}
        if(fault==7){bad.frame=1;bad.cancel_frame=44;}
        check(!steam_controller_button_fallback(bad),
              "后备frame与真正消费的反转值明确拒绝0..43维护预算之外或缺值，不静默夹到首帧");
    }
    // 静态指令独立oracle：75方图中心为37.5，整数路径向零截断而非floor。
    struct ImageOracle {int width,height;SteamButtonImageKind kind;float w,h;int x,y;};
    const std::array<ImageOracle,6> image_oracles{{
        {128,128,SteamButtonImageKind::dynamic_glyph,60.F,60.F,-10,21},
        {128,128,SteamButtonImageKind::right_click,75.F,75.F,-17,14},
        {127,65,SteamButtonImageKind::dynamic_glyph,60.F,30.7086620331F,-10,36},
        {129,63,SteamButtonImageKind::dynamic_glyph,60.F,29.3023262024F,-10,37},
        {67,129,SteamButtonImageKind::dynamic_glyph,31.1627922058F,60.F,5,21},
        {101,53,SteamButtonImageKind::right_click,75.F,39.3564376831F,-17,32}}};
    for(const auto &expected:image_oracles)for(bool matrix:{false,true}) {
        const auto p=steam_button_image_layout({expected.kind,expected.width,expected.height,{20,51},matrix});
        check(p&&p->scaled_size==std::array<float,2>{expected.w,expected.h}&&
                  p->draw_anchor==(matrix?std::array<int,2>{20,51}:std::array<int,2>{expected.x,expected.y})&&
                  bool(p->translation)==matrix&&p->recheck_matrix_before_pop,
              "Steam有效Image保持单精度长宽和独立矩阵平移，无矩阵偏移先截断再加整数锚");
        if(matrix)check(*p->translation==std::array<float,2>{expected.w*-0.5F,expected.h*-0.5F},
              "已有矩阵保持负半尺寸浮点平移，Draw仍用原锚，末尾重查矩阵后才Pop");
    }
    CpuImage mouse_image(LoadImage((source_root.parent_path()/steam_right_click_image_path).string().c_str()));
    check(mouse_image.image.data&&mouse_image.image.width==128&&mouse_image.image.height==128,
          "无控制器返回提示实际消费已出版128方图，未用RB或SEB裁片代替");
    for(int fault=0;fault<7;++fault) {
        SteamButtonImageInput bad{SteamButtonImageKind::dynamic_glyph,128,128,{20,51},false};
        if(fault==0)bad.width=0;if(fault==1)bad.height=-1;if(fault==2)bad.width=65537;
        if(fault==3)bad.height=65537;if(fault==4)bad.anchor[0]=-65537;
        if(fault==5)bad.anchor[1]=65537;if(fault==6)bad.kind=static_cast<SteamButtonImageKind>(-1);
        check(!steam_button_image_layout(bad),"动态图布局坏尺寸/类型/锚明确拒绝，不造缺图或夹值");
    }
    const auto negative=steam_button_image_layout({SteamButtonImageKind::right_click,128,128,{-20,-51},false});
    check(negative&&negative->draw_anchor==std::array<int,2>{-57,-88},
          "负锚仍先截断中心偏移，不能合并浮点锚后截断");
    const SteamControllerButtonFallbackInput stable{13,0,1,true,16,{}};
    for(int repeat=0;repeat<3;++repeat) {
        const auto plan=steam_controller_button_fallback(stable);
        check(plan&&plan->frame==32&&plan->layer==0&&plan->resource.image==22&&plan->resource.sprite==19&&
                  stable.frame==0&&stable.layer==1&&stable.okay_frame==16&&!stable.cancel_frame,
              "重复控制器后备只读输出保持固定一个请求，不改输入或累积资源设备任务");
    }
    SteamPlatformButtonEventInput event;
    for(int type:{0,1,2,13,99}) {
        event.event_type=type;
        const auto p=steam_platform_button_event(event);
        check(p&&p->actions.empty(),"非DEBUG的非CLICK回调不消费平台/语言字段，也不派传播结果");
    }
    event.event_type=5;
    for(int gates=0;gates<4;++gates) {
        event.use_steam=(gates&1)!=0;event.initialized=(gates&2)!=0;
        const auto p=steam_platform_button_event(event);
        check(gates==3?!p:bool(p)&&p->actions.empty(),
              "绑定CLICK先看USE_STEAM与初始化，实际调用才要求已取得handle");
    }
    event.debug=true;
    for(const auto handle:std::array<std::uint64_t,2>{0,0xFEDCBA9876543210ULL}) {
        event.controller_handle=handle;
        const auto p=steam_platform_button_event(event);
        check(p&&p->actions.size()==1&&std::holds_alternative<SteamBindingPanelRequest>(p->actions[0])&&
              std::get<SteamBindingPanelRequest>(p->actions[0]).controller_handle==handle,
              "DEBUG下CLICK也不读长按钟；绑定原样传64位handle包括零，不以HasController拦截");
    }
    event={};event.kind=SteamPlatformButtonKind::keyboard_guide;event.event_type=5;
    check(!steam_platform_button_event(event),"未取语言码与源null不同，不能静默补空外链");
    event.language_code_resolved=true;
    for(const auto &code:std::array<std::optional<std::string>,4>{{{},"","zh-CN","ZH-cn A"}}) {
        event.language_code=code;
        const auto p=steam_platform_button_event(event);
        check(p&&p->actions.size()==1&&std::holds_alternative<SteamControlMapRequest>(p->actions[0])&&
              std::get<SteamControlMapRequest>(p->actions[0]).url==
                "https://kairosoft.net/steam/?_lh="+code.value_or("")+"#controlmap",
              "键盘CLICK不依赖SDK；null/空合法，语言串按原Concat不编码/trim或改大小写");
    }
    event.language_code=std::string(4097,'x');check(!steam_platform_button_event(event),"已消费语言码超过维护预算明确拒绝");
    event.event_type=2;event.debug=true;
    const auto enter=steam_platform_button_event(event);
    check(enter&&enter->actions.size()==1&&std::holds_alternative<SteamCaptureSharedButtonTimeRequest>(enter->actions[0]),
          "DEBUG ENTER只请求取当前钟写共同字段，不消费无关语言/SDK或猜旧时刻");
    event.event_type=13;
    check(!steam_platform_button_event(event),"DEBUG ON缺实际当前钟或共同旧钟必须拒绝");
    using Limit=std::numeric_limits<std::int64_t>;
    struct ClockCase {std::int64_t now,down;bool open;};
    for(const auto c:std::array<ClockCase,11>{{{3000,0,false},{3001,0,true},{0,1,false},
        {Limit::min()+1000,Limit::max()-3000,true},{Limit::max(),-1,false},
        {Limit::min()+0x200000000LL,Limit::max()-0xffffffffLL,false},
        {Limit::min()+0x200000000LL+4001,Limit::max()-0xffffffffLL,true},
        {Limit::max()-0xffffffffLL,Limit::min()+0x2ffffffffLL,false},
        {0x100000000LL,0xfffff447LL,true},{0x100000000LL,0xfffff448LL,false},
        {0x300000000LL,0,true}}}) {
        event.now_millis=c.now;event.shared_down_millis=c.down;
        for(const auto kind:{SteamPlatformButtonKind::controller_binding,SteamPlatformButtonKind::keyboard_guide}) {
            event.kind=kind;
            const auto p=steam_platform_button_event(event);
            check(p&&p->actions.size()==(c.open?2U:0U),"两按钮共用长按钟；3000边界及原x86溢出flags转低位比较保留");
            if(c.open)check(std::holds_alternative<SteamDebugTrophyRequest>(p->actions[0])&&
                std::holds_alternative<SteamCaptureSharedButtonTimeRequest>(p->actions[1])&&
                event.shared_down_millis==c.down,
                "先请求奖杯页，再重新取钟；纯候选不把判定now偷写回或执行平台动作");
        }
    }
    event.kind=static_cast<SteamPlatformButtonKind>(99);
    check(!steam_platform_button_event(event),"未知按钮身份不按键盘分支蒙混通过");
}
// 平台登记表只验证调用计划；原GetJoystickUp、SDK图标及窗口输入另有消费者。
void steam_joystick_prompt_checks(Checks &check) {
    check(steam_joystick_is_shown(true,{})==std::optional<bool>{true}&&
          !steam_joystick_is_shown(false,{}),
          "固定x86编辑器不读Canvas；非编辑器缺真实设备状态拒绝，不猜成未连接手柄");
    struct Visibility {std::int32_t device;bool shown;};
    for(const auto c:std::array<Visibility,9>{{{0,true},{1,false},{2,false},{3,false},{4,true},
            {5,false},{-1,false},{std::numeric_limits<std::int32_t>::min(),false},
            {std::numeric_limits<std::int32_t>::max(),false}}})
        check(steam_joystick_is_shown(false,c.device)==std::optional<bool>{c.shown}&&
              steam_joystick_is_shown(true,c.device)==std::optional<bool>{true},
              "固定平台4以Unknown或Joystick判显示资格，其余设备及未定义值为假，不夹到合法枚举");
    const auto same=[](const SteamJoystickPromptState &a,const SteamJoystickPromptState &b) {
        if(a.table_present!=b.table_present||a.entries.size()!=b.entries.size())return false;
        for(std::size_t i=0;i<a.entries.size();++i) {
            const auto &x=a.entries[i];const auto &y=b.entries[i];
            if(x.button!=y.button||x.registration!=y.registration||x.displayed!=y.displayed||
               x.anchor!=y.anchor||x.life!=y.life||x.scale!=y.scale)return false;
        }
        return true;
    };
    const SteamJoystickPromptState empty{true,{}};
    SteamJoystickRegisterInput input;input.request={8,24,50,{20,30}};
    SteamJoystickDrawInput draw;
    {
        SteamJoystickRegisterInput ignored;ignored.application_type=4;ignored.request={-7,-9,-1,{}};
        SteamJoystickDrawInput ignored_draw;ignored_draw.application_type=4;ignored_draw.cursor_draw_frame=-1;
        const auto result=steam_joystick_register({},ignored);
        const auto plan=steam_joystick_draw({},ignored_draw);
        check(result&&!result->registered&&!result->state.table_present&&plan&&plan->operations.empty(),
              "无登记表合法早退，不消费请求、尺寸、frame或ratio");
    }
    input.application_type=4;
    for(const int button:{0,1,8,9,43}) {
        input.request.button=button;
        const auto result=steam_joystick_register(empty,input);
        check(result&&!result->registered&&same(result->state,empty),"type4无手柄不登记非2/3/7，8/9也不例外");
    }
    input.request.button=7;
    check(!steam_joystick_register(empty,input),"type4无手柄button7缺右软键标签拒绝");
    input.right_soft_label="返回";
    const auto unqualified=steam_joystick_register(empty,input);
    check(unqualified&&!unqualified->registered,"中文近义词不能替代原日文准入串");
    input.screen_size=std::array<int,2>{799,1001};input.application_size=std::array<int,2>{1000,800};
    input.steam_offset_enabled=true;
    for(const auto *label:{"戻る","メニュー","ﾒﾆｭｰ"}) {
        input.right_soft_label=label;
        const auto result=steam_joystick_register(empty,input);
        check(result&&result->registered&&result->state.entries[0].anchor==std::array<int,2>{-80,130},
              "button7精确三标签准入；尺寸差为负奇数仍向零除2");
    }
    input.right_soft_label.reset();input.steam_offset_enabled=false;
    for(const int button:{2,3}) {
        input.request.button=button;
        const auto result=steam_joystick_register(empty,input);
        check(result&&result->registered&&result->state.entries[0].anchor==std::array<int,2>{20,30},
              "type4无手柄2/3可登记；offset关闭不改锚点");
    }
    for(int fault=0;fault<4;++fault) {
        auto bad=input;
        if(fault==0)bad.screen_size.reset();
        if(fault==1)bad.application_size.reset();
        if(fault==2)(*bad.screen_size)[0]=0;
        if(fault==3)(*bad.application_size)[1]=65537;
        check(!steam_joystick_register(empty,bad),"type4即使offset关闭仍读取并校验四尺寸");
    }
    input.has_controller=true;input.request.button=8;
    const auto first=steam_joystick_register(empty,input);
    check(first&&first->registered&&first->state.entries.size()==1&&!first->auto_cancel_button&&
          first->state.entries[0].life==4&&first->state.entries[0].scale==50,"有手柄登记8/24并保存scale及初始life");
    auto prior=first->state;prior.entries[0].displayed=false;prior.entries[0].life=0;
    const auto frozen=prior;
    input.request.scale=-1;input.request.anchor={91,92};input.top_form_present=true;
    const auto reused=steam_joystick_register(prior,input);
    check(reused&&reused->registered&&!reused->auto_cancel_button&&reused->state.entries.size()==1&&
          reused->state.entries[0].displayed&&reused->state.entries[0].life==0&&
          reused->state.entries[0].scale==50&&reused->state.entries[0].anchor==std::array<int,2>{91,92}&&
          same(prior,frozen),"同key仅刷新显示与锚点，不复活life、不读取新scale、不产生autoCancel");
    input.request={9,2,100,{40,50}};
    const auto second=steam_joystick_register(first->state,input);
    check(second&&second->state.entries.size()==2&&second->state.entries[0].button==8&&
          second->state.entries[1].button==9&&!second->auto_cancel_button,"8/24与9/2保持登记次序，无方向键autoCancel");
    input.application_type=0;input.screen_size.reset();input.application_size.reset();
    input.has_controller=false;
    for(int button=0;button<4;++button) {
        input.request={button,-1,100,{}};
        const auto result=steam_joystick_register(empty,input);
        check(result&&result->registered&&result->auto_cancel_button==button,"新0..3且顶页存在输出autoCancel意图");
    }
    input.top_form_present=false;
    const auto no_top=steam_joystick_register(empty,input);
    check(no_top&&no_top->registered&&!no_top->auto_cancel_button,"无顶页不发autoCancel；非type4不读尺寸");
    for(int fault=0;fault<6;++fault) {
        auto bad=input;
        if(fault==0)bad.request.button=-1;
        if(fault==1)bad.request.button=44;
        if(fault==2)bad.request.registration=-2;
        if(fault==3)bad.request.scale=-1;
        if(fault==4)bad.request.scale=10001;
        if(fault==5) {
            bad.application_type=4;bad.has_controller=true;bad.steam_offset_enabled=true;
            bad.screen_size=std::array<int,2>{3,3};bad.application_size=std::array<int,2>{1,1};
            bad.request.anchor[0]=std::numeric_limits<int>::max();
        }
        check(!steam_joystick_register(prior,bad)&&same(prior,frozen),"坏新项或偏移溢出显式拒绝且旧表不变");
    }
    SteamJoystickPromptState lifecycle{true,{{8,24,true,{1,2},2,50},{8,25,true,{3,4},0,100},
        {8,26,false,{5,6},1,100},{9,2,true,{},3,100},{24,0,true,{},4,100},{43,0,true,{},4,100}}};
    const auto lifecycle_before=lifecycle;
    std::array<bool,24> releases{};releases[8]=true;
    const auto released=steam_joystick_release(lifecycle,releases);
    const auto reset=steam_joystick_reset(lifecycle);
    check(released&&reset,"有效生命周期操作返回候选");
    const auto raised=steam_joystick_raise(*reset);
    check(raised&&released->entries[0].life==1&&released->entries[1].life==0&&
          released->entries[2].life==1&&released->entries[3].life==3&&released->entries[4].life==4&&
          released->entries[5].life==4,"释放按button匹配所有可见registration，夹零且不索引24..43");
    for(std::size_t i=0;i<lifecycle.entries.size();++i) {
        const auto &before=lifecycle.entries[i];const auto &hidden=reset->entries[i];
        check(!hidden.displayed&&hidden.life==before.life&&hidden.anchor==before.anchor&&hidden.scale==before.scale&&
              !raised->entries[i].displayed&&raised->entries[i].life==4,"reset仅隐藏；raise包含隐藏项但不恢复displayed");
    }
    check(same(lifecycle,lifecycle_before)&&same(*released,*steam_joystick_release(lifecycle,releases)),
          "生命周期查询不改输入，重复查询不叠加释放");
    SteamJoystickPromptState single{true,{{9,2,true,{40,50},2,50}}};
    const auto plan=steam_joystick_draw(single,draw);
    check(plan&&plan->operations.size()==5,"半比例提示输出内Push、混合、按钮、默认混合、Pop");
    const auto *scale=std::get_if<SteamCursorPushScale>(&plan->operations[0]);
    const auto *mode=std::get_if<SteamCursorRenderMode>(&plan->operations[1]);
    const auto *button=std::get_if<SteamJoystickDrawButton>(&plan->operations[2]);
    const auto *restore=std::get_if<SteamCursorRenderMode>(&plan->operations[3]);
    check(scale&&scale->x==0.5F&&scale->y==0.5F&&scale->pivot==std::array<int,2>{0,0}&&
          mode&&mode->operation==1&&mode->source==127&&mode->destination==128&&
          button&&button->button==9&&button->anchor==std::array<int,2>{40,50}&&button->layer==0&&button->auto_key_reverse&&
          restore&&restore->operation==0&&restore->source==255&&restore->destination==0&&
          std::holds_alternative<SteamCursorPopMatrix>(plan->operations[4]),"life2独立alpha127；原锚点不预缩，恢复默认混合");
    single.entries[0].scale=100;
    for(const auto sample:std::array<std::array<int,2>,4>{{{1,63},{2,127},{3,191},{4,255}}}) {
        single.entries[0].life=sample[0];const auto p=steam_joystick_draw(single,draw);
        check(p&&p->operations.size()==3,"scale100无内矩阵");
        const auto *m=std::get_if<SteamCursorRenderMode>(&p->operations[0]);
        check(m&&m->source==sample[1]&&m->destination==255-sample[1],"life1..4独立透明度oracle");
    }
    single.entries[0].scale=0;
    const auto zero_scale=steam_joystick_draw(single,draw);
    check(zero_scale&&std::get<SteamCursorPushScale>(zero_scale->operations[0]).x==0.F,"scale0仍输出零矩阵");
    draw.application_type=4;draw.scale_ratio=100.F;
    const auto empty_scaled=steam_joystick_draw(empty,draw);
    check(empty_scaled&&empty_scaled->operations.size()==2&&
          std::get<SteamCursorPushScale>(empty_scaled->operations[0]).x==0.2222222238779068F&&
          std::holds_alternative<SteamCursorPopMatrix>(empty_scaled->operations[1]),"空表仍输出type4外层240/1080浮点矩阵");
    draw.application_type=0;draw.scale_ratio.reset();draw.cursor_draw_frame=167;
    const auto before_pulse=steam_joystick_draw(empty,draw);
    draw.cursor_draw_frame=168;const auto pulse=steam_joystick_draw(empty,draw);
    check(before_pulse&&before_pulse->operations.empty()&&pulse&&pulse->operations.size()==2&&
          std::get<SteamCursorPushScale>(pulse->operations[0]).x==1.1F,"所有平台frame168开始外层1.1缩放");
    draw={};draw.use_steam=true;
    SteamJoystickPromptState right{true,{{7,-1,true,{3,4},0,100}}};
    check(!steam_joystick_draw(right,draw),"Steam无手柄button7缺标签拒绝，即使登记life0");
    draw.right_soft_label="戻る";
    check(!steam_joystick_draw(right,draw),"匹配右标签必须提供点击计数");
    draw.right_click_counts=std::array<int,2>{1,0};
    const auto return_key=steam_joystick_draw(right,draw);
    check(return_key&&return_key->operations.size()==3&&
          std::get<SteamCursorRenderMode>(return_key->operations[0]).source==63&&right.entries[0].life==0,
          "返回使用右计数0替换有效life，不回写登记life");
    right.entries[0].life=4;
    for(const auto *label:{"メニュー","ﾒﾆｭｰ"}) {
        draw.right_soft_label=label;const auto p=steam_joystick_draw(right,draw);
        check(p&&p->operations.empty()&&right.entries[0].life==4,"两种菜单使用计数1，零有效life抑制旧life4");
    }
    draw.right_click_counts=std::array<int,2>{-1,99};draw.right_soft_label="メニュー";
    const auto capped=steam_joystick_draw(right,draw);
    check(capped&&std::get<SteamCursorRenderMode>(capped->operations[0]).source==255,"右点击计数超过4时alpha夹255");
    draw.right_soft_label="戻る";
    const auto negative=steam_joystick_draw(right,draw);
    check(negative&&negative->operations.empty(),"负右点击计数不绘制");
    draw.right_soft_label="其他";draw.right_click_counts.reset();
    const auto other=steam_joystick_draw(right,draw);
    check(other&&other->operations.size()==3,"不匹配标签使用登记life，不消费点击计数");
    draw.right_soft_label.reset();draw.has_controller=true;
    check(bool(steam_joystick_draw(right,draw)),"有手柄不消费软键标签与计数");
    draw.has_controller=false;draw.use_steam=false;draw.xbox=true;
    check(!steam_joystick_draw(right,draw),"Xbox同样执行button7软键life分支");
    draw.xbox=false;
    check(bool(steam_joystick_draw(right,draw)),"非Steam非Xbox使用登记life");
    draw.use_steam=true;
    const auto old_triggers=steam_joystick_draw(second->state,draw);
    check(old_triggers&&old_triggers->operations.size()==8,"断开设备的旧8/9仍交下层provider，不在登记表draw擅自丢弃");
    for(int fault=0;fault<7;++fault) {
        SteamJoystickDrawInput bad;bad.application_type=4;
        if(fault==1)bad.scale_ratio=0.F;
        if(fault==2)bad.scale_ratio=std::numeric_limits<float>::quiet_NaN();
        if(fault==3)bad.scale_ratio=std::numeric_limits<float>::infinity();
        if(fault==4)bad.scale_ratio=10001.F;
        if(fault==5){bad.scale_ratio=100.F;bad.cursor_draw_frame=-1;}
        if(fault==6){bad.scale_ratio=100.F;bad.cursor_draw_frame=192;}
        check(!steam_joystick_draw(empty,bad),"有表必须拒绝坏ratio或frame，空表也不绕过");
    }
    for(int fault=0;fault<8;++fault) {
        auto bad=single;
        if(fault==0)bad.table_present=false;
        if(fault==1)bad.entries.push_back(bad.entries.front());
        if(fault==2)bad.entries[0].button=44;
        if(fault==3)bad.entries[0].registration=-2;
        if(fault==4)bad.entries[0].life=-1;
        if(fault==5)bad.entries[0].life=5;
        if(fault==6)bad.entries[0].scale=-1;
        if(fault==7)bad.entries[0].scale=10001;
        const auto before=bad;
        check(!steam_joystick_register(bad,input)&&!steam_joystick_reset(bad)&&!steam_joystick_raise(bad)&&
              !steam_joystick_release(bad,releases)&&!steam_joystick_draw(bad,{})&&same(bad,before),
              "各入口拒绝损坏表／重复key且保持旧状态");
    }
    SteamJoystickPromptState full{true,{}};
    for(int i=0;i<4096;++i)full.entries.push_back({43,i,false,{},0,100});
    input.request={43,4095,-1,{}};
    const auto full_reuse=steam_joystick_register(full,input);
    input.request={43,4096,100,{}};
    check(full_reuse&&full_reuse->registered&&full_reuse->state.entries.size()==4096&&
          !steam_joystick_register(full,input)&&full.entries.size()==4096&&!full.entries.back().displayed,
          "4096维护预算允许复用但拒绝新增；button43合法且输入仍不变");
    for(auto &entry:full.entries){entry.displayed=true;entry.life=4;entry.scale=50;}
    SteamJoystickDrawInput largest;largest.application_type=4;largest.scale_ratio=100.F;
    const auto maximum=steam_joystick_draw(full,largest);
    check(maximum&&maximum->operations.size()==20482&&
          std::holds_alternative<SteamCursorPushScale>(maximum->operations.front())&&
          std::holds_alternative<SteamCursorPushScale>(maximum->operations[1])&&
          std::holds_alternative<SteamCursorPopMatrix>(maximum->operations[20480])&&
          std::holds_alternative<SteamCursorPopMatrix>(maximum->operations.back()),
          "最大输出4096乘5加外矩阵两项，20482操作；内外矩阵按原顺序闭合");
    full.entries.push_back({43,4096,false,{},0,100});
    check(!steam_joystick_reset(full),"超过维护预算明确拒绝，不把4096声称原版硬上限");
}
void steam_language_checks(Checks &check,const std::filesystem::path &root) {
    using ResultKind=SteamLanguagePanelResultKind;
    SteamLanguagePanelResultInput result;
    check(!steam_language_panel_result(result),"尚未取得平台结果拒绝，不把缺载荷当null取消而提前Pop");
    result.kind=ResultKind::null_result;result.selection=-99;
    const auto cancelled=steam_language_panel_result(result);
    check(cancelled&&cancelled->actions.size()==1&&
          std::holds_alternative<SteamLanguagePopRequest>(cancelled->actions[0]),
          "平台null结果只Pop，不要求无关目录或当前包，不把未知取消产生者认证为此分支");
    result.kind=ResultKind::parsed_selection;result.selection=1;
    result.frozen_files={"unused.csv","DungeonVillage_SC.csv",std::nullopt};
    result.current_file_resolved=true;result.current_file="DungeonVillage_SC.csv";
    const auto same=steam_language_panel_result(result);
    check(same&&same->actions.size()==1&&std::holds_alternative<SteamLanguagePopRequest>(same->actions[0]),
          "选择当前冻结文件仅退出，不重新安装或重建Boot/标题");
    result.current_file="DungeonVillage_TC.csv";
    const auto changed=steam_language_panel_result(result);
    check(changed&&changed->actions.size()==3&&
          std::get_if<SteamLanguageInstallRequest>(&changed->actions[0])&&
          std::get<SteamLanguageInstallRequest>(changed->actions[0]).file==std::optional<std::string>{"DungeonVillage_SC.csv"}&&
          std::get<SteamLanguageInstallRequest>(changed->actions[0]).save&&
          std::holds_alternative<SteamLanguageSuccessCallbackRequest>(changed->actions[1])&&
          std::holds_alternative<SteamLanguagePopRequest>(changed->actions[2]),
          "新包请求严格安装并保存→成功条件读取共享callback→Pop，不先退页或无条件回调");
    result.selection=2;result.current_file.reset();
    const auto same_null=steam_language_panel_result(result);
    check(same_null&&same_null->actions.size()==1,
          "已解析有效行中的null文件与当前null相等，与平台结果null的入口语义分开");
    result.current_file="current.csv";
    const auto install_null=steam_language_panel_result(result);
    check(install_null&&install_null->actions.size()==3&&
          !std::get<SteamLanguageInstallRequest>(install_null->actions[0]).file,
          "选中null文件保留原SetLanguagePack(null,null,true)参数，不补包名");
    for(int selected:{-3,-2,-1,3,std::numeric_limits<int>::max()}) {
        result.selection=selected;
        check(!steam_language_panel_result(result),"负HTTP/模板/外链及越界索引显式拒绝，不当取消或map.at异常");
    }
    result.selection=1;
    for(int fault=0;fault<5;++fault) {
        auto bad=result;
        if(fault==0)bad.current_file_resolved=false;
        if(fault==1)bad.frozen_files[1]=std::string(4097,'x');
        if(fault==2)bad.current_file=std::string(4097,'x');
        if(fault==3)bad.frozen_files.resize(4097);
        if(fault==4)bad.kind=static_cast<ResultKind>(99);
        check(!steam_language_panel_result(bad),"缺实际当前值、已消费超预算或坏结果类型明确拒绝");
    }
    result.frozen_files[0]=std::string(4097,'x'); // 未选行不在原本分支消费。
    result.current_file="dungeonVillage_SC.csv";
    const auto case_changed=steam_language_panel_result(result);
    check(case_changed&&case_changed->actions.size()==3&&
          steam_language_panel_result(result)->actions.size()==3&&
          result.current_file==std::optional<std::string>{"dungeonVillage_SC.csv"},
          "按文件串精确比较不折叠大小写，未消费行不补验证，重复计划不安装/写偏好/保存回调");
    const auto directory=root.parent_path()/"steam-platform";
    CpuImage button(LoadImage((directory/"button_language.png").string().c_str()));
    const auto button_seb=tools::parse_legacy_seb(read_bytes(directory/"button_language.seb"));
    check(button.image.width==60&&button.image.height==60&&button_seb.layers.size()==1&&
          button_seb.frame_count==1&&button_seb.layers[0].parts.size()==1,"语言按钮实际原资源独立于键盘/控制器按钮");
    const auto &part=button_seb.layers[0].parts[0];
    check(part.frame==0&&part.image_index==0&&part.source_x==0&&part.source_y==0&&part.width==60&&
          part.height==60&&part.offset_x==-30&&part.offset_y==-30&&part.flip_x==0&&part.flip_y==0,
          "语言SEB0/image0保留原60方裁片与中心偏移");
    CpuImage badge(LoadImage((root.parent_path()/steam_language_new_image_path).string().c_str()));
    const auto new_seb=tools::parse_legacy_seb(read_bytes(root.parent_path()/steam_language_new_sprite_path));
    check(badge.image.width==28&&badge.image.height==22&&new_seb.frame_count==16&&
          new_seb.layers.size()==1&&new_seb.layers[0].parts.size()==4,"NEW使用真实图字和关键帧，不以字体文字替代");
    for(std::size_t i=0;i<4;++i) {
        const auto &p=new_seb.layers[0].parts[i];
        check(p.frame==std::array<int,4>{0,7,8,15}[i]&&p.image_index==24&&p.source_x==0&&
              p.source_y==(i<2?0:11)&&p.width==28&&p.height==11&&p.offset_x==-14&&p.offset_y==-5&&
              p.flip_x==0&&p.flip_y==0,"原NEW四关键帧及两种裁片保持，标题只显式消费第8帧");
    }
    SteamLanguageButtonInput in;in.switch_enabled=true;in.width=241;in.height=321;in.scale_ratio=100.F;
    const auto plain=steam_language_button_skin(in);
    using Step=SteamLanguageButtonStep;
    const std::vector<Step> base_steps{Step::push_origin_zero,Step::replace_callback,Step::push_origin_zero,
        Step::draw_button,Step::register_touch,Step::register_joystick,Step::pop_origin,Step::pop_origin};
    check(plain&&plain->body&&!plain->new_text&&plain->steps==base_steps&&
          plain->body->destination==std::array<float,4>{206.F,5.F,30.F,30.F}&&
          plain->body->touch_rectangle==std::array<int,4>{206,5,30,30}&&
          plain->body->margin==std::array<int,4>{16,16,8,8}&&plain->collision_mode==1&&plain->screen_mode==1&&
          plain->body->component==3&&plain->body->value==0x133A066&&!plain->body->blend&&!plain->body->reset_origin&&
          plain->body->resource.image==0&&plain->body->resource.sprite==0&&
          plain->body->joystick&&plain->body->joystick->button==9&&plain->body->joystick->registration==2&&
          plain->body->joystick->scale==100&&plain->body->joystick->anchor==std::array<int,2>{235,6},
          "标题anchor20在奇数宽的右上定位，callback在两层原点之间替换，先绘制后注册");
    in.touched=true;in.new_state=2;
    for(const auto [old_frame,next_frame,y]:std::array<std::array<int,3>,3>{{{0,1,36},{7,8,35},{15,0,36}}}) {
        in.new_frame=old_frame;
        const auto plan=steam_language_button_skin(in);
        auto steps=base_steps;steps.insert(steps.begin()+4,Step::draw_new);
        check(plan&&plan->body&&plan->new_text&&plan->steps==steps&&
              plan->body->destination==std::array<float,4>{204.5F,3.5F,33.F,33.F}&&
              plan->body->touch_rectangle==plain->body->touch_rectangle&&plan->body->blend&&
              plan->body->blend->mode==1&&plan->body->blend->rgba==std::array<int,4>{255,255,255,128}&&
              plan->new_text->anchor==std::array<float,2>{220.5F,float(y)}&&
              plan->new_text->next_frame==next_frame&&plan->new_text->frame==8&&plan->new_text->layer==-1&&
              plan->new_text->scale==0.75F&&in.new_frame==old_frame,
              "高亮只缩按钮不扩热区；NEW先算下一钟再独立摆动，图字始终frame8/scale0.75");
        const auto repeated=steam_language_button_skin(in);
        check(repeated&&repeated->steps==steps&&repeated->new_text->next_frame==next_frame,
              "只读重复计划不写callback/NEW钟或新增累计登记，副作用待实际准入一次消费");
    }
    in.anchor=2|4|32|64;in.touched=false;in.new_state=0;in.new_frame=-1;
    const auto centered=steam_language_button_skin(in);
    check(centered&&centered->body->touch_rectangle==std::array<int,4>{105,145,30,30}&&!centered->new_text,
          "居中位优先于右下位且整数除2；无NEW资格不消费无效可选计数");
    in.width=1;in.height=1;in.anchor=4|64;in.scale_ratio=50.F;
    const auto negative=steam_language_button_skin(in);
    check(negative&&negative->body->joystick->anchor==std::array<int,2>{-2,-16},
          "原负锚手柄位置乘比例后向零截断，不floor也不缩放按钮本体锚");
    for(int fault=0;fault<7;++fault) {
        auto bad=in;
        if(fault==0)bad.width=0;if(fault==1)bad.height=65537;if(fault==2)bad.scale_ratio.reset();
        if(fault==3)bad.scale_ratio=std::numeric_limits<float>::infinity();if(fault==4)bad.scale_ratio=0.F;
        if(fault==5){bad.new_state=2;bad.new_frame.reset();}if(fault==6){bad.new_state=2;bad.new_frame=16;}
        check(!steam_language_button_skin(bad),"已消费尺寸/比例/NEW计数明确拒绝，不补默认值");
    }
    in.switch_enabled=false;in.width=0;in.scale_ratio.reset();in.new_state=2;in.new_frame.reset();
    const auto hidden=steam_language_button_skin(in);
    check(hidden&&!hidden->body&&!hidden->new_text&&hidden->steps.empty(),
          "Switch资格不满足合法隐藏，不替换callback/推进NEW或消费其它字段");
    for(int event:{-1,0,1,2,5}) {
        const auto matched=steam_language_button_touch(3,0x133A066,event);
        const auto wrong_component=steam_language_button_touch(2,0x133A066,event);
        const auto wrong_value=steam_language_button_touch(3,0x133A067,event);
        check(!matched.propagate&&matched.open_language==(event==1)&&wrong_component.propagate&&
              !wrong_component.open_language&&wrong_value.propagate&&!wrong_value.open_language,
              "只在匹配UP1请求入语言页，匹配其它事件截断分发，不匹配继续原链");
    }
    // TitleForm调用者资格另验；下层按钮资源/高亮/触区组合由上面的主责检查覆盖。
    SteamTitlePlatformInput title;
    title.width=241;title.height=321;title.title_frame=74;title.state=0;title.application_type=4;
    title.on_top=true;title.desktop=true;title.language_switch_enabled=true;title.scale_ratio=100.F;
    title.new_state=2;title.new_frame=7;
    for(int frame:{0,9,10,74}) {
        title.title_frame=frame;
        const auto p=steam_title_platform_skin(title);
        check(p&&p->language.body&&p->language.new_text&&p->after_menu.empty()&&
              p->language.body->destination==std::array<float,4>{206.F,5.F,30.F,30.F}&&
              p->language.new_text->next_frame==8&&title.new_frame==7,
              "语言在Logo之后但75菜单门槛之前，早期标题仍给语言请求，不实际推进NEW");
    }
    title.title_frame=75;
    const auto visible=steam_title_platform_skin(title);
    check(visible&&visible->language.body&&visible->after_menu.size()==2,
          "第75帧起菜单后另给两平台按钮，语言保持在菜单前独立层");
    const auto &binding=visible->after_menu[0],&keyboard=visible->after_menu[1];
    check(binding.resource.sprite==23&&binding.component==2&&binding.destination==std::array<float,4>{4.F,5.F,32.F,30.F}&&
          binding.joystick&&binding.joystick->button==8&&binding.joystick->registration==24&&
          keyboard.resource.sprite==25&&keyboard.component==2&&!keyboard.joystick&&
          keyboard.destination==std::array<float,4>{5.F,40.5F,32.F,21.F},
          "菜单后绑定20/20先于键盘21/51，图元和手柄登记请求不能混为同一序列");
    for(int branch=0;branch<5;++branch) {
        auto hidden=title;
        if(branch==0)hidden.on_top=false;
        if(branch==1)hidden.state=1;
        if(branch==2)hidden.application_type=3;
        if(branch==3)hidden.desktop=false;
        if(branch==4)hidden.xbox=true;
        const auto p=steam_title_platform_skin(hidden);
        check(p&&bool(p->language.body)==(branch>=2)&&p->after_menu.size()==(branch==4?1U:0U),
              "顶页/选档隐藏全部；APPLI_TYPE和Platform各自挡平台但不挡语言；Xbox只挡键盘");
    }
    title.controller_touched=true;title.keyboard_touched=true;title.language_touched=true;
    title.title_frame=std::numeric_limits<int>::max()-1;
    const auto lit=steam_title_platform_skin(title);
    check(lit&&lit->language.body->blend&&lit->after_menu.size()==2&&
          lit->after_menu[0].blend&&lit->after_menu[1].blend&&title.new_frame==7,
          "三路独立CheckTouch传递且最大标题钟不溢出，不从鼠标悬停或逻辑钟猜高亮/NEW");
    title.language_switch_enabled=false;title.new_frame.reset();title.width=0;
    const auto no_language=steam_title_platform_skin(title);
    check(no_language&&!no_language->language.body&&no_language->after_menu.size()==2,
          "语言禁用不消费NEW或画布字段，也不屏蔽平台两按钮");
    title.scale_ratio.reset();
    check(!steam_title_platform_skin(title),"可见绑定按钮缺比例时整份候选拒绝，不返回部分按钮");
    title.title_frame=74;
    const auto none=steam_title_platform_skin(title);
    check(none&&!none->language.body&&none->after_menu.empty(),"门槛前隐藏平台不要求其未消费比例");
    for(int fault=0;fault<4;++fault) {
        auto bad=title;bad.on_top=false;
        if(fault==0)bad.title_frame=-1;if(fault==1)bad.title_frame=std::numeric_limits<int>::max();
        if(fault==2)bad.state=-1;if(fault==3)bad.state=2;
        check(!steam_title_platform_skin(bad),"坏标题身份/计数即使被覆盖也明确拒绝");
    }
}
void steam_cursor_checks(Checks &check,const std::filesystem::path &root) {
    const auto assets=root.parent_path();
    CpuImage image(LoadImage((assets/steam_cursor_image_path).string().c_str()));
    const auto seb=tools::parse_legacy_seb(read_bytes(assets/steam_cursor_sprite_path));
    check(image.image.width==311&&image.image.height==236&&seb.frame_count==2&&seb.layers.size()==4&&
          seb.layers[1].parts.size()==2,"虚拟光标原PNG与SEB，主责消费layer1两帧而不冒称全层消费者");
    for(int frame=0;frame<2;++frame) {
        const auto &part=seb.layers[1].parts[frame];
        check(part.frame==frame&&part.image_index==12&&part.source_x==frame*64&&part.source_y==0&&
              part.width==64&&part.height==64&&part.offset_x==(frame?-16:-9)&&part.offset_y==(frame?-8:-11)&&
              part.flip_x==0&&part.flip_y==0,"光标两帧分别使用真实裁片和不同热点，不画整PNG或共用偏移");
        CpuImage crop(ImageFromImage(image.image,{float(part.source_x),0,64,64}));
        check(crop.image.data!=nullptr,"实际消费两帧原裁片且作用域释放");
    }
    // 独立具名组合：反转不二次读取A，Y键的GUI限制不传播到Steam Y。
    for(const auto &sample:std::array<std::pair<SteamCursorPressInput,bool>,8>{{
        {{271,8,271,0,false,true,false,false,false},true},
        {{13,8,271,0,false,true,false,false,false},false},
        {{271,8,271,0,true,true,false,false,false},false},
        {{271,8,271,0,true,false,true,false,false},true},
        {{271,8,271,1,false,false,false,true,false},false},
        {{271,8,271,1,false,false,false,false,true},true},
        {{271,8,13,0,false,false,false,true,true},false},
        {{-1,-1,271,0,false,false,false,true,false},true}}})
        check(steam_cursor_pressed(sample.first)==sample.second,"按下图仅按实际顶页映射、反转与Y资格选择");
    SteamCursorSkinInput in;in.draw_frame=191;in.position=std::array<float,2>{17.9F,22.9F};
    in.application_type=4;in.screen_size={1001,701};in.canvas_size={801,600};
    const auto plain=steam_cursor_skin(in);
    check(plain&&plain->next_draw_frame==0&&plain->operations.size()==1&&
          std::get<SteamCursorDraw>(plain->operations[0]).anchor==std::array<int,2>{117,72},
          "drawFrame先回卷；浮点位置先向零截断、再加奇数尺寸差的一半，不重复缩放");
    struct RecoveryOracle {int count;float scale;int alpha;};
    for(const auto &expected:std::array<RecoveryOracle,5>{{
            {1,1.19168162346F,2},{3,1.76930403709F,11},{6,3.69887304306F,39},
            {11,10.3583488464F,136},{12,12.F,160}}}) {
        in.recover=expected.count;in.pressed=true;
        const auto p=steam_cursor_skin(in);
        check(p&&p->operations.size()==6&&p->next_draw_frame==0,"恢复仅产生固定六条原序请求，不自行递减N");
        const auto &base=std::get<SteamCursorDraw>(p->operations[0]);
        const auto &scale=std::get<SteamCursorPushScale>(p->operations[1]);
        const auto &blend=std::get<SteamCursorRenderMode>(p->operations[2]);
        const auto &overlay=std::get<SteamCursorDraw>(p->operations[3]);
        const auto &reset=std::get<SteamCursorRenderMode>(p->operations[4]);
        check(base.anchor==std::array<int,2>{117,72}&&base.frame==1&&base.layer==1&&
              overlay.anchor==base.anchor&&overlay.frame==1&&overlay.layer==1&&
              scale.x==expected.scale&&scale.y==expected.scale&&scale.pivot==std::array<int,2>{0,0}&&
              blend.operation==1&&blend.source==expected.alpha&&blend.destination==255-expected.alpha&&
              reset.operation==0&&reset.source==255&&reset.destination==0&&
              std::holds_alternative<SteamCursorPopMatrix>(p->operations[5]),
              "独立Easing数值及有序PushScale/混合/同锚Draw/默认重置/Pop，不能预缩锚或恢复旧混合");
        const auto repeated=steam_cursor_skin(in);
        check(repeated&&repeated->operations.size()==6&&in.recover==expected.count&&in.draw_frame==191,
              "重复计划查询不推进输入计数或累积图元/事件");
    }
    in.recover=0;in.application_type=0;in.position=std::array<float,2>{-17.9F,-22.9F};
    in.screen_size={0,0};in.canvas_size={0,0};
    const auto signed_anchor=steam_cursor_skin(in);
    check(signed_anchor&&std::get<SteamCursorDraw>(signed_anchor->operations[0]).anchor==std::array<int,2>{-17,-22},
          "Draw局部负值按向零截断；非type4不消费尺寸，此夹具不冒称SetScreenCursor合法路径");
    for(int fault=0;fault<6;++fault) {
        auto bad=in;
        if(fault==0)bad.draw_frame=192;if(fault==1)bad.draw_frame=-1;
        if(fault==2)bad.recover=13;if(fault==3)(*bad.position)[0]=std::numeric_limits<float>::quiet_NaN();
        if(fault==4)(*bad.position)[1]=65537.F;if(fault==5)bad.application_type=4;
        check(!steam_cursor_skin(bad),"非法计数/已消费坐标/平台尺寸明确拒绝，不夹值凑绘制");
    }
    in.joystick=true;in.recover=-1;in.application_type=4;
    (*in.position)[0]=std::numeric_limits<float>::infinity();
    auto hidden=steam_cursor_skin(in);
    check(hidden&&hidden->next_draw_frame==0&&hidden->operations.empty(),
          "Joystick隐藏仍推进drawFrame，不提前消费坐标/恢复值/尺寸");
    in.joystick=false;in.position.reset();hidden=steam_cursor_skin(in);
    check(hidden&&hidden->next_draw_frame==0&&hidden->operations.empty(),
          "无位置数组同样合法隐藏，不猜造鼠标坐标");
}
struct SpriteOracle {
    int sprite, image, frames;
    const char *sprite_name, *image_name, *sprite_hash, *image_hash;
};
constexpr std::array<SpriteOracle,3> sprites{{
    {31,172,10,"chara_hisho.seb","chara_hishoko.png","a67b143fa460e36e0cc6aeb869d0ecb3bdd87f02cf9f6f268179f2e0cdf65a82","d5aa8408c0fb3d2d08ec3dcfbddcf9b40e7b11e01745c74366663e4ba2727153"},
    {46,171,12,"chara_president.seb","chara_president.png","cc62ef04c7db627893a4083fc451973119ff80dfe0a0708fcbbaba662e4b586d","a339413720c507679501ff05fc560724a588569b9627c288927244984822e543"},
    {2,72,2,"arrow01.seb","arrow01.png","15036548a2810b4e8b0b3837d7bf6e49876c4419084efdd5b37f13f1c87f80f8","864fcf16e52634142f776b6245f0964ad12a21d901a4af7308aa6ee939f1ca65"}
}};
tools::SpritePart part(const AssetTables &assets, const StartupSkinDraw &draw, Checks &check) {
    const auto expected=std::find_if(sprites.begin(),sprites.end(),[&](const auto &p){return p.sprite==draw.sprite;});
    check(expected!=sprites.end() && draw.package==Package::common && draw.layer==0,
          "仅消费本批三种具名SEB原层");
    check(draw.sprite>=0 && static_cast<std::size_t>(draw.sprite)<assets.sprites.size() &&
              assets.sprites[draw.sprite].filename()==expected->sprite_name,"真实seb.inf原序绑定");
    const auto raw=read_bytes(assets.sprites[draw.sprite]);
    check(tools::sha256_hex(raw)==expected->sprite_hash,"原SEB冻结哈希");
    const auto seb=tools::parse_legacy_seb(raw);
    check(seb.layers.size()==1 && seb.frame_count==expected->frames && draw.frame>=0 &&
              draw.frame<seb.frame_count,"原单层SEB与请求帧边界，不将legacy_tag当计数");
    const auto &records=seb.layers[0].parts;
    const auto found=std::find_if(records.begin(),records.end(),[&](const auto &p){return p.frame==draw.frame;});
    check(found!=records.end() && found->image_index==expected->image && draw.image==expected->image &&
              found->flip_x==0 && found->flip_y==0,"原指定图片及本批实际无翻转层");
    if(draw.sprite==2)
        check(draw.frame==0 && found->source_x==0 && found->source_y==0 && found->width==7 &&
                  found->height==5 && found->offset_x==0 && found->offset_y==0,
              "继续提示只画7x5 frame0，不画整14x5图");
    else {
        // 页17只请求六个有效角色帧，明确不开放大臣10/11历史越界记录。
        check((draw.sprite==31 && (draw.frame==0||draw.frame==4||draw.frame==6)) ||
                  (draw.sprite==46 && (draw.frame==1||draw.frame==5||draw.frame==7)),"计分仅用原角色合法帧子集");
        const int expected_x=draw.frame==0||draw.frame==1?0:draw.frame==4||draw.frame==5?30:45;
        check(found->source_x==expected_x && found->source_y==(draw.sprite==31?0:22) &&
                  found->width==15 && found->height==22 && found->offset_x==-7 && found->offset_y==-22,
              "角色原15x22裁片与脚底偏移literal oracle");
    }
    return *found;
}
void sprite_pixels(const AssetTables &assets, Checks &check, Image *contact_sheet) {
    for(const auto &asset:sprites) {
        const auto image=assets.common.find(asset.image);
        check(image!=assets.common.end() && image->second.filename()==asset.image_name &&
                  tools::sha256_hex(read_bytes(image->second))==asset.image_hash,
              "原common稀疏img.inf解析绑定角色和箭头PNG");
    }
    // 静止及兴奋A/B共六个原角色帧；页面背景和文字不进入角色像素夹具。
    int column{};
    constexpr std::array<std::array<int,2>,3> samples{{{0,0},{4,0},{4,4}}};
    for(const auto &sample : samples) {
        const auto skin=startup_clear_skin(sample[0],0,sample[1]);
        check(skin.has_value(),"CPU角色有效演出条件夹具");
        CpuImage panel(GenImageColor(240,200,BLANK));
        for(const auto &plan:skin->actors) {
            const auto record=part(assets,plan,check);
            CpuImage source(LoadImage(assets.common.at(plan.image).string().c_str()));
            check(source.image.width==75 && source.image.height==44 && record.source_x+record.width<=75 &&
                      record.source_y+record.height<=44,"实际角色PNG与所用SEB裁片完整在界");
            const std::array<int,4> crop{record.source_x,record.source_y,record.width,record.height};
            CpuImage clipped(ImageFromImage(source.image,rectangle(crop)));
            draw(panel.image,clipped.image,{0,0,15,22},plan.offset[0]+record.offset_x,
                 plan.offset[1]+record.offset_y);
            const int expected_x=plan.image==171?183:44;
            int opaque{};
            for(int y=0;y<22;++y)
                for(int x=0;x<15;++x) {
                    const auto pixel=GetImageColor(source.image,record.source_x+x,record.source_y+y);
                    if(pixel.a==255)
                        check(same(GetImageColor(panel.image,expected_x+x,159+y),pixel),
                              "原页面锚加SEB(-7,-22)仅一次，实际不透明源像素一致");
                    opaque+=pixel.a>0;
                }
            check(opaque>0,"实际角色帧裁片非空");
        }
        for(int y=0;y<panel.image.height;++y)
            for(int x=0;x<panel.image.width;++x)
                if(!(y>=159 && y<181 && ((x>=44 && x<59)||(x>=183 && x<198))))
                    check(GetImageColor(panel.image,x,y).a==0,"角色片不能被二次SEB偏移画出原矩形");
        if(contact_sheet)
            draw(*contact_sheet,panel.image,{40,155,162,30},12+192*column,468);
        ++column;
    }
    const auto normal=startup_clear_skin(0,75,21);
    check(normal && normal->continue_marker,"CPU继续箭头具名条件");
    for(const auto &actor:normal->actors)
        (void)part(assets,actor,check); // 正常frame1／0与兴奋4/5/6/7同样认证。
    const auto arrow=*normal->continue_marker;
    const auto record=part(assets,arrow,check);
    CpuImage source(LoadImage(assets.common.at(72).string().c_str()));
    check(source.image.width==14 && source.image.height==5,"原箭头PNG14x5载体");
    CpuImage crop(ImageFromImage(source.image,{0,0,7,5}));
    int opaque{};
    for(int y=0;y<5;++y)
        for(int x=0;x<7;++x) {
            const auto pixel=GetImageColor(crop.image,x,y);
            check(same(pixel,GetImageColor(source.image,record.source_x+x,record.source_y+y)),
                  "继续箭头CPU像素来自原frame0前半图");
            opaque+=pixel.a>0;
        }
    check(opaque>0,"继续箭头非空，不补演示图");
    if(contact_sheet)draw(*contact_sheet,crop.image,{0,0,7,5},536,524);
}
void frame_geometry(Checks &check) {
    const auto records=startup_window_skin(200,170,0,0,-12,73);
    check(records && records->images.size()==6 && records->title,
          "纪录窗五块非空木纹与标题带；源第六个零宽请求不形成绘制");
    check(records->borders[0].rect==std::array<int,4>{19,21,202,173} &&
          records->borders[1].rect==std::array<int,4>{20,22,200,171} &&
          records->borders[0].rgb==std::array<int,3>{89,103,91} &&
          records->borders[1].rgb==std::array<int,3>{239,239,221} &&
          records->borders[0].outline && records->borders[1].outline,
          "原o.d末像素差值转半开尺寸，不能漏底边或多加一次1");
    check((*records->title)[0].offset==std::array<int,2>{84,26} &&
          (*records->title)[1].offset==std::array<int,2>{84,25} &&
          (*records->title)[0].rgb==std::array<int,3>{44,54,105} &&
          (*records->title)[1].rgb==std::array<int,3>{247,253,247},
          "实际测宽73向零整数居中，阴影先画且低一像素");
    const auto odd=startup_window_skin(201,171,3,-1,-12);
    check(odd && !odd->title && odd->images.size()==7 &&
          odd->borders[0].rect==std::array<int,4>{19,21,203,174} &&
          odd->images[5].crop==std::array<int,4>{0,0,1,171} &&
          odd->images[5].offset==std::array<int,2>{220,23} &&
          odd->images[6].crop==std::array<int,4>{21,0,199,17},
          "奇数窗/sceneTop分别整数除法，尾木纹裁1且标题带按left取源");
    const auto low=startup_window_skin(240,216,24,0,0);
    const auto high=startup_window_skin(240,217,std::numeric_limits<int>::max(),0,
                                         std::numeric_limits<int>::max());
    check(low && high && low->images[0].offset==std::array<int,2>{0,24} &&
          high->images[0].offset==std::array<int,2>{0,12},
          "216保留sceneTop，217忽略sceneTop/style而不溢出无用算式");
    const auto content=startup_content_skin(17,81,224,177,-3);
    check(content && content->rectangles[0].rect==std::array<int,4>{17,80,207,96} &&
          content->rectangles[1].rect==std::array<int,4>{17,80,207,96} &&
          content->rectangles[2].rect==std::array<int,4>{18,81,205,94} &&
          !content->rectangles[0].outline && content->rectangles[1].outline &&
          content->rectangles[2].outline,"内容填充/双线和负奇数sceneTop向零截断");
    constexpr std::array<std::array<int,2>,4> anchors{{{17,80},{224,80},{224,176},{17,176}}};
    for(int i=0;i<4;++i)
        check(content->corners[i].package==Package::common && content->corners[i].image==30 &&
              content->corners[i].sprite==6 && content->corners[i].frame==i &&
              content->corners[i].layer==0 && content->corners[i].offset==anchors[i],
              "内容框保留原顺时针SEB角锚，不能先减4又加SEB偏移");
    check(!startup_window_skin(2,170,0,0,0) && !startup_window_skin(241,170,0,0,0) &&
          !startup_window_skin(200,16,0,0,0) && !startup_window_skin(200,241,0,0,0) &&
          !startup_window_skin(200,170,0,0,0,-1) &&
          !startup_window_skin(200,170,std::numeric_limits<int>::max(),0,0) &&
          !startup_window_skin(200,170,0,std::numeric_limits<int>::max(),0) &&
          !startup_window_skin(200,170,0,std::numeric_limits<int>::min(),-200),
          "维护裁片/字宽/坐标拒绝与原Java溢出行为分开");
    check(!startup_content_skin(17,81,16,177,0) && !startup_content_skin(0,0,3,10,0) &&
          !startup_content_skin(0,0,10,3,0) &&
          !startup_content_skin(std::numeric_limits<int>::min(),0,std::numeric_limits<int>::max(),10,0) &&
          !startup_content_skin(0,0,10,std::numeric_limits<int>::max(),2),
          "内容框坏顺序/过小边/溢出显式拒绝");
}
void paint_rect(Image &image,const StartupSkinRect &part) {
    const auto &r=part.rect;
    const Color color{static_cast<unsigned char>(part.rgb[0]),static_cast<unsigned char>(part.rgb[1]),
                      static_cast<unsigned char>(part.rgb[2]),255};
    if(!part.outline) { ImageDrawRectangle(&image,r[0],r[1],r[2],r[3],color); return; }
    ImageDrawRectangle(&image,r[0],r[1],r[2],1,color);
    ImageDrawRectangle(&image,r[0],r[1]+r[3]-1,r[2],1,color);
    ImageDrawRectangle(&image,r[0],r[1],1,r[3],color);
    ImageDrawRectangle(&image,r[0]+r[2]-1,r[1],1,r[3],color);
}
void frame_pixels(const AssetTables &assets,Checks &check,Image *contact_sheet) {
    constexpr std::array<int,3> ids{{28,29,30}};
    constexpr std::array<const char*,3> names{{"wnd_back.png","wnd_bar.png","wnd_conner.png"}};
    constexpr std::array<const char*,3> hashes{{
        "9c3d7fbc12329f0b7894bbb0581f268e045ff16031853686b5e006d02c843eb1",
        "67b99691cb848805a0070d4f2924c7931da02de6f29191dcb7443e634a8d988e",
        "15f03b0851257a973487a5ef5e7045d18adda4c017f0f3b6dc3491e7b3602079"}};
    constexpr std::array<std::array<int,2>,3> sizes{{{40,240},{240,17},{8,8}}};
    for(int i=0;i<3;++i) {
        const auto found=assets.common.find(ids[i]);
        check(found!=assets.common.end() && found->second.filename()==names[i] &&
              tools::sha256_hex(read_bytes(found->second))==hashes[i],"三种共用窗PNG绑定与冻结身份");
        CpuImage image(LoadImage(found->second.string().c_str()));
        check(image.image.width==sizes[i][0] && image.image.height==sizes[i][1],"三种窗PNG尺寸");
    }
    check(assets.sprites.size()>6 && assets.sprites[6].filename()=="wnd_conner.seb" &&
          tools::sha256_hex(read_bytes(assets.sprites[6]))==
          "cfab2deba8d8c3433498fe05c8fa4bacaa0cba3ae29495cb82c36e4d4df58618","四角SEB原身份");
    const auto corners=tools::parse_legacy_seb(read_bytes(assets.sprites[6]));
    check(corners.frame_count==4 && corners.layers.size()==1 && corners.layers[0].parts.size()==4,
          "四角单层四帧，无拉伸九宫格假设");
    constexpr std::array<std::array<int,4>,4> expected{{{0,0,0,0},{4,0,-4,0},{4,4,-4,-4},{0,4,0,-4}}};
    for(int i=0;i<4;++i) {
        const auto &p=corners.layers[0].parts[i];
        check(p.frame==i && p.image_index==30 && p.width==4 && p.height==4 &&
              p.source_x==expected[i][0] && p.source_y==expected[i][1] &&
              p.offset_x==expected[i][2] && p.offset_y==expected[i][3] && !p.flip_x && !p.flip_y,
              "角4x4与右/下负偏移均来自SEB，不推测镜像");
    }
    constexpr std::array<std::array<int,3>,3> shapes{{{200,170,-12},{220,186,0},{222,155,0}}};
    for(int i=0;i<3;++i) {
        CpuImage panel(GenImageColor(240,240,BLANK));
        const auto plan=startup_window_skin(shapes[i][0],shapes[i][1],0,0,shapes[i][2]);
        check(plan.has_value(),"三个真实窗口尺寸夹具有效");
        for(const auto &r:plan->borders) paint_rect(panel.image,r);
        for(const auto &p:plan->images) {
            CpuImage image(LoadImage(assets.common.at(p.image).string().c_str()));
            check(p.crop[0]>=0 && p.crop[1]>=0 && p.crop[2]>0 && p.crop[3]>0 &&
                  p.crop[0]+p.crop[2]<=image.image.width && p.crop[1]+p.crop[3]<=image.image.height,
                  "木纹与标题带所有请求实际PNG内、没有零宽裁剪");
            draw(panel.image,image.image,p.crop,p.offset[0],p.offset[1]);
        }
        if(i==0) {
            check(same(GetImageColor(panel.image,19,21),{89,103,91,255}) &&
                  same(GetImageColor(panel.image,20,22),{239,239,221,255}) &&
                  same(GetImageColor(panel.image,220,193),{89,103,91,255}) &&
                  GetImageColor(panel.image,221,193).a==0,"框外线极值像素及外侧透明边界");
            CpuImage bar(LoadImage(assets.common.at(29).string().c_str()));
            check(same(GetImageColor(panel.image,21,23),GetImageColor(bar.image,21,0)),
                  "标题带沿原横坐标裁片，不把整图横向缩放");
        }
        if(i>0) {
            const auto inner=i==1?startup_content_skin(17,48,219,193,0):
                                  startup_content_skin(17,81,224,177,0);
            check(inner.has_value(),"原91/17内容框夹具");
            for(const auto &r:inner->rectangles) paint_rect(panel.image,r);
            CpuImage image(LoadImage(assets.common.at(30).string().c_str()));
            for(int j=0;j<4;++j) {
                const auto &p=corners.layers[0].parts[j];
                const auto &a=inner->corners[j].offset;
                draw(panel.image,image.image,{p.source_x,p.source_y,4,4},a[0]+p.offset_x,a[1]+p.offset_y);
            }
            const int y=i==1?48:81;
            check(same(GetImageColor(panel.image,22,y+5),{247,253,247,255}) &&
                  same(GetImageColor(panel.image,22,y),{172,202,179,255}) &&
                  same(GetImageColor(panel.image,22,y+1),{222,234,225,255}),"实际内容框填充和两条1像素边");
        }
        if(contact_sheet) draw(*contact_sheet,panel.image,{0,0,240,240},10+250*i,680);
    }
}
// 标题/纪录基础人物的资源展开；不复制世界人物动作机或把实际PNG列数当动画顺序。
void steam_title_actor_projection_checks(Checks &check) {
    std::array<StartupTitleSlot,20> slots{};
    for(auto &s:slots)s.y=217;
    slots[0]={1,4,12,212,1,19};slots[1]={1,5,14,212,0,20};slots[2]={0,0,0,210,0,0};
    const auto plan=steam_title_actor_projection(slots,277);
    check(plan&&plan->people.size()==2&&plan->people[0].person.slot==1&&plan->people[1].person.slot==0,
          "Steam完整20槽先非稳定交换再过滤，inactive低y使相等深度的两活跃人反序");
    const auto &left=plan->people[0],&right=plan->people[1];
    check(left.person.definition==5&&left.person.step==0&&left.person.facing==2&&
          right.person.definition==4&&right.person.step==3&&right.person.facing==1&&
          right.person.anchor==std::array<int,2>{12,249}&&!left.edge_mode&&!right.edge_mode&&
          right.person.layers==std::array<StartupTitlePersonLayer,2>{StartupTitlePersonLayer::shadow,StartupTitlePersonLayer::body},
          "来源人物实参使用view高度，中央不设也不重置混合，保持阴影→人物助手");
    slots={};
    for(const auto [x,source]:std::array<std::array<int,2>,10>{{
        {-18,0},{-17,64},{-16,128},{-15,192},{-14,-1},{250,-1},{251,192},{252,128},{253,64},{254,0}}}) {
        slots[0]={1,0,x,210,1,0};
        const auto p=steam_title_actor_projection(slots,320);
        check(p&&p->people.size()==1&&p->people[0].person.anchor==std::array<int,2>{x,290}&&
              bool(p->people[0].edge_mode)==(source>=0),
              "Steam活跃边界独立为-18..254，不能套APK的-10..250校验或提前退休");
        if(source>=0)check(*p->people[0].edge_mode==std::array<int,3>{1,source,255-source},
              "Steam边缘四级src/dst按独立oracle，绘后才重置默认模式");
    }
    slots[0]={0,7,255,216,1,2147483646};slots[1]={0,3,-19,215,0,8};
    const auto retired=steam_title_actor_projection(slots,320);
    check(retired&&retired->people.empty()&&slots[0].age==2147483646&&slots[1].x==-19,
          "退休端点和age/y合法保留但不输出图元，投影不清历史或复用槽");
    for(auto &s:slots)s={1,10,1,217,0,2147483646};
    const auto full=steam_title_actor_projection(slots,65536);
    check(full&&full->people.size()==20&&full->people[0].person.step==1&&
          full->people[0].person.anchor==std::array<int,2>{1,65513}&&
          steam_title_actor_projection(slots,65536)->people.size()==20,
          "最大合法计数与维护高度可投影，输出最多20且重复查询无累积");
    for(int fault=0;fault<9;++fault) {
        auto bad=slots;int height=320;
        if(fault==0)bad[0].active=2;if(fault==1)bad[0].definition=11;if(fault==2)bad[0].direction=2;
        if(fault==3)bad[0].age=std::numeric_limits<int>::max();if(fault==4)bad[0].x=255;
        if(fault==5){bad[0].active=0;bad[0].x=256;}if(fault==6)bad[0].y=209;
        if(fault==7)height=0;if(fault==8)height=65537;
        check(!steam_title_actor_projection(bad,height),"坏Steam人物槽/投影尺寸明确拒绝，不过滤坏载荷凑正常画面");
    }
}
void title_actor_pixels(const std::filesystem::path &root,Checks &check,Image *contact_sheet) {
    const auto humans=image_table(root,"human",check),weapons=image_table(root,"weapon",check);
    check(humans.count(32)==0 && humans.size()==37,"human32缺资源，不能补透明图蒙混通过");
    std::vector<tools::SpriteDefinition> body,arms;
    std::vector<std::uint8_t> source_sequence;
    for(const char *package:{"human","weapon"}) {
        const auto rows=tools::parse_tsv(read_bytes(root/package/"seb.inf"));
        const int count=std::string(package)=="human"?4:16;
        for(int i=0;i<count;++i) {
            check(rows.size()>static_cast<std::size_t>(i) && rows[i].size()==1,"行走SEB包序存在");
            const auto bytes=read_bytes(root/package/rows[i][0]);
            source_sequence.insert(source_sequence.end(),bytes.begin(),bytes.end());
            (std::string(package)=="human"?body:arms).push_back(tools::parse_legacy_seb(bytes));
        }
    }
    const auto shadow_bytes=read_bytes(root/"common/shadow00.seb");
    source_sequence.insert(source_sequence.end(),shadow_bytes.begin(),shadow_bytes.end());
    check(tools::sha256_hex(source_sequence)=="0bf84785d4389a36d66eb0a48d1a73db4224907260fd9bf05c98f91144f90723",
          "四个human、十六个weapon和shadow原SEB顺序内容冻结");
    const auto shadow=tools::parse_legacy_seb(shadow_bytes);
    check(shadow.layers.size()==1 && !shadow.layers[0].parts.empty() && shadow.layers[0].parts[0].image_index==3 &&
          shadow.layers[0].parts[0].width==12 && shadow.layers[0].parts[0].height==2 &&
          shadow.layers[0].parts[0].offset_x==-6 && shadow.layers[0].parts[0].offset_y==-1,
          "common25 shadow帧0的12x2裁片及负偏移");
    constexpr std::array<int,4> frame_x{{0,18,0,36}};
    for(int face=0;face<4;++face) {
        check(body[face].frame_count==4 && body[face].layers.size()==1 && body[face].layers[0].parts.size()==4,
              "human walk四方向均为单层四步");
        for(int step=0;step<4;++step) {
            const auto &p=body[face].layers[0].parts[step];
            check(p.frame==step && p.image_index==0 && p.source_x==frame_x[step] && p.source_y==24*face &&
                  p.width==18 && p.height==24 && p.offset_x==-9 && p.offset_y==-24 && !p.flip_x && !p.flip_y,
                  "body步2重复站立列而非第三列；四方向不猜镜像");
        }
    }
    std::set<int> body_images;
    const auto &rules=startup_world_rules();
    for(std::size_t job=0;job<rules.jobs.size();++job)
        for(int sex=0;sex<2;++sex) {
            const auto p=startup_title_actor_skin(static_cast<int>(job),sex,-1,3,2,false);
            check(p && !p->shadow && !p->weapon && p->body.image==rules.jobs[job].sprites[sex] &&
                  p->body.sprite==2 && p->body.frame==3,"职业/性别原表对应身体，无武器-1不造fallback");
            body_images.insert(p->body.image);
        }
    for(int id:body_images) {
        check(humans.count(id)==1,"全部职业/性别引用实际human资源");
        CpuImage image(LoadImage(humans.at(id).string().c_str()));
        for(const auto &s:body)
            for(const auto &p:s.layers[0].parts) {
                check(p.source_x+p.width<=image.image.width && p.source_y+p.height<=image.image.height,
                      "全部37身体图的真实四向四帧裁片不越界");
            }
    }
    constexpr std::array<int,4> weapon_width{{21,21,26,32}},weapon_height{{25,25,28,35}};
    constexpr std::array<std::array<std::array<int,2>,4>,4> offset_oracle{{
        {{{-3,-28},{-3,-28},{-18,-28},{-18,-28}}},
        {{{-7,-22},{-7,-22},{-15,-22},{-15,-22}}},
        {{{-5,-28},{-5,-28},{-20,-28},{-20,-28}}},
        {{{-9,-38},{-9,-37},{-22,-37},{-23,-38}}}
    }};
    int weapon_count{};
    for(const auto &definition:rules.equipment) {
        if(definition.shop.kind!=1)continue;
        ++weapon_count;
        const int style=definition.render_style;
        check(style>=0 && style<4 && weapons.count(definition.render_image)==1,"33武器的稀疏图片及动作风格均有正式资源");
        CpuImage image(LoadImage(weapons.at(definition.render_image).string().c_str()));
        for(int face=0;face<4;++face) {
            const auto &sprite=arms[style*4+face];
            check(sprite.layers.size()==1 && !sprite.layers[0].parts.empty(),"weapon方向SEB单层");
            const auto &p=sprite.layers[0].parts.front();
            check(p.frame==0 && p.source_x==0 && p.source_y==weapon_height[style]*face &&
                  p.width==weapon_width[style] && p.height==weapon_height[style] &&
                  p.offset_x==0 && p.offset_y==0 && !p.flip_x && !p.flip_y &&
                  p.source_x+p.width<=image.image.width && p.source_y+p.height<=image.image.height,
                  "行走武器固定帧0，指定武器图片覆盖SEB默认图片且裁片合法");
            for(int step=0;step<4;++step) {
                const auto plan=startup_title_actor_skin(0,0,definition.shop.id,step,face,true);
                const auto xy=offset_oracle[style][face];
                check(plan && plan->shadow && plan->weapon && plan->shadow->sprite==25 &&
                      plan->shadow->image==3 && plan->shadow->offset==std::array<int,2>{0,0} &&
                      plan->weapon->resource==StartupVisualResource::weapon && plan->weapon->sprite==style*4+face &&
                      plan->weapon->image==definition.render_image && plan->weapon->frame==0 &&
                      plan->weapon->offset==std::array<int,2>{xy[0],xy[1]+step%2} &&
                      plan->body.image==14 && plan->body.frame==step && plan->body.sprite==face,
                      "33武器四向四步依原偏移走动，身体和武器帧参数分开");
            }
        }
    }
    check(weapon_count==33,"固定原表33武器定义均已消费");
    const auto female=startup_title_actor_skin(0,1,0,0,1,false);
    check(female && female->body.image==15,"女性独立原图，不以男性镜像代替");
    for(const auto &input:std::array<std::array<int,5>,8>{{{-1,0,0,0,0},{23,0,0,0,0},{0,-1,0,0,0},
            {0,2,0,0,0},{0,0,-2,0,0},{0,0,9999,0,0},{0,0,0,4,0},{0,0,0,0,4}}})
        check(!startup_title_actor_skin(input[0],input[1],input[2],input[3],input[4],true),
              "坏职业/性别/武器/步帧/朝向显式拒绝，不靠map.at抛出");
    // 最小CPU输出消费：同一锚点严格shadow→weapon→body，原身体不透明像素最终盖住武器。
    constexpr std::array<int,4> sample_weapons{{0,13,26,25}};
    CpuImage body_image(LoadImage(humans.at(14).string().c_str()));
    CpuImage shadow_image(LoadImage((root/"common/shadow00.png").string().c_str()));
    for(int style=0;style<4;++style)
        for(int face=1;face<=2;++face)
            for(int step=0;step<4;++step) {
                const auto p=startup_title_actor_skin(0,0,sample_weapons[style],step,face,true);
                check(p && p->weapon,"四种真实武器样本基础计划");
                CpuImage panel(GenImageColor(64,56,BLANK));
                const auto &sp=shadow.layers[0].parts[0];
                draw(panel.image,shadow_image.image,{sp.source_x,sp.source_y,sp.width,sp.height},32+sp.offset_x,50+sp.offset_y);
                CpuImage weapon_image(LoadImage(weapons.at(p->weapon->image).string().c_str()));
                const auto &wp=arms[p->weapon->sprite].layers[0].parts[0];
                draw(panel.image,weapon_image.image,{wp.source_x,wp.source_y,wp.width,wp.height},
                     32+p->weapon->offset[0]+wp.offset_x,50+p->weapon->offset[1]+wp.offset_y);
                const auto &bp=body[p->body.sprite].layers[0].parts[p->body.frame];
                draw(panel.image,body_image.image,{bp.source_x,bp.source_y,bp.width,bp.height},32+bp.offset_x,50+bp.offset_y);
                int opaque{};
                for(int y=0;y<bp.height;++y)for(int x=0;x<bp.width;++x) {
                    const auto original=GetImageColor(body_image.image,bp.source_x+x,bp.source_y+y);
                    if(original.a==255) {
                        ++opaque;
                        check(same(GetImageColor(panel.image,32+bp.offset_x+x,50+bp.offset_y+y),original),
                              "身体最后绘制，SEB负偏移只加一次，不遮错武器层序");
                    }
                }
                check(opaque>20,"原身体四步存在实际不透明像素");
                if(contact_sheet)draw(*contact_sheet,panel.image,{0,0,64,56},10+90*((face-1)*4+step),940+70*style);
            }
}
void item_information(Checks &check) {
    auto s=test_support::world_fixture();
    // 库存/status/NEW为只读目录条件夹具，不声称发生自然奖励或玩家使用。
    for(auto &item:s.items) {
        item.second.inventory=0;
        s.catalog.at({0,item.first}).inventory=0;
    }
    s.items.at(0).inventory=2;s.catalog.at({0,0}).inventory=2;
    s.items.at(35).inventory=1;s.catalog.at({0,35}).inventory=1;
    s.items.at(0).status=0;s.catalog.at({0,0}).status=0;
    s.items.at(35).status=2;s.catalog.at({0,35}).status=2;
    s.items.at(0).newly_unlocked=true;s.catalog.at({0,0}).newly_unlocked=true;
    const auto before=startup_world_state_digest(s);
    auto rows=startup_item_information(s);
    check(rows && rows->size()==2 && rows->at(0).definition==0 && rows->at(1).definition==35 &&
          rows->at(0).inventory==2 && rows->at(1).inventory==1,
          "37正库存按原定义顺序，不过滤status0或2");
    check(rows->at(0).description=="培育的很好的马铃薯" && rows->at(1).description=="恢复魔法可以学会",
          "item0/35原第23列说明完整交付，不由效果名称合成");
    check(rows->at(0).newly_unlocked && startup_world_state_digest(s)==before,
          "持有查询不清NEW、不使用库存或修改Owner");
    StartupWorldRules reversed=*s.rules;
    std::swap(reversed.items.front(),reversed.items.back());
    auto reverse=s;reverse.rules=&reversed;
    rows=startup_item_information(reverse);
    check(rows && rows->size()==2 && rows->at(0).definition==35 && rows->at(1).definition==0,
          "私有原序夹具证明不按ID或status另排序");
    for(int fault=0;fault<7;++fault) {
        auto bad=s;
        if(fault==0)bad.items.erase(0);
        if(fault==1)bad.catalog.erase({0,0});
        if(fault==2)bad.catalog.at({0,0}).inventory=1;
        if(fault==3)bad.catalog.at({0,0}).status=1;
        if(fault==4)bad.catalog.at({0,0}).newly_unlocked=false;
        if(fault==5)++bad.catalog.at({0,0}).unlock_counter;
        if(fault==6)bad.rules=nullptr;
        const auto digest=startup_world_state_digest(bad);
        check(!startup_item_information(bad) && startup_world_state_digest(bad)==digest,
              "37缺来源或两份镜像不一致显式拒绝，查询不修补Owner");
    }
}
void equipment_information(Checks &check) {
    using Edition=StartupInformationEdition;
    auto s=test_support::world_fixture();
    constexpr std::array<std::size_t,4> apk_counts{33,17,33,30},steam_counts{33,16,33,27};
    const auto contains=[](const StartupEquipmentInformation &page,int id) {
        return std::any_of(page.rows.begin(),page.rows.end(),[=](const auto &r){return r.definition==id;});
    };
    const auto before=startup_world_state_digest(s);
    for(int slot=0;slot<4;++slot) {
        const auto apk=startup_equipment_information(s,slot,Edition::apk_1_0_8);
        const auto steam=startup_equipment_information(s,slot,Edition::steam_2_56);
        check(apk && steam && apk->rows.size()==apk_counts[slot] && steam->rows.size()==steam_counts[slot],
              "38明确版本目录APK33/17/33/30与Steam33/16/33/27");
        check(apk->edition==Edition::apk_1_0_8 && apk->nonpositive_text.empty() &&
              steam->edition==Edition::steam_2_56 && steam->nonpositive_text=="--",
              "已知装备非正属性APK留空、Steam画字面--，版本信息不丢失");
        check(apk->attributes==(slot==0?std::array<int,2>{1,3}:std::array<int,2>{0,2}),
              "武器显示攻击/魔法，其余显示HP/防御");
        if(slot==1)check(contains(*apk,45) && !contains(*steam,45),"Steam铠甲flag0定义45不入目录");
        if(slot==3)for(int id:{26,27,28})
            check(contains(*apk,id) && !contains(*steam,id),"Steam饰品flag0定义26/27/28不入目录");
    }
    check(startup_world_state_digest(s)==before,"两版本只读目录不修改flags、NEW、库存或Owner");
    auto current=s;
    current.catalog.at({2,45}).flags=1;current.catalog.at({3,26}).flags=1;
    check(startup_equipment_information(current,1,Edition::steam_2_56)->rows.size()==17 &&
          startup_equipment_information(current,3,Edition::steam_2_56)->rows.size()==28,
          "Steam过滤读取当前catalog.flags，不缓存定义初值");
    for(auto &entry:current.catalog)if(entry.first.first!=0) {
        entry.second.status=0;entry.second.free_purchases=999;entry.second.newly_unlocked=true;
    }
    current.catalog.at({1,0}).status=1;current.catalog.at({1,10}).status=1;
    current.catalog.at({1,1}).status=2;
    StartupWorldRules rules=*current.rules;current.rules=&rules;
    for(auto &definition:rules.equipment)if(definition.shop.kind==1) {
        if(definition.shop.id==0)definition.shop.combat={0,5,0,-4};
        if(definition.shop.id==10)definition.shop.combat={0,0,0,6};
    }
    const auto digest=startup_world_state_digest(current);
    const auto page=startup_equipment_information(current,0,Edition::apk_1_0_8);
    const auto steam_page=startup_equipment_information(current,0,Edition::steam_2_56);
    check(page && page->rows.size()==33 && page->known_count==2,
          "known_count只数p1定义种类，不数免费份数或所有非零p");
    for(const auto &row:page->rows) {
        if(row.definition==0)
            check(row.visible && row.visible->render_icon==0 && row.visible->newly_unlocked &&
                  row.visible->values[0]==5 && !row.visible->values[1],"已知武器0只显示正攻击，不显示负魔法");
        else if(row.definition==10)
            check(row.visible && row.visible->render_icon==26 && !row.visible->values[0] &&
                  row.visible->values[1]==6,"已知武器10图标26，零攻击不画，正魔法可画");
        else check(!row.visible,"p0和p2均保留定义行身份但不暴露名称/图标/属性");
    }
    check(steam_page && steam_page->nonpositive_text=="--" &&
          std::any_of(steam_page->rows.begin(),steam_page->rows.end(),[](const auto &row){
              return row.definition==10 && row.visible && !row.visible->values[0] && row.visible->values[1]==6;
          }),"Steam已知零攻击保持visible并交--占位，不误判为整件未知");
    check(startup_world_state_digest(current)==digest,"已知/未知查询不消费NEW或免费份数");
    // 四个独立原序/等键定义夹具。严格交换结果是2,1,0,3，稳定排序会错成2,0,1,3。
    auto ordered=s;StartupWorldRules tiny=*s.rules;tiny.equipment.clear();
    for(int id=0;id<4;++id) {
        StartupWorldEquipment definition;
        definition.shop.kind=1;definition.shop.id=id;definition.gift_order=std::array<int,4>{2,2,1,3}[id];
        tiny.equipment.push_back(definition);
    }
    ordered.rules=&tiny;
    const auto sorted=startup_equipment_information(ordered,0,Edition::apk_1_0_8);
    check(sorted && sorted->rows.size()==4 && sorted->rows[0].definition==2 &&
          sorted->rows[1].definition==1 && sorted->rows[2].definition==0 && sorted->rows[3].definition==3,
          "38严格逆向内循环交换保留原等键结果，不能改stable_sort");
    for(const int slot:{-1,4})check(!startup_equipment_information(s,slot,Edition::apk_1_0_8),"38拒绝无效第五页签");
    auto missing=s;missing.catalog.erase({1,0});
    const auto missing_digest=startup_world_state_digest(missing);
    check(!startup_equipment_information(missing,0,Edition::apk_1_0_8) &&
          startup_world_state_digest(missing)==missing_digest &&
          !startup_equipment_information(s,0,static_cast<Edition>(99)),"缺当前catalog或未知版本拒绝而不改Owner");
}
void equipment_information_icons(Checks &check,const std::filesystem::path &root) {
    auto s=test_support::world_fixture();
    CpuImage background(LoadImage((root/"common/icon_back00.png").string().c_str()));
    check(background.image.width>=72 && background.image.height>=18,"正式common24包含底框源54/0/18/18");
    int largest_weapon_icon=-1;
    for(const auto &definition:s.rules->equipment)if(definition.shop.kind==1)
        largest_weapon_icon=std::max(largest_weapon_icon,definition.shop.type);
    check(largest_weapon_icon==35,"33条真实武器定义的列表图标最大为35，不与定义数量混用");
    // 独立原表字段oracle：kind/id/listIcon/bodyPNG，后者仅武器用于防止两字段混淆。
    constexpr std::array<std::array<int,4>,6> cases{{
        {{1,0,0,50}},{{1,10,26,3}},{{1,32,22,46}},{{2,6,20,-1}},{{2,49,18,-1}},{{3,29,29,-1}}
    }};
    for(const auto sample:cases) {
        const auto plan=startup_world_equipment_icon_draws(s,sample[0],sample[1]);
        check(plan && plan->size()==2 && plan->at(0).resource==StartupVisualResource::common &&
              plan->at(0).image==24 && plan->at(0).sprite==-1 &&
              plan->at(0).crop==std::array<int,4>{54,0,18,18} && plan->at(0).offset==std::array<int,2>{0,0},
              "装备列表先画common24第3格底框，不套人物举物底框偏移");
        check(plan->at(1).image==(sample[0]==1?12:sample[0]==2?20:21) &&
              plan->at(1).crop==std::array<int,4>{(sample[2]%10)*18,(sample[2]/10)*18,18,18} &&
              plan->at(1).offset==std::array<int,2>{0,0},"三类装备图标按实际列表字段定位18格");
        if(sample[0]==1) {
            const auto definition=std::find_if(s.rules->equipment.begin(),s.rules->equipment.end(),[&](const auto &d){
                return d.shop.kind==1&&d.shop.id==sample[1];});
            check(definition!=s.rules->equipment.end() && definition->render_image==sample[3],
                  "武器bodyPNG字段与列表图标独立，不能拿50/3/46去裁列表");
        }
    }
    const std::array<const char *,3> files{"icon_weapon00.png","icon_armour00.png","icon_accessry00.png"};
    constexpr std::array<int,3> counts{40,50,30};
    for(int kind=1;kind<=3;++kind) {
        CpuImage source(LoadImage((root/"common"/files[kind-1]).string().c_str()));
        check(source.image.width==180 && source.image.height==(counts[kind-1]/10)*18,
              "实际图集容量是40/50/30，不是装备定义数");
        auto private_owner=s;StartupWorldRules rules=*s.rules;private_owner.rules=&rules;
        auto definition=std::find_if(rules.equipment.begin(),rules.equipment.end(),[=](const auto &d){return d.shop.kind==kind;});
        check(definition!=rules.equipment.end(),"三类真实原表都有图标入口");
        int &icon=kind==1?definition->shop.type:definition->render_image;
        for(const int value:{0,counts[kind-1]-1}) {
            icon=value;const auto plan=startup_world_equipment_icon_draws(private_owner,kind,definition->shop.id);
            check(plan && plan->at(1).crop[0]+18<=source.image.width &&
                  plan->at(1).crop[1]+18<=source.image.height,"私有边界图标最后一格仍可合法裁剪");
        }
        if(kind==1) {
            icon=35;check(startup_world_equipment_icon_draws(private_owner,kind,definition->shop.id).has_value(),
                          "实际武器最大icon35不被33定义总数错误截断");
        }
        for(const int value:{-1,counts[kind-1],std::numeric_limits<int>::max()}) {
            icon=value;check(!startup_world_equipment_icon_draws(private_owner,kind,definition->shop.id),
                             "越界图标不靠取模藏成合法裁片");
        }
    }
    const auto digest=startup_world_state_digest(s);
    check(!startup_world_equipment_icon_draws(s,0,0) && !startup_world_equipment_icon_draws(s,4,0) &&
          !startup_world_equipment_icon_draws(s,1,-1) && !startup_world_equipment_icon_draws(s,1,999) &&
          startup_world_state_digest(s)==digest,"坏kind或缺定义拒绝，图标查询不写Owner");
}
void information_directory_skin(Checks &check,const std::filesystem::path &root) {
    using Role=SteamInformationTextRole;
    using Mode=SteamInformationTextMode;
    using Number=SteamInformationNumberKind;
    const auto label=[&](const SteamInformationSkinPlan &plan,Role role,int row=-1) -> const SteamInformationText & {
        const auto found=std::find_if(plan.draws.begin(),plan.draws.end(),[&](const auto &draw) {
            const auto *text=std::get_if<SteamInformationText>(&draw);
            return text && text->role==role && (row<0 || text->slot==row);
        });
        check(found!=plan.draws.end(),"目录计划存在所需文字角色/绝对行");
        return std::get<SteamInformationText>(*found);
    };
    const auto image_index=[&](const SteamInformationSkinPlan &plan,int image) {
        const auto found=std::find_if(plan.draws.begin(),plan.draws.end(),[=](const auto &draw) {
            const auto *part=std::get_if<StartupSkinDraw>(&draw);
            return part && part->image==image;
        });
        check(found!=plan.draws.end(),"目录计划含真实资源请求");
        return static_cast<std::size_t>(found-plan.draws.begin());
    };
    const auto scrolling=[&](const SteamInformationSkinPlan &plan,int count,int visible,
                             std::array<int,4> thumb,std::array<int,3> color) {
        check(plan.draws.size()>=3 && plan.touches.size()>=2,"滚动输出不能空遍历通过");
        const auto &track=std::get<StartupSkinRect>(plan.draws[plan.draws.size()-3]);
        const auto &slider=std::get<StartupSkinRect>(plan.draws[plan.draws.size()-2]);
        check(track.rect==std::array<int,4>{220,85,5,110} && track.rgb==std::array<int,3>{7,5,78} &&
              slider.rect==thumb && slider.rgb==color && !track.outline && !slider.outline,
              "组件12下游真实轨道及滑块+1高度，不按helper名漏画或猜尺寸");
        const auto &bar=plan.touches[plan.touches.size()-2],&back=plan.touches.back();
        check(bar.component==12 && bar.value==0x40000 && bar.rectangle==std::array<int,4>{221,85,3,110} &&
              bar.margin==std::array<int,4>{3,20,0,0} && bar.option==0 &&
              bar.scroll_arguments==std::array<int,3>{count,visible,0x20000} &&
              back.component==25 && back.value==0 && back.rectangle==bar.rectangle && back.option==4 &&
              !back.scroll_arguments,"滚动条与背景独立注册，原margin/args/优先级不丢失");
    };
    // 仅造可见库存/NEW条件；保留全部原定义，不把布局夹具称为自然获得。
    auto items=test_support::page_fixture(37);
    const auto item_page=items.scripts.pages.back().id;
    items.scripts.pages.back().lifecycle=0;
    for(auto &entry:items.items) {
        entry.second.inventory=entry.first<7?(entry.first==0?123:1):0;
        entry.second.newly_unlocked=entry.first==0;
        auto &mirror=items.catalog.at({0,entry.first});
        mirror.inventory=entry.second.inventory;mirror.newly_unlocked=entry.second.newly_unlocked;
    }
    check(initialize_startup_world_information_pages(items),"37皮肤使用真实Init冻结七项原序目录");
    SteamInformationSkinOptions options{0,false,std::array<int,2>{80,82},false};
    const auto before=startup_world_state_digest(items);
    const auto item_result=steam_item_information_skin(items,item_page,options);
    check(item_result && startup_world_state_digest(items)==before,"37只读皮肤不清NEW/改库存/推进输出");
    const auto &item_plan=*item_result;
    check(item_plan.raw==37 && item_plan.soft_labels==std::array<int,2>{0,2} && item_plan.touches.size()==7 &&
          label(item_plan,Role::name_header).position==std::array<int,2>{30,69} &&
          label(item_plan,Role::inventory_header).position==std::array<int,2>{175,69},
          "37五行及两滚动组件，不产生标题箭头或额外确认按钮");
    const auto start=image_index(item_plan,147);
    const auto &notice=std::get<StartupSkinDraw>(item_plan.draws[start]);
    const auto &hand=std::get<StartupSkinDraw>(item_plan.draws[start+1]);
    const auto &background=std::get<StartupSkinDraw>(item_plan.draws[start+2]);
    const auto &icon=std::get<StartupSkinDraw>(item_plan.draws[start+3]);
    const auto &name=std::get<SteamInformationText>(item_plan.draws[start+4]);
    const auto &quantity=std::get<SteamInformationNumber>(item_plan.draws[start+5]);
    check(notice.offset==std::array<int,2>{10,99} && notice.crop==std::array<int,4>{0,0,20,9} &&
          hand.image==70 && hand.sprite==21 && hand.frame==-1 && hand.offset==std::array<int,2>{21,105} &&
          background.image==24 && background.crop==std::array<int,4>{18,0,18,18} &&
          background.offset==std::array<int,2>{29,94} && icon.image==9 &&
          icon.crop==std::array<int,4>{80,0,16,16} && icon.offset==std::array<int,2>{30,95},
          "37独立源顺序NEW→当前帧手形→分类背景→原icon5，不能拿定义ID0裁片");
    check(name.role==Role::row_name && name.value=="北国马铃薯" && name.slot==0 &&
          name.position==std::array<int,2>{50,95} && name.extent==std::array<int,2>{130,15} &&
          name.mode==Mode::layout && name.anchor==0x20 && name.line_space==0 && name.font_size==0 &&
          quantity.kind==Number::inventory_count && quantity.value==123 && quantity.position==std::array<int,2>{210,98},
          "37名称保持TextLayout实际矩形后才画数量，不借APK普通文字或字号");
    check(label(item_plan,Role::row_name,4).position==std::array<int,2>{50,171} &&
          label(item_plan,Role::item_description).value=="培育的很好的马铃薯" &&
          label(item_plan,Role::item_description).position==std::array<int,2>{120,200} &&
          label(item_plan,Role::item_description).anchor==2,"37原19行距与选中说明，非效果摘要");
    check(item_plan.touches[0].component==11 && item_plan.touches[0].value==0x20000 &&
          item_plan.touches[0].rectangle==std::array<int,4>{3,95,231,16} &&
          item_plan.touches[0].margin==std::array<int,4>{0,-20,0,0} && item_plan.touches[0].option==0 &&
          std::none_of(item_plan.draws.begin(),item_plan.draws.end(),[](const auto &draw) {
              const auto *rect=std::get_if<StartupSkinRect>(&draw);
              return rect && rect->rgb==std::array<int,3>{255,153,55};
          }),"37原flag0行注册，无凭SetColor猜造的橙色选中背景");
    scrolling(item_plan,7,5,{220,85,5,79},{48,160,255});
    options.scroll_first_touch=true;
    auto shifted_items=items;
    shifted_items.information_page_data.at(item_page).selection=6;
    shifted_items.information_page_data.at(item_page).first_visible=2;
    const auto scrolled=*steam_item_information_skin(shifted_items,item_page,options);
    scrolling(scrolled,7,5,{220,116,5,79},{246,129,0});
    check(scrolled.touches.front().value==0x20002 && label(scrolled,Role::item_description).value=="让人心荡神驰的起司",
          "滚动后行输入保留绝对索引，说明读取实际选中定义");
    options.view_y=20;options.japanese=true;
    const auto moved=*steam_item_information_skin(items,item_page,options);
    check(label(moved,Role::inventory_header).position==std::array<int,2>{185,69} &&
          label(moved,Role::title).position[1]==label(item_plan,Role::title).position[1]+10 &&
          label(moved,Role::row_name,0).position==name.position &&
          std::get<StartupSkinDraw>(moved.draws[image_index(moved,70)]).offset==hand.offset &&
          moved.touches[0].rectangle==item_plan.touches[0].rectangle,
          "日文表头分支与VIEW_Y仅框/box偏移，行/手形/触摸不重复移动");
    check(std::any_of(moved.draws.begin(),moved.draws.end(),[](const auto &draw) {
              const auto *rect=std::get_if<StartupSkinRect>(&draw);
              return rect && !rect->outline && rect->rect==std::array<int,4>{17,84,202,110} &&
                     rect->rgb==std::array<int,3>{247,253,247};
          }),"内框也接VIEW_Y半偏移，后两项原边界不能当作宽高");

    // p1/NEW只是展示条件；不改原属性表，强击剑70/52和短剑5/0提供正值/占位oracle。
    auto equipment=test_support::page_fixture(38);
    const auto equipment_page=equipment.scripts.pages.back().id;
    equipment.scripts.pages.back().lifecycle=0;
    for(auto &entry:equipment.catalog)if(entry.first.first!=0) {
        entry.second.status=0;entry.second.newly_unlocked=false;
    }
    equipment.catalog.at({1,25}).status=1;equipment.catalog.at({1,25}).newly_unlocked=true;
    equipment.catalog.at({1,0}).status=1;
    check(initialize_startup_world_information_pages(equipment),"38皮肤使用真实Steam目录Init");
    options={0,false,std::array<int,2>{80,82},false};
    const auto equipment_before=startup_world_state_digest(equipment);
    const auto equipment_result=steam_equipment_information_skin(equipment,equipment_page,options);
    check(equipment_result && startup_world_state_digest(equipment)==equipment_before,"38绘制保留Owner/装备NEW/随机");
    const auto &equipment_plan=*equipment_result;
    const auto known=image_index(equipment_plan,12);
    const auto &weapon=std::get<StartupSkinDraw>(equipment_plan.draws[known]);
    const auto &weapon_name=std::get<SteamInformationText>(equipment_plan.draws[known+1]);
    const auto &attack=std::get<SteamInformationNumber>(equipment_plan.draws[known+2]);
    const auto &magic=std::get<SteamInformationNumber>(equipment_plan.draws[known+3]);
    const auto &get=std::get<StartupSkinDraw>(equipment_plan.draws[known+4]);
    const auto &weapon_hand=std::get<StartupSkinDraw>(equipment_plan.draws[known+5]);
    check(weapon.crop==std::array<int,4>{108,0,18,18} && weapon.offset==std::array<int,2>{30,94} &&
          weapon_name.value=="强击剑" && weapon_name.mode==Mode::layout && weapon_name.extent==std::array<int,2>{85,15} &&
          weapon_name.position==std::array<int,2>{50,95} && weapon_name.font_size==11 && weapon_name.anchor==0x20 &&
          attack.value==70 && attack.position==std::array<int,2>{164,98} &&
          magic.value==52 && magic.position==std::array<int,2>{207,98} &&
          attack.kind==Number::positive_attribute && magic.kind==Number::positive_attribute &&
          get.image==148 && get.offset==std::array<int,2>{10,99} &&
          weapon_hand.sprite==21 && weapon_hand.frame==-1 && weapon_hand.offset==std::array<int,2>{21,105},
          "38图标→非日文名字→两属性→GET→手形，不能套37 NEW在前的顺序");
    check(label(equipment_plan,Role::unknown_row,1).position==std::array<int,2>{30,121} &&
          label(equipment_plan,Role::unknown_row,1).rgb==std::array<int,3>{156,155,155} &&
          label(equipment_plan,Role::known_count).mode==Mode::rich_text &&
          label(equipment_plan,Role::known_count).argument==2 &&
          label(equipment_plan,Role::known_count).extent==std::array<int,2>{-1,-1} &&
          !label(equipment_plan,Role::known_count).line_space && label(equipment_plan,Role::known_count).anchor==2,
          "未知行保留灰色角色，底部是整个目录已知种类数的原富文本点重载");
    scrolling(equipment_plan,33,4,{220,85,5,14},{48,160,255});
    options.japanese=true;
    const auto jp=*steam_equipment_information_skin(equipment,equipment_page,options);
    check(label(jp,Role::row_name,0).mode==Mode::plain && label(jp,Role::row_name,0).font_size==0 &&
          !label(jp,Role::row_name,0).anchor && label(jp,Role::row_name,0).position==std::array<int,2>{50,97},
          "38日文直接使用默认DrawString，不继承非日文11字号或TextLayout");
    auto unknown_selected=equipment;unknown_selected.information_page_data.at(equipment_page).selection=1;
    const auto unknown_plan=*steam_equipment_information_skin(unknown_selected,equipment_page,options);
    const auto unknown_hand=image_index(unknown_plan,70);
    check(unknown_hand>0 && std::get<SteamInformationText>(unknown_plan.draws[unknown_hand-1]).role==Role::unknown_row &&
          std::get<StartupSkinDraw>(unknown_plan.draws[unknown_hand]).offset==std::array<int,2>{21,129},
          "未知装备行仍在灰色占位之后画选中手形，不因p0跳过整行");
    auto last=equipment;
    last.information_page_data.at(equipment_page).selection=32;
    last.information_page_data.at(equipment_page).first_visible=29;
    const auto last_plan=*steam_equipment_information_skin(last,equipment_page,options);
    check(label(last_plan,Role::attribute_placeholder,32).value=="--" &&
          label(last_plan,Role::attribute_placeholder,32).position==std::array<int,2>{189,168},
          "短剑原魔法0由Steam明确画--，不能沿APK空白或伪造正值");
    for(int tab=0;tab<4;++tab) {
        auto page=equipment;page.page_phases.at(equipment_page)=tab;
        const auto plan=*steam_equipment_information_skin(page,equipment_page,options);
        const auto &head=std::get<StartupSkinDraw>(plan.draws[image_index(plan,128)]);
        const auto attr=image_index(plan,37);
        check(head.sprite==88 && head.frame==tab+1 && head.offset==std::array<int,2>{25,66} &&
              std::get<StartupSkinDraw>(plan.draws[attr]).frame==(tab==0?1:0) &&
              std::get<StartupSkinDraw>(plan.draws[attr+1]).frame==(tab==0?3:2),
              "38四页头实际SEB帧与属性列，不按定义ID或PNG列数猜帧");
    }
    auto empty=test_support::page_fixture(38);
    const auto empty_page=empty.scripts.pages.back().id;
    empty.scripts.pages.back().lifecycle=0;
    for(auto &entry:empty.catalog)if(entry.first.first==3)entry.second.flags=0;
    check(initialize_startup_world_information_pages(empty),"空类只改变条件flags，通过真实Init");
    empty.page_phases.at(empty_page)=3;options.scroll_first_touch=true;
    const auto empty_plan=*steam_equipment_information_skin(empty,empty_page,options);
    check(label(empty_plan,Role::empty_directory).position==std::array<int,2>{120,97} &&
          label(empty_plan,Role::known_count).argument==0 && empty_plan.touches.size()==4,
          "合法空38仍有空文本、页头/箭头和滚动注册，无伪造行");
    scrolling(empty_plan,0,4,{220,85,5,111},{48,160,255});
    check(!steam_item_information_skin(equipment,equipment_page,options) &&
          !steam_equipment_information_skin(items,item_page,options),"37/38不能读取对方页面身份");
    for(auto widths:{std::optional<std::array<int,2>>{},std::optional<std::array<int,2>>{{-1,80}}}) {
        options.title_widths=widths;
        check(!steam_item_information_skin(items,item_page,options) &&
              !steam_equipment_information_skin(equipment,equipment_page,options),"目录缺失/负标题实测宽拒绝整份计划");
    }
    const auto counted=steam_information_number_draws({Number::inventory_count,123,{210,98}});
    const auto plus=steam_information_number_draws({Number::positive_attribute,1200,{164,98}});
    check(counted && counted->size()==4 && counted->at(0).frame==1 && counted->at(0).offset==std::array<int,2>{176,98} &&
          counted->at(2).frame==3 && counted->at(2).offset==std::array<int,2>{192,98} &&
          counted->back().image==85 && counted->back().sprite==76 && counted->back().frame==2 &&
          counted->back().offset==std::array<int,2>{200,98},"库存右锚先减10，三数字之后才画独立数量单位");
    check(plus && plus->size()==6 && plus->at(1).frame==2 && plus->at(2).frame==10 &&
          plus->at(2).offset==std::array<int,2>{138,98} && plus->back().frame==14 &&
          plus->back().offset==std::array<int,2>{124,98},"正属性复用Steam逗号后覆盖顺序和末尾加号");
    check(!steam_information_number_draws({Number::inventory_count,1000,{210,98}}) &&
          !steam_information_number_draws({Number::positive_attribute,0,{164,98}}) &&
          !steam_information_number_draws({Number::inventory_count,1,{std::numeric_limits<int>::min(),0}}),
          "目录数字边界拒绝，不夹库存或溢出坐标生成部分输出");
    const auto assets=root.parent_path();
    check(steam_information_image(9)=="original/common/tresureIcon00.png" &&
          steam_information_image(85)=="steam-common/menuRT01.png" &&
          steam_information_image(128)=="steam-common/icon_objRoots.png" && !steam_information_image(-1),
          "实际common9文件名及两份Steam差异图明确解析，不借同名APK资源");
    for(const auto record:std::array<std::array<int,3>,2>{{{85,57,10},{128,148,36}}}) {
        const auto path=steam_information_image(record[0]);
        check(path.has_value(),"新增Steam图有正式路径");
        CpuImage decoded(LoadImage((assets/std::string(*path)).string().c_str()));
        check(decoded.image.width==record[1] && decoded.image.height==record[2],"已发布Steam差异PNG实际解码尺寸");
    }
    const auto roots=tools::parse_legacy_seb(read_bytes(root/"common/icon_objRoots.seb"));
    for(int frame=1;frame<=4;++frame) {
        const auto &parts=roots.layers.at(0).parts;
        const auto part=std::find_if(parts.begin(),parts.end(),[=](const auto &p){return p.frame==frame;});
        check(part!=parts.end() && part->image_index==128 && part->source_x>=0 && part->source_y>=0 &&
              part->source_x+part->width<=148 && part->source_y+part->height<=36,
              "38四类页头的实际SEB帧都落在新版148×36图内，不能空遍历");
    }
    for(const auto &draw:*counted) {
        const auto seb=tools::parse_legacy_seb(read_bytes(root/"common"/(draw.sprite==76?"menuRT01.seb":"number05.seb")));
        const auto &parts=seb.layers.at(0).parts;
        const auto part=std::find_if(parts.begin(),parts.end(),[&](const auto &p){return p.frame==draw.frame;});
        check(part!=parts.end() && part->image_index==draw.image,"数量每个实际SEB请求有对应原帧");
        const auto path=steam_information_image(draw.image);
        check(path.has_value(),"数字及单位图片有显式版本资源");
        CpuImage decoded(LoadImage((assets/std::string(*path)).string().c_str()));
        check(part->source_x>=0 && part->source_y>=0 && part->source_x+part->width<=decoded.image.width &&
              part->source_y+part->height<=decoded.image.height,"数值/单位SEB裁片落在真正Steam图内");
        if(draw.image==85)check(part->source_x==20 && part->source_y==0 && part->width==10 && part->height==10 &&
                               part->offset_x==0 && part->offset_y==0,"数量单位帧2原裁片/offset独立oracle");
    }
}
void adventurer_information_skin(Checks &check,const std::filesystem::path &root) {
    using Role=SteamInformationTextRole;
    using Mode=SteamInformationTextMode;
    using Asset=SteamFacilityAsset;
    using Number=SteamFacilityNumberKind;
    // 最小六人目录/成长/NEW/HP展示条件，不声称这些人物已自然到访。
    auto owner=test_support::page_fixture(35);
    const auto id=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=0;
    for(auto &entry:owner.human_presence)entry.second=entry.first>=1&&entry.first<=6?1:0;
    owner.scripts.humans.at(1).pending_notice=true;
    auto &ai=owner.scene.world.world.ai;
    ai.growth.at(1).experience=0;
    auto &master=ai.growth.at(2);
    master.definition.profession_levels.at(master.definition.current_profession)=10;
    master.experience=0;
    owner.shop_humans.at(1).equipment[1].reset();
    owner.shop_humans.at(1).equipment[2].reset();
    owner.shop_humans.at(1).equipment[3].reset();
    ref::BattleActorRecord actor;actor.id={900};actor.kind=ref::ActorKind::human;actor.definition=1;
    actor.hp.displayed=7;actor.hp.target=19;actor.capacity=999;
    ai.battle.actors.emplace(actor.id,actor);ai.human_order.push_back(actor.id);
    SteamInformationSkinOptions options{0,false,std::array<int,2>{80,82},false};
    check(!steam_adventurer_information_skin(owner,id,options),"35未Init不由绘制补载荷");
    check(initialize_startup_world_information_pages(owner),"35真实Init冻结六人目录并计算贡献");
    const auto label=[&](const auto &plan,Role role) -> const SteamInformationText & {
        const auto it=std::find_if(plan.draws.begin(),plan.draws.end(),[=](const auto &draw) {
            const auto *text=std::get_if<SteamInformationText>(&draw);return text&&text->role==role;
        });
        check(it!=plan.draws.end(),"35必需文字角色存在");return std::get<SteamInformationText>(*it);
    };
    const auto image=[&](const auto &plan,int resource,int occurrence=0) -> const StartupSkinDraw & {
        for(const auto &draw:plan.draws)if(const auto *part=std::get_if<StartupSkinDraw>(&draw))
            if(part->image==resource && occurrence--==0)return *part;
        check(false,"35必需图片及出现次数存在");throw std::runtime_error("unreachable");
    };
    const auto number=[&](const auto &plan,Asset asset,std::array<int,2> position) -> const SteamFacilityNumber & {
        const auto it=std::find_if(plan.draws.begin(),plan.draws.end(),[=](const auto &draw) {
            const auto *value=std::get_if<SteamFacilityNumber>(&draw);
            return value&&value->asset==asset&&value->position==position;
        });
        check(it!=plan.draws.end(),"35必需数字SEB及原锚存在");return std::get<SteamFacilityNumber>(*it);
    };
    const auto baseline=startup_world_state_digest(owner);
    const auto result=steam_adventurer_information_skin(owner,id,options);
    check(result.has_value(),"35合法Owner产生完整计划");const auto &plan=*result;
    check(plan.raw==35 && plan.touches.size()==9 && plan.soft_labels==std::array<int,2>{0,2} &&
          label(plan,Role::adventurer_count).argument==6 &&
          label(plan,Role::adventurer_count).position==std::array<int,2>{120,200} &&
          label(plan,Role::adventurer_count).mode==Mode::rich_text,"35两箭头五行两滚动及全目录底栏");
    const auto selected=std::find_if(plan.draws.begin(),plan.draws.end(),[](const auto &draw) {
        const auto *rect=std::get_if<StartupSkinRect>(&draw);
        return rect&&rect->rgb==std::array<int,3>{255,153,55};
    });
    check(selected!=plan.draws.end(),"35实际画选中底，不能套37/38无底色规则");
    const auto first=static_cast<std::size_t>(selected-plan.draws.begin());
    const auto &fill=std::get<StartupSkinRect>(plan.draws.at(first));
    const auto &green=std::get<StartupSkinRect>(plan.draws.at(first+1));
    const auto &border=std::get<StartupSkinRect>(plan.draws.at(first+2));
    const auto &clip=std::get<SteamFacilityClip>(plan.draws.at(first+3));
    const auto &body=std::get<SteamInformationHumanBody>(plan.draws.at(first+4));
    const auto &pop=std::get<SteamFacilityClip>(plan.draws.at(first+5));
    const auto &notice=std::get<StartupSkinDraw>(plan.draws.at(first+6));
    const auto &hand=std::get<StartupSkinDraw>(plan.draws.at(first+7));
    check(fill.rect==std::array<int,4>{23,94,191,18} && !fill.outline &&
          green.rect==std::array<int,4>{29,95,16,16} && green.rgb==std::array<int,3>{196,236,169} &&
          border.rect==std::array<int,4>{29,95,16,16} && border.outline &&
          clip.kind==SteamFacilityClipKind::push_intersect && clip.rectangle==std::array<int,4>{29,95,15,15} &&
          body.body.sprite==1 && body.body.frame==0 && body.position==std::array<int,2>{37,118} &&
          body.body.image==owner.rules->jobs.at(ai.growth.at(1).definition.current_profession).sprites.at(owner.rules->humans.at(1).sex) &&
          pop.kind==SteamFacilityClipKind::pop && notice.image==147 && notice.offset==std::array<int,2>{10,99} &&
          hand.sprite==21 && hand.frame==-1 && hand.offset==std::array<int,2>{21,105},
          "35选中→底/边→15格裁剪→静态身体→pop→NEW→当前帧手形保留原序");
    check(std::get<SteamInformationText>(plan.draws.at(first+8)).position==std::array<int,2>{48,97} &&
          number(plan,Asset::number03,{147,97}).kind==Number::number &&
          number(plan,Asset::number03,{147,97}).anchor==4 &&
          image(plan,129).offset==std::array<int,2>{132,118} &&
          image(plan,87,0).crop==std::array<int,4>{0,0,44,5} &&
          image(plan,87,1).crop==std::array<int,4>{82,0,1,5} &&
          image(plan,87,2).crop==std::array<int,4>{2,5,0,3} &&
          image(plan,87,2).offset==std::array<int,2>{165,102} &&
          image(plan,87,5).crop==std::array<int,4>{2,5,42,3},"35零经验与10级大师强制满条保持三次裁片请求");
    check(plan.touches.at(2).component==11 && plan.touches.at(2).value==0x20000 &&
          plan.touches.at(2).rectangle==std::array<int,4>{3,94,231,18} &&
          plan.touches.at(2).margin==std::array<int,4>{0,-20,0,0},"35行热区18高及绝对索引");
    const auto &bar=plan.touches.at(7);
    check(bar.component==12 && bar.rectangle==std::array<int,4>{221,85,3,111} &&
          bar.scroll_arguments==std::array<int,3>{6,5,0x20000} &&
          std::get<StartupSkinRect>(plan.draws.at(plan.draws.size()-3)).rect==std::array<int,4>{220,85,5,111} &&
          std::get<StartupSkinRect>(plan.draws.at(plan.draws.size()-2)).rect==std::array<int,4>{220,85,5,93},
          "35真实111高轨道/滑块，不能硬套37/38的110");
    auto scrolled=owner;
    scrolled.information_page_data.at(id).selection=5;
    scrolled.information_page_data.at(id).first_visible=1;
    options.scroll_first_touch=true;
    const auto scrolling=*steam_adventurer_information_skin(scrolled,id,options);
    check(scrolling.touches.at(2).value==0x20001 && scrolling.touches.at(6).value==0x20005 &&
          std::get<StartupSkinRect>(scrolling.draws.at(scrolling.draws.size()-2)).rect==std::array<int,4>{220,103,5,93} &&
          std::get<StartupSkinRect>(scrolling.draws.at(scrolling.draws.size()-2)).rgb==std::array<int,3>{246,129,0},
          "35滚动保留绝对行、111高度的18像素位移和真实首次触摸颜色");
    options.scroll_first_touch=false;options.view_y=20;
    const auto shifted=*steam_adventurer_information_skin(owner,id,options);
    check(label(shifted,Role::title).position[1]==label(plan,Role::title).position[1]+10 &&
          label(shifted,Role::row_name).position==label(plan,Role::row_name).position &&
          shifted.touches.at(2).rectangle==plan.touches.at(2).rectangle,
          "35 VIEW_Y仅框/box，行内容与热区不重复位移");
    options.view_y=0;
    owner.page_phases.at(id)=1;
    const auto second=*steam_adventurer_information_skin(owner,id,options);
    check(label(second,Role::satisfaction_header).position==std::array<int,2>{47,69} &&
          label(second,Role::effort_header).position==std::array<int,2>{95,69} &&
          label(second,Role::equipment_header).extent==std::array<int,2>{60,12} &&
          label(second,Role::equipment_header).anchor==0x22 &&
          label(second,Role::equipment_header).line_space==0 &&
          number(second,Asset::number05,{80,98}).value==owner.shop_humans.at(1).satisfaction &&
          number(second,Asset::number05,{119,98}).value==ai.growth.at(1).definition.legacy_u,
          "35第二页三个TextLayout与满足/努力独立数字位置");
    for(int slot=1;slot<4;++slot) {
        const auto empty=std::find_if(second.draws.begin(),second.draws.end(),[=](const auto &draw) {
            const auto *part=std::get_if<StartupSkinDraw>(&draw);
            return part&&part->image==24&&part->crop==std::array<int,4>{126,0,18,18}&&
                   part->offset==std::array<int,2>{141+18*slot,94};
        });
        check(empty!=second.draws.end(),"35空防具/饰品严格mode5/index7，18像素等距");
    }
    owner.page_phases.at(id)=2;
    const auto third=*steam_adventurer_information_skin(owner,id,options);
    options.english=true;
    const auto english=*steam_adventurer_information_skin(owner,id,options);
    check(image(third,37).offset[0]==53 && image(english,37).offset[0]==48,
          "英语表头独立于非日文；中文不能误用英语48锚");
    for(int slot=0;slot<4;++slot)
        check(number(third,Asset::number08,{80+36*slot,99}).value==ai.growth.at(1).derived.combat[slot],
              "35第三页只读既有四战斗值缓存");
    check(number(third,Asset::number05,{211,98}).value==owner.human_calendar.at(1).celebrations,
          "35获勋次数不借贡献值");
    owner.page_phases.at(id)=3;options.english=false;
    const auto fourth=*steam_adventurer_information_skin(owner,id,options);
    check(label(fourth,Role::contribution_header).mode==Mode::layout &&
          label(fourth,Role::contribution_header).position==std::array<int,2>{48,70} &&
          label(fourth,Role::contribution_header).font_size==10 &&
          image(fourth,31).sprite==44 && image(fourth,31).frame==4 &&
          image(fourth,31).offset==std::array<int,2>{114,67} &&
          image(fourth,104).frame==13 && image(fourth,104).offset==std::array<int,2>{84,99} &&
          number(fourth,Asset::number11,{152,98}).value==ai.battle.humans.at(1).killed_stat1 &&
          number(fourth,Asset::number08,{212,98}).kind==Number::money,"35第四页SEB44/点数单位/年度村点/消费各走原helper");
    options.japanese=true;
    const auto japanese=*steam_adventurer_information_skin(owner,id,options);
    check(label(japanese,Role::contribution_header).position==std::array<int,2>{82,69} &&
          label(japanese,Role::contribution_header).font_size==0 &&
          image(japanese,31).offset==std::array<int,2>{116,67} &&
          image(japanese,104).offset==std::array<int,2>{84,97},"35日文第四页普通字号及点数单位图原y");
    owner.page_phases.at(id)=0;options.japanese=false;
    for(int count=0;count<8;++count) {
        const auto repeated=steam_adventurer_information_skin(owner,id,options);
        check(repeated && repeated->draws.size()==plan.draws.size() && repeated->touches.size()==plan.touches.size() &&
              startup_world_state_digest(owner)==baseline,"35重复查询不增长请求/Owner/随机/HP或消费NEW");
    }
    for(int fault=0;fault<6;++fault) {
        auto invalid=owner;
        if(fault==0)invalid.information_page_data.erase(id);
        if(fault==1)invalid.page_phases.at(id)=4;
        if(fault==2)invalid.information_page_data.at(id).selection=6;
        if(fault==3)invalid.scene.world.world.ai.growth.erase(1);
        if(fault==4)invalid.scripts.pages.back().lifecycle=4;
        if(fault==5)invalid.page_counters.erase(id);
        const auto before=startup_world_state_digest(invalid);
        check(!steam_adventurer_information_skin(invalid,id,options) && startup_world_state_digest(invalid)==before,
              "35缺载荷/源/阶段或越界索引拒绝整份计划且不修补Owner");
    }
    options.title_widths.reset();
    check(!steam_adventurer_information_skin(owner,id,options),"35缺真实标题测宽拒绝而非猜字宽");
    // 同一最小人物夹具的40条件页，只验证区别；35已主责公共行底/clip/NEW/身体原序。
    auto present=owner;
    present.scripts.pages.back().legacy_page=40;
    present.human_presence.at(2)=2; // 原!=0含另一种presence；入住D[2]仍独立。
    for(auto &home:present.human_homes)home.second[2]=0;
    present.human_homes.at(1)[2]=1;
    options.title_widths=std::array<int,2>{80,82};
    options.arrow_selected={true,false};
    const auto present_before=startup_world_state_digest(present);
    const auto gifts=steam_present_directory_skin(present,id,options);
    check(gifts&&gifts->raw==40&&gifts->touches.size()==9&&
          label(*gifts,Role::name_header).position==std::array<int,2>{30,69}&&
          label(*gifts,Role::equipment_header).position==std::array<int,2>{172,69}&&
          label(*gifts,Role::equipment_header).mode==Mode::plain&&
          label(*gifts,Role::choose_adventurer).position==std::array<int,2>{120,200}&&
          label(*gifts,Role::choose_adventurer).mode==Mode::plain&&
          label(*gifts,Role::choose_adventurer).anchor==2,
          "40首格姓名+装备及普通选择提示，不借35等级/经验/人数富文本");
    const auto &gift_left=std::get<SteamManualArrowRequest>(gifts->draws.at(*gifts->touches.at(0).image_draw));
    const auto &gift_right=std::get<SteamManualArrowRequest>(gifts->draws.at(*gifts->touches.at(1).image_draw));
    check(gift_left.position==std::array<int,2>{26,55}&&gift_left.bounds_frame==3&&gift_left.selected&&
          gift_right.position==std::array<int,2>{214,55}&&gift_right.bounds_frame==0&&!gift_right.selected&&
          gifts->touches.at(0).component==1&&gifts->touches.at(0).value==16&&
          gifts->touches.at(1).component==1&&gifts->touches.at(1).value==18&&
          std::none_of(gifts->draws.begin(),gifts->draws.end(),[](const auto &draw){
              const auto *image=std::get_if<StartupSkinDraw>(&draw);return image&&image->image==74;
          }),"40两原bounds请求分别携真实选中结果，不再重复画common74冒充buttoneffect");
    check(std::none_of(gifts->draws.begin(),gifts->draws.end(),[](const auto &draw){
        const auto *part=std::get_if<StartupSkinDraw>(&draw);return part&&part->image==87;
    })&&gifts->touches.at(2).value==0x20000&&gifts->touches.at(2).rectangle==std::array<int,4>{3,94,231,18}&&
          gifts->touches.at(7).scroll_arguments==std::array<int,3>{6,5,0x20000},
          "40确无经验条且复用真实六人目录及18高行触摸，presence2不被排除");
    for(int slot=1;slot<4;++slot) {
        const auto empty=std::find_if(gifts->draws.begin(),gifts->draws.end(),[=](const auto &draw){
            const auto *part=std::get_if<StartupSkinDraw>(&draw);
            return part&&part->image==24&&part->crop==std::array<int,4>{126,0,18,18}&&
                   part->offset==std::array<int,2>{141+18*slot,94};
        });
        check(empty!=gifts->draws.end(),"40页0实际沿当前四装备图标，不挪用35页0内容");
    }
    present.page_phases.at(id)=1;
    const auto status=*steam_present_directory_skin(present,id,options);
    check(image(status,38).offset==std::array<int,2>{54,70}&&
          label(status,Role::residence_header).position==std::array<int,2>{88,71}&&
          label(status,Role::satisfaction_header).position==std::array<int,2>{142,71}&&
          label(status,Role::effort_header).position==std::array<int,2>{187,71}&&
          label(status,Role::residence_header).font_size==10&&
          number(status,Asset::number03,{67,97}).value==1&&
          image(status,129).offset==std::array<int,2>{52,118}&&
          image(status,91).crop==std::array<int,4>{80,0,16,16}&&image(status,91).offset==std::array<int,2>{91,95}&&
          number(status,Asset::number05,{151,98}).value==present.shop_humans.at(1).satisfaction&&
          number(status,Asset::number05,{193,98}).value==present.scene.world.world.ai.growth.at(1).definition.legacy_u,
          "40页1独立等级/大师/居住/满足/努力坐标，居住图仅D[2]==1");
    check(std::count_if(status.draws.begin(),status.draws.end(),[](const auto &draw){
        const auto *part=std::get_if<StartupSkinDraw>(&draw);return part&&part->image==91;
    })==1,"40 presence2但未入住者不画住宅，不能按presence推resident");
    options.japanese=true;
    const auto status_jp=*steam_present_directory_skin(present,id,options);
    check(label(status_jp,Role::residence_header).position==std::array<int,2>{88,69}&&
          label(status_jp,Role::satisfaction_header).position==std::array<int,2>{140,69}&&
          label(status_jp,Role::effort_header).position==std::array<int,2>{187,69}&&
          label(status_jp,Role::residence_header).font_size==0,"40日文表头不误套非JP字号/两像素偏移");
    options.japanese=false;options.english=true;present.page_phases.at(id)=2;
    const auto combat=*steam_present_directory_skin(present,id,options);
    check(image(combat,37).offset==std::array<int,2>{48,67}&&
          number(combat,Asset::number08,{80,99}).value==present.scene.world.world.ai.growth.at(1).derived.combat[0]&&
          number(combat,Asset::number05,{211,98}).value==present.human_calendar.at(1).celebrations,
          "40页2用当前战斗缓存/获勋，英语属性头专属48锚");
    present.page_phases.at(id)=0;
    present.information_page_data.at(id).selection=5;present.information_page_data.at(id).first_visible=1;
    const auto last=*steam_present_directory_skin(present,id,options);
    check(last.touches.at(2).value==0x20001&&last.touches.at(6).value==0x20005&&
          last.touches.at(7).rectangle==std::array<int,4>{221,85,3,111},
          "40保留真实首行及绝对选择，原111高滚动轨道不借110");
    const auto frozen_before=startup_world_state_digest(present);
    check(steam_present_directory_skin(present,id,options)&&startup_world_state_digest(present)==frozen_before,
          "40查询不清NEW/重算贡献/发98/扣款/改随机或父子栈");
    for(int fault=0;fault<4;++fault) {
        auto bad=present;
        if(fault==0)bad.page_phases.at(id)=3;
        if(fault==1)bad.information_page_data.at(id).lists.front()[0]=7;
        if(fault==2)bad.human_homes.erase(1);
        if(fault==3)bad.scripts.pages.back().legacy_page=35;
        const auto before=startup_world_state_digest(bad);
        check(!steam_present_directory_skin(bad,id,options)&&startup_world_state_digest(bad)==before,
              "40拒原不可达第四页/名单失配/缺当前住宅字段/错页，不修补字段");
    }
    present.information_page_data.at(id).selection=0;present.information_page_data.at(id).first_visible=0;
    check(startup_world_state_digest(present)==present_before,"40三页及滚动条件恢复后完整Owner未被绘制改变");
    const auto assets=root.parent_path();
    for(const auto asset:{Asset::number03,Asset::number11}) {
        const int width=asset==Asset::number03?8:7;
        const int expected_image=asset==Asset::number03?102:108;
        const int expected_sprite=asset==Asset::number03?11:20;
        const int height=asset==Asset::number03?12:9;
        const auto resource=steam_facility_resource(asset);
        check(resource && resource->image==expected_image && resource->sprite==expected_sprite &&
              resource->published_sprite,"35数字SEB11/20明确绑定image102/108而非同名图");
        SteamFacilityNumber request{Number::number,asset,0,{100,20},0,4,-1};
        const auto zero=steam_facility_number_draws(request,width);
        check(zero && zero->size()==1 && zero->front().frame==0 &&
              zero->front().position==std::array<int,2>{100-width,20},"35新数字零值仍展开frame0并右锚");
        request.value=12;
        const auto digits=steam_facility_number_draws(request,width);
        check(digits && digits->size()==2 && digits->at(0).frame==1 && digits->at(1).frame==2 &&
              digits->at(0).position==std::array<int,2>{100-2*width,20} &&
              digits->at(1).position==std::array<int,2>{100-width,20},"35新数字按各自8/7步宽而非统一Font宽");
        request.value=-12;
        const auto negative=steam_facility_number_draws(request,width);
        check(negative && negative->size()==1 && negative->front().frame==-12 &&
              negative->front().position==std::array<int,2>{100-width,20},
              "35新数字负值保留单个负帧请求，不abs或伪造减号像素");
        check(!steam_facility_number_draws(request,width+1),"35新增数字拒绝不匹配的SEB步宽");
        for(const auto kind:{Number::money,Number::plus_value}) {
            request.kind=kind;
            check(!steam_facility_number_draws(request,width),"35纯数字图集不接受金额/加号单位帧");
        }
        const auto seb=tools::parse_legacy_seb(read_bytes(assets/resource->published_sprite));
        CpuImage png(LoadImage((assets/resource->published_image).string().c_str()));
        check(png.image.width>0 && png.image.height>0,"35两张数字图片实际解码成功");
        for(const int frame:{0,9}) {
            const auto &parts=seb.layers.at(0).parts;
            const auto part=std::find_if(parts.begin(),parts.end(),[=](const auto &p){return p.frame==frame;});
            check(part!=parts.end() && part->image_index==expected_image &&
                  part->source_x==width*frame && part->source_y==height &&
                  part->width==width && part->height==height &&
                  part->source_x+part->width<=png.image.width && part->source_y+part->height<=png.image.height,
                  "35新增SEB首末数字真实裁片边界及源行，不由文件名推断");
        }
    }
    const auto result_path=steam_information_image(31);
    check(result_path=="steam-common/icon_result00.png","35贡献图明确使用Steam差异PNG31");
    CpuImage result_png(LoadImage((assets/std::string(*result_path)).string().c_str()));
    check(result_png.image.width==63 && result_png.image.height==48,"35已发布PNG31实际63×48");
    const auto result_seb=tools::parse_legacy_seb(read_bytes(root/"common/icon_result00.seb"));
    const auto &parts=result_seb.layers.at(0).parts;
    const auto contribution=std::find_if(parts.begin(),parts.end(),[](const auto &part){return part.frame==4;});
    check(contribution!=parts.end() && contribution->image_index==31 && contribution->source_x==0 &&
          contribution->source_y==16 && contribution->width==16 && contribution->height==16 &&
          contribution->source_x+contribution->width<=result_png.image.width &&
          contribution->source_y+contribution->height<=result_png.image.height,
          "35贡献SEB44 frame4真实裁片落在Steam图内，不以PNG列数猜帧");
}
void tax_information_skin(Checks &check) {
    using R=SteamInformationTextRole;
    const auto labels=[](const SteamInformationSkinPlan &p,R role) {
        std::vector<SteamInformationText> out;
        for(const auto &draw:p.draws)if(const auto *text=std::get_if<SteamInformationText>(&draw))
            if(text->role==role)out.push_back(*text);
        return out;
    };
    SteamInformationSkinOptions options;options.title_widths=std::array<int,2>{48,50};
    for(const bool japanese:{false,true})for(const int count:{0,6}) {
        options.japanese=japanese;SteamTaxInformationSkinInput in;
        for(int row=0;row<count;++row)in.rows.push_back({row,0,row%2,(row+1)*10,"住户"+std::to_string(row)});
        if(count){in.selection=5;in.first_visible=1;in.total=210;}
        const auto plan=steam_tax_information_skin(in,options);
        check(plan&&plan->raw==90&&plan->soft_labels==std::array<int,2>{0,0},"90空/六人纯计划无返回软键，不借普通目录页签");
        const auto titles=labels(*plan,R::tax_title),heads=labels(*plan,R::tax_owner_header),income=labels(*plan,R::tax_income_header);
        check(titles.size()==2&&titles[0].period==0&&heads.size()==1&&heads[0].position==std::array<int,2>{30,69}&&
            income.size()==1&&income[0].position==std::array<int,2>{japanese?179:154,69}&&income[0].font_size==0,
            "90标题两遍、所有者与两语言收入列头原位置，不猜字号");
        int bodies=0,numbers=0,clips=0,selected=0,unit=0,fingers=0;
        for(const auto &draw:plan->draws) {
            if(const auto *body=std::get_if<SteamInformationHumanBody>(&draw)) {
                const int row=bodies++,y=97+19*row;
                check(body->body.sprite==1&&body->body.frame==0&&body->position==std::array<int,2>{37,y+21},
                    "90当前性别职业静态身体action0/direction1，不附武器或造人物实例");
            }
            if(const auto *clip=std::get_if<SteamFacilityClip>(&draw)) {
                ++clips;if(clip->kind==SteamFacilityClipKind::push_intersect)
                    check(clip->rectangle[0]==29&&clip->rectangle[2]==15&&clip->rectangle[3]==15,"90头像15x15 clip独立于16x16底板");
            }
            if(const auto *number=std::get_if<SteamFacilityNumber>(&draw)) {
                const int row=numbers++;
                if(count&&row<5)check(number->asset==SteamFacilityAsset::number08&&number->kind==SteamFacilityNumberKind::money&&
                    number->value==(row+2)*10&&number->position==std::array<int,2>{212,98+19*row},"90行款沿当前G原SEB15和Draw_money锚");
                else check(number->asset==SteamFacilityAsset::number05&&number->kind==SteamFacilityNumberKind::money&&
                    number->value==in.total&&number->position==std::array<int,2>{212,201},"90合计用独立SEB12，不复制行钱SEB15");
            }
            if(const auto *rect=std::get_if<StartupSkinRect>(&draw)) {
                if(rect->rect==std::array<int,4>{23,170,191,18}&&rect->rgb==std::array<int,3>{255,153,55})++selected;
            }
            if(const auto *image=std::get_if<StartupSkinDraw>(&draw)) {
                if(image->image==103&&image->sprite==12&&image->frame==20&&image->offset==std::array<int,2>{204,70})++unit;
                if(image->image==70&&image->sprite==21&&image->frame==-1&&image->offset==std::array<int,2>{21,181})++fingers;
            }
        }
        const auto names=labels(*plan,R::row_name),totals=labels(*plan,R::tax_total_label);
        check(bodies==(count?5:0)&&clips==2*bodies&&numbers==bodies+1&&unit==1&&selected==(count?1:0)&&fingers==(count?1:0)&&
            names.size()==static_cast<std::size_t>(bodies)&&totals.size()==1&&totals[0].position==std::array<int,2>{30,200},"90五行末窗/空目录输出规模与合计、手形明确");
        for(int row=0;row<bodies;++row) {
            check(names[row].slot==row+1&&names[row].value=="住户"+std::to_string(row+1)&&names[row].position==std::array<int,2>{48,97+19*row}&&
                names[row].mode==SteamInformationTextMode::plain,"90姓名原普通文本，保留冻结绝对行号");
            const auto &touch=plan->touches[row];
            check(touch.component==11&&touch.value==(0x20000|(row+1))&&touch.option==0&&touch.margin==std::array<int,4>{}&&
                touch.rectangle==std::array<int,4>{3,94+19*row,231,18},"90行无marker/额外margin，命中框保留绝对索引");
        }
        const auto &bar=plan->touches[bodies],&track=plan->touches[bodies+1],&confirm=plan->touches.back();
        check(bar.component==12&&bar.rectangle==std::array<int,4>{221,85,3,111}&&bar.margin==std::array<int,4>{3,20,0,0}&&
            bar.scroll_arguments==std::array<int,3>{count,5,0x20000}&&track.component==25&&track.option==4&&
            confirm.component==2&&!confirm.rectangle&&confirm.option==2,"90 H111源滚动注册与末确认Touch2；不添加猜测确认按钮");
        if(count) {
            auto bad=in;bad.rows[0].definition=bad.rows[1].definition;
            check(!steam_tax_information_skin(bad,options),"90隐藏行重复源也拒绝");
            bad=in;bad.rows[0].sex=2;check(!steam_tax_information_skin(bad,options),"90隐藏坏身体身份拒绝");
            bad=in;bad.first_visible=0;check(!steam_tax_information_skin(bad,options),"90选择不在五行首窗拒绝，不隐式修游标");
        }
    }
    auto state=test_support::page_fixture(90);const auto page=state.scripts.pages.back().id;
    check(!steam_tax_information_skin(state,page,options),"90未初始化皮肤不得借实时枚举兜底");
    for(int row=0;row<6;++row) {
        state.human_presence.at(row)=1;state.human_homes.at(row)[2]=1;
        state.human_calendar.at(row).legacy_G=(row+1)*10;
    }
    const auto initialized=prepare_startup_world_runtime(state);
    check(initialized.candidate.has_value(),"90皮肤夹具走真实框架Init，不直接将pending页当已准入页");
    state=*initialized.candidate;
    check(state.scripts.pages.back().lifecycle==2&&
        act_startup_world_tax_page(state,page,StartupWorldTaxAction::select,5)==StartupWorldRuntimeError::none,"90真实初始化冻结名单/已存在选择跟随消费者");
    state.human_calendar.at(5).legacy_G=90;state.human_homes.at(5)[2]=0;
    const auto before=startup_world_state_digest(state);const auto plan=steam_tax_information_skin(state,page,options);
    check(plan.has_value(),"90Owner桥沿冻结身份读取currentG，不重枚举入住资格");
    int current=-1,total=-1;for(const auto &draw:plan->draws)if(const auto *n=std::get_if<SteamFacilityNumber>(&draw)) {
        if(n->position==std::array<int,2>{212,174})current=n->value;
        if(n->position==std::array<int,2>{212,201})total=n->value;
    }
    check(current==90&&total==240,"90末行 currentG与全冻结名单合计接线，税务计算沿pages主责");
    const auto same=steam_tax_information_skin(state,page,options);
    check(same&&same->draws.size()==plan->draws.size()&&same->touches.size()==plan->touches.size()&&
        startup_world_state_digest(state)==before,"90重复查询无资金/随机/页/输出改变且资源规模不增长");
    for(int fault=0;fault<7;++fault) {
        auto bad=state;
        if(fault==0)bad.tax_page_residents.erase(page);
        if(fault==1)bad.tax_page_selection.erase(page);
        if(fault==2)bad.tax_page_scroll.erase(page);
        if(fault==3)bad.page_counters.erase(page);
        if(fault==4)bad.tax_page_residents.at(page)[0]=99999;
        if(fault==5)bad.scene.world.world.ai.growth.erase(1);
        if(fault==6)bad.scripts.pages.back().legacy_page=35;
        check(!steam_tax_information_skin(bad,page,options),"90缺初始化/错页/未知冻结人或坏身体源明确拒绝");
    }
    options.title_widths.reset();check(!steam_tax_information_skin(state,page,options),"90缺标题实测宽拒绝");
    options.title_widths=std::array<int,2>{48,50};
    check(act_startup_world_tax_page(state,page,StartupWorldTaxAction::confirm)==StartupWorldRuntimeError::none&&
        !steam_tax_information_skin(state,page,options),"90确认退休后桥拒引用，不在皮肤执行98入账");
}
void town_facility_information_skin(Checks &check,const std::filesystem::path &root) {
    using Role=SteamInformationTextRole;using Mode=SteamInformationTextMode;
    using Asset=SteamFacilityAsset;using Number=SteamFacilityNumberKind;
    const auto label=[&](const auto &plan,Role role,int slot=-1) -> const SteamInformationText & {
        const auto it=std::find_if(plan.draws.begin(),plan.draws.end(),[=](const auto &draw) {
            const auto *text=std::get_if<SteamInformationText>(&draw);
            return text&&text->role==role&&(slot<0||text->slot==slot);
        });
        check(it!=plan.draws.end(),"34/39所需文字角色与绝对行存在");return std::get<SteamInformationText>(*it);
    };
    const auto images=[](const auto &plan,int image) {
        std::vector<StartupSkinDraw> result;
        for(const auto &draw:plan.draws)if(const auto *part=std::get_if<StartupSkinDraw>(&draw))
            if(part->image==image)result.push_back(*part);
        return result;
    };
    const auto numbers=[](const auto &plan) {
        std::vector<SteamFacilityNumber> result;
        for(const auto &draw:plan.draws)if(const auto *n=std::get_if<SteamFacilityNumber>(&draw))result.push_back(*n);
        return result;
    };
    // 统计/语言/测宽是显示条件；不跑经营前缀、不制造自然星级证明。
    auto town=test_support::page_fixture(34);const auto town_id=town.scripts.pages.back().id;
    town.scripts.pages.back().lifecycle=0;town.rank=3;town.task_progress.successes=7;town.events_held=9;
    for(auto &h:town.human_presence)h.second=0;
    town.human_presence.at(1)=1;
    SteamInformationSkinOptions options;options.title_widths=std::array<int,2>{80,82};options.bottom_width=80;
    check(!steam_town_information_skin(town,town_id,options),"34皮肤不代替真实Init");
    check(initialize_startup_world_information_pages(town),"34真实Init安装已核统计页面载荷");
    const auto before=startup_world_state_digest(town);
    const auto plan=steam_town_information_skin(town,town_id,options);
    check(plan && plan->raw==34 && plan->touches.size()==1 && images(*plan,74).empty() &&
          label(*plan,Role::village_name).value==town.scripts.village_name &&
          label(*plan,Role::village_name).position==std::array<int,2>{30,69},"34实际村名、单底栏触摸，无标题箭头");
    const auto stars=images(*plan,13);
    check(stars.size()==5,"34总共五个星级SEB请求");
    for(int slot=0;slot<5;++slot) {
        check(stars[slot].sprite==33 && stars[slot].frame==(slot<3?0:1) &&
              stars[slot].offset==std::array<int,2>{147+12*slot,69},"34首星是147，已获/空星沿原五槽");
        const auto &text=label(*plan,Role::town_stat_label,slot);
        check(text.position==std::array<int,2>{22,93+20*slot} && text.extent==std::array<int,2>{68,15} &&
              text.mode==Mode::layout && text.line_space==0 && text.anchor==0x20 &&
              text.rgb==std::array<int,3>{0,100,255} && text.font_size==(slot==0?9:slot==3?8:10),
              "34非日文五行TextLayout的独立字号/坐标/蓝色");
    }
    const auto stats=numbers(*plan);
    check(stats.size()==5 && stats[0].value==1 && stats[3].value==7 && stats[4].value==9,
          "34绘制读取Owner统计而不追加住宅/分母行");
    for(int slot=0;slot<5;++slot)check(stats[slot].asset==Asset::number05 && stats[slot].kind==Number::number &&
          stats[slot].position==std::array<int,2>{109,96+20*slot} && stats[slot].anchor==4,
          "34五行数值SEB12右锚保持原行序");
    const auto equipment=images(*plan,128);
    check(equipment.size()==4,"34四种类保留四个SEB图标");
    for(int slot=0;slot<4;++slot)check(equipment[slot].sprite==88 && equipment[slot].frame==slot+1 &&
          equipment[slot].offset==std::array<int,2>{121,91+20*slot} &&
          label(*plan,Role::town_equipment_count,slot).position==std::array<int,2>{213,94+20*slot} &&
          label(*plan,Role::town_equipment_count,slot).font_size==11,
          "34非日文装备数量与各自原图标帧、临时11字号");
    for(const auto sample:std::array<std::array<int,3>,3>{{{0,112,16},{81,72,97},{201,12,216}}}) {
        auto widths=options;widths.bottom_width=sample[0];const auto button=*steam_town_information_skin(town,town_id,widths);
        const auto &fill=std::get<StartupSkinRect>(button.draws.at(button.draws.size()-3));
        const auto &text=std::get<SteamInformationText>(button.draws.at(button.draws.size()-2));
        const auto &hand=std::get<StartupSkinDraw>(button.draws.back());
        check(fill.rect==std::array<int,4>{sample[1],197,sample[2],16} && fill.rgb==std::array<int,3>{255,153,55} &&
              text.role==Role::facility_income_list && text.mode==Mode::layout && text.anchor==0x22 && text.line_space==0 &&
              text.position==std::array<int,2>{sample[1],197} && text.extent==std::array<int,2>{sample[2],16} &&
              hand.sprite==21 && hand.frame==-1 && hand.offset==std::array<int,2>{sample[1]-4,206} &&
              button.touches[0].component==4 && button.touches[0].value==20 &&
              button.touches[0].rectangle==std::array<int,4>{sample[1]-20,177,sample[2]+40,56},
              "34底部真实测宽先截200再加16，保留奇数整除、绘序与扩展触摸");
    }
    options.japanese=true;const auto japanese=*steam_town_information_skin(town,town_id,options);
    check(numbers(japanese).size()==4 && label(japanese,Role::town_adventurer_count).argument==1 &&
          label(japanese,Role::town_adventurer_count).position==std::array<int,2>{109,95} &&
          label(japanese,Role::town_adventurer_count).anchor==4 &&
          label(japanese,Role::town_stat_label,0).position==std::array<int,2>{27,95} &&
          label(japanese,Role::town_stat_label,0).mode==Mode::plain &&
          label(japanese,Role::town_stat_label,0).font_size==0 &&
          images(japanese,128).front().offset==std::array<int,2>{122,91} &&
          label(japanese,Role::town_equipment_kinds,0).position==std::array<int,2>{211,93} &&
          label(japanese,Role::town_equipment_kinds,0).font_size==0,"34日文人数改文本带后缀，装备改种类翻译，不套非日文数字");
    options.japanese=false;
    for(const int rank:{0,5}) {
        auto ranked=town;ranked.rank=rank;const auto ranked_plan=*steam_town_information_skin(ranked,town_id,options);
        const auto rank_stars=images(ranked_plan,13);
        check(rank_stars.size()==5 && std::all_of(rank_stars.begin(),rank_stars.end(),[=](const auto &s){return s.frame==(rank?0:1);}),
              "34零星/五星仍请求恰五槽，不越界或多画一颗");
    }
    for(int fault=0;fault<4;++fault) {
        auto bad=town;auto input=options;
        if(fault==0)input.bottom_width.reset();if(fault==1)input.bottom_width=-1;
        if(fault==2)bad.page_counters.erase(town_id);if(fault==3)input.title_widths.reset();
        check(!steam_town_information_skin(bad,town_id,input),"34缺真实测宽/已Init载荷显式拒绝");
    }
    check(startup_world_state_digest(town)==before,"34所有语言/测宽查询不改Owner或输出");
    // 39采用真实34确认及真实Init；额外同定义实例仅供六行显示条件，不从副本运行经营。
    auto tick=prepare_startup_world_runtime(town);check(tick.candidate.has_value(),"34实际框架进入可输入阶段");
    town=std::move(*tick.candidate);
    StartupInformationInput confirm;confirm.confirm=true;
    check(input_startup_world_information_page(town,town_id,confirm)==StartupWorldRuntimeError::none,"真实34确认创建39");
    const auto page=town.scripts.pages.back().id;
    const auto original=startup_facility_information(town);check(original && !original->empty(),"39原世界已有营业设施");
    const auto first=original->front().instance;
    for(int count=static_cast<int>(original->size()),ordinal=20;count<6;++count,++ordinal) {
        const auto extra=town.next_facility_identity++;
        auto instance=town.scene.world.world.facilities.at(first);instance.placement.instance_id={extra};
        town.scene.world.world.facilities.emplace(extra,instance);town.scene.world.facility_order.push_back(extra);
        town.facility_ordinals.emplace(extra,ordinal);town.facility_monthly_cash[extra]={};
    }
    check(!steam_facility_information_skin(town,page,options),"39未Init不由皮肤创建目录");
    check(initialize_startup_world_information_pages(town),"39真实Init冻结稳定实例目录");
    const auto ids=town.information_page_data.at(page).facilities;
    check(ids.size()==6,"39六行最小滚动条件");
    const std::array<int,6> profits{0,123,-12,std::numeric_limits<int>::min(),5,9};
    for(std::size_t row=0;row<ids.size();++row) {
        town.facility_monthly_cash.at(ids[row])={};town.facility_monthly_cash.at(ids[row])[0][0]=profits[row];
    }
    town.facility_ordinals.at(first)=7;
    const auto frozen=startup_world_state_digest(town);
    const auto facilities=steam_facility_information_skin(town,page,options);
    check(facilities && facilities->raw==39 && facilities->touches.size()==7 && images(*facilities,74).empty(),
          "39五行两滚动，无标题箭头或底栏按钮");
    const auto selected=std::find_if(facilities->draws.begin(),facilities->draws.end(),[](const auto &draw) {
        const auto *rect=std::get_if<StartupSkinRect>(&draw);return rect&&rect->rgb==std::array<int,3>{255,153,55};
    });
    check(selected!=facilities->draws.end(),"39选中行有真实橙底");
    const auto start=static_cast<std::size_t>(selected-facilities->draws.begin());
    const auto &fill=std::get<StartupSkinRect>(facilities->draws[start]);
    const auto &hand=std::get<StartupSkinDraw>(facilities->draws[start+1]);
    const auto &icon=std::get<StartupSkinDraw>(facilities->draws[start+2]);
    const auto &name=std::get<SteamInformationText>(facilities->draws[start+3]);
    const auto &money=std::get<SteamFacilityNumber>(facilities->draws[start+4]);
    check(fill.rect==std::array<int,4>{23,95,191,16} && hand.sprite==21 && hand.frame==-1 &&
          hand.offset==std::array<int,2>{21,105} && icon.image==91 &&
          icon.crop==std::array<int,4>{16*original->front().icon,0,16,16} && icon.offset==std::array<int,2>{30,95} &&
          name.role==Role::facility_name && name.value==original->front().name && name.argument==8 &&
          name.mode==Mode::layout && name.extent==std::array<int,2>{110,11} && name.line_space==0 &&
          name.position==std::array<int,2>{48,97} && name.anchor==0x20 && name.font_size==0 &&
          money.kind==Number::money && money.value==0 && money.asset==Asset::number05,
          "39橙底→手形→无底板定义icon→双参数名称→金额原序，编号取ordinal+1");
    const auto amounts=numbers(*facilities);
    check(amounts.size()==5 && amounts[1].asset==Asset::number05 && amounts[1].value==123 &&
          amounts[2].asset==Asset::number12 && amounts[2].value==12 &&
          amounts[3].asset==Asset::number12 && amounts[3].value==std::numeric_limits<int>::min(),
          "39零/正利润用SEB12，负利润用SEB19；INT_MIN保持32位neg结果");
    for(int row=0;row<5;++row)check(amounts[row].position==std::array<int,2>{210,98+19*row} &&
          facilities->touches[row].rectangle==std::array<int,4>{3,95+19*row,231,16} &&
          facilities->touches[row].value==(0x20000|row),"39行锚19递增，触摸使用16高与绝对索引");
    check(label(*facilities,Role::name_header).position==std::array<int,2>{28,69} &&
          label(*facilities,Role::facility_profit_header).position==std::array<int,2>{179,69} &&
          label(*facilities,Role::facility_tracking_hint).position==std::array<int,2>{120,198} &&
          label(*facilities,Role::facility_tracking_hint).mode==Mode::plain &&
          label(*facilities,Role::facility_tracking_hint).rgb==std::array<int,3>{0,100,255} &&
          label(*facilities,Role::facility_tracking_hint).anchor==2 && images(*facilities,70).size()==1,
          "39末尾仅蓝色输入提示，不添加底部按钮或第二手形");
    auto scrolled=town;scrolled.information_page_data.at(page).selection=5;scrolled.information_page_data.at(page).first_visible=1;
    options.scroll_first_touch=true;const auto scrolling=*steam_facility_information_skin(scrolled,page,options);
    check(scrolling.touches.front().value==0x20001 && scrolling.touches[5].rectangle==std::array<int,4>{221,85,3,110} &&
          std::get<StartupSkinRect>(scrolling.draws[scrolling.draws.size()-3]).rect==std::array<int,4>{220,85,5,110} &&
          std::get<StartupSkinRect>(scrolling.draws[scrolling.draws.size()-2]).rect==std::array<int,4>{220,103,5,92},
          "39滚动复用110高与绝对行，不能套35的111");
    options.scroll_first_touch=false;options.japanese=true;options.view_y=20;
    const auto shifted=*steam_facility_information_skin(town,page,options);
    check(label(shifted,Role::name_header).position==std::array<int,2>{26,69} &&
          label(shifted,Role::facility_profit_header).position==std::array<int,2>{185,69} &&
          label(shifted,Role::facility_name,0).position==name.position &&
          label(shifted,Role::title).position[1]==label(*facilities,Role::title).position[1]+10 &&
          shifted.touches.front().rectangle==facilities->touches.front().rectangle,
          "39日文仅表头改锚，VIEW_Y仅交框/box，不重复移动行或热区");
    options.japanese=false;options.view_y=0;
    for(int n=0;n<8;++n) {
        const auto repeated=steam_facility_information_skin(town,page,options);
        check(repeated && repeated->draws.size()==facilities->draws.size() && repeated->touches.size()==7 &&
              startup_world_state_digest(town)==frozen,"39重复只读计划不增长图元/页载荷、不改资金、随机或输出");
    }
    for(int fault=0;fault<5;++fault) {
        auto bad=town;
        if(fault==0)bad.information_page_data.erase(page);if(fault==1)bad.information_page_data.at(page).selection=6;
        if(fault==2)bad.facility_monthly_cash.erase(first);if(fault==3)bad.facility_ordinals.at(first)=std::numeric_limits<int>::max();
        if(fault==4)for(auto &p:bad.scripts.pages)if(p.id==town_id)p.legacy_page=36;
        const auto digest=startup_world_state_digest(bad);
        check(!steam_facility_information_skin(bad,page,options) && startup_world_state_digest(bad)==digest,
              "39缺目录/账目/错父/越界编号及选择拒绝整份皮肤");
    }
    const auto assets=root.parent_path();
    for(const auto record:std::array<std::array<int,3>,3>{{{13,25,14},{91,112,16},{109,100,21}}}) {
        const auto path=steam_information_image(record[0]);check(path.has_value(),"34/39所用图有显式版本路径");
        CpuImage png(LoadImage((assets/std::string(*path)).string().c_str()));
        check(png.image.width==record[1] && png.image.height==record[2],"34/39正式PNG实际解码尺寸");
    }
    check(steam_information_image(109)=="steam-common/number12.png" &&
          read_bytes(assets/"steam-common/number12.png")!=read_bytes(root/"common/number12.png"),
          "39负利润明确Steam差异PNG而非APK同名资源");
}
void information_menu_skin(Checks &check,const std::filesystem::path &root) {
    auto owner=test_support::page_fixture(9);const auto id=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=0;
    SteamInformationMenuSkinOptions options;
    check(!steam_information_menu_skin(owner,id,options),"raw9皮肤不初始化Owner页面");
    check(initialize_startup_world_information_pages(owner),"raw9实际Init建立选择/计数载荷");
    owner.page_phases.at(id)=2;
    for(auto &p:owner.human_presence)p.second=0;
    for(auto &h:owner.scripts.humans)h.second.pending_notice=true;
    const auto images=[](const auto &plan,int image) {
        std::vector<StartupSkinDraw> result;
        for(const auto &draw:plan.draws)if(const auto *part=std::get_if<StartupSkinDraw>(&draw))
            if(part->image==image)result.push_back(*part);
        return result;
    };
    const auto labels=[](const auto &plan) {
        std::vector<SteamInformationText> result;
        for(const auto &draw:plan.draws)if(const auto *text=std::get_if<SteamInformationText>(&draw))result.push_back(*text);
        return result;
    };
    constexpr std::array<int,4> widths{0,29,59,89},heights{0,9,19,29};
    constexpr std::array<int,5> tags{15,14,16,17,18};
    for(int frame=0;frame<4;++frame) {
        owner.page_counters.at(id)=frame;
        options.measured_text_widths=frame==3?std::optional<std::array<int,5>>{{80,81,0,80,81}}:std::nullopt;
        const auto plan=steam_information_menu_skin(owner,id,options);
        check(plan && plan->raw==9 && plan->origin==std::array<int,2>{0,0} && plan->touches.size()==6,
              "raw9所有展开阶段登记五行及共用KEYCLICK基触摸");
        const auto backgrounds=images(*plan,25);const auto text=labels(*plan);
        check(backgrounds.size()==(frame?5U:0U) && text.size()==(frame==3?5U:0U) &&
              images(*plan,147).empty() && images(*plan,70).size()==(frame==3?1U:0U),
              "raw9 frame0不画底图；文字/手形等待frame3；p0的NEW不显示");
        for(int row=0;row<5;++row) {
            const auto &touch=plan->touches[row];
            check(touch.component==9 && touch.value==(0x20000|row) && touch.option==0 &&
                  touch.rectangle==std::array<int,4>{40,28*row,50,28},"raw9触摸全尺寸不随展开裁片缩小");
            if(frame)check(backgrounds[row].crop==std::array<int,4>{68,row==2?0:29,widths[frame],heights[frame]} &&
                  backgrounds[row].offset==std::array<int,2>{1,28*row+1},
                  "raw9原menu89×29裁片缩源，保留源起点和选择帧，不能拉伸");
            if(frame==3)check(text[row].role==SteamInformationTextRole::menu_entry && text[row].argument==tags[row] &&
                  text[row].slot==row && text[row].position==std::array<int,2>{7,28*row+9} &&
                  text[row].rgb==(row==2?std::array<int,3>{76,58,50}:std::array<int,3>{255,242,220}) &&
                  text[row].font_size==((row==1||row==4)?11:0),
                  "raw9五原tag及选中色，只有实测宽严格超过80才用11字号");
        }
        const auto &key=plan->touches.back();
        check(key.component==4 && key.value==22 && key.option==2 && !key.rectangle,
              "raw9尾KEYCLICK无矩形，不能解释为全屏点击关闭");
    }
    // NEW由定义状态与pending决定，没有地图W仍合法；其余目录NEW不是本页的灯号。
    owner.human_presence.at(1)=2;
    check(owner.scene.world.world.ai.human_order.empty(),"raw9 NEW夹具没有伪造场上人物");
    for(const auto language:std::array<std::array<int,4>,4>{{{0,0,68,50},{0,1,74,50},{1,0,78,44},{1,1,78,44}}}) {
        options.japanese=language[0]!=0;options.english=language[1]!=0;
        options.measured_text_widths=std::array<int,5>{74,75,0,0,0};
        const auto plan=*steam_information_menu_skin(owner,id,options);
        const auto notice=images(plan,147);const auto text=labels(plan);
        check(notice.size()==1 && notice[0].offset==std::array<int,2>{language[2],16} &&
              plan.touches.front().rectangle==std::array<int,4>{40,0,language[3],28} &&
              text[0].font_size==0 && text[1].font_size==(options.japanese?11:0),
              "raw9 NEW英语加6但日文覆盖为10，日文布局84与测宽阈值74独立");
    }
    options.japanese=false;options.english=false;
    owner.human_presence.at(1)=0;
    for(auto &item:owner.items)item.second.newly_unlocked=true;
    for(auto &item:owner.catalog)item.second.newly_unlocked=true;
    check(images(*steam_information_menu_skin(owner,id,options),147).empty(),"raw9道具/装备NEW不代替唯一人物NEW资格");
    owner.human_presence.at(1)=1;
    // 只提供已知Graphics原点；本函数不能擅自为场景直达入口造raw3或居中。
    options.canvas={400,300};options.origin={200,0};options.safe_left=10;
    const auto ordinary=*steam_information_menu_skin(owner,id,options);
    check(ordinary.origin==options.origin && images(ordinary,25).front().offset==std::array<int,2>{11,1} &&
          images(ordinary,70).front().sprite==22 && images(ordinary,70).front().offset==std::array<int,2>{106,69},
          "raw9宽画布不再居中，手形按局部dx10选22而非屏幕x210");
    options.canvas={240,240};options.origin={200,200};options.safe_left=10;
    const auto overflow=*steam_information_menu_skin(owner,id,options);
    check(overflow.origin==options.origin && images(overflow,25).front().offset==std::array<int,2>{-49,-104} &&
          overflow.touches.front().rectangle==std::array<int,4>{-10,-105,50,28} &&
          images(overflow,70).front().sprite==22 && images(overflow,70).front().offset==std::array<int,2>{46,-36},
          "raw9右底越界覆盖safe-left为负局部偏移，保留负锚而非硬夹到零");
    options.origin={0,0};options.safe_left=120;
    const auto left_hand=*steam_information_menu_skin(owner,id,options);
    check(images(left_hand,70).front().sprite==21 && images(left_hand,70).front().offset==std::array<int,2>{118,69},
          "raw9局部dx恰120使用21，21/22都消费image70");
    options.safe_left=200;
    check(images(*steam_information_menu_skin(owner,id,options),25).front().offset==std::array<int,2>{201,1},
          "raw9溢出判断不把safe-left再加入Ox，不自行追加右侧裁限");
    options.safe_left=0;options.on_top=false;
    const auto covered_menu=*steam_information_menu_skin(owner,id,options);
    check(images(covered_menu,25).size()==5 && images(covered_menu,70).empty() && covered_menu.touches.size()==6,
          "被菜单覆盖仍画五行但非栈顶不画手形");
    options.covered_by_nonmenu_subform=true;options.measured_text_widths.reset();
    const auto hidden=steam_information_menu_skin(owner,id,options);
    check(hidden && hidden->draws.empty() && hidden->touches.empty(),"被非菜单SubForm覆盖时返回空计划，无需猜不可见文字宽");
    options.covered_by_nonmenu_subform=false;options.on_top=true;options.measured_text_widths=std::array<int,5>{80,81,0,80,81};
    const auto before=startup_world_state_digest(owner);const auto base=*steam_information_menu_skin(owner,id,options);
    for(int repeat=0;repeat<8;++repeat) {
        const auto again=steam_information_menu_skin(owner,id,options);
        check(again && again->draws.size()==base.draws.size() && again->touches.size()==base.touches.size() &&
              startup_world_state_digest(owner)==before,"raw9重复查询不增长图元/触摸或消费NEW、计数、随机、一次性输出");
    }
    for(int fault=0;fault<9;++fault) {
        auto bad=owner;auto input=options;
        if(fault==0)bad.page_phases.erase(id);if(fault==1)bad.page_counters.at(id)=4;
        if(fault==2)input.measured_text_widths.reset();if(fault==3)(*input.measured_text_widths)[1]=-1;
        if(fault==4)input.canvas[0]=0;if(fault==5)input.safe_left=-1;
        if(fault==6)input.safe_left=std::numeric_limits<int>::max();
        if(fault==7){input.canvas={1,1};input.origin={std::numeric_limits<int>::max(),std::numeric_limits<int>::max()};}
        if(fault==8)bad.scripts.humans.erase(1);
        const auto digest=startup_world_state_digest(bad);
        check(!steam_information_menu_skin(bad,id,input) && startup_world_state_digest(bad)==digest,
              "raw9缺已Init载荷/NEW源、未知测宽、坏画布或算术越界拒绝，不修补Owner");
    }
    const auto menu=tools::parse_legacy_seb(read_bytes(root/"common/menu.seb"));
    for(const int frame:{2,3}) {
        const auto &parts=menu.layers.at(0).parts;
        const auto part=std::find_if(parts.begin(),parts.end(),[=](const auto &p){return p.frame==frame;});
        check(part!=parts.end() && part->image_index==25 && part->source_x==68 && part->source_y==(frame==2?0:29) &&
              part->width==89 && part->height==29,"raw9 menu选中/普通真实SEB裁片oracle");
    }
    for(const auto file:{"finger_r.seb","finger_l.seb"}) {
        const auto seb=tools::parse_legacy_seb(read_bytes(root/"common"/file));
        check(!seb.layers.empty() && !seb.layers[0].parts.empty() &&
              std::all_of(seb.layers[0].parts.begin(),seb.layers[0].parts.end(),[](const auto &part){return part.image_index==70;}),
              "raw9两方向手形SEB均引用finger_r.png的image70，不能寻找不存在的finger_l.png");
    }
}
void navigation_menu_skin(Checks &check) {
    const auto fixture=[&](int raw,int count) {
        auto s=test_support::page_fixture(raw);s.scene.framework_paused=false;
        s.scripts.pages.back().lifecycle=0;
        if(raw==4) {
            s.scripts.user_flags=count>1?4U:0U;
            if(count==3) {
                s.tasks[1].identity=1;s.next_task_identity=2;s.active_task=1;
            }
        } else s.scripts.user_flags=count==3?24U:count==2?16U:0U;
        check(initialize_startup_world_menu_pages(s),"raw4/7条件夹具由真实Init冻结可变目录");
        s.scripts.pages.back().lifecycle=2;
        return s;
    };
    const auto images=[](const auto &plan,int id) {
        std::vector<StartupSkinDraw> result;
        for(const auto &draw:plan.draws)if(const auto *part=std::get_if<StartupSkinDraw>(&draw))
            if(part->image==id)result.push_back(*part);
        return result;
    };
    constexpr std::array<int,4> crop_widths{0,29,59,89},crop_heights{0,9,19,29};
    for(int raw:{4,7})for(int count:{1,2,3}) {
        auto s=fixture(raw,count);const auto id=s.scripts.pages.back().id;
        const auto tags=s.menu_page_data.at(id).tags;
        check(static_cast<int>(tags.size())==count,"条件fixture包含一至三项，不伪填五项");
        s.menu_page_data.at(id).selection=count-1;
        SteamNavigationMenuSkinOptions options;options.origin={200,280};options.safe_left=9;
        for(int frame=0;frame<4;++frame) {
            s.page_counters.at(id)=frame;
            options.measured_text_widths=frame==3?std::optional<std::vector<int>>(std::vector<int>(count,81)):std::nullopt;
            const auto plan=steam_navigation_menu_skin(s,id,options);
            check(plan && plan->raw==raw && plan->origin==options.origin && plan->touches.size()==std::size_t(count+1),
                  "raw4/7读真实Owner且边界高/触摸数随冻结目录长度");
            const auto backgrounds=images(*plan,25);
            check(backgrounds.size()==(frame?std::size_t(count):0U),"子菜单底图保持原缩源展开时点");
            const int dy=240-(count*28+5)-280;
            for(int row=0;row<count;++row) {
                check(plan->touches[row].component==9 && plan->touches[row].value==(0x20000|row) &&
                      plan->touches[row].rectangle==std::array<int,4>{-10,dy+28*row,50,28},
                      "Steam type1 ID9从dx40开始，不能套APK整行热区");
                if(frame)check(backgrounds[row].crop==std::array<int,4>{68,row==count-1?0:29,crop_widths[frame],crop_heights[frame]},
                              "一至三行都复用menu.seb原89x29裁片");
            }
            if(frame==3) {
                std::vector<SteamInformationText> labels;
                for(const auto &draw:plan->draws)if(const auto *part=std::get_if<SteamInformationText>(&draw))labels.push_back(*part);
                check(labels.size()==tags.size(),"每冻结条目消费一条实际测宽");
                for(int row=0;row<count;++row)
                    check(labels[row].argument==tags[row] && labels[row].font_size==11 &&
                          labels[row].position==std::array<int,2>{-43,dy+28*row+9},"保留真实tag和超过80临时11字号");
            } else {
                options.measured_text_widths=std::vector<int>{-1};
                check(steam_navigation_menu_skin(s,id,options).has_value(),"展开期不消费错误长度/负测宽");
            }
        }
        const auto before=startup_world_state_digest(s);
        check(steam_navigation_menu_skin(s,id,options).has_value() && startup_world_state_digest(s)==before,
              "子菜单绘制不清NEW、不消费随机/输出或改缓存位置");
        for(int fault=0;fault<5;++fault) {
            auto bad=s;auto altered=options;
            if(fault==0)altered.measured_text_widths.reset();
            if(fault==1)altered.measured_text_widths->push_back(0);
            if(fault==2)(*altered.measured_text_widths)[0]=-1;
            if(fault==3)bad.menu_page_positions.erase(id);
            if(fault==4)bad.menu_page_data.at(id).selection=count;
            check(!steam_navigation_menu_skin(bad,id,altered),"稳定子菜单缺测宽/错长度/位置/行号显式拒绝");
        }
    }
    for(int raw:{4,7}) {
        auto minimal=fixture(raw,1);const auto id=minimal.scripts.pages.back().id;
        minimal.page_counters.at(id)=3;
        if(raw==4)minimal.task_order={minimal.next_task_identity};
        else {minimal.shop_item_stock.clear();minimal.facility_presence.clear();}
        SteamNavigationMenuSkinOptions options;options.measured_text_widths=std::vector<int>{0};
        check(steam_navigation_menu_skin(minimal,id,options).has_value(),
              "未冻结tag7/11的菜单不消费其悬空任务或商会字段");
    }
    auto adventure=fixture(4,2);const auto aid=adventure.scripts.pages.back().id;
    adventure.page_counters.at(aid)=3;adventure.task_order={1};adventure.next_task_identity=2;
    adventure.tasks[1].identity=1;adventure.tasks[1].newly_available=true;
    for(auto &[key,item]:adventure.catalog)item.newly_unlocked=false;
    adventure.catalog.at({1,0}).status=-1;adventure.catalog.at({1,0}).newly_unlocked=true;
    SteamNavigationMenuSkinOptions options;options.measured_text_widths=std::vector<int>{74,75};
    for(const auto language:std::array<std::array<int,4>,4>{{{0,0,68,65},{0,1,74,71},{1,0,76,76},{1,1,76,76}}}) {
        options.japanese=language[0];options.english=language[1];
        const auto plan=*steam_navigation_menu_skin(adventure,aid,options);
        check(images(plan,147).size()==1 && images(plan,148).size()==1 &&
              images(plan,147)[0].offset==std::array<int,2>{language[2],16} &&
              images(plan,148)[0].offset==std::array<int,2>{language[3],44},
              "raw4任务NEW与赠礼GET独立共存，JP8覆盖En6且仅非JP GET减3");
    }
    adventure.catalog.erase({1,0});
    check(!steam_navigation_menu_skin(adventure,aid,options),"任务NEW不能跳过tag9的缺装备来源");
    options.covered_by_nonmenu_subform=true;options.measured_text_widths.reset();
    check(steam_navigation_menu_skin(adventure,aid,options)->draws.empty(),"非菜单遮挡不读NEW/GET或测宽");
    auto village=fixture(7,3);const auto vid=village.scripts.pages.back().id;village.page_counters.at(vid)=3;
    for(auto &[id,a]:village.scripts.activities)a.status=0;
    const int activity=village.rules->activities.front().identity;
    village.scripts.activities.at(activity).status=1;village.scripts.activities.at(activity).pending_notice=true;
    village.activity_flags.at(activity)=0;village.activity_counts.erase(activity);
    const int item=village.rules->items.front().identity;
    village.shop_item_stock.at(item).quantity=1;village.item_commerce_read.at(item)=false;
    village.facility_presence.clear(); // 首商品命中，原OR不读设施。
    village.scripts.user_flags=0; // Init冻结11仍存在，绘制不按当前flag16重新筛选。
    options={};options.japanese=true;options.english=true;options.measured_text_widths=std::vector<int>{74,75,0};
    const auto plan=steam_navigation_menu_skin(village,vid,options);
    check(plan && images(*plan,147).size()==2 && images(*plan,147)[0].offset==std::array<int,2>{78,44} &&
          images(*plan,147)[1].offset==std::array<int,2>{78,72},"raw7商会与活动NEW独立，JP10，缓存11不重查当前flag16");
    village.scripts.activities.erase(activity);
    check(!steam_navigation_menu_skin(village,vid,options),"商会NEW不能遮蔽独立tag12缺活动源");
}
void system_menu_skin(Checks &check) {
    SteamSystemMenuSkinInput input;input.selection=4;
    SteamInformationMenuSkinOptions options;
    constexpr std::array<int,4> widths{0,29,59,89},heights{0,9,19,29};
    const auto images=[](const SteamInformationSkinPlan &plan,int image) {
        std::vector<StartupSkinDraw> result;
        for(const auto &draw:plan.draws)if(const auto *p=std::get_if<StartupSkinDraw>(&draw))
            if(p->image==image)result.push_back(*p);
        return result;
    };
    for(int frame=0;frame<4;++frame) {
        input.frame=frame;
        options.measured_text_widths=frame==3?std::optional<std::array<int,5>>{{80,81,0,80,81}}:std::nullopt;
        const auto plan=steam_system_menu_skin(input,options);
        check(plan && plan->raw==10 && plan->origin==options.origin && plan->touches.size()==6,
              "Steam raw10纯计划沿原五行及共尾KEYCLICK，不建立Owner系统页");
        const auto backgrounds=images(*plan,25),hands=images(*plan,70);
        check(backgrounds.size()==(frame?5U:0U) && hands.size()==(frame==3?1U:0U) &&
              plan->draws.size()==(frame==0?0U:frame==3?11U:5U) &&
              images(*plan,147).empty() && images(*plan,168).empty(),
              "raw10展开只画底图，稳定五文字一手形，无NEW或主菜单图标");
        for(int row=0;row<5;++row) {
            const auto &touch=plan->touches[row];
            check(touch.component==9 && touch.value==(0x20000|row) && touch.option==0 &&
                  touch.rectangle==std::array<int,4>{40,28*row,50,28},
                  "raw10使用ID9与原右侧热区，不误用主菜单ID8");
            if(frame)check(backgrounds[row].crop==std::array<int,4>{68,row==4?0:29,widths[frame],heights[frame]} &&
                  backgrounds[row].offset==std::array<int,2>{1,28*row+1},
                  "raw10复用type1源裁片缩短与选中底图");
            if(frame==3) {
                const auto &label=std::get<SteamInformationText>(plan->draws[2*row+1]);
                check(label.role==SteamInformationTextRole::menu_entry && label.argument==20+row &&
                      label.slot==row && label.position==std::array<int,2>{7,28*row+9} &&
                      label.rgb==(row==4?std::array<int,3>{76,58,50}:std::array<int,3>{255,242,220}) &&
                      label.font_size==((row==1||row==4)?11:0),
                      "Steam系统五tag20..24，末项不套APK28；严格实测宽阈值80");
            }
        }
        const auto &key=plan->touches.back();
        check(key.component==4 && key.value==22 && key.option==2 && !key.rectangle,
              "raw10尾KEYCLICK无矩形不扩成全屏退出按钮");
        if(frame<3) {
            options.measured_text_widths=std::array<int,5>{-1,-1,-1,-1,-1};
            check(steam_system_menu_skin(input,options).has_value(),"未稳定帧不提前消费未绘制测宽");
        }
    }
    for(const auto language:std::array<std::array<int,2>,4>{{{0,0},{0,1},{1,0},{1,1}}}) {
        options.japanese=language[0];options.english=language[1];
        options.measured_text_widths=std::array<int,5>{74,75,0,0,0};
        const auto plan=*steam_system_menu_skin(input,options);
        check(plan.touches[0].rectangle==std::array<int,4>{40,0,options.japanese?44:50,28} &&
              std::get<SteamInformationText>(plan.draws[1]).font_size==0 &&
              std::get<SteamInformationText>(plan.draws[3]).font_size==(options.japanese?11:0),
              "raw10日文84宽与74阈值、英语独立，不因菜单类别改变type1布局");
    }
    options={};options.measured_text_widths=std::array<int,5>{};
    options.origin={200,200};options.safe_left=10;
    const auto overflow=*steam_system_menu_skin(input,options);
    check(overflow.origin==options.origin && overflow.touches[0].rectangle==std::array<int,4>{-10,-105,50,28} &&
          images(overflow,70)[0].offset==std::array<int,2>{46,20},
          "raw10右底修正覆盖safe-left，保留原点及负局部坐标");
    options.origin={0,0};options.safe_left=120;
    const auto left=*steam_system_menu_skin(input,options);
    check(images(left,70)[0].sprite==21 && images(left,70)[0].offset==std::array<int,2>{118,125},
          "raw10手形按局部dx恰120转向，仍用独立当前SEB帧");
    options.on_top=false;
    check(images(*steam_system_menu_skin(input,options),70).empty() &&
          steam_system_menu_skin(input,options)->touches.size()==6,"菜单覆盖保留raw10触摸但不画手形");
    options.covered_by_nonmenu_subform=true;options.measured_text_widths.reset();
    const auto hidden=steam_system_menu_skin(input,options);
    check(hidden && hidden->draws.empty() && hidden->touches.empty(),"非菜单遮挡跳过raw10测宽与绘制");
    options={};options.measured_text_widths=std::array<int,5>{};
    for(int fault=0;fault<9;++fault) {
        auto bad=input;auto changed=options;
        if(fault==0)bad.frame=-1;if(fault==1)bad.frame=4;
        if(fault==2)bad.selection=-1;if(fault==3)bad.selection=5;
        if(fault==4)changed.measured_text_widths.reset();if(fault==5)(*changed.measured_text_widths)[4]=-1;
        if(fault==6)changed.canvas[1]=0;if(fault==7)changed.safe_left=-1;
        if(fault==8)changed.safe_left=std::numeric_limits<int>::max();
        check(!steam_system_menu_skin(bad,changed),"raw10非法帧/选择/测宽/画布/溢出显式拒绝");
    }
}
void information_menu_status_skin(Checks &check,const std::filesystem::path &root) {
    using NK=SteamInformationNumberKind;
    auto owner=test_support::page_fixture(9);
    owner.scripts.pages.back().lifecycle=0;
    const auto id=owner.scripts.pages.back().id;
    check(initialize_startup_world_information_pages(owner),"菜单状态栏使用真实raw9 Init");
    SteamInformationMenuSkinOptions options;
    options.origin={20,30};options.view_y=7;
    owner.village_points=345;
    const auto village=*steam_information_menu_status_skin(owner,id,options);
    check(village.draws.size()==2 && village.touches.empty() && village.origin==options.origin,
          "没有选中任务仍画村点，不制造四摘要或交互／声音");
    const auto &label=std::get<StartupSkinDraw>(village.draws[0]);
    const auto &point=std::get<SteamFacilityNumber>(village.draws[1]);
    check(label.image==84 && label.sprite==75 && label.frame==2 && label.offset==std::array<int,2>{163,-23} &&
          point.kind==SteamFacilityNumberKind::comma_number && point.asset==SteamFacilityAsset::number11 &&
          point.value==345 && point.position==std::array<int,2>{218,-18} && point.anchor==4,
          "村点HUD抵消真实origin，用VIEW_Y而非画布高度，SEB20保留Comma右锚");
    // 选中任务/名册均为表现条件；自然接受及遭遇quota由既有规则／projection主责。
    const auto definition=std::find_if(owner.rules->tasks.begin(),owner.rules->tasks.end(),[](const auto &t) {
        return t.factory.kind==1;
    });
    check(definition!=owner.rules->tasks.end(),"原目录含type1任务供摘要条件使用");
    ref::DungeonFinishTask task;task.identity=1;task.definition=definition->factory.identity;
    owner.tasks.emplace(1,task);owner.task_order={1};owner.active_task=1;owner.next_task_identity=2;
    owner.task_subperiods=5;
    constexpr std::array<int,4> animated_x{240,221,202,182};
    for(int count=0;count<=3;++count) {
        owner.page_counters.at(id)=count;
        const auto plan=steam_information_menu_status_skin(owner,id,options);
        check(plan && plan->draws.size()==(count<3?10U:25U),"HUD先完成，四摘要背景每阶段存在且内容只在3展开");
        const auto &background=std::get<StartupSkinDraw>(plan->draws[6]);
        check(background.image==86 && background.crop==std::array<int,4>{0,0,58,18} &&
              background.offset==std::array<int,2>{animated_x[count],36},
              "四摘要整数RateConvert朝零截断，不减origin或加入VIEW_Y");
    }
    const auto before=startup_world_state_digest(owner);
    const auto plan=*steam_information_menu_status_skin(owner,id,options);
    const auto &monster_number=std::get<SteamFacilityNumber>(plan.draws[2]);
    check(monster_number.asset==SteamFacilityAsset::number03_hud && monster_number.anchor==2 &&
          monster_number.position==std::array<int,2>{193,-20} &&
          std::get<StartupSkinDraw>(plan.draws[3]).image==84 &&
          std::get<StartupSkinDraw>(plan.draws[3]).frame==3,
          "非日文怪物HUD使用独立SEB10及正确普通数字锚／单位");
    const auto &push=std::get<SteamFacilityClip>(plan.draws[9]);
    const auto &set=std::get<SteamInformationImageClip>(plan.draws[10]);
    const auto &body=std::get<SteamInformationHumanBody>(plan.draws[11]);
    const auto &clear=std::get<SteamInformationImageClip>(plan.draws[12]);
    const auto &pop=std::get<SteamFacilityClip>(plan.draws[13]);
    check(push.kind==SteamFacilityClipKind::push_intersect && push.rectangle==std::array<int,4>{188,38,15,14} &&
          set.kind==SteamInformationImageClipKind::set && set.rectangle==push.rectangle &&
          body.position==std::array<int,2>{196,59} && body.body.sprite==1 && body.body.frame==0 &&
          body.body.image==owner.rules->jobs.at(owner.scene.world.world.ai.growth.at(0).definition.current_profession).sprites.at(0) &&
          clear.kind==SteamInformationImageClipKind::clear && pop.kind==SteamFacilityClipKind::pop,
          "空名单用定义0当前职业身体；普通clip／ImageClip／body／clear／pop完整原序");
    const auto &people=std::get<SteamInformationNumber>(plan.draws[14]);
    const auto &monsters=std::get<SteamInformationNumber>(plan.draws[17]);
    const auto &residence=std::get<StartupSkinDraw>(plan.draws[19]);
    const auto &time=std::get<SteamFacilityNumber>(plan.draws[23]);
    check(people.kind==NK::person_count && people.value==0 && people.position==std::array<int,2>{237,41} &&
          monsters.kind==NK::monster_count && monsters.value==0 && monsters.position==std::array<int,2>{237,59} &&
          residence.image==91 && residence.crop==std::array<int,4>{80,0,16,16} &&
          time.value==7 && time.position==std::array<int,2>{226,94} &&
          std::get<StartupSkinDraw>(plan.draws[24]).image==151,
          "四摘要数字分别全名单／type12／12减周期间，末覆盖框在期间数字之后");
    for(int repeat=0;repeat<5;++repeat) {
        const auto again=steam_information_menu_status_skin(owner,id,options);
        check(again && again->draws.size()==plan.draws.size() && again->touches.empty() &&
              startup_world_state_digest(owner)==before,"重复状态栏查询不增长Owner、缓存、随机或一次性输出");
    }
    options.japanese=true;
    const auto jp=*steam_information_menu_status_skin(owner,id,options);
    check(std::get<SteamFacilityNumber>(jp.draws[2]).position==std::array<int,2>{200,-20} &&
          std::get<StartupSkinDraw>(jp.draws[3]).image==85 && std::get<StartupSkinDraw>(jp.draws[3]).frame==1 &&
          std::get<SteamInformationNumber>(jp.draws[14]).japanese,
          "日文HUD独立数值位置／怪物单位，count3/4同步保留语言参数");
    for(const auto kind:{NK::person_count,NK::monster_count}) {
        const auto digits=steam_information_number_draws({kind,1234,{100,7},true});
        check(digits && digits->size()==6 && digits->front().offset==std::array<int,2>{58,7} &&
              digits->at(2).frame==10 && digits->at(2).offset==std::array<int,2>{64,7} &&
              digits->back().sprite==76 && digits->back().frame==(kind==NK::person_count?4:1) &&
              digits->back().offset==std::array<int,2>{90,7},"日文count3/4先减10再Comma，数字先于逗号，各自单位最后");
        const auto zero=steam_information_number_draws({kind,0,{100,7},false});
        check(zero && zero->size()==1 && zero->front().frame==0 &&
              zero->front().offset==std::array<int,2>{92,7},"非日文空名单仍显示0，不附单位或按库存拒零");
        check(!steam_information_number_draws({kind,-1,{100,7},false}) &&
              !steam_information_number_draws({kind,0,{std::numeric_limits<int>::min(),7},true}),
              "名单数量负值及日文定位溢出显式拒绝");
    }
    const auto type0=std::find_if(owner.rules->tasks.begin(),owner.rules->tasks.end(),[](const auto &t) {
        return t.factory.kind==0;
    });
    check(type0!=owner.rules->tasks.end(),"原目录含type0任务");
    owner.tasks.at(1).definition=type0->factory.identity;
    const auto dungeon=*steam_information_menu_status_skin(owner,id,options);
    check(std::get<StartupSkinDraw>(dungeon.draws[6]).offset==std::array<int,2>{182,18} &&
          std::get<StartupSkinDraw>(dungeon.draws[22]).crop==std::array<int,4>{16,0,16,16},
          "type0摘要始于18并取quest格1，不错误套type1位置或图标");
    options.covered_by_nonmenu_subform=true;
    check(steam_information_menu_status_skin(owner,id,options)->draws.empty(),"非菜单遮挡使HUD摘要也无绘制／触摸");
    options.covered_by_nonmenu_subform=false;
    for(int fault=0;fault<5;++fault) {
        auto bad=owner;auto input=options;
        if(fault==0)bad.page_counters.erase(id);
        if(fault==1)bad.tasks.erase(1);
        if(fault==2)input.canvas[0]=0;
        if(fault==3)input.origin[0]=std::numeric_limits<int>::min();
        if(fault==4){input.view_y=std::numeric_limits<int>::max();input.origin[1]=-1;}
        const auto digest=startup_world_state_digest(bad);
        check(!steam_information_menu_status_skin(bad,id,input) && startup_world_state_digest(bad)==digest,
              "坏菜单／已退休任务／画布及坐标算术显式拒绝，不留下状态");
    }
    for(const auto file:{"menuRT00.png","menuRT04.png"}) {
        const auto path=root.parent_path()/"steam-common"/file;
        auto image=LoadImage(path.string().c_str());
        check(image.data && image.width==(std::string(file)=="menuRT00.png"?59:11) &&
              image.height==(std::string(file)=="menuRT00.png"?45:11),"新Steam状态栏PNG真实解码和尺寸通过");
        UnloadImage(image);
    }
    const auto seb=tools::parse_legacy_seb(read_bytes(root.parent_path()/"steam-common/menuRT00.seb"));
    check(seb.frame_count==4 && seb.layers.size()==1 && seb.layers[0].parts.size()==4,
          "Steam SEB75真实四帧身份，不能借APK版SEB");
    for(const auto &part:seb.layers[0].parts)
        check(part.image_index==84 && part.source_x>=0 && part.source_y>=0 &&
              part.source_x+part.width<=59 && part.source_y+part.height<=45,"状态栏SEB75裁片在新PNG内");
}
} // namespace

// 同一visuals套件集中调用；返回检查数，失败抛具名诊断，由主入口统一收口。
int check_startup_skin(const std::filesystem::path &source_root,
                       const std::filesystem::path &optional_output_png) {
    Checks check;
    const auto assets=tables(source_root,check);
    phases(check);
    steam_clear_score_skin_checks(check);
    monster_introduction_skin(check,source_root);
    monster_detail_skin(check,source_root);
    monster_notice_skin(check,source_root);
    steam_new_game_skin_checks(check);
    steam_config_skin_checks(check);
    steam_commerce_skin_checks(check,source_root);
    steam_commerce_result_skin_checks(check);
    steam_commerce_item_skin_checks(check);
    steam_commerce_facility_skin_checks(check,source_root);
    steam_platform_skin_checks(check,source_root);
    steam_joystick_prompt_checks(check);
    steam_title_actor_projection_checks(check);
    steam_language_checks(check,source_root);
    steam_cursor_checks(check,source_root);
    frame_geometry(check);
    income_information(check);
    income_skin(check,source_root);
    item_information(check);
    equipment_information(check);
    equipment_information_icons(check,source_root);
    information_directory_skin(check,source_root);
    adventurer_information_skin(check,source_root);
    tax_information_skin(check);
    town_facility_information_skin(check,source_root);
    information_menu_skin(check,source_root);
    system_menu_skin(check);
    navigation_menu_skin(check);
    information_menu_status_skin(check,source_root);
    if(optional_output_png.empty()) {
        static_images(assets,check,nullptr);
        sprite_pixels(assets,check,nullptr);
        frame_pixels(assets,check,nullptr);
        title_actor_pixels(source_root,check,nullptr);
    } else {
        check(optional_output_png.extension()==".png" && !optional_output_png.filename().empty(),
              "可选CPU素材输出是明确PNG路径");
        CpuImage sheet(GenImageColor(760,1230,{239,239,221,255}));
        static_images(assets,check,&sheet.image);
        sprite_pixels(assets,check,&sheet.image);
        frame_pixels(assets,check,&sheet.image);
        title_actor_pixels(source_root,check,&sheet.image);
        if(!optional_output_png.parent_path().empty())
            std::filesystem::create_directories(optional_output_png.parent_path());
        check(ExportImage(sheet.image,optional_output_png.string().c_str()),"导出CPU静态参考素材拼图");
    }
    return check.count + check_steam_startup_skin(source_root) + check_steam_facility_skin(source_root) +
           check_steam_human_skin(source_root);
}
