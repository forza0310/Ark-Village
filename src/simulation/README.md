# Complete World Runtime

This standard C++17 module imports the maintained complete-world rules and startup/runtime
consumers from research. The source/data snapshot and each mechanical translation are pinned
in `assets/simulation/SOURCES.json`; build/runtime never read research, APK or reverse-engineered
work files. `scripts/import_world_research.mjs` freezes and rechecks the whole dependency closure
before copying it, then changes include paths, namespaces and regression data paths.
Evidence-backed product corrections use `--record-patch product-root relative-file reason`;
test-only source-justified fixture corrections add the `test_fixture_patch` final argument.
the manifest retains the original translated fingerprint and source pin alongside the reason.
Re-import preserves an unchanged recorded patch, and refuses an implicit rebase to new research.

`ark_world_rules` contains the underlying rules; `ark_world_runtime` owns one scene world, cash
ledger, random stream, shared definitions, roster, tasks, scripts and pages. Route/script/finish
projections are temporary transaction candidates. Construct a reset snapshot once, transfer it
to `StartupWorldRuntimeSession`, and discard the reset owner. Never schedule this runtime beside
the old `app::Game` over a second mutable village.

The standard C++ entry is `ark_world_simulation --frames 20000 --months 2 --seed 1 --auto-confirm`.
Frames mean framework calls, not rendered frames or seconds; auto-confirm is an explicit test-user
strategy. Desktop `ark_village --world` selects this owner instead of the old Game. Default old
gameplay remains a separate launch mode until construction and all UI commands are transferred;
the two owners are never scheduled together. This does not claim APK equivalence or implement
normal construction input. Future Game commands/UI views must read/write this same owner.

| Files | Responsibility |
| --- | --- |
| startup/startup_map and generated data | Validated reset evidence and static reconstruction |
| startup_world_projection | One-time bootstrap, all shared definitions and source identities |
| startup_world_routes | Current facts for actor decision/control; no durable second world |
| startup_world_runtime | Unique aggregate, projection writeback and transactional update |
| startup_world_runtime_arrival | Eligibility, creation, roster and first-visit scripts |
| startup_world_runtime_scene/focus | Framework scenes, camera and detached focus actor |
| startup_world_runtime_calendar | Calendar, maintenance, month report and checkpoints |
| startup_world_runtime_pages | Real top-page input/lifecycle; no implicit close-all policy |
| startup_world_runtime_tasks | Creation, encounter/task consumers and exploration restore |
| startup_world_runtime_nonactors | Projectile/object/presentation requests and render facts |

Each important interface retains its source contracts and comments. Explicit page confirmation
in tests is user input, not an automatic game policy; immutable checkpoints are audit artifacts,
not saves. `simulation.source_provenance` checks every imported product file, including the updated
hash of recorded product patches. Newly authored launch/UI files are covered by their normal
source review and integration tests rather than pretending they came from research.
