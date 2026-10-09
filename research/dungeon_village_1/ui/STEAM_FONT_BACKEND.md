# Steam文字后端、默认皮肤与字体绑定

2026-10-09。接续[字体选择／测宽](STEAM_FONT_CONSUMER.md)和[语言包安装](STEAM_LANGUAGE_INSTALL.md)，只读固定Steam2.56研究副本，新增16个具名方法、1个由具名调用者定位的清理helper和3个Unity对象引用。证据及复算见[专题包](../work/steam-font-backend-contract/README.md)。没有运行游戏、查看当前进程字体、访问系统字库／用户设置、导出字体或修改产品。

本批闭合两个先前缺口：**纹理文字开关如何装入当前字体及压栈恢复；Unity内置GameSkin实际指向哪个字体对象。** 同时把DrawString的后端落点追到GUI.Label／GL文字队列／纹理／Image任务，不能据此宣称完整栅格化和中文实际字体已还原。

## 默认皮肤的真实资源引用

固定`Resources/unity default resources`含以下引用链；它与`resources.assets`里4个可按语言加载的Font不是同一组对象。

| 对象 | pathID／classID | 本批核到的内容 |
| --- | --- | --- |
| GameSkin | 11000／114 | 8488字节；名称之后首个Font引用为file0/path10102；其后是名称box的GUIStyle |
| GUISkin MonoScript | 12001／115 | 92字节；类GUISkin、命名空间UnityEngine、程序集UnityEngine.IMGUIModule.dll |
| Arial Font | 10102／128 | 286字节；对象名Arial，与前批字体清单同一对象 |

GameSkin的script引用也是file0/path12001。MonoBehaviour公共前缀、对齐名称、两个本地引用、随后box样式名称及`GUISkin`的`[SerializeField] m_Font/m_box`声明共同约束字段位置；不是搜索到Arial字样便认为所有窗口都用它。容器／三对象哈希、相对偏移和读取范围见[DATA.json](../work/steam-font-backend-contract/DATA.json)。未解析GameSkin全部样式或Font剩余字段，不把这段前缀视为完整Unity序列化格式。

**现在可以说默认资源包的GameSkin引用Arial对象，仍不能说中文字形来自Arial。** 还缺引擎实际选用该皮肤的运行状态、null-font处理、中文缺字回退的字体身份与尺寸。Arial的286字节对象也不是本批可导出的完整TTF；此前没有中文/default资源路径的结论保持不变。

## OnGUI与DrawString的字体交接

`IApplication.OnGUI`（RVA `0x7889D0`）进入时检查静态`initFont_`。若Unity对象相等判断认为它为空，就读取当时`GUI.skin.font`并缓存；不是每轮重新发现系统字体，也没有在该段按中文创建命名字体。`GUISkin.get_font`与多个无关getter共用地址，本批按实际接收对象及元数据完整别名认证，不能采用探针注释的第一条JLocale／JThread别名。

OnGUI的多条正常／异常收尾调用`0x1077BDF0`。该116字节helper从当前GUI.skin取对象、把`initFont_`设回skin.font，然后结束Profiler采样；来源锚点是OnGUI中的直接call，已从OnGUI入口顺序核指令边界。它没有独立元数据名，因此证据单独标“直接调用目标helper”，不伪造原函数名。

`Graphics.DrawString(TextFormat,string,x,y,anchor)`（`0x7663D0`）中的已核关系：

1. 翻译／自动TextLayout准入仍按前批合同；带标签等条件转TextLayout后直接返回，不经过以下普通分支。
2. 先按TextFormat／本地化设置暂改字体，再处理anchor和逻辑坐标变换。当前Font的usedSize、fontScale和Graphics.fontScaleOffsetEnable分别参与位置；不能只用字符串长度和一个固定中文偏移替代。
3. `fontKeep_`为false时，把当前Font.GetFont()与FontStyle装入Graphics的GUIStyle，并置fontKeep；随后设置本轮GUI字号。
4. 读取当前GUI.skin.font，与GUIStyle.font作Unity对象不等比较；不同就把GUIStyle.font设进skin.font，包括GUIStyle.font为空的情形。
5. 之后才进入文字后端分支。正常路径恢复临时textColor；函数末尾另有局部清理helper。本批未把该局部helper的完整字号／锁恢复行为补猜成已证。

