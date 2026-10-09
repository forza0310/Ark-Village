# 完整应用计分短回放交付

2026-10-09，接续4ef4a98。用户先批准具体设计，随后又明确授权StartupApplication接口、研究新文件及隔离失败测试；自动审批曾要求先验证内部边界再公开接口。本批依此先完成路径、无覆盖发布、计分只读资格及两项现有套件（14.36秒），再接[完整模块](../../prototype/APPLICATION_REPLAY.md)，公开接口最终获准写入。该阻断已解除，不继续标为等待用户授权。

## 验收结果

首轮Release完整构建和8项相关CTest通过137.01秒。采样发现每轮应用摘要重复执行文件级编码／解码自检；改用全部应用字段＋完整Session摘要，真实保存仍encode→decode自检，文件格式和world schema不变。补6例“正常摘要、坏计分关系”的重签输入，并收紧研究CLI的目录预检和trace无覆盖发布后，最终构建通过。

最终7项CTest全部通过28.85秒：应用、持久化、codec字段覆盖、visuals、world数据、原回放Driver和assets；独立执行已注册的进程runner同入口，附加认证快照保留选项，完整通过。该runner保留原自然短段与表现控制器检查，并增加同次条件reference＋四点各两次新进程恢复。没有新增target／CTest，未运行无关多年长测／Debug／产品验收。

持久化套件本轮墙钟116.00→28.85秒，是同机两次实际记录，新增关系断言仍通过；存在并行调度差异，不当受控性能基准。每轮应用摘要仍覆盖13个保存成员，路径重新绑定、error要求为空，共15成员分类。完整摘要不是磁盘容器hash，也不单独保证文件预算／可恢复性。

新高／等分、非首行44→45、结算64、真实系统文件锁失败／解锁重试、一次声音与事件4／5／6、控制器退休、可选段原序／未知段拒绝、正常摘要坏sum／六行／counter／页ID／捕获最高／三元组均验。坏输入不能部分安装app／Driver或提前创建系统文件。系统文件双写者竞争恰一成功、已有文件／目录／硬链接不改、临时文件收口均通过。Windows符号链接夹具因权限不足明确未跑；POSIX实现未在本机执行。

## 已保留四个条件快照

本机目录为`work/snapshots/application-clear-v1/`，9文件1395611字节，包括4个347507字节容器、各自证书及总证书。它们是合法raw17条件入口，**不是自然经营180月**。捕获与恢复实际终点一致：1422次请求、7200分、1次故障重试、1次收尾声音、7110项Driver检查。

| 捕获点 | 下一请求 | 已认证尾段 |
| --- | ---: | ---: |
| row2计数44 | 660 | 763轮 |
| row2计数45 | 661 | 762轮 |
| stage6计数64 | 1421 | 2轮（失败→重试） |
| 失败已记录 | 1422 | 1轮重试 |

源文件hash／三路尾段摘要见[机器审核](VALIDATION.json)，原本机[总证书](../snapshots/application-clear-v1/CERTIFICATE.json)保留完整资格。二进制不提交；证书摘要随本批归档。默认CTest清临时目录，本次显式命令在全部比较通过后才独占保留：

```powershell
node research/dungeon_village_1/prototype/tests/replay_file_test.mjs --exe research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests.exe --work-dir research/dungeon_village_1/work/release/prototype/replay-process-tests --presentation-exe research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_persistence_tests.exe --application-exe research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_persistence_tests.exe --application-snapshot-directory research/dungeon_village_1/work/snapshots/application-clear-v1 --producer-revision 4ef4a98-application-replay
```

复跑生成必须选择另一个全新输出目录，不能覆盖已认证证据。只复验某个尾段可用同一持久化测试程序的`application-clear-conditions-v1 --work-dir <已存在研究专用子目录> --load-file <上述某个.avra> --trace-file <新的trace>`；其父目录必须在研究work内，trace只创建新文件。输入源hash仍须与对应证书核对。

## 资源与边界

world与全部历史仍共用原节点／分配预算，应用容器整体128MiB，没有增加阈值、删历史或换原表凑通过。字段覆盖检查通过后清理36315062字节AST；最终应用类过滤AST为200902字节，在内存解析不留大文件。Release树1419文件101496080字节，无待发布临时文件。原Owner schema及16项数据身份不变。新字段规范与应用schema独立，快照不能冒称任意版本永远兼容。

原世界、Driver、页及一次性声音均纳入输出消费检查；候选用RAII退役，计分完成不保留三元组。弱引用历史缓存不延寿历史，完整合法账本／任务／审计增长仍非永久有界。本批仅保留必要文本日志、CPU拼图和约1.40MB认证条件快照；单Release树、输出hash／规模见[审核](VALIDATION.json)。日志归档统一UTF-8／LF并去行尾空白，不改诊断内容。所有本轮构建、测试、runner及子任务收齐；没有操作原游戏或实时存档。

下一步：原标题人物Owner与显式请求接线、Steam实际字体／页面绘制消费者和外部窗口反馈；自然路线先补应用层年度退出等具名玩家动作与独立自然Driver，再一次生成当前布局的前缀和分段成本。条件快照不能换Driver标签当自然前缀，完整自然路线也不等于五星／全部任务／BOSS。
