# world

标准C++格坐标、100单位连续世界坐标、地图索引与寻路，公开接口在`include/ark/world/`。
loaded_map.hpp保留加载后格的逻辑状态、类别、显示、道路/边界标记及原始/稳定身份，不混用这些编号。
navigation.cpp提供按出发格计费的距离场、前驱回溯和到达身份检查；终点设施不能作为普通中转。
首步离开例外与搜索预算显式传入；默认平局沿用维护C++示例的小索引策略。
SearchLimits的reverse_equal_cost可显式采用已证c.l逆行主序扫描/严格更小替换，改变等成本前驱而不改距离；max_expanded_cost在代价超过阈值的节点展开前结束，保留已经发现、可回溯的外沿。
该阈值与max_cost不同：max_cost裁剪超预算边并报cost_limit，max_expanded_cost返回有效部分距离场；未发现的目标仍为unreachable。正常消费者按world_departure已证读取点启用，旧诊断默认保持原策略，不由纯寻路测试宣称全随机/整段行为与APK相同。
不保存屏幕坐标、相机或纹理；显示记录不等于设施定义ID。越界索引在转无符号前拒绝。
facilities/app使用这些类型，desktop负责投影，3D重构复用本模块。
terrain.cpp的connect_roads根据当前未占用路面派生四方向掩码和原版16帧，不修改源地图、路径类别或建造审批。
同类道路与越界连接；仅实现维护STARTUP已发布的帧映射，加载后快照已接入，2×2/地图上下边缘补块在desktop/road_render按当前未占用路面派生，PNG偏移不进入world。
依据：[新局数据](../../research/dungeon_village_1/data/startup/README.md)、[地图访问](../../research/dungeon_village_1/rules/MAP_ACCESS.md)。
