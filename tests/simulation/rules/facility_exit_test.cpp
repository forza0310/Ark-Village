#include "ark/simulation/facilities/rules/facility_exit.hpp"

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

FacilityUseCandidate complete(int id, LevelEndpoints endpoints, const FacilityUseProgress &state) {
    const auto result = prepare_facility_use_completion(id, endpoints, state);
    check(result.error == FacilityExitError::none && result.candidate.has_value(),
          "valid shared completion produces full candidate");
    return *result.candidate;
}

FacilitySatisfactionCandidate satisfy(const FacilitySatisfactionInput &input) {
    const auto result = prepare_facility_satisfaction(input);
    check(result.error == FacilityExitError::none && result.candidate.has_value(),
          "valid satisfaction produces full candidate");
    return *result.candidate;
}

void shared_completion_boundaries() {
    const std::array<int, 5> thresholds{50, 162, 275, 387, 500};
    for (int level = 1; level <= 5; ++level) {
        const auto threshold = thresholds[static_cast<std::size_t>(level - 1)];
        for (int offset = -2; offset <= 1; ++offset) {
            for (const bool pending : {false, true}) {
                const FacilityUseProgress input{level, threshold + offset, pending};
                const auto result = complete(28, {50, 500}, input);
                check(result.definition_id == 28 && result.progress.level == level &&
                          result.progress.completed_uses == threshold + offset + 1 &&
                          result.upgrade_threshold == threshold &&
                          result.progress.upgrade_pending ==
                              (pending || (level < 5 && offset >= -1)),
                      "post-increment threshold sets only pending, never level or counter reset");
                check(input.completed_uses == threshold + offset &&
                          input.upgrade_pending == pending,
                      "completion leaves input unchanged");
            }
        }
    }
    for (int level = 1; level <= 5; ++level) {
        for (const bool pending : {false, true}) {
            const auto result = complete(0, {10, 0}, {level, 1000, pending});
            check(result.progress.completed_uses == 1001 &&
                      result.progress.upgrade_pending == pending,
                  "zero fifth endpoint disables new signal without clearing existing pending");
        }
    }
    auto shared = complete(29, {20, 200}, {1, 18, false}).progress;
    check(!shared.upgrade_pending && shared.completed_uses == 19,
          "first instance use contributes to shared definition");
    shared = complete(29, {20, 200}, shared).progress;
    check(shared.upgrade_pending && shared.completed_uses == 20 && shared.level == 1,
          "another instance use reaches same shared definition threshold");
    const auto disabled = complete(30, {0, 0}, {1, 0, false});
    check(disabled.progress.completed_uses == 1 && !disabled.progress.upgrade_pending,
          "weapon shop zero upgrade endpoints still count completion");
    const auto zero_start = complete(33, {0, 200}, {1, 0, false});
    check(zero_start.progress.upgrade_pending,
          "zero current threshold with positive fifth is ready");
    const auto maximum = std::numeric_limits<std::int32_t>::max();
    const auto last = complete(36, {30, 300}, {5, maximum - 1, true});
    check(last.progress.completed_uses == maximum && last.progress.upgrade_pending,
          "maximum exact count at full level retains pending");
    for (int mutation = 0; mutation < 7; ++mutation) {
        int id = 36;
        LevelEndpoints endpoints{30, 300};
        FacilityUseProgress state;
        switch (mutation) {
        case 0:
            id = -1;
            break;
        case 1:
            state.level = 0;
            break;
        case 2:
            state.level = 6;
            break;
        case 3:
            state.completed_uses = -1;
            break;
        case 4:
            endpoints.first = -1;
            break;
        case 5:
            endpoints.fifth = -1;
            break;
        case 6:
            state.completed_uses = maximum;
            break;
        }
        const auto result = prepare_facility_use_completion(id, endpoints, state);
        check(result.error == (mutation == 6 ? FacilityExitError::numeric_overflow
                                             : FacilityExitError::invalid_input) &&
                  !result.candidate,
              "invalid shared completion has no partial output");
    }
}

FacilitySatisfactionInput ordinary() { return {{1}, {2}, 36, 40, {10, 60}, 30, 5}; }

