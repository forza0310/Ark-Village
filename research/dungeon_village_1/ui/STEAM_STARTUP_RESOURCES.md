# Steam启动资源与字体身份

2026-10-09。此批从已登记Steam2.56研究副本的Unity对象重新读取、解开Kairo归档，并与固定APK1.0.8发布素材比较；没有运行游戏、读取实时档案或导出字体文件。结论是**多数共用木框图块相同，但标题背景、Logo和草边不同，字体不能沿用Android字体假定**。原布局消费者见[启动皮肤合同](STARTUP_SKIN.md)；本批不把相同图片升级为相同Steam绘制代码。

可复算[脚本](../work/steam-startup-resource-map/audit.py)与[机器证据](../work/steam-startup-resource-map/EVIDENCE.json)保留容器／对象／条目身份、字节及解码RGBA SHA-256、目录原行、字体表范围和校验和。来源是[全图像清单](../assets/IMAGE_COVERAGE.md)，这次只深化启动专题，不改变全量分母或素材发布范围。

## 图块身份

三组均为`KairoGames_Data/resources.assets`的TextAsset：title pathID1138、event pathID1140、common pathID1148。容器SHA-256为`75a7ac65688841505603393e3b013f26cd581d74f829aec9de3a55c73c21271b`。ID不是渲染层ID，归档中图片槽必须另看img.inf。

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

Steam title img.inf保留前五原名称顺序，新增第6槽`upper.png`，240×9，SHA-256为`1f311279fb86d248557e3752f1d4616384492ccde9a9c838f81a38935645a18c`。APK目录没有该槽。Steam元数据`form.TitleForm`亦声明`RESTITLE_IMG_UPPER=6`，但本批未解码该槽的实际Draw调用，不由截图顶部纹理猜其用途。

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

## 后续最小消费者定位

已有本机IL2CPP[元数据](../work/persistence-replay-analysis/exe/dumper/dump.cs:37183)给出`kairo.unity.ui.Font`，包含`defaultGUIFontLanguage_`、`invalidFontNames_`、`GUIStyle`、本地化scale／offset和纹理开关。它证明这些名字和入口存在，空方法体不证明逻辑。

- 字体选择：`_getDefaultGUIFont` RVA0x77F4A0、`_updateGUIFont` RVA0x780710；先核语言→对象／系统字体回退，再核中文的实际对象。
- 字宽与基线：`_stringWidthF` RVA0x77FFB0、`_stringWidth` RVA0x780620、`_draw` RVA0x77F450；分开整数／浮点测宽、本地化缩放、纹理或GUIStyle分支。
- 标题：`TitleForm.Draw` RVA0x20A640与`_draw` RVA0x20DF30；核600×380背景的裁取／缩放、English.lproj选择、upper槽和层序，不能照抄APK240宽坐标。
- 木框：从Steam实际纪录／新局／计分页追调用，再核common槽28/29与角SEB6的接收参数；现有同图证据不能证明拉伸／重复／边线策略。

以上仅登记具名入口，不在本批扩展机器码解码范围。原窗口可补观察实际文字、框线与缩放，但截图不能单独确定字体对象、measure算法、语言回退或内部槽。
