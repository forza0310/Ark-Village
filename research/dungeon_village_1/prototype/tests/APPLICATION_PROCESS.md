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

主动runner另提供显式`--load-candidate <完整轮末候选>`，与`--load-prefix`互斥。用途是从失败长跑已捕获的完整应用候选开始**新的reference**，并在之后的新捕获上执行两个独立恢复；不把缺失证书自动当候选，不改`--load-prefix`原有证书校验。候选真实路径必须在research/work内，文件不超过128MiB，预检须满足应用7容器摘要和主动Controller；C++仍完整校验世界4、Driver关系、系统及全部引用blob，不能只靠文件可解析就发布认证。

候选入口仍使用既有AVRAPP01格式和`--load-file`恢复协议，不增加业务格式、不修改原候选字节。新证书`source_prefix`明确写`source_status=candidate`、原hash／字节数／next_frame及来源版本；`uncertified_history`记录此前未认证历史边界，且沿后来`--load-prefix`生成的证书继续保留。新尾段三路相同只认证新捕获以后的短区间，不追认先前失败前缀、首星或全路线已验。所有路径的finally均核原候选字节不变；候选没有证书时不会伪造源证书。

例如从工作包中的晚期候选恢复，必须显式选一个晚于候选next_frame的保存轮数及有界尾段：

```powershell
node research/dungeon_village_1/prototype/tests/application_active_process.mjs --exe <application测试程序> --work-dir <独立工作目录> --load-candidate research/dungeon_village_1/work/active-application-later/application-active-5D8HYk/process-0/prefix.avra --save-at <新的捕获轮> --tail-frames 20 --frame-limit <捕获轮加20或更大上限>
```

两入口共用认证文件发布辅助：先独占创建快照，再独占创建证书，**双文件不是原子发布**。第二次创建或写入失败时，仅回收本轮创建、文件身份与实际字节仍匹配的文件；既有证书、源前缀与外部改动不删除。回收失败报告明确残留，不宣称发布成功。进程崩溃仍可能留未认证候选；缺失或不匹配证书的前缀不具备恢复认证资格。

既有`replay_file_test.mjs`的可选`--active-application-exe`挂接同一420／440认证，不新建target或CTest。本页是机制说明，实际构建、进程结果与资源规模以当批交付记录为准。

## 首星到二星策略的有限交接

`application_second_star_process.mjs`对应[交接设计第1批](APPLICATION_PROCESS.md)。它不替换容器中的controller字符串，也不将旧世界重新激活为新应用；C++私有恢复先按v1原样完成已核的34402–34429共28轮，再在相同旧metadata下比较交接前后完整摘要及隔离文件清单，构造独立`application-active-progression-v2`／`AVACTDR2`。应用7、世界4、系统2及128MiB限制保持。v2命令及输出计数从自身起点开始，完整旧Driver和334条旧音频的身份仍在origin中。

首次只接受明确的`--handoff-prefix`：已核`frame34401.avra`及相邻证书，producer为`11818a9-income-gate`，快照SHA-256为`16a1b8c48993a96c536e4fc5868c7408355776c3b4631145047a2b4706c8062b`，证书SHA-256为`8484c8cac5578c431dfd307903d7659b24296741eeace84cd875ab176c3332e3`。必须证明34429的当月实际设施收入400G，不能接纳原34419当月收入为0的旧terminal。脚本沿显式证书引用检查最多32层来源，核实际文件／分区摘要、世界4／系统2、捕获轮及来源producer，不扫描“最新”文件。

```powershell
node research/dungeon_village_1/prototype/tests/application_second_star_process.mjs --exe <application测试程序> --work-dir <research/work下已存在独立目录> --handoff-prefix research/dungeon_village_1/work/snapshots/active-application-v1/frame34401.avra --tail-frames 20 --producer-revision <本批版本> --snapshot-file <新的v2快照路径>
```

