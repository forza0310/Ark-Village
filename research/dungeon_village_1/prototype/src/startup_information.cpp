#include "dungeon_village_prototype/startup_information.hpp"
#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_reference/world_encounters.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_prototype {
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
std::optional<std::vector<StartupFacilityInformation>>
startup_facility_information(const StartupWorldRuntimeState &s) {
    const int month=s.scene.calendar.month;
    if(!s.rules || month<0 || month>=12)return {};
    std::map<int,const StartupDefinition *> definitions;
    for(const auto &definition:s.rules->facilities)
        if(definition.id<0 || !definitions.emplace(definition.id,&definition).second)return {};
    const auto &instances=s.scene.world.world.facilities;
    std::set<std::uint64_t> seen;
    std::set<std::pair<int,int>> ordinals;
    std::vector<StartupFacilityInformation> result;
    for(const auto id:s.scene.world.facility_order) {
        const auto instance=instances.find(id);
        if(!id || id>=s.next_facility_identity || !seen.insert(id).second ||
           instance==instances.end() || instance->second.placement.instance_id.value!=id)return {};
        const int definition_id=instance->second.placement.definition_id;
        const auto definition=definitions.find(definition_id);
        if(definition==definitions.end() || instance->second.kind!=definition->second->kind)return {};
        const auto &d=*definition->second;
        if(d.kind!=3)continue;
        const auto ordinal=s.facility_ordinals.find(id);
        const auto cash=s.facility_monthly_cash.find(id);
        if(ordinal==s.facility_ordinals.end() || ordinal->second<0 ||
           ordinal->second==std::numeric_limits<int>::max() ||
           !ordinals.emplace(definition_id,ordinal->second).second ||
           cash==s.facility_monthly_cash.end() || d.legacy_icon<0 || d.legacy_icon>6)return {};
        std::uint32_t profit{};
        // APK(total+收入)-支出与Steam先收入-支出再累计均为mod2^32；禁止有符号溢出。
        for(int index=0;index<=month;++index) {
            profit+=static_cast<std::uint32_t>(cash->second[static_cast<std::size_t>(index)][0]);
            profit-=static_cast<std::uint32_t>(cash->second[static_cast<std::size_t>(index)][1]);
        }
        result.push_back({id,definition_id,ordinal->second,d.legacy_icon,d.name,signed_value(profit)});
    }
    // 历史账目可以保留；目录必须恰好覆盖全部活动实例，不从账目map复活退休设施。
    if(seen.size()!=instances.size())return {};
    return result;
}
std::optional<StartupMenuInformation> startup_menu_information(const StartupWorldRuntimeState &s) {
    if(s.village_points<0)return {};
    StartupMenuInformation result;
    result.village_points=s.village_points;
    if(!s.active_task)return result;
    if(!s.rules || s.task_subperiods<0)return {};
    const auto selected=s.tasks.find(*s.active_task);
    if(!*s.active_task || *s.active_task>=s.next_task_identity || selected==s.tasks.end() ||
       selected->second.identity!=*s.active_task ||
       std::count(s.task_order.begin(),s.task_order.end(),*s.active_task)!=1)return {};
    const auto definition_id=selected->second.definition;
    const StartupWorldTask *definition=nullptr;
    for(const auto &candidate:s.rules->tasks)
        if(candidate.factory.identity==definition_id) {
            if(definition)return {};
            definition=&candidate;
        }
    if(!definition || definition->factory.kind<0 || definition->factory.kind>1)return {};
    const auto &ai=s.scene.world.world.ai;
    StartupMenuTaskInformation summary;
    summary.type=definition->factory.kind;
    summary.remaining_subperiods=12-s.task_subperiods; // 原未夹到0；不得换成月数。
    const auto actor_count=[&](const auto &order,ref::ActorKind kind)->std::optional<int> {
        if(order.size()>static_cast<std::size_t>(std::numeric_limits<int>::max()))return {};
        std::set<ref::CharacterId> seen;
        for(const auto id:order) {
            const auto actor=ai.battle.actors.find(id);
            if(!id.value || id.value>=ai.next_actor_id || !seen.insert(id).second || actor==ai.battle.actors.end() ||
               !(actor->second.id==id) || actor->second.kind!=kind || ai.retired_actors.count(id))return {};
        }
        return static_cast<int>(order.size());
    };
    const auto humans=actor_count(ai.human_order,ref::ActorKind::human);
    const auto monsters=actor_count(ai.monster_order,ref::ActorKind::monster);
    if(!humans || !monsters)return {};
    summary.humans=*humans;
    summary.monsters=*monsters;
    if(!ai.human_order.empty())
        summary.portrait_definition=ai.battle.actors.find(ai.human_order.front())->second.definition;
    // 空名单仍取定义0的当前共享成长和主角覆盖，不读创建时metadata或冻结性别。
    if(std::count_if(s.rules->humans.begin(),s.rules->humans.end(),[&](const auto &human) {
           return human.identity==summary.portrait_definition;
       })!=1)return {};
    const auto growth=ai.growth.find(summary.portrait_definition);
    const auto profile=startup_world_human_profile(s,summary.portrait_definition);
    if(growth==ai.growth.end() || !profile)return {};
    summary.portrait_profession=growth->second.definition.current_profession;
    summary.portrait_sex=profile->sex;
    if(summary.portrait_profession<0 ||
       static_cast<std::size_t>(summary.portrait_profession)>=s.rules->jobs.size())return {};
    summary.portrait_body=s.rules->jobs[static_cast<std::size_t>(summary.portrait_profession)]
                              .sprites[static_cast<std::size_t>(summary.portrait_sex)];
    // 正式human资源目录缺32，公开支持0..31/33..37；拒绝无法消费的身体计划。
    if(summary.portrait_body<0 || summary.portrait_body==32 || summary.portrait_body>37)return {};
    std::set<std::uint64_t> facilities;
    for(const auto id:s.scene.world.facility_order) {
        const auto instance=s.scene.world.world.facilities.find(id);
        if(!id || id>=s.next_facility_identity || !facilities.insert(id).second ||
           instance==s.scene.world.world.facilities.end() ||
           instance->second.placement.instance_id.value!=id)return {};
        const StartupDefinition *source=nullptr;
        for(const auto &candidate:s.rules->facilities)
            if(candidate.id==instance->second.placement.definition_id) {
                if(source)return {};
                source=&candidate;
            }
        if(!source || instance->second.kind!=source->kind)return {};
        if(source->kind==12) {
            if(summary.residences==std::numeric_limits<int>::max())return {};
            ++summary.residences;
        }
    }
    if(selected->second.facility) {
        // 非空引用只控制怪物栏准入，但不能把退休/不存在的设施当成合法隐藏条件。
        if(!facilities.count(*selected->second.facility))return {};
    } else {
        const auto progress=s.task_progress.definitions.find(definition_id);
        if(progress==s.task_progress.definitions.end())return {};
        const auto quota=ref::prepare_task_encounter_quota(
            definition->encounter_quota,progress->second.completed,progress->second.flags);
        if(!quota)return {};
        std::int64_t remaining=*quota;
        if(s.task.encounter) {
            const auto encounter=ai.encounters.find(*s.task.encounter);
            if(encounter==ai.encounters.end() || encounter->second.runtime.id!=*s.task.encounter ||
               ai.retired_encounters.count(*s.task.encounter) ||
               std::count(ai.encounter_order.begin(),ai.encounter_order.end(),*s.task.encounter)!=1 ||
               encounter->second.runtime.spawned<0 || encounter->second.linked_monsters<0)return {};
            remaining+=static_cast<std::int64_t>(encounter->second.linked_monsters)-
                       encounter->second.runtime.spawned;
        }
        if(remaining<std::numeric_limits<int>::min() || remaining>std::numeric_limits<int>::max())return {};
        result.monster_remaining=static_cast<int>(remaining);
    }
    result.task=summary;
    return result;
}
} // namespace dungeon_village_prototype
