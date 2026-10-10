# Steam存读档窗口实验索引

2026-10-09接续皮肤研究：[当前外部提示词](NEXT_SKIN_OBSERVATION_PROMPT.md)只补公共窗口框／字体观察、可安全取得的标题人物及现成计分画面，反馈用[新模板](SKIN_OBSERVATION_FEEDBACK_TEMPLATE.md)。自定义姓名→切性别→取消重进已由用户取消补测；不重做已证存读档实验，不新增原档写入权限。

2026-10-07整理。这里是本机冻结实验与反馈入口，work不进入Git或产品运行。
正式能力／下一步见[阶段页](../../stages/README.md)，原版语义见[存档合同](../../rules/PERSISTENCE.md)，
历史验收见[存取归档](../PERSISTENCE_HISTORY.md)。Steam2.56同身份，APK1.0.8仍独立。

| 顺序／目录 | 输入与目的 | 实际结果／分析 | 证明限度 |
| --- | --- | --- | --- |
| [102645：首次备份恢复](../../work/window-restore-observation/20261007-102645-backup-restore-trials/RESULT.md) | 原手动／中断档、设施道具前后保存 | [window-save-analysis](../../work/window-save-analysis/ANALYSIS.md)：72项证据、9份不同游戏档；道具落盘、消失遭遇保存引用退休、跨周相容 | 9档p全空；未做设施候选重载；目录隔离失败后获授权备份恢复 |
| [120056：业务重载／撤除](../../work/window-restore-observation/20261007-120056-business-reload-p/RESULT.md) | 固定设施候选9f125dbe…，库存与J恢复；尝试取非空p | [business-reload-analysis](../../work/business-reload-analysis/ANALYSIS.md)：A实际恢复库存1／J保留，B只删定义32实例、邻接引用退休 | B采样与保存错开，无非空p；缺怪物引用如实保留；重要图S052–S061 |
| [125641：既有非空前档](../../work/window-restore-observation/20261007-125641-nonempty-p-only/RESULT.md) | 只读最新备份，不运行游戏 | [nonempty-p-candidate-analysis](../../work/nonempty-p-candidate-analysis/ANALYSIS.md)：0003 3e1caa3c…，UID21／定义40／p[3,9]；0/17/19首次实样非空 | 只取得前提，产生操作／UI名称未知；同批手动p0不是重载后档；active writer转交失败由用户交回解决 |
| [131546：非空中断实际恢复](../../work/window-restore-observation/20261007-131546-nonempty-p-reload/RESULT.md) | 自动栏读3e1caa3c…，有界固定首次不同0003 | [nonempty-p-reload-analysis](../../work/nonempty-p-reload-analysis/ANALYSIS.md)：7f09b86c…首档p空／业务保留，稍后施工完成和手动保存；四档100段／56引用 | 达真实输入恢复端点等级，非首帧／不独立排除自然消费，具名详情未得；重要图S062–S068 |

已完成的业务恢复与非空输入恢复不再重复。[NEXT_RESTORE_PROMPT](../../work/window-restore-observation/NEXT_RESTORE_PROMPT.md)已收口，旧步骤仅追溯，无新窗口操作。
[WINDOW_TASK_PROMPT](../../work/window-restore-observation/WINDOW_TASK_PROMPT.md)及[反馈模板](../../work/window-restore-observation/WINDOW_FEEDBACK_TEMPLATE.md)是历史任务／记录格式，不新增原档写入权限。

## 证据使用与保留

报告、清单、原图、前后密文与动作时序保持冻结。路径／文件标签可能只表拍摄意图，必须实际看图并审档；
目录日期来自0000，不能替代游戏记录日期；同目录多文件固定不是统一事务。
只读取实验备份的账号目录名在本机解码，摘要不含账号、key、村名或完整明文；不提交存档／身份根。
后续写原档必须沿操作会话的当前明确授权、届时最新备份及已确认落点，不能用旧批许可／备份回退新进度。
本地恢复hash与云端分别记；未重核实时原件不声称当前状态一致。

当前无后台监测或待运行窗口任务。保留有效候选及身份根供只读复核；重复输出单列，未确认退役前不删除。
重要截图原字节已在[视觉归档](../../references/README.md)发布，全部原图仍在各批目录。
工具是只读取证支撑，不改游戏内存、注入数值或代替实际UI加载日志；维护AVRS快照是另一种格式。
