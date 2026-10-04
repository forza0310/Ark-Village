# 可玩研究原型包

默认运行已发布的新局证据模式，旧7×7自主访问演示只在显式夹具模式运行。
两者均不是完整原版复刻；本包独立于产品主构建，只修改研究代码。
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
| [main.cpp](src/main.cpp) | CLI入口，显式分流默认新局与旧夹具 |
| dungeon_village_startup_model | 标准C++17静态证据与新局规则，无raylib/字体/JSON运行依赖 |
| dungeon_village_prototype_model | 旧夹具聚合，复用[领域示例](../example/README.md)与[工具](../tools/README.md)表解析 |
| [测试目录](tests/) | 新局状态/事务/调度、加载后地图/行走/受限使用组合、发布快照/错误输入与旧回归；含21项领域测试共34项CTest |

[CMake](CMakeLists.txt)把所需原始图集/SEB、秘书和农家图片复制到程序旁。
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

默认窗口为240×320逻辑画布、480×640物理像素。建设→目录分类→条目→地图格→确定。
返回或Esc取消，放置模式支持连续建设；右键拖动视图，暂停/继续和1倍/2倍按钮控制模拟资格。
施工只在正常模式推进。首访两句提示逐次确认，然后镜头移到人物；没有人物移动命令。
退出不保存。详细人工清单见[试玩说明](../stages/PLAYTEST.md)。

页面截图检查不模拟鼠标，必须指定有界帧数：

```sh
research/dungeon_village_1/work/prototype-debug-llvm/dungeon_village_prototype --inspect-page shops --frames 8 --screenshot research/dungeon_village_1/work/startup-shops.png
```

支持`roads`、`shops`、`food`、`arrival`、`visitor`；它们明确安排检查状态，不能作为正常新局运行轨迹。

## 默认模式的来源边界

- 源24×24格、8设施种子、5000金币/10点数/50人气、1年4月、空人物场景与初期9项目录来自发布包。
  核心已重建576逻辑/显示格、8实例身份和向量顺序，建设使用加载后地表；原始ID0通过独立映射保留。
  这是静态重建，不是原版动态捕获；窗口仍按源格/设施适配绘制，边界覆盖、入口标记和道路画面尚未接入。
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
  表现参数只属于原型表现边界，不引入领域库；窗口源图绘制器尚未消费该参数。
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

[loop_pacing.hpp](include/dungeon_village_prototype/loop_pacing.hpp)维护固定APK默认47ms最小外层开始间隔、
等待后观测提交和普通模式倍速迭代上限；[回归](tests/loop_pacing_test.cpp)覆盖整数除法、长停顿无补算及时钟回拨。
来源/限定见[限速规格](../rules/ai/LIFECYCLE.md#原版墙钟限速)。这两个适配均无窗口、无线程、无真实sleep，
交付给后续产品接入，不擅自修改当前窗口节奏或解开AI/月界保护。

## 旧夹具

`--fixture`显式进入旧7×7场景；`--demo`隐含该模式并自动退出。
`--fixture --check`执行原自主访问/周期闭环；`--orientation 0/1`与`--data-file`仅作用于旧夹具。
`--asset-root`是两个模式共同的程序旁素材根目录，新局要求保留`original`结构，旧模式要求七个清单键。

旧模式10000初始金币、三次建设后7300、两位夜骑士、600个100ms tick月份都是夹具。
仅类别1/2、独占预约、完成回接近格、统一旋转收费、周期报表仍是原型策略。
它保留此前自主访问回归，不用于认证首名人物行为或产品新局初值。
