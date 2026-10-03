# 人物与怪物AI专项

日期：2026-10-04。用户授权持续自主研究、文档与独立C++同步推进；仅修改research。
固定输入/行号/警告见[证据清单](../../EVIDENCE.md)，上一检查点为`06e6563`。
本目录维护长篇AI专项，既有[活动](../ACTIVITY.md)、[运动](../CHARACTERS.md#continuous-motion)
与[设施使用](../FACILITY_USE.md)继续作为唯一对应规格，不复制其正文。

## 范围与完成条件

人物/怪物共用`c.b`，不是两套互不相关AI。需要覆盖正常生活、任务/野外、战斗、救援/携物、
死亡/恢复/清理、怪物生成/漫游/离场、解释器与全局调度，以及存取后的引用恢复。
源码事实、局部控制流推断、安全契约、原型策略和夹具分别记录；截图版本不覆盖固定APK规则。

| 专题 | 验收要求 | 当前状态 |
| --- | --- | --- |
| 共享状态/调度 | 21状态、c()/d()/控制队列及B/M/L先后、清理与删除返回值 | [总览](LIFECYCLE.md)及计数状态/救援/引用恢复已有维护规则；完整入场/两遍聚合待组合 |
| 感知/资格 | 移动区域、城内外、敌人/物体/救援候选与平局 | G/e/F/K/L与救援/回复/抢占已有[规则与回归](PERCEPTION.md)；完整物体生命周期与实例提交待补 |
| 人物日常 | 全部活动参数与任务/救援/物体优先级、设施/区域/住所/出口 | o前置优先级已低层交叉并维护；既有设施规则复用；整体失败/首访组合仍缺 |
| 战斗策略 | 近攻/远攻/魔法/回复、职业/武器条件、目标与移动评价 | [策略/评分/物理魔法/影响场](COMBAT.md)、[动作/弹道/HP及死亡请求](CONTROL.md#combat-execution)已有规则；全局提交仍待组合 |
| 怪物生命周期 | 定义初值、事件产生、模式0–4、漫游、登场/死亡/撤退 | [组/数量/定义/完整事件收尾](ENCOUNTERS.md)及计数状态已有回归；实时T写点已核对，T2/3/4仅见恢复入口；全局提交待组合 |
| 救援/道具 | 条件、接近/拾取/双向引用、带回与释放 | 拾物/物体时序及救援绑定/修复/跟随/旅店双人事务已有规则；全局库存和实例提交待组合 |
| 控制解释器 | 每个实际opcode、等待/早停/异常、动作与显示队列分离 | [全部34码消费表](CONTROL.md)与本地前缀已维护；复杂码/清队列再入/副作用仍待完成 |
| 聚合与原型 | 已证规则独立C++、所有权/原子提交、错误/组合回归 | 九个模块分批同步；真实默认AI仍保护，不能称全量完成 |

“全部完成”要求上述专题有证据覆盖和可执行规则/组合验收，不能用一份状态表、局部函数或静止原型充当整体完成。
每批修正有效断言必须有来源；小错误自行处理。只有来源冲突/关键契约歧义才暂停相应分支。
阶段结果记录[当前验证](../../VERIFICATION.md)，本地小步提交，不推送或夹带产品改动。

## 本批接口

| 接口 | 已执行的职责 | 不包含 |
| --- | --- | --- |
| [actor_ai.hpp](../../example/include/dungeon_village_reference/actor_ai.hpp) | G、指定名单e、状态5/17决策 | 攻击/组/活动的真实提交 |
| [ai_perception.hpp](../../example/include/dungeon_village_reference/ai_perception.hpp) | F/K/L、救援/回复目标、抢占、o前置 | 真创建事件、运动碰撞、设施消费 |
| [combat_ai.hpp](../../example/include/dungeon_village_reference/combat_ai.hpp) | 职业/武器策略、九格评分、物理/魔法候选、两侧影响场 | 当前成长重算、连攻、miss与投射提交 |
| [encounter_ai.hpp](../../example/include/dungeon_village_reference/encounter_ai.hpp) | 组半轮/整轮、成员清理、姿态、普通数量、定义解锁抽取 | 完整事件k0/k3与任务/金币收尾 |
| [encounter_lifecycle.hpp](../../example/include/dungeon_village_reference/encounter_lifecycle.hpp) | k0/1/2/3分支、任务生成/胜利有序请求、共享J/K与N/O延迟成长 | 真实创建实例、页面/任务/职业成长提交及完整PRNG序列 |
| [actor_lifecycle.hpp](../../example/include/dungeon_village_reference/actor_lifecycle.hpp) | 计数状态有序请求、双向救援绑定、r清理 | 所有状态setter、递归释放/副作用 |
| [actor_control.hpp](../../example/include/dungeon_village_reference/actor_control.hpp) | 全34码校验、本地前缀/成功8、失败8、状态setter、漫游10/12/13、装备尾部/提交 | 复杂命令全局执行、显示/延迟队列与聚合 |
| [combat_execution.hpp](../../example/include/dungeon_village_reference/combat_execution.hpp) | 连攻准备、14–17窗口、旧au、弹道/碰撞、命中HP与有序死亡请求 | 递归setter/真实统计提交、全局名单更新 |
| [object_ai.hpp](../../example/include/dungeon_village_reference/object_ai.hpp) | 掉落选择、物体构造/计数/奖励/H/拾取控制队列 | 全局库存和发布原型实例提交 |

九个AI模块只有标准C++17，不调用Java或窗口、不接管原版向量，返回候选由唯一所有者重校验并提交。
外部命令明确delegated，未知/截断明确错误，不静默补演示AI；原型默认保护不因纯规则通过而解除。
当前可复现检查数与六套结果见[验证](../../VERIFICATION.md)。

## 下一批闭合顺序

1. 已补控制14/17窗口/移动、miss/连攻、投射碰撞及命中候选，下一项接全局两遍/伤害提交；已有局部策略/数值组合。
2. 物体11/12、救援13/16/旅店事务、装备显示/提交分离及失败8已补，继续退出所有前部事务与防具/饰品选择。
3. T静态写点、引用恢复与普通/任务完整事件分支已补，继续全部更新顺序逐tick组合。
4. 首访→首个自主目标→到达/使用→退出/下一活动的真实数据组合后，再评估解除研究原型默认AI保护。

这些是仍未完成的研究/代码项，不是需要用户选方案的小问题；沿用无人值守授权继续核对，不把未知填成默认值。
