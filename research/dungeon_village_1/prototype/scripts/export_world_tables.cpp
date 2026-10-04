// 固定归档原表发布器；复用严格archive/CRC/XOR，不重复实现归档解码。
#include "dungeon_village_tools/archive.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace tools = dungeon_village_tools;
void publish(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes) {
    if (std::filesystem::exists(path)) {
        if (tools::read_binary_file(path) != bytes)
            throw std::runtime_error("拒绝覆盖不同原表内容");
        return;
    }
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    if (!output)
        throw std::runtime_error("原表写入失败");
}
int main(int argc, char **argv) {
    try {
        if (argc != 3 && argc != 4)
            throw std::invalid_argument("参数：固定xls.dat 输出目录 [额外单表]");
        const auto raw = tools::read_binary_file(argv[1]);
        const auto archive_hash = tools::sha256_hex(raw);
        if (archive_hash != "8baacbb181dcd4eb18435eb39938ef2ad21d7dee9db27654b07c3428f4739958")
            throw std::runtime_error("固定xls.dat哈希不一致");
        const auto archive = tools::decode_game_archive("xls.dat", raw);
        const std::filesystem::path directory = argv[2];
        std::filesystem::create_directories(directory);
        std::string manifest = "archive\tarchive_sha256\tentry\tentry_sha256\tbytes\n";
        const std::vector<std::string> names =
            argc == 4 ? std::vector<std::string>{argv[3]}
                      : std::vector<std::string>{"monster.txt", "questData.txt", "armour.txt",
                                                 "accessory.txt", "item.txt"};
        for (const auto &name : names) {
            if (!tools::is_safe_relative_path(name) ||
                std::filesystem::path(name).filename() != name)
                throw std::runtime_error("只发布安全单层原表名");
            const auto *entry = archive.find(name);
            if (!entry || entry->flags != 0)
                throw std::runtime_error("原表缺失或标志不支持");
            publish(directory / name, entry->data);
            manifest += "xls.dat\t" + archive_hash + "\t" + name + "\t" +
                        tools::sha256_hex(entry->data) + "\t" + std::to_string(entry->data.size()) +
                        "\n";
        }
        publish(directory / (argc == 4 ? std::string(argv[3]) + ".SOURCE.tsv" : "SOURCE.tsv"),
                {manifest.begin(), manifest.end()});
        std::cout << manifest;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
