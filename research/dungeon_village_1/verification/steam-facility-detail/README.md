# Steam设施详情74静态研究包

2026-10-10。正式合同见[设施详情](../../ui/STEAM_FACILITY_DETAIL.md)。仅本目录与该独立合同是本批交付，不改旧冻结包、共同索引、C++、产品或原游戏文件，不构建，不启动游戏。

## 输入与范围

固定Steam GameAssembly SHA-256 `9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a`，metadata SHA-256 `80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369`。使用已有Il2CppDumper映射和iced-x86 1.21.0。

- 复用已冻结具名_draw2_3与Update2的指令行，展示前重新对照DLL实际字节；旧方法范围合计55,344字节，不重新输出全文。
- 复用已有GameForm._update具名入口，顺序解码10,480字节，仅stdout显示设施入口窗口。
- 新增Init2、DrawMapchip2、Draw_titleBarArrow、Draw_icon四个具名方法共19,792字节；每方法≤16KiB，Init2仅stdout显示74分派和其分支。没有中途地址初始化解码器、全区扫描或反汇编全文副本。
- 61个直接调用／固定指令锚，28个metadata原文字，20个资源引用（含SEB间接图片）；18个已出版同字节/Steam副本，2个未出版Steam效果／数字图明确留缺口。旧证据raw哈希按本机真实文件登记，不重签原历史清单。

原文字不等于当前语言屏幕字面；S043等历史画面与静态消费者分别登记，APK未作为本包缺项替代来源。

## 复核命令

在仓库根运行：

核对命令属于本地研究过程，不随仓库交付。

其他固定键为returns、previewReturn、mapchip2、arrow、icon。inspect只写stdout；audit只生成本目录EVIDENCE.json。脚本、JSON与Markdown统一UTF-8无BOM、LF，先规范化再生成哈希。检查包括语法、锚点、资源别名、重复生成幂等、文档链接和Git差异；不触发游戏回归。

## 输出消费、引用与规模边界

EVIDENCE由独立合同消费，登记方法／分支／脚本／源及资源身份；不重复复制PNG、SEB、原表或原方法全文。ary2只是列表层复制且保留int[]引用，原表单退休未核完整，不宣称原页常驻内存有界。维护测试长历史增长与本包静态规模无关。

本轮所有命令为前台短任务，无新增后台／游戏／监测进程，无需清理他人缓存。本包无构建缓存，未更动父会话正在运行的测试DLL。
