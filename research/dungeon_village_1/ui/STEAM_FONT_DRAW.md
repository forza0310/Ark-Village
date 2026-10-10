# Steam文字绘制：锚点、阴影与GUI坐标

2026-10-10。本页接[字体选择与测宽合同](STEAM_FONT_CONSUMER.md)，闭合Graphics普通文字请求的局部落点及阴影调用顺序。固定DLL／metadata、9个具名方法共8,064字节、68个指令／调用锚见[研究包](../work/steam-font-draw-contract/README.md)和[EVIDENCE](../work/steam-font-draw-contract/EVIDENCE.json)。没有指定当前中文字体名称，也没有把GUI矩形y当作字体的实际字形基线。

[本次原窗口反馈](../work/skin-observation-analysis/ANALYSIS.md)观察到白色阴影标题等外观，但JPEG、1067×910外框及未知DPI不能反推原Font测宽、RGB或比例。本页公式来自原代码，与该窗口观察分开认证。

## 入口及普通文字边界

`Graphics.DrawString(string,x,y)`（RVA0x766290）转入`DrawString(TextFormat,string,x,y,anchor)`，传TextFormat=null、**anchor=0x11**。显式anchor重载（0x7678C0）原样传入anchor。常量为LEFT1、CENTER2、RIGHT4、TOP16、MIDDLE32、BOTTOM64；默认0x11即左／上。

主实现RVA0x7663D0先处理null空文本和`Language.TranslateText(text,true)`。自动TextLayout开启且文本含`<`，或静态lineSpacing非0且包含换行时，转入TextLayout.Draw；本页以下普通GUI路径不替代这些标签／换行消费者。

非日文且有TextFormat时，可能临时设其指定字号，再调用`GetX/GetY/GetAnchor`修改输入；具体TextFormat字段解释仍由该接口负责。没有显式格式字号而原`IsNeedLocalizeScale`成立时，按前页公式临时缩放字号；若anchor不含MIDDLE／BOTTOM（`anchor & 0x60 == 0`），先把y加上：

```text
float32(旧usedSize − 新usedSize) × float32(0.5)
```

然后锁住本地化设置，防止内层再次缩放。这里是一次文字请求的临时调整，不把Font的业务请求字号永久改小；原代码后续有恢复／解锁清理，具体异常清理跳板不在本批给未命名入口强行命名。

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

最后的GUI位置为：

```text
(PX,PY) = screenMatrix.MapPoints(锚点X,锚点Y)
(GUIX,GUIY) = (PX − clipRegion.left, PY − clipRegion.top)
```

先锚点、后当前screenMatrix、再减整数裁剪区域左／上。不能直接从窗口截图宽高求一个倍率替代这条路径，也不能在矩阵已经包含平移时再加世界坐标。

## GUI字号和矩形不是字形基线

短方法`Font.GetWidth`（RVA0x77D980）确实返回`float32(U)×0.5`。这只是原Font的基准宽度，**不是任意字符串宽度W**。

代码把逻辑矩形`(left0,top0,rightU/2,bottomU)`交`GetTransRectGUI`，取转换后的宽A和高B。该helper分别MapPoints左上和右下两点，再减clipRegion.left/top；它没有遍历四角重算旋转包围盒。正常正尺寸路径计算：

```text
sx = A / (U/2)
sy = B / U
GUIStyle.fontSize = trunc(float32(U × min(sx,sy) + float32(0.001)))
```

0.001是位型`0x3A83126F`的float32，不是精确十进制或四舍五入。GUIStyle字体对象／样式有`fontKeep_`缓存分支；实际字体来源与空font回退仍沿前页边界，不因此认证当前中文字体已定位。

非font-texture且非glRender路径先调用UpdateGUIGroup，转换文本`UnicodeToChanakya`，最终`GUI.Label`收到矩形`(GUIX,GUIY,1,2×B)`、文本和GUIStyle。宽1是此原调用矩形参数，不能推成“文字只绘制一像素宽”：实际overflow、padding、clip和字形排版还由GUIStyle／GUI组决定。

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

所以普通支有`3×(Q+1)+1`次文字调用，不是单一右下偏移，也不是描四周边框。每次调用复用上述字体／锚点／矩阵消费者；外层已经做完TextFormat修正，传给内层的TextFormat为null。必要时尾部解除本地化锁并恢复原Font.size。D、Q的当前默认值和设置时机本批没有追上游，不能用“通常1像素”替代输入字段，也不把标签Shadow2分支自动认证。

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
