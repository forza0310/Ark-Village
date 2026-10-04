# 独立领域规则包

本包只有标准 C++17，不依赖 raylib、窗口、图片、APK 或平台坐标。
公开接口位于 [include](include/dungeon_village_reference/)，实现位于 [src](src/)，对应回归位于 [tests](tests/)。
[CMake](CMakeLists.txt)提供 `dungeon_village_reference` 库及44个 CTest 测试程序，不加入产品主构建。

## 模块与依据

| 模块 | 职责与边界 | 规格 |
| --- | --- | --- |
| domain、simulation | R1事务、独占预约、单点自主模拟夹具；当前窗口不使用旧模拟 | [状态与建设](../rules/STATE_CONSTRUCTION.md)、[人物](../rules/CHARACTERS.md) |
| geometry、facility_economy、neighbourhood | 格坐标占地、经营推导、来源实例去重与道路魅力 | [设施](../rules/FACILITIES.md) |
| navigation、map_access | 加权寻路、完整设施绑定、距离场和到达身份 | [地图访问](../rules/MAP_ACCESS.md) |
| activity_choice、activity_candidates | 类别计划、候选生成/排序，票号由调用方注入 | [活动选择](../rules/ACTIVITY.md) |
| ranked_facility_choice、snapshot_facility_choice、regional_choice | 保留不同历史契约；重复计权、抽中项/目标格、六次区域回退 | [活动选择](../rules/ACTIVITY.md) |
| facility_departure | 普通设施分支的两级选择/回溯/绑定/初始朝向组合；不决定任务/救援/物体优先级、不执行运动或失败清理 | [首次活动](../rules/CHARACTERS.md#first-activity) |
| character_motion | 6.7世界单位位移、入口路点±40、矩形到达、向零截断和逻辑格进入；不收费/使用、不含表现投影 | [连续运动](../rules/CHARACTERS.md#continuous-motion) |
| facility_arrival、facility_exit、character_hp | 到达统计/现金候选、共享使用数/满足度请求、生命值目标与显示协议 | [设施使用](../rules/FACILITY_USE.md) |
| facility_use | 普通食物/旅店的占用请求、同轮等待扣减、旧计数170恢复与待处理退出；不提交完整退出或下一活动 | [使用计数](../rules/FACILITY_USE.md#use-timing) |
| facility_exit | 另提供完整绑定出口位置、普通活动0/属性/表情尾部顺序；不运行解释器或提交占用/属性 | [出口与下一活动](../rules/FACILITY_USE.md#exit-position) |
| weapon_choice | 武器/防具/饰品重选计数、不同rank窗口/空当前回退及票号；不装配/收费 | [武器与装备选择](../rules/FACILITY_USE.md#weapon-choice) |
| facility_events、facility_items | 实例事件门槛、定义共享改良与库存候选 | [事件与道具](../rules/FACILITY_EFFECTS.md) |
| accounting | 即时金币、周期费用、报表快照、延迟点数与幂等身份 | [周期账本](../rules/ACCOUNTING.md) |
| actor_ai、ai_perception | G/e/F/K/L、人物/怪物等待、附近抢占、救援/回复目标与出发前置 | [AI感知](../rules/ai/PERCEPTION.md)、[生命周期](../rules/ai/LIFECYCLE.md) |
| combat_ai | 职业/武器策略、九格评分、物理/魔法候选与两侧影响场 | [战斗](../rules/ai/COMBAT.md) |
| combat_execution | 连攻/miss准备、动作窗口、怪物攻击位置、弹道/碰撞、延迟命中及HP/有序死亡请求 | [控制时间线](../rules/ai/CONTROL.md#combat-execution) |
| object_ai | 掉落选择、地上物体抛出/等待/授予/删除、H平局和state11完整拾取队列 | [地上物体](../rules/ai/LIFECYCLE.md#地上物体与拾取链) |
| encounter_ai | 组时点/清理/姿态、普通怪物数量及定义解锁抽取 | [遭遇](../rules/ai/ENCOUNTERS.md) |
| encounter_lifecycle | 完整事件分支/任务生成、胜利与页面有序请求、定义共享近期统计和延迟成长；不提交全局现金或职业升级 | [事件收尾](../rules/ai/ENCOUNTERS.md#完整事件候选与延迟成长) |
| actor_lifecycle | 计数状态、救援绑定/修复/跟随/释放、普通旅店双人原子到达、r清理候选 | [生命周期](../rules/ai/LIFECYCLE.md) |
| actor_control | 全34码校验、本地前缀、失败8、状态setter、漫游、装备显示/提交分离；全局副作用由所有者应用 | [控制解释器](../rules/ai/CONTROL.md) |
| ai_schedule | 实时名单两遍、反向/正向删除、同轮追加、删除前设施释放；消费者只准备私有聚合副本，不是通用事件总线 | [聚合调度](../rules/ai/LIFECYCLE.md#聚合遍历与同轮新建) |
| actor_effects | cd/ce逻辑时间线、删除跳过、表情票号/抑制、动作/状态计数及命中标签过期；不绘图或直接播放声音 | [显示与延迟效果](../rules/ai/CONTROL.md#显示与延迟效果的实际推进) |
| human_growth | 定义共享六属性/装备/法术重算、九步奖励成长、多级/满级及最后一级提示/职业解锁请求；不改变人物当前HP或代替全局页面提交 | [定义成长](../rules/ai/CONTROL.md#人物定义重算与职业成长) |
| facility_service | 装备/携物价格前置、全部设施使用队列、完整退出前部/普通/装备/住宅尾部；救援哨兵必须交双人事务 | [设施事务](../rules/FACILITY_USE.md#设施服务组合与首段私有所有者) |
| actor_housekeeping | c前缀、状态18事件身份守卫、重力/K/退回旧位置、d尾部清理/超时；保留旧格与新投影读取点 | [共用更新尾部](../rules/ai/LIFECYCLE.md#共用更新前缀与尾部) |
| battle_commit | HP/状态、双方救援引用、定义共享统计、全局倒地/击杀和掉落ID一次提交；保留尸体重复命中 | [战斗提交](../rules/ai/COMBAT.md#跨所有者战斗提交) |
| object_commit | 物体/拾取人物原子提交、目录库存或装备解锁、商店通知、事件151/216及直接授予110差异 | [拾物提交](../rules/ai/LIFECYCLE.md#拾物与目录实际提交) |
| ai_rewards | 尸体删除/引用保留、战斗组实际提交、任务生成/胜利、延迟成长与全局职业开放；地图/页面/表情保持显式请求 | [奖励所有者](../rules/ai/ENCOUNTERS.md#死亡战斗组与奖励所有者组合) |
| encounter_creation | 普通事件地图/任务/重叠守卫、数量/解锁/介绍/实际生成及整批回滚；不补任务配额 | [普通创建](../rules/ai/ENCOUNTERS.md#普通遭遇创建事务) |
| rescue_commit | 共用人物所有者、追踪/双向绑定/返程/递归双人到达、定义共享B2、旅店使用退出；复杂分支明确交接 | [普通旅店世界事务](../rules/ai/PERCEPTION.md#普通旅店世界事务) |
| combat_commit | 状态1策略→组加入/实际九方向移动/攻击及法术队列，14–17与同次本地解释器续行；当前动作和武器独立 | [策略世界提交](../rules/ai/COMBAT.md#状态1策略的世界提交) |
| world_perception | 当前地图K/e/F、旧缓存城内外/低血量、引用修复/抢占、状态18、全局场与物理投影、实际d执行前段；不提前刷新s/t/ak | [共同世界感知](../rules/ai/PERCEPTION.md#共同世界感知与缓存)、[执行前段](../rules/ai/LIFECYCLE.md#实际执行前段与解释器续行) |
| world_encounters | 任务F创建/配额/下一次绑定、当前地图候选重建、事件bit2刷新与原时点场快照；声音/提示保持请求 | [任务与场事务](../rules/ai/ENCOUNTERS.md#任务创建与影响场地图事务) |

`prepare_*` 纯函数返回候选值，调用方负责跨域原子提交及事件去重。
`GlobalState` 是早期安全夹具，不能与当前原型聚合或原作初值混用；R1净额结算不能与即时现金账本叠加。
头文件注释维护输入、所有权、失败与返回语义，非直观计权/时点在实现处补注释；不逐行描述赋值。

## 构建与检查

从仓库根目录执行：

```sh
cmake -S research/dungeon_village_1/example -B research/dungeon_village_1/work/example-debug-llvm -DCMAKE_BUILD_TYPE=Debug
cmake --build research/dungeon_village_1/work/example-debug-llvm --parallel 2
ctest --test-dir research/dungeon_village_1/work/example-debug-llvm --output-on-failure
```

Release使用独立目录。本机显式工具链路径见 [环境基线](../verification/BASELINE.md)，
本轮结果见 [当前验证](../VERIFICATION.md)。测试覆盖严格错误、边界、重放、随机差分及组合链，
不替代固定 APK 动态认证。
