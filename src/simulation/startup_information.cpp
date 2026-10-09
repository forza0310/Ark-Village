#include "ark/simulation/startup_information.hpp"

#include <limits>

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
} // namespace ark::simulation
