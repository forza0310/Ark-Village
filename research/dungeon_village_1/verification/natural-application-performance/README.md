# 自然应用路线性能审计

2026-10-09。本页最初为只读代码审计：没有新性能采样、附着原程序、启动长测或修改产品。主会话进行中的month1→month8约270 CPU秒、12891外层轮、约31MB工作集，仅作为待解释的现象；这不是单函数耗时、受控基准、长期峰值或泄漏结论。审计发现一个普通消费者漏接，随后按主会话指令单独修复；下列性能优化候选均**未实现**。

## 先修正消费者缺口

实际调用链不是“应用复制Session→Session.update→再次复制候选”：当前[应用update](../../prototype/src/startup_application.cpp:252)直接调用`prepare_startup_world_runtime`，移动成功候选到新的Session，再经`commit_world`联合提交。私有默认Session构造是`=default`，不会每轮重建真实新局。

审计发现应用路径原先漏掉[Session.update](../../prototype/src/startup_world_runtime.cpp:1132)中的`update_startup_world_render_cache`。全prototype源中此前只有Session这一调用点；`prepare_startup_world_runtime`本体没有调用，应用Driver也没有另外补调用。

这不是可以省略的纯像素工作。[缓存消费者](../../prototype/src/startup_world_runtime_nonactors.cpp:93)写`actor_metadata.cached_screen_position/render_position`；[人物声音](../../prototype/src/startup_world_runtime_nonactors.cpp:121)读取前者，[命中声音可见性](../../prototype/src/startup_world_runtime_nonactors.cpp:83)读取后者。应用[logic模式约定](../../prototype/STARTUP_APPLICATION.md)仅禁止标题背景／纪录装饰随机，没有授权省去世界轮末消费者。遗漏会使应用与既有Session的完整状态及后续声音资格分叉，即使日期和现金暂时相同也不能认证等价。

已在本批源码补回：`prepare`成功→对私有候选执行相同缓存消费者→失败显式拒绝→成功才移动候选及进入系统／世界联合提交。没有移动这一消费者到原检查点之前，没有改变Session公开返回独立candidate的契约，也没有在失败后修改旧世界来“修复”缓存。原始不可变检查点仍保留其原捕获时点，当前轮末缓存另由Session最终消费者维护。

回归复用[startup_application_test.cpp](../../prototype/tests/startup_application_test.cpp)原`natural_cash`真实新局前缀：每轮先从同一个旧`app.world`复制独立Session并更新，再执行应用更新，在玩家确认之前逐轮比较完整Session摘要和原序声音。实际可见human累计8轮后关闭额外oracle，继续原现金路径，原断言保留。失败系统文件／旧世界回滚继续由既有应用用例承担。本分支没有构建；最终结果以主会话集中验收为准。

修前自然快照即使形状／摘要自洽，也只证明旧应用路径可复演。主会话负责退休其旧应用语义资格、保留修前诊断记录并重新认证正确消费者前缀；不能将旧12月结果称为修后路线完成。

## 已有性能证据不能重复套用

[研究3000轮历史采样](../../work/validation/perf-baseline-summary.md)曾观察到`owned_schedule`独占约20.5%、`runtime_domain`约17.6%、`owned_scene`约15.2%、显式模板Owner复制约14.3%、路由读约7.4%。这是当时i686 Release、给定自然晋级短轨迹的插桩分层；独占剩余不全是复制，嵌套包含时间不能求和，不能当本次已修正应用路线的百分比。

那份报告的adapter每轮重建热点已经在当前[运行时入口](../../prototype/src/startup_world_runtime.cpp:1102)使用`static const`只读adapter解决。公开adapter工厂仍返回独立值以隔离测试修改。本轮不能把“再缓存adapter”列为新收益。

产品[历史采样说明](../../../../scripts/simulation/profile_world.md)仅作方法参考：它也提示外层最终复制和渲染缓存占比小于内部多层投影，且并发运行不能用墙钟差推加速比。本分支没有修改、运行或重新认证产品代码，也没有将产品结果算作research验收。

## 精确可见的剩余开销

