# 美术素材与原型接入

原始素材发布包以固定的一代汉化重签 APK 为输入。身份与限制见 [证据索引](../EVIDENCE.md)，
实施授权见 [R2-A 阶段记录](../stages/history/R2-prototype.md#assets)。这里只描述研究交付，产品接入单独验收。

2026-10-08新增[APK／EXE全容器图像覆盖](IMAGE_COVERAGE.md)和[资源代码反查](RESOURCE_CODE_INDEX.md)。
下文398 PNG／761文件是既有11视觉归档发布范围，**不是全部APK／EXE图像分母**；平台、引擎、语言及重复别名另在新清单登记。

## 当前交付

- [Steam控制器后备图集](steam-platform)：`buttons_switch`／`buttons_ps4`／`buttons_xbox`各含原PNG及SEB，共6文件103028字节；3张图均512×512，SEB各1层44帧，按[全量索引](IMAGE_COVERAGE.json)中的managed kairolib身份校验。源槽分别为SEB13／image15、SEB19／image22、SEB29／image33；[正式后备接口](../prototype/include/dungeon_village_prototype/steam_platform_button_skin.hpp)保留设备类型分流、实际OK／Cancel反转与层重映射。[裁片参考](../ui/examples/steam-controller-buttons.png)不是当前设备绑定或原窗口；动态图像优先、无控制器frame7鼠标图及外层字体矩阵仍按[字体合同](../ui/STEAM_FONT_CONSUMER.md#内嵌按钮提示)分开。

- [Steam平台键盘／绑定按钮](steam-platform)：`button_keyboard.png/.seb`（1681／28字节，64×42，平台img29／SEB25）与`button_gamepad.png/.seb`（4225／28字节，64×60，img27／SEB23），共4原文件5962字节。原容器为`DungeonVillageEXE/KairoGames_Data/il2cpp_data/Resources/KairoLibrary.dll-resources.dat`中的`KairoLibrary.resource.kairolib.bytes`，沿[全量索引](IMAGE_COVERAGE.json)同名四条记录校验原字节／PNG像素，不新增指纹清单。不是Unity同名kairolib对象，也不属游戏common；两SEB均一层一帧一裁片，键盘绘制用25但热区仍取23。供[正式纯计划](../prototype/include/dungeon_village_prototype/steam_platform_button_skin.hpp)消费，运行只需复制该子集；高亮生成shader、控制器附加glyph和平台动作的资格见[交互合同](../ui/STEAM_INTERACTIONS.md#平台键盘与控制器按钮的素材和局部绘制)。

- 说明页箭头新增[Steam buttoneffect.png](steam-common/buttoneffect.png)：128×128、14563字节，原样来自固定`resources.assets`的system对象pathID1136、`drawable-mdpi/buttoneffect.png`，身份沿[全量索引](IMAGE_COVERAGE.json)的`EXE:resources.assets:1136:system:drawable-mdpi/buttoneffect.png`。同对象另有hdpi／ldpi，当前显式选择mdpi，不声称原窗口运行密度已核；与APK同名图不同像素，不覆写原包。GameView AddTouch使用arrow02的SEB bounds，再消费此独立纹理的普通／选中箭头裁片；逻辑0.75缩放和图片密度比各自处理，合同见[说明页](../ui/PAGES.md#说明13固定正文输入消费与装饰随机)。实查`steam-common`当前共8张PNG＋1个SEB、20101字节；保留来源入口，不新增指纹清单。

- 主菜单期间新增Steam `common2/mpot_event.png`（image0、94×22／744字节），出版为[steam-common/mpot_event.png](steam-common/mpot_event.png)。来源为固定resources.assets的common2对象pathID1146，沿[全量索引](IMAGE_COVERAGE.json)原记录；与APK同名图不同，不覆写原包。主菜单底图、七图标及NEW／GET／手形使用已核同字节副本，期间数字复用Steam设施number08；[消费者合同](../ui/PAGES.md#steam主菜单缓存输入与局部皮肤)与[路径接口](../prototype/include/dungeon_village_prototype/steam_main_menu_skin.hpp)明确组域。此图加入时`steam-common`为七PNG＋一SEB、5538字节，当前规模以上项为准。

- [Steam共用差异资源](steam-common)：其中common对象子集为六张PNG及一个SEB共4794字节。既有`menuRT01.png`（image85，57×10／469字节）、`icon_objRoots.png`（image128，148×36／1432字节）、`icon_result00.png`（image31，63×48／1000字节）、`number12.png`（image109，100×21／636字节）；菜单状态栏新增`menuRT00.png`（image84，59×45／945字节）及其SEB75（88字节／四帧）、`menuRT04.png`（image151，11×11／224字节）。均为固定`resources.assets`的common对象pathID1148原条目，源身份沿[全量索引](IMAGE_COVERAGE.json)的`EXE:resources.assets:1148:common:<文件名>`记录；同名APK有差异，不复制其它同字节SEB。image31用于35贡献及菜单怪物摘要，image109用于39负利润，84／151用于菜单HUD与剩余期；均未裁切或重绘。消费者见[信息菜单](../ui/INFORMATION_MENU.md)，正式[资源路径查询](../prototype/include/dungeon_village_prototype/steam_information_skin.hpp)供产品复制后使用；不另建指纹清单，语言替换与原字体另验。

- [Steam人物详情差异图](steam-human-common/README.md)：image88 wnd_ato，10×7／187字节；明确源条目／RGBA／APK差异及raw60经验区消费者，不扩称全部语言替换已核。

- [Steam设施详情差异图](steam-facility-common/README.md)：common图片37／105独立发布，共2,438字节；SEB15仅引用已有同字节副本，3逻辑资源共2,866字节。两图均与APK像素不同，消费者见[设施详情74](../ui/STEAM_FACILITY_DETAIL.md)，操作补证见[左右软标签](../ui/STEAM_FACILITY_LABELS.md)；不把静态资源发布当作窗口或产品接入验收。
- [Steam启动／选档子集](steam-startup/README.md)：38逻辑条目，21项同字节复用原包、17项差异载荷独立发布；保留完整原INF但只允许加载显式发布条目，不含未知中文字体。
- [APK音频](audio/README.md)：26个原始Ogg与实际ID／通道清单，独立于视觉761项；播放操作与设备后端另验。
- [原始素材](original)：11 个视觉归档，761 个文件，包含 398 张 PNG、341 个 SEB、22 个 INF。
- [规范化素材](normalized)：七个活动逻辑键使用的 PNG；完整图集字节不变，帧在清单中指定。
- [来源清单](MANIFEST.tsv)：每个原始文件的归档名、内部条目名、SHA-256、图片尺寸和转换说明。
- [独立原型](../prototype)：C++17 + raylib；通过 [状态聚合层](../prototype/include/dungeon_village_prototype/village.hpp)
  组装 [维护规则](../example)，只读必要设施表，不依赖 APK 或反编译 Java。
- [提取工具](../tools)：循环 XOR、游戏专用 CRC、归档边界检查、SEB 解析与确定性发布。

全部 PNG 可解码；371 张含透明像素，没有字节相同的 PNG 副本。未发现 PAL、OPT、未知图片编码或
压缩视觉条目，因此无需额外图像转换。所有归档均按存在 `img.inf` 检测，而不是只处理地图资源。

原始素材用于用户授权的内部学习，权利仍属于原权利人；本说明不授予公开再分发许可。
`original/` 保留只读参考，不应作为换图的编辑目标。

## 归档与图片索引

归档先验证固定输入的游戏 CRC，再在内存副本上循环 XOR；条目头使用大端字段。
标准 `crc32` 与非标准 `game_crc32` 分开，不能互换。发布器再次解析 SEB 并核对 PNG 头；
PNG 的实际像素解码由原型素材测试验证。

`img.inf` 是 CRLF 文本，形式为 `图片索引<TAB>文件名`，索引可能稀疏。
原始索引内的 `.gif` 在当前包中对应实际 `.png`；不能把文件名扩展名当成图片编码。
`seb.inf` 给出 SEB 加载顺序，还可能带实例修饰参数；它不等于动画时间表。
当前恢复并保留全部原始索引，但没有实现通用索引修饰符执行器。

## SEB 格式与证据边界

本次 341 个 SEB 全部为旧格式；精灵读取与绘制的直接证据是 JADX 输出中的
`kairo/android/ui/t.java`：读取方法第 98—170 行，绘制方法第 359 行起。
这些生成文件不属于维护源码。

所有整数按大端读取。旧格式结构：

| 层级 | 字段 | 处理 |
| --- | --- | --- |
| 文件头 | 图层数量、总帧数量，各 16 位 | 按有符号计数拒绝负值与超限 |
| 图层头 | 记录数量、`legacy_tag`，各 16 位 | 后者按原始位保存，不当作数量 |
| 每条记录 | 帧号、图片索引、源 x/y、宽/高、偏移 x/y、两个附加字段，共十个 16 位整数 | 完整保留 |

非负图片索引的两个附加字段用于水平/垂直翻转；负图片索引可代表绘图命令，附加字段还有颜色等含义，
不能对所有记录一律限制为布尔值。当前解析器严格验证结构、计数、读取边界、截断和尾随字节，
不声称已验证所有命令的运行时语义。原型选中的帧另检查实际 PNG 边界。

`common/finger_d.seb` 为 6 帧、4 条关键帧、`legacy_tag = 6`；旧 Java 读取器直接丢弃该字段。
因此不能要求它等于记录数量，也不能把“图层跨度”这一猜测写成事实。用户已确认保留未解释标签。
`image/plain00.seb` 还含不用于首帧显示的额外记录，故不能把每条记录都当成可独立渲染的 PNG 切片。

全量结果：346 个图层、1228 条记录，最大总帧数为 60。压缩 SEB 格式明确拒绝，尚未实现。
本轮不复刻关键帧插值、复合精灵或完整动画播放器。

## 七个逻辑键

本节仅属早期R2七键换图适配；当前完整世界和Steam页面按各自资源合同消费，不受此七键范围限制。

清单的 `frame` 是源图上的 `x,y,width,height`，`anchor` 是相对帧左上角的 `x,y`。
原型按“屏幕锚点减 anchor”放置图片。

| 逻辑键 | 来源 PNG | 首帧 | 原型锚点 |
| --- | --- | --- | --- |
| `terrain.grass` | `image/plain00.png` | `0,0,60,29` | `30,14` |
| `terrain.road` | `image/road00.png` | `0,0,60,29` | `30,14` |
| `building.inn` | `image/tenant10.png` | `0,0,60,60` | `30,45` |
| `building.cafe` | `image/t_cafe.png` | `0,0,60,60` | `30,45` |
| `building.inn.pair.front` | `image/t_inn00.png` | `0,0,60,48` | `30,33` |
| `building.inn.pair.back` | `image/t_inn01.png` | `0,0,60,51` | `30,36` |
| `character.night` | `human/chara_night00.png` | `0,0,18,24` | `9,24` |

图片索引由对应归档的 `img.inf` 确认，源矩形由同名 SEB 的首记录确认。
角色使用 `human/walk00.seb`，其中图片索引 0 对应夜骑士而非冒险者，不能仅按图片名字猜角色绑定。
地表取菱形中心 `(30,14)`；建筑采用同一地表中心与 SEB 偏移相减：普通旅店和咖啡館 y 偏移均为 -31，
换算后为 `(30,45)`；人物取负绘制偏移 `(9,24)`。
这些屏幕锚点属于原型坐标适配，不是原作设施入口、占地或完整几何规则。

R2-C 已纠正旅店绑定：定义 28 是单格普通旅店，通过地图显示记录 48 使用 `tenant10.seb`/图片索引 25；
早期 R2-A 的 `t_inn00` 实际是定义 29 双格旅店的一个分片，不能用一帧代表完整双格设施。
当前规范化路径为 `normalized/building.inn.single.png`；旧 `normalized/building.inn.png` 仅为历史快照保留，
没有逻辑键指向它。收敛版补了双格旅店两片的活动逻辑键，奇数分片按绑定表水平翻转。
完整已核对的两朝向/双格分片见 [精灵绑定表](../data/SPRITE_BINDINGS.tsv) 和 [设施报告](../rules/FACILITIES.md#definitions)。
两片锚点同样以地表中心减 SEB 的 y 偏移 -19/-22 得到，不能当成设施入口。
收敛版只修改研究交付，不改产品中已有素材副本。

## 构建与操作

当前使用项目内单Release动态构建树，具体命令见[原型入口](../prototype/README.md)；新增素材按受影响消费者做短检查，不恢复历史Debug／Release双缓存矩阵。资源示例、原窗口观察和产品验收分别见[图例](../ui/examples/README.md)、[参考归档](../references/README.md)及[验证入口](../VERIFICATION.md)。

早期7×7演示、固定数值／首帧、无存档和macOS窗口结果均仅属[R2历史原型](../stages/history/R2-prototype.md)，不是当前完整世界能力或Steam动态结论。已退休缓存路径和重复演示命令不在本资源入口继续维护。

## 替换自有素材

早期七键换图夹具仍按`MANIFEST.tsv`的逻辑键、规范化路径、帧与锚点解释；只在新的隔离目录生成替换图，不改`original/`或把自有图片标成原游戏字节。来源哈希指向固定输入，自有素材另记来源／转换。完整页面使用对应版本的原资源索引与消费者，不靠修改原表或扩大旧七键清单消除差异；工具用法见[素材工具](../tools/README.md)。

## 当前待验收

当前完成范围与剩余字体／动态图像／平台输入均以[阶段矩阵](../stages/README.md)和各主责合同为准。R2曾免验的鼠标操作及未运行的用户试玩仅保留历史资格，不等于后继Steam窗口或物理输入免验；既有试玩参考见[PLAYTEST](../stages/PLAYTEST.md)。
