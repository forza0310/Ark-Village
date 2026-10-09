# Steam窗框／裁剪与保存菜单样式补证

2026-10-09。正式合同见[STEAM_WINDOW_FRAME](../../ui/STEAM_WINDOW_FRAME.md)。本批只读固定Steam2.56 DLL／metadata及既有资源清单、出版副本；没有执行原游戏、操作原档、修改产品／C++或旧冻结包，没有构建／CTest。

## 来源和复算

[inspect.cjs](inspect.cjs)固定登记19个具名方法入口／末界，共13,264字节；逐次只接受manifest或白名单键，核DLL、metadata、方法索引和PE执行区映射。默认打印有界反汇编供审读，不落盘方法全文，不用全镜像扫描推测调用者。可用键为windowDefault/window/boxDefault/box/window2/window3/whiteCorner/drawRect/fillRect/pushClip/popClip/setClip/clipRect/guiGroup/setClipInternal/updateGLClip/transRect/beginMatrix/transRectGL。

```powershell
node research/dungeon_village_1/work/steam-window-frame-contract/inspect.cjs manifest
node research/dungeon_village_1/work/steam-window-frame-contract/audit.cjs
```

[audit.cjs](audit.cjs)复核27个E8调用锚点、两个实际float常量、3个颜色初始化窗口、7个已有调用窗口、5项资源身份与图片／SEB索引，生成唯一[EVIDENCE.json](EVIDENCE.json)。19方法总预算≤16KiB；调用窗口均在旧标题／SubForm已核范围内，复用来源hash单独登记，不将它们算新增方法字节。

固定DLL SHA-256 `9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a`，metadata SHA-256 `80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369`。方法末界取下一已知入口，可能含padding，不把padding解释成逻辑；不同重载机器码相同不合并方法身份。

## 本批增量

Steam DrawWindow／DrawBox标准几何与APK合同相容，现在有Steam自身指令支持；补齐白角mode1图片121、无角其它mode、DrawWindow2仅木纹及DrawWindow3显式坐标／H+1裁高差异。DrawRect以外／内矩形四边环实现，FillRect保持原尺寸，不能混作同一stroke。

当前Steam计划留空的slot_type／slot_name／date／cash／empty_slot／answer颜色已定位，空手动选中底色是现有计划真实漏项。此批只登记缺口不改实现。选档短clip在类型文字绘制之前已Pop，不能错误应用到整窗；内部clip取整常量实际为约0.01，不是猜测的0.5。

原5份资源共3,563字节，均为已出版副本；没有复制图片或改38项启动包。clip栈按历史深度复用容量，GLText引用生命周期沿既有合同；本批不宣称当前计数归零就释放全部资源，也不认证整局内存有界。

未决限定：当前中文glyph／Unity fallback、完整TextLayout／屏幕矩阵／shader内部、物理DPI及真实窗口对照、其它页面全部样式。静态合同不等于新窗口验收或完整皮肤实现。探针同步退出，无后台进程。
