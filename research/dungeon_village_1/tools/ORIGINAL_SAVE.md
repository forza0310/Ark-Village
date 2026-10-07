# 原版存档只读检查

`kairo_save_inspect`读取原记录的外壳和tag容器，输出字段路径、类型、解壳后偏移、长度及值／摘要，
也可比较两份记录。显式审计模式进一步检查25分区布局和已证引用，只输出数值摘要。
它不执行原游戏代码，不重建维护Owner，不修改输入，不提供原档写回或迁移接口。
原版合同见[存档研究](../rules/PERSISTENCE.md)，实际样本与验收见[验证入口](../VERIFICATION.md)。

## 显式选择格式

| 参数 | 输入与来源范围 |
| --- | --- |
| `container` | 已解壳的tag容器，按APK非null布局读取；不验证外壳或判定所属游戏版本 |
| `apk-record` | 固定APK记录的二进制外壳：44字节全局key循环XOR，BE64游戏校验值＋容器 |
| `apk-base64` | Android偏好中单个记录字符串；先Base64再同上。不要传整个XML；原读器的GZIP分支当前明确拒绝 |
| `steam-record --steam-id ID` | 当前已登记Steam样本的文件；key是SteamID的8字节小端表示，与APK不同 |

SteamID来自用户自己的存档账号目录名，只用于本机解析；工具不查询登录状态、不联网、不打印该参数。
不猜测key，不把错误key导致的校验失败当作“没有存档”。无magic的旧格式不能仅凭校验通过确认版本身份。
文件和实际安装的GameAssembly／metadata需另登记哈希，不能对未知更新版自动承诺兼容。

```powershell
# 显式填入自己的数字目录名，只读原件或研究副本；输出可重定向到研究work。
& research/dungeon_village_1/work/release/bin/kairo_save_inspect.exe --format steam-record --steam-id <自己的SteamID> <记录文件>
# 同一种来源／key下的两份记录，报告有变化的索引路径。
& research/dungeon_village_1/work/release/bin/kairo_save_inspect.exe --format steam-record --steam-id <自己的SteamID> <较早记录> --compare <较晚记录>
# 固定APK的单条Base64文本。
& research/dungeon_village_1/work/release/bin/kairo_save_inspect.exe --format apk-base64 <记录文本>
```

未提供SteamID、在APK模式误传SteamID、非法或溢出参数均拒绝；十进制ID未打印到诊断。
命令本身有可能保留在调用方的本地进程／终端记录中，研究证据不包含账号值或派生key。

## 25分区只读审计

在文件参数前添加`--audit-profile apk108`或`--audit-profile steam-9cf4bb10`。
后者仅绑定已登记GameAssembly哈希前缀，实际输入身份仍需另行核对，不通过字段形状猜游戏版本。
`apk-record/apk-base64`只配APK profile，`steam-record`只配Steam profile；`container`由调用者显式选择。
本审计针对世界根`1/16/3/1/25`、事件子数组APK200／Steam228项，系统根、空档及null分区明确拒绝。

```powershell
& research/dungeon_village_1/work/release/bin/kairo_save_inspect.exe --format steam-record --steam-id <自己的SteamID> --audit-profile steam-9cf4bb10 <记录文件>
# 两份审计均通过后才输出before/after摘要；该模式不输出原字符串或容器原值差分。
& research/dungeon_village_1/work/release/bin/kairo_save_inspect.exe --format steam-record --steam-id <自己的SteamID> --audit-profile steam-9cf4bb10 <较早记录> --compare <较晚记录>
```

报告列出每段字节数／精确消费数／记录数／非规范bool数量、14组引用诊断及固定数值事实。
人物、怪物、遭遇、任务、设施分别建身份集合；重复身份计数保留，不把重复UID某条遭遇状态当唯一事实。
标量引用排除已证`-1`空值，重复引用仍逐次计数；设施`w`用完整定义／序号二元键，未证存在`-1`空键政策，故原样检查。
缺目标及非规范bool只报告，不自动判坏档、不执行原加载省略或修复；缺引用明细每组至多16项，总次数不截断。
投射物源／目标域、地图几何及完整旧格式修复仍未认证，`partition.19.reference_audit_covered=0`明确保留缺口。

