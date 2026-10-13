# Steam 字体选择、缩放与测宽消费者

2026-10-10后继[文字绘制合同](STEAM_FONT_DRAW.md)补普通锚点、字号补偿、矩阵→裁剪原点→GUI.Label次序及普通阴影；当前中文实际Font、字形baseline和插件内部仍未认证，窗口字形外观不代替它们。

2026-10-09。本页是固定 Steam 2.56 研究副本的局部静态事实，不是 APK 字体合同，也不是运行时字体采样。沿[嵌入字体身份](STEAM_STARTUP_RESOURCES.md)核了12个具名方法入口、6个字符串使用槽、7个float常量及资源目录。固定输入身份见[证据入口](../EVIDENCE.md)，来源与解析限制合并于本页。未运行游戏、读取原档、导出字体或修改产品。

结论：**默认字体按语言分层查找资源；当前资源目录没有中文专用或通用 default 字体。不能因为内嵌 M+ 有大量字形就认定中文窗口使用它。** GUIStyle 默认／系统字形回退仍须另核，当前只能闭合到传入 Unity 的字体对象边界。

## 默认字体资源链

`Font._getDefaultGUIFont`（RVA `0x77F4A0`）首先比较缓存的原语言码与`Language.GetLanguageCode()`，相同时直接复用当前对象；语言变化才清当前默认对象并重新查找。非空语言先`ToLower()`、`Trim()`，再按`-`拆分。

查找仅在当前对象为空时继续，顺序为：

1. 至少两段时：`Fonts/default-<第1段>-<第2段>`。即便有第三段，这个分支也只拼前两段。
2. `Fonts/default-<第1段>`。
3. 若`Language.Japanese()`成立，尝试`Fonts/default-ja`。
4. 尝试`Fonts/default`。

空／null语言跳过前两步；并非无条件指定`default-ja`或`default-en-switch`。资源加载结果显式检查为Unity Font对象；类型不符走异常路径，不能当作“任何同名对象都行”。语言变化尾部更新缓存语言并调用`TextLayout.ClearCache()`。旧／新字体名称还经过`invalidFontNames_`的泛型helper；本批未给这些共享泛型helper强行命名Add／Remove，不能从字段名推完整名单维护算法。

ResourceManager（`globalgamemanagers` pathID13）首个资源表共1251行。字体路径实际每组各有Font、Material、Texture2D三个引用；路径重名合法，不能建立单值name字典覆盖掉Font。外部文件表已经核`file_id=4`为`resources.assets`：

| 资源目录路径（小写登记） | Font pathID | 已核字体内部family | 同路径其它对象 |
| --- | ---: | --- | --- |
| `fonts/default-en-switch` | 1178 | M+ 1p medium | Material 1、Texture2D 14 |
| `fonts/default-hi` | 1181 | Kairo Hindi | Material 4、Texture2D 64 |
| `fonts/default-ja` | 1180 | m_plus | Material 3、Texture2D 55 |
| `fonts/default-th` | 1179 | Noto Sans Thai | Material 2、Texture2D 39 |

该已核目录没有`fonts/default`、`fonts/default-en`、`fonts/default-zh`、`fonts/default-zh-cn`或`fonts/default-zh-tw`。例如**若实际语言码为zh-cn且Japanese为false**，上述链会尝试中文地区／基础／通用三个缺失路径；不能改判成英语Switch字体。原生代码请求的首字母大小写与目录登记不同，Unity实际资源解析及所有可能运行时注入不由这张目录单独证明。当前未监测真实窗口的`GetLanguageCode()`返回值，条件例子不等于已观测中文运行对象。

`Font._init`（`0x77FC90`）在缺GUIStyle时构造一个新GUIStyle，然后把`GetDefaultGUIFont()`的结果传给`GUIStyle.font`，包括结果为空的路径。`_updateGUIFont`（`0x780710`）在`FONT_CANCEL`时返回null；否则读取当前GUIStyle.font的名字，空对象使用固定key `*`，逐项比较`invalidFontNames_`，只有名称匹配时才改设默认字体，最后返回当前GUIStyle.font。它不是每次测宽都无条件覆盖自定义字体。本批方法内没有创建指定系统字体的调用；不能据此宣称整个Unity或原生插件不使用系统字体，更不能指定为“微软雅黑”或“Arial中文”。

## 本地化字号与基线偏移

以下只描述被调用时的已核函数，不假定当前全局开关值或所有上游设置入口已证。

- `IsNeedLocalizeScale`（`0x77DAA0`）：先采用本地化设置栈顶布尔值；栈计数≤0时读`Config.LOCALIZE_FONT_SCALE`。还需未锁定、非Japanese、`usedSize_≥6`。
- `GetLocalizeScale`（`0x77D810`）：默认float32比例约0.83；`Config.ASIA_FONT_SCALE`为真且语言以`ko`或`zh`开头（`StringComparison`参数1）时改为约0.98。按float32计算`usedSize_×比例+0.1`后向零截断；`usedSize_≤9`时结果不小于`usedSize_-1`。原始float位型见机器证据，不把十进制显示写成精确实数计算。
- `UpdateFontScale`（`0x77EB70`）：当实例`fontScale_`与静态`requestFontScale_`不同，先同步实例值，再以当前`size_`、`usePreset=true`调用`SetSize`重算。静态初始化已核`requestFontScale_=100`，`SetSize`局部取整与preset见下节；全部后继请求值仍独立。
- `GetOffset`（`0x77D920`）：返回`float32(usedSize_) × requestFontOffset_ / 12`。这是返回的偏移量，不是本批已核完整DrawString基线公式；请求偏移的后继设置时机和消费位置尚未闭合。

`Font._draw`（`0x77F450`）仅把当前字体压入Graphics，调用`Graphics.DrawString(text,x,y)`，然后PopFont。本页不越过该调用边界猜绘制基线、抗锯齿、裁剪或纹理坐标。

## 请求字号、preset与usedSize

2026-10-13补固定Steam静态链，未运行原窗口或指定中文Font对象。`Font._setSize` RVA77FD50先保存请求`size_`、`usedSize_`及`usePreset_`（VA1077FD7C–82）。仅当usePreset=true且有符号请求值小于HEIGHT.Length(3)才索引HEIGHT；此处负值仍会在数组无符号边界检查失败，不能当作接受负索引。其它请求直接用输入值。真正preset为：

| 请求 | usePreset=true | usePreset=false |
| --- | ---: | ---: |
| 0 | 12 | 0 |
| 1 | 24 | 1 |
| 2 | 30 | 2 |
| ≥3 | 请求本值 | 请求本值 |

之后按原整数次序计算`usedSize_=trunc_toward_zero(int32(usedSize_×fontScale_)/100)`，VA1077FDE8–FE04先32位整数乘再有符号除100；没有float、+0.1／+0.001或最小1夹限。然后将usedSize传`GUIStyle.fontSize`（1077FE11）。GUIStyle缺失走原异常，不在此函数补Init。维护若拒负输入、非正scale或溢出，须明确标安全策略，不能称原代码有clamp；C++实现也不能直接依赖有符号溢出未定义行为。

