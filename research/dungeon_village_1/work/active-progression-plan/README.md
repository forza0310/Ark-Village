# 世界4／应用7之后：主动经营、BOSS与日期通关的复用计划

2026-10-09。只读核对当前Driver、Owner公开接口和[后期路线合同](../../rules/PROGRESSION_ROUTES.md)。世界4／应用7为当前在途价格修复身份，是否已完成集中验收以主会话交付为准。本文不改C++／快照格式／预算，不构建、不启动长跑；来源hash见[EVIDENCE.json](EVIDENCE.json)。

2026-10-10后续：主会话已安排下文“应用接线”最小实现，17项转发已写入公开头与现有application.cpp，**尚待集中编译／集成验收**。下文原只读分析和EVIDENCE保留接线前身份，不重签旧源码hash；新增具体接口与验收范围见文末。未增加持久字段，继续世界4／应用7，未生成主动Driver或主动自然前缀。

## 应优先执行的下一条路径

**先认证一条当前世界4的主动首星短前缀及短尾段，再沿同一主动策略推进第二星；被动180月独立保留为计分/保存成本路线。** 现有被动应用Driver没有建设、接受任务和真正晋级命令，无论跑多久都不能拿它证明高星或六BOSS。

下一次可直接执行且不需要新格式的动作是现有continuous可执行的 `natural_progression` 有界420轮捕获→440轮reference→两个新进程各续20轮。它复用已登记世界回放Driver和原经营命令，仅重新认证被价格修复退休的最短主动前缀，不要求先重跑完整首星或旧12月。完成后以独占分段目录继续到本次真正需要的晋级/任务边界，周期文件先称候选，匹配reference尾段才升认证。

现有CLI参数已核，可由主会话使用已验的单Release动态树执行；下面`EXE/OUT/REV`是待绑定变量，本文未执行：

```text
EXE natural_progression 1 0 --save-at 420 --save-file OUT/prefix420.awr --stop-at 440 --trace-from 421 --trace-file OUT/reference.trace --producer-revision REV
EXE natural_progression 1 0 --load-file OUT/prefix420.awr --stop-at 440 --trace-from 421 --trace-file OUT/restore-a.trace
EXE natural_progression 1 0 --load-file OUT/prefix420.awr --stop-at 440 --trace-from 421 --trace-file OUT/restore-b.trace
```

同一批确认摘要、完整Driver字节、逐轮typed音频三路一致后再保留证书，不把成功加载当续跑等价。`frame`是该Driver从0起的外层轮号；不能直接套被动应用Driver从1起的轮号或证书。三个进程按依赖顺序执行，不同时运行三份长前缀。

## 已有消费者、当前Driver与实际缺口

| 范围 | 可直接复用的实现／真实命令 | 尚未闭合的组合 |
| --- | --- | --- |
| 应用被动日期 | `startup_application_natural_replay.cpp`，`application-natural-clear-v3`／`AVNATDR3`；真实新局、更新、对话、自动页、年度结束、48返回、83离开、17计分；一次性音频/资源峰值/引用审计；完整应用三路恢复runner | `plan`在scene只wait，绝不接受任务/建设/真正晋级；这里只能证明日期通关与系统计分 |
| 主动首星／扩张 | `startup_world_continuous_test.cpp:natural_progression`；真实raw21面包房35、设施详情→81升级、村办51–54、任务22/23/25/28/33、48晋级、年度87；已登记世界回放Controller `natural-progression-expansion-v2` | 当前目标在首星后活动16、营业、道路/移动/撤除尾段结束。非扩张模式到终点就terminal；扩张只再完成活动25与新区域道路/营业。不能把现目标参数改称五星 |
| 住宅 | 同文件`natural_housing`：募集定义24真实100G、人物礼物/装备、达满足度→raw80入住→施工→授勋→原日期税收 | 当前只认证一住宅，策略优先人物1短剑赠礼；第二星需要四住宅，不可复制一个住民或注入满足度 |
| 普通道具／商会 | `natural_tools`：真实库存、赠礼、设施道具、商会买卖/领取及营业 | 单条工具链有消费者，无与高星资金/装备/住宅计划联合的可恢复Driver |
| 魔法壶 | 村办类型5/6及壶既有Owner与规则回归；二星事件43→95领取活动30→52/53真实开展 | 自然二星→导入壶→实际投入/处理/发现/炼制未连成自然路线；不能先给配方/点数 |
| 高星 | 48/49条件与晋级、95领取、活动开放、建设、任务成功消费者已在 | 三星西餐厅40/收入35000/F15、四星25设施/10住宅/30成功、五星博物馆64/收入70000/F30的主动策略与自然认证缺失 |
| BOSS | 真实任务池目录、G阈值、六特殊任务选择、战斗/救援/期限/成果、x推进、延迟152奖励、最终新闻217均有来源及维护消费者 | 未有逐章装备/培养/参战策略与自然胜利证书；不能把六特殊任务目录已初始化写成六BOSS已胜 |

