# 素材与原型入口

`research/` 是 Ark-Village 的素材、规则原型与行为证据的唯一来源入口。规则冲突以维护的独立示例为准；产品实现仍按阶段设计接入，研究结果不等于已经完成产品验收。

## 当前研究

- 当前优先级：先 [原版页面/地图/视觉交互基线](dungeon_village_1/stages/R3-original-visual-baseline.md)，
  再逐步恢复人物完整优先级、进入/退出效果与成长。首批 [静态视觉索引](dungeon_village_1/VISUAL_BASELINE.md)
  已形成，另有 [用户两批共14张实况图片](dungeon_village_1/references/README.md)
  分类保存并登记观察。用户选择截图路线，不安装模拟器；固定 APK 运行与连续交互未验证，不是已完成画面复刻。
- [冒险迷宫村一代](dungeon_village_1/README.md)：研究范围、复现与构建方法。
- [行为规格](dungeon_village_1/BEHAVIOR.md)：直接事实、推断、Ark 设计建议及未知项。
- [自主行动与路径](dungeon_village_1/AUTONOMY.md)：人物自主调度和道路加权成本证据；
  [R2-B](dungeon_village_1/stages/R2-character-autonomy.md) 核心与新版画面通过，鼠标验证已免验，用户试玩未运行。
- [证据索引](dungeon_village_1/EVIDENCE.md)：输入身份、哈希、工具和定位信息。
- [设施数据与几何](dungeon_village_1/FACILITIES.md)：85 条严格校验的原始定义、两种朝向、建设价格/维护费消费者；
  独立几何与经营纯函数已验证，三类设施已接入研究原型；完整效果/日历细节顺延，不代写产品状态。
- [周期账本与双资源](dungeon_village_1/ACCOUNTING.md)：即时金币与村子点数分离、费用批次、报表快照；
  独立示例不把月报净额再入账，也不等于完整日历/任务复刻。
- [地图访问与多格设施](dungeon_village_1/MAP_ACCESS.md)：真实占用格绑定、完整准入与连通状态、到达身份；
  独立规则已验证，收敛版原型消费真实多格绑定/准入/到达身份，旧 R1 单点示例保留。
- [邻接属性与来源](dungeon_village_1/NEIGHBOURHOOD.md)：有序周边格、同类来源叠加/多格去重、道路魅力；
  严格重算并安全接续经营纯函数，不把外环当入口。
- [设施事件与实例门槛](dungeon_village_1/FACILITY_EVENTS.md)：严格事件矩阵、道具/月份计数与延迟展示计划；
  第 33 列不是每次人物使用效果，完整解释器与 UI 调度仍有未知项。
- [道具表与共享改良](dungeon_village_1/FACILITY_ITEMS.md)：36 条道具、12 类适配与候选事务；
  同类共享改良，当前实例独立事件计数，不混入人物消费或维护费。
- [人物活动类别](dungeon_village_1/ACTIVITY_CHOICE.md)：访问计数、标志 8192 与带权类别计划；
  外部提供票号，不把原作取模称均匀随机，不宣称已替换原型完整 AI。
- [活动候选与类别 4](dungeon_village_1/ACTIVITY_CANDIDATES.md)：普通/区域过滤、事件重复和不可达项、
  原交换排序与计数/扫描差异；[普通多格选择](dungeon_village_1/RANKED_FACILITY_CHOICE.md)保留重复权重。
- [普通设施到达](dungeon_village_1/FACILITY_ARRIVAL.md)：分类计数、收费守卫、角色统计和实例月销售候选；
  不把统计当钱包，不实现同行/装备抽选，现金重试不代替整个到达事实去重。
- [完整候选快照选择](dungeon_village_1/SNAPSHOT_FACILITY_CHOICE.md)：直接消费含重复/不可达事件的候选，
  保留活动/完整列表下标，不把选择成功当作寻路或使用成功。
- [C++ 原型](dungeon_village_1/example/)：独立编写的 C++17 示例及测试，不属于主 CMake 构建。
- [R1 阶段记录](dungeon_village_1/stages/R1-dungeon-village-reference.md)：设计、用户确认、问题与交付历史。
- [R2-A 阶段记录](dungeon_village_1/stages/R2-art-assets-cpp-prototype.md)：美术资源提取与可替换素材的
  C++ 可视原型设计和实施状态。
- [素材交付与原型操作](dungeon_village_1/ASSETS.md)：761 个原始视觉文件、七个活动逻辑键、来源清单与换图契约。
- [可视原型](dungeon_village_1/prototype/)：独立 C++17 + raylib；已接入三类真实设施、多格/两朝向、
  两级选择与自主行动、到达收入和周期维护；完整 AI/日历仍非原作等价。
- [R2 收敛记录](dungeon_village_1/stages/R2-convergence.md)与 [试玩清单](dungeon_village_1/stages/R2-playtest.md)：
  研究交付与自动验收完成，用户已免除鼠标验证，用户试玩未运行；不再阻塞下一阶段研究设计。
- [日历与月报调度候选](dungeon_village_1/stages/R3-calendar-report-scheduling.md)：
  用户调整优先级后暂缓（Planned），保留候选，未编码；不再作为当前下一项实现。

## 素材接入

当前原始输入为 `research/dungeon_village_1/maoxianmigongcun.apk`；原包及 `work/` 中的生成分析保持只读参考，不作为维护源码提交。用户已允许在必要时使用包内素材。

后续新增素材及原型先在本目录登记来源。实际接入时记录输入哈希、内部资源路径、解码/裁剪方式、输出文件与产品映射，再将运行所需副本导入产品 `assets/`；不让游戏运行依赖研究工具、原 APK 或反编译源码。当前未解码的内容不得描述为已经可用的资源。

产品 `assets/` 的临时建筑映射与 catalog 数值不自动成为原作参数或新的来源依据。新增规则和素材继续从本入口获取，现有产品行为与原型冲突时按原型调整。
