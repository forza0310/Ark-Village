# Steam设施页共用文字、滚动注册与数字绘制

2026-10-10。为[详情74](STEAM_FACILITY_DETAIL.md)和[升级81](STEAM_FACILITY_UPGRADE.md)补实际调用的共用helper；[工作包](../verification/steam-facility-draw-helpers/README.md)只读固定Steam具名入口，不改旧合同／证据或维护实现。本页只描述原逻辑实参和请求顺序，不认证OS最终坐标、中文字体或整套渲染框架。

## Draw_btmMsg：可选的背景、触摸与手形

74使用重载RVA0x30A2C0，参数为(g,str,dy,flgSelect,flgCursor,widthMax)。先取当前Font，对**传入原str**执行StringWidth，再ClampMax到widthMax。这里没有预先LT调用、强制字号或字符数估宽；翻译／文本标签的后端行为沿既有[字体合同](STEAM_FONT_CONSUMER.md)。

令w=min(原Font测宽,widthMax)、W=w+16、L=120−trunc(W/2)，原顺序为：

| 条件 | 实际输出 |
| --- | --- |
| flgSelect=true | SC_WINDOW_SELECT橙色FillRect(L,dy,W,16)；随后注册Touch组件4，value20 |
| 无论是否选中 | 重设SC_WINDOW_BLACK；TextLayout.Draw(str,L,dy,W,16,currentColor,lineSpace0,anchor0x22) |
| flgCursor=true | common SEB21 finger_r，Draw当前帧、lineNo−1，锚(L−4,dy+9) |

触摸矩形来自SubForm静态区+0x134的第0行，已从原metadata blob独立核为**[−20,−20,40,40]**，所以注册(L−20,dy−20,W+40,56)。不是视觉橙底的矩形，也不是任意全屏确认面。widthMax=200限制的是测宽后的w，橙底最大宽216、注册最大宽256；不能把200直接当最终按钮宽。

flgSelect=false不注册该组件，也不画橙底；flgCursor与它独立，原helper允许只画手形。手形使用SEB当前帧，并没有在这里依据页面frame选择固定frame0。它的动画推进／退休属于资源与渲染调度，不能单凭本helper宣称每次调用永远相同帧。

本方法不自行ClipRect、不更改页选择、不扣费或推进Owner；但注册输入组件、设置Graphics颜色和提交SEB绘制是真实副作用，不能当纯测量查询。74第二页传false/false时只画说明文本；第一页使用道具、商品、入住提示所传true/true才会产生橙底／输入／手形。

## DrawVerticalScroll2：实际是两项组件注册

RVA0x2517B0的全部304字节范围已核。它没有DrawRect、FillRect、SEB、字体或直接轨道／滑块绘制调用，也**没有读取value实参**。不应凭函数名实现一个自算滚动拇指然后认证成Steam。

调用顺序为：

1. 新建int[3]，内容[maximum+1,largeChange,0x20000]，构造TouchOption，再Margin(3,20,0,0)。
2. 注册组件12，矩形(x,y,w−1,h)，value=0x40000，上述TouchOption作为附加参数。
3. TouchOption.Create(4)，注册组件25，同一矩形，value=0。

74传maximum=count−1、largeChange=5，因此附加参数还原为[count,5,0x20000]；几何实参(221,85,4,110)变成两个注册的(221,85,3,110)。空表count=0仍注册，helper没有“无需滚动则不绘／不注册”门槛；它也不自己夹scroll、最大项或可见数。21目录复用同一合同。

最终轨道显示、margin如何扩展命中、组件12/25的优先级及拖动路径继续在GameView/Surface消费者，当前仅复用既有输入研究，不为闭合这个局部扩扫框架。不把新增的int[3]与TouchOption生命周期叫作世界泄漏，也不宣称原框架注册池永久有界。

## 普通DrawNumImage与带逗号版本是不同算法

74／81的普通入口RVA0x24FCB0将header/footer都设为空串，委托RVA0x24FD00。后者从所选common SEB.GetSprite(frame0,line0)取SP_W=索引4作为**数字步宽**，没有Font测宽。padding表示字间间隔，不是左补零数量。

