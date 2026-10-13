# Steam设施详情74：定义预览、实例与两页内容

后继状态：本页最初登记的softLabels74缺口已由[独立补证](STEAM_FACILITY_LABELS.md)闭合，两张差异图已在[设施素材包](../assets/steam-facility-common/README.md)发布。下文原批次覆盖与冻结EVIDENCE保持原身份；其余字体、OS输入和表单退休限制仍有效。

2026-10-11维护后继：[皮肤模块](../prototype/STEAM_FACILITY_SKIN.md)提供普通实例两页的有序只读计划与真实Owner桥，[既有HTML示例](examples/index.html)补空/有邻接/七行滚动三条件。Steam静态绘制、维护功能短测、浏览器示例与原窗口动态分开；该历史批不含普通定义预览；后继85→74已提供独立皮肤／Owner只读桥，本批功能／逻辑短测及有界研究窗口已通过。装备店／植物实例及85预览后继已接独立皮肤；入住人数及互动后继已接；住宅按下述已核入口边界保留静态合同，全局字体仍独立。

2026-10-11原窗口补证：用户转交本机`work/window-restore-observation/20261011-094852-facility-detail/RESULT.md`及动作记录，12张1067×905 JPEG已检查格式／尺寸，关键三图[归档](../references/README.md#steam普通设施两页2026-10-11)。同一已选原程序窗口标题可见ver2.56；用户负责读档、处理事件并打开咖啡店，入口选择不算本次自动化认证。第一页显示542G／品质49／魅力46／等级2／距下级71人／一枚爱心加号，名称无编号。第二页维护费240G，来源原序为马戏团广场1（魅力+20）、罗汉松9（价格+20、品质+25）；标签蓝、加号／值棕，两条来源不能代替五行滚动验证。

本次实际输入为右1→2、左2→1、左1→2，单击第二行只移动手形，右下返回回地图。一次Down无可见变化；Return成功注入后也无变化，但是否由游戏消费未知，不提升为原键位／确认认证。原第二页右侧可见蓝色竖条；后继[滚动消费者](STEAM_FACILITY_DRAW_HELPERS.md#drawverticalscroll2实际是两项组件注册)已追到GameView注册下层立即绘图，维护展开补上原短表蓝条与长表整数比例，源helper本身不直接绘图的结论保持。空／超过五来源、拖动、切页保留选择、右2→1、HTML标准／窄窗口仍未实测。原动态局部与完整UI验收分开。

2026-10-10。固定Steam2.56局部绘制、初始化及输入合同，直接来源为GameAssembly具名方法和metadata，[身份入口](../EVIDENCE.md)。没有用APK填补Steam字段；S043等截图只作独立画面观察。坐标是原Graphics调用实参，不能直接当桌面像素；字体、翻译、原点、缩放及裁剪另依对应合同。

## 两个入口不能合并

| 入口 | 已核原载荷与前置分流 |
| --- | --- |
| 建设目录21的详情 | [前批合同](STEAM_BUILD_LIST_BUSINESS.md)核到构造74、value1=1、tenantData=选中定义；不写tenant实例。软键7只给type2/3/13，住宅type12不从该入口预览 |
| 场景当前选中设施 | `_update` 0x10305670从当前selectedTenant取实际实例，调用GetTenantData。**先看定义isLvUp**，真则打开81；否则施工state=0不打开详情；已入住住宅type12且myHomeCharaDataId≠−1直接开人物60、charaInfoChase=1；其余路径才打开74，写实例、定义、value1=0 |

场景升级81先于施工拒绝，是原实际检查顺序；不能改写为“点任意建筑都先打开74再提示升级”。既有说明事件先执行：type13查81，role2查87，武器／防具／饰品distinction1/4/5分别查84/85/86；未发生过则ExecEvent，随后仍走74构造。这里81是**事件ID**，与升级raw81不是一个命名空间。

住宅74绘制与输入确实存在，但不代表场景正常点已入住住宅可达该支。固定APK／Steam的住宅定义25、26、27均为type12、saleRank=−1；[85初始化](../rules/COMMERCE.md)要求saleRank≠−1，因此也不从商会进入住宅预览。APK显式`new g(74)`仅见场景`b/c:1633`、商会`b/g:5899`和建设`b/g:11583`，在这三个已核入口中尚无有效住宅74路径；有限producer扫描不等于全程序不可达。无居民住宅若被错误送入74住宅绘制会按−1访问人物定义，原代码并没有维护层的显式载荷拒绝。产品不得据“分支存在”创造一条不满足身份条件的入口。

74本身不收费、不投道具、不完成升级；它展示并交接至75/79/80，或住宅追踪。维护Owner仍要保留唯一提交者与失败回滚，不能把原私有对象引用布局当跨域事务保障。

## 初始化与引用身份

Init2的74支为0x1030F8C3–0x1030FC57：新建ary1；distinction1、4、5分别枚举已开放state≠0的武器、防具、饰品定义，6调用EnumerateMyHomeChara。它们用于“种类”／入住人数，不是邻接实例列表。

**74只消费这份冻结列表的计数。** Steam完整局部Draw的ary1读取只有装备种类计数（10345769–7E）及入住人数的判零／取数（10346F23–5C），均调用size，没有逐项取姓名、装备属性或候选身份；74确认也不沿ary1选取对象，而是新建79／80，由子页重新枚举自己的目录。74局部Update未重建ary1；SubForm.Resume与Finish的实际共享入口RVA1EDFA0仅`mov al,1; ret`，返回不刷新此列表。固定APK独立对应`b/g.java:3726–3763`的X初始化、7004处种类与7141／7144处入住人数的size消费、5481处子页生产；其末尾c()/d()同样只返回true。不能把“返回后显示相同人数”归因为原页实时重新枚举。

**已核子链不会把变动后的资格目录带回存活74。** Steam79（1031AB8D–AED5）只有导航、查看72、确认／返回；收尾调用RefreshNewEquipParts（RVA2E35C0），该helper三类循环只清BaseData.new_字节+18，不改state_+10，因此不改变74的state≠0种类数；Steam72共用1031B88C–B9D5，仅翻页／确认或返回关闭。APK79的5617起及`c/n.java:2200`对应只清r；72的5466起只有翻页／关闭，不购买或解锁装备。Steam80的取消／空名单／资金不足11都不改入住资格；成功1031AA41–AB49才扣费、撤候选地、建住宅并SetMyHomeParam／刷新入住请求，随后1031AB72调用ChangeCurrentForm(GameForm)。该框架入口在107E4D00–D93将其余栈页置mode4，包含父74，故成功后不恢复这份旧申请人数。APK80的5665起同样成功后切回主表单；这与普通取消Pop80返回74不同。背景世界因模态页可重画不等于继续经营，栈顶准入沿[框架合同](INFORMATION_MENU.md)，不以截图间隔增加满足度或解锁。

据此，**当前维护Owner的合法模态路径可用实时资格计数等价投影，不必仅因原版持有Vector就新增74持久目录**：装备子链只清NEW；入住资格真正改变的成功事务同时退休74，失败／取消保留资格。此判断是具名路径的不变量，不是“原ary1实时”或对任意异步修改的证明；应用快照恢复仍须恢复同一Owner及父子栈，查询不能补缺源。若以后允许74被覆盖时执行解锁／赠礼／满足度／入住变动，再返回同一74，或允许恢复历史资格与当前世界不一致的页面，该等价条件失效，须独立设计冻结显示计数（本页没有消费完整成员身份的依据）。本次不新增字段、不改变既有存档契约，也不把仅计数投影当作79／80自身冻结目录的替代。

tenant非null时另外新建ary2，逐项加入tenant.tenantBonusList中的int[]对象；这一层是列表复制，**不是每条int[]深复制**。条目绘制读[0]邻接建筑定义ID、[1]编号（显示加1）。本批不把它改名为实例uid，也不猜其全部生产和退休时机。预览没有tenant则不生成该实例邻接表。

原Init共同入口按softLabels[type]装配软键，再委托Init2。当前已由[独立标签合同](STEAM_FACILITY_LABELS.md)确认softLabels74为{0,2}，标签2取消消费者已证；不添加未注册的左软键。页面select、scroll、page与字段初始化沿构造/父页生命周期；不能由单张第一页截图证明每次Resume全部归零。

## 五种布局与共同窗框

分派顺序是type12→住宅布局0；distinction1/4/5→装备店布局1；6→入住布局2；type2→植物布局4；其余→普通布局3。metadata还声明INN=5，但本绘制分支不选择5，不据枚举补一套旅馆专属布局。

普通布局3且value1=0时标题为`LT("施設情報 <0>/2",page+1)`，其他为`施設情報`。均调用DrawWindow(220,168,0)与DrawBox(17,74,219,184)。木纹、标题带、边框、文字色和角图按[窗框合同](STEAM_WINDOW_FRAME.md)，不能靠把截图背景缩放成整张弹窗替代。

只有普通实例调用Draw_titleBarArrow(220,168)：common SEB3 arrow02，左frame3、右frame0；Touch组件1的值分别16、18。令k=trunc((frame%16)/8)，逻辑点为左(26−3k,55)、右(214+3k,55)。这些是原帧字段，不换算秒。两边都持续注册；按当前逻辑两页循环，不因第一页禁左或末页禁右，左1→2已有本次原窗口证据。

页0标题下：设施小图标在(21,66)，Draw_icon(mode9,index=定义iconIndex)。它读common图片91 icon_tenantInfo，裁片(16×(index%10),0,16,16)。名称日文在(41,69)anchor1；非日文临时字号11，TextLayout(41,69,85,11)，anchor0x20、lineSpace0，随后ResetFontSize。名称色SC_WINDOW_BLACK；右侧标签为SC_WINDOW_BLUE。

普通价格标签日文(136,69)，非日文字号10、(133,70)。数值以Draw_money(x223,y70,seb15)；装备店改显示“種類”与ary1.size，DrawNumImage(seb15,x214,y70,padding0,anchor4)。不能将种类数当库存总量。

## 定义预览与实例数值

普通页0价格／品质／魅力分别为TP0/1/2。value1=1走GetTenantParam，不加实例bonus；value1=0走GetTenantParamAndBonus(…,tenant)，并与GetTenantParamLimit比较，达到或超过上限画common图片129 wnd_max。MAX是显示标记，不证明本页重新封顶或扣费。

品质／魅力行基准y=93、108。标签日文x136，非日文临时字号8、x130、y再加2；数值SEB15，日文x205、非日文x207，y仍93/108，padding0、anchor4。MAX点为(160,y+3)。价格MAX另在(160,72)。预览不走这些实例MAX判断。

大图先画绿色底板：普通布局x=23，住宅／装备／入住／植物x=71；外框DrawRect(x,88,97,74)，色(181,219,166)，内填(x+1,89,96,73)，色(219,249,207)。ClipRect(x,88,98,75,true)后调用**DrawMapchip2(g,x+49,125,mapchipId,0)**，随后PopClip。无论实例朝向如何，本页传的方向都是0。

DrawMapchip2使用已核[pattern和格偏移](STEAM_MAPCHIP_PATTERNS.md)，但比建设锚点另加居中偏移：pattern0=(-30,0)，1=(-45,7)，2=(-30,15)。每块实际点为中心点＋此偏移＋(30(u+v),15(u−v))，再走原SEB裁片偏移。不能沿用建设DrawMapchip而漏掉大图居中修正，也不把底板宽高当整座图的强制缩放。

后继具名helper复核：DrawMapchip2的整个已登记方法直接使用pattern帧，没有普通DrawMapchip对kind6道路的11／1覆盖。因而即便道路条件查询也保留0／1，不把建设道路帧策略套入详情图；真实74能否从某入口显示道路仍由入口资格另验。85项静态资源与定义映射已核相同字节，维护展开见[皮肤模块](../prototype/STEAM_FACILITY_SKIN.md)。

普通布局附common图片38 wnd_lv于(25,151)。lv≠5用SEB12数字，日文(53,151)、非日文(55,151)，anchor4；lv=5用wnd_max于日文(44,154)、非日文(46,154)。这仍读取共享定义lv，不是每栋实例独立等级。

右侧两块浅黄色区：外框(123,88,90,34)、(123,124,90,38)，色(255,207,99)；内填(124,89,89,33)、(124,125,89,37)，色(255,250,213)。使用效果逐行从effectTUseType/Value读取：图标mode7在(136,127+17i)，common图片37第二行16×16；类型5特殊取x96，其余16×(类型%10)。数量以common SEB15 frame14由x192向左每次8，y=130+17i重复，不能误认成普通数字字符串。无效果画`---`于(168,137)、anchor2。

lv≠5下方显示“距下一等级”：剩余=GetNextLvUpNum()−tenantUserNum，SEB16于(188,172)，后接SEB12搭配**Steam**图片103 number05、frame18于(189,175)。En()为真另画图片178 ppl于(191,174)。lv=5换成最高等级提示。预览同样显示定义成长信息，但未认证当前截图中的具体剩余数字。

## 页1：邻接加成而非普通详情继续按钮

普通实例第2页上方画“施設ボーナス”(27,69)、anchor1；“維持費”(132,69)，TP3 GetTenantParamAndBonus以Draw_money(214,70,seb15)展示。列表空时(120,97)居中显示“ボーナスはありません”。

ary2最多5可见行，y=97+19×(绝对index−scroll)。选中行画SEB21 finger_r于(17,y+8)。行Touch SLIST11的矩形是(-1,y−2,231,18)、value=0x20000|绝对index，**本页没有raw75那套marker16/Margin(29…)调用**。只复用通用SLIST分派，不能复制道具选择行的二次点击资格。

每行以条目[0]查tenantData、mode9图标(26,y−2)，名称拼接条目[1]+1；非日文字号10于(44,y+1)，日文(44,y)。增益文本来自邻接定义effectAroundTenantValue：普通显示魅力＋第0项；type2植物另外显示价格＋第0项、品质＋第1项。原字符串包含`<co=0065FF>`与`</co>+`，要交给原文本标签规则，不能把标签字面显示出来或先用当前TP重算另一套数值。

日文普通魅力位置x147，植物价格x121／品质x171；非日文字号9，普通魅力(x151,y+2)，植物分别TextLayout(130,y+2,35,9)、(168,y+2,35,9)，anchor0x20。末尾恢复字号。滚动条调用DrawVerticalScroll2(221,85,4,110,scroll,count−1,5)，上界实参是count−1；空列表也走此helper，边界规则沿其消费者。

页1上/下重复脉冲独立检查，select按count循环且只在count>0时处理；scroll跟随保持5行。符合普通实例资格时，左和右各自令page=(page+1)%2；同轮两者都命中会切两次。原代码没因切页清select/scroll。第2页确认直接返回false，不推75、不选择邻接设施、更不消费道具。

## 特殊布局与底部说明

住宅布局10346A5D–10346BCB通过实例myHomeCharaDataId（+58h）查人物定义，以职业GetImgId和当前武器绘人物；动画索引trunc((frame%16)/4)，SetDispPlayerData传seb0、direction1、updateCnt=完整frame；人物绘于(145,155)。底文槽110EA59C为`<0>さんの住居です`，代入GetName后居中(120,172)。住宅不画底部橙按钮／Touch4；定义预览末尾虽仍有Touch2，不能绕过前文所需的有效住宅实例。

共享表现不是纯演员裁片：SetDispPlayerData RVA2E6B90复用UserData同一显示Character2，仅写action／bodyImg／animeIndex／direct／updateCnt，不清其它历史字段。DrawDispPlayer RVA2CCEA0先SetDrawParam；scratch类别为人物且武器ID≠−1时先画武器，再Draw_human(...,true)，末布尔是isSoccerForm。已核主体朝向1对应HUMAN SEB1，身体步帧与武器帧分开；本页没有额外阴影调用，不能借标题外层阴影。haveObj残留可改变身体SEB；SetDrawParam与Draw_human还写draw_param及old_screen_pos，见[共享人物合同](STEAM_TITLE_ACTORS.md)。Draw_human仍调用DrawEffDamage／DrawEffect；二者局部入口分别读取damage计数+114h和效果列表+19Ch，其直接调用中未见共同Random，但未穷尽附加效果子树及scratch所有历史状态，不能据此宣称整页零抽数。当前Owner表现请求尚未接住宅74，纯裁片复用不等于该副作用／恢复已完成。

入住布局detail6在10346EFC–10346FB5使用棕色(92,51,31)，ary1为空显示`現在入居希望者はいません`（槽110F8614），否则用LT(string,long)将人数代入`現在<0>名の入居希望者がいます`（110F856C）；均普通DrawString(120,172,anchor2)，此段不临时换字号。标题无页码，大图沿x71居中底板；不画种类数字、经营三属性、等级、距升级、页签或列表。候选枚举RVA221250严格按人物定义原序取state≠0、satisfaction≥threshold、myHouse[2]==0；APK `a/o:330–341`独立同三谓词，不额外读flags／活跃实例或触发刷新。维护查询可共享此只读资格；空集合有效，缺字段应显式拒绝。

入住实例底部103471CE–103471FB调用Draw_btmMsg(`入居希望者`,197,true,true,200)：W=min(实测宽,200)+16、L=120−W/2，橙底(L,197,W,16)，Touch4/value20为(L−20,177,W+40,56)，棕字同底rect／anchor34，手形SEB21当前帧(L−4,206)。人数0也保留按钮，确认Push80，由80的真实空名单事件16退出；不自行禁用。value1=1预览仍显示人数，但跳过按钮，仅保留末尾Touch2/option2；soft仍{0,2}。APK `b/g:7141–7144、7239–7240`独立同几何，汉化“入住希望者”只作为维护中文适配，不认证Steam翻译。

装备店底部将distinction1/4/5翻译为武器／防具／饰品后填“正在出售”；入住布局按ary1.size显示当前申请人数或无人；植物布局显示effectAroundTenantValue的价格和品质，并在y199说明影响周围。植物非日文临时字号10，日文字号11；不能把日文和中文排版当同一路。

植物两语言的精确锚点在10346BEB–10346EF7：日文蓝色“価格”(54,172)，棕色`"+"+YEN("G",value0)`右锚(118,172,anchor4)，蓝色“品質”(134,172)，棕色`"+"+value1`右锚(187,172,anchor4)。**Steam非日文改为标签右锚、值左锚**：价格(78,173,anchor4)／值(82,173,anchor1)，品质(158,173,anchor4)／值(162,173,anchor1)，仍分别蓝／棕，均普通DrawString，末尾ResetFontSize。固定APK `b/g.java:7147`起非日文对应58／108／128／181，不能套入Steam。值来自定义effectAroundTenantValue的前两项，不是设施当前售价／品质或使用效果；两处金额调用10346CBB／10346E37实际是MyFormBase.YEN(string,long) RVA202140，不是LT：先把int值符号扩展为long，包装固定comma=true；RVA2019F0在10201AF8–1B24匹配精确Ｇ／G后直接走1020206B的Unit(value,true)（RVA201890），每三位插逗号、保留负号并拼回原后缀，不走语言或币种倍率分支。因此本页价格为无条件“+”加有符号分组三位十进制再加ASCII G，例如+1,234G／+-1,234G；品质仍是“+”加普通十进制，没有千分逗号。实例底部1034717D–103471CC仅棕色普通字`周囲に好影響を与えます`@(120,199,anchor2)，没有橙按钮／Touch4／手形；定义预览跳过此句，末尾仍沿Touch2／option2。

普通页0底部Draw_btmMsg“使用道具”，y197、select=true、cursor=true、widthMax200；页1为周围设施加成说明，select=false、cursor=false。同一helper还用于装备店“查看商品”、入住“入住申请者”。value1≠0跳过这些互动提示。预览value1=1最后登记Touch组件2、TouchOption.Create(2)；其最终物理热区及关闭键分派仍由框架决定，不根据无坐标重载猜成某个屏幕矩形。

## 确认、取消与行为交接

确认脉冲0x100000优先；后继metadata复核将额外keyState0x40门控明确为`Config.DEBUG_CMD`（VA1031B5A3，静态字段+83），正常UI不复制该调试快捷键。未确认再检查softlabel2，命中Pop。确认时顺序为：

1. value1恰1：直接Pop，回建设父页；不扣款、不转道具页。
2. page恰1：返回false，不执行后续业务。
3. type12：从当前人物实例列表找定义ID等于住宅myHomeCharaDataId的人，写相机追踪人物、清追踪设施，ChangeState(6)，再Pop；找不到同样Pop。
4. distinction1/4/5：构造79，只写value1=distinction；本支不传tenant实例。
5. distinction6：构造80，复制tenant和tenantData；普通非植物构造75，同样复制两者。
6. type2植物：无确认业务，返回共享true。

植物实例确认的无副作用分支属于Steam74的Update2局部1031B3B6–1031B88B；固定APK `b/g.java:5553`同样仅type非2才创建75。预览的确认关闭优先，不能把植物实例确认改成关闭。维护`act_startup_world_facility_page`已将kind2确认修正为成功且不修改页栈、资金、随机或一次性输出；预览仍优先关闭，取消沿框架释放引用。

Pop与Push的本地调用已证，不代表完整FormManager退休、所有父级选中保留和最终OS投递均已认证。页面私有坏值不能靠原代码的非null崩溃分支作维护错误协议；原实例、共享定义、邻接引用与历史对象仍要分别校验。

同轮顺序是第二页上repeat→下repeat→五行scroll追踪→左repeat→右repeat→确认→未确认才返回。左右同时命中会切两次，第二页确认与返回同时命中则确认no-op且不再取消。Update的共同frame/frame2递增、wait门槛、loop/jingle、SEB21/22/23 Frame在Update2之前；当前SEB手形帧不能由页面计数猜定。74、已追键脉冲与局部触摸helper未直接发SE，不据此断言整个平台输入无声。

字段交叉：SubForm.select/scroll/page分别+90/+88/+D8，ary2为+170；GetTouchValue RVA30E500、SetTouchValue RVA317600按高位0x20000/0x40000/0x50000访问这三项。GameView.OnTouchDlgSel RVA23C730先写选择，是否首次只标记由option16控制；74行没有75的该选项。OnTouchDlgPage RVA23C620走键Down/Up，不套列表二次确认。原OS最终输入与声音仍需独立观察。

## 资源与交付资格

本包登记20项common引用，含SEB间接图片依赖；18项逐字节对应已有出版副本，number05必须使用[Steam专用副本](../assets/steam-build-common/README.md)。两张差异图现已在[设施素材包](../assets/steam-facility-common/README.md)出版，均与APK同名图片字节和RGBA不同；身份由该包清单统一维护：

| 图片 | Steam身份 | 已证消费者 |
| --- | --- | --- |
| 37 icon_param00.png | 112×32、1790字节 | Draw_icon(mode7)使用效果图标，第二行16×16，类型5选x96 |
| 105 number08.png | 100×21、648字节 | SEB15的价格／维护费／品质／魅力／种类数字与使用效果frame14点数 |

不能取APK副本冒充这两个Steam依赖。现维护普通74计划已绑定这两张正式Steam图；完整SEB裁片／偏移及图片依赖沿现素材清单，不在本页复制指纹。

静态范围为局部raw74绘制0x103453B8–0x1034728A及上述具名helper。普通实例两页的维护计划、Owner桥及短验已由[皮肤模块](../prototype/STEAM_FACILITY_SKIN.md)交付，不再列作缺口；85→74普通定义预览已提供皮肤／Owner只读桥，使用共享定义值及单页原触摸请求，不借实例双页／邻接；本批功能／逻辑短测及有界研究窗口已通过。装备店／植物实例及85预览亦已接独立计划和严格Owner桥；入住detail6人数／按钮及严格共享资格已接；建设21预览入口、住宅的其它潜在产生者、最终中文字体、场景到selectedTenant的全路径及原表单动态仍独立。既有S043/S047/S054及后继原图只证明各自已见状态，不认证逐像素一致、住宅自然可达或产品完整接入。

## 静态来源范围

来源范围为_draw2_3／Update2的74分支、GameForm._update设施入口、Init2、DrawMapchip2、Draw_titleBarArrow与Draw_icon。只认证已列分派／helper，不把整个巨型方法视为完整认证。ary2仅复制列表层并保留int[]引用，完整原表单退休未核；局部映射不能证明原页内存永久有界。
