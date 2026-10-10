# 三至五星功能覆盖审计

2026-10-10。先只读检查当前合同、维护实现和测试源码，再按主会话分工补两处最小组合测试；本文件不是本轮执行结果。按当前研究／产品分工，research负责短时功能、逻辑与Owner组合验证，真实经营长跑由产品负责。未做高星自然长跑不等于功能尚未实现。本分支没有构建、改原表或修改测试注册，由主会话统一验收。

依据为[高星路线合同](../../rules/PROGRESSION_ROUTES.md)。该合同以固定APK1.0.8为来源；下列维护测试不能代替Steam窗口认证。

## 增补前的实现与覆盖审计

|链路|维护消费者|现有主责测试与实际边界|尚未证明的组合|
|---|---|---|---|
|三／四／五星实际晋级|[晋级规则](../../example/src/world_calendar_tasks.cpp) `prepare_world_rank_promotion`；[Owner页48](../../prototype/src/startup_world_runtime_pages.cpp) `act_startup_world_runtime_rank_page`|[world_calendar_tasks_test](../../example/tests/world_calendar_tasks_test.cpp) `rank_promotion_and_celebration()`遍历旧rank0..4，校验前后脚本顺序、五星47→46、拒绝与随机；[startup_world_pages_test](../../prototype/tests/startup_world_pages_test.cpp) `rank_promotion()`覆盖真实Owner首星、日期、活动j开放、重入拒绝和失败回滚|当前Owner专责场景以首星为主；规则层遍历全部星级不能替代高星奖励接线|
|三星学校63领取|脚本事件44→等待3→指令34创建95类型3；Owner95调用现有设施奖励写回|`world_scripts_test::all_fixed_programs()`跑所有固定脚本及续体；`startup_world_pages_test::unlock_rewards()`覆盖95满40、类型3设施领取，但设施使用定义0|未见真实事件44等待边界、63领取前未开放／领取后开放的同一Owner场景|
|学校63完工→活动7|[设施完成](../../example/src/world_facility_update.cpp)首次flags事件73；[脚本17](../../example/src/world_scripts.cpp)开放活动|[world_facility_update_test](../../example/tests/world_facility_update_test.cpp) `construction()`验证首次完成、重复不执行、活动7/13/21/22、晚期缺活动整体回滚；`world_scripts_test::all_fixed_programs()`显式事件73/74|完工测试使用定义35附多种flags的条件夹具，未见真实63表行→Owner建设→完成→7的短链|
|四星事件45→95领取活动26|指令33创建95类型11，领取后开放活动定义|所有固定脚本续体可运行；`unlock_rewards()`类型11证明只开放，不增加举办数或收费|未见真实事件45等待30、26领取前后状态专责断言|
|活动26第二次扩张|[村办](../../prototype/src/startup_world_village_activity.cpp)与[地图扩张](../../prototype/src/startup_world_expansion.cpp)|`startup_world_pages_test::village_expansion_pages()`已有`{26,1,2,200}`：以真实扩张得到前置level1，51不扣点；52扣200、增加m/F；53在119拒绝效果、120扣q并level2；无54，重复拒绝、页面退休。另有level2封顶与第一扩张晚期回滚|第二次扩张的功能消费已有覆盖；不应机械再写同等用例。只需上游事件45／95交接|
|四星商会85→博物馆64→93领取|[商会](../../prototype/src/startup_world_commerce.cpp)目录过滤、点数支付、93领奖|`startup_world_pages_test::commerce_facility_and_projection()`使用真实34冷饮店，覆盖目录、定义预览返回、85支付与93领取分时点、40门槛、插页失败回滚、重复拒绝|未见64的四星过滤、200点与真实列32／列12组合|
|博物馆64领取→建设→完成→活动21|普通建设报价／提交、设施完成事件75和新闻215、指令17|[startup_world_building_test](../../prototype/tests/startup_world_building_test.cpp) `normal_construction()`覆盖真实普通建设与完工；通用`construction()`覆盖事件75／215及活动21|未见64的3000G报价、领取不等于免金币、完成才开放21的短链|
|五星事件46→职业21／22开放|脚本31直接开放共享职业并创建95类型4；[Owner投影](../../prototype/src/startup_world_runtime.cpp) `write_startup_world_runtime_scripts`同步AI职业unlocked|[world_scripts_test](../../example/tests/world_scripts_test.cpp) `all_fixed_programs()`包含自定义702的`31,21`／`31,22`，验证职业状态及图标extra；`unlock_rewards()`验证类型4 Owner同步；`human_profession_and_mastery()`另管真实转职时序|未见真实事件46等待30→31→Owner两份状态同步，且人物名单／当前职业不变的组合断言|

