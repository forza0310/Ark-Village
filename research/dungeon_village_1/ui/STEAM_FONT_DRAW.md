# Steam文字绘制：锚点、阴影与GUI坐标

2026-10-10。本页接[字体选择与测宽合同](STEAM_FONT_CONSUMER.md)，闭合Graphics普通文字请求的局部落点及阴影调用顺序。固定DLL／metadata身份见[证据入口](../EVIDENCE.md)，9个具名方法共8,064字节及68个指令／调用锚的消费者归于本页。没有指定当前中文字体名称，也没有把GUI矩形y当作字体的实际字形基线。

[本次原窗口反馈](../verification/skin-observation-analysis/ANALYSIS.md)观察到白色阴影标题等外观，但JPEG、1067×910外框及未知DPI不能反推原Font测宽、RGB或比例。本页公式来自原代码，与该窗口观察分开认证。

## 入口及普通文字边界

`Graphics.DrawString(string,x,y)`（RVA0x766290）转入`DrawString(TextFormat,string,x,y,anchor)`，传TextFormat=null、**anchor=0x11**。显式anchor重载（0x7678C0）原样传入anchor。常量为LEFT1、CENTER2、RIGHT4、TOP16、MIDDLE32、BOTTOM64；默认0x11即左／上。

主实现RVA0x7663D0先处理null空文本和`Language.TranslateText(text,true)`。自动TextLayout开启且文本含`<`，或静态lineSpacing非0且包含换行时，转入TextLayout.Draw；本页以下普通GUI路径不替代这些标签／换行消费者。

非日文且有TextFormat时，可能临时设其指定字号，再调用`GetX/GetY/GetAnchor`修改输入；具体TextFormat字段解释仍由该接口负责。没有显式格式字号而原`IsNeedLocalizeScale`成立时，按前页公式临时缩放字号；若anchor不含MIDDLE／BOTTOM（`anchor & 0x60 == 0`），先把y加上：

```text
float32(旧usedSize − 新usedSize) × float32(0.5)
```

然后锁住本地化设置，防止内层再次缩放。这里是一次文字请求的临时调整；下面补核恢复的具体顺序和保护范围，不能将其概括为完整Font对象回滚。

### 临时字号与本地化锁的清理

2026-10-13补核普通DrawString的直接调用helper RVA75CC00（VA1075CC00–1075CC63），不为未具名helper编造原函数名。调用者保存的是旧请求`Font.size_`，不是`usedSize_`或整个Font：旧字号槽初值−1（10766606），TextFormat确实改变字号时在10766662–65保存，本地化缩放时在1076672F–32保存；无临时修改则保留−1。本次是否取得本地化锁由独立布尔槽记录，初值false，只在`IsNeedLocalizeScale`结果为真时设定并调用Lock（10766713／107667B0）。

helper从调用者传入的三个局部引用读取Graphics、本次锁标志和旧请求字号，严格按以下顺序执行：

1. 调用当前Graphics的`EndRenderMatrix` RVA7690D0。
2. 仅本次锁标志为true时，调用`Font.UnlockLocalizeSetting` RVA77EB30。
3. 仅旧字号不是−1时，对**当前Graphics.font_**调用`SetSize(旧size_,true)`；并不恢复此前的`usePreset_`或直接写回旧`usedSize_`。

