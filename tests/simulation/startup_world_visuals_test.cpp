#include "ark/simulation/startup_world_visuals.hpp"
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
#include <stdexcept>

using namespace ark::simulation;
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
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc != 2)
            throw std::runtime_error("需要原素材根目录");
        SetTraceLogLevel(LOG_WARNING);
        owner_mapping();
        cpu_portraits(argv[1]);
        std::cout << "startup world visuals: " << checks << " checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
