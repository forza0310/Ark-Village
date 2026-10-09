# app

`ark_world_save`实现当前版本两栏手动档：显式字段协议、有界解码、候选引用/占地验证、纯缓存恢复及同目录临时文件替换。`WorldSession`捕获/载入均经FIFO，成功载入更换generation拒绝旧世界输入。文件不存随机/页面/held/通知/声音，不重放账本历史；玩家当前暂停与随机流由提交时Owner提供。Windows默认`LOCALAPPDATA/Ark-Village/saves`，测试用显式目录。原25分区对应及首版边界见[存档说明](world_save.md)。

`world_session.hpp/.cpp`是默认桌面的标准C++线程边界，链接runtime/system/save/timing/Threads、不依赖raylib。工作线程是唯一世界提交者；主线程持不可变`WorldFrame`。FIFO命令只在事务间执行，相邻待处理镜头可以合并，决定/暂停/诊断速度保持顺序。47ms实际开始截止、卡顿不追赶；玩家窗口固定一倍速，显式无窗口研究回归仍可查询源双轮合同。加载沿用当前会话速度。停止唤醒并join，失败保留最后成功世界。

`world_commands.hpp/.cpp`按稳定职责分离显式决定的源适配，调用当前冻结4ef4a98及其前置维护消费者；线程/队列/快照提交仍归session。建设目录21、选择/连续放置/取消、设施详情74、入住80、任务管理/人物详情/中止、授勋87及晋级48共享同一候选Owner、地图、账本和随机流。目录/页面命令绑定页稳定ID，建筑点击绑定设施稳定ID；放置和取消同时携带UI观察到的预期definition，旧点击不能操作新选择。设施升级81沿原页确认消费者，不在产品重新计算等级/阈值。

人物点击同时校验实时actor与定义身份，raw60四页及61–66/68/70/73动作等待源page_ready，选择索引与装备槽由Owner持有。65只回传答案，64恢复才消费库存/现金与奖励；63的55切职业/197最终重算不由UI替代。税收90只接受源选择/确认、没有取消；98由实际更新自动结算。声音请求在初始发布及每次成功提交后只领取一次到当前静默sink，快照读取不重放；不等于音频播放已实现。本批验收通过（结果见B1）。

游戏拒绝保留源合法反馈页，用可恢复rejected回应；旧页/暂停/旧建造选择/已消失设施也可恢复拒绝。缺源、脚本或运行时失败仍停止worker；普通ack及月报错误保持原明确失败协议，不代替任务、建设、入住、授勋或晋级选择。`WorldFrame.command_results`仅保留最近64条显式决定的serial/outcome、源错误、task/build denial及实际新建ID，跨镜头/tick留存，不是游戏事件日志。授勋selection是源贡献排名索引，入住selection是实际人物定义ID，不混用。

任务`open_task_menu`在无活动任务时开目录22，有活动任务时按源开管理26；`open_task_control_menu`单独开中止入口4。`act_task_page`传入明确inspect/request_abort等动作，raw1询问必须由该协议确认；不以普通ack自动选择中止。征集held绑定raw24页身份，仅逻辑页更新消费；切页、暂停时清除，迟到旧页release不能清除新页press。自动月报不另加输入阻断。普通ack不替任务、人物、建设、税收等显式决定动作。

桌面五项菜单是产品适配：`WorldFrame.main_menu_open`由worker持有，只影响冻结资格，不伪造raw3或改写显式暂停。打开接受源主场景，可见月报不阻止；关闭保留暂停，47ms门槛继续运行但不产生世界工作、关闭不补算。`open_menu_tasks`/`open_menu_build`在一个FIFO候选中打开真实源页面并关闭菜单，避免关菜单和开源页面之间世界插队；暂停/失效选择拒绝且保留菜单，源空任务目录提示正常保留。菜单打开时直接快捷入口拒绝旁路。系统提供两栏手动保存/读取及只读跨局纪录，独立`save_menu_open`冻结门槛，村办已开放0–3活动和按flag16开放的商会，情报禁用。