因此字号恢复仍经过[字号／preset合同](STEAM_FONT_CONSUMER.md#请求字号preset与usedsize)。例如进入前显式`SetSize(1,false)`、本次TextFormat实际暂改字号时，尾部恢复会以preset1重算为24（静态100%前提），不能擅改成恢复显式1。Lock／Unlock两个具名方法只将Font静态+18布尔字段置true／false（1077DBF9／1077EB69），不是计数锁，也不操作`PushLocalizeSetting`的布尔设置栈；未取得本次锁的内层调用不会在这个helper中解开外层锁。

正常GUI支在10767504／10767528、无目标Image支在10767531、Image文字任务完成后均汇入107677BF，107677C6调用同一helper，再由107677E8结束Profiler采样。已核异常续体10767801–10767829也在10767819调用helper，随后检查并重新抛出保存的异常；外层续体10767839–1076786A另负责EndSample及重抛。这证明已见受保护绘制分支会尝试上述清理，**不证明整个请求的异常事务性**：临时设字号／Lock和BeginRenderMatrix都位于内层保护记录建立（107667CA–107667F3）之前，helper自身若在EndRenderMatrix或SetSize抛出也没有继续全部清理的保证。已排入GLText／Image的任务、已绘制像素、GUI.skin.font及颜色另有各自消费者；本helper没有撤销它们。标签／换行转TextLayout的早返回不经过此普通分支，仍须独立核查。

## 逻辑锚点先于屏幕矩阵

普通入口先调用`BeginRenderMatrix(ref x,ref y)`，再进入`isGUI_`分支的锚点计算。该函数不是普通无副作用加法：

- 没有局部矩阵（localMatrixIndex<0）且没有局部坐标（localCoordinateCount≤0）时，返回false，保留x/y。
- 有相关上下文且输入不全为0时，保存原世界矩阵；存在局部坐标则交给`BeginLocalCoordination(x,y)`，否则对世界矩阵PreTranslate(x,y)并UpdateScreenMatrix，随后把传入x/y置0。
- 输入x/y均0时走独立栈标记分支。矩阵／局部坐标栈内部维护仍以原接口为边界，不能先把所有位置手动缩放后又让原矩阵重复平移。

设经过上述前置操作后的x/y为X/Y，当前Font.usedSize为U，当前实例Font.fontScale为F；文字测宽为W。GUI锚点的原优先顺序：

| 方向 | 条件（按此顺序） | 锚点结果 |
| --- | --- | --- |
| 水平 | anchor含CENTER2 | X−W×0.5 |
| 水平 | 否则含RIGHT4 | X−W |
| 水平 | 其它 | X |
| 垂直 | anchor含MIDDLE32 | Y−float32(U)×0.5 |
| 垂直 | 否则含BOTTOM64 | Y−float32(U) |
| 垂直 | 其它且fontScaleOffsetEnable=true | Y＋float32(trunc((trunc(U×100/F)−U)/2)) |
| 垂直 | 其它 | Y |

W来自Font RVA0x77E770的共享入口；methods索引中`StringWidthF(string)`与`StringWidthF2(string)`共用该机器码地址，不能凭反汇编打印最后一个别名强行择其一。它是字体测宽结果，不是字符数乘半个字号。上述F相关项先整数除法再整数除2，按向零截断；不能改成完整浮点除法，或与前一节本地化字号差补偿合并舍入。

锚点尚不是最终映射点。完成下节字号设置及非等比变换的额外Y补偿后，GUI位置才为：

```text
(PX,PY) = screenMatrix.MapPoints(锚点X,补偿后Y)
(GUIX,GUIY) = (PX − clipRegion.left, PY − clipRegion.top)
```

先锚点和字号／Y补偿、再当前screenMatrix、最后减整数裁剪区域左／上。CENTER／RIGHT才分别真实调用StringWidthF，CENTER优先（10766804／10766820／10766864）；其它水平位不测宽。不能为统一入口无条件测宽，也不能在矩阵已经包含平移时再加世界坐标。

### DrawText宽度重载的垂直锚定

Graphics.DrawText(string,x,y,width,anchor) RVA7688A0转7688E0；10768B37读取Font.usedSize。anchor含0x60且实测字宽大于width时先逐号缩小临时字号，最低3（10768B44–10768BC7）；再以实际usedSize处理垂直锚：MIDDLE32先y减usedSize×0.5，否则BOTTOM64减usedSize（10768BDB／10768C12–10768C33），去掉相应垂直位后将高度usedSize交TextLayout.Draw RVA7CABD0。0.5原float常量位于10DDE528。研究窄执行器已消费此宽度重载的缩字号／垂直锚，不把零高占位当作没有垂直处理；完整TextLayout换行／插件仍不是这一局部修正的认证范围。

## GUI字号和矩形不是字形基线

短方法`Font.GetWidth`（RVA0x77D980）确实返回`float32(U)×0.5`。这只是原Font的基准宽度，**不是任意字符串宽度W**。

代码把逻辑矩形`(left0,top0,rightU/2,bottomU)`交`GetTransRectGUI`，取转换后的宽A和高B。该helper分别MapPoints左上和右下两点，再减clipRegion.left/top；它没有遍历四角重算旋转包围盒。正常正尺寸路径计算：

```text
sx = A / (U/2)
sy = B / U
M = min(sx,sy)
P = float32(float32(U) × M)
GUIStyle.fontSize = trunc(float32(P + float32(0.001)))
Y = float32(Y + float32(float32(float32(U) − float32(P/sy)) × 0.5f))
```

0.001是位型`0x3A83126F`的float32，不是精确十进制或四舍五入。最后一行是10766B6E–10766BA2的额外Y补偿，**使用尚未截整的P**，不能用已写GUI字号逆推或在非等比矩阵下删除。比如U12、A6、B24时sx1／sy2、P12，Y另加3；这只是源公式条件输入，不是原窗口尺寸测量。

fontKeep_为false时，先真实Font.GetFont→GUIStyle.font、再GetFontStyle→GUIStyle.fontStyle，最后置fontKeep=true（10766AB1–10766B12）；为true跳过这两组调用，**每次仍写fontSize**（10766B35–55）。补偿后先把GUI.skin.font与style.font同步（10766BB4–C56），才MapPoints并减clip（10766CCD–D64）。实际默认字体与缺字供给见[字体后端](STEAM_FONT_BACKEND.md)，未认证当前会话中文最终face。

随后保存GUIStyle.normal.textColor（10766D7A–D9F）。仅renderOperator为0／1且renderSrcRatio<255时再读取一次当前normal色并临改：0对RGB各执行`float32(float32(ratio×通道)/255)`、保留alpha；1只对alpha同式处理（10766DA3–EE4）。不把这次读取替换成先前保存值，也不自行把负ratio夹零；其它operator或ratio≥255不作这次临改。正常绘制后仍恢复保存色。

**先**`UnicodeToChanakya`（10766F05），再分字体纹理、GLText及普通GUI路径。二者均否时才`UpdateGUIGroup`（10766F3A），最终`GUI.Label`收到矩形`(GUIX,GUIY,1,float32(2×B))`、已转换文本和当前GUIStyle（10766FEC）；原“先分组再转换”叙述据此更正。宽1是原调用矩形参数，不能推成文字只绘一像素；overflow／padding／clip和实际字形仍交GUIStyle／引擎。正常尾先恢复normal色，再将非空GUI.skin.font清null（10767480／107674A2–D9），不恢复进入前font；最后才到前述外层矩阵／本地化锁／请求字号清理。

[普通GUI执行接口](../prototype/include/dungeon_village_prototype/steam_font_draw.hpp)接已完成上游翻译、TextFormat、临时字号及BeginRenderMatrix的真实上下文，按上述顺序调用显式Backend，最终执行Label及正常清理；保留条件测宽、fontKeep、两次颜色读取和字符转换结果。接口仅接实际isGUI=true、useFontTexture=false、glRender=false的获准分支，不能把Image或GLText当普通GUI重放。后端供实际Font、矩阵、GUI对象／分组和绘制，接口不安装字形、写Owner、抽随机或增加持久状态。维护正常域U1..65536、文本转换前后各≤4096字节、输入及映射点有限且绝对值≤65536、参考尺寸(0,65536]、有效正比率；TOP字号补偿才要求F非0。clip原点允许全int32，最后Label坐标不受65536限制，高可至131072。退化／非有限及转换超预算明确拒绝，不夹分母；后端可能已改样式或画图，拒绝／异常不回滚源副作用、不能盲重放。外层EndRenderMatrix／锁／请求字号恢复仍由实际调用者负责；这不是原请求整体事务保证或原生字形等价认证。

本批认证到GUI.Label参数和调用边界；字形ascent/descent、真正baseline、kerning、抗锯齿及系统fallback仍未认证。`Font.GetOffset()`在前页已核返回公式，但本主方法没有直接调用它；不能在这里自行再加一遍requestFontOffset。它的其它间接消费位置仍需另批研究。

GLText队列、字体纹理GetFontTexture／GetFontTextureBold、离屏TextRenderTask有独立分支；本页只记录存在，不把普通GUI矩形公式未经检查移植为全部分支的最终像素坐标。

## 普通文字阴影是多次偏移绘制

`DrawStringShadow(TextFormat,text,x,y,color,anchor)`（RVA0x765E60）先保存当前Graphics.color；参数color是随后绘制影子的颜色，正文仍使用原保存色。先翻译文本；自动TextLayout且含`<`时转`DrawStringShadow2`，本节不覆盖该标签分支。

普通支先按TextFormat／本地化规则处理字号和坐标，临时锁定本地化设置。令Q为静态`outlineQuality_`、D为`outlineWidth_`，正常Q≥0时，i从1至Q+1，按float32运算：

```text
delta = float32(i) × D / float32(Q+1)
以参数color依次DrawString(text,x+delta,y,anchor)
                      DrawString(text,x,y+delta,anchor)
                      DrawString(text,x+delta,y+delta,anchor)
恢复进入函数时的Graphics.color
DrawString(text,x,y,anchor)
```

所以普通支有`3×(Q+1)+1`次文字调用，不是单一右下偏移，也不是描四周边框。每次调用复用上述字体／锚点／矩阵消费者；外层已经做完TextFormat修正，传给内层的TextFormat为null。必要时尾部解除本地化锁并恢复原Font.size。D／Q上游现已补下述有限静态链，不用“通常1像素”替代实际输入，也不把标签Shadow2分支自动认证。

### 阴影参数的静态初值与Splash后继

2026-10-13独立核Graphics.cctor RVA775990：VA10775DEE将static+2C写float32位型0x3F800000，即D=1.0；cctor未写static+30，Q采用静态零初值0。`SetOutlineWidth` RVA7748E0／`SetOutlineQuality` RVA774890各直接写对应字段，无夹限／归一；getter76E1C0／76E180与字段对应。不能从调用者可写任意值推原函数会拒绝坏值。

原Splash还会改变宽度：独立setter helper RVA7E0410–7E0457写同一D；SplashForm.Update RVA7F14B0在VA107F1948传0x3F000000、194F调用，即D=0.5。state0在frame>60时转9(READY)并进入共享192A分支清frame、设置D；state11的Resume成立后转12也经过该分支。state9路径107F1B6F切nextForm并移出栈，没有再改D。

因此通过该已核启动路径且未有后继改写时，普通阴影使用D0.5、Q0：三次(0.5,0)／(0,0.5)／(0.5,0.5)逻辑偏移再正文。这里的0.5是原逻辑输入，不等于最终屏幕半像素，仍须经过上述字体与矩阵／clip链。具名setter的有限直接调用扫描不能证明不存在内联或间接writer，也未读取当前原进程参数；不能声称所有原窗口始终是这个值。原默认白标题阴影与当前中文glyph对象是两项不同证据。

## 局部裁剪的整数边界

`Graphics._setClip(x,y,w,h)`（RVA0x77BC40）先调用CheckClip，随后将四个float原参数保存到clipRect，再调用整数Rectangle.Set：

```text
left   = trunc(float32(x     + float32(0.01)))
top    = trunc(float32(y     + float32(0.01)))
right  = trunc(float32(x + w + float32(0.01)))
bottom = trunc(float32(y + h + float32(0.01)))
```

0.01为float位型`0x3C23D70A`；先按原float阶段相加，再提升并向零转整数，不是floor，也不是round。对负坐标尤其不能替换截断方向。这里输入是否已由外层SetClip转换是上游责任，函数本地没有再乘屏幕比例。

CheckClip读取clipEnable；关闭时可能按原警告条件记日志／递增警告计数并返回false。`_setClip`不拿返回值提前退出，仍更新上述字段；因此“clipEnable=false”不能被实现成“不存裁剪矩形”。反过来，保存裁剪区域也不单独证明所有图元已被硬件裁剪。

这些原请求、临时状态和坐标接口可用于研究皮肤接线；本包未写维护渲染代码、未运行游戏或导出字体。父窗口的完整裁剪栈、实际DPI／屏幕矩阵、原字体字形基线及原窗口像素验收分别保留。

## 具名入口范围

| 原方法 | RVA | 有界方法字节 |
| --- | --- | ---: |
| 默认／显式anchor DrawString | 766290／7678C0 | 各64 |
| DrawString主实现 | 7663D0 | 5360 |
| 普通DrawStringShadow | 765E60 | 1072 |
| CheckClip／_setClip | 75DA80／77BC40 | 224／288 |
| Font.GetWidth | 77D980 | 32 |
| BeginRenderMatrix | 75D570 | 592 |
| GetTransRectGUI | 76F4A0 | 368 |

这些窗口核到普通GUI、阴影、锚点和局部裁剪，4个float常量及16个字段／anchor映射支持上述公式；GLText、纹理、离屏和TextLayout标签阴影没有因同属方法体而自动认证。截图只作外观参照，不推实际语言返回值、Font对象、DPI或客户端比例。源方法范围是静态证据范围，不是运行内存有界保证；临时字体／颜色／矩阵状态清理也不替代世界随机及存档恢复。
