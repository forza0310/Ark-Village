# 项目目标与架构

## 目标与依据

基于research中《冒险迷宫村》一代逆向结果，使用C++17 + raylib复现原版玩法、数值、事件顺序、UI设计和操作流程。
先稳定二维行为，再推进3D化；3D替换投影、场景与输入适配，复用独立领域规则及验收。
旧代码、schema、自定资金/居民/工程日/地图和测试均不是兼容要求；当前已有正常建设/首访生活切片。

- [research入口](../research/README.md)是规格、示例、素材和证据来源；原型夹具/安全策略不能自动当作原作事实。
- 固定输入为一代汉化重签APK，身份见 [EVIDENCE](../research/dungeon_village_1/EVIDENCE.md)；不复制反编译实现。
- 三批22张用户图片提供部分外观，第3批明确不同于APK版本；视觉和规则分别登记，截图数值不能覆盖原表。
- 新局静态地图/初值/首访和加载后格/实例静态重建已交付；未改图的首段AI条件组合先接入私有诊断，现复用已证规则接入正常Game和当前建设状态。完整世界/月度组合仍有缺口，不用自定demo填补。

研究覆盖、产品接入、原版对照分别记录；测试、画面、输入和行为等价不能相互替代。
具体条目与缺口见 [原版对照](reference/REFERENCE_CHECKLIST.md)和 [研究需求](reference/RESEARCH_REQUESTS.md)。

## 当前实现

完整世界接入新增`ark_world_rules`与`ark_world_runtime`，接口在`include/ark/simulation`，实现按规则、初始化、路由、场景、到访、日历、页面、任务与非人物消费者分文件。固定源和机械改写哈希见`assets/simulation/SOURCES.json`；全部定义/脚本构建时从产品副本生成。
`ark_world_simulation`和`ark_village --world`只创建完整运行时，bootstrap投影后销毁。地图、共同账本、随机、人物/怪物、共享定义和页面均由一个Owner持有，路由/脚本/收尾只是事务投影。普通Game建设切片仍为默认入口；两者不并跑、不互相同步，玩家建设尚待直接写新Owner的桥接。
以下Game说明属于默认建设切片。完整世界的框架轮次、可见性缓存、页栈和真实跨月逻辑单独沿用维护运行时，不能套用旧1456步保护或单人物state.life。

标准C++的ark_game包含world、facilities、people、economy与app聚合；ark_launch负责参数，ark_asset_metadata负责维护SEB/TSV子集。
ark_timing的app/SimulationClock负责平台时钟到离散外层更新的适配；Game仍只接收逻辑步，不依赖raylib或秒数。默认OriginalLoopPacing采用整数47ms最小开始间隔、实际观测提交、卡顿不补算的桌面政策；最新研究确认原门槛位于render/input后段，产品独立更新截止不能视为原框架循环等价。桌面单调时钟与原Android墙钟也有平台差异。
逻辑截止与60FPS绘制/输入截止共同决定等待，逻辑可在绘制之间运行；暂停/模态只阻止世界工作，不积累债务，重开清时钟。--tick-rate显式覆盖才采用FixedStepClock的固定频率/暂停清积累/最多8次补算实验政策。二倍速增加有资格逻辑轮数，人物每步6.7单位不变；47ms静态门槛不是恒定原版FPS动态认证。
raylib的ark_village只处理窗口、资源、投影、输入与UI。src各实际模块有README，接口在include/ark同名目录。
desktop/scene负责加载后地表/连续世界位置投影及预览，desktop/ui拆分共享布局、临时导航/控制、原版皮肤、HUD和页面；窗口/循环由game_view协调。
界面响应窗口比例，命中和绘制共用逻辑布局，业务数据不移入UI。逻辑尺寸仅用于坐标，渲染画布使用framebuffer原生像素（含Retina）。
像素素材最近邻采样，文字按物理显示密度生成字形；鼠标继续使用窗口点坐标，不重复乘DPI。
地表SEB绘制原点与人物脚底分开，连续人物位置对齐60×29地块中心；desktop/character_animation依据连续位置的原始整数投影选择walk00..03四向身体，每方向四帧，按获准轮次播放并在暂停时冻结。显示朝向缓存独立于镜头/zoom/DPI，静止保留，初次/控制转向读取当前人物；当前生活切片尚未持有原u/v缓存，因此显示结果不回写AI。每6移动轮次换帧仍为桌面政策，完整原动作时钟另行接入。
facilities/economy与neighbourhood为纯派生规则，app/facility_queries提供只读查询；UI按页74条件显示实际属性、维护费与来源。
world/loaded_map保留加载后证据，navigation提供加权距离场/回溯，facilities/map_binding按当前完整占地绑定；
people的activity_candidates/activity_choice/facility_choice/departure提供候选、两级选择及完整出发候选；票号显式输入，上层优先级由调用方负责。
people/motion提供显式目标行程与一次性进入信号。上述纯规则已接入；actor_ai/ai_perception补资格和优先级，decision组合普通出发，actor_control准备本地控制/状态/漫游/装备候选。
正常Game已接普通生活链；完整世界更新及其余跨所有者提交按已交付研究逐项迁入，缺证分支另登记。world/terrain按已证掩码连接道路，desktop/road_render按当前占用派生2×2/上下边缘PNG补块并进入共享深度队列；desktop/projection负责场景小步缩放/锚点/拾取，HUD与领域不随缩放变化。
app/ai_schedule提供实时名单两遍与删除/同轮追加；world_schedule在其上明确共同阶段，game_world.cpp将已有人物生活和施工接入正常Game，完整世界所有者仍未齐备。
people/actor_effects分离显示/延迟/控制时点，weapon_choice提供武器/防具/饰品候选，human_growth与delayed_reward提供定义共享属性/职业成长和九步经验。
各模块只返回候选/请求；组合测试验证装备延迟提交、共享成长与调度私有副本，不能自动推导完整世界已经运行。正常人物自主活动由Game的生活适配入口负责。
people/actor_housekeeping已接c前缀/状态18、物理与留存清理候选；combat_ai公开接口对应strategy/damage/influence三个实现文件，接入策略、九向评分、物理/魔法、怪物成长和双侧影响场。事件原ID/当前对象身份分开，b恢复与c(state)/动作指令分开。
这些标准C++规则通过app调度/控制/运动/清理的条件组合测试；完整共同世界感知、遭遇/救援/掉落及跨域解释器的产品所有者尚待接入，不制造新局战斗。

