#pragma once

// Candidate snapshot construction preserves the observed filters, duplicate events and exchange
// order.

#include "dungeon_village_reference/map_access.hpp"

#include <array>

namespace dungeon_village_reference {

struct TownBounds {
    int left{};
    int right{};
    int top{};
    int bottom{};
};

struct CandidateDefinition {
    std::int32_t definition_id{};
    std::int32_t legacy_category{};
    std::int64_t definition_charm{};
    int legacy_kind{-1}; // o.e；同定义调用点可省略，混合地表必须提供真实种类。
};

struct CandidateInstance {
    BuildingId instance_id;
    std::int32_t definition_id{};
    std::int32_t legacy_phase{};
};

struct CandidateMapEvent {
    Position position;
    std::int32_t legacy_type{};
};

struct ActivityCandidateInput {
    std::int32_t legacy_activity{};
    TownBounds town;
    std::vector<std::int32_t> cell_definition_ids;
    std::vector<CandidateDefinition> definitions;
    std::vector<CandidateInstance> instances;
    std::vector<CandidateMapEvent> events;
    std::optional<BuildingId> last_visited_instance;
    int ground_definition{-1}; // 原o.S，不能将任意kind7替代为基础地面。
};

enum class CandidateOrigin { map_scan, event };

struct ActivityCandidateCell {
    Position position;
    CandidateDefinition definition;
    std::optional<CandidateInstance> instance;
    // Event appends may have no route cost; selection must not erase these entries implicitly.
    std::optional<std::int64_t> cost;
    CandidateOrigin origin{CandidateOrigin::map_scan};
    std::size_t source_index{};
    // dk使用definition，dl使用x.e()；铺撤路后可能不同，不能互相覆盖。
    std::optional<CandidateDefinition> instance_definition{};
    int legacy_state{-1};
    RouteCategory route_category{RouteCategory::ground};
};

struct ActivityCandidateSnapshot {
    std::vector<ActivityCandidateCell> cells;
    std::array<std::int64_t, 11> category_counts{};
    int ground_definition{-1};
};

// 同定义手工快照可复用格定义；实际收集时显式发布两份定义。
inline const CandidateDefinition &candidate_instance_definition(const ActivityCandidateCell &cell) {
    return cell.instance_definition ? *cell.instance_definition : cell.definition;
}

enum class ActivityCandidateError {
    none,
    invalid_field,
    invalid_input,
    binding_mismatch,
    candidate_limit,
    invalid_snapshot,
    no_candidate,
    invalid_ticket
};

struct ActivityCandidateResult {
    ActivityCandidateError error{ActivityCandidateError::none};
    std::optional<ActivityCandidateSnapshot> snapshot;
};

struct CountedCategoryTarget {
    std::size_t index{};
    ActivityCandidateCell cell;
};

struct CountedCategoryResult {
    ActivityCandidateError error{ActivityCandidateError::none};
    std::optional<CountedCategoryTarget> target;
};

bool inside_town(Position position, TownBounds bounds);
bool valid_activity_candidate_snapshot(const ActivityCandidateSnapshot &snapshot);
// Keep event duplicates and absent route costs; candidate membership is not successful routing.
ActivityCandidateResult collect_activity_candidates(const LegacyDistanceField &field,
                                                    const ActivityCandidateInput &input);
// Draw using the filtered count, then scan the complete category-four view in legacy order.
CountedCategoryResult select_counted_category_four(const ActivityCandidateSnapshot &snapshot,
                                                   std::int64_t ticket);

} // namespace dungeon_village_reference