非负value的位数至少1；宽度为(header长度+十进制位数+footer长度)×(SP_W+padding)−padding。anchor先检查bit2，命中按向零截断的半宽居中；否则检查bit4，命中右对齐；其他保持x。**这是位判断，anchor6以居中优先**。

先header、再数值、再footer。header/footer的空格只前进不绘图，其余字符请求SEB frame=字符码−'0'；正数按十进制除数逐位请求frame0–9。每项锚x按SP_W+padding前进，y不变，lineNo−1。这不支持任意中文header或footer文字，它们不是字体字符串输出。

负值不经过绝对值或“-”字符排版：用于计算位数的value/10为非正，位数循环保持1；之后数值除法结果仍直接传SEB frame。它不能与下面“signed串逐字符”的负号路径混为一谈。负数和极端signed64下最终SEB像素效果未在本批完整认证，不通过改符号、取绝对值或夹frame补成合法显示；合法游戏数值准入与维护坏载荷拒绝另由Owner合同承担。

## DrawNumImageComma的signed串、逗号和anchor

RVA0x24F760复用此前[Steam数字差异证据](ATTRIBUTE_GAIN_RENDER.md#steam具体消费者相同合同与数值差异)，本批重新核源字节。它固定使用8的数字步距，不读取上述SP_W。令N为value原signed十进制字符串长度，p为padding，c为comma_padding：

```text
width = N*(8+p) + trunc((N-1)/3)*c - p
anchor == 2: x -= trunc(width/2)
anchor == 4: x -= width
其他anchor: x不变
```

因此其anchor是精确值判断，anchor6不等于普通DrawNumImage的bit2优先。N含负号，不能用GetFig(abs(value))替代宽度。

CommaSeparate先对负数做signed64取负，对绝对值原串从右每3字符插逗号，再补负号；LONG_MIN取负溢出的原边界未额外修正。绘制从左处理：遇逗号只置pending标志，不立即前进；下一非逗号字符先增加c，再画frame=字符码−'0'；若pending，再在该字符x−2画frame10，之后清pending。最后x前进8+p。**原顺序是后一个数字先画、逗号随后覆盖**。

负号'-'请求frame−3；此处记录请求，不将APK末端空绘策略当作本批Steam完整负帧像素事实。当前74/81传p=0、c=0，逗号覆盖不新增数字间距，不能用字体千分格式化后的测宽替代。

## 金额与加号的最终锚点

Draw_money RVA0x255210先令X=dx−9，调用DrawNumImageComma(value,X,dy,p0,c0,anchor4)，再用**同一SEB**在(X,dy)画frame20货币符号。不是数值右端仍在dx、货币另挪9；两者实际锚都是X，内部裁片offset另算。

Draw_plusValue RVA0x255410在(dx,dy)画同样的逗号数值，再取GetFig(value)，在(dx−(位数+1)×8,dy)画frame14。这个helper**无条件画加号**，包括0和负数；外层81用差值≠0控制是否调用，不能把外层守卫误抄到helper里。GetFig对signed64做绝对值后计十进制位数，0为1；int增量扩展到long可覆盖INT_MIN，但LONG_MIN绝对值仍为原溢出边界，本批不人为修成19位。

以上金额／加号都不测Font。Steam加号不同于APK依据signed原串Font测宽的定位，双方来源保持独立，未改维护APK实现或有效断言。81价格差之后额外画货币frame20，也是外层实际调用，不能因为名称plus就自动加货币。

## 资源与验收限制

消费common SEB12/15/16和手形SEB21；图片103使用[Steam建设差异包](../assets/steam-build-common/README.md)，图片105用[详情差异包](../assets/steam-facility-common/README.md)，106和70沿已核同字节原副本。8项引用核SHA，不新增图像。SEB裁片／偏移沿前批已核资源，不把PNG列数当动态帧数。

本批9个方法共3744字节，其中4个新方法2144字节，既有数字／滚动5方法1600字节复核；37机器码锚与1个16字节metadata原margin。没有原窗口、逐像素比较、完整TextLayout标签／压缩、负帧SEB后端或新维护代码测试。方法输出由将来的只读计划与表现消费者承接，Owner只负责逻辑身份；不对全部历史与渲染池作永久有界承诺。
