#include "ark/simulation/village/startup_information.hpp"
#include "ark/simulation/world/startup_world_runtime.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation {
namespace {
static_assert(std::numeric_limits<int>::digits == 31, "Owner现金桶需要32位int");
// 只用无符号模运算还原Java int位模式；转有符号前先在64位范围减2^32。
std::int32_t signed_value(std::uint32_t bits) {
    const auto value = static_cast<std::int64_t>(bits);
    return static_cast<std::int32_t>(bits <= 0x7fffffffU ? value : value - 0x100000000LL);
}
std::string currency(std::int32_t value) {
    const auto wide = static_cast<std::int64_t>(value);
    std::string digits = std::to_string(wide < 0 ? -wide : wide);
    for (std::size_t position = digits.size(); position > 3; position -= 3)
        digits.insert(position - 3, 1, ',');
    return (wide < 0 ? "-" : "") + digits + "Ｇ";
}
} // namespace

std::optional<StartupIncomeInformation> startup_income_information(
    const StartupInformationCash &monthly_cash, int current_month, int period) {
    if (current_month < 0 || current_month >= 12 || period < 0 || period > 1)
        return std::nullopt;
    constexpr std::array<std::string_view, 5> labels{{"设施", "怪物", "冒险者", "商店", "其它"}};
    StartupIncomeInformation result;
    std::uint32_t profit{};
    for (std::size_t category = 0; category < labels.size(); ++category) {
        std::array<std::uint32_t, 2> amounts{};
        const int begin = period == 0 ? current_month : 0;
        const int end = period == 0 ? current_month + 1 : 12;
        for (int month = begin; month < end; ++month)
            for (std::size_t direction = 0; direction < amounts.size(); ++direction)
                amounts[direction] += static_cast<std::uint32_t>(monthly_cash[month][category][direction]);
        auto &row = result.rows[category];
        row.label = labels[category];
        row.income = signed_value(amounts[0]);
        row.expense = signed_value(amounts[1]);
        row.income_text = currency(row.income);
        row.expense_text = currency(row.expense);
        profit += amounts[0];
        profit -= amounts[1];
    }
    result.profit = signed_value(profit);
    result.profit_text = currency(result.profit);
    return result;
}

std::optional<std::vector<StartupItemInformation>>
startup_item_information(const StartupWorldRuntimeState &s) {
    if (!s.rules) return {};
    std::vector<StartupItemInformation> result;
    std::set<int> seen;
    for (const auto &definition : s.rules->items) {
        const int id = definition.identity;
        const auto item = s.items.find(id);
        const auto catalog = s.catalog.find({0, id});
        if (id < 0 || !seen.insert(id).second || item == s.items.end() || catalog == s.catalog.end())
            return {};
        const auto &a = item->second;
        const auto &b = catalog->second;
        // 与恢复入口相同的唯一道具事实；查询不修补不同步的镜像。
        if (a.inventory < 0 || a.inventory > 999 || a.inventory != b.inventory ||
            a.status != b.status || a.unlock_counter != b.unlock_counter ||
            a.newly_unlocked != b.newly_unlocked)
            return {};
        if (a.inventory > 0)
            result.push_back({id, a.inventory, definition.render_icon, a.newly_unlocked,
                              definition.name, definition.description});
    }
    return result;
}

