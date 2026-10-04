#include "dungeon_village_reference/world_map_refresh.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
std::size_t cell(Position p) { return static_cast<std::size_t>(p.y * 7 + p.x); }
// 非原新局夹具；目录/围栏/base_variants均显式输入，不补猜测值。
WorldMapRefreshState fixture() {
    WorldMapRefreshState s;
    s.map = {7, 7, std::vector<LegacyMapCell>(49)};
    s.surface.assign(49, {7, 91, 99, 98, 15, 7, 44, true, true});
    s.base_variants.assign(49, 6);
    s.ground_definition = 7;
    s.special_ground_definition = 8;
    s.definitions = {{7, {70, 0, FacilityShape::single, {}}},
                     {8, {80, 10, FacilityShape::single, {}}},
                     {40, {400, 6, FacilityShape::single, {}}},
                     {41, {410, 6, FacilityShape::single, {}}},
                     {42, {-1, 9, FacilityShape::single, {}}},
                     {50, {500, 3, FacilityShape::single, {{2, 10}}}},
                     {51, {510, 2, FacilityShape::single, {{0, 20}, {1, 5}}}},
                     {52, {520, 3, FacilityShape::square, {{2, 20}}}}};
    s.fence_level = 0;
    s.fence_levels = {{{{1, 5}, {5, 1}}}};
    return s;
}
void road(WorldMapRefreshState &s, Position p, int definition = 40, int state = 3) {
    s.surface[cell(p)].definition = definition;
    s.map.cells[cell(p)].legacy_state = state;
    s.map.cells[cell(p)].category = RouteCategory::road;
}
WorldMapRefreshState neighbours_fixture() {
    auto s = fixture();
    s.facilities = {{{1}, 50, FacilityShape::single, FacilityOrientation::first, {2, 3}},
                    {{2}, 51, FacilityShape::single, FacilityOrientation::first, {3, 3}},
                    {{3}, 52, FacilityShape::square, FacilityOrientation::first, {4, 4}}};
    std::vector<BoundFacility> bound;
    for (const auto &f : s.facilities) {
        bound.push_back({f, 1});
        s.neighbours[f.instance_id.value] = {{1, 2, 3}, {9, 9, 9}, {}, true, {{1, 7}}};
        for (const auto &occupied :
             facility_footprint(f.shape, f.orientation, f.anchor, 7, 7).cells)
            s.surface[cell(occupied.position)].definition = f.definition_id;
    }
    s.map = *bind_facility_map(s.map, bound).map;
    road(s, {2, 2});
    road(s, {1, 3});
    return s;
}
void display_fields() {
    for (int state = 0; state <= 12; ++state) {
        auto s = fixture();
        s.map.cells[cell({3, 3})].legacy_state = state;
        s.surface[cell({3, 3})].definition = 42;
        const auto r = prepare_world_map_display(s);
        check(r.candidate.has_value(), "all supported map states accept display rebuild");
        const auto &next = r.candidate->state.surface[cell({3, 3})];
        const bool ground = state == 4 || state == 5 || state == 7;
        check(next.display == (ground        ? 70
                               : state == 11 ? 80
                                             : -1) &&
                  next.variant == (ground || state == 11 ? 0 : 98) && next.fragment == -1 &&
                  next.updates == 91 && next.road_mask == 15 && next.instance_field == 44 &&
                  next.road_quad && next.edge_road_pair && !r.candidate->state.refresh_pending,
              "c changes only l/h/special i, preserves f/m/road mask/patches and no scene f");
    }
}
void masks_and_base_variants() {
    const std::array<int, 16> variants{0, 13, 15, 3, 14, 11, 5, 7, 12, 9, 1, 10, 4, 8, 6, 2};
    const std::array<Position, 4> adjacent{{{3, 4}, {4, 3}, {3, 2}, {2, 3}}};
    for (int mask = 0; mask < 16; ++mask) {
        auto s = fixture();
        road(s, {3, 3});
        for (int direction = 0; direction < 4; ++direction)
            if (mask & (1 << direction))
                road(s, adjacent[direction], 40, 4); // 同定义即可，不查邻居道路状态。
        const auto r = prepare_world_map_roads(s);
        const auto &next = r.candidate->state.surface[cell({3, 3})];
        check(next.road_mask == mask && next.variant == variants[mask] && next.display == 99 &&
                  next.instance_field == 44,
              "all16 source F variants/direction bits; matching definition independent of state");
    }
    auto s = fixture();
    road(s, {0, 0});
    road(s, {1, 0}, 41);
    auto r = prepare_world_map_roads(s);
    check(r.candidate->state.surface[0].road_mask == 12 &&
              r.candidate->state.surface[0].variant == variants[12],
          "out-of-bounds neighbors connect; different road definition does not connect");
    for (int state : {3, 4, 5, 7, 11}) {
        s = fixture();
        s.map.cells[cell({3, 3})].legacy_state = state;
        s.base_variants[cell({3, 3})] = -128;
        r = prepare_world_map_roads(s);
        check(r.candidate->state.surface[cell({3, 3})].variant ==
                  (state == 4 || state == 5 ? -128 : 98),
              "d refreshes base signed-byte variant only old4/5, preserves7/11 and nonroad3");
    }
}
void fences_and_patches() {
    auto s = fixture();
    road(s, {1, 5}); // 四角无守卫，连道路也变围栏；定义仍保留。
    road(s, {2, 5}); // 非四角状态3不改围栏。
    s.map.cells[cell({5, 1})].facility = FacilityTileBinding{{99}, 7, 0};
    s.map.cells[cell({5, 1})].legacy_state = 8;
    s.map.cells[cell({5, 1})].category = RouteCategory::terminal;
    for (const auto p : {Position{2, 3}, Position{3, 3}, Position{3, 2}, Position{2, 2}})
        road(s, p);
    road(s, {2, 0});
    road(s, {3, 0});
    const auto r = prepare_world_map_roads(s);
    check(r.candidate.has_value(), "map d needs no fake facility consumer");
    const auto &n = r.candidate->state;
    for (const auto pair : {std::pair<Position, int>{{1, 5}, 4},
                            {{5, 5}, 2},
                            {{1, 1}, 3},
                            {{5, 1}, 5},
                            {{1, 3}, 0},
                            {{4, 1}, 1}}) {
        const auto at = cell(pair.first);
        check(n.map.cells[at].legacy_state == 5 &&
                  n.map.cells[at].category == RouteCategory::blocked &&
                  n.surface[at].fragment == pair.second && n.surface[at].updates == 0,
              "exact corner and side fragments also change route category/state and zero counter");
    }
    check(n.map.cells[cell({2, 5})].legacy_state == 3 && n.surface[cell({2, 5})].fragment == -1 &&
              n.surface[cell({1, 5})].definition == 40 &&
              n.map.cells[cell({5, 1})].facility->instance_id.value == 99,
          "guarded sides skip roads; unconditional corner preserves definition/x binding");
    check(n.surface[cell({2, 3})].road_quad && !n.surface[cell({3, 3})].road_quad &&
              !n.surface[cell({1, 3})].road_quad && n.surface[cell({2, 0})].edge_road_pair &&
              !n.surface[cell({3, 0})].edge_road_pair,
          "real 2x2 and boundary-pair patch flags, no unrelated grass quad");
    check(n.surface[cell({3, 0})].road_quad && n.surface[cell({6, 3})].road_quad &&
              n.surface[cell({3, 3})].edge_road_pair && n.surface[cell({6, 0})].edge_road_pair,
          "d preserves flags outside original scan subdomains; cannot blanket-clear all patches");
    check(n.refresh_pending &&
              r.candidate->steps ==
                  std::vector<WorldMapRefreshStep>{WorldMapRefreshStep::roads_fences,
                                                   WorldMapRefreshStep::scene_refresh},
          "map d marks main scene f at end only");
}
void exhaustive_patch_domain() {
    // 定义匹配与状态是两条独立轴：只要求左下基格state3/category6。
    for (int definition_mask = 0; definition_mask < 16; ++definition_mask)
        for (int state_mask = 0; state_mask < 16; ++state_mask) {
            auto s = fixture();
            const std::array<Position, 4> positions{{{2, 3}, {3, 3}, {3, 2}, {2, 2}}};
            for (int part = 0; part < 4; ++part)
                road(s, positions[part], (definition_mask & (1 << part)) ? 40 : 41,
                     (state_mask & (1 << part)) ? 3 : 4);
            const auto r = prepare_world_map_roads(s);
            check(r.candidate &&
                      r.candidate->state.surface[cell({2, 3})].road_quad ==
                          ((state_mask & 1) && (definition_mask == 0 || definition_mask == 15)),
                  "quad checks exact shared definition; neighbors may not be state3");
        }
}
void neighbour_cache_and_chain() {
    auto s = neighbours_fixture();
    auto r = prepare_world_map_neighbours(s, true);
    check(r.candidate.has_value(), "full validated layout computes real neighbour caches");
    const auto &n = r.candidate->state.neighbours;
    check(n.at(1).previous == std::array<int, 3>{1, 2, 3} &&
              n.at(1).current == std::array<int, 3>{20, 5, 24} && n.at(1).sources.size() == 2 &&
              n.at(1).sources[0].instance_id.value == 2 &&
              n.at(1).sources[1].instance_id.value == 3 && n.at(1).visited,
          "oldG=s, source order/de-dupe footprint, road charm and final source H kept");
    check(n.at(2).current == std::array<int, 3>{0, 0, 0} && !n.at(2).visited &&
              n.at(3).current == std::array<int, 3>{20, 5, 10} && !n.at(3).visited,
          "category2 receives no modifiers/road charm; last-source H resets all first");
    check(n.at(1).notices == std::vector<std::array<int, 2>>{{1, 7}, {2, 0}, {3, 0}} &&
              n.at(2).notices == std::vector<std::array<int, 2>>{{1, 7}, {4, 0}, {5, 0}, {6, 0}},
          "success queues signs1..6 slot-order; same notice preserves original age and queue");
    r = prepare_world_map_neighbours(s, false);
    check(r.candidate && r.candidate->added_notices.empty() &&
              r.candidate->state.neighbours.at(1).notices == s.neighbours.at(1).notices &&
              r.candidate->state.neighbours.at(1).current == n.at(1).current,
          "false still commits real new modifiers/sources/G/H, suppresses change prompts only");
    r = prepare_world_map_refresh(s, true);
    check(r.candidate && r.candidate->state.refresh_pending &&
              r.candidate->steps ==
                  std::vector<WorldMapRefreshStep>{
                      WorldMapRefreshStep::display, WorldMapRefreshStep::roads_fences,
                      WorldMapRefreshStep::scene_refresh, WorldMapRefreshStep::scene_refresh,
                      WorldMapRefreshStep::neighbours},
          "full restore tail calls c,d,inner f,outer f,neighbours in original sequence");
    check(s.surface[0].fragment == 7 && !s.refresh_pending &&
              s.neighbours.at(1).current == std::array<int, 3>{1, 2, 3},
          "all success preparation remains private");
    // 无有效类别2/3源时，原H不被重置，但s/w仍被清空。
    s = fixture();
    s.facilities = {{{1}, 42, FacilityShape::single, FacilityOrientation::first, {3, 3}}};
    s.surface[cell({3, 3})].definition = 42;
    s.map = *bind_facility_map(s.map, {{s.facilities.front(), 1}}).map;
    s.neighbours[1].visited = true;
    s.neighbours[1].current = {1, 2, 3};
    r = prepare_world_map_neighbours(s, false);
    check(r.candidate && r.candidate->state.neighbours.at(1).visited &&
              r.candidate->state.neighbours.at(1).current == std::array<int, 3>{0, 0, 0},
          "no eligible source preserves H rather than inventing blanket clear");
}
void validation_and_late_rollback() {
    for (int mutation = 0; mutation < 14; ++mutation) {
        auto s = neighbours_fixture();
        switch (mutation) {
        case 0:
            s.surface.pop_back();
            break;
        case 1:
            s.base_variants.pop_back();
            break;
        case 2:
            s.base_variants[0] = 128;
            break;
        case 3:
            s.ground_definition = -1;
            break;
        case 4:
            s.definitions.erase(8);
            break;
        case 5:
            s.fence_level = 99;
            break;
        case 6:
            s.fence_levels[0][0].y = 7;
            break;
        case 7:
            s.neighbours.erase(2);
            break;
        case 8:
            s.map.cells[cell({3, 3})].facility.reset();
            break;
        case 9:
            s.map.cells[0].facility = FacilityTileBinding{{1}, 7, 0};
            break;
        case 10:
            s.definitions.at(51).modifiers = {{9, 1}};
            break;
        case 11:
            s.definitions.at(51).modifiers = {{0, std::numeric_limits<int>::max()}, {0, 1}};
            break;
        case 12:
            s.neighbours.at(1).current[1] = std::numeric_limits<int>::min();
            break;
        case 13:
            s.definitions.at(51).modifiers = {
                {0, std::numeric_limits<int>::max()}, {0, 1}, {0, -1}};
            break;
        }
        check(!prepare_world_map_refresh(s, true).candidate && !s.refresh_pending &&
                  s.surface[0].fragment == 7,
              "missing tables/layout/late modifier overflow rejects complete refresh candidate");
    }
}
} // namespace
int main() {
    try {
        display_fields();
        masks_and_base_variants();
        fences_and_patches();
        exhaustive_patch_domain();
        neighbour_cache_and_chain();
        validation_and_late_rollback();
        std::cout << "world map refresh checks: " << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
