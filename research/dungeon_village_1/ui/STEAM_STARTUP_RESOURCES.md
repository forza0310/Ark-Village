# Steam启动资源与字体身份

2026-10-09。此批从已登记Steam2.56研究副本的Unity对象重新读取、解开Kairo归档，并与固定APK1.0.8发布素材比较；没有运行游戏、读取实时档案或导出字体文件。结论是**多数共用木框图块相同，但标题背景、Logo和草边不同，字体不能沿用Android字体假定**。原布局消费者见[启动皮肤合同](STARTUP_SKIN.md)；本批不把相同图片升级为相同Steam绘制代码。

可复算脚本（本地核对材料，不随仓库交付）与机器证据（本地核对材料，不随仓库交付）保留容器／对象／条目身份、字节及解码RGBA SHA-256、目录原行、字体表范围和校验和。来源是[全图像清单](../assets/IMAGE_COVERAGE.md)，这次只深化启动专题，不改变全量分母或素材发布范围。

后继已交付[Steam启动正式素材](../assets/steam-startup/README.md)：38逻辑项以21个同字节别名复用APK发布副本，另发布17个差异文件；原761包分母不变。标题upper、两Logo和十种saveload路径具备可核文件，PNG／SEB／目录均登记；这批不发布未核字体，也不代表完整Steam皮肤或运行时语言选择已经认证。

## 图块身份

三组均为`KairoGames_Data/resources.assets`的TextAsset：title pathID1138、event pathID1140、common pathID1148。容器与条目身份统一查[图像索引](../assets/IMAGE_COVERAGE.json)，不在本文重复指纹。ID不是渲染层ID，归档中图片槽必须另看img.inf。

| 图块 | APK尺寸 | Steam尺寸 | 当前核验 |
| --- | --- | --- | --- |
| title00 | 240×330 | 600×380 | 字节和像素均不同，不能把APK背景放大称为Steam原图 |
| title_window | 98×68 | 98×68 | 字节、RGBA相同 |
| title_logo默认条目 | 236×115 | 236×115 | 字节、RGBA不同 |
| English.lproj/title_logo | 对应APK默认236×115 | 236×132 | 字节、RGBA不同；存在英语变体不单独证明当前中文UI选它 |
| title_cursor | 28×23 | 28×23 | 字节、RGBA相同 |
| title_grass | 240×18 | 240×18 | 字节、RGBA不同 |
| event_backGlad | 180×80 | 180×80 | 字节、RGBA相同；纪录整图／新局裁片属于不同消费者 |
| event_medelCelemony_back2 | 180×80 | 180×80 | 字节、RGBA相同 |
| wnd_back／wnd_bar／wnd_conner | 40×240／240×17／8×8 | 同左 | 字节、RGBA相同 |
| arrow01／arrow02／finger_r | 14×5／16×8／16×10 | 同左 | 字节、RGBA相同 |

同时核过六个common SEB：wnd_back、wnd_bar、wnd_conner、arrow01、arrow02、finger_r，均与APK逐字节相同。这只让相应原SEB结构可交叉复用；实际帧、偏移、翻转、时钟与输入扩大范围仍由各版调用者决定。

Steam title img.inf保留前五原名称顺序，新增第6槽`upper.png`，240×9；APK目录没有该槽。`form.TitleForm.RESTITLE_IMG_UPPER=6`与后继具名机器码一致：[标题Draw](STEAM_TITLE_DRAW.md)已核它在原点0、y0处从x0起按240重复至逻辑W之前，位于背景之后、人物之前。身份沿启动素材清单，不由截图顶部纹理猜用途或按APK画布拉伸。

event的img.inf／seb.inf与APK字节相同。common则有扩展：img.inf行数135→136、seb.inf101→102；新增179号`bouken2_icon.png`及同名SEB，且若干旧图行的扩展修饰不同。因此不能以本批六个SEB相同宣称整个common包或加载修饰等价。机器证据保留两版完整目录行；INF行数不等于最大图片ID。

