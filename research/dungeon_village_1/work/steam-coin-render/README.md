# Steam金币X4局部来源交叉

2026-10-10。正式增量见[STEAM_COIN_RENDER](../../ui/STEAM_COIN_RENDER.md)，独立于已验APK的X4代码和旧工作包；本批不改其源码、测试、COMBAT_RENDER冻结内容或产品文件。

只读具名入口：AppData.DrawEffect、AddEffectCoin、UpdateEffect，Camera.ConvertScreenToCamera的int重载，GameUtil.SetVStop／SetAStop。6方法唯一16,240字节，单方法最大13,488字节，总量低于16KiB；不是全DLL扫描。DrawEffect先核入口kind4分派，再从具名入口顺序解码，仅输出该530字节分支及尾循环；其它分支不作合同认证。最初4KiB前缀用于定位，最终预算为包含它的完整登记范围，不重复累计。

```powershell
node research/dungeon_village_1/work/steam-coin-render/inspect.cjs manifest
node research/dungeon_village_1/work/steam-coin-render/inspect.cjs drawCoin
node research/dungeon_village_1/work/steam-coin-render/audit.cjs
```

探针核固定DLL／方法索引hash与PE可执行映射。常量[-16]／[23]复用此前`headless-save-audit-analysis/steam-static/disassembly.json`已登记的AppData.cctor，重新核方法hash和必要字段写入字节，只保存最小窗口身份，不复制旧反汇编。独立字段来自dump.cs，资源来自EXE覆盖清单并与出版副本hash比对。

已确认：负年龄隐藏；运动整数截断和11门槛；正dy截0；14计数循环帧；相机转换后加dy；SEB94／图片144；Update达到23移除及前向删除后不回退索引；生产数组六字段和v=-320／a=29。待证：Steam死亡处调用wait、完整场景／HUD调用相位、原窗口连续动作及实际绘制次数。资源同字节只是其中一项证据，未替代调用链。

[audit.cjs](audit.cjs)生成[EVIDENCE.json](EVIDENCE.json)，含6调用锚点、6语义窗口、复用静态初始化身份、实际float -2常量、字段身份及两份资源记录。脚本已执行通过；不构建、不运行CTest／原游戏。无进程驻留、无图像／存档／缓存副本；原素材仍626字节，本包只增加脚本、来源摘要和中文说明。工作包当前规模由交付时实际文件清单统计，不宣称已有历史增长是泄漏。