HEIGHT来自Font.cctor RVA77EDD0的实际InitializeArray调用（1077EE60，slot110F6CBC→fieldRef352／field7026／metadata dataOffset3014052），12字节为`[12,24,30]`；独立WIDTH slot110F6F08为`[6,12,15]`，_setSize不读它。构造77F2F0从当前requestFontScale复制实例比例，初次_init77FC90调用SetSize(0,true)，因此在原静态100%条件下usedSize12；这不等于所有动态页面都使用12。

公开`SetSize` RVA77DF50冻结this／size／usePreset；CheckUiThread为真直接调用_setSize，否则经Font.RunOnUiThread RVA77DE20以wait=true交IApplication，lambda792CC0读取同三字段进入_setSize。此合同只给调度要求，不模拟真实平台线程。SetScale77DF00仅写静态请求，不立刻遍历实例；UpdateFontScale77EB70发现实例比例变化才同步，并用原size_、**固定usePreset=true**重算，不读取旧usePreset_。例如显式size1在比例改变后的重算会走preset1，不能擅自“保留显式模式”。

可用于产品独立数值校验的源公式样例：preset0／1／2在100%为12／24／30；显式0／1／2仍为0／1／2；请求3在100%为3；显式11在98%为10，preset0在98%为11，显式11在125%为13。这些是静态公式结果，不是字形实际像素高度或运行字体测量。后继本地化比例与最终GUI比例仍分别执行，不能并成一次舍入。

## 测宽不是字数乘固定字宽

`_stringWidth`（`0x780620`）与`_stringWidthF`（`0x77FFB0`）都先拒绝null文本或缺GUIStyle并返回0。整数入口先翻译文本；浮点入口在`LT_DISABLE_TEXTLAYOUT=false`时翻译。自动TextLayout开启且文本含`<`时，分别转`StringWidth2`／`StringWidthF2`，不能把包含标签的字符串直接交普通字宽分支。

普通浮点路径按TextFormat指定字号或本地化规则临时调整字号，调用`UpdateGUIFont`，保存当前GUIStyle.fontSize，再读取`IApplication.GetScaleRatio(false)`：

1. 临时GUI字号是`trunc(fontSize×scaleRatio/100 + 0.001)`，计算保持原float32阶段。
2. 文本经过`UnicodeToChanakya`；非纹理分支把空格替为句点后放入GUIContent，以`GUIStyle.CalcSize`取得x宽度。
3. `useFontTexture_=true`时走`KairoPlugin.GetFontTextureSize`，输入为变换后的原文本，**不使用上述替空格版本**；字号另乘100%，`UNITY_EDITOR=true`时乘75%，再以整数除100。
4. 得到的宽度再乘100／scaleRatio，随后恢复保存的GUI字号；曾临时改Font.size_时也恢复旧值。

整数普通路径最终是`trunc(StringWidthF(...) + 0.01)`，不是四舍五入。null分支、TextLayout分支与原生纹理分支不能合并成一个简单字宽公式。`CalcSize`与插件内部字形选择／kerning／fallback仍是外部消费者边界；本批未执行它们生成“实测中文宽度”。

### TextLayout渐显块的测宽

2026-10-13补核实例`TextLayout.StringWidth` RVA7CCB10（400字节）、`TextBlock.StringWidth(len,out width)` RVA7DB0D0（880字节）及其直接清理helper7D0660；这段消费**已经排好的块**；标签生产、字号适配与换行分别见本节后继合同，不能由渐显测宽倒推。

实例入口将`pos_`有符号向零除1000作为本次字符预算，按`textBlocks_`原序逐块消费，预算≤0即停。千分单位另由`Flush` RVA7CAC70的VA107CAC7B直接`length_×1000→pos_`交叉。首横坐标初值为NaN（float位型0x7FC00000），遇首块才写其`x_`；每块返回已消费数量，从预算扣除，同时把末端写成该块`x_+width`。结束时有有效末端则返回`trunc(float32(末端−首横坐标))`，无消费仍是NaN时返回0（107CCC6E–8F）。**不是各行最大宽或全部块宽相加**；函数内没有最大值比较，也不调用Frame／Flush推进渐显。完整布局`textWidth_`另由GetTextWidth／GetTextWidthF读取，不能混用实例渐显测宽。

块入口先把输出宽度置0，分支顺序为图标、按钮、普通文本：

- 普通文本把消耗数量取为`min(len,String.Length)`；预算0直接返回0，否则不足整串时测`Substring(0,len)`，足够时测整串，调用本块Font.StringWidth RVA77E930。这里是C#字符串的UTF-16代码单元，不证明按Unicode码点或用户可见字素截取；负预算不在本函数夹成0，外层正常调用资格才保证预算>0。
- 按钮块在此固定输出宽12、消耗1（107DB255–25B）；不是标签串长度或实际按钮字宽。这是测宽分支，不据此改按钮绘制尺寸。
- 图标预算0消耗0，否则消耗1。先按`pictographParam_[0]`查已注册Pictograph；找不到或静态pictographVisible=false时宽仍为0，不补Font字宽。默认帧取Pictograph.Frame，第二参数非空时若为精确字面`∞`（槽110EF444）则用−1，否则交Int32.Parse。取得Seb.GetSprites(frame) RVA7C2A80后，按记录原序对`record[1]≥0`的项把输出宽覆写为`record[4]`（107DB393–3A5），因此本局部是最后符合项的值，不求各层最大包围盒；Margin／Scale／Transform没有在本测宽方法消费。SEB展开及图标实际绘制仍由各自消费者负责。

测宽存在原共享副作用：块入口VA107DB18E把`Language.UnicodeToChanakyaEnable`静态+C8写false；正常两收尾及所核异常续体均调用helper7D0660–06B9，将同字段写true。这里**未保存进入前布尔值**，不能实现成“始终无修改”或擅称恢复原值；异常清理后仍重新抛出，不吞解析／空引用错误。类型槽110FB03C独立解码为Language（TypeDef由本样本registration解析），与dump的+C8字段一致，不靠共享setter名字认证。Font、插件字形度量及标签生产者边界保持；本批不运行原游戏、不导出字体、不新增核对脚本或过程指纹。

### 标签入口及颜色组

2026-10-13续核`TextLayout._makeTextBlocks` RVA7CD750的真实扫描入口、`TextLayout.cctor` RVA7CCD70的TAG写入，以及颜色组分支；与固定APK `kairo/android/ui/aa.java`的`f()`交叉。此处闭合标签识别和`co`组；其它标签及行布局见本节后继合同，不能套用Unity原生富文本规则。

Steam先对原text调用`Language.TranslateText(text,true)` RVA8211A0，再调用`UnicodeToChanakya` RVA821340；随后`StringUtil.Replace` RVA86A390把精确`<br>`替成LF（VA107CDBE2–DC17，槽110EB3DC／110FC0D8）。扫描按C#字符串代码单元进行：当前字符为`<`才查从当前位置开始的第一个`>`；找到且`结束索引−起始索引≤32`才取含括号的候选串（107CDCDF–DD24）。这个32约束是索引差，不是参数长度；超过门槛走另一分支，不应以浏览器HTML规则解释。

