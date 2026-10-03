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


## 感知、前置优先级与控制

本批消费研究af85eb1的actor_ai/ai_perception及b532d2a的actor_control维护实现。
接口/实现仍在include/ark/people与src/people，标准C++，不持有Game或raylib对象。

| 文件 | 职责 |
| --- | --- |
| actor_id.hpp | 非零强类型AI快照身份；原UID0有效，映射由聚合所有者负责，不能把原UID直接当空值 |
| actor_ai.hpp / actor_ai.cpp | G战斗资格、名单敌人选择、人物状态5与怪物状态17的局部决策 |
| ai_perception.hpp / ai_perception.cpp | F事件资格、K局部区域、救援/拾物抢占、回复目标、L生成探针、活动出发优先级 |
| actor_control.hpp / actor_control.cpp | 34码最小格式、本地FIFO前缀、状态setter、失败分支、漫游和装备尾部/提交候选 |
| decision.hpp / decision.cpp | 关联同一距离场/候选顺序与身份后，先上层优先级、再普通设施出发；特殊目标只返回交接请求 |

典型调用：collect_activity_candidates → prepare_decision → 所有者处理特殊目标或普通FacilityDeparture。
prepare_decision要求优先级和普通输入的活动/flags一致，候选坐标/成本/绑定匹配距离场，保持重复项与顺序。
nearest_down/object/encounter、任务有效性与别人目标仍由调用方基于世界解析，当前没有默认填空的Game适配。
ordinary仅表示当前输入没有优先目标，不是证明了全局首访前置；特殊目标也不是已成功寻路/到达。
状态5的活动/状态请求要先落实再查生成探针，不沿用旧状态5概率；F忽略1024而G检查，禁止合并资格。

控制前缀不运行完整解释器：复杂命令和失败8返回delegated并保留队首，所有者之后移除/提交，不能当原作未移除的事实。
成功8直接写A=0并早停，保留属性/装备尾部；c(state)的重置与此不同，也不等于b()基线恢复。
失败8纯规则区分旧1024与新置1024；r清理已提取状态/标志/120等待及引用释放候选，实际占用/引用提交和完整离场仍由所有者实现。
漫游返回世界坐标运动和队列追加候选，票号顺序含恒真分支抽样；不是加权格寻路。
装备27/29只显示，28/30才产生装备提交候选；先活动0、再等待/显示/装备。调用方仍须验证目录ID、重算定义并原子提交。

维护控制扩展保持在同一模块（约300行实现），不预建装备/战斗空子系统；后续实际聚合出现时按所有权拆分。
参考：[AI专项](../../research/dungeon_village_1/rules/ai/README.md)、
[感知](../../research/dungeon_village_1/rules/ai/PERCEPTION.md)、[控制](../../research/dungeon_village_1/rules/ai/CONTROL.md)。
本批不启用默认全局调度、使用/退出事务或真实任务/战斗；条件组合测试与正常窗口行为分别记录。
