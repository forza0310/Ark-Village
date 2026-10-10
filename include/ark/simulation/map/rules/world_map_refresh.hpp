#pragma once

#include "ark/simulation/map/rules/map_access.hpp"
#include "ark/simulation/facilities/rules/neighbourhood.hpp"

#include <array>
#include <map>
#include <utility>

namespace ark::simulation::rules {
// 原c/i地表字段，路径类别/状态/x唯一保存在map，不复制第二份。
struct WorldMapSurface {
    int definition{};       // b，真实目录身份；不是h显示索引。
    int updates{};          // f，围栏e也会归0。
    int display{-1};        // h
    int variant{};          // i
    int road_mask{};        // k
    int fragment{-1};       // l，非设施binding的占地片号。
    int instance_field{-1}; // m，c/d不改它。
    bool road_quad{};       // c，接已有road4block00绘制。
    bool edge_road_pair{};  // d，接已有road4block01绘制。
};
struct WorldMapDefinition {
    int display{-1};                            // o.k，允许隐藏-1。
    int category{};                             // o.e，6是道路。
    FacilityShape shape{FacilityShape::single}; // o.l
    std::vector<NeighbourModifier> modifiers;   // o.x/y，仅已维护0..2槽。
};
struct WorldMapNeighbourCache {
    std::array<int, 3> current{};            // s
    std::array<int, 3> previous{};           // G
    std::vector<NeighbourSource> sources;    // w，源定义和稳定实例身份。
    bool visited{};                          // H，最后一个有效源扫描留下的值。
    std::vector<std::array<int, 2>> notices; // p，[身份,年龄]，c(int)按身份去重。
};
struct WorldMapRefreshState {
    LegacyMap map;
    std::vector<WorldMapSurface> surface;
    std::map<int, WorldMapDefinition> definitions;     // br，key原n。
    int ground_definition{-1};                         // S.n/k，不补默认定义。
    int special_ground_definition{-1};                 // T.n/k
    std::vector<int> base_variants;                    // 原h.i signed byte，逐格[-128,127]。
    int fence_level{-1};                               // UserData.o
    std::vector<std::array<Position, 2>> fence_levels; // h.l：[左下,右上]，包含端点。
    std::vector<FacilityPlacement> facilities;         // 原g顺序，身份唯一。
    std::map<std::uint64_t, WorldMapNeighbourCache> neighbours;
    bool refresh_pending{}; // 主场景aS；重复f调用仍只是置true。
};
enum class WorldMapRefreshStep { display, roads_fences, scene_refresh, neighbours };
enum class WorldMapRefreshError {
    none,
    invalid_input,
    missing_definition,
    stale_facility,
    numeric_overflow,
    neighbourhood_failed
};
struct WorldMapRefreshCandidate {
    WorldMapRefreshState state;
    std::vector<WorldMapRefreshStep> steps;
    std::vector<std::pair<BuildingId, int>> added_notices;
};
struct WorldMapRefreshResult {
    WorldMapRefreshError error{WorldMapRefreshError::none};
    std::optional<WorldMapRefreshCandidate> candidate;
};
// c/h.c：全图重置l、特殊状态h/i；不刷新道路或场景。
WorldMapRefreshResult prepare_world_map_display(const WorldMapRefreshState &state);
// c/h.d：真实F道路掩码、围栏e、两补块扫描，结尾标记场景aS。
WorldMapRefreshResult prepare_world_map_roads(const WorldMapRefreshState &state);
// a/o.a(success)：先G=s再重算s/w/H，success才按设施/属性顺序去重排变化提示。
WorldMapRefreshResult prepare_world_map_neighbours(const WorldMapRefreshState &state, bool success);
// a/o恢复格和移除实例之后的真实剩余链：c→d(含f)→f→邻接。
// 输入必须已经移除该实例/所有占地绑定；本函数不替代逐格恢复或任务结算。
WorldMapRefreshResult prepare_world_map_refresh(const WorldMapRefreshState &state, bool success);
} // namespace ark::simulation::rules
