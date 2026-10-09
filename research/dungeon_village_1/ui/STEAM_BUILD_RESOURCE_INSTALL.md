# Steam地图资源安装与旋转准入

2026-10-10。接续[图块完整对应](STEAM_MAPCHIP_PATTERNS.md)，本批追到Steam实际地图资源组的安装／退休、旋转软标签的生成与输入消费，并[独立发布两张差异图](../assets/steam-build-common/README.md)。证据在[工作包](../work/steam-build-resource-install/README.md)。未改维护C++或产品，未运行原程序／原档；具名方法静态合同不等于窗口动态验收。

## 地图图块资源来自哪个组

`GameForm.Init` RVA0x2FA820的原调用次序已核：

1. `AppData.resMapChip_ +0x20`已有对象时，0x102FA99D先调用其Dispose。
2. 新建ResourceManager，七个目录参数均null，0x102FA9D2调用构造；构造复制默认目录表，只有非null实参才覆盖对应槽，不能把全null理解成“禁用所有资源”。
3. 0x102FA9E5把新manager写到AppData+0x20，随后读取`RecordStore.ReadRecord(1,2)`，0x102FAA1B调用`Load(bytes,m3dVer=3)`。

这里1是`RS_RESOURCE`，2是`RSRES_MAPCHIP`。AppData.Init在0x10258CF8取`RSFILENAMESS`，索引1的数组在0x10258D36装入RecordStore资源名字表；其索引2由AppData.cctor中的真实literal `image`填入，metadata位置0x110F4690。资源ReadRecord在rsId=1且名字表存在时按rcId取名，补`.dat`后走`Storage.Read(media=3,security=false,name)`；不会在这条分支读取玩家数字档或走存档CRC64解封。

因此固定输入链是`resMapChip ← 资源记录(1,2) ← image.dat候选 ← AssetReader`，与[图块包](STEAM_MAPCHIP_PATTERNS.md)独立解出的TextAsset1143 `image`及img.inf／seb.inf对应。这里只认证根资源身份；语言目录、覆盖候选是否先命中仍受AssetReader实际配置影响。

## Unity字节与资源加载生命周期

media3在Storage.Read调用AssetReader.GetData。它通过`RunOnUiThread(...,wait=true)`执行具名回调；回调取得`GetAccessFilesList(file)`的有序候选，遇第一个非null数据即返回。`_getTextAsset`回调先尝试带扩展名的Resources.Load，失败后去掉末尾扩展再试；取到TextAsset后，`_getData`复制`get_bytes()`，正常与异常路径都调用UnloadAsset。

GetData对无斜杠、后缀`.dat`等且加密开关开启的请求调用Encrypter.Decode；本批没有把“凡.dat都必定解码”写成无条件规则。ResourceManager.Load(bytes,3)把返回字节交JarInflater，再依序LoadReady、LoadStart；成功和异常路径均关闭JarInflater。LoadStart各图像／SEB的全部异步消费者、语言覆盖顺序及GPU对象内部退休仍单独待核。

`GameForm.Finish` RVA0x2FA040在0x102FA084 Dispose地图manager，随后0x102FA09D清掉AppData+0x20；再处理离屏对象并请求Gc。这个显式引用退休是已核事实，不能仅凭Gc调用宣称所有Unity/GPU内存同步释放。

原Init的顺序是**先退休旧对象、再发布新对象、最后Load**，本身没有维护Owner所要求的联合失败回滚保证。未来研究原型或产品应按已确认Owner契约做安全安装；不要因原函数如此就把部分装载失败当成功，也不要反过来把维护事务策略记成原版事实。

## 旋转按钮资格与脉冲

`GameForm.ChangeState` RVA0x2F6360先按状态设置基础软标签、写state并把stateCnt清0。进入BUILD=1后，清建设提示；普通非负tenantBuildId才读取设施定义flags。撤除ID=-1和移动选择ID=-2是独立分支，不能拿它们索引设施数组。

普通定义在0x102F659D检查`FLAG_INVERSION=32`，通过时将左软标签换为`AppData.SOFTLABELS[9]`，右标签沿该状态基础配置；未通过则保留基础标签。这是flags掩码32，不是“第32位”或任意原图有两帧就可旋转。BitUtil.Check的16字节实现明确按位与并判非零。

`IsPushSoftLabel(9)`默认额外key参数为0。它读取当前左右标签，按文字与SOFTLABELS[9]匹配后分别检查逻辑脉冲0x200000／0x400000；额外key0既不匹配摇杆映射，也不命中Keypad位掩码。Keypad.CheckKeyPulse读取且清除已请求的脉冲位；不是纯只读查询。之后已有GameForm._update消费链才令`tenantBuildInversion=1-旧值`。原更新顺序先检查确认，再检查取消标签2／10，再检查旋转标签9；不能把同轮脉冲无序处理。

这些值是原逻辑软键，仍不能直接命名为桌面键盘“9”或具体鼠标坐标；最终OS输入映射沿[Steam交互覆盖](STEAM_UI_COVERAGE.md#建设定位拖动旋转与提交)独立验。

固定85设施原表中57项带FLAG_INVERSION；其中46项同时带FLAG_BUILDABLE=4。这46项的两个朝向均逐片对应到已有SEB精确关键帧和图内裁片。其余带旋转位的定义包含野外目标；例如8–13带32但没有4，不能把标志枚举称为玩家已能建设或已解锁。这个交叉解释了上一批全定义朝向1出现无帧，而实际可建设＋可旋转子集未出现该缺口。当前日期可用目录、建设选择业务和所有移动模式仍需各自验收。

## 交付与剩余范围

差异图98 tenant_resident与103 number05已按原字节发布，总1213字节；SEB81／12同字节alias另登记。发布器只访问固定Unity来源和正式assets，不读取work；两PNG已查看、hash／RGBA及重复只读校验通过。

静态探针保留20个具名窗口共9344字节，并复用旧已核AppData初始化／GameForm更新窗口；41个调用、赋值或参数锚可复算。没有扩展成全DLL扫描，也没有把候选路径清单等价为本机当前语言的实际命中结果。

本批闭合原资源组根身份和软标签级旋转准入；下一必要边界是资源语言覆盖／默认目录表完整装载、建设Init/Update与移动模式选中过程、世界图块深度与完整皮肤。所有旧证据包保持冻结，本页作为后继增量消费它们。