void satisfaction_boundaries() {
    auto input = ordinary();
    for (int ticket = 0; ticket < 10; ++ticket) {
        input.ticket = ticket;
        const auto result = satisfy(input);
        const auto gain = ticket <= 5 ? 1 : 0;
        check(result.adjusted_threshold == 30 + ticket - 5 && result.satisfaction == 40 + gain &&
                  result.satisfaction_delta == gain && result.popularity.delta == gain &&
                  !result.popularity.show_notice && result.popularity.legacy_reason == 0 &&
                  result.popularity.legacy_countdown == 10,
              "threshold equality succeeds, jitter is minus five through four, zero request kept");
        check(result.character_id == input.character_id &&
                  result.instance_id == input.instance_id &&
                  result.definition_id == input.definition_id && input.satisfaction == 40,
              "satisfaction candidate preserves identity and leaves input unchanged");
    }
    input.satisfaction = 100;
    input.resolved_instance_quality = 100;
    const auto capped = satisfy(input);
    check(capped.satisfaction == 100 && capped.satisfaction_delta == 0 &&
              capped.popularity.delta == 1,
          "successful full satisfaction still requests one popularity");
    input.satisfaction = 0;
    input.ticket = 0;
    input.legacy_job_thresholds = {-10, -20};
    input.resolved_instance_quality = -15;
    check(satisfy(input).satisfaction == 1, "negative quality is not silently floored to zero");
    input.satisfaction = 33;
    input.ticket = 5;
    input.legacy_job_thresholds = {10, 5};
    input.resolved_instance_quality = 9;
    check(satisfy(input).adjusted_threshold == 9 && satisfy(input).satisfaction_delta == 1,
          "descending thresholds truncate negative division toward zero");
    const auto maximum = std::numeric_limits<std::int32_t>::max();
    const auto minimum = std::numeric_limits<std::int32_t>::min();
    input.legacy_job_thresholds = {minimum, maximum};
    input.satisfaction = 100;
    input.ticket = 9;
    input.resolved_instance_quality = maximum;
    const auto high = satisfy(input);
    check(high.adjusted_threshold == static_cast<std::int64_t>(maximum) + 4 &&
              high.popularity.delta == 0,
          "wide interpolation and jitter do not wrap above int32 maximum");
    input.satisfaction = 0;
    input.ticket = 0;
    input.resolved_instance_quality = minimum;
    const auto low = satisfy(input);
    check(low.adjusted_threshold == static_cast<std::int64_t>(minimum) - 5 &&
              low.popularity.delta == 1,
          "wide jitter does not wrap below int32 minimum");
    for (int mutation = 0; mutation < 7; ++mutation) {
        auto bad = ordinary();
        switch (mutation) {
        case 0:
            bad.character_id = {0};
            break;
        case 1:
            bad.instance_id = {0};
            break;
        case 2:
            bad.definition_id = -1;
            break;
        case 3:
            bad.satisfaction = -1;
            break;
        case 4:
            bad.satisfaction = 101;
            break;
        case 5:
            bad.ticket = -1;
            break;
        case 6:
            bad.ticket = 10;
            break;
        }
        const auto result = prepare_facility_satisfaction(bad);
        check(result.error == FacilityExitError::invalid_input && !result.candidate,
              "invalid satisfaction has no partial request");
    }
}

void independent_scalar_oracles() {
    std::mt19937 random(0xE817A11U);
    for (int trial = 0; trial < 2000; ++trial) {
        const auto level = static_cast<int>(random() % 5U) + 1;
        const LevelEndpoints endpoints{static_cast<std::int32_t>(random() % 1000U),
                                       static_cast<std::int32_t>(random() % 1000U)};
        const FacilityUseProgress state{level, static_cast<std::int32_t>(random() % 2000U),
                                        random() % 2U != 0};
        const auto threshold =
            endpoints.first + (level - 1) * (endpoints.fifth - endpoints.first) / 4;
        const auto result = complete(trial, endpoints, state);
        const auto expected_pending =
            state.upgrade_pending ||
            (endpoints.fifth > 0 && level < 5 && state.completed_uses + 1 >= threshold);
        check(result.progress.level == level &&
                  result.progress.completed_uses == state.completed_uses + 1 &&
                  result.upgrade_threshold == threshold &&
                  result.progress.upgrade_pending == expected_pending,
              "random shared progress matches independent scalar threshold oracle");

        auto input = ordinary();
        input.satisfaction = static_cast<std::int32_t>(random() % 101U);
        input.legacy_job_thresholds = {static_cast<std::int32_t>(random() % 1000U) - 500,
                                       static_cast<std::int32_t>(random() % 1000U) - 500};
        const auto difference = input.legacy_job_thresholds[1] - input.legacy_job_thresholds[0];
        const auto product = input.satisfaction * difference;
        const auto quotient = product >= 0 ? product / 100 : -((-product) / 100);
        for (int ticket = 0; ticket < 10; ++ticket) {
            input.ticket = ticket;
            const auto expected_threshold = input.legacy_job_thresholds[0] + quotient + ticket - 5;
            for (int offset = -1; offset <= 1; ++offset) {
                input.resolved_instance_quality = expected_threshold + offset;
                const auto candidate = satisfy(input);
                const auto gain = offset < 0 ? 0 : 1;
                const auto expected_satisfaction =
                    input.satisfaction == 100 ? 100 : input.satisfaction + gain;
                check(candidate.adjusted_threshold == expected_threshold &&
                          candidate.satisfaction == expected_satisfaction &&
                          candidate.satisfaction_delta ==
                              expected_satisfaction - input.satisfaction &&
                          candidate.popularity.delta == gain,
                      "random all-ticket threshold-adjacent oracle distinguishes cap and request");
            }
        }
    }
}

