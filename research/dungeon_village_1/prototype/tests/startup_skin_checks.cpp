#include "dungeon_village_prototype/startup_skin.hpp"
#include "dungeon_village_tools/archive.hpp"
#include "dungeon_village_tools/sprite.hpp"
#include "dungeon_village_tools/table.hpp"
#include <raylib.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

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
} // namespace

// 同一visuals套件集中调用；返回检查数，失败抛具名诊断，由主入口统一收口。
int check_startup_skin(const std::filesystem::path &source_root,
                       const std::filesystem::path &optional_output_png) {
    Checks check;
    const auto assets=tables(source_root,check);
    phases(check);
    if(optional_output_png.empty()) {
        static_images(assets,check,nullptr);
        sprite_pixels(assets,check,nullptr);
    } else {
        check(optional_output_png.extension()==".png" && !optional_output_png.filename().empty(),
              "可选CPU素材输出是明确PNG路径");
        CpuImage sheet(GenImageColor(600,660,{239,239,221,255}));
        static_images(assets,check,&sheet.image);
        sprite_pixels(assets,check,&sheet.image);
        if(!optional_output_png.parent_path().empty())
            std::filesystem::create_directories(optional_output_png.parent_path());
        check(ExportImage(sheet.image,optional_output_png.string().c_str()),"导出CPU静态参考素材拼图");
    }
    return check.count;
}
