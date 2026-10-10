# 世界所有者与调度

`startup_world_runtime` 持有唯一聚合 Owner，准备完整下一状态并在成功后发布；arrival/pages/scene/nonactors 接线跨模块消费者。`startup` 安装固定初值，`rules/` 提供共享领域类型、随机、场景和调度基础。

这里允许依赖各领域模块，领域规则不反向依赖运行时所有者。失败保留原世界及随机游标；正常窗口的线程/FIFO/47ms 调度由 `src/app/session` 与 timing 负责。
