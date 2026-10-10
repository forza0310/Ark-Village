#pragma once

#include "ark/simulation/facilities/rules/world_facility_update.hpp"

namespace ark::simulation::rules {
struct WorldGiftFacilityDefinition {
    int status{};          // br.p，领取建筑旧p0才改2。
    bool pending_notice{}; // br.r，旧p0才置真。
    int unlock_counter{};  // br.q，旧p0才清0。
    int free_builds{};     // br.H，每次领取都+1，上限99。
};
struct WorldGiftPageState {
    WorldFacilityUpdateState facility; // 原共享物品/装备在finish.dungeon.catalog，不建第二份库存。
    std::map<int, WorldGiftFacilityDefinition> facility_unlocks;
    std::vector<std::uint64_t> facility_order; // 原g，o.g按实例g细类扫描，允许重复引用。
    int village_points{};                      // 原n.b(int)点数；临时Owner投影，不与现金合并。
};
struct WorldGiftPageInput {
    std::uint64_t page{};
    int counter{};          // 框架已经推进的f124d，函数只处理本次c分支，不再次++。
    bool confirm_pressed{}; // 原aw.b(1048576)，不是鼠标悬停/菜单自动确认。
};
enum class WorldGiftPageError {
    none,
    invalid_owner,
    unsupported_page,
    overflow,
    script_failed,
    close_failed
};
struct WorldGiftPageCandidate {
    WorldGiftPageState state;
    int counter{};
    bool claimed{};
    bool closed{};
    std::optional<int> sound; // 本次旧counter恰1时原d(5)；外层RAII音频处理，不据声音再发奖励。
    std::vector<WorldScriptTrace> scripts;
    std::vector<WorldScriptPage> pages;
};
struct WorldGiftPageResult {
    WorldGiftPageError error{WorldGiftPageError::none};
    std::optional<WorldGiftPageCandidate> candidate;
};
// 页94r3/5/6/7/8真实确认：counter<40第一次只快进40；>=40才br.c/bt.b/bz.a/bA.a/by.c然后关闭。
// 直接定义领取不复用c/e.a掉落消费者：不加UserData.E、不发额外notice2/10/11/110/151。
// 页95只接固定脚本实际产生的r0/1/3/4/9/10/11；与94共用上述计数/确认时点。
// r0现金与月类别4收入经scripts.finance投影提交，不调用会额外通知34的脚本opcode0。
WorldGiftPageResult prepare_world_gift_page(const WorldGiftPageState &state,
                                            const WorldGiftPageInput &input,
                                            const WorldScriptCatalog &catalog);
} // namespace ark::simulation::rules
