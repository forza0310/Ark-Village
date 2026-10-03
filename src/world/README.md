# world

标准C++格坐标、边界和源地图索引，公开接口在`include/ark/world/grid.hpp`。
不保存屏幕坐标、相机或纹理；显示记录不等于设施定义ID。越界索引在转无符号前拒绝。
facilities/app使用这些类型，desktop负责投影，3D重构复用本模块。
依据：[新局数据](../../research/dungeon_village_1/data/startup/README.md)、[地图访问](../../research/dungeon_village_1/rules/MAP_ACCESS.md)。