| 位置 | 当前事实 | 判断 |
| --- | --- | --- |
| 应用`update` | 移动`prepare`成功候选；每轮复制`checkpoints_`的shared_ptr向量并追加新检查点引用 | 没有再次深拷贝每份历史Owner；不能把向量复制估成全部历史快照复制 |
| 应用`commit_world` | 每轮先复制完整`StartupSystemRecords`，即使峰值、继承和显式保存要求均不变 | 正常小纪录成本可能很小；含未知可选段时仍会复制这些字节 |
| `prepare_startup_world_runtime` | 先完整复制`admitted`以退休页面、初始化子页并设置执行根；内层场景又维护私有scratch | 不能直接移走真实Owner，也不能跳过页面／初始化验证来省复制 |
| scene／schedule模板 | 各持有独立私有scratch；多次将值投影交给消费者并重投影结果 | 部分是事务边界及原序审计，不是可无条件删除的冗余 |
| actor路由 | 当前`startup_world_runtime_routes`构造完整world／facts／随机／任务／商店／目录等；actor schedule随后重新覆盖共同world/facts | 重复值工作存在，但接口允许不同投影实现，不能凭一个适配器就删泛型写回 |
| nonactor路由 | 为取得`objects.shops`，调用`startup_world_runtime_routes(s).shops`，其余整份人物路由临时结果全部丢弃 | 最明确的局部多余计算，且有必须保留的隐含校验，见下节 |
| facility收尾 | 只为判断刚处理设施是否还存在，再执行一次完整`adapter.facilities.read(next)` | 有潜在开销；通用write回调可能做额外规范化，不能未经证明改读旧candidate |
| 自然Driver | 每轮前后编码／解码1480字节Driver、核日期/资源/引用，逐轮更新声音hash；仅所选trace窗口做完整摘要 | 没有每个普通前缀轮都序列化整份AVRAPP；关闭核验不是合格性能优化 |

当前[Session.update](../../prototype/src/startup_world_runtime.cpp:1132)为同时返回完整独立candidate与安装Session，确实保留一次完整状态复制；应用直接prepare路径已避开此返回值复制。公共Session调用者依赖独立可变候选的行为，不能将移动空candidate也当成功，更不能据应用无需这个结果而改掉公共契约。

## 无需新格式的最小优化候选

### 优先一：仅构造nonactor实际需要的商店投影

位置在[startup_world_runtime.cpp:600](../../prototype/src/startup_world_runtime.cpp:600)：`r.objects.shops = startup_world_runtime_routes(s).shops`。完整路由函数先复制世界、执行`project_human_task_flags`，再复制其它扩展；最后真正需要的是`shops`副本及`facility_details[id].notices`覆盖。

可在同模块提取只读商店投影助手，供完整actor路由和nonactor共同调用，从而复用相同notice覆盖逻辑。**必须同时保留原调用隐含的缺失人物task flag／actor context拒绝**：原完整投影会在这些关系缺失时抛具名错误。不能直接把该行换成`s.shops`而默默跳过校验。建议将任务旗标关系验证提为不修改输入的独立内部检查，nonactor路径保留同一检查时点和错误，actor路径在私有world中继续真实写入投影旗标。

此方案不新增Owner字段、不借用短生命周期引用、不改变随机、反向写回或文件协议。收益需要新采样，不能由“少构造一份大对象”宣称具体倍数。

最小独立oracle：保留修后旧完整`startup_world_runtime_routes(s).shops`作为对照，在同一真实世界状态比较新商店投影的全部字段和原序notice；覆盖已有notice／无details覆盖、缺human flag、缺actor context的明确拒绝，并核输入状态和随机不变。然后使用修后认证快照的一段短尾，逐轮比较完整Session／Driver摘要、原序声音、未来随机及检查点原时点内容。先做这一个局部优化，不同时修改generic模板以便定位差异。

### 优先二：系统纪录确实改变时才复制

`commit_world`先以只读比较确定`cash_peak`增加、继承变化或`save_system=true`；三者均不成立时，直接noexcept安装已经验证的世界候选，保留现有`records_`。有变化时仍复制系统纪录、原样保存未知可选段、原子写系统成功后联合安装。`save_system=true`不可被“字段刚好相同”短路。

