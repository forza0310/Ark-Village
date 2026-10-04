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
