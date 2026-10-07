# 当前待办

当前实现和边界见[文档索引](docs/README.md)、[原版对照](docs/reference/REFERENCE_CHECKLIST.md#research-history-current-audit)。此前已完成项原文见[历史任务台账](docs/stages/history/PRODUCT_TASK_LOG.md)；测试结果按批次查[B1](docs/stages/B1-playable-prototype.md)。

## 本批收口

6061a2c/5eae10a补审、授勋视觉子集、存档覆盖与文档/代码职责整理已完成本地验收，见[B1本批记录](docs/stages/B1-playable-prototype.md#research-save-organization)。标题/开始与两栏继续入口的接入、验收见[标题批次](docs/stages/B1-playable-prototype.md#title-start-flow)。

- [ ] 对应main的desktop-release完整CTest与发布流水线验证；本地检查不替代CI，不自动推送。

## 研究交付后的接入

- [ ] 探索底栏：共同随机时点、滑入/背景/资源桥、丢实例清任务与231→aW消费者，见[最小缺口](docs/reference/RESEARCH_REQUESTS.md#dungeon-strip-consumer-gap)。
- [ ] 完整施工阶段、正门/进出、手持武器/物体/投射物、连击/升级/浮标与76/77演出：逐项等精确帧/锚点/时钟合同。
- [ ] 设施74逐来源奖励/类别图标、75道具图标、商品79、传说82/opcode40、村办4–6、情报/设置/标题/音频：按[候选门槛](docs/reference/REFERENCE_CHECKLIST.md#候选接续状态与接入门槛)接线。
- [ ] 跨周自动中断档：正式轮内恢复消费者交付后单独设计；保持当前玩家随机/暂停/格式政策，见[RQ12](docs/reference/RESEARCH_REQUESTS.md#persistence-integration-gap)。

## 产品工程

- [ ] 正常窗口输入/回执进一步按实际职责整理，保留generation、pending、held与失焦边界；不预建框架。
- [ ] 公共DLL指纹防错，避免绕过统一构建入口混用旧库；四套消费者仍共用一套Release DLL。
- [ ] 后续性能优化按采样热点与独立candidate对照推进；不改47ms、一倍速、随机或补算。
- [ ] Windows原游戏PC交互与产品OS输入对照；归档截图、产品渲染、真实鼠标操作分别记录，不接Steam软件。

## 不在当前范围

旧档迁移/原游戏档兼容、退出自动保存、复活玩家二倍速、扩展旧Game补默认世界、修改或构建research。
