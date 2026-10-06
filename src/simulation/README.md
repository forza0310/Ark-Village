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

当前冻结8f12654，共315项源/测试/数据；人物经营、住宅税收和配套运行时修正已迁入，产品验收通过（结果见B1）。新增`startup_world_human`处理当前人物四页、转职、四槽装备赠礼和大师奖励，`startup_world_tax`处理90查看/确认与98自动结算。转职55/197、赠礼子页答案/父页恢复、最大HP缓存不回血及住宅替换记录退休均保持源契约，不由表现层补算。

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
| startup_world_runtime_arrival | Eligibility, creation, roster and first-visit scripts |
| startup_world_runtime_scene/focus | Framework scenes, camera and detached focus actor |
| startup_world_runtime_calendar | Calendar, maintenance, month report and checkpoints |
| startup_world_runtime_pages | Real top-page input/lifecycle; no implicit close-all policy |
| startup_world_runtime_tasks | Creation, encounter/task consumers and exploration restore |
| startup_world_runtime_task_pages/deadline | Task offer, recruitment, extra members, departure and renewal/abort transactions |
| startup_world_human/tax | Current human details, profession/gift page transactions and automatic residential tax settlement |
| startup_world_runtime_nonactors | Projectile/object/presentation requests and render facts |

Each important interface retains its source contracts and comments. Explicit page confirmation
in tests is user input, not an automatic game policy; immutable checkpoints are audit artifacts,
not saves. `simulation.source_provenance` checks every imported product file, including the updated
hash of recorded product patches. Newly authored launch/UI files are covered by their normal
source review and integration tests rather than pretending they came from research.

The continuous regression now runs three months with seed 1 and speed 0, crossing the naturally
reached raw49 rank-conditions page beyond the former two-month boundary. Its input only sets u8
and closes; it does not upgrade rank or charge cash. Calendar projection refreshes the four
source caches once at page initialization; display adapters must only read those caches.
Three months do not certify annual transitions, arbitrary seeds, or natural task success.


以下为33ee056的298项历史接收记录，后续e8已补raw88授予、建筑/设施和活动任务管理，当前8f冻结以上文为准：全局create_encounter保留created/denial，局部任务目录含2700+住宅续体；raw16/57/89与raw87年度页按专门消费者处理。
产品唯一勋章Owner为`StartupWorldRuntimeState.medal_count`；脚本读写临时投影共用该字段。早期遗漏的产品桥已被e8同义研究实现覆盖，不再保留重复补丁；独立world_medals回归仍验证真实53/54/108脚本与年度+1、终止、溢出回滚。

任务页22–28/33及相关59/83/97/99/100消费者沿冻结来源执行，费用、参与者、路线、自然成功和成果退栈均提交同一Owner。24的按住加速是源逻辑输入，桌面只发送边沿；28/33在动画中允许源confirm加速，不能由绘制帧自动触发。产品`world_session`负责物理按住的页身份和过期输入拒绝。完整主菜单、活动任务管理、raw74和普通建设不包含在这次交付。

自然任务回归保留研究的全部断言，四套均编译，运行需开启`ARK_LONG_WORLD_TESTS`，标签`natural_tasks;long_world`。阶段矩阵以无窗口Debug验证默认自然轨迹、Release验证两种子与年度轨迹；raylib配置保留页面输入及世界接线检查，不重复同一标准C++长链。
