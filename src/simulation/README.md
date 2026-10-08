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

当前冻结研究`88eb658`中的维护闭包，共363项源/测试/数据；`startup_world_facility_catalog`维护商品79、信息72、口碑82的真实页载荷与动作，最终人气请求交共同世界后续消费。`startup_world_magic_pot`维护41–47、40条配方与村办5/6链；`startup_world_visuals`只读映射购买后举物及建筑队首升降，不推进计数或装配。维护codec保存完整Owner；玩家两栏schema3仅新增配方耐久进度，清理页面/评语等瞬态，静态设施icon加载时纯重建。回放runner及既有产品适配保留。此前村办0–3、raw95、普通道具、商会、地图编辑、人物经营与住宅税收均沿既有唯一Owner；转职55/197、赠礼子页答案/父页恢复、最大HP不回血和库存双向同步保持源契约。

月报继续使用源自动70/70阶段、关闭后点数一次消费；桌面已撤销旧手动冻结政策。日历、人物、施工和随机是否更新依原场景/框架资格，显式暂停仍保留；绘制不推进报告或日期。日期周内比例仅来自已证`units/10800`，原版视觉几何不属于规则层。

`ark_world_rules` contains the underlying rules; `ark_world_runtime` owns one scene world, cash
ledger, random stream, shared definitions, roster, tasks, scripts and pages. Route/script/finish
projections are temporary transaction candidates. Construct a reset snapshot once, transfer it
to `StartupWorldRuntimeSession`, and discard the reset owner. Never schedule this runtime beside
the old `app::Game` over a second mutable village.

The standard C++ entry is `ark_world_simulation --frames 20000 --months 3 --seed 1 --auto-confirm`.
Frames mean framework calls, not rendered frames or seconds; auto-confirm is an explicit test-user
strategy. Desktop `ark_village` selects this owner by default; `--world` is an explicit alias.
The old construction slice is available only through `--legacy-slice` or named legacy diagnostics;
the two owners are never scheduled together. This does not claim APK equivalence or implement
all original UI workflows. Product construction, human and tax commands read/write this same owner.

| Files | Responsibility |
| --- | --- |
| startup/startup_map and generated data | Validated reset evidence and static reconstruction |
| startup_world_projection | One-time bootstrap, all shared definitions and source identities |
| startup_world_routes | Current facts for actor decision/control; no durable second world |
| startup_world_runtime | Unique aggregate, projection writeback and transactional update |
| startup_world_codec/persistence/file_io/restore_validation | Full maintenance capture, bounded wire codec, private restore and atomic file replacement |
| startup_world_runtime_arrival | Eligibility, creation, roster and first-visit scripts |
| startup_world_runtime_scene/focus | Framework scenes, camera and detached focus actor |
| startup_world_runtime_calendar | Calendar, maintenance, month report and checkpoints |
| startup_world_runtime_pages | Real top-page input/lifecycle; no implicit close-all policy |
| startup_world_runtime_tasks | Creation, encounter/task consumers and exploration restore |
| startup_world_runtime_task_pages/deadline | Task offer, recruitment, extra members, departure and renewal/abort transactions |
| startup_world_human/tax | Current human details, profession/gift page transactions and automatic residential tax settlement |
| startup_world_village_activity | Activity 51–54 type0/1/2/3/5/6, source counters, points, human effects, expansion and pot unlocks |
| startup_world_expansion | Fixed-map town bounds, ordered retirement/replacement, entrance rebuilding and source actor cleanup |
| startup_world_facility_items/commerce | Item consumption/improvement phases, gold trades, point payment and later facility receipt |
| startup_world_facility_catalog | Facility merchandise 79, equipment information 72 and publicity 82 lifecycle |
| startup_world_magic_pot | Recipe progress, item input, calendar processing, discovery and development pages 41–47 |
| startup_world_visuals | Read-only building fragments/preview admission, equipment lift and facility notice draw plans |
| startup_world_presentation | Explicit transactional draw requests, page admission, shared jitter/cleanup/sound outputs; no render-loop scheduling |
| startup_world_building/editing | One map, stable placement/retirement, road axis/pricing and source move/remove transactions |
| startup_world_runtime_nonactors | Projectile/object/presentation requests and render facts |

Each important interface retains its source contracts and comments. Explicit page confirmation
in tests is user input, not an automatic game policy; immutable checkpoints are audit artifacts,
not intra-round resume points. Maintenance `startup_world_persistence.hpp` captures the full
Owner, random and ledger; replay also captures ordered audit history and an external controller
at a completed outer-round boundary. Restore validates both private candidates before joint
installation. The player two-slot ARKSAVE1 flow remains separate. Maintenance codec, file I/O and generated
identity belong to `ark_world_persistence`, which depends on the one `ark_world_runtime` and
portable `ark_world_hash`; normal world updates and the player executable do not depend on
these maintenance modules. The digest module has no raylib/image dependency. `simulation.source_provenance` checks every imported product file, including the updated
hash of recorded product patches. Newly authored launch/UI files are covered by their normal
source review and integration tests rather than pretending they came from research.

The continuous regression now runs three months with seed 1 and speed 0, crossing the naturally
reached raw49 rank-conditions page beyond the former two-month boundary. Its input only sets u8
and closes; it does not upgrade rank or charge cash. Calendar projection refreshes the four
source caches once at page initialization; display adapters must only read those caches.
Three months do not certify annual transitions, arbitrary seeds, or natural task success.


以下为33ee056的298项历史接收记录，后续e8已补raw88授予、建筑/设施和活动任务管理，当前冻结以上文为准：全局create_encounter保留created/denial，局部任务目录含2700+住宅续体；raw16/57/89与raw87年度页按专门消费者处理。
产品唯一勋章Owner为`StartupWorldRuntimeState.medal_count`；脚本读写临时投影共用该字段。早期遗漏的产品桥已被e8同义研究实现覆盖，不再保留重复补丁；独立world_medals回归仍验证真实53/54/108脚本与年度+1、终止、溢出回滚。

任务页22–28/33及相关59/83/97/99/100消费者沿冻结来源执行，费用、参与者、路线、自然成功和成果退栈均提交同一Owner。24的按住加速是源逻辑输入，桌面只发送边沿；28/33在动画中允许源confirm加速，不能由绘制帧自动触发。产品`world_session`负责物理按住的页身份和过期输入拒绝。完整主菜单、活动任务管理、raw74和普通建设不包含在这次交付。

自然任务回归保留研究的全部断言，四套均编译，运行需开启`ARK_LONG_WORLD_TESTS`，标签`natural_tasks;long_world`。阶段矩阵以无窗口Debug验证默认自然轨迹、Release验证两种子与年度轨迹；raylib配置保留页面输入及世界接线检查，不重复同一标准C++长链。
