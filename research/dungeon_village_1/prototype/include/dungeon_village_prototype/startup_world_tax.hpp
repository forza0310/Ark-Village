#pragma once

#include "dungeon_village_prototype/startup_world_runtime.hpp"

namespace dungeon_village_prototype {
enum class StartupWorldTaxAction { previous, next, select, confirm };
struct StartupWorldTaxRow {
    int definition{};
    int amount{}; // a.e.G，原月份3准备；不是当场重新计算的报价。
};
struct StartupWorldTaxView {
    std::vector<StartupWorldTaxRow> rows;
    int total{};
    int selection{};
    int first_visible{};
};

// raw90初始化只冻结bv源序的居民定义引用；金额仍读取唯一Owner中的G。
StartupWorldRuntimeError initialize_startup_world_tax_page(StartupWorldRuntimeState &state,
                                                           std::uint64_t page);
// raw90更新计数；raw98自动入账、通知、清全bv的F/G并关闭，不等待玩家确认。
std::optional<StartupWorldRuntimeState>
update_startup_world_tax_page(const StartupWorldRuntimeState &state, std::uint64_t page);
// raw90上下循环、五行滚动及立即确认；不接受取消，不在此页入账。
StartupWorldRuntimeError act_startup_world_tax_page(StartupWorldRuntimeState &state,
                                                    std::uint64_t page,
                                                    StartupWorldTaxAction action,
                                                    int selection = 0);
std::optional<StartupWorldTaxView>
inspect_startup_world_tax_page(const StartupWorldRuntimeState &state, std::uint64_t page);
} // namespace dungeon_village_prototype
