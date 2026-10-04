# 人物与怪物AI专项

日期：2026-10-04。用户授权持续自主研究、文档与独立C++同步推进；仅修改research。
固定输入/行号/警告见[证据清单](../../EVIDENCE.md)，本批以`730e7ee`成长检查点为基线继续。
本目录维护长篇AI专项，既有[活动](../ACTIVITY.md)、[运动](../CHARACTERS.md#continuous-motion)
与[设施使用](../FACILITY_USE.md)继续作为唯一对应规格，不复制其正文。

## 范围与完成条件

人物/怪物共用`c.b`，不是两套互不相关AI。需要覆盖正常生活、任务/野外、战斗、救援/携物、
死亡/恢复/清理、怪物生成/漫游/离场、解释器与全局调度，以及存取后的引用恢复。
源码事实、局部控制流推断、安全契约、原型策略和夹具分别记录；截图版本不覆盖固定APK规则。

| 专题 | 验收要求 | 当前状态 |
| --- | --- | --- |
| 共享状态/调度 | 21状态、c()/d()/控制队列及B/M/L先后、清理与删除返回值 | [总览](LIFECYCLE.md)含两遍实时名单/同轮生成与死亡奖励/拾物组合；完整逐tick所有者仍待完成 |
| 感知/资格 | 移动区域、城内外、敌人/物体/救援候选与平局 | G/e/F/K/L与救援/回复/抢占已有[规则与回归](PERCEPTION.md)；物体实例/目录事务已补，世界感知重查串接仍缺 |
| 人物日常 | 全部活动参数与任务/救援/物体优先级、设施/区域/住所/出口 | o前置、完整使用/退出安排已维护；真实两出生点首段自主生活已组合；复杂失败/住宅/特殊入口世界串接仍缺 |
| 战斗策略 | 近攻/远攻/魔法/回复、职业/武器条件、目标与移动评价 | 策略/评分/伤害/弹道、HP/救援/统计/掉落原子提交及同轮两遍通过；完整战斗控制/投射世界串接仍缺 |
| 怪物生命周期 | 定义初值、事件产生、模式0–4、漫游、登场/死亡/撤退 | 任务实例创建、死亡/增长/现金、组引用保留与奖励成长已组合；普通事件创建完整所有者仍缺；T2/3/4只见恢复入口 |
| 救援/道具 | 条件、接近/拾取/双向引用、带回与释放 | 拾物/库存/装备解锁及救援绑定/修复/跟随/旅店双人规则已维护；完整往返旅店所有者仍缺 |
| 控制解释器 | 每个实际opcode、等待/早停/异常、动作与显示队列分离 | [34码消费表/前缀/setter/漫游/装备及显示随机](CONTROL.md)已有维护规则；完整全局副作用仍待完成 |
| 聚合与原型 | 已证规则独立C++、所有权/原子提交、错误/组合回归 | 首段生活、战斗/组、物体和成长私有所有者已补；默认AI与全世界串接仍保护，不能称全量完成 |

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
| [ai_schedule.hpp](../../example/include/dungeon_village_reference/ai_schedule.hpp) | 实时名单遍历、两遍更新/删除跳过/同轮追加、设施释放顺序 | 更新资格/主场景前置、世界与统计的完整所有者及真实新局默认AI |
| [actor_effects.hpp](../../example/include/dungeon_village_reference/actor_effects.hpp) | cd/ce顺序与时长、表情概率/抑制/票号、i/l/B/aw/aq计数 | 图像/声音真实播放、平台文字选择及全部表现渲染 |
| [human_growth.hpp](../../example/include/dungeon_village_reference/human_growth.hpp) | 逐职业六属性、当前职业/legacy_u倍率、装备/法术、九步成长与有序解锁请求 | 全局页面/定义实际提交，不把成长显示当作HP恢复 |
| [facility_service.hpp](../../example/include/dungeon_village_reference/facility_service.hpp) | 特殊价格/携物、全部使用计划、完整退出前部/尾部 | 住宅世界副作用、默认窗口使用 |
| [actor_housekeeping.hpp](../../example/include/dungeon_village_reference/actor_housekeeping.hpp) | c前缀/状态18、P/重力/K、d尾部/超时/删除请求 | 真实位置投影、设施释放与世界删除 |
| [battle_commit.hpp](../../example/include/dungeon_village_reference/battle_commit.hpp) | 双方HP/救援、定义/全局统计、事件/掉落一次提交 | 全部控制/投射循环与显示随机 |
| [object_commit.hpp](../../example/include/dungeon_village_reference/object_commit.hpp) | 拾物人物/物体事务、库存/装备解锁/商店通知/事件 | 原矩形帧查询、追物实际运动、默认全局目录 |
| [ai_rewards.hpp](../../example/include/dungeon_village_reference/ai_rewards.hpp) | 尸体/引用保留、组提交、任务生成/胜利、金币与延迟成长 | 普通事件创建组合、地图/页面/表情额外随机、绑定旅店r |
| [startup_ai.hpp](../../prototype/include/dungeon_village_prototype/startup_ai.hpp) | 真实首段自主选路→收费→使用→完整退出→下一活动/属性/武器 | 首个月界、玩家改图/复杂事件、默认窗口接通 |

AI相关规则与私有所有者只有标准C++17，不调用Java或窗口、不接管原版向量，返回候选由唯一所有者重校验并提交。
外部命令明确delegated，未知/截断明确错误，不静默补演示AI；原型默认保护不因纯规则通过而解除。
当前可复现检查数与六套结果见[验证](../../VERIFICATION.md)。

## 下一批闭合顺序

1. 普通事件创建：地图守卫/中心冲突→数量/解锁抽取→多个实例提交，与L生成探针和影响场刷新同一所有者组合。
2. 将完整战斗控制/投射/组/延迟效果、拾物/救援往返和旅店释放接入共同实时世界；不以独立事务数量代替全局串接。
3. 完整随机消费与显示请求：表情/变体/投射/装备票号按真实顺序供给，明确主场景资格与日期/装备A月度扣减边界。
4. 首段私有生活已有真实数据组合；上述世界依赖完成后再评估默认窗口AI。窗口/原版动态验收单列，不提前解除保护。

这些是仍未完成的研究/代码项，不是需要用户选方案的小问题；沿用无人值守授权继续核对，不把未知填成默认值。
