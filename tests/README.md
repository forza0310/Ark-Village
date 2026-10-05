# 测试组织

按被测所有者和依赖找用例；迁入研究测试与产品适配测试分开。标准CTest仍执行全部适用用例，标签用于定位，不能以单层通过替代阶段验收。

| 目录 | 负责的契约 | 主标签 |
| --- | --- | --- |
| `simulation/rules/` | 冻结研究纯规则与已证拒绝/边界 | `rules;frozen` |
| `simulation/` | 冻结研究共同Owner、页面、路由和自然世界组合 | `runtime;frozen`，连续轨迹为`e2e` |
| `app/` | 产品启动/原时钟、世界会话FIFO、手动月报、勋章桥、当前设施查询 | `runtime`或`rules` |
| `desktop/` | 显示计划、布局/输入、地图拼块/动画；纯计划也在headless运行 | `presentation` |
| `legacy/` | `ark_game`旧切片的规则、组合与诊断，仍由`--legacy-slice`使用 | `legacy` |
| `assets/`、`data/` | 素材解析和固定新局数据编译契约 | `rules`或`provenance` |
| `integration/` | 真实CLI、打包资源变异和长期世界包装器 | `e2e` |
| `support/` | 断言/进程入口与当前世界夹具；旧切片夹具放`legacy/support/` | 不注册业务用例 |

产品注册集中在`cmake/ProductTests.cmake`，冻结研究注册在`cmake/WorldSimulation.cmake`。两者都显式列出源文件；不使用GLOB。所有标准测试名称、参数和超时沿原合同保留。文件移动保留原文件名，原`tests/<name>`按上表迁入；研究`tests/simulation/**`路径与内容不变，SOURCES.json不重算。

## 套件与独立进程

| 原入口 | 现在的可执行与显式用例 | 保留覆盖 |
| --- | --- | --- |
| `ark_world_report_tests` / `ark_world_medals_tests` | `ark_world_contract_tests world_report` / `world_medals` | 月报冻结/两步确认/一次奖励/错误拒绝；勋章真实脚本与年度共享Owner/溢出回滚 |
| `ark_world_panels_tests` / `ark_world_award_ui_tests` / `ark_world_crew_summary_tests` / `ark_world_tasks_tests` | `ark_world_ui_tests world_panels` / `world_award_ui` / `world_crew_summary` / `world_tasks` | 页面布局、年度操作、真实有序成果名单、任务ID/费用/阶段资格/输入 |

新增菜单输入回归使用同一`ark_world_ui_tests world_menu`入口；窗口冻结与任务原子切换由`world_session`负责，避免在表现层重复完整世界流程。

合并的是重复链接和入口，不把不同场景合成串行大流程。正文仍按职责独立文件；CTest使用原名称，每次只选择一个case并启动新进程，错误归属和夹具隔离保留。未指定或指定不存在的case返回2，断言失败返回1；没有隐式“运行全部”。月报/勋章采用相同真实新局夹具与运行时依赖，因此共享可执行；线程会话、长跑、CLI进程检查及冻结来源测试保持独立。

共享`Checks`保留Release断言、单case计数和诊断上下文；六个重构case的原断言表达式、消息和调用顺序逐项保留。其他套件的专有检查暂留在各自文件，只有确认相同语义时才抽取。

## 运行与扩展

```sh
ctest --preset desktop-release --output-on-failure
ctest --preset desktop-release -L presentation --output-on-failure
ctest --preset headless-debug -R '^(world_report|world_medals|world_session)$' --output-on-failure
```

四套标准验收均含seed1三个月基线。`ARK_LONG_WORLD_TESTS=ON`额外注册`long_world`，子标签`annual_world`和`natural_tasks`，按本阶段规则风险选择；测试组织修改且规则/长跑正文/参数未变时，不重复前批长链。结构调整仍核对长测的路径、命令、月份/种子/断言参数与注册。

新增回归先找行为所属套件，明确“旧契约→保留场景”和独立期望值。保留有效错误拒绝、回滚、随机与调用顺序保障；不能因文件同名删除旧切片或冻结研究测试。详细要求见[开发流程](../docs/CONTRIBUTING.md#测试设计与组织)。
