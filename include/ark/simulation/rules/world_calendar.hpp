#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace ark::simulation::rules {
struct WorldCalendarState {
    std::int32_t year{};           // P.b[1]：显示前的原始年值。
    std::int32_t month{};          // P.b[2]：0..11。
    std::int32_t subperiod{};      // P.b[3]：0..3，不解释为一日。
    std::int32_t units{};          // P.b[4]：0..10799。
    std::int32_t previous_units{}; // P.b[5]：每次c(R)先保存旧b[4]。
    std::int32_t month_ticks{};    // UserData.t：每次c调用加1，月段再清0。
};
// 原作独立方法/循环的提交顺序；不是可选表现通知。没有实际消费者就不能跨界成功。
enum class WorldCalendarStage {
    checkpoint_before_normalize, // av.a(true)看到旧units、新previous_units、旧tick/日期。
    year_statistics,             // aH.D()
    year_characters,             // 全bv.k()，不筛p。
    year_facilities,             // 全g.g()
    year_refresh,                // aH.i()
    month_special_scripts,       // 年15/月3及月11脚本/页面。
    month_kill_reset,            // 清全bw.u，t已先清0。
    month_rank_check,            // a.n.a(k)→脚本36/页面48。
    month_resident_countdown,    // bv.p!=0 && m>0 → m--。
    month_resident_presence,     // bv.p!=0：bl中有同n则aq=0，否则aq++。
    month_shop_restock,          // (month+1)%3==2 && k(16) → H，by/a.g商店补货，不是bq任务。
    month_quarter_items,         // month2/5/8/11：q=o+3、全bx.o清位4。
    month_equipment,             // 全bv.p!=0的全部A正槽减1，不限bl。
    month_facility_definitions,  // a.o.g()
    month_facility_age,          // 全g.y[1]++。
    month_housing_tax,           // month3且year>0：住宅G/脚本122/页面90/123。
    month_all_residents_script,  // 未203且全bv.D[2]==1 → 脚本203。
    month_kill_display_clear,    // aH.G()：清N。
    subperiod_refresh,           // aH.b(true)，不是月报准备。
    subperiod_task_deadline,     // 活跃任务y++、y>=12结果页、y==8提示。
    subperiod_task_midpoint,     // subperiod2且任务o位2：选k，a(-10,true,3)。
    subperiod_task_generation,   // subperiod1/3：资格、随机、脚本/页面；使用共同随机流。
    subperiod_capacity_hint      // subperiod3：脚本163的设施比例检查。
};
using WorldCalendarConsumer = std::function<std::optional<WorldCalendarState>(
    const WorldCalendarState &, WorldCalendarStage)>;
enum class WorldCalendarError {
    none,
    invalid_state,
    overflow,
    missing_consumer,
    consumer_failed,
    invalid_calendar_mutation
};
struct WorldCalendarCandidate {
    WorldCalendarState state;
    std::vector<WorldCalendarStage> calls;
};
struct WorldCalendarResult {
    WorldCalendarError error{WorldCalendarError::none};
    std::optional<WorldCalendarCandidate> candidate;
};
// 主场景b()入口只在state0且P13恰1时选择两轮；轮内切state后不重新计算轮数。
int reference_world_frame_rounds(int scene_state, int speed_setting);
bool valid_world_calendar_state(const WorldCalendarState &state);
// b/c.java:c(int)，低层MainScene:368起。只接受已正常归一化日期、非负推进量。
// 不是主场景/菜单资格判断器，也不自行准备月报；月报在之前UserData.e()中处理。
// 所有callback必须只操作私有候选值；年月/子周期比较只看最终值，不按跳过月数重复执行。
WorldCalendarResult prepare_world_calendar(const WorldCalendarState &state, std::int32_t advance,
                                           const WorldCalendarConsumer &consumer);
// MainScene:L28a→L2b6：定义p非零才扣全部A正槽，0/负哨兵不变。
std::vector<std::int32_t> prepare_world_month_equipment(std::int32_t definition_presence,
                                                        const std::vector<std::int32_t> &slots);

// 任务、人物定义、设施定义、随机和脚本仍由外层Owner唯一拥有，不能闭包修改真实状态。
template <class Owner> struct OwnedWorldCalendarAdapter {
    std::function<const WorldCalendarState &(const Owner &)> read;
    std::function<WorldCalendarState &(Owner &)> write;
    std::function<std::optional<Owner>(const Owner &, WorldCalendarStage)> consume;
};
template <class Owner> struct OwnedWorldCalendarResult {
    WorldCalendarError error{WorldCalendarError::none};
    std::optional<Owner> state;
    std::optional<WorldCalendarCandidate> audit;
};
template <class Owner>
OwnedWorldCalendarResult<Owner>
prepare_owned_world_calendar(const Owner &state, std::int32_t advance,
                             const OwnedWorldCalendarAdapter<Owner> &adapter) {
    if (!adapter.read || !adapter.write)
        return {WorldCalendarError::missing_consumer, {}, {}};
    Owner scratch = state;
    WorldCalendarConsumer consumer;
    if (adapter.consume) {
        consumer = [&](const WorldCalendarState &calendar,
                       WorldCalendarStage stage) -> std::optional<WorldCalendarState> {
            adapter.write(scratch) = calendar;
            auto next = adapter.consume(scratch, stage);
            if (!next)
                return {};
            scratch = std::move(*next);
            return adapter.read(scratch);
        };
    }
    const auto result = prepare_world_calendar(adapter.read(state), advance, consumer);
    if (!result.candidate)
        return {result.error, {}, {}};
    adapter.write(scratch) = result.candidate->state;
    return {WorldCalendarError::none, std::move(scratch), result.candidate};
}
} // namespace ark::simulation::rules
