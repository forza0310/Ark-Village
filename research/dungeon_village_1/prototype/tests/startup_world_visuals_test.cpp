#include "dungeon_village_prototype/startup_world_visuals.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_tools/sprite.hpp"
#include "dungeon_village_tools/table.hpp"
#include "support/world_fixture.hpp"

#include <raylib.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_prototype;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
std::vector<std::uint8_t> bytes(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("素材读取失败");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
void owner_mapping() {
    auto s = test_support::world_fixture();
    const auto first = startup_world_portrait(s, 1);
    check(first && first->image == 20 &&
              first->image == s.rules->jobs.at(1).sprites.at(s.rules->humans.at(1).sex) &&
              first->sprite == 1 && first->frame == 0 && first->clip_width == 15 &&
              first->clip_height == 14 && first->anchor_x == 8 && first->anchor_y == 21,
          "first real human uses job/sex image with source portrait clip and anchor");
    s.scene.world.world.ai.growth.at(1).definition.current_profession = 2;
    check(startup_world_portrait(s, 1)->image ==
              s.rules->jobs.at(2).sprites.at(s.rules->humans.at(1).sex),
          "portrait reads current shared profession rather than initial actor metadata");
    auto &world = s.scene.world.world;
    const auto facility = std::find_if(world.facilities.begin(), world.facilities.end(),
                                       [](const auto &f) { return f.second.category == 2; });
    check(facility != world.facilities.end(), "real startup contains inn");
    for (std::uint64_t n = 1; n <= 5; ++n) {
        ref::BattleActorRecord actor;
        actor.id = {n};
        actor.definition = static_cast<int>(n);
        actor.control.flags = n == 2 ? 0 : 32U;
        actor.state_counter = n == 1 ? 169 : 170;
        actor.hp.displayed = world.ai.growth.at(actor.definition).derived.combat[0] / 2;
        world.ai.battle.actors.emplace(actor.id, actor);
        facility->second.occupants.push_back(actor.id);
    }
    const auto rows = startup_world_inn_rows(s, facility->first);
    check(rows && rows->size() == 3 && rows->at(0).actor.value == 1 &&
              rows->at(1).actor.value == 3 && rows->at(2).actor.value == 4 &&
              rows->at(2).row == 2 && rows->at(0).bar_width == 29 && !rows->at(0).healing &&
              rows->at(1).healing,
          "only first four references, flag32 filter and compact20px rows with169/170 boundary");
    const auto image = rows->front().portrait.image;
    world.ai.battle.actors.at({1}).state_counter = 170;
    check(startup_world_inn_rows(s, facility->first)->front().portrait.image == image &&
              startup_world_inn_rows(s, facility->first)->front().healing,
          "switch to capacity/HP bar never switches portrait identity");
    const auto draws = s.scene.random.draws();
    const auto funds = world.ai.accounting.funds();
    (void)startup_world_inn_rows(s, facility->first);
    check(draws == s.scene.random.draws() && funds == world.ai.accounting.funds() &&
              world.ai.battle.actors.at({1}).state_counter == 170,
          "display query consumes no service ticks, cash or random");
    world.ai.battle.actors.erase({3});
    check(!startup_world_inn_rows(s, facility->first) && !startup_world_portrait(s, -1),
          "stale occupancy and unknown human are explicit display failures");
}
void cpu_portraits(const std::filesystem::path &root) {
    const auto seb = dungeon_village_tools::parse_legacy_seb(bytes(root / "human/walk01.seb"));
    check(seb.layers.size() == 1 && seb.frame_count == 4, "published source portrait SEB schema");
    const auto &p = seb.layers.front().parts.front();
    check(p.frame == 0 && p.source_x == 0 && p.source_y == 24 && p.width == 18 && p.height == 24 &&
              p.offset_x == -9 && p.offset_y == -24 && p.flip_x == 0 && p.flip_y == 0,
          "walk01 frame0 is exact18x24 body, not guessed independent portrait tile");
    std::map<int, std::filesystem::path> images;
    for (const auto &row : dungeon_village_tools::parse_tsv(bytes(root / "human/img.inf"))) {
        auto path = std::filesystem::path(row.at(1));
        path.replace_extension(".png");
        images.emplace(dungeon_village_tools::parse_table_integer(row.at(0)), path);
    }
    const auto same = [](Color a, Color b) {
        return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
    };
    for (const auto &job : startup_world_rules().jobs)
        for (const auto image_id : job.sprites) {
            Image image = LoadImage((root / "human" / images.at(image_id)).string().c_str());
            check(
                image.data && image.width >= 18 && image.height >= 48,
                "every current profession/sex resolves to decoded image and valid body rectangle");
            // 脚底(8,21)+SEB(-9,-24)=(-1,-3)，15×14裁剪等于原图(1,27,15,14)。
            Image portrait = ImageFromImage(image, {1, 27, 15, 14});
            Image panel = GenImageColor(17, 17, {0, 0, 0, 0});
            ImageDraw(&panel, portrait, {0, 0, 15, 14}, {1, 1, 15, 14}, WHITE);
            int opaque{};
            for (int y = 0; y < 17; ++y)
                for (int x = 0; x < 17; ++x) {
                    const auto color = GetImageColor(panel, x, y);
                    if (x == 0 || x == 16 || y == 0 || y >= 15)
                        check(color.a == 0, "portrait never overwrites17x17 panel border");
                    else {
                        const auto original = GetImageColor(image, x, y + 26);
                        if (original.a == 255)
                            check(same(color, original), "clipped opaque pixel matches source1,27");
                        opaque += color.a > 0;
                    }
                }
            check(opaque > 20, "every job/sex portrait contains nonblank actual image pixels");
            UnloadImage(panel);
            UnloadImage(portrait);
            UnloadImage(image);
        }
}
StartupWorldRuntimeState lift_fixture() {
    auto s = test_support::world_fixture();
    ref::BattleActorRecord actor;
    actor.id = {1};
    actor.definition = 1;
    actor.kind = ref::ActorKind::human;
    actor.control.facing = 0;
    s.scene.world.world.ai.battle.actors.emplace(actor.id, actor);
    s.scene.world.world.ai.contexts.emplace(actor.id, ref::RewardActorContext{});
    return s; // 只准备举物快照条件，不冒称实际购买轨迹。
}
void equipment_lift_queries() {
    auto s = lift_fixture();
    auto &ai = s.scene.world.world.ai;
    auto &display = ai.contexts.at({1}).effects.display;
    const auto draws = s.scene.random.draws();
    const auto cash = ai.accounting.funds();
    const auto equipment = s.shop_humans.at(1).equipment;
    const auto pages = s.scripts.pages.size();
    // 独立源常量oracle：原武器0图片50，原武器9图片2；同为挥动风格0。
    const std::array<std::array<int, 3>, 10> stages{{
        {0,0,50}, {1,5,50}, {7,20,50}, {8,20,50}, {19,20,50},
        {20,20,2}, {31,20,2}, {32,20,2}, {33,19,2}, {39,0,2}}};
    constexpr std::array<std::array<int, 2>, 4> offsets{{{-5,-36},{-4,-36},{-19,-37},{-17,-36}}};
    for (const auto &stage : stages)
        for (int direction = 0; direction < 4; ++direction) {
            ai.battle.actors.at({1}).control.facing = direction;
            display = {{15,stage[0],91,37,0,9,571,-71}};
            const auto before = display;
            const auto commands = startup_world_equipment_lift_draws(s,{1});
            check(commands && commands->size() == 1 &&
                      commands->front().resource == StartupVisualResource::weapon &&
                      commands->front().sprite == direction && commands->front().image == stage[2] &&
                      commands->front().frame == 0 && commands->front().layer == 0 &&
                      commands->front().offset == std::array<int,2>{offsets[direction][0],offsets[direction][1]-stage[1]} &&
                      display == before,
                  "cd15 exact age20 identity switch, integer rise/fall and source direction offsets without advancing cd");
        }
    // 显示21/22读定义图标，而非装备定义ID；13/20的原图标为41/20。
    for (const auto stage : {std::array<int,2>{0,-32}, {1,-34}, {5,-40}, {11,-44}, {12,-44}, {21,-44}})
        for (int kind : {21,22}) {
            display = {{kind,stage[0],218,-18,kind==21?13:20}};
            const auto commands = startup_world_equipment_lift_draws(s,{1});
            check(commands && commands->size() == 2 &&
                      commands->at(0).resource == StartupVisualResource::common &&
                      commands->at(0).sprite == -1 && commands->at(0).image == 24 &&
                      commands->at(0).crop == std::array<int,4>{54,0,18,18} &&
                      commands->at(1).image == (kind==21?20:21) &&
                      commands->at(1).crop == (kind==21?std::array<int,4>{18,72,18,18}:std::array<int,4>{0,36,18,18}) &&
                      commands->at(0).offset == std::array<int,2>{-8,stage[1]} &&
                      commands->at(1).offset == commands->at(0).offset,
                  "cd21/22 draws original background then real18px atlas icon at integer lift height");
        }
    display = {{7,-8,0,0},{15,0,0,0,0,9,571,-71},{21,1,218,-18,13},{22,5,218,-18,20},
               {15,0,0,0,0,9,571,-71}};
    const auto owner_before = startup_world_state_digest(s);
    const auto order = startup_world_equipment_lift_draws(s,{1});
    check(order && order->size() == 6 && order->at(0).resource == StartupVisualResource::weapon &&
              order->at(1).image == 24 && order->at(2).image == 20 && order->at(3).image == 24 &&
              order->at(4).image == 21 && order->at(5).resource == StartupVisualResource::weapon,
          "mixed cd source order and duplicate lifts remain distinct; smoke is delegated rather than guessed");
    check(startup_world_state_digest(s) == owner_before,
          "combined weapon/armour/accessory draw query preserves the complete serialized Owner digest");
    ai.battle.actors.at({1}).control.flags = 1;
    check(startup_world_equipment_lift_draws(s,{1})->empty(), "original visibility flag1 suppresses complete lift drawing");
    ai.battle.actors.at({1}).control.flags = 0;
    display = {{15,-1,0,0,0,9,571,-71}};
    check(startup_world_equipment_lift_draws(s,{1})->empty(), "negative cd age remains pending and produces no lift pixels");
    for (const auto &payload : std::vector<ref::ActorEffectRecord>{{15,0,0}, {21,0,218,-18},
            {15,0,0,0,999,9,571,-71}, {22,0,218,-18,999},
            {15,std::numeric_limits<int>::max(),0,0,0,9,571,-71},
            {21,2,std::numeric_limits<int>::max(),-18,13}}) {
        display = {payload};
        const auto before = display;
        check(!startup_world_equipment_lift_draws(s,{1}) && display == before,
              "truncated, unknown and overflowing lift records explicitly fail without partial output or mutation");
    }
    display = {{15,0,0,0,0,9,571,-71}};
    // 合法原世界+私有规则损坏夹具，检查资源索引拒绝；冻结原表和共享rules不修改。
    for (const auto fault : {std::array<int,3>{1,0,-1}, {1,0,4},
                             {1,1,-1}, {1,1,7}, {1,1,57}, {2,1,50}, {3,1,30}}) {
        auto broken = s;
        StartupWorldRules private_rules = *s.rules;
        broken.rules = &private_rules;
        const int definition = fault[0] == 1 ? 0 : fault[0] == 2 ? 13 : 20;
        broken.scene.world.world.ai.contexts.at({1}).effects.display = fault[0] == 1
            ? std::vector<ref::ActorEffectRecord>{{15,0,0,0,definition,9,571,-71}}
            : std::vector<ref::ActorEffectRecord>{{fault[0] == 2 ? 21 : 22,0,218,-18,definition}};
        check(startup_world_equipment_lift_draws(broken,{1}).has_value(),
              "resource rejection fixture starts with an actual valid original definition and lift");
        auto d = std::find_if(private_rules.equipment.begin(),private_rules.equipment.end(),
            [&](const auto &v) { return v.shop.kind == fault[0] && v.shop.id == definition; });
        check(d != private_rules.equipment.end(), "private rule fault binds actual original equipment");
        if (fault[1] == 0) d->render_style = fault[2];
        else d->render_image = fault[2];
        const auto digest = startup_world_state_digest(broken);
        check(!startup_world_equipment_lift_draws(broken,{1}) &&
                  startup_world_state_digest(broken) == digest,
              "bad private render style/image domain explicitly rejects without mutating complete Owner");
    }
    for (int fault = 0; fault < 4; ++fault) {
        auto broken = s;
        if (fault == 0) broken.scene.world.world.ai.contexts.erase({1});
        if (fault == 1) broken.scene.world.world.ai.battle.actors.erase({1});
        if (fault == 2) broken.scene.world.world.ai.battle.actors.at({1}).control.facing = 4;
        if (fault == 3) broken.rules = nullptr;
        check(!startup_world_equipment_lift_draws(broken,{1}), "missing actor/context/rules or bad direction rejects source query");
    }
    check(s.scene.random.draws() == draws && ai.accounting.funds() == cash &&
              s.shop_humans.at(1).equipment == equipment && s.scripts.pages.size() == pages &&
              s.sound_requests.empty(), "all lift queries preserve common random, cash, equipment, pages and sound output");
}
void facility_growth_queries() {
    auto s = test_support::world_fixture();
    const auto facility = s.scene.world.facility_order.front();
    constexpr std::array<int,15> phases{0,0,1,1,2,2,3,3,3,4,4,4,5,5,5};
    constexpr std::array<int,9> down_stages{0,0,0,0,1,1,1,1,1};
    constexpr std::array<int,7> up_y{-24,-25,-26,-27,-28,-29,-31};
    constexpr std::array<int,7> down_y{-32,-32,-31,-30,-30,-29,-28};
    const auto draws = s.scene.random.draws();
    const auto neighbourhood = s.neighbourhood;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    for (int kind = 1; kind <= 6; ++kind)
        for (int age = 0; age <= 42; ++age) {
            auto &notices = s.facility_details.at(facility).notices;
            notices = {{kind,age},{kind==1?6:1,0}};
            const auto before = notices;
            const auto digest = age == 6 ? startup_world_state_digest(s) : std::string{};
            const auto result = startup_world_facility_growth_draws(s,facility);
            const int phase = age < 15 ? phases[age] : 6;
            const int count = kind <= 3 && age >= 6 && age <= 14 ? 2 : 1;
            check(result && static_cast<int>(result->size()) == count &&
                      result->front().resource == StartupVisualResource::common &&
                      result->front().sprite == (kind<=3?68+kind:85) &&
                      result->front().frame == (kind<=3?phase:(kind-4)*3+(age<9?down_stages[age]:2)) &&
                      result->front().layer == 0 && result->front().image == -1 &&
                      result->front().offset == std::array<int,2>{0,kind<=3?up_y[std::min(age,6)]:down_y[std::min(age,6)]} &&
                      (count==1 || (result->at(1).layer==1 && result->at(1).frame==phase &&
                                    result->at(1).sprite==result->front().sprite && result->at(1).offset==result->front().offset)) &&
                      notices == before,
                  "growth head exact0..42 phase/layer and signed integer anchor; later notice never affects current drawing");
            if (age == 6)
                check(startup_world_state_digest(s) == digest,
                      "each positive/negative growth kind preserves complete Owner digest at active draw/layer boundary");
        }
    for (int kind : {0,7}) {
        s.facility_details.at(facility).notices = {{kind,0},{1,6}};
        check(startup_world_facility_growth_draws(s,facility)->empty(), "growth query delegates construction/NEW and does not draw second queue element");
    }
    for (const auto bad : {std::array<int,2>{1,-1}, {-1,0}, {8,0}}) {
        s.facility_details.at(facility).notices = {bad};
        check(!startup_world_facility_growth_draws(s,facility), "invalid growth head rejects without fabricated first frame");
    }
    auto broken = s;
    broken.facility_details.erase(facility);
    check(!startup_world_facility_growth_draws(broken,facility) &&
              !startup_world_facility_growth_draws(s,std::numeric_limits<std::uint64_t>::max()) &&
              s.scene.random.draws()==draws && s.neighbourhood==neighbourhood &&
              s.scene.world.world.ai.accounting.funds()==cash && s.sound_requests.empty(),
          "growth query rejects missing owner references and preserves neighbour cache, cash, random and sounds");
}
std::map<int,std::filesystem::path> source_images(const std::filesystem::path &root, const char *group) {
    std::map<int,std::filesystem::path> result;
    for (const auto &row : dungeon_village_tools::parse_tsv(bytes(root/group/"img.inf"))) {
        auto p = std::filesystem::path(row.at(1));
        if (p.extension()==".gif") p.replace_extension(".png");
        result.emplace(dungeon_village_tools::parse_table_integer(row.at(0)),root/group/p);
    }
    return result;
}
void cpu_equipment_and_growth(const std::filesystem::path &root) {
    auto s = lift_fixture();
    const auto weapon_images = source_images(root,"weapon");
    const auto common_images = source_images(root,"common");
    constexpr std::array<const char *,4> styles{"sword","bow","spear","greatSword"};
    int directions{};
    for (const auto &d : s.rules->equipment) {
        if (d.shop.kind==1) {
            Image image=LoadImage(weapon_images.at(d.render_image).string().c_str());
            check(image.data!=nullptr,"every original weapon image index decodes from original INF");
            for (int direction=0;direction<4;++direction) {
                s.scene.world.world.ai.battle.actors.at({1}).control.facing=direction;
                s.scene.world.world.ai.contexts.at({1}).effects.display={{15,8,0,0,d.shop.id,d.shop.id,571,-71}};
                const auto commands=startup_world_equipment_lift_draws(s,{1});
                const auto filename=std::string(styles.at(d.render_style))+"0"+std::to_string(direction)+".seb";
                const auto seb=dungeon_village_tools::parse_legacy_seb(bytes(root/"weapon"/filename));
                const auto part=std::find_if(seb.layers.at(0).parts.begin(),seb.layers.at(0).parts.end(),[](const auto &p){return p.frame==0;});
                check(commands && commands->size()==1 && commands->front().image==d.render_image &&
                          commands->front().sprite==d.render_style*4+direction && part!=seb.layers.at(0).parts.end() &&
                          part->source_x>=0 && part->source_y>=0 && part->width>0 && part->height>0 &&
                          part->source_x+part->width<=image.width && part->source_y+part->height<=image.height &&
                          part->flip_x==0 && part->flip_y==0,
                      "all33 weapons in four original direction SEBs use valid frame0 on definition override PNG");
                Image crop=ImageFromImage(image,{static_cast<float>(part->source_x),static_cast<float>(part->source_y),
                                                static_cast<float>(part->width),static_cast<float>(part->height)});
                check(crop.data && crop.width==part->width && crop.height==part->height,
                      "CPU crop materializes real weapon slice without a window or guessed icon frame");
                UnloadImage(crop); ++directions;
            }
            UnloadImage(image);
        } else if (d.shop.kind==2 || d.shop.kind==3) {
            s.scene.world.world.ai.contexts.at({1}).effects.display={{d.shop.kind==2?21:22,12,218,-18,d.shop.id}};
            const auto commands=startup_world_equipment_lift_draws(s,{1});
            check(commands && commands->size()==2,"every original defensive/accessory definition has two drawing commands");
            for (const auto &command:*commands) {
                Image image=LoadImage(common_images.at(command.image).string().c_str());
                const auto &r=command.crop;
                check(image.data && r[0]>=0 && r[1]>=0 && r[0]+r[2]<=image.width && r[1]+r[3]<=image.height,
                      "all original equipment atlas icons and backgrounds fit their decoded PNG");
                UnloadImage(image);
            }
        }
    }
    check(directions==132,"CPU resources cover all33 original weapons and four directions");
    const auto facility=s.scene.world.facility_order.front();
    for (int kind=1;kind<=6;++kind) {
        const auto seb=dungeon_village_tools::parse_legacy_seb(bytes(root/"common"/
            (kind<=3?std::string("eff_tenantUse0")+std::to_string(kind-1)+".seb":"eff_tenantUse04.seb")));
        for (const int age:{0,2,4,6,9,12,15,42}) {
            s.facility_details.at(facility).notices={{kind,age}};
            const auto commands=startup_world_facility_growth_draws(s,facility);
            check(commands.has_value(),"CPU growth resource has a source draw plan");
            for (const auto &command:*commands) {
                const auto &layer=seb.layers.at(command.layer);
                const auto p=std::find_if(layer.parts.begin(),layer.parts.end(),[&](const auto &v){return v.frame==command.frame;});
                check(p!=layer.parts.end() && p->image_index>=0 && p->width>0 && p->height>0,
                      "explicit original layer selection never feeds legal empty records into image lookup");
                Image image=LoadImage(common_images.at(p->image_index).string().c_str());
                check(image.data && p->source_x+p->width<=image.width && p->source_y+p->height<=image.height,
                      "all positive glow and negative-stage crops resolve actual original images within bounds");
                UnloadImage(image);
            }
        }
    }
}
// 固定原定义28/29/65分别为单格旅店、双格旅店、四格城堡。oracle直接登记a.o.ah/ai，
// 不调用被测查询或geometry生成期望；每项为frame、相对屏幕X、相对屏幕Y。
const std::array<int,3> building_definitions{28,29,65};
const std::array<std::vector<std::array<int,3>>,6> building_source_parts{{
    {{0,0,0}}, {{1,0,0}},
    {{0,30,-15},{2,0,0}}, {{1,-30,-15},{3,0,0}},
    {{0,0,-30},{2,-30,-15},{4,30,-15},{6,0,0}},
    {{1,0,-30},{3,30,-15},{5,-30,-15},{7,0,0}}
}};
void building_draw_queries() {
    auto s=test_support::world_fixture();
    for (int shape=0;shape<3;++shape) {
        const auto &d=s.rules->facilities.at(building_definitions[shape]);
        check(d.id==building_definitions[shape] && d.shape==shape,
              "literal building identities bind actual original single/pair/square definitions");
        const auto art=std::find_if(startup_evidence().displays.begin(),startup_evidence().displays.end(),
            [&](const auto &v){return v.id==d.display_id;});
        check(art!=startup_evidence().displays.end(),"source building display identity resolves actual SEB");
        for (int direction=0;direction<2;++direction) {
            const auto digest=startup_world_state_digest(s);
            const auto plan=startup_world_building_draws(s,d.id,static_cast<ref::FacilityOrientation>(direction));
            const auto &expected=building_source_parts[shape*2+direction];
            check(plan && plan->size()==expected.size(),"complete source building fragment count");
            for (std::size_t n=0;n<expected.size();++n)
                check(plan->at(n).sprite==art->sprite && plan->at(n).frame==expected[n][0] &&
                          plan->at(n).offset==std::array<int,2>{expected[n][1],expected[n][2]},
                      "three shapes/two orientations preserve literal ah/ai order, frame and pixel offsets");
            check(startup_world_state_digest(s)==digest,"building plan preserves complete Owner digest");
        }
    }
    const auto road=std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),
        [](const auto &v){return v.kind==6;});
    check(road!=s.rules->facilities.end(),"original road definition exists independently of ordinary catalog");
    for (int direction=0;direction<2;++direction) {
        const auto plan=startup_world_building_draws(s,road->id,static_cast<ref::FacilityOrientation>(direction));
        check(plan && plan->size()==1 && plan->front().frame==(direction==0?11:1) &&
                  plan->front().offset==std::array<int,2>{0,0},
              "kind6 helper selects original road11/1 rather than ordinary fragment0/1");
    }
    // 只读调用点夹具：不创建设施，不伪称自然进入移动模式，不放宽建设审批。
    s.scene.scene_state=1; s.build_definition=65;
    for (int mode:{0,7}) {
        s.build_mode=mode;
        for (int age=0;age<40;++age) {
            s.scene.scene_counter=age;
            const auto digest=startup_world_state_digest(s);
            for (const auto cursor:{ref::Position{0,0},ref::Position{s.scene.world.world.map.width-1,
                                                                   s.scene.world.world.map.height-1}})
                for (int direction=0;direction<2;++direction) {
                    const auto plan=startup_world_building_preview_draws(s,cursor,
                        static_cast<ref::FacilityOrientation>(direction));
                    check(plan && plan->size()==(age%20<10?4U:0U),
                          "ordinary/moving preview has exact20-counter blink and retains boundary-crossing fragments");
                    if (!plan->empty())
                        check(plan->front().offset==std::array<int,2>{0,-30} &&
                                  plan->at(direction==0?1:2).offset==std::array<int,2>{-30,-15},
                              "full-map cursor retains original parts lying outside map instead of suppressing candidate");
                }
            check(startup_world_state_digest(s)==digest,
                  "preview preserves complete Owner including counters, common random, cash and references");
        }
        for (const auto cursor:{ref::Position{-1,0},ref::Position{0,-1},
                               ref::Position{s.scene.world.world.map.width,0},
                               ref::Position{0,s.scene.world.world.map.height}}) {
            const auto plan=startup_world_building_preview_draws(s,cursor,ref::FacilityOrientation::first);
            check(plan && plan->empty(),"outside-map cursor is distinct from boundary-crossing footprint");
        }
        for (int age:{0,10}) {
            s.scene.scene_counter=age;
            for (int direction:{-1,2})
                check(!startup_world_building_preview_draws(s,{0,0},static_cast<ref::FacilityOrientation>(direction)),
                      "malformed orientation rejects in visible and hidden phases");
            auto broken=s; broken.build_definition=9999;
            check(!startup_world_building_preview_draws(broken,{0,0},ref::FacilityOrientation::first),
                  "unknown selected definition rejects in visible and hidden phases");
            broken=s; broken.build_definition.reset();
            check(!startup_world_building_preview_draws(broken,{0,0},ref::FacilityOrientation::first),
                  "missing selected definition cannot be concealed by blink phase");
        }
    }
    for (int mode:{1,2,3,4,5,6}) {
        s.build_mode=mode;
        const auto plan=startup_world_building_preview_draws(s,{0,0},ref::FacilityOrientation::first);
        check(plan && plan->empty(),"building preview delegates other management modes");
    }
    s.build_mode=0; s.scene.scene_state=0;
    check(startup_world_building_preview_draws(s,{0,0},ref::FacilityOrientation::first)->empty(),
          "ordinary scene suppresses stale selected construction image");
    s.scene.scene_state=1;
    auto broken=s; broken.scene.scene_counter=-1;
    check(!startup_world_building_preview_draws(broken,{0,0},ref::FacilityOrientation::first),
          "negative source scene counter rejects candidate query");
    broken=s; broken.scene.world.world.map.cells.pop_back();
    check(!startup_world_building_preview_draws(broken,{0,0},ref::FacilityOrientation::first),
          "malformed source map rejects candidate query");
    for (int fault=0;fault<5;++fault) {
        broken=s;
        StartupWorldRules private_rules=*s.rules;
        broken.rules=&private_rules;
        auto &d=private_rules.facilities.at(65);
        if (fault==0) broken.rules=nullptr;
        if (fault==1) d.shape=3;
        if (fault==2) d.display_id=9999;
        if (fault==3) private_rules.facilities.erase(private_rules.facilities.begin()+65);
        if (fault==4) d.display_id=private_rules.facilities.at(28).display_id;
        const auto digest=startup_world_state_digest(broken);
        check(!startup_world_building_draws(broken,65,ref::FacilityOrientation::first),
              "missing rules/definition/display, bad shape and inconsistent display.g explicitly reject");
        check(startup_world_state_digest(broken)==digest,"malformed private rules never mutate Owner or frozen source tables");
    }
}
// CPU-only组合器消费计划与实际PNG/SEB；期望分片独立由上面的原字面量准备。
Image building_thumbnail(const std::filesystem::path &root,
                         const std::map<int,std::filesystem::path> &images,
                         const std::vector<StartupBuildingDraw> &draws) {
    Image panel=GenImageColor(64,32,{190,242,230,255});
    for (const auto &draw:draws) {
        const auto seb=dungeon_village_tools::parse_legacy_seb(bytes(root/"image"/draw.sprite));
        if (draw.frame>=seb.frame_count) continue; // 地图缺该朝向帧是空绘，不复用frame0。
        for (const auto &layer:seb.layers)
            for (const auto &part:layer.parts) {
                if (part.frame!=draw.frame) continue;
                Image image=LoadImage(images.at(part.image_index).string().c_str());
                check(image.data && part.source_x>=0 && part.source_y>=0 && part.width>0 && part.height>0 &&
                          part.source_x+part.width<=image.width && part.source_y+part.height<=image.height &&
                          part.flip_x>=0 && part.flip_x<=1 && part.flip_y>=0 && part.flip_y<=1,
                      "real building fragment resolves valid original PNG rectangle and raw flips");
                Image crop=ImageFromImage(image,{static_cast<float>(part.source_x),static_cast<float>(part.source_y),
                    static_cast<float>(part.width),static_cast<float>(part.height)});
                if (part.flip_x) ImageFlipHorizontal(&crop);
                if (part.flip_y) ImageFlipVertical(&crop);
                ImageDraw(&panel,crop,{0,0,static_cast<float>(crop.width),static_cast<float>(crop.height)},
                    {static_cast<float>(2+draw.offset[0]+part.offset_x),
                     static_cast<float>(10+draw.offset[1]+part.offset_y),
                     static_cast<float>(crop.width),static_cast<float>(crop.height)},WHITE);
                UnloadImage(crop); UnloadImage(image);
            }
    }
    return panel;
}
void cpu_building_thumbnails(const std::filesystem::path &root) {
    auto s=test_support::world_fixture();
    const auto images=source_images(root,"image");
    for (int shape=0;shape<3;++shape) {
        const auto &d=s.rules->facilities.at(building_definitions[shape]);
        const auto art=std::find_if(startup_evidence().displays.begin(),startup_evidence().displays.end(),
            [&](const auto &v){return v.id==d.display_id;});
        const auto plan=startup_world_building_draws(s,d.id,ref::FacilityOrientation::first);
        check(plan.has_value(),"catalog fixed orientation0 has complete real building plan");
        std::vector<StartupBuildingDraw> expected;
        for (const auto &part:building_source_parts[shape*2])
            expected.push_back({art->sprite,part[0],{part[1],part[2]}});
        Image actual=building_thumbnail(root,images,*plan);
        Image oracle=building_thumbnail(root,images,expected);
        int painted{};
        for (int y=0;y<32;++y)
            for (int x=0;x<64;++x) {
                const auto a=GetImageColor(actual,x,y), b=GetImageColor(oracle,x,y);
                check(a.r==b.r && a.g==b.g && a.b==b.b && a.a==b.a,
                      "64x32 catalog composition matches literal source fragments at2,10 without scale or centering");
                painted+=a.r!=190 || a.g!=242 || a.b!=230;
            }
        check(painted>20,"all three catalog shapes contain real source pixels within64x32 clip");
        Image framed=GenImageColor(68,36,MAGENTA);
        ImageDraw(&framed,actual,{0,0,64,32},{2,2,64,32},WHITE);
        for (int y=0;y<36;++y)
            for (int x=0;x<68;++x)
                if (x<2 || x>=66 || y<2 || y>=34) {
                    const auto p=GetImageColor(framed,x,y);
                    check(p.r==255 && p.g==0 && p.b==255 && p.a==255,
                          "catalog64x32 clipping preserves surrounding row pixels");
                }
        UnloadImage(framed); UnloadImage(oracle); UnloadImage(actual);
    }
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc != 2)
            throw std::runtime_error("需要原素材根目录");
        SetTraceLogLevel(LOG_WARNING);
        owner_mapping();
        cpu_portraits(argv[1]);
        equipment_lift_queries();
        facility_growth_queries();
        cpu_equipment_and_growth(argv[1]);
        building_draw_queries();
        cpu_building_thumbnails(argv[1]);
        std::cout << "startup world visuals: " << checks << " checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
