# app

Game是唯一可变村庄聚合，接口在`include/ark/app/game.hpp`，协调设施、人物、资金和更新。
confirm复用预览校验，在候选状态上分配实例、登记支出、扣款后提交；拒绝不改资金、ID或占地。
update推进1/2个有资格逻辑步，目录/放置/教程/镜头/暂停不积累时间。
首名冒险者420次更新后免费加入，事件89先锁存再展示，关闭不重复创建。
StartupData在构建期生成，不依赖research/Node运行时；启动参数独立在ark_launch。
本轮仅源地图/种子投影，1456步后保守停止，防止跳过未闭合月报费用，不是完整原版新局/AI/结算。
facility_queries.cpp提供只读经营/邻接视图，按当前完整实例占地排除被建筑覆盖的源道路，不缓存第二份可变状态。
职业输入仅固定无继承初局已解锁定义中的农家2/木匠1，不能替换为当前场景人数或推广到未知后续职业。
research已发布LOADED_MAP/LOADED_INSTANCES；产品尚未消费完整快照，入口身份/边界覆盖仍使用此前源投影。
依据：[STARTUP](../../research/dungeon_village_1/rules/STARTUP.md)，参照[研究聚合](../../research/dungeon_village_1/prototype/src/startup.cpp)。
