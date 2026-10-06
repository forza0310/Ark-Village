# 当前世界手动档

`world_save_codec.cpp` 用 `world_save_fields.hpp` 显式编码唯一 Owner 的耐久字段；
`world_save_restore.cpp` 校验候选并恢复主场景。规则/研究源保持冻结，文件不是原 APK 档。
正常入口仅允许稳定主场景，征集、问答、授勋、成果与未结束月报不能丢页保存。
编辑退出后源保留的工具编号属于瞬态，不单独阻止主场景保存；活动编辑场景、
未清选区和待移动实例仍拒绝，正常页面退休完成后才允许捕获。
存档库跟随世界运行时的链接类型；共享核心配置使用同一个规则单例，不混入独立静态副本。

当前协议为 `ARKSAVE1`、u32 schema 2、带长度的数据集身份与村名/日期/资金摘要、
u32 负载长度、负载、u64 FNV-1a 校验。全部整数小端；领域 `int` 为32位、
稳定身份为64位，路线/续体的 `size_t` 显式转换为u64并在读取时核对宿主范围；
浮点为 IEEE-754 binary32，布尔仅允许0/1。容器长度u32，拒绝重复键及尾字节。
文件与动态解码分配各最多64 MiB，动态条目累计最多100万。校验用于发现损坏，
候选还必须通过领域引用/占地/范围验证。数据集身份来自冻结 `SOURCES.json.snapshot_sha256`。
当前冻结2b479f6，身份为`92dfab7c3c640a939ce68bd5741d1e92fd0630c59bfaf5d099e8e0e17ac6301c`；
旧schema或数据集明确拒绝，不迁移旧档。魔数保持文件族身份，字段版本单独登记。

## 原25分区与当前字段

下表是来源覆盖对应，不是沿用原二进制分区编号或承诺原加载器全部行为。
没有迁入当前 Owner 的原字段不造槽；静态目录从固定规则取得，部分已有值缓存保留其更新时间。

| 原分区 | 当前编码字段或边界 |
| --- | --- |
| 0 延迟脚本 | `scripts.continuations/context/event_calls`，下一指令、等待、替换及参数；即时执行栈/页面不保存 |
| 1 饰品 | `catalog[(3,id)]` 的开放/提示/免费购买等共享状态 |
| 2 防具 | `catalog[(2,id)]` 同上 |
| 3 村办定义 | `scripts.activities/activity_flags/activity_counts`：开放、季度标志及累计举办次数 |
| 4 人物定义 | `ai.growth/shop_humans/human_*`：职业/级别/属性/装备/住宅/满意度/消费/贡献/改名及延迟成长；`human_activity_previous`保留最后活动前的共享维护值 |
| 5 道具 | `items/catalog[(0,id)]/shop_item_stock/item_commerce_read`：持有库存、开放、补货库存、提示及原g.B阅读状态 |
| 6 职业 | `scripts.professions/ai.professions` 的共享开放状态及当前维护规则值 |
| 7 未闭合原目录 | 当前 Owner 没有单独维护该目录的动态p/r；不伪造原槽或认证原档兼容 |
| 8 怪物定义 | `ai.monster_growth/battle.monsters`：共享成长、累计、开放和提示 |
| 9 人气定义 | `popularity_rewards`，原序及已维护状态；程序身份仍受固定数据集约束 |
| 10 任务定义 | `task_progress.definitions` 的完成/开放进度及当前任务目录维护状态 |
| 11 城镇称号 | 当前 `rank/rank_history/rank_met/rank_values`；没有独立维护的原目录p/r不造槽 |
| 12 设施定义 | `facility_uses/scripts.facilities/facility_definitions/facility_presence`、建设资格次数、目录开放/提示及`facility_commerce_read`原o.O阅读状态；资格次数不表示免造价 |
| 13 武器 | `catalog[(1,id)]` 的开放/提示/免费购买等共享状态 |
| 14 摄像 | 当前维护的 `camera/previous_camera/camera_velocity`；仅现有二维值，跟随引用不保存 |
| 15 实时人物 | `battle.actors/retired_actors`、原序名单、完整FIFO/HP/计数/坐标/稳定引用；routes/shop/dungeon/metadata 的动作事实与时点缓存 |
| 16 实时怪物 | 同一实体协议、怪物原序名单及可达退休对象；原UID与维护稳定ID独立 |
| 17 世界物体 | `battle.objects/world.object_order/next_object_id`，位置/速度/阶段/计数/奖励定义 |
| 18 遭遇 | `encounters/retired_encounters/encounter_order`，原ID、阶段、计时、双方原序名单、影响场时点缓存 |
| 19 投射物 | `projectiles/projectile_order/next_projectile_id`，来源/目标、运动、接触和伤害计数 |
| 20 任务实例 | `tasks/task_order/task_original_ids/sites/task`，实例与设施/遭遇绑定、原ID、阶段所依赖的设施进度 |
| 21 全局 | 活动任务/参与者、资金、日期六字段、月账/报表/统计、点数/人气/勋章/称号、`legacy_*`、延迟人气、维护allocator和系统解锁数据 |
| 22 设施位置/朝向 | `facilities[].placement`，定义、稳定实例身份、锚点、形状和朝向 |
| 23 地图 | `map/surface/base_variants/map_flags/road_patches/fence_level`；候选恢复重建派生表现/邻接缓存 |
| 24 完整设施 | `facilities/dungeon_facilities/facility_details/facility_*` 的占用、施工、销售、月/年历史、原实例ID/同定义序号及`facility_item_confirmations`原m.y[0]计数 |

实例 `facility_details.notices`（原m.p）保留为维护效果历史；原写器包含该项，
原读后完整保留尚未闭合，因此不宣称精确原加载演出。邻接和商店中的重复通知投影不另写。
原调度时点缓存如人物逻辑格/half-cell/low-HP、cached view、遭遇影响场不能随意重算。

`items`与`catalog[(0,id)]`必须完整一致（flags、status、unlock_counter、newly_unlocked、
inventory、free_purchases），读取分歧直接拒绝，不猜哪份更新。`shop_item_stock`的p/q/r
是补货调用点的旧投影，合法滞后于当前定义；A商会库存与z持有量本就不同。
新增共享表按固定目录检查完整身份，活动/设施道具累计数拒绝负值和溢出边界。

退休设施的保留引用仅接受已分配身份、仍有原实例记录、且由已离开活动名单的历史任务明确引用。
不会仅凭`id < next_facility_identity`补造实例或接受任意悬空目标。编辑撤除/移动若尚有
不能证明可恢复的旧目标引用，需要等真实消费者完成清理；未完成的编辑锚点/移动选择不开放保存。

不保存共同随机、规则指针、页面及局部选择/父引用/答案、活动/设施道具/商会页缓存、
最近设施道具反应值、编辑锚点/移动选择、held输入、全局通知/浮标/声音、
人物cd/ce、审计检查点及逐笔 `CashEntry`。账本用保存余额构造，月账累计单独恢复，
不得重放费用以重建审计历史。同进程恢复使用当前随机流，确定性测试显式控制随机输入。
自动跨周档待轮内恢复合同闭合后接入，不能把当前手动档当作该检查点消费者。
