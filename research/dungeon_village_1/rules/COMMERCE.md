# 商会购买、出售与设施兑换

本页登记固定汉化重签 APK 1.0.8 的静态来源、独立维护契约及下述用户确认修正，输入身份见
[证据索引](../EVIDENCE.md)。异版本截图“阿南商会”不替代本输入“南瓜商会”的价格、库存或操作次序。

## 建筑图纸资格修正（2026-10-10，用户确认）

用户确认：**设施点数购买是一次性图纸解锁，已开放设施不再列入商会；解锁后可另付金币反复建设。** 初始开放的道路18、入住募集24、向日葵66、旅店28、武器店30、防具店31、花店45、面包房35、包子铺33均不应售卖。名称对应用户所称“道路、募集、向日葵、旅店、武器店、防具店、花店、面包店、包子店”。

当前采用的85资格为`p==0 && D!=-1 && D<=当前星级k`，保留原br顺序、星级和售卖资格；目录、付款前重校验、85→74定义预览及目录快照恢复应一致排除p1/p2。道路另有D=-1，本来就没有商会报价资格。85扣村点、93满40领取、普通建设另付金币的时点不变，H不是普通建筑免费券或一次性建造数量。旧进度和历史资金不追溯修改。

后继原代码交叉确认：**原85确为`p!=2`，实际错误首先在维护新局初值。** APK `a/o.a():761`在flags bit1时调用`b():817`写p2／q0，再置r=false／O=true；Steam `TenantData.NewGame`（RVA `221EB0`，VA `10221F0F`）同样写state2。两版原表第35列bit1集合恰为上述九定义。维护生成器曾误输出`bit1?1:0`，现改为`bit1?2:0`，原表不变。Steam85的`SubForm.Init2`分支VA `1030EADF`、比较点`1030EB74`仍排p2；`TenantData.GetTenant`（RVA `2215E0`）与APK93领取均仅旧p0转p2。

