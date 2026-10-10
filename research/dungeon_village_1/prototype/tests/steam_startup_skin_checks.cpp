#include "dungeon_village_prototype/steam_startup_skin.hpp"
#include "dungeon_village_prototype/steam_main_menu_skin.hpp"
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
    return check.count;
}
