# Steam启动与选档素材交付

2026-10-09。固定Steam研究副本的最小图块集合，供产品依据[MANIFEST.json](MANIFEST.json)复制实际文件到自身assets。运行不读取APK、Unity容器或research/work；本包不修改固定APK的761文件发布包。

清单38逻辑条目：25PNG、7SEB、6INF，总359,293字节。21项已与Steam条目逐字节核同，显式别名引用兄弟目录`../original`；17项差异文件置于本目录`original`，共304,770字节。`file`始终相对MANIFEST所在目录，`storage=alias`也必须核SHA后消费。按原语言路径保留独立条目，部分语言图相同hash是已核事实；每个副本均有对应逻辑条目和用途，未增加未登记文件。

标题背景、Logo、草边与upper按[Steam标题绘制](../../ui/STEAM_TITLE_DRAW.md)；菜单和手动／中断两裁片按[Steam标题菜单](../../ui/STEAM_TITLE_MENU.md)。event与公共木框另供[纪录／91／17皮肤合同](../../ui/STARTUP_SKIN.md)对应消费者使用。清单`usages`登记这些正式合同中的用途，**同字节图块不把APK页面绘制提升为Steam页面代码已核**；具体坐标、遮挡、帧和调用资格按各合同的来源层级判断。

标题共有默认／英语Logo；存读档保留根目录与de/es/fr/hi/it/pt/ru/tr/zh-CN共10种原路径。清单不指定尚未核定的中文运行时语言fallback。common SEB22=`finger_l.seb`仍引用70号`finger_r.png`，不需要不存在的finger_l.png。七个已发布SEB的正图片引用全部落在38项内。

六INF是**完整原始目录证据**，不是本包文件列表；不要删行、重编号或按它们默认加载整组。本包只发布MANIFEST明确列出的条目，例如title广告图和common其余动画没有随本包发布。空title/seb.inf保留合法身份。字体、标题人物完整98项、音频与通关完整皮肤分别交付，本包不补猜测字体或默认动画。

复算与只读检查：

```powershell
python research/dungeon_village_1/tools/scripts/publish_steam_startup.py
python research/dungeon_village_1/tools/scripts/publish_steam_startup.py --check
```

工具每次核固定resources.assets SHA、三个TextAsset对象／payload、归档边界与白名单条目，再核PNG CRC／RGBA和SEB引用。该Unity内嵌归档沿已核格式读入，没有虚构额外游戏CRC字段。全部目标先预检；已有异内容、未登记文件、符号链接／联接一律拒绝。缺失文件独占创建，相同内容重复运行不新增，失败只清理本工具本次创建且仍同身份的文件。工具不读取work，不运行原DLL，不改原存档。

原始素材仍归原权利人；本交付用于用户已授权的内部研究和项目接入，不增加公开再分发授权。产品接入、实际窗口及最终缩放另行验收。