void economy_composition() {
    FacilityEconomyDefinition cafe;
    cafe.attributes = {LevelEndpoints{420, 630}, LevelEndpoints{6, 60}, LevelEndpoints{5, 50},
                       LevelEndpoints{192, 384}};
    cafe.upgrade_uses = {30, 300};
    for (int level = 1; level <= 5; ++level) {
        FacilityEconomyInput input;
        input.level = level;
        input.definition_improvements[1] = 3;
        input.instance_modifiers[1] = 7;
        const auto economy = derive_facility_economy(cafe, input);
        check(economy.error == FacilityEconomyError::none && economy.values.has_value(),
              "composition quality derives from maintained economy");
        auto satisfaction_input = ordinary();
        satisfaction_input.resolved_instance_quality =
            static_cast<std::int32_t>(economy.values->instance_attributes[1]);
        satisfaction_input.legacy_job_thresholds = {satisfaction_input.resolved_instance_quality,
                                                    satisfaction_input.resolved_instance_quality};
        const auto satisfaction = satisfy(satisfaction_input);
        check(satisfaction.popularity.delta == 1 && satisfaction.satisfaction_delta == 1,
              "resolved instance quality not charm or definition-only quality used in equality");
        const auto threshold = static_cast<std::int32_t>(economy.values->upgrade_uses);
        const auto progress = complete(36, cafe.upgrade_uses, {level, threshold - 1, false});
        check(progress.progress.upgrade_pending == (level < 5) && progress.progress.level == level,
              "economy upgrade threshold feeds pending only, never automatic upgrade");
    }
}

void exit_positions() {
    std::mt19937 random(0xE817U);
    constexpr std::array<Position, 4> directions{{{0, 1}, {1, 0}, {0, -1}, {-1, 0}}};
    for (auto shape : {FacilityShape::single, FacilityShape::pair, FacilityShape::square}) {
        for (auto orientation : {FacilityOrientation::first, FacilityOrientation::second}) {
            const FacilityPlacement placement{{3}, 28, shape, orientation, {2, 2}};
            const auto footprint = facility_footprint(shape, orientation, {2, 2}, 5, 5).cells;
            for (int trial = 0; trial < 100; ++trial) {
                LegacyMap terrain{5, 5, std::vector<LegacyMapCell>(25)};
                for (auto &cell : terrain.cells) {
                    cell.legacy_state = trial == 0 ? 0 : static_cast<int>(random() % 6U);
                    // Intentionally distinct from state: exits do not use path categories.
                    cell.category = RouteCategory::blocked;
                }
                const auto bound = bind_facility_map(terrain, {{placement, 3}});
                check(bound.map.has_value(), "complete footprint binding");
                const auto &map = *bound.map;
                for (const auto &part : footprint) {
                    const auto cell = part.position;
                    const WorldPosition world{cell.x * 100.0F + 17.0F, cell.y * 100.0F + 88.0F};
                    auto expected = cell;
                    bool moved = false;
                    const auto near = [&](Position p) -> std::optional<Position> {
                        for (const auto d : directions) {
                            const Position n{p.x + d.x, p.y + d.y};
                            if (n.x < 0 || n.y < 0 || n.x >= 5 || n.y >= 5)
                                continue;
                            const auto state =
                                map.cells[static_cast<std::size_t>(n.y * 5 + n.x)].legacy_state;
                            if (state == 3 || state == 4)
                                return n;
                        }
                        return std::nullopt;
                    };
                    if (shape != FacilityShape::single && !near(cell)) {
                        for (const auto &other : footprint) {
                            if (other.position == cell)
                                continue;
                            if (const auto target = near(other.position)) {
                                expected = *target;
                                moved = true;
                                break;
                            }
                        }
                    }
                    const auto result = prepare_facility_exit_position(map, placement, world);
                    check(result.candidate && result.candidate->logical_cell == expected &&
                              result.candidate->instance_id == placement.instance_id &&
                              result.candidate->status ==
                                  (moved ? FacilityExitPositionStatus::relocated
                                         : FacilityExitPositionStatus::retained),
                          "exit follows footprint then four-direction order, not shortest path");
                    check(result.candidate->position.x ==
                                  (moved ? expected.x * 100.0F + 50.0F : world.x) &&
                              result.candidate->position.z ==
                                  (moved ? expected.y * 100.0F + 50.0F : world.z),
                          "retained world position is not snapped; relocated is plain centre");
                }
                auto bad = map;
                bad.cells[static_cast<std::size_t>(footprint[0].position.y * 5 +
                                                   footprint[0].position.x)]
                    .facility->definition_id = 99;
                const WorldPosition world{250, 250};
                const auto reject = prepare_facility_exit_position(bad, placement, world);
                check(reject.error == FacilityExitError::invalid_input && !reject.candidate,
                      "invalid other occupied fragment rejects the whole candidate");
            }
        }
    }
    LegacyMap terrain{3, 3, std::vector<LegacyMapCell>(9)};
    const FacilityPlacement facility{
        {1}, 33, FacilityShape::single, FacilityOrientation::first, {1, 1}};
    const auto bound = *bind_facility_map(terrain, {{facility, 3}}).map;
    for (const WorldPosition world :
         {WorldPosition{0, 0}, WorldPosition{std::numeric_limits<float>::infinity(), 150}})
        check(!prepare_facility_exit_position(bound, facility, world).candidate,
              "outside binding and nonfinite position rejected");
    auto extra = bound;
    extra.cells[0].facility = FacilityTileBinding{{1}, 33, 0};
    check(!prepare_facility_exit_position(extra, facility, {150, 150}).candidate,
          "extra stale instance binding rejected");
    auto truncated = bound;
    truncated.cells.pop_back();
    check(!prepare_facility_exit_position(truncated, facility, {150, 150}).candidate,
          "truncated map rejected");
}

