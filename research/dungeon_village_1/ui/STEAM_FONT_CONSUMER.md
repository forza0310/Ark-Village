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

2026-10-13补核实例`TextLayout.StringWidth` RVA7CCB10（400字节）、`TextBlock.StringWidth(len,out width)` RVA7DB0D0（880字节）及其直接清理helper7D0660；这段消费**已经排好的块**，不认证`_makeTextBlocks`完整标签解析、换行或字号适配。

实例入口将`pos_`有符号向零除1000作为本次字符预算，按`textBlocks_`原序逐块消费，预算≤0即停。千分单位另由`Flush` RVA7CAC70的VA107CAC7B直接`length_×1000→pos_`交叉。首横坐标初值为NaN（float位型0x7FC00000），遇首块才写其`x_`；每块返回已消费数量，从预算扣除，同时把末端写成该块`x_+width`。结束时有有效末端则返回`trunc(float32(末端−首横坐标))`，无消费仍是NaN时返回0（107CCC6E–8F）。**不是各行最大宽或全部块宽相加**；函数内没有最大值比较，也不调用Frame／Flush推进渐显。完整布局`textWidth_`另由GetTextWidth／GetTextWidthF读取，不能混用实例渐显测宽。

块入口先把输出宽度置0，分支顺序为图标、按钮、普通文本：

- 普通文本把消耗数量取为`min(len,String.Length)`；预算0直接返回0，否则不足整串时测`Substring(0,len)`，足够时测整串，调用本块Font.StringWidth RVA77E930。这里是C#字符串的UTF-16代码单元，不证明按Unicode码点或用户可见字素截取；负预算不在本函数夹成0，外层正常调用资格才保证预算>0。
- 按钮块在此固定输出宽12、消耗1（107DB255–25B）；不是标签串长度或实际按钮字宽。这是测宽分支，不据此改按钮绘制尺寸。
- 图标预算0消耗0，否则消耗1。先按`pictographParam_[0]`查已注册Pictograph；找不到或静态pictographVisible=false时宽仍为0，不补Font字宽。默认帧取Pictograph.Frame，第二参数非空时若为精确字面`∞`（槽110EF444）则用−1，否则交Int32.Parse。取得Seb.GetSprites(frame) RVA7C2A80后，按记录原序对`record[1]≥0`的项把输出宽覆写为`record[4]`（107DB393–3A5），因此本局部是最后符合项的值，不求各层最大包围盒；Margin／Scale／Transform没有在本测宽方法消费。SEB展开及图标实际绘制仍由各自消费者负责。

测宽存在原共享副作用：块入口VA107DB18E把`Language.UnicodeToChanakyaEnable`静态+C8写false；正常两收尾及所核异常续体均调用helper7D0660–06B9，将同字段写true。这里**未保存进入前布尔值**，不能实现成“始终无修改”或擅称恢复原值；异常清理后仍重新抛出，不吞解析／空引用错误。类型槽110FB03C独立解码为Language（TypeDef由本样本registration解析），与dump的+C8字段一致，不靠共享setter名字认证。Font、插件字形度量及标签生产者边界保持；本批不运行原游戏、不导出字体、不新增核对脚本或过程指纹。

### 标签入口及颜色组

2026-10-13续核`TextLayout._makeTextBlocks` RVA7CD750的真实扫描入口、`TextLayout.cctor` RVA7CCD70的TAG写入，以及颜色组分支；与固定APK `kairo/android/ui/aa.java`的`f()`交叉。此处闭合的是标签识别和`co`组，未认证完整布局／其它标签效果，也不是Unity原生富文本合同。

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

APK的TAG只有前9项，`aa.java`同样先翻译、精确`<br>`换LF，再以索引差≤32、整串／含`=`前缀匹配；颜色参数十进制／十六进制尝试、逗号RGB拼合、关闭色直接回`f543c`与Steam该组一致。APK反编译内普通换行／英文单词循环有明显条件失真，本批未以它认证Steam完整换行；Steam新增的`pic`／`bo`／`btn`／`ff`也不能倒填成固定APK已有标签。本次只读机器码与现有反编译输入，不新增图片、字体、核对脚本或临时产物。

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

APK `aa.f()`同三组使用十进制参数，`</si>`回原Font请求`n.c()`，`</st>`回`n.d()`，`</bl>`清0；`ab`构造复制`n`并冻结周期`f552f`。`aa.c()`按每调用一次推进块`g=(g+1)%(2*f552f)`，`aa.a()`普通绘制用同半周期资格但继续扣字符预算，与Steam普通块局部一致。后端分别登记：APK `n.b(style)`用三项`[0,1,2]`映射Android Typeface.NORMAL／BOLD／ITALIC并创建Typeface；Steam交Unity GUIStyle，不能据接口语义相同认证字形／测宽相同。APK换行循环失真仍不用于补Steam完整布局；位置标签及收尾见下节；`pic/bo/btn/ff`、完整自动换行及真实中文字体仍未闭合。

