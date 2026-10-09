# Steam文字后端与默认GameSkin证据

2026-10-09。正式合同见[文字后端](../../ui/STEAM_FONT_BACKEND.md)。只读固定Steam2.56，复用旧[字体消费者](../steam-font-consumer/README.md)和[启动资源身份](../steam-startup-resource-map/EVIDENCE.json)；不修改旧证据，不运行游戏／窗口，不读用户配置或存档，不导出字体素材。

- [EVIDENCE.json](EVIDENCE.json)：16个具名方法与1个直接调用目标helper，共19窗口、21508字节独立范围；方法表、探针脚本哈希及边界限制。
- [DATA.json](DATA.json)：GameSkin→GUISkin脚本／Arial对象的三个本地引用身份，字体／后端相关字段，以及Editor插件反射所用的3个固定字符串槽。
- `inspect.cjs`从具名入口顺序解码，单次打印≤4112字节；OnGUI清理helper先核具名调用者入口到call的指令边界及实际目标，再只读116字节。DefaultUseFontTexture setter只取前80字节，不把到下一具名方法的496字节间距全部当函数体。
- `data.cjs`对已有全素材清单中的3对象逐项复核容器／对象hash、名称／引用／字段位置；不重跑全图像扫描、不保存完整对象或字体。

复算：

```powershell
node research/dungeon_village_1/work/steam-font-backend-contract/audit.cjs
```

GameSkin引用Arial是资源事实，当前中文实际字形仍未知。纹理开关的Graphics.Init／Push／Pop／SetFont与四类文字落点已核；完整后端、矩阵、clip、TextLayout与退休不作已完成结论。方法范围并非每条分支都已形成正式合同；未采纳的范围保持未知。

两个派生JSON均由正式合同消费，无新增大缓存、原图、字体副本或后台进程。纯静态研究不构建、不运行CTest；文档链接、脚本语法、来源及幂等复算由本批收口。
