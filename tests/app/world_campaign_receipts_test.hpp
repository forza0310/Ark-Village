#pragma once

#include "ark/app/session/world_session.hpp"
#include <functional>

namespace ark::test {
// Source qualification/transaction fixtures reuse the real campaign command boundary.
// The consumer operates only on the caller's private candidate; no second Owner is retained.
using CampaignCommandConsumer =
    std::function<app::WorldCommandResult(app::WorldState &, const app::WorldCommand &)>;
void campaign_receipt_contract(const CampaignCommandConsumer &apply);
// Village effect fixtures use the same command consumer and campaign suite.
void campaign_village_effect_contract(const CampaignCommandConsumer &apply);
} // namespace ark::test
