#pragma once

// 仅visuals可选CPU导出：消费正式只读计划，不复制窗口/数字/Mapchip布局算法。
#include "dungeon_village_prototype/steam_facility_skin.hpp"
#include "dungeon_village_prototype/startup_world_projection.hpp"
#include "dungeon_village_tools/archive.hpp"
#include "dungeon_village_tools/sprite.hpp"
#include "dungeon_village_tools/table.hpp"
#include <raylib.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace steam_facility_example {
namespace fs=std::filesystem;
namespace p=dungeon_village_prototype;
namespace t=dungeon_village_tools;
using Box=std::array<int,4>;
inline void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
inline std::vector<std::uint8_t> bytes(const fs::path &file){
    std::ifstream in(file,std::ios::binary);need(bool(in),"Steam81 example source unavailable");
    return {std::istreambuf_iterator<char>(in),{}};
}
struct ImageOwner {
    Image image{};
    explicit ImageOwner(Image in):image(in){need(image.data!=nullptr,"Steam81 CPU image decode failed");}
    ImageOwner(const ImageOwner&)=delete;
    ~ImageOwner(){UnloadImage(image);}
};
inline Rectangle rectangle(Box b){return {float(b[0]),float(b[1]),float(b[2]),float(b[3])};}
inline Box intersection(Box a,Box b){
    const int x=std::max(a[0],b[0]),y=std::max(a[1],b[1]);
    return {x,y,std::max(0,std::min(a[0]+a[2],b[0]+b[2])-x),std::max(0,std::min(a[1]+a[3],b[1]+b[3])-y)};
}
inline std::string array(Box a){std::ostringstream s;s<<'['<<a[0]<<','<<a[1]<<','<<a[2]<<','<<a[3]<<']';return s.str();}
struct Source {
    std::size_t size{};
    std::string hash;
};
struct Renderer {
    fs::path root,assets;
    std::map<fs::path,Source> sources;
    std::map<fs::path,std::unique_ptr<ImageOwner>> images;
    std::map<fs::path,t::SpriteDefinition> sprites;
    std::map<int,fs::path> map_images;
    std::vector<Box> clips{{0,0,240,240}};
    std::ostringstream plan;
    bool first{true};
    explicit Renderer(fs::path source):root(std::move(source)),assets(root.parent_path()){
        const auto inf=root/"image/img.inf";remember(inf);
        for(const auto &row:t::parse_tsv(bytes(inf))){
            need(row.size()==2,"Steam81 image index columns");
            fs::path file(row[1]);need(!file.has_parent_path(),"Steam81 image basename");file.replace_extension(".png");
            need(map_images.emplace(t::parse_table_integer(row[0]),root/"image"/file).second,"Steam81 duplicate image ID");
        }
    }
    void remember(const fs::path &file){if(!sources.count(file)){const auto raw=bytes(file);sources.emplace(file,Source{raw.size(),t::sha256_hex(raw)});}}
    const Image &image(const fs::path &file){
        remember(file);auto &ptr=images[file];if(!ptr)ptr=std::make_unique<ImageOwner>(LoadImage(file.string().c_str()));return ptr->image;
    }
    const t::SpriteDefinition &sprite(const fs::path &file){
        remember(file);auto it=sprites.find(file);if(it==sprites.end())it=sprites.emplace(file,t::parse_legacy_seb(bytes(file))).first;return it->second;
    }
    void record(const std::string &value){if(!first)plan<<',';first=false;plan<<value;}
    void fill(Image &out,Box b,Color color){
        b=intersection(b,clips.back());if(b[2]&&b[3])ImageDrawRectangle(&out,b[0],b[1],b[2],b[3],color);
    }
    void pixels(Image &out,const fs::path &file,Box crop,int x,int y,int flipx=0,int flipy=0){
        const auto &source=image(file);
        need(crop[0]>=0&&crop[1]>=0&&crop[2]>0&&crop[3]>0&&crop[0]+crop[2]<=source.width&&crop[1]+crop[3]<=source.height,
             "Steam81 example source crop outside actual PNG");
        need((flipx==0||flipx==1)&&(flipy==0||flipy==1),"Steam81 example unsupported flip");
        record("{\"type\":\"pixels\",\"source\":\""+fs::relative(file,assets).generic_string()+"\",\"crop\":"+array(crop)+
            ",\"destination\":"+array({x,y,crop[2],crop[3]})+",\"clip\":"+array(clips.back())+",\"flip\":["+std::to_string(flipx)+","+std::to_string(flipy)+"]}");
        ImageOwner cut(ImageFromImage(source,rectangle(crop)));
        if(flipx)ImageFlipHorizontal(&cut.image);if(flipy)ImageFlipVertical(&cut.image);
        const auto shown=intersection({x,y,crop[2],crop[3]},clips.back());
        if(shown[2]&&shown[3])ImageDraw(&out,cut.image,rectangle({shown[0]-x,shown[1]-y,shown[2],shown[3]}),rectangle(shown),WHITE);
    }
    void seb(Image &out,const fs::path &file,int frame,int x,int y,const p::SteamFacilityResource *resource=nullptr){
        const auto &s=sprite(file);need(frame>=0&&frame<s.frame_count,"Steam81 example negative/invalid SEB frame not certified");
        bool drawn=false;
        for(const auto &layer:s.layers)for(const auto &part:layer.parts)if(part.frame==frame){
            if(part.image_index<0||part.width<=0||part.height<=0)continue;
            const auto image_file=resource?assets/resource->published_image:map_images.at(part.image_index);
            need(!resource||part.image_index==resource->image,"Steam81 published sprite/image identity mismatch");
            pixels(out,image_file,{part.source_x,part.source_y,part.width,part.height},x+part.offset_x,y+part.offset_y,part.flip_x,part.flip_y);drawn=true;
        }
        need(drawn,"Steam81 example requested empty SEB frame");
    }
    void draw_image(Image &out,const p::SteamFacilityImage &draw){
        const auto resource=p::steam_facility_resource(draw.asset);need(bool(resource),"Steam81 asset mapping unavailable");
        if(draw.crop)pixels(out,assets/resource->published_image,*draw.crop,draw.position[0],draw.position[1]);
        else {need(resource->published_sprite!=nullptr,"Steam81 missing direct crop/SEB");seb(out,assets/resource->published_sprite,draw.frame,draw.position[0],draw.position[1],&*resource);}
    }
    void render(Image &out,const p::SteamFacilitySkinPlan &skin){
        for(const auto &draw:skin.draws){
            if(const auto *r=std::get_if<p::StartupSkinRect>(&draw)){
                const Color c{static_cast<unsigned char>(r->rgb[0]),static_cast<unsigned char>(r->rgb[1]),static_cast<unsigned char>(r->rgb[2]),255};
                record("{\"type\":\"rect\",\"rect\":"+array(r->rect)+",\"rgb\":["+std::to_string(r->rgb[0])+","+
                    std::to_string(r->rgb[1])+","+std::to_string(r->rgb[2])+"],\"outline\":"+(r->outline?"true":"false")+"}");
                if(!r->outline)fill(out,r->rect,c);
                else {const auto b=r->rect;fill(out,{b[0],b[1],b[2],1},c);fill(out,{b[0],b[1]+b[3]-1,b[2],1},c);
                    fill(out,{b[0],b[1],1,b[3]},c);fill(out,{b[0]+b[2]-1,b[1],1,b[3]},c);}
            }else if(const auto *i=std::get_if<p::SteamFacilityImage>(&draw))draw_image(out,*i);
            else if(const auto *c=std::get_if<p::SteamFacilityClip>(&draw)){
                if(c->kind==p::SteamFacilityClipKind::pop){need(clips.size()>1,"Steam81 clip underflow");clips.pop_back();}
                else clips.push_back(intersection(clips.back(),c->rectangle));
                record("{\"type\":\"clip\",\"pop\":"+std::string(c->kind==p::SteamFacilityClipKind::pop?"true":"false")+",\"rect\":"+array(clips.back())+"}");
            }else if(const auto *n=std::get_if<p::SteamFacilityNumber>(&draw)){
                const auto resource=p::steam_facility_resource(n->asset);need(resource&&resource->published_sprite,"Steam81 digit SEB missing");
                const auto &s=sprite(assets/resource->published_sprite);need(!s.layers.empty(),"Steam81 digit layer missing");
                const auto part=std::find_if(s.layers[0].parts.begin(),s.layers[0].parts.end(),[](const auto &v){return v.frame==0;});
                need(part!=s.layers[0].parts.end(),"Steam81 digit width missing");
                const auto expanded=p::steam_facility_number_draws(*n,part->width);need(bool(expanded),"Steam81 number expansion rejected");
                for(const auto &image:*expanded)draw_image(out,image);
            }else if(const auto *m=std::get_if<p::SteamFacilityMapchip2>(&draw)){
                const auto expanded=p::steam_facility_mapchip2_draws(*m);need(bool(expanded),"Steam81 mapchip expansion rejected");
                for(const auto &piece:*expanded)seb(out,root/"image"/piece.sprite,piece.frame,piece.position[0],piece.position[1]);
            }else if(const auto *text=std::get_if<p::SteamFacilityText>(&draw)){
                // 洋红十字仅为研究锚点标记；不拿默认字体冒充原中文字形或假测宽。
                const auto x=text->rectangle[0],y=text->rectangle[1];fill(out,{x-2,y,5,1},MAGENTA);fill(out,{x,y-2,1,5},MAGENTA);
                record("{\"type\":\"text_anchor_only\",\"role\":"+std::to_string(static_cast<int>(text->role))+",\"argument\":"+
                    std::to_string(text->argument)+",\"rectangle\":"+array(text->rectangle)+",\"anchor\":"+
                    (text->anchor?std::to_string(*text->anchor):"null")+",\"font_size\":"+std::to_string(text->font_size)+
                    ",\"rgb\":["+std::to_string(text->rgb[0])+","+std::to_string(text->rgb[1])+","+std::to_string(text->rgb[2])+
                    "],\"japanese\":"+(text->japanese?"true":"false")+"}");
            }
        }
        need(clips.size()==1,"Steam81 clip leak");
    }
};
inline void write_text(const fs::path &file,const std::string &value){
    need(!fs::exists(file),"Steam81 example refuses output overwrite");std::ofstream out(file,std::ios::binary);out<<value;
    need(bool(out),"Steam81 example text output failed");
}
inline void export_sheet(const fs::path &root,const fs::path &directory){
    need(!directory.empty()&&!fs::exists(directory),"Steam81 example requires a new output directory");
    fs::create_directories(directory);Renderer renderer(root);
    ImageOwner sheet(GenImageColor(744,504,{48,52,58,255}));
    const std::array<std::array<int,3>,6> frames{{{0,0,0},{0,40,5},{0,50,20},{1,4,5},{1,34,19},{1,55,20}}};
    std::ostringstream manifest;manifest<<"{\"qualification\":\"research_cpp_plan_fixture_not_original_screenshot\",\"font\":\"magenta_anchor_only\","
        "\"fixture\":{\"definition\":35,\"level\":2,\"attributes\":[[100,110,10],[10,14,4],[20,26,6]],\"limits\":[110,30,40],"
        "\"title_widths\":[80,80],\"notice_widths\":[12,30,4,30],\"measurement_qualification\":\"fixture_not_font_measurements\"},\"samples\":[";
    for(std::size_t i=0;i<frames.size();++i){
        p::SteamFacilityUpgradeSkinInput in;in.definition=35;in.mapchip=p::startup_world_rules().facilities.at(35).display_id;
        in.level=2;in.phase=frames[i][0];in.frame=frames[i][1];in.frame2=frames[i][2];
        in.attributes={{{100,110,10},{10,14,4},{20,26,6}}};in.limits={110,30,40};
        in.title_widths=std::array<int,2>{80,80};in.notice_widths=std::array<int,4>{12,30,4,30};
        const auto skin=p::steam_facility_upgrade_skin(in);need(bool(skin),"Steam81 example fixture rejected");
        ImageOwner panel(GenImageColor(240,240,{218,223,226,255}));renderer.plan.str("");renderer.plan.clear();renderer.first=true;
        renderer.render(panel.image,*skin);const std::string name="phase"+std::to_string(in.phase)+"-frame"+std::to_string(in.frame);
        const auto png=directory/(name+".png"),json=directory/(name+".json");need(ExportImage(panel.image,png.string().c_str()),"Steam81 PNG export failed");
        write_text(json,"{\"phase\":"+std::to_string(in.phase)+",\"frame\":"+std::to_string(in.frame)+",\"frame2\":"+
            std::to_string(in.frame2)+",\"draws\":["+renderer.plan.str()+"]}\n");
        ImageDraw(&sheet.image,panel.image,{0,0,240,240},{float(6+(i%3)*246),float(6+(i/3)*246),240,240},WHITE);
        if(i)manifest<<',';manifest<<"{\"png\":\""<<name<<".png\",\"plan\":\""<<name<<".json\",\"plan_sha256\":\""<<t::sha256_hex(bytes(json))
            <<"\",\"plan_bytes\":"<<fs::file_size(json)<<",\"sha256\":\""<<t::sha256_hex(bytes(png))<<"\",\"bytes\":"<<fs::file_size(png)<<",\"width\":240,\"height\":240}";
    }
    const auto output=directory/"contact-sheet.png";need(ExportImage(sheet.image,output.string().c_str()),"Steam81 sheet export failed");
    manifest<<"],\"contact_sheet\":{\"file\":\"contact-sheet.png\",\"sha256\":\""<<t::sha256_hex(bytes(output))<<"\",\"bytes\":"<<fs::file_size(output)
        <<",\"width\":744,\"height\":504},\"sources\":[";bool first=true;
    for(const auto &[file,source]:renderer.sources){if(!first)manifest<<',';first=false;manifest<<"{\"path\":\""<<fs::relative(file,renderer.assets).generic_string()
        <<"\",\"sha256\":\""<<source.hash<<"\",\"bytes\":"<<source.size<<'}';}
    manifest<<"]}\n";write_text(directory/"MANIFEST.json",manifest.str());
}
} // namespace steam_facility_example
