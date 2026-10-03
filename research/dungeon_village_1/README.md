# 《冒险迷宫村》一代参考研究

本目录保存 R1 行为研究、R2 素材、维护规则与收敛版可视原型。研究以用户提供的一代 Android 安装包为行为证据，并将由此形成的
Ark 设计示例与产品构建保持独立。

## 当前优先级

用户已明确先做 [原版页面/地图/视觉交互基线](stages/R3-original-visual-baseline.md)，
再逐步恢复人物完整优先级、进入/退出效果和成长。
[视觉索引](VISUAL_BASELINE.md)与 [用户两批共14张实况图片](references/README.md)
已归档，新增募集入住/城镇/事件/冒险/赠礼/商店观察；按用户选择继续用截图补页面，
不安装模拟器。固定 APK 运行和连续交互尚未验证。
既有退出/生命值辅助交付保持独立，不代表完整人物行为；日历候选暂缓。

## 范围与证据边界

- [EVIDENCE.md](EVIDENCE.md) 记录输入身份、工具版本、类映射、源码位置和反编译限制。
- [用户视觉参考](references/README.md)按来源批次和页面类别保存截图原图、时间点、尺寸/哈希与观察，
  独立于 APK 原始资产；不同版本/存档差异不自动合并成规则。
- [BEHAVIOR.md](BEHAVIOR.md) 记录建造、角色寻路与设施使用、设施实例、全局状态、结算时序及
  可拆分的领域边界。
- [人物自主行动报告](AUTONOMY.md) 补充自主调度、两级设施选择、道路/空地加权寻路与到达/退出事件区别。
- [R2-B 阶段记录](stages/R2-character-autonomy.md) 记录无人值守授权、自主行动设计与验收；
  核心与新版画面通过；用户已免除鼠标验证，用户试玩未运行，不再阻塞后续研究设计。
- [设施字段与几何](FACILITIES.md) 和 [原始数据](data/README.md) 保存 85 条设施定义、来源与未知列；
  [R2-C](stages/R2-facility-data-geometry.md) 的有限数据/规则交付已收尾，完整效果/日历细节顺延。
- [周期账本与双资源结算](ACCOUNTING.md) 区分即时金币、费用批次、月报快照与村子点数，
  修正 R1 对报表关闭时资金入账的过度概括。
- [地图访问与多格设施](MAP_ACCESS.md) 和 [R2-D](stages/R2-map-access.md)维护实际占用格绑定、
  五类路径准入、完整距离场、可达格及到达身份检查；收敛版已接入，旧单点模拟保留。
- [邻接属性与来源](NEIGHBOURHOOD.md) 和 [R2-E](stages/R2-neighbourhood.md)维护 8/10/12 格外环、
  来源实例去重、道路魅力及经营输入转换；收敛版已接入，不代写产品状态。
- [设施事件与实例门槛](FACILITY_EVENTS.md) 和 [R2-F](stages/R2-facility-events.md)维护第 33 列整数矩阵、
  道具确认/跨月计数和受限延迟展示计划；不将它当人物消费效果或直接发人气。
- [道具表与共享改良](FACILITY_ITEMS.md) 和 [R2-G](stages/R2-facility-items.md)维护 36 条道具、类别适配、
  共享改良/库存/实例计数候选事务；封顶无显示变化仍消费，未接入旧模拟或产品。
- [人物活动类别选择](ACTIVITY_CHOICE.md) 和 [R2-H](stages/R2-activity-choice.md)维护访问计数/标志/可用类别
  计划与权重票号；收敛版仅消费普通类别 1/2，完整角色优先级未恢复。
- [已排序设施格选择](RANKED_FACILITY_CHOICE.md)和 [R2-I](stages/R2-ranked-facility-choice.md)通过低层
  输出核实多格重复计权与所选实例首格，维护地图到达组合测试；后续候选细化见 R2-J。
- [活动候选生成](ACTIVITY_CANDIDATES.md)和 [R2-J](stages/R2-activity-candidates.md)继续核对普通/城内外
  扫描、事件重复/不可达项、交换平局次序和类别 4 计数/扫描差异；完整角色优先级仍未恢复。
- [设施普通到达](FACILITY_ARRIVAL.md)和 [R2-K](stages/R2-facility-arrival.md)维护分类计数/收费守卫、
  角色统计/实例月销售和即时收入候选；不把统计当钱包，不实现同行/装备抽选或退出效果。
