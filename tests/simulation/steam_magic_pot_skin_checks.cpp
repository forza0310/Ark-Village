#include "ark/simulation/presentation/steam_magic_pot_effect_skin.hpp"
#include "ark/simulation/facilities/startup_world_magic_pot.hpp"
#include "ark/simulation/persistence/startup_world_persistence.hpp"
#include "ark/assets/sprite.hpp"
#include "support/world_fixture.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

using namespace ark::simulation;
namespace {
struct Checks {
    int count{};
    void operator()(bool ok,const char *why) {++count;if(!ok)throw std::runtime_error(std::string("Steam魔法壶皮肤：")+why);}
};
template<class T> std::vector<T> parts(const SteamMagicPotPlan &plan) {
    std::vector<T> result;
    for(const auto &operation:plan.operations)if(const auto *part=std::get_if<T>(&operation))result.push_back(*part);
    return result;
}
SteamMagicPotOptions options() {
    SteamMagicPotOptions o;o.title_widths=std::array<int,2>{60,62};
    o.button_widths={48,72};o.hint_width=240;return o;
}
SteamMagicPotEffectOptions effect_options() {
    SteamMagicPotEffectOptions o;o.common=options();o.report_header_widths=std::array<int,4>{6,22,22,22};
    o.comment_width=36;o.reward_name_width=48;o.confirm_width=24;return o;
}
std::vector<StartupVisualDraw> item_icon() {
    StartupVisualDraw background;background.image=24;background.crop={0,0,18,18};background.offset={-1,-1};
    StartupVisualDraw item;item.image=9;item.crop={0,0,16,16};return {background,item};
}
SteamMagicPotSkinInput input(int raw) {
    // 仅显示组合夹具；定义身份和数值不代表自然解锁或已消费库存。
    SteamMagicPotSkinInput in;in.raw=raw;in.options=options();in.parameters[11]=2;
    in.parameters[0]=37;in.parameters[1]=3;in.parameters[3]=2;in.parameters[4]=4;
    for(int i=0;i<7;++i) {
        in.items.push_back({i,2,"道具条件",item_icon(),{0,4,5,10}});
        in.recipes.push_back({i,0,i,2,true,false,false,"配方条件",item_icon(),{1,5,0,0}});
    }
    return in;
}
SteamMagicPotEffectInput effect(int raw) {
    SteamMagicPotEffectInput in;in.raw=raw;in.level=2;in.profession=0;in.processed=2;
    in.options=effect_options();in.item_icon=item_icon();in.reward_icon=item_icon();
    in.comment="感觉不错";in.reward_name="结果道具";in.costs={1,5,0,0};in.elements={2,4,0,0};
    in.attributes[0]={10,20,30,999,40};in.attributes[1]={11,22,30,999,41};in.attributes[2]={1,2,0,4,1};
    return in;
}
void balanced(Checks &c,const SteamMagicPotPlan &plan) {
    int depth{};
    for(const auto &operation:plan.operations)if(const auto *clip=std::get_if<SteamMagicPotClip>(&operation)) {
        if(clip->kind==SteamMagicPotClipKind::push_intersect)++depth;
        else if(clip->kind==SteamMagicPotClipKind::replace) {if(!depth)++depth;}
        else {c(depth>0,"clip不可下溢");--depth;}
    }
    c(depth==0,"整个有序计划消费后clip归零");
}
void body_and_directory(Checks &c) {
    // UserData.cctor五组坐标＋Draw_magicPot独立源oracle，非由被测函数推期望。
    const auto body=steam_magic_pot_body_skin(2,10,{21,88},0);c(body.has_value(),"二级壶全景计划");
    const auto images=parts<SteamMagicPotImage>(*body);
    c(images.size()==7&&images[0].asset==SteamMagicPotAsset::background&&images[0].position==std::array<int,2>{21,88}&&
      images[1].asset==SteamMagicPotAsset::ladder&&images[1].frame==1&&images[1].position==std::array<int,2>{64,158}&&
      images[2].asset==SteamMagicPotAsset::character&&images[2].frame==1&&images[2].position==std::array<int,2>{65,134}&&
      images[3].asset==SteamMagicPotAsset::pot&&images[3].position==std::array<int,2>{78,165}&&
      images[4].asset==SteamMagicPotAsset::bar&&images[4].frame==3&&images[4].position==std::array<int,2>{73,121}&&
      images[6].asset==SteamMagicPotAsset::fire&&images[6].frame==1&&images[6].position==std::array<int,2>{78,169},
      "level2 frame10原资源层序、独立坐标及SEB帧");
    c(std::get<SteamMagicPotClip>(body->operations[5]).rectangle==std::array<int,4>{21,137,93,33}&&
      std::get<SteamMagicPotImage>(body->operations[6]).asset==SteamMagicPotAsset::pot&&
      std::get<SteamMagicPotClip>(body->operations[7]).kind==SteamMagicPotClipKind::pop,
      "二级壶遮挡clip包住第二次pot且在fire前释放");balanced(c,*body);
    auto in=input(41);auto plan=steam_magic_pot_skin(in);c(plan.has_value(),"41原窗口和两个选项");
    auto touch=parts<SteamStartupTouch>(*plan);
    c(touch.size()==2&&touch[0].component==3&&touch[0].rectangle==std::optional<std::array<int,4>>({22,154,104,56})&&
      touch[1].rectangle==std::optional<std::array<int,4>>({108,154,116,56}),"41测宽夹60后真实按钮热区");
    auto text=parts<SteamMagicPotText>(*plan);
    c(text.back().rectangle==std::array<int,4>{12,197,216,16}&&text.back().anchor==34,
      "41说明测宽夹200且保留TextLayout锚点");
    const auto facility=parts<SteamFacilityImage>(*plan);
    c(std::any_of(facility.begin(),facility.end(),[](const auto &v){return v.asset==SteamFacilityAsset::level_label&&!v.crop;}),
      "level_label是正式wnd_lv整图，不造SEB或裁框");
    in.options.hint_width.reset();c(!steam_magic_pot_skin(in),"41缺真实测宽拒绝");

    in=input(42);in.selection=in.first=6;plan=steam_magic_pot_skin(in);c(plan.has_value(),"42合法一行尾窗");
    auto rows=parts<SteamFacilityEffectTouch>(*plan);
    c(rows.size()==1&&rows[0].touch.option==16&&rows[0].touch.value==(0x20000|6)&&
      rows[0].touch.rectangle==std::optional<std::array<int,4>>({23,95,191,16})&&rows[0].margin==std::array<int,4>{29,0,0,0},
      "42保留绝对行号、marker16和Steam右侧margin");
    text=parts<SteamMagicPotText>(*plan);
    c(text.back().value=="道具条件"&&text.back().rectangle==std::array<int,4>{50,95,130,15}&&text.back().anchor==32,
      "42行名实际TextLayout及源锚点");
    auto bars=parts<SteamHumanCatalogueScroll>(*plan);
    c(bars.size()==1&&bars[0].touch.rectangle==std::optional<std::array<int,4>>({221,85,3,110})&&
      bars[0].arguments==std::array<int,3>{7,5,0x20000},"42原110高轨道与真实目录数");
    for(const auto language:{0,1,2}) {
        in.options.japanese=language==1;in.options.english=language==2;
        plan=steam_magic_pot_skin(in);c(plan.has_value(),"42三语言分支合法");
        text=parts<SteamMagicPotText>(*plan);const auto numbers=parts<SteamFacilityNumber>(*plan);
        c(text[1].value==(language==2?"素材 Left":"素材")&&text[1].rectangle[0]==(language==2?77:85)&&
          text[1].rectangle[1]==(language==1?69:70)&&numbers[0].position==std::array<int,2>{language==1?126:180,70},
          "JP／en-US／其它非JP素材与数值锚点分开");
    }
    in.items[6].icon[1].crop={239,0,16,16};c(!steam_magic_pot_skin(in),"42越界PNG裁片显式拒绝");
    in=input(42);in.selection=0;in.first=1;c(!steam_magic_pot_skin(in),"不可见选择不偷偷改首行");

    in=input(43);in.recipes[0].discovered=false;in.recipes[0].icon.clear();in.selection=in.first=0;
    plan=steam_magic_pot_skin(in);c(plan.has_value(),"未知配方不要求可见图标");
    rows=parts<SteamFacilityEffectTouch>(*plan);
    c(rows.size()==5&&rows[0].touch.option==0&&rows[0].touch.rectangle==std::optional<std::array<int,4>>({-1,95,231,16})&&
      rows[0].margin==std::array<int,4>{0,-20,0,0},"43无marker且margin不沿42");
    text=parts<SteamMagicPotText>(*plan);
    c(std::count_if(text.begin(),text.end(),[](const auto &v){return v.value=="？？？？？";})==1&&
      std::count_if(text.begin(),text.end(),[](const auto &v){return v.value=="配方条件";})==4,
      "未知行只显示问号，已知行显示真实结果名");
    c(std::any_of(text.begin(),text.end(),[](const auto &v){return v.value=="配方条件"&&v.rgb==std::array<int,3>{255,14,1};}),
      "不可制作且道具库存2仍红字，不能把inventory==1放宽成正库存");
    in.recipes[1].inventory=1;plan=steam_magic_pot_skin(in);text=parts<SteamMagicPotText>(*plan);
    c(std::any_of(text.begin(),text.end(),[](const auto &v){return v.value=="配方条件"&&v.rectangle[1]==116&&v.rgb==std::array<int,3>{92,51,31};}),
      "恰一库存满足43源绘制颜色分支");
    in.tab=1;in.first=in.selection=6;plan=steam_magic_pot_skin(in);c(plan.has_value(),"43第二页保持实际短尾首行");
    const auto costs=parts<SteamFacilityNumber>(*plan);
    c(costs.size()==8&&costs[0].value==1&&costs[0].asset==SteamFacilityAsset::number08&&costs[0].position==std::array<int,2>{123,98}&&
      costs[1].value==5&&costs[1].asset==SteamFacilityAsset::number12&&costs[1].position==std::array<int,2>{151,98},
      "43成本足够蓝number08、不足number12；尾窗仍从原首绘行开始");
}
int number_at(const SteamMagicPotPlan &p,int x,int y) {
    for(const auto &v:parts<SteamFacilityNumber>(p))
        if(v.kind==SteamFacilityNumberKind::number&&v.position==std::array<int,2>{x,y})return v.value;
    throw std::runtime_error("Steam魔法壶皮肤：源锚点缺数值");
}
void effects(Checks &c) {
    auto in=effect(44);in.frame=35;in.frame2=7;
    auto p=steam_magic_pot_effect_skin(in);c(p.has_value(),"44双钟演出计划");
    auto images=parts<SteamMagicPotImage>(*p);
    c(images[0].position==std::array<int,2>{30,66}&&images[0].crop==std::optional<std::array<int,4>>({0,0,93,82})&&
      images[1].position==std::array<int,2>{109,66}&&images[1].crop==std::optional<std::array<int,4>>({1,0,91,82})&&
      images[2].position==std::array<int,2>{189,66}&&images[2].crop==std::optional<std::array<int,4>>({1,0,20,82}),
      "44原背景三裁片按序铺开，不猜缩放");
    c(std::any_of(images.begin(),images.end(),[](const auto &v){return v.asset==SteamMagicPotAsset::fire&&v.frame==1&&v.position==std::array<int,2>{120,147};}),
      "44壶火使用frame2=7，不从主frame35反推");
    auto flash=parts<SteamFacilityEffectImage>(*p);
    c(flash.size()==1&&flash[0].asset==SteamFacilityEffectAsset::throw_flash&&flash[0].frame==0&&flash[0].position==std::array<int,2>{120,116},
      "44恰35原闪光位置和SEB6首帧");
    in.frame=36;p=steam_magic_pot_effect_skin(in);images=parts<SteamMagicPotImage>(*p);
    c(std::any_of(images.begin(),images.end(),[](const auto &v){return v.asset==SteamMagicPotAsset::fire&&v.frame==1;}),
      "主36变化不改变冻结副钟绘制帧");
    for(const auto step:std::array<std::array<int,2>,5>{{{34,-1},{39,1},{43,2},{46,2},{47,-1}}}) {
        in.frame=step[0];p=steam_magic_pot_effect_skin(in);c(p.has_value(),"44闪光边界计划");
        flash=parts<SteamFacilityEffectImage>(*p);
        c(step[1]<0?flash.empty():flash.size()==1&&flash[0].frame==step[1],"35至46每四槽闪光，不越界多播");
    }
    for(const auto frame:{70,84,85}) {
        in.frame=frame;p=steam_magic_pot_effect_skin(in);c(p.has_value(),"44数值过渡计划");
        c(number_at(*p,86,166)==(frame==85?11:10),"44 delta1 countAnime在终段前保持before，85才显示after");
        const auto values=parts<SteamFacilityNumber>(*p);
        c(std::any_of(values.begin(),values.end(),[](const auto &v){return v.kind==SteamFacilityNumberKind::plus_value&&v.value==4&&v.position==std::array<int,2>{216,189};}),
          "饱和后显示请求量4，不用实际差额0抹去加值");
    }
    for(const auto frame:{96,97,99}) {
        in.frame=frame;p=steam_magic_pot_effect_skin(in);c(p.has_value(),"44评语门槛计划");
        const auto text=parts<SteamMagicPotText>(*p);
        c(std::any_of(text.begin(),text.end(),[](const auto &v){return v.value=="感觉不错";})==(frame>=97),"97起才出现已抽评语");
        const auto touches=parts<SteamStartupTouch>(*p);
        c(touches.size()==1&&touches[0].component==2&&touches[0].option==2&&!touches[0].rectangle,
          "44无矩形确认组件不能由皮肤伪造全屏命中");
    }
    in.level=3;p=steam_magic_pot_effect_skin(in);c(p.has_value(),"三级44遮挡计划");
    const auto clips=parts<SteamMagicPotClip>(*p);
    c(clips.size()==2&&clips[0].rectangle==std::array<int,4>{97,97,46,51},"三级44独立壶前景clip");balanced(c,*p);
    in.options.comment_width.reset();c(!steam_magic_pot_effect_skin(in),"97后缺评语测宽拒绝");
    in.frame=96;c(steam_magic_pot_effect_skin(in).has_value(),"97前不提前要求尚未绘制评语测宽");

    in=effect(45);
    for(const auto frame:{62,76,77,82,83}) {
        in.frame=frame;p=steam_magic_pot_effect_skin(in);c(p.has_value(),"45结果计划");
        c(p->soft_labels==std::array<int,2>{0,0},"45没有返回消费者，不能绘制无效返回软标签");
        c(number_at(*p,178,98)==(frame<77?10:11),"45 delta1保持before至77，Steam当前值right178");
        const auto cursors=parts<SteamHumanImage>(*p);
        c(std::any_of(cursors.begin(),cursors.end(),[](const auto &v){return v.asset==SteamHumanAsset::continue_cursor&&v.position==std::array<int,2>{208,187};})==(frame>=83),
          "45恰83出现继续箭头，77快进阶段仍无箭头");balanced(c,*p);
    }
    in.options.report_header_widths.reset();c(!steam_magic_pot_effect_skin(in),"非JP45必须有实际标题测宽");
    in.options.common.japanese=true;p=steam_magic_pot_effect_skin(in);c(p.has_value(),"JP45沿固定布局不借非JP测宽");
    c(number_at(*p,120,71)==2,"JP45消费数原120,71居中锚点");
    in=effect(47);in.frame=6;p=steam_magic_pot_effect_skin(in);c(p.has_value(),"47计数6只读呈现不足结果");
    auto text=parts<SteamMagicPotText>(*p);
    c(std::any_of(text.begin(),text.end(),[](const auto &v){return v.value=="魔法点数不足"&&v.rectangle[1]==113;}),"47不足文案不隐式扣元素");
    auto controls=parts<SteamStartupTouch>(*p);
    c(controls.size()==1&&controls[0].component==4&&controls[0].value==20&&
      controls[0].rectangle==std::optional<std::array<int,4>>({80,161,80,56}),"46/47实际测宽按钮Touch4/20");
    in.elements[1]=5;in.frame=7;p=steam_magic_pot_effect_skin(in);c(p.has_value(),"47计数7可制作显示");
    text=parts<SteamMagicPotText>(*p);
    c(text.back().value=="制作!"&&text.back().rectangle==std::array<int,4>{100,181,40,16},"47满足时按钮制作仍仅绘图");
    in.raw=46;in.elements={0,0,0,0};p=steam_magic_pot_effect_skin(in);c(p.has_value(),"46发现页不借47资金资格隐藏");
    text=parts<SteamMagicPotText>(*p);
    c(std::any_of(text.begin(),text.end(),[](const auto &v){return v.value=="可以制作了!";})&&text.back().value=="了解",
      "46发现说明与47炼制问句分离");
}

std::uint64_t active_id(const StartupWorldRuntimeState &s) {
    const auto p=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),[](const auto &v){return v.lifecycle!=4;});
    if(p==s.scripts.pages.rend())throw std::runtime_error("魔法壶皮肤夹具没有活页");return p->id;
}
void init(Checks &c,StartupWorldRuntimeState &s) {
    c(initialize_startup_world_magic_pot_pages(s),"显示夹具经过真实Owner初始化");
    const auto id=active_id(s);
    for(auto &p:s.scripts.pages)if(p.id==id)p.lifecycle=2;
    s.scripts.executing_page.reset();
}
void owner(Checks &c) {
    using E=StartupWorldRuntimeError;using A=StartupMagicPotAction;
    auto root=test_support::page_fixture(41);root.scripts.pages.back().lifecycle=0;
    root.scripts.event_calls[101]=1;root.items.at(0).inventory=2;root.catalog.at({0,0})=root.items.at(0);
    const auto root_id=active_id(root);
    c(!steam_magic_pot_skin(root,root_id,options()),"未Init查询不代初始化Owner");init(c,root);
    auto deposits=root;c(act_startup_world_magic_pot_page(deposits,root_id,A::confirm)==E::none,"真实41打开42");init(c,deposits);
    auto thrown=deposits;c(act_startup_world_magic_pot_page(thrown,active_id(thrown),A::confirm)==E::none,"真实42投入冻结结果并打开44");init(c,thrown);
    auto recipes=root;c(act_startup_world_magic_pot_page(recipes,root_id,A::select,1)==E::none&&
      act_startup_world_magic_pot_page(recipes,root_id,A::confirm)==E::none,"真实41打开43");init(c,recipes);
    auto question=recipes;question.magic_pot_recipes.at(1).status=1;
    c(act_startup_world_magic_pot_page(question,active_id(question),A::confirm)==E::none,"条件已发现配方打开47");init(c,question);
    auto report=test_support::page_fixture(45);report.scripts.pages.back().lifecycle=0;init(c,report);
    auto discovery=test_support::page_fixture(46);discovery.scripts.pages.back().lifecycle=0;discovery.scripts.pages.back().legacy_g=1;init(c,discovery);
    for(auto *s:{&root,&deposits,&recipes,&thrown,&report,&discovery,&question}) {
        const auto id=active_id(*s);const int raw=s->scripts.pages.back().legacy_page;
        const auto query=[&](const auto &state){return raw<=43?steam_magic_pot_skin(state,id,options()):steam_magic_pot_effect_skin(state,id,effect_options());};
        const auto before=startup_world_state_digest(*s);const auto sounds=s->sound_requests;const auto draws=s->scene.random.draws();
        const auto p=query(*s);c(p.has_value(),"41至47完整Owner只读桥");balanced(c,*p);
        for(int repeat=0;repeat<8;++repeat) {
            const auto again=query(*s);
            c(again&&again->operations.size()==p->operations.size()&&startup_world_state_digest(*s)==before&&
              s->sound_requests==sounds&&s->scene.random.draws()==draws,"重复八次查询规模稳定且不消费声音、随机、库存或页面");
        }
        auto bad=*s;bad.magic_pot_page_data.erase(id);c(!query(bad),"坏已初始化载荷返回空，不以map.at异常掩盖");
        bad=*s;bad.scripts.pages.back().lifecycle=4;c(!query(bad),"退休页面不可留下绘制消费者");
        if(raw==44) {bad=*s;bad.page_secondary_counters.erase(id);c(!query(bad),"44缺独立副钟拒绝绘制");}
    }
}

