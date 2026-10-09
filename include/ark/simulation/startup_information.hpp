#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ark::simulation {
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
// 不更新世界、NEW标记、随机、页栈或文件，不等于已维护raw9/36控制器。
std::optional<StartupIncomeInformation> startup_income_information(
    const StartupInformationCash &monthly_cash, int current_month, int period);
} // namespace ark::simulation
