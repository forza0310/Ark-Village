# app

Game是唯一可变村庄聚合，接口在`include/ark/app/game.hpp`，协调设施、人物、资金和更新。
confirm复用预览校验，在候选状态上分配实例、登记支出、扣款后提交；拒绝不改资金、ID或占地。
update推进1/2个有资格逻辑步，目录/放置/教程/镜头/暂停不积累时间。
首名冒险者420次更新后免费加入，事件89先锁存再展示，关闭不重复创建。
StartupData在构建期生成，不依赖research/Node运行时；启动参数独立在ark_launch。
本轮仅源地图/种子投影，1456步后保守停止，防止跳过未闭合月报费用，不是完整原版新局/AI/结算。
依据：[STARTUP](../../research/dungeon_village_1/rules/STARTUP.md)，参照[研究聚合](../../research/dungeon_village_1/prototype/src/startup.cpp)。
