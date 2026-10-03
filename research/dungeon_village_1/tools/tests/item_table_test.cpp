#include "dungeon_village_tools/archive.hpp"
#include "dungeon_village_tools/table.hpp"

#include <cstdlib>
#include <iostream>
#include <set>
#include <stdexcept>

using namespace dungeon_village_tools;

namespace {

int checks = 0;
void check(bool condition, const char *message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}
template <class Function> void rejects(Function function) {
    bool rejected = false;
    try {
        function();
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    check(rejected, "invalid item table rejected");
}
std::vector<std::uint8_t> bytes(const std::string &text) {
    return {text.begin(), text.end()};
}
std::string text(const std::vector<std::string> &row) {
    std::string result;
    for (std::size_t column = 0; column < row.size(); ++column) {
        if (column != 0) {
            result += '\t';
        }
        result += row[column];
    }
    return result;
}

} // namespace

int main(int argc, char **argv) {
    if (argc == 3) {
        const auto original = read_binary_file(argv[1]);
        check(sha256_hex(original) ==
                  "95e29d253688e30ac623b925a9a5dacd1f6280f60ed0528b551707df1252d6b8",
              "fixed original item bytes and hash");
        check(original.size() == 3327, "fixed original item byte count");
        const auto items = parse_item_table(original);
        const auto facilities = parse_facility_table(read_binary_file(argv[2]));
        check(items.size() == 36 && facilities.size() == 85, "fixed source inventories");
        std::set<std::int32_t> categories;
        for (std::size_t index = 0; index < items.size(); ++index) {
            const auto &item = items[index];
            check(item.definition_id == static_cast<std::int32_t>(index), "fixed item ID order");
            check(item.legacy_category >= 0 && item.legacy_category < 12,
                  "fixed item category within fixed facility affinity dimensions");
            categories.insert(item.legacy_category);
            for (const auto delta : item.facility_improvements) {
                check(delta >= 0 && delta <= 1000, "fixed increments use small exact integers");
            }
        }
        for (const auto &facility : facilities) {
            const auto affinities = parse_integer_list(facility.fields[34]);
            check(affinities.size() == 12, "all fixed facility category affinities present");
            for (const auto affinity : affinities) {
                check(affinity >= 0 && affinity <= 2, "fixed affinity codes are known");
            }
            for (const auto category : categories) {
                check(static_cast<std::size_t>(category) < affinities.size(),
                      "every fixed item category can index every facility");
            }
        }
        check(items[0].legacy_category == 1 &&
                  items[0].facility_improvements == std::array<std::int32_t, 3>{0, 2, 0},
              "potato facility increment slots");
        check(items[1].legacy_category == 5 &&
                  items[1].facility_improvements == std::array<std::int32_t, 3>{30, 4, 0},
              "milk fixed record used for cafe double affinity");
        std::cout << checks << " fixed-item checks passed\n";
        return 0;
    }
    check(argc == 1, "valid item test arguments");
    std::vector<std::string> row(25, "0");
    row[0] = "7";
    row[1] = "fixture item";
    row[3] = "5";
    row[9] = "30";
    row[10] = "4";
    row[11] = "-1";
    row[23] = "unknown description";
    const auto parsed = parse_item_table(bytes(text(row) + "\r\n"));
    check(parsed.size() == 1 && parsed[0].definition_id == 7 && parsed[0].name == row[1] &&
              parsed[0].legacy_category == 5 &&
              parsed[0].facility_improvements == std::array<std::int32_t, 3>{30, 4, -1} &&
              parsed[0].fields[23] == row[23],
          "item typed subset and all original fields preserved");
    rejects([] { parse_item_table({}); });
    rejects([&row] { parse_item_table(bytes(text(row) + '\n' + text(row))); });
    for (std::size_t column = 0; column < row.size(); ++column) {
        if (column != 1 && column != 23) {
            auto bad = row;
            bad[column] = "2147483648";
            rejects([&bad] { parse_item_table(bytes(text(bad))); });
        }
    }
    for (const std::size_t column : {0U, 1U, 3U, 9U, 24U}) {
        auto bad = row;
        bad[column] = "";
        rejects([&bad] { parse_item_table(bytes(text(bad))); });
    }
    for (const std::size_t column : {0U, 3U}) {
        auto bad = row;
        bad[column] = "-1";
        rejects([&bad] { parse_item_table(bytes(text(bad))); });
    }
    auto bad = row;
    bad.pop_back();
    rejects([&bad] { parse_item_table(bytes(text(bad))); });
    bad = row;
    bad.push_back("extra");
    rejects([&bad] { parse_item_table(bytes(text(bad))); });
    rejects([] { parse_item_table({0xE4, 0xB8}); });
    rejects([] { parse_item_table({'1', '\t', 0}); });
    auto empty_description = row;
    empty_description[23] = "";
    check(parse_item_table(bytes(text(empty_description)))[0].fields[23].empty(),
          "empty unknown description remains empty");
    std::cout << checks << " checks passed\n";
}
