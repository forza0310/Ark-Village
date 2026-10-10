#pragma once

#include "ark/app/session/world_session.hpp"

#include <functional>

namespace ark::test::world_session {
namespace app = ark::app;
namespace sim = ark::simulation;
namespace rules = sim::rules;

// One transport suite shares waiting/diagnostics and fresh source fixtures. No mutable
// WorldSession or WorldState is retained between cases or translated into a second model.
void check(bool condition, const char *message);
app::WorldState initial(bool paused = false);
app::WorldState advance(app::WorldState state);
void same_world(const app::WorldState &a, const app::WorldState &b);
int event_count(const app::WorldState &state, int event);
std::shared_ptr<const app::WorldFrame>
await(app::WorldSession &session, const std::function<bool(const app::WorldFrame &)> &predicate);
const rules::WorldScriptPage &task_top(const app::WorldState &state);
std::shared_ptr<const app::WorldFrame> input_frame(app::WorldSession &session,
                                                   std::uint64_t serial);
const app::WorldCommandResult &input_result(const app::WorldFrame &frame, std::uint64_t serial);

void main_menu_gate_and_pause();
void main_menu_task_transaction();
void main_menu_modal_rejections();
void task_inputs_and_denials();
void recruitment_held_transport();
void task_menu_report_and_departure();
void building_command_transactions();
void facility_and_rank_commands();
void facility_catalog_commands();
void residence_replacement_command();
void human_command_transactions();
void human_gift_parent_transaction();
void tax_command_transactions();
void save_command_transactions();
void system_command_transactions();
void village_command_transactions();
void commerce_command_transactions();
void magic_pot_commands();
void facility_item_command_transactions();
void editing_command_transactions();
} // namespace ark::test::world_session