首份v2快照固定捕获在34429轮末、`next_frame=34430`、`next_command=1`，尚未发送v2输入。此后reference与两个独立恢复进程至少执行20轮，逐字节比较完整命令、Driver、应用／目录、系统字节摘要与typed声音。终点除捕获位置和性能计时外全字段比较，新增诊断字段也纳入比较。v2独立命令ID为0等待、1页面确认、2打开任务、3任务动作、4年度授勋动作、5退出商会；不能沿v1同号解释。wait仍是独立全零五元组，不递增命令序号。

证书资格为`active_second_star_management_tail`，保留完整`handoff.origin_metadata`、旧Driver十六进制及摘要、交接前后旧digest、来源快照／证书hash和文件清单摘要。`uncertified_history`原对象及实际候选身份沿所有后继保留：20000轮候选SHA-256为`e6452b6d5b7ab9d9124ffde656144f1f4780c0a19538eb5f1c07a74b32978218`，17,380,140字节，producer为`87023c3`；当前回放不会追认更早历史。源链所有实际文件在发布前和finally重新核hash，不能仅保留一行限制文字。

当前固定来源的34429终点旧Driver为635字节，SHA-256为`a76854cfd6a0ad2349648ac89da42b783a3972e7fb1980b1a0be564e0794f0d6`；交接前后目录清单摘要为`ece8dc44f2f2b910d2a9e8498e5f5ead021c676ac61418a7c59b90ebeff6028d`。脚本和C++各自核对这两个已取得的固定值，不能只接受“改写Driver后重算hash仍自洽”或“同时替换前后文件digest且二者相等”。这些是本次有限交接来源的身份，不是任意游戏存档的常量。

`stage_complete`只能由真实任务接受回执计数产生，`active_command_count`另记本尾段实际非wait命令数。20轮完全等待可以认证完整存取一致，但`stage_complete=false`时**不能称主动策略批次完成**。若等待尚未满足任务冷却／资格，继续使用新证书的`--load-prefix`，在显式轮预算内继续经营，再选新的捕获边界与至少20轮尾段；该选项与`--handoff-prefix`互斥，不接纳无证书v2候选。二星、四住宅和活动30真正领取均不由首个任务接受回执认证。

两个`active_command_count`按所属层区分：证书根字段是**本捕获尾段**的非wait输入数；`terminal.active_command_count`来自C++summary，是**自34429交接以来v2累计**的非wait输入数。后继20轮全等待时，前者可以为0、后者仍为3；`stage_complete=true`沿来源链继承先前真实接受回执，不能把它写成本尾段又接受一次。接受量另有根字段`capture_accepted_tasks`和`accepted_tasks_in_tail`，两者之和必须等于`terminal.accepted_tasks`。保留已生成证书字段名及原字节，不通过改档统一这两个不同的计数域。

进程timeout、取消后的close、独占双文件发布及按身份回收共用现有support。每次只删除本次成功运行的临时根；失败保留诊断及未认证候选，不覆盖来源、旧证书或其它进程目录。当前实现是否已实跑、取得了哪条真实操作回执，以当批交付记录为准，本机制文档不代替验收结果。

## 从任务募集接续真实住宅与经营建设

`application_residence_process.mjs`沿[住宅策略设计](APPLICATION_PROCESS.md)，使用独立`application-active-residence-v1`／`AVRESDR1`。首次`--handoff-prefix`只接受已核v2 `frame34469.avra`及相邻证书：快照SHA-256为`dc30d31a83c021d6af6ac66fbb31fbb40ea01e49a2679c6823f9567724a3f8f6`，证书为`4dcb2135d5ee4045b7a176df31b124af3fd04ab3121031a25469372ee27889ad`。固定producer为`2c612c9-handoff-audit`。原v2完整恢复并推进34470–34489的20轮后，才只读构造住宅策略；不取消任务7、不改变募集页、世界、随机、文件或输出。

34489终点旧metadata完整摘要为`62aaa3970f616e4af63c8cf4c12685b0e510aee9c70f7b84ed9553068063ba25`；旧Driver为1572字节，SHA-256为`f293d9463eb014cb6a61f62a3f5fae2d285b50197dc7f77f138d3ea98358afc7`，next_frame34490、next_command4。后者与快照捕获34469的Driver不是同一份字节。脚本固定核这些身份及交接文件清单摘要，`handoff.prior_handoff`必须与原v2证书中的完整v1交接对象相同；两层来源不能压成controller名字或几个终点数值。

