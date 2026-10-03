#include "dungeon_village_prototype/asset_manifest.hpp"

#include <raylib.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int checks = 0;

void check(bool condition, const std::string &message) {
    ++checks;
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Function> void check_throws(Function function, const std::string &message) {
    ++checks;
    try {
        function();
    } catch (const std::exception &) {
        return;
    }
    throw std::runtime_error(message);
}

void write_manifest(const std::filesystem::path &root, const std::string &rows) {
    std::ofstream stream(root / "MANIFEST.tsv");
    stream << "logical_key\tsource_archive\tsource_entry\tsource_sha256\tnormalized_path\t"
              "width\theight\tframe\tanchor\ttransform\n"
           << rows;
    if (!stream) {
        throw std::runtime_error("测试清单写入失败");
    }
}

void malformed_manifests(const std::filesystem::path &root) {
    using dungeon_village_prototype::load_asset_manifest;
    const auto row = [](const std::string &path, const std::string &frame) {
        return "test\timage.dat\ttest.png\tunused\t" + path + "\t1\t1\t" + frame + "\t0,0\t测试\n";
    };
    write_manifest(root, row("missing.png", "0,0,1,1"));
    check_throws([&] { (void)load_asset_manifest(root); }, "未拒绝缺失图片");
    write_manifest(root, row("../outside.png", "0,0,1,1"));
    check_throws([&] { (void)load_asset_manifest(root); }, "未拒绝父目录路径");
    write_manifest(root, row("/outside.png", "0,0,1,1"));
    check_throws([&] { (void)load_asset_manifest(root); }, "未拒绝绝对路径");
    write_manifest(root, row("normalized/building.inn.png", "0,0,-1,1"));
    check_throws([&] { (void)load_asset_manifest(root); }, "未拒绝负尺寸");
    write_manifest(root, row("normalized/building.inn.png", "0,0,1x,1"));
    check_throws([&] { (void)load_asset_manifest(root); }, "未拒绝非整数坐标");
    const auto valid = row("normalized/building.inn.png", "0,0,1,1");
    write_manifest(root, valid + valid);
    check_throws([&] { (void)load_asset_manifest(root); }, "未拒绝重复逻辑键");
}

void write_replacement(const std::filesystem::path &source, const std::filesystem::path &output) {
    if (std::filesystem::exists(output)) {
        throw std::runtime_error("替换夹具目录必须不存在，以免覆盖已有数据");
    }
    std::filesystem::create_directories(output / "normalized");
    std::filesystem::copy(source / "normalized", output / "normalized",
                          std::filesystem::copy_options::recursive);
    malformed_manifests(output);
    std::filesystem::copy_file(source / "MANIFEST.tsv", output / "MANIFEST.tsv",
                               std::filesystem::copy_options::overwrite_existing);
    const auto manifest = dungeon_village_prototype::load_asset_manifest(output);
    for (const auto *key : {"building.inn", "character.night"}) {
        const auto path = manifest.at(key).path;
        auto image = LoadImage(path.string().c_str());
        check(image.data != nullptr, "替换夹具图片无法读取");
        ImageColorTint(&image, Color{170, 200, 255, 255});
        check(ExportImage(image, path.string().c_str()), "替换夹具图片无法导出");
        UnloadImage(image);
    }
    check(dungeon_village_prototype::load_asset_manifest(output).size() == 7,
          "替换根未保持七个逻辑键");
}

void compare_screenshots(const std::filesystem::path &original,
                         const std::filesystem::path &replacement) {
    auto before = LoadImage(original.string().c_str());
    auto after = LoadImage(replacement.string().c_str());
    check(before.data != nullptr && after.data != nullptr, "无法读取验收截图");
    check(before.width == 720 && before.height == 720 && after.width == 720 && after.height == 720,
          "截图不是精确的 720x720 画面");
    auto *before_pixels = LoadImageColors(before);
    auto *after_pixels = LoadImageColors(after);
    check(before_pixels != nullptr && after_pixels != nullptr, "无法读取截图像素");
    const auto same = [](Color left, Color right) {
        return left.r == right.r && left.g == right.g && left.b == right.b && left.a == right.a;
    };
    std::size_t nonblack = 0;
    std::size_t different = 0;
    for (int pixel = 0; pixel < 720 * 720; ++pixel) {
        const auto value = before_pixels[pixel];
        nonblack += (value.r != 0 || value.g != 0 || value.b != 0) ? 1U : 0U;
        different += !same(value, after_pixels[pixel]) ? 1U : 0U;
    }
    check(nonblack > 400000, "截图出现大面积空白");
    check(different > 1000, "素材替换没有产生可见像素差异");
    for (int y = 0; y < 720; y += 3) {
        for (int x = 0; x < 720; x += 3) {
            for (int dy = 0; dy < 3; ++dy) {
                for (int dx = 0; dx < 3; ++dx) {
                    if (!same(before_pixels[y * 720 + x], before_pixels[(y + dy) * 720 + x + dx])) {
                        throw std::runtime_error("截图不符合 3 倍最近邻像素契约");
                    }
                }
            }
        }
    }
    ++checks;
    UnloadImageColors(after_pixels);
    UnloadImageColors(before_pixels);
    UnloadImage(after);
    UnloadImage(before);
    std::cout << checks << " 项截图检查通过；非黑像素=" << nonblack
              << "；替换差异像素=" << different << '\n';
}

} // namespace

