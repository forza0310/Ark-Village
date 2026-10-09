# 文档索引

本文是产品文档入口。当前事实、已确认设计、历史验收分开维护；research 的索引由研究侧维护。

| 分类 | 入口 | 内容与维护责任 |
| --- | --- | --- |
| 上手与当前动作 | [根 README](../README.md)、[TODO](../TODO.md) | 构建、运行、操作；TODO只列当前待办，不堆叠历史测试数字 |
| 工程约定 | [AGENTS](../AGENTS.md)、[开发流程](CONTRIBUTING.md) | 来源边界、阶段验收、本地/CI分工、提交与文档约定 |
| 现行架构 | [架构](ARCHITECTURE.md)、[可维护性进度](PRODUCT_REVIEW.md) | 唯一Owner、依赖方向、模块职责及剩余工程风险 |
| 已确认决策 | [计划与ADR](MILESTONES.md) | 决策理由与替代关系；历史Accepted不代表仍未实施 |
| 当前研究对照 | [对照清单](reference/REFERENCE_CHECKLIST.md#research-history-current-audit)、[研究需求](reference/RESEARCH_REQUESTS.md) | 正式研究身份、已接子集、缺口和原版一致性边界 |
| 存档合同 | [玩家档模块](../src/app/world_save.md)、[本次审计](stages/B1-playable-prototype.md#research-save-organization) | 玩家ARKSAVE1、维护AVRSAVE1、原游戏档分别说明，不能互换认证 |
| 阶段验收 | [B1当前批次](stages/B1-playable-prototype.md) | 本批范围、检查、未完成项；旧锚点保留转向历史 |
| 主动经营与收益 | [长测审计与建设方案](stages/ACTIVE_VILLAGE_PLAN.md) | 现有长跑覆盖、五星原条件、初局报价/布局候选、现金安全与主动玩家验收 |
| 历史归档 | [B1实现记录](stages/history/B1-implementation-log.md)、[任务台账](stages/history/PRODUCT_TASK_LOG.md)、[审阅基线](stages/history/PRODUCT_REVIEW-20261007.md) | 不改写旧验收结果；旧机器/版本的结果不能替代新检查 |

## 代码与工具导航

| 职责 | 实现入口 | 说明 |
| --- | --- | --- |
| 原规则与世界运行时 | [simulation](../src/simulation/README.md) | 标准C++，冻结来源；不依赖app/desktop/raylib |
| 会话、命令、玩家存档 | [app](../src/app/README.md) | FIFO、generation、只读快照、文件候选替换；旧Game标明诊断边界 |
| 窗口与输入 | [desktop](../src/desktop/README.md) | world_view协调窗口；world_management负责页面命令；inspection仅显式诊断 |
| 只读UI | [ui](../src/desktop/ui/README.md) | 页面数据映射、布局、绘制；不推进时钟/随机/奖励 |
| 测试 | [tests](../tests/README.md) | 按行为职责注册；维护冻结测试与产品适配测试分开 |
| 构建与维护工具 | [scripts](../scripts/README.md)、[simulation工具](../scripts/simulation/README.md) | 公共Release DLL统一构建、字段/来源验证与精确回放 |

## 归档规则

- 当前实现以架构、模块说明和reference为准；带日期的验收结论必须保留提交/配置/排除项。
- 已完成阶段写入B1历史；TODO只留未完成动作和下一步。不要在多个入口复制完整验收数字。
- 移动文档须修正相对链接并保留已被引用的锚点。本轮B1旧入口保留导航，不破坏研究侧外链。
- 不移动或改写research文件，不将未提交研究文档当作正式交付。
