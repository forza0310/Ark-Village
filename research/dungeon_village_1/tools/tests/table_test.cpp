#include "dungeon_village_tools/archive.hpp"
#include "dungeon_village_tools/table.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
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
    check(rejected, "invalid table rejected");
}
std::vector<std::uint8_t> bytes(const std::string &text) {
    return {text.begin(), text.end()};
}

std::string facility_text(const std::vector<std::string> &row) {
    std::string result;
    for (std::size_t index = 0; index < row.size(); ++index) {
        if (index > 0) {
            result += '\t';
        }
        result += row[index];
    }
    return result;
}

} // namespace

int main(int argc, char **argv) {
    if (argc == 2) {
        const auto data = read_binary_file(argv[1]);
        check(sha256_hex(data) ==
                  "5ae35310fbd178f98b273fc2bbe98b1bbf5b72950fca56dbd93c089ca345834a",
              "published table matches fixed entry bytes");
        const auto definitions = parse_facility_table(data);
        check(definitions.size() == 85, "all fixed facility definitions present");
        std::array<int, 3> shapes{};
        std::array<int, 11> categories{};
        int populated_neighbour_lists = 0;
        int populated_event_programs = 0;
        for (std::size_t index = 0; index < definitions.size(); ++index) {
            check(definitions[index].definition_id == static_cast<std::int32_t>(index),
                  "fixed definition IDs are contiguous and ordered");
            ++shapes[static_cast<std::size_t>(definitions[index].footprint_kind)];
            const auto category = definitions[index].activity_category;
            check(category >= 0 && category < 11, "fixed activity category dimension");
            ++categories[static_cast<std::size_t>(category)];
            const auto selectors = parse_integer_list(definitions[index].fields[26]);
            const auto deltas = parse_integer_list(definitions[index].fields[27]);
            check(selectors.size() == deltas.size(), "fixed neighbour parallel list lengths match");
            for (const auto selector : selectors) {
                check(selector >= 0 && selector <= 2,
                      "fixed neighbour selector uses known attribute slot");
            }
            if (!selectors.empty()) {
                ++populated_neighbour_lists;
            }
            const auto &definition = definitions[index];
            if (index >= 33 && index <= 58) {
                check(definition.event_program ==
                          EventProgram{{6, 100}, {40, definition.definition_id}},
                      "fixed facility events are delay then own definition presentation");
                check(definition.fields[2] == (index <= 44 ? "2" : "3"),
                      "fixed event presentation icon categories");
            } else {
                check(definition.event_program.empty(), "other fixed definitions have no script");
            }
            if (!definition.event_program.empty()) {
                ++populated_event_programs;
            }
        }
        check(shapes == std::array<int, 3>{75, 7, 3}, "fixed shape inventory");
        check(categories == std::array<int, 11>{20, 36, 2, 6, 4, 13, 0, 0, 0, 3, 1},
              "fixed activity inventory has no categories six, seven or eight");
        check(definitions[28].kind == 3 && definitions[28].activity_category == 2 &&
                  definitions[28].fields[13] == "1000",
              "inn raw fields");
        check(definitions[36].kind == 3 && definitions[36].activity_category == 1 &&
                  definitions[36].fields[13] == "700",
              "cafe raw fields");
        check(populated_neighbour_lists == 51, "fixed populated neighbour field inventory");
        check(populated_event_programs == 26, "fixed event program inventory");
        check(definitions[28].fields[26] == "2" && definitions[28].fields[27] == "10" &&
                  definitions[36].fields[26] == "2" && definitions[36].fields[27] == "10",
              "maintained merchant neighbour fixtures match raw data");
        check(definitions[66].fields[26] == "0&1" && definitions[66].fields[27] == "20&5" &&
                  definitions[74].fields[27] == "70&20" && definitions[25].kind == 12,
              "maintained decoration and house neighbour fixtures match raw data");
        std::cout << checks << " fixed-data checks passed\n";
        return 0;
    }
    check(argc == 1, "valid test arguments");
    const auto table = parse_tsv(bytes("1\t\tname\r\n2\t3\t\n"));
    check(table.size() == 2 && table[0].size() == 3 && table[0][1].empty() && table[1][2].empty(),
          "empty columns and CRLF preserved structurally");
    check(parse_tsv(bytes("\n\n")).size() == 2, "interior blank rows not dropped");
    check(parse_tsv({}).empty(), "empty table");
    check(parse_tsv({0xEF, 0xBB, 0xBF, '1'}).front().front() == "1", "UTF-8 BOM recognized");
    check(parse_tsv({0xE4, 0xB8, 0xAD, '\t', '1'}).front().front().size() == 3,
          "non-ASCII UTF-8 remains byte exact");
    check(parse_tsv(bytes(" 1\t2 ")).front().front() == " 1", "no silent whitespace trim");
    rejects([] { parse_tsv({'a', 0, 'b'}); });
    rejects([] { parse_tsv({0xC0, 0x80}); });
    rejects([] { parse_tsv({0xED, 0xA0, 0x80}); });
    rejects([] { parse_tsv({0xF4, 0x90, 0x80, 0x80}); });
    rejects([] { parse_tsv({0xE4, 0xB8}); });
    rejects([] { parse_tsv({0xE4, 'x', 0xAD}); });
    rejects([] { parse_tsv(bytes("1\r2")); });
    rejects([] { parse_tsv(bytes("1\r")); });
    rejects([] { parse_tsv(std::vector<std::uint8_t>(16U * 1024U * 1024U + 1, '1')); });
    rejects([] { parse_tsv(bytes(std::string(1024, '\t'))); });
    check(parse_table_integer("-2147483648") == std::numeric_limits<std::int32_t>::min(),
          "minimum 32-bit integer");
    check(parse_table_integer("2147483647") == std::numeric_limits<std::int32_t>::max(),
          "maximum 32-bit integer");
    for (const auto *field : {"", "2147483648", "-2147483649", "1x", " 1", "1 ", "1.2"}) {
        rejects([field] { parse_table_integer(field); });
    }
    check(parse_integer_list("").empty(), "empty list distinct from zero");
    check(parse_integer_list("1&-2&0") == std::vector<std::int32_t>{1, -2, 0}, "integer list");
    rejects([] { parse_integer_list("1&"); });
    rejects([] { parse_integer_list("&1"); });
    rejects([] { parse_integer_list("1&&2"); });
    std::vector<std::string> row(36, "0");
    row[1] = "fixture inn";
    row[30] = "opaque description";
    row[33] = "777, +1 & -99, -2";
    row[26] = "";
    const auto parsed = parse_facility_table(bytes(facility_text(row)));
    check(parsed.size() == 1 && parsed.front().definition_id == 0 &&
              parsed.front().fields[33] == row[33] && parsed.front().fields[26].empty() &&
              parsed.front().event_program == EventProgram{{777, 1}, {-99, -2}},
          "facility structure preserves raw event text and unknown instructions");
    check(parse_event_program("").empty(), "empty script distinct from empty instruction");
    check(parse_event_program(" \t+6, 100\r &40, 33 ") == EventProgram{{6, 100}, {40, 33}},
          "Java ASCII trim and leading positive sign");
    check(parse_event_program("-2147483648,2147483647,0,-0") ==
              EventProgram{{std::numeric_limits<std::int32_t>::min(),
                            std::numeric_limits<std::int32_t>::max(), 0, 0}},
          "event signed integer boundaries");
    for (const auto *script : {" ", "&", "6,", ",6", "6,,1", "6,1&", "&6,1", "6,1&&40,1",
                               "6,2147483648", "6,-2147483649", "6,1x", "6,1.0", "6,1 0", "6,+",
                               "6,++1", "6,+-1", "6,--1", "6,−1", "６,1"}) {
        rejects([script] { parse_event_program(script); });
    }
    std::string script(1024U * 1024U, ' ');
    script[0] = '6';
    check(parse_event_program(script) == EventProgram{{6}}, "exact event byte budget accepted");
    script += ' ';
    rejects([&script] { parse_event_program(script); });
    script = "6";
    for (int index = 1; index < 64; ++index) {
        script += ",1";
    }
    check(parse_event_program(script).front().size() == 64, "exact instruction width budget");
    script += ",1";
    rejects([&script] { parse_event_program(script); });
    script = "6";
    for (int index = 1; index < 4096; ++index) {
        script += "&6";
    }
    check(parse_event_program(script).size() == 4096, "exact event instruction budget");
    script += "&6";
    rejects([&script] { parse_event_program(script); });
    auto bad_event = row;
    bad_event[33] = "not an event";
    rejects([&bad_event] { parse_facility_table(bytes(facility_text(bad_event))); });
    rejects([] { parse_facility_table({}); });
    rejects(
        [&row] { parse_facility_table(bytes(facility_text(row) + '\n' + facility_text(row))); });
    for (const auto column : {0, 2, 13, 25, 32, 35}) {
        auto bad = row;
        bad[static_cast<std::size_t>(column)] = "";
        rejects([&bad] { parse_facility_table(bytes(facility_text(bad))); });
    }
    auto bad = row;
    bad[10] = "3";
    rejects([&bad] { parse_facility_table(bytes(facility_text(bad))); });
    bad = row;
    bad[0] = "-1";
    rejects([&bad] { parse_facility_table(bytes(facility_text(bad))); });
    bad = row;
    bad.pop_back();
    rejects([&bad] { parse_facility_table(bytes(facility_text(bad))); });
    bad = row;
    bad.push_back("extra");
    rejects([&bad] { parse_facility_table(bytes(facility_text(bad))); });
    std::cout << checks << " checks passed\n";
}