int main(int argc, char **argv) {
    try {
        SetTraceLogLevel(LOG_WARNING);
        if (argc == 4 && std::string(argv[1]) == "--compare") {
            compare_screenshots(argv[2], argv[3]);
            return 0;
        }
        if (argc != 2 && argc != 3) {
            throw std::invalid_argument(
                "用法: dungeon_village_asset_tests <素材根> [新替换夹具根]");
        }
        const std::filesystem::path root = argv[1];
        const auto manifest = dungeon_village_prototype::load_asset_manifest(root);
        check(manifest.size() == 7, "源清单必须具有七个逻辑键");
        std::size_t png_count = 0;
        std::size_t transparent_count = 0;
        std::map<std::string, std::size_t> duplicate_groups;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(root / "original")) {
            if (!entry.is_regular_file() || entry.path().extension() != ".png") {
                continue;
            }
            auto image = LoadImage(entry.path().string().c_str());
            check(image.data != nullptr && image.width > 0 && image.height > 0,
                  "原始 PNG 解码失败: " + entry.path().string());
            auto *colors = LoadImageColors(image);
            check(colors != nullptr, "原始 PNG 像素读取失败");
            bool transparent = false;
            for (int pixel = 0; pixel < image.width * image.height; ++pixel) {
                transparent = transparent || colors[pixel].a < 255;
            }
            transparent_count += transparent ? 1U : 0U;
            ++png_count;
            UnloadImageColors(colors);
            UnloadImage(image);
            std::ifstream stream(entry.path(), std::ios::binary);
            const std::string bytes{std::istreambuf_iterator<char>(stream),
                                    std::istreambuf_iterator<char>()};
            ++duplicate_groups[bytes];
        }
        check(png_count == 398, "原始 PNG 数量不匹配");
        std::size_t redundant_count = 0;
        for (const auto &group : duplicate_groups) {
            redundant_count += group.second - 1;
        }
        for (const auto &entry : manifest) {
            auto image = LoadImage(entry.second.path.string().c_str());
            const auto &frame = entry.second.frame;
            check(image.data != nullptr && frame.x <= image.width && frame.y <= image.height &&
                      frame.width <= image.width - frame.x &&
                      frame.height <= image.height - frame.y,
                  "逻辑帧越界: " + entry.first);
            UnloadImage(image);
        }
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto output = argc == 3 ? std::filesystem::path(argv[2])
                                      : std::filesystem::current_path() /
                                            ("asset-fixture-" + std::to_string(stamp));
        write_replacement(root, output);
        std::cout << checks << " 项检查通过；PNG=" << png_count
                  << "；含透明像素=" << transparent_count << "；字节重复副本=" << redundant_count
                  << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "失败: " << error.what() << '\n';
        return 1;
    }
}
