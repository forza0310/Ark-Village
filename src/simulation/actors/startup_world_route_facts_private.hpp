#pragma once

#include "ark/simulation/actors/startup_world_routes.hpp"

namespace ark::simulation {
// Synchronous input preparation only. Large facts borrow the current Owner;
// every returned input and callback owns its observations before this view dies.
// This is neither a persistent cache nor the publicly returned routes/audit.
struct StartupWorldRouteFactsView {
    using ImprovementLookup = std::function<const std::array<int, 4> *(int)>;
    StartupWorldRouteFactsView(
        const StartupWorldRules *rules, const std::vector<ref::DungeonFinishSurface> &surface,
        const std::vector<ref::Position> &exits,
        const std::map<int, std::array<int, 4>> &human_homes,
        const std::map<std::uint64_t, std::array<int, 3>> &neighbourhood,
        const std::map<ref::CharacterId, StartupWorldActorMetadata> &actor_metadata,
        ImprovementLookup improvements)
        : rules(rules), surface(surface), exits(exits), human_homes(human_homes),
          neighbourhood(neighbourhood), actor_metadata(actor_metadata),
          facility_improvements(std::move(improvements)) {}

    const StartupWorldRules *rules;
    const std::vector<ref::DungeonFinishSurface> &surface;
    const std::vector<ref::Position> &exits;
    const std::map<int, std::array<int, 4>> &human_homes;
    const std::map<std::uint64_t, std::array<int, 3>> &neighbourhood;
    const std::map<ref::CharacterId, StartupWorldActorMetadata> &actor_metadata;
    ImprovementLookup facility_improvements;
    std::vector<ref::CandidateMapEvent> tasks;
    std::array<int, 10> job_counts{};
    std::map<ref::CharacterId, int> facing;
    std::map<ref::CharacterId, bool> actor_visible;
    std::optional<ref::WorldEventEntryInput> task_entry;
    ref::WorldPathTaskAttempt task_attempt;
    std::optional<ref::CollisionBox> actor_box;
    std::optional<ref::CollisionBox> rescue_box;
    std::optional<ref::CollisionBox> object_box;
    std::array<int, 4> calendar{};
    bool primary_expression_table{true};
    ref::MiscSoundProjection sound_projection;
};

std::optional<ref::WorldActorDecisionInput>
prepare_startup_world_decision_input_borrowed(const ref::WorldActorRoutesState &routes,
                                              ref::CharacterId actor,
                                              const StartupWorldRouteFactsView &facts);
std::optional<ref::WorldActorCommandInput> prepare_startup_world_command_input_borrowed(
    const ref::WorldActorRoutesState &routes, ref::CharacterId actor,
    const ref::LegacyActorControl &command, const StartupWorldRouteFactsView &facts);
} // namespace ark::simulation
