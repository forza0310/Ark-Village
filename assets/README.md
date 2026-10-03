# 运行资源

新增新局静态数据、源地图PNG/SEB、初期建筑、农家人物、秘书与窗底。
55份文件的来源/哈希/字节数/PNG尺寸见[SOURCES.json](SOURCES.json)，由scripts/import_research.mjs原样复制。
原始数据与INF通过.gitattributes禁用换行规范化，避免新检出改变哈希；SOURCES.json使用产品标准LF。
data在构建期编译为只读C++，程序旁素材独立运行，不读取APK、research或Node。
首屏是源格/设施种子投影，不是加载后完整快照；不复用7×7夹具或夜骑士人物。

| 产品文件 | research源 | 尺寸 | SHA-256 | 用途 |
| --- | --- | --- | --- | --- |
| title-background.png | dungeon_village_1/assets/original/title/title00.png | 240×330 | bfd506538ad546e671e33519a51b23941a09d6cb98c4398b688df5afab85f3a5 | 原样复制，启动背景；point采样，等比例显示 |

源归档/版本证据见research的EVIDENCE与ASSETS。仅用于用户授权的本地学习。
运行读取程序旁assets副本，不读取research或APK；用户视频截图不是产品纹理。

中文字体未复制/分发；macOS默认系统Arial Unicode.ttf，其他环境用`--font`指定中文TTF。