`world_report.hpp/.cpp`提供报告可见性查询和显式诊断skip，正常桌面与headless均使用源自动月报：phase1/2各70次有资格更新，关闭时一次消费点数。报告不冻结世界、不阻断正常命令；原模态/场景资格和显式暂停照常生效，不能自动resume。诊断skip校验期望阶段、非暂停及世界场景0/2，不重新收费或伪造世界轮次。旧手动政策已由最新用户要求取代。

`world_facility_queries.hpp/.cpp`只读当前simulation Owner的设施实例，复用发布经营/邻接规则，返回page74模板、共享等级/使用/提示、实例施工/占用、当月收支与原口径累计利润。消费价格不是建设报价，工期读取建设时固化阈值；不使用旧Game或初局职业计数。初始邻接来源缓存缺失时仅作纯查询重建，必须与Owner三属性相同；已有缓存还需来源身份/顺序一致，否则返回明确错误，不显示虚假的零奖励，也不刷新世界。接口不负责raw74开关、暂停资格或道具/购买/升级/入住动作。

`world_simulation_main.cpp`是完整共同世界的标准C++有界入口，仅链接`ark_world_runtime`，不会创建旧Game；严格解析帧预算、月份目标和显式Java种子。自动确认普通真实页栈仅在`--auto-confirm`启用；raw16/56/57/97自行推进，任务页22..28/33和商店83始终等待显式决策，不隐式接任务/出发/续费/返回；raw87仅在显式`--end-awards`时执行请求/确认终止，`--speed 0|1`传入源1/2轮规则。未达月份目标或运行失败返回非零；输出实际账本、名单、随机次数与页身份供复查。

默认桌面入口使用simulation唯一Owner，`--world`是显式别名。以下Game职责属于`--legacy-slice`建设切片及其明确诊断，两种模式不能并行同步。

Game是唯一可变村庄聚合，接口在`include/ark/app/game.hpp`，协调设施、人物、资金和更新。
confirm复用预览校验，在候选状态上分配实例、登记CashLedger支出后提交；拒绝不改资金、ID或占地。State.accounting是唯一现金账本，money仅供既有HUD读取。
update推进1/2个有资格逻辑步，目录/放置/教程/暂停不积累时间；首访跟随镜头对应scene6，人物、施工与日期均冻结，恢复后重新验资格。
首名冒险者420次更新后免费加入，事件89先锁存再展示，关闭不重复创建。
StartupData在构建期生成，不依赖research/Node运行时；启动参数独立在ark_launch。
ark_timing仅标准C++，不更改Game或领域的tick单位。original_loop.hpp/cpp提供已证绘制/输入门槛的数值查询：默认v21、整数47ms、等待后实际观测提交、超时不补算，以及正常场景的倍速轮数快照条件。最新LIFECYCLE/WORLD_SCHEDULE明确原版先更新主场景，再在框架绘制/输入路径等待；该纯查询接口本身不安排生命周期。
simulation_clock.hpp/cpp转换平台秒数到整数毫秒；当前产品借用47ms参数调度独立离散更新，暂停/模态期间门槛继续运行但不产生世界工作，重开清时钟。它尚未复刻原版更新→绘制门槛生命周期；独立60FPS绘制和GetTime单调观测均为产品平台政策，实际原版节奏另验。
显式--tick-rate才采用fixed_step_clock.hpp/cpp：阻塞清积累、恢复丢弃跨阻塞间隔、每次最多8次补算并丢弃超限时间，作为固定频率实验保留。绘制截止由desktop管理，不进入本包。
本轮消费加载后576格/8实例快照；1456步后保守停止，防止跳过未闭合月报费用，不是完整原版AI/结算。
facility_queries.cpp提供只读经营/邻接视图，按当前完整实例占地排除被建筑覆盖的当前道路，不缓存第二份可变状态。
职业输入仅固定无继承初局已解锁定义中的农家2/木匠1，不能替换为当前场景人数或推广到未知后续职业。
map_queries.cpp按加载后底图和当前实例重建访问绑定，route_to只读查询，不修改资金/人物。
reset时原ID+1映射到稳定非零ID，保留原向量顺序instance_order，新增实例使用单调ID并追加顺序。
建设审批读取加载后逻辑状态3/4；显示ID不作为可建判据。围栏/外入口附加覆盖由desktop按发布标记绘制，领域不持有纹理。
依据：[STARTUP](../../research/dungeon_village_1/rules/STARTUP.md)，参照[研究聚合](../../research/dungeon_village_1/prototype/src/startup.cpp)。

