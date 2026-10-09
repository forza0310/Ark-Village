# 建设目录21：Steam局部合同与APK交叉边界

2026-10-08。本专题服务已确认的Steam 2.56 UI／操作还原目标；固定汉化重签APK1.0.8仍作为规则基线。
这里只读既有局部探针及冻结Java来源，不启动游戏、不改存档、不修改维护Owner或产品。
2026-10-09后继已从具名`_draw`入口闭合raw21局部分支的行绘制、图块调用、marker及滚动注册；
Steam与APK的行margin／类目命中矩形存在差异，分别登记。目录初始化／确认业务、最终OS命中及原窗口仍未全部闭合。

## 来源与证据范围

| 输入 | 身份／范围 |
| --- | --- |
| Steam GameAssembly.dll | SHA-256 `9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a` |
| Steam global-metadata.dat | SHA-256 `80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369` |
| Steam methods.json | SHA-256 `399156eff1f2623623d489b3e36ab9a83b9695d311c7c91e21b013dafc665c42` |
| Steam dump.cs | SHA-256 `1bf5680223c495b5d24aa5934831a17e9ecf03b95ba4ddb3ad4b9459c3c81912`；SubForm字段及TYPE_BUILD=21声明 |
| APK b/g.java | SHA-256 `0eecb47f1996abc34e6fa1ffcbbeeb40d2a597f46168a2583833ed6fe1e33081`；下列具名局部窗口 |

