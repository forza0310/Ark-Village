# 设施74第二页：逐来源图标与显示载荷

2026-10-08。来源合同、只读维护接口与真实新局有界窗口已交付；研究适配与原EXE动态分开验收。
针对[产品高优先级请求](../../../docs/reference/RESEARCH_REQUESTS.md#facility-details-display-gap)，
交付固定汉化重签APK1.0.8的74第二页规则及资源桥。Steam原页面机器码／动态尚未独立认证，
异版本截图“罗汉松10/11”不能改写固定APK定义名或数值。

## 来源与证据

- APK身份沿用[证据清单](../EVIDENCE.md)；正式规则、来源去重及道路分别见[设施邻接](../rules/FACILITIES.md#neighbourhood)。
- [初始化b/g:3752](../work/decompiled/sources/b/g.java:3752)至3760将目标实例`m.w`逐项复制为页面`Y`，保持序列，且无排序／去重。
- [绘制b/g:7185](../work/decompiled/sources/b/g.java:7185)至7244消费`Y`，
  由[低层GamePage:25265](../work/world-page-fallback/GamePage.java:25265)至25524的L712–L93e交叉。
- [来源写入a/o:419](../work/decompiled/sources/a/o.java:419)至460按全局活跃实例顺序重算；
  [实例序号c/m:101](../work/decompiled/sources/c/m.java:101)至158与读写225／281闭合名称后缀。
- [图标helper b/g:1407](../work/decompiled/sources/b/g.java:1407)至1410：类别9是整PNG裁片，没有额外背景层，也没有SEB帧调用。

输入哈希、范围和当前全表结构核验归档于[工作包](../work/facility-bonus-rows-source/README.md)。
本次只读既有缓存和原表，没有新反编译／DLL探针、游戏窗口、存档修改或后台进程。

## 来源身份、顺序和名称后缀

原来源项是`m.w[i]=[sourceDefinition.n, sourceInstance.f207c]`。
规则重算以全局实例`n.g`的来源次序逐源处理；一个来源对同一目标只记一次，
即便多个外围格命中同一大设施；两个同定义实例仍是两条来源。
页面初始化冻结这一序列，绘制每项直接使用，既不按设施名、属性、等级排序，也不合并重复项。
若页面`Y`出现重复来源，原绘制照序画两行；正常Owner构造去重与只读投影保留序列是两个合同。

| 原字段 | 显示／维护映射 |
| --- | --- |
| 来源定义n | `definition_id`；决定名称、kind、图标、x/y载荷 |
| 来源实例f207c | `facility_ordinals[stable_instance_id]`；从0开始、同定义内首个未占用序号 |
| 显示名称 | `definition.name + decimal(ordinal+1)`，不加空格／Lv／括号，不读取共享等级G |
| 稳定实例ID | 维护`NeighbourSource.instance_id`，用于准确绑定；不当名称后缀，不暴露原对象引用 |

因此截图“防具店1”对应同定义序号0；“罗汉松10/11”的**后缀机制**对应9/10，
但固定APK表定义71名为“槙树”，并非截图的“罗汉松”，不能仅凭相似外观断言两来源定义相同。
原序号分配逐0、1…检查当前同定义实例，取首个空号；撤除后可复用，既不是累计建造次数，也不是永久全局ID。

## 左侧图标的精确资源桥

每行`rowY=97+19*(index-scroll)`，最多5行；类别9图标锚为`(26,rowY−2)`。
输入是来源定义`o.f81d`，即原设施表第2列、维护`legacy_icon`。

| 项目 | 原合同 |
| --- | --- |
| 图片槽 | common `img[91]`，`icon_tenantInfo.png` |
| 原素材 | [icon_tenantInfo.png](../assets/original/common/icon_tenantInfo.png)，112×16，1009字节 |
| SHA-256 | `84b281a46d2a82d1c8ed75d7ac36c7a90b804d7b6aeee4291986e060c3db7aeb` |
| 源裁片 | `((legacy_icon%10)*16,0,16,16)`；按原尺寸绘，无缩放／旋转／居中策略 |
| 背景 | 类别9helper仅画此裁片，无独立背景；它不套普通道具类别1的18×18背景 |

来源名x=44；适配判断`h.a()`为真用y=rowY，另一分支切字体10后用y=rowY+1。
这是原画布坐标，不是桌面DPI物理热区。原行注册SLIST11为`(-1,rowY−2,231,18)`，
value=`0x20000|index`；此调用没有marker16显式选项。选中指示SEB21锚`(17,rowY+8)`。
滚动helper参数`(221,85,110,Y.size−1,5)`，不能用滚动条宽度推来源条目宽度。

## 右侧载荷不是累计贡献分摊

**原绘制按来源kind选择固定位置y值，完全不读取x槽位、实例邻接总额、共享等级或目标上限。**

| 来源kind | 原序显示属性ID／标签 | raw值 | h.a()真坐标 | 另一分支坐标 |
| --- | --- | --- | --- | --- |
| 2（装饰） | 0／价格，随后1／品质 | y[0]、y[1] | x=121、171；y=rowY | 字体9；x=130、168；y=rowY+2 |
| 其它（正常来源为3） | 2／魅力 | y[0] | x=147；y=rowY | 字体9；x=151；y=rowY+2 |

标签经过语言映射并着色`#0065FF`；值的原字符串是**字面“+”连接原有符号整数**。
不加G后缀、不取绝对值、不乘等级、不按十取整、不封顶、不省略0。
例如负数载荷夹具`−3`的原表达式会得到`+-3`；这不是固定原表存在负显示数值的证明。
当前原表47个可传播来源（kind2九个、kind3三十八个）均非负，kind2至少两值、kind3至少一值。

规则消费者另按`x/y`配对累加。重复x槽应保留原列表并逐项累计，
但显示仍使用上述固定位置和固定标签；不能为“更合理”按属性归并或改变列表标签。
当前85定义全部x/y并行等长，47来源无空列表／负值／重复x；因此现有维护
`neighbour_effects[i].delta`保持原序时可取i=0／1作为原y，而不能按`attribute_slot`检索替代。
这项全表一致性只涵盖固定输入；缺少必需y载荷必须显式拒绝，不能填0凑行。

## 道路、无效果来源与空态

- 每个目标外环state3道路格给累计魅力`+2`；道路这一轮不追加`m.w`，所以原74来源行没有“道路”伪来源。
  即使邻接总魅力非0，只要Y空，仍显示“没有奖励”，不能用总额生成一行。
- 来源kind2／3与目标kind3命中后，即使来源x/y为空仍追加身份。
  原绘制并不设“无效果”分支，而会直接读必需y索引；固定原表可传播来源没有这种空载荷。
  维护安全投影应报告载荷错误，既不省略身份，也不补演示数值。
- 零载荷合法时保留`+0`；两个同定义实例、重复页面项和重复属性槽按各自合同保留。
- `Y.size()==0`显示“没有奖励”，锚`(120,97)`、居中对齐2；仍保留维护费。
  74第二页确认没有经营操作，不把只读列表选择当领奖／消费入口。

## 可实施的只读read-model

已实现`startup_world_facility_bonus_rows(state,page)`，声明在[building接口](../prototype/include/dungeon_village_prototype/startup_world_building.hpp)，只消费已初始化`facility_page_neighbours[page]`，
按原序每行携带稳定instance／definition、ordinal、原名称与完整显示名、legacy_icon、
类别9裁片，以及原序`values[{attribute_id,label,raw_signed,display_text}]`。
另给空态与可见窗映射；定义预览无实例不伪造邻接列表。

先校验页面类型／初始化、父绑定及每个来源实例／定义一致性、ordinal存在且非负、图标合法，
再读必需位置载荷；任一错误返回显式失败，不能`map.at`异常或产出部分行。
用宽整数计算ordinal+1后检查显示载荷；整个投影不改世界、页面、资金、随机或来源序列。
不新增Owner长期缓存、不改schema、不从总量反推贡献；共享定义等级改变也不倍乘这一载荷。

主责验收应覆盖两个同定义来源的不同ordinal、装饰两值和商店魅力一值、原序与重复项，
道路有累计却无来源行、空态、负／零夹具表达式、坏初始化／身份／载荷显式拒绝以及Owner摘要只读。
原表数值／资源身份为独立oracle，维护组合通过与原窗口认证分别报告。

`StartupFacilityBonusRow`提供instance、definition、ordinal、icon、完整显示名name及原序values（attribute、label、value、text）；type9像素计划由[visuals接口](../prototype/include/dungeon_village_prototype/startup_world_visuals.hpp)按同definition取得。ordinal为零基原f207c，name已经追加ordinal+1；不得再加等级或第二次拼后缀。values.attribute是固定显示标签槽，不是从原x搜来的规则贡献槽。

`startup_world_facility_bonus_window(state,page,first)`返回first、total和最多5条原序行；first仅接受0…max(total−5,0)，空列表只接受0。全部来源先验证，隐藏到后面的坏来源也不能被滚动掩盖。返回计划不持有第二个Owner，不更改bJ／页面，实际选择与首行由调用方管理。

现有building套件覆盖两同定义槙树10／11、武器店魅力+10、重复行、七条来源的首／末五行窗口、越界滚动、空Y且累计非零、重复规则槽而固定显示标签、+-3／+0、坏ordinal／引用／图标／载荷／页字段，以及完整Owner摘要不变。字面量期望来自原71的20／25和30的10，不调用被测查询产生期望。

本批13项相关CTest通过37.20秒，补五行视窗后building／visuals／codec三项最终复验通过4.24秒；building2414检查、visuals20528检查。原型`--world --inspect-page world-facility-bonuses --frames 8`只开真实新局已有实例的74第二页，截图已查看“商店1”类别图标及魅力+10；0更新／0人物／0抽／5000G，没有注入设施、邻接行或资金。窗口键／滚轮与标题坐标是研究适配；实际OS投递、两同定义自然建设和Steam原窗口未由此认证。

具体日志／截图／资源及哈希见[本批验收](../work/facility-bonus-delivery/README.md)。现维护schema、原表和黄金回放不改；产品只读接收后自行迁入和验收，本批没有改产品页面。
