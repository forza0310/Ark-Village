# people

公开接口`include/ark/people/adventurer.hpp`区分定义ID与场景UID，保留首访职业、属性、装备和HP。
first_visit按已证初值创建角色，标志为2|8192，位置为出生格中心，pending_activity保留0；出生点由app选择，UI不能注入属性。
人物先创建再触发对话；住宅入住、任务征集与使用设施的到达均不是这里的招募。
motion.hpp/cpp独立实现6.7单位运动、入口±40偏移、向零取整和移动前按逻辑格检查进入。
plan_travel/advance_travel仅组合显式目标与路径；返回候选和一次性进入信号，不预约、收费、选目标或退出。
过期身份/被阻断的路径安全停止是产品保护策略；自动重规划未实现。默认人物仍停在出生格。
依赖world的地图/连续坐标与facilities的实例身份；app拥有可变人物和访问计数，本包只返回纯候选。
完整默认AI、后续访客仍待组合；定义成长已接纯规则，不沿用夜骑士演示。
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

## 效果、装备与定义成长

本批消费26e65c7/730e7ee已提交独立规则；实际Game接通依赖仍未完成。

| 文件 | 职责与依赖 |
| --- | --- |
| actor_effects.hpp/cpp | cd显示/ce延迟时间线、表情概率/变体票号、动作/状态计数与命中标签；无renderer/音频依赖 |
| weapon_choice.hpp/cpp | 武器、防具、饰品候选；原目录顺序、不同rank区间、重选计数与空当前回退，票号显式输入 |
| delayed_reward.hpp/cpp | 从encounter_lifecycle提取定义N/O九步奖励及合并；消费方负责XP提交，真实升级丢弃余量 |
| human_growth.hpp/cpp | 依赖效果和延迟奖励，六属性/装备/法术重算、职业经验/多级成长与有序事件/解锁请求 |

ce先逆序派发再推进cd；cd正序删除后继续加下标，本轮跳过紧邻后项。控制新加的显示在下一轮推进，不能与m控制队列共用时钟。
表情先消耗概率票号，概率通过再检查cd12/24抑制，未被抑制才需要变体票号；平台变体数量由调用方提供。
装备选择不收费或装配；调用现有actor_control准备退出尾部，27/29为显示，28/30才生成提交候选，随后按装备属性重算定义。
human_growth拥有的是定义共享状态，不是每个人物自己的HP；每实例调用可推进一次共享N/O，不按定义每世界轮次去重。
各职业贡献分别截断后累加，当前职业倍率和量化legacy_u分别取整/上限；战斗值最后加装备，没有自定下限0。
升级只生成请求：事件109→成长报告，掌握时再页70/职业解锁/页94/事件113/notice33，真实所有者另做幂等与原子提交。
tests/people_progression组合实际产品控制器、效果、成长、app调度；只使用显式条件夹具，不能作为真实新局职业/库存来源。
依据：[显示与延迟](../../research/dungeon_village_1/rules/ai/CONTROL.md#显示与延迟效果的实际推进)、
[定义重算与职业成长](../../research/dungeon_village_1/rules/ai/CONTROL.md#人物定义重算与职业成长)、
[装备选择](../../research/dungeon_village_1/rules/FACILITY_USE.md#weapon-choice)。


## HP协议与真实首段调用

hp.hpp/cpp消费研究character_hp：分离目标、显示、延迟HP及恢复请求，伤害/恢复先准备候选，显示推进独立验证。
app/initial_ai在c阶段读取旧B170并请求旅店恢复，在d阶段推进HP显示；到达/退出、定义属性累计与生命恢复不可混为一件事。
未改图初局真实选目标→使用→退出现已接入私有会话，详见[app](../app/README.md)；正常Game仍受默认AI保护。

## 战斗决策与共用更新

本批消费e8bd81c已提交规则及其依赖，标准C++，不链接research或读取研究运行目录。共同世界感知/遭遇/救援事务依赖尚未全部迁入，不解除默认AI保护或注入夹具敌人。

| 文件 | 职责与边界 |
| --- | --- |
| combat_ai.hpp / combat_strategy.cpp | 状态1职业/武器/怪物策略与九方向评分；显式策略/回复票号，返回组/运动/攻击/法术候选，不执行攻击 |
| combat_ai.hpp / combat_damage.cpp | 物理/魔法数值、增强与扰动票号、怪物定义成长；保留整数除法/float截断与两种增强顺序，不提交HP |
| combat_ai.hpp / combat_influence.cpp | 敌方25格累加、友方9格逐人截断、两侧场和固定地形掩码；资格读取上轮区域缓存，非寻路成本 |
| actor_housekeeping.hpp / actor_housekeeping.cpp | c前缀/状态18事件资格、旧P/重力/K回退及d尾部计数/有序清理删除请求；不投影、不自行释放设施或删除名单 |

actor_ai允许战斗组里一致的重复快照保留原顺序，矛盾或普通重复身份仍拒绝；名单ActorId非零与原事件ID0分开。
ai_perception的F使用task_original_id匹配原事件号，绑定当前encounters中的对象ID；未提供原号的既有条件输入沿用维护兼容分支，真实世界适配需显式提供原号。
actor_control新增独立prepare_actor_baseline_restore：b保留备用动作计数i与外部B/C/D，清16/队列/动作/l，只5/17的已证分支追加漫游。不是c(D)，也不是会清i的opcode3。
actor_housekeeping复用唯一prepare_actor_cleanup，不建立第二份r规则；清理请求先于后续状态/标志/事件守卫，删除交由app调度先释放设施再移出名单。
tests/ai_update使用明确条件夹具组合策略/评分→6.7运动→区域失败回退→旧区域留存，并验证调度晚失败不提交、删除前释放；不是原版新局或全世界时序认证。
依据：[战斗](../../research/dungeon_village_1/rules/ai/COMBAT.md)、[共用前后段](../../research/dungeon_village_1/rules/ai/LIFECYCLE.md#共用更新前缀与尾部)、[共同世界感知](../../research/dungeon_village_1/rules/ai/PERCEPTION.md#共同世界感知与缓存)。