TAG由cctor实际创建14项数组并按原序写入（107CCF16–D2A8）：

| 索引 | 精确模板 |
| --- | --- |
| 0／1 | `<co=>`／`</co>` |
| 2／3 | `<si=>`／`</si>` |
| 4／5 | `<st=>`／`</st>` |
| 6／7 | `<bl=>`／`</bl>` |
| 8／9 | `<po=>`／`<pic=>` |
| 10／11 | `<bo=>`／`</bo>` |
| 12／13 | `<btn=>`／`<ff>` |

逐项先`CompareTo(candidate)==0`；不相等时，只有模板和候选都含`=`才取模板从开头至第一个`=`的前缀，交`candidate.StartsWith(prefix)`（107CDD9D–DE96）。无小写转换、Trim或通用HTML属性解析；具体比较保留C# CompareTo／StartsWith调用语义，不能擅称全部是Ordinal。关闭`</co>`因此按整串匹配；带参数模板按包含`=`的前缀匹配。找到以后参数取候选第一个`=`之后至最终`>`之前，用`StringEx.SubstringJ` RVA851D80，不把括号写入参数。此表存在仅证明识别资格，不能直接宣称14种绘制都已维护。

颜色参数先尝试十进制`JInteger.ParseInt`，再独立尝试基16版本（107CDF35、107CDFF6）；这两个失败各有捕获，初始数值0保留。独立helper RVA7F6950仅在radix16时转`Int32.Parse(s,NumberStyles.HexNumber=0x200)`，其余转十进制`Int32.Parse(s)`，无命名色字典；不能把`red`／`blue`当作已支持的颜色名称，也不自行增加`#`前缀语法。`co`与`bo`索引的十六进制结果为0且参数含逗号时，才`StringUtil.Split(value,",")`并将前三段十进制值依次左移16／8／0后按位或（107CE0B5–E16A）；各段此处未先夹限到0–255，后续取RGB时才分别掩到8位。不将解析失败默认为布局原色：`co`最终按所得RGB调用`Graphics.GetColorOfRGB` RVA76CF70，写当前色（107CE8AA–E8ED），例如合法`000000`是黑色。未知／越界／RGB段解析异常的完整后继尚未认证，不在产品补容错后冒称原版。

`</co>`直接从本实例`color_`（+10）重置当前色（107CE889–E89B），**没有颜色栈或恢复上一嵌套色**。当前色冻结给随后新建的`TextBlock`（107CF0F2／107CF332），构造RVA7DB440在107DB4C6写块`color_`。因此嵌套例`<co=FF0000>A<co=0000FF>B</co>C</co>`的C恢复布局基色，不恢复外层红色；这是源分支结果，尚非原窗口动态样本。颜色标签本身不作为普通可见文字块；新块仍交已核Font测宽，不能用标记串长度计宽。

APK的TAG只有前9项，`aa.java`同样先翻译、精确`<br>`换LF，再以索引差≤32、整串／含`=`前缀匹配；颜色参数十进制／十六进制尝试、逗号RGB拼合、关闭色直接回`f543c`与Steam该组一致。APK反编译内普通换行／英文单词循环有明显条件失真，Steam行提交及整词前瞻改由下文机器码独立认证；Steam新增的`pic`／`bo`／`btn`／`ff`也不能倒填成固定APK已有标签。本次只读机器码与现有反编译输入，不新增图片、字体、核对脚本或临时产物。

### 字号、样式与闪烁组

2026-10-14补核同一`_makeTextBlocks`中TAG2–7的实际消费者；参数使用上节已经独核的十进制结果，解析失败保留0，十六进制结果不作为这三组输入。工作字体先从布局原Font复制（107CDB6F–73调用Font复制构造RVA77F130），不是直接改布局原Font。三组结束标签都没有嵌套栈：

| 标签 | Steam实际效果及位置 | 关闭行为 |
| --- | --- | --- |
| `<si=n>` | 107CE7E3–E80B对工作Font调用`SetSize(n,true)`，读取所得`usedSize_`（+14）；仅当高于当前行高时提高该行高 | `</si>`在107CE7A4–E7D7取布局原Font的请求`size_`（+8），再`SetSize(size_,true)`；仍只提高行高，不当场缩短已累计行高 |
| `<st=n>` | 107CE79D→E77F调用工作Font.`SetStyle(n)` | `</st>`在107CE76C–E783读布局原Font.`style_`（+C）传给SetStyle，恢复基样式，不恢复前一嵌套样式 |
| `<bl=n>` | 107CE754–E763写当前闪烁周期n，后续新块冻结到`blink_`（+20） | `</bl>`在107CE73B–E74B直接写周期0，不恢复外层周期 |

字号仍消费本页已核preset和比例次序：`<si=0/1/2>`是12／24／30请求映射，再施实例比例，不是0／1／2像素；其它整数交原SetSize。关闭也固定`usePreset=true`，不会保存／恢复进入标签前的显式字号模式。例如布局原请求1且原为显式模式时，关闭会按preset1重算，而非回显式1；嵌套`<si=24>…<si=30>…</si>…`在原请求0条件下回preset0，不回24。负数仍可能在原preset数组边界抛异常，不能把局部解析捕获误写成SetSize也受容错保护。

样式后继已核到`Font._setStyle` RVA77FE30：先将n写实例`style_`（1077FE5B），再以无符号数组边界检查索引静态STYLE，调用`GUIStyle.set_fontStyle` RVA C306B0（1077FE81–E94）。STYLE由Font.cctor实际建立三元素数组，默认首项0、显式写1／2（1077EEB9–EEF5），对应Unity `Normal=0／Bold=1／Italic=2`；3不能因为Unity另有BoldAndItalic枚举就当作这里可用组合，负数／≥3也不夹限。原错误前已写style_，本地维护若要求拒绝后无部分修改，须作为维护校验先拒；不能称原setter事务。`st`是Font样式，不是Steam另有的`bo`描边／粗绘标志，也不由本批指定中文粗体字形。

新普通块在107CF0DB–F0FA及107CF31B–F33A把当前工作Font、色、闪烁周期交`TextBlock`构造RVA7DB440；构造另复制Font及GUIStyle，并在107DB4D1保存周期。因此后续标签不会倒改已建块字号／样式／周期。块测宽走冻结Font，普通可见绘制在107DA50B–DA566或107DA61A–DA650用该Font调用Draw；完整字形仍由已核Unity／插件边界处理，不生成近似字体。

闪烁是块自己的`blinkCnt_`（+24），新对象初值0。`TextLayout.Frame` RVA7CACD0在107CAE0C–E23仅对周期>0的块执行`blinkCnt_=(blinkCnt_+1)%(2*blink_)`；这是每次Frame调用，不写成秒数、逻辑世界tick或由绘制推进。普通块`Draw`在107DA3A1–DA3AB检查周期：周期≤0不闪，周期>0且计数≥周期跳过普通文字绘制；仍经107DAFDA–DAFEC返回`min(len,text.Length)`的字符消费量，隐藏块不会使后面的渐显文字向前顶。例周期2从初值0可见，连续Frame到1仍见、2／3隐藏、0再次见；本页StringWidth不读blinkCnt，隐藏也保留测宽。周期乘2／递增是原32位算术，未见大周期保护；溢出及异常完整后继不在本批维护保证内。图标／按钮在块Draw更早分流，本批没有将普通块闪烁规则外推给它们。

