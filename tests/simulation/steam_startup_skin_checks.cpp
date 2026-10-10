#include "ark/simulation/presentation/steam_startup_skin.hpp"
#include "ark/assets/sprite.hpp"
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
    return ark::assets::parse_legacy_seb({std::istreambuf_iterator<char>(input),{}});
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
    return check.count;
}
