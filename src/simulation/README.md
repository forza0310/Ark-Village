# 世界规则与运行时

当前冻结来源、模块映射及产品适配以[SOURCES](../../assets/simulation/SOURCES.json)为准。核心仅标准C++17，不依赖raylib、窗口或屏幕坐标；运行不读取research、APK或研究work文件。各批来源与验证历史集中在[B1](../../docs/stages/B1-playable-prototype.md)，当前产品边界见[架构](../../docs/ARCHITECTURE.md)。

代码按游戏大模块组织，公开头在 `include/ark/simulation` 下对应目录。模块内 `rules/` 保留纯规则层，其余实现负责唯一Owner的查询、消费者与事务。目录归类不改变命名空间或DLL依赖，也不建立额外世界。

| 模块 | 职责 |
| --- | --- |
| [facilities](facilities/README.md) | 建设、道路编辑、邻接经营、设施使用、商品/强化、住宅及魔法壶 |
| [actors](actors/README.md) | 人物生命周期、移动/路线、HP/成长、职业/赠礼及主角操作 |
| [ai](ai/README.md) | 感知、行为选择、队列调度和设施/装备候选 |
| [combat](combat/README.md) | 战斗执行、伤害提交、遭遇生命周期、物体与救援 |
| [tasks](tasks/README.md) | 任务生成/征集/出发、期限、迷宫探索和成果 |
| [map](map/README.md) | 格坐标、占地、通路、地图恢复/刷新和扩张 |
| [village](village/README.md) | 账本、日期/月报、活动、人气、授勋、税收、事件、情报及计分 |
| [world](world/README.md) | 聚合状态、初始化、随机、场景/页栈、真实导航及保存页协调 |
| [persistence](persistence/README.md) | 维护世界编解码、恢复校验、文件原子替换 |
| [application](application/README.md) | 维护标题/应用控制器、系统纪录、存储桥、完整回放及继承 |
| [presentation](presentation/README.md) | 只读皮肤计划、声音请求与显式表现消费者 |

`ark_world_rules`负责规则计算，`ark_world_runtime`持有唯一 `StartupWorldRuntimeState`：地图、账本、随机、共享定义、人物、任务和页面。route/script/finish等投影仅供当前事务使用；候选完整成功才提交，失败不残留扣款、名册、随机或部分页面状态。到达收费、退出效果、升级提示与真正升级分别沿源消费者，最大HP变化不等于恢复当前HP。

导入路径由 `scripts/simulation/module_paths.mjs` 显式维护，CMake源清单不用GLOB。`assets/simulation/SOURCES.json`记录正式来源与产品适配；导入器先冻结依赖闭包，再翻译命名空间、头路径及测试资源根。已有适配需要显式重基，不因研究更新覆盖独立候选、随机顺序、拒绝/回滚断言。持续构建所需来源登记与一次性交付指纹脚本分别处理，后者不进入Git。

## 页面与表现边界

`startup_world_menu`维护真实raw3/4/7/10的冻结目录、父页、选择、位置和展开计数；`startup_world_information`维护信息9、城镇34、冒险者35、收支36、物品37、装备38、设施39及赠礼人物40。设施39保存64位实例目录，人物列表保留定义身份；35→60追踪另存来源资格与可选实际人物实例。任务58按当前任务site推进镜头，不由绘制线程移动世界。

`startup_world_save`仅维护raw14请求/结果与严格稳定导出，不在规则回调访问文件。`startup_application_world_save`在维护应用内对接四目录；产品会话另将两个玩家栏位对接自己的codec。设置12、说明13、结束或其他未交付平台消费者保持拒绝，不用可关闭空页冒认实现。

Steam皮肤模块生成只读绘制/触摸计划，桌面层执行资源与输入映射。维护计划可调用不等于窗口验收通过，接线及结果按本批B1记录。标题20槽和有副作用表现请求仍按自身准入合同处理，不能挂到60FPS绘制时钟；只读建筑/人物/数字计划不推进奖励、装备或动画计数。

月报按源70/70阶段自动关闭并一次消费点数，不要求确认、不额外冻结世界。人物、施工、日期及随机是否更新依源场景/框架资格，显式暂停保留。插值与日期周内比例只是显示，不另建逻辑时钟。

## 维护格式与玩家格式

当前维护格式为 **AVRSAVE1世界语义5／AVRAPP01应用语义8／AVRSYS01系统2**。世界维护codec保存完整Owner、随机与账本；回放另存有序审计及外部控制器，应用回放同时保存标题/系统/文件视图。恢复先验证私有候选及控制器，再共同安装；完整外轮快照不是原跨周中断轮内续体。初始设施p2、任务NEW及菜单字段更新后，旧语义或布局拒绝，不迁移或重签历史前缀。

产品玩家存档独立为 **ARKSAVE1/schema5，两栏手动档**，保存任务实例 `newly_available`、配方进度、主角资料及耐久业务字段。随机不保存，加载沿用当前会话随机/暂停；导航与建设NEW缓存冷载清空，后续真实Init刷新。仅稳定主场景可捕获；raw14通过正式纯导出产生稳定候选，不扩大为任意模态保存。旧schema/数据身份拒绝，不自动删除用户文件，详见[玩家存档](../app/save/world_save.md)。

`ark_world_persistence`依赖唯一运行时及纯C++哈希模块，完整维护应用目标保持独立；玩家程序不因保存页接线而依赖维护全Owner随机codec。系统文件原子替换与玩家世界文件是分别的事务，不宣称跨文件联合原子性。

## 入口与验证

默认桌面及显式 `--world` 使用同一持续世界。标准C++入口 `ark_world_simulation --frames 20000 --months 3 --seed 1 --auto-confirm` 中frames代表框架调用，自动确认是显式测试输入。旧 `ark_game`、`--legacy-slice` 及专属诊断已退役，不恢复第二套村庄Owner。

标准三个月基线保留原参数与有效断言，只证明对应输入下持续运行；额外自然任务/年度检查按风险显式启用。完整经营验收需要真实建设营业、活动培养、任务胜利、条件升星、新解锁消费和跨进程存读档，逐项区分取得、消费、效果及后继，不能以日期推进或维护回放自洽替代。执行配置与研究/产品分工见[开发流程](../../docs/CONTRIBUTING.md)，覆盖与策略见[主动经营计划](../../docs/stages/ACTIVE_VILLAGE_PLAN.md)。
