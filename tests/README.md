# 测试组织

现有长跑的实际玩家输入、成功判据与注册范围见[长测审计与建设方案](../docs/stages/ACTIVE_VILLAGE_PLAN.md)。3月/年度持续基线与短回放仍保留原oracle，但不能称为完整经营验收；产品主长测应有建设、活动/培养、任务胜利、逐星条件和新内容消费。被动日期campaign已停止，不计通关/继承通过。

周期扣费允许负金币，经营观察和策略记录保留有符号最低值；主动付款仍按源资格拒绝。现campaign短合同用真实月报验证跨零、建设拒绝及五种策略配置往返，复用账本/月报规则套件的费用、幂等与回滚oracle；负余额不是即时失败，也不能直接认证经营健康。

`ark_world_campaign_tests`的Steam布局路线复用同一生产命令/玩家存档驱动及回执接口：`steam-new`接已验证的Late玩家前缀，完成建设/强化/任务胜利后保存；`steam-resume`接Steam前缀推进四星50目标布局，`steam-verify`冷载最终第二栏。独立目录需具备`ark-steam-village-v1`标记，严格位于产品build内，输入前缀先复制、原件只读。季度稳定主场景另存`checkpoints/month-N`的第一栏及控制器；`steam-inspect`仅冷载第一栏，不推进世界，也不把前缀当四星通过。地图目标、人物公平分配、库存强化分别在`world_steam_layout`、`world_steam_people`和`world_active_income_strategy`；组合决策归`world_steam_strategy`，无新套件或第二Owner。`--contract`以真实命令短夹具核建设、蓝图支付/领取及活动支付/壶开放回执，实际长跑另行执行；冷载不倒填历史，排队不冒充已应用效果。

Steam可选活动保留当前已提供且布局需要的扩张预算；原合同以真实目录Init/介绍消费者验证99点等待、100点选扩张及不需要扩张时释放预算。目录/星级/边界为明确资格夹具，真实支付、围栏变化、新区使用和入住由分段经营另验，不能用策略选择证明效果。诊断保留点数、季度名额、扩张开放/flags/次数，分清预算不足与源资格未开放。

`world_steam_budget`集中实际报价、两月现金预算和分类支出诊断，组合策略不再复写维护算法。任务结束后保留一个完整营业月再征集，下次月份沿既有sidecar冷载；最终征集/续期重新核预算，入住预留新住宅维护，低现金只暂缓可选活动。原经营合同覆盖报价差1G、续期显示/当前名单差异、跨年支出与冷载、必要扩张及免费强化保持；住宅回执仅明确补足短夹具的现金资格，真实经营必须自行赚取。命令日志含raw/tick/实际现金前后，`BUSINESS_EXPENSE`标明命令或更新阶段、五类中实际支出及当时预算，不将月总差额推造成某项费用。

