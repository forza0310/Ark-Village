#pragma once

// 魔法壶纯规则：输入仅是原13槽及定义投影；不持有Owner、页面或随机流。
// 原表/静态合同见rules/MAGIC_POT.md；溢出和坏状态拒绝属于维护安全策略。
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace dungeon_village_reference {
using WorldMagicPotState = std::array<std::int32_t, 13>;
struct WorldMagicPotDate {
    std::int32_t year{}, month{}, subperiod{}; // 原年/月0..11/周序号0..3。
};
struct WorldMagicPotItem {
    std::int32_t identity{};
    std::array<std::int32_t, 4> elements{};
};
struct WorldMagicPotRecipeDefinition {
    std::int32_t identity{};
    std::string name;
    std::int32_t reward_kind{}, reward_definition{}, experience_required{}; // 0道具/1武器/2防具/3饰品/4设施。
    std::array<std::int32_t, 4> costs{};
    std::uint32_t flags{};
};
struct WorldMagicPotRecipeProgress {
    std::int32_t identity{}, status{};
    bool pending_notice{};
};
enum class WorldMagicPotError { none, invalid_input, numeric_overflow };
enum class WorldMagicPotDenial {
    none,
    full19,
    no_inventory,
    undiscovered,
    already_owned106,
    insufficient_elements
};
struct WorldMagicPotDateResult {
    WorldMagicPotError error{WorldMagicPotError::none};
    std::optional<std::int32_t> value;
};
WorldMagicPotDateResult world_magic_pot_date(const WorldMagicPotDate &date);
std::optional<std::int32_t> world_magic_pot_capacity(std::int32_t level);
// 恢复只读门槛，不执行处理、不创建aQ输出；未来时间戳拒绝为维护策略。
bool valid_world_magic_pot_state(const WorldMagicPotState &state, const WorldMagicPotDate &date);
struct WorldMagicPotComment {
    std::int32_t group{}, bound{}; // 原评语组分别4、3、2条，由Owner抽票一次。
};
struct WorldMagicPotCommentResult {
    WorldMagicPotError error{WorldMagicPotError::none};
    std::optional<WorldMagicPotComment> candidate;
};
WorldMagicPotCommentResult world_magic_pot_comment(const WorldMagicPotItem &item);
struct WorldMagicPotDeposit {
    WorldMagicPotState state;
    std::int32_t inventory{}, comment_group{}, comment_ticket{};
    // 原投入只覆盖aM前4列，第5经验列由Owner保持原值。
    std::array<std::array<std::int32_t, 4>, 3> display{};
};
struct WorldMagicPotDepositResult {
    WorldMagicPotError error{WorldMagicPotError::none};
    WorldMagicPotDenial denial{WorldMagicPotDenial::none};
    std::optional<WorldMagicPotDeposit> candidate;
};
// 对应42确认：库存、元素、待件数和评语一起返回；本函数不消费随机。
WorldMagicPotDepositResult prepare_world_magic_pot_deposit(
    const WorldMagicPotState &state, const WorldMagicPotItem &item, std::int32_t inventory,
    const WorldMagicPotDate &date, std::int32_t comment_ticket);
struct WorldMagicPotProcessing {
    WorldMagicPotState state;
    bool changed{};
    std::int32_t processed{}, percentage{};
    std::array<std::int32_t, 4> produced{};
    std::array<std::array<std::int32_t, 5>, 3> display{};
};
struct WorldMagicPotProcessingResult {
    WorldMagicPotError error{WorldMagicPotError::none};
    std::optional<WorldMagicPotProcessing> candidate;
};
// 对应菜单入口m；无有效输出成功返回changed=false，不刷新时间戳或创建页面。
WorldMagicPotProcessingResult prepare_world_magic_pot_processing(
    const WorldMagicPotState &state, const WorldMagicPotDate &date);
struct WorldMagicPotRecipeListResult {
    WorldMagicPotError error{WorldMagicPotError::none};
    std::optional<std::vector<std::int32_t>> identities;
};
// 原序目录只列flags bit2；发现候选另核p!=1及经验门槛，不提前修改p/r。
// 动态进度必须与静态定义一一对应，不要求进度容器顺序相同；输出保持定义原序。
WorldMagicPotRecipeListResult catalogue_world_magic_pot_recipes(
    const std::vector<WorldMagicPotRecipeDefinition> &definitions);
WorldMagicPotRecipeListResult discoverable_world_magic_pot_recipes(
    const WorldMagicPotState &state,
    const std::vector<WorldMagicPotRecipeDefinition> &definitions,
    const std::vector<WorldMagicPotRecipeProgress> &progress);
struct WorldMagicPotRecipeGate {
    WorldMagicPotError error{WorldMagicPotError::none};
    WorldMagicPotDenial denial{WorldMagicPotDenial::none};
};
// 43只检查发现与已有奖励；reward_status是对应奖励目录p，不是库存数量。
// Owner先验证该配方确属43目录并解析真实奖励引用，本层不创建奖励或选择页面。
WorldMagicPotRecipeGate check_world_magic_pot_recipe_selection(
    const WorldMagicPotRecipeDefinition &recipe, const WorldMagicPotRecipeProgress &progress,
    std::int32_t reward_status);
struct WorldMagicPotRecipeCost {
    WorldMagicPotState state;
    std::int32_t recipe{}, reward_kind{}, reward_definition{};
};
struct WorldMagicPotRecipeCostResult {
    WorldMagicPotError error{WorldMagicPotError::none};
    WorldMagicPotDenial denial{WorldMagicPotDenial::none};
    std::optional<WorldMagicPotRecipeCost> candidate;
};
// 47满计数确认后只核元素并扣四槽；107、授予/93及页面退休由Owner统一提交。
WorldMagicPotRecipeCostResult prepare_world_magic_pot_recipe_cost(
    const WorldMagicPotState &state, const WorldMagicPotRecipeDefinition &recipe);
struct WorldMagicPotDiscoveryResult {
    WorldMagicPotError error{WorldMagicPotError::none};
    std::optional<WorldMagicPotRecipeProgress> candidate;
};
// 46确认消费者；重复p1保持原r，不推导事件106或发奖励。
WorldMagicPotDiscoveryResult prepare_world_magic_pot_discovery(
    const WorldMagicPotRecipeProgress &progress);
} // namespace dungeon_village_reference