Steam身份、方法登记范围及哈希继承[既有UI证据](../work/steam-ui-coverage/README.md)，
对应[方法清单](../work/steam-ui-coverage/methods.json)与[检查记录](../work/steam-ui-coverage/CHECK.json)。
先用既有`analyze.cjs --inspect`消费SetTouchValue及Update_scrollValue有限指令窗口。
用户明确授权后，新增固定入口只读探针，消费外层draw、Update、Init2各1024字节入口，
合计唯一3072字节，范围／哈希见[入口摘要](../work/steam-build-list-contract/EVIDENCE.json)。
外层`SubForm._draw`登记范围197,616字节，本次仅核入口前段，尚未到21分支；不能宣称全部方法或21绘制已认证。
APK巨型Java方法的普通反编译警告仍有效，图块绘制已另以低层窗口交叉，见[图块合同](PAGES.md#固定apk候选建筑两朝向与目录裁剪)。

## Steam已证的页面字段与输入

`SubForm`metadata字段：`type_ +0x84`、`scrollValue_ +0x88`、`select_ +0x90`、
`page_ +0xD8`、`buildList_ +0x168`；`value1_ +0xB8`、`value2_ +0xBC`是其它字段。
以下映射由实际DLL消费者交叉，不能把类目索引写成value1_。

| 合同 | 实际机器码位置 | 已证内容／限制 |
| --- | --- | --- |
| 建设类目事件 | OnTouchEvent RVA0x3160B0；VA0x10316103–0x103161E5 | TYPE21、组件3且value高16位0x50000处理UP／ENTER；类目循环模3，并清scroll／select。名称及注册矩形未知 |
| 类目写入 | SetTouchValue RVA0x317600；VA0x10317612–0x10317626 | type高16位0x50000写page_；此方法本身不限定21页 |
| 列表选择写入 | 同方法VA0x10317653–0x10317667 | type高16位0x20000写select_；不证明21行已注册此value |
| 滚动写入 | 同方法VA0x10317668–0x1031767C | type高16位0x40000写scrollValue_；不证明21滚动组件绑定 |
| 通用选择跟窗 | Update_scrollValue RVA0x3284B0；VA0x103284E3–0x103285B0 | select不小于集合长度先归0；scroll高于select则下调；选择越过可见尾则上移窗；scroll过count−disp时归max(0,count−disp)，并夹选择到0…count−1。**21调用及disp实参尚未核** |

通用DLGSEL3／LIST10／SLIST11的marker16共享消费者已证：首次未标记UP选择并记key，
第二次同key UP发逻辑确认，没有点击间隔判定；键盘会清marker。
见[Steam输入合同](STEAM_INTERACTIONS.md#输入选中标记与确认分开)。
后继在`0x10354A8D–0x10354AF1`确认Steam21实际注册marker16，因而本页具备通用首次UP标记、再次同key UP确认资格；
它不是带时间阈值的OS双击。最终窗口鼠标事件与组件key仍需原窗口／surface单独核对。
Steam适配事件CANCEL=10；APK原始Android ACTION_CANCEL=3，两个事件层保持分开。

## APK21目录及特殊边界

以下是固定APK事实，Steam交叉栏未核部分保留缺口。坐标是原画布局部坐标，不是物理DPI热区。

| 项目 | APK局部合同／来源 | Steam交叉状态 |
| --- | --- | --- |
| 类目／开放集合 | [b/g:10647](../work/decompiled/sources/b/g.java:10647)：3个Vector；定义开放p非0、flags4，住宅kind12还需H>0，按定义j入类；第0类索引1插撤除−1，flag32开放后索引2插移动−2 | Steam只有三类输入消费已证；同集合生成待核 |
| 类目文字 | [b/g:9604](../work/decompiled/sources/b/g.java:9604)：设备／一般／饮食 | Steam独立metadata literal为設備／一般／飲食，最终中文译文仍走翻译 |
| 目录原点／外框 | [b/g:9591](../work/decompiled/sources/b/g.java:9591)：J=((c.m+c.o)/2)−87−画布偏移X，K=((c.n+c.p)/2)−104−画布偏移Y；外框175×210 | Steam独立核同局部公式，GameForm VIEW四字段及Graphics原点见下节 |
| 行／图块 | [b/g:9655](../work/decompiled/sources/b/g.java:9655)：最多5个实际条目；行距37；图块左上(J+10,K+25+37row)，64×32裁剪，RGB(190,242,230)，固定朝向0按原分片画 | Steam核同裁剪；实际DrawMapchip锚为(J+12,K+35+37row)，非直接整PNG |
| 特殊图标 | 撤除−1图片17偏移(4,3)，移动−2图片119偏移(1,0)；住宅H、NEW147及kind13提示81另叠加 | Steam图槽及真实字段已核，见下节；NEW不能一律用定义新开放位 |
| 报价 | [b/g:9688](../work/decompiled/sources/b/g.java:9688)：撤除0G、移动300G、普通定义报价h()；资金不足换价色，0G仍绘 | Steam调用GetBuildCost；0/300及资金比较已核，具体价格规则独立于绘制 |
| 每行输入登记 | [b/g:9730](../work/decompiled/sources/b/g.java:9730)：SLIST11，矩形(J+2,K+22+37row,165,37)，value=(scroll+row)\|0x20000，marker16，扩命中(10,10,0,0) | Steam同矩形与marker，但Margin为**(10,0,0,0)** |
| 类目输入登记 | [b/g:9742](../work/decompiled/sources/b/g.java:9742)：DLGSEL3，矩形(J+3+57cat,K+2,54,20)，value=0x50000\|cat，扩命中(0,0,30,0) | Steam为**(J+57cat,K+2,60,20)**，Margin同(0,0,30,0) |
| 滚动绘制实参 | [b/g:9740](../work/decompiled/sources/b/g.java:9740)：(J+170,K+21,189,count−1,5)传滚动helper | Steam显式宽4、高189；helper注册宽3的组件12与25，见下节 |

APK需要保留两个易被普通列表规则抹掉的边界：

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

2026-10-09。[新证据包](../work/steam-build-list-render/README.md)沿具名入口`SubForm._draw`从`0x10352E20`顺序解码8704字节，
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
后加内线及内容框；最外色RGB(79,72,48)，内线RGB(193,234,94)。这些仍是Graphics调用实参，未认证最终像素扩边规则。

三类literal在Steam metadata独立解析为`設備／一般／飲食`，不能凭日文literal推定当前中文显示。
标签文字锚在`(J+5+57cat,K+3)`，TextLayout高15、anchor0x22；正常宽55，
`UserData.isBuildNew[cat]`真时文字宽35，并在`(J+39+57cat,K+9)`画common图147。
非日文临时字号9，画后恢复；日文保留当前字体。类目选中/未选文字色来自SubForm静态色字段，未用近似RGB填空。

行号r固定遍历0…4；实际索引`i=scrollValue+r`。实际条目超界时在`(J+90,K+41+37r)`画`---`、anchor0x22；
并没有因此缩短后面的五行注册循环。选中可见行使用`select-scrollValue`，不能把它当定义ID。

令`P=(J+10,K+25+37r)`。合法条目先以RGB(190,242,230)填64×32再`SetClip(P,64,32)`，
图标／建筑绘完明确`ClearClip()`。它是图块预览裁剪，后续名称／价格不留在这个小裁剪内。

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
价格经`YEN("Ｇ",price)`，锚`(J+164,P.y+18)`、anchor4；0也显式画，非零价格按当前系统所持资金比较后取正常／不足两静态色字段。

### 建筑字段到mapchip图层

`MapchipData.DrawMapchip`在`0x1021BCA0–0x1021BF97`独立核实：mapchip定义ID不是PNG索引。
它先取`AppData.mapchipData_[id]`，再以该记录`tenantDataId_`反查设施定义；按该设施`mapchipPattern_`与传入朝向
遍历`TenantData.SETMAPCHIP_ADDRESS[pattern][orientation]`。分片地址(u,v)产生目标偏移`(30*(u+v),15*(u-v))`，
加到调用者锚点；资源组来自`AppData.resMapChip_ +0x20`，SEB索引取`MapchipData.seb_ +0x24`。
非道路的帧来自`SETPATTERN_MAPCHIP[pattern][orientation][piece]`；道路type6则朝向1用frame1，其它朝向用frame11。
最终为`Seb.DrawFrame(...,resource.images,frame,lineNo=-1)`，由SEB再关联真实图片和裁片。

本链没有将`MapchipData.img_`或`TenantData.icon_`直接拿来画整张建筑PNG。
本批闭合字段／调用链，但未解Steam这两个pattern数组的全部数值，也未认证所有mapchip图集分片；不能借APK数组填成Steam已核。
[资源摘要](../work/steam-build-list-render/EVIDENCE.json)核7个直接图／SEB槽与已发布默认文件字节一致，并追5个SEB图片依赖：
arrow02／arrow01／finger_r同字节，**number05图103与tenant_resident图98的Steam字节不同于APK发布副本**。
两个SEB相同不证明其图片也相同；这两张不能直接alias APK，后续需独立Steam解析／发布。
common图147的INF仍有`20x9`修饰，语言覆盖和最终Image装载另有资格，不把文件hash相同当运行时语言路径已定。

### 实际marker、滚动和箭头注册

| 调用位置 | Steam实际注册参数 |
| --- | --- |
| `0x10354A70–0x10354B01` | 固定5行SLIST11，矩形`(J+2,K+22+37r,165,37)`，value=`0x20000|(scroll+r)`，`TouchOption.Create(16).Margin(10,0,0,0)` |
| `0x10354C81–0x10354CFD` | 固定3类DLGSEL3，矩形`(J+57cat,K+2,60,20)`，value=`0x50000|cat`，默认option再Margin(0,0,30,0) |
| `0x10354C55–0x10354C71` | `DrawVerticalScroll2(g,J+170,K+21,4,189,scroll,count-1,5)` |
| 滚动helper `0x1025187E–0x102518BF` | VSCROLLBAR12：同x/y、宽3高189、value0x40000，option args=`[count,5,0x20000]`及Margin(3,20,0,0)；再注册SCROLLBG25同矩形、value0、FLAG_MAX_PRIORITY4 |
| `0x10354D87–0x10354E73` | 左右箭头使用组件18、common SEB3 frame3／0，value16／18；y=K+9，x分别为`30-trunc((frame2%20)/5)`、`207+trunc((frame2%20)/5)`；**x未加J**，Margin分别(40,−10,20,−10)与(−10,40,20,−10) |

滚动helper本身在此小方法内只注册组件，不直接画滚动条；最终surface绘制／拖拽仍需后继消费，不能把helper名当已画出滚动图。
手形最后画common SEB21当前帧，y=`K+33+37*(select-scroll)`；非日文x=J+68，日文x=J+71。
行marker16现在有真实注册证据，但空行的确认资格仍要交叉Steam21 Update，不能从注册存在推定空行可建或删除有效边界检查。

## 下一闭合范围

后继优先交叉真实Init和Update的21专用分支、通用surface滚动／marker最终消费，以及Steam mapchip两组pattern数组。
上述raw21绘制局部已从入口闭合，不重复当作待定位；仍不能把APK专用空行策略或初始目录直接当Steam事实。
每个窗口须有已核指令边界、方法登记范围、地址／哈希及预算；大型外层不得按完整方法覆盖报告。
自然窗口仍需在原程序单独认证最终命中区、滚轮／键盘投递及空行输入，不由静态事实替代。

历史非入口续窗被自动审批拒绝的现场保留于[旧专题工作包](../work/steam-build-list-contract/README.md)，没有执行该被拒方案。
本次沿后来已核的具名入口顺序解码前缀完成安全替代，只保存固定入口脚本和有限事实摘要；不写指令全文、原游戏档或构建缓存，无后台进程。