APK `aa.f()`同三组使用十进制参数，`</si>`回原Font请求`n.c()`，`</st>`回`n.d()`，`</bl>`清0；`ab`构造复制`n`并冻结周期`f552f`。`aa.c()`按每调用一次推进块`g=(g+1)%(2*f552f)`，`aa.a()`普通绘制用同半周期资格但继续扣字符预算，与Steam普通块局部一致。后端分别登记：APK `n.b(style)`用三项`[0,1,2]`映射Android Typeface.NORMAL／BOLD／ITALIC并创建Typeface；Steam交Unity GUIStyle，不能据接口语义相同认证字形／测宽相同。APK换行循环失真不作为Steam布局依据；换行主干、整词前瞻、位置及其余标签见本节后继合同。图标注册的有限排查和真实中文字体边界分别保留。

### 普通宽溢出、显式换行与块提交

`_makeTextBlocks`的107CE90A–CEA40、107CEE44–CF250补齐普通行提交主干；以下自动断行结论限定在未被后述组合探测／整词前瞻改写的路径，不认证完整语言排版。设当前行累计宽L、布局宽W、本字符增量D：新块用工作Font.StringWidthF测当前字符；已有块用`StringWidthF(块当前文字+字符)−块已测宽`，保留float32差值（107CE948–E9DA），不能将每个字独立测宽再相加。自动断行条件为W不等于精确−1、非组合续体、`L+D>W`且multiLine_为真；等宽不换行，也没有epsilon或最小行宽修正。format_为真且溢出字符是U+3001／U+3002时另将D置0而不在该处置断行标志（107CEA18–3A）；当轮跳过普通追加，最终是否进入块字符串由下述源范围重建决定，不据此认证完整标点悬挂算法。

LF（U+000A）独立置断行标志，不要求multiLine_为真，也不受W=−1豁免；精确`<br>`已在扫描前换成LF。CR（U+000D）不走LF分支，本步不测增量宽，同样交下述源范围规则。发生断行时，107CEE44–EF9E先为当前待排块写旧行坐标，随后清L、把待排起点置当前块数，通常令Y增加`H+lineSpace_`，将下一行高重置为当前工作Font.usedSize并请求新块；ff另沿后节已核路径。**当前溢出字符在旧行提交之后才进入新块**，并按新块重新测宽，不回退源游标或循环重试fit。因此超宽首字符也可能先推进一个空行，再保留超宽字符；连续LF可各推进一行，但没有为LF创建可见字符块。非日式布局的溢出ASCII空格另在107CEE0B–2D把本次增量置0，不能统一成所有行首空格自动裁掉。

普通块只在需要新块且本次D>0、组合续体或pic／btn资格成立时创建（107CF0A0–138）；普通字符追加另检查D>0或组合续体。识别到的样式标签只改变工作状态／请求新块，本身不作为普通字符宽；pic／btn沿各自已核布局宽进入同一溢出与行提交主干，之后作为独立特殊块。循环终点仅在仍有待排块时再收末行并计其H，否则沿已有Y，不无条件补尾部空块／空行（107CF381–4D4）。进入本方法时实例text_长度0，则在107CD918→107CF7EB建立空textBlocks数组并返回，尚未到107CDBAD的textWidth_=0；这与非空原串后来被翻译为空的路径不同。

空串上游由两个SetText入口闭合：公开RVA7CC170先将null转换为空literal，再调用TextLayout.Replace(text,null)，计算hash后交私有RVA7CC200。后者先TranslateConsoleText及特定标记处理，在107CC4A6–4D7安装text／hash、清pos／beginTime／timeOffset；107CC546才调用_makeTextBlocks，未调用Init，也未清textWidth。以下结论限定于该次装入的text_确为空且调用正常完成，不假定运行替换表／平台文案转换必保持所有输入不变，也不将私有null载荷当合法空串。

| 空SetText正常完成后的读取 | 已核结果与来源 |
| --- | --- |
| 块／渐显长度 | textBlocks为零长数组；SetText后段107CC699–71A先清length_再遍历块求和，故length=0，pos仍为0。没有保留旧文字块作为空串占位 |
| 完整缓存宽 | textWidth_保留此前值；GetTextWidthF RVA7CB400直接返回+50，GetTextWidth RVA7CB410返回`trunc(float32(textWidth+0.9f))`。不是空串必返回0，也不是按旧字符串重新测量；新构造旧缓存本来为0是另一前提 |
| 高与渐显宽 | 没有独立textHeight_字段；height_ +24是配置框高。GetHeight RVA7CAE80按块的非空text求最大`y+font.usedSize`，空数组返回0；GetNextHeight RVA7CB130与已核实例StringWidth亦因没有可消费块返回0。不能用配置框高代替内容高 |
| 原绘制 | Draw RVA7C9AC0在107CA054–0C6跳过全部TextBlock.Draw，不会借旧宽画出旧文本。但仍可执行坐标／文字设置栈、按overArea压弹clip，正常尾107CA11D–128还增加draw_；renderRectVisible且框尺寸有效时可另画框线。因此无文字块输出不等于整个Draw完全无操作／无图元 |

SetText的autoScale路径也保留这一区别：多行时用GetHeight，空块为0；单行且配置宽高为正时，107CC651–697比较的是旧textWidth，过宽可反复以`当前usedSize−1`调用SetSize(...,false)并再次_makeTextBlocks，空早退仍不更新该宽。局部退出检查是该次减一实参恰1或宽度条件解除，不据此保证任意比例／字号下循环终止。正常尾107CC71A–729恢复此前复制的Font；不能把这些临时调用说成空串真实字形缩小或缓存宽已重置，全部异常路径亦未由此认证。

Init RVA7CB430是另一组赋值：重建Font，text设空、清渐显时间／位置、width_/height_设−1，并在107CB570将textWidth清0；**没有写textBlocks或length_**。新构造RVA7CD4A0先在107CD4CC–4E3建零长块数组，再内联同Init赋值，不能由新对象的零状态推断对旧对象再Init会联合清空所有字段。若仅重复Init而未SetText／重排，旧块及length仍在、GetHeight仍读那些块，但pos0暂不消费文字；没有据此认证原页面真的执行这种复用链。固定APK `aa.a(String)`（744行起）同样归一null、置文本／pos、调用f，再清总长度并按块求和；f的空串早退留下旧m宽，b()按空块返回高度0。APK构造初始化空块／m0，但没有此Steam独立Init入口；其失真的非空换行循环不参与上述小型调用链结论。

游标及块start_/length_基于UTF-16代码单元。TextBlock构造在107DB4EA–F1令start=−1、length=0；普通追加门槛为D>0或组合续体（107CF14F–167）。通过时仅首个接受单元设置start，此后每次写`length=当前源索引−start+1`，三单元组合资格另加1（107CF1CF–1E9）；未通过则不扩范围、不追加临时text，也不增加width。全部块最后在107CF6E1–73F对start≠−1者，用**翻译／字符转换／br替换后的局部串**执行`Substring(start,length)`覆盖text。此后本方法不再测宽，原width和已排坐标保留。