## 嵌入字体：已核身份，不代替运行时选字

Unity class128共有五对象。默认资源中的Arial只有286字节对象，本批只核名字和对象身份，不宣称其带完整字形。resources.assets另有四个Font对象，逐一核对内嵌TrueType sfnt目录、表范围／不重叠、每表校验和、name/head/maxp字段。

| pathID／对象名 | 字体内部family | 字体载荷字节 | unitsPerEm／glyph数 |
| --- | --- | ---: | --- |
| 1178 default-en-switch | M+ 1p medium | 2,072,612 | 1024／15,563 |
| 1179 default-th | Noto Sans Thai | 33,872 | 2048／108 |
| 1180 default-ja | m_plus | 1,920,716 | 1024／15,563 |
| 1181 default-hi | Kairo Hindi（Thin） | 86,736 | 1000／197 |

两个15,563字形字体不是同一字节载荷；相同glyph数不证明字形、字宽或字符覆盖相同。glyph数来自maxp，不是可用Unicode字符数。这次没有解析cmap覆盖、实际字形轮廓或运行时fallback，也没有把这些字体复制到产品。

本样本四个Font载荷在对齐对象名称后68字节处，前4字节为长度；这是经内部sfnt自洽校验的局部位置，不是完整Unity Font字段合同。载荷后分别剩42／46／38／58字节，保留未解码状态。五张零尺寸`Font Texture`仍是原盘点中的运行时占位，不能推断中文不支持或图集丢失。

## 消费者现状与剩余边界

已有本机IL2CPP[元数据](../work/persistence-replay-analysis/exe/dumper/dump.cs:37183)给出`kairo.unity.ui.Font`，包含`defaultGUIFontLanguage_`、`invalidFontNames_`、`GUIStyle`、本地化scale／offset和纹理开关。它证明这些名字和入口存在，空方法体不证明逻辑。

- 字体选择与测宽：[字体消费者](STEAM_FONT_CONSUMER.md)已核语言地区→基础→Japanese专属→通用资源链、GUIStyle条件替换、整数／浮点测宽及本地化公式；ResourceManager首表1251行只有en-switch／hi／ja／th四组字体路径，没有中文或通用default。不能据M+载荷猜中文实际字体。
- 字体原生交接：[文字后端](STEAM_FONT_BACKEND.md)已核托管空font到原生默认字体的交接、开关资格与栈恢复；[普通文字绘制](STEAM_FONT_DRAW.md)已核锚点、字号补偿、矩阵→clip→GUI位置。真实当前GUI.skin、原生返回字体对象、中文glyph fallback提供者和字形度量仍未知，不能把交接闭合写成真实字体已知。
- 标题：[标题Draw](STEAM_TITLE_DRAW.md)已核600×380背景按实际尺寸居中贴底、upper／草边重复、Logo公式、10／75／100门槛，以及语言／DPI候选首命中链。纯背景有序计划止于语言／菜单之前，保留实际人物绘制插入点；实际中文会话选择的Logo仍不由UI文字推断。
- 菜单与木框：[标题菜单](STEAM_TITLE_MENU.md)和[Steam窗框](STEAM_WINDOW_FRAME.md)分别闭合局部菜单／选档及木纹重复、标题带、边线、角片与裁剪边界；[维护启动皮肤接口](../prototype/include/dungeon_village_prototype/steam_startup_skin.hpp)已有标题两项、两栏选档、raw20／1／14及共用窗框的只读计划。它们是已核代码消费者和维护局部交付，不能退回“仅同图”；也不代表所有页面的完整组合、真实字体或原窗口像素已经验收。

原窗口可补观察实际文字、框线与缩放；截图不能单独确定字体对象、测宽算法、语言回退、内部槽或逻辑到物理像素的完整变换。后续沿上述合同的具名剩余边界推进，不重复扫描已核upper、木框或字体交接。