这里“消费者可复用”不是“当前新语义已重新自然验证”。旧用例中明确保留的源边界和拒绝断言继续有效，过时的轨迹黄金另按下节处理。

## 应用接线是关键缺口，不能强换被动Driver身份

`StartupApplication::world()`只返回const Session。应用公开世界命令目前只有更新/确认、年度、晋级返回、商会离开、保存/加载；**没有建设、村办、任务、住宅、装备/道具和真正晋级的应用转发**。世界Session已有这些命令，应用层缺口是把它们通过现有`commit_world`事务与系统纪录、一次性音频一并提交。

因此把世界`natural_progression`代码粘到被动应用Driver中尚不能直接运行，也不能const_cast取world、逐轮导出再重装、或用generic acknowledge冒充管理命令。最小后续实现应给当前要用的真实命令增加有类型的应用转发：先复制Session候选→执行已有命令→成功后`commit_world`。复用已有Owner失败/系统写入失败回滚，不新增任意状态注入接口或另一套经济逻辑。

主动应用策略优先只接首星必需命令：建造目录/放置/退出，详情/升级，村办选择/确认，任务选择/接受/出发/续期，真正晋级。随后扩到四住宅所需的人物礼物/住宅选择。不要一次铺所有晚期命令，先取得应用层一个真实建设→营业→保存/双恢复尾段，核新的收入也同步系统最高资金与音频。

主动策略和被动策略必须有不同Controller身份。可沿现AVRAPP01的已授权Driver分区保存规范命令、阶段和必要策略计数，复用128MiB预算及轮末边界；这不需要改系统2/世界4字段或新容器。本文不定义新二进制字段表，不把修改既有Controller字节或重签证书当自然来源。

当前被动420轮／首月前缀可继续服务**同一被动策略**；主动Driver的第一条操作更早，不能直接继承其证书。`load_world_replay`虽可在标题导入已校验世界回放，但会经过加载/应用激活，且不带原应用的系统纪录与Driver，不能当精确应用自然前缀转换器。

## 推进优先级和成功门槛

1. **首星主动基础与恢复**：按前述现有world短命令先确认新语义；应用管理转发接入后建立独立主动应用Controller，从真实新局一次生成最短前缀。保留原操作条件，三路尾段相同后才向后推进。
2. **第二星**：从主动首星完成点继续，逐个真实募集/赠礼/达门槛/入住/施工，形成4有效住宅；补到10有效一般/住宅设施，累计12次实际任务成功、人气800，48真正晋级；等待事件43真实延迟、95满40领取、活动30满120提交。四条件、钱点支出、页面退休和壶入口都核，不以只看到二星说明页完成。
3. **BOSS首章，与三星经济并行定向取样**：继续真实任务提高G；跨第一年探索限制后自然生成39，实际接受、征集、培养/装备、出发、胜利，核x推进、任务成功与152续体在当时x取奖励，再95领取。失败走原救援/续期/中止，不提高角色值凑胜。首BOSS成功先保存短尾段，后续按47/55/63/72/80分别认证。各章资金与装备消费也反馈经济计划。
4. **三星→四星→五星**：依真实条件加设施、活动及住宅。四星到博物馆要经过85支付200村子点数→93满40领取→另付3000G建设→施工完成→事件75开活动21；不能只发一张64卡片。完成五星与六BOSS分别标记，不把其一代替另一项。
5. **日期180月及计分**：被动支线只在当前已认证前缀上分段前进，先下一个有价值月界/预算采样点，避免每次从零；主动支线成熟后才再验证其自然raw17。原零基15/3触发17，实际UI16年4月；stage0/3/6和ED后经营各取必要短尾段。日期通关不要求五星，五星也不等于日期计分完成。