- [完整候选快照选择](SNAPSHOT_FACILITY_CHOICE.md)和 [R2-L](stages/R2-snapshot-facility-choice.md)维护
  活动/完整列表下标、重复事件权重及可空目标成本，不自动删不可达项或重抽，不修改 R2-I 旧契约。
- [退出共享使用数与满足度](FACILITY_EXIT.md)和 [R3-A](stages/R3-facility-exit-helpers.md)维护升级提示
  锁存与品质/职业门槛随机比较，人气请求和实际满足度变化分离；不扩张 R2 收敛或接入原型。
- [旅店恢复与生命值过渡](CHARACTER_HP.md)和 [R3-B](stages/R3-character-hp.md)维护真实目标/显示值、
  变化重启/直接赋值/短计数协议；不把逻辑计数当秒数，不改原型完成策略。
- [VERIFICATION.md](VERIFICATION.md) 记录本研究快照可复现的分析、构建、测试、格式和维护树检查。
- [R2-M 区域回退](stages/R2-regional-choice.md)维护五次条件抽取与第六次无条件回退，
  票号前缀显式可重放；已验收，不包含完整区域 AI。
- [R2 收敛](stages/R2-convergence.md)固定有限原型目标与剩余验收；细节研究不再无限扩张 R2。
- [日历与月报调度候选](stages/R3-calendar-report-scheduling.md)保留此前方案；
  用户调整优先级后暂缓（Planned），未编码，不把规划当作已交付规则。
- [历史持续研究续作](stages/持续研究续作-2026-10-02.md)汇总当时 R2-J/K、月报低层核对和缺口，
  最新交接以收敛记录为准。
- [第二批无人值守研究交接](stages/无人值守研究-2026-10-02-第二批.md)汇总 R2-D–I 检查点、当前验收与
  下一批最小缺口；[第一批历史交接](stages/无人值守研究-2026-10-02.md)保持原时段记录。
- [R1 阶段记录](stages/R1-dungeon-village-reference.md) 保存 R1 的设计、确认、实施问题与交付历史。
- [R2-A 阶段记录](stages/R2-art-assets-cpp-prototype.md) 定义已确认的美术资源提取、可替换素材契约和
  C++ 可视原型设计。
- [example/](example/) 是独立编写的 C++17 参考代码。它是 Ark 设计草图，不是反编译代码翻译，
  也不属于仓库根 CMake 工程。
- [素材与接入说明](ASSETS.md) 记录视觉资源、格式与证据边界、七个活动逻辑键、原型操作及换图方式。
- [prototype/](prototype/) 的 [状态接口](prototype/include/dungeon_village_prototype/village.hpp)、
  [聚合实现](prototype/src/village.cpp)与 [组合测试](prototype/tests/village_test.cpp)组装维护规则；
  [试玩清单](stages/R2-playtest.md)单独记录真实操作，不替代产品验收。
- `work/` 保存 JADX 生成结果。该目录被忽略，不得提交，也不得视为维护源码。

APK 是汉化重签包，因此结论只描述这一固定输入，不代表游戏的所有发行版本。维护成果目前不包含
完整数值表或反编译实现；已维护本轮授权的原始与规范化视觉素材。产品导入及映射必须通过
[research 统一入口](../README.md)记录。

## 复现反编译分析

确保 `research/dungeon_village_1/maoxianmigongcun.apk` 存在，并安装 JADX 1.5.6：

```sh
jadx --show-bad-code --comments-level warn \
  --output-dir research/dungeon_village_1/work/decompiled \
  research/dungeon_village_1/maoxianmigongcun.apk
```

本次执行还生成了 `callgraph.json`。带有 JADX 控制流警告的方法只作为辅助证据引用，其重建后的
分支顺序不会被报告为直接事实。

## 构建独立示例

```sh
cmake -S research/dungeon_village_1/example \
  -B research/dungeon_village_1/work/example-debug \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build research/dungeon_village_1/work/example-debug --parallel 2
ctest --test-dir research/dungeon_village_1/work/example-debug --output-on-failure
```

Release 验证使用独立构建目录并指定 `-DCMAKE_BUILD_TYPE=Release`。本目录内的命令不会修改 APK、
生成的反编译结果或 Ark 产品状态。