数值摘要包含原年月周/time/oldtime、64位现金、设施p／k数量和唯一遭遇的自身／组状态。
`user.tp_clear.*`只计算已证年月纠正条件及候选，不改变根值，不推进日期，也不执行读档。
默认值与特殊边界按[原版恢复合同](../rules/PERSISTENCE.md#原版恢复合同缺失引用冒泡与日历2026-10-07)解释。
通过布局审计不等于该档可安全导入维护Owner，更不等于原程序已实际加载成功。

## 输出含义与能力边界

路径`$/tag/index`保持原容器索引；嵌套时继续追加子容器的tag/index。偏移从**解壳容器**第0字节算起。
`group`记录原tag和数量；`int32/int64`按原大端有符号数解释；`string-bytes`输出原十六进制，
不猜字符集、不经过解码再编码；`opaque-bytes`保留原容器内字节，报告长度和SHA-256。
Steam tag4长度`-1`单独输出`null-bytes`，不同于长度0的空数组；APK模式不接受该协议。
Steam写字符串用UTF-8，但读取存在字符集检测；APK写端未显式选字符集，故显示文本不能代替原字节。

`$/4/0`—`$/4/24`在已登记游戏根中对应25业务分区。未指定审计profile时，仍仅作为不透明字节段报告，
不把分区摘要变动解释为某个具体玩法效果。显式审计的字段边界与已覆盖引用见上节；详细来源见存档规格。
容器同索引并不证明角色／任务身份相同，两个时间不同的已有档案也不是单一操作的受控前后样本。
比较以字段类型、原值／摘要及长度为准，不把前面字符串增长造成的后续偏移变化误报为业务值改变。

空记录明确返回`empty_record=1`，不补新局。文件失败或结构错误返回非零，错误通常附容器偏移；
全部解析和比较成功后才输出报告，不输出一半“成功”结果。输出原始字符串十六进制仍属于样本内容，保留在忽略work中。

## 独立保护与验证

保护不是原游戏的读取宽容策略：单文件16MiB、全树最多131072行、深度32；
长度先核对剩余字节再读取，嵌套共用一个缓冲区和全局行预算。未知tag没有整组长度，明确拒绝；
重复tag、负计数、截断、尾字节、非零校验高32位、校验不匹配和不支持的GZIP均拒绝。
Base64只接普通字母表及空白，填充位置和未使用位严格校验，不复制原读器宽松行为。
业务审计另限制单动态计数10000、25段共享131072动态项；先核对剩余字节，再分配或迭代。
字符串／浮点／尚未解释字段只消费原字节，不重编码、补零或伪造语义。公开Inspection重新验证容器，不能用过期偏移绕过布局校验。

集中用例扩展既有`dungeon_village_tools.archive`套件，覆盖手工字节、所有截断点、整数符号边界、
APK／Steam合成key分离、Steam null、损坏校验、递归／行／文件预算，以及一字段变化的差分定位。
公开合成SteamID仅是固定测试数值，不使用用户账号生成提交夹具。真实用户存档及报告保留在忽略work，另按哈希核验。
25分区用例继续归入archive套件，在`original_save_audit_checks.cpp`组织手工完整容器与边界；
包括真实样本为空的0／17／19非空夹具、每段截断／尾字节、嵌套计数、跨段预算、64位现金、重复／缺失引用及年月候选。
合成覆盖不等于这些分支已取得原游戏自然样本，验收级别分开登记。
后续用户交回的非空p中断档已实际含0段2脚本、17段1效果、19段1投射物，C++／独立Node精确布局交叉通过，
见[候选分析](../work/nonempty-p-candidate-analysis/ANALYSIS.md)。这只提高该实样布局证据等级，19源／目标引用域仍未认证；
没有因此新增原档导入、写回或完整原游戏恢复能力。

实现职责：`original_save.*`为有界外壳／容器解析和差分；`original_save_audit.*`为25分区与引用审计；`save_inspect.cpp`为只读命令行；
复用已有`archive`中的游戏校验／APK key及共同SHA-256库，不在原型或产品中增加第二套世界恢复。