第二星是当前最小新增完整功能目标，不应先连续挂到180月才发现住宅／参战策略未接。预算允许的确定动作可自主调整，例如只选实际可支付任务、保留续期/住宅费、已知恢复月间隔；不修改原费用、角色AI或世界字段。策略变化若影响下一命令或前缀，更新Controller资格并保留原因。

## 身份、黄金值和预算限制

- 本轮价格修复使世界3／应用6历史退休；只能读取其报告学习，不能用旧DLL转存、加载暗补、改头或换schema续接。现世界4／应用7短前缀必须有本次证书。
- `natural-progression-expansion-v2`的策略未变时不必机械改其payload格式，但新的世界语义会拒旧世界；若扩为第二星/BOSS且改变策略或terminal语义，需要独立Controller/验证规则，不能继续声称同一v2路线。
- 现`natural_expansion`在首星编辑终点还断言seed1/speed0的`frame38282、23388G、322697抽`，结尾另有历史黄金。它们来自修前轨迹；本批邻接收费、已先改变的任务池/音频均可能改变路径。**先把旧数字作为历史身份保留，在完整来源和独立不变量不变前提下重新建立当前预期；不能直接删断言或放宽容差。** 短420尾段没有证明后续这些黄金仍成立。
- 当前应用被动Driver固定seed1/speed0、两百万轮／十万轮停滞界和180月目标，未知页立即报错；不能为加速改内存日期、随机、AI速度或把墙钟时间换逻辑tick。无窗口headless已消除帧率等待，剩余是模拟/历史成本。
- 全Session审计历史通常每月新增4份，现金流水和任务历史继续合法增长。旧35MB级主动首星快照仅是旧身份规模线索；联合128MiB分配/16Mi节点可能先于180月耗尽。每个候选同时记录文件大小、历史数、保存/恢复耗时、引用/输出及资源峰值；接近预算就冻结当时证据并完成可做短尾段，不删除有效历史、抬预算或假称永久有界。

本包只新增本文与小型源码hash索引，没有生成世界blob、构建缓存或后台进程。具体实现与三路认证由后续批次报告，本文不预报通过。

## 应用管理转发：本次已写接口与集中验收范围

实现仅修改[公开应用接口](../../prototype/include/dungeon_village_prototype/startup_application.hpp)与[现有实现](../../prototype/src/startup_application.cpp)，未新增源文件／target／Owner字段。既有授勋、晋级返回与商会退出继续使用已有消费者。

| 本次新增公开方法 | 返回值和参数 |
| --- | --- |
| `open_build_menu`、`cancel_build_menu(page)`、`cancel_build` | string，非空表示事务错误 |
| `select_build_menu(page,definition)`、`confirm_build(anchor,orientation)` | StartupApplicationBuildResult |
| `open_facility_page(instance)`、`act_facility_page(page,action)` | string |
| `act_residence_page(page,human,cancel=false)` | StartupApplicationBuildResult |
| `open_village_activities`、`act_village_activity_page(page,action,selection=0)` | string |
| `open_task_menu`、`open_task_control_menu` | string |
| `act_task_page(page,action,selection=0)` | StartupApplicationTaskResult |
| `open_human_page(human)`、`act_human_page(page,action,selection=0)` | string |
| `act_rank_page(page,selection=0,cancel=false)`、`cancel_page(page)` | string |

两种新结果都是非持久命令回执：Build包含`error/denial/created`，Task包含`error/denial/accepted/departed`。**非空error代表没有提交；空error但有业务denial不代表可以自动重试付款**，不足资金等原业务拒绝可能已合法生成提示页。任何Session错误、异常或应用提交失败都清除created/accepted/departed回执，不能把未提交候选ID交给下一动作。

所有新入口经私有`apply_world_action`检查应用健康、活动世界与无计分页，再复制Session候选，调用既有Session资格／领域命令；成功后复用`commit_world`同步系统最高资金／继承与一次性音频。计分页17未初始化时也显式拒绝管理输入，不依赖clear字段已经建立。没有公开任意callback、可写world、直接第二世界或状态setter。商会购买、魔法壶和道路编辑不在本次新增接口清单。

集中验收由既有应用测试承担：真实新局经应用目录建设/取消、活动选择及任务菜单资格；合法成功声音从Session转至应用并只领取一次；缺源/错页及提交失败保持完整应用、Driver、随机、音频和系统目录；构建/任务错误回执不得残留候选身份。与原Session底层测试分工，不逐转发函数新建target；当前`diff --check`通过，尚未以构建或回归认证这些新接口。
