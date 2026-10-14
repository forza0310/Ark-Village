// Steam raw21有序计划；来源与独立Owner缺口见ui/STEAM_BUILD_LIST.md。
#include "ark/simulation/presentation/steam_build_menu_skin.hpp"
#include "ark/simulation/world/startup.hpp"
#include "ark/simulation/map/startup_world_projection.hpp"
#include "ark/simulation/map/rules/geometry.hpp"
#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation {
namespace {
constexpr std::array<int,3> name_color{33,36,51}, price_color{99,101,112}, bad_price{210,0,0};
bool bounded_text(const std::string &s) { return !s.empty() && s.size() <= 4096; }
bool fits(std::int64_t n) {
    return n >= std::numeric_limits<int>::min() && n <= std::numeric_limits<int>::max();
}
std::optional<std::vector<SteamFacilityMapchipDraw>> mapchip_draws(
    const SteamBuildMenuRow &row, std::array<int,2> position) {
    const auto &art=startup_evidence().displays;
    const auto source=std::find_if(art.begin(),art.end(),[&](const auto &a){return a.id==row.mapchip;});
    const auto &defs=startup_world_rules().facilities;
    const auto definition=std::find_if(defs.begin(),defs.end(),[&](const auto &d){return d.id==row.definition;});
    if(source==art.end() || definition==defs.end() || source->definition_id!=row.definition ||
       definition->display_id!=row.mapchip || definition->kind!=row.kind || source->sprite.empty())return {};
    // 复用已核pattern分片顺序；普通DrawMapchip没有Mapchip2的居中偏移。
    const auto shape=ref::facility_footprint(static_cast<ref::FacilityShape>(definition->shape),
        ref::FacilityOrientation::first,{1,1},3,3);
    if(shape.error!=ref::GeometryError::none)return {};
    std::vector<SteamFacilityMapchipDraw> out;
    for(const auto &piece:shape.cells) {
        const int u=piece.position.x-1,v=piece.position.y-1;
        const auto x=std::int64_t(position[0])+30*(u+v),y=std::int64_t(position[1])+15*(u-v);
        if(!fits(x)||!fits(y))return {};
        out.push_back({source->sprite,row.kind==6?11:piece.fragment_index,
            {static_cast<int>(x),static_cast<int>(y)}});
    }
    return out;
}
}
std::optional<SteamFacilityResource> steam_build_menu_resource(SteamBuildMenuAsset a) {
    switch(a) {
    case SteamBuildMenuAsset::demolition:
        return SteamFacilityResource{"common",17,-1,"original/common/destruct00.png",nullptr};
    case SteamBuildMenuAsset::movement:
        return SteamFacilityResource{"common",119,-1,"original/common/moveTenant.png",nullptr};
    case SteamBuildMenuAsset::resident:
        return SteamFacilityResource{"common",98,81,"steam-build-common/original/common/tenant_resident.png",
            "original/common/tenant_resident.seb"};
    case SteamBuildMenuAsset::new_tag:
        return SteamFacilityResource{"common",147,-1,"original/common/wnd_new.png",nullptr};
    }
    return {};
}
std::optional<SteamBuildMenuSkinPlan> steam_build_menu_skin(const SteamBuildMenuSkinInput &in) {
    std::array<int,3> counts{};std::set<int> identities;
    for(std::size_t c=0;c<3;++c) {
        if(in.catalogs[c].size()>87 || !bounded_text(in.category_names[c]))return {};
        counts[c]=static_cast<int>(in.catalogs[c].size());
        for(const int id:in.catalogs[c])
            if(id < -2 || id >= 85 || (id<0 && c!=0) || !identities.insert(id).second)return {};
    }
    if(!prepare_startup_build_menu_cursor(counts,in.cursor,{}) ||
       in.finger_frame<0 || in.finger_frame>5 || in.frame2<0 ||
       in.frame2==std::numeric_limits<int>::max())return {};
    const int J=in.origin[0],K=in.origin[1];
    // 页内固定最大偏移及mapchip分片均在此显式边界内，避免私有坐标加法溢出。
    if(J < -65536 || J > 65536 || K < -65536 || K > 65536)return {};
    const auto &catalog=in.catalogs[static_cast<std::size_t>(in.cursor.page)];
    const auto remaining=std::max(counts[static_cast<std::size_t>(in.cursor.page)]-in.cursor.scroll,0);
    const auto visible=std::min(remaining,5);
    if(in.visible_rows.size()!=static_cast<std::size_t>(visible))return {};
    std::vector<std::vector<SteamFacilityMapchipDraw>> row_mapchips(static_cast<std::size_t>(visible));
    for(int r=0;r<visible;++r) {
        const auto &row=in.visible_rows[static_cast<std::size_t>(r)];
        if(row.definition!=catalog[static_cast<std::size_t>(in.cursor.scroll+r)] ||
           !bounded_text(row.name)||!bounded_text(row.price_text)||!row.name_width||*row.name_width<0||
           !fits(std::int64_t(*row.name_width)+15)||
           !fits(std::int64_t(J)+79+*row.name_width+15)||row.remaining<0)return {};
        if(row.definition<0) {
            if(row.mapchip!=-1 || row.kind!=0 || row.remaining!=0 || row.build_now || row.resident ||
               row.quote!=(row.definition==-1?0:300))return {};
        } else {
            const auto pieces=mapchip_draws(row,{J+12,K+35+37*r});
            if(!pieces)return {};
            row_mapchips[static_cast<std::size_t>(r)]=*pieces;
        }
    }
    SteamBuildMenuSkinPlan p;
    const auto rectangle=[&](int x,int y,int w,int h,std::array<int,3> rgb,bool outline=false) {
        // DrawRect实参为末像素差，沿现正式矩形后端转为半开大小；FillRect不加1。
        p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{x,y,w+(outline?1:0),h+(outline?1:0)},rgb,outline}});
    };
    const auto line=[&](int x1,int y1,int x2,int y2,std::array<int,3> rgb) {
        p.operations.emplace_back(SteamBuildMenuLine{{x1,y1,x2,y2},1,rgb});
    };
    const auto text=[&](const std::string &s,std::array<int,4> rect,int anchor,int size,bool layout,
                        std::array<int,3> color) {
        p.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::name,
            s,rect,anchor,size,layout,color});
    };
    rectangle(J,K,175,210,{79,72,48},true);
    rectangle(J+1,K+1,173,208,{193,234,94},true);
    line(J+2,K+2,J+2,K+208,{235,241,132});
    for(int c=0;c<3;++c) {
        const bool selected=c==in.cursor.page;
        rectangle(J+3+57*c,K+2,56,16,selected?std::array<int,3>{59,119,0}:std::array<int,3>{60,109,0},true);
        rectangle(J+4+57*c,K+3,55,15,selected?std::array<int,3>{3,255,133}:std::array<int,3>{223,255,67});
        text(in.category_names[static_cast<std::size_t>(c)],{J+5+57*c,K+3,in.category_new[c]?35:55,15},
            0x22,in.japanese?0:9,true,selected?std::array<int,3>{42,98,4}:std::array<int,3>{76,126,66});
        if(in.category_new[c])p.operations.emplace_back(SteamBuildMenuImage{
            SteamBuildMenuAsset::new_tag,{J+39+57*c,K+9},0,{}});
    }
    line(J+2,K+19,J+173,K+19,{232,237,166});
    line(J+2,K+21,J+173,K+21,{232,237,166});
    line(J+2,K+20,J+173,K+20,{100,135,71});
    rectangle(J+2,K+22,165,186,{235,241,132},true);
    rectangle(J+3,K+22,163,185,{5,184,2},true);
    rectangle(J+4,K+23,162,184,{251,253,249});
    for(int n=1;n<5;++n)line(J+4,K+22+37*n,J+166,K+22+37*n,{5,184,2});
    auto current_color=std::array<int,3>{5,184,2};
    for(int r=0;r<5;++r) {
        const int y=K+25+37*r;
        if(r>=visible) {
            // 空行没有SetColor，继承最近一条价格或整个空类的最后分隔线颜色。
            text("---",{J+90,K+41+37*r,0,0},0x22,0,false,current_color);
            continue;
        }
        const auto &row=in.visible_rows[static_cast<std::size_t>(r)];
        rectangle(J+10,y,64,32,{190,242,230});
        p.operations.emplace_back(SteamBuildMenuClip{SteamBuildMenuClipKind::set,{J+10,y,64,32}});
        if(row.definition<0)p.operations.emplace_back(SteamBuildMenuImage{
            row.definition==-1?SteamBuildMenuAsset::demolition:SteamBuildMenuAsset::movement,
            {J+10+(row.definition==-1?4:1),y+(row.definition==-1?3:0)},0,{}});
        else {
            for(const auto &piece:row_mapchips[static_cast<std::size_t>(r)])p.operations.emplace_back(piece);
            if(row.kind==12)p.operations.emplace_back(SteamFacilityNumber{SteamFacilityNumberKind::multi_value,
                SteamFacilityAsset::number05,row.remaining,{J+75,K+47+37*r},0,4});
            else if((row.kind==2||row.kind==3)&&!row.build_now)p.operations.emplace_back(SteamBuildMenuImage{
                SteamBuildMenuAsset::new_tag,{J+10,K+48+37*r},0,{}});
            if(row.kind==13&&row.resident)p.operations.emplace_back(SteamBuildMenuImage{
                SteamBuildMenuAsset::resident,{J+42,K+40+37*r},0,{}});
        }
        p.operations.emplace_back(SteamBuildMenuClip{SteamBuildMenuClipKind::clear,{}});
        const bool selected=in.cursor.selection==in.cursor.scroll+r;
        if(in.japanese) {
            if(selected)rectangle(J+79,y,static_cast<int>(std::int64_t(*row.name_width)+15),15,{248,193,108});
            text(row.name,{J+86,y+2,0,0},0x11,0,false,name_color);
        } else {
            const int N=static_cast<int>(std::min(std::int64_t(*row.name_width)+9,std::int64_t(88)));
            if(selected)rectangle(J+76,y,N,15,{248,193,108});
            text(row.name,{J+80,y,N-4,15},0x20,10,true,name_color);
        }
        current_color=row.quote==0||row.quote<=in.funds?price_color:bad_price;
        text(row.price_text,{J+164,y+18,0,0},4,0,false,current_color);
        if(selected&&row.definition>=0&&(row.kind==2||row.kind==3||row.kind==13))p.soft_labels={7,2};
    }
    for(int r=0;r<5;++r)p.operations.emplace_back(SteamBuildMenuTouch{
        {11,0x20000|(in.cursor.scroll+r),std::array<int,4>{J+2,K+22+37*r,165,37},{},16},
        {10,0,0,0},{}});
    // 原DrawVerticalScroll2→component12/type0，再注册25。此几何只读当前scroll，不夹21空行选择。
    const int count=counts[static_cast<std::size_t>(in.cursor.page)],D=std::max(count,5);
    const int top=K+21+static_cast<int>(189LL*in.cursor.scroll/D);
    int length=static_cast<int>(945LL/D);
    rectangle(J+169,K+21,5,189,{7,5,78});
    // 源越尾分支真实使用h-top，不补回y、不夹首行；返回null仍保留轨道和12／25注册。
    if(top+length>K+21+189)length=189-top;
    auto scroll_rectangle=std::array<int,4>{J+170,K+21,3,189};
    int scroll_flags{};std::optional<float> drag_grid;
    if(length>0) {
        const bool dragging=in.scroll_first_touch&&count>5;
        rectangle(J+169,top,5,length+1,dragging
            ?std::array<int,3>{246,129,0}:std::array<int,3>{48,160,255});
        if(dragging) {
            scroll_rectangle={J+140,-1000,125,2000};scroll_flags=0x24;
            const float grid=static_cast<float>(189-length)/static_cast<float>(count-5);
            drag_grid=grid<=0.F?1.F:grid; // 源只修非正值，不把正的小于1值夹到1。
        } else scroll_rectangle={J+170,top,5,length};
    }
    p.operations.emplace_back(SteamBuildMenuTouch{
        {12,0x40000,scroll_rectangle,{},0},{3,20,0,0},std::array<int,3>{count,5,0x20000},scroll_flags,drag_grid});
    p.operations.emplace_back(SteamBuildMenuTouch{
        {25,0,std::array<int,4>{J+170,K+21,3,189},{},4},{},{}});
    for(int c=0;c<3;++c)p.operations.emplace_back(SteamBuildMenuTouch{
        {3,0x50000|c,std::array<int,4>{J+57*c,K+2,60,20},{},0},{0,0,30,0},{}});
    const int drift=(in.frame2%20)/5;
    p.operations.emplace_back(SteamBuildMenuArrowTouch{{30-drift,K+9},3,16,{40,-10,20,-10},in.arrow_selected[0]});
    p.operations.emplace_back(SteamBuildMenuArrowTouch{{207+drift,K+9},0,18,{-10,40,20,-10},in.arrow_selected[1]});
    p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::finger,
        {J+(in.japanese?71:68),K+33+37*(in.cursor.selection-in.cursor.scroll)},in.finger_frame,{}});
    return p;
}
} // namespace ark::simulation