`simulation.replay_runner_contract`是产品回放CLI/路径/前缀资格的主责套件，复用现有continuous可执行程序生成真实短扩张快照，验证已认证及裸候选接续、三路一致性与拒绝；不仿造文件解析器。冻结默认晋级`startup_world_replay_process`保留原参数/断言，二者不替代彼此；未按函数或场景新增EXE。长前缀与自然晚期认证仍按风险独立执行，见[工具说明](../scripts/simulation/README.md#场景与既有前缀接续)。

按被测所有者和依赖找用例；迁入研究测试与产品适配测试分开。标准CTest保留全部适用用例；默认本地desktop-debug排除三个月连续模拟，CI desktop-release完整执行，headless按需追加，选择规则见[构建检查](../docs/CONTRIBUTING.md#构建检查)。标签用于定位，不能以单层通过替代阶段验收。

四套测试EXE共用独立`shared-libraries`预设的Release DLL，库源码只编译一次；Debug/Release描述的是测试与应用消费者的编译配置。库内不保留调试符号，测试的有效断言仍按原合同保留。`build/bin/`只保留一套DLL，EXE用配置后缀区分；先重建公共库再构建消费者。CI打包脚本的递归依赖/缺失拒绝/架构合同在`.github/ci/build_test.py`，由CI构建驱动先执行，不增加游戏CTest或新依赖。

| 目录 | 负责的契约 | 主标签 |
| --- | --- | --- |
| `simulation/rules/` | 冻结研究纯规则与已证拒绝/边界 | `rules;frozen` |
| `simulation/` | 冻结研究共同Owner、页面、路由和自然世界组合 | `runtime;frozen`，连续轨迹为`e2e` |
| `app/` | 产品启动/原时钟、世界会话FIFO、建设/设施/人物/税收事务接线、自动月报、勋章桥、当前设施查询 | `runtime`或`rules` |
| `desktop/` | 显示计划、布局/输入、地图拼块/动画；纯计划也在headless运行 | `presentation` |
| `assets/`、`data/` | 素材解析和桌面字形需求编译契约 | `rules`或`presentation` |
| `integration/` | 真实CLI、打包资源变异和长期世界包装器 | `e2e` |
| `support/` | 断言/进程入口与当前世界夹具 | 不注册业务用例 |

产品注册集中在`cmake/ProductTests.cmake`，冻结研究注册在`cmake/WorldSimulation.cmake`。两者都显式列出源文件，不使用GLOB。旧ark_game及其35项专属测试随切片退役；当前世界的规则、事务、随机、回滚、存档与三个月基线继续保留。纯交付文件指纹检查已移除，DLL防错、存档身份和字段覆盖检查仍负责真实契约。冻结simulation测试按正式研究交付演进，来源与适配记录保留在SOURCES.json；不改有效断言，也不纳入research在途成果。

## 套件与独立进程

经营合同沿统一回执补人物装备、授勋与住宅：三装备命名空间各用独立到访Owner，真实消费免费资格、装配并核四属性；年度和入住夹具走原页及付款事务。资格显式准备，不认证自然取得；装配/属性写回与战斗使用、入住与完工/税收分别记录。绑定/命名空间/上下界拒绝复用C++和Node原合同，Steam子策略全部oracle通过才发布，不增套件或复制底层奖励算法。

任务胜利取证复用同一合同：匹配提交实例、完成数及有序参与名单、首引用移除和新30页；探索摘要是正式绑定，战斗页的可选重复字段不能矛盾。绑定夹具只检验取证边界，真实探索/战斗另由经营接续验收。各策略共用tick观察签名，驱动不按策略类型分派回执；取证拒绝保存更新前Owner，避免与运行时事务失败混淆。

村庄事务观察集中在`app/world_campaign_village_receipts.cpp`：原raw98税收断言原样移动，rank回执绑定原Init四项缓存、raw48确认/退休、历史日期、新庆典50和对应活动开放。当前条件的纯查询仅作诊断，不代替原确认缓存；短夹具保留实际建设/Init/晋级，并覆盖缓存后当前值变化、错误条件/日期/后继及取消拒绝。C++与Node核16列转换/mask，显示星1–5映射源rankUpTerm ID0–4。命令侧也先集中通过全部观察，再发布回执；冷载与条件说明不补历史升星。

村庄事务观察集中在`app/world_campaign_village_receipts.cpp`：原raw98税收断言原样移动，rank回执绑定原Init四项缓存、raw48确认/退休、历史日期、新庆典50和对应活动开放。当前条件的纯查询仅作诊断，不代替原确认缓存；短夹具保留实际建设/Init/晋级，并覆盖缓存后当前值变化、错误条件/日期/后继及取消拒绝。C++与Node核16列转换/mask，显示星1–5映射源rankUpTerm ID0–4。命令侧也先集中通过全部观察，再发布回执；冷载与条件说明不补历史升星。

设施强化取证同样沿经营合同：短资格夹具走原商会500G购买、75库存/实例计数及76首次改良，分别检验品质与价格/魅力。回执第四数值关联设施或道具定义，C++/Node核两端命名空间、边界与不变绑定；77或不变观察不重新发成功记录。自然经营续跑另验实际取得/消费，不能把初始存货、预测收益或一条改良回执冒充完整营业/全部效果。

这些资格/交易夹具集中在`app/world_campaign_receipts_test.cpp`；`world_campaign_test.cpp`负责进程阶段、档案/控制器和真实经营驱动。夹具借用驱动的同一命令消费者，不复制生产提交开关或长期持有Owner；仍属于原`player_active_campaign_contract`，CMake只新增显式源条目，未新增套件、可执行文件或CTest。

任务胜利与直接探索战利品的纯查询按职责放在`app/world_campaign_task_receipts.cpp`。战利品绑定本轮挑战/任务与目录效果，区分物品库存、装备免费资格和本轮页面上下文；覆盖四空间、999封顶、重复不补资格、不变观察不重发及地面竞争。异定义普通地面拾取只计共享E，同定义竞争输出`COVERAGE_LOOT_GAP`，不影响世界推进也不计成功；实际领取/装配与自然战斗使用另验。仍复用原合同与16列，运行不读取研究工作区。

`app/world_campaign_battle_receipts.cpp`只读绑定尸体B≥12/C0退休、定义成长与实际怪物收入新账目，同定义多尸体聚合；预测金额必须匹配全部新账目。月击倒数k.u清零与持续成长k.v分开，跨月回执保留最终零值并使用独立阶段，不倒填不存在的中间计数。原campaign合同复用真实分配/死亡/月界消费者的资格夹具，覆盖99→100、取消无奖、多尸体、错金额/绑定和不变观察；不复写整套命中矩阵，也不认证击杀者收益、事件XP、装备战斗或BOSS后继。未知绑定输出`COVERAGE_BATTLE_GAP`并保留未验证，已有成功oracle不会倒填旧执行。

住宅税收由原驱动统一观察raw98退休与实际现金/其它收入、源序居民G和全定义F/G清零；人物级行保留同一事务合计背景，不按行重复求收入。原合同补资格/零额/符号及错账、漏清、错误通知、随机变化拒绝；职业税率与年度准备计算仍由冻结规则套件主责。raw90确认、冷载已有居民或不变观察不能倒填税收回执，真实年度经营与新增入住另列。

`ark_world_campaign_tests`与`app/world_campaign_process.mjs`负责主动首星：P1建设营业、真实赠礼培养、任务胜利、活动、原四条件晋级、绘画展消费及继续营业；两进程在真实业务节点跨玩家档重启，策略证据用SHA绑定。它执行生产命令事务消费者，线程FIFO另由会话套件负责。原被动日期路线已替换，不再把日期推进称为完整经营。`--resume-prefix`只能复用已成功new阶段及哈希绑定的真实档案，保留原证据，不能继承失败续跑的世界进度。

同一campaign目标接二星与魔法壶分段策略，不复制Owner更新/文件驱动。`--first-star-prefix`从真实首星档继续建设/入住/任务/二星/活动30；`--second-star-prefix`要求通过独立终态复核的二星档，再执行真实投入/自然处理/配方发现→第一栏保存→新进程炼制/使用伤药恢复HP→后续营业→第三进程冷文件复核。策略只持证据，所有款项、元素、库存、成长与日期经生产命令/消费者提交；严格区分条件夹具、真实业务前缀和旧数据身份。新e73bb31语义不复用旧空任务池档。

runner记录实际EXE、冻结来源、公共库输入/产物身份，在阶段前和结束重新核验；前缀必须具备真实成功阶段及匹配的存档/策略/日志SHA。后续运行新建input副本，并复核原件和副本未变；失败或复用阶段不冒充完整原始二星证书。魔法壶阶段A明确禁止提前炼制/用药，只有冷载策略证据并验证实际Owner后才开放阶段B。

`ark_world_economy_tests`与`app/world_economy_process.mjs`执行五组seed1三个月建设对照，最多两个内存世界进程并发，输出实际逐店/分类财务、价格缓存和最低现金，不写玩家档。主责为布局收益分析，不认证首星或全局最优。两个EXE的`--contract`进入默认CTest；耗时路线仅`ARK_LONG_WORLD_TESTS=ON`注册`player_active_first_star`/`player_construction_economy`，默认关闭。费用/随机/三个月冻结oracle不因这些产品策略而改变。

89f157c完整应用AVRAPP01的字段/路径/状态检查归原`startup_application_test`，跨进程短回放复用`startup_world_replay_process`。路径适配只把研究work隔离根映射到产品build，保留外根/别名/链接/覆盖拒绝；玩家EXE不链接维护应用codec。

| 原入口 | 现在的可执行与显式用例 | 保留覆盖 |
| --- | --- | --- |
| `ark_world_report_tests` / `ark_world_medals_tests` | `ark_world_contract_tests world_report` / `world_medals` | 自动月报70/70次序/一次奖励/不重复维护费，诊断skip资格/拒绝；勋章真实脚本与年度共享Owner/溢出回滚 |
| `ark_world_panels_tests` / `ark_world_award_ui_tests` / `ark_world_crew_summary_tests` / `ark_world_tasks_tests` | `ark_world_ui_tests world_panels` / `world_award_ui` / `world_crew_summary` / `world_tasks` | 页面布局/通知映射、授勋与晋级输入门槛、真实有序成果名单、任务ID/费用/管理/阶段资格/输入 |

菜单输入回归使用`ark_world_ui_tests world_menu`，建设/设施/入住/升级与预览映射使用同一套件的`world_building`；窗口冻结、菜单原子切换、任务/授勋/晋级FIFO由`world_session`负责。`world_building_commands`单独覆盖当前Owner建设和设施命令的失败/回滚/实际提交，不在UI中重复底层造价、完整建设规则组合或自然任务长链。

`world_award_ui`覆盖授勋与progression页面的只读映射/输入门槛，`world_tasks`保留任务身份/费用、26/27及4→1边界；raw60完整映射与输入转由`world_human`承担，包含源ready、目录顺序、选择索引、库存报价、63当前职业/等级与目标分离、赠礼/大师/只读页。`world_tax`覆盖五行原序/合计/确认且无取消、自动98无输入。两者复用同一UI runner，不复制底层奖励算法。

`world_panels`覆盖notice前两条布局/标签解码及日期只读映射；周内比例只验已证`units/10800`，不认证原皮肤几何。`world_session`覆盖报告可见时菜单/人物/任务可操作、自动phase推进时真实世界继续、显式暂停不自恢复、与逐步源运行时等价，以及人物父子页恢复事务和声音一次领取。原规则计数、随机及奖励组合仍由冻结研究套件主责，上层只补接线风险。

`ark_world_ui_tests world_human_render_fixture`为显式窗口诊断，复用同一runner而不加入标准CTest。它用标注为synthetic的raw70三阶段输入检查实际皮肤/文字/布局，并验证绘制不改变日期、现金、随机或奖励属性；不运行奖励消费者，不替代自然大师整线或原版动画验收。

`ark_world_ui_tests world_edit_render_fixture`同样仅为手动窗口诊断。它显式提供flag32门控，使用初局真实包子铺、完整mode7预览和实际300G移动事务，保存预览/移动后两图并检查绘制不推进世界；不替代自然首星解锁、OS输入或原APK光标验收。实际Windows命令使用`build/bin/ark_world_ui_tests-desktop-release.exe world_edit_render_fixture`。

`ark_world_ui_tests world_combat_render_fixture`为显式P/cd24显示夹具，仅手动调用、不注册标准CTest；一图对照count0/6/55/72，实际消费普通头标素材两帧与EXP条，不运行成长/奖励或world update。它不认证自然待升级出现/完整bl锚点或原版动画。主责边界、隐藏/Pfalse/寿命/真实cd14独立性与只读契约在已有world_combat_visuals套件。

设施只读查询回归比较查询前后完整邻接缓存（键、current/previous、visited、完整有序sources/notices）及资金/随机/日期。e8已在初局初始化缓存，因此不能继续断言缓存初始为空，也不能退化为只比较数量。

合并的是重复链接和入口，不把不同场景合成串行大流程。正文仍按职责独立文件；CTest使用原名称，每次只选择一个case并启动新进程，错误归属和夹具隔离保留。未指定或指定不存在的case返回2，断言失败返回1；没有隐式“运行全部”。月报/勋章采用相同真实新局夹具与运行时依赖，因此共享可执行；线程会话、长跑、CLI进程检查及冻结来源测试保持独立。

共享`Checks`保留Release断言、单case计数和诊断上下文；六个重构case的原断言表达式、消息和调用顺序逐项保留。其他套件的专有检查暂留在各自文件，只有确认相同语义时才抽取。

## 维护文件与精确回放

`simulation.startup_world_persistence_test`独立于玩家`world_save`，集中同一维护文件协议的codec、完整候选恢复与文件替换支撑源，不将support cpp逐个注册。新协议全Owner/随机/账本和预算/损坏/用途拒绝由该冻结套件负责；`startup_world_replay_driver`复用continuous程序核对全部控制器进度，`startup_world_replay_process`默认420保存、421–840尾段与两个独立载入进程逐帧比较。它们是测试维护策略，不改变玩家随机不落盘的两栏档。

`simulation.startup_world_codec_coverage`在Clang配置中从产品AST反向规范化字段/类型，严格比较源规范JSON/布局真hash及正向机械翻译inc。新增Owner字段必须更新正式交付，不能手改清单漏过覆盖。三个月用例注册、参数和有效断言保留。较长认证前缀只按资格/风险复用尾段；产品双尾段通过不表示重新自然跑过研究前缀，具体证据见[B1](../docs/stages/B1-playable-prototype.md#research-524415a-design)。

## 运行与扩展

`world_save`是字节协议/文件隔离独立职责，合用同一runner中的codec、恢复和文件场景；规则计算仍由既有冻结套件主责。它检查带有效校验的恶意字段、两个栏位与失败替换、真实经营/施工/服务、延迟续体及跨月恢复；会话generation/旧命令/暂停和失败Owner保留归现有`world_session`，槽选择/确认/禁用归现有`world_menu`。夹具只读取真实新局，不复制领域算法，Release断言有效。

```sh
# 本地代码阶段默认
ctest --preset desktop-debug -E '^simulation\.startup_world_continuous_test$' --output-on-failure
# CI完整标准测试；本地仅相关失败定位等需要时执行
ctest --preset desktop-release --output-on-failure
# 核心接口、依赖或CMake改动时追加
ctest --preset headless-debug -E '^simulation\.startup_world_continuous_test$' --output-on-failure
# 定向定位示例，不替代阶段收口
ctest --preset desktop-release -L presentation --output-on-failure
ctest --preset headless-debug -R '^(world_report|world_medals|world_session)$' --output-on-failure
```

e8历史批次四套602项标准CTest和6次自然/年度长测、8f12654批次四套606项标准CTest和4条额外自然长测均已通过，详见B1。历史全测结果不要求后续每批重复本地Debug长测。

四套标准均保留seed1三个月基线注册，但不要求每批全部构建/执行。按2026-10-06策略，本地默认desktop-debug使用上述精确排除条件，main CI的desktop-release完整执行。核心接口/依赖或CMake变更补headless-debug；长期模拟、性能及持续世界回归按风险使用headless-release，`ARK_LONG_WORLD_TESTS=ON`额外注册自然/年度长期回归，CI默认不执行这些额外长测。仅必要诊断或用户明确要求时补跑Debug三个月，不能将CI Release结果记作Debug通过。只改产品表现/接线而冻结规则及既有长跑未变时，不重复已通过的同一核心长链。保留路径、命令、月份/种子、断言参数与注册，改变执行分工不缩减断言。

新增回归先找行为所属套件，明确“既有契约→保留场景”和独立期望值。保留当前世界有效错误拒绝、回滚、随机与调用顺序保障，不因文件同名删除冻结研究测试。退役的旧切片及专属测试不再维护；旧参数拒绝归启动参数套件。详细要求见[开发流程](../docs/CONTRIBUTING.md#测试设计与组织)。