这仅调整局部临时对象创建时点，不改收费、原脚本、写档顺序或格式。独立oracle至少保留：无变化不写文件且opaque字节原序保留；现金峰值／继承变化真实写入；替换失败完整应用及旧文件不变；相同自然短尾摘要一致。正常空opaque样本中它可能只有很小收益，排在明确的完整路由浪费之后。

### 暂不做：历史向量借用与泛型Owner借用

避免每轮复制检查点引用可能需要把历史合并推迟到系统文件发布之后；必须确保合并后的安装完全noexcept，否则会出现“系统已更新、世界未安装”。预留vector容量、共享所有权、失败时生命周期及输出消费均须精确设计。当前只是指针向量复制，没有证据值得为它改变提交协议。

scene日期只读窄投影、设施存在性窄查询、actor共同投影重复、模板私有scratch移动也是后续候选，但会触及泛型adapter合同；需先细分采样和有独立旧实现对照，不凭历史占比直接重写。不得删除历史、关闭声音／render cache、降低断言、修改47ms门槛、跳日历或放大文件预算来制造性能收益。

## 建议的下一次短采样

先完成漏接修复与语义退休，取得一个修后认证的中途入口。将同一入口恢复到两份隔离应用，分别运行修后基线和单项优化，使用完全相同请求序列及输出消费。采集`prepare`、nonactor商店投影、最终应用提交的包含时间／调用次数，并注明是否嵌套；墙钟、CPU、后台并发、状态规模分别记录。用200…500轮尾段即可先证等价与热点，不从新局重跑180月。

本次审阅时关键输入SHA-256：修复前`startup_application.cpp`为`d99ff7225c058f6f2a193fee1c71a5ae2fbf79d43e63b428e5ee35927710dc62`；`startup_world_runtime.cpp`为`ebd365e14510d4dfeed8183d436a740b4b8b07363f7144e6ba97718ab9d7ab31`；`example/include/dungeon_village_reference/world_runtime.hpp`为`41c7b02ae5b5d82d5a88ee769c97fdadbe73b9d83c3d76413adbd6e08867144a`。应用修复会改变前者，该hash只定位本次发现现场，不作为修后基线。

## 窄商店投影已实施与验收

本批仅实施优先一。完整人物routes与非人物窄读共用任务旗标引用遍历、商店notice投影；非人物窄读不再构造随即丢弃的world／facts／任务等完整人物路由。仍在原时点拒绝缺human flag或RescueActorContext，不擅自新增ai.contexts／怪物上下文要求；原序notice、空覆盖及缺details保留分别断言。没有改Owner字段、随机、原表、文件身份、页生命周期或历史保留。

完整Release构建成功；非人物调度、共同运行时、旧回放Driver三项通过0.09秒。随后新增可选微基准入口并仅重编该套件，默认无参复验通过0.04秒；无新target／CTest。旧world420→840三进程回放仍保持ddebdf80尾段hash。

从同一当前首月快照继续600轮，优化前后终点完整摘要均为`3b1589d58722a448d77580065800c0096f5928b6a971e13ec52dd6ba83f5b312`，末20轮逐字节相同（`cfa152bd…8a799fe`），随机6509、声音累计17，完整资源统计相同。两次墙钟10.71／21.77秒的执行权限环境不同，不作为速度收益或退化的证据；自动审批超时后改用项目内受限执行，均未更改业务输入。

同进程可选`projection_benchmark`复用一份明确的条件投影夹具，64对预热、7对各1000次，交替full-first／narrow-first，均消费相同checksum且输入摘要不变。局部中位数旧完整投影57.3482ms、新窄投影25.676ms，约减少55.2%；这是**单次商店投影微基准**，不是整局加速倍数。正常测试不执行基准，不以快慢断言通过。

当前语义3的12月已从首月认证前缀接续完成：19295轮捕获19,662,928字节，19315轮终点双恢复与reference一致；reference463.50秒，恢复各约1秒，证书在`work/snapshots/natural-application-v2/month12.avra.json`。这是缓存修正后的当前输入，和旧语义2同名月份诊断分开。下一步先采24月文件与解码成本，再决定是否可到60月，不越过128MiB预算或删历史凑通过。

本批日志、短trace及[审计](VALIDATION.json)保留；仅一个既有Release树，无新构建树／原素材副本，本阶段构建与测试进程已退出。合法历史仍可能增长；未实施优先二和泛型Owner借用。
