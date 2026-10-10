# Steam 字体选择、缩放与测宽消费者

2026-10-10后继[文字绘制合同](STEAM_FONT_DRAW.md)补普通锚点、字号补偿、矩阵→裁剪原点→GUI.Label次序及普通阴影；当前中文实际Font、字形baseline和插件内部仍未认证，窗口字形外观不代替它们。

2026-10-09。本页是固定 Steam 2.56 研究副本的局部静态事实，不是 APK 字体合同，也不是运行时字体采样。沿[嵌入字体身份](STEAM_STARTUP_RESOURCES.md)核了12个具名方法入口、6个字符串使用槽、7个float常量及资源目录。复算边界、来源哈希见[证据包](../work/steam-font-consumer/README.md)。未运行游戏、读取原档、导出字体或修改产品。

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
- `UpdateFontScale`（`0x77EB70`）：当实例`fontScale_`与静态`requestFontScale_`不同，先同步实例值，再以当前`size_`、`usePreset=true`调用`SetSize`重算。静态初始化已核`requestFontScale_=100`，但`SetSize`全部规则及后继请求值仍需另核。
- `GetOffset`（`0x77D920`）：返回`float32(usedSize_) × requestFontOffset_ / 12`。这是返回的偏移量，不是本批已核完整DrawString基线公式；请求偏移的后继设置时机和消费位置尚未闭合。

`Font._draw`（`0x77F450`）仅把当前字体压入Graphics，调用`Graphics.DrawString(text,x,y)`，然后PopFont。本页不越过该调用边界猜绘制基线、抗锯齿、裁剪或纹理坐标。

## 测宽不是字数乘固定字宽

`_stringWidth`（`0x780620`）与`_stringWidthF`（`0x77FFB0`）都先拒绝null文本或缺GUIStyle并返回0。整数入口先翻译文本；浮点入口在`LT_DISABLE_TEXTLAYOUT=false`时翻译。自动TextLayout开启且文本含`<`时，分别转`StringWidth2`／`StringWidthF2`，不能把包含标签的字符串直接交普通字宽分支。

普通浮点路径按TextFormat指定字号或本地化规则临时调整字号，调用`UpdateGUIFont`，保存当前GUIStyle.fontSize，再读取`IApplication.GetScaleRatio(false)`：

1. 临时GUI字号是`trunc(fontSize×scaleRatio/100 + 0.001)`，计算保持原float32阶段。
2. 文本经过`UnicodeToChanakya`；非纹理分支把空格替为句点后放入GUIContent，以`GUIStyle.CalcSize`取得x宽度。
3. `useFontTexture_=true`时走`KairoPlugin.GetFontTextureSize`，输入为变换后的原文本，**不使用上述替空格版本**；字号另乘100%，`UNITY_EDITOR=true`时乘75%，再以整数除100。
4. 得到的宽度再乘100／scaleRatio，随后恢复保存的GUI字号；曾临时改Font.size_时也恢复旧值。

整数普通路径最终是`trunc(StringWidthF(...) + 0.01)`，不是四舍五入。null分支、TextLayout分支与原生纹理分支不能合并成一个简单字宽公式。`CalcSize`与插件内部字形选择／kerning／fallback仍是外部消费者边界；本批未执行它们生成“实测中文宽度”。

## 仍需闭合及产品使用边界

已核层级是资源查找顺序、GUIStyle对象切换资格、本地化公式和两种测宽调用链。仍缺当前语言真实返回值、Unity空font具体回退、当前`useFontTexture_`赋值、局部开关／scale／offset完整设置来源、TextLayout标签处理及DrawString实际基线。

产品可据此分开语言字体选择、测宽与绘制，并把比例／临时字号恢复作为契约候选；不能把静态存在的字体直接发布为“原Steam中文字体”，不能在未核中文字体和真实尺寸前用截图目测常量替代测宽。本批没有导出或发布字体制品。
