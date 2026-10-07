# 文件存取与精确回放

本模块实现研究维护格式，不读取APK／Steam原档，不迁移旧demo或未知schema。
来源事实见[原存档分析](../rules/PERSISTENCE.md)，已确认方案见[快照设计](../stages/PERSISTENCE_REPLAY.md)。
本批已完成标准回归、正常窗口及38000帧跨进程快照验收，具体边界、耗时与哈希见[验证记录](../VERIFICATION.md)。
固定APK规则／原表与自然玩家黄金值不变；原版读档会丢失的随机／页面／部分效果，在维护回放格式中完整保留。
原版引用恢复／设施冒泡丢弃／日历续跑的后续来源见[恢复合同](../rules/PERSISTENCE.md#原版恢复合同缺失引用冒泡与日历2026-10-07)。
这些是原档加载行为，不改变本模块的精确状态保存与严格候选校验，也不让原中断档成为可直接续帧的维护快照。

## 入口与用途

公开接口为[文件存取](include/dungeon_village_prototype/startup_world_persistence.hpp)：
`save_startup_world_file`不修改Session；`load_startup_world_file`返回完整私有候选，失败只返回错误。
调用者先校验自己的控制器载荷，再同时安装候选Session和控制器；不得在解析控制器前先替换世界。

| 用途 | 保存边界 | 包含内容 |
| --- | --- | --- |
| normal | 稳定主场景、无建设操作或模态页、声音已消费 | 全Owner动态状态和随机；逻辑输入清零；不包含测试控制器或全部审计历史 |
| replay | 外层测试轮的update、玩家命令、声音消费及断言均完成 | 全Owner、原序审计历史、控制器版本／全部进度、下一帧与来源追溯；支持已完整校验的模态等待页 |

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
文件参数不能与inspect-page夹具混用。窗口字体仍按现有`--font`参数选择。

## 文件布局与资源预算

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
`--save-at`和`--stop-at`均为原循环零基frame，文件存下一帧；`--trace-from`决定从哪一轮记录完整Session摘要和控制器规范字节。
可选`--save-every N --save-directory <目录>`在正整数倍frame的同一轮末保存`prefix-<frame>.awr`，默认关闭，
不会代替主认证点；独占发布，已有同名快照明确拒绝覆盖。首次生成较长前缀时可启用，失败后保留最近成功档和现场。
这些周期前缀首先属于已生成且读器自检通过的候选，不自动称为已经单独完成两次尾段对照的认证快照。

```powershell
# 默认420轮完成后保存，继续到840；另启两个进程恢复并逐帧比较同一尾段。
node research/dungeon_village_1/prototype/tests/replay_file_test.mjs --exe research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests.exe --work-dir research/dungeon_village_1/work/validation/replay
# 首次中后期认证：只生成一次真实前缀，保持原38282黄金终点；成功后独占保留快照。
node research/dungeon_village_1/prototype/tests/replay_file_test.mjs --exe research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests.exe --work-dir research/dungeon_village_1/work/validation/replay --save-at 38000 --stop-at complete --producer-revision <研究代码提交或源码指纹> --snapshot-file research/dungeon_village_1/work/snapshots/progression38000.awr --save-every 5000 --save-directory research/dungeon_village_1/work/snapshots/progression-prefixes
# 已认证快照的后续复用，只运行相关尾段，不重新生成前缀。
& research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests.exe natural_progression --load-file research/dungeon_village_1/work/snapshots/progression38000.awr --trace-file research/dungeon_village_1/work/validation/tail.trace
```

runner核对三路逐帧全Session和Driver、尾段stdout／检查数、源文件未变；完整模式另核对既有38282／23388G／322697抽。
短跨进程场景进标准CTest；完整认证按风险显式执行。认证记录要附来源版本、文件／尾段摘要、规模及耗时，不以尾段通过宣称当前代码重新从新局自然可达。
每批选择相关尾段；前缀依赖变化、快照首次生成或集成风险时才跑完整链。账本、任务及完整审计可合法增长，短期输出消费、无引用退休对象与候选释放另行检查。
