#include "dungeon_village_tools/archive.hpp"
#include "dungeon_village_tools/table.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
    using namespace dungeon_village_tools;
    try {
        if (argc != 3 && !(argc == 5 && (std::string(argv[3]) == "--publish" ||
                                         std::string(argv[3]) == "--row"))) {
            throw std::invalid_argument(
                "用法: kairo_table_inspect xls.dat 条目名 [--publish 新目录 | --row 下标]");
        }
        if (std::filesystem::path(argv[1]).filename() != "xls.dat") {
            throw std::invalid_argument("当前数据研究输入必须为 xls.dat");
        }
        const auto encrypted = read_binary_file(argv[1]);
        const auto archive =
            decode_game_archive(std::filesystem::path(argv[1]).filename().string(), encrypted);
        const auto *entry = archive.find(argv[2]);
        if (entry == nullptr) {
            throw std::runtime_error("归档缺少指定表条目");
        }
        const auto rows = parse_tsv(entry->data);
        const bool row_mode = argc == 5 && std::string(argv[3]) == "--row";
        std::size_t first_row = 0;
        if (row_mode) {
            const auto row_index = parse_table_integer(argv[4]);
            if (row_index < 0 || static_cast<std::size_t>(row_index) >= rows.size()) {
                throw std::runtime_error("表行下标越界");
            }
            first_row = static_cast<std::size_t>(row_index);
        }
        std::vector<FacilityTableRow> facilities;
        std::vector<ItemTableRow> items;
        std::size_t validated_columns = 0;
        if (entry->name == "tenantData.txt") {
            facilities = parse_facility_table(entry->data);
            validated_columns = 36;
        } else if (entry->name == "item.txt") {
            items = parse_item_table(entry->data);
            validated_columns = 25;
        }
        std::cout << "来源归档 SHA-256=" << sha256_hex(encrypted) << "\n条目=" << entry->name
                  << " 字节=" << entry->data.size() << " SHA-256=" << sha256_hex(entry->data)
                  << "\n行数=" << rows.size() << '\n';
        const auto last_row = row_mode ? first_row + 1 : std::min<std::size_t>(rows.size(), 6);
        for (std::size_t index = first_row; index < last_row; ++index) {
            std::cout << "行 " << index << " 列数=" << rows[index].size() << '\n';
            for (std::size_t column = 0; column < rows[index].size(); ++column) {
                std::cout << column << '=' << rows[index][column]
                          << (column + 1 == rows[index].size() ? '\n' : '\t');
            }
        }
        for (std::size_t index = 0; index < facilities.size(); ++index) {
            if (row_mode && index != first_row) {
                continue;
            }
            const auto &facility = facilities[index];
            std::cout << facility.definition_id << '\t' << facility.name
                      << "\tkind=" << facility.kind << "\tcategory=" << facility.activity_category
                      << "\tshape=" << facility.footprint_kind
                      << "\tcolumn13=" << facility.fields[13]
                      << "\tevent_instructions=" << facility.event_program.size() << '\n';
        }
        for (std::size_t index = 0; index < items.size(); ++index) {
            if (row_mode && index != first_row) {
                continue;
            }
            const auto &item = items[index];
            std::cout << item.definition_id << '\t' << item.name
                      << "\tcategory=" << item.legacy_category
                      << "\timprovements=" << item.facility_improvements[0] << ','
                      << item.facility_improvements[1] << ',' << item.facility_improvements[2]
                      << '\n';
        }
        if (argc == 5 && !row_mode) {
            if (validated_columns == 0) {
                throw std::runtime_error("当前发布范围仅支持已校验的 tenantData.txt 和 item.txt");
            }
            const std::filesystem::path output(argv[4]);
            const std::filesystem::path temporary(output.string() + ".partial");
            if (output.empty() || output.filename().empty() || std::filesystem::exists(output) ||
                std::filesystem::exists(temporary)) {
                throw std::runtime_error("发布目录或临时目录已存在/路径无效，不覆盖");
            }
            std::filesystem::create_directories(temporary / "original");
            std::ofstream original(temporary / "original" / entry->name, std::ios::binary);
            original.exceptions(std::ios::badbit | std::ios::failbit);
            original.write(reinterpret_cast<const char *>(entry->data.data()),
                           static_cast<std::streamsize>(entry->data.size()));
            original.close();
            std::ofstream manifest(temporary / "SOURCE.tsv", std::ios::binary);
            manifest.exceptions(std::ios::badbit | std::ios::failbit);
            manifest << "archive\tarchive_sha256\tentry\tentry_sha256\tbytes\trows\tcolumns\n"
                     << "xls.dat\t" << sha256_hex(encrypted) << '\t' << entry->name << '\t'
                     << sha256_hex(entry->data) << '\t' << entry->data.size() << '\t' << rows.size()
                     << '\t' << validated_columns << '\n';
            manifest.close();
            std::filesystem::rename(temporary, output);
            std::cout << "已发布=" << output.string() << '\n';
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "错误: " << error.what() << '\n';
        return 1;
    }
}
