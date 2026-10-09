#include "dungeon_village_prototype/startup_skin.hpp"
#include "dungeon_village_prototype/startup_information.hpp"
#include "dungeon_village_prototype/startup_title_actor_skin.hpp"
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
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

int check_steam_startup_skin(const std::filesystem::path &source_root);

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
} // namespace

// 同一visuals套件集中调用；返回检查数，失败抛具名诊断，由主入口统一收口。
int check_startup_skin(const std::filesystem::path &source_root,
                       const std::filesystem::path &optional_output_png) {
    Checks check;
    const auto assets=tables(source_root,check);
    phases(check);
    frame_geometry(check);
    income_information(check);
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
    return check.count + check_steam_startup_skin(source_root);
}
