#include "dungeon_village_prototype/steam_startup_skin.hpp"
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
template<class T> std::vector<T> parts(const SteamStartupSkinPlan &p) {
    std::vector<T> result;
    for(const auto &draw:p.draws) if(const auto *item=std::get_if<T>(&draw)) result.push_back(*item);
    return result;
}
auto sprite(const std::filesystem::path &p) {
    std::ifstream input(p,std::ios::binary);
    if(!input) throw std::runtime_error("Steam皮肤公共资源缺失："+p.string());
    return dungeon_village_tools::parse_legacy_seb({std::istreambuf_iterator<char>(input),{}});
}
}
// 挂既有visuals套件：局部合同与源资源oracle，不建立新target或窗口状态机。
int check_steam_startup_skin(const std::filesystem::path &root) {
    Checks check;
    using Asset=SteamStartupAsset;
    using Role=SteamStartupTextRole;
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
          dialog_texts[0].line_space==6 && dialog_texts[1].anchor==2,
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
    return check.count;
}
