# Steam 标题绘制局部证据

2026-10-09。正式合同：[STEAM_TITLE_DRAW.md](../../ui/STEAM_TITLE_DRAW.md)。只读固定DLL／metadata，复用已安装iced-x86；不运行游戏，不读原档，不导出机器码全文／图片，不构建／提交或修改产品。

[inspect.cjs](../../tools/steam-title-draw/inspect.cjs)只接受固定具名入口。`wrapper`为Draw完整208字节，`draw`为`_draw`入口2048字节，`logo`仍从同一入口顺序解码前4096字节、仅抑制已审首段stdout；`begin`为SmartBeginPaint304字节，`staticInit`为类型静态初始化992字节。后继加入`init`及ResourceManager／JarInflater／AssetReader／Language具名加载链；`imageLoad`同样从图像任务入口顺序解码前4096字节，非中途起点。没有从任意中途地址新建decoder，没有使用历史被拒的建设续窗。

20个具名方法、22个窗口，按不重叠范围统计唯一24576字节；其中`_draw`只核4096／11008，图像任务只核4096／5120，并非两个全方法。工具核固定DLL／metadata身份、方法／下个方法起点、PE执行节／raw映射、文件界和4096字节单窗口预算。窗口末端可截断一条指令，首窗末尾的`(bad)`在扩大的入口解码中得到完整指令，未把坏尾当作事实。其它末尾同样不作为新入口或语义证据。本批没有自动审批阻断。

[EVIDENCE.json](EVIDENCE.json)登记窗口哈希、关键已消费锚点、类型字段证据和原资源证据引用。指令只在stdout检查，不保存全文。资源身份继续引用上批20图／SEB审计，不重复导出图像或创建第二套素材清单。[animation-data.cjs](../../tools/steam-title-draw/animation-data.cjs)沿单个已见RuntimeFieldHandle核fieldRef、私有类型、字段默认区、12字节与字段名摘要，输出[ANIMATION.json](ANIMATION.json)；[resource-data.cjs](../../tools/steam-title-draw/resource-data.cjs)核12个已见固定字符串槽及冻结Logo条目引用，输出[RESOURCE_DATA.json](RESOURCE_DATA.json)。两者均无自动写入或任意地址参数。

具体产出是背景色、图0居中／贴底、upper槽6与草边槽4的240步长重复、人物调用相对层序、Logo实参和逻辑尺寸资格；标题动画常量独立证得10／75／100。Logo候选的语言／DPI优先级、首命中及缺资源回退已串起，仍未证用户中文会话实际安装的langPackFolders／基础FOLDERS；不把当前字段名当运行值。`Language.cctor`77600字节未解，保留该明确边界；`_draw`全部后续菜单仍未闭合。

没有后台任务或新构建缓存；工作目录为三个只读脚本、三份机器证据与本README，正式合同独立放ui。主会话统一检查链接、规模、共享索引并保存checkpoint。
