# 可玩研究原型包

默认运行已发布的新局建设/首访保护切片；`--world`显式运行完整目录的共同世界AI。
旧7×7自主访问演示只在`--fixture`运行，三者不共享可写世界，也不冒充完整原版复刻。
本包独立于产品主构建，只修改研究代码。
证据与缺口集中在[新局报告](../rules/STARTUP.md)，不以截图或演示初值补齐未知原版规则。

## 职责与依赖

| 文件/目标 | 职责 |
| --- | --- |
| [startup.hpp](include/dungeon_village_prototype/startup.hpp)、[startup.cpp](src/startup.cpp) | 标准C++17新局聚合；消费加载后身份与建设地表，免费初始化、建设事务、施工、首访和模态调度 |
| [startup_map.hpp](include/dungeon_village_prototype/startup_map.hpp)、[startup_map.cpp](src/startup_map.cpp) | 576格/8实例静态重建、入口编号、逻辑/显示分离、边界/道路刷新与非零寻路身份投影；不是可变运行地图 |
| [dump_startup_map.cpp](src/dump_startup_map.cpp) | 导出[发布TSV](../data/startup/README.md#加载后快照)，不访问APK；全字段对账是独立CTest |
| [数据编译脚本](scripts/README.md) | 构建期用Node内置JSON解析生成只读C++；完整地图字节哈希、原表和派生契约交叉验证，运行不依赖Node |
| [startup_view.hpp](include/dungeon_village_prototype/startup_view.hpp)、[startup_view.cpp](src/startup_view.cpp) | raylib新局画面、中文替代字体、源SEB/图片绑定、目录/放置/提示与有界页面检查；核心不接收屏幕坐标 |
| [village.hpp](include/dungeon_village_prototype/village.hpp)、[village.cpp](src/village.cpp) | 保留旧夹具的建设、多格布局、自主访问、到达收入和周期事务，不混入默认新局 |
| [catalog.cpp](src/catalog.cpp) | 旧夹具设施表投影，仍只接28/29/36 |
| [asset_manifest.hpp](include/dungeon_village_prototype/asset_manifest.hpp)、[asset_manifest.cpp](src/asset_manifest.cpp) | 旧夹具的七键替换素材契约 |
| [main.cpp](src/main.cpp) | CLI入口，显式分流默认保护切片、共同世界和旧夹具 |
| [startup_world_projection.hpp](include/dungeon_village_prototype/startup_world_projection.hpp)、[实现](src/startup_world_projection.cpp) | 真实新局一次性接管、完整目录和稳定维护身份，不保存第二个可写StartupState |
| [startup_world_runtime.hpp](include/dungeon_village_prototype/startup_world_runtime.hpp)、[实现](src/startup_world_runtime.cpp) | 唯一世界/台账/随机/日历所有者；队首时读取最新事实，整帧事务和不可变内存检查点 |
| [场景](src/startup_world_runtime_scene.cpp)、[日历](src/startup_world_runtime_calendar.cpp)、[到访](src/startup_world_runtime_arrival.cpp) | 原入口资格/全局表现、跨月各域和真实人物创建/89/Q刷新时序 |
| [独立焦点](src/startup_world_runtime_focus.cpp) | 主场景2的W单独所有权、按键运动/共享成长/FIFO/物理与留存，不混入自主人物名单；普通入口守卫不放宽 |
| [任务](src/startup_world_runtime_tasks.cpp)、[非人物](src/startup_world_runtime_nonactors.cpp)、[页面](src/startup_world_runtime_pages.cpp) | 真实任务工厂/探索地图/奖励、投射/拾物/遭遇消费，以及成果/赠礼页的独立更新和输入 |
| [玩家任务页](src/startup_world_runtime_task_pages.cpp) | 页22原名单、23付费接受、24真实征集、25队伍、27追加、28正式出发；临时领域投影同步消费真实脚本/页栈，不另存任务世界 |
| [期限与返回](src/startup_world_runtime_deadline.cpp) | 页33续费/中止演出、栈外原关闭页引用及真正主场景返回；当前报价/现金/任务清理/地图恢复整轮提交 |
| dungeon_village_startup_model | 标准C++17静态证据与新局规则，无raylib/字体/JSON运行依赖 |
| dungeon_village_startup_world | 标准C++17共同世界与完整原表；不依赖raylib，不在运行时读取APK/研究临时文件 |
| dungeon_village_prototype_model | 旧夹具聚合，复用[领域示例](../example/README.md)与[工具](../tools/README.md)表解析 |
| [测试目录](tests/) | 新局/共同世界/任务/页面/连续运行、发布表/错误输入及旧夹具；当前计数以[本轮验证](../VERIFICATION.md)为准 |

[CMake](CMakeLists.txt)把所需原始图集/SEB及人物、怪物、common目录复制到程序旁。
运行只读该素材目录；设施新局数据编译进程序，不访问APK、反编译结果或发布JSON。
源路径/哈希见[素材清单](../assets/MANIFEST.tsv)，不重复发布或修改原图。

## 构建与运行

构建需要C++17、CMake、Node、raylib和pkg-config。从仓库根目录执行：

```sh
cmake -S research/dungeon_village_1/prototype -B research/dungeon_village_1/work/prototype-debug-llvm -DCMAKE_BUILD_TYPE=Debug
cmake --build research/dungeon_village_1/work/prototype-debug-llvm --parallel 2
ctest --test-dir research/dungeon_village_1/work/prototype-debug-llvm --output-on-failure
research/dungeon_village_1/work/prototype-debug-llvm/dungeon_village_prototype --paused
```

本机沿用[现有CLion/LLVM工具链](../verification/BASELINE.md)。暂停开始用`--paused`，
无窗口新局自检为`--check`，有界运行用`--frames N`，`--screenshot`仅用于有界截图。
默认字体是本机`/System/Library/Fonts/Supplemental/Arial Unicode.ttf`，未复制或分发；
其他环境用`--font /绝对路径/中文字体.ttf`指定字体，缺少字形明确失败，不悄悄画方框。

## 共同世界入口

```sh
research/dungeon_village_1/work/prototype-debug-llvm/dungeon_village_prototype --world
research/dungeon_village_1/work/prototype-debug-llvm/dungeon_village_prototype --world --check
research/dungeon_village_1/work/prototype-debug-llvm/dungeon_village_prototype --world --inspect-page world-month --frames 12 --screenshot research/dungeon_village_1/work/world-month.png
```

`--world`从真实空人物新局开始：介绍、自动到访、89提示/镜头、自主设施选择/移动/使用/退出，
之后继续执行共同世界、日历、后续到访、遭遇、任务生成、费用和月报。
Enter或确认点击逐段处理当前页；暂停同时冻结世界和栈顶页；倍速只改变框架内1/2逻辑轮数，
不把移动速度乘二。右键拖动是表现视图偏移，不能直接控制人物。
退出不写正常存档；在原自动保存调用时点仅保存完整不可变Owner内存检查点，这是明确研究策略。

任务自然出现后，点击底部“任务”或按T打开列表；上下键/点击选择，Enter确认，Esc取消。
接受页按原表扣征集费，征集页自主计数，按住Enter可按原held规则加速；队伍页选队员确认出发，
末行“追加队员”进入按实际职业等级报价的付费候选页。出发确认先播放计数阶段，到门槛才安装任务；
此后人物自行寻路、进入设施/战斗，玩家不控制移动。原型列表为简化240×320表现适配，不宣称原UI皮肤一致。
当前菜单公开入口仅在主场景且无活动任务时开放；期限页已接继续/中止选择、演出和真正返回主场景的结算。
主动打开活动任务名单及菜单中的“终止任务”仍未接，不把期限中止当作全任务管理菜单完成。

`--world --inspect-page task-team --frames 12 --screenshot 路径`从真实新局自然等候任务，
提供明确的测试玩家接受输入，停在真实队伍页作有界窗口检查，不注入任务、人物或资金。

显示使用原人物职业/性别、怪物体型/变体索引及SEB，逐格消费建筑分片/道路补块/边界/入口。
月报读取唯一Owner已提交的快照，不重复扣维护费；页面30/31/32关闭不再次发奖励。
成果后的事件消息页11保留40计数快进/关闭门槛；首次战斗成功延迟出现的商店追加页83用Esc或取消返回，
确认购买84/85尚未接入，不用取消替代购买。人气奖励终点页97等待实际页面更新自动清R/返回，
不接受玩家确认，不重复奖励。页面只是当前原脚本链，不能直接清整栈恢复世界。
人物解锁页59首轮音效5，满70次页面更新后确认才提高到访优先值并返回；不快进或直接创建人物。
`world-active`/`world-month`检查先在同一真实世界逐轮确认页面，到达三人物/月报时再开画面；
这是明确输入策略，不改世界数值，但也不是人工操作或固定APK动态捕获。
`visitor`是旧首访静态接管检查，不混入自然长跑结果。

默认seed1仅为明确可重放研究输入，固定APK的实际默认seed未观测。

### 可持续世界验收入口

[连续测试](tests/startup_world_continuous_test.cpp)接受`月份数 种子 速度`，月份1..36、速度0/1。
默认仍是seed1两个月；框架更新后检查日期、随机和真实页栈，不注入人物、任务、资金或等待状态。
raw16定时等待、raw56/57镜头页及raw97人气奖励终点自动推进，不以确认删除；
raw49条件页不执行晋级；raw89保留40快进门槛。
年度raw87在明确测试输入中选择“终止→确认”，未用勋章保留，事件22与背景音乐按原序消费。
这些输入只用于验收：窗口尚未接授勋按钮/是非弹窗，也没有授予/raw88、活动任务管理或文件存取。

```sh
research/dungeon_village_1/work/prototype-debug-llvm/dungeon_village_startup_world_continuous_tests 12 1 0
research/dungeon_village_1/work/prototype-debug-llvm/dungeon_village_startup_world_continuous_tests 6 20261005 1
```

配置时加`-DDUNGEON_VILLAGE_LONG_WORLD_TESTS=ON`可登记两个可重复长期CTest，
以`ctest --test-dir <构建目录> -L long-world --output-on-failure`执行；3600秒仅是墙钟保护，不参与原规则。
当前跨年、多种子、插桩与六套实际结果见[验证](../VERIFICATION.md)，不从单条路径推导全部状态可达或无限期认证。

[自然任务流程测试](tests/startup_world_task_flow_test.cpp)接受可选`种子 速度`（默认`1 1`），
从真实空人物新局等待任务，按真实现金选择可支付任务，显式接受、等待征集并确认出发。
世界自行创建人物、寻路、探索或战斗，测试不注入成功/奖励。两类自然成功后不再接受新任务，
继续确认实际成果页与脚本，等待126/128/201/205/92/80相关任务续体退出、
成功后的真实任务工厂生成并返回主场景，才算闭环完成。
期限能支付则续费，否则明确选择原中止；取消、旧页重试、晚期页锁失败及成果重复奖励另有断言。
150000框架调用和3600秒是验收保护，不是原任务期限或游戏时钟；普通两月检查墙钟保护为1800秒。
并行构建导致CPU限流时先减少研究长测并发，不能改日期目标、删断言或补成功来缩短检查。

完整目录/共同AI模式与建设切片分开：本入口尚未提供全建设/全部任务管理交互和完整原UI皮肤；
不能从真实AI连续运行推出全部菜单、音频、图层桶溢出或跨版本截图视觉等价。
图标/文字、逻辑240×320及窗体480×640都是表现适配，验收边界见[当前验证](../VERIFICATION.md)。

默认窗口为240×320逻辑画布、480×640物理像素。建设→目录分类→条目→地图格→确定。
返回或Esc取消，放置模式支持连续建设；右键拖动视图，暂停/继续和1倍/2倍按钮控制模拟资格。
施工只在正常模式推进。首访两句提示逐次确认，然后镜头移到人物；没有人物移动命令。
退出不保存。详细人工清单见[试玩说明](../stages/PLAYTEST.md)。

页面截图检查不模拟鼠标，必须指定有界帧数：

```sh
research/dungeon_village_1/work/prototype-debug-llvm/dungeon_village_prototype --inspect-page shops --frames 8 --screenshot research/dungeon_village_1/work/startup-shops.png
```

支持`roads`、`shops`、`food`、`arrival`、`visitor`；它们明确安排检查状态，不能作为正常新局运行轨迹。

## 默认保护切片的来源边界

- 源24×24格、8设施种子、5000金币/10点数/50人气、1年4月、空人物场景与初期9项目录来自发布包。
  核心已重建576逻辑/显示格、8实例身份和向量顺序，建设使用加载后地表；原始ID0通过独立映射保留。
  这是静态重建，不是原版动态捕获；此旧窗口与共同世界的新逐格绘制入口分开登记。
- 普通商店280逻辑计数、入住募集1、向日葵立即可用；职业影响按已解锁定义计算，不按活人计算。
  募集建好后的住宅/入住副作用未实现，不由“施工完成”推导居民加入。
- 自动首访420次有资格更新，先创建UID0/定义1丰田龟次郎再触发事件89，不收取1500入住费。
  属性/装备及装配后的武器重选计数6保留；出生点由本地可重放策略选择，不还原APK随机消费。
  最终朝向/武器合成表现仍未知，介绍事件7和逐字对话节奏尚未接入。
- 首访实例标志为2与8192的组合，保留待处理活动0；人物仍停留出生格，暂不套用旧自主AI。
  原版可在教程触发同轮执行首次选路，原型延后请求是研究保护，不是原版“关闭教程才启动AI”。
  不能把当前静止当作原版行为，也不安排后续普通来客；[出发规则](../rules/CHARACTERS.md#first-activity)可独立组合验证。
  [地图组合](tests/startup_map_test.cpp)已覆盖两出生点到三个商店的连续行走与逻辑格进入，但只用于无玩家修改的新局测试，
  不在窗口自动执行；武器店未从候选删除，武器单类选择已交付，但装备到达/完整使用仍受保护。
  [使用组合](tests/startup_use_test.cpp)已连接两出生点的逻辑进入、原表基础端点、到达收入、包子60/旅店200等待、
  旧计数170恢复和共享完成/满足度候选；以显式目标及零实例修正为前置，只做无窗口条件测试。
  条件测试另覆盖单格出口保留、包子属性尾部与活动0顺序、首名武器计数正时不消耗抽选票号。
  [首段私有AI](include/dungeon_village_prototype/startup_ai.hpp)及[回归](tests/startup_ai_test.cpp)另已连接自主两级选择、
  邻接价格、占用/等待/完整退出、下一活动、包子属性累计和武器实际装配，覆盖两个出生点。
  它不接默认窗口/日历/玩家改图；表情/装备显示保留请求，不伪造随机变体或像素载荷。
  运行`dungeon_village_startup_ai_tests`可执行此无窗口组合；不能登记为默认全局AI或固定APK轨迹认证。
- [道路绘制参数](include/dungeon_village_prototype/road_render.hpp)交付整图绑定/尺寸、SEB锚点偏移及原深度；
  [CPU素材组合](tests/road_render_test.cpp)验证2×2原草心、原表道路flags1深度与前景遮挡。
  表现参数只属于原型表现边界，不引入领域库；共同世界窗口已消费，旧保护切片仍单独登记。
- 每正常逻辑步日历加27，每子周期10800、月4子周期；展示帧目标60FPS只是适配节奏。
  目录/放置/提示/镜头不推进模拟；2倍速每外层最多2步，不换算成原版真实秒。
- 首个月界进入受保护的月末状态，不扣夹具维护费、不给演示收益，等待跨月组合时序闭合。
- 新增/撤除后的地表暂用显示记录27，道路暂用37的源首帧；占用/扣款可测，邻接刷新不是原版重建。
  原型连续放置、撤除退款为0、窗体尺寸/物理热区是适配边界。
- 草地、建筑、入口与人物使用原始美术；首访使用秘书，而非后续解锁插画。
  字体、窗体和图标缩放是替代，不宣称固定APK或不同版本截图的视觉等价。

## AI表现适配交付

[facility_projection.hpp](include/dungeon_village_prototype/facility_projection.hpp)维护类别8休息/类别6特殊入口的
旧s→原60×30投影偏移→h.e世界坐标截断。领域只消费世界目标，不出现原像素位置；
[回归](tests/facility_projection_test.cpp)检查四方向×576格并连接完整使用队列。

[loop_pacing.hpp](include/dungeon_village_prototype/loop_pacing.hpp)维护固定APK默认47ms绘制路径门槛、
等待后观测提交和普通模式倍速迭代上限；[回归](tests/loop_pacing_test.cpp)覆盖整数除法、长停顿无补算及时钟回拨。
来源/限定见[限速规格](../rules/ai/LIFECYCLE.md#原版墙钟限速)。这两个适配均无窗口、无线程、无真实sleep，
交付给后续产品接入，不擅自修改当前窗口节奏或解开AI/月界保护。
框架调用链纠正了旧“栈顶更新前限速”的解释：更新路径先返回，下一g才限速/刷新输入/绘制。
纯等待数值函数未变；不能把其门槛应用在每个1/2逻辑轮之前。

## 旧夹具

`--fixture`显式进入旧7×7场景；`--demo`隐含该模式并自动退出。
`--fixture --check`执行原自主访问/周期闭环；`--orientation 0/1`与`--data-file`仅作用于旧夹具。
`--asset-root`是两个模式共同的程序旁素材根目录，新局要求保留`original`结构，旧模式要求七个清单键。

旧模式10000初始金币、三次建设后7300、两位夜骑士、600个100ms tick月份都是夹具。
仅类别1/2、独占预约、完成回接近格、统一旋转收费、周期报表仍是原型策略。
它保留此前自主访问回归，不用于认证首名人物行为或产品新局初值。
