# 应用回放的隔离进程验收

`application_natural_process.mjs`保留被动日期策略`application-natural-clear-v3`；`application_active_process.mjs`只接受主动管理策略`application-active-progression-v1`。二者分别校验自己的Driver、证书资格和逐轮输出，不能用被动前缀证明主动建设／活动／任务或自然首星。

共同的`application_process_support.mjs`仅处理AVRAPP01容器与分区摘要、指定Controller身份、单个子进程的超时／取消／日志。它不决定前缀资格、不恢复世界、不产生证书。当前容器1／应用7、128MiB预算保持，世界4与规范Driver载荷仍由C++权威恢复器验证。每进程日志限1MiB，超限显式失败；窗口隐藏，任何返回均等待直接子进程close。失败保留现场，成功只回收本次mkdtemp目录，不删除输入、历史证书或其他构建。

主动入口示例（在项目根调用，程序使用既有application测试可执行文件）：

```powershell
node research/dungeon_village_1/prototype/tests/application_active_process.mjs --exe <application测试程序> --work-dir <research/work下已存在独立目录> --save-at 420 --tail-frames 20 --frame-limit 2000 --timeout-seconds 120
```

reference使用真实新局，经应用公开命令到420轮捕获，再继续至440；两个新进程在不同根恢复同一捕获，逐字节比较20行尾段。每行保留实际命令`[id,page,arg0,arg1,arg2]`、递增命令序号、完整Driver十六进制及其SHA-256、完整应用摘要、实际system字节摘要、typed声音`[operation,id]`。应用摘要覆盖世界／历史与全部四目录引用身份；save／restore另外核验所有实际blob，不每轮重复读取blob。摘要相同不单独证明原游戏动态等价。

命令ID按主动v1固定枚举0–14校验：0等待，1 acknowledge_page，2 open_build_menu，3 select_build_menu，4 confirm_build，5 cancel_build，6 open_facility_page，7 open_village_activities，8 act_village_activity_page，9 cancel_page，10 open_task_menu，11 act_task_page，12 act_rank_page，13 act_award_page，14 leave_commerce_page。商会退出不能错误记录成另一通用取消API。等待轮只允许一条`[0,0,0,0,0]`，不能混有其他输入；`next_command`仅按非零ID的已提交输入数量递增，等待与自动世界更新不冒充玩家命令。终点还比较任务接受／出发／成功、完成活动、升级次数及是否真正达到目标，短尾段不能省略这些资格字段。来源版本通过`--producer-revision`进入实际metadata，证书必须与内层一致。

显式`--snapshot-file <新文件>`只在三路通过后无覆盖发布快照与`.json`证书；`--load-prefix`必须持有同一主动Controller、`active_application_management_tail`资格及匹配实际容器hash的证书，捕获边界必须晚于源。此证书只认证所述短尾段，不以420／440结果宣称首星或自然通关已经完成。长阶段使用单独显式观察预算，不改原世界数值、旧黄金或有效断言。

两入口共用认证文件发布辅助：先独占创建快照，再独占创建证书，**双文件不是原子发布**。第二次创建或写入失败时，仅回收本轮创建、文件身份与实际字节仍匹配的文件；既有证书、源前缀与外部改动不删除。回收失败报告明确残留，不宣称发布成功。进程崩溃仍可能留未认证候选；缺失或不匹配证书的前缀不具备恢复认证资格。

既有`replay_file_test.mjs`的可选`--active-application-exe`挂接同一420／440认证，不新建target或CTest。本页是机制说明，实际构建、进程结果与资源规模以当批交付记录为准。