所以零增量单元的保留取决于位置：若它位于同块两个已接受单元之间，后一次扩大源范围会把中间单元带回最终字符串；块首之前或块尾之后没有被范围包住的单元仍不进入。换行或已识别标签请求新块后，后继字符不会扩前一块的范围。以普通非组合路径、无额外整词／换行干预为条件：`A\rB`若A/B在同块被接受，临时测宽串为AB，最终块串为A\rB；`\rA`及`A\r`最终均为A。format溢出的`、`／`。`同理：末尾不会凭零增量自动附到前块；若后继字符仍被同块接受，标点可由范围重建带回，若后继换行或开始新块则不会。这些是块字符串的条件推导，不是原窗口像素观测；含CR／标点的最终字形和实际宽度仍交字体后端，不能将保留源字符等同于可见字形或按新串补算布局宽。

通常每轮游标加1；107CE227–393另有**按实际等宽结果判断**的组合续体探测：前后单元值都超过阈值（语言码精确`hi`用128，其它255）时比较两单元串与前一单元的StringWidthF；等宽才标组合续体，符合后继三单元等宽时再额外跳过一单元（107CF366–379）。这不是检查D800–DFFF代理对，也不是完整Unicode字素分割；不能承诺所有emoji／组合字绝不拆开，或用通用码点循环取代而声称一致。固定APK `kairo/android/ui/ab.java`只有累计text／width等字段，没有Steam这对start／length；`aa.f()`末尾约713–722行只转移块并加纵偏移，没有按源范围重建文字。该局部差异由字段及收尾交叉，不用APK反编译失真的换行条件推断Steam范围，也不认证全部APK零宽字符路径。

整词前瞻107CEA40–CEDE2现补核到源游标及退化路径。入口要求原输入含ASCII空格、multiLine_、非`Language.IsJapaneseStyleTextLayout()`、待词首标志为真且本字符不是空格（107CD97E–9CA、107CDC4C、107CEA40–50）。该语言helper RVA811F90实际对非空语言码调用Regex.IsMatch，模式`^(ja|zh|th|ar)`、options1（IgnoreCase），不是只Japanese，也不含ko。待词首标志初始true，非日式遇ASCII空格再次置true；本次前瞻结束清false，不因LF自动再次置true。预测从当前游标k开始；本轮是pic／btn时k改为已消费标签末端加1，并先计本轮特殊块宽。107CEA80–ADD在**翻译、字符转换及br替换后的局部串**中找首个ASCII空格／LF／精确`<br>`或串尾，所得非空段按单UTF-16单元的当前Font.StringWidthF累计预测宽；它不同于普通实际块的追加串差值，不模拟未来标签后的工作字号／字距。

前瞻中的标签跳过不能修饰成正确递归排版：遇`<`先从下一单元找首个`>`；找到就把前瞻索引改为这个**结束位置**（107CEB4D–58），之后却以新索引调用StringEx.StartsWith检查`<co=`、`<si=`、`<st=`、`<po=`、`<bl=`、`<bo=`、`<ff>`、`</`、`<br>`、`<pic=`、`<btn=`。独立helper RVA851CF0确实从传入offset逐单元比较，没有回退到`<`。因此有闭合`>`时这些前缀均不匹配，本轮预测只加先前保留字符`<`的字宽，再从`>`后一单元继续（107CEC99–CDC）；内部标签与内容被跳过，但不是零宽标签或未来pic的真实图宽。没有`>`时索引不改，前九种前缀匹配只略去本次`<`的计宽；pic／btn匹配另加**本轮外层**特殊块宽后仍加`<`字宽，不是解析未来图标注册。前瞻不会执行未来样式标签；正式扫描后续仍按已核TAG消费者执行。两条路径的不同结果必须保留，不能借APK循环或浏览器HTML规则“修正”原来源。

预测段非空、W≠−1且`L+预测宽>W`时，107CED20–DDE才检查是否有可移行的既有前缀：从索引0到k之前读**原实例text_**的单元，转单字符串并Trim；空白跳过，非空且不是`<`就置断行标志。遇原串`<`却在上述**处理后局部串**的同一数值索引加1处找`>`，找到则跳过该范围，找不到就退出检查。没有任何原串／翻译串索引映射，也不把检查起点缩到当前行开头；处理后串变长时，原串取字符仍受其自身边界检查，不能补映射或兜底称原代码安全。若此前缀扫描没有找到非空普通单元，则关闭本次make剩余全程的整词前瞻资格（107CEDDE），不缩字号、不丢整词，也不清已置的普通断行标志；后续继续普通逐单位溢出。因此“首词过长”退化为普通拆分，已有前缀的长词则先请求移行、再由普通分支处理仍超宽的部分。此处闭合具名局部算法及其原索引差异，不等于所有Unicode禁则、真实翻译输入或字形宽度已验证；本批没有运行原窗口、新增排版消费者或过程产物。

### 位置标签与末行对齐收尾

2026-10-14续核TAG8的`<po=...>`与`_makeTextBlocks`扫描结束后的待排末行／全块纵向偏移。换行主干及整词前瞻见前节，pic／btn块布局见后节；此处只描述末行与纵向对齐，不据此认证全部语言禁则或实际字形度量。

位置参数使用原字符串，不使用已尝试解析的整数。107CE67D–E71E按固定顺序分别`IndexOf(char)`查ASCII小写`l/c/r`与`t/m/b`：有l写横值1，有c覆盖为2，有r再覆盖为4；纵值同理为16／32／64。选择按消费者扫描顺序而非参数出现顺序，因此同串含l/c/r最终r胜出，含t/m/b最终b胜出；某轴没有这些字符则保留该轴旧值，没有小写转换或“必须恰一个字符”的局部校验。初始两轴在107CDB49–57取布局`anchor_&7`／`anchor_&0x70`。TAG没有`</po>`项，不能新增嵌套位置栈；107CE721–E736将当前累计纵坐标清0并请求新块，未同时清当前行宽／待排块起点，不能直接实现成普通换行或新独立文本框。

扫描结束时，仅对仍未排坐标的末行块执行107CF381–F4C9的收尾。设该行已测宽L、当前行高H、累计纵位置Y、布局宽W：横值2取中心，W≠−1时按原float32顺序`W*0.5−L*0.5`，W=−1时`−L*0.5`；横值4取W−L或W=−1时−L；其它值起点0（107CF390–F41A）。按原块序写`block.x_`，每块后加其已测`width_`，不用单字宽重建。每块`y_=Y+trunc_toward_zero((H−block.font.usedSize_)/2)`，整数半高补偿先算再转float（107CF47D–F4B5）；末行存在时最终内容高T=Y+H，没有待排块时T=Y。前段行提交资格见“普通宽溢出、显式换行与块提交”，此处也不以字体请求字号代替块usedSize。

