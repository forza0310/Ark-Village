# Steam危险图标偏移表：独立静态闭合

2026-10-10。本包只闭合raw60普通人物肖像引用的`Character2.OFF_EFPINCE`静态偏移表，正式合同见[STEAM_HUMAN_PINCH](../../ui/STEAM_HUMAN_PINCH.md)。先前[人物表现包](../steam-human-presentation/README.md)的历史限制不改写；本包明确补证该一项，不把完整原皮肤或全部人物动态一并记为完成。

## 来源与结论

固定DLL SHA-256为`9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a`，metadata为`80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369`。既有methods索引、dump字段映射、资源清单与生成Java各自记录原文件hash，不修改来源。

最终值为`[[-14,-28],[5,-28]]`。静态初始化是四条signed int32立即数直接写数组，不是RuntimeFieldHandle／InitializeArray blob。原consumer以direction2或3取行1，其余取行0；最终图标y=93，x为原肖像插值加住宅修正再加该行x偏移。SEB39/image22明确frame0，裁片11×13及零局部偏移均已核。

证据等级分别是：Steam直接机器码／metadata字段映射；基于该表和原调用式的锚点算式推导；APK两处生成Java局部交叉；原窗口动态未做。不能用算式示例冒充原截图。

## 有界读取与方法边界

`Character2..cctor`在RVA0x293560，最近更高具名方法入口是`Character2..ctor` RVA0x29D2E0，故完整方法身份范围为40,320字节。方法索引按类型归组，不按地址排序，不能直接拿索引中的下一项`DelayEvent..ctor`作为方法末端。探针最初按下一列项选边界时被严格“范围内不得含别的方法入口”检查拒绝，随即按实际最近地址修正；未放宽检查，也未将失败范围记作证据。

完整cctor仅做有界顺序解码／字段写点定位及方法身份hash，语义断言仅消费`0x102968DA–0x102969F0`的278字节初始化窗口。既有人物肖像方法1,312字节hash复用；其中危险consumer窗口`0x102D02D0–0x102D0401`为305字节，属于复用消费者，不计新原逻辑解析量。固定指令锚28项，覆盖二维数组长度／行顺序、四常量、静态字段写入、方向分支、两个偏移读取及最终DrawSeb调用。

APK仅取`c/b.java:141`和`c/n.java:1645–1660`两窗口共17行，分别核表值与具体调用链；没有新增整段源码副本、DEX解码或动态认证。两资源沿既有EXE索引及同字节发布路径引用：PNG240字节、SEB48字节，无图片复制。

## 复现与输出

```powershell
node --check research/dungeon_village_1/work/steam-human-pinch-offset/inspect.cjs
node --check research/dungeon_village_1/work/steam-human-pinch-offset/audit.cjs
node research/dungeon_village_1/work/steam-human-pinch-offset/inspect.cjs manifest
node research/dungeon_village_1/work/steam-human-pinch-offset/inspect.cjs locate
node research/dungeon_village_1/work/steam-human-pinch-offset/inspect.cjs offset
node research/dungeon_village_1/work/steam-human-pinch-offset/inspect.cjs consumer
node research/dungeon_village_1/work/steam-human-pinch-offset/audit.cjs
```

inspect只接受固定键、只写stdout，不输出整个cctor。audit只覆盖本新目录的[EVIDENCE.json](EVIDENCE.json)，检查固定DLL／metadata、方法真实边界、28锚、四立即数、资源引用／裁片、旧肖像证据与APK局部窗口；同时记录脚本自身hash和限制。

本次语法、源hash、字节锚、两版局部交叉和资源核验通过；两次audit幂等，EVIDENCE SHA-256为`c3b137ebcb0af2698b5c1620aaea564f5a0d3d6d054b29eac0fa080175a191b4`。UTF-8无BOM／LF、7个Markdown本地链接及差异检查通过。证据由本包README及新正式合同消费；不改旧证据、共享索引、CMake或C++。只有前台短审计，没有游戏、构建、后台进程或新增资源缓存，静态包规模不宣称原程序运行内存永久有界。

未闭合范围仍包括原窗口缩放／像素、其它世界危险特效消费者、完整人物／武器动画树与实际输入；本包不生成原窗口操作任务，也不需要原游戏存档。
