#pragma once

#include "world_active_trade_evidence.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

namespace ark::test {
// Observations of one real date-triggered score. This is test evidence, not a player save
// or a second world; only stable checkpoints may encode it alongside the existing sidecar.
class CampaignClearEvidence {
  public:
    void observe_world(const app::WorldState &before, const app::WorldState &after,
                       const app::WorldSystemState &system);
    void observe_score(const app::WorldState &before, const app::WorldSystemState &old_system,
                       const app::WorldState &after, const app::WorldSystemState &system);
    bool confirm_due(const app::WorldSystemState &system) const;
    bool finished() const { return phase_ >= 2; }
    bool inherited() const { return phase_ == 3; }
    bool continued() const { return continued_month_ >= 0; }
    void verify(const app::WorldState &state, const app::WorldSystemState &system) const;
    void verify_cold_inheritance(const app::WorldState &cold, const app::WorldSystemState &system);
    void certify_continuation(int month, std::int64_t sales, std::uint64_t inputs,
                              std::uint64_t effects);
    void certify_inherited_business();
    void encode(std::ostream &out) const;
    static CampaignClearEvidence decode(std::istream &in);
    std::string diagnose() const;

  private:
    int phase_{}; // 0 real business; 1 score in flight; 2 retired score; 3 inherited new world.
    std::uint64_t page_{}, trigger_step_{}, score_updates_{};
    std::array<std::int64_t, 3> event_base_{}; // Source events4,5,6, captured at trigger.
    std::array<std::vector<std::uint8_t>, 2> inheritance_;
    simulation::StartupClearScoreRows rows_{};
    bool rows_captured_{}, inherited_business_{};
    std::int64_t old_high_{}, sum_{};
    std::string old_village_, village_;
    int old_trophy_{}, result_trophy_{}, continued_month_{-1};
    std::int64_t continued_sales_{};
    std::uint64_t continued_inputs_{}, continued_effects_{};
};

// The shared campaign owns transactions and the only world. These three callbacks expose
// its existing step/save/qualification without introducing a duplicate business driver.
struct CampaignClearDriver {
    app::WorldState &state;
    app::WorldSystemState &system;
    CampaignClearEvidence &evidence;
    const ActiveTradeEvidence &trade;
    std::function<void()> step;
    std::function<void(int)> save;
    std::function<bool()> ready;
    std::function<std::uint64_t()> input_effects;
    std::function<std::uint64_t()> commands;
    std::function<std::string()> diagnose;
};

// Same private-candidate score and last-fallible durable commit used by WorldSession.
void advance_campaign_clear(app::WorldState &state, app::WorldSystemState &system,
                            CampaignClearEvidence &evidence, bool confirm,
                            const std::filesystem::path &directory, std::ostream &log);
void run_campaign_clear_segment(CampaignClearDriver driver, bool continuation);
void run_campaign_inherited_segment(CampaignClearDriver driver);
void write_campaign_clear_evidence(const std::filesystem::path &directory, int slot,
                                   const CampaignClearEvidence &evidence);
CampaignClearEvidence read_campaign_clear_evidence(const std::filesystem::path &directory, int slot,
                                                   const app::WorldState &state,
                                                   const app::WorldSystemState &system,
                                                   bool allow_business_without_evidence = false);
void campaign_clear_contract();
} // namespace ark::test