std::optional<StartupEquipmentInformation> startup_equipment_information(
    const StartupWorldRuntimeState &s, int slot, StartupInformationEdition edition) {
    if (!s.rules || slot < 0 || slot > 3 ||
        (edition != StartupInformationEdition::apk_1_0_8 && edition != StartupInformationEdition::steam_2_56))
        return {};
    const int kind = slot == 0 ? 1 : slot == 3 ? 3 : 2;
    std::vector<const StartupWorldEquipment *> definitions;
    std::set<int> seen;
    for (const auto &definition : s.rules->equipment) {
        if (definition.shop.kind != kind) continue;
        if (kind == 2 && (definition.shop.type == 2 ? 1 : 2) != slot) continue;
        const auto current = s.catalog.find({kind, definition.shop.id});
        if (definition.shop.id < 0 || !seen.insert(definition.shop.id).second || current == s.catalog.end())
            return {};
        if (edition == StartupInformationEdition::steam_2_56 && (slot == 1 || slot == 3) &&
            current->second.flags == 0)
            continue;
        definitions.push_back(&definition);
    }
    // 原外层向前、内层从末尾向前、严格小于交换；等键不等价于stable_sort。
    for (std::size_t first = 0; first + 1 < definitions.size(); ++first)
        for (std::size_t later = definitions.size() - 1; later > first; --later)
            if (definitions[later]->gift_order < definitions[first]->gift_order)
                std::swap(definitions[first], definitions[later]);
    StartupEquipmentInformation result;
    result.slot = slot;
    result.edition = edition;
    result.nonpositive_text = edition == StartupInformationEdition::steam_2_56 ? "--" : "";
    result.attributes = slot == 0 ? std::array<int, 2>{1, 3} : std::array<int, 2>{0, 2};
    for (const auto *definition : definitions) {
        const auto current = s.catalog.find({kind, definition->shop.id});
        StartupEquipmentInformationRow row{definition->shop.id, {}};
        if (current->second.status == 1) {
            StartupEquipmentInformationVisible shown;
            shown.name = definition->name;
            shown.render_icon = kind == 1 ? definition->shop.type : definition->render_image;
            shown.newly_unlocked = current->second.newly_unlocked;
            for (std::size_t column = 0; column < 2; ++column) {
                const int value = definition->shop.combat[result.attributes[column]];
                if (value > 0) shown.values[column] = value;
            }
            row.visible = std::move(shown);
            ++result.known_count;
        }
        result.rows.push_back(std::move(row));
    }
    return result;
}

std::optional<StartupTownInformation> startup_town_information(const StartupWorldRuntimeState &s) {
    if(!s.rules || s.rank<0 || s.task_progress.successes<0 || s.events_held<0)return {};
    StartupTownInformation result;
    result.rank=s.rank;
    result.completed_tasks=s.task_progress.successes;
    result.activities_held=s.events_held;
    const auto increment=[](int &value) {
        if(value==std::numeric_limits<int>::max())return false;
        ++value;return true;
    };
    // 只核本查询的源身份，不把scripts的人物临时投影状态当第二份权威presence。
    std::set<int> humans;
    for(const auto &definition:s.rules->humans) {
        const int id=definition.identity;
        const auto present=s.human_presence.find(id);
        const auto home=s.human_homes.find(id);
        if(id<0 || !humans.insert(id).second || present==s.human_presence.end() ||
           home==s.human_homes.end())return {};
        if(present->second==1 && !increment(result.adventurers))return {};
        if(home->second[2]==1 && !increment(result.residents))return {};
    }
    if(humans.size()!=s.human_presence.size() || humans.size()!=s.human_homes.size())return {};
    std::map<int,int> kinds;
    for(const auto &definition:s.rules->facilities)
        if(definition.id<0 || !kinds.emplace(definition.id,definition.kind).second)return {};
    const auto &instances=s.scene.world.world.facilities;
    std::set<std::uint64_t> referenced;
    for(const auto id:s.scene.world.facility_order) {
        const auto found=instances.find(id);
        if(id==0 || found==instances.end() || found->second.placement.instance_id.value!=id)return {};
        const auto kind=kinds.find(found->second.placement.definition_id);
        if(kind==kinds.end() || found->second.kind!=kind->second)return {};
        referenced.insert(id); // 仅核孤立实例；重复原g引用仍按每次出现计数。
        if((kind->second==3 || kind->second==9) && !increment(result.facilities))return {};
    }
    if(referenced.size()!=instances.size())return {};
    std::set<std::pair<int,int>> equipment;
    for(const auto &definition:s.rules->equipment) {
        const auto key=std::make_pair(definition.shop.kind,definition.shop.id);
        const auto current=s.catalog.find(key);
        if(key.first<1 || key.first>3 || key.second<0 || !equipment.insert(key).second ||
           current==s.catalog.end())return {};
        const int slot=key.first==1?0:key.first==3?3:definition.shop.type==2?1:2;
        if(current->second.status==1 && !increment(result.known_equipment[static_cast<std::size_t>(slot)]))return {};
    }
    for(const auto &entry:s.catalog)
        if(entry.first.first!=0 && !equipment.count(entry.first))return {};
    return result;
}
} // namespace ark::simulation
