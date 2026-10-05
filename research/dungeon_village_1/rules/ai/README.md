# 人物与怪物AI专项

日期：2026-10-05。用户授权持续自主研究、文档与独立C++同步推进；仅修改research。
固定输入/行号/警告见[证据清单](../../EVIDENCE.md)，最新共同世界整线检查见[验证](../../VERIFICATION.md)。
本目录维护长篇AI专项，既有[活动](../ACTIVITY.md)、[运动](../CHARACTERS.md#continuous-motion)
与[设施使用](../FACILITY_USE.md)继续作为唯一对应规格，不复制其正文。

## 范围与完成条件

人物/怪物共用`c.b`，不是两套互不相关AI。需要覆盖正常生活、任务/野外、战斗、救援/携物、
死亡/恢复/清理、怪物生成/漫游/离场、解释器与全局调度，以及存取后的引用恢复。
源码事实、局部控制流推断、安全契约、原型策略和夹具分别记录；截图版本不覆盖固定APK规则。

| 专题 | 验收要求 | 当前状态 |
| --- | --- | --- |
| 共享状态/调度 | 21状态、c()/d()/控制队列及B/M/L先后、清理与删除返回值 | [共同所有者](WORLD_SCHEDULE.md)接真实前段/尾部与[全状态路由](LIFECYCLE.md#日常登场拾物与全状态路由)；真实新局跨12个月、同轮击杀掉落与晚失败回滚有执行验证 |
| 感知/资格 | 移动区域、城内外、敌人/物体/救援候选与平局 | G/e/F/K/L、当前地图/旧缓存、引用修复/抢占、状态18及物理重查已[组合](PERCEPTION.md#共同世界感知与缓存)，共同调度复用一次；旧o/bm另按frame刷新 |
| 人物日常 | 全部活动参数与任务/救援/物体优先级、设施/区域/住所/出口 | 休息/住宅/特殊入口、探索、商店及[出发/P路径到达](WORLD_DEPARTURE.md)接完整目录；自然新局设施收入、原实例离场与再次到访已观察 |
| 战斗策略 | 近攻/远攻/魔法/回复、职业/武器条件、目标与移动评价 | 策略→组/实际九方向移动/近攻与法术队列→14–17及同次本地续行、投射/延迟/尸体重击已组合；懒表情与131/217同步脚本已接，不以静态模块验收替代真实连续世界 |
| 怪物生命周期 | 定义初值、事件产生、模式0–4、漫游、登场/死亡/撤退 | 普通/任务创建、配额/地图/场快照、死亡/增长/组/引用与成长已组合；T2/3/4全源写入核对只见存取，未造正常生成入口 |
| 救援/道具 | 条件、接近/拾取/双向引用、带回与释放 | 拾物、真实优先级、普通旅店返程、P到达/类别8双人递归与全状态路由接共同Owner；自然长跑不代替双人救援条件组合 |
| 控制解释器 | 每个实际opcode、等待/早停/异常、动作与显示队列分离 | [全码自动路由](CONTROL_COMPOSITION.md#全码自动路由与当前随机接入)、共同Java48懒消费、同步脚本/表现已接；全状态/全码条件输入与自然可达分开验收 |
| 聚合与原型 | 已证规则独立C++、所有权/原子提交、错误/组合回归 | 真实新局唯一Owner/完整目录跨年，多种子/倍速有界检查；显式`--world`窗口实际移动和月报画面已验证；完整UI、OS输入和APK动态一致性分别登记 |

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
| [combat_commit.hpp](../../example/include/dungeon_village_reference/combat_commit.hpp) | 状态1策略→组/九向实际移动/攻击与法术队列，14–17及同次本地续行、当前e/J、投射/回复B/原子回滚 | 全部跨域解释器、表情额外随机与投影朝向 |
| [object_commit.hpp](../../example/include/dungeon_village_reference/object_commit.hpp) | 拾物人物/物体事务、库存/装备解锁/商店通知/事件 | 原矩形帧查询、追物实际运动、默认全局目录 |
| [ai_rewards.hpp](../../example/include/dungeon_village_reference/ai_rewards.hpp) | 尸体/可达引用、组提交、任务生成/胜利、金币/成长、当前名单投射碰撞与延迟重击 | 地图/页面/表情额外随机、绑定旅店r |
| [encounter_creation.hpp](../../example/include/dungeon_village_reference/encounter_creation.hpp) | L成功后地图/任务/中心守卫、原ID、数量、逐只解锁/介绍/实际生成及原子回滚 | 创建任务后的调用方配额、完整PRNG/地图刷新 |
| [rescue_commit.hpp](../../example/include/dungeon_village_reference/rescue_commit.hpp) | 同一人物所有者下重新I追踪/双向绑定/跟随/参数4返程/旅店两人记账与使用退出 | 类别8、返程被更高优先级抢占的后续执行、默认窗口及完整世界 |
| [world_perception.hpp](../../example/include/dungeon_village_reference/world_perception.hpp) | 当前地图K/e/F、旧缓存ak/ax、R修复/抢占、状态18、全局场/物理投影和实际d前段 | 全部状态/保留计数消费者、表现朝向、世界循环与外部引用根 |
| [world_encounters.hpp](../../example/include/dungeon_village_reference/world_encounters.hpp) | 任务F创建/配额/再次绑定、当前地图生成候选、bit2刷新及原请求时点快照 | 音乐/页面/额外表情随机、完整全局任务目录与逐tick世界 |
| [world_facilities.hpp](../../example/include/dungeon_village_reference/world_facilities.hpp) | 共同前段一次→携带表情→设施控制、休息/特殊入口、类别9退出、人物/怪物状态15 | 商店/类别5探索通用提交、全部跨域控制与完整世界 |
| [world_actor_tail.hpp](../../example/include/dungeon_village_reference/world_actor_tail.hpp) | 旧s的L/M→物理/K/新s→保留计数/r/删除请求、调度原时点释放/退休 | 原u/v朝向、任务/页面外部引用根及完整世界循环 |
| [dungeon_ai.hpp](../../example/include/dungeon_village_reference/dungeon_ai.hpp) | 类别5进入/长度、逆序队伍/挑战/错峰完成、阶段2任务有序请求 | 阶段2任务/UI/地图实际提交 |
| [world_dungeon.hpp](../../example/include/dungeon_village_reference/world_dungeon.hpp) | 当前O进入、真实地图撤退/q/HP、队伍目录/抛出奖励、原子落地续行及任务成功统计/探索日期/怪物开放/任务池阈值 | 地图恢复与任务/UI整段共同提交、完整事件脚本/实际模态结算时点 |
| [world_shop.hpp](../../example/include/dungeon_village_reference/world_shop.hpp) | 四店型到达/携物、装备收费前选择、满足度/人气队列、属性和28/30实际装配/共享重算 | 完整商店目录/随机流及默认窗口接通；cd由下列显示适配消费 |
| [world_control.hpp](../../example/include/dungeon_village_reference/world_control.hpp) | 唯一control的同次FIFO续行、hold/true区别、真实退出/r/战斗/商店组合与晚期回滚 | 不内置所有领域适配，不重复共同前段/尾部 |
| [world_schedule.hpp](../../example/include/dungeon_village_reference/world_schedule.hpp) | 唯一世界中的场/到访/队列/S、两遍实时名单、共同前段/尾部/释放、真实非人物消费者与外层原子候选 | 实际运行目录/所有者与表现输入；21状态另由既有routes消费 |
| [world_overlap.hpp](../../example/include/dungeon_village_reference/world_overlap.hpp) | 实际L的三段名单/随机pair、共享矩形别名、不对称几何和只写n.x/z | 不刷新s/t/u/ax、不推进状态/控制 |
| [world_departure.hpp](../../example/include/dungeon_village_reference/world_departure.hpp) | 全部活动/优先级、住宅/实际出口/票号、路线、front8真假返回/r，真实P/F/旧格身份/路点/普通到达与救援组合 | 完整商店装备/物体目录交接、全部状态与同一随机流；选择成功不是到达 |
| [world_wander.hpp](../../example/include/dungeon_village_reference/world_wander.hpp) | 10/12/13读取旧s和当前/退休db/S、真实邻格/抽号/队尾扩展 | 不提前移动/推进计数，不伪造缺失引用 |
| [world_misc_control.hpp](../../example/include/dungeon_village_reference/world_misc_control.hpp) | 实际c0..20、HP回复、共享定义离村、竖速和旧u投影音效 | 真实投影回调和使用方共享定义仍需显式提供 |
| [world_equipment_display.hpp](../../example/include/dungeon_village_reference/world_equipment_display.hpp) | 27/29实际cd追加、负年龄烟效、已发请求消费与时间线组合 | 不改装备/不再删控制/不提前计时；完整图片绘制另验 |
| [world_lifecycle.hpp](../../example/include/dungeon_village_reference/world_lifecycle.hpp) | 2/4/6/7/10/12/14/16/19真实共享c消费者、b与c区分、状态17缓存G/实际P→旧L500/r及共同20轮回归 | 不重跑共同前段/引用修复；其他状态明确交接，完整自动路由/随机/表现/窗口另验 |
| [world_daily.hpp](../../example/include/dungeon_village_reference/world_daily.hpp)、[world_actor_routes.hpp](../../example/include/dungeon_village_reference/world_actor_routes.hpp) | 日常L/F/P/登场/拾物与全21状态/34控制同一候选路由、真实队首输入及部分懒随机 | 完整运行目录/其他随机/世界与表现逐层接通 |
| [world_actor_schedule.hpp](../../example/include/dungeon_village_reference/world_actor_schedule.hpp) | 共同c/d前段、成长/携物表情、全状态/控制、原尾部/删除点的外层Owner组合 | 其他域必须实际提供，不允许默认成功消费者 |
| [world_exploration.hpp](../../example/include/dungeon_village_reference/world_exploration.hpp) | 真实阶段2页栈/奖励/任务/地图/邻接/126/92/201续体共同提交 | 格/偏移表现请求未渲染；其他脚本域拒绝 |
| [world_scripts.hpp](../../example/include/dungeon_village_reference/world_scripts.hpp) | 固定事件、调用计数、实时续体与真实框架页栈的独立消费者 | 外层唯一目录/金融/选择根等须实际投影，不保持第二份事实 |
| [world_calendar.hpp](../../example/include/dungeon_village_reference/world_calendar.hpp)、[world_scene.hpp](../../example/include/dungeon_village_reference/world_scene.hpp) | 真实日期归一化/有序域、主场景保存轮数/实时分支/跳转和绘制门槛 | 实际月报/任务/页面/窗口全部组合另验 |
| [world_calendar_maintenance.hpp](../../example/include/dungeon_village_reference/world_calendar_maintenance.hpp) | 年统计/清理、月居民/库存/装备/设施/住宅G及请求 | 未维护stage拒绝，请求须同步真实消费者后提交 |
| [world_month_report.hpp](../../example/include/dungeon_village_reference/world_month_report.hpp) | 旧t1457费用/现金/z/v/K/L/M/O、70+70报告与关闭后点数 | 不封账、不当作独立模态页、不提前推进日期 |
| [world_calendar_tasks.hpp](../../example/include/dungeon_village_reference/world_calendar_tasks.hpp)、[world_task_creation.hpp](../../example/include/dungeon_village_reference/world_task_creation.hpp) | 特殊月份/等级提示/任务期限/生成/容量，实际定义/落点/奖励/挑战工厂 | 实际完整目录/持久请求和任务页面输入须外层提供 |
| [world_task_commands.hpp](../../example/include/dungeon_village_reference/world_task_commands.hpp) | 原费用、全目录征集随机/入场演出、追加报价、预测和96计数正式出发，取消/拒绝与同步消费者 | 不替人物寻路、进入任务或制造自然成功 |
| [world_task_display.hpp](../../example/include/dungeon_village_reference/world_task_display.hpp) | 99/100页40门槛、共享8×9表的19抽初始化/F更新，同一Java流 | 仅原演出状态，不创建实际战斗怪物或发奖励 |
| [world_task_deadline.hpp](../../example/include/dungeon_village_reference/world_task_deadline.hpp) | 页33选择/演出/关闭原引用与真正主场景返回，实时费用/月账本/中止有序消费 | 不把关页当作扣费，不替玩家默认继续；主动4/1另走[独立父页消费者](DUNGEONS.md#主动任务管理) |
| [world_facility_update.hpp](../../example/include/dungeon_village_reference/world_facility_update.hpp)、[world_residence.hpp](../../example/include/dungeon_village_reference/world_residence.hpp) | 设施前缀/施工/延迟人气及住宅现有人物奖励/成长/真实程序/页面 | 住宅不是新建人物；礼物页94插入不等于领取 |
| [world_popularity.hpp](../../example/include/dungeon_village_reference/world_popularity.hpp) | 原序奖励/显示delta/最大值/跨百重放/实际程序与新闻 | 页97须实际更新才清R，不能用任意关闭替代 |
| [world_nonactor_schedule.hpp](../../example/include/dungeon_village_reference/world_nonactor_schedule.hpp) | bo/bp/bn/L实际路由、统一懒随机、事件原时点同步与typed全球/I写回 | 未支持请求明确失败，实际绘制/声音另验 |
| [world_runtime.hpp](../../example/include/dungeon_village_reference/world_runtime.hpp) | 主场景→真实脚本→月报→全状态/控制/非人物→跨月真实任务工厂，同一私有Owner晚期整体回滚 | 空村1600轮是旧隔离夹具，真实非空新局的当前检查见验证入口 |
| [startup_world_runtime.hpp](../../prototype/include/dungeon_village_prototype/startup_world_runtime.hpp) | 具体唯一Owner、完整原表、真实到访/脚本/队首facts/任务/金融/月报与frame渲染缓存 | seed/视口/字体/内存检查点是明确研究输入；全UI和正常文件存取不由此认证 |
| [startup_ai.hpp](../../prototype/include/dungeon_village_prototype/startup_ai.hpp) | 真实首段自主选路→收费→使用→完整退出→下一活动/属性/武器 | 首个月界、玩家改图/复杂事件、默认窗口接通 |

AI相关规则与私有所有者只有标准C++17，不调用Java或窗口、不接管原版向量，返回候选由唯一所有者重校验并提交。
外部命令明确delegated，未知/截断明确错误，不静默补演示AI；原型默认保护不因纯规则通过而解除。
当前可复现检查数与六套结果见[验证](../../VERIFICATION.md)。

## 整线覆盖与后续边界

本批沿用户指定顺序，将六段接入同一真实新局所有者，当前源与验收入口集中在[验证](../../VERIFICATION.md)。
登场90见状态同次同步、击杀掉落同轮名单、任务成功的怪物p/r跨层写回和旧渲染缓存分别有回归；
任务目录/成果页外部根由真正页栈保存，不将AI根子集当成整个Java堆。

上轮两月seed1路径任务0；本轮补全七月全局入口、任务镜头/定时等待、rank条件/新怪物介绍与年度终止，
同一真实新局已跨12个月、自然生成3项任务；另一种子双轮跨6个月、自然生成2项任务。
第三个seed0双轮跨24个月/两次年度返回，自然任务3；三条实际指标和历史398项见[验证](../../VERIFICATION.md)。
年度终止仍是上述长跑的明确测试玩家选择，不清未用勋章、不自动授予；raw88完整奖励现已补接，
条件组合及简化窗口独立验收，原授勋皮肤/动画/OS输入仍不由该长跑认证。
任务工厂、探索地图/阶段2、战斗胜利和成果页仍有真实目录的显式条件组合，
本批另接玩家任务页/期限/成果返回，同一真实新局两条seed/速度路径都自然完成探索及战斗，
并等实际成果续体退休、最近成功之后新任务生成，真正打开新接受页再取消返回。
新增页11/83/97/59分别按原消息、取消、自动更新与人物解锁门槛处理，不给未知页通用关闭。
最新六套/十专项及简化队伍窗口见顶部当前批次，不能从长跑推导每个状态/任务自然到达，也不制造T2/3/4入口。
47ms为框架绘制路径门槛，不是每逻辑轮更新前的等待；两轮保存/跳过本轮/结束帧区别保留。

当前优先[可持续世界](WORLD_SCHEDULE.md#可持续世界推进)：跨年/多种子/倍速、真实停点与页面恢复，
玩家任务自然闭环已验，完整授予/raw88及主动管理现已补Owner消费者；当前结果见验证，
后续按真实停点推进剩余消费者、引用退役/资源规模；
不重新研究已交付路由。完整建设/任务选单皮肤、武器图层合成、
全局特效/音频播放、原图层满桶行为及正常文件存取仍单独规划。绘制期迷宫随机抖动也读同一流，
没有固定APK默认seed和渲染轨迹时，不宣称C++运行与原APK逐帧随机等价。
真实OS键鼠验收受macOS自动化权限阻挡，不绕过隐私授权；控制器或截图通过不能替代它。
