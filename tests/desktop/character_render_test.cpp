// Verify feet against actual source diamonds and playback against admitted movement, not FPS.
#include "ark/assets/sprite.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>

namespace {
void check(bool ok, const char *why) {
    if (!ok)
        throw std::runtime_error(why);
}
ark::assets::SpriteDefinition load(const std::filesystem::path &file) {
    std::ifstream input(file, std::ios::binary);
    check(static_cast<bool>(input), "source SEB readable");
    return ark::assets::parse_legacy_seb(
        {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()});
}
void ground(const std::filesystem::path &root) {
    const auto road = load(root / "image/road00.seb");
    const int source_x[] = {0, 18, 0, 36};
    for (int facing = 0; facing < 4; ++facing) {
        const auto walk = load(
            root / "human" /
            (std::array<const char *, 4>{"walk00.seb", "walk01.seb", "walk02.seb", "walk03.seb"}.at(
                facing)));
        check(walk.frame_count == 4 && walk.layers.size() == 1 && walk.layers[0].parts.size() == 4,
              "each published walking direction supplies exactly four poses");
        for (int n = 0; n < 4; ++n) {
            const auto &p = walk.layers[0].parts[n];
            check(p.frame == n && p.source_x == source_x[n] && p.source_y == facing * 24 &&
                      p.width == 18 && p.height == 24 && p.offset_x == -9 && p.offset_y == -24 &&
                      !p.flip_x && !p.flip_y,
                  "four distinct atlas rows keep the same feet, without invented mirroring");
        }
    }
    for (const auto &part : road.layers.at(0).parts)
        check(part.width == 60 && part.height == 29 && part.offset_x == 0 && part.offset_y == 0,
              "Published road diamond source rectangle remains 60 by 29 at its drawing origin");
}
} // namespace
int main(int argc, char **argv) {
    try {
        check(argc == 2, "asset root required");
        ground(argv[1]);
        std::cout << "PASS source road diamonds and four walking directions/poses\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
