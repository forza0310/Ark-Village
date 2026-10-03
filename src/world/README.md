# world

标准C++格坐标、边界和源地图索引，公开接口在`include/ark/world/grid.hpp`。
不保存屏幕坐标、相机或纹理；显示记录不等于设施定义ID。越界索引在转无符号前拒绝。
facilities/app使用这些类型，desktop负责投影，3D重构复用本模块。
terrain.cpp的connect_roads根据当前未占用路面派生四方向掩码和原版16帧，不修改源地图、路径类别或建造审批。
同类道路与越界连接；仅实现维护STARTUP已发布的帧映射，2×2拼块覆盖和完整加载后快照分开接收。
依据：[新局数据](../../research/dungeon_village_1/data/startup/README.md)、[地图访问](../../research/dungeon_village_1/rules/MAP_ACCESS.md)。
