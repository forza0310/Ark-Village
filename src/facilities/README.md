# facilities

公开接口`include/ark/facilities`区分不可变设施、定义共享等级/改良/使用累计/提示位、实例和有序占地绑定。
footprint保留1/2/4格两朝向顺序和分片编号，不按屏幕旋转矩形，不增加接路前置。
当前可建目录只有单格；多格底层契约已有测试，双格旅店/咖啡厅按真实初局保持未开放。
依赖world；app拥有实例/共享进度并协调资金，本包不拥有全局状态。
economy.cpp按原表端点/flags计算四属性、职业倍率、建设报价/工期和升级资格；逐组整数取整、维护费取整与两次封顶顺序不得合并。
neighbourhood.cpp按完整外环重新计算，种类2/3影响商店3，同来源/目标去重，同定义多实例叠加；每个道路格魅力+2。
来源保留稳定实例身份，包括空修正来源；with_neighbours替换旧修正，避免重复查询累加。查询不收费、不消耗道具、不自动升级。
效果图标与标记原表列表独立保留，部分非初期设施长度不同，不把未知消费者写成一一配对。
依据：[设施规格](../../research/dungeon_village_1/rules/FACILITIES.md#definitions)、[新局目录](../../research/dungeon_village_1/rules/STARTUP.md)。
