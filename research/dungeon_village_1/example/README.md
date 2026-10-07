# 独立领域规则包

本包只有标准 C++17，不依赖 raylib、窗口、图片、APK 或平台坐标。
公开接口位于 [include](include/dungeon_village_reference/)，实现位于 [src](src/)，对应回归位于 [tests](tests/)。
[CMake](CMakeLists.txt)提供 `dungeon_village_reference` 库及对应 CTest 测试程序，不加入产品主构建；
当前程序数和当轮结果以[验证](../VERIFICATION.md)为准。

历史基线`2b479f6`已接普通人物／设施道具、商会及道路／移动／撤除的维护消费者，
该批Release标准、五项长测与三个有界研究窗口已通过；聚合Release入口123项标准及迁移窗口也已通过。
本批第一次自然地图扩张／新区经营及特殊格道路引用组合已独立验收；上述历史结果与更早`9897d64`的村办／95／晋级结果不替代本批检查。
商会、编辑及住宅重建属于[原型唯一Owner](../prototype/README.md)的组合，领域库不新增第二套世界。
住宅H是重建资格而非造价减免，重建仍付原800G；85定义预览已通过上述历史基线的Release合同验收，只读共享定义、没有实例邻接。

## 模块与依据

| 模块 | 职责与边界 | 规格 |
| --- | --- | --- |
| human_management | 职业／装备目录原交换排序、预览与有序拒绝、三档R奖励、中点／终点计划、大师加成、装备喜好矩阵与普通道具赠礼候选；普通道具按职业适配与品质各半评价，原品质10在1..9插值端点夹紧；不持有页面、随机或第二世界 | [人物管理](../rules/CHARACTERS.md) |
| [world_village_activity.hpp](include/dungeon_village_reference/world_village_activity.hpp) | 村办原序目录、51先季度次数后点数拒绝、52扣点及m／F候选、53恰70声音／满120确认计划；类型0单精度满足度、类型1按当前职业增加extra后重算；类型2全局人气交Owner，不冒充人物效果 | [村办合同](../rules/ACCOUNTING.md#下一批来源村办活动与晋级前置) |
| domain、simulation | R1事务、独占预约、单点自主模拟夹具；当前窗口不使用旧模拟 | [状态与建设](../rules/STATE_CONSTRUCTION.md)、[人物](../rules/CHARACTERS.md) |
| geometry、facility_economy、neighbourhood | 格坐标占地、经营推导、来源实例去重与道路魅力；真正共享等级升级扣旧门槛/保留剩余次数，旧/新/差额分别含实例邻接 | [设施](../rules/FACILITIES.md)、[建筑合同](../rules/STATE_CONSTRUCTION.md#共同世界建筑合同) |
| navigation、map_access | 加权寻路、完整设施绑定、距离场和到达身份 | [地图访问](../rules/MAP_ACCESS.md) |
| activity_choice、activity_candidates | 类别计划、候选生成/排序，票号由调用方注入 | [活动选择](../rules/ACTIVITY.md) |
| ranked_facility_choice、snapshot_facility_choice、regional_choice | 保留不同历史契约；重复计权、抽中项/目标格、六次区域回退 | [活动选择](../rules/ACTIVITY.md) |
| facility_departure | 普通设施分支的两级选择/回溯/绑定/初始朝向组合；不决定任务/救援/物体优先级、不执行运动或失败清理 | [首次活动](../rules/CHARACTERS.md#first-activity) |
| character_motion | 6.7世界单位位移、入口路点±40、矩形到达、向零截断和逻辑格进入；不收费/使用、不含表现投影 | [连续运动](../rules/CHARACTERS.md#continuous-motion) |
| facility_arrival、facility_exit、character_hp | 到达统计/现金候选、共享使用数/满足度请求、生命值目标与显示协议 | [设施使用](../rules/FACILITY_USE.md) |
| facility_use | 普通食物/旅店的占用请求、同轮等待扣减、旧计数170恢复与待处理退出；不提交完整退出或下一活动 | [使用计数](../rules/FACILITY_USE.md#use-timing) |
| facility_exit | 另提供完整绑定出口位置、普通活动0/属性/表情尾部顺序；不运行解释器或提交占用/属性 | [出口与下一活动](../rules/FACILITY_USE.md#exit-position) |
| weapon_choice | 武器/防具/饰品重选计数、不同rank窗口/空当前回退及票号；不装配/收费 | [武器与装备选择](../rules/FACILITY_USE.md#weapon-choice) |
| facility_events、facility_items | 实例事件门槛、定义共享改良与库存候选；独立改良候选供76首次初始化复用，75消费库存与推进实例计数不提前改共享J；原合一API保留 | [事件与道具](../rules/FACILITY_EFFECTS.md) |
| accounting | 即时金币、周期费用、报表快照、延迟点数与幂等身份 | [周期账本](../rules/ACCOUNTING.md) |
| actor_ai、ai_perception | G/e/F/K/L、人物/怪物等待、附近抢占、救援/回复目标与出发前置 | [AI感知](../rules/ai/PERCEPTION.md)、[生命周期](../rules/ai/LIFECYCLE.md) |
| combat_ai | 职业/武器策略、九格评分、物理/魔法候选与两侧影响场 | [战斗](../rules/ai/COMBAT.md) |
| combat_execution | 连攻/miss准备、动作窗口、怪物攻击位置、弹道/碰撞、延迟命中及HP/有序死亡请求 | [控制时间线](../rules/ai/CONTROL.md#combat-execution) |
| object_ai | 掉落选择、地上物体抛出/等待/授予/删除、H平局和state11完整拾取队列 | [地上物体](../rules/ai/LIFECYCLE.md#地上物体与拾取链) |
| encounter_ai | 组时点/清理/姿态、普通怪物数量及定义解锁抽取 | [遭遇](../rules/ai/ENCOUNTERS.md) |
| encounter_lifecycle | 完整事件分支/任务生成、胜利与页面有序请求、定义共享近期统计和延迟成长；不提交全局现金或职业升级 | [事件收尾](../rules/ai/ENCOUNTERS.md#完整事件候选与延迟成长) |
| actor_lifecycle | 计数状态、救援绑定/修复/跟随/释放、普通旅店双人原子到达、r清理候选 | [生命周期](../rules/ai/LIFECYCLE.md) |
| actor_control | 全34码校验、本地前缀、失败8、状态setter、漫游、装备显示/提交分离；全局副作用由所有者应用 | [控制解释器](../rules/ai/CONTROL.md) |
| ai_schedule | 实时名单两遍、反向/正向删除、设施内部自删后正向跳过、同轮追加、删除前设施释放；消费者只准备私有聚合副本，不是通用事件总线 | [聚合调度](../rules/ai/LIFECYCLE.md#聚合遍历与同轮新建) |
| actor_effects | cd/ce逻辑时间线、删除跳过、表情票号/抑制、动作/状态计数及命中标签过期；不绘图或直接播放声音 | [显示与延迟效果](../rules/ai/CONTROL.md#显示与延迟效果的实际推进) |
| human_growth | 定义共享六属性/装备/法术重算、九步奖励成长、多级/满级及职业解锁；授勋/住宅复用满足/努力/完成量即时奖励，跨十位才重算；不改变当前HP或代替Owner页面提交 | [定义成长](../rules/ai/CONTROL.md#人物定义重算与职业成长)、[建筑合同](../rules/STATE_CONSTRUCTION.md#共同世界建筑合同) |
| facility_service | 装备/携物价格前置、全部设施使用队列、完整退出前部/普通/装备/住宅尾部；救援哨兵必须交双人事务 | [设施事务](../rules/FACILITY_USE.md#设施服务组合与首段私有所有者) |
| actor_housekeeping | c前缀、状态18事件身份守卫、重力/K/退回旧位置、d尾部清理/超时；保留旧格与新投影读取点 | [共用更新尾部](../rules/ai/LIFECYCLE.md#共用更新前缀与尾部) |
| battle_commit | HP/状态、双方救援引用、定义共享统计、全局倒地/击杀和掉落ID一次提交；保留尸体重复命中 | [战斗提交](../rules/ai/COMBAT.md#跨所有者战斗提交) |
| object_commit | 物体/拾取人物原子提交、目录库存或装备解锁、商店通知、事件151/216及直接授予110差异 | [拾物提交](../rules/ai/LIFECYCLE.md#拾物与目录实际提交) |
| ai_rewards | 尸体删除/引用保留、战斗组实际提交、任务生成/胜利、延迟成长与全局职业开放；地图/页面/表情保持显式请求 | [奖励所有者](../rules/ai/ENCOUNTERS.md#死亡战斗组与奖励所有者组合) |
| encounter_creation | 普通事件地图/任务/重叠守卫、数量/解锁/介绍/实际生成及整批回滚；不补任务配额 | [普通创建](../rules/ai/ENCOUNTERS.md#普通遭遇创建事务) |
| rescue_commit | 共用人物所有者、追踪/双向绑定/返程/递归双人到达、定义共享B2、旅店使用退出；复杂分支明确交接 | [普通旅店世界事务](../rules/ai/PERCEPTION.md#普通旅店世界事务) |
| combat_commit | 状态1策略→组加入/实际九方向移动/攻击及法术队列，14–17与同次本地解释器续行；当前动作和武器独立 | [策略世界提交](../rules/ai/COMBAT.md#状态1策略的世界提交) |
| world_perception | 当前地图K/e/F、旧缓存城内外/低血量、引用修复/抢占、状态18、全局场与物理投影、实际d执行前段；不提前刷新s/t/ak | [共同世界感知](../rules/ai/PERCEPTION.md#共同世界感知与缓存)、[执行前段](../rules/ai/LIFECYCLE.md#实际执行前段与解释器续行) |
| world_encounters | 任务F创建/配额/下一次绑定、当前地图候选重建、事件bit2刷新与原时点场快照；声音/提示保持请求 | [任务与场事务](../rules/ai/ENCOUNTERS.md#任务创建与影响场地图事务) |
| world_facilities | 一次共同d前段/成长→携带表情→设施控制、类别8休息/类别6特殊入口/类别9退出；人物与怪物状态15旧B15/60/70 | [设施控制](../rules/ai/CONTROL.md#设施控制的共同执行组合) |
| world_actor_tail | 旧s的L/M→物理/K/新s→保留/清理/删除请求，调度原时点释放/退休；不把控制停留当删除 | [执行尾部](../rules/ai/LIFECYCLE.md#执行尾部的世界提交) |
| dungeon_ai、world_dungeon | 类别5进入/队伍/挑战/撤退/错峰退出、目录及抛出奖励原子候选、状态20直接落地续行；阶段2任务请求和实际成功统计/探索日期/怪物开放/任务池阈值 | [洞穴与迷宫](../rules/ai/DUNGEONS.md) |
| world_shop | 当前商店到达/携物、装备选择和收费、满足度/延迟人气、退出/属性/真实装配；显示仍显式交接 | [通用商店事务](../rules/ai/CONTROL.md#通用商店的世界事务) |
| world_control | 同一控制所有者的跨域FIFO续行、真实hold/true删除，阻止重复共同前段和等待扣减；不内置缺失消费者 | [控制组合](../rules/ai/CONTROL_COMPOSITION.md) |
| world_schedule、world_overlap | 轮首场/到访/提示/人气/S、两遍实时名单、共同前段/尾部、释放/退休与实际L；支持外层目录/任务/随机原子候选 | [共同调度](../rules/ai/WORLD_SCHEDULE.md) |
| [world_detached_actor.hpp](include/dungeon_village_reference/world_detached_actor.hpp) | UID-1独立W的严格身份验证；cleanup/departure/tail显式入口复用原主体，不改变普通名单约束 | [独立W](../rules/ai/WORLD_SCHEDULE.md#state2独立w对象接入与边界) |
| world_departure | 任务/救援/物体/事件优先级、全部活动、住宅/实际出口、两级票号及真实路线；到达不是选路成功 | [出发事务](../rules/ai/WORLD_DEPARTURE.md) |
| world_wander | 从旧s/当前或退休db、S读取中心，扩展10/12/13的原尾部；无引用零随机，不提前移动 | [真实漫游](../rules/ai/WORLD_SCHEDULE.md#真实世界漫游控制-101213) |
| world_misc_control | 实际c0..20、25恢复/显示、26共享定义与删除请求、32起跳、33旧u音效；不重复共同计数 | [杂项与状态](../rules/ai/CONTROL_COMPOSITION.md#回复离村起跳与拾物音效的实际消费者) |
| world_equipment_display | 27/29实际cd载荷、负年龄烟效和旧u；已发商店请求不再次消费控制，不装配/计时 | [装备显示](../rules/ai/CONTROL_COMPOSITION.md#27与29的装备显示记录) |
| world_lifecycle | 九个共享c分支及怪物17、真实b/c/r/HP/表情/路径组合，其他状态明确交接；共同20轮回归，不重新运行前段或d | [共同生命周期](../rules/ai/LIFECYCLE.md#共同世界生命周期分支) |
| world_daily、world_actor_routes | 日常/F/L、登场/拾物、全21状态及34控制同一人物/设施候选路由；实际队首提供当前输入，缺依赖拒绝 | [全状态](../rules/ai/LIFECYCLE.md#日常登场拾物与全状态路由)、[全码](../rules/ai/CONTROL_COMPOSITION.md#全码自动路由与当前随机接入) |
| world_dungeon_finish、world_map_refresh | 探索阶段2任务/奖励/全占地恢复及实际地图显示/道路围栏/补块/邻接；页面/脚本同步合同 | [探索](../rules/ai/DUNGEONS.md) |
| world_random、world_random_consumers | Java48显式种子或原始磁带、原nextInt余数语义、懒表情/两变体表；不猜APK默认seed | [随机与路由](../rules/ai/CONTROL_COMPOSITION.md#全码自动路由与当前随机接入) |
| world_scripts | 稀疏ID目录、实时延迟续体、事件调用计数与实际框架页栈；原表与消费事实分开 | [原始脚本目录](../data/scripts/) |
| [world_gift_page.hpp](include/dungeon_village_reference/world_gift_page.hpp) | 94的r3/5/6/7/8及固定脚本95的r0/1/3/4/9/10/11：计数／确认与金币、点数、定义解锁、标志、勋章候选；金币与勋章不额外通知34/21，不持有第二账本 | [奖励页面合同](../rules/ACCOUNTING.md#脚本奖励页面9495) |
| [world_notices.hpp](include/dungeon_village_reference/world_notices.hpp) | 唯一通知队首两项逆序计数/过期与声音11，21/19高度和整数展开/收回布局；不绘皮肤、不过度推进第三条 | [通知合同](../ui/PAGES.md#共同底部通知队列) |
| world_calendar、world_scene | 年/月/子周期有序合同与主场景1/2轮、真实资格/跳转/绘制门槛；实际日历域消费者必须另接 | [主场景](../rules/ai/WORLD_SCHEDULE.md) |
| world_month_report、world_calendar_maintenance、world_calendar_tasks | 月报/费用/点数、年度清理、跨月任务/等级提示、raw48真正晋级的有序规则及raw50全范围洗牌/布局；外部请求须同步消费 | [世界跨月](../rules/ai/WORLD_SCHEDULE.md#实际跨月域与组合入口)、[晋级](../rules/ai/WORLD_SCHEDULE.md#城镇真正晋级与庆典) |
| [world_award_page.hpp](include/dungeon_village_reference/world_award_page.hpp) | 年度raw87贡献/交换排序、终止与授予的独立询问绑定、扣唯一勋章/奖励请求、raw88旧局部计数与关闭抽选、raw67空窗口推进；Owner执行真实脚本/共享奖励，不宣称原窗口模态等价 | [年度授勋](../rules/ai/WORLD_SCHEDULE.md#年度授勋最小闭环) |
| world_task_creation、world_facility_update、world_residence | 真实任务工厂/全占地、设施前缀/施工/共享人气、住宅现有人物奖励和原程序 | [探索](../rules/ai/DUNGEONS.md)、[共同调度](../rules/ai/WORLD_SCHEDULE.md) |
| [world_task_commands.hpp](include/dungeon_village_reference/world_task_commands.hpp) | 玩家页23/24募集费用、原序全列表随机/入场/队伍提交、页27当前候选与追加费用、页28预测/动画/正式任务启动；回主场景、消息与页面同步请求，取消/拒绝及晚期失败不留部分世界或随机 | [玩家接受与出发](../rules/ai/DUNGEONS.md#玩家接受募集追加与正式出发) |
| [world_task_display.hpp](include/dungeon_village_reference/world_task_display.hpp) | raw99确认40门槛与raw100共享8×9演出表、19抽初始化、逐行更新和统一随机候选；不生成怪物实例、不重复本帧更新、不承担实际战斗结算 | [演出与成果后的阻塞点](../rules/ai/DUNGEONS.md#演出与成果后的阻塞点) |
| [world_task_deadline.hpp](include/dungeon_village_reference/world_task_deadline.hpp) | raw33费用快照守卫、继续/中止动画与栈外原页返回；主场景重新汇总当前费用、同步月账本和完整中止请求，关页不提前扣款 | [期限页与返回主场景](../rules/ai/DUNGEONS.md#期限页与返回主场景) |
| world_popularity、world_nonactor_schedule、world_runtime | 实际奖励/新闻、bo/bp/bn/L路由、主场景至真实工厂的同一私有Owner组合；无缺依赖默认成功 | [人气和脚本](../data/scripts/)、[组合入口](include/dungeon_village_reference/world_runtime.hpp) |

`prepare_*` 纯函数返回候选值，调用方负责跨域原子提交及事件去重。
`GlobalState` 是早期安全夹具，不能与当前原型聚合或原作初值混用；R1净额结算不能与即时现金账本叠加。
头文件注释维护输入、所有权、失败与返回语义，非直观计权/时点在实现处补注释；不逐行描述赋值。

村办领域回归扩展现有[成长套件](tests/human_growth_test.cpp)，页面51—54的唯一Owner事务、载荷拒绝和退休
由[原型](../prototype/README.md)的现有页面套件承担。类型0／1／2已有已验基线，类型3地图扩张已由唯一Owner组合
现有地图／占地／邻接／人物重置函数，第一次自然扩张及新区经营已通过；第二次及封顶仅属组合验收。
类型5／6已由[魔法壶Owner](../prototype/include/dungeon_village_prototype/startup_world_magic_pot.hpp)接线并验条件组合，类型4仍明确拒绝。
`9897d64`村办／晋级及`2b479f6`普通工具／编辑验收均保留为历史。最新结果及各类型覆盖边界见[当前验证](../VERIFICATION.md)。
普通道具计算继续扩展成长与设施道具套件；库存、页面恢复、随机、同定义实例刷新和失败回滚由原型现有页面／建筑套件主责。
自然工具经营与晋级后的编辑尾段各有显式连续模式，组合夹具、自然玩家路径和简化窗口分别登记。

[world_magic_pot](include/dungeon_village_reference/world_magic_pot.hpp)为实际纯C++17模块：原13槽投入、日期处理、两次整数舍入、
配方目录／发现及43资格／47成本分别返回只读候选；不持有Owner、窗口、脚本或随机。
恢复用只读状态校验，不以执行处理来补字段；维护溢出／坏状态拒绝与原版事实分开，来源见[合同](../rules/MAGIC_POT.md)。
投入评语由唯一Owner抽4／3／2票一次；规则测试归现有facility_items管理道具套件，脚本／页栈／奖励风险留原型主责。
本批邻接新增显式`NeighbourRoadBinding`输入：仅真实占地、实例／定义／片号和道路格全部吻合的绑定允许两种影响并存，
原无绑定入口仍拒绝道路与实例重叠；不是全局放宽布局校验。

## 构建与检查

日常从仓库根目录使用[研究聚合入口](../CMakeLists.txt)，与tools、prototype共用一套Release构建树：

```sh
cmake -S research/dungeon_village_1 -B research/dungeon_village_1/work/release -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
cmake --build research/dungeon_village_1/work/release --parallel 2
ctest --test-dir research/dungeon_village_1/work/release -LE long-world --output-on-failure
```

聚合构建需要原型使用的Node、动态raylib及pkg-config，配置方式见[原型构建说明](../prototype/README.md#构建与运行)。
prototype已包含本包的87项领域测试，聚合入口不重复注册；连同31项原型及5项工具测试，共123项唯一标准测试。
长测按风险显式启用并单独以`-L long-world`执行。Debug只在定位问题时使用`work/debug`，不再每批重复六套验收。
本包[CMake入口](CMakeLists.txt)仍可独立配置以检查标准C++17依赖边界，但不常驻另一份重复缓存。
本机显式工具链路径见 [环境基线](../verification/BASELINE.md)，
本轮结果见 [当前验证](../VERIFICATION.md)。测试覆盖严格错误、边界、重放、随机差分及组合链，
不替代固定 APK 动态认证。

本地Debug和Release均默认动态链接，策略由[共享构建模块](../cmake/ResearchLibraries.cmake)统一维护。
可执行文件及运行DLL放本构建目录`bin/`，导入库／静态归档放`lib/`；同一领域DLL由各测试及原型共用。
不同构建树不共写DLL目录，按需Debug构建自己的匹配库；进程收齐后清理不再需要的临时构建树。
Windows使用目标架构的libc++／unwind／winpthread，按导入库位置查找并验证PE架构，不从宿主PATH猜DLL。
旧缓存若含`CMAKE_EXE_LINKER_FLAGS=-static`会明确拒绝；收齐该缓存进程后显式加`-DCMAKE_EXE_LINKER_FLAGS=`重新配置，
不静默覆写已有flag。只有独立向玩家发布的Release制品才显式选择`BUILD_SHARED_LIBS=OFF`并配置相应静态运行库；
日常Release回归不采用该例外。切换后的验收与体积记录见验证入口，不沿用旧静态结果。
