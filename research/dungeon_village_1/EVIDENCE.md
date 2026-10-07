# 证据清单与符号映射

跨版本分析按[研究流程](../WORKFLOW.md)执行；本页集中登记输入与工具，功能结论在对应规格中标明所属样本和证据等级。
2026-10-06用户确认将Windows制品作为独立交叉来源纳入流程；本页APK身份、原表及既有回归的来源不变。

## 输入与工具

| 项目 | 观测值 | 证据等级 |
| --- | --- | --- |
| 输入 | 研究目录内的 `maoxianmigongcun.apk` | 直接事实 |
| SHA-256 | `1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5` | 直接事实 |
| 包名 | `net.kairosoft.android.bouken_ja` | 直接事实 |
| 版本 | `1.0.8`，版本代码 9 | 直接事实 |
| 运行时形态 | 单个 Dalvik/Java 应用，不含原生库 | 直接事实 |
| 签名/来源限制 | 包含汉化内容并使用 Android 调试签名证书 | 直接事实 |
| 反编译器 | JADX 1.5.6；早期macOS使用Homebrew，本次Windows恢复使用官方发布CLI，详见下文 | 直接事实 |
| 反编译规模 | 188 个类、1,953 个方法、295,672 条指令，并生成调用图 | 工具报告 |

包身份和哈希将所有结论固定到这一输入。调试证书和汉化字符串意味着不能把该安装包描述为未经修改的
官方发行版。

2026-10-06用户另提供其购买的Steam Windows安装目录，仅授权并行评估可分析性与未来切换成本。
[静态可行性评估](verification/STEAM_ASSESSMENT.md)独立登记PE／IL2CPP、文件哈希和版本差异边界；
它不是固定APK的补丁或等价认证，本页APK身份、现有原表及黄金轨迹不变，尚未决定更换正式研究基础。