### 位置标签与末行对齐收尾

2026-10-14续核TAG8的`<po=...>`与`_makeTextBlocks`扫描结束后的待排末行／全块纵向偏移。本批不认证自动换行、英语整词避断、全部标点禁则或图标块布局；以下是固定Steam机器码的局部布局事实，不是实际字形度量。

位置参数使用原字符串，不使用已尝试解析的整数。107CE67D–E71E按固定顺序分别`IndexOf(char)`查ASCII小写`l/c/r`与`t/m/b`：有l写横值1，有c覆盖为2，有r再覆盖为4；纵值同理为16／32／64。选择按消费者扫描顺序而非参数出现顺序，因此同串含l/c/r最终r胜出，含t/m/b最终b胜出；某轴没有这些字符则保留该轴旧值，没有小写转换或“必须恰一个字符”的局部校验。初始两轴在107CDB49–57取布局`anchor_&7`／`anchor_&0x70`。TAG没有`</po>`项，不能新增嵌套位置栈；107CE721–E736将当前累计纵坐标清0并请求新块，未同时清当前行宽／待排块起点，不能直接实现成普通换行或新独立文本框。

扫描结束时，仅对仍未排坐标的末行块执行107CF381–F4C9的收尾。设该行已测宽L、当前行高H、累计纵位置Y、布局宽W：横值2取中心，W≠−1时按原float32顺序`W*0.5−L*0.5`，W=−1时`−L*0.5`；横值4取W−L或W=−1时−L；其它值起点0（107CF390–F41A）。按原块序写`block.x_`，每块后加其已测`width_`，不用单字宽重建。每块`y_=Y+trunc_toward_zero((H−block.font.usedSize_)/2)`，整数半高补偿先算再转float（107CF47D–F4B5）；末行存在时最终内容高T=Y+H，没有待排块时T=Y。这里不证明此前哪些条件产生换行，也不以字体请求字号代替块usedSize。

随后对**全部块**应用当前纵对齐值（107CF4D4–F564、107CF6AF–F6B9）：设布局高A，纵值32取`(A−T)*0.5`，A=−1时`−T*0.5`；纵值64取A−T，A=−1时−T；其它值偏移0。A≠−1且所得偏移<0时改为0，A=−1则保留负偏移。`−1`是精确float32未定尺寸哨兵（常量VA10E27A90），0.5来自10DDE528；不能把“高度不够仍从顶部开始”误写成缩小字体或裁掉前块。末次po的纵值参与这次全块偏移，不是每块冻结一份纵锚；block绘制还会加调用者dx/dy，不能在计划和窗口转换里重复居中。

APK `aa.f()`同样依次查l/c/r与t/m/b、缺轴保留、清纵累计，末行及最终纵偏移公式在该方法末尾（约684–718行）与上述局部一致；对应`ab.f550d/f551e/h`是x/y/已测宽，块Font `n.f()`是实际高度。该交叉不采用反编译前段失真的换行条件，尤其不能据它倒置Steam横值1／2语义。至此只闭合po选择及明确末行／纵对齐收尾；`pic/bo/btn/ff`、自动换行资格、语言差异和真实字体仍分别待核。本轮只读既有源与内联反汇编，无磁盘过程产物或新增字体制品。

## 仍需闭合及产品使用边界

已核层级是资源查找顺序、GUIStyle对象切换资格、本地化公式和两种测宽调用链。仍缺当前语言真实返回值、Unity空font具体回退、局部开关／scale／offset完整设置来源、TextLayout完整标签处理及实际字形baseline。[文字后端](STEAM_FONT_BACKEND.md)已有开关资格及栈恢复，[普通DrawString](STEAM_FONT_DRAW.md)已核锚点／字号补偿／矩阵→clip→GUI位置；这些不代替当前运行字体对象和字形度量。

产品可据此分开语言字体选择、测宽与绘制，并把比例／临时字号恢复作为契约候选；不能把静态存在的字体直接发布为“原Steam中文字体”，不能在未核中文字体和真实尺寸前用截图目测常量替代测宽。本批没有导出或发布字体制品。

## 来源与解析限制

本合同沿固定具名入口独立核PE映射及方法边界，12入口共6416字节（含异常尾和填充），并交叉6个字符串使用槽、7个float32常量及ResourceManager首表1251行；不由字节总数声称所有分支完成。ResourceManager同路径可分别引用Font／Material／Texture2D，必须保留原序并以路径＋file_id＋path_id辨别，不按路径单值覆盖。首表结束于对象内63852，2352字节尾部未解析，不影响已核首表引用但不构成完整格式认证。IL2CPP共享地址的方法别名须结合接收对象／参数判断，invalidFontNames的泛型名单维护仍未命名。具体RVA及消费者就在上述章节，旧核对脚本／指纹不是产品运行依赖，不再另保过程报告。
