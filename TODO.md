# 当前待办

当前范围见[计划与决策](docs/MILESTONES.md)、[文档索引](docs/README.md)及[原版对照](docs/reference/REFERENCE_CHECKLIST.md#research-history-current-audit)。已完成批次、历史验证数字和证据入口统一查[B1](docs/stages/B1-playable-prototype.md)；776508c菜单、信息与保存桥已完成本地验收，不重复列为待接功能。

## 当前接入与收口

- [x] 正式395fa0b及前置已接人物四页、职业62/63、装备65/73和赠礼结果66，455来源/70适配保持，719素材；复用既有套件，见[B1](docs/stages/B1-playable-prototype.md#research-395fa0b-human-results)。
- [x] 现actors UI执行raw60四页有序计划、实际字体测宽/恢复提示/比例条；保留PC页签/捷径与FIFO/pending，装备方向先走四槽、行点击选择、Enter进入对应赠礼槽，魔法Enter无操作。
- [x] 用户指定的小窗preview.mjs及所引设施/道具/菜单/提示/人物示例已纳入本goal，复用各自正式合同及套件，对照入口见[清单](docs/reference/REFERENCE_CHECKLIST.md#ui-small-window-goal)。预览服务、CSS和夹具数值不进入游戏。
- [x] 6a0f8e2／395fa0b职业预览、转职演出和赠礼结果计划已消费，真实转职/赠礼窗口通过；共享奖励人物时钟未建模，静态帧0边界明确，不从参考PNG代做业务或时钟。
- [ ] 人物原字体、OS输入与完整动态/滚动条鼠标、控制器提示及职业/赠礼余页皮肤继续按正式交付接续，不用四页和65/73局部计划与产品截图宣称原版全部对齐。
- [x] 获批steam-month分段月96→97→98→99及三次独立冷载通过；每段6分钟/10000轮，原季度入口保持，失败观察档不作输入，见[B1](docs/stages/B1-playable-prototype.md#research-e22ed0a-human-ui)。

- [x] 正式bb61d5f普通设施两页维护计划：获批四目标已导入，455来源/70适配保留，新增资源映射现有Steam属性图标；后续c6b1a5b无闭包增量，见[方案](docs/MILESTONES.md#research-bb61d5f-detail-plan)。
- [x] 普通raw74两页已接现facilities UI：正式c3b568c滚动皮肤/事件及原套件增量迁入，原序绘制/Steam箭头、字体测宽与五行光标沿已批桌面适配；窗口/剩余原版输入边界见[B1](docs/stages/B1-playable-prototype.md#research-c3b568c-detail-ui)。

- [ ] 系统纪录身份解耦待用户确认：说明正文变化不应未经决策改变既有系统档资格；具体兼容边界与验证范围见上述设计。玩家仍为两栏schema5、随机不保存，不迁移旧档或自动删除用户文件。
- [ ] raw39极端金额列宽适配待确认：十位金额可能侵入名称列；按实际字体和最小窗口评估50像素金额列，不宣称极端排版已收口。
- [ ] 对应main的desktop-release完整CTest及发布流水线验证；本地结果与CI分别记录，不自动推送或发布。

## 完整经营验收

- [x] 对齐周期扣费负余额：真实月报跨零、主动建设拒绝及五种经营策略配置的负最低值往返通过，长跑不以一次负债判世界失败；健康经营仍须实际恢复与后继推进，见[B1](docs/stages/B1-playable-prototype.md#campaign-negative-balance)。

- [ ] 按[全解锁主动经营合同](docs/stages/ACTIVE_VILLAGE_PLAN.md#full-unlock-campaign)覆盖所有可达定义的取得、消费、效果与后继，包含建设营业、活动培养、任务胜利、逐星和新解锁消费，最终自然通关、继续经营与继承。代表样例、仅解锁或日期推进不算全覆盖。
- [x] 修后三星申请及学校首次营业：月74收入35551G及四条件正式回执通过，学校63真实支付2000G、完工并营业690G，烤肉店41图纸支付90点/领取；月75保存/冷载通过，见[B1](docs/stages/B1-playable-prototype.md#campaign-third-star)。
- [x] 月87定向中止恢复营业、月88玩家存读、限时经营至月90四星及独立冷载；真实付费补员至9人后任务42胜利，现金145338G、两月预算31460G、布局50/50，见[经营状态](docs/stages/ACTIVE_VILLAGE_PLAN.md#campaign-fourth-star)。
- [x] 月93有限季度存读通过：真实13次付费/4免费装备装配、任务43胜利、炖肉39营业及25目标位置全收入；月亏损仍保留，见[B1](docs/stages/B1-playable-prototype.md#four-star-consumption-20261012)。
- [ ] 从最后有效月100前缀继续买/建博物馆64（136/200点）；任务45已实际胜利并冷载，现金119828G/预算35280G，但仍须复核完整后继月经营与收益。五星人气差464、最高月收入差33135；活动19独立延迟效果、活动7人物效果、职业/魔法和余项继续覆盖，再通关/继续/继承及全部可达定义，不注资或降低阈值。

## 表现与研究边界

- [ ] 桌面自动表现请求及探索底栏：补原准入/包装资格、滑入/背景/资源桥后接线，见[最小缺口](docs/reference/RESEARCH_REQUESTS.md#dungeon-strip-consumer-gap)；不按60FPS推导随机调用。
- [ ] 完整施工、正门/进出、手持武器/物体/投射物、连击/升级/浮标与76/77演出按精确帧、锚点和时钟合同接入；受击不凭观感添加减速、硬直或击退，见[受击缺口](docs/reference/RESEARCH_REQUESTS.md#hit-reaction-gap)。
- [ ] 设置、世界内排行榜/退出及标题完整动效等余项按[候选门槛](docs/reference/REFERENCE_CHECKLIST.md#候选接续状态与接入门槛)推进；已接的真实菜单和34/39不再列为缺失消费者。
- [ ] 自动跨周中断档等待轮内恢复合同后单独设计，保持既有玩家存档政策，见[RQ12](docs/reference/RESEARCH_REQUESTS.md#persistence-integration-gap)。
- [ ] 正常窗口输入/回执按实际职责整理，保留generation、pending、held与失焦边界；Windows原游戏与产品OS输入对照单列，不将截图或程序内输入当作真实鼠标验收。

## 不在当前范围

性能专项已按用户要求停止追加；历史结果保留，不作为当前行动。旧档迁移/原游戏档兼容、退出自动保存、二倍速、旧Game第二套世界、Steam软件接入，以及修改或构建research均不在本批范围。
