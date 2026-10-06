# 当前世界手动档

`world_save_codec.cpp` 用 `world_save_fields.hpp` 显式编码唯一 Owner 的耐久字段；
`world_save_restore.cpp` 校验候选并恢复主场景。规则/研究源保持冻结，文件不是原 APK 档。
正常入口仅允许稳定主场景，征集、问答、授勋、成果与未结束月报不能丢页保存。

协议为 `ARKSAVE1`、u32 schema、带长度的数据集身份与村名/日期/资金摘要、
u32 负载长度、负载、u64 FNV-1a 校验。全部整数小端；领域 `int` 为32位、
稳定身份为64位，路线/续体的 `size_t` 显式转换为u64并在读取时核对宿主范围；
浮点为 IEEE-754 binary32，布尔仅允许0/1。容器长度u32，拒绝重复键及尾字节。
文件与动态解码分配各最多64 MiB，动态条目累计最多100万。校验用于发现损坏，
候选还必须通过领域引用/占地/范围验证。数据集身份来自冻结 `SOURCES.json.snapshot_sha256`。

## 原25分区与当前字段

下表是来源覆盖对应，不是沿用原二进制分区编号或承诺原加载器全部行为。
没有迁入当前 Owner 的原字段不造槽；静态目录从固定规则取得，部分已有值缓存保留其更新时间。

| 原分区 | 当前编码字段或边界 |
| --- | --- |
| 0 延迟脚本 | `scripts.continuations/context/event_calls`，下一指令、等待、替换及参数；即时执行栈/页面不保存 |
| 1 饰品 | `catalog[(3,id)]` 的开放/提示/免费购买等共享状态 |
| 2 防具 | `catalog[(2,id)]` 同上 |
| 3 村办定义 | 现有 `scripts.activities/activity_flags`；未迁入9897村办的新经营状态不写入 |
| 4 人物定义 | `ai.growth/shop_humans/human_*`：职业/级别/属性/装备/住宅/满意度/消费/贡献/改名及延迟成长 |
| 5 道具 | `items/catalog[(0,id)]/shop_item_stock`：库存、开放、补货和提示状态 |
| 6 职业 | `scripts.professions/ai.professions` 的共享开放状态及当前维护规则值 |
| 7 未闭合原目录 | 当前 Owner 没有单独维护该目录的动态p/r；不伪造原槽或认证原档兼容 |
| 8 怪物定义 | `ai.monster_growth/battle.monsters`：共享成长、累计、开放和提示 |
| 9 人气定义 | `popularity_rewards`，原序及已维护状态；程序身份仍受固定数据集约束 |
| 10 任务定义 | `task_progress.definitions` 的完成/开放进度及当前任务目录维护状态 |
| 11 城镇称号 | 当前 `rank/rank_history/rank_met/rank_values`；没有独立维护的原目录p/r不造槽 |
| 12 设施定义 | `facility_uses/scripts.facilities/facility_definitions/facility_presence`、免费次数、目录开放及提示计数 |
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
| 24 完整设施 | `facilities/dungeon_facilities/facility_details/facility_*` 的占用、施工、销售、月/年历史、原实例ID/同定义序号 |

实例 `facility_details.notices`（原m.p）保留为维护效果历史；原写器包含该项，
原读后完整保留尚未闭合，因此不宣称精确原加载演出。邻接和商店中的重复通知投影不另写。
原调度时点缓存如人物逻辑格/half-cell/low-HP、cached view、遭遇影响场不能随意重算。

不保存共同随机、规则指针、页面及局部选择/父引用/答案、held输入、全局通知/浮标/声音、
人物cd/ce、审计检查点及逐笔 `CashEntry`。账本用保存余额构造，月账累计单独恢复，
不得重放费用以重建审计历史。同进程恢复使用当前随机流，确定性测试显式控制随机输入。
自动跨周档待轮内恢复合同闭合后接入，不能把当前手动档当作该检查点消费者。