随后对**全部块**应用当前纵对齐值（107CF4D4–F564、107CF6AF–F6B9）：设布局高A，纵值32取`(A−T)*0.5`，A=−1时`−T*0.5`；纵值64取A−T，A=−1时−T；其它值偏移0。A≠−1且所得偏移<0时改为0，A=−1则保留负偏移。`−1`是精确float32未定尺寸哨兵（常量VA10E27A90），0.5来自10DDE528；不能把“高度不够仍从顶部开始”误写成缩小字体或裁掉前块。末次po的纵值参与这次全块偏移，不是每块冻结一份纵锚；block绘制还会加调用者dx/dy，不能在计划和窗口转换里重复居中。

APK `aa.f()`同样依次查l/c/r与t/m/b、缺轴保留、清纵累计，末行及最终纵偏移公式在该方法末尾（约684–718行）与上述局部一致；对应`ab.f550d/f551e/h`是x/y/已测宽，块Font `n.f()`是实际高度。该交叉不采用反编译前段失真的换行条件，尤其不能据它倒置Steam横值1／2语义。此节闭合po选择及末行／纵对齐收尾；bo／ff及pic／btn见后节。行提交资格已见前节，全部语言禁则及实际字形仍未认证。本轮只读既有源与内联反汇编，无磁盘过程产物或新增字体制品。

### 描边标签与待行收尾标记

2026-10-14补核TAG10／11的`<bo=...>`／`</bo>`与TAG13精确`<ff>`，只描述已认证扫描入口和实际块／行消费者。固定APK的九项TAG中没有这两组，不把Steam局部扩展倒填成APK功能。

`bo`沿已核颜色参数的十六进制／逗号RGB结果，在107CE3AE–E3F4取各通道8位、调用`Graphics.GetColorOfRGB`，写当前描边色并将独立标志设true（107CE3FA）；**不调用Font.SetStyle，不等于`st=1`**。`</bo>`在107CE398–E39F仅把该标志清false，未复原描边色或建立嵌套栈。随后普通绘制块在107CF0FF–F10E将色／标志冻结到`borderColor_`（+3C）／`isBold_`（+40）；新块构造默认−1／false不替代这些显式写入，标签本身不计普通可见字符。

普通`TextBlock.Draw`的部分串／整串分支分别在107DA443–DA46C及107DA573–DA59C检查同一资格：块isBold为真、不是图标或按钮、**当前文字色的最高alpha字节为255**时走描边；否则仍用本块Font.Draw，不修改style来补效果。该资格检查的是块文字color_，不是borderColor_。部分串先取`Substring(0,len)`，整串直接读取text_；描边路径`PushFont(block.font_)→Graphics.DrawStringBold(text,x,y,borderColor_)→PopFont`（107DA4C6–DA4FE／107DA5D3–DA60D），坐标继续用块x/y与调用者dx/dy。私有同名包装RVA7DA190保持同次序；Graphics公开入口RVA765720再交RVA765060的TextFormat／anchor17重载。到这里闭合标签实际调用，不把后端多次绘制／插件GetFontTextureBold的像素外扩、真实字形或半透明描边当作本批已验。已核块StringWidth不读isBold／borderColor，不能按描边外缘扩大正常文字测宽。

`<ff>`在107CE1E9–E1F7只置一个局部待行收尾标志true并请求新块，未直接写LF、改布局字号／速度、清纵坐标或立即调用Flush；TAG也无`</ff>`。该标志的唯一实际消费在107CEF9E–CF092的行收尾：标志为false或布局autoScale_（+60）为true时仍普通`Y += H+lineSpace`；标志true且autoScale=false时进入下面的对齐路径，最后同样清标志（107CF092）。扫描结束的末行收尾没有该检查，不能把孤立末尾ff无条件解释成一次分页。

设该次行收尾前累计纵坐标Y、该行整数高H、整数行距S、布局高A；仅在原算术有定义的输入域，令`C=int32(H+S)`、`N=trunc_toward_zero(float32(float32(A+S)/float32(C)))`。107CEFC9–CF016先算N，再算`trunc_toward_zero(float32(Y/C+0.1f))%N`；余数非0则按C逐次增加Y，直到该式余数0（107CF01A–F045）。随后先减C再加C（107CF047–F062），因此保留刚对齐的Y；已在边界时可保持原Y，不是严格跳到下一个边界。`0.1f`的原常量10DDE550位型3DCCCCCD。该分支未见C／N非0或A有效高度检查，C=0、N=0等可能走原数值异常；不能自行夹为1、将未定A=−1转成固定页高或宣称原标签安全分页。整数和float32阶段仍分别执行，不合成一次高精度ceil公式。

上述ff消费前节已核的行提交及源范围重建；全部语言禁则与最终字形仍未认证。APK既有`aa/ab`普通绘制及TAG不消费bo/ff，本批不据其缺项断言所有Android版本均不支持。pic／btn消费者及图标注册有限排查见后节；具体运行注册绑定及实际中文字体仍保留。本轮只读源，无游戏操作、代码、素材或临时磁盘产物。

### 内嵌按钮提示

2026-10-14补核TAG12的`<btn=n[,layer]>`从参数、块宽到实际平台图标调用。它是正文内的独立特殊块，不是可见标签文字，也不是菜单按钮或触摸区域；TAG没有`</btn>`。固定APK九项TAG没有btn，不将Steam扩展倒填到APK。

`_makeTextBlocks`在107CE1FC–CE220取当前工作Font的`usedSize_+2`作为临时块宽，将原参数按精确逗号交`StringUtil.Split` RVA86A4D0；107CF255–CF29C把分段数组存入块`pictographParam_`（+30），写`isButton_=true`、`isBold_=false`和该宽，计入行宽与最大宽，再建立普通空块。按钮与pic共用参数字段，不因字段名把btn当图标注册键。行高仍由本行Font的usedSize消费，此分支未额外增加按钮行高。已核渐显`TextBlock.StringWidth`却固定输出12、消费1；例如usedSize12时布局宽14而渐显测宽12，不能强行合成同一常量。

块`Draw` RVA7DA1F0在普通文字闪烁／视口检查之前分流至107DA65D–DA8F6。先取本块Font的实际字号H，直接用`Int32.Parse` RVAA06440解析参数0为buttonFrame；参数1存在时同法解析为layer，缺项取0。这里没有局部解析catch或frame夹限，标签扫描入口的其他数值容错不能外推到此处。然后检查TextLayout静态`buttonVisible_`（+6）：false不绘图但仍返回消费1，布局宽也没有清0；隐藏按钮不使后文前移。

可见时依次PushMatrix：平移`(float32(H)*0.5+1, float32(H)*0.5)`、缩放`(0.25,0.25)`，H不等于12再压`(float32(float32(H)/12.0f),float32(float32(H)/12.0f))`缩放；坐标先按float32计算`block.x+dx`／`block.y+dy`再向零截断为整数。107DA852–DA883调用内部`Kairolib.DrawButtons(g,x,y,buttonFrame,layer,true,true)` RVA804CA0，两个true分别是autoKeyReverse和force；包装器传给下层的末尾null是MethodInfo，不是force=false。绘制前保存`AutoCancelJoystick`并临时置false，正常收尾先弹字号缩放、恢复该布尔原值、再弹两基础矩阵。未认证异常时的联合恢复，不能将这条原链声明为事务；此路径也未注册touch。

