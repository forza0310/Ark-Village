# Steam Windows 版研究可行性评估

2026-10-06，用户提供其在Steam购买的《冒险迷宫村》安装目录，授权并行评估研究难度及替换APK基础的可行性。
本次只读静态检查，没有运行游戏、修改原文件或更换固定研究来源。APK 1.0.8仍是维护规则与现有黄金轨迹的输入。

## 判断

**可以研究，适合先作为独立对照来源；尚不适合直接替换APK基线。**
这份Windows版不是完全丢失语义的原生黑盒：类型／方法名相当完整，功能定位可能比混淆APK更容易；
但具体方法已由IL2CPP编译为x86机器码，还原计算和控制流比直接读取Dalvik／JADX方法体更费力。
总体是可行性较好的研究对象；2026-10-07已验证本机方法地址映射并检查代表方法真实字节，不能承诺恢复原源码。
进一步资产差分发现地图、代表表的非本地化字段和多组素材字节一致，支持大量数据工作复用，
因此切换并不意味着推倒重来；主要未知仍是Steam版消费者、时序与平台适配。

## 输入身份与已证事实

目录为`DungeonVillageEXE/`，25个文件合计77,464,328字节。购买来源由用户陈述；
`KairoGames_Data/app.info`标识Kairosoft／冒険ダンジョン村。目录没有Steam appmanifest、buildID或depot manifest，
因此本次以文件哈希固定样本，尚不能登记Steam游戏更新号。

| 文件 | 字节数 | SHA-256 |
| --- | ---: | --- |
| KairoGames.exe | 822192 | `923a4307a44386e088f4fedd2d42e56fd26b003f660318840da62b9ea44aaf8d` |
| GameAssembly.dll | 19217920 | `9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a` |
| global-metadata.dat | 5467040 | `80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369` |
| resources.assets | 13595612 | `75a7ac65688841505603393e3b013f26cd581d74f829aec9de3a55c73c21271b` |

- EXE是PE32/i386，导入`UnityPlayer.dll!UnityMain`。引擎版本为Unity 2021.3.11f1，**不是游戏版本号**。
- GameAssembly为原生PE32，含`il2cpp`节和`il2cpp_init/il2cpp_class_from_name/il2cpp_runtime_invoke`导出。
  CLR目录为空，不能用ILSpy直接读取全部IL方法体；后续工具生成的dummy DLL也不能当作原始实现。
- 元数据位于`KairoGames_Data/il2cpp_data/Metadata/global-metadata.dat`，魔数`0xFAB11BAF`、版本29。
  解析得到5669条类型记录，名字索引和类型token校验通过；该数字包含引擎及公共库，不与APK的188类直接比较规模。
- 保留`game.Map/game.Tenant/game.Quest/game.RouteSearch/game.UserData/data.TenantData`等语义名。
  `Map`的31个方法保留`CreateMap/LoadMap/Serialize/Deserialize/IsInTown/IsInTownFence/SpreadTown/SetTownExtend`等名称。
  初次评估只登记`SpreadTown`元数据记录文件偏移2548488；后续已建立方法映射，但本批未认证其扩张机器码语义。
- 保留`java.util/java.lang/java.io`和`kairo.unity.*`命名。它们提示Java兼容／移植痕迹，
  **不能证明自动转译、规则未变或与汉化APK 1.0.8逐行等价**。
- 有PDB路径但没有随包PDB；路径不能代替调试符号。EXE入口在`.bind`节，仅能登记启动包装迹象，
  本批未识别其具体格式。业务分析首先面向独立GameAssembly和元数据，无须先研究启动包装。

## 已完成的资产与代表表差分

按Unity SerializedFile v22对象边界解析，`resources.assets`的1253个对象中有25个TextAsset。
在本地探针中，`xls/map/image/human/monster/weapon/common/common2/effect/event/load/system/title`的13份载荷
可用既有APK循环XOR key和大端归档布局解析；头部长度、条目范围均通过检查，`kairolib`为明文归档。
没有修改正式提取工具的APK身份／CRC校验，也没有把EXE条目伪装成APK输入。

| 比较项 | 静态差分结果 |
| --- | --- |
| map/game.gmap | 2356字节，与APK已登记地图字节哈希相同 |
| 英／日tenantData | 各85行×36列，ID顺序相同；仅零基列1名称、30说明不同，其余34列逐单元相同 |
| 英／日item | 各36行×25列，ID顺序相同；仅零基列1名称、23说明不同，其余23列逐单元相同 |
| human／monster／weapon／load | 分别67/67、88/88、51/51、11/11个素材条目与维护APK原素材字节相同 |
| image | 203/216个条目找到相同字节；不能宣布整组或其余素材组全同 |

地图SHA-256为`51032b93d9d5c3539ad0d2edd0e155e7a3e82a77ca1f0b53a51cf34487f7b5cb`，
与[data/startup/MAP.json](../data/startup/MAP.json)登记相同。代表表按有序ID和逐行逐列比较，
没有用“提取所有数字后相同”替代记录对齐；英／日语言条目与汉化APK的文本差异保留。
完整xls有45个条目，本次只将上述两表作为代表结论，其余辅助比较不升级为整表体系认证。

这些结果证明字节和字段内容一致，**不证明Steam消费者赋予各列相同语义**。
同一地图不能代替加载后新局状态验证，同一PNG／SEB不能证明帧选择、朝向、缩放、UI布局或输入顺序相同；
用户先前的异版本截图也不能因此自动归为当前Steam样本。

## 与现有APK研究的差别

