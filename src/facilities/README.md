# facilities

公开接口`include/ark/facilities/facility.hpp`区分不可变设施、定义共享等级、实例和有序占地绑定。
footprint保留1/2/4格两朝向顺序和分片编号，不按屏幕旋转矩形，不增加接路前置。
当前可建目录只有单格；多格底层契约已有测试，双格旅店/咖啡厅按真实初局保持未开放。
依赖world；app拥有实例/共享进度并协调资金，本包不拥有全局状态。
依据：[设施规格](../../research/dungeon_village_1/rules/FACILITIES.md#definitions)、[新局目录](../../research/dungeon_village_1/rules/STARTUP.md)。
