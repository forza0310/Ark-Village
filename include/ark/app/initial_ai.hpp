#pragma once

// Researched ordinary life engine: strict initial-interval owner for diagnostics, or a temporary
// current-village adapter. Normal Game owns durable state and submits the prepared candidate.
#include "ark/app/initial_ai_data.hpp"
#include "ark/app/life_state.hpp"
#include "ark/app/random.hpp"
#include "ark/app/startup_data.hpp"
#include "ark/economy/cash.hpp"
#include "ark/facilities/neighbourhood.hpp"
#include "ark/facilities/service.hpp"
#include "ark/people/departure.hpp"
#include "ark/people/hp.hpp"
namespace ark::app {
class Game;
struct InitialAiTickets {
    int category{}, facility{}, satisfaction{};
    std::optional<int> attribute;
    std::optional<int> weapon;
};
struct InitialAiFacility {
    facilities::Placement placement;
    facilities::FacilityArrivalState sales;
    std::vector<people::ActorId> occupants;
    int phase{1}; // Current construction0/usable1; initial interval always1.
};
struct InitialAiState : LifeActorState {
    std::map<int, facilities::FacilityUseProgress> uses;
    std::map<facilities::InstanceId, InitialAiFacility> facilities;
    economy::CashLedger accounting;
    std::uint64_t next_cash_id{1};
};
class InitialAiSession {
  public:
    // Select either published birth point for a repeatable conditional run, without changing Game.
    InitialAiSession(const Game &startup, world::Cell birth);
    // Transient adapter: take the CURRENT village and actor, prepare one round, then discard.
    static InitialAiSession from_village(const Game &game, const LifeActorState &actor);
    const InitialAiState &state() const;
    // One admitted c/d round: invalid input discards all edits. The live adapter can successfully
    // hand off an unsupported choice with its proven preceding actions and queue retained.
    InitialAiError round(const InitialAiTickets &tickets);
    // Draw the shared Java-semantic stream only at actual rule consumption points. A failed round
    // discards RNG consumption; the explicit seed is not the APK's unobserved default seed.
    InitialAiError round_random(RandomStream &random);

  private:
    friend class Game; // Same-owner schedule consumes split c/d phases before one commit.
    InitialAiSession() = default;
    InitialAiError prepare_round(const InitialAiTickets &, RandomStream *);
    InitialAiError decision(InitialAiState &, const InitialAiTickets &) const;
    InitialAiError execution(InitialAiState &, const InitialAiTickets &, RandomStream *) const;
    InitialAiError depart(InitialAiState &, const InitialAiTickets &, RandomStream *) const;
    InitialAiError live_depart(InitialAiState &, int activity, const InitialAiTickets &,
                               RandomStream *) const;
    InitialAiError live_decision(InitialAiState &, RandomStream *) const;
    // State0/5 L and human idle decisions, separate from path P and d/FIFO execution.
    InitialAiError live_spawn(InitialAiState &, RandomStream *) const;
    InitialAiError live_idle(InitialAiState &, RandomStream *) const;
    InitialAiError live_wander(InitialAiState &, RandomStream *) const;
    InitialAiError execution_prefix(InitialAiState &) const;
    InitialAiError live_tail(InitialAiState &) const;
    InitialAiError cleanup(InitialAiState &) const;
    InitialAiError arrive(InitialAiState &) const;
    InitialAiError exit(InitialAiState &, const InitialAiTickets &, RandomStream *) const;
    world::RouteMap map_;
    std::vector<facilities::InstanceId> instance_order_;
    std::map<facilities::InstanceId, facilities::Neighbourhood> neighbourhoods_;
    std::map<facilities::InstanceId, facilities::EconomyInput> economy_inputs_;
    bool live_{};
    std::uint64_t layout_revision_{};
    InitialAiState state_;
};
} // namespace ark::app
