# 建设目录21：Steam局部合同与APK交叉边界

本页整理固定Steam2.56建设目录21的局部绘制、输入登记及与汉化重签APK1.0.8的差异；
当前结论合并至2026-10-14。三项后继合同分别负责：

- [初始化／选择／确认](STEAM_BUILD_LIST_BUSINESS.md)：三目录、五行空行、专用跟窗、资金预检、21→74预览返回及BUILD交接。
- [图块与朝向全集](STEAM_MAPCHIP_PATTERNS.md)：两个pattern数组、85设施反向对应及SEB请求；raw21固定朝向0均有效。
- [资源安装及旋转准入](STEAM_BUILD_RESOURCE_INSTALL.md)：地图资源组根身份、安装／退休与flags32软键资格；common两张差异图已在[独立Steam包](../assets/steam-build-common/README.md)发布。

这些是静态来源合同，不等于维护Owner／完整快照、原窗口或产品接入验收。
Steam与APK的行margin／类目命中矩形不同；最终OS投递、完整父栈退休和建设落点仍分别核验。

## 来源与证据范围

固定DLL／metadata及APK身份统一见[证据入口](../EVIDENCE.md)与[Steam样本评估](../verification/STEAM_ASSESSMENT.md)。
Steam方法登记及SubForm字段声明沿用既有methods.json／dump.cs；它们是本地派生定位材料，不另抄一份指纹清单。
下文保留已核方法地址与局部范围。早期draw／Update／Init2各1024字节的入口探针仅证明外层前段；
raw21绘制后来已沿`SubForm._draw`入口顺序闭合，实际范围见本页绘制节。
这仍不表示197,616字节的外层登记跨度全部经过分析；目录业务另由上述真实Init／Update合同负责。
APK巨型Java方法的普通反编译警告仍有效，图块绘制已另以低层窗口交叉，见[图块合同](PAGES.md#固定apk候选建筑两朝向与目录裁剪)。

## Steam已证的页面字段与输入

`SubForm`metadata字段：`type_ +0x84`、`scrollValue_ +0x88`、`select_ +0x90`、
`page_ +0xD8`、`buildList_ +0x168`；`value1_ +0xB8`、`value2_ +0xBC`是其它字段。
以下映射由实际DLL消费者交叉，不能把类目索引写成value1_。

| 合同 | 实际机器码位置 | 已证内容／限制 |
| --- | --- | --- |
| 建设类目事件 | OnTouchEvent RVA0x3160B0；VA0x10316103–0x103161E5 | TYPE21、组件3且value高16位0x50000处理UP／ENTER；类目循环模3，并清scroll／select。名称及注册矩形见下文绘制节 |
| 类目写入 | SetTouchValue RVA0x317600；VA0x10317612–0x10317626 | type高16位0x50000写page_；此方法本身不限定21页 |
| 列表选择写入 | 同方法VA0x10317653–0x10317667 | type高16位0x20000写select_；21五行的实际注册见下文 |
| 滚动写入 | 同方法VA0x10317668–0x1031767C | type高16位0x40000写scrollValue_；21组件12／25的实际注册见下文 |
| 通用选择跟窗 | Update_scrollValue RVA0x3284B0；VA0x103284E3–0x103285B0 | 通用helper按count裁剪选择及首行；**21不调用它**，而用允许空行的五行专用分支，见[业务合同](STEAM_BUILD_LIST_BUSINESS.md#五行范围空行与键盘顺序) |

通用DLGSEL3／LIST10／SLIST11的marker16共享消费者已证：首次未标记UP选择并记key，
第二次同key UP发逻辑确认，没有点击间隔判定；键盘会清marker。
见[Steam输入合同](STEAM_INTERACTIONS.md#输入选中标记与确认分开)。
后继在`0x10354A8D–0x10354AF1`确认Steam21实际注册marker16，因而本页具备通用首次UP标记、再次同key UP确认资格；
它不是带时间阈值的OS双击。最终窗口鼠标事件与组件key仍需原窗口／surface单独核对。
Steam适配事件CANCEL=10；APK原始Android ACTION_CANCEL=3，两个事件层保持分开。

## APK21目录及特殊边界

以下分别登记固定APK事实与Steam独立交叉结果。坐标是原画布局部坐标，不是物理DPI热区。

| 项目 | APK局部合同／来源 | Steam交叉状态 |
| --- | --- | --- |
| 类目／开放集合 | [b/g:10647](../work/decompiled/sources/b/g.java:10647)：3个Vector；定义开放p非0、flags4，住宅kind12还需H>0，按定义j入类；第0类索引1插撤除−1，flag32开放后索引2插移动−2 | Steam Init独立核同过滤、定义顺序及哨兵插入；随后还刷新入住标志，见[业务合同](STEAM_BUILD_LIST_BUSINESS.md#目录生成与打开时的副作用) |
| 类目文字 | [b/g:9604](../work/decompiled/sources/b/g.java:9604)：设备／一般／饮食 | Steam独立metadata literal为設備／一般／飲食，最终中文译文仍走翻译 |
| 目录原点／外框 | [b/g:9591](../work/decompiled/sources/b/g.java:9591)：J=((c.m+c.o)/2)−87−画布偏移X，K=((c.n+c.p)/2)−104−画布偏移Y；外框175×210 | Steam独立核同局部公式，GameForm VIEW四字段及Graphics原点见下节 |
| 行／图块 | [b/g:9655](../work/decompiled/sources/b/g.java:9655)：最多5个实际条目；行距37；图块左上(J+10,K+25+37row)，64×32裁剪，RGB(190,242,230)，固定朝向0按原分片画 | Steam核同裁剪；实际DrawMapchip锚为(J+12,K+35+37row)，非直接整PNG |
| 特殊图标 | 撤除−1图片17偏移(4,3)，移动−2图片119偏移(1,0)；住宅H、NEW147及kind13提示81另叠加 | Steam图槽及真实字段已核，见下节；NEW不能一律用定义新开放位 |
| 报价 | [b/g:9688](../work/decompiled/sources/b/g.java:9688)：撤除0G、移动300G、普通定义报价h()；资金不足换价色，0G仍绘 | Steam调用GetBuildCost；0/300及资金比较已核，具体价格规则独立于绘制 |
| 每行输入登记 | [b/g:9730](../work/decompiled/sources/b/g.java:9730)：SLIST11，矩形(J+2,K+22+37row,165,37)，value=(scroll+row)\|0x20000，marker16，扩命中(10,10,0,0) | Steam同矩形与marker，但Margin为**(10,0,0,0)** |
| 类目输入登记 | [b/g:9742](../work/decompiled/sources/b/g.java:9742)：DLGSEL3，矩形(J+3+57cat,K+2,54,20)，value=0x50000\|cat，扩命中(0,0,30,0) | Steam为**(J+57cat,K+2,60,20)**，Margin同(0,0,30,0) |
| 滚动绘制实参 | [b/g:9740](../work/decompiled/sources/b/g.java:9740)：(J+170,K+21,189,count−1,5)传滚动helper | Steam显式宽4、高189；helper注册宽3的组件12与25，见下节 |

APK与Steam均需保留两个易被普通列表规则抹掉的边界；Steam自己的Update证据见[业务合同](STEAM_BUILD_LIST_BUSINESS.md#五行范围空行与键盘顺序)：

1. **输入登记始终5行**，实际条目绘制才受`scroll+row<count`限制；集合不足5项时，空行也有登记。
2. [b/g:11537](../work/decompiled/sources/b/g.java:11537)的键选择上界是`max(count−1,4)`，
   允许选择空行；首尾循环、左右换类、跟窗采用21自己的分支。确认时才检查`select<count`。
   因而不能将Steam通用`Update_scrollValue`的count裁剪直接套为21精确合同。

## APK选择、预检查与建设提交分开

[b/g:11571](../work/decompiled/sources/b/g.java:11571)至11629：选择变化刷新对应软键；
未收到确认时，取消退出，动作7仅对非负定义打开74详情并以value1=1限制详情模式。
确认且选择在真实条目范围内时，先比报价与资金；不足拒绝，这一页尚不实际扣款。
撤除−1进入建设模式3，移动−2进入模式6；道路kind6进入模式1，普通flags4进入模式0。
普通选中设置定义ID与朝向0，清定义新标r，再切GameForm建设场景；提示旗标与真实扣款属于后续消费者。
已有[普通候选绘制与旋转](PAGES.md#固定apk候选建筑两朝向与目录裁剪)不是本页选中输入已完整认证。

Steam建设候选闪烁、朝向传递及建设确认消费另见[覆盖清单](STEAM_UI_COVERAGE.md#建设定位拖动旋转与提交)，
与本页目录21的行确认不混合。现有维护查询及原型／产品缺图修复也不当成Steam原窗口证明。

## Steam后继：完整raw21局部绘制分支

2026-10-09。新证据包沿具名入口`SubForm._draw`从`0x10352E20`顺序解码8704字节，
不是从历史被拒的中间地址启动解码。`0x10353248`比较type21，跳转`0x10353799`；该分支在`0x10354F8D`返回，
共6133字节。另两个具名完整小方法为`MapchipData.DrawMapchip`784字节和`AppData.DrawVerticalScroll2`304字节，
总唯一前缀9792字节；仍不称197616字节登记跨度全部属于已分析函数。

**绘制有状态副作用**：raw21入口`0x10353799–0x103537AF`直接把`SubForm.frame_ +0xA0`加1并钳到3。
共享手形/箭头另读`frame2_ +0xA4`。因此后续只读布局计划只能接已准备的计数，完整应用必须另按显式表现准入维护这次改变；
不能把任意重画说成无副作用，也不能由此推出每秒或固定逻辑tick次数。本批没有将它接入Owner或回放。

### 坐标、标签和五行

令`J=trunc((GameForm.VIEW_X+VIEW_W)/2)-87-Graphics.GetOriginX()`，
`K=trunc((GameForm.VIEW_Y+VIEW_H)/2)-104-Graphics.GetOriginY()`。
这里只保留真实字段算式，不擅自把VIEW_W/H解释成OS客户区边界。外层先请求`DrawRect(J,K,175,210)`，
后加内线及内容框；以下是原Graphics实参。`DrawRect`的半开转换沿[Steam窗框](STEAM_WINDOW_FRAME.md#drawrect不是默认线宽的stroke)：
外界为(x,y,w+1,h+1)的一单位内边环，不得把DrawRect误作FillRect。

2026-10-14沿同一`_draw`入口顺序补核`10353831–10354147`，原顺序如下。所有线宽均为1；表内RGB均来自实际GetColorOfRGB参数。

| 顺序／项目 | 原请求 | RGB |
| --- | --- | --- |
| 外线→内线 | DrawRect(J,K,175,210)；DrawRect(J+1,K+1,173,208) | (79,72,48)；(193,234,94) |
| 左侧亮线 | DrawLine(J+2,K+2,J+2,K+208) | (235,241,132) |
| 每类边框 | DrawRect(J+3+57cat,K+2,56,16) | 选中(59,119,0)；未选(60,109,0) |
| 每类填充 | FillRect(J+4+57cat,K+3,55,15)，随后该类文字及NEW | 选中(3,255,133)；未选(223,255,67) |
| 三类之后的亮横线 | 先(J+2,K+19)→(J+173,K+19)，再(J+2,K+21)→(J+173,K+21) | (232,237,166) |
| 中间横线 | (J+2,K+20)→(J+173,K+20) | (100,135,71) |
| 内容两层边框 | DrawRect(J+2,K+22,165,186)；DrawRect(J+3,K+22,163,185) | (235,241,132)；(5,184,2) |
| 内容填充 | FillRect(J+4,K+23,162,184) | (251,253,249) |
| 四条行分隔 | n=1…4，DrawLine(J+4,K+22+37n,J+166,K+22+37n) | (5,184,2) |

线端点不能套DrawRect规则；[已有DrawLine合同](INFORMATION_MENU.md#steam36的完整局部绘制合同)说明普通非lineGraph、宽1且轴对齐时
转FillRect(x1,y1,dx+1,dy+1)，包括终点。纯计划保留端点和线宽，可让后端按明确模式展开；不认证任意矩阵／DPI光栅。

三类literal在Steam metadata独立解析为`設備／一般／飲食`，不能凭日文literal推定当前中文显示。
标签文字锚在`(J+5+57cat,K+3)`，TextLayout高15、anchor0x22；正常宽55，
`UserData.isBuildNew[cat]`真时文字宽35，并在`(J+39+57cat,K+9)`画common图147。
非日文临时字号9，画后恢复；日文保留当前字体。类目文字色来自SubForm静态字段，不取类目填充色。
`SubForm.cctor` RVA3285E0的`1032BB52–1032BBE9`连续RGB构造及字段写入已补核：

| 字段／静态偏移 | RGB | raw21读取处 |
| --- | --- | --- |
| SC_BUILD_TAB_SELECT／+4C | (42,98,4) | 10353B71 |
| SC_BUILD_TAB_NOTSELECT／+50 | (76,126,66) | 10353B55 |
| SC_BUILD_NAME／+54 | (33,36,51) | 1035474D、10354896 |
| SC_BUILD_PRICE_BRACK／+58 | (99,101,112) | 103548FF、103549C5 |
| SC_BUILD_PRICE_RED／+5C | (210,0,0) | 103549E8 |

这些名字来自metadata字段，数值来自Steam构造器，不能用其它页面SC_WINDOW_BLACK或截图近似色代替。

行号r固定遍历0…4；实际索引`i=scrollValue+r`。实际条目超界时在`(J+90,K+41+37r)`画`---`、anchor0x22；
并没有因此缩短后面的五行注册循环。`103541AF→10354A13–10354A43`空行直接调用DrawString而不SetColor：
它继承前一真实行的最终价格色；整类为空时沿此前分隔线色(5,184,2)。不能把`---`一律染成名称色。
选中可见行使用`select-scrollValue`，不能把它当定义ID。

令`P=(J+10,K+25+37r)`。合法条目先以RGB(190,242,230)填64×32再`SetClip(P,64,32)`，
图标／建筑及图上数量／NEW／入住提示绘完明确`ClearClip()`，之后才画名称／价格。
调用`10354262`确为SetClip，沿[替换clip合同](STEAM_WINDOW_FRAME.md#clip从逻辑矩形到字体队列)，不压栈、不与旧clip相交。
本次另核`Graphics.ClearClip` RVA75DBA0的128字节：`1075DBA6`取`renderRect_ +14`的四int，转float后
`1075DC02`调用`_setClip`；它恢复renderRect，不调用PopClip，也不恢复此前局部clip。维护计划须保留set／clear语义。

| 条目／资格 | 真实绘制请求 |
| --- | --- |
| 普通非负定义 | `tenantData[id].mapchipDataId_`取mapchip记录，再把该记录`BaseData.id_`传`DrawMapchip(g,J+12,K+35+37r,id,0)`；最后实参朝向固定0 |
| 撤除−1 | common图17 `destruct00.png`，P+(4,3)；文字literal撤去，报价0 |
| 移动−2 | common图119 `moveTenant.png`，P+(1,0)；文字literal配置替え，报价300 |
| 住宅type12 | `TenantData.haveNum_ +0x88`交`Draw_multiValue`，SEB12 `number05.seb`，锚`(J+75,K+47+37r)` |
| type2或3、`isBuildNow +0xA1=false` | common图147 `wnd_new.png`于`(J+10,K+48+37r)`；这不是简单读取BaseData新标志，不能和类目isBuildNew合并 |
| type13且任一定义`isResident_ +0x99=true` | common SEB81 `tenant_resident.seb` frame0、lineNo−1，于`(J+42,K+40+37r)`；SEB源引用图98，内部偏移另加一次 |

普通名称取`TenantData.name_ +0x1C`，价格取`GetBuildCost()`。
非日文临时字号10，令`N=min(StringWidth(name)+9,88)`；选中底色RGB(248,193,108)在`(J+76,P.y,N,15)`，
文字TextLayout矩形`(J+80,P.y,N-4,15)`、anchor0x20，画后恢复字体。
日文选中底宽`StringWidth(name)+15`、x=J+79；直接文字于`(J+86,P.y+2)`。
日文名称的三参DrawString RVA766290明确委托默认anchor0x11（LEFT1|TOP16）；两语言类目仍走上文TextLayout的0x22，不混成同一重载。
价格经`YEN("Ｇ",price)`，锚`(J+164,P.y+18)`、anchor4；0也显式画，非零价格按当前系统所持资金比较后取正常／不足两静态色字段。

同一真实行的次序为：填64×32底→SetClip→普通DrawMapchip→住宅数量或普通NEW→满足条件的入住提示→ClearClip→
名称选中底（若选中）→名称→价格。type12分支直接画数量，不再走type2／3的NEW分支；type13入住检测独立位于其后。
负哨兵用各自图片替代Mapchip，同样先ClearClip再名称／价格。不得把叠图、数字或裁剪跨行排序。

住宅`Draw_multiValue` RVA2552A0的224字节与[现数字helper合同](STEAM_FACILITY_DRAW_HELPERS.md#drawnumimagecomma的signed串逗号和anchor)交叉：
先对SEB12调用DrawNumImageComma(value,dx,dy,padding0,comma_padding0,anchor4)，再画同SEB frame16乘号，
锚(dx−8×GetFig(value)−9,dy)、lineNo−1；本页dx=J+75、dy=K+47+37r，不画字体乘号。
现`SteamFacilityNumberKind::multi_value`／number05可消费该请求；SEB12沿已核同字节资源，图片必须用Steam版103。

### 建筑字段到mapchip图层

`MapchipData.DrawMapchip`在`0x1021BCA0–0x1021BF97`独立核实：mapchip定义ID不是PNG索引。
它先取`AppData.mapchipData_[id]`，再以该记录`tenantDataId_`反查设施定义；按该设施`mapchipPattern_`与传入朝向
遍历`TenantData.SETMAPCHIP_ADDRESS[pattern][orientation]`。分片地址(u,v)产生目标偏移`(30*(u+v),15*(u-v))`，
加到调用者锚点；资源组来自`AppData.resMapChip_ +0x20`，SEB索引取`MapchipData.seb_ +0x24`。
非道路的帧来自`SETPATTERN_MAPCHIP[pattern][orientation][piece]`；道路type6则朝向1用frame1，其它朝向用frame11。
最终为`Seb.DrawFrame(...,resource.images,frame,lineNo=-1)`，由SEB再关联真实图片和裁片。

本链没有将`MapchipData.img_`或`TenantData.icon_`直接拿来画整张建筑PNG。
Steam两个pattern数组及85定义到mapchip／SEB的逐片对应已由[图块合同](STEAM_MAPCHIP_PATTERNS.md)独立闭合，不以APK数组填值。
该合同保留源SEB异常；raw21朝向0有效不代表所有定义都允许朝向1。
资源摘要（本地核对材料，不随仓库交付）核7个直接图／SEB槽与已发布默认文件字节一致，并追5个SEB图片依赖：
arrow02／arrow01／finger_r同字节，**number05图103与tenant_resident图98的Steam字节不同于APK发布副本**。
两个SEB相同不证明其图片也相同；这两张已独立解析并发布于[Steam资源包](../assets/steam-build-common/README.md)，不能alias APK图片。
地图资源组根身份与安装／退休见[资源安装合同](STEAM_BUILD_RESOURCE_INSTALL.md)；完整语言覆盖、异步图像装载及GPU内部退休仍未因此闭合。
common图147的INF仍有`20x9`修饰，不把文件hash相同当当前语言的实际命中结果。

### 实际marker、滚动和箭头注册

| 调用位置 | Steam实际注册参数 |
| --- | --- |
| `0x10354A70–0x10354B01` | 固定5行SLIST11，矩形`(J+2,K+22+37r,165,37)`，value=`0x20000|(scroll+r)`，`TouchOption.Create(16).Margin(10,0,0,0)` |
| `0x10354C81–0x10354CFD` | 固定3类DLGSEL3，矩形`(J+57cat,K+2,60,20)`，value=`0x50000|cat`，默认option再Margin(0,0,30,0) |
| `0x10354C55–0x10354C71` | `DrawVerticalScroll2(g,J+170,K+21,4,189,scroll,count-1,5)` |
| 滚动helper `0x1025187E–0x102518BF` | VSCROLLBAR12：同x/y、宽3高189、value0x40000，option args=`[count,5,0x20000]`及Margin(3,20,0,0)；再注册SCROLLBG25同矩形、value0、FLAG_MAX_PRIORITY4 |
| `0x10354D87–0x10354E73` | 左右箭头使用组件18、common SEB3 frame3／0，value16／18；y=K+9，x分别为`30-trunc((frame2%20)/5)`、`207+trunc((frame2%20)/5)`；**x未加J**，Margin分别(40,−10,20,−10)与(−10,40,20,−10) |

滚动helper本身只注册组件；其下层组件12立即绘type0轨道／滑块的源链已在[设施共享helper](STEAM_FACILITY_DRAW_HELPERS.md#drawverticalscroll2实际是两项组件注册)闭合，21复用相同调用。
现`SteamFacilityScrollRequest`可表达矩形及五行参数，但`steam_facility_scroll_skin`当前严格限定74的(221,85,3,110)，
不能直接把21的189高请求交给它或借74的选择夹界抹去空行。最终OS拖拽／命中优先级仍独立。
2026-10-14重核`GameView.DrawVerticalScroll` RVA23A720的1440字节：原helper**不夹value／scroll**。
令D=max(maximum,largeChange−1,0)+1，top=y+trunc(h×value/D)、len=trunc(h×largeChange/D)。
`1023A9BB–1023A9D1`另有原边界：若top+len>y+h，直接改len=h−top（这里没有补回y）；len≤0返回null，
否则type0才画len+1高滑块。21的189高和合法独立首行须保留这次原裁短，不能把原算式改成y+h−top或夹scroll凑常态公式。
暗轨道已在`1023A98E`画出，早于len检查，故返回null不是整项无绘图。
`GameView._addTouch` RVA23E010中`1023E227–1023E231`检查该返回：null经`1023E5B7→1023E822`继续普通组件注册链，
保留原矩形，不读取两项返回数组、不改为滑块几何，也不设置长表拖动flags／grid。
因此21的null分支仍有暗轨道FillRect(J+169,K+21,5,189)、组件12原基矩形(J+170,K+21,3,189)及原Margin／args；
非null且非有效拖动才在`1023E590–1023E5B4`把基矩形改为(x,top,5,len)。`count≤5`在`1023E179–1023E182`只强制firstTouch=false，
不屏蔽12；正常scroll0仍画190高蓝条。这里是逻辑注册请求，实际入池仍须通过共享顶页、clip、变换等准入，不能当最终物理热区。
非null且firstTouch有效、count>5时，`1023E252`将TouchOption.flag（+8）按位或0x24（最大优先级4／取消屏幕裁限32），
`1023E2B4–1023E2DD`写dragGridH（+14）为float32(h−len)/float32(count−5)，仅结果≤0才改1.0f，正的小数仍保留；
`1023E570–1023E58B`改基矩形为(x−30,−1000,125,2000)，直接进共同注册链，跳过普通滑块矩形分支。
21取x=J+170、h=189，原Margin／args保留；null分支不追加上述flags／grid或扩区。这里不是把grid无条件夹到至少1。
`DrawVerticalScroll2`在`1025189D`调用12后不检查返回值，随后`102518BF`仍请求25原基矩形、value0、flag4。
不能因滑块null就删除12／25或暗轨道。纯页面计划可保留具名请求供独立helper消费；未消费该边界时不得声称全部首行图形都已展开。
箭头实际是`10354DCE／10354E73`调用`GameView.AddTouch(Graphics,id18,x,y,SEB3,frame3/0,value16/18,option)`，
本分支不另画一次箭头SEB。[共享箭头消费者](PAGES.md#说明13固定正文输入消费与装饰随机)按SEB取得bounds后
实际画surface `buttoneffect.png`，并按触摸状态选独立TEX；common SEB3的身份不能直接替代最终图片。
手形最后画common SEB21当前帧，y=`K+33+37*(select-scroll)`；非日文x=J+68，日文x=J+71。
`10354F7F`为Seb.Draw而非固定帧DrawFrame，纯计划可沿现Facility的外部`finger_frame`输入，资源为common图70／SEB21 `finger_r`；
不能由page frame或frame2自行推导共同SEB当前帧，也不能让多次纯查询推进该动画。
行marker16与空行输入资格均已交叉Steam21 Update：空行可以被选中，确认直接返回、不进入BUILD。
这是注册、选择和确认三层合同，不能因注册存在就删除真实条目范围检查。

## 维护纯计划与产品消费

[steam_build_menu_skin](../prototype/include/dungeon_village_prototype/steam_build_menu_skin.hpp)已提供原序只读计划：调用者显式传入冻结三类目录、绝对选择／首行、五行内的定义／Mapchip／报价及当前译文测宽、住宅H／已建／入住谓词、分类NEW、J／K、共同手形帧与副钟。函数验证可见行身份及预算，按普通DrawMapchip展开固定朝向0的真实分片；不重新筛目录、不补Init、不扣款、不推进源绘制计数或共享动画。它不替代Owner目录资格和持久化校验。

所有操作保持原序消费，尤其是SetClip／ClearClip、住宅／NEW／入住叠图、名称和价格。特殊图片复用已出版资源；数字沿现number05，箭头以SEB bounds交现surface helper，不另画common箭头。滚动同时给出绘图、12／25的基矩形、Margin、参数及实际拖动flags／网格，null滑块不删除轨道或触区。计划不注册OS控件，输入、字体栅格及活动拖动仍需产品末端接入；负报价和空行继承颜色不由表现层改成免费或统一灰字。

[目录及返回图例](examples/steam-build-menu.png)来自8帧有界示例`build-menu-equipment/general/food/overlays`（实际命令逐个使用完整名称，不加`--world`）。前三态读取真实新局定义、余额和正式NEW刷新结果，补出源道路／撤除目录的只读投影；第四态只在显示输入中设全开放、移动、住宅H1、入住／NEW及余额0，并显示左箭头高亮。没有真实建设、自然解锁或原窗口认证。中文／Noto、绿色外背景及画布平移40属于研究适配。图中最下排另为当前`world-building-preview`／`world-building-return`两个真实新局Owner入口：从21选择实际定义24，进入74再返回，保留选择2／首行0；未注入人物、资金或解锁。旧四态显示夹具与这条实际消费者链分别登记。

## 下一闭合范围

当前Owner已保存21绝对选择／独立首行，复用类目／完整冻结目录与双钟，显式Draw及21→74返回也已接入；
窗口局部只保留marker／拖动手势，完整合同与版本提示差异见[业务合同](STEAM_BUILD_LIST_BUSINESS.md#软键详情与取消)。
源端仍需完整ChangeCurrentForm页栈退休、触摸value1生命周期、surface物理拖拽／命中优先级、当前语言实际命中、
建设落点后续事务、世界遮挡及完整SEB插值。已核21→74局部Pop与父字段保持不代替这些全局结论。
自然窗口另认证最终命中区、滚轮／键盘投递及空行输入；静态目录、pattern和旋转资格不重复列为待核。

早期受阻探针及有限入口历史保留于[旧专题工作包](../verification/steam-build-list-contract/README.md)；
未执行动作不计证据。现行结论以上述具名入口和后继合同为准，不将大型外层登记范围当作完整分析量。
