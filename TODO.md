# 当前待办

当前实现和边界见[文档索引](docs/README.md)、[原版对照](docs/reference/REFERENCE_CHECKLIST.md#research-history-current-audit)；已完成阶段与测试结果按批次查[B1](docs/stages/B1-playable-prototype.md)。

## 本批收口

- [ ] 商会建筑一次性解锁：已复现旅店/面包房等8种初始可建定义仍列入85，包子铺实际20点购买后仍可再次入目录。当前研究明确`p!=2`，与用户确认行为冲突；具体最小复现、修正范围及验收见[研究交接](docs/reference/RESEARCH_REQUESTS.md#commerce-building-blueprint)。产品尚未改规则，未改玩家档。

- [x] b99c6db正式维护及信息接入：442项来源、708项素材、28处适配；正常信息9/35–38和35→60追踪的FIFO/渲染接线完成。desktop-debug139/139通过；headless首次118/119、旧scene夹具补正后6项定向通过，均仅排三个月。Release七窗口及月51玩家业务检查点冷载通过；玩家schema4/身份保持，34/39与完整动态仍待后续。具体边界见[B1](docs/stages/B1-playable-prototype.md#research-b99c6db-integration)。
- [x] b719030本地Release ZIP 15.85MiB，732文件独立解压核对与纯系统PATH信息窗口启动通过；最新经营前缀已实际推进并冷载到月54，布局21/50、到位营业10/25，仍二星。后续从保存前缀继续，不能以此替代三星和全内容验收。

- [ ] [全解锁主动经营长跑](docs/stages/ACTIVE_VILLAGE_PLAN.md#full-unlock-campaign)：按定义ID核对建筑/强化/道具/装备/职业/魔法/活动/配方/副本/BOSS，逐星→自然通关→继续/继承。已补全量目录和分段观测，不等于完成覆盖；当前优先突破三星收入，再补全部驱动、效果和恢复断言。
- [x] Windows本地文本崩溃诊断：故障线程栈、临近日志和64回执接入；隔离崩溃/轮转、两Debug标准及Release窗口/错误出口检查见[本批结果](docs/stages/B1-playable-prototype.md#campaign-coverage-crash-diagnostics)。

- [x] 按建筑/人物/AI/战斗等11个大模块重排源码与公开头，拆资源/建筑UI、补重要注释和导航；两Debug标准138/118项、Release资源/窗口通过，build净减约2.28GiB（36.31%），见[B1](docs/stages/B1-playable-prototype.md#source-domain-modules)。

- [x] 产品冗余与旧切片退役：删除172个跟踪文件，保留当前世界/DLL防错/存档身份；两Debug标准138/118项、Release构建/资源/标题/拾取窗口通过。用户旧默认档按明确删除指令处理后已恢复标题启动，详见[本批记录](docs/stages/B1-playable-prototype.md#product-cleanup-legacy-retirement)。
- [x] 1f19c88维护/UI/音频本地验收：430项冻结来源、711项素材通过；两Debug标准178/154项通过（仅排三个月），Steam窗框/建设/81/74/60静态四页、X4金币及可见建筑像素拾取完成Release绘制检查，26音频全部加载。实际听音/OS输入不在认证内，完整动态委托仍见剩余请求。玩家schema4两栏及随机政策保持，维护世界4/应用7/系统2不替换玩家存档。
- [ ] 修后主动三星：前段搬店铺路、西餐厅兑换/营业、20胜及保存冷载通过；月56法术接触表现丢目标已定位并补正，失败Owner单轮复验通过，见[研究交接](docs/reference/RESEARCH_REQUESTS.md#spell-contact-visual-target)。收益策略增加点数购买新建筑、道具强化与预算，并参考用户四星档布局；三星/学校/活动7仍未认证，见[经营状态](docs/stages/ACTIVE_VILLAGE_PLAN.md#third-star-diagnostics-income)。
- [x] 本批b6a5099 Release便携包16.00MiB，736文件逐字节解压检查、纯系统PATH启动与隔离保存/新进程读取通过；仅本地制品，CI待验证、未推送或发布。

- [x] e73bb31新版经营→魔法壶→便携包：[本批结果](docs/stages/B1-playable-prototype.md#product-business-e73bb31)。新版首/二星跨进程链、魔法壶投入/发现/冷载/炼制/真实用药、两Debug标准177/153项通过；PC标题拖动误激活已修复。13.45MiB Release ZIP已独立解压、启动和隔离存读验证；OS输入通道不可用，CI待验证，未推送/发布。

- [x] S019及设施加成2/2面板部分补正：紧凑布局、原数字/箭头、分区和按钮位置，通过本地测试/窗口；当时图标及逐来源加成未完成，后续72a5bf4接线见[B1](docs/stages/B1-playable-prototype.md#facility-detail-visual-correction)。
- [x] 设施74场景底栏名称/累计收益接线及对应HUD鼠标热区：用户已批准，累计净收益/正负字块/最小窗口完成本地验收，Space暂停保留，见上述B1底栏接续。

a57958c魔法壶/村办5–6、schema3和0a5b5e2购买后举物/邻接提示/商品图标已完成[本地验收](docs/stages/B1-playable-prototype.md#research-a57958c-design)；类型4无独立效果，不列为缺失按钮。

6061a2c/5eae10a补审、授勋视觉子集、存档覆盖与文档/代码职责整理已完成本地验收，见[B1本批记录](docs/stages/B1-playable-prototype.md#research-save-organization)。标题/开始与两栏继续入口的接入、验收见[标题批次](docs/stages/B1-playable-prototype.md#title-start-flow)。

- [x] 真实经营流程验收：武器/防具自然购买→举物→装配、真实建设邻接属性/提示、独立进程载入→继续收费；饰品、自然搬迁及OS输入未认证。
- [x] 公共DLL防错：构建输入与库产物指纹、消费者检查及启动早期诊断；四套消费者仍共用一套Release DLL。
- [x] 现状文档清理：修正已接经营链/表现及schema3说明，保留历史来源与验收身份。范围和结果见[B1](docs/stages/B1-playable-prototype.md#business-validation-dll-docs)。
- [ ] 对应main的desktop-release完整CTest与发布流水线验证；本地检查不替代CI，不自动推送。

## 研究交付后的接入

- [ ] 本批冻结b99c6db后，研究正式提交a45642c交付城镇统计/设施目录与镜头流程（34/39等）。已核提交范围，尚未迁入；下一批集中核其维护布局、玩家字段分类与桌面皮肤，不再称该维护消费者“研究未交付”。本批运行及ZIP仍对应b99c6db。

- [x] 72a5bf4完整闭包及设施74图标/逐来源加成/后缀、普通道具图标、人物cd13头标已接线并完成本地验收，见[B1](docs/stages/B1-playable-prototype.md#research-72a5bf4-integration)。

- [x] 4ef4a98主角配置/schema4、跨局系统纪录/六类通关计分和启动皮肤已完成本地验收；见[B1](docs/stages/B1-playable-prototype.md#research-4ef4a98-startup)。
- [x] 89f157c公共窗框/内容框、完整应用维护短回放和文件校验修正已接入；冻结391项，玩家schema4政策不变，见[B1](docs/stages/B1-playable-prototype.md#research-89f157c-integration)。
- [x] [五组短收益对照与主动首星验收](docs/stages/ACTIVE_VILLAGE_PLAN.md)：真实建设营业、培养、7次任务胜利、9次活动、升星/绘画展及跨进程玩家存档通过；两个快速合同进入标准CTest。
- [x] 3780a3b正式维护闭包已迁入：402项、7适配，含标题20槽、基础人物皮肤、商店窄投影及自然应用回放；桌面新局男女静态预览已窗口检查，完整标题宿主动效不在本批接线范围。
- [x] 3780a3b本地验收：两Debug标准177/153项通过；主动二星真实建设/入住/16次任务胜利、晋级、付费活动30和后续营业、玩家跨进程恢复及终态复核通过，首次超时现场保留；CI待验证，见[B1](docs/stages/B1-playable-prototype.md#research-3780a3b-integration)。
- [x] e73bb31正式闭包404项/7适配已迁入：新局任务目录与设施人气初值修正、收支只读查询、维护AVRSAVE语义2/AVRAPP语义4；Steam三张静态启动图已接。玩家schema4布局保持，更新数据身份拒绝旧档、不迁移。
- [x] e73bb31本地验收：desktop-debug177项/headless-debug153项、当前语义自然首月三进程回放和4个Release窗口通过；CI待验证，见[B1](docs/stages/B1-playable-prototype.md#research-e73bb31-integration)。此前首星/二星/五方案结果仅属旧初始化语义，后续主动经营须重新取得本版真实新局前缀。
- [x] 建设/邻接到达收费修复aa87318已随1f19c88迁入并通过实际收费/账本/回滚及五方案复验；[原请求](docs/reference/RESEARCH_REQUESTS.md#facility-arrival-price-cache)闭合，不宣称布局最优。
- [ ] 主动经营后继：本批先修后三星；四至五星、明确身份的BOSS、计分/继续/继承仍独立待验。e73首/二星及魔法壶实际炼制/用药通过仅属该历史语义，不能替代修后业务前缀。

- [x] 1e6b291建设查询与Owner显式表现请求/独立回放迁入并本地验收，来源核至88eb658，363项冻结来源；结果见[B1](docs/stages/B1-playable-prototype.md#research-88eb658-integration)。
- [ ] 桌面自动表现请求及探索底栏：补原准入/包装资格、滑入/背景/资源桥后接线，见[最小缺口](docs/reference/RESEARCH_REQUESTS.md#dungeon-strip-consumer-gap)；不按60FPS推导随机抽取次数。
- [ ] 完整施工阶段、正门/进出、手持武器/物体/投射物、连击/升级/浮标与76/77演出：逐项等精确帧/锚点/时钟合同。
- [ ] 人物/怪物受击表现：879cb17已补Owner命中短轨迹及拒绝测试，本批随完整闭包迁入；仍不依据观感增设减速/硬直/击退，动态表现按[受击缺口](docs/reference/RESEARCH_REQUESTS.md#hit-reaction-gap)。
- [ ] 情报/设置/标题余项：b99c6db已迁入raw9/35–38维护消费者与目录/说明/图标/皮肤及35→60追踪，本批接桌面FIFO、渲染与输入验收；34/39剩余消费者按后续正式交付接。类型化声音、26资源与桌面播放已有历史验收；标题20槽宿主调度与完整动画未接。见[候选门槛](docs/reference/REFERENCE_CHECKLIST.md#候选接续状态与接入门槛)和[交接状态](docs/reference/RESEARCH_REQUESTS.md#information-audio-animation-89f157c)。
- [ ] 设施81独立frame2及人物循环/HP条/危险/奖章等正式计划已随c19ab6a迁入；下一步绑定桌面执行并窗口验收，不再等待重复研究交付，不从绘制帧推进计数，见[剩余接线](docs/reference/RESEARCH_REQUESTS.md#steam-ui-delegates-1f19c88)。
- [ ] 跨周自动中断档：正式轮内恢复消费者交付后单独设计；保持当前玩家随机/暂停/格式政策，见[RQ12](docs/reference/RESEARCH_REQUESTS.md#persistence-integration-gap)。

## 产品工程

- [ ] 正常窗口输入/回执进一步按实际职责整理，保留generation、pending、held与失焦边界；不预建框架。
- [ ] 后续性能优化按采样热点与独立candidate对照推进；不改47ms、一倍速、随机或补算。
- [ ] Windows原游戏PC交互与产品OS输入对照；归档截图、产品渲染、真实鼠标操作分别记录，不接Steam软件。

## 不在当前范围

旧档迁移/原游戏档兼容、退出自动保存、复活玩家二倍速、恢复旧Game第二套世界、修改或构建research。