内部包装转`Kairolib._drawButtons` RVA805D60。autoKeyReverse使frame0／1先分别读取`Canvas.GetJoystickKeyOK`／`GetJoystickKeyCancel`，不是固定A／B字母；先按原frame0／1互斥读取OK／Cancel：OK返回1不会再当取消替换；随后仅原layer1的frame16／17／18／19重映射为32／33／34／35。2026-10-14补核10805E5E–E9E：**任何非零layer最终都归0**，layer1其它帧及layer2／−1等也归0但不重映射；不能因SEB仅一层就拒绝源layer1其它帧。`Config.USE_STEAM`为真时向SteamInputManager取得按钮图像：0／1／2／3对应Select／Back／X／Y，6／7为SoftL／SoftR，8／9为TriggerL／TriggerR，10／11为Start／Option，4或29为StickL，5或30为StickR，16另走glyph helper RVA7B7960。各按钮对象的`GetImage()`虚调用决定动态图像，不是固定PNG索引；实际Steamworks glyph提供者和当前控制器仍待独立核验。

没有可用动态图像时走原SEB回退（10806190–62E8）：默认`Kairolib.resource.seb[13]`（BUTTONS_SWITCH）；`GetInputType` RVA7B7BD0返回PS4=5或PS5=13时取seb19，SteamController=1、Xbox360=2或XboxOne=3时取seb29，其余保留seb13。最终交`Seb.Draw_layer` RVA7BF810，仍使用重映射后的frame与layer。动态图像宽高均大于0时则按较长边缩到60、保持长宽比并居中，交`Graphics.DrawScaledImage` RVA7641B0；未处于局部矩阵时居中偏移先向零截断再加整数坐标，存在局部矩阵时另PushTranslate并于正常收尾Pop（108062F8–6435）。特殊frame7在USE_STEAM链且HasController=false时于10805FB0–10806002调用GetMouseButtonImage(1)，目标最大边改为75（常量10E27EB4），有控制器则在10806007–10806025取SoftR动态图并沿60。GetMouseButtonImage（RVA7B7C90）实际不读取button实参：通过get_RightClick（RVA807250）取kairolib SEB27的frame0／line0，按其图片索引31返回整张`right_click.png`（128×128），不是裁SEB后再缩放；其后75分支沿同一长宽比／矩阵处理。该PNG／SEB已在全量索引登记；现仅出版实际消费的整张PNG，不复制无人消费的SEB，也不替代为后备RB图。

三套后备原PNG／SEB现已出版到[steam-platform](../assets/steam-platform)，资源槽分别为Switch SEB13／image15、PS4／PS5 SEB19／image22、Xbox／SteamController SEB29／image33。每SEB一层44帧44记录，每PNG512×512；各帧按原SEB取裁片／偏移，尤其Xbox帧0／1的裁片X与另两套相反，不能共用同一张图。六文件原字节身份复用[全量索引](../assets/IMAGE_COVERAGE.json)，不新建指纹清单。

[后备选择接口](../prototype/include/dungeon_village_prototype/steam_platform_button_skin.hpp)`steam_controller_button_fallback`接显式input_type、frame／layer、auto_key_reverse及实际被消费的OK／Cancel映射，按上述次序返回资源和最终帧／层；不发现设备、不请求Steam、不推进时钟或写Owner。未消费的可选映射不校验，已出版帧0..43是维护预算，坏请求明确拒绝。调用者必须先走源动态图像优先分支；Steam无控制器的frame7鼠标图不能直接替换成此后备。[同模块](../prototype/include/dungeon_village_prototype/steam_platform_button_skin.hpp)的`steam_button_image_layout`接已分流的有效Image尺寸、整数锚及localMatrix资格，返回Draw锚、float32目的尺寸及独立Translate请求；普通／右键最大边分别为60／75。无矩阵的负半尺寸先向零截断再加锚；已有矩阵保留浮点TranslateTemporary→PushMatrix并用原锚Draw，两支均在Draw后重查实际矩阵才决定Pop（108063F1–10806454）。128方右键图在锚(20,51)无矩阵时画于(-17,14)、尺寸75×75；有矩阵时仍画于(20,51)，独立平移(-37.5,-37.5)。接口不合并外层矩阵、不调用SDK、不声称后端最终像素；尺寸1..65536与锚±65536只是维护预算。原右键图由`steam_right_click_image_path`提供，宽高取真实Image。字体的外层矩阵、当前设备glyph、输入映射及共享状态仍独立，[原裁片示例](examples/steam-controller-buttons.png)不是原游戏窗口或当前按键绑定证据。

### 内嵌SEB图标

2026-10-14补核TAG9的`<pic=key[,frame[,wait[,x[,y]]]]>`。这是外部注册的SEB图标块，不从key自动加载PNG或将它转换为定义ID；固定APK九项TAG没有pic。公开`AddPictograph` RVA7C96C0接受key、Seb、Frame（默认−1）、Wait（默认1）、X／Y／Margin（默认0）、Scale（默认100）与EnableColor（默认false）。首次为key建立Pictograph并存入全局`Dictionary<string,Pictograph>`，已有key则更新同一对象的这些字段且重建单位Transform（107C9721–9810）；Wait至少1，未见Scale或Frame范围夹限。`GetPictograph` RVA7CB280只用原key向该表TryGetValue；标签处理与绘制不补资源，也不复制整个SEB，后续绘制重新查表，所以注册对象变化可影响既有块。具体页面注册了哪些键、各键的真实SEB出版入口，仍应由对应调用者合同逐项绑定。

扫描生产者107CE407–E4EA将参数按逗号拆分：0为注册key，1非空时精确`∞`（U+221E，slot110EF444）写frame=−1，否则直接Int32.Parse；缺项／空项用注册Frame。布局仅另外读取参数3的X，缺项／空项用注册X，参数2的Wait和4的Y留给Draw消费。没有这组局部解析catch；不能把颜色标签的失败默认数值外推。找不到注册项时仍建立宽0的pic块（107CE814–E82E）；不把未知key字面当可见文字。107CF2B9–F300冻结参数数组、写isPictograph=true／isBold=false、计入块宽及行宽，再创建普通空块；标签不是样式开关，也没有`</pic>`。

布局宽与本页已经核验的渐显宽是不同消费者。frame不为−1时，生产者仅把超过末帧的输入改成`Seb.Length−1`，取`Seb.GetBoundingRect(frame)` RVA7C0700的四项short；frame=−1取整体`GetBoundingRect()` RVA7C0650的四项int。先以`(r[0]+r[2],r[1]+r[3])`交注册Transform.`MapPoints` RVA7ABF50，设所得横分量为U，注册Margin为M、当前X为X、注册Scale为S、工作Font.usedSize为H；原宽次序是`V=trunc(float32(U+0.9f))+M+X`，S≠100时再用有符号整数`V=trunc(int32(V*S)/100)`，最后`width=trunc(float32(float32(V)*float32(float32(H)/12.0f)+0.01f))`（107CE4EA–E675）。原0.9f／0.01f常量分别为10DDE558／10DDE54C；保持float32与整数阶段，不合并舍入或按包围盒最大宽替换。纵分量也被MapPoints处理，但这条pic生产路径未据它提高本行Font行高；大图不自动撑高文字行。负帧、空SEB、异常Transform等不由此有限上界修正证明安全。

