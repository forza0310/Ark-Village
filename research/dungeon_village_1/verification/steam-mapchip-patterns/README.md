# Steam设施pattern与两张差异图片

2026-10-09。正式交付见[STEAM_MAPCHIP_PATTERNS](../../ui/STEAM_MAPCHIP_PATTERNS.md)。只读固定Steam2.56 DLL／metadata、现有Unity容器与索引；不运行原游戏、不接触原档、不修改产品／C++／旧冻结包，不构建。

## 工具与证据

- [inspect.cjs](../../tools/steam-mapchip-patterns/inspect.cjs)：8个具名入口，核固定样本、方法边界与PE映射，共10685字节；逐入口stdout，不保存指令全文。cctor仅4381字节必要前缀，另外7个窗口核表字段和SEB范围边界。
- [arrays.cjs](../../tools/steam-mapchip-patterns/arrays.cjs)：复用已验人物数组探针的窄静态解释方案，限制到TenantData两个目标字段、固定4381字节；未知指令／调用／引用空洞失败。成功分配路径不是运行原函数。两个InitializeArray从真实metadata私有字段链核blob哈希；[ARRAYS.json](ARRAYS.json)保留2数组42个整型叶、发布与写入锚点。
- [resources.py](../../tools/steam-mapchip-patterns/resources.py)：复用[既有纯归档／PNG读取器](../../tools/scripts/image_coverage.py)，先核容器／TextAsset／payload，再在内存解common、image、xls；只写[RESOURCES.json](RESOURCES.json)。85定义、87 SEB、194资源及异常记录，不导出图片、整归档或重复原表。
- [audit.cjs](../../tools/steam-mapchip-patterns/audit.cjs)：独立核手审数组、两发布点、表列→字段锚及SEB范围／null边界，冻结源异常的精确列表；[EVIDENCE.json](EVIDENCE.json)登记19锚、规模和两图身份。

```powershell
node research/dungeon_village_1/tools/steam-mapchip-patterns/inspect.cjs manifest
node research/dungeon_village_1/tools/steam-mapchip-patterns/arrays.cjs tenant
python research/dungeon_village_1/tools/steam-mapchip-patterns/resources.py
node research/dungeon_village_1/tools/steam-mapchip-patterns/audit.cjs
```

第二条只输出摘要；归档ARRAYS.json时保留UTF-8及真实换行，其余两条只更新本专题派生摘要。原输入／源码hash在证据中，所有路径仅来自现有具名方法／原表／INF，不做全DLL盲扫。

## 关键结果与失败诊断

Steam pattern全集为三类1／2／4片、各两个朝向；完整地址及帧见正式合同。85定义反向身份一致，pattern分布75／7／3。raw21固定朝向0的请求全部能精确映射到有效原图片裁片。

初次资源核验以“所有方向必须有帧、所有SEB记录都在图内”为假设，源样本明确否定：27个朝向1请求超各层关键帧范围；定义19朝向1请求超图sea帧；plain00另有未请求超图关键帧；casino两个未请求关键帧引用缺图槽。没有改原表或忽略失败，现分为原记录结构、实际请求范围与动态准入三层登记；audit严格断言具体异常列表，变化仍失败。GetSpriteLocal静态链证实超实际层范围返回null，不能把它写成万能取模或回退frame0。

common图片98／103真实像素与APK不同，两个SEB却相同；本批独立解Steam图而不alias APK。mapchip组资源逐字节一致，只能复用已逐项核过的副本。动态资源组安装、语言覆盖、建设旋转准入、原窗口及完整SEB插值仍未完成。

全部探针同步退出，无录制、监测、构建缓存或后台任务。原资源没有复制，派生摘要由正式合同消费；历史资源／缓存与本批临时内存分开，不以当前条目数宣称游戏永久有界。Git整理与提交交主会话统一完成。
