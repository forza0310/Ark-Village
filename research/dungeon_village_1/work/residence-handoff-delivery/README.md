# 住宅终点只读来源与窄交接工具

2026-10-10。本包仅归档用户调整测试分工前的本地实现实验和短检查，**不是正式工具交付**。交接工具3个源码文件的新增修改未提交，原工作区及`second-star-completion-plan/deferred-code`逐字节草稿同时保留；自动审批拒绝在新分工下提交工具代码，也拒绝恢复这3个文件，因此本次只提交文档与证据。**不继续新增研究经营Driver，不要求研究自然完成二星**。世界长跑改由产品负责，规则研究按[最新测试分工](../../../WORKFLOW.md#研究与产品的测试分工)执行。此前[二星方案](../second-star-completion-plan/README.md)仅保留历史设计和来源。

未提交实验代码曾在本地persistence可执行新增`application-residence-handoff-check --source <固定源> --work-dir <新隔离目录>`，复用现target。测试专用接口`residence_replay_support`只提供私有应用的成功交接、原validator、固定origin校验与前代来源查询，不公开可写住宅Driver。没有新存档语义、Owner或经营策略；产品不能按本包文档假定提交版本已提供这些接口。

固定输入为`frame38820-bound.avra`，36,589,440字节、SHA-256 `fd773a56d401552bd02e6227e78e8923e6dfde0ca22126aedd762a639ecdf7a0`，证书SHA-256 `ca9bc506bd7a1e0415f975d534d5821a6095e8020acf261683d2a8fc11101c94`。原住宅策略执行已认证20轮至38840，固定producer保持`1f19c88-residence-bound`；终点完整Driver2881字节、SHA-256 `5e8583c5427bd8ff63ff24e6fe304d04cfdec67835c272dc247ae6a51654882d`，不是38820捕获Driver。

终点完整应用摘要`cdcad4e182464403d558917862a234eadd2cb00569bd70758c4a5a85da2497aa`，实际目录摘要`ece8dc44f2f2b910d2a9e8498e5f5ead021c676ac61418a7c59b90ebeff6028d`，下一轮38841、命令1188、下一任务策略冷却月25。只读来源包构造及拒绝测试前后，应用、完整metadata、目录和源hash不变。保留完整v1→v2→住宅origin，不清空冷却或重新计算来源。

Release构建通过；原样观察20行trace与原证书逐字节摘要一致；独立窄工厂CLI一次成功调用通过，7项origin变异拒绝（截断、改字节、controller、producer、frame、command、extensions），已有恢复目标及错误来源都在新应用目录创建前拒绝。受影响persistence套件39.63秒通过；没有重跑无关世界长测。检查原记录见[CHECK.json](CHECK.json)，完整origin／源观察见[origin-observation.json](../second-star-completion-plan/origin-observation.json)，可复算脚本见[extract_origin.py](../second-star-completion-plan/extract_origin.py)。

成功返回前完成所有原恢复校验和20轮推进；失败丢弃私有应用，不交还半推进对象。已成功恢复后才发生的错误可能留下本轮隔离诊断文件，不声称所有失败都从未落盘。本工具尚未构造新经营Controller，不能把来源复制不变证明写成新策略运行或二星成功。

全部构建／测试进程正常退出，内存对象随可执行退出释放；原样trace、固定快照来源、观察JSON及隔离错误夹具留本地用于追溯，不再新增长链证书。没有新素材、构建树、原游戏操作或产品修改；有效历史未删改，资源规模与输出消费沿已交付住宅证书，不宣称长期永久有界。
