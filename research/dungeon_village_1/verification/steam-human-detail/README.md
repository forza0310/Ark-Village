# Steam人物详情60静态研究包

2026-10-10。[正式合同](../../ui/STEAM_HUMAN_DETAIL.md)；EVIDENCE.json（本地核对材料，不随仓库交付）。本包只读固定Steam原文件，输出四页局部绘制、输入、初始化、软标签、人物展示委托及common图像对应。

## 证据与规模

GameAssembly SHA-256 `9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a`；metadata SHA-256 `80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369`。固定具名方法映射来自既有Il2CppDumper索引，解码使用既有iced-x86 1.21.0。

- 新增4个具名方法顺序解码共20,480字节：`_draw2_1`14,768，普通人物展示1,312，实例查找320，CharacterData静态初始化4,080；每方法不超过16KiB。stdout只显示固定有限分支，不保存原方法全文。
- 复用既有Update2、Init2、SubForm静态表冻结指令，读取时核源方法哈希、展示时逐行核DLL字节；AppData静态表8,068字节前缀和Draw_icon 2,784字节为已研究具名范围，不重签旧文件。
- 53个调用／固定字节锚；四页主体、Update2、Init2及表分支各自记录字节哈希。
- 25个common图像／SEB引用（18图、7SEB），24个已有同字节出版路径；image88没有匹配发布副本，明确保留缺口。仅引用，不复制PNG／SEB；所有已读SEB的图片依赖与裁片边界核验。

范围为原静态消费者，不是原运行窗口观察，不认定APK示例等同Steam；动态人物、武器、窗口、字体和委托helper不是上述资源分母。完整字段与具体原文见EVIDENCE；metadata原文字不等于当前语言最终画面字面。

## 复核与交付边界

核对命令属于本地研究过程，不随仓库交付。

inspect固定键还有dispatch、magic、equipment、stats、overview、footer、pop、init、table、labels、names、portrait、lookup、icon；只写stdout。audit只覆盖本目录EVIDENCE，不改旧证据、游戏、存档或用户数据。脚本、JSON、Markdown均UTF-8无BOM、LF；先规范化再登记脚本哈希。收口检查语法、字节锚、同字节资源引用、SEB边界、重复生成幂等、Markdown本地链接和差异。

EVIDENCE由正式合同消费；没有运行态持久引用、生成素材副本或构建缓存。原绘制共享展示写入／触摸注册与框架退休明确分开，未声称整个原程序内存永久有界。没有构建，不碰父会话运行中的DLL；所有复核都是前台短任务，未启动游戏或后台进程。共同索引、阶段验证与本地checkpoint由父会话集中处理。

## 本批生成器修正与后继发布

首次review证据SHA为`c81c9986361865715a71e0735a36414e1b179242b2df5c057855c36405ecffc6`。尚未提交期间，[image88新包](../../assets/steam-human-common/README.md)发布后复算曾因动态枚举全部steam-*目录，把历史24/25匹配数改变为25/25。已确认差异只有image88的published_alias；其改回null即可逐字节恢复上述旧哈希，旧副本仅作本地review对照，不列入交付。

现生成器改为固定当批三个出版目录白名单，并在EVIDENCE写明publication_scope。修后证据登记修后audit脚本真实哈希，不能冒称仍为旧c81；两次复算幂等，新包存在也不再漂移。24/25是限定出版范围的历史清单，当前image88已由后继包补齐。没有改源DLL、原表或其他已提交证据。
