# 文件存取与精确回放

当前35／60追踪批新增`StartupHumanDetailContext`及`human_detail_contexts`，保存来源0／1和可选真实人物实例；35复用`information_page_data`保存一组定义目录、选择与滚动，phase保存四页签。codec按实际字段生成布局身份，旧布局研究普通档、世界及应用精确快照均明确拒绝，不迁移或重签历史证书。协议用途、预算及普通档只允许稳定主场景的政策不变；初始化标记、来源／实例合法性、孤立载荷与退休分别校验，不把字段登记当作测试通过。

此前到达价格修正使用AVRSAVE1格式1、状态语义4，当时world schema为`7f33851d8b0430afbb6234595ab01d2190f29959515d216e6da64b1919dbf440`。该修正未改字段布局，但邻接价格影响后续收入及轨迹；旧世界语义1–3拒绝，不能加载后重算价格冒称修复旧历史。预算不变、旧档不改写不迁移，验收见[收费缓存交付](../VERIFICATION.md#邻接价格与语义4应用7修正2026-10-09)。此前[声音语义3](../VERIFICATION.md#声音操作遭遇通知与steam菜单语言2026-10-09)为历史版本。

本模块实现研究维护格式，不读取APK／Steam原档，不迁移旧demo或未知schema。
来源事实见[原存档分析](../rules/PERSISTENCE.md)，已确认方案见[快照设计](../stages/PERSISTENCE_REPLAY.md)。

2026-10-09初始化修正：AVRSAVE1仍格式1，**状态语义当时升为2**；旧状态语义1拒绝、不迁移。补齐原标题已填的六特殊／三十复发任务目录及实际reset后的设施共享人气20／30。Owner字段布局与固定原表身份不变，旧前缀不能据此视作兼容；不修改旧文件／证书，最新认证见[初始化交付](../VERIFICATION.md#先前交付索引)。

2026-10-07实现批已完成标准回归、正常窗口及38000帧跨进程快照验收，具体边界、耗时与哈希见[原批记录](../verification/PERSISTENCE_HISTORY.md#维护文件存取与精确回放2026-10-07已验收)。
固定APK规则／原表与自然玩家黄金值不变；原版读档会丢失的随机／页面／部分效果，在维护回放格式中完整保留。
原版引用恢复／设施冒泡丢弃／日历续跑的后续来源见[恢复合同](../rules/PERSISTENCE.md#原版恢复合同缺失引用冒泡与日历2026-10-07)。
这些是原档加载行为，不改变本模块的精确状态保存与严格候选校验，也不让原中断档成为可直接续帧的维护快照。

## 入口与用途

2026-10-09另增[完整应用研究回放](APPLICATION_REPLAY.md)：AVRAPP01保存应用系统／标题／计分控制器＋完整Session＋实际Driver，在新研究隔离目录恢复。它复用本文世界字节codec和既有预算，计分途中不再仅保存世界而漏掉应用控制器；正常AVRSAVE1和AVRSYS01用途保持分开。

2026-10-08主角覆盖新增Owner布局，旧无覆盖schema拒绝、不迁移；跨局纪录使用独立AVRSYS01系统文件，不能从单世界恢复重建。具体应用事务、标题回放与计分页快照边界见[标题／系统模块](STARTUP_APPLICATION.md)。

公开接口为[文件存取](include/dungeon_village_prototype/startup_world_persistence.hpp)：
`save_startup_world_file`不修改Session；`load_startup_world_file`返回完整私有候选，失败只返回错误。
调用者先校验自己的控制器载荷，再同时安装候选Session和控制器；不得在解析控制器前先替换世界。

| 用途 | 保存边界 | 包含内容 |
| --- | --- | --- |
| normal | 稳定主场景、无建设操作或模态页、声音已消费 | 全Owner动态状态和随机；逻辑输入清零；不包含测试控制器或全部审计历史 |
| replay | 外层测试轮的update、玩家命令、声音消费及断言均完成 | 全Owner、原序审计历史、控制器版本／全部进度、下一帧与来源追溯；支持已完整校验的模态等待页 |

显式表现请求模式独立于旧逻辑控制器：调用序号、原准入方式、66包装资格及声音上下文由独立控制器保存，
Owner同步提交并返回当次计划，不保留等待输出字段，schema不因这项接口改变。
捕获前控制器必须已消费计划／声音；尾段轨迹同时核全部请求／冻结计划／声音和全Session，
重复画同一计划不抽随机，重复调用Owner则是另一实际调用。首版合同见[输入及表现请求](../ui/INPUT_RENDER_REQUESTS.md)。

两种文件有不同用途标识，不能混读；原日历轮内检查点仅作为审计历史恢复，不能拿来调用下一帧。
文件内容完整性与自然前缀资格分开：人工合成文件即使合法也不因此成为自然轨迹证据。

原型窗口支持正常存取：

```powershell
# 在稳定主场景退出时保存；模态／建设中退出会报告拒绝，保留原有效文件。
& research/dungeon_village_1/work/release/bin/dungeon_village_prototype.exe --world --save-file research/dungeon_village_1/work/manual.avrs
# 从正常档开始继续经营，退出时再保存。
& research/dungeon_village_1/work/release/bin/dungeon_village_prototype.exe --world --load-file research/dungeon_village_1/work/manual.avrs --save-file research/dungeon_village_1/work/manual.avrs
# 只读校验正常档，不启动窗口。
& research/dungeon_village_1/work/release/bin/dungeon_village_prototype.exe --world --load-file research/dungeon_village_1/work/manual.avrs --check
```

正常文件由调用者明确指定，未实现原两栏保存菜单或自动文件保存；当前原日历检查点行为保持不变。
文件参数通常不能与inspect-page混用；本批仅开放只读normal档＋有界`world-magic-pot`诊断，沿已校验世界调用真实壶入口、禁止同时写档。
窗口诊断的输入资格（自然／组合）由文件来源决定，不因读档成功升级。窗口字体仍按现有`--font`参数选择。

## 文件布局与资源预算

37／38的typed页面载荷保存冻结目录ID、选择与滚动；页签／时钟仍在原phase／counter。恢复校验全部列表（包括38未选页签）的身份、资格和原顺序，拒绝缺键、错误组数、越界选择／滚动、错页类型及残留绑定；不执行初始化补默认或重复清NEW。关闭4与下一框架退休仍分开，空37只允许保留已提示后的退休载荷。38按Steam2.56目录合同恢复，未开放38→73。详情见[信息页面](../ui/INFORMATION_MENU.md#现有维护owner足够不新造数据)。普通存档仍拒绝模态37／38；精确回放才保留这些完整页面。

此前壶41–47新增8组Owner字段，复用13槽／用户标志；当时93可达结构／11枚举布局身份为
`e5a40276fe0b1dfc0c8ef3f3f6eb703bcbc475f14e106e0685472acc2df66d7f`。
固定数据身份新增magicPot原表，16输入身份为`c3f9419d56e2db0c4a4fcf582f3b34fe5100a12b6acaa2b3bbf3ca0273faa690`。
此前`0500cff0...`自然晋级／扩张档和79批`f6b3cd3c...`档保留为各自历史证据，当前读器明确拒绝，不做迁移或修改证书。
同布局代码才能复用对应前缀；当前新增消费者的恢复／续跑由本批页面往返及短跨进程用例验收，未重新认证晚期前缀。
壶45/46先于41展示，未轮到的lower41可以合法未初始化；缺已初始化载荷仍拒绝，恢复不执行m／重抽评语／再扣道具或元素。
关闭标记和下一框架退休分别保存，aM／aN／aQ全局跨页事实保留；日期差非0零输出重写aQ，不能只按业务changed决定保存内容。

所有协议整数小端，与原APK大端格式无兼容关系。外层依次为：

1. 8字节`AVRSAVE1`；32位格式版本、状态语义版本、用途（1正常／2回放）。
2. 长度前缀数据集身份、codec字段布局身份；各值为SHA-256十六进制ASCII文本。
3. 32位分区数；逐段32位ID／段版本／必需标记、64位长度、长度前缀SHA-256文本、原字节。
4. 前面全部字节的SHA-256，64个ASCII字符，无长度前缀；禁止尾字节。

| 分区 | 内容 |
| --- | --- |
| 1 | 来源提交字符串、控制器语义身份、下一帧；字符串长度为32位，下一帧64位 |
| 2 | 当前Owner规范字段编码 |
| 3 | 仅回放：32位历史数量、每项64位长度和Owner编码，保持原序 |
| 4 | 仅回放：独立控制器载荷；由对应控制器严格解析并与Owner交叉验证 |
| ≥1024 | 不影响世界身份／引用的可选附属字节；已知长度原样保留，未知必需段拒绝 |

重复分区、保留ID、用途不符、未知版本／必需段、错误摘要、截断、超预算均拒绝。
摘要检测损坏，不是来源认证或对恶意修改的签名；不能用摘要相同代替业务引用校验。

Owner编码将整数字段规范为64位，布尔为1字节，float／double按IEEE位模式写入；读回核对目标类型范围。
容器数量为64位；数组按已登记固定长度写入，optional带严格布尔标记；map／set按原比较顺序编码，重复键拒绝。
非有限浮点拒绝，负零位模式保留。`rules`按固定目录重新绑定，不编码指针、C++对象布局或padding。
随机保存完整48位引擎状态或原磁带、模式和游标；账本直接恢复字段并核对报表，绝不重新执行收费。

预算：文件128MiB、单Owner64MiB、控制器1MiB、最多64分区和4096份审计历史；Owner共用16Mi节点及128MiB估计字段分配预算，嵌套深度64。
计数须先满足剩余载荷最小宽度与总预算，才允许分配。估计字段分配预算不等于进程总RSS，编码缓冲、容器实现和临时候选另有开销。
新局Owner实测169896字节、8516节点、140793字节估计分配；仅244份此规模历史已超两百万节点，因此没有采用不足的百万节点上限。
文件写入前用同一读器和共享预算完整自检，不能成功写出自己读不回的文件。

## 实际模块与持续覆盖

- `startup_world_codec.*`：有界基础类型和容器编码，私有随机／账本快照。
- `startup_world_codec_fields.inc/json`：92个可达结构、11个枚举的显式字段访问与清单；生成器同时登记两个私有包装类的原字段、别名和枚举数值。
- `startup_world_restore_validation.*`：不推进世界的目录、地图混合绑定、稳定ID／原序、可达实例、页面父子和载荷校验；合法退休历史与活动对象分开。
- `startup_world_persistence.cpp`：版本／来源／用途、分区、联合预算、全Session规范摘要及候选安装边界。
- `startup_world_file_io.*`：独占临时文件、刷新、关闭后回读校验、同目录原子替换；失败清临时文件并保留旧目标。未承诺所有文件系统的掉电耐久性。
- `startup_world_replay_driver.*`：原自然晋级／扩张自动玩家的外部策略与统计，逐字段往返；不属于原游戏状态。

新增Owner或可达类型字段必须更新清单，修改状态含义须更新语义版本并评估前缀资格。Clang构建的标准CTest重新生成AST检查覆盖；
32／64位生成身份已核对一致。生成工具只在构建／开发使用，运行游戏不依赖Clang、Node、APK或research/work。
普通实现提交不同不会机械拒绝快照；数据、字段布局、语义或控制器版本不兼容则明确拒绝，受前缀规则影响的快照另外标记失效。

## 回放命令与验收层级

原持续测试参数、断言及黄金终点保留。新增选项仅作用于`natural_progression`和`natural_expansion`；其它自然控制器暂未提供文件恢复。
历史布局的晋级38000→38282及扩张420→840三路结果保持各自版本；新增字段后当前晚期前缀尚未重新认证。
`--save-at`和`--stop-at`均为原循环零基frame，文件存下一帧；`--trace-from`决定从哪一轮记录完整Session摘要和控制器规范字节。
可选`--save-every N --save-directory <目录>`在正整数倍frame的同一轮末保存`prefix-<frame>.awr`，默认关闭，
不会代替主认证点；独占发布，已有同名快照明确拒绝覆盖。首次生成较长前缀时可启用，失败后保留最近成功档和现场。
这些周期前缀首先属于已生成且读器自检通过的候选，不自动称为已经单独完成两次尾段对照的认证快照。

```powershell
# 默认420轮完成后保存，继续到840；另启两个进程恢复并逐帧比较同一尾段。
node research/dungeon_village_1/prototype/tests/replay_file_test.mjs --exe research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests.exe --work-dir research/dungeon_village_1/work/validation/replay
# 扩张使用独立控制器；这是早期有界恢复检查，不含真实扩张里程碑。
node research/dungeon_village_1/prototype/tests/replay_file_test.mjs --exe research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests.exe --work-dir research/dungeon_village_1/work/validation/expansion-replay --scenario natural_expansion
# 首次中后期认证：只生成一次真实前缀，保持原38282黄金终点；成功后独占保留快照。
# 以下38000历史文件已是旧布局；新布局生成须使用新的独占文件名，不能覆盖旧证据。
node research/dungeon_village_1/prototype/tests/replay_file_test.mjs --exe research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests.exe --work-dir research/dungeon_village_1/work/validation/replay --save-at 38000 --stop-at complete --producer-revision <研究代码提交或源码指纹> --snapshot-file research/dungeon_village_1/work/snapshots/progression38000-e5a4.awr --save-every 5000 --save-directory research/dungeon_village_1/work/snapshots/progression-prefixes-e5a4
# 后续复用只运行相关尾段；current-qualified.awr是路径占位，须换成实际已核当前版本文件。
& research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests.exe natural_progression --load-file research/dungeon_village_1/work/snapshots/current-qualified.awr --trace-file research/dungeon_village_1/work/validation/tail.trace
```

runner核对三路逐帧全Session和Driver、尾段stdout／检查数、源文件未变；默认场景仍为晋级。
完整模式分别核对晋级38282／23388G／322697抽、扩张111808／138463G／1047804抽，不互借黄金值。
证书明确主快照及所覆盖尾段；周期保存只产生自检通过的候选，没有自动获得尾段认证。
短跨进程场景进标准CTest；完整认证按风险显式执行。认证记录要附来源版本、文件／尾段摘要、规模及耗时，不以尾段通过宣称当前代码重新从新局自然可达。
每批选择相关尾段；前缀依赖变化、快照首次生成或集成风险时才跑完整链。账本、任务及完整审计可合法增长，短期输出消费、无引用退休对象与候选释放另行检查。

扩张晚期建议一次生成108700前缀，先三路认证108701→108720，覆盖108713实际扩张与108715新区道路；
随后需要整月经营时再复用同前缀到111808。该晚期命令尚未执行，预算能否容纳全部历史仍未知。
晋级38000档35.09MB中历史占34.53MB；扩张终点的244份历史不能由早期档大小推定预算必然通过。
首次生成按30000周期保留合法候选，预算拒绝时保留最近成功档及失败现场，不删历史或提高预算凑通过。
runner现支持从既有前缀继续生成新的参考尾段，源文件只读；无需因中途预算失败重新跑新局。
默认`--load-prefix`消费与具体源hash绑定的三路证书；裸周期候选必须明确`--prefix-status candidate`及`--prefix-next-frame`，
工具核实际帧／场景／seed／speed／摘要，完整Owner及预算仍由C++加载器权威校验。
新捕获轮必须不早于源实际next_frame，之后两次新进程只比较新捕获点后的尾段；证书记录源资格及来源链。
候选恢复参考轨迹、已认证前缀恢复参考轨迹、新局不中断参考轨迹是三个等级，不能互换或将新尾段通过追溯升级成完整新局认证。
已成功加载的裸候选仍只认证本轮新捕获后的尾段，之前的自然可达性／历史重放需要独立证据。

```powershell
# 从已核当前版本前缀继续；文件名是占位，save/stop须匹配实际源轮数。
node research/dungeon_village_1/prototype/tests/replay_file_test.mjs --exe research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests.exe --work-dir research/dungeon_village_1/work/validation/replay-resume --load-prefix research/dungeon_village_1/work/snapshots/current-qualified.awr --save-at 38001 --stop-at 38002
# 裸周期候选例：文件实际下一帧421，440轮新捕获、比较441至460；场景与源身份必须相符。
# 此420候选亦属旧布局；当前调用应选择与当前codec身份相符的新候选。
node research/dungeon_village_1/prototype/tests/replay_file_test.mjs --exe research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests.exe --work-dir research/dungeon_village_1/work/validation/candidate-resume --scenario natural_expansion --load-prefix research/dungeon_village_1/work/expansion-replay-assessment/periodic-candidates/prefix-420.awr --prefix-status candidate --prefix-next-frame 421 --save-at 440 --stop-at 460
```

审核、短验收和未执行晚期命令见[扩张记录](../work/expansion-replay-assessment/README.md)；
既有前缀及裸候选入口的具体检查见[接续记录](../work/prefix-reference-replay/README.md)。没有放宽128MiB或删历史。
