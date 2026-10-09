#include "ark/simulation/startup_world_visuals.hpp"
#include "ark/simulation/startup_world_persistence.hpp"
#include "ark/assets/sprite.hpp"
#include "ark/assets/table.hpp"
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

using namespace ark::simulation;
// 应用皮肤拥有独立源包；仍复用本套件的真实PNG/SEB与CPU图像生命周期。
int check_startup_skin(const std::filesystem::path &root, const std::filesystem::path &output);
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
    const auto seb = ark::assets::parse_legacy_seb(bytes(root / "human/walk01.seb"));
    check(seb.layers.size() == 1 && seb.frame_count == 4, "published source portrait SEB schema");
    const auto &p = seb.layers.front().parts.front();
    check(p.frame == 0 && p.source_x == 0 && p.source_y == 24 && p.width == 18 && p.height == 24 &&
              p.offset_x == -9 && p.offset_y == -24 && p.flip_x == 0 && p.flip_y == 0,
          "walk01 frame0 is exact18x24 body, not guessed independent portrait tile");
    std::map<int, std::filesystem::path> images;
    for (const auto &row : ark::assets::parse_tsv(bytes(root / "human/img.inf"))) {
        auto path = std::filesystem::path(row.at(1));
        path.replace_extension(".png");
        images.emplace(ark::assets::parse_table_integer(row.at(0)), path);
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
void attribute_gain_queries() {
    auto s = lift_fixture();
    auto &ai = s.scene.world.world.ai;
    auto &display = ai.contexts.at({1}).effects.display;
    int measurements{};
    const auto measure = [&](const std::string &text) {
        ++measurements;
        return text[0] == '-' || (text[0] >= '0' && text[0] <= '9')
            ? static_cast<int>(text.size()) * 6 : 24;
    }; // 仅字体测量夹具；24/6不是原平台动态实测。
    constexpr std::array<const char *,6> names{"体力","力量","灵活","结实","魔力","运气"};
    constexpr std::array<int,6> icon_x{0,16,32,48,64,96};
    for (int attribute = 0; attribute < 6; ++attribute)
        for (int age : {0,1,6,20,39,40}) {
            display = {{13,age,attribute,7}};
            const auto digest = startup_world_state_digest(s);
            const auto plan = startup_world_attribute_gain_draws(s,{1},measure);
            check(plan && plan->size() == 1 && plan->front().record_index == 0 &&
                      plan->front().age == age && plan->front().attribute == attribute &&
                      plan->front().delta == 7 && plan->front().width == 60 &&
                      plan->front().text == names[attribute] &&
                      plan->front().text_rgb == std::array<int,3>{0,100,255} &&
                      plan->front().text_offset == std::array<int,2>{-12,-37},
                  "cd13 six source attributes have fixed Y-40 panel rather than age-dependent bounce");
            const auto &before = plan->front().before_text;
            const auto &after = plan->front().after_text;
            check(before.size() == 4 && before[0].image == 7 && before[0].sprite == -1 &&
                      before[0].crop == std::array<int,4>{6,0,60,21} &&
                      before[0].offset == std::array<int,2>{-30,-40} &&
                      before[1].sprite == 28 && before[1].frame == 0 &&
                      before[1].offset == std::array<int,2>{-36,-40} &&
                      before[2].sprite == 28 && before[2].frame == 1 &&
                      before[2].offset == std::array<int,2>{30,-40} &&
                      before[3].image == 37 && before[3].crop == std::array<int,4>{icon_x[attribute],16,16,16} &&
                      before[3].offset == std::array<int,2>{-30,-39} &&
                      after.size() == 2 && after[0].sprite == 15 && after[0].frame == 7 &&
                      after[0].offset == std::array<int,2>{22,-36} && after[1].frame == 14 &&
                      after[1].offset == std::array<int,2>{14,-36},
                  "exact original background/caps and luck icon96,16 precede text then blue digit and plus");
            check(startup_world_state_digest(s) == digest,
                  "attribute render preserves complete Owner including cumulative stats, HP, cd, pages and random");
        }
    for (int age : {-6,-1}) {
        display = {{13,age,2,1}};
        const int old_measurements = measurements;
        check(startup_world_attribute_gain_draws(s,{1},measure)->empty() && measurements == old_measurements,
              "negative delay hides pixels without measuring or advancing delayed cd13");
    }
    display = {{13,-1,2,1}};
    const auto ready = ref::advance_actor_effects(ai.contexts.at({1}).effects);
    check(ready.candidate && ready.candidate->state.display.front()[1] == 0 && display.front()[1] == -1,
          "only existing logical effect advancement releases delay and leaves input Owner unchanged");
    display = {{13,39,2,1}};
    const auto retired = ref::advance_actor_effects(ai.contexts.at({1}).effects);
    check(retired.candidate && retired.candidate->state.display.empty() &&
              retired.candidate->removed_display_indices == std::vector<std::size_t>{0},
          "40 gate belongs to logical retirement; read-only renderer never removes record");
    for (int delta : {0, -1, -1234, std::numeric_limits<int>::min()}) {
        display = {{13,0,0,delta}};
        const auto plan = startup_world_attribute_gain_draws(s,{1},measure);
        check(plan && plan->front().delta == delta && plan->front().after_text.back().frame == 14 &&
                  std::all_of(plan->front().after_text.begin(),plan->front().after_text.end(),
                    [](const auto &v) { return v.sprite == 15 && v.frame >= 0 && v.frame <= 14; }),
              "signed/zero/INT_MIN values preserve raw measurement and original plus without guessed minus or abs overflow");
        if (delta == -1234) {
            const auto &numbers = plan->front().after_text;
            constexpr std::array<int,6> frames{1,2,10,3,4,14};
            constexpr std::array<int,6> x{-2,6,4,14,22,-18};
            check(numbers.size() == frames.size(),"negative1234 keeps empty minus cell and one comma overlay");
            for (std::size_t n = 0; n < frames.size(); ++n)
                check(numbers[n].frame == frames[n] && numbers[n].offset == std::array<int,2>{x[n],-36},
                      "source number glyph order is digit then comma overlay; plus position uses raw signed width");
        }
        if (delta == std::numeric_limits<int>::min())
            check(plan->front().after_text.size() == 14,"INT_MIN materializes ten digits, three commas and original plus");
    }
    display = {{13,0,0,1}};
    const auto wide = startup_world_attribute_gain_draws(s,{1},[](const std::string &text) {
        return text == "体力" ? 25 : 6;
    });
    check(wide && wide->front().width == 61 && wide->front().before_text.size() == 7 &&
              wide->front().before_text[0].crop == std::array<int,4>{6,0,27,21} &&
              wide->front().before_text[1].offset == std::array<int,2>{-3,-40} &&
              wide->front().before_text[2].crop == std::array<int,4>{6,0,7,21} &&
              wide->front().before_text[3].crop == std::array<int,4>{33,0,5,21} &&
              wide->front().before_text[3].offset == std::array<int,2>{-2,-40},
          "width60/61 switches to original27px repeat plus centered5px tail without stretching");
    display = {{13,0,0,1},{21,12,218,-18,13},{13,0,5,0},{13,-1,2,3},{13,0,0,1}};
    const auto mixed = startup_world_attribute_gain_draws(s,{1},measure);
    const auto lifts = startup_world_equipment_lift_draws(s,{1});
    check(mixed && mixed->size() == 3 && mixed->at(0).record_index == 0 &&
              mixed->at(1).record_index == 2 && mixed->at(2).record_index == 4 && lifts &&
              lifts->size() == 2 && lifts->at(0).record_index == 1 && lifts->at(1).record_index == 1,
          "mixed cd13/21 plans retain source indices and duplicates for caller-order merge");
    for (const auto &bad : {ref::ActorEffectRecord{13,0,0}, {13,0,-1,1}, {13,-1,6,1},
                           {13,std::numeric_limits<int>::max(),0,1}}) {
        display = {bad};
        check(!startup_world_attribute_gain_draws(s,{1},measure),
              "missing cd13 payload, invalid attribute even hidden and age overflow reject explicitly");
    }
    display = {{13,0,0,1}};
    check(!startup_world_attribute_gain_draws(s,{99},measure) &&
              !startup_world_attribute_gain_draws(s,{1},{}) &&
              !startup_world_attribute_gain_draws(s,{1},[](const std::string &) {return -1;}) &&
              !startup_world_attribute_gain_draws(s,{1},[](const std::string &) {return std::numeric_limits<int>::max();}) &&
              !startup_world_attribute_gain_draws(s,{1},[](const std::string &v) {
                  return v == "体力" ? 24 : std::numeric_limits<int>::max();
              }),
          "stale actor, missing metric source, negative/oversized widths and plus-position overflow reject");
    ai.battle.actors.at({1}).kind = ref::ActorKind::monster;
    check(!startup_world_attribute_gain_draws(s,{1},measure),"human property label cannot be assigned to monster fixture");
    ai.battle.actors.at({1}).kind = ref::ActorKind::human;
    ai.battle.actors.at({1}).control.flags = 1;
    check(startup_world_attribute_gain_draws(s,{1},measure)->empty(),"original visibility bit1 suppresses cd13 with actor");
    ai.battle.actors.at({1}).control.flags = 0;
    ai.contexts.erase({1});
    check(!startup_world_attribute_gain_draws(s,{1},measure),"missing effect Owner reference rejects attribute query");
}
std::map<int,std::filesystem::path> source_images(const std::filesystem::path &root, const char *group) {
    std::map<int,std::filesystem::path> result;
    for (const auto &row : ark::assets::parse_tsv(bytes(root/group/"img.inf"))) {
        auto p = std::filesystem::path(row.at(1));
        if (p.extension()==".gif") p.replace_extension(".png");
        result.emplace(ark::assets::parse_table_integer(row.at(0)),root/group/p);
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
                const auto seb=ark::assets::parse_legacy_seb(bytes(root/"weapon"/filename));
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
        const auto seb=ark::assets::parse_legacy_seb(bytes(root/"common"/
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
void cpu_attribute_gains(const std::filesystem::path &root) {
    auto s = lift_fixture();
    const auto images = source_images(root,"common");
    const auto borders = ark::assets::parse_legacy_seb(bytes(root/"common/fukidashi_back.seb"));
    const auto numbers = ark::assets::parse_legacy_seb(bytes(root/"common/number08.seb"));
    check(borders.layers.size() == 1 && borders.layers[0].parts.size() == 2 &&
              borders.layers[0].parts[0].image_index == 7 && borders.layers[0].parts[0].width == 6 &&
              borders.layers[0].parts[1].image_index == 7 && borders.layers[0].parts[1].source_x == 65 &&
              borders.layers[0].parts[1].width == 7 && numbers.frame_count == 21,
          "real common28 caps and common15 numeric SEB match original source identities");
    for (int attribute = 0; attribute < 6; ++attribute)
        for (int width : {24,25}) {
            s.scene.world.world.ai.contexts.at({1}).effects.display = {{13,0,attribute,-1234}};
            const auto plan = startup_world_attribute_gain_draws(s,{1},[&](const std::string &text) {
                return text == "-1234" ? 30 : width;
            });
            check(plan.has_value(),"CPU attribute resource resolves complete source draw plan");
            const auto crop = [&](int image_id, std::array<int,4> rect) {
                Image image = LoadImage(images.at(image_id).string().c_str());
                check(image.data && rect[0]>=0 && rect[1]>=0 && rect[2]>0 && rect[3]>0 &&
                          rect[0]+rect[2]<=image.width && rect[1]+rect[3]<=image.height,
                      "all six attributes, both background branches and signed glyph crops fit decoded real PNG");
                Image part = ImageFromImage(image,{static_cast<float>(rect[0]),static_cast<float>(rect[1]),
                    static_cast<float>(rect[2]),static_cast<float>(rect[3])});
                check(part.data && part.width == rect[2] && part.height == rect[3],
                      "CPU crop materializes source attribute/background/glyph pixels without window or fabricated icon");
                UnloadImage(part); UnloadImage(image);
            };
            const auto consume = [&](const auto &commands) {
                for (const auto &command : commands) {
                    check(command.record_index == 0,"every attribute image command preserves originating cd index");
                    if (command.sprite < 0) crop(command.image,command.crop);
                    else {
                        const auto &seb = command.sprite == 28 ? borders : numbers;
                        const auto &parts = seb.layers.at(command.layer).parts;
                        const auto p = std::find_if(parts.begin(),parts.end(),[&](const auto &v) {
                            return v.frame == command.frame;
                        });
                        check(p != parts.end() && p->image_index >= 0,"each nonempty numeric/cap frame has actual original SEB record");
                        crop(p->image_index,{p->source_x,p->source_y,p->width,p->height});
                    }
                }
            };
            consume(plan->front().before_text); consume(plan->front().after_text);
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
        const auto seb=ark::assets::parse_legacy_seb(bytes(root/"image"/draw.sprite));
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
void ordinary_item_icons(const std::filesystem::path &root) {
    auto s=test_support::world_fixture();
    const auto digest=startup_world_state_digest(s);
    // 原item列5字面量；不能使用被测查询或业务ID生成期望。
    constexpr std::array<int,36> icons{{5,27,13,23,20,22,35,19,16,42,65,50,56,55,49,30,37,39,
                                      60,77,36,61,14,75,40,34,59,51,52,78,79,80,82,81,83,84}};
    check(s.rules->items.size()==icons.size(),"all36 original item definitions remain distinct from atlas slots");
    const auto images=source_images(root,"common");
    Image foreground=LoadImage(images.at(9).string().c_str());
    Image background=LoadImage(images.at(24).string().c_str());
    check(foreground.data && foreground.width==240 && foreground.height==96 &&
              background.data && background.width==144 && background.height==18,
          "type1 resolves actual PNG9 and PNG24, not same-numbered SEB or inferred item image");
    for(std::size_t id=0;id<icons.size();++id) {
        check(s.rules->items[id].identity==static_cast<int>(id) && s.rules->items[id].render_icon==icons[id],
              "startup projection retains exact original g.g for each item");
        const auto plan=startup_world_item_icon_draws(s,static_cast<int>(id));
        check(plan && plan->size()==2 && plan->at(0).image==24 && plan->at(1).image==9 &&
                  plan->at(0).offset==std::array<int,2>{-1,-1} &&
                  plan->at(1).offset==std::array<int,2>{0,0} &&
                  plan->at(1).crop==std::array<int,4>{icons[id]%15*16,icons[id]/15*16,16,16} &&
                  !plan->at(0).record_index && !plan->at(1).record_index,
              "ordinary gift/facility/commerce icon retains ordered background and foreground with original offsets");
        for(const auto &p:*plan) {
            const auto &img=p.image==9?foreground:background;
            check(p.crop[0]>=0 && p.crop[1]>=0 && p.crop[0]+p.crop[2]<=img.width &&
                      p.crop[1]+p.crop[3]<=img.height && p.sprite==-1,
                  "every original item crop is inside decoded source PNG");
        }
    }
    for(const auto sample:std::array<std::array<int,2>,4>{{{{0,18}},{{20,72}},{{25,108}},{{29,36}}}}) {
        const auto plan=startup_world_item_icon_draws(s,sample[0]);
        check(plan && plan->front().crop==std::array<int,4>{sample[1],0,18,18},
              "literal samples cover all four ordinary icon background categories");
    }
    for(int id:{-1,36,9999})check(!startup_world_item_icon_draws(s,id),"unknown item identity rejects explicitly");
    auto broken=s;broken.rules=nullptr;
    check(!startup_world_item_icon_draws(broken,0),"missing static rules rejects icon query");
    StartupWorldRules private_rules=*s.rules;broken=s;broken.rules=&private_rules;
    for(int icon:{-1,89,std::numeric_limits<int>::max()}) {
        private_rules.items[0].render_icon=icon;
        check(!startup_world_item_icon_draws(broken,0),"invalid raw icon cannot index classification or clamp to default");
    }
    private_rules.items[0].render_icon=14;
    const auto changed=startup_world_item_icon_draws(broken,0);
    check(changed && changed->front().crop[0]==72 && changed->back().crop[0]==224,
          "icon field, not unchanged definition or business category, controls the type1 picture");
    check(startup_world_state_digest(s)==digest,"item icon queries preserve full Owner, inventory, cash, random and page stack");
    UnloadImage(foreground);UnloadImage(background);
}
void facility_detail_icons(const std::filesystem::path &root) {
    auto s = test_support::world_fixture();
    const auto digest = startup_world_state_digest(s);
    // 固定原表列2独立oracle；不是调用绘制查询或从原类kind猜类别图标。
    constexpr std::array<int,85> icons{{
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,5,5,5,0,0,
        1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
        4,4,4,4,4,4,4,6,6,6,6,6,6,6,6,6,0,0,0,0,0,0,0,0,0,0}};
    Image image = LoadImage((root/"common/icon_tenantInfo.png").string().c_str());
    Image params = LoadImage((root/"common/icon_param00.png").string().c_str());
    Image numbers = LoadImage((root/"common/number08.png").string().c_str());
    const auto seb = ark::assets::parse_legacy_seb(bytes(root/"common/number08.seb"));
    const auto plus = std::find_if(seb.layers.at(0).parts.begin(),seb.layers.at(0).parts.end(),
        [](const auto &p) {return p.frame == 14;});
    check(image.data && image.width == 112 && image.height == 16 && params.data && numbers.data &&
              plus != seb.layers.at(0).parts.end() && plus->image_index == 105 &&
              plus->source_x == 44 && plus->source_y == 10 && plus->width == 8 && plus->height == 10,
          "real facility category atlas, attribute atlas and blue+ SEB bind actual PNG identities");
    int effect_rows{}, plus_count{};
    for (int id = 0; id < 85; ++id) {
        const auto icon = startup_world_facility_icon_draw(s,id);
        check(icon && s.rules->facilities.at(id).legacy_icon == icons[id] &&
                  icon->image == 91 && icon->sprite == -1 &&
                  icon->crop == std::array<int,4>{icons[id]*16,0,16,16} &&
                  icon->offset == std::array<int,2>{0,0} && !icon->record_index,
              "all85 facility IDs bind original type9 crop; icon0 valid and type1 background absent");
        Image crop = ImageFromImage(image,{static_cast<float>(icon->crop[0]),0,16,16});
        check(crop.data && crop.width == 16 && crop.height == 16,
              "every category0..6 CPU crop uses decoded source image with no guessed frame");
        UnloadImage(crop);
        const auto effects = startup_world_facility_exit_effect_draws(s,id);
        check(effects && effects->size() == s.rules->facilities.at(id).exit_effects.size(),
              "all85 effect plans preserve z-prefix rows, including legitimate empty definition");
        for (const auto &row : *effects) {
            const auto &r = row.icon.crop;
            check(row.slot == static_cast<std::size_t>(&row-effects->data()) && row.attribute >= 0 &&
                      row.attribute < 6 && r[0]+r[2]<=params.width && r[1]+r[3]<=params.height &&
                      row.icon.offset == std::array<int,2>{136,127+17*static_cast<int>(row.slot)},
                  "every original effect row uses actual attribute crop and original17px page anchor");
            check(row.pluses.size() == static_cast<std::size_t>(row.delta),
                  "source positive effect has literal delta count of + glyphs rather than digit+value helper");
            for (const auto &draw : row.pluses)
                check(draw.sprite == 15 && draw.frame == 14 && !draw.record_index &&
                          plus->source_x+plus->width<=numbers.width && plus->source_y+plus->height<=numbers.height,
                      "each plus command resolves source SEB15/frame14 in actual PNG bounds");
            ++effect_rows; plus_count += static_cast<int>(row.pluses.size());
        }
    }
    check(effect_rows == 45 && plus_count == 87,"all original45 attribute slots/87+ consumed; raw tails remain separate");
    constexpr std::array<int,6> x{{0,16,32,48,64,96}};
    for (int id = 0; id < 6; ++id) {
        const auto icon = startup_world_attribute_icon_draw(id);
        check(icon && icon->image == 37 && icon->crop == std::array<int,4>{x[id],16,16,16} &&
                  icon->offset == std::array<int,2>{0,0},"type7 attributes include source luck96 rather than regular80");
    }
    for (int id : {-1,6,std::numeric_limits<int>::max()})
        check(!startup_world_attribute_icon_draw(id),"invalid attribute is not modulo-wrapped into valid icon");
    for (int id : {54,56,57,58}) {
        const auto rows = startup_world_facility_exit_effect_draws(s,id);
        check(rows && rows->size() == 1 && rows->front().attribute == 5 && rows->front().delta == 3 &&
                  rows->front().pluses.size() == 3 && s.rules->unconsumed_exit_deltas.at(id) == std::vector<int>{3},
              "four legal z/A mismatches keep unused tail and draw exactly one luck row with three+ glyphs");
    }
    const auto two = startup_world_facility_exit_effect_draws(s,59);
    check(two && two->size() == 2 && two->at(0).attribute == 1 && two->at(0).delta == 2 &&
              two->at(1).attribute == 3 && two->at(1).delta == 1 &&
              two->at(0).pluses.at(0).offset == std::array<int,2>{192,130} &&
              two->at(0).pluses.at(1).offset == std::array<int,2>{184,130} &&
              two->at(1).icon.offset == std::array<int,2>{136,144} &&
              two->at(1).pluses.at(0).offset == std::array<int,2>{192,147},
          "literal training59 strength2/solid1 keeps original row order and right-to-left plus positions");
    for (int id : {-1,85,std::numeric_limits<int>::max()})
        check(!startup_world_facility_icon_draw(s,id) && !startup_world_facility_exit_effect_draws(s,id),
              "unknown facility definition rejects icon and effects rather than returning empty identity");
    auto broken=s;broken.rules=nullptr;
    check(!startup_world_facility_icon_draw(broken,0) && !startup_world_facility_exit_effect_draws(broken,0),
          "missing rules rejects detail icon and effect plan");
    StartupWorldRules rules=*s.rules;broken=s;broken.rules=&rules;
    for (int icon : {-1,7,10,std::numeric_limits<int>::max()}) {
        rules.facilities[0].legacy_icon=icon;
        check(!startup_world_facility_icon_draw(broken,0),"bad facility icon cannot modulo-wrap or index missing PNG column");
    }
    rules.facilities[0].legacy_icon=6;
    check(startup_world_facility_icon_draw(broken,0)->crop[0] == 96,
          "facility legacy_icon field controls category image independently of unchanged definitionID");
    rules.facilities[33].exit_effects={{2,0},{2,-3},{5,2}};
    auto rows=startup_world_facility_exit_effect_draws(broken,33);
    check(rows && rows->size() == 3 && rows->at(0).pluses.empty() && rows->at(1).pluses.empty() &&
              rows->at(1).delta == -3 && rows->at(2).pluses.size() == 2,
          "duplicate attribute slots retain raw signed values; zero/negative original loop draws no invented sign");
    rules.facilities[33].exit_effects={{6,0}};
    check(!startup_world_facility_exit_effect_draws(broken,33),"invalid attribute rejects even when no+ would be drawn");
    rules.facilities[33].exit_effects={{0,std::numeric_limits<int>::max()}};
    check(!startup_world_facility_exit_effect_draws(broken,33),"oversized bad private output rejects before allocating billions of+ commands");
    rules.facilities[33].exit_effects.assign(4097,{0,0});
    check(!startup_world_facility_exit_effect_draws(broken,33),"malformed effect list exceeds explicit maintenance output budget");
    check(startup_world_state_digest(s) == digest,
          "all facility/attribute icon and effect queries preserve complete Owner including shared uses, economy and random");
    UnloadImage(image);UnloadImage(params);UnloadImage(numbers);
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc != 2 && argc != 3)
            throw std::runtime_error("需要原素材根目录，可另给研究皮肤CPU拼图输出路径");
        SetTraceLogLevel(LOG_WARNING);
        checks += check_startup_skin(argv[1], argc == 3 ? std::filesystem::path(argv[2])
                                                      : std::filesystem::path{});
        owner_mapping();
        cpu_portraits(argv[1]);
        equipment_lift_queries();
        facility_growth_queries();
        attribute_gain_queries();
        cpu_equipment_and_growth(argv[1]);
        cpu_attribute_gains(argv[1]);
        building_draw_queries();
        cpu_building_thumbnails(argv[1]);
        ordinary_item_icons(argv[1]);
        facility_detail_icons(argv[1]);
        std::cout << "startup world visuals: " << checks << " checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
