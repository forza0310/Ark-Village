#include "ark/simulation/facilities/rules/facility_items.hpp"
#include "ark/simulation/facilities/rules/world_magic_pot.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

using namespace ark::simulation::rules;

namespace {

int checks = 0;
void check(bool condition, const char *message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

FacilityItemDefinition cafe() {
    FacilityItemDefinition result;
    result.definition_id = 36;
    result.legacy_icon = 2;
    result.category_affinities = {0, 0, 0, 0, 0, 2, 0, 0, 1, 1, 1, 0};
    result.economy.attributes = {LevelEndpoints{420, 630}, LevelEndpoints{6, 60},
                                 LevelEndpoints{5, 50}, LevelEndpoints{192, 384}};
    result.economy.upgrade_uses = {30, 300};
    result.economy.construction_cost = 700;
    result.economy.construction_ticks = 400;
    result.economy.legacy_flags = 262244;
    return result;
}

ImprovementItemDefinition milk() { return {1, 5, {30, 4, 0}}; }

FacilityItemCandidate prepare(const FacilityItemDefinition &facility,
                              const ImprovementItemDefinition &item,
                              const FacilityEconomyInput &input = {},
                              const FacilityEventCounters &counters = {}, int inventory = 1) {
    const auto result = prepare_facility_item(facility, item, input, counters, inventory);
    check(result.error == FacilityItemError::none && result.candidate.has_value(),
          "complete item candidate produced");
    return *result.candidate;
}

void fixed_records_and_affinities() {
    const auto result = prepare(cafe(), milk(), {}, {}, 2);
    check(result.definition_id == 36 && result.item_id == 1 && result.remaining_inventory == 1,
          "identity and inventory carried in complete candidate");
    check(result.applied_improvements == std::array<std::int32_t, 3>{60, 8, 0} &&
              result.definition_improvements == std::array<std::int32_t, 4>{60, 8, 0, 0},
          "cafe category-five doubles milk improvements");
    check(result.before.instance_attributes == std::array<std::int64_t, 4>{420, 6, 5, 190} &&
              result.after.instance_attributes == std::array<std::int64_t, 4>{480, 14, 5, 190} &&
              result.visible_deltas == std::array<std::int64_t, 3>{60, 8, 0} &&
              result.legacy_response == 2,
          "actual old and new attributes include maintenance rounding");
    check(!result.instance_event.script_triggered &&
              result.instance_event.counters.item_confirmations == 1 &&
              result.instance_event.counters.months_since_trigger == 0,
          "item confirmation belongs to selected instance");
    const auto half = prepare(cafe(), {9, 8, {10, 1, 1}});
    check(half.applied_improvements == std::array<std::int32_t, 3>{5, 0, 0} &&
              half.legacy_response == 1,
          "half affinity truncates each odd increment individually");
    const auto neutral = prepare(cafe(), {0, 1, {0, 2, 0}});
    check(neutral.applied_improvements == std::array<std::int32_t, 3>{0, 2, 0} &&
              neutral.legacy_response == 0 && neutral.remaining_inventory == 0,
          "neutral affinity does not disable item use");
    const auto signed_half = prepare(cafe(), {100, 8, {-3, -5, 3}});
    check(signed_half.applied_improvements == std::array<std::int32_t, 3>{-1, -2, 1},
          "signed fixture halves toward zero, not down");
    const auto negative = prepare(cafe(), {100, 5, {-3, -5, -3}});
    check(negative.visible_deltas == std::array<std::int64_t, 3>{-6, -10, -6} &&
              negative.after.instance_attributes[1] == -4,
          "signed fixture does not invent a lower attribute cap");
}

void separate_improvement_phase() {
    FacilityEconomyInput input;
    input.definition_improvements = {0, 0, 0, 7};
    const auto improved = prepare_facility_improvement(cafe(), milk(), input);
    check(improved.candidate && improved.error == FacilityItemError::none &&
              improved.candidate->definition_improvements ==
                  std::array<std::int32_t, 4>{60, 8, 0, 7} &&
              improved.candidate->visible_deltas == std::array<std::int64_t, 3>{60, 8, 0},
          "raw76 improvement remains available after previous raw75 exhausted inventory");
    input.definition_improvements[0] = std::numeric_limits<int>::max();
    const auto refused = prepare_facility_improvement(cafe(), milk(), input);
    check(!refused.candidate && refused.error == FacilityItemError::numeric_overflow,
          "separate raw76 rejects overflowing shared improvement without partial candidate");
}

void shared_definition_and_caps() {
    FacilityEconomyInput input;
    input.level = 2;
    input.definition_improvements = {0, 0, 0, 7};
    input.instance_modifiers = {30, 1, 2, 999};
    input.legacy_job_counts[1] = 1;
    input.completed_definition_uses = 123;
    const auto original = input;
    const auto result = prepare(cafe(), milk(), input, {4, 36}, 999);
    check(result.visible_deltas == std::array<std::int64_t, 3>{66, 8, 0},
          "price delta includes definition multiplier before instance modifier");
    check(result.instance_event.script_triggered &&
              result.instance_event.counters.item_confirmations == 0 &&
              result.instance_event.counters.months_since_trigger == 0,
          "confirmed item candidate includes script trigger and two-counter reset");
    check(result.definition_improvements[3] == 7 &&
              result.before.instance_attributes[3] == result.after.instance_attributes[3] &&
              result.after.upgrade_uses == result.before.upgrade_uses &&
              result.after.upgrade_ready == result.before.upgrade_ready &&
              result.after.construction_cost == result.before.construction_cost &&
              result.after.construction_ticks == result.before.construction_ticks,
          "item does not change maintenance improvement, level, uses or construction inputs");
    check(input.level == original.level &&
              input.definition_improvements == original.definition_improvements &&
              input.instance_modifiers == original.instance_modifiers &&
              input.legacy_job_counts == original.legacy_job_counts &&
              input.completed_definition_uses == original.completed_definition_uses,
          "preparing candidate does not mutate caller input");
    auto other_before_input = input;
    other_before_input.instance_modifiers = {100, 10, 20, 0};
    auto other_after_input = other_before_input;
    other_after_input.definition_improvements = result.definition_improvements;
    const auto other_before = derive_facility_economy(cafe().economy, other_before_input);
    const auto other_after = derive_facility_economy(cafe().economy, other_after_input);
    check(other_after.values->instance_attributes[0] -
                      other_before.values->instance_attributes[0] ==
                  66 &&
              other_after.values->instance_attributes[1] -
                      other_before.values->instance_attributes[1] ==
                  8,
          "same definition improvement affects another instance with its own neighbours");
    const FacilityEventCounters other_counters{4, 35};
    check(other_counters.item_confirmations == 4 && other_counters.months_since_trigger == 35,
          "other instance event counters are not shared definition data");
    input = {};
    input.definition_improvements = {10000, 1000, 1000, 0};
    const auto capped = prepare(cafe(), milk(), input, {4, 36});
    check(capped.visible_deltas == std::array<std::int64_t, 3>{0, 0, 0} &&
              capped.legacy_response == -1 && capped.remaining_inventory == 0,
          "all capped attributes still consume item with no-visible-change response");
    check(capped.definition_improvements == std::array<std::int32_t, 4>{10060, 1008, 1000, 0} &&
              capped.instance_event.script_triggered,
          "capped item keeps raw shared accumulation and still triggers instance script");
    const auto empty_delta = prepare(cafe(), {100, 5, {0, 0, 0}}, {}, {4, 36});
    check(empty_delta.legacy_response == -1 && empty_delta.remaining_inventory == 0 &&
              empty_delta.instance_event.script_triggered,
          "zero-effect fixture is a confirmed consumed item, not a cancelled action");
}

void failures_and_limits() {
    auto facility = cafe();
    auto item = milk();
    FacilityEconomyInput input;
    auto rejects = [&](FacilityItemError error, FacilityEventCounters counters = {},
                       int stock = 1) {
        const auto original_improvements = input.definition_improvements;
        const auto result = prepare_facility_item(facility, item, input, counters, stock);
        check(result.error == error && !result.candidate &&
                  input.definition_improvements == original_improvements,
              "failure does not expose partial stock, improvements or counters");
    };
    rejects(FacilityItemError::no_inventory, {}, 0);
    rejects(FacilityItemError::invalid_input, {}, -1);
    rejects(FacilityItemError::invalid_input, {}, 1000);
    facility.definition_id = -1;
    rejects(FacilityItemError::invalid_input);
    facility = cafe();
    facility.legacy_icon = -1;
    rejects(FacilityItemError::invalid_input);
    facility = cafe();
    facility.category_affinities[0] = 3;
    rejects(FacilityItemError::invalid_input);
    facility = cafe();
    facility.category_affinities.clear();
    rejects(FacilityItemError::invalid_input);
    facility = cafe();
    item.item_id = -1;
    rejects(FacilityItemError::invalid_input);
    item = milk();
    item.legacy_category = -1;
    rejects(FacilityItemError::invalid_input);
    item.legacy_category = 12;
    rejects(FacilityItemError::invalid_input);
    item = milk();
    input.level = 6;
    rejects(FacilityItemError::invalid_input);
    input = {};
    rejects(FacilityItemError::invalid_input, {-1, 0});
    rejects(FacilityItemError::invalid_input, {0, -1});
    rejects(FacilityItemError::numeric_overflow, {std::numeric_limits<std::int64_t>::max(), 36});
    const auto maximum = std::numeric_limits<std::int32_t>::max();
    const auto minimum = std::numeric_limits<std::int32_t>::min();
    item.improvements = {maximum, 0, 0};
    rejects(FacilityItemError::numeric_overflow);
    item.improvements = {minimum, 0, 0};
    rejects(FacilityItemError::numeric_overflow);
    item = milk();
    input.definition_improvements[0] = maximum - 59;
    rejects(FacilityItemError::numeric_overflow);
    input.definition_improvements[0] = maximum - 60;
    check(prepare(facility, item, input).definition_improvements[0] == maximum,
          "last representable raw improvement accepted despite cap");
    input = {};
    item.improvements = {-1, 0, 0};
    input.definition_improvements[0] = minimum;
    rejects(FacilityItemError::numeric_overflow);
    facility.economy.attributes[0] = {1000000000, 1000000000};
    facility.economy.legacy_flags = 4096;
    input = {};
    input.legacy_job_counts[2] = 400000000;
    item = {100, 1, {1000000000, 0, 0}};
    rejects(FacilityItemError::numeric_overflow);
    input.legacy_job_counts[2] = maximum;
    rejects(FacilityItemError::numeric_overflow);
}

void random_exact_scaling() {
    std::mt19937 random(0x17E17U);
    for (int trial = 0; trial < 2000; ++trial) {
        auto facility = cafe();
        facility.economy.legacy_flags = 0;
        facility.economy.attributes = {LevelEndpoints{1000, 1000}, LevelEndpoints{1000, 1000},
                                       LevelEndpoints{1000, 1000}, LevelEndpoints{100, 100}};
        const auto affinity = static_cast<std::int32_t>(random() % 3U);
        facility.category_affinities[5] = affinity;
        auto item = milk();
        FacilityEconomyInput input;
        input.definition_improvements[3] = 7;
        std::array<std::int32_t, 4> expected_improvements = input.definition_improvements;
        std::array<std::int32_t, 3> expected_scaled{};
        std::array<std::int64_t, 3> expected_visible{};
        bool any_change = false;
        for (std::size_t slot = 0; slot < 3; ++slot) {
            item.improvements[slot] = static_cast<std::int32_t>(random() % 201U) - 100;
            input.definition_improvements[slot] = static_cast<std::int32_t>(random() % 201U) - 100;
            input.instance_modifiers[slot] = static_cast<std::int32_t>(random() % 101U) - 50;
            auto amount = item.improvements[slot];
            if (affinity == 1) {
                amount = amount < 0 ? -((-amount) / 2) : amount / 2;
            } else if (affinity == 2) {
                amount += amount;
            }
            expected_scaled[slot] = amount;
            expected_improvements[slot] = input.definition_improvements[slot] + amount;
            expected_visible[slot] = amount;
            any_change = any_change || amount != 0;
        }
        const FacilityEventCounters counters{static_cast<std::int64_t>(random() % 7U),
                                             30 + static_cast<std::int64_t>(random() % 11U)};
        const auto inventory = static_cast<int>(random() % 999U) + 1;
        const auto result = prepare(facility, item, input, counters, inventory);
        check(result.applied_improvements == expected_scaled &&
                  result.definition_improvements == expected_improvements &&
                  result.visible_deltas == expected_visible,
              "random small signed values match independent integer scaling and addition");
        check(result.remaining_inventory == inventory - 1 &&
                  result.legacy_response == (any_change ? affinity : -1),
              "random complete consumption and no-change response");
        const bool triggered =
            counters.item_confirmations >= 4 && counters.months_since_trigger >= 36;
        check(result.instance_event.script_triggered == triggered &&
                  result.instance_event.counters.item_confirmations ==
                      (triggered ? 0 : counters.item_confirmations + 1) &&
                  result.instance_event.counters.months_since_trigger ==
                      (triggered ? 0 : counters.months_since_trigger),
              "random item and instance event coordination");
        for (std::size_t slot = 0; slot < 3; ++slot) {
            check(result.before.instance_attributes[slot] ==
                          1000 + input.definition_improvements[slot] +
                              input.instance_modifiers[slot] &&
                      result.after.instance_attributes[slot] ==
                          1000 + expected_improvements[slot] + input.instance_modifiers[slot],
                  "independent noncapped derived attributes");
        }
        check(result.after.instance_attributes[3] == 100 && result.definition_improvements[3] == 7,
              "maintenance unchanged and rounded independently of item");
    }
}

// 魔法壶与本套件的设施投入共用纯候选/库存生命周期；数值为隔离夹具，不是新局初值。
WorldMagicPotState pot_fixture() {
    return {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0};
}
void magic_pot_dates_and_comments() {
    for (const auto &row : std::array<std::array<int, 4>, 3>{{{0, 0, 0, 0}, {2, 11, 3, 143}, {3, 0, 0, 144}}}) {
        const auto r = world_magic_pot_date({row[0], row[1], row[2]});
        check(r.value && *r.value == row[3], "壶日期使用原年/月/周序号，不是墙钟");
    }
    for (int level = 1; level <= 3; ++level)
        check(world_magic_pot_capacity(level) == std::optional<std::int32_t>(level * 10),
              "壶级1/2/3容量固定10/20/30");
    check(!world_magic_pot_capacity(0) && !world_magic_pot_capacity(4), "坏壶级显式拒绝");
    for (const auto &row : std::array<std::array<int, 3>, 6>{{{0, 2, 2}, {4, 2, 2}, {5, 1, 3}, {9, 1, 3}, {10, 0, 4}, {16, 0, 4}}}) {
        const WorldMagicPotItem item{7, {row[0], 0, 0, 0}};
        const auto comment = world_magic_pot_comment(item);
        check(comment.candidate && comment.candidate->group == row[1] && comment.candidate->bound == row[2],
              "评语元素和边界独立oracle：4/3/2条，由Owner抽票");
        for (int ticket = 0; ticket < row[2]; ++ticket) {
            const auto r = prepare_world_magic_pot_deposit(pot_fixture(), item, 1, {0, 0, 0}, ticket);
            check(r.candidate && r.candidate->comment_group == row[1] && r.candidate->comment_ticket == ticket,
                  "纯投入回传合法票，不消费私有随机");
        }
        for (int ticket : {-1, row[2]}) {
            const auto r = prepare_world_magic_pot_deposit(pot_fixture(), item, 1, {0, 0, 0}, ticket);
            check(!r.candidate && r.error == WorldMagicPotError::invalid_input, "评语票负值/恰bound拒绝");
        }
    }
}
void magic_pot_deposit_candidates() {
    auto s = pot_fixture();
    s[0] = 7;
    s[3] = 998; s[4] = 10; s[5] = 20; s[6] = 30;
    s[7] = 3; s[8] = 4; s[9] = 5; s[10] = 6;
    const auto before = s;
    const WorldMagicPotItem item{9, {5, 3, 1, 0}};
    const auto r = prepare_world_magic_pot_deposit(s, item, 2, {0, 0, 2}, 2);
    const WorldMagicPotState expected{7, 1, 0, 999, 13, 21, 30, 5, 5, 5, 6, 1, 2};
    const std::array<std::array<std::int32_t, 4>, 3> display{{{998, 10, 20, 30}, {999, 13, 21, 30}, {5, 3, 1, 0}}};
    check(r.candidate && r.candidate->state == expected && r.candidate->inventory == 1 &&
              r.candidate->display == display && r.candidate->comment_group == 1,
          "投入库存/四元素/整数半值/首次日期整体候选，封顶显示仍是原增量5");
    check(s == before, "投入源13槽和库存输入不变");
    const auto again = prepare_world_magic_pot_deposit(expected, item, 1, {0, 1, 0}, 0);
    check(again.candidate && again.candidate->state[1] == 2 && again.candidate->state[12] == 2 &&
              again.candidate->state[0] == 7 && again.candidate->inventory == 0,
          "后续投入不刷新首次日期、不增加经验");
    auto full = pot_fixture(); full[1] = 10;
    const auto denied = prepare_world_magic_pot_deposit(full, item, 1, {0, 0, 0}, 0);
    check(!denied.candidate && denied.error == WorldMagicPotError::none && denied.denial == WorldMagicPotDenial::full19,
          "满容量是事件19资格拒绝，不是数值错误");
    const auto empty = prepare_world_magic_pot_deposit(s, item, 0, {0, 0, 2}, 0);
    check(!empty.candidate && empty.error == WorldMagicPotError::none && empty.denial == WorldMagicPotDenial::no_inventory,
          "零库存无候选，不能把调用当一次投入");
}
void magic_pot_processing_candidates() {
    const WorldMagicPotState s{995, 3, 0, 20, 30, 40, 50, 5, 8, 2, 3, 1, 0};
    const auto r = prepare_world_magic_pot_processing(s, {0, 0, 1});
    const WorldMagicPotState expected{996, 2, 1, 21, 32, 40, 50, 4, 6, 2, 3, 1, 1};
    const std::array<std::array<std::int32_t, 5>, 3> display{{{20, 30, 40, 50, 995}, {21, 32, 40, 50, 996}, {1, 2, 0, 0, 1}}};
    check(r.candidate && r.candidate->changed && r.candidate->processed == 1 && r.candidate->percentage == 33 &&
              r.candidate->produced == std::array<std::int32_t, 4>{1, 2, 0, 0} &&
              r.candidate->state == expected && r.candidate->display == display,
          "原1/3先舍入33再元素乘除100，两次舍入及经验第5列独立oracle");
    check(s == WorldMagicPotState{995, 3, 0, 20, 30, 40, 50, 5, 8, 2, 3, 1, 0}, "处理不修改输入");
    for (int pending : {0, 3}) {
        auto no_output = pot_fixture(); no_output[1] = pending;
        const auto zero = prepare_world_magic_pot_processing(no_output, {0, 0, 1});
        check(zero.candidate && !zero.candidate->changed && zero.candidate->state == no_output &&
                  zero.candidate->produced == std::array<std::int32_t, 4>{},
              "无有效输出13槽未变/aQ输出为零；不据changed=false断言所有Owner附属未变");
    }
    const auto same_date = prepare_world_magic_pot_processing(s, {0, 0, 0});
    check(same_date.candidate && !same_date.candidate->changed && same_date.candidate->state == s,
          "同日期无需再次处理或刷新13槽");
    auto fallback = pot_fixture(); fallback[1] = 10;
    const auto three = prepare_world_magic_pot_processing(fallback, {0, 2, 2});
    check(three.candidate && three.candidate->changed && three.candidate->processed == 10 &&
              three.candidate->produced == std::array<std::int32_t, 4>{1, 1, 1, 0} &&
              three.candidate->state == WorldMagicPotState{10, 0, 10, 1, 1, 1, 0, 0, 0, 0, 0, 1, 10},
          "无元素且达到容量才兜底火冰雷各1，暗不补，经验/时间/件数同时提交");
    fallback[11] = 2;
    const auto partial = prepare_world_magic_pot_processing(fallback, {0, 2, 2});
    check(partial.candidate && !partial.candidate->changed && partial.candidate->state == fallback,
          "待件数全处理但尚未达到20容量不能误用10级兜底");
    const WorldMagicPotState capped{999, 10, 0, 999, 999, 999, 999, 10, 10, 10, 10, 1, 0};
    const auto cap = prepare_world_magic_pot_processing(capped, {0, 2, 2});
    check(cap.candidate && cap.candidate->state[0] == 999 && cap.candidate->state[3] == 999 &&
              cap.candidate->produced == std::array<std::int32_t, 4>{10, 10, 10, 10} &&
              cap.candidate->display[2] == std::array<std::int32_t, 5>{10, 10, 10, 10, 10} &&
              cap.candidate->state[7] == 0 && cap.candidate->state[1] == 0,
          "处理封顶仍保留原输出/经验显示量，不按封顶实际差额回退待元素或件数");
}
void magic_pot_recipe_contracts() {
    const std::vector<WorldMagicPotRecipeDefinition> recipes{
        {7, "隔离配方A", 0, 3, 3, {1, 2, 3, 4}, 2},
        {2, "隔离隐藏B", 1, 4, 0, {0, 0, 0, 0}, 0},
        {9, "隔离配方C", 4, 40, 8, {2, 0, 0, 0}, 2},
        {1, "隔离配方D", 2, 5, 2, {0, 1, 0, 0}, 2}};
    const auto list = catalogue_world_magic_pot_recipes(recipes);
    check(list.identities && *list.identities == std::vector<std::int32_t>{7, 9, 1}, "配方flags2目录保持定义原序，不按ID重排");
    auto s = pot_fixture(); s[0] = 4;
    const std::vector<WorldMagicPotRecipeProgress> progress{{1, 0, false}, {9, 0, false}, {7, 0, false}, {2, 0, false}};
    const auto discoverable = discoverable_world_magic_pot_recipes(s, recipes, progress);
    check(discoverable.identities && *discoverable.identities == std::vector<std::int32_t>{7, 1},
          "发现只选未发现/flags2/足经验，进度输入反序也保留定义原序");
    const auto found = prepare_world_magic_pot_discovery({7, 0, false});
    const auto old = prepare_world_magic_pot_discovery({7, 1, false});
    check(found.candidate && found.candidate->status == 1 && found.candidate->pending_notice &&
              old.candidate && old.candidate->status == 1 && !old.candidate->pending_notice,
          "46只独立发现p/r，已发现不重造NEW提示或奖励");
    check(check_world_magic_pot_recipe_selection(recipes[0], {7, 0, false}, 1).denial == WorldMagicPotDenial::undiscovered,
          "43 p0选择拒绝，不提前扣元素");
    for (int type = 0; type <= 4; ++type) {
        auto recipe = recipes[0]; recipe.reward_kind = type;
        for (int status = 0; status <= 2; ++status) {
            const auto gate = check_world_magic_pot_recipe_selection(recipe, {7, 1, false}, status);
            const bool owned = (type >= 1 && type <= 3 && status == 1) || (type == 4 && status == 2);
            check(gate.error == WorldMagicPotError::none && gate.denial == (owned ? WorldMagicPotDenial::already_owned106 : WorldMagicPotDenial::none),
                  "43已有装备p1/设施p2拒绝，普通道具可重复且不是库存判断");
        }
    }
    s[3] = 5; s[4] = 6; s[5] = 7; s[6] = 8;
    const auto cost = prepare_world_magic_pot_recipe_cost(s, recipes[0]);
    auto expected = s; expected[3] = expected[4] = expected[5] = expected[6] = 4;
    check(cost.candidate && cost.candidate->state == expected && cost.candidate->recipe == 7 &&
              cost.candidate->reward_kind == 0 && cost.candidate->reward_definition == 3,
          "47仅扣足额四元素并带奖励意图，不再合入43发现/拥有策略或授予库存");
    auto owned_recipe = recipes[0]; owned_recipe.reward_kind = 1;
    const auto separate = prepare_world_magic_pot_recipe_cost(s, owned_recipe);
    check(check_world_magic_pot_recipe_selection(owned_recipe, {7, 1, false}, 1).denial ==
              WorldMagicPotDenial::already_owned106 && separate.candidate && separate.candidate->state == expected,
          "43已有装备拒绝与47足额四成本是不同调用合同，47不重跑43门槛");
    s[6] = 3; const auto before = s;
    const auto insufficient = prepare_world_magic_pot_recipe_cost(s, recipes[0]);
    check(!insufficient.candidate && insufficient.denial == WorldMagicPotDenial::insufficient_elements && s == before,
          "47第四元素不足拒绝，没有前三槽部分扣费");
    auto broken = progress; broken[0].identity = 9;
    check(discoverable_world_magic_pot_recipes(s, recipes, broken).error == WorldMagicPotError::invalid_input,
          "进度重复/缺身份明确拒绝，不按数组位置猜配方");
    auto duplicate = recipes; duplicate[1].identity = 7;
    check(catalogue_world_magic_pot_recipes(duplicate).error == WorldMagicPotError::invalid_input,
          "定义重复不静默去重");
}
void magic_pot_rejections_and_overflow() {
    constexpr auto maximum = std::numeric_limits<std::int32_t>::max();
    const WorldMagicPotItem item{1, {2, 0, 0, 0}};
    for (int slot : {0, 1, 2, 3, 7, 11, 12}) {
        auto bad = pot_fixture(); bad[slot] = -1; const auto before = bad;
        const auto deposit = prepare_world_magic_pot_deposit(bad, item, 1, {0, 0, 0}, 0);
        const auto processing = prepare_world_magic_pot_processing(bad, {0, 0, 0});
        check(!valid_world_magic_pot_state(bad, {0, 0, 0}) &&
                  !deposit.candidate && deposit.error == WorldMagicPotError::invalid_input &&
                  !processing.candidate && processing.error == WorldMagicPotError::invalid_input && bad == before,
              "负13槽状态两消费者拒绝且源不变");
    }
    for (const auto date : {WorldMagicPotDate{-1, 0, 0}, {0, 12, 0}, {0, 0, 4}})
        check(world_magic_pot_date(date).error == WorldMagicPotError::invalid_input, "日期三域越界拒绝");
    check(world_magic_pot_date({maximum, 0, 0}).error == WorldMagicPotError::numeric_overflow,
          "日期序号不能有符号回绕");
    auto future = pot_fixture(); future[12] = 1;
    check(!valid_world_magic_pot_state(future, {0, 0, 0}) &&
              !prepare_world_magic_pot_processing(future, {0, 0, 0}).candidate,
          "未来处理日期为维护非法状态，不自动补时间");
    check(valid_world_magic_pot_state(future, {0, 0, 1}) && future[12] == 1,
          "恢复只读接受当前日期，不执行处理或创建输出");
    for (const auto &change : std::array<std::array<int, 2>, 5>{{{0, 1000}, {1, 11}, {2, 11}, {3, 1000}, {11, 4}}}) {
        auto bad = pot_fixture(); bad[change[0]] = change[1];
        check(!valid_world_magic_pot_state(bad, {0, 0, 0}), "经验/待数/最近数/当前元素/壶级上界非法拒绝");
    }
    auto pending = pot_fixture(); pending[7] = maximum; const auto old = pending;
    const auto add = prepare_world_magic_pot_deposit(pending, item, 1, {0, 0, 0}, 0);
    check(!add.candidate && add.error == WorldMagicPotError::numeric_overflow && pending == old,
          "半元素累积溢出整候选拒绝，库存/元素不残留");
    pending[1] = 1;
    const auto product = prepare_world_magic_pot_processing(pending, {0, 0, 1});
    check(!product.candidate && product.error == WorldMagicPotError::numeric_overflow && pending[7] == maximum && pending[1] == 1,
          "待元素乘100中间值溢出拒绝，不用64位放宽原算术域");
    check(!world_magic_pot_comment({1, {maximum, maximum, 0, 0}}).candidate,
          "评语四元素和溢出不抽号");
    check(check_world_magic_pot_recipe_selection({1, "隔离", 0, 0, 0, {}, 2}, {1, 1, false}, 3).error == WorldMagicPotError::invalid_input &&
              !prepare_world_magic_pot_discovery({1, 2, false}).candidate,
          "坏奖励status或配方p不靠自动规范化通过");
    auto invalid_recipe = WorldMagicPotRecipeDefinition{1, "隔离", 0, 0, 0, {-1, 0, 0, 0}, 2};
    check(!prepare_world_magic_pot_recipe_cost(pot_fixture(), invalid_recipe).candidate,
          "负元素成本不能变相增加元素");
}

} // namespace

int main() {
    fixed_records_and_affinities();
    separate_improvement_phase();
    shared_definition_and_caps();
    failures_and_limits();
    random_exact_scaling();
    magic_pot_dates_and_comments();
    magic_pot_deposit_candidates();
    magic_pot_processing_candidates();
    magic_pot_recipe_contracts();
    magic_pot_rejections_and_overflow();
    std::cout << checks << " checks passed\n";
}
