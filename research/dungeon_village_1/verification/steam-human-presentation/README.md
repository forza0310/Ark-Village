# Steam人物表现helper：静态证据与复核

2026-10-10。补齐人物详情60的普通肖像调度、实例血条、危险图标条件、奖章和气泡。正式消费者合同见[STEAM_HUMAN_PRESENTATION](../../ui/STEAM_HUMAN_PRESENTATION.md)，原四页布局和倒下入口沿[人物详情60](../../ui/STEAM_HUMAN_DETAIL.md)，外层输入沿[人物输入](../../ui/STEAM_HUMAN_INPUT.md)。本包不是原窗口实验，不涉及当前玩家存档、账号或内存修改。

## 已核事实与范围

固定Steam样本GameAssembly SHA-256为`9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a`，metadata为`80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369`。使用既有Il2CppDumper方法索引及iced-x86 1.21.0有限解码，不重新导出程序、不新增整套反编译副本。

| 新具名方法 | RVA | 核验字节 | 本包消费 |
| --- | --- | ---: | --- |
| Draw_medal | 0x254F70 | 448 | 5枚阈值、重复图标与乘号数字 |
| Draw_fukidashi | 0x253230 | 768 | Font测宽、60阈值、背景拼接与文本 |
| Draw_charaPowerBarToSubForm | 0x2D0A80 | 1152 | raw60实际item=null／eff=false分支及实例PB_NOW |
| UserData..cctor | 0x2F0040 | 20368 | 仅输出MOVE_DATA六数组初始化的固定片段；方法全体字节只用于身份 |

合计22,736字节，36个调用／固定字节锚。另复用已冻结的`Draw_charaInfoChara`1,312字节摘要（RVA0x2CFF00），没有把它重列为新增方法；EVIDENCE中`reused_portrait`保留旧清单对象，其中旧对象自带`reused:false`是其原批次字段，不表示本批新解码计数包含它。正文以本段新增／复用边界为准。

MOVE_DATA六个16字节blob从初始化调用传入的RuntimeFieldHandle，反查metadata字段默认值，再核其SHA-256与字段名一致。六行实际为`[1,75,75,10]`、`[1,75,98,30]`、`[1,98,98,10]`、`[2,98,98,10]`、`[2,98,75,30]`、`[2,75,75,10]`。100槽周期是原表duration之和；区间0–9／10–39／40–49／50–59／60–89／90–99是结合旧肖像consumer严格比较条件的直接推导，不是实测时间。

metadata映射列PB_NOW=1、PB_AFTER=3；机器码分别读`powerBar_[1]`作为血条长度、`powerBar_[3]`作为≤30整数百分比的危险判定。raw60不存在活动实例时在肖像之后返回，不调用通用血条的null fallback。倒下state7由raw60外层另一分支绘制及“ﾀﾞｳﾝ中”气泡，不能把它套用普通MOVE_DATA。

## 素材引用与证据消费

EVIDENCE.json（本地核对材料，不随仓库交付）记录7个资源引用：4 PNG共1,807字节、3 SEB共524字节；不复制资源。PNG为image7／35／22／103，SEB为28／39／12，所有SEB图片索引及裁片范围已核。image103明确复用Steam建设资源包；其余通过EXE覆盖清单逐文件证明已有路径字节相同，不按APK同名自动认证。

`inspect.cjs`只接受固定键，stdout输出有限指定范围；`audit.cjs`只重写本包EVIDENCE，记录固定来源、方法摘要、36锚、六blob、资源、旧证据及脚本自身hash。不会扫描“最新”游戏文件或向产品目录写入。

证据由新正式UI合同消费，旧人物详情合同不在本包改写；本包不改变共享索引、历史证书、C++／CMake或正式资源。没有运行态对象或输出队列，资源统计是本包静态文件规模，不是整个游戏内存有界证明。

## 复现命令与本次结果

在仓库根执行：

核对命令属于本地研究过程，不随仓库交付。

复用肖像consumer可执行`node research/dungeon_village_1/tools/steam-human-detail/inspect.cjs portrait`。`schedule-location`只列已命名静态初始化方法中访问+0x128字段的指令；不将其它静态表一并宣布解析完成。

本次已独立复算脚本语法、固定源hash、四方法边界、36字节锚、六blob、7资源与SEB裁片；已读普通肖像及全部三个小helper，交叉metadata的PB常量和字段。重复audit幂等，EVIDENCE SHA-256为`10fc0b796b5b404715b6d41491bb15651636975fed37df0fc8c5ad3480cd98fe`；固定schedule输出165行。UTF-8无BOM／LF、本文及新合同10个本地链接、差异检查通过。两份脚本及JSON共27,525字节，中文合同和本文分别归档，不生成资源副本。全部是前台短任务，没有构建、游戏窗口、后台服务或原游戏写入。

## 仍未闭合

1. 危险图标的Character2静态相对偏移表+0xA0只证索引分支，未解具体值；不补猜测坐标。
2. 人物／武器动态资源树和GetAnimeIndex仍委托已有／后继专题，不从六行MOVE_DATA推算全部动画帧。
3. 原字体实际像素、父级裁剪、OS输入、原窗口同版截图及完整皮肤没有本批动态认证。
4. power-bar的item加值数字／eff动画是通用helper其它分支，raw60调用明确不使用；本合同不扩大成其它页面全部已还原。
5. 共享`SetDispPlayerData`写入须沿已授权Owner显式表现请求／独立回放接线；本文仅来源合同，没有假装已增加维护渲染消费者。
