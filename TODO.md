# 当前待办

当前实现和边界见[文档索引](docs/README.md)、[原版对照](docs/reference/REFERENCE_CHECKLIST.md#research-history-current-audit)。此前已完成项原文见[历史任务台账](docs/stages/history/PRODUCT_TASK_LOG.md)；测试结果按批次查[B1](docs/stages/B1-playable-prototype.md)。

## 本批收口

- [x] S019及设施加成2/2面板部分补正：紧凑布局、原数字/箭头、分区和按钮位置，通过本地测试/窗口；当时图标及逐来源加成未完成，后续72a5bf4接线见[B1](docs/stages/B1-playable-prototype.md#facility-detail-visual-correction)。
- [x] 设施74场景底栏名称/累计收益接线及对应HUD鼠标热区：用户已批准，累计净收益/正负字块/最小窗口完成本地验收，Space暂停保留，见上述B1底栏接续。

a57958c魔法壶/村办5–6、schema3和0a5b5e2购买后举物/邻接提示/商品图标已完成[本地验收](docs/stages/B1-playable-prototype.md#research-a57958c-design)；类型4无独立效果，不列为缺失按钮。

6061a2c/5eae10a补审、授勋视觉子集、存档覆盖与文档/代码职责整理已完成本地验收，见[B1本批记录](docs/stages/B1-playable-prototype.md#research-save-organization)。标题/开始与两栏继续入口的接入、验收见[标题批次](docs/stages/B1-playable-prototype.md#title-start-flow)。

- [x] 真实经营流程验收：武器/防具自然购买→举物→装配、真实建设邻接属性/提示、独立进程载入→继续收费；饰品、自然搬迁及OS输入未认证。
- [x] 公共DLL防错：构建输入与库产物指纹、消费者检查及启动早期诊断；四套消费者仍共用一套Release DLL。
- [x] 现状文档清理：修正已接经营链/表现及schema3说明，保留历史来源与验收身份。范围和结果见[B1](docs/stages/B1-playable-prototype.md#business-validation-dll-docs)。
- [ ] 对应main的desktop-release完整CTest与发布流水线验证；本地检查不替代CI，不自动推送。

## 研究交付后的接入

- [x] 72a5bf4完整闭包及设施74图标/逐来源加成/后缀、普通道具图标、人物cd13头标已接线并完成本地验收，见[B1](docs/stages/B1-playable-prototype.md#research-72a5bf4-integration)。

- [ ] 开局主角配置/到访、标题纪录：后继9ec4c1a已正式交付，另批核对产品入口及存档政策后接入；按[具体依赖](docs/reference/RESEARCH_REQUESTS.md#player-feedback-20261008)接入；不能用漏画推断规则没生效。

- [x] 1e6b291建设查询与Owner显式表现请求/独立回放迁入并本地验收，来源核至88eb658，363项冻结来源；结果见[B1](docs/stages/B1-playable-prototype.md#research-88eb658-integration)。
- [ ] 桌面自动表现请求及探索底栏：补原准入/包装资格、滑入/背景/资源桥后接线，见[最小缺口](docs/reference/RESEARCH_REQUESTS.md#dungeon-strip-consumer-gap)；不按60FPS推导随机抽取次数。
- [ ] 完整施工阶段、正门/进出、手持武器/物体/投射物、连击/升级/浮标与76/77演出：逐项等精确帧/锚点/时钟合同。
- [ ] 人物/怪物受击表现：879cb17已补Owner命中短轨迹及拒绝测试，本批随完整闭包迁入；仍不依据观感增设减速/硬直/击退，动态表现按[受击缺口](docs/reference/RESEARCH_REQUESTS.md#hit-reaction-gap)。
- [ ] 情报/设置/标题余项/音频：按[候选门槛](docs/reference/REFERENCE_CHECKLIST.md#候选接续状态与接入门槛)接线。商品79/72、82和魔法壶已接，后续补精确动画与自然流程证据。
- [ ] 跨周自动中断档：正式轮内恢复消费者交付后单独设计；保持当前玩家随机/暂停/格式政策，见[RQ12](docs/reference/RESEARCH_REQUESTS.md#persistence-integration-gap)。

## 产品工程

- [ ] 正常窗口输入/回执进一步按实际职责整理，保留generation、pending、held与失焦边界；不预建框架。
- [ ] 后续性能优化按采样热点与独立candidate对照推进；不改47ms、一倍速、随机或补算。
- [ ] Windows原游戏PC交互与产品OS输入对照；归档截图、产品渲染、真实鼠标操作分别记录，不接Steam软件。

## 不在当前范围

旧档迁移/原游戏档兼容、退出自动保存、复活玩家二倍速、扩展旧Game补默认世界、修改或构建research。
