// Steam DrawConfig_en RVA305F00；窗口返回坐标与实际窗框偏移必须分别消费。
#include "dungeon_village_prototype/steam_config_skin.hpp"
#include <algorithm>

namespace dungeon_village_prototype {
namespace {
using Plan=SteamConfigSkinPlan;
constexpr std::array<int,3> ink{30,30,30}, selected{159,189,255}, disabled{128,128,128};
bool valid_text(const std::string &s) { return !s.empty()&&s.size()<=4096; }
void rectangle(Plan &p,std::array<int,4> r,std::array<int,3> rgb) {
    p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{r,rgb,false}});
}
void text(Plan &p,const std::string &value,std::array<int,4> r,int anchor=0,
          std::array<int,3> rgb=ink,bool layout=false) {
    p.operations.emplace_back(SteamHumanOverviewText{
        SteamHumanOverviewTextRole::equipment_name,value,r,anchor,0,layout,rgb});
}
void touch(Plan &p,int component,int value,std::array<int,4> source,
           std::optional<int> extra={},std::optional<std::array<int,6>> drag={},
           std::optional<std::array<int,4>> registered={}) {
    p.operations.emplace_back(SteamConfigTouch{
        {component,value,registered?*registered:source,{},drag?4:0},source,extra,drag});
}
}
std::optional<SteamConfigImagePlan> steam_config_image_skin(const SteamConfigImage &in,
                                                          int image_width) {
    // IMAGE_DATA行只取本页实际消费者；128为源图基宽，0.75是独立逻辑缩放。
    // 8192为维护图像预算，不改变原表或把密度误作界面缩放。
    if(image_width<=0||image_width>8192)return {};
    std::array<int,6> row{};
    switch(in.image) {
    case 4:row={-11,-11,1,85,22,21};break;
    case 5:row={-11,-11,24,85,22,21};break;
    case 6:row={-14,-14,47,85,28,28};break;
    case 7:row={-14,-14,76,85,28,28};break;
    case 11:row={-2,-13,73,57,9,23};break;
    default:return {};
    }
    const float density=float(image_width)/128.F;
    SteamConfigImagePlan out;
    for(int axis=0;axis<4;++axis)out.crop[axis]=int(float(row[axis+2])*density+0.5F);
    if(out.crop[2]<=0||out.crop[3]<=0||out.crop[0]+out.crop[2]>image_width)return {};
    out.destination={float(in.anchor[0])+float(row[0])*0.75F,
                     float(in.anchor[1])+float(row[1])*0.75F,
                     float(row[4])*0.75F,float(row[5])*0.75F};
    return out;
}
std::optional<SteamConfigSkinPlan> steam_config_skin(const SteamConfigSkinInput &in,
                                                    const SteamConfigSkinOptions &o) {
    if(in.selection<0||in.selection>=4||o.view_y< -65536||o.view_y>65536||
       std::any_of(o.graphics_origin.begin(),o.graphics_origin.end(),
                   [](int v){return v< -65536||v>65536;})||
       !valid_text(o.text.title)||!o.text.title_widths||
       std::any_of(o.text.title_widths->begin(),o.text.title_widths->end(),
                   [](int w){return w<0||w>4096;}))return {};
    for(int row=0;row<4;++row)
        if(in.values[row]<0||in.values[row]>(row<2?1:5)||!valid_text(o.text.names[row]))return {};
    for(const auto &choices:o.text.choices)for(const auto &s:choices)if(!valid_text(s))return {};
    for(const auto &s:o.text.explanation)if(s.size()>4096)return {};
    // DrawWindow2: AppData窗口off_y=-12；返回坐标top43/left15，与VIEW_Y无关。
    // BOX先减12后才由底层DrawBox加VIEW_Y/2；正文不能再加这次偏移。
    const auto window=steam_startup_window_skin(
        {210,186,-12,0,SteamStartupTextRole::message_title,0},o.view_y,o.text.title_widths);
    const auto box=steam_startup_box_skin({19,35,221,162},o.view_y);
    if(!window||!box)return {};
    Plan p;p.title=o.text.title;
    for(const auto &op:window->draws)p.operations.emplace_back(op);
    for(const auto &op:box->draws)p.operations.emplace_back(op);
    for(int row=0;row<4;++row) {
        const int y=57+24*row,key=0x20000|row,type=0x60000+row*0x10000;
        if(row==in.selection)rectangle(p,{28,y-1,56,14},selected);
        touch(p,6,key,{-120,y-6,179,24});
        // SE名称原TextLayout(40×11,anchor32)，其它名称是普通DrawString。
        text(p,o.text.names[row],{29,y,row==3?40:0,row==3?11:0},row==3?32:0,ink,row==3);
        if(row<2) {
            const int display=(in.values[row]+1)%2;
            for(int choice=0;choice<2;++choice) {
                // GetEvenlyDivisionArea(94,206,2,choice,2)返回宽55，整数中点121/178。
                const int center=121+57*choice;
                if(choice==display)rectangle(p,{center-27,y-1,54,14},selected);
                const bool blocked=row==1&&choice==0&&!in.speed_unlocked;
                text(p,o.text.choices[row][choice],{center,y,0,0},2,blocked?disabled:ink);
                if(!blocked)touch(p,6,type|(1-choice),
                    choice==0?std::array<int,4>{59,y-6,92,24}:
                              std::array<int,4>{149,y-6,240,24},key);
            }
        } else {
            // 数字原普通字体long；图像和轨道由_addTouch即时绘制，随后才登记组件。
            text(p,std::to_string(in.values[row]),{82,y,0,0},4);
            const int volume_row=row-2;
            p.operations.emplace_back(SteamConfigImage{o.arrow_selected[volume_row][0]?6:4,{99,y+6}});
            touch(p,21,key,{59,y-6,55,24},0x10010);
            p.operations.emplace_back(SteamConfigImage{o.arrow_selected[volume_row][1]?7:5,{201,y+6}});
            touch(p,20,key,{186,y-6,240,24},0x10012);
            rectangle(p,{120,y+4,60,4},ink);
            rectangle(p,{119,y+5,62,2},ink);
            const int thumb=119+12*in.values[row]; // (62-2)/(6-1)，先整除再乘。
            p.operations.emplace_back(SteamConfigImage{11,{thumb,y+6}});
            touch(p,19,type,{119,y-6,62,24},key,
                std::array<int,6>{119+o.graphics_origin[0],o.graphics_origin[1],62,0,6,0},
                std::array<int,4>{thumb-15,y-6,32,24});
        }
    }
    text(p,o.text.explanation[0],{34,178,0,0},17);
    // 速度页第二行始终红色限框，资格只改选项；不擅自按解锁条件隐藏说明。
    if(in.selection==1)text(p,o.text.explanation[1],{34,196,180,11},32,{200,0,0},true);
    else text(p,o.text.explanation[1],{34,196,0,0},17);
    if(o.steam_platform&&o.on_top)p.operations.emplace_back(SteamConfigPlatformKeyboard{{21,51},2,o.platform_keyboard_touched});
    return p;
}
std::optional<SteamConfigUpdatePlan> steam_config_english_update(const SteamConfigUpdateInput &in) {
    if(in.state.selection<0||in.state.selection>=4)return {};
    for(int row=0;row<4;++row)
        if(in.state.values[row]<0||in.state.values[row]>(row<2?1:5))return {};
    SteamConfigUpdatePlan plan;plan.candidate=in.state;
    auto &state=plan.candidate;
    plan.checked_keys[0]=true;
    if(in.up_repeat)state.selection=(state.selection+3)%4;
    plan.checked_keys[1]=true;
    if(in.down_repeat)state.selection=(state.selection+1)%4;
    const int row=state.selection,before=state.values[row],count=row<2?2:6;
    int display=row<2?(before+1)%2:before;
    // 键盘禁改整个未解锁速度行，区别于Draw只禁高速选项的Touch注册。
    if(row!=1||state.speed_unlocked) {
        plan.checked_keys[2]=true;
        if(in.left_repeat)display=(display+count-1)%count;
        plan.checked_keys[3]=true;
        if(in.right_repeat)display=(display+1)%count;
        const int after=row<2?(display+1)%2:display;
        if(after!=before) {
            state.values[row]=after;
            plan.operations.emplace_back(SteamConfigSettingRequest{row,before,after});
        }
    }
    plan.checked_keys[4]=true;
    if(in.back_soft_label) {
        plan.operations.emplace_back(SteamConfigReturnRequest::save_system);
        plan.operations.emplace_back(SteamConfigReturnRequest::reset_offscreen);
        plan.operations.emplace_back(SteamConfigReturnRequest::pop);
    }
    return plan;
}
// Steam MyFormBase.SetTouchValue RVA200FC0；APK b/d.java:191–244独立交叉。
// 拆分为只读请求供唯一执行者消费，不在计划生成时重复setter或真实平台调用。
std::optional<SteamConfigSetterPlan> steam_config_setting_effects(const SteamConfigSettingRequest &request) {
    if(request.row<0 || request.row>=4)return {};
    const int maximum=request.row<2?1:5;
    if(request.before<0 || request.before>maximum || request.after<0 || request.after>maximum)return {};
    SteamConfigSetterPlan plan;
    if(request.row<2) {
        plan.operations.emplace_back(SteamConfigPropertyWrite{
            SteamConfigProperty::world,request.row==0?12:13,request.after});
        if(request.row==0) {
            plan.operations.emplace_back(SteamConfigLinearFilter{request.after==1});
            plan.operations.emplace_back(SteamConfigSetterAction::draw_all_forms);
        }
    } else if(request.before!=request.after) {
        const int channel=request.row-2;
        plan.operations.emplace_back(SteamConfigPropertyWrite{
            SteamConfigProperty::system,channel==0?3:4,request.after});
        plan.operations.emplace_back(SteamConfigVolume{channel,request.after*51});
        plan.operations.emplace_back(SteamConfigMute{channel,request.after==0});
        plan.operations.emplace_back(SteamConfigSetterAction::reflect_audio);
        if(channel==1)plan.operations.emplace_back(SteamConfigResourceSound{7});
    }
    return plan;
}
} // namespace dungeon_village_prototype
