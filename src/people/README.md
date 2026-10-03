# people

公开接口`include/ark/people/adventurer.hpp`区分定义ID与场景UID，保留首访职业、属性、装备和HP。
first_visit按已证初值创建角色，标志为2|8192，位置为出生格中心，pending_activity保留0；出生点由app选择，UI不能注入属性。
人物先创建再触发对话；住宅入住、任务征集与使用设施的到达均不是这里的招募。
motion.hpp/cpp独立实现6.7单位运动、入口±40偏移、向零取整和移动前按逻辑格检查进入。
plan_travel/advance_travel仅组合显式目标与路径；返回候选和一次性进入信号，不预约、收费、选目标或退出。
过期身份/被阻断的路径安全停止是产品保护策略；自动重规划未实现。默认人物仍停在出生格。
依赖world的地图/连续坐标与facilities的实例身份；app拥有可变人物和访问计数，本包只返回纯候选。
完整AI、后续访客/成长待研究，不沿用夜骑士演示。
依据：[首名人物与加入顺序](../../research/dungeon_village_1/rules/STARTUP.md)、[普通运动](../../research/dungeon_village_1/example/src/character_motion.cpp)。


## 普通设施决策

公开接口均位于include/ark/people，四个源文件各负责一个可独立验证的步骤：

| 接口 / 实现 | 输入与输出 |
| --- | --- |
| activity_candidates.hpp / activity_candidates.cpp | 同一距离场、逻辑定义、实例阶段、事件、上次访问身份→有序出现项与类别计数 |
| activity_choice.hpp / activity_choice.cpp | 活动0/2/6、六槽访问计数、标志、类别计数→类别权重/强制类别；独立票号选择器 |
| facility_choice.hpp / facility_choice.cpp | 候选快照、类别1/2/6/8、票号→抽中活动项/完整项/最终目标三套下标 |
| departure.hpp / departure.cpp | 同一距离场和候选快照、两级票号→完整目标身份、路径、出发方向，或明确错误 |

调用顺序：world::search → collect_activity_candidates → prepare_facility_departure。
返回的非空route可连同当前连续位置交给已有Travel/advance_travel；出发规划本身不改变世界位置，不应重新吸附到格中心。
空路径只表示起终点重合，由上层处理活动协议，不能据此制造进入/占用或收费。
调用方须先证明上层任务/救援/物体优先级已进入普通设施分支；当前Game默认更新不调用这些接口。
不内置RNG，不重抽失败目标；两级票号需要调用方准备，强制类别绕开类别抽样。
类别3/4/-1出发由其他策略负责，当前返回unsupported_category；类别4计数抽选只作为独立规则提供，不能冒充其完整出发/退出流程。

容易误用的契约：

- 定义魅力参与选择，不能用实例邻接后的魅力代替。类别权重不乘候选数量。
- 候选是格/事件出现项，同一实例可重复计权；事件可能无路径成本，仍保留到回溯时拒绝。
- 交换排序能间接反转同成本项，不能替换成stable_sort；寻路前驱平局仍保留维护C++差异。
- 普通扫描排除阶段0、保留阶段2；只有无实例或阶段1增加类别计数，普通加权还要求有实例且阶段1。
- 起点有实例时才过滤上次访问身份；事件追加后只删除首个起点项。城镇内区严格排除边界。
- 快照不能跨地图/设施阶段/定义状态修改复用；出发校验距离场、目标成本和绑定，调用方负责一致快照的生命周期。
- 无候选、不支持、输入错误、不可达和身份失效分别报告；失败无部分目标，不执行原作失败清理或固定间隔重试。

依据：[ACTIVITY](../../research/dungeon_village_1/rules/ACTIVITY.md)、
[首次活动前置](../../research/dungeon_village_1/rules/CHARACTERS.md#first-activity)；
维护实现为example/src下activity_choice、activity_candidates、snapshot_facility_choice和facility_departure。
产品单独编译实现和测试，不链接research目标，不读取APK/研究运行目录。
