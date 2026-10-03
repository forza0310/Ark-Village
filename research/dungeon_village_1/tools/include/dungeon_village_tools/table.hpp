#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace dungeon_village_tools {

using TableRows = std::vector<std::vector<std::string>>;
using EventProgram = std::vector<std::vector<std::int32_t>>;

TableRows parse_tsv(const std::vector<std::uint8_t> &bytes);
std::int32_t parse_table_integer(const std::string &field);
std::vector<std::int32_t> parse_integer_list(const std::string &field);
EventProgram parse_event_program(const std::string &field);

struct FacilityTableRow {
    std::int32_t definition_id{};
    std::string name;
    std::int32_t kind{};
    std::int32_t activity_category{};
    std::int32_t footprint_kind{};
    EventProgram event_program;
    std::array<std::string, 36> fields;
};

std::vector<FacilityTableRow> parse_facility_table(const std::vector<std::uint8_t> &bytes);

struct ItemTableRow {
    std::int32_t definition_id{};
    std::string name;
    std::int32_t legacy_category{};
    std::array<std::int32_t, 3> facility_improvements{};
    std::array<std::string, 25> fields;
};

std::vector<ItemTableRow> parse_item_table(const std::vector<std::uint8_t> &bytes);

} // namespace dungeon_village_tools