std::vector<std::uint8_t> bytes(const std::filesystem::path &path) {
    std::ifstream stream(path,std::ios::binary);if(!stream)throw std::runtime_error("魔法壶正式资源缺失："+path.string());
    return {std::istreambuf_iterator<char>(stream),{}};
}
void resources(Checks &c,const std::filesystem::path &source_root) {
    const auto root=source_root;
    const std::array<SteamMagicPotAsset,7> assets{SteamMagicPotAsset::background,SteamMagicPotAsset::character,
        SteamMagicPotAsset::pot,SteamMagicPotAsset::bar,SteamMagicPotAsset::fire,SteamMagicPotAsset::ladder,SteamMagicPotAsset::element};
    constexpr std::array<int,7> images{39,40,41,42,43,44,118},sprites{-1,47,48,49,50,51,-1},frames{0,2,3,4,4,3,0};
    for(std::size_t i=0;i<assets.size();++i) {
        const auto r=steam_magic_pot_resource(assets[i]);
        c(r&&r->image==images[i]&&r->sprite==sprites[i]&&std::filesystem::is_regular_file(root/std::filesystem::path(r->published_image).lexically_relative("original")),"七种资源正式路径与原索引");
        if(r->published_sprite) {
            const auto seb=ark::assets::parse_legacy_seb(bytes(root/std::filesystem::path(r->published_sprite).lexically_relative("original")));
            c(seb.frame_count==frames[i],"SEB真实帧数与源动作一致，不按PNG切猜帧");
        } else c(sprites[i]==-1,"无SEB资源只用整图或已证裁片");
    }
}
} // namespace

// 复用visuals的来源资源生命周期；业务扣费/门槛及事务矩阵由pages主责，不重复建立target。
int check_steam_magic_pot_skins(const std::filesystem::path &source_root) {
    Checks c;body_and_directory(c);effects(c);owner(c);resources(c,source_root);return c.count;
}
