# 情报菜单与统计页面

2026-10-09。回应产品[信息菜单最小缺口](../../../docs/reference/RESEARCH_REQUESTS.md#information-audio-animation-89f157c)，先闭合固定APK1.0.8的“主菜单→情报→收支36→翻页／返回→权威统计”原程序静态消费者链。2026-10-10的[维护页面控制器](../prototype/include/dungeon_village_prototype/startup_world_information.hpp)续接37／38，raw9现开放36／37／38；34／35保留条目但显式拒绝打开。38页面采用已核Steam2.56消费者差异，原表身份不变；本批集中验收另记，不宣称完整情报菜单或原窗口已验。

已有[UI索引](README.md)负责页面定位，[主菜单合同](PAGES.md#主菜单修正)负责默认五项，[周期账本](../rules/ACCOUNTING.md)负责资金与月报消费者。本文不重复设施74／商品79／人物60等已交付目录，不拿自动月报替代本页面；先完整交付一个无需人物稳定引用的新子页。

## 入口原序及返回路径

原主场景初始化保存`aJ = new g(3,0,25)`，菜单软键进入它。主菜单标签5打开`new g(9,父页横移+68或语言分支偏移,父页选中行*28)`，标签不是菜单行号。

raw9初始化实际按以下顺序追加，每项的原标签状态参数为1，没有逐项解锁过滤：

| 0起行号 | 标签ID | 原文 | 目标raw页 |
| --- | --- | --- | --- |
| 0 | 15 | 冒险者 | 35 |
| 1 | 14 | 村情报 | 34 |
| 2 | 16 | 收支情报 | 36 |
| 3 | 17 | 持有物品 | 37 |
| 4 | 18 | 装备一览 | 38 |

来源[b/g:10580](../work/decompiled/sources/b/g.java:10580)。**不是按14…18排序**；标签池中的19“职业一览”未在这个初始化分支加入，不能仅因字符串存在就显示第六项。人物行的NEW资格是另读`n.y()`，本批不扩为完整人物目录合同。

共用菜单更新[h():6807](../work/decompiled/sources/b/g.java:6807)与[低层h()](../work/world-page-fallback/GamePage.java:23993)一致：**上→else-if下→else确认／右→else返回／左**。方向是`aw.c`的重复资格，上下循环5行；上或下命中即结束本轮选择处理，不再消费同轮确认，不能按raw36的独立左右检查套用。确认脉冲1048576或右脉冲262144直接返回当前条目；否则左脉冲65536或软标签返回令`K=-1`并退休9，保留实际父菜单及其选择。raw9触摸行注册组件type9、值`0x20000|row`、行高28；维护显式选行不得与按键同次混用，这不是Steam物理点击语义认证。

原公共update先将`f124d`加1并按INT_MAX回卷，raw9的h()再加1、封顶3并请求重画，所以新页完整更新是0→2→3；确认／右选择还会置3。raw36不调用h()，仅作公共每轮加1回卷。两者只能在获准更新中推进，纯查询不加计数。所核菜单／关闭／压页局部没有直接播放声音请求，维护本批不补猜测的选择／确认音效。

选择标签16的[b/g:11260](../work/decompiled/sources/b/g.java:11260)分支先压入`new g(36)`，随后调用[d/a.m:545](../work/decompiled/sources/d/a.java:545)。后者逆序扫描表单，将`g.e()`识别的菜单类全部标记退休；该分类包括raw3和raw9，**不包括raw36**。因此正常链为：

```text
场景 → 主菜单3 → 情报9
选择“收支情报” → 压入36 → 退休菜单3和9
36确认或返回 → 退休36 → 恢复底层场景
```

不能把收支页返回固定实现成回情报菜单，也不能在选择36时仅保留一个看不见但仍可更新的raw9。若非菜单入口另行打开36，返回对象取实际未退休父栈；不从页号硬编码“永远场景”。重新按正常菜单入口创建36是新对象，其`i`由Java默认值0开始；未找到36专用初始化重置／数据复制分支，不能把重用同一对象再次初始化也认证成自动归零。

## 页签、输入和模态资格

raw36只有两种`i`：0月页、1年页。没有可选类别行、没有滚动、没有人物或设施实例引用。原[b/g:12064](../work/decompiled/sources/b/g.java:12064)与[低层分支](../work/world-page-fallback/GamePage.java:39653)相互支持：

1. `aw.c(65536)`成立先执行`i=(i−1+2)%2`。
2. 独立检查`aw.c(262144)`，成立再执行`i=(i+1)%2`。
3. 确认脉冲1048576成立则调用`m()`退休；否则检查当前软标签返回，也可退休。

左右不是`else if`：两者同时获准会按序切两次，结果回原页；同轮方向和确认可先切页再关闭。确认脉冲通过`aw.b`清除对应位，方向重复检查`aw.c`不按该消费方式处理。页面标题和箭头共用`f124d`的原获准更新计数，不从绘图FPS或墙钟秒数推断。

raw36的软标签配置由`bH[36]={0,2}`与`d/a.D`给出：左空、右“返回”。箭头由共用[b/g:11070](../work/decompiled/sources/b/g.java:11070)注册common SEB3 frame3／0，对应16／18方向标签；平台输入映射沿既有[输入合同](INPUT_RENDER_REQUESTS.md)。底部“月收支情报／年收支情报”调用的是只画文字的六参数助手，不注册点击组件，不应做成另一套“确认／切换”按钮。

框架[kairo/android/a/b.g](../work/decompiled/sources/kairo/android/a/b.java:311)只给栈顶且生命周期2的表单更新；生命周期0初始化、4退休、父页1→2恢复由独立入口处理。36盖在主场景上时，不调用主场景世界更新／日历；仍可重画背景，不意味着背景在经营。它不是自动月报那条“原世界继续更新”的路径。失焦／锁屏的全框架门槛另见同文件，不能以原页局部逻辑绕过。

## 权威统计及五行口径

原`c/n.z`为12×5×2有符号int数组：月份、类别、收入／支出。当前月索引来自`P.f515b[2]`（0…11）。[类别标签](../work/decompiled/sources/c/n.java:103)固定原序：

| 类别 | 标签 | 月页收入／支出 | 年页收入／支出 |
| --- | --- | --- | --- |
| 0 | 设施 | `z[currentMonth][0][0/1]` | `Σm=0…11 z[m][0][0/1]` |
| 1 | 怪物 | `z[currentMonth][1][0/1]` | `Σm=0…11 z[m][1][0/1]` |
| 2 | 冒险者 | `z[currentMonth][2][0/1]` | `Σm=0…11 z[m][2][0/1]` |
| 3 | 商店 | `z[currentMonth][3][0/1]` | `Σm=0…11 z[m][3][0/1]` |
| 4 | 其它 | `z[currentMonth][4][0/1]` | `Σm=0…11 z[m][4][0/1]` |

利润按上表顺序累计`income−expense`，收入列与支出列分别显示；不先改写为净额，不把点数／人气混入收入。月页不是前一个已结算月，年页不是最近12个月滚动窗口或从开局以来累计。原年页确实遍历全部12桶，不仅遍历0…currentMonth；普通路径未到月份的桶因跨年清空而为零。

日历跨年时[b/c:184](../work/decompiled/sources/b/c.java:184)调用`c/n.i()`，后者[4560](../work/decompiled/sources/c/n.java:4560)清全部12×5×2桶。原收入／支出通过[c/n.b(int,int):3162](../work/decompiled/sources/c/n.java:3162)及[c/n.c(int,int):3459](../work/decompiled/sources/c/n.java:3459)累加当前月对应方向，不是在打开收支页时结算。读写档保存全部桶，窗口不根据现金总额或事件历史重建它。

五类原口径已有[账本合同](../rules/ACCOUNTING.md#能直接交叉核对的消费者)：设施类包含建设支出，不只维护费；商店及其它确实是独立类别。[自动月报](TASK_REPORT_RENDER.md)仅使用前三类，故其K[2]/K[3]/K[4]不能代替36五行或利润。设施详情单实例月流水、标题系统最高资金也都不是这份统计。

原年行累加、每行差额及总利润均为Java int逐步运算，最后转long格式化；超32位范围时是原int回卷，不能用C++有符号溢出仿真，也不能悄悄改为无限64位年度合计。维护查询若复刻此边界，应显式实现32位位模式回卷，再安全扩展到int64显示；已有世界写入的数值拒绝规则保持不变，不为了显示计算放宽业务校验。

## 字段显示、位置与格式

下列是固定APK逻辑画布，来源[b/g:1800](../work/decompiled/sources/b/g.java:1800)，不是Steam物理像素。外层页变换沿[b/g:9105](../work/decompiled/sources/b/g.java:9105)；窗框素材与文字基线沿[启动窗框合同](STARTUP_SKIN.md#画布木框和字形)。

| 内容 | 原绘制参数 |
| --- | --- |
| 外窗 | 228×176，标题“收支情报 1/2”或“2/2” |
| 内容框 | 原边界参数(12,69,227,190) |
| 页签标记 | y65显示“月”或“年”，不是另加当前年月数字；语言分支x28／26 |
| 收入／支出表头 | y65，语言分支x121/194或x108/178，灰(156,155,155)，后分支临时字号10 |
| 五行标签 | x22，y=`90+18*category`，棕(92,51,31) |
| 收入值 | 右端x143，同一行y，蓝(0,100,255) |
| 支出值 | 右端x216，同一行y，红(255,14,1) |
| 分类横线 | 前四行在rowY+14，从x22至220，色(199,223,148)；第五行不画该线 |
| 利润分隔线 | y180，x22…220，色(146,184,247) |
| 利润 | 标签(22,184)，数值右端(216,184)；非负蓝、负值红 |
| 底部说明 | 中心x120，助手锚y202、实际文字y204；棕色，无橙底／手形／点击注册 |
| 左右箭头 | `(22−3*((f124d%16)/8),51)`与`(218+3*((f124d%16)/8),51)`；SEB3 frame3／0，原0或3像素偏移 |

**反编译重载警告已经交叉解决**：普通Java输出把三处金额写成`a("Ｇ", int)`，看起来像字符串模板替换；[低层收入／支出](../work/world-page-fallback/GamePage.java:8633)及[利润](../work/world-page-fallback/GamePage.java:8774)均保留明确`int→long`后调用金额重载。实际采用[b/d.a(String,long)](../work/decompiled/sources/b/d.java:56)，十进制按三位加逗号，负值保留`-`，后附全角“Ｇ”。不能据普通JADX省略的转换把数字画成单独一个Ｇ，也不能在利润负数时只换红色而漏负号。

## 现有维护Owner足够，不新造数据

已核维护映射如下，不要求新增世界统计字段或重建账本：

| 原载荷 | 当前维护来源与消费资格 |
| --- | --- |
| `P.f515b[2]` | `StartupWorldRuntimeState.scene.calendar.month` |
| `c/n.z[12][5][2]` | [monthly_cash](../prototype/include/dungeon_village_prototype/startup_world_runtime.hpp:92)，完整原维度已在Owner和codec中 |
| 五类方向 | `CashCategory={facilities,monsters,adventurers,shop,other}`及`CashDirection={income,expense}`原序 |
| 真实流水写桶 | [routes写回](../prototype/src/startup_world_runtime.cpp:269)只消费新现金事务，按当前月／类别／方向累加；原溢出拒绝保留 |
| 脚本资金链 | `startup_world_runtime_scripts`提供完整`finance.monthly_totals`，`post_finance`整数组写回，不只保留前三类 |
| 跨年 | [year_refresh](../prototype/src/startup_world_runtime_calendar.cpp:282)清完整monthly_cash |

已新增[最小只读投影](../prototype/include/dungeon_village_prototype/startup_information.hpp) `startup_income_information(const monthly_cash&、current_month、period)`：0月／1年，返回五个固定顺序的标签／收入／支出、利润值及已按原规则格式化的金额串。[独立实现](../prototype/src/startup_information.cpp)通过无符号模运算和安全64位符号扩展复刻Java int，避免C++有符号溢出及最小负值取负溢出。非法月份或period显式返回空optional；纯查询不推进页计数、日历、资金、随机、声音或文件，也不改变任何NEW标记。后继[页面实现](../prototype/src/startup_world_information.cpp)复用该查询，不另存现金桶副本；查询自身仍不负责创建或关闭页面。

维护页面复用`page_phases`保存raw9选行0..4、raw36页签0..1、raw37固定0、raw38页签0..3，`page_counters`保存原计数。37／38另增`information_page_data`：一组／四组冻结目录ID、当前选择和首个可见行；不保存第二份库存或装备定义。仅生命周期0且全部载荷不存在时初始化；已初始化缺任一字段、错误目录／选择／滚动均拒绝，不补默认值掩盖坏快照。生命周期4的完整载荷由下一框架入口统一退休；只读view可读1／2／3，输入只允许未暂停栈顶生命周期2。新增类型／map使生成codec的布局身份随字段自动变化，旧布局精确快照拒绝、不迁移；普通存档仍只允许稳定主场景。

入口接受稳定主场景作为维护主菜单适配，或真实存在的raw3，不伪造一张主菜单。返回只关9并恢复实际父级；选择36／37／38在候选Owner先压真实子页，再逆序退休原菜单分类3／4／5／7／8／9／10／20，失败不留半页栈。34／35仍显示原序及未实现标记，选择时显式拒绝、完整状态不变。子页关闭恢复实际未退休父栈；从普通菜单链进入时是场景，从其它合法入口进入时不硬编码父页。37空目录的事件15留到真实初始化，不在菜单选中时提前执行。

37初始化按正库存原序冻结一组目录；空目录必须产生事件15的真实提示页后才退休，不清NEW。正常确认／返回在同一候选先核对全部道具镜像、清全部items／catalog的NEW，再关闭；任一步失败完整保留原Owner。38冻结四组Steam目录，左／右独立，最终页签改变才重置选择与滚动，之后独立上／下；37为5行窗，38为4行窗，38合法空类跳过取模。独立选行不得与按键混用；不接调试确认后备或38→73未证入口。只读view取冻结身份对应的当前文字、库存和属性，关闭38不清装备NEW。

本维护实现对全部冻结目录（含未选页签）核对当前身份、资格和原交换排序，并要求完整一致。这是当前模态页且未开放改变库存／flag子命令的恢复约束，不声称原程序所有异步路径均不改共享定义；未来若新增此类合法命令须先重审该约束。空37仅可作为已执行提示并退休的生命周期4载荷，不能恢复成可选空列表。

原最小查询由[既有startup_skin_checks套件](../prototype/tests/startup_skin_checks.cpp)承接五类月／年、未到月份仍计全年、负利润、金额格式、非法输入及输入不变、原32位回卷的独立oracle；合成现金桶明确为条件夹具，不称自然路径。后继页面接线沿现有页面套件覆盖输入、父栈退休与模态资格，不重复底层金额矩阵。Steam36的[只读皮肤](../prototype/include/dungeon_village_prototype/steam_information_skin.hpp)也已接Owner视图，保留有序图元、默认文字重载、金额串、线端点和箭头注册；[布局示例](examples/steam-income.png)用替代字体展示产品接入形态。实际编译／统一测试结果见[当前验证](../VERIFICATION.md#信息菜单9与收支36维护消费者2026-10-10)，原字体／物理窗口另验。

资源规模：收支查询固定五行、11个金额字符串，不保留桶副本、人物／设施引用或输出队列；字符串仅由有界32位值生成。37／38目录上界来自当前规则定义数，typed载荷及phase／counter随真实页退休清理；这不代表世界现金流水或历史记录永久有界。页面与恢复测试负责输入顺序、整类NEW清理、空目录事件、模态资格及退休引用；不重复底层金额和图标矩阵。原自然字段可以用于UI示例，但没有本次Steam36截图，不生成伪原版界面。

Steam36的具名方法体交叉见后文，实际字体／热区的原窗口动态仍另验。37/38静态合同如下；raw34→39、raw35四页与60引用的局部进展见[统计目录分析](../verification/information-statistics-contract/ANALYSIS.md)。完整情报菜单维护交付仍需逐子页收口，35/37的NEW清理不能套到只读36。

## 持有道具37：目录只读，关闭清整类NEW

[初始化10823](../work/decompiled/sources/b/g.java:10823)按`av.by`原数组序只收`g.z>0`，没有额外`p==1`过滤或重排序；空目录请求事件15并退休，不制造可选空行。[绘制2232](../work/decompiled/sources/b/g.java:2232)标题“持有道具”，列头字面“姓名／持有”；最多5行、行高19、首行基线97。各行显示NEW、`g.g`图标、名字`g.c`、库存`g.z`，底部显示当前选中的`g.y`说明。NEW为common image147 `wnd_new.png`；选择手形为common SEB21 `finger_r.seb`；道具图标沿已交type1前景／类别底色合同。

[输入12078](../work/decompiled/sources/b/g.java:12078)先上、再下，独立重复检查并循环目录，再调整5行滚动窗。确认1048576或否则软返回2均关闭；**确认不使用道具、不赠礼、不打开详情**。行type11／`0x20000|row`只负责选中。两种正常关闭先调用[c/n.r:2236](../work/decompiled/sources/c/n.java:2236)清**全部**普通道具r，包括屏外及库存零项，然后退休37；不改库存、p或装备NEW。初始化空目录自动退出不经过该清除链。

正常从9进入时3/9已退休，37关闭返回底层场景。模态资格同36，绘制不推进世界。维护`items[id]`及`catalog[{0,id}]`已有库存／状态／NEW，关闭通过唯一Owner的`clear_startup_world_item_notices`与其它道具入口共用整类清除职责，不让UI另持第二可变副本。

静态说明`g.y`现已由正式生成器读取[原item表](../data/world/item.txt)第23列（0起），导出为`StartupWorldItem.description`，不改原表或加入可变世界schema。[持有查询](../prototype/include/dungeon_village_prototype/startup_information.hpp)按定义原序读取正库存，并拒绝items／catalog镜像不一致；返回原说明、名字、库存、NEW和图标。查询不清NEW或使用道具，整类NEW清除仅由37正常关闭事务消费；页面接线和原版完整皮肤分别验收。

## 装备一览38：四类含未知，确认关闭

[初始化10835](../work/decompiled/sources/b/g.java:10835)明确`Z=new Vector[4]`；顺序如下。原残留`i==4`道具分支不在数组与页签模数范围内，不能据此增加第五页。

| 页签 | 来源与过滤 | 原交换排序键 | 维护身份 |
| --- | --- | --- | --- |
| 0 | 全部武器，包含p0 | `p.u` | kind1＋原ID |
| 1 | 防具`b.d==2`，包含p0 | `b.j` | kind2＋原ID，slot1 |
| 2 | 防具`b.d!=2`，包含p0 | `b.j` | kind2＋原ID，slot2 |
| 3 | 全部饰品，包含p0 | `a.j` | kind3＋原ID |

每类保留原来源顺序，再执行“外层向后、内层从末尾向前、严格小于才交换”；已有赠礼目录过滤p0，不能直接用于38，稳定排序也不保证原等键顺序。

[绘制2273](../work/decompiled/sources/b/g.java:2273)每窗4行、行高24、首行基线97。仅`p==1`显示图标／名称／属性及r对应的common image148 `wnd_get.png`，其它p显示灰色“？？？？？”。底部“现在持有N种”只数当前页p1，**不是装备库存、免费份数或穿戴件数**。武器列`s[1]/s[3]`配属性图标1/3，其它列`h[0]/h[2]`配0/2，值大于0才画。页签头为common SEB88＋image128、帧1/2/3/4；行选中手形仍为SEB21。37／38的NEW图片147／148分别消费，不合并。

[输入12104](../work/decompiled/sources/b/g.java:12104)先左再右，最终页签改变才重置选中／滚动；双向同轮回原页时不重置。随后上／下，空类跳过取模，调整4行窗。确认关闭；否则返回关闭；本关闭分支**不清装备r**。只读绘制没有购买／赠礼／装配操作。

图标字段须严格区分：武器列表用`p.d`（原weapon第2列），现已存于`StartupWorldEquipment.shop.type`，取common image12；`render_image`是武器身体`p.e`，不能拿它作列表icon，也不需增加同值字段。防具／饰品render_image分别对应`b.e/a.e`，取common20／21。[装备图标计划](../prototype/include/dungeon_village_prototype/startup_world_visuals.hpp)先画common24第3格背景，再画18×18前景，格坐标`((icon%10)*18,(icon/10)*18)`；分别按原PNG的40／50／30格校验，不按武器定义数33截断图标35。

[装备目录查询](../prototype/include/dungeon_village_prototype/startup_information.hpp)明确要求选择APK1.0.8或Steam2.56语义，分别处理下文的flag过滤与非正属性占位；保留原交换排序、p!=1未知行及当前页已知种类数。维护38控制器固定选择Steam2.56，名称／属性仍读调用者安装的rules，不冒称已导入Steam所有原表。纯查询输出只含短期展示数据，不复制世界、清NEW、支付或安装页面载荷；冻结目录和动作由Owner控制器负责，完整皮肤另行收口。

## 38→73的条件分支不能冒充可达入口

38更新中确有软标签7处理分支，创建73并只传slot／原定义ID，不传人物；但**`bH[38]={0,2}`是左空／右返回**。[d/a.a:384](../work/decompiled/sources/d/a.java:384)检查7必须当前软标签文字为“情报”，单个物理键不能绕过。已查全文未见38设置左7，因此本批仅认证处理分支存在，**未认证正常玩家入口可达，不新增情报按钮**；Steam是否提供此按钮也未证。

如果另有合法入口，73[初始化3641](../work/decompiled/sources/b/g.java:3641)按同槽`p!=0`过滤并交换排序；传入目标不在目录时回首项。普通Java与[低层分支](../work/world-page-fallback/GamePage.java:14213)都确认72／73先进入目录逻辑，后续再次出现的单项73分支不可达。空目录的原越界边界没有明确保护，维护须显式拒绝、原子失败，不能制造默认条目。

当前维护73要求有效`page_human_bindings`，属已交人物装备路径；不应伪造人物引用来调用无人物来源的目录分支，也不能放宽既有人物路径验证。后续须先闭合入口可达性，再决定独立详情适配。此处是来源边界，未改C++、Owner或schema。

## 其余信息页的关键区别与后续

[34/39与35/60分析](../verification/information-statistics-contract/ANALYSIS.md)已登记：34按p1统计冒险者而35按p非0建定义目录；34设施数包含类型3/9，39只含类型3。39利润只累加实例0…当前月，区别于36年页全部12桶；39确认实际退出表单并以mode7移动镜头到设施，不能只因“点击进店”文字就直接开商店详情。35入页重算贡献，确认／返回清目录内人物NEW；进60传定义及y1，追踪时才寻找实际W，缺W则事件137。

37/38维护已分开纯目录投影与Owner关闭／退休动作，typed载荷交给既有codec和恢复校验；后继Steam完整局部绘制见下节。34／35／39消费者及完整文字／物理输入继续独立收口。前置资源复用与本批新增Steam两图分别登记；列表上界来自真实定义数，页退休释放ID绑定，不把合法世界历史增长当泄漏。维护验收见[当前验证](../VERIFICATION.md)，不以局部计划替代原窗口认证。

### 34统计投影与35贡献计算的维护前置

`startup_town_information`已复用当前唯一Owner给出34的原显示口径。固定APK与Steam raw34绘制分支（VA0x10335781–0x10336227）一致：p1冒险者、D[2]恰1的居民、原g内定义类型3／9的每次出现、任务成功v及活动F；右四种类只数装备p1，不用38的flag过滤，不额外增加住宅数或分母。村名仍属于应用配置，不在世界统计中复制。缺定义／名单引用或定义kind镜像失配显式拒绝，查询不修补Owner、不请求Steam平台成就。固定tenant表没有类型9行，测试仅用明确私有规则变体覆盖该原分支，不宣称发现了自然可建“类型9公园”。

35所需`prepare_world_human_contributions`从既有[授勋模块](../example/include/dungeon_village_reference/world_award_page.hpp)抽出同一算法：p非0参与，保持定义顺序、其它字段及p0原贡献，返回完整候选或明确错误；不加奖章、不排序、不产生输出。87仍独立验证页状态、增加一枚奖章及按原严格反向交换排序，调用时移动已有私有人物列表，避免重复复制。原整数均值／方差、double平方根转float及取整口径、显式溢出拒绝全部保留。35未来初始化可在唯一Owner内消费这份候选，不能直接调用授勋初始化。

现有projection／world_award_page／pages三套短测通过，总耗时2.35秒。统计条件、纯算法及年度页面分别由所属套件主责，无新增target或持久字段。这只是34／35入口所需的数据和算法前置；34平台成就、34→39、35贡献写回／NEW／60来源及追踪仍需后继Owner接线，不能据查询存在开放假入口。

### 35→60追踪的Steam后继合同

2026-10-10续核固定Steam2.56的具名方法体。35确认先在自身定义目录内清`BaseData.new_ +0x18`（VA `0x10323BBF`），随后构造60，写所选`CharacterData`至`charaData_ +0x104`，在`0x10323C27`写`charaInfoChase +0x134=1`后Push。不是先找到场上人物才准许打开详情，也不在此复制一个W。原APK对应详情与追踪链可在`c/n.a(a.e)`（生成Java约3086行）局部交叉；本节Steam地址、字段和方法以DLL／metadata为独立来源。

60初始软标签为`[8,2]`，其后`Init2`在VA `0x10311A02`只对来源值**恰0**改成`[0,2]`。`Update2`先处理确认／调试确认、返回2、转职11，才检查追踪8；在`0x1031E2F6`再次要求来源值**恰1**，才调用追踪helper。来源0不显示左追踪；来源1有正常追踪资格；其它整数不会被此Init覆盖左标签，但也不满足追踪分支，不能实现成“来源非零都可追踪”。这项原分支事实不放宽维护层只接受已确认来源值的载荷校验。[软标签与输入合同](STEAM_HUMAN_INPUT.md)已证翻页不自行重设标签。`AppData.IsPushSoftLabel(8)`的无额外键重载委托key0版本，比较当前左右标签与标签表第8项，再读取对应软键脉冲；标签8不是直接等于某个PC键码或任意“确认”。

`UserData.ProcChaseCommandFromInfoWindow`（RVA `0x2E23B0`）从`AppData.humans_ +0xF0`第0项开始，按名单顺序取`Character2.GetCharacterData()`，比较双方`BaseData.id +0x08`，选**首个同定义活跃W**；没有按人物实例UID／定义ID当同一个键，没有筛HP、倒下状态或重新创建人物。只读`GetCharaInTownFromCharaInfoWindow`（RVA `0x2DC0E0`）使用相同名单／定义匹配，缺实例返回null。名单为空或没有匹配是正常业务分支：追踪helper调用事件137，替换参数为当前定义`GetName()`、整数参数null，返回false；不退详情、不切场景、不改镜头引用。外层60在调用后直接返回已处理true，不把这个false当异常。事件137自己的页面退出后恢复实际父60；这与损坏的名单元素／引用应显式拒绝是不同情况。

找到W后原顺序是：`FormManagerBase.ChangeCurrentForm(AppData.frmGame_,false)`→`GameForm.ChangeState(6)`→`Camera.state_ +0x14=1`→`Camera.subPlayer_ +0x18=W`。`ChangeCurrentForm`（RVA `0x7E4A50`）对已经在栈中的目标先移出，给其余表单设mode4，再把目标加入并恢复；这不是仅退休菜单3／9，也不是只Pop60，因此正常35→60追踪成功后35和60都退出。`ChangeState`（RVA `0x2F6360`）设置相应场景软标签、`state_ +0x84=6`、`stateCnt_ +0x88=0`，清非空`Camera.subTenant_ +0x20`；本分支不因此清人物／怪物全部选择，更不改冒险者移动目标。原追踪helper没有检查`ChangeCurrentForm`的bool返回值，不能将原代码称为事务实现；维护Owner仍须联合校验、候选提交。

镜头移动在后续`GameForm._update`（RVA `0x303430`）的state6分支`0x1030385E–0x10303C77`发生，目标是所选W的`screen_pos_ +0x4C`，不是人物定义坐标。距离经`RateConvert(distance,10,150,5,26,true)`确定本轮速度，近于速度时贴合目标并回state0，否则按方向比例推进镜头；这些量不是墙钟秒数，也不是角色移动速度。后续若`subPlayer_`已为空，则回state0并返回false。追踪过程的确认会先回state0，再压新60，同时绑定W及其当前定义，来源仍设1（`0x10303C49`），不能自动改成来源0或回到旧35。维护现有`actor_camera_input`已具备镜头推进，但缺引用与此确认重开详情仍有明确拒绝边界，不能只因state6存在就声明这条玩家操作已维护。

返回和NEW分开：60软返回2走`FormBase.Pop()`（VA `0x1031FECD`），没有来源0／1分支，也不重清35目录NEW；返回哪一页取实际父栈，来源标志并不是父页ID。35已在进入60前清过目录NEW，因此追踪缺W、退出事件137、从60普通返回以及追踪成功的批量退休均不额外重清。`SubForm.Finish`（RVA `0x1EDFA0`）仅返回true，不是另一个NEW消费者；之后用户在35再次确认或返回，才会再次执行35自身的清除分支。

维护接线可复用`scene.world.world.ai.human_order`的原序稳定实例ID、`battle.actors[id].definition`、`actor_metadata[id].cached_view`、`scripts.selected_actor/selection_mode/selected_facility`及`scene.scene_state/scene_counter`，由唯一Owner协调页栈与事件137。须显式保存来源资格和父子引用，不能把定义ID填入实例选择器，或因无活跃W就删去可读的定义详情。本节只交源合同与现有字段映射，未实现35／60追踪控制器，未执行原窗口或自然经营链。

## Steam9／36的有限交互交叉

2026-10-10续批。固定Steam2.56 GameAssembly及metadata具名方法体的只读交叉，补本页先前仅有INFO_INCOME声明的边界；不是原窗口动态操作、字体／触摸热区或完整收支皮肤验收。此处页号均为十进制，RVA属于该固定样本。

`SubForm.Init`（RVA `0x312460`）的raw9分支在VA `0x1031503D–0x10315077`依次调用`AddMenu(15,1)、(14,1)、(16,1)、(17,1)、(18,1)`，与APK原序相同，没有追加19或逐项解锁过滤。`SubForm.Update`（RVA `0x321990`）在raw9调用`FrameMenu()`，标签14／15／16／17／18分别新建raw34／35／36／37／38；标签16分支先构造36并Push，然后执行`AppData.RemoveAllMenuForms`（RVA `0x25FDC0`）。后者逆序检查表单，按`SubForm.IsMenu`（RVA `0x3153E0`）识别并Pop：分类包含raw3／9，不含36。因此正常菜单链返回底层场景；非标准入口仍以实际未退休父栈为准，不硬编码回9。

`FrameMenu(int backResult)`（RVA `0x30E270`）先查上重复输入，命中即调整选择并返回；否则查下重复输入，命中同样返回。只有没有上下动作时，才处理确认脉冲／右脉冲，再处理软标签返回／左脉冲；取消将result写为backResult，无参包装传−1。这是与APK一致的互斥优先级，不能将“上、下、确认”实现为同轮顺序全部执行。raw9五行循环及确认右、返回左的资格在此成立，不据逻辑mask推具体PC物理键或鼠标点击次数。

raw36在`SubForm.Update`的VA `0x103237BB–0x10323879`则依次独立检查`CheckKeyRepeat(0x10000)`和`CheckKeyRepeat(0x40000)`，每次执行`page_=(page_+1)%2`；对合法0／1页签，与APK的左减一加二取模等价。随后检查`CheckKeyPulse(0x100000)`，否则检查`IsPushSoftLabel(2)`，成立即Pop。双方向同轮抵消，方向加确认先切页再关闭；不要把raw9的互斥优先器套到36。未见本局部分支写统计桶、资金或人物引用。

父子生命周期与页面动作分开。`FormManagerBase`的Push委托（RVA `0x7F2580`）将新对象设为mode0；Pop指定表单（RVA `0x7E7D60`）经`RunOnUiThread(wait=true)`调用Pop委托（RVA `0x7F29E0`），后者将目标设mode4，并逆序寻找未退休父页，父mode3时转为PREUPDATE1。`RunOnUiThread`（RVA `0x78B100`）在UI线程直接执行委托，非UI线程提交后等待完成；不能将它一概称为异步延迟关闭。`_executeInitAndFinish`（RVA `0x7E9360`）先逆序对mode4调用Finish、仍为4才从列表移除，再正序对mode0调用Init；Pop本身不直接物理删除列表，也不直接调用父世界Update。

框架`_execute`（RVA `0x7E98D0`）的管理器pause门槛、栈末复核、PREUPDATE→UPDATE和实际Update分派已沿[设施升级独立计数](STEAM_FACILITY_UPGRADE.md#两侧人物与独立frame2)局部核查。36处于栈顶时，不因背景可重画就补跑底层主场景经营；关闭后待真实框架准入恢复父页。此结论不把管理器pause与世界模态暂停混为同一字段，也不把渲染FPS、框架执行次数或截图间隔当世界tick。以上未发现需要改变已确认APK9／36业务契约的Steam差异，实际维护接线与测试仍独立记录。

### Steam36的完整局部绘制合同

同日继续从具名`SubForm._draw1_2(Graphics)`（RVA `0x332DB0`）进入真实raw36分支，范围VA `0x10333027–0x10333804`，止于下一raw35分支；没有将截图或相邻页面坐标补入。独立读取结果与前文APK几何表相同，但原Steam字符串与字体消费者单独登记：

| 内容／顺序 | Steam实际请求及与APK的关系 |
| --- | --- |
| 外框→内框→标题箭头 | `DrawWindow(228,176,0,LT("収支情報 <0>/2",page+1))`→`DrawBox(12,69,227,190)`→`Draw_titleBarArrow(228,176)`；参数相同。框、木纹、标题两次测宽与白角复用[Steam窗口合同](STEAM_WINDOW_FRAME.md)，不缩放一张整窗截图 |
| 页签与表头 | 页签原串`月間／年間`在y65，日文x28、非日文x26；收入／支出原串`収入／支出`在y65，日文x121／194，非日文x108／178。非日文仅表头临时`SetFontSize(10)`后`ResetFontSize`，页签在改字号前绘制；坐标与APK相同 |
| 五行与分类线 | 原`RECORD_NAME`依次为`施設、ﾓﾝｽﾀｰ、冒険者、ｼｮｯﾌﾟ、その他`，在(22,90+18×slot)。收入右锚x143、支出右锚x216，anchor4；每行先标签、前四行横线、再收入与支出。横线x22…220、y=rowY+14、lineWidth1，第五行无分类线；同APK |
| 利润与颜色 | 分隔线(22,180)→(220,180)，lineWidth1；标签`利益`在(22,184)，数值(216,184)、anchor4。正文棕(92,51,31)、表头灰(156,155,155)、收入及非负利润蓝(0,100,255)、支出及负利润红(255,14,1)；分类线(199,223,148)、利润线(146,184,247)，独立cctor及实参核值与APK相同 |
| 底部说明 | 原串`月間の収支情報です／年間の収支情報です`交给六参数`Draw_btmMsg(120,202,false,false)`；helper RVA `0x30A5F0`仍调用当前Font测宽，但这两个false跳过底色和手形，只设棕色并于(120,204)以anchor2画文字，无点击组件注册；同APK |

上述原串从实际metadata字符串引用解出；五类别由`UserData.cctor`（RVA `0x2F0040`）在静态`RECORD_NAME +0xB0`发布，顺序不从中文版标签倒推。实际中文／其它语言字形、翻译及测宽沿[Steam字体消费者](STEAM_FONT_CONSUMER.md)，不是把这些日文原串原样显示给所有语言，也不能按字符数估标题宽。

页签、两列表头、五行标签与`利益`标签都实际调用三参数`Graphics.DrawString`（RVA `0x766290`），由该重载传入默认anchor **0x11（LEFT1|TOP16）**；不是页面显式传0。金额与底部说明则调用显式anchor重载（RVA `0x7678C0`），分别传4与2。维护请求中的空anchor表示保留默认重载，不能消费成另一个自拟默认值；字号及基线的后续调整仍沿[字体绘制合同](STEAM_FONT_DRAW.md)。

分类线与利润线实际调用`Graphics.DrawLine`（RVA `0x760D40`）。它在`beginLineGraph_`（实例+0xB8）为false、线宽等于1、且dx或dy为0时调用`FillRect(x1,y1,dx+1,dy+1)`（RVA `0x76A990`）；因此本页正向水平(22,y)→(220,y)在该普通路径是199×1，包含终点。其它情况走四顶点绘制路径，不能无条件把所有DrawLine改成矩形。纯皮肤保留端点及线宽，让后端按明确模式消费，不把本页长度误算为198，也不套用DrawRect的展开规则。

金额在原调用点先将32位结果符号扩展为long，再调用`MyFormBase.YEN("Ｇ",value)`（RVA `0x202140`）。该重载令comma=true，完整helper RVA `0x2019F0`的**Ｇ专支**调用`Unit(value,true)`（RVA `0x201890`），后附全角Ｇ；Unit对绝对值十进制从右每三位插逗号，负值再补`-`。本页不走其它货币的前缀或倍率换算支。月页读当前月，年页分别累加全部12桶，行差与利润仍是32位逐步加减后再扩展，故既有纯统计查询的五类、负数和回卷口径可复用；11个金额均用Font文本，不使用数字SEB。

`Draw_titleBarArrow`（RVA `0x30DB80`）令`q=3*trunc((frame%16)/8)`，本页左锚(22−q,51)、右锚(218+q,51)，经`AddTouch`的SEB重载分别登记id1、value16／18、common SEB3 frame3／0。已有Steam资源交叉确认其为image74 `arrow02`；左frame3源(8,0,4,8)、右frame0源(4,0,4,8)，SEB offset均(0,−3)，不可把逻辑锚当裁片左上，也不把这项注册推成任意平台的物理热区。沿已出版窗框／箭头资源即可，不新增图片副本。

页内位置在父级原点下解释。`SubForm.Draw`（RVA `0x30CE30`）对raw36这类非菜单页面，在画布宽或高超过240时按各轴相对240的半差叠加页面偏移，再`SetOrigin`调用内部绘制；窗框helper自身的VIEW_Y规则仍分别沿既有合同。维护皮肤应保留这些层次，不给所有坐标重复叠同一偏移，不由本次逻辑坐标推断OS客户区／DPI。此局部绘制没有写现金桶、页签或页计数，也未直接调用随机／声音；它会改Graphics颜色／字号状态并注册箭头，不能因此宣称所有深层绘制没有副作用。完整原窗口、实际输入与维护C++接线仍需各自验收。

raw36普通文字、分类线／利润线及`Draw_titleBarArrow`没有额外读取VIEW_Y；底部两个false路径仅把输入y加2。VIEW_Y只交给该页窗框／box helper各自处理，页面原点再由父层统一叠加，不能给表格、底部或箭头重复加VIEW_Y。

### Steam37／38的目录与关闭消费者

同日独立读固定Steam `SubForm.Init`（RVA `0x312460`）、`Update`（`0x321990`）和`_draw1_3`（`0x337600`），不将前文APK目录自动当作Steam合同。raw37初始化分支VA `0x1031365B–0x10313752`按`AppData.itemData_`原序只收`ItemData.haveNum_ +0x64 > 0`，没有额外state过滤或排序；为空则事件15→Pop，不清NEW。普通Update分支`0x10323658–0x103237BA`先上、再下独立循环，调整5行滚动窗，确认或返回时先`UserData.RefreshNewItem`（RVA `0x2E3990`）再Pop；helper遍历全部itemData并清`BaseData.new_ +0x18`，含库存零项，不改库存／state或装备NEW。这与APK既有消费者一致。

**Steam38的铠甲／饰品过滤与APK有实质差异。** Init从VA `0x10312BEC`新建4个目录，读取的是当前定义对象`BaseData.flag_ +0x0C`，不是`state_ +0x10`；不能将flag0解释为“尚未发现”。`ArmourData.Load`（RVA `0x211210`）及`AccessoryData.Load`（`0x210C80`）将原表零起第12列写入该flag字段，但本项不声称运行期所有写入者已穷尽。按固定原表初始定义，目录如下：

| 页签 | Steam实际准入 | 固定原表结果／APK差异 |
| --- | --- | --- |
| 0 武器 | 全部武器，不滤state或flag | 33条，与APK本目录一致 |
| 1 铠甲 | `type_==2 && flag_!=0`；flag守卫VA `0x10312E08` | 17条type2中排除ID45（英文原名Red Armor），剩16；APK不加flag过滤 |
| 2 头／盾 | `type_!=2`，不加flag过滤 | 33条，与APK本目录一致 |
| 3 饰品 | `flag_!=0`；守卫VA `0x10312D08` | 30条中排除ID26／27／28（Bronze／Silver／Gold Medal），剩27；APK全部纳入 |

这些被排定义的flags在两版原表一致，差异位于消费者；维护须以显式版本选择表达，不改原表、不改变APK赠礼或其它已确认目录。残留页签4分支仍被数组长度4和循环条件挡住，不能加第五页。四目录均先按来源顺序收集，再按原“外层向后、内层从末尾向前、后项键严格小于前项才交换”排序；武器键为`older_ +0x50`，防具／饰品为`older_ +0x38`。这与APK交换次序一致，不可替换为稳定排序后宣称所有等键顺序相同。

38更新分支VA `0x1032324B–0x10323657`先独立左／右，只有最终页签改变才清选择与滚动；再独立上／下，当前目录为空则跳过取模；滚动窗4行。确认只Pop，否则软返回2也Pop，均不清装备NEW；空页保留并显示原串`アイテムを所持していません`，不自动退出。37／38另有`kairo.common.cfg.Config.DEBUG_CMD`静态字段+0x83门控的`CheckKeyState(0x40)`确认替代支：普通确认脉冲`0x100000`未命中后，才检查调试开关与该按住状态，随后才到返回软键。实际类型引用与metadata TypeDef551／字段dump已交叉；这是调试命令分支，不增设正常玩家按钮，不据逻辑mask命名PC物理键，也未观察当前运行开关值。

Steam `SubForm.cctor`（RVA `0x3285E0`）独立核出softLabels37／38均为`[0,2]`；上述本地Init／Update／Draw没有把左标签改成7。38虽存在`IsPushSoftLabel(7)`后构造raw73并传槽位／装备定义ID的处理分支，仍不能把字符串或分支存在当作正常玩家可达入口；它不传人物。前文“不伪造人物绑定、不新增情报按钮”的边界在此继续成立。父栈退休、模态和恢复沿同节9／36的真实框架合同。

### Steam37／38的显示字段与资源身份

`_draw1_3`的37分支从VA `0x1033B16C`到`0x1033B658`，38从`0x1033A23D`到`0x1033B16B`。37仍为5行／19单位间距，读取`name_ +0x1C、icon_ +0x2C、haveNum_ +0x64`；底部直接读取选中`explain_ +0x60`并于(120,200)、anchor2画字，不用效果摘要替换静态说明。37使用image147 NEW，38使用image148 GET；这两种标记不可合并。

38每窗4行／24单位间距。仅`state_==1`显示图标、名称、属性和new_对应GET，其余显示灰色`？？？？？`；底部原串`現在 <co=0064FF><0></co> 種類を所持`只计当前目录state1定义数。武器两列读`eq_param_[1]/[3]`，其它两列读`eq_param_[0]/[2]`。**Steam属性值大于0才画SEB12的Draw_plusValue，非正值明确画字面`--`**（两列x146／189、rowY−1）；正值数字锚为x164／207、rowY+1。这里存在第二项版差：APK非正值留空。普通Java及[低层分支](../work/world-page-fallback/GamePage.java:9387)均核，武器L35b→L377、防具L41e→L43a、饰品L4da→L4f6的`<=0`直接跳下一列，第二列`<=0`直接跳L2bf，没有占位文字请求。纯查询用optional表达“无正数”可共用，但消费时APK留空、Steam画`--`，不能悄悄统一。

武器列表明确读取`WeaponData.icon_ +0x20`（VA `0x1033AA5C`），与身体`imgId_ +0x24`不同；现有维护`shop.type`已对应APK的p.d，可作为正式列表图标来源，不需重复新造字段。防具／饰品读各自`icon_ +0x24`。三者交给`Draw_icon`（RVA `0x30C350`）的mode2／3／4：先取`ItemData.ITEMBACK_INDEX`末项铺common24背景，再分别画common12／20／21的18×18十列前景。`ItemData.cctor`（RVA `0x21A610`）的实际fieldRef解码为`[1,4,6,2,3]`，故背景固定为第3格、源(54,0,18,18)，不是未核默认色。

本次重新从固定Steam common TextAsset核身份、解码目录与PNG，确认image12／20／21／24分别为`icon_weapon00、icon_armour00、icon_accessry00、icon_back00`，尺寸180×72、180×90、180×54、144×18；image147／148为20×9的`wnd_new`及23×9的`wnd_get`。这六图均与`assets/original/common`已出版副本逐字节相同，无需另存同图。属性加号的Steam number05仍沿[设施绘制helper](STEAM_FACILITY_DRAW_HELPERS.md)及已出版差异包，不能由上述六图相同推成所有common资源无版差。后继完整局部绘制与维护接线如下，原窗口及文字后端另验。

### Steam37／38的完整局部绘制

2026-10-10继续读取上述两段完整分支，并展开实际helper及触摸下游。正式入口为[steam_information_skin](../prototype/include/dungeon_village_prototype/steam_information_skin.hpp)的`steam_item_information_skin`和`steam_equipment_information_skin`；输入为已初始化Owner与平台实际标题双测宽、语言及`CheckFirstTouch(12,0x40000)`结果。绘制不初始化、清NEW或推进随机／声音／页计数，局部计划不代替原字体与OS输入认证。

两页先`DrawWindow(220,168,0,title)`，再`DrawBox(17,74,219,184)`。37标题原串`所持ｱｲﾃﾑ`，无标题箭头；棕色表头`名前`在(30,69)，`所持`在(日文185／非日文175,69)，普通文字默认anchor0x11，没有显式字号请求。38标题为本地化`装備一覧 <0>/<1>`（tab+1、4），箭头左(26−q,55)／右(214+q,55)，q=3×((frame%16)/8)，SEB3帧3／0，组件1值16／18。

38头部先在(25,66)画common SEB88／image128，合法四页帧1／2／3／4。映射来自`CharacterData.cctor`的`PRESENT_ICON=[1,2,3,4,0]`，不能扩第五页；四帧源矩形分别(50,0,46,18)、(96,0,52,18)、(0,18,53,18)、(53,18,63,18)，offset0。再在(137,67)／(180,67)画SEB98／image37，武器帧1／3、其它页帧0／2；四帧源x=0／27／54／81、y1、27×14，offset0，不能套74的第二行16px属性图。

| 局部原序 | 37持有道具 | 38装备目录 |
| --- | --- | --- |
| 行与选择注册 | 最多5行，rowY=97+19×可见行号 | 最多4行，rowY=97+24×可见行号 |
| 行注册后 | 有NEW先画image147完整20×9，锚(10,rowY+2)；选中手形随后画；再普通道具背景／图标(30,rowY−2) | 未知项先用灰(156,155,155)普通文字`？？？？？`于(30,rowY)，无名称／属性泄露；已知项先装备背景／图标(30,rowY−3) |
| 名称 | 棕(92,51,31)，`TextLayout`矩形(50,rowY−2,130,15)，lineSpace0、anchor0x20；保持当前字号 | 日文普通文字(50,rowY)，默认0x11；非日文临时size11，`TextLayout`矩形(50,rowY−2,85,15)，lineSpace0、anchor0x20，随后恢复字号 |
| 数字与标记 | `Draw_count(210,rowY+1,inventory,SEB12)`：先数字右锚x200，再SEB76 frame2同锚(200,rowY+1)，不能先画单位 | 两列正值依次`Draw_plusValue(164／207,rowY+1,value,12)`；非正值棕色字面`--`在(146／189,rowY−1)。属性后才画GET image148完整23×9于(10,rowY+2)，选中手形最后画 |
| 行后 | 滚动两组件／轨道→选中原说明，棕色普通文字(120,200)、anchor2 | 滚动两组件／轨道→当前整个目录p1数量富文本；空目录仍画棕色提示(120,97)、anchor2，并继续滚动及底部 |

两页每行先注册SLIST组件11，矩形(3,rowY−2,231,16)，value=`0x20000|绝对行号`，`TouchOption.Create()`的flag0，Margin(0,−20,0,0)。选中分支虽设置SC_WINDOW_SELECT=(255,153,55)，**没有FillRect**；`GameView._addTouch`对flag16／128／256／2048才走marker／描框／闪底／触亮，本页flag0均跳过，不画自拟橙色选中底。手形均是common SEB21／image70，锚(21,rowY+8)、当前SEB帧／lineNo−1；未知装备选中仍画，空类无行所以不画。实际6帧中0／2偏移(−8,−5)、3／5偏移(−6,−5)，不由page.frame推固定帧。维护`frame=-1`保留当前帧请求，平台执行时仍需沿原`PushThisFrameRender`调度，不能把纯计划生成当动画已推进。

库存数字的`Draw_count`先调用`DrawNumImageComma`（padding0、comma_padding0、anchor4），再数量单位；库存合法1..999，因此这一域无逗号／负号，可复用已核8步宽普通数字的逐项输出，但不宣称两个原helper算法普遍相同。数量单位SEB76 frame2源image85的(20,0,10,10)，offset0。装备正数沿真实`Draw_plusValue`，复用[正式数字展开器](../prototype/include/dungeon_village_prototype/steam_facility_skin.hpp)，不重新估字体宽度。

38底部原串`現在 <co=0064FF><0></co> 種類を所持`经LT后调用`DrawString2(120,200,anchor2)`，不是普通DrawString。RVA0x7642A0委托TextLayout，再由点重载RVA0x7CA250传width=height=−1、全局行距；基色棕、数字蓝。维护保留`rich_text`与数量参数、extent(−1,−1)、空line_space表示全局值，不能打印原标签或按字符数猜宽。完整tag解析／缩放／最终字形仍未认证，示例的中文翻译和字体适配不能冒充原后端。

两页`DrawVerticalScroll2(221,85,4,110,first_visible,count−1,可见行数)`先登记组件12，再组件25：原矩形均(221,85,3,110)；12的value0x40000、参数[count,5或4,0x20000]、Margin(3,20,0,0)，25的value0、flag4。helper虽只注册，但组件12在`_addTouch`中同步调用`GameView.DrawVerticalScroll`（RVA0x23A720），从表单GetTouchValue取得真实滚动值。type0先画RGB(7,5,78)轨道(220,85,5,110)，令D=max(count,可见行数)、Y=85+trunc(110×first_visible/D)、H=trunc(110×可见行数/D)，再画(220,Y,5,H+1)滑块。默认蓝(48,160,255)，仅实际first-touch且count大于可见行数时橙(246,129,0)；少量／空目录仍有111高滑块，不自行隐藏或截成110。合法Owner滚动边界以外的原异常裁高不作为维护输入契约。

上述窗框／box各自消费VIEW_Y，其余内容／箭头／滚动不额外叠加；父页面origin仍由外层统一安装。新增[Steam数量单位](../assets/steam-common/menuRT01.png)及[装备分类图](../assets/steam-common/icon_objRoots.png)共1901字节，SEB76／88同字节复用既有原包；image9的真实条目是`tresureIcon00.png`，与APK同字节，不能误写不存在的`icon_item00.png`。属性头image37与数字image103沿既有Steam差异包。统一图片路径由`steam_information_image`明确提供，不按同名图擅自跨版本复用；来源索引见[素材入口](../assets/README.md)。
