#pragma once

// Monster89/92/99/100 read the source definition and clocks; this module owns no actor or timer.
#include "../common/layout.hpp"
#include "ark/simulation/presentation/steam_monster_skin.hpp"
#include "ark/simulation/village/rules/world_scripts.hpp"

namespace ark::desktop::ui {
class Skin;
enum class WorldMonsterDetailAction { confirm, cancel };
bool world_monster_intro_page(const simulation::rules::WorldScriptPage &page);
// Enter follows the source fast-forward/close gate. Touch2 has no mouse rectangle.
bool world_monster_intro_confirm(const simulation::StartupWorldRuntimeState &state,
                                 const simulation::rules::WorldScriptPage &page, bool enter,
                                 bool blocked);
std::optional<simulation::SteamMonsterSkinPlan>
prepare_world_monster_intro(const simulation::StartupWorldRuntimeState &state,
                            const simulation::rules::WorldScriptPage &page, const Skin &skin);
void draw_world_monster_intro(const std::optional<simulation::SteamMonsterSkinPlan> &plan,
                              Extent extent, const Skin &skin);
bool world_monster_detail_page(const simulation::rules::WorldScriptPage &page);
// Detail92 has no waiting threshold. Enter and PC Back map to the two source Pop consumers;
// coordinate-free Touch2 never becomes a full-screen mouse confirmation.
std::optional<WorldMonsterDetailAction>
world_monster_detail_input(const simulation::StartupWorldRuntimeState &state,
                           const simulation::rules::WorldScriptPage &page, bool enter, bool back,
                           bool blocked);
std::optional<simulation::SteamMonsterSkinPlan>
prepare_world_monster_detail(const simulation::StartupWorldRuntimeState &state,
                             const simulation::rules::WorldScriptPage &page, const Skin &skin);
void draw_world_monster_detail(const std::optional<simulation::SteamMonsterSkinPlan> &plan,
                               Extent extent, const Skin &skin);
bool world_monster_notice_page(const simulation::rules::WorldScriptPage &page);
// Like89, coordinate-free Touch2 admits Enter through the source consumer, with no PC cancel.
bool world_monster_notice_confirm(const simulation::StartupWorldRuntimeState &state,
                                  const simulation::rules::WorldScriptPage &page, bool enter,
                                  bool blocked);
std::optional<simulation::SteamMonsterSkinPlan>
prepare_world_monster_notice(const simulation::StartupWorldRuntimeState &state,
                             const simulation::rules::WorldScriptPage &page, const Skin &skin);
void draw_world_monster_notice(const std::optional<simulation::SteamMonsterSkinPlan> &plan,
                               Extent extent, const Skin &skin);
} // namespace ark::desktop::ui
