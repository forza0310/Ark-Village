# app

Game是唯一可变村庄聚合，接口在`include/ark/app/game.hpp`，协调设施、人物、资金和更新。
confirm复用预览校验，在候选状态上分配实例、登记支出、扣款后提交；拒绝不改资金、ID或占地。
update推进1/2个有资格逻辑步，目录/放置/教程/镜头/暂停不积累时间。
首名冒险者420次更新后免费加入，事件89先锁存再展示，关闭不重复创建。
StartupData在构建期生成，不依赖research/Node运行时；启动参数独立在ark_launch。
fixed_step_clock.hpp/cpp为独立ark_timing目标，仅标准C++，将调用方提供的单调时钟间隔转换为离散外层更新次数；不更改Game或领域的tick单位。
暂停/模态/重开清空积累，恢复丢弃跨越阻塞的首段间隔；每绘制帧最多补算8次，超出时间丢弃，防止调试停顿后大量追赶。这是产品适配策略，原版墙钟规则待研究。
本轮消费加载后576格/8实例快照；1456步后保守停止，防止跳过未闭合月报费用，不是完整原版AI/结算。
facility_queries.cpp提供只读经营/邻接视图，按当前完整实例占地排除被建筑覆盖的当前道路，不缓存第二份可变状态。
职业输入仅固定无继承初局已解锁定义中的农家2/木匠1，不能替换为当前场景人数或推广到未知后续职业。
map_queries.cpp按加载后底图和当前实例重建访问绑定，route_to只读查询，不修改资金/人物。
reset时原ID+1映射到稳定非零ID，保留原向量顺序instance_order，新增实例使用单调ID并追加顺序。
建设审批读取加载后逻辑状态3/4；显示ID不作为可建判据。围栏/外入口附加覆盖仍待绘制绑定。
依据：[STARTUP](../../research/dungeon_village_1/rules/STARTUP.md)，参照[研究聚合](../../research/dungeon_village_1/prototype/src/startup.cpp)。

## 实时名单调度

ai_schedule.hpp/cpp消费研究d412d6e的已获准世界轮次：先全部人物决策、全部人物执行，再怪物决策/执行，随后投射/物体/遭遇/设施与finalize。
人物两遍逆序，怪物执行和设施正序；正序删除继续加下标，保留跳过与同轮追加语义，不改为稳定快照遍历。
名单身份按种类隔离，原始ID0有效；调度接口的原始名单身份不能直接当people::ActorId非零快照身份。
handler看到本轮候选名单，必须只修改独立所有者副本并暂存外部请求。失败或限额返回无候选，调用方丢弃副本；调度器不能撤销handler已经执行的真实世界写入。
人类执行返回删除且绑定设施时，先请求释放后删名单。admitted由真实运行所有者解决模态资格，安全dispatch_limit不是原版玩法人数限制。
产品已编译该纯调度并做组合验证；普通建设模式Game尚未调用它，显式AI预览通过InitialAiSession消费初局两遍调度，完整世界默认AI仍待接通。
依据：[聚合遍历](../../research/dungeon_village_1/rules/ai/LIFECYCLE.md#聚合遍历与同轮新建)。

## 真实初局AI私有会话

initial_ai.hpp及initial_ai.cpp拥有整段会话状态、前置检查和c/d调度；initial_ai_motion.cpp负责优先级/选目标/道路运动，initial_ai_service.cpp负责到达收费、使用、退出及控制消费。
来源为d7ca763的prototype/startup_ai；复用产品地图绑定、邻接、调度、设施和人物纯规则，无raylib依赖。
仅接受未修改的真实420步首访Game快照；允许显式选择两个已证出生点。构造不修改Game；普通建设窗口不创建，显式AI预览的Game在构造后消费并投影视图。
每轮在私有副本上推进所有统计、资金、占用、共享使用、HP与队列；任何后段失败全部放弃，成功后一次替换。
到达普通价格取真实邻接派生值，武器取原表装备价；退出后成功8先出发，19只累计属性，28才装配并重算。
initial_ai_data.hpp从固定源表编译23职业/33武器及三个服务输入；initial_ai_check运行六条有界案例供--check-ai调用。
票号显式输入，未复演APK随机消费/同成本路径；不包括日历/月界、玩家改图、任务/救援/战斗或默认AI启用。

## 主程序可视预览

Game的PlayMode::ai_preview明确选择有限初局窗口。game_ai_preview.cpp拥有接通、整轮执行和UI投影；InitialAiSession仍是AI状态唯一事实来源。
先执行真实首访，首目标决策在教程同轮完成；普通模态/暂停资格冻结后续AI。desktop直接安排首访后的视图供--ai-preview查看。
round_random只在需要时按实际权重生成类别/设施票号与退出满足度/属性票号。会话与mt19937均准备副本，错误不消费随机状态；不是APK随机实现。
Game.state的money/adventurer/definition_progress是预览的只读UI投影，建设命令在此模式拒绝，没有第二个业务写入者。
不推进首访之后的日历、不允许改图，不接战斗/救援/离场；未知类别/回退或1000轮保护结束，窗口可重启，错误可查询ai_error。
普通PlayMode::startup仍提供建设/首访，不自动创建预览会话；Game按值复制会话使update/倍速与失败提交保持原子。
