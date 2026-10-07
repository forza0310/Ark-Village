# 运行资源

新增新局静态数据、源地图PNG/SEB、初期建筑、农家人物、秘书与窗底。
670份素材/旧切片数据的来源、哈希、字节数和PNG尺寸见[SOURCES.json](SOURCES.json)。
标题入口新增`title/`背景、236×115 Logo和98×68书本按钮三份原PNG；S038–S040仅参考布局，不把截图或Steam标志/版本信息当作运行资源。
完整世界新增human/monster/image/common/weapon的维护目录副本，由scripts/import_world_assets.mjs原样复制；包含职业/性别图集、全部动作SEB、怪物体型与地图设施，不用农家图片代替所有人物。
weapon新增51份原PNG/SEB/INF，冻结0a5b5e2。用途为cd15举物的四种风格/四方向及稀疏PNG override；cd21/22和商品79/72使用common既有18×18图标及背景。建筑价格/品质/魅力提示按common原SEB指定层绘制，不把空层或同名PNG当作有效裁片。
simulation保存完整世界独立发布数据及源码来源清单，构建期生成目录/脚本；与现有data切片版本分别校验，不在运行时互相覆盖。
人物行走使用human/walk00..03四套原始SEB与同一农家图集；每方向四帧、独立24像素行、统一脚底锚点，启动时校验全部16帧，无运行时research依赖。
新增common/common2上下栏、菜单/图标/手形、日期/属性数字、人气/点数、内容角/分类/方向箭头等UI资源。
设施详情新增arrow01/arrow02及icon_param00；road00的全部16个邻接帧在启动时检查实际PNG边界。
原始数据与INF通过.gitattributes禁用换行规范化，避免新检出改变哈希；SOURCES.json使用产品标准LF。
data在构建期编译为只读C++，程序旁素材独立运行，不读取APK、research或Node。
首屏消费LOADED_MAP.tsv/LOADED_INSTANCES.tsv加载后静态快照，保留源MAP作证据，不复用7×7夹具或夜骑士人物。
STATE.json与本批维护数据同步；快照包括576格/8实例，逻辑、显示和身份分开。道路2×2/上下边缘整图road4block00/01已消费；栅栏fence01 PNG和三套fence010/011/012 SEB、外部门柱door00 PNG/SEB新增原样副本，全部18+2帧在启动校验。此轮仅UI导入，data快照未改变。

| 产品文件 | research源 | 尺寸 | SHA-256 | 用途 |
| --- | --- | --- | --- | --- |
| title-background.png | dungeon_village_1/assets/original/title/title00.png | 240×330 | bfd506538ad546e671e33519a51b23941a09d6cb98c4398b688df5afab85f3a5 | 原样复制，启动背景；point采样，等比例显示 |

源归档/版本证据见research的EVIDENCE与ASSETS。仅用于用户授权的本地学习。
运行读取程序旁assets副本，不读取research或APK；用户视频截图不是产品纹理。

Windows玩家包附带Noto中文子集与许可；构建清单和运行图集共用生成字形需求，显式`--font`可覆盖。历史macOS字体发现只属于保留的平台适配。