void exit_tails() {
    const auto inn = prepare_ordinary_exit_tail(2, 0, {});
    check(inn.candidate && !inn.candidate->evaluate_satisfaction_now &&
              inn.candidate->requests.size() == 2 &&
              inn.candidate->requests[0].action == ExitDeferredAction::choose_activity &&
              inn.candidate->requests[0].parameter == 0 &&
              inn.candidate->requests[1].action == ExitDeferredAction::expression &&
              inn.candidate->requests[1].parameter == 9,
          "inn expression is queued behind activity zero, not another numerical heal");
    const std::vector<FacilityAttributeEffect> effects{{0, 1}, {5, -2}, {0, 3}};
    for (int ticket = 0; ticket < 3; ++ticket) {
        const auto shop = prepare_ordinary_exit_tail(1, 0, effects, ticket);
        check(shop.candidate && shop.candidate->evaluate_satisfaction_now &&
                  shop.candidate->requests.size() == 3 &&
                  shop.candidate->requests[0].action == ExitDeferredAction::choose_activity &&
                  shop.candidate->requests[1].action == ExitDeferredAction::shop_marker &&
                  shop.candidate->requests[2].action == ExitDeferredAction::attribute &&
                  shop.candidate->requests[2].parameter == effects[ticket].attribute_index &&
                  shop.candidate->requests[2].value == effects[ticket].delta,
              "satisfaction now; duplicate attribute rows weighted; growth remains deferred");
    }
    check(prepare_ordinary_exit_tail(1, 0, {}).candidate->requests.size() == 2,
          "empty effects require no random ticket");
    check(prepare_ordinary_exit_tail(1, 0, effects).error == FacilityExitError::missing_ticket,
          "effect selection cannot invent randomness");
    for (int ticket : {-1, 3}) {
        const auto result = prepare_ordinary_exit_tail(1, 0, effects, ticket);
        check(result.error == FacilityExitError::invalid_ticket && !result.candidate,
              "invalid effect ticket rejects entire plan");
    }
    for (int index : {-1, 6})
        check(!prepare_ordinary_exit_tail(1, 0, {{index, 1}}, 0).candidate,
              "out of range attribute index rejected");
    for (const auto branch : std::array<std::array<int, 2>, 4>{{{1, 1}, {1, 4}, {9, 0}, {3, 0}}})
        check(prepare_ordinary_exit_tail(branch[0], branch[1], {}).error ==
                  FacilityExitError::unsupported_branch,
              "unclosed equipment/other categories not silently replaced by ordinary exit");
}

} // namespace

int main() {
    shared_completion_boundaries();
    satisfaction_boundaries();
    independent_scalar_oracles();
    economy_composition();
    exit_positions();
    exit_tails();
    std::cout << checks << " checks passed\n";
}
