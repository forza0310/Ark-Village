# 世界规则与运行时

当前冻结 `1f19c88` 的 430 项维护来源。代码按下面的游戏大模块组织，公开头在 `include/ark/simulation` 下对应目录。每个模块内的 `rules/` 保留纯规则层；其余实现负责唯一 Owner 上的查询/消费者/事务接线。目录归类不改变命名空间、数据协议或原有 DLL 依赖。

| 模块 | 职责 |
| --- | --- |
| [facilities](facilities/README.md) | 建设、道路编辑、邻接经营、设施使用、商品/强化、住宅及魔法壶 |
| [actors](actors/README.md) | 人物生命周期、移动/路线、HP/成长、职业/赠礼及主角操作 |
| [ai](ai/README.md) | 感知、行为选择、队列调度和设施/装备候选 |
| [combat](combat/README.md) | 战斗执行、伤害提交、遭遇生命周期、物体与救援 |
| [tasks](tasks/README.md) | 任务生成/征集/出发、期限、迷宫探索和成果 |
| [map](map/README.md) | 格坐标、占地、通路、地图恢复/刷新和扩张 |
| [village](village/README.md) | 账本、日期/月报、活动、人气、授勋、税收、事件和通关计分 |
| [world](world/README.md) | 聚合状态、初始化、随机、场景/页栈和整轮事务协调 |
| [persistence](persistence/README.md) | 维护世界编解码、恢复校验、文件原子替换 |
| [application](application/README.md) | 维护标题/应用控制器、系统纪录、完整回放和继承 |
| [presentation](presentation/README.md) | 已发布的只读皮肤计划、声音请求与显式表现消费者 |

导入路径由 `scripts/simulation/module_paths.mjs` 显式维护，新增研究文件必须先指定职责归属。`cmake/WorldSimulationSources.cmake` 仍显式列源，不使用 GLOB。维护世界/应用/系统与玩家两栏存档分别保持各自协议；本次路径变化不使玩家档失效。收费缓存修复、类型化音频及相关维护接口已随当前冻结版本接入。

## 来源与历史接入说明

以下保留先前接入批次的来源说明；其中版本号、功能范围与验证结果属于对应历史批次，当前产品边界以 [架构](../../docs/ARCHITECTURE.md) 为准。

This standard C++17 module imports the maintained complete-world rules and startup/runtime
consumers from research. The source/data snapshot and each mechanical translation are pinned
in `assets/simulation/SOURCES.json`; build/runtime never read research, APK or reverse-engineered
work files. `scripts/import_world_research.mjs` freezes and rechecks the whole dependency closure
before copying it, then changes include paths, namespaces and regression data paths.
Evidence-backed product corrections use `--record-patch product-root relative-file reason`;
test-only source-justified fixture corrections add the `test_fixture_patch` final argument.
the manifest retains the original translated fingerprint and source pin alongside the reason.
Re-import preserves an unchanged recorded patch, and refuses an implicit rebase to new research.

当前冻结研究`e73bb31`中的维护闭包，共404项源/测试/数据、7项产品适配；构造唯一Owner时一次安装原序六特殊/三十复发任务目录，生成器按实际reset输出设施共享人气20/30。`startup_world_facility_catalog`维护商品79、信息72、口碑82的真实页载荷与动作，最终人气请求交共同世界后续消费。`startup_world_magic_pot`维护41–47、40条配方与村办5/6链；`startup_world_visuals`只读映射购买后举物、cd13属性头标、设施类别/效果与普通道具图标及建筑队首升降，不推进计数或装配。维护codec保存完整Owner；玩家两栏schema4包含配方耐久进度及定义0主角资料，清理页面/评语等瞬态，静态设施icon加载时纯重建。本批玩家布局不变、数据身份更新，拒绝旧初始化身份档且不迁移。回放runner及既有产品适配保留。此前村办0–3、raw95、普通道具、商会、地图编辑、人物经营与住宅税收均沿既有唯一Owner；转职55/197、赠礼子页答案/父页恢复、最大HP不回血和库存双向同步保持源契约。

`startup_title_presentation`维护标题20槽表现Owner，`startup_title_actor_skin`生成基础人物只读皮肤计划；完整应用动作/标题控制器仍由独立维护应用目标编排，AVRAPP01应用语义4、AVRSAVE1状态语义2，旧语义拒绝。自然应用回放用于维护输入与恢复对照，不能替代产品主动经营及玩家schema4冷载验收；旧首星/二星前缀不能穿过本次初始化语义边界。桌面目前只消费配置草稿的静态人物预览，没有以绘制FPS驱动标题控制器。商店窄投影不代表建设邻接后的到达收费缓存已修复。

`startup_information`直接查询Owner的12×5×2现金桶，返回原序五类别月/年收支及利润，显式复刻32位回卷和金额格式，不改变Owner。raw9/36的页面控制器、输入、模态与退休消费者尚未迁入，桌面信息入口仍禁用；此查询不承担自动月报消费。

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
| startup_application/replay/replay_paths | Complete application maintenance snapshots, explicit input replay and build-contained path validation; separate from the player session |
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
these maintenance modules. The digest module has no raylib/image dependency. Imported paths and product adaptations
remain recorded in the source manifest. Product behavior is covered by source review and
functional tests; standalone delivery-file fingerprint checks have been removed.

The continuous regression now runs three months with seed 1 and speed 0, crossing the naturally
reached raw49 rank-conditions page beyond the former two-month boundary. Its input only sets u8
and closes; it does not upgrade rank or charge cash. Calendar projection refreshes the four
source caches once at page initialization; display adapters must only read those caches.
Three months do not certify annual transitions, arbitrary seeds, or natural task success.


以下为33ee056的298项历史接收记录，后续e8已补raw88授予、建筑/设施和活动任务管理，当前冻结以上文为准：全局create_encounter保留created/denial，局部任务目录含2700+住宅续体；raw16/57/89与raw87年度页按专门消费者处理。
产品唯一勋章Owner为`StartupWorldRuntimeState.medal_count`；脚本读写临时投影共用该字段。早期遗漏的产品桥已被e8同义研究实现覆盖，不再保留重复补丁；独立world_medals回归仍验证真实53/54/108脚本与年度+1、终止、溢出回滚。

任务页22–28/33及相关59/83/97/99/100消费者沿冻结来源执行，费用、参与者、路线、自然成功和成果退栈均提交同一Owner。24的按住加速是源逻辑输入，桌面只发送边沿；28/33在动画中允许源confirm加速，不能由绘制帧自动触发。产品`world_session`负责物理按住的页身份和过期输入拒绝。完整主菜单、活动任务管理、raw74和普通建设不包含在这次交付。

自然任务回归保留研究的全部断言，四套均编译，运行需开启`ARK_LONG_WORLD_TESTS`，标签`natural_tasks;long_world`。阶段矩阵以无窗口Debug验证默认自然轨迹、Release验证两种子与年度轨迹；raylib配置保留页面输入及世界接线检查，不重复同一标准C++长链。

`startup_world_profile`统一解析定义0覆盖及原表回退；`startup_world_inheritance`按原设施/职业顺序安装冷启动继承。`startup_world_clear_score`提供六类binary32计算和计分阶段；`startup_skin`只读图块身份/裁片/人物帧，不推进世界。`ark_world_file_io`承载原子替换/数据身份，`ark_world_system`承载AVRSYS01，完整`ark_startup_application`和AVRSAVE留在维护测试依赖中，玩家复用纯helper并由原WorldSession协调。