新局七类缺口的本轮研究见 [新局、到访与建设](rules/STARTUP.md)，原始字节与推导分开发布到
[新局数据包](data/startup/README.md)。地图保留未消费的两个零字节，不作为完整格式已闭合的声明。
首名自动到访是人物表flags8命中的ID1；首次教程对话ID69与页面69、普通解锁页59分别登记。
既有低层 `UserData.java` 的 `L206→L244→L450→L469→L4c9`佐证创建、加入和事件89的局部顺序。
加载后格/身份的静态重建使用新增 `work/startup-map-fallback/Map.java`，
JADX参数为 `--single-class c.h --single-class-output <路径> --decompilation-mode fallback --no-res --log-level warn`。
输出SHA-256为 `faa6ac955724a8d32937a60b45a40c87cace6c0274a9a2a5ca6b3f1c0f84f6fe`，仍不提交生成Java。
低层修正常规g()循环跳转误读，c()/d()/实例分配小函数交叉定位见[新局报告](rules/STARTUP.md#加载后逻辑显示快照)。
本次村办类型3研究在Windows以同版JADX恢复该路径，增加`--config none`，按LF归一化后的SHA-256与上值一致。
`Map.java:1021`的g及`3596`的f用于核对扩张：`L1fd→Lc6→Lcf→L93`证明非flags16实例只跳过本项，
常规`c/h.java:365`的`break`不能作为提前结束扫描的依据。扩张仍是固定24×24地图内的边界级别切换，
逐次删除／创建／邻接及人物重置见[建筑合同](rules/STATE_CONSTRUCTION.md#村办类型3地图扩张)。
恢复输出仍属于忽略的研究证据缓存，不提交生成Java，也不将静态顺序核对称为APK动态轨迹认证。
运动/路点/逻辑格进入复用既有Character低层输出，数值与巨型P()局部证据分级见[连续运动](rules/CHARACTERS.md#continuous-motion)。

本轮新增单类fallback：`work/first-visit-fallback/MainScene.java`（b.c）与
`work/weapon-choice-fallback/Weapon.java`（a.p），参数均为
`--config none --decompilation-mode fallback --no-res --single-class <类> --single-class-output <路径>`。
SHA-256依次为`cadcb21544111c305214c67f4e25724a17ada111b3c728f7ed20b9772ddf21bb`、
`abeb2ed48ccac67c702cbca1c6113d9e5e678c9addfea35273b750e1cba3c66d`。
主场景自动扫描/延迟局部见[首访前置](rules/CHARACTERS.md#first-activity)，
武器消费者及A[0]赋6见[武器报告](rules/FACILITY_USE.md#weapon-choice)，
道路补块复用既有Map低层与小型图片/深度消费者，见[绘制交付](ui/README.md#road-patches)。
生成Java保持忽略、不提交；局部交叉证据不消除整套巨型方法的警告。

2026-10-04人物/怪物AI专项新增`work/ai-fallback/Encounter.java`（c.f）和
`work/ai-fallback/Projectile.java`（c.j），参数为
`--config none --decompilation-mode fallback --no-res --log-level warn --single-class <类> --single-class-output <路径>`。
SHA-256分别为`6926d7ff344accf558986f1c2fa1e44d97227a1208ee5f5298490c626f87799c`、
`b386c299543a294b012361a42ab3256132a7f557dc1bb6e226fa1f2ee84d18c6`。
复用Character低层哈希`4bd10321f4a6a256664f5700dcc9dd71638e3c66c4fb9cb5634b1d96d712a3bd`和既有Map低层。
[AI入口](rules/ai/README.md)逐功能记录普通/低层定位、维护C++职责、未闭合副作用与组合条件。
F/G差异、o活动6条件、d的8且9条件、攻击整数除法、影响场除2掩码已有低层交叉；
事件完整奖励已与低层740–1513交叉，e.K近期击杀数和N/O职业成长消费者另由小方法佐证；
状态setter、漫游与装备提交扩展见[控制规格](rules/ai/CONTROL.md#状态setter和装备提交)。
完整全局运行仍不以一份普通输出宣称等价。生成文件只留忽略work，不纳入发布源或Git。

## 恢复行为交叉证据（2026-10-07）

并行续批交付[S038–S051截图](references/screenshots/2026-10-07-steam-restore/README.md)和[Steam交互](ui/STEAM_INTERACTIONS.md)，
另补退出委托、普通文件失败及非空p产生链。截图源扩展名.png实际JPEG/JFIF，归档只规范目标为.jpg、不重编码。
用户本轮明确授权截图作UI设计参考；村名不转录到说明，完整图不作为产品直接运行美术，不混入APK基线。

| 并行续批证据 | SHA-256 |
| --- | --- |
| references/screenshots/2026-10-07-steam-restore/MANIFEST.tsv | `a37ac8002e6a546899638ef2946bdf6cdabc92bb20ff61db1f56e5ee773014eb` |
| work/steam-interaction-analysis/manifest.json | `421064c44371102887421bb0e8956bfe7e0afd665930328e78421fb3c9a11b86` |
| work/steam-interaction-analysis/disassembly.json | `005abbee8899f36c84db175973a7dcfeab6cb478b197e700a9773e8cc5effbcf` |
| work/steam-save-lifecycle-analysis/manifest.json | `10f49141a2b832a97bdc176e1b81d79c45068d553750407178e86ee8089e4695` |
| work/steam-save-lifecycle-analysis/slot.json | `8a30efdf95529cb3807b9ba1cfe1cd2d8d81f26ad4d135902fdf976e33f70129` |
| work/steam-save-lifecycle-analysis/disassembly.json | `339036497b364348553979075508035abf00d33e7ac2d3a0dac585b60cd9546f` |
| work/nonempty-p-capture/trigger-static.json | `6e03fcc1843be6054b3dd5ea567ae9b924c53bd88a46727674417fdc6795e1ca` |
| work/nonempty-p-capture/capture.cjs | `ed721039e2d13475e28f8fd1f216b719b88faeb6fe4e8c4463a20dcf703bd567` |

交互19方法／89,840字节，生命周期13方法／5,808字节，p产生与消费5方法／11,552字节分别受白名单预算约束。
所有机器码仅只读解码，不运行DLL。生命周期全区泛型虚槽扫描被自动审批拒绝，已改精确方法研究；
拒绝范围与未闭合SaveAll上游见[本地报告](work/steam-save-lifecycle-analysis/README.md)，不以候选未发现证明不可达。
只读p采样60秒未命中，其零候选结果见[记录](work/nonempty-p-capture/TRIGGERS.md)，不冒充已验证原动态恢复。

用户随后交回[Steam窗口实验](work/window-restore-observation/20261007-102645-backup-restore-trials/RESULT.md)：标题实见ver2.56，
本机Steam appID1859360、buildID14371088；GameAssembly／metadata仍为此前固定身份。原报告登记截图尺寸，不把截图像素当逻辑客户区／DPI。
本会话[只读交叉](work/window-save-analysis/ANALYSIS.md)核验37截图、28记录文件、7清单共72项，
其中游戏档14路径／9种内容、225段及126引用组一致。静态、原窗口观察、保存端点与未覆盖部分在[合同](rules/PERSISTENCE.md#steam窗口恢复实验与密文交叉2026-10-07)分别登记。
已验证的端点包括163不在后续输出、道具库存／共享改良落盘，以及中断自然推进后的旧周自动保存；非空p及候选重载尚未完成。

| 窗口实验交回证据（相对work） | SHA-256 |
| --- | --- |
| window-restore-observation/20261007-102645-backup-restore-trials/RESULT.md | `9fdea748a3926042ba0af02bb9ddd4ba651e1fc66aa7ea2ea9cfc443a45ce48d` |
| window-restore-observation/20261007-102645-backup-restore-trials/EVIDENCE_INDEX.json | `62cf22c2b9b94e1c75cbb2230ab71bd03d7ef2e379eb92caa0ae0036aacc0a04` |
| window-save-analysis/analyze.cjs | `f84748d0cadf81cae44684018aca8c718435d69e00d8bcac72bccdb99f1c42ac` |
| window-save-analysis/AUDIT.json | `24a25eee3676359cb6c52903e30a4fee3f470829a3729f6b22fa0222aa100a61` |

输入明细／密文／可能含名字的截图只留本地work，不提交原程序或用户档。实时原目录在分析期间再次变化，另见工作记录，
不因此改变冻结实验期望或覆盖原件；最终分析仅依交回清单与完整备份。

无窗口续批将已核25分区／引用合同接入[审计工具](tools/ORIGINAL_SAVE.md#25分区只读审计)，
新增[Steam静态补证](work/headless-save-audit-analysis/steam-static/README.md)：24个明确方法共45,664字节，
另复用既有LoadGame 20,544字节与16字节共享空入口。默认步长27有初始化指令证据；完整运行期写入及退出委托仍未闭合。
两份既有游戏档的50段／28引用组与独立Node探针一致，年月纠正条件均为false；没有运行原游戏或生成动态新档。

| 无窗口续批证据（相对work） | SHA-256 |
| --- | --- |
| headless-save-audit-analysis/steam-static/manifest.json | `db61bd97e7f7bad9379d427bfda7b86b6a12bf5de98b0ad4c0096dff8721d5f3` |
| headless-save-audit-analysis/steam-static/disassembly.json | `b167cbdf31f29c6320b80e79996251fba276eb277eec7af7023fdc493d4959c4` |
| headless-save-audit-analysis/steam-static/evidence.json | `c5913a24a44f15ab656fabd90e76085750a70393c0dd9f57f3fb3ce33cda1597` |
| headless-save-audit-analysis/steam-static/save-xrefs.json | `6b31f98ac90c12cfaae63fa1b5a6e12aca3be735d45cb430e6f4e40f59986417` |
| original-save-analysis/audit-cli-validation.json | `62e1e20633e82756e48fcdb598e12f5c7dfe197114d550924fc152aee852dbf8` |
| original-save-analysis/audit-calendar-crosscheck.json | `cbcd278b6c0c9eb3abda4f3334f7f1576bc2f7a942ce3c5b57975b9f6a46a32c` |

审计只输出无账号／key／名字的数值、计数、哈希和有界缺引用值；原字节未写回，原件与密文副本前后哈希一致。
原游戏动态交给用户的另一对话，实验提示词与反馈模板留在`work/window-restore-observation/`，不将准备实验写成已验证。

用户继续迭代，沿既有只读副本与脱敏摘要保存授权，新增[恢复合同](rules/PERSISTENCE.md#原版恢复合同缺失引用冒泡与日历2026-10-07)。
原APK／Steam输入身份均未变；窗口native pipe不可用，本轮未运行原游戏或写回存档。
APK日历仅复用既有普通／低层文件，设施局部来源为`c/m.java`与`a/o.java`；Steam只读明确方法范围。
引用恢复13个方法范围／9216字节，另核3个泛型MethodInfo；日历／加载14个新增目标／37504字节，
另复用先前LoadGame的20544字节；方法尾界可能含padding，不把它当作新语义或完整全路径证明。

| 证据（相对work/restore-behavior-analysis） | SHA-256 |
| --- | --- |
| steam-references/restore-methods.json | `b1efe7656ca60d484697beefbfde337e7e39b6b14f09bbe6dbf596eeadfa2a89` |
| steam-references/direct-callees.json | `d65a3ce4b3224518500096b2f60079e3d9bb31e8807e163bf50c25158c4727fc` |
| steam-references/generic-bindings.json | `238ed2bd3af91306dc0352a0819b3184f6d24720885c4b8c9272f152e10f63ea` |
| steam-calendar/manifest.json | `29c6b1857aca880190b51269a39497a67609afcb033f24ec9a49b28801e57db9` |
| steam-calendar/calendar-disassembly.json | `8eceba2d529db9743ec25319c928049d53c7c062823dd31eeb99cfc81c8e5536` |
| apk-calendar/README.md | `ff60a13d392e54dbb14ded4cc1cb9ea7e8d9e3aa6ceffe5a7b1660665d77bf26` |
| effects-sample.json | `fd0b21fef5d31e380882d74ae3bccb23585737acb4f549cf10fce421509f8d83` |

设施普通来源hash：`work/decompiled/sources/c/m.java`为`1002d55a70323e7d03f632bd5fff1c4d4ca8195a5c056383e2d9ab2b8716945c`，
`a/o.java`为`f1a7f4fa36de27a685623d5a0ed6c2a284d8d1cb1efc58706ea97af20c7d9603`。
报告入口：[Steam引用](work/restore-behavior-analysis/steam-references/REFERENCES.md)、
[Steam日历](work/restore-behavior-analysis/steam-calendar/README.md)、[APK日历](work/restore-behavior-analysis/apk-calendar/README.md)、
[设施表现](work/restore-behavior-analysis/EFFECTS.md)。样本只补业务状态／计数，没有原程序加载后导出或非空冒泡覆盖。
账号、key、姓名与完整明文不进入报告；原件和密文研究副本的哈希在收尾复核，动态和静态分别验收。

## 存取与快照交叉证据（2026-10-07）

后续原档只读研究的新增静态证据保留在`work/original-save-analysis/`，复用同一APK／DLL／metadata身份。
[APK字节合同](work/original-save-analysis/apk/CONTRACT.md)补25段字段顺序和13个固定目录长度，
[配置链](work/original-save-analysis/exe/CONFIG_CHAIN.md)补正常根应用配置传播，
[Steam记录合同](work/original-save-analysis/steam-codec/README.md)补独立key、Property／null／字符串差异。
配置链新增5方法／2800字节，字节合同有界解码36方法／11184字节，均先验输入哈希、按已映射方法范围处理，不加载原DLL。

| 新静态证据（相对work/original-save-analysis） | SHA-256 |
| --- | --- |
| exe/config-chain-disassembly.json | `7f6992849c046bf14c3f65ce5b30e79119bec2a1af3fe62c9c4fc207b280e7bd` |
| exe/config-chain-strings.json | `73877c253d9727d23a9142eb0ddc1147a369f26a28d38e4d8ed5ab99c016a70b` |
| exe/config-chain-type-references.json | `c9c0c162aa4554cb046e946f5f49bb54d3515c9b1ea288389b62cf58632a8dbe` |
| steam-codec/manifest.json | `30d10efbc810b379f82d69e8f7612fdc35099fbadbff1e10662047cd14ddba7f` |
| steam-codec/disassembly.json | `91e0786336dbd91427c34f092d2790058620d80c98d1dab09e3193efd688f8c8` |
| apk/directories.json | `76f0eae05c981e1ff5453337c337823549cba7bb7118c06107c53afc8802ad4d` |
| apk/envelope-probe.json | `b7baf897f1950635fcc444a517edce534da662e7a6dcfd2fb4c457146e399120` |

真实样本身份另行登记，不由静态证据替代：

本轮用户提供的Steam记录已按明确授权只读复制；本地观察路径为安装目录下`saves/<SteamID>/000N`，不登记账号值。
实际安装DLL／metadata仍为本页先前固定哈希；样本不是本轮受控操作生成，不按时间戳推断单一行为。

| 记录 | 字节数 | 密文文件SHA-256 |
| --- | ---: | --- |
| 0000 | 301 | `99d4cab451959136cab3667c0e9926415bc1d800a7e865fa58e994c4f7c13207` |
| 0001 | 31666 | `843fb112add603ef9c92d8d79073e4a86bab03dd91f9847b3bf9102e96bcd288` |
| 0002 | 0 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| 0003 | 31892 | `88eaf11ba294fd173f21a4a758a72cf0e9f1cbaec64d302e734e26707757cfce` |

副本位于忽略的`work/original-save-analysis/samples/steam-existing`，复制前后原件与副本哈希一致；
校验、形状与原字节往返见[最小验证数据](work/original-save-analysis/steam-codec/sample-validation.json)。
维护交付是只读工具和不含账号／key／村名／完整明文的研究摘要，不发布原档或原代码。

以下为前批历史证据：

来源合同见[存档规格](rules/PERSISTENCE.md)，维护设计见[快照与回放](stages/PERSISTENCE_REPLAY.md)。
本批为静态研究与有界独立探针，不运行原游戏，不更换APK规则输入，不交付文件codec。
生成Java、IL2CPP映射和探针只保留在忽略的`work/persistence-replay-analysis/`，不作为维护实现或产品素材。

APK使用既有JADX1.5.6／同一固定输入，单类参数：
`--config none --decompilation-mode fallback --no-res --log-level warn -j 1 --single-class <类> --single-class-output <文件>`。
Java堆上限768MB，配置／缓存／临时路径在项目内。25分区读写与九项来源身份见
[APK索引](work/persistence-replay-analysis/apk/evidence.json)，可用同目录`evidence.cjs`复做。

| 新增低层文件 | 类 | 原始SHA-256 | LF归一化SHA-256 |
| --- | --- | --- | --- |
| `apk/Coordinator.java` | d.a | `bda557b86211b0a594fc968663bcda3e763e266bd5478f881a75023143a2c1cb` | `ee26e35483ad14b26938fc378c6d6b41563ff12109aa4e918c9a286609bab1e8` |
| `apk/Tenant.java` | c.m | `ccd147e5b95b6a28a341e68d5d82ed36c5ae0ad2d70393c7a28c963c15127a18` | `fa39882c6a943413f6a88141af5641b7a899b76ebbde955f969c87920a321596` |
| `apk/Encounter.java` | c.f | `6d500b0d95fd644ece189ee293119200c2bad0ddfd258188ccd72a54286ec862` | `64db52f28ddc775da690c082db4df18ea9d3bd6db456a089c32659ed1e8c410f` |

Steam输入保持[既有清单](verification/STEAM_ASSESSMENT.md#输入身份与已证事实)：GameAssembly SHA-256
`9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a`，metadata
`80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369`；游戏buildID仍未知。

| 工具／输出 | 身份与范围 |
| --- | --- |
| Il2CppDumper6.7.46 | 官方`Il2CppDumper-net7-v6.7.46.zip`，SHA-256 `c7f365347ce6bd816cdf774830e44ac46aea59c391c94f388605667394d6aba2` |
| 宿主与参数 | .NET8.0.24 x64，`--roll-forward Major`运行net7工具；GenerateDummyDll／GenerateStruct／RequireAnyKey均false；样本x86 |
| 静态注册地址 | ImageBase `0x10000000`；CodeRegistration VA `0x10E13B44`；MetadataRegistration VA `0x10F59638`；37942条非零映射经PE节表独立核对 |
| iced-x86 1.21.0 | npm发布包`https://registry.npmjs.org/iced-x86/-/iced-x86-1.21.0.tgz`，SHA-256 `88abc8fa5a1aeb0fc0e13b43689410e5e12474cba0392d38b1928636ba56e7e6`；解压调用WASM，无安装脚本 |
| `exe/analyze.cjs` | SHA-256 `f85475614faf00fc874efa5de416b4f792269a88efc494ec70d35137c84507f1`；映射核对及51个代表方法解码 |
| `exe/disassembly.json` | SHA-256 `c9a1bf2cfc3845905753b159cd1d338453b146ae6faf4cce34071732a7244b45`；真实字节／指令，非dummy方法体 |

运行前审查了工具Program.cs／PELoader.cs。静态搜索失败会尝试PE加载，但本次x64宿主配x86样本在回退载入前会拒绝位数；
本次实际静态搜索直接成功，未走加载回退、未执行原DLL。详细日志、地址／文件偏移和方法结论见
[Steam技术报告](work/persistence-replay-analysis/exe/README.md)。解码按下个方法地址限界且上限64KiB，可能含填充，不是完整CFG／异常恢复。

| 维护策略探针 | 可复查材料及SHA-256 |
| --- | --- |
| 容器不透明保留 | `container-probe.cjs`：`261e18ee23e8b5b24b7accc6b5ff1b342d558f31a9b3cae9765dec6a69be179b`；独立手工86字节夹具，非真实游戏档 |
| 容器夹具输入／改写后 | `f39cbfc686c9a474e66402aa61e7f4b489dd0a3dac16ce41a2269c19c0bb3355`／`1287f320e997629a1bf0594bd76ac78769bc8c7547d78d622dde22a08acf00a2` |
| C++ Session分叉 | `runtime/replay_probe.cpp`：`ac957e5e0a6d759e5d6d72aefca9f6e226c19b23a7186d12e7deb1ef57cc0114` |
| 最终运行日志 | `runtime/replay_probe.log`：`f4fcf21e4fe3c3b8ffecff5010eec50d026500d9e3f3aa20c5e4019e85d9bc0f`；18.7482秒，退出0 |

C++探针以`898b6536e71bea6240620d3407e8483030142895`为研究基线，用既有LLVM-MinGW i686，
`-std=c++17 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Werror`，链接唯一Release导入库，未另建构建树。
startup_world DLL SHA-256 `0087687d634615035267e5ac339de2903f75a23462ba4fccaceb2ebb51d5f388`，
reference DLL SHA-256 `bb0da82464cf1d8cdf0f5a89c90a3b0f02b18982945738631030624a6d6640cf`。
同进程值复制、已列观测字段及随机未来样本一致，不等于全字段codec或跨进程认证；覆盖与反例见
[探针报告](work/persistence-replay-analysis/runtime/REPORT.md)。临时可执行文件验后清理，源／日志保留。

后续实施期另作一次有界Steam存储补证，未覆盖上述51方法证据文件。仍用同一DLL／metadata及iced-x861.21.0，
独立输出13＋5个局部方法和6个字符串使用槽；前组方法区间上限64KiB、后组16KiB，输入哈希重新核对一致，未执行原程序。
结论与VA／RVA／文件偏移见[存储局部报告](work/persistence-replay-analysis/exe/STORAGE_LOCAL.md)：
初始化setter实际写false、media4根Storage与条件路径已定位，配置加载后的实际USE_STEAM／ROOT_FOLDER仍未知。

| 新增输出 | SHA-256 |
| --- | --- |
| `exe/storage-local-disassembly.json` | `8968b875830fa074fb09d2b57ce9a828d5785e81385a3cb8fbbba43707f4f398` |
| `exe/storage-tail-disassembly.json` | `133d47e461e1f5338af3e86acd370ae958780362067d747c36675a578d7b7066` |
| `exe/storage-local-strings.json` | `8c7e362b16bbf15b0b3d726853fd7e6113555e3fa5d96c5be4aa71cda89c0f57` |

维护文件的字段身份由自有C++声明生成，不来自反编译实现。当前92结构／11枚举、私有包装字段及别名清单见
[codec字段清单](prototype/src/startup_world_codec_fields.json)，SHA-256为
`0500cff0cd937c6967836c6bf7c594ff43dd64c23f408e7ea9188e4fc3423403`。
数据集身份另外覆盖15份已发布原表／脚本／新局输入；正常文件、测试控制器与原存档的契约分开，见[模块说明](prototype/PERSISTENCE.md)。

## 共同世界接管与运行证据

2026-10-06 Windows恢复：用户补回`maoxianmigongcun.apk`，SHA-256与本页固定输入完全相同。
使用官方JADX1.5.6发布包（ZIP SHA-256 `545ea2be9c242511bc145755cf4bda2485ade42966e096f8b4d3da2a230e8974`），
通过CLI类`jadx.cli.JadxCLI`恢复普通输出及单类fallback；工具、配置、临时文件和生成证据均留项目work，
不写入产品素材或维护实现。普通参数为`--config none --show-bad-code --comments-level warn -j 2`，
低层仍沿本节既有fallback参数。

| Windows输出 | 原始字节SHA-256 | 换行为LF后的SHA-256 |
| --- | --- | --- |
| `work/world-page-fallback/GamePage.java` | `15d1f2232da54406cbbbf67bc059de9dace51a388672db4bff9f2110cb986fc6` | `e4b7eea7f8b2ecb40c0a49ba21cc8175c8177f9ac92222fa1d75b45193de2e56` |
| `work/construction-render-fallback/MainScene.java` | `a64b18b56d600bfd6dffe7907cc56b701bcc18743ffc7d51dd3b2c0ba69cba84` | `cadcb21544111c305214c67f4e25724a17ada111b3c728f7ed20b9772ddf21bb` |

两份LF哈希分别与已有GamePage／MainScene证据一致，差异仅平台换行；没有修改固定APK、原表或原图。
raw95输入及领取分支由普通`b/g.java:6116`与GamePage低层21341起`L1e92→L1f84`交叉，
恰1声音、40快进及领取后关闭得到支持；实际现金／点数／职业／用户flag／勋章／活动效果按小方法分别核对，
维护合同见[周期账本](rules/ACCOUNTING.md)。候选建筑闪烁／两朝向／原21裁剪见[绘制映射](ui/PAGES.md)。
本轮是固定输入静态研究，不认证原APK动态、OS输入或产品窗口已经符合。

2026-10-05可持续世界补充低层单类`work/world-page-fallback/GamePage.java`（b.g）。
JADX参数为`--config none --decompilation-mode fallback --no-res --log-level warn --single-class b.g --single-class-output <路径>`，
SHA-256为`e4b7eea7f8b2ecb40c0a49ba21cc8175c8177f9ac92222fa1d75b45193de2e56`；生成Java保持忽略。
页49初始化低层13463起`L11→L24`、确认17307起`L6e→L81→L1a8`交叉普通3393–3398、4538–4623：
只重算晋级条件；确认置u8，不调用页48的晋级副作用，rank5初始化执行事件48后关闭。
原框架`kairo/android/a/b.java:190–232`在页回调设置j并finally清空；同轮新增页不能替代执行锚，
下一入口移除关闭页后重建执行根。维护checkpoint不保存回调临时根。

长跑新增停点分别由原小消费者闭合：`b/g.java:4815–4887`的raw56/57读取第一怪物u或第一任务绑定Tenant.f()，
后者复用`c/m.java:865–895`的真实首占地形状坐标；raw57无绑定不能以任务site替代。
指令7在`d/a.java:721–724`调用`b/g.a(L):252–255`创建raw16；`b/g.b:11089–11100,11530–11534`
先减L，剩余为正提前返回，到零后同次减f并关闭，不接受玩家确认跳过。
raw89确认`b/g.java:6003–6011`早于40只快进，之后关页。raw87年度来源与独立规则见
[年度最小闭环](rules/ai/WORLD_SCHEDULE.md#年度授勋最小闭环)，完整授予/raw88仍不作为本轮完成项。

七月全局入口沿既有`world_world_entry`来源与严格创建规则，原型先前漏接`create_encounter`回调。
补接保留真实created/denial类型与即时元数据，不以名单增长猜成功，不改抽选/原表或把拒绝当错误。
任务/遭遇目录也保留2700+人物住宅程序，否则验证其他合法存活续体时会误拒绝；同步说话人替换仍沿既有规则。
上述静态来源不等于固定APK动态或完整年度授勋UI认证，本轮实际执行只登记于[验证](VERIFICATION.md)。

本批继续使用同一APK和既有生成源码，没有重新解释截图版本或改动原表。
完整原表发布到[共同世界目录](data/world/README.md)，固定事件发布到[脚本目录](data/scripts/README.md)，
[原型唯一所有者](prototype/include/dungeon_village_prototype/startup_world_runtime.hpp)在运行时只读取编译C++和程序旁素材。
本小节记录新增原事实与维护边界；当前检查结果只以[本轮验证](VERIFICATION.md)为准。

- 原`c/i.a(Tenant,definition,fragment)`按设施种类分支：1/8/9分别进入8/9/10且g=0，
  3/12/13进入1且g=0；初期task0/1/2引用的设施种类1不能被误判为不可绑定。
  工厂/地图恢复消费真实定义，不改questData来避开该分支。
- Java空槽UID可以复用，但已退休任务/设施对象仍可被成果页引用。
  维护稳定ID单调递增，原UID另存；不复用稳定ID冒充旧对象，也不直接以UID作为C++容器键。
- 本地`L`和全局遭遇创建都在原请求执行时登记共同怪物事实，介绍程序和事件89位于每次创建之前。
  不能把所有介绍推迟到创建队列完成后，也不能漏掉d尾部要读取的元数据。
- 事件调用计数`aM`由实际脚本所有者保存；战斗域的已见事件集合是正计数的派生缓存。
  事件90在登场旧B73同步返回后，本次后续判断必须立即读取新缓存；不能等到下轮刷新。
  人物近战创建掉落时也须按实际返回序追加`bp`，使后面的同轮物体遍能够推进新对象。
  两项接线修正没有放宽名单、身份或字段校验，见[共同调度专项](example/tests/world_actor_schedule_test.cpp)。
- `c/n.J:303–305`明确写当前人气50/历史最高0；`a(delta,show):738–744`在第一次调用才以结果提升最高值。
  因此`maximum>=current`不是合法的新局校验前提；下界、目录与溢出拒绝仍保留。
- `c/b`构造函数的`au`和渲染`o`均新建为零；`c/n.a:1663–1669`只写n/s/t/u/v，不写au。
  首访保护快照尚未运行d，不能把au初值擅自补成出生点。
  渲染`o`、世界`n`、攻击备份`au`、缓存`u/bm`是不同字段，读取与更新时点另见
  [共同世界规则](rules/ai/WORLD_SCHEDULE.md)及[实际渲染缓存](prototype/src/startup_world_runtime_nonactors.cpp)。
- `c/b.java:5664–5683`先保存旧u到v，再投影物理后的n到u；`c/a.java:123–126`明确包含截断后的高度。
  状态4/20跳过行走判向，任一投影轴相等保留方向；随后才执行ab/r与留存判断。
  因此原型不能将u当成地面投影，也不能用r清理后的n重建本次已保存u。
  [尾部载荷](example/include/dungeon_village_reference/world_actor_tail.hpp)保留该只读投影时点，
  普通与独立W消费者共用方向回调；原o/bm的框架末刷新仍另行执行。
- `a/j`原第5列是绘制深度偏移、第7列是flags；`c/h.b`以
  `(flags1 ? D.y-50 : D.y+15)+depthOffset`提交地表，再用原偏移提交栅栏/外部门柱。
  `c/i.i`保存多格建筑的分片帧；逐格绘制，不能只画锚点朝向帧。
  `DungeonFinishSurface.instance`保留的旧字段名实际对应`c/i.m`外部入口方向，和设施维护ID无关。
- 成果页30每段都先判`counter<40`，再从phase0切phase1，最后关闭；31初始化复制参与者X并按任务flags2处理H，
  确认只关页；32仅展示已发奖励。来源`b/g:10781–10814,11921–11984`。
  94已维护的r3领取增加建筑H建设资格次数、不免造价；r5／6／7分别修改武器／防具／饰品目录，r8增加普通道具库存。
  各分支不能当纯对话关闭，见[赠礼消费者](example/src/world_gift_page.cpp)；这与商会85付款／93领取是不同入口。
- 成长109在原第一次职业等级提升内部触发；维护成长域输出后才接程序。
  本固定目录109=`2,86`、113=`2,90`仅插入对话，不读等级/资金或消费随机，
  因此本目录可静态证明可观测结果一致；这不是任意扩展脚本都具有严格时点等价的证明。
- 投射接触源`c/n.java:911–922`实际创建人物cd10；法术4/5/6追加目标cd16，7/8/9追加ce，
  法术落地为全局22、地面拾物为全局12。正常死亡依次X20、X3七字段、X4六字段、旧u重映射bm、声音23，
  取消死亡只有X21。state15旧B60实际为cd18，不是全局20。
  原数据载荷及可执行适配见[表现消费者](prototype/src/startup_world_runtime_nonactors.cpp)。
- 原自动保存时点在研究原型中只保留完整不可变Owner内存检查点。
  此策略不是原APK文件序列化，也不是正常存取/迁移功能；替代字体、240×320研究视口和seed1均明确标为适配输入。

同参数新增`work/ai-fallback/WorldObject.java`（c.e），SHA-256
`9db7e7d2375b48d14ed3183ff2f62e82f6e8b05a80e3bd2829a3655140d90075`。
物体奖励先增加计数再判20/60、普通物品提示先于授予、装备相反、全局E取模与事件151条件已低层交叉。
攻击14共同尾部/弓无目标早停、17旧au及救援递归到达先清引用后复制位置，复用上述Character低层核对。
聚合更新复用`UserData.java:13364–13587/L548→L709`，纠正常规人物d删除条件和旅店S可用判断，
证实怪物d正向删除跳过与同轮追加可见性，见[调度规格](rules/ai/LIFECYCLE.md#聚合遍历与同轮新建)。
显示/延迟队列及表情复用`Character.java:14683–14744,16515–17045`，
防具/饰品复用`Weapon.java:1171–1287`，修正常规防具rank条件反写并保留不同空当前回退。
对应[控制时间线](rules/ai/CONTROL.md#显示与延迟效果的实际推进)及
[装备选择](rules/FACILITY_USE.md#防具饰品选择与收费前置)与独立C++同步维护。

人物定义新增`work/ai-fallback/HumanDefinition.java`（a.e），参数同上，SHA-256为
`41dfeff5e26bb642743ae090aec4523ccfb75ec40fbc99c8e5760c3d56e6a75e`。
属性重算1098–1393、职业成长1677–1964，与常规`a/e.java:146–152,304–396,498–613`
及`c/b.java:5856–5868`成长显示交叉；逐职业截断、满级扣费、最后一级提示与职业解锁顺序见
[定义成长](rules/ai/CONTROL.md#人物定义重算与职业成长)。生成Java仍只留忽略work。

同日继续复用上述生成证据，没有重新反编译或修改输入。
完整服务安排/退出前部见[设施事务](rules/FACILITY_USE.md#设施服务组合与首段私有所有者)，
状态18及d尾部复用`Character.java:16350–16443,17030–17365`，见[共用更新](rules/ai/LIFECYCLE.md#共用更新前缀与尾部)。
重复参战匹配、定义p只清J/K及全局N只追加ID，分别核对命中低层`Ld0→Lf2/L19a→L1bc`、
`a/e.java:817–820`、`c/n.java:2483–2485`，见[战斗事务](rules/ai/COMBAT.md#跨所有者战斗提交)。
授予目录消费者`a/g.java:173–188,a/p.java:182–192,a/b.java:60–67,a/a.java:60–67`及商店通知
`a/o.java:711–735,c/m.java:832–840`为小方法直接事实；直接授予/拾取入口差异见[拾物提交](rules/ai/LIFECYCLE.md#拾物与目录实际提交)。
怪物创建`c/n.java:518–535,1663–1669`、原UID首个空槽`c/b.java:2202–2227`、
尸体旧B12与先事件d/后增长e、组返回值忽略/引用保留、完成量f215e以及定义成长共享容量见
[奖励所有者](rules/ai/ENCOUNTERS.md#死亡战斗组与奖励所有者组合)。月度A槽扣减见`b/c.java:259–268`，
不擅自挪到每次访问或AI tick。有限组合/稳定ID/异常拒绝仍与原版完整世界、Java回绕及动态轨迹分级。

本批继续复用同一生成输入，普通创建守卫/数量/实例安装见`c/f.java:58–153,c/k.java:106–147,c/b.java:2248–2289`，
与[创建事务](rules/ai/ENCOUNTERS.md#普通遭遇创建事务)及C++同步。原事件ID0/取模、前期强制一只仍消费抽取逐项记录。
普通救援返程的活动4用Character低层`L32c→L363`核对成本严格最小、首平局及完整dk，而不是随机dl；
递归到达/共享B2与普通旅店使用见[救援所有者](rules/ai/PERCEPTION.md#普通旅店世界事务)。
R/S/db/dc、组及投射引用寿命与当前名单身份分开，见[对象可达性](rules/ai/ENCOUNTERS.md#对象引用可达性)。
当前攻击控制14–17继续对照既有低层；武器p.h与当前动作k时间端点独立，原`bc`表121–124交叉。
回复小函数682–690为直接事实：显示追加、HP改变及倒地B提前；大控制消费者仍保留局部控制流推断等级，
见[实际控制提交](rules/ai/COMBAT.md#控制14至17的世界提交)。本批没有新反编译、原表修改或固定APK动态认证。

本轮继续复用上述固定生成输入：共同c前段`c/b.java:4807–4880`，d前段/物理投影
`5446–5675`，K`476–500`，e`5754–5816`、J`461–474`，转换`c/a.java:148–167`。
普通e/F/事件计数比较原`f164b`而非维护稳定对象身份，退休引用按原对象关系保留；
怪物初始中心缓存/随后偏移分开见`c/n.java:1663–1669,518–535`。
小函数n只写k/l（`c/b.java:2300–2303`）；低层`Character.java:9371–9377,14422–14465`
交叉b不清i，opcode3消费者却另清i。由此修正维护基线与救援修复的实现/预期。
九方向移动`c/b.java:1881–1944,2030–2107`，半格中心`c/h.java:308–315`；
影响场快照`c/f.java:327–333,459–466`、初始零数组`42–45`。
任务配额`a/m.java:105–107`、地图bit2刷新`b/c.java:2227–2249`为小函数直接事实；
F新建后调用顺序/事件完整组合仍保留巨型方法局部推断等级。详见
[共同感知](rules/ai/PERCEPTION.md#共同世界感知与缓存)、[实际执行前段](rules/ai/LIFECYCLE.md#实际执行前段与解释器续行)、
[策略世界提交](rules/ai/COMBAT.md#状态1策略的世界提交)和[任务与场事务](rules/ai/ENCOUNTERS.md#任务创建与影响场地图事务)。
无新增截图输入或反编译实现复制；六套C++验收与APK动态/窗口验收仍分别记录。

## 稳定符号映射

探索续补新增`work/ai-fallback/Tenant.java`（c.m），JADX1.5.6同参数fallback单类输出，
SHA-256为`b4982c78321f655bae6fd42e85a48c4349f078cbdc2aee40270d828b9f138d53`。
常规`c/m.java:169–221,513–526,612–831`与低层`L1ff→L44a,L45f→L609`局部交叉，
源挑战生成`c/n.java:680–690`证实战斗记录第五字段是怪物ID，不是0..3奖励类别。
人物撤退/状态20、实际q精确O格、直接目录与抛出入口另与小方法交叉，详见[探索规格](rules/ai/DUNGEONS.md)。
阶段2从全局任务设施取摘要，不从当前设施猜奖励；巨型方法警告和动态未认证边界保留。

通用商店复用同一固定输出，未新反编译：`c/b.java:986–1062,3835–4003`、
`a/g.java:173–188`、`a/e.java:191–195,270–285,485–487,721–724`、
`c/n.java:2469–2475`与既有Character/Weapon低层交叉。
携物的g.c与c.e直接授予不是同一入口，不增加奖励E；满足度即时提交、人气I前插延迟、
19只写z、28只改发起者ae而重算共享h等区别见[通用商店](rules/ai/CONTROL.md#通用商店的世界事务)。
显示请求/实际cd适配与完整共同世界仍分别登记，不能从局部事务通过外推原版动态等价。

补充复核：Character低层`L9d2→L9f7`明确状态15旧B70调用h.b(s)，并不读取缓存ax；
`c/k.java:62–104`记录任务成功/定义t/全局v,w,G及条件提示，不是解锁任务定义。
`c/n.java:4204–4206`仅将任务m.m加f215e。对应维护接口/文档/有效断言同步修正，无新来源输入。

本轮设施/共同尾部继续只读既有输出：使用`c/b.java:1117–1203`、控制`3370–3380,3801–3832`、
抛物线`5306–5333`、尾部`5625–5750`、清理`5886–5921`，与既有Character低层小函数交叉。
投影`c/a.java:122–137,c/h.java:56–64,190–197,334–339`、抛物线数值
`c/d.java:64–69,151–157,c/n.java:1972–1973`为小函数直接事实。
人物r的c19保留k/l，特殊opcode2清掉控制尾部，类别8无占用/回血；
原表交叉纠正类别5为洞穴/迷宫探索，住宅类别9、募集类别10，不能把探索遗漏登记成住宅。
新增C++组合定位见[设施控制](rules/ai/CONTROL.md#设施控制的共同执行组合)与
[执行尾部](rules/ai/LIFECYCLE.md#执行尾部的世界提交)。

限速首次补齐跨类链：`net/kairosoft/android/bouken_ja/Main.java:97–104`、
`kairo/android/a/b.java:177–215,312–345`和`b/b.java:12–18,29–31,55–103`。
正常默认v21/47ms等待、等待后取当前时间/无补算为短函数直接事实；
`b/c.java:1500–1520`的两次场景迭代继续保留原巨型方法局部推断等级，复用既有MainScene低层。
`IApplication.java:65–149`与框架独立绘制线程`kairo/android/a/b.java:460–478`的50ms不作AI频率。
详见[墙钟规格](rules/ai/LIFECYCLE.md#原版墙钟限速)；本轮未改变APK、源表、素材或取得动态认证。

共同调度本批复用 `UserData.java:12919–13587/L1ed→L709`，修正人气队列在递减至非正时消费、
设施状态恰1才可救援，补齐到访对轮首影响场的时点差异。末尾L复用常规 `c/n.java:375–426,868–912`
及低层 `1691–1815,3275–3528`；矩形共享 `br[0]` 别名、预检前抽方向号和纵向边界上限均明确保留。
维护共同所有者、显式外部/设施引用根与实际非人物消费者组合见[共同世界](rules/ai/WORLD_SCHEDULE.md)。
这不是完整Java堆GC、主场景日期前置或APK动态轨迹认证。

真实出发复用 `work/activity-choice-fallback/Character.java:9379–10123/L18→L53b` 与
普通 `c/b.java:2337–3327`，搜索上限、两级抽号、任务/救援/物体/事件懒读取见
[出发事务](rules/ai/WORLD_DEPARTURE.md)。控制8成功A直写、旧位失败守卫及r同次续行由低层交叉；
通用状态2及25/26/32/33复用 `Character.java:3130–3194,12276–12359,14525–14682,17566–17610`。
漫游复用 `10313–10730`，装备27/29复用 `12123–12275`；
原cd负年龄烟效不改成ce延迟，显示不装配，参见[控制组合](rules/ai/CONTROL_COMPOSITION.md)。
本批没有新反编译输出，不复制Java实现、不改原表或素材。

探索阶段2新增交叉 `c/k.java:62–104`、`c/n.java:4164–4206`、`a/f.java:69–74` 和
`a/o.java:224–235,792–803`：任务统计、探索日期/阶段、怪物开放、任务池阈值已维护实际候选；
恢复场址的逐占用格/刷新/镜头/邻接顺序及摘要页30/32只展示见
[完成消费者](rules/ai/DUNGEONS.md#completion-consumers)。
事件指令22才将待结算值排入人气延迟并清零，未把摘要窗口当实际结算，也未补未知脚本时点。
怪物地面出口0→26与无类型守卫的共享bv访问另由低层 `Character.java:2850–2910,12294–12310`
核对，纠正维护额外human-only拒绝；原有效安全校验仍保留，见[离村控制](rules/ai/CONTROL_COMPOSITION.md#26共享定义变化在前调度删除在后)。
共享运动小函数 `c/b.java:1065–1076,1843–1851` 明确先写r再加到n，64与零距离不改旧r。
维护运动返回实际速度写值，P/控制0/救援读取该值而非加法后位置差；源路线G/H不塞入m0。
救援绑定清G/H时同步清两种维护路线，返程新G替换旧地面G，旧O按原时点保留。

新增三张建筑运行补图 S023–S025 单独归档并登记哈希/尺寸，来源版本未核实，见
[第四批截图](references/screenshots/2026-10-04-user-04/README.md)。
固定APK对应消费者为 `c/m.java:363–480`、`c/b.java:1025–1058`、`a/o.java:399–490`
与 `d/a.java:3437–3507`，具体资源/计时/HP字段和邻接提示链见[UI映射](ui/PAGES.md#inn-rest-and-neighbourhood)。
静帧、用户描述与固定APK静态规则分别记录，不把截图22/300G作为新局原表数值或连续动作验收。

下列名称是根据残留诊断字符串、序列化职责和调用关系建立的工作别名，不是恢复出的 Java 原始类型名。

| 混淆类型 | 工作别名 | 直接锚点 |
| --- | --- | --- |
| `c.b` | `Character` | `c/b.java:132` 的状态标签表；3271-3287 的 `Character.java setGoTenant()` 诊断；4205/4310 的序列化 |
| `c.m` | `Tenant` / 设施实例 | `c/m.java:96` 的 `Tenant.java deserialize2()` 诊断；111 的实例初始化；223/279 的序列化 |
| `c.n` | `UserData` | `c/n.java:3950/3971` 的 `UserData.java update()` 诊断；2492/2573 的序列化 |
| `b.c` | 主场景与建造控制器 | `b/c.java:136,601` 的建造模式提示；1359 的校验；1702-1890 的提交分支 |
| `b.g` | 菜单/事件 UI 状态机 | `b/g.java:109` 的主菜单标签；1880 的月报入口；7010 的设施维护费显示 |
| `d.a` | 资源/存档/运行时协调器 | `d/a.java:3194-3268` 将角色和运行时向量写入存档分区 |
| `c.l` | `RouteSearch` 工作别名 | `c/b.java:3265-3267` 调用；3271 的失败诊断直接命名 |
| `c.i` | 地图格 | `c/i.java:6` 路径类别矩阵；56-133 的清空、设施和道路绑定 |

## 证据规则

1. **直接事实**：在固定 JADX 输出中可见的字段读写、调用、分支条件、序列化操作或字面量。
2. **控制流推断**：将多项直接事实组合成职责或时序结论。混淆名称和带反编译警告的方法必须使用该等级。
3. **Ark 建议**：[example/](example) 中独立作出的设计选择，不声称属于原作行为。

大型方法 `Character.o(int)`、`Character` 控制方法、`UserData.e()` 和 `b.c.b()` 带有 JADX 的重复代码块、
移除指令、嵌套 `try` 或类型推断警告。诊断字符串和较小被调方法可以佐证其总体职责，但具体分支优先级
不是直接事实。存档读写方法和较小的 `Tenant` 方法反编译较干净，可信度更高。

## 生成证据位置

生成证据有意保持在忽略目录中：

```text
work/decompiled/sources/c/b.java       Character
work/decompiled/sources/c/m.java       Tenant
work/decompiled/sources/c/n.java       UserData
work/decompiled/sources/c/l.java       加权寻路与前驱回溯
work/decompiled/sources/c/i.java       地图格类别与准入矩阵
work/decompiled/sources/c/d.java       邻接方向与随机 helper
work/decompiled/sources/b/c.java       主场景/建造控制器
work/decompiled/sources/b/g.java       UI 状态机
work/decompiled/sources/d/a.java       存档/运行时协调器
work/decompiled/callgraph.json         JADX 调用图
```

本研究中的行号对应由固定 APK 和 JADX 1.5.6 生成的输出。更换参数或 JADX 版本重新生成后，行号可能变化。

自主行动新增定位与证据限制集中在 [人物自主行动报告](rules/CHARACTERS.md)，不提交上述生成 Java。

设施表加载、字段消费者和两朝向几何定位集中在 [设施报告](rules/FACILITIES.md#definitions)。
维护数据来源见 [清单](data/SOURCE.tsv)，原始字段与定义 ID 不作为产品自行推测规则的授权。

周期费用、五类统计、双资源与报表局部时序定位见 [周期账本报告](rules/ACCOUNTING.md)。
其中金币/点数消费者为小函数交叉证据；月报完整顺序和讨伐退出仍按带警告控制流推断处理。

地图状态/路径类别、占用格活动目标、全图连通标记和到达身份定位见 [地图访问报告](rules/MAP_ACCESS.md)。
辅助周边表用于邻接修正，不作为入口证据；主场景完整建设条件仍带警告限制。

`a/o.java:399-490` 的重算/去重/道路魅力消费者及第 26/27 列解释见 [邻接属性报告](rules/FACILITIES.md#neighbourhood)。
维护示例用稳定实例 ID 记录来源，不复刻定义加同类序号的显示引用或变化提示动画。

设施事件语法、小型计数门槛、延迟记录和传说商店展示定位见 [设施事件报告](rules/FACILITY_EFFECTS.md#events)。
opcode 6/40 与 UI 的整体关系按带警告局部推断，小函数/固定原表分别核对；不复制完整解释器。

道具表、按类别适配、共享改良与库存消费者见 [道具改良报告](rules/FACILITY_EFFECTS.md#items)。
小型属性/库存函数与 UI 局部顺序分级记录，候选事务是 Ark 安全设计，不是照搬原作实现。

小型类别计划、访问计数与带偏差随机 helper 见 [人物活动类别报告](rules/ACTIVITY.md#categories)。
多格候选收集和特例过滤仍保留巨型方法限制；不以类别纯函数验收替代完整 AI 证明。

补充证据使用同版 JADX 的 `--config none --decompilation-mode fallback --no-res` 单类输出：
`work/activity-choice-fallback/Character.java` 与 `work/user-data-fallback/UserData.java`。
生成文件仍只在忽略目录，行号与常规输出不同，不替换上述原始定位。
局部寄存器/跳转交叉核对见 [多格选择报告](rules/ACTIVITY.md#ranked)和
[人气队列核对](rules/FACILITY_EFFECTS.md#events-人气队列低层核对)。这些局部结论不消除整个巨型方法的警告。

`work/candidate-filter-fallback/RouteSearch.java` 是 R2-J 对 `c.l` 的独立低层输出。
[活动候选报告](rules/ACTIVITY.md#candidates)记录扫描资格、事件追加、交换排序、类别 4 两套视图及寻路平局
局部证据；原型/原作路径完全一致仍未认证，不能从候选层通过外推。

R2-K 使用已有 `Character` fallback 的 4036 起较小到达记账 helper，与常规 986–1062 和三个
独立金币/角色统计/实例销售消费者交叉核对；具体分支、状态投影和未实现路径见
[普通到达报告](rules/FACILITY_USE.md#arrival)。原有状态机警告仍保留，不将候选验证当全状态认证。

R2-L 连接已有 `Character` fallback 9437 收集与 3585/3609/3641 的普通选择消费者，
保留完整候选/阶段 1 活动实例两套视图，见 [快照选择](rules/ACTIVITY.md#snapshot)。
局部计权包括重复/不可达事件，不借此声称完整事件使用路径已恢复。

R3-A 继续复用已有 Character fallback `Lc7c/Ld72/Lda0`，与较小的 `a/o.java:833-839` 使用累计、
`a/e.java:270-276,485-487` 满足度、`c/d.java:118-131` 整数插值交叉核对。
UI 81 初始化/关闭及升级 helper 只记录局部顺序，不把它当自动升级或已实现存取，见
[退出报告](rules/FACILITY_USE.md#exit)。本阶段没有运行新反编译任务。

R3-B 复用 Character fallback `17566 g(int)`、`17800 s()` 和 `16495 d(int)`，核对六槽生命值
协议；巨型 `c()` 的 `L969→L995` 只作旅店恢复调用点局部证据。常规小函数、静态计数表与实际
绘制读取分别交叉记录，见 [生命值报告](rules/FACILITY_USE.md#hp)。不从显示协议外推全游戏时钟。

共同世界生命周期续批复用上述固定输出，核对常规c/b的2/4/10/12/14/16/17分支与
fallback `L4c7→L50a`、`L5a3→L606`、`L8b9→L8e2`、`L969→L995`、`La08→La96`。
小型b()/c(int)/M()/O()分别核对基线恢复、实际setter、携物哨兵和路径清理差异；
具体行号/消费者/1,580项组合边界见[共同生命周期](rules/ai/LIFECYCLE.md#共同世界生命周期分支)。
本轮没有生成新Java或修改APK/源表；计时/世界夹具不认证原版动态轨迹。

2026-10-04世界闭环续批使用同一固定输出，新增以下交叉结论：

- 主场景低层`MainScene.java:L10b→L159`、`L247→L159`：首个自动事件/到期续体后立即结束当前逻辑轮，
  不因scene仍为0继续AI。纠正旧批量扫描；第二倍速轮仍按入口保存轮数执行，见[共同调度](rules/ai/WORLD_SCHEDULE.md)。
- 地图读取`c/h.java166–171,317–327`：Y翻转后区域0为`[6,10,17,2]`，人物L与最终重叠边界均读较小Y2。
  不将数组行号解释为排序后的top/bottom，见[最终L](rules/ai/WORLD_SCHEDULE.md)。
- 无继承新局`c/n.java3346–3348→a/e.h/r440–480`：首名共享D全零/m0，首访创建不改变；
  认证范围仅首局至首访，非任意动态默认，见[新局](rules/STARTUP.md)。
- 真实任务工厂`c/n.f/a`、`c/k.c/a`和低层UserData的L369/L387：普通kind1同stage，
  稀有探索立即返回，最终挑战覆盖97之前仍抽10，见[探索](rules/ai/DUNGEONS.md)。
- `c/n.a(e,5,15,false)`只奖励住宅现有住户，不创建人物；原人物字段e.i是脚本而非激活方法，
  页94礼物仍须真正输入领取，见[共同调度](rules/ai/WORLD_SCHEDULE.md)。

维护组合模板/随机种子/磁带/空村或长等待夹具只验证独立C++契约，
不是固定APK动态轨迹、真实用户输入或完整新局窗口认证。当前检查单独记录[验证](VERIFICATION.md)。