渐显`StringWidth(len,out width)`和`Draw`在len=0时均返回消费0；否则pic消费1，未知key、隐藏或落在纵视口外也消费1。渐显宽只在注册项存在且pictographVisible为真时读取`Seb.GetSprites(frame)`，对有效texture记录依原序覆写原sprite宽，不读Margin／Transform／Scale／字号，也不使用Draw的动画帧推导；因此不能用测宽结果代替布局／绘制包围盒。pic分流早于普通文字blink，不外推`<bl>`半周期到图标。

Draw在107DA945–DAA8A取注册Frame／Wait／X／Y，再按非空参数1／2／3／4覆盖；frame的∞特例如上，其余直接Int32.Parse。Wait再次夹为至少1；frame=−1时用`trunc(parent.draw_/Wait)%Seb.Length`取得本次帧（107DAA8A–DAABE），没有此处空帧数保护，也不由Draw递增动画计数。注册Scale和Transform不是标签参数。取字号比`Q=float32(float32(H)/12.0f)`；偏移为`dx+X*Q`、`dy+Y*Q`，另在`GetLanguageCode().ToLower()`不匹配原正则`^(ja|ko|zh)`时先将Y加1（107DAAF2–DAB6E），不能写成全部语言统一一像素补偿。纵视口资格用本次帧包围矩形和Q，在完全高于／低于视口时跳过；静态pictographVisible随后控制绘图，二者均保留消费1。

实际图像来自注册Seb.Resource的`img[record[1]]`（Seb字段+58、ResourceManager字段+8），不是TextLayout自己的固定atlas。107DAC4B–DAFCF取得GetSprites并按记录原序绘制，texture索引<0跳过；源矩形为`record[2..5]`，目的位置为块位置＋上述偏移＋`record[6/7]*Q`。目的宽高分别先`trunc(float32(record[4/5]*Q+0.01f))`，S≠100再有符号整数乘S／除100；S在这条路径只进一步改目的宽高，不再乘目的偏移。注册Transform非单位时另PushMatrix；最终交`Graphics.DrawScaledImage` RVA7641B0，正常逐片收尾弹该Transform。

有效sprite的`record[10]`为1／2／3时分别PushRenderMode `(1,a,255−a)`／`(1,a,255)`／`(2,a,255)`，a来自record[11]最高alpha字节；当当前renderOperator=0且当前srcRatio≠255时，先以整数`a*srcRatio/255`合成，超过255夹255。绘后PopRenderMode。record[10]=0且注册EnableColor为真时，另以`PushBlendMode(2,Graphics.color)`绘制，正常收尾PopBlendMode；没有EnableColor时不因文字块color自动染色。未知非零record[10]不能从上述三种资格推广出安全压栈／弹栈，最终像素混色仍交Graphics／shader合同，不能凭字段名画近似效果。

外层TextBlock.Draw在已有局部矩阵时先压本块坐标平移并清局部x/y，随后将`Language.UnicodeToChanakyaEnable`置false；正常及所核异常尾都走helper RVA7D06C0，写该开关true并在CheckLocalMatrix成立时PopMatrix。它没有保存开关原值，不是完全无副作用；逐片render／blend／Transform的Pop只见于正常路径，helper不证明这些栈遇中途异常也恢复。产品应把已证key→SEB／源图／参数／Transform绑定作为外部提供者，缺绑定显式登记，维护拒绝与RAII作为独立策略。具体注册键仍见后节有限排查，已核换行主干不替代真实字体与全部语言禁则；没有游戏操作、素材导出、核对脚本或磁盘过程产物。

### 图标注册上游的有限排查

固定样本的`.text`与`il2cpp`两可执行节中，未找到以`E8 rel32`直接调用或以`E9 rel32`直接跳转至`AddPictograph` RVA7C96C0的引用；同范围内Pictograph构造RVA7ADC10的唯一这类调用是Add内部VA107C9787。作为指令边界对照，GetPictograph确有四处直接调用：Add内部107C9727、布局107CE44B、块绘制107DA934、块测宽107DB29E，均可从对应具名方法入口解码抵达。因此已核布局／绘制消费者不能反推为游戏已注册具体图标。

DLL中Add入口绝对VA的唯一四字节匹配在`.data`的1112D608；邻项依次对应SetTextCheckSize、SetRenderRectVisible、GetRenderRectVisible、Add／Get／Remove／ClearPictographs及SetPictographVisible，呈方法指针表排列，**不是具名游戏注册调用证据**。metadata全部11013条字符串literal中，含精确`<pic=`的只有前缀`<pic=`与模板`<pic=>`，没有可直接绑定SEB的具体key实例；这不覆盖外部本地化文本、运行时拼串或资源注入。当前仍无已证游戏调用者／key→SEB绑定，不凭素材名称补注册表，也不据上述有限扫描排除间接／虚调用、反射、Unity或原生注入，更不宣称运行时字典必为空。后继应从实际具名生产者或注册证据继续，不重复扫描helper绘制。

## 仍需闭合及产品使用边界

已核资源查找、GUIStyle切换、本地化公式、两种测宽链、14项TAG局部生产／消费，以及普通溢出／LF行提交、整词前瞻、末行对齐、format标点／CR等零增量单元的最终块字符串范围和SetText／Init空串字段分工。仍缺当前语言与实际翻译串样本、Unity空font具体回退、局部开关／scale／offset完整设置来源、全部语言禁则覆盖、具体运行图标注册与素材绑定，以及真实字形宽度／baseline；已核原串与处理后串混用索引及字符串重建不等于这些运行输入／像素已验。[文字后端](STEAM_FONT_BACKEND.md)已有开关资格及栈恢复，[普通DrawString](STEAM_FONT_DRAW.md)已核锚点／字号补偿／矩阵→clip→GUI位置；这些不代替当前运行字体对象和字形度量。

产品可据此分开语言字体选择、测宽与绘制，并把比例／临时字号恢复作为契约候选；不能把静态存在的字体直接发布为“原Steam中文字体”，不能在未核中文字体和真实尺寸前用截图目测常量替代测宽。本批没有导出或发布字体制品。

## 来源与解析限制

本合同沿固定具名入口独立核PE映射及方法边界，12入口共6416字节（含异常尾和填充），并交叉6个字符串使用槽、7个float32常量及ResourceManager首表1251行；不由字节总数声称所有分支完成。ResourceManager同路径可分别引用Font／Material／Texture2D，必须保留原序并以路径＋file_id＋path_id辨别，不按路径单值覆盖。首表结束于对象内63852，2352字节尾部未解析，不影响已核首表引用但不构成完整格式认证。IL2CPP共享地址的方法别名须结合接收对象／参数判断，invalidFontNames的泛型名单维护仍未命名。具体RVA及消费者就在上述章节，旧核对脚本／指纹不是产品运行依赖，不再另保过程报告。
