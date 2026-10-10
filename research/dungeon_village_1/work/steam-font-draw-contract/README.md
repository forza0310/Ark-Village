# Steam文字Graphics消费者：有界静态合同

2026-10-10。按[字体消费者缺口](../../ui/STEAM_FONT_CONSUMER.md)，接续[新窗口反馈分析](../skin-observation-analysis/ANALYSIS.md)，选择具名文字绘制／阴影／局部裁剪入口。正式成果为[STEAM_FONT_DRAW](../../ui/STEAM_FONT_DRAW.md)。本包止于已命名矩阵、TextLayout和Unity GUI接口，不追外部插件内部或从截图猜字体。

## 固定来源及新增范围

GameAssembly SHA-256 `9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a`；metadata `80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369`。沿原methods索引逐项检查具名入口、最近下一入口、PE映射及方法hash；索引／dump映射及旧字体包的hash写入[EVIDENCE.json](EVIDENCE.json)。

| 方法范围 | RVA | 字节 |
| --- | --- | ---: |
| 默认anchor DrawString重载 | 0x766290 | 64 |
| 显式anchor DrawString重载 | 0x7678C0 | 64 |
| DrawString主实现 | 0x7663D0 | 5360 |
| 普通DrawStringShadow主实现 | 0x765E60 | 1072 |
| CheckClip | 0x75DA80 | 224 |
| _setClip | 0x77BC40 | 288 |
| Font.GetWidth | 0x77D980 | 32 |
| BeginRenderMatrix | 0x75D570 | 592 |
| GetTransRectGUI | 0x76F4A0 | 368 |

合计9方法8,064字节，审计保持单批≤8KiB守卫。完整方法字节用于身份与顺序解码，正文只认已经阅读的普通GUI／阴影／锚点／裁剪分支；不能由方法总字节声称主实现的GLText、纹理和离屏所有分支已还原。原始反汇编只输出stdout固定片段，不保存方法全文副本。

## 本批闭合的合同

- 默认anchor0x11；中心优先于右，中间优先于底部；Font.usedSize、StringWidthF共享入口与Font.GetWidth基准宽度分离。
- 原本地化字号差补偿、整数fontScale补偿、BeginRenderMatrix副作用、screenMatrix映射及减clipRegion.left/top的次序。
- GUIStyle字号的变换比例及float32+0.001向零取整，普通GUI.Label最终矩形参数；不把矩形y认证成字形baseline。
- 普通阴影每轮三个正向偏移，再原色正文；outlineWidth／quality仍是原动态字段，不补默认数。
- `_setClip`各边float32+0.01后向零取整，CheckClip返回false不阻止该函数更新矩形。

机器证据68锚、4个原float常量位型、16条字段／anchor映射。RVA0x77E770存在StringWidthF和StringWidthF2两个metadata别名，同机器体只登记共享入口；不随反汇编名称映射“最后项”推不同实现。类似共享构造器的打印别名没有作为正文类语义依据。

## 复现与验收

```powershell
node --check research/dungeon_village_1/work/steam-font-draw-contract/inspect.cjs
node --check research/dungeon_village_1/work/steam-font-draw-contract/audit.cjs
node research/dungeon_village_1/work/steam-font-draw-contract/inspect.cjs manifest
node research/dungeon_village_1/work/steam-font-draw-contract/inspect.cjs shadow
node research/dungeon_village_1/work/steam-font-draw-contract/inspect.cjs position
node research/dungeon_village_1/work/steam-font-draw-contract/inspect.cjs gui
node research/dungeon_village_1/work/steam-font-draw-contract/inspect.cjs setClip
node research/dungeon_village_1/work/steam-font-draw-contract/audit.cjs
```

其它固定inspect键有plain、anchor、start、draw-calls、checkClip、width、beginMatrix、transRect、texture；最后一项仅供明确后继边界查看，不改变当前合同覆盖等级。inspect无文件写入；audit仅更新本包EVIDENCE，核固定源、方法、原指令与调用目标、常量位型、字段／别名、旧证据和脚本hash。

本次已运行源hash、边界、68锚、4常量、16字段及脚本语法检查；两次audit幂等，EVIDENCE SHA-256为`2e269c1232e10edea712412c6225e28138d722cfee61482efa6595b8a7ed80af`。UTF-8无BOM／LF、8个本地链接及diff检查通过。未读取原存档、未启动游戏、未构建或提交，没有后台任务。没有字体／PNG／DLL副本或新增缓存；证据由本README和新正式合同消费，旧合同与索引保持不动。

## 证据限制

窗口截图只能提供外观参照，不能确定当前语言返回值、实际字体对象、字形baseline或客户端缩放。本包没有导出或发布字体；Unity GUIStyle空字体fallback、字形ascent/descent、kerning和插件纹理内部仍未闭合。矩阵接口的调用顺序已证，但其所有设置来源、局部坐标栈实现、默认outline参数、TextLayout标签Shadow2及GL／离屏路径仍独立。

静态方法和审计文件规模不是运行时内存有界保证；临时字体／颜色／矩阵状态的恢复属于表现生命周期，不能与世界随机、人物业务状态或存档恢复混用。
