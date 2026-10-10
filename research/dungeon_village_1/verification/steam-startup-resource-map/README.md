# Steam启动素材专题复算

2026-10-09。[正式结论](../../ui/STEAM_STARTUP_RESOURCES.md)供后续皮肤接入读取；[EVIDENCE.json](EVIDENCE.json)是逐身份/hash索引，不含原字体、图片载荷或反编译实现。

在仓库根目录运行：

```powershell
py -3 research/dungeon_village_1/tools/steam-startup-resource-map/audit.py --check
```

脚本从原有全图清单取Unity对象位置，先校验完整容器和对象hash；只借用既有image_coverage.py的两个纯函数重新解开归档／解码PNG，不执行其写入口。字体只读本机研究副本内Font对象的sfnt载荷，校验表目录、范围、不重叠、校验和及name/head/maxp；Unity其余字段、cmap覆盖、栅格化和运行时选择未认证。

本批20条两版对应：14条PNG（含English Logo变体）、6条SEB；16条字节相同，10条PNG像素相同。另核Steam独有upper.png及6份两版INF。5个Font对象中4份含已校验嵌入字体。生成与`--check`复算均通过，累计22,870项结构/身份检查；这些是分析器检查，**不是CTest或完整游戏回归**。

资源／引用／输出检查：仅保留脚本、JSON与本说明，零原始图像/字体副本、零新增构建树；纯只读脚本无后台线程、窗口、监测或运行时实体。JSON由正式专题引用，按固定对象和图块数增长，不使用常驻缓存。原图全量清单、原研究程序和其他会话文件未修改。主会话将集中核差异和链接并提交；本子任务未提交。