## 实时名单调度

ai_schedule.hpp/cpp消费研究d412d6e的已获准世界轮次：先全部人物决策、全部人物执行，再怪物决策/执行，随后投射/物体/遭遇/设施与finalize。
人物两遍逆序，怪物执行和设施正序；正序删除继续加下标，保留跳过与同轮追加语义，不改为稳定快照遍历。
名单身份按种类隔离，原始ID0有效；调度接口的原始名单身份不能直接当people::ActorId非零快照身份。
handler看到本轮候选名单，必须只修改独立所有者副本并暂存外部请求。失败或限额返回无候选，调用方丢弃副本；调度器不能撤销handler已经执行的真实世界写入。
人类执行返回删除且绑定设施时，先请求释放后删名单。admitted由真实运行所有者解决模态资格，安全dispatch_limit不是原版玩法人数限制。
world_schedule.hpp/cpp组合共同阶段与上述实时名单协议，显式消费者不能无条件冒充已执行世界副作用；owned适配器在完整调用方副本上准备，晚失败不提交半状态。
正常Game的game_world.cpp/step_normal_world按首访→c/live_decision→一次execution_prefix→FIFO→缓存/留存尾部→设施更新→final阶段消费现有人物与施工，首次同轮追加和删除时点走同一协议。game_life.cpp只负责人物创建、候选提交与投影。
当前influence未物化（无战斗所有者），popularity仍由life.requests保留而未接全消费者，final只有零/一人且无怪物的空配对；怪物/物体/遭遇等真实所有者仍未接通，不能将阶段经过当成完整世界运行。
依据：[聚合遍历](../../research/dungeon_village_1/rules/ai/LIFECYCLE.md#聚合遍历与同轮新建)。

## 真实初局AI私有会话

initial_ai.hpp及initial_ai.cpp拥有整段会话状态、前置检查和c/d调度；initial_ai_motion.cpp负责优先级/选目标/道路运动，initial_ai_service.cpp负责到达收费、使用、退出及控制消费。
来源为d7ca763的prototype/startup_ai；复用产品地图绑定、邻接、调度、设施和人物纯规则，无raylib依赖。
严格诊断构造仅接受未修改的真实420步首访Game快照；允许显式选择两个已证出生点。构造不修改Game，显式AI预览的Game持久消费并投影视图。正常生活使用from_village临时适配入口，不使用这个严格初局前置。
每轮在私有副本上推进所有统计、资金、占用、共享使用、HP与队列；任何后段失败全部放弃，成功后一次替换。
到达普通价格取真实邻接派生值，武器取原表装备价；退出后成功8先出发，19只累计属性，28才装配并重算。
initial_ai_data.hpp从固定源表编译23职业/33武器及28/30/33/35/45服务输入；initial_ai_check运行六条有界案例供--check-ai调用。
票号显式输入，未复演APK随机消费/同成本路径；严格诊断不包括日历/月界、玩家改图、任务/救援/战斗，不作为正常玩法的状态所有者。

## 正常人物生活

life_state.hpp定义持久人物与设施服务状态；Game::State.life持有人物，facility_life持有各实例销售/占用，definition_progress仍是定义共享进度唯一来源。
live_life_context.cpp从当前地图、设施、施工阶段、邻接、等级/改良、账本构造每轮临时候选；成功后一次回写Game并丢弃InitialAiSession，失败不留下部分扣款/占用/随机消费。
建设和访问读写同一CashLedger及单调现金事件ID。布局变更/施工完成递增layout_revision，当前行程重校验原目标和路线，不保留初局路径快照。
关闭首访教程后自主生活与日期、施工共用正常更新资格；暂停/菜单/建设模态冻结它们，恢复不积累逻辑债务，倍速每次重新验资格。首个月报准备前1456步保护保留。
live_departure.cpp/live_motion.cpp接类别4门口设施→活动6→真实地面路线→P到达转5→同次d执行10小步漫游；c不提前刷新旧s，目标O与缓存位置独立，r清理保留原O/路径身份。
正常出发显式启用已证reverse_equal_cost和max_expanded_cost，严格初局诊断默认搜索不变。普通FIFO成功8早停而24/失败r继续同次解释，d前缀仅执行一次。
live_daily.cpp接state0的L→P及state5的表情8→HP/城外计数重选→L；c0重选执行真实状态转换，不在同次c重复P。L按启动区域下限2、实际空怪物名单与研究初始上限4资格懒抽1000，state0概率0仍消费；state5只有票号<13才保存spawn_ticket/spawn_center并交接真实遭遇创建请求，其余正常漫游继续。
无继承首访的D=[0,0,0,0]编译为home坐标/状态/第四槽，m0归Game共享定义表；类别3/活动5可走真实Map.f出口0/26并写m1退休。未知home输入仍明确交接，不把reset事实推广到读档/继承/其他定义。
未接活动/装备选择保留真实队列与pending_category/pending_definition/pending_activity，局部error停止该人物并向UI交接；日期、施工继续。该保护是产品边界，不能称为原版等待或通过重抽跳过。

## 主程序可视预览

Game的PlayMode::ai_preview明确选择有限初局窗口。game_ai_preview.cpp拥有接通、整轮执行和UI投影；InitialAiSession仍是AI状态唯一事实来源。
先执行真实首访，首目标决策在教程同轮完成；普通模态/暂停资格冻结后续AI。desktop直接安排首访后的视图供--ai-preview查看。
random.hpp/cpp的RandomStream由Game唯一持有，按研究Java48位LCG生成nextInt，再有符号取余绝对值；不是nextInt(bound)/均匀分布。明确seed与raw tape供复现，不代表已观测APK默认seed。正常/预览在实际消费点抽取；会话与随机均准备副本，失败不消费随机状态。正常首访前段先提交出生消费，后段人物候选失败只回滚自己的消费。当前已接消费者的次序可验，完整未接场景/世界消费者仍不能声称全局APK随机等价。
Game.state的money/adventurer/definition_progress是预览的只读UI投影，建设命令在此模式拒绝，没有第二个业务写入者。
不推进首访之后的日历、不允许改图，不接战斗/救援/离场；未知类别/回退或1000轮保护结束，窗口可重启，错误可查询ai_error。
普通PlayMode::startup提供建设/首访和当前村庄生活，不创建持久预览会话；预览仍作为严格初局golden诊断，与正常有限生活各自验收。

`world_system.hpp/.cpp`复用正式主角/继承/计分与系统编码helper，协调产品标题和唯一worker。新局先准备资料候选，再写system.arksys；未到访定义0只增加覆盖资料，ID1仍是原首访。系统写入是世界/计分发布前最后一步，失败保留旧发布及待提交候选，retry_system_write重试原候选。raw17不走generic ack，尾段只执行一次关页、声音请求、事件6→4/5和系统更新。玩家schema4与完整维护AVRSAVE协议分开，详见[存档说明](world_save.md)。
