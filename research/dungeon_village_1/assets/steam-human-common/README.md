# Steam人物详情差异素材

2026-10-10。独立发布固定Steam common图片88 [wnd_ato.png](original/common/wnd_ato.png)，10×7、187字节；已实际查看输出原图。[清单](MANIFEST.json)登记原容器／对象边界与哈希、原条目、INF槽、RGBA、APK差分和消费者。PNG SHA-256为`ffbf2c65ae896f069228161125df46291121c68555b184603346e745de81c056`。

已核消费者为[Steam人物详情60](../../ui/STEAM_HUMAN_DETAIL.md)四页共用经验区：`SubForm._draw2_1`的`0x1033D92B`完整绘制图片88，非日文锚`(149,113)`，日文`(148,112)`。原图极小，不放大重采样后充当原始素材。

APK同名图片也为10×7，但字节及RGBA均不同，不可作Steam别名；本包保留Steam原条目字节，不覆盖旧APK包。1个逻辑资源、1个新PNG、187字节，无新增SEB或额外图片依赖。这仅补齐人物详情批已识别的common差异图，不代表全部人物、字体、窗口或运行时语言变体完成。

人物详情证据（本地核对材料，不随仓库交付）保留限定出版白名单内图片88未匹配的历史记录；生成器在提交前修正了动态枚举导致的漂移，修前/修后身份见其README。本包是后续发布证据，不把后来出现的副本冒充当时已有。产品侧以本清单复制并核验自身运行资源，不读取Unity容器或research/work。

```powershell
python -B research/dungeon_village_1/tools/scripts/publish_steam_human_common.py
python -B research/dungeon_village_1/tools/scripts/publish_steam_human_common.py --check
```

[发布器](../../tools/scripts/publish_steam_human_common.py)复用既有普通路径校验、拒绝异内容、独占创建、失败按身份回收协议；只在内存解固定common对象。首次创建3文件（PNG、清单、.gitattributes），后续publish和只读check均created0，清单哈希一致；PNG受`-text`保护。无游戏启动、构建缓存、后台任务或旧文件改写。
