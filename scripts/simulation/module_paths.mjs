// Product module layout used by the maintained importer. Explicit assignments keep a new
// research file from silently falling back into a flat directory; add it after ownership review.
import path from 'node:path';

export const simulationModules = {
  facilities: `facility_arrival facility_departure facility_economy facility_events facility_exit
    facility_items facility_service facility_use neighbourhood world_facilities
    world_facility_update world_shop world_residence world_magic_pot facility_projection
    startup_world_building startup_world_editing startup_world_facility_catalog
    startup_world_facility_items startup_world_commerce startup_world_magic_pot`,
  actors: `actor_effects actor_housekeeping actor_lifecycle character_hp character_motion
    human_growth human_management world_actor_routes world_actor_tail world_arrivals
    world_departure world_detached_actor world_equipment_display world_gift_page
    world_lifecycle startup_world_human startup_world_profile startup_world_routes
    startup_world_runtime_focus`,
  ai: `activity_candidates activity_choice actor_ai actor_control ai_perception ai_rewards
    ai_schedule dungeon_ai encounter_ai object_ai ranked_facility_choice snapshot_facility_choice
    weapon_choice world_actor_schedule world_misc_control world_nonactor_schedule world_perception
    world_schedule world_wander startup_ai`,
  combat: `battle_commit combat_ai combat_commit combat_execution encounter_creation
    encounter_lifecycle object_commit rescue_commit world_encounters`,
  tasks: `world_dungeon world_dungeon_finish world_exploration world_task_commands
    world_task_creation world_task_deadline world_task_display startup_world_runtime_tasks
    startup_world_runtime_task_pages startup_world_runtime_deadline`,
  map: `geometry map_access navigation world_map_refresh world_overlap startup_map
    startup_world_expansion startup_world_projection`,
  village: `accounting world_award_page world_calendar world_calendar_maintenance
    world_calendar_tasks world_daily world_month_report world_notices world_popularity
    world_scripts world_village_activity startup_information startup_world_tax
    startup_world_village_activity startup_world_clear_score startup_world_runtime_calendar`,
  world: `domain world_control world_random world_random_consumers world_runtime world_scene
    world_world_entry loop_pacing startup startup_world_runtime startup_world_runtime_arrival
    startup_world_runtime_nonactors startup_world_runtime_pages startup_world_runtime_scene`,
  persistence: `startup_persistence_bytes startup_world_codec startup_world_codec_fields
    startup_world_file_io startup_world_persistence startup_world_restore_validation`,
  application: `startup_application startup_application_actions startup_application_menu
    startup_application_replay startup_application_replay_fields startup_application_replay_paths
    startup_application_storage startup_application_storage_paths startup_application_storage_replay
    startup_application_title startup_system_records startup_title_menu startup_world_inheritance`,
  presentation: `startup_audio startup_skin startup_title_actor_skin startup_title_presentation
    startup_world_presentation startup_world_visuals steam_facility_skin steam_startup_skin`,
};

const owners = new Map();
for (const [module, names] of Object.entries(simulationModules))
  for (const name of names.trim().split(/\s+/)) {
    if (owners.has(name)) throw Error('Duplicate simulation module owner: ' + name);
    owners.set(name, module);
  }

// Short product maintenance notes accompany the unchanged imported algorithms. Keeping
// anchors explicit makes a changed research signature require review instead of losing notes.
const functionNotes = {
  'startup_world_runtime.cpp': [
    ['StartupWorldRuntimeResult prepare_startup_world_runtime(const State &s) {',
      '// Prepare the complete next Owner privately. Failure must not publish partial state or random draws.\n'],
  ],
  'startup_world_building.cpp': [
    ['bool refresh_map(State &s, bool initial_neighbours',
      '// Rebuild map and optional neighbourhood economics together; surface-only refresh must not reprice shops.\n'],
  ],
  'startup_world_runtime_focus.cpp': [
    ['bool install(State &s) {', '// Temporarily bind the detached focus actor to the existing actor consumers using its reserved ID.\n'],
    ['bool extract(State &s) {', '// Return all focus state to its owner and remove temporary roster entries before publishing.\n'],
  ],
  'startup_world_restore_validation.cpp': [
    ['bool validate_restored_state(const State &s, std::string &reason, bool audit_checkpoint) {',
      '// Validate the private decoded candidate before installation; this function never repairs an invalid save.\n'],
  ],
  'ai_schedule.cpp': [
    ['AiScheduleResult prepare_ai_schedule(const AiScheduleInput &i, const AiScheduleHandler &handler) {',
      '// Preserve roster visitation and insertion order: changing traversal also changes downstream random consumption.\n'],
  ],
};

export function annotateModuleSource(text, target) {
  for (const [anchor, note] of functionNotes[path.posix.basename(target)] ?? []) {
    if (!text.includes(anchor)) throw Error('Review module comment after research signature change: ' + target);
    if (!text.includes(note + anchor)) text = text.replace(anchor, note + anchor);
  }
  return text;
}

// Namespaces and wire-format field names remain unchanged; only authored file paths move.
export function simulationProductPath(file) {
  const match = /^(src\/simulation\/|include\/ark\/simulation\/|ark\/simulation\/)(rules\/)?([^/]+)$/.exec(file);
  if (!match || match[3] === 'README.md') return file;
  const stem = path.posix.parse(match[3]).name;
  const module = owners.get(stem);
  if (!module) throw Error('Assign a product simulation module before importing: ' + file);
  return match[1] + module + '/' + (match[2] ?? '') + match[3];
}

// Also used for generated C++ includes and imported test references. Data manifests keep
// their original logical names; they are not rewritten by this source-path adaptation.
export function translateModulePaths(text, target = '') {
  text = text.replace(/\b(?:include\/ark\/simulation\/|src\/simulation\/|ark\/simulation\/)(?:rules\/)?[A-Za-z_][\w]*\.(?:hpp|cpp|inc|json)\b/g,
    file => simulationProductPath(file));
  if (target.startsWith('src/simulation/') || target.startsWith('tests/simulation/'))
    text = text.replace(/(#include ")(startup_(?:persistence_bytes|world_codec|world_file_io|world_restore_validation|application_storage_replay|application_storage_paths|application_replay_paths)\.hpp)(")/g,
      (_, begin, name, end) => begin + path.posix.relative(path.posix.dirname(target),
        simulationProductPath('src/simulation/' + name)) + end);
  text = text
    .replaceAll("path.join(root,'src/simulation',file)", "path.join(root,'src/simulation/persistence',file)")
    .replaceAll("path.join(root,'src/simulation',name)", "path.join(root,'src/simulation/application',name)");
  return annotateModuleSource(text, target);
}