## 必须保留的语义

- 三星资格：当前人气1500、历史最高月收入35000、实际活动F至少15、存在西餐厅40。四星资格：人气2500、有效kind3／9设施25、独立有效kind12住宅10、任务成功30。五星资格：人气3500、历史最高月收入70000、F至少30、存在博物馆64。设施／住宅计数不能混合。
- 三、四、五星按活动j直接开放18、19、20，与延迟奖励学校63／活动26不同。
- 学校和博物馆的卡片领取不生成地图实体；对应活动7／21在施工完成后才开放，开放也不等于已经开展。
- 博物馆85扣200村点与93满40领奖是两个成功时点。后续领奖失败不应退回先前已成功付款；类别3实际建设另扣3000G，H不能用作免金币依据。
- 职业31与活动33时序不同：31执行时已置共享职业status，再展示95；33仅创建待领取95。不要为了统一奖励UI而把职业开放延迟到95确认，更不能凭职业开放生成冒险者或切换现有人物职业。
- 三星／四星等待3／30是脚本逻辑更新，不能换算墙钟秒数。条件快照须明确夹具边界，不虚称自然满足资格。

## 审计时的最小缺口（已按下节集中补齐）

1. 扩展现有`world_scripts_test::all_fixed_programs()`的具名场景或同文件局部函数：使用真实事件44、45、46，核等待前一轮／到期、95载荷、职业31即时效果。不复制所有200事件执行循环。
2. 扩展现有`startup_world_pages_test`：高星Owner晋级后的领取／投影短链；沿已有辅助函数退休父子页和消费一次性输出。活动26最终效果复用既有覆盖，只补45→95→26开放的桥接。
3. 在现有商会场景表驱动补64：rank3不列、rank4列、199点拒绝／200点成交、93领取前后。建设及完成由现有building套件主责，以真实63／64定义表行准备最小条件，不重写底层flags全排列；核未完工不开放、首次完工正确活动及重复不触发。
4. 功能批次完成后集中运行受影响套件，另核资源规模、页面／绑定退休、表现输出消费；不新增逐链target，也不为这次维护交付启动完整高星长跑。

以上是测试缺口，不是新增规则或持久化格式需求。现有设施p/q/r/G/H、活动状态、脚本续体及共同Owner字段已承载这些功能。自然三至五星路线、经营策略成效与长期历史增长仍由产品长跑报告，research接受最小复现定向修正。

## 本批交给主会话的最小实现

- `startup_world_building_test::progression_building_unlocks()`：使用真实63／64表行，只准备领取后的p2／H1条件，调用Owner建设、取消建设模式、逐次设施更新；验证博物馆另付3000G，完工前不开放，恰到门槛开放7／21，首次事件73／75及博物馆新闻215，不增加实际举办数、随机或重复实体。没有再次实现95或商会测试。
- `startup_world_runtime_test::final_rank_profession_unlocks()`：真实事件46及30轮脚本续体，经Owner写回，校验21／22开放时间和AI职业投影；95未确认已经开放，人物所处职业、出现状态、资金与随机不变。
- 上述两项已由主会话统一构建，通过Release的runtime／building两个现有CTest，记录见[首次高星检查](high-stars.log)，总计1.88秒。它们是条件功能检查，不是自然高星路线。
- 后续扩展同一runtime套件的`delayed_rank_rewards()`，表驱动真实事件44等待3→95/r3/s63、事件45等待30→95/r11/s26，经Owner领取前保持锁定、早确认到40仍锁、满40确认开放；校验学校不自动建设、活动26不自动举办或扩张。该增补已重新构建并通过runtime 0.05秒，见[末次日志](runtime-final.log)。
- 原pages商会套件追加真实博物馆64：rank3目录排除／rank4纳入、199点触发原拒绝但不扣款／不开资格、200点先支付且金币不变、93/r3/s64满40才p2／H1，实例数和next_instance不变。与二星壶组合统一末次pages通过2.08秒，见[独立输出](pages-verified.log)；没有重复34的通用预览／回滚或building的3000G建设检查。
- 均扩展现有职责套件，不增加target。上表保留增补前审计，当前状态以本节及主会话最终验收记录为准。

当前功能层面，学校／博物馆真实表行完工、五星职业投影、44／45真实领奖桥接及64商会精确门槛均已有本轮通过结果；第二次扩张26随现pages套件保留验收。晋级与各领取／建设消费者的维护实现均已存在。产品侧剩余职责是连续真实经营、自然满足高星资格、解锁后实际消费及长期资源增长验证；本轮短测不替它认证，也不宣布其它未审主题已完成。