特别保留机器码边界：DrawString末端有`UnityEngine.Object.op_Inequality(null,null)`的调用及其条件设置空font分支；本批不将其润色成“用之前保存的skin字体恢复”。外层OnGUI的已证恢复负责另一时点。不能把旧GUI.skin状态、当前GUIStyle.font和最终字形回退合成同一个“字体字段”。

`IApplication.InitGUI`（`0x7869C0`）还会从当前GUISkin复制多类平台输入／对话用GUIStyle，按输入scale调整字号，并构造白纹理背景样式。该层不等同游戏木框／菜单SEB：本批没有把引擎GameSkin的box等样式认作原游戏所有弹窗素材。

## 文字后端选择的所有权

本批已核`Font.useFontTexture_`的主要局部读写链：

- `Graphics.Init(g,img,camera)`（`0x76F8E0`）在末段读取静态DefaultUseFontTexture，把值写进当前font_，并将useFontTextureCount归零。这是本次Graphics初始化时点的赋值，不能说每次DrawString都重新按平台选择。
- `PushFontTexture(use)`把当前font_的布尔值写进栈的当前索引，必要时扩容，然后增加计数并安装use。
- `PopFontTexture()`仅在计数>0时取上一项，减计数并恢复当前font_；空栈不改变flag。
- `Font._copy`和`_set`本身会复制来源Font的flag。但是`Graphics.SetFont(f)`先保存自己当前flag，执行Copy后又把旧flag写回，再调整字号。**切换Font不等于切换文字后端。**
- 静态DefaultUseFontTexture getter读取字段+0x3C，setter写它；Graphics.cctor没有给该字段显式写true。本批未完成所有内联／间接设置入口，因此不把默认零初始化升级为当前运行值必为false。

这里的栈是原表现层状态，不占用维护世界共同随机流，也不是产品应新增的第二个业务Owner。维护实现尚未接整套Steam文字后端；本批只交付原消费者规则。

## 四类落点及插件边界

在普通画面分支且完成UnicodeToChanakya之后：

| 条件／落点 | 已核消费者 | 本批边界 |
| --- | --- | --- |
| useFontTexture=false、glRender=false | 更新GUI组后调用UnityEngine.GUI.Label(Rect,text,GUIStyle) | 最终字体栅格化／缺字回退由引擎消费，未认证 |
| useFontTexture=false、glRender=true | 准备Graphics.GLText、传入位置／文字／GUIStyle／clip并增加glTextCount | 队列最终绘制与实际字形仍待闭合 |
| useFontTexture=true | 取得FontTexture后调用DrawScaledImage，透明度不足时配对RenderMode | 纹理生成器、缓存完整生命周期与最终采样仍待闭合 |
| 目标Image的另一分支 | 复制Font，建立Image.TextRenderTask并AddTextRenderTask | 不是即时GUI.Label；任务执行者仍待闭合 |

`KairoPlugin.GetFontTextureSize(text,size)`（`0x8749A0`）构造闭包并以wait=true交IApplication.RunOnUiThread，要求取得可解箱SizeF的非空结果。闭包（`0x8870A0`）检查的是Config.UNITY_EDITOR（元数据静态字段+0x28）：false直接返回null；true才反射`WinLib`程序集的`kairo.windows.plugin.Utility.GetFontTextureSize`，把返回字节读成两个float。

因此不能把这个名字含native的入口无条件叫作“Steam当前Windows字体测宽”。本DLL的这条实现包含明确Editor资格；强行把普通运行的纹理开关打开不保证得到有效SizeF，wrapper对null有异常路径，并不会自动返回0或退回GUIStyle.CalcSize。本批不运行Editor、插件或反射调用。

## 可消费内容与剩余缺口

产品可按上述契约区分字体对象、后端开关、临时GUIStyle与外层GUISkin恢复，避免SetFont误改后端或认为一次DrawString已恢复所有外部状态。默认资源的Arial引用可用于后续取证，不能直接作为中文字体交付或以另选系统字体冒充原样。

仍缺：真实当前GUI.skin及中文glyph fallback、DefaultUseFontTexture完整上游、TextLayout标签与位置变换／clip全链、GLText／Image任务最终消费者、FontTexture资源退休，以及完整游戏窗框的全部页面使用资格。窗口观察可补最终画面尺寸和字形表现，但单张截图仍不能证明font对象身份。后续继续静态缩小这些边界，必要动态由既有外部提示词分支取得。