app/initial_ai接入d7ca763初局真实生活链，拆分初始化/调度、运动、设施服务与执行；复制整轮候选后提交，失败不残留扣款/占用/队列。
facilities的arrival/exit/service分离到达收入与使用退出；people/hp区分目标/显示HP，economy/cash记录即时现金且按事件身份去重。
严格诊断构造只接受未改图的420步首访快照，`--check-ai`验证两个出生点×三目标；这个前置不再约束正常玩法。
显式--ai-preview由Game拥有同一会话，game_ai_preview.cpp协调获准轮次并投影只读的资金/人物/共享使用到既有UI；窗口按真实连续位置绘制，不使用motion检查覆盖。
首访后仅推进AI，不推进日历，布局固定；正常与诊断随机均由Game唯一持有的RandomStream提供，使用Java48位LCG与原abs(nextInt()%bound)，不采用均匀有界抽样。实际分支懒消费，失败候选回滚随机游标；固定seed/raw tape只作可重复输入，未认证APK默认种子。未知离场/回退或1000轮预览保护结束本轮。正常建设启动不自动启用此预览。

正常PlayMode::startup由game_world.cpp的step_normal_world协调共同轮次，game_life.cpp负责持久状态创建/提交及UI投影；State.life保存人物控制/运动/HP/成长/待交接状态，State.facility_life保存实例销售与占用，definition_progress保存共享等级/使用累计。
顺序为首访前段→人物c/live_decision→一次execution_prefix→FIFO→旧缓存位置/留存尾部→设施施工→最后配对阶段。首访同轮加入实时名单，删除请求按调度时点处理；当前仅零或一名人物且无怪物，最后配对无实际对象。协议不持有另一份耐久世界。
轮首影响场尚未物化，因为没有战斗所有者；人气请求仍保留于人物requests，没有提交全部原版消费者，不能称为完整共同世界已经闭合。
live_life_context.cpp每个获准轮次从当前Game构造InitialAiSession::from_village临时候选，重建当前地图绑定、邻接和等级/改良经营输入，成功后回写唯一聚合并丢弃适配器；正常模式不持久保存另一套地图、设施或资金。
建设与到达收费均记入State.accounting唯一CashLedger，money仅为HUD投影；失败候选不改真实账本或随机状态。
建设提交和施工完成递增layout_revision，已有行程按当前地图重新验证原目标与路线；建设模态/教程/暂停阻止后续轮次，日期、施工和已接生活在正常资格下共同推进。
live_departure.cpp与live_motion.cpp接已证类别4入口选择、入口设施后活动6、真实无绑定地面路线与P到达；c移动连续位置而d尾部才重投影旧s，O目标身份独立，r保留O，不把出口身份伪装成商店。
正常出发可显式采用原逆行主序同成本顺序及max_expanded_cost；后者保留已发现外沿而不是裁边。地面到达转c5，同次d可执行10产生小步漫游。live_daily已接state0先L后P、state5表情8→按HP/城外计数重选c0→L；state5同次c不再落入P。L读取已交付初局区域下限2，仅资格通过才抽1000；state0创建概率为0，state5票号<13产生真实遭遇请求时才保留handoff，未触发则继续活动。实际遭遇所有者尚未迁入。
新增源表35/45等待与退出效果投影；无继承首访D=[0,0,0,0]与m=0已纳入校验数据，Game初始化LifeHome并保留fourth_slot，departed_definitions保存共享离村状态。类别3/活动5可读取真实新局home，出口按Map.f→0/26→m1→退休处理；这些初值不泛化到继承/加载或其他角色。尚未接装备消费者仍保留真实请求。
未接分支保留队列、已选类别/定义并显示局部handoff，只停止该人物，日期/施工继续；不重抽掩盖缺分支，这不是原版等待状态。当前Mode::camera对应scene6首访跟随镜头，冻结人物/施工/日期；上一轮把它等同于UserData内部镜头延迟的放行已撤回。严格初局预览保持原镜头准入约束。
探索task_success完整纯消费者已接设施模块；真实任务所有者、地图恢复和脚本/UI尚未进入正常Game。desktop按真实HP绘制原版矩形RGB血条，不因新增血条就注入新局战斗。

