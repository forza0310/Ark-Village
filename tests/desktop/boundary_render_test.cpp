// Static BOUNDARY contract against published reset cells and real PNG/SEB assets. No window.
#include "ark/app/game.hpp"
#include "ark/assets/sprite.hpp"
#include "boundary_render.hpp"
#include <raylib.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
int checks{};
void check(bool ok, const char *why) {
    ++checks;
    if (!ok)
        throw std::runtime_error(why);
}
template <typename F> void rejected(F action) {
    try {
        action();
    } catch (const std::invalid_argument &) {
        check(true, "invalid input rejected");
        return;
    }
    check(false, "invalid boundary input must fail explicitly");
}
ark::assets::SpriteDefinition load(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    check(static_cast<bool>(stream), "packaged SEB readable");
    return ark::assets::parse_legacy_seb(
        {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()});
}
struct CpuImage {
    Image value;
    explicit CpuImage(const std::filesystem::path &path) : value(LoadImage(path.string().c_str())) {
        check(value.data != nullptr, "packaged boundary PNG decodes");
    }
    ~CpuImage() { UnloadImage(value); }
    CpuImage(const CpuImage &) = delete;
    CpuImage &operator=(const CpuImage &) = delete;
};
void mappings() {
    using ark::desktop::boundary_overlays;
    ark::world::LoadedCell cell;
    check(boundary_overlays(cell, 0, 0, 0, true).empty(), "unmarked cell draws no boundary");
    const int xy[6][2]{{13, 22}, {16, 22}, {13, 21}, {42, 22}, {15, 23}, {16, 9}};
    for (int index = 0; index < 3; ++index)
        for (int frame = 0; frame < 6; ++frame)
            for (int flags : {0, 1, 2, 3})
                for (int depth : {-17, 0, 23}) {
                    cell.boundary_fragment = frame;
                    const auto commands = boundary_overlays(cell, index, flags, depth, true);
                    check(commands.size() == 1,
                          "one fragment means one command, including corners");
                    const auto &c = commands.front();
                    check(std::string(c.sprite) == "fence01" + std::to_string(index) + ".seb" &&
                              c.frame == frame && c.offset_x == xy[frame][0] &&
                              c.offset_y == xy[frame][1] &&
                              c.depth_offset == ((flags & 1) ? 10 : 75) + depth,
                          "all skins keep exact frame/anchor and current display-dependent depth");
                    check(boundary_overlays(cell, index, flags, depth, false).empty(),
                          "ineligible display or viewport suppresses all overlays");
                }
    const int door_xy[4][2]{{15, 24}, {26, 19}, {15, 18}, {28, 24}};
    cell.boundary_fragment = -1;
    for (int direction = 0; direction < 4; ++direction) {
        cell.external_direction = direction;
        const auto commands = boundary_overlays(cell, 0, 1, 99, true);
        check(commands.size() == 1 && std::string(commands[0].sprite) == "door00.seb" &&
                  commands[0].frame == direction / 2 &&
                  commands[0].offset_x == door_xy[direction][0] &&
                  commands[0].offset_y == door_xy[direction][1] &&
                  commands[0].depth_offset == door_xy[direction][1],
              "external pillar uses its own direction/frame/depth, never ground base depth");
    }
    cell.boundary_fragment = 2;
    const auto both = boundary_overlays(cell, 0, 0, 0, true);
    check(both.size() == 2 && std::string(both[0].sprite) == "fence010.seb" &&
              std::string(both[1].sprite) == "door00.seb",
          "independent original branches preserve fence-before-pillar submission");
    for (int index : {-1, 3})
        rejected([&] { boundary_overlays(cell, index, 0, 0, true); });
    for (int fragment : {-2, 6}) {
        cell.boundary_fragment = fragment;
        rejected([&] { boundary_overlays(cell, 0, 0, 0, true); });
    }
    cell.boundary_fragment = 0;
    for (int direction : {-2, 4}) {
        cell.external_direction = direction;
        rejected([&] { boundary_overlays(cell, 0, 0, 0, true); });
    }
    cell.external_direction = -1;
    rejected([&] { boundary_overlays(cell, 0, 0, std::numeric_limits<int>::max(), true); });
    check(boundary_overlays(cell, -1, 0, 0, false).empty(),
          "caller-ineligible cell never submits or consumes unused region index");
}

