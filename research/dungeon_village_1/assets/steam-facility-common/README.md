# Steam设施详情差异素材

2026-10-10。独立发布固定Steam2.56已核common图片37／105；旧APK包、Steam启动包与建设包保持原冻结身份。产品依据[MANIFEST.json](MANIFEST.json)复制自身运行资源，不读取Unity容器或research/work。

| 槽 | 素材 | 尺寸／字节 | 已核消费者 |
| --- | --- | --- | --- |
| 图片37 | [icon_param00.png](original/common/icon_param00.png) | 112×32／1790 | raw74使用效果Draw_icon(mode7)，第二行16×16；类型5用x96，其他x=16×(类型%10) |
| 图片105 | [number08.png](original/common/number08.png) | 100×21／648 | raw74价格／维护费／品质／魅力／种类数及效果点数，common SEB15 |

两PNG共2438字节，都是Steam原条目字节，已实际查看输出图片。两者与APK同名图的字节、RGBA均不同，不能把旧APK副本当别名。SEB15 number08.seb与现有出版副本逐字节一致，仅在清单相对引用，不复制；其21帧裁片、偏移、翻转及图片105依赖全部登记。3逻辑资源共2866字节。

源容器、Unity对象边界／hash、INF槽、原条目／RGBA hash、APK差分、消费者与坐标合同逐项登记。完整行为见[Steam设施详情](../../ui/STEAM_FACILITY_DETAIL.md)。这里只发布固定common根条目，不证明全部语言替换、最终字体、真实窗口或产品接入已完成。

```powershell
python -B research/dungeon_village_1/tools/scripts/publish_steam_facility_common.py
python -B research/dungeon_village_1/tools/scripts/publish_steam_facility_common.py --check
```

[发布器](../../tools/scripts/publish_steam_facility_common.py)仅在内存解固定common；采用既有普通路径校验、拒绝异内容、独占创建与失败按身份回收协议。重跑幂等，--check只读，.gitattributes禁止PNG换行转换。未复制整游戏／归档，无构建缓存或后台任务。
