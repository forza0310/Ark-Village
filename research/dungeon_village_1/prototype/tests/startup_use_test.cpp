// Source-backed composition, conditional on the ordinary branch; NOT default startup AI.
// Verify both actual birth points, all three shop goals and distinct owner/timing boundaries.
#include "dungeon_village_prototype/startup.hpp"
#include "dungeon_village_reference/character_hp.hpp"
#include "dungeon_village_reference/character_motion.hpp"
#include "dungeon_village_reference/facility_arrival.hpp"
#include "dungeon_village_reference/facility_exit.hpp"
#include "dungeon_village_reference/facility_use.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace dungeon_village_prototype;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
void branch(ref::Position birth, int definition_id) {
    StartupSession session;
    const auto &loaded = session.state().loaded_map;
    const auto map = startup_route_map(loaded);
    const auto &definition = session.definition(definition_id);
    const auto &actor = startup_evidence().first_character;
    // UID0 -> nonzero reference identity is only this research test's explicit projection.
    const ref::CharacterId actor_id{static_cast<std::uint64_t>(actor.uid) + 1};
    const auto target =
        std::find_if(loaded.instances.begin(), loaded.instances.end(),
                     [&](const auto &v) { return v.definition_id == definition_id; });
    check(target != loaded.instances.end(), "actual loaded shop instance");
    const ref::ArrivalBinding binding{
        target->anchor, ref::BuildingId{static_cast<std::uint64_t>(target->legacy_id) + 1},
        definition_id};
    const auto field = ref::search_legacy_map(map, birth);
    check(field.field.has_value(), "real birth point distance field");
    const auto route = ref::trace_legacy_path(*field.field, binding.goal);
    check(route.error == ref::MapAccessError::none && !route.steps.empty(), "real shop route");
    ref::WorldPosition position{birth.x * 100.0F + 50.0F, birth.y * 100.0F + 50.0F};
    std::size_t waypoint = 0;
    bool entered = false;
    for (int pair = 0; pair < 1000; ++pair) {
        const auto status = ref::inspect_facility_entry(map, binding, position, true);
        if (status == ref::FacilityEntryStatus::ready) {
            entered = true;
            break;
        }
        check(status == ref::FacilityEntryStatus::not_entered, "route binding before arrival");
        const auto cell = route.steps.at(waypoint);
        const auto &tile =
            loaded.cells.at(static_cast<std::size_t>(cell.y * loaded.width + cell.x));
        const auto goal = ref::character_waypoint(cell, tile.legacy_state,
                                                  session.definition(tile.definition_id).direction);
        check(goal.target.has_value(), "source entrance definition direction");
        const auto motion = ref::advance_character_motion(position, *goal.target, 2U | 8192U);
        check(motion.step.has_value(), "source-coordinate motion");
        position = motion.step->position;
        if (motion.step->waypoint_overlap && waypoint + 1 < route.steps.size())
            ++waypoint;
    }
    check(entered, "both birth routes enter logical goal");
    ref::FacilityEconomyInput economy_input;
    const auto economy = ref::derive_facility_economy(definition.economy, economy_input);
    check(economy.values.has_value(), "published level-one economy endpoints");
    const ref::FacilityArrivalInput arrival_input{
        actor_id,
        binding.instance_id,
        definition_id,
        definition.kind,
        definition.category,
        definition.detail,
        0,
        2U | 8192U,
        -1,
        3,
        static_cast<std::int32_t>(economy.values->instance_attributes[0])};
    const auto arrival = ref::prepare_facility_arrival({}, arrival_input);
    const ref::FacilityUseInput use_input{actor_id,
                                          binding.instance_id,
                                          definition_id,
                                          definition.category,
                                          definition.detail,
                                          0,
                                          definition.use_wait,
                                          arrival_input.legacy_flags};
    const auto use = ref::prepare_facility_use(use_input);
    if (definition_id == 30) {
        check(arrival.error == ref::FacilityArrivalError::unsupported_branch &&
                  !arrival.candidate && use.error == ref::FacilityUseError::unsupported_branch &&
                  !use.candidate,
              "weapon goal stays reachable but cannot silently become ordinary/free use");
        check(session.state().accounting.funds() == 5000,
              "unsupported arrival changes no shared owner");
        return;
    }
    check(arrival.candidate && use.candidate, "source food/inn arrival and use candidates");
    check(definition.use_wait == 60 && arrival.candidate->cash_income == 300,
          "column25 wait, consumption price is not construction quote");
    auto cash =
        session.state().accounting; // Separate staged owner projection, not session mutation.
    check(cash.post_cash({1, 1, ref::CashCategory::facilities, ref::CashDirection::income,
                          arrival.candidate->cash_income}) == ref::AccountingError::none &&
              cash.funds() == 5300 && cash.village_points() == 10,
          "income belongs to arrival, not wait completion or village points");
    check(arrival.candidate->state.current_month_facility_sales == 300 &&
              arrival.candidate->state.legacy_visit_counts[definition_id == 28 ? 2 : 0] == 1,
          "arrival updates correct instance sales and actor category");
    auto state = use.candidate->state;
    ref::CharacterHpState hp{0, actor.hp[0], actor.hp[1], actor.hp[2], false, 0};
    const int duration = definition_id == 28 ? 200 : 60;
    int occupations = 0, recoveries = 0;
    for (int pair = 1; pair <= duration; ++pair) {
        const auto result = ref::advance_facility_use(state);
        check(result.candidate.has_value(), "eligible use pair");
        const auto &next = *result.candidate;
        occupations += next.register_occupation;
        recoveries += next.request_capacity_hp;
        check(next.request_exit == (pair == duration) &&
                  next.request_capacity_hp == (definition_id == 28 && pair == 171),
              "source use exit/recovery phase ordering");
        if (next.request_capacity_hp) {
            const auto change = ref::prepare_hp_change(hp, actor.combat[0], actor.combat[0]);
            check(change.candidate && change.candidate->requested_delta == 22 &&
                      change.candidate->target == 22,
                  "even full initial HP still requests capacity recovery");
            hp = *change.candidate;
        }
        const auto animation = ref::advance_hp_animation(hp);
        check(animation.candidate.has_value(), "HP animation precedes control interpreter");
        hp = *animation.candidate;
        state = next.state;
        check(cash.funds() == 5300 && arrival.candidate->state.current_month_facility_sales == 300,
              "waiting never recharges or increments arrival counts");
    }
    check(occupations == 1 && recoveries == (definition_id == 28 ? 1 : 0),
          "occupation once; inn-only recovery");
    check(state.phase == ref::FacilityUsePhase::exit_ready,
          "position, release and next activity still require owner-side exit");
    // Existing exit helpers are composed here as candidate checks, NOT a complete exit commit.
    const auto completion =
        ref::prepare_facility_use_completion(definition_id, definition.economy.upgrade_uses, {});
    check(completion.candidate && completion.candidate->progress.completed_uses == 1 &&
              completion.candidate->progress.level == 1 &&
              !completion.candidate->progress.upgrade_pending &&
              completion.candidate->upgrade_threshold == (definition_id == 28 ? 50 : 20),
          "definition-shared completion uses source threshold, not instance revenue");
    if (definition_id == 33) {
        check(actor.job_satisfaction_thresholds == std::array<int, 2>{10, 100},
              "actual farmer thresholds are compiled from source columns13/14");
        for (int ticket = 0; ticket < 10; ++ticket) {
            const auto satisfaction = ref::prepare_facility_satisfaction(
                {actor_id, binding.instance_id, definition_id, actor.satisfaction,
                 actor.job_satisfaction_thresholds,
                 static_cast<std::int32_t>(economy.values->instance_attributes[1]), ticket});
            const int expected = ticket == 0 ? 1 : 0;
            check(satisfaction.candidate && satisfaction.candidate->popularity.delta == expected &&
                      satisfaction.candidate->satisfaction == expected &&
                      satisfaction.candidate->adjusted_threshold == 5 + ticket,
                  "farmer/zero-modifier quality5: equality succeeds only for injected ticket0");
        }
    }
    check(session.state().accounting.funds() == 5000 && !session.state().character,
          "conditional composition never enables unproved first-play AI");
}
} // namespace
int main() {
    try {
        for (const auto birth : startup_evidence().spawn_points)
            for (const int definition : {28, 33, 30})
                branch(birth, definition);
        std::cout << "startup use composition checks=" << checks << '\n';
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
