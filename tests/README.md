# 测试组织

按被测所有者和依赖找用例；迁入研究测试与产品适配测试分开。标准CTest保留全部适用用例；默认本地desktop-debug排除三个月连续模拟，CI desktop-release完整执行，headless按需追加，选择规则见[构建检查](../docs/CONTRIBUTING.md#构建检查)。标签用于定位，不能以单层通过替代阶段验收。

四套测试EXE共用独立`shared-libraries`预设的Release DLL，库源码只编译一次；Debug/Release描述的是测试与应用消费者的编译配置。库内不保留调试符号，测试的有效断言仍按原合同保留。`build/bin/`只保留一套DLL，EXE用配置后缀区分；先重建公共库再构建消费者。CI打包脚本的递归依赖/缺失拒绝/架构合同在`.github/ci/build_test.py`，由CI构建驱动先执行，不增加游戏CTest或新依赖。

| 目录 | 负责的契约 | 主标签 |
| --- | --- | --- |
| `simulation/rules/` | 冻结研究纯规则与已证拒绝/边界 | `rules;frozen` |
| `simulation/` | 冻结研究共同Owner、页面、路由和自然世界组合 | `runtime;frozen`，连续轨迹为`e2e` |
| `app/` | 产品启动/原时钟、世界会话FIFO、建设/设施/人物/税收事务接线、自动月报、勋章桥、当前设施查询 | `runtime`或`rules` |
| `desktop/` | 显示计划、布局/输入、地图拼块/动画；纯计划也在headless运行 | `presentation` |
| `legacy/` | `ark_game`旧切片的规则、组合与诊断，仍由`--legacy-slice`使用 | `legacy` |
| `assets/`、`data/` | 素材解析和固定新局数据编译契约 | `rules`或`provenance` |
| `integration/` | 真实CLI、打包资源变异和长期世界包装器 | `e2e` |
| `support/` | 断言/进程入口与当前世界夹具；旧切片夹具放`legacy/support/` | 不注册业务用例 |

产品注册集中在`cmake/ProductTests.cmake`，冻结研究注册在`cmake/WorldSimulation.cmake`。两者都显式列出源文件；不使用GLOB。所有标准测试名称、参数和超时沿原合同保留。文件移动保留原文件名，原`tests/<name>`按上表迁入；测试整理本身不改研究`tests/simulation/**`内容或重算哈希；按新发布版本迁入时，以SOURCES.json记录的明确源差异更新。当前8f12654已迁入315项源/测试/数据，后续在途研究不纳入本批。

## 套件与独立进程

| 原入口 | 现在的可执行与显式用例 | 保留覆盖 |
| --- | --- | --- |
| `ark_world_report_tests` / `ark_world_medals_tests` | `ark_world_contract_tests world_report` / `world_medals` | 自动月报70/70次序/一次奖励/不重复维护费，诊断skip资格/拒绝；勋章真实脚本与年度共享Owner/溢出回滚 |
| `ark_world_panels_tests` / `ark_world_award_ui_tests` / `ark_world_crew_summary_tests` / `ark_world_tasks_tests` | `ark_world_ui_tests world_panels` / `world_award_ui` / `world_crew_summary` / `world_tasks` | 页面布局/通知映射、授勋与晋级输入门槛、真实有序成果名单、任务ID/费用/管理/阶段资格/输入 |

菜单输入回归使用`ark_world_ui_tests world_menu`，建设/设施/入住/升级与预览映射使用同一套件的`world_building`；窗口冻结、菜单原子切换、任务/授勋/晋级FIFO由`world_session`负责。`world_building_commands`单独覆盖当前Owner建设和设施命令的失败/回滚/实际提交，不在UI中重复底层造价、完整建设规则组合或自然任务长链。

`world_award_ui`覆盖授勋与progression页面的只读映射/输入门槛，`world_tasks`保留任务身份/费用、26/27及4→1边界；raw60完整映射与输入转由`world_human`承担，包含源ready、目录顺序、选择索引、库存报价、63当前职业/等级与目标分离、赠礼/大师/只读页。`world_tax`覆盖五行原序/合计/确认且无取消、自动98无输入。两者复用同一UI runner，不复制底层奖励算法。

`world_panels`覆盖notice前两条布局/标签解码及日期只读映射；周内比例只验已证`units/10800`，不认证原皮肤几何。`world_session`覆盖报告可见时菜单/人物/任务可操作、自动phase推进时真实世界继续、显式暂停不自恢复、与逐步源运行时等价，以及人物父子页恢复事务和声音一次领取。原规则计数、随机及奖励组合仍由冻结研究套件主责，上层只补接线风险。

`ark_world_ui_tests world_human_render_fixture`为显式窗口诊断，复用同一runner而不加入标准CTest。它用标注为synthetic的raw70三阶段输入检查实际皮肤/文字/布局，并验证绘制不改变日期、现金、随机或奖励属性；不运行奖励消费者，不替代自然大师整线或原版动画验收。

设施只读查询回归比较查询前后完整邻接缓存（键、current/previous、visited、完整有序sources/notices）及资金/随机/日期。e8已在初局初始化缓存，因此不能继续断言缓存初始为空，也不能退化为只比较数量。

合并的是重复链接和入口，不把不同场景合成串行大流程。正文仍按职责独立文件；CTest使用原名称，每次只选择一个case并启动新进程，错误归属和夹具隔离保留。未指定或指定不存在的case返回2，断言失败返回1；没有隐式“运行全部”。月报/勋章采用相同真实新局夹具与运行时依赖，因此共享可执行；线程会话、长跑、CLI进程检查及冻结来源测试保持独立。

共享`Checks`保留Release断言、单case计数和诊断上下文；六个重构case的原断言表达式、消息和调用顺序逐项保留。其他套件的专有检查暂留在各自文件，只有确认相同语义时才抽取。

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

新增回归先找行为所属套件，明确“旧契约→保留场景”和独立期望值。保留有效错误拒绝、回滚、随机与调用顺序保障；不能因文件同名删除旧切片或冻结研究测试。详细要求见[开发流程](../docs/CONTRIBUTING.md#测试设计与组织)。