因此本批分别保留两层：新局p2是已核原版事实；商会仅p0是用户批准的维护策略收紧，不冒称两版原消费者如此过滤。目录、支付准入、定义预览及恢复采用同一p0资格；93待领取保持p0，领取退休后允许p2。普通建设仍p非0且bit4，仅kind12住宅额外要求并消耗H，普通建筑不消耗H或因兑换免金币。世界语义5／应用8拒绝旧研究档，不在加载时重写既有初值、退款或修改历史。产品复现及独立玩家政策见[商会研究交接](../../../docs/reference/RESEARCH_REQUESTS.md#commerce-building-blueprint)，研究修正不自动迁移产品档。

## 既有静态来源与当前合同

| 入口／状态 | 行为与来源 |
| --- | --- |
| 主动入口 | raw7的商会菜单11要求用户开放位16；首次选择先事件97说明，再开83。脚本直接插入83是另一入口，不套用菜单守卫。见[b/g.java:10574](../work/decompiled/sources/b/g.java)、[11228](../work/decompiled/sources/b/g.java) |
| 83 | 三行依次购买道具、出售道具、购买设施，确认分别开启84的f=0、84的f=1、85；取消只关闭83。见[b/g.java:5745](../work/decompiled/sources/b/g.java) |
| 84初始化 | 购买依原by顺序收集A>0，出售收集z>0；不按p过滤。空购买执行事件13，空出售执行15，再关闭本页。见[b/g.java:3878](../work/decompiled/sources/b/g.java) |
| 84两种展示 | 仅购买有i=0／1两页，分别显示剩余A＋价格u、持有z；切到持有展示仍是购买。五行、上下循环、滚动追随当前选择。见[b/g.java:5772](../work/decompiled/sources/b/g.java)、[绘制7951](../work/decompiled/sources/b/g.java) |
| 购买 | 先以u检查现金，不足执行事件11；有A才减A、调用g.c()增z到999、仅p0时开放p1并清q/置r、以g(u,3)记录金币支出。A耗尽从本页目录移除。见[c/n.java:853](../work/decompiled/sources/c/n.java)、[a/g.java:175](../work/decompiled/sources/a/g.java) |
| 出售 | z>0才减z，归零从目录移除；以f(u/2,3)增加金币与商店类别3收入，不回补商会A。见[c/n.java:1699](../work/decompiled/sources/c/n.java) |
| 交易结果86 | 买卖在84确认中已经提交，86只绑定同一道具及交易方向；其下一次更新直接m()关闭，不再次收费／奖励。固定APK声音A[B]在B=0时为25。见[b/g.java:5825](../work/decompiled/sources/b/g.java)、[5906](../work/decompiled/sources/b/g.java) |
| 反馈 | 若交易后目录仍非空，父84显示“多谢惠顾!”与金额，bv=20；每次84实际更新递减，尚有反馈时确认只清反馈，不能重复交易。见[b/g.java:525](../work/decompiled/sources/b/g.java)、[1428](../work/decompiled/sources/b/g.java) |
| 空目录收尾 | 最后一件购买后事件13，最后一件出售后事件14，关闭84但保留刚插入的86和脚本消息。初始化空出售的15与卖完的14不同 |
| 已读 | 84取消对当时剩余目录逐一写B=true；并非清r。85成功购买或取消对当时完整目录写O=true。B/O只影响NEW提示，不决定购买资格 |
| 85目录 | 当前按上节用户确认：原br顺序，p==0、D!=-1、D<=当前星级k；三行。D为tenant表索引32。原版p!=2见[b/g.java:3906](../work/decompiled/sources/b/g.java)及上节Steam交叉；维护按用户要求收紧，原初始p2本就不应售卖 |
| 85支付 | 价格是设施定义o.s，即tenant索引12，源码复用同字段；检查与扣除的是村子点数，现金不变。不足执行事件12；足够先c(s)扣点并钳制0..999，再开93(r=3,s=设施ID)，关闭85。见[b/g.java:5858](../work/decompiled/sources/b/g.java)、[c/n.java:3455](../work/decompiled/sources/c/n.java) |
| 93领取设施 | 计数恰1请求声音5，提前确认只快进40，满40确认才调用o.c(ID)，H建设资格次数增加1上限99，仅旧p0时b()开放p2、q=0、r=true，然后关闭。H不免除h()造价；不支持取消领取。见[b/g.java:6086](../work/decompiled/sources/b/g.java)、[a/o.java:605及845](../work/decompiled/sources/a/o.java) |

道具价格u来自item表索引19；初始z来自s（索引17），A来自t（索引18），
初始B仅flags bit1为真。设施初始O同样来自其flags bit1；它不是住宅目录M。
出售金币还按[d/a.f](../work/decompiled/sources/d/a.java:3983)维护非完成模式的现金最高纪录；
类别3收入不混入设施／讨伐／冒险者三项最高月收入门槛。

Steam84的Init2实际分支`1030EC06`按原道具目录序，购买只收货源>0、出售只收库存>0，不按state过滤，空目录分别执行13／15并关闭。实际购买在`10319FBA`先用当前价格检查现金，成功才BuyItem；BuyItem RVA`2C93D0`内还要求货源>0，依次减货源、AddItem、耗尽删目录、现金类别3扣款，本体不再查预算。SellItem RVA`2E41E0`要求库存>0，减库存／耗尽删行后按有符号向零半价入类别3，不回补货源。交易完成才建86（`1031A161`），最后一件买完／卖完分别13／14，取消只对剩余目录写已读B；反馈未结束时确认只清反馈，不重复交易。StockShopItem RVA`2EB400`按原序，仅售卖星级非−1且≤当前星级、货源上限减现货源>0才抽Random(3)，补量为`clamp(差额*4/10+票−1,0,差额)`；正补量才增加货源，旧state0才开放，无首次开放库存赠送。首次新开放发17，否则有补货发16，无补货不发提示；补货不等于玩家买入。

Steam85在`10319B9E`检查村点，足额`10319BC5`先扣点，`10319BE7–19C05`再建／Push93/r3并绑定设施ID，随后Pop85，再逐全目录写O已读；取消也写全目录O。不足沿事件12分支保留页，支付本身不调用GetTenant，不增加H／不开放建设。此前维护p0收紧与原版p!=2资格仍分别保留，正常93满40才领取的共用helper已经独立交叉；原Push失败／坏引用不具维护Owner回滚保证。

商会补货与开放位16的时点不同。[战斗成功c/f.java:545](../work/decompiled/sources/c/f.java:545)
在事件128尚未调用时先执行H()补货，再调用事件128，**本分支不立即设置16**；
仅事件128已经调用且16仍未开放的后续战斗成功分支直接设置16。
事件128原程序`6,500&36,0`等待500次有资格续体更新，执行指令36时才设置16，
并插入120对话、83、121对话及提示23，见[d/a.java:922](../work/decompiled/sources/d/a.java:922)。
探索成功不走这条战斗收尾，累计任务成功数大于零不能单独证明商会已经开放。
维护消费者分别见[战斗收尾](../example/src/encounter_lifecycle.cpp:265)和[脚本36](../example/src/world_scripts.cpp:554)。

原85的按钮7另可查看74的**设施定义**详情（原f=1），不是已建实例详情。
当前`inspect`动作通过独立`facility_definition_page_bindings`绑定定义，要求父85存活且当前选择一致；
没有实例绑定或邻接缓存。普通设施只读共享I中的价格／品质／魅力；武器店等显示已开放商品种类。
确认／取消只关闭74返回原85，不扣费、不改NEW、不开放75／79／80；定义预览只有一页。
维护页参数`legacy_f=定义ID,legacy_g=1`属于显式适配，不伪称原Java字段布局。
原93另有r=0..8的领取分支，本批只消费商会实际生成的r=3，其他r显式拒绝。

<a id="steam-commerce-top"></a>

### Steam商会主页83：生命周期、三入口与有序绘制

固定Steam2.56身份沿[交互合同](../ui/STEAM_INTERACTIONS.md#来源与证据等级)。本段只核83及实际调用的局部helper，不把84／86已有业务交叉扩大为三页完整皮肤，也不认证原窗口／字体／OS投递。来源为构造RVA`32EAD0`、公共Init `312460`、Init2 `30E570`、Update2 `317940`及`_draw2_4` `3472B0`；后两者83分派分别在`10317C49–4C`、`10347728–2B`，实际页主体为`1031A37E–1031A54F`与`1034DBD7–1034E384`。

构造在`1032EBD6`置select0。公共Init调用基类、绑定当前UserData、DecideKeyState、清frame，并读该raw的两软标签；83没有专用Init分支或商品目录冻结，转Init2后走公共true尾。SubForm.cctor的`1032A563–A5B8`直接建立两零int数组、只写第二项2，再安装到下标83，因此soft为`{0,2}`。它不在此重新检查菜单开放位16、现金、点数或库存；主动菜单入口的资格与脚本插入83分开。

83的Update先独立处理上repeat（`select=(select+2)%3`），再独立处理下repeat（`select=(select+1)%3`），两者同轮都成立时均消费；随后确认pulse优先于软返回2。普通确认分支不先检查价格／空目录：select0创建84并写value1=0，select1创建84并写value1=1，select2创建85，均`this.Push(child)`，父83保留，不关闭或重置选择。无确认时才沿`1031F640–F680`检查软返回2并Pop83；左右无本页消费者。另有`Config.DEBUG_CMD && keyState0x40`的调试确认旁支，正常产品输入不复制它。原Push返回值未额外判失败，维护Owner的整体回滚不能归给原程序。

83局部Draw先画220×168、top0的普通窗，再Box(17,74,219,184)。标题metadata字面`ﾊﾟﾝﾌﾟｷﾝ商会`；欢迎字面`いらっしゃいませ`用棕色SC_WINDOW_BLACK、普通DrawString(45,70,anchor1)。common38 `wnd_lv`整图锚(181,71)，UserData.nowRank_加1以SEB10数字锚(202,69)、padding0、anchor1显示；这是商会等级展示，不是当前资金。左侧请求`Draw_spCharaClip(g,21,66,imgId2,direct0,18,18)`：helper先push交集clip(21,66,18,18)，按其独立三项SPCHARA数组取图后DrawSeb@(30,94)，末PopClip。cctor在`1032DC4A–DCC7`安装的SPCHARA_SEB为`[31,31,38]`、IMG为`[172,172,173]`，对应字段默认值metadata offset`2FB8D9`／`2FAF96`；故此处确为common38／173南瓜人物，不能套`Draw_spChara`的另一套公共ID2大臣映射。实际frame0源裁(18,0,18,28)、offset(−9,−28)，只显示上部18高。

三行令Y=`101+26*row`，每行原序为底图→Touch→文字→可选NEW→类别图标→选中手形：

| 具名消费者 | 原实参及限制 |
| --- | --- |
| 底图／触摸 | common19 `bar_middleWindow`在(53,Y)，选中裁(0,0,134,26)，其余裁(0,26,134,26)，两裁均在原134×52 PNG内。组件11基矩形(33,Y,174,26)，value=`0x20000\|row`，无marker16、附加Margin或方向页签，不套75／79热区。 |
| 三名称 | 字面依次`ｱｲﾃﾑの購入／ｱｲﾃﾑの売却／施設の購入`。日文普通DrawString(127,Y+7,anchor2)；非日文临时字号11，TextLayout(99,Y+7,85,11,anchor32,lineSpace0)，随后ResetFontSize，正文棕色。 |
| NEW | 仅row0调用IsExistShopSeeItem（RVA`2DFFD0`）：原序任一道具restNum_>0且isShopSee为false即成立；row1不查询／不画；仅row2调用IsExistShopSeeTenant（`2E0090`）：state!=2、售卖星级非−1且≤当前星级、isShopSee为false。成立画common147 `wnd_new`整图@(164,Y+14)。原设施提示谓词不收紧为维护85的p0目录资格。 |
| 类别／手形 | common2 SEB1／image2 `icon_shopTop`取frame=row，日文锚(77,Y+4)、非日文(75,Y+4)；实际三裁片为(16*row,0,16,17)、offset0。选中行随后直接画common SEB21 `finger_r`，日文锚(68,Y+13)、非日文(66,Y+13)，不在本页强制改其当前帧。 |

共尾重设棕色，在(120,198,anchor2)画当前行说明，原序字面为`アイテムを購入します／アイテムを売却します／施設を購入します`。正文没有底栏按钮、标题箭头、滚动条、价格或购买付款；三项始终可选，NEW不是资格门控。文字槽依次为title `110FC3AC`、welcome `110F09A4`、三名称`110FB7E0／110FB7B4／110F7558`、三说明`110F21BC／110F2168／110F7580`，按真实metadata usage5解得；当前语言转换／字体度量仍由文字后端消费，不以这些日文字面认证中文显示。

上述common19／38／147／173、SEB21／38及common2的图2／SEB1均已有原样出版；IMAGE_COVERAGE对Steam对应具名PNG／SEB登记与固定APK字节相同。同图由具名Steam实际调用认证；维护83纯计划已消费这些原出版资源，完整原窗口仍未观察，不新增整包素材副本。

现Owner已持久化`commerce_pages_initialized`、`commerce_page_data[id]`及`page_counters[id]`；83六槽为`{mode0,tab0,selection,first0,binding−1,feedback0}`，没有commerce_page_lists。已有inspect输出selection/counter，三子页、取消、保留父选择、严格拒绝及退休可复用，不需要为83再造schema。现已新增[独立纯皮肤](../prototype/include/dungeon_village_prototype/steam_commerce_skin.hpp)及`input_startup_world_commerce_top`：设备选择先安装，再上→下→确认优先返回，子页插入与选择在同一候选提交，失败回滚；Session／应用／研究窗口均接此入口，未初始化或缺字段明确拒绝。研究窗口按源无marker的行作松开选择并确认，键盘用现离散按键适配；不认证原repeat及OS投递。Shop NEW可直接读取已有货源／item_commerce_read、facility_presence／facility_commerce_read及星级，现`steam_commerce_input`保持上文原谓词和短路消费，不用85维护策略替换提示资格；只读查询不初始化、清NEW或抽随机。研究文字／Noto和显式静态手形帧0是适配，共享原手形调度未接。84主体下一入口为Update2 `10319CA9`、Draw `1034D218`；86为Update2 `10319895`、Draw `1034C86F`，本段不提前认证其完整表现。

<a id="steam-commerce-items"></a>

### Steam买卖目录84：两展示页、反馈时序与版差

固定Steam2.56的Init2／Update2／`_draw2_4`方法索引与上节相同；本次直接从原DLL方法入口按指令边界核84局部，范围分别为`1030EC06–1030EE00`、`10319CA9–1031A37D`、`1034D218–1034DBD6`，未操作游戏。mode来自value1_，0购买／1出售；不是展示页page_。构造初选0、公共Init清frame及装软标签，cctor在`1032A5BD–A612`安装84两软标签`{0,2}`。Init2新建ary1，沿AppData.itemData原序仅按购买restNum_>0／出售haveNum_>0收集，不读state；空目录分别事件13／15后Pop，非空才SetHelpMsgInit(mode)。

普通Update先调用UpdateHelpMsg(mode)，随后独立上→下循环选择；仅mode0再独立左→右各翻一次两页，mode1不读取左右。之后只修首行使selection落在五行窗内，再确认优先软返回2。确认有同83的DEBUG_CMD旁支，正常输入不接；无确认的返回逐剩余ary1写isShopSee=true才Pop，不清定义NEW。正反馈在这一轮开始先减1：减后仍>0的确认只清反馈并SetHelpMsgInit、返回false，不交易；原counter1的这一轮可在减到0后真正交易，不能在前一轮快照上先判“有反馈”阻挡它。方向／展示翻页不因正反馈屏蔽。

真正交易仍沿前文BuyItem／SellItem。买入先ChPayMoney(price)不足事件11并保留页；mode0调用BuyItem，mode1调用SellItem。交易后若所选索引≥新count归0，先修五行可见窗，再把过高first夹到max(count−5,0)、必要时夹selection；**只有这一交易后段显式夹新目录尾部**，不能把这段误写成所有普通方向输入的步骤。随后绑定本次ItemData、mode创建并Push86，再PlaySe(`AppData.SHOP_SE[shopSeIndex]`)；声音不是86再次发。若目录空，买完／卖完分别事件13／14后Pop84，已创建86不被抹掉；目录非空才SetHelpMsg(`まいどあり!`,本次买价或向零半卖价)。正常成功次序不证明原异常／Push失败的事务安全，维护Owner仍独立回滚。

84 Draw外框220×168、top0，Box(17,74,219,184)。购买标题用LT(`ﾊﾟﾝﾌﾟｷﾝ商会 <0>/2`,page+1)且调用Draw_titleBarArrow(220,168)；出售标题只是商会名，无序号／箭头。箭头复用已核helper：q=3×trunc((frame%16)/8)、左(26−q,55)／右(214+q,55)、SEB3帧3／0、组件1值16／18，不增加独立持久时钟。表头／行／底栏均沿棕色SC_WINDOW_BLACK，只有选中底沿SC_SELECT。

| 具名绘制／输入 | Steam实际消费者；APK差异单列 |
| --- | --- |
| 表头 | 名称(30,69)普通字；购买page0“残り”始终TextLayout(137,69,40,13,anchor32,lineSpace0)，价格JP185／非JP179；购买page1仅“所持”JP185／非JP175；出售“所持”JP137／非JP128，价格JP185／非JP179。此处无额外临时字号。APK买剩余普通字x134、卖持有JP134／非JP125，不套其位置。 |
| 五行原序 | 从first最多五行，Y=97+19×(absolute−first)。选中底FillRect(23,Y−2,191,16)，先底→Touch→NEW→手形→道具图标→名称→数量／价格。此页NEW读isShopSee而非new_；false就画common147@(10,Y+2)，不因state或买卖mode隐藏。手形common SEB21当前帧@(21,Y+8)；图标helper的type1指普通道具，两种买卖模式均画两层图标@(30,Y−2)，读取ItemData.icon_，见[共享图标](../ui/ITEM_ICON_RENDER.md)。 |
| 行Touch | SLIST组件11、base(23,Y−2,191,16)、value=`0x20000\|absolute`，TouchOption flag16及Margin(29,0,0,0)，实参顺序为left／right／top／bottom。APK同处Margin(29,29,0,0)。保留首次仅标记资格，不能照79或83取消marker，也不将base＋margin当最终OS实测热区。 |
| 名称 | JP普通DrawString(50,Y)。非JP是继承当前字号的TextLayout(50,Y−3,85,17,anchor32,lineSpace0)，本分支未SetFontSize。APK则临时字号10、普通字(50,Y+1)，两者不能合并为同一消费者。 |
| 数量／钱 | 购买page0：restNum_用Draw_count(161,Y+1,SEB12)，售价Draw_money(210,Y+1)足额15／不足19；购买page1：只读haveNum_并Draw_count(210,Y+1,SEB12)，不改交易方向或跳过付款。出售：haveNum_在161，向零半价金额在210、SEB15，不检查购买预算。APK第一数量锚158。count先Comma右锚dx−10再SEB76/frame2单位；买入持有展示的0合法，不能复用只收1..999的正库存接口而改成1。 |
| 滚动／正常底栏 | 原DrawVerticalScroll2(221,85,4,110,first,count−1,5)，此页110高，不套79的111。无正反馈时对当前所选ItemData调用Draw_itemExplain(g,item,120,199)，不是description／价格拼句。现[普通道具说明计划](../prototype/include/dungeon_village_prototype/steam_human_skin.hpp)同锚可复用其已证效果分类及实际测宽；完整字体后端仍另验。 |

正反馈底栏不读取当前商品价：原静态SubForm三字段shopMode_helpMsg／Cnt／Money位于`+158／+15C／+160`。SetHelpMsg（RVA`316760`）保存传入文本／金额并置Cnt20；SetHelpMsgInit（`3166D0`）只取SHOPMODE_HELPMSGS[mode]、置Cnt0，**不清Money**；UpdateHelpMsg（`320AF0`）只减正Cnt，到0才恢复默认。Draw在Cnt>0时先棕色原文本(54,199,anchor1)，再Draw_spCharaClip(g,30,195,imgId2,direct0,18,18)；mode0冻结金额以SEB19 Draw_minusValue(201,200)后同SEB19／image109 frame20单位，mode1以SEB12 Draw_plusValue后同SEB12／image103 frame20单位。原序数字／正负号→单位，不把钱数画成普通文字或重新交易。20是获准页面更新次数，非秒数；这些原全局静态字段不能被描述为原文件必保存的页面载荷。

文字槽按usage5独立核：购买标题`110FC3D8`、名称`110F5D68`、持有`110F6E48`、价格`110F4CB4`、剩余`110F7FD8`、交易反馈`110F1CA4`；对应字面见上文。当前语言转换、真正字体度量、继承字号／SEB当前帧、所有TextLayout排版及触控到OS的投递不由这批机器码／PNG关系证明。资源均沿现出版common图标、数字／单位、手形、箭头和南瓜肖像，不新复制图集；本次只交静态合同，没有新增84研究窗口认证。

现Owner的`commerce_page_data[id]={mode,tab,selection,first,binding−1,feedbackCounter}`、`commerce_page_lists[id]`原序ID、初始化集合及page_counters已持久；库存／货源／已读、现金与月桶使用唯一消费者。2026-10-16批准的`commerce_page_feedback_amount`冻结实际成交价，86覆盖保留，反馈到0／确认清除及Finish同事务释放；正反馈缺金额、负值、错页、孤立键以及关闭待退休缺键均拒绝，旧布局不迁移，见[维护存取](../prototype/PERSISTENCE.md#商会84交易反馈金额设计待确认)。

完整整轮入口现为`StartupCommerceItemInput`，仅当轮借用，必须绑定当前已Init生命周期2的唯一84；通过Runtime／Session／Application的既有Update统一消费，先主钟推进及反馈递减，再滚动／设备选择、上→下、买入左→右、五行追随、确认优先返回。反馈1可当轮再次成交，2仅清反馈；卖出忽略左右。旧逐action保留兼容，不冒充这条源调度。所有迟发失败沿完整候选回滚，包含系统现金纪录发布失败；不增加Owner／codec字段或持久“已减”标记。84恢复允许原合法独立尾首行，85三行边界保持。

正式`steam_commerce_item_skin`读取同一Owner和当前语言标题／表头、真实测宽选项；交付五行、单侧marker16、原数量／金额、两个买入展示页／出售、实际选中说明及冻结成交反馈的有序计划。Steam尾部窄核`1034DAAA–DAB6`／`1034DB45–DB51`及`1034DBB4–DBC8`确认正负金额和frame20单位均锚`(201,200)`，没有86式的单位x+1。图标type1对买卖均适用。正常说明复用既有中文维护helper，不认证所有语言翻译或字体后端；研究窗口短期合并设备脉冲到下一获准轮，切页／暂停／失焦释放，不持第二个业务游标，也不认证原OS仲裁。参考见[普通目录](../ui/examples/steam-commerce-items.png)及[实际反馈](../ui/examples/steam-commerce-items-feedback.png)。

<a id="steam-commerce-facilities"></a>

### Steam设施购买85：三行预览、详情与两个提交时点

来源仍为固定Steam2.56：Init2局部`1030EADF–1030EC05`、Update2 `103198B1–10319CA8`、`_draw2_4` `1034CBC9–1034D217`。Init按TenantData原序收`state!=2 && saleRank!=-1 && saleRank<=UserData.nowRank_`，空目录执行13后Pop；它不预筛足额或isShopSee。构造初选0、公共Init软标签读取沿83，cctor `1032A617–A67D`独立安装85的`{7,2}`。**原p!=2与用户批准的维护p0目录收紧分开**，皮肤查询只消费已冻结目录，不能再按另一种资格重新排列表。

Update独立上→下循环选择，再修first使selection落在三行窗内，无左右；随后确认优先返回2，返回优先详情7，调试确认仍只属DEBUG_CMD。详情在`10319A26–AAF`构造74、value1=1、tenantData绑定当前定义，再`this.Push`，不付点、不写已读、不绑定地图实例或清父85选择。确认先ChPayTownPoint(cost)，不足事件12并保留85；足额先SubTownPoint，再建93/r3、奖励ID取当前定义，Push93→Pop85→逐旧完整目录写isShopSee=true。返回也逐完整目录写已读后Pop；详情不写。领取仍由93满40另一次确认，85支付不调用GetTenant、不增加H或自动建设，原失败不具有维护Owner原子保证。

Draw先220×168、top0窗与Box(17,74,219,184)，标题槽`110F7558`字面`施設の購入`。棕色设施头`110F7484`字面`施設`在(51,69)；价格先LT槽`110F4CB4`，JP(162,69)／非JP(158,69)，再用**当前真实Font**的StringWidth得到W，common SEB44／image31 frame4锚(166+W,67)显示点数单位，两语言均用166+W而非各表头x加4。此资源须沿已出版Steam `icon_result00`，不能因SEB相同借APK不同像素。

三行令Y=`89+34*(absolute−first)`，每行有序请求如下：

| 顺序／职责 | Steam实参与APK差异 |
| --- | --- |
| 建筑图块 | RGB(0,255,128) DrawRect(21,Y,89,34)，端点语义转半开90×35；push交集clip(22,Y+1,88,33)→DrawMapchip2(65,Y+11,定义mapchip,orientation0)→PopClip。不是普通Mapchip、当前实例朝向或绿色填底。 |
| NEW／右框 | isShopSee=false才画common147@(22,Y+25)，不读new_。右绿DrawRect(110,Y,105,34)转半开106×35；选中才填SC_SELECT矩形(111,Y+1,104,33)。本页没有手形或标题箭头。 |
| 行Touch／名称 | Touch11 base(110,Y,106,35)、value=`0x20000\|absolute`、marker16、Margin(29,0,0,0)；不是图块全行热区。棕色名称所有语言均TextLayout(116,Y+5,90,11,anchor32,lineSpace0)，继承字号。APK同锚普通DrawString、Margin左右各29；不得复制其字形策略或右边距。 |
| 点价 | 当前村点足额用SEB20／image108、不足用SEB19／image109，DrawNumImage(cost,206,Y+20,padding0,anchor4)。读定义cost字段+48，不用金币造价或每行加G／点数单位；选择与不足颜色独立，零价合法。 |
| 滚动／说明 | DrawVerticalScroll2(221,85,4,110,first,count−1,3)，110高不套79的111。末棕色槽`110F93A8`字面`街ポイントで購入できます。`在(45,199,anchor1)，再Draw_spCharaClip(g,21,195,id2,direct0,18,18)，复用上文南瓜私有数组和clip，不创建购买按钮。 |

当前Owner已有85的六槽`{0,0,selection,first,−1,0}`、原序`commerce_page_lists`、初始化集合／计数、村点及设施已读；定义预览74用独立绑定，93领取和退休已有消费者，皮肤／整轮输入无需新持久字段。Mapchip2、点数字、Steam单位、NEW和肖像资源均可复用已出版接口；绘制桥必须提供实际价格头测宽，并严格读取当前定义／点数，不补估宽或重查商品资格。局部静态与APK普通文本交叉不冒称自然兑换、原窗口或产品验收。

维护已在同一商会模块接`steam_commerce_facility_skin`和`input_startup_world_commerce_facility`。设备滚动／选择后按上→下→修窗→确认优先返回优先详情消费；既有单动作与新整轮入口复用同一候选内付款／已读逻辑，失败保留选择、点数、目录和子页。Session／应用／窗口接线，鼠标marker16保留同页同绝对行的第二次松开资格，暂停／失焦／切页／拖动清理手势；它不入存档，不替代Owner选择。窗口沿现PC离散键与拖条适配，原OS投递仍未认证。原序地图块、Steam单位、价格测宽及[三态参考](../ui/examples/steam-commerce-facilities.png)已消费；74普通定义预览及其它特殊模板的完整皮肤仍是后继，不能由85交付升级为已完成。

<a id="steam-commerce-result"></a>
### Steam交易反馈86：已付款后的自动关闭页

Steam `Update2`分派`10317C64–67`到`10319895`，专属处理仅调用`FormBase.Pop`（RVA`7E3A20`）后返回，没有按键门槛、数量、付款或父答案回传；APK `b/g.java:5906`同为下一获准更新直接`m()`。普通Init转Init2，后者没有86专属分支，走`10312333`默认true尾；cctor `1032A682–A6BE`安装两零软标签，故为`{0,0}`。84先在`1031A013／10319FF4`完成买／卖，修目录后才建86、绑定原道具和mode（`1031A161–18A`），声音随后由84请求；86不重复发声或交易。

`_draw2_4`的`1034C86F–1034CBC8`绘170×125普通窗、top0，标题字面`ショップ`，Box端点(42,95,197,162)。棕色RGB(92,51,31)表头(47,91)虽写`所持金`，右侧**实际消费本笔交易差额，不是余额**：mode1将道具原价向零除2，以SEB12 plusValue@(184,92)，同SEB／图103另画frame20@(185,92)；mode0用原价、SEB19 minusValue及图109的同坐标frame20。没有乘数量。

正文先实测当前Font的道具名宽W，令H=trunc((W+20)/2)，不夹宽；Draw_icon(mode1,真实iconIndex)锚(116−H,119)，名称普通文字@(136−H,122)。尾字居中(120,146,anchor2)，分别为`を売却しました／を購入しました`。底部`Draw_btmMsg('了解',175,true,true,200)`复用现帮助器：宽=min(实际字宽,200)+16，left=120−trunc(宽/2)，橙底(left,175,宽,16)，Touch4／value20=(left−20,155,宽+40,56)，文字anchor34，手形SEB21当前帧@(left−4,184)。**画出“了解”并注册Touch不改变下一获准更新自动Pop的消费者**，不能据截图新增等待确认。APK `b/g.java:8112`几何同构，译文与Steam字面仍分开。

现Owner的mode／道具绑定／page_counter及真实定义名、icon、price足够提供本页只读输入，不新增数量或父答案字段。86是否实际被绘制仍受原框架调度影响；本次只有局部静态交叉，没有原窗口动态。维护84冻结反馈金额的缺口独立，不能借86临时绑定在其退休后继续猜金额；源异常非事务，唯一Owner回滚属于维护保证。

维护`steam_commerce_result_skin`已在同一[商会皮肤模块](../prototype/include/dungeon_village_prototype/steam_commerce_skin.hpp)接严格86只读桥，实际测宽由后端提供；研究窗口消费完整原序计划，保留原自动关闭消费者。正文图标复用真实道具定义图标及类别底图，数字与单位分别按原SEB消费，不新增素材／字段／等待状态。[两态参考](../ui/examples/steam-commerce-result.png)通过显式条件下的真实83→84交易产生，冻结86只为取图，不能据8帧重复绘制认定原页有8帧寿命。坏载荷、缺测宽、错绑定或错误附属目录显式拒绝；原字体与物理输入未认证。

### 大炮店与未开放装备的有限排除

固定两版原表中，设施58大炮店的售卖星级D为−1、flags为131172（不含初始bit1）；因此新局不开放，且**升到任意星级都不会使它进入85商会目录**。Steam `SubForm.Init2`（RVA `0x30E570`）在VA `0x1030EB74`排state2、`0x1030EB7D–0x1030EB80`独立排D=−1，之后才比较当前星级。这是普通商会入口的排除证明，不是全部玩法绝对不可达证明；维护p0收紧也不改变这一结果。

设施58自己的表列33程序`6,100&40,58`只是等候后请求82的设施演出：APK `d/a.java:946`的opcode40按设施种类2／3创建82并绑定定义，不开放目录、不增加建设资格。不能因为程序参数出现58就计为获得58的上游。配方38指向58但没有普通制造目录资格，见[壶目录的Steam交叉](MAGIC_POT.md#steam配方目录与奖励消费者交叉)。

Steam直接调用复核补充了消费者范围：`TenantData.ChangeState`（RVA `0x220280`）的已核直接调用点为脚本执行器VA `0x1025677A`，传state2；`TenantData.GetTenant`（RVA `0x2215E0`）的两个直接调用点在`SubForm.Update2`的`0x1031825A／0x1031854B`，使用页面绑定的定义ID。后者先把`haveNum_ +0x88`加1并封顶99，只有旧state0才写state2／NEW并清计数；不在helper内选择某个新定义。因此获得页绑定的来源仍须逐项证明，不能用helper存在填平58缺口。

防具45血红护甲及饰品26／27／28三种奖章仍无正常正向获取来源。它们的flags0同时排除初始bit1和宝物bit2，既有事件／人气／人物程序及配方结果也未指向它们；年度授勋的名称不能代替装备定义引用。Steam `ArmourData.ChangeState`（RVA `0x211130`）直接调用点为`ItemObj.Proc_getTreasure2`的`0x102A7765`、`Proc_getTreasure`的`0x102A79A9`、`SubForm.Update2`的`0x10318457`和`SubForm.Update`的`0x103220DB`；饰品对应RVA `0x210B60`及`0x102A770B／0x102A793D／0x103183FF／0x1032208B`。两helper均只在旧state0且新state1时给共享库存1／NEW并通知匹配商店，随后写state；它们本身不另挑奖励ID。普通宝物、奖励页与壶三类调用未暴露额外的专属获得入口。

年度授勋与通关奖杯另作有限交叉，不能因为译名同为奖章就补这三件装备来源。APK raw87真正父确认调用`c/n.a(e,...,true):492–515`：人物E加1；Steam `SubForm.Update2`在`0x1031960C`减少全局待授勋`UserData.medalNum_ +0x2C`，`0x10319667`调用`SetMedalParam`（RVA `0x2E8420`），helper只在授勋参数为真时于`0x102E8717`增人物`medalNum_ +0x7C`，随后建立raw88演出。所核分支不修改人物`equipDataId_ +0x58`、饰品状态或库存；事件58的`6,3&22,0`只延迟结算人气。人物E因此确为已授勋计数，与全局待授勋j、道具取得计数或饰品定义ID分开。

通关系统奖杯是`J.int[10]`的1..4档：Steam `Update_clearPoint`（RVA `0x321190`）在`0x1032150F/43/74/A5`写该项，事件4／5／6只有对话，没有装备奖励指令；APK同义尾段另保存最高分与村名。控制18和显示18是表情／表现请求，不给装备。以上只排除这些具体正常消费者作为四件缺口装备的授予者；它们究竟是未启用残留还是另有未定位入口，仍不能据此定论。

上述直接调用数来自固定GameAssembly的可执行节内`E8 rel32`目标筛查，再从具名方法入口解码确认指令边界；输入身份沿[内容来源](PROGRESSION_CONTENT.md#apk与exe表格交叉)。它不覆盖虚分派、间接调用、内联赋值或任意旧档输入，不能包装成全程序写点穷尽证明。[加载尾声](PERSISTENCE.md#加载尾声的目录修复)另有直接状态修复；四件装备均不在原初装中，该修复也不是它们的正常奖励来源。逐ID验收应保留这些明确缺口，不注入开放位来制造自然路线。

进一步核对metadata的字段归属：四类定义继承`BaseData`的`id_ +0x08／flag_ +0x0C／state_ +0x10／stateCnt_ +0x14／new_ +0x18`；防具／饰品`haveNum_ +0x3C`及设施`haveNum_ +0x88`是不同派生类字段，不能把世界实体的同偏移或别名helper混算为目录写点。Steam新局确有**内联开放赋值**：`AppData.NewGame`在`0x1025F37D`调用`UserData.NewGame`，后者遍历各目录，在`0x102E051F／0x102E05AF／0x102E0B9F`分别调用饰品／防具／设施NewGame。饰品`0x10210E58–0x10210E63`、防具`0x102113E8–0x102113F3`先清state／NEW／库存，只在flag1检查通过后直接赋state1／库存1，最后清NEW；设施仍沿已核flag1→state2。故四件flag0装备从这个正常新局入口不能开放或得到库存，不以先前直接call数量作为理由。

原脚本支持动态参数，不能只搜奖励ID字面：APK `d/a.java:659`及Steam `ExecuteScript`的`0x10255C25–0x10255C3C`对参数≤`INT_MIN+99`执行实参替换；随后26／27在`0x102569AF／0x102569E0`写奖励种类6／7，并经`0x10256947–0x1025694C`把已替换的参数1直接绑定到页面。复核固定event列4、人气列5、人物列9／10、设施列33的所有20／24／26／27／34指令：均无这种占位参数；设施目标只有43／55／59／61／62／63／65，防具21／36／40，饰品1／18／25。两版这些数字列已交叉相同，因此**所核原表程序入口**没有动态补出58、45或26–28的途径；此范围不包含未证明来源的运行时代码新造程序数组。

反过来，加载是已确认的额外直接写入路径，不能被正常入口排除掩盖：饰品`Deserialize`在`0x10210C18／0x10210C2F`、防具在`0x102111E8／0x102111FF`分别直接安装short读取的state／库存；设施在`0x10220339／0x1022036A`安装state／H。这些方法不检查flag，也不调用领取helper；因此它们可保留档中已有状态，却不证明该状态由本版正常游戏取得。全程序间接／内联写点、旧格式／外部状态的产生来源仍未穷尽，本批不把它们升级为自然路线，也不修改维护恢复策略。

从已核原初装出发，玩家赠礼和自主采购也不会首次引入上述四件装备：64目录要求p非零；自主采购的新候选同样排state0，锁定或空候选只能保留当前装备，防具无当前时唯一字面回退为ID0。Steam `WeaponData.GetBuyItemDataId`（RVA `0x2291F0`）已独立核到饰品／防具的state0排除及该回退；具体选择次序见[设施采购](FACILITY_USE.md#防具饰品选择与收费前置)。装备setter只安装传入ID、重算并设置重选锁，不另开放目录。因此这是正常初装与已核获取渠道内的闭包；若外部档已装四件之一，采购仍可能保留它，不能写成任意状态均禁止。

奖励载荷亦不凭页面或物体类型新增ID：已定位raw93产生者只绑定设施r3，装备94来自脚本26／27的原参数，大师94固定职业r4；通用93／95有装备处理分支不证明存在对应正常产生者。探索直接领取与抛出物原样沿任务记录的类型／ID，地面物体保存这个绑定后才grant；任务记录中`[5]--`属于怪物挑战分支，不是把装备ID递减成隐藏物品。Steam抛物注册`RegistDropItem2_parabola`也直接保存输入类型／ID。以上排除的是已核普通生产链，不能据此将四件定义认定为全程序死内容；任意运行时代码造程序、未定位赋装和外部档来源仍另列未知。

## 维护实现与验证边界

[商会Owner模块](../prototype/src/startup_world_commerce.cpp)组合已经维护的脚本页栈和唯一现金账本；
API见[接口](../prototype/include/dungeon_village_prototype/startup_world_commerce.hpp)。
这不是复制反编译实现：原表是只读输入，业务状态是稳定定义ID与原始页面参数的显式投影。

金币、村子点数、商会剩余A、持有z、定义p/q/r、B/O已读各自保存语义；
84操作整候选成功才写回，现金台账／月统计／库存修改之后插页失败也不残留部分交易。
85扣点与93领取是两个真实操作时点，不能因为本轮需要事务而合成一次确认。
93本轮领取失败回滚该次领取，不回滚先前已经成功确认的85付款。
新页必须由框架初始化；已初始化后的目录、选择、绑定或计数缺失／越界明确拒绝，不用map.at异常掩盖载荷错误。

新覆盖扩展既有pages套件：真实初值出售、最小补货夹具购买、两种货币、反馈拦截、空目录不同事件、
库存上限、NEW清除、暂停／过期页／缺载荷、现金和页ID耗尽回滚、93两个时点、页载荷退休与输出消费。
条件组合夹具不宣称自然玩家路线已经完成；当前阶段实际结果统一记入[验证记录](../VERIFICATION.md)。

研究窗口商会快捷键S遵循开放位16，83/84/85/93输入调用同一Owner；86与设施76自行推进。
有界检查`--world --inspect-page world-commerce`从真实新局接受任务、出发与领取成果，
实际任务成功、无当前任务、开放位16已开且真实补货中有可负担道具时主动进入83，
也允许先到达自然脚本83；开放后无可负担货物时继续经营等月度补货，不反复接任务。
开放位16尚未出现时，策略继续真实任务；不能在一次探索成功后就停止接任务并假定已开放。
原事件128需要上述500次有资格主场景续体更新；旧窗口诊断遗漏了已经开放后主动进入商会的路径。
诊断停在主场景只能说明该玩家策略未到目标，不据此推定补货失效、16已开放或首次战斗已经完成。
这些来源合同与自然策略的实际运行结果分别记录。
`world-item-gift`继续真实购买一件可负担的补货道具，
返回人物64第五页。预运行最多20000次框架调用，逐轮领取声音输出；不注入资金、库存、日期或开放位。
若途中跨年，诊断玩家在87明确请求结束授勋，再确认实际生成的终止问答；
不授予、不清空剩余勋章，也不使用通用确认跳过授勋。遇48条件页明确返回，不在商会诊断中申请晋级。
这是本窗口检查的显式玩家输入策略，不能视为原版自动结束年度或自然授勋验收。
该策略不认证完整原皮肤、固定APK动态或操作系统输入。

<a id="设施商品79来源已核维护消费者待接"></a>
## 设施商品79：维护消费者已接

固定APK低层`GamePage`的79初始化／更新与普通输出交叉，证据及源身份见[本批记录](../work/commodity-effects/README.md)。
这是设施装备商品目录，不复用84的买卖事务。

| 时点 | 原版合同 |
| --- | --- |
| 打开 | 74的装备活动类别`g=1/4/5`确认创建79，保留同类别`f`；不扣款、不分配装备 |
| 目录 | 只列本类别`p!=0`的定义；武器按`u`升序，防具／饰品按`j`升序；原交换遍历不是稳定排序，不能追加等值保留原序保证 |
| 输入 | 上下循环选择、四行滚动，左右切两页；查看信息键7创建72，绑定原装备ID，类别1→0、4→1、5→3 |
| 关闭 | 确认及返回都调用`n.m(f)`，清该类别全部定义的`r`提示，再关闭79；不是只清选中项 |

显示价格、属性、图标的既有来源仍见[UI合同](../ui/PAGES.md)，完整原皮肤／OS输入没有由本批静态核对认证。
72是独立只读信息子页，不能用73人物道具页代替或借商品确认发购买奖励。

维护方案复用唯一Owner已有`catalog`和设施类别，持久记录本页目录、选择／滚动、页数与72绑定；
初始化成功却缺载荷、错父页、目录为空、越界选择及过期身份须显式拒绝。
新增字段同步codec清单、恢复引用校验与往返；本页和子页退休同时清附属引用，不能靠`map.at`异常兜底。
主责扩展既有building／pages套件，验证原交换遍历所得顺序及等值风险、四行边界、两种关闭清全部r、信息返回、无资金／库存／随机变化及失败不留半页。
本批沿已确认合同接入[设施目录模块](../prototype/include/dungeon_village_prototype/startup_world_facility_catalog.hpp)、
真实74→79、79→72与Session／框架／简化研究窗口。价格页和属性页分开，72浏览不改变父79选择，不清提示。
冻结目录、选择／首行、父72→79及初始化标记均进入codec；错父页必须拒绝，不能指向任意远处存活79。
原滚动只追随选择，合法末页可不足四行，不添加`size-4`夹限。目录保持原非稳定交换遍历。
关闭帮助器只处理生命周期，维护Owner在候选清提示后仍验证完整脚本页栈；失败保留全类别r及原页。
关闭当下仅标lifecycle4，下一框架入口才清附属字段；恢复可以保留这个合法过渡，不能把错页型载荷当合法历史。
实际检查见[验证入口](../VERIFICATION.md)；两个自然新局有界窗口分别进入79和72，没有原APK动态／OS输入／完整图标皮肤认证。

### Steam商品79／详情72只读皮肤

2026-10-13沿固定Steam2.56具名`SubForm._draw2_3`（RVA342C30）的79分支VA1034339C–10343DBA核完整局部：220×168窗、内框(17,74,219,184)，始终双箭头；名称／价格表头(30,69)/(193,69)。四行从y98起间距24，原SLIST11矩形(1,y−2,233,16)及Margin(0,−20,0,0)，没有marker16二次点击资格；NEW用common148，图标锚(30,y−3)，选中手形(19,y+8)。非日文名称字号10并下移1，原直接DrawString而非猜宽裁剪。售价用SEB15金额右锚(211,y+1)；第二页武器显示战斗槽1／3，防具／饰品槽0／2，只有正值绘SEB12加号数，右锚160／206。底部类别／定义数量为只读说明，确认不是购买。

滚动调用VA10343C86–C9C为DrawVerticalScroll2(221,85,4,111,first,count−1,4)，沿已核GameView下层绘制、组件12／25及真实首行；方向修窗允许末端不足四行，不改成`count−4`夹限。组件12拖动事件自身有合法滚动范围，与方向保留尾窗分别处理。Update2的VA1031AB8D–1031AED5先合成上／下再修窗，左右各切一次；确认优先于返回，再检查详情。维护`StartupFacilityCatalogInput`在同一Owner候选完成整个输入，失败不留下部分选择／NEW，设备手势不保存。

72的`_draw2_2`（RVA33F040）在VA1033F2F9–1033F3D2始终画目录序号及170×125双箭头，**单项也有1/1**；73的VA1033F3D4另按目录数1隐藏序号／箭头。二者从1033F445起共用装备图标、基础售价和四战斗属性，维护提取共用内容以免重复实现。72仍核真实父79和当前浏览定义，不改父选择或清NEW。

[商品皮肤接口](../prototype/include/dungeon_village_prototype/steam_facility_catalog_skin.hpp)复用已出版Steam装备／战斗图标、窗框、数字及有序计划，查询不初始化或推进Owner、不收费／装配／抽随机／消费声音。中文文字和Noto字体是维护适配；本次机器码合同、研究短测／窗口及原OS动态分别登记，没有据接口声称原字体或物理输入已认证。
