# World Rules

Pure standard C++17 rules for actors, facilities, map/navigation, combat, objects, accounting,
randomness, events/pages and world scheduling. Imported interfaces live in
`include/ark/simulation/rules`, implementations here, namespace `ark::simulation::rules`.
The explicitly registered dependency closure preserves published algorithms, contracts and
regression assertions. `SOURCES.json` records both source and translated hashes.

The concentrated rules directory is a transition boundary for receiving the complete world
without adapting research algorithms to the old first-actor slice. Each existing named source
keeps its domain responsibility; no raylib, desktop input or mutable singleton is introduced.
Runtime consumers coordinate private candidates and commit one owner. The existing construction
slice remains available through `--legacy-slice` until its commands write this owner directly.
The complete world is now the default; it never constructs the old Game alongside it.

| Source Families | Responsibility |
| --- | --- |
| domain/geometry/navigation/map_access/neighbourhood | Stable identities, occupancy, routes and neighbour facts |
| activity/actor/ai_perception/character/human/weapon | Choice, control, counters, effects, HP and shared growth |
| combat/battle/encounter/ai_rewards/object/rescue | Actual combat, creation, drops, rewards and cross-actor commits |
| facility/dungeon/world_shop/world_residence | Arrival/use/exit, shared facility state, shops and housing |
| world_actor_routes/schedule/tail/daily/control | Composed c/d updates and ordered actor consumers |
| world_schedule/nonactor_schedule/overlap/lifecycle | Live rosters, projectiles/objects/encounters and final pairing |
| accounting/world_calendar/maintenance/tasks/month_report | Immediate money, calendar domains and report lifecycle |
| world_scripts/popularity/gift_page/world_world_entry | Fixed events, continuations, pages and actual reward consumers |
| world_task_creation/dungeon_finish/exploration/map_refresh | Task factories, success, restoration and dynamic map state |
| world_task_commands/display/deadline | Explicit offers/fees, recruitment/departure presentation and renewal/abort candidates |
| world_scene/runtime/random/random_consumers | Framework routing, private owner composition and one lazy stream |

Headers without matching implementation files contain the existing template compositions;
the CMake inventory follows the actual include/implementation dependency closure. Tests retain
the original numeric/ordering/rejection assertions; source-relative data paths are mechanically
adapted to explicit `ARK_WORLD_TEST_DATA` supplied from product assets, independent of cwd.
