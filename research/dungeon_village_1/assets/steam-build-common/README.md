# Steam建设目录差异素材

2026-10-10。按用户全部素材供产品消费的授权，从固定Steam2.56研究副本独立发布common图片98／103。供产品按[MANIFEST.json](MANIFEST.json)复制资源到自身assets；运行不读取Unity容器或research/work。旧APK包和Steam启动包没有修改。

| 槽 | 文件 | 尺寸／字节 | 消费者 |
| --- | --- | --- | --- |
| 图片98 | [tenant_resident.png](original/common/tenant_resident.png) | 50×15／512 | Steam raw21入驻状态，SEB81 frame0 |
| 图片103 | [number05.png](original/common/number05.png) | 100×21／701 | Steam raw21住宅数量，Draw_multiValue＋SEB12 |

两个PNG是Steam条目原始字节，共1213字节；均与APK的PNG字节、RGBA像素不同，不可alias APK图片。已实际查看两张输出原图。对应SEB81／12与现有APK出版副本逐字节相同，因此清单以相对路径alias引用，未重复复制SEB。清单共有4逻辑资源、1669逻辑字节；所有alias也须核hash。

清单逐项登记容器、TextAsset对象、条目、SHA-256、RGBA SHA-256、尺寸、INF槽和已核消费者；SEB原裁片／偏移／翻转一并登记。完整绘制合同见[设施图块](../../ui/STEAM_MAPCHIP_PATTERNS.md)，动态装载与旋转资格增量见[Steam资源安装](../../ui/STEAM_BUILD_RESOURCE_INSTALL.md)。这些是固定common根条目，不代表所有运行时语言替换已认证。

```powershell
python -B research/dungeon_village_1/tools/scripts/publish_steam_build_common.py
python -B research/dungeon_village_1/tools/scripts/publish_steam_build_common.py --check
```

[发布器](../../tools/scripts/publish_steam_build_common.py)仅允许两张PNG成为新素材；在内存解common，不复制整归档。发布前核全部来源与目标，复用既有发布器的普通路径／拒绝异内容／独占创建／失败按身份回收协议；重跑不增加副本。独立`.gitattributes`使两图保持原始字节。`--check`只读验证，不覆盖旧内容。
