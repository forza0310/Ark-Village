// Read the original facility table and project only the three definitions supported by the
// prototype. Package responsibilities and evidence boundaries: ../README.md.

#include "dungeon_village_prototype/village.hpp"
#include "dungeon_village_tools/table.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>

namespace dungeon_village_prototype {

std::vector<PrototypeDefinition> load_prototype_catalog(const std::filesystem::path &table) {
    if (std::filesystem::file_size(table) > 1024 * 1024) {
        throw std::runtime_error("设施表超出原型读取上限");
    }
    std::ifstream input(table, std::ios::binary);
    if (!input)
        throw std::runtime_error("无法读取设施表");
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input),
                                          std::istreambuf_iterator<char>()};
    const auto rows = dungeon_village_tools::parse_facility_table(bytes);
    std::vector<PrototypeDefinition> catalog;
    for (const auto &row : rows) {
        if (row.definition_id != 28 && row.definition_id != 29 && row.definition_id != 36)
            continue;
        const auto number = [&](std::size_t column) {
            return dungeon_village_tools::parse_table_integer(row.fields[column]);
        };
        if (row.kind != 3 || (row.activity_category != 1 && row.activity_category != 2) ||
            number(5) != 0 || row.footprint_kind < 0 || row.footprint_kind > 1) {
            throw std::runtime_error("设施数据不满足有限原型契约");
        }
        PrototypeDefinition definition;
        definition.id = row.definition_id;
        definition.name = row.name;
        definition.kind = row.kind;
        definition.category = row.activity_category;
        definition.detail = number(5);
        definition.shape = static_cast<reference::FacilityShape>(row.footprint_kind);
        for (std::size_t slot = 0; slot < 4; ++slot) {
            definition.economy.attributes[slot] = {number(15 + slot * 2), number(16 + slot * 2)};
        }
        definition.economy.upgrade_uses = {number(23), number(24)};
        definition.economy.construction_cost = number(13);
        definition.economy.construction_ticks = number(14);
        if (number(35) < 0)
            throw std::runtime_error("设施标志不能为负");
        definition.economy.legacy_flags = static_cast<std::uint32_t>(number(35));
        const auto slots = dungeon_village_tools::parse_integer_list(row.fields[26]);
        const auto deltas = dungeon_village_tools::parse_integer_list(row.fields[27]);
        if (slots.size() != deltas.size())
            throw std::runtime_error("邻接槽位与值不匹配");
        for (std::size_t i = 0; i < slots.size(); ++i) {
            definition.neighbours.push_back({slots[i], deltas[i]});
        }
        catalog.push_back(std::move(definition));
    }
    if (catalog.size() != 3)
        throw std::runtime_error("设施表缺少原型所需定义");
    return catalog;
}

} // namespace dungeon_village_prototype
