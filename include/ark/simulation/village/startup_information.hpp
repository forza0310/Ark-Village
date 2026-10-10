#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ark::simulation {
struct StartupWorldRuntimeState;
// 信息页只读数据投影；复用Owner的12月×5类别×收入/支出，不持页面或实例引用。
using StartupInformationCash = std::array<std::array<std::array<int, 2>, 5>, 12>;
struct StartupIncomeRow {
    std::string_view label;
    std::int32_t income{}, expense{};
    std::string income_text, expense_text;
};
struct StartupIncomeInformation {
    std::array<StartupIncomeRow, 5> rows;
    std::int32_t profit{};
    std::string profit_text;
};
// 固定APK raw36：period=0读取当前月，1累加本年全部12桶。
// 非法month/period显式拒绝；原Java int回卷后转long的金额语义保留。
// 不更新世界、NEW标记、随机、页栈或文件；raw9/36控制器见startup_world_information.hpp。
std::optional<StartupIncomeInformation> startup_income_information(
    const StartupInformationCash &monthly_cash, int current_month, int period);

struct StartupItemInformation {
    int definition{}, inventory{}, render_icon{};
    bool newly_unlocked{};
    std::string name, description; // item原name及第23列；不以效果摘要替代说明。
};
// raw37只取正库存，保留原定义顺序，不过滤status、不清NEW或使用道具。
std::optional<std::vector<StartupItemInformation>>
startup_item_information(const StartupWorldRuntimeState &state);

enum class StartupInformationEdition { apk_1_0_8, steam_2_56 };
struct StartupEquipmentInformationVisible {
    std::string name;
    int render_icon{}; // 列表图标，武器不是身体PNG的render_image。
    bool newly_unlocked{};
    std::array<std::optional<int>, 2> values; // 只将大于0的属性交数字帮助器；空值按目录版本画占位。
};
struct StartupEquipmentInformationRow {
    int definition{};
    std::optional<StartupEquipmentInformationVisible> visible; // p!=1只保留未知条目身份。
};
struct StartupEquipmentInformation {
    int slot{};
    StartupInformationEdition edition{};
    std::string_view nonpositive_text; // APK空白，Steam字面"--"；不把无正值当未知装备。
    std::array<int, 2> attributes{};
    std::size_t known_count{}; // 当前类中p==1的定义数，不是库存或穿戴件数。
    std::vector<StartupEquipmentInformationRow> rows;
};
// raw38四类目录及原交换排序。版本显式选择：Steam铠甲/饰品另过滤当前flag==0，非正值画"--"。
// 名称/属性来自调用者安装的rules，不声称导入Steam全部数据；不创建页、不清NEW。
std::optional<StartupEquipmentInformation> startup_equipment_information(
    const StartupWorldRuntimeState &state, int slot, StartupInformationEdition edition);
struct StartupTownInformation {
    int rank{}, adventurers{}, residents{}, facilities{}, completed_tasks{}, activities_held{};
    std::array<int,4> known_equipment{}; // 武器/衣(type2)/其它防具/饰品，只计status==1。
};
// APK/Steam34的权威统计，不复用38的flag过滤。居民不额外过滤人物presence，
// 设施按原g每次出现计类型3/9；只读查询不执行Steam入页的平台成就请求。
std::optional<StartupTownInformation> startup_town_information(const StartupWorldRuntimeState &state);
} // namespace ark::simulation
