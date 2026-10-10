#pragma once

#include "ark/simulation/village/rules/world_calendar.hpp"
#include "ark/simulation/world/rules/world_random.hpp"

#include <array>

namespace ark::simulation::rules {
struct CalendarMaintenanceHuman {
    int definition{};                        // bv.n，活动名单用同n判断，不用演员实例ID。
    std::int32_t presence{};                 // p，所有非零值都进入对应人物循环。
    std::int32_t leave_months{};             // m。
    std::int32_t absent_months{};            // aq。
    std::vector<std::int32_t> equipment;     // A，全部槽。
    std::vector<std::int32_t> yearly_totals; // B。
    std::int32_t residence_status{};         // D[2]。
    int profession_type{};                   // b().i，仅住宅G计算使用。
    std::int32_t legacy_F{};                 // 不扩展成当前金币或正式税款提交。
    std::int32_t legacy_G{};
};
struct CalendarMaintenanceFacilityDefinition {
    int definition{};             // br.n。
    int category{};               // f82e。
    std::int32_t month_counter{}; // N，g()按category2设30，否则20。
};
struct CalendarMaintenanceFacility {
    std::uint64_t identity{};
    int definition{};
    std::int32_t month_age{};                             // y[1]。
    std::vector<std::array<std::int32_t, 2>> yearly_cash; // v，原数组行数。
};
struct CalendarMaintenanceShopItem {
    int definition{};                // by.n。
    int minimum_rank{};              // i，-1禁止补货。
    std::int32_t maximum_quantity{}; // v。
    std::int32_t quantity{};         // A。
    std::int32_t presence{};         // p。
    std::int32_t legacy_q{};
    bool newly_available{}; // r。
};
// 仅外层唯一Owner的临时投影，不能与真实bv/br/g/by/z/C等分别持久复制。
struct WorldCalendarMaintenanceState {
    WorldRandomStream random;
    std::vector<CalendarMaintenanceHuman> humans;
    std::vector<int> active_human_definitions; // bl中的n，保留重复，存在性判断而非计数。
    std::vector<CalendarMaintenanceFacilityDefinition> facility_definitions;
    std::vector<CalendarMaintenanceFacility> facilities;
    std::vector<CalendarMaintenanceShopItem> shop_items;
    std::vector<std::int32_t> monster_month_kills; // 全bw.u，不能替代永久v。
    std::vector<std::uint32_t> item_flags;         // 全bx.o，季度清位4。
    std::vector<int> kill_display;                 // N，G()只清此列表。
    std::array<std::array<std::array<std::int32_t, 2>, 5>, 12> monthly_cash{}; // z。
    std::vector<std::array<std::int32_t, 10>> yearly_statistics; // C，保留既有累加值。
    std::int32_t rank{};                                         // k。
    std::int32_t popularity{};                                   // f214d。
    std::int32_t legacy_v{};
    std::int32_t legacy_D{};
    std::int32_t legacy_n0{};
    std::int32_t quarter_base{};    // static n.o，必须来自实际Owner。
    std::int32_t quarter_counter{}; // q。
    bool shop_refresh_enabled{};    // UserData位16。
    bool event203_seen{};           // av.f(203)，真实脚本登记源。
};
enum class CalendarMaintenanceRequestKind { hint, script, page };
struct CalendarMaintenanceRequest {
    CalendarMaintenanceRequestKind kind{};
    int code{};
    std::optional<std::int32_t> number; // 脚本123的金额参数；不是现金事务。
    int delay{};                        // hint的av.a(code,null,az16)，原初始计数=-delay、持续80。
};
enum class CalendarMaintenanceError {
    none,
    unsupported_stage,
    invalid_owner,
    overflow,
    random_failed
};
struct CalendarMaintenanceCandidate {
    WorldCalendarMaintenanceState state;
    std::vector<CalendarMaintenanceRequest> pending_requests;
};
struct CalendarMaintenanceResult {
    CalendarMaintenanceError error{CalendarMaintenanceError::none};
    std::optional<CalendarMaintenanceCandidate> candidate;
};
// 只执行已证数组/标量规则，不冒充完整calendar消费者。pending_requests需在原位置
// 交真实脚本/页面/提示消费者后才能提交该stage；不能排成通知后称月界完成。
// 对未知特殊脚本/等级/任务等stage明确unsupported，不能用默认成功回调跳过。
CalendarMaintenanceResult
prepare_world_calendar_maintenance(const WorldCalendarMaintenanceState &state,
                                   const WorldCalendarState &date, WorldCalendarStage stage);
} // namespace ark::simulation::rules