启动顺序：参数→程序旁CPU素材校验→无窗口首访/AI检查或显示器检查→窗口/RAII资源→Game静态新局→输入/逻辑更新→绘制。
静态输入为24×24加载后地图、8个保留原向量顺序与原ID的设施实例、5000G/10点数/50人气；首名冒险者420次有资格更新后免费加入。
静态入口身份/逻辑和围栏/外入口附加覆盖绘制已接入；desktop/boundary_render纯映射复用加载后格标记，scene按当前地表显示/视口/深度提交，区域0来自STATE而非设施等级。扩张及其通行重建未实现；七种单格设施可建设，双格旅店/咖啡厅初期未开放。
道路/入住募集为禁用预览，移动/撤除/住宅/完整AI/存档未实现；普通访问产生实际共享收入。1456逻辑步后仍保守结束本轮，防止跳过尚未接入的月报消费者，不宣称无限连续经营。完整Scene Owner、脚本/页面、日历/月度、战斗/人气世界消费者属于后续产品接入。
启动事实、随机和日常决策这一批四套构建/170次CTest及正常窗口controller验收通过，见[B1阶段记录](stages/B1-playable-prototype.md#启动事实随机与日常决策接入2026-10-04)。自然新局仍在真实遭遇请求处交接；出口退休只验显式会话条件，OS输入及APK动态未由产品检查替代。

## 领域设计原则

依赖方向为表现/输入→应用协调→领域规则→基础类型/不可变定义。领域仅标准C++，不出现Texture2D、Vector2、屏幕像素或窗口API。
静态设施定义、定义共享进度、设施实例、人物状态、经济统计各有唯一事实来源。派生视图和缓存不作为第二份耐久事实。
应用协调者准备候选、重新校验并一次提交；非法命令不能留下部分扣款/布局/引用。

| 候选职责 | 依据及边界 |
| --- | --- |
| 定义目录 | 原表ID、种类/活动类别、几何、已解释端点/flags；未知字段不猜名称 |
| 共享设施状态 | 等级、改良、使用累计、升级提示按定义共享；不放进每个实例 |
| 地图/实例 | 底图显示状态与路径类别分开、完整占地绑定、实例身份/位置/朝向及实例统计 |
| 人物/活动 | 资格、候选、目标、到达/使用/退出；职业统计不由UI注入 |
| 经济/城镇 | 金币、村子点数、人气分开；费用/分类统计/报表时点依研究 |
| 调度与应用 | 明确逻辑更新资格/顺序、跨域提交；不预建通用事件总线 |
| 表现 | 页面栈、选中/输入、投影/纹理/动画，规则从领域视图读取 |

已实现职责及剩余候选见[切片记录](stages/B1-playable-prototype.md)，不预建完整经营、战斗或通用事件总线。

## 为3D保留的边界

逻辑格坐标、占地、访问和业务身份不依赖2D等距投影。先完成2D行为，再由3D renderer转换同一世界视图。
不提前引入3D引擎、ECS或材质/相机类型到核心；未来3D输入也需调用同样的领域命令与校验。

## 建设规则依据

建设已消费[设施规格](../research/dungeon_village_1/rules/FACILITIES.md#definitions)及[新局目录](../research/dungeon_village_1/rules/STARTUP.md)。已接入[地图访问](../research/dungeon_village_1/rules/MAP_ACCESS.md)及[邻接](../research/dungeon_village_1/rules/FACILITIES.md#neighbourhood)维护纯规则，本轮不宣称经营闭环。
已证：定义/实例身份分开；锚点式1/2/4格、两朝向；每个占用格绑定实例/定义/分片；
邻接已按来源实例去重、含对角、每外环道路格魅力+2；等级/改良/使用累计按定义共享。当前职业输入只覆盖初局已解锁定义，完整经营流程未实现。
连通是访问状态，不能自定“必须接路才允许建设”。
完整解锁、道路报价/覆盖/退款、新局剩余刷新/调度、施工更新资格与反馈继续等待研究；详情显示价格不等于建设价。