// Verify reset distribution and access states, so visual acceptance cannot hide a missing
// logical boundary or replace real entrance gaps with a uniform decorative ring.
void reset_cells() {
    using namespace ark;
    const auto &startup = app::startup_data();
    check(startup.boundary_index == 0, "reset region index selects original low wood fence skin");
    app::Game game;
    const auto routes = game.route_map();
    std::array<int, 6> counts{};
    int pillars{};
    for (std::size_t i = 0; i < startup.loaded_cells.size(); ++i) {
        const auto &cell = startup.loaded_cells[i];
        if (cell.boundary_fragment >= 0) {
            ++counts.at(cell.boundary_fragment);
            check(cell.legacy_state == 5 && cell.category == world::RouteCategory::blocked &&
                      routes.cells[i].legacy_state == 5 &&
                      routes.cells[i].category == world::RouteCategory::blocked,
                  "every visible reset fence keeps blocked logical state in current routing");
            check(!world::route_transition({4, world::RouteCategory::ground, 17, -1, {}},
                                           routes.cells[i], true),
                  "even first-step escape cannot enter a fence tile");
        }
        if (cell.external_direction >= 0) {
            ++pillars;
            check(cell.boundary_fragment == -1 && cell.legacy_state == 7 &&
                      cell.category == world::RouteCategory::access,
                  "external pillars retain an access gap instead of fence blocking");
        }
    }
    check(counts == std::array<int, 6>{14, 16, 1, 1, 1, 1} && pillars == 2,
          "published first village has 34 fences and two external pillars");
    const world::Cell corners[4]{{17, 10}, {6, 2}, {6, 10}, {17, 2}};
    for (int n = 0; n < 4; ++n)
        check(startup.loaded_cells[startup.map.index(corners[n])].boundary_fragment == n + 2,
              "map corners retain their original nonsymmetric fragment numbering");
    for (world::Cell entrance : {world::Cell{11, 2}, {12, 2}, {11, 10}, {12, 10}})
        check(startup.loaded_cells[startup.map.index(entrance)].boundary_fragment == -1,
              "four real entrance cells have no fence overlay");
    check(startup.loaded_cells[startup.map.index({11, 10})].external_direction == 2 &&
              startup.loaded_cells[startup.map.index({12, 10})].external_direction == 3,
          "reset consumes external directions two and three");
}

struct ExpectedPart {
    int x, y, width, height, offset_x, offset_y;
};
void assets(const std::filesystem::path &root) {
    const ExpectedPart fence[3][6]{{{0, 99, 32, 25, 0, -23},
                                    {32, 99, 32, 25, 0, -23},
                                    {65, 102, 5, 22, 0, -22},
                                    {65, 100, 5, 24, 0, -24},
                                    {71, 111, 37, 13, -5, -9},
                                    {71, 111, 28, 13, 0, -11}},
                                   {{0, 7, 32, 33, 0, -31},
                                    {32, 7, 32, 33, 0, -31},
                                    {65, 9, 5, 31, 0, -30},
                                    {65, 7, 5, 33, 0, -32},
                                    {71, 22, 31, 19, 0, -17},
                                    {71, 22, 28, 19, 0, -19}},
                                   {{0, 48, 32, 37, 0, -36},
                                    {32, 48, 32, 37, 0, -36},
                                    {65, 49, 5, 37, 0, -35},
                                    {65, 47, 5, 39, 0, -40},
                                    {71, 62, 31, 25, 0, -24},
                                    {71, 62, 31, 25, 0, -24}}};
    const CpuImage image(root / "common/fence01.png");
    check(image.value.width == 135 && image.value.height == 124, "real fence atlas dimensions");
    for (int index = 0; index < 3; ++index) {
        const auto seb = load(root / ("common/fence01" + std::to_string(index) + ".seb"));
        check(seb.frame_count == 6 && seb.layers.size() == 1 && seb.layers[0].parts.size() == 6,
              "every skin consists of exactly six single-record frames");
        for (int frame = 0; frame < 6; ++frame) {
            const auto &part = seb.layers[0].parts[frame];
            const auto &expected = fence[index][frame];
            check(part.frame == frame && part.image_index == 64 && part.flip_x == 0 &&
                      part.flip_y == 0 && part.source_x == expected.x &&
                      part.source_y == expected.y && part.width == expected.width &&
                      part.height == expected.height && part.offset_x == expected.offset_x &&
                      part.offset_y == expected.offset_y,
                  "all 18 original crop/offset records retained, including asymmetric corners");
            check(part.source_x >= 0 && part.source_y >= 0 && part.width > 0 && part.height > 0 &&
                      part.source_x + part.width <= image.value.width &&
                      part.source_y + part.height <= image.value.height,
                  "all fence frame rectangles lie in real decoded atlas");
            int visible_pixels{};
            for (int y = 0; y < part.height; ++y)
                for (int x = 0; x < part.width; ++x)
                    visible_pixels +=
                        GetImageColor(image.value, part.source_x + x, part.source_y + y).a != 0;
            check(visible_pixels > 0, "each requested frame contains visible source pixels");
        }
    }
    const CpuImage door(root / "common/door00.png");
    const auto seb = load(root / "common/door00.seb");
    check(seb.frame_count == 2 && seb.layers.size() == 1 && seb.layers[0].parts.size() == 2,
          "real external pillar has exactly two frames");
    for (int frame = 0; frame < 2; ++frame) {
        const auto &part = seb.layers[0].parts[frame];
        check(part.frame == frame && part.image_index == 5 && part.source_x == (frame ? 1 : 21) &&
                  part.source_y == 1 && part.width == 19 && part.height == 30 &&
                  part.offset_x == 0 && part.offset_y == -30 && part.flip_x == 0 &&
                  part.flip_y == 0 && part.source_x + part.width <= door.value.width &&
                  part.source_y + part.height <= door.value.height,
              "external pillar crop/binding/offset and decoded image bounds");
    }
}
} // namespace

int main(int argc, char **argv) {
    try {
        SetTraceLogLevel(LOG_WARNING);
        check(argc == 2, "asset root required");
        mappings();
        reset_cells();
        assets(argv[1]);
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