| 工作 | 固定APK 1.0.8 | 这份Windows样本 |
| --- | --- | --- |
| 功能定位 | 类／方法混淆，但已有大量人工映射 | 语义名保留较好，有利于按Map、Tenant、Quest定位 |
| 计算与控制流 | Dalvik指令、JADX及fallback可交叉核对 | 需IL2CPP元数据映射与x86反编译，优化、内联和虚调用增加工作量 |
| 数据／素材 | 已有原条目、表格、PNG／SEB／INF及哈希链 | Unity资源容器及开罗资源系统，需独立解析和逐项比较 |
| 运行对照 | 当前规则来源主要为静态研究，固定APK动态仍缺 | 正版Windows运行条件更接近当前机器，便于后续画面／输入观察；本批未运行 |
| 证据成熟度 | 已有页面、随机、时序、Owner和长期黄金轨迹 | 已有身份／资产差分、地址映射及存取／随机局部机器码证据，尚无Steam自然轨迹认证 |

已核对JRandom按Logic选择三种随机实现的机器码，但不能据默认值或Java兼容层推断实际运行选型。
主循环时钟、1／2轮更新、输入及暂停、页面计数、存档和平台适配都需要查真实调用路径。
现有APK的47ms框架观察、Java48随机流、页码及原表条目不自动升级为Steam事实。

## 后续切换需要的最小门槛

1. 固定完整Steam样本及游戏版本／buildID，建立独立来源登记；APK继续保留为历史基线。
2. IL2CPP映射及存取／随机代表方法的解码已完成；继续核对`Map.SpreadTown`、一个道具／收费消费者，以及主循环。
   映射成功只解决定位，尚不能省去优化、虚调用与分支范围的逐项核对。
3. 比较初始地图、设施／人物／职业／物品表、费用、整数取整、脚本与素材；同名、同ID或相似画面都不算同一规则的证明。
4. 将相同项、版本差异、平台差异和未知项分开。为Steam新建验收结果，不能改APK原表或改旧黄金值凑通过。
5. 用户确认以哪个版本为复刻目标后，才迁移正式规格和产品消费来源；迁移按功能批次完成，避免混用两版事实。

可复用现有C++17模块划分、唯一Owner、事务回滚、稳定身份及测试组织；具体数据、算法时序、页合同、素材绑定须按证据决定复用范围。
如果目标仍是固定APK 1.0.8，Steam版适合作为语义命名和表现观察的辅助；如果目标改为Steam版体验，切换技术上可行，
但应先完成上述小规模差分，不能把当前已验收研究直接标成Steam复刻完成。

本地技术记录与只读脚本位于`work/exe-assessment/`；原Steam程序与资源已按原始输入策略忽略，不进入Git或维护交付。
`static-identity.json`保存关键文件哈希及元数据记录；`text-assets.json/asset-comparison.json/table-comparison.json`
保存资源对象、条目哈希和逐单元差分；探针脚本与两份技术报告同目录保留。
初次评估未执行本机方法解码；2026-10-07补充范围如下。两批均未执行原游戏真实窗口或Steam自然轨迹验收。

## 存取与随机机器码补充（2026-10-07）

Il2CppDumper6.7.46静态搜索成功，PE节表独立核对37942条非零方法地址；iced-x86 1.21.0解码51个代表方法。
地址索引与dummy签名不是方法实现，本批结论来自DLL真实字节，未加载执行原DLL。
[工具与证据身份](../EVIDENCE.md#存取与快照交叉证据2026-10-07)登记复查参数和哈希；
[存档交叉表](../rules/PERSISTENCE.md#steam交叉结果独立样本)记录两版逐项结果。

- SaveGame实际调用25分区写器，并按当前槽位加1／3选择手动／自动记录。
- DelayEvent确实写入等待与脚本续体；Tenant效果队列在读取后丢弃，与APK低层局部行为相同。
- RecordStore前置8字节校验值；CRC64只是类名，实际是32位游戏校验。补充18个局部方法后确认初始化创建media4根Storage，
  SetPreferenceMode(true)的setter实际写false，因此该初始化且无后续改写时走Storage.Write。Steam条件路径和记录逻辑名已定位，
  但配置值链、最终目录／后缀和失败原子性仍未锁定，详见存档交叉表。
- JRandom调用有System.Random／Random2018／Xorshift三分支，实际Logic未确认；ActionReplay条件有固定seed0线索，未启用。
  已查保存路径未发现随机持久化，但全部嵌套分支未排除，不能套用APK负面结论。

这使Steam从“只有可读名称”推进到可做方法级交叉研究，仍不足以替换APK全部规则与黄金轨迹。
正常存档文件、精确测试快照及原档兼容仍为三个不同目标；当前设计见[快照与回放](../stages/PERSISTENCE_REPLAY.md)。

## 原档与实际配置补充（2026-10-07）

后续研究已闭合正常根应用的MyConfig反射覆盖链：USE_STEAM=true、ROOT_FOLDER为空、平台保持x86，
文件落在安装目录`saves/<SteamID>/000N`，数字名不加扩展，与用户实际目录一致。
RecordStore构造器以SteamID的小端8字节覆盖key，不能用APK全局44字节key解Steam记录；
Property的binary项允许-1表示null，系统整数槽也比APK多一项。

用户授权只读四个已有记录，实际安装DLL／metadata哈希与本评估样本一致。
系统、手动和中断三份非空记录的校验、完整tag容器端点及独立字节往返通过，一份空记录明确区分；
C++工具与独立Node的610个字段交叉一致。此处只认证实际观察的记录与结构，不认证原程序动态加载或两版存档互通。
全部原档和账号标识不提交；来源、分区及实际边界见[存档规格](../rules/PERSISTENCE.md)和[本批验证](../VERIFICATION.md)。