```powershell
node research/dungeon_village_1/prototype/tests/application_residence_process.mjs --exe <application测试程序> --work-dir <research/work下已存在独立目录> --handoff-prefix research/dungeon_village_1/work/snapshots/active-application-v2/frame34469.avra --tail-frames 20 --producer-revision <本批版本> --snapshot-file <新住宅策略快照>
```

首捕固定34489完整轮末、零新策略输入；next_command从1开始。后续`--load-prefix`只接`active_residence_management_tail`资格的已认证住宅前缀，使用显式`--save-at`和至少20轮尾段捕获父答案消费、入住或完工边界，无需重复新局、v1首星或原v2交接前缀。其来源递归最终必须回到上述固定v2证书；旧v2/v1链由既有`readSecondStarSource`核验。共用`application_progression_evidence.mjs`只预检数据集、应用7／世界4／系统2、分区摘要及显式Driver magic，不代替C++语义校验或来源资格。旧v2接口和严格映射保持。

住宅命令ID独立登记：0等待、1确认具名页面、2任务动作、3授勋、4退出商会、5打开建设、6选择建筑、7确认放置、8取消放置、9打开设施、10设施动作、11入住动作、12打开人物、13人物动作。全部五元组及实际参数进入trace；自动更新、父64消费和完工不伪装为新玩家命令。未知输入、wait混其他命令、命令序号失配均拒绝。

回执分为`gifts_committed`（父64实际消费）、`recruitments_created`（实际募集创建）、`homes_admitted`（80入住替换）、`completed_houses`（Owner当前已完工住宅）与`completed_business_facilities`（当前已完工且连接合格的kind3/9）。首捕前3项为0，经营设施原有4座不强制清零。`capture_receipts`加本尾段`tail_receipts`等于终点同型计数；自动消费可出现在没有新输入的尾段，不能因此删掉合法回执或补造一次确认。

`homes`只记录施工完成、绑定正确、旧募集退休且完成演出退回场景后的`human/instance/completed_frame`，不放施工候选；人物0合法，以`first_home_frame>0`判断是否取得首住宅。`stage_complete`只表示第一住宅完整生命周期完成，`terminal`还须四座完工住宅和十座可经营设施。赠礼父提交、募集创建、入住施工、首住宅完工分别交付，不能由`created`宣称住宅完成或二星通过。

新证书根字段`tail_active_command_count`明确只计本尾段；C++summary的累计计数留在`terminal`对象。完整summary除捕获位置、捕获回执／待消费标志、性能计时及入口检查诊断外全字段三路比较，包含住宅观察、已有任务7、页绑定、资源及嵌套origin。入口另强制`driver_checks=13+(entry_gift_pending?12:0)`，两个恢复入口的待消费标志须等于reference捕获值。待消费时清空标志／支付字段仍拒绝；已初始化65的Driver父子页错误在恢复发布前拒绝。源链保留完整`uncertified_history`，所有实际源文件在发布前及finally复核未变；文件／trace／进程／层数预算、独占发布及失败保留沿现协议。没有新业务格式、迁移或预算放宽；三路短尾段通过仍不代表已自然完成四宅十店。

住宅`stage_complete`表示策略历史上已取得第一住宅，不表示本尾段又完成一套。恢复来源审计逐项检查`capture_receipts + tail_receipts = terminal`，根capture回执与summary捕获回执相同；`homes`按真实完成轮排序且数量等于已完成住宅计数，完成轮不晚于捕获轮的记录数必须等于捕获时住宅数。根尾段命令数等于终点next_command减真实快照metadata.next_command，summary累计命令数等于终点next_command减1。比如36000→36060允许stage=true、所有tail回执和尾段命令均0，首住宅仍是35202的人物0／实例16；这不认证36000尾段发生了首住宅动作。来源预检不能替代C++恢复时对业务状态和规范Driver的绑定校验。
