#include "dungeon_village_prototype/startup_skin.hpp"
#include "dungeon_village_prototype/startup_information.hpp"
#include "dungeon_village_prototype/steam_information_skin.hpp"
#include "dungeon_village_prototype/startup_world_information.hpp"
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
        const auto &image=std::get<StartupSkinDraw>(plan.draws.at(*touch.image_draw));
        check(image.image==74 && image.sprite==3 && image.frame==(side==0?3:0) &&
                  image.offset==std::array<int,2>{side==0?22:218,51},"箭头原身份/帧/初始锚");
    }
    owner.page_phases.at(id)=1;owner.page_counters.at(id)=8;options.japanese=true;options.view_y=20;
    const auto year=*steam_income_information_skin(owner,id,options);
    check(labels(year,Role::period)[0]->position[0]==28 && labels(year,Role::income_header)[0]->position[0]==121 &&
              labels(year,Role::income_header)[0]->font_size==0 && labels(year,Role::expense_header)[0]->position[0]==194 &&
              labels(year,Role::profit_value)[0]->value=="400Ｇ" &&
              labels(year,Role::profit_value)[0]->rgb==std::array<int,3>{0,100,255},
          "日文分支原表头与年统计正利润，未借非日文临时字体");
    check(labels(year,Role::title)[0]->position[1]==45 && labels(year,Role::category)[0]->position[1]==90 &&
              std::get<StartupSkinDraw>(year.draws.at(*year.touches[0].image_draw)).offset==std::array<int,2>{19,51} &&
              std::get<StartupSkinDraw>(year.draws.at(*year.touches[1].image_draw)).offset==std::array<int,2>{221,51},
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
} // namespace

// 同一visuals套件集中调用；返回检查数，失败抛具名诊断，由主入口统一收口。
int check_startup_skin(const std::filesystem::path &source_root,
                       const std::filesystem::path &optional_output_png) {
    Checks check;
    const auto assets=tables(source_root,check);
    phases(check);
    frame_geometry(check);
    income_information(check);
    income_skin(check,source_root);
    item_information(check);
    equipment_information(check);
    equipment_information_icons(check,source_root);
    information_directory_skin(check,source_root);
    adventurer_information_skin(check,source_root);
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
