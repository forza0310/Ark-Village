// Product adaptation of maintained original-PNG composition, plus current terrain checks.
// CPU-only composition of the actual PNGs; no window or renderer-equivalence claim.
#include "ark/app/game.hpp"
#include "ark/assets/sprite.hpp"
#include "road_render.hpp"

#include <raylib.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

using namespace ark::desktop;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
struct CpuImage {
    Image value;
    explicit CpuImage(const std::filesystem::path &path) : value(LoadImage(path.string().c_str())) {
        if (!value.data)
            throw std::runtime_error("PNG decode failed");
    }
    explicit CpuImage(Image image) : value(image) {
        if (!value.data)
            throw std::runtime_error("CPU image allocation failed");
    }
    ~CpuImage() { UnloadImage(value); }
    CpuImage(const CpuImage &) = delete;
    CpuImage &operator=(const CpuImage &) = delete;
};
bool same(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }
void compose(const std::filesystem::path &root) {
    check(!road_patch({0, 0}, true, false, false), "unmarked tile has no patch");
    const auto quad = *road_patch({0, 1}, true, true, true);
    check(quad.width == 30 && quad.height == 20 && quad.offset_x == 14 && quad.offset_y == 21 &&
              quad.depth_offset == -10,
          "quad exact PNG parameters");
    const auto edge = *road_patch({0, 0}, true, false, true);
    check(edge.width == 27 && edge.height == 15 && edge.offset_x == 20 && edge.offset_y == 19 &&
              edge.depth_offset == -10,
          "edge exact PNG parameters");
    check(!road_patch({0, 0}, false, true, true), "hidden base suppresses patch");
    CpuImage road(root / "image/road00.png");
    std::ifstream stream(root / "image/road00.seb", std::ios::binary);
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(stream),
                                          std::istreambuf_iterator<char>()};
    const auto seb = ark::assets::parse_legacy_seb(bytes);
    CpuImage canvas(GenImageColor(160, 120, Color{32, 160, 32, 255}));
    // D(x,y) = (30*(x+y), 15*(x-y)-15) plus this isolated canvas origin (20,60).
    // These masks use only the four cells, rather than map-edge implicit connections.
    const int frames[2][2]{{3, 9}, {5, 4}};
    for (int y = 1; y >= 0; --y) {
        for (int x = 0; x < 2; ++x) {
            const int dx = 20 + 30 * (x + y), dy = 60 + 15 * (x - y) - 15;
            int parts = 0;
            for (const auto &layer : seb.layers) {
                for (const auto &part : layer.parts) {
                    if (part.frame != frames[y][x])
                        continue;
                    check(part.image_index == 10 && part.source_x >= 0 && part.source_y >= 0 &&
                              part.width > 0 && part.height > 0 &&
                              part.source_x + part.width <= road.value.width &&
                              part.source_y + part.height <= road.value.height &&
                              part.flip_x >= 0 && part.flip_x <= 1 && part.flip_y >= 0 &&
                              part.flip_y <= 1,
                          "requested road frame rectangle/binding strictly valid");
                    CpuImage piece(ImageFromImage(road.value, {static_cast<float>(part.source_x),
                                                               static_cast<float>(part.source_y),
                                                               static_cast<float>(part.width),
                                                               static_cast<float>(part.height)}));
                    if (part.flip_x)
                        ImageFlipHorizontal(&piece.value);
                    if (part.flip_y)
                        ImageFlipVertical(&piece.value);
                    ImageDraw(
                        &canvas.value, piece.value,
                        {0, 0, static_cast<float>(part.width), static_cast<float>(part.height)},
                        {static_cast<float>(dx + part.offset_x),
                         static_cast<float>(dy + part.offset_y), static_cast<float>(part.width),
                         static_cast<float>(part.height)},
                        WHITE);
                    ++parts;
                }
            }
            check(parts > 0, "all four real road frames were drawn");
        }
    }
    CpuImage before(ImageCopy(canvas.value));
    // Quad anchored at (0,1). The patch's own centre must replace the former grass heart.
    CpuImage patch(root / "common" / quad.image);
    check(patch.value.width == quad.width && patch.value.height == quad.height,
          "quad PNG dimension contract");
    const int px = 50 + quad.offset_x, py = 30 + quad.offset_y;
    const auto centre = GetImageColor(patch.value, 15, 10);
    check(centre.a == 255 && !same(GetImageColor(before.value, px + 15, py + 10), centre),
          "actual four-frame heart differs from opaque patch centre");
    ImageDraw(&canvas.value, patch.value, {0, 0, 30, 20},
              {static_cast<float>(px), static_cast<float>(py), 30, 20}, WHITE);
    check(same(GetImageColor(canvas.value, px + 15, py + 10), centre),
          "quad centre covered by original asset at source offset");
    int changed = 0;
    for (int y = 0; y < canvas.value.height; ++y)
        for (int x = 0; x < canvas.value.width; ++x) {
            const bool differs =
                !same(GetImageColor(before.value, x, y), GetImageColor(canvas.value, x, y));
            changed += differs;
            if (x < px || x >= px + quad.width || y < py || y >= py + quad.height)
                check(!differs, "whole PNG does not change pixels outside its rectangle");
        }
    check(changed > 10, "patch has a nontrivial actual pixel effect");
    CpuImage edge_image(root / "common" / edge.image);
    check(edge_image.value.width == edge.width && edge_image.value.height == edge.height,
          "edge PNG dimension contract");

    // Unlike the patch-only check above, this pass queues all base frames and the quad together.
    // Fewer than10 commands per depth: bucket overflow cannot change this isolated ordering.
    struct Command {
        int x;
        int y;
        int frame;
        int depth;
        bool patch;
    };
    std::vector<Command> commands;
    for (int y = 1; y >= 0; --y)
        for (int x = 0; x < 2; ++x) {
            const int dx = 20 + 30 * (x + y), dy = 60 + 15 * (x - y) - 15;
            // mapchip37 has flags1 and depth offset0: use D.y-50, not low-ground D.y+15.
            commands.push_back({dx, dy, frames[y][x], dy - 50, false});
        }
    commands.push_back({px, py, 0, 30 + quad.depth_offset, true});
    // Explicit foreground fixture, not another recovered original draw command.
    commands.push_back({px + 15, py + 10, -1, 100, false});
    std::stable_sort(commands.begin(), commands.end(),
                     [](const auto &a, const auto &b) { return a.depth < b.depth; });
    check(!commands.front().patch && !commands.back().patch && commands[commands.size() - 2].patch,
          "quad shares source depth order instead of being globally topmost");
    CpuImage ordered(GenImageColor(160, 120, Color{32, 160, 32, 255}));
    for (const auto &command : commands) {
        if (command.frame == -1) {
            ImageDrawPixel(&ordered.value, command.x, command.y, RED);
            continue;
        }
        if (command.patch) {
            ImageDraw(&ordered.value, patch.value, {0, 0, 30, 20},
                      {static_cast<float>(px), static_cast<float>(py), 30, 20}, WHITE);
            continue;
        }
        for (const auto &layer : seb.layers)
            for (const auto &part : layer.parts) {
                if (part.frame != command.frame)
                    continue;
                CpuImage piece(ImageFromImage(road.value, {static_cast<float>(part.source_x),
                                                           static_cast<float>(part.source_y),
                                                           static_cast<float>(part.width),
                                                           static_cast<float>(part.height)}));
                if (part.flip_x)
                    ImageFlipHorizontal(&piece.value);
                if (part.flip_y)
                    ImageFlipVertical(&piece.value);
                ImageDraw(&ordered.value, piece.value,
                          {0, 0, static_cast<float>(part.width), static_cast<float>(part.height)},
                          {static_cast<float>(command.x + part.offset_x),
                           static_cast<float>(command.y + part.offset_y),
                           static_cast<float>(part.width), static_cast<float>(part.height)},
                          WHITE);
            }
    }
    check(same(GetImageColor(ordered.value, px + 14, py + 10), GetImageColor(patch.value, 14, 10)),
          "real grass heart is covered under source road depth ordering");
    check(same(GetImageColor(ordered.value, px + 15, py + 10), RED),
          "foreground remains above road patch instead of unconditional top overlay");
}
} // namespace
int main(int argc, char **argv) {
    SetTraceLogLevel(LOG_WARNING);
    if (argc != 2)
        throw std::invalid_argument("asset root required");
    compose(argv[1]);
    ark::app::Game game;
    auto map = ark::app::startup_data().map;
    for (const auto &[id, instance] : game.state().facilities) {
        (void)id;
        map.cells[map.index(instance.anchor)] = {27, 0};
    }
    const auto patches = road_patches(map, 37);
    for (std::size_t index = 0; index < map.cells.size(); ++index) {
        const auto &cell = ark::app::startup_data().loaded_cells[index];
        auto found = std::find_if(patches.begin(), patches.end(),
                                  [&](const auto &p) { return map.index(p.cell) == index; });
        check((found != patches.end()) == (cell.road_quad || cell.edge_road_pair),
              "dynamic marks equal full published startup marks");
        if (found != patches.end())
            check(std::string(found->image) ==
                      (cell.road_quad ? "road4block00.png" : "road4block01.png"),
                  "published patch kind preserved");
    }
    check(game.display(37).flags == 1 && game.display(37).depth_offset == 0,
          "raw mapchip road depth metadata");
    ark::world::SourceMap square{4, 4, std::vector<ark::world::SourceCell>(16, {27, 0})};
    for (auto cell : {ark::world::Cell{1, 1}, {2, 1}, {1, 2}, {2, 2}})
        square.cells[square.index(cell)] = {37, 0};
    const auto square_patch = road_patches(square, 37);
    check(square_patch.size() == 1 && square_patch[0].cell == ark::world::Cell{1, 2},
          "quad anchor is lower source-y row, not opposite corner");
    for (auto cell : {ark::world::Cell{1, 1}, {2, 1}, {1, 2}, {2, 2}}) {
        auto occupied = square;
        occupied.cells[occupied.index(cell)] = {27, 0};
        check(road_patches(occupied, 37).empty(), "covering any corner removes central patch");
    }
    ark::world::SourceMap edges{2, 2, std::vector<ark::world::SourceCell>(4, {37, 0})};
    auto pair = road_patches(edges, 37);
    check(pair.size() == 2 && pair[0].cell == ark::world::Cell{0, 1} && pair[0].width == 30 &&
              pair[1].cell == ark::world::Cell{0, 0} && pair[1].width == 27,
          "y-descending order and quad precedence over lower-edge pair");
    edges.cells[1] = {27, 0};
    pair = road_patches(edges, 37);
    check(pair.size() == 1 && pair[0].width == 27, "broken quad falls back to remaining edge pair");
    std::cout << checks << " checks passed\n";
}
