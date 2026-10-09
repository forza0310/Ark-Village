# Steam设施升级81静态研究包

2026-10-10。正式合同：[升级绘制／输入](../../ui/STEAM_FACILITY_UPGRADE.md)。仅新建本目录和该合同，不修改原游戏、旧冻结包、共同索引、C++或产品，不构建提交。

固定DLL SHA-256 `9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a`、metadata `80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369`。复用具名_draw2_4／Update2／Init2／SubForm.cctor的冻结反汇编，显示有限分支前逐行核DLL字节，未复制巨型全文。

新增8个具名helper共6352字节，最大2832字节；另直接从既有调用目标0x10002640核19字节数组读取成功路径，证实传入0读首项，不拿APK替代。计数表原blob重核metadata字节，C4/C8与softLabels81按实际cctor赋值核对。60锚、7原文字、17个已出版资源引用，裁片／偏移／图片依赖可复算。

```powershell
node research/dungeon_village_1/work/steam-facility-upgrade/inspect.cjs manifest
node research/dungeon_village_1/work/steam-facility-upgrade/inspect.cjs draw
node research/dungeon_village_1/work/steam-facility-upgrade/inspect.cjs actors
node research/dungeon_village_1/work/steam-facility-upgrade/inspect.cjs input
node research/dungeon_village_1/work/steam-facility-upgrade/inspect.cjs strength
node research/dungeon_village_1/work/steam-facility-upgrade/inspect.cjs count
node research/dungeon_village_1/work/steam-facility-upgrade/audit.cjs
```

其他固定键为touch、init、tables、labels、prepare、actor、cursor、rate、parabola、cursor2。inspect只stdout；audit仅写本目录EVIDENCE.json。自写文本UTF-8无BOM、LF，规范化后生成hash；语法、源身份、资源引用、链接与重复生成幂等收口。不对旧证据重签。

EVIDENCE由独立合同消费；只保留有限摘要，不复制原PNG、SEB、原表或方法全文。共享显示三槽、实例与定义引用的退休不是本批完整认证范围，不承诺长期世界常驻内存有界。没有新后台／游戏／监测进程，也不触碰其他会话的构建或长测。
