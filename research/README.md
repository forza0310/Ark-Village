# 逆向研究入口

本目录是《冒险迷宫村》一代规则、素材和行为证据的唯一研究入口，与产品工程分开维护。
目标为C++17＋raylib先还原二维玩法、UI和数值，之后再考虑3D。
固定APK1.0.8是汉化重签输入，Steam2.56是独立交叉来源，用户异版本截图分别登记。

| 需要了解 | 入口 |
| --- | --- |
| 当前完成范围、存档能力与下一步 | [阶段矩阵](dungeon_village_1/stages/README.md) |
| 功能／包／运行方式 | [一代研究总览](dungeon_village_1/README.md) |
| 原作规则／未证条件 | [规则索引](dungeon_village_1/rules/README.md) |
| 维护正常存取／精确测试快照 | [原型存取模块](dungeon_village_1/prototype/PERSISTENCE.md) |
| 原版存档剖析／只读检查 | [原版合同](dungeon_village_1/rules/PERSISTENCE.md)、[原档工具](dungeon_village_1/tools/ORIGINAL_SAVE.md) |
| UI／素材／截图 | [视觉基线](dungeon_village_1/ui/README.md)、[参考归档](dungeon_village_1/references/README.md) |
| 输入身份／来源／工具 | [EVIDENCE](dungeon_village_1/EVIDENCE.md) |
| 实际检查与历史记录 | [VERIFICATION](dungeon_village_1/VERIFICATION.md) |
| 研究步骤与两版交叉 | [WORKFLOW](WORKFLOW.md) |

当前核心自主经营世界、玩家任务／住宅／晋级／道具商会／地图编辑和首次扩张已有维护交付；
研究正常文件与晋级精确回放已验，Steam若干真实存档恢复已交叉。
它们不等于完整原档兼容、所有后期功能或完整原皮肤／同版动态。
剩余经营消费者、魔法壶、存档完整生命周期、后期路线及精确表现按阶段矩阵逐批收口。

## 消费与保留约定

- 原版事实、局部推断、维护安全策略、夹具和异版本动态分开；同名／同图不自动认证消费者相同。
- 产品只消费已交付规则、独立实现、数据和有来源清单的素材；运行不读取APK或research/work，研究不代改产品。
- 生成Java、IL2CPP映射、原游戏／存档、账号身份、候选及分析缓存保持只读来源或本地work，不进入发布源。
- 日常研究构建只留项目内单Release动态树，Debug／插桩按需；测试按功能批次集中，相关认证尾段代替无关重复长前缀。
- 原窗口观察、研究C++检查、OS输入与产品验收分别报告；纯文档整理不运行游戏回归，也不把旧检查记成本轮通过。
- 冻结实验与失败证据不删改；有效引用历史不当作泄漏，账本／任务／检查点不宣称永久有界。
- 阶段结束收齐本轮进程，只提交已验research文件、保留其他维护者改动，不自动推送。
