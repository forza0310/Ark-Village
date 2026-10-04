# facilities

公开接口`include/ark/facilities`区分不可变设施、定义共享等级/改良/使用累计/提示位、实例和有序占地绑定。
footprint保留1/2/4格两朝向顺序和分片编号，不按屏幕旋转矩形，不增加接路前置。
当前可建目录只有单格；多格底层契约已有测试，双格旅店/咖啡厅按真实初局保持未开放。
map_binding.cpp将完整有序占地绑定到未占用底图，保留实例/定义/分片与定义方向；拒绝重叠、未知种类及身份不一致。
绑定只构造地图视图，不代替建设审批。实例reset_legacy_id只记录重置来源，后建实例不伪造原ID。
依赖world；app拥有实例/共享进度并协调资金，本包不拥有全局状态。
economy.cpp按原表端点/flags计算四属性、职业倍率、建设报价/工期和升级资格；逐组整数取整、维护费取整与两次封顶顺序不得合并。
neighbourhood.cpp按完整外环重新计算，种类2/3影响商店3，同来源/目标去重，同定义多实例叠加；每个道路格魅力+2。
来源保留稳定实例身份，包括空修正来源；with_neighbours替换旧修正，避免重复查询累加。查询不收费、不消耗道具、不自动升级。
效果图标与标记原表列表独立保留，部分非初期设施长度不同，不把未知消费者写成一一配对。
依据：[设施规格](../../research/dungeon_village_1/rules/FACILITIES.md#definitions)、[新局目录](../../research/dungeon_village_1/rules/STARTUP.md)。


## 到达、使用与退出

arrival.hpp/cpp处理到达价格分支、访问/消费/设施销量统计，装备价替换普通价；只生成收入候选，不直接写账本。
exit.hpp/cpp处理定义共享使用累计/升级提示、满足度/人气请求、完整占地出口及普通延迟尾部。
service.hpp/cpp组合到达使用队列、旅店等待和完整退出计划，保留先释放、续活动再消费属性/装备尾部的顺序。
依赖people控制/身份、world地图及本包经济/几何；不依赖app。Placement是退出校验的输入投影，不是第二份实例所有者。
app/initial_ai对照研究d7ca763真实首访，将这些候选与现金/占用统一提交；普通默认世界、月报和人气显示仍另行接入。
依据：[使用协议](../../research/dungeon_village_1/rules/FACILITY_USE.md)与维护example/src/facility_service.cpp。

## 洞穴与迷宫探索

dungeon.hpp公开类别5的探索候选；住宅是类别9、募集是类别10，不共用探索语义。
dungeon_crew.cpp处理耐力、入口初始化、探索长度、阶段1队伍/挑战/撤退及有序奖励请求，保留逆序和重复人物引用、负进度及明确的撤退结果输入。
dungeon_completion.cpp只处理阶段2任务/场地/通知/奖励显示的有序请求；不直接发放目录奖励或改变地图。
来源4ba4805及前置c4ce4b2维护example/dungeon_ai与[探索规则](../../research/dungeon_village_1/rules/ai/DUNGEONS.md)。阶段2请求携带真实任务定义ID和待结算值，任务成功统计不能命名为解锁；清活动任务由任务所有者清全部人物定义flags2。仅依赖people身份/world格及标准容器，无app/UI依赖。
队伍快照是设施候选输入，不创建第二份持久人物状态；实际人物/任务/地图/目录由未来统一世界所有者提交。world_dungeon、o0和任务UI尚未进入产品，正常新局不自动创建夹具洞穴或开放探索。
