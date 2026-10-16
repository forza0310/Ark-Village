# Steam文字后端、默认皮肤与字体绑定

2026-10-09。接续[字体选择／测宽](STEAM_FONT_CONSUMER.md)和[语言包安装](STEAM_LANGUAGE_INSTALL.md)，只读固定Steam2.56研究副本，新增16个具名方法、1个由具名调用者定位的清理helper和3个Unity对象引用。证据及复算见专题包。没有运行游戏、查看当前进程字体、访问系统字库／用户设置、导出字体或修改产品。

本批闭合两个先前缺口：**纹理文字开关如何装入当前字体及压栈恢复；Unity内置GameSkin实际指向哪个字体对象。** 同时把DrawString的后端落点追到GUI.Label／GL文字队列／纹理／Image任务，不能据此宣称完整栅格化和中文实际字体已还原。

## 默认皮肤的真实资源引用

固定`Resources/unity default resources`含以下引用链；它与`resources.assets`里4个可按语言加载的Font不是同一组对象。

| 对象 | pathID／classID | 本批核到的内容 |
| --- | --- | --- |
| GameSkin | 11000／114 | 8488字节；名称之后首个Font引用为file0/path10102；其后是名称box的GUIStyle |
| GUISkin MonoScript | 12001／115 | 92字节；类GUISkin、命名空间UnityEngine、程序集UnityEngine.IMGUIModule.dll |
| Arial Font | 10102／128 | 286字节；对象名Arial，与前批字体清单同一对象 |

GameSkin的script引用也是file0/path12001。MonoBehaviour公共前缀、对齐名称、两个本地引用、随后box样式名称及`GUISkin`的`[SerializeField] m_Font/m_box`声明共同约束字段位置；不是搜索到Arial字样便认为所有窗口都用它。容器／三对象哈希、相对偏移和读取范围见DATA.json（本地核对材料，不随仓库交付）。未解析GameSkin全部样式或Font剩余字段，不把这段前缀视为完整Unity序列化格式。

**现在可以说默认资源包的GameSkin引用Arial对象，仍不能说中文字形来自Arial。** 引擎实际选用的皮肤对象、中文缺字回退身份与尺寸仍未取得；null-font的托管交接已补核如下。Arial的286字节对象也不是本批可导出的完整TTF；此前没有中文/default资源路径的结论保持不变。

### 空font并不在托管层自动换成包内Arial

2026-10-14从具名Unity入口顺序补核，不运行窗口或读进程。`GUIUtility.BeginGUI` RVA`C30AE0`先把调用者skinMode写入静态字段（VA`10C30B1D–20`），随后以null调用GUI.set_skin（`10C30B5C`）。实际setter RVA`C39A40`及同逻辑DoSetSkin RVA`C348F0`按Unity对象有效性判断输入；输入无效才把**当次skinMode**交原生`Internal_GetDefaultSkin`，并检查所返对象确为GUISkin。然后安装GUI.s_Skin／GUISkin.current，把该皮肤`m_Font +0x0C`交`GUIStyle.SetDefaultFont`。这证明每次GUI入口的默认皮肤选择有实际消费者，不能把磁盘GameSkin路径直接等同当次原生返回对象。

`GUISkin.MakeCurrent` RVA`C2DA60`也将自身m_Font交同一默认字体入口。`GUISkin.set_font` RVA`C2DD50`先原样写m_Font；只有自身与GUISkin.current相等时，才将新值交SetDefaultFont，随后调用Apply重建样式。因此游戏DrawString把当前skin.font设null时，**null也会被提交给原生默认字体入口**，不能解释成此setter先重新加载Arial、M+或某个中文Font。GUIStyle的get_font／set_font（`C30060`／`C306E0`）本身同样只跨原生对象边界，不在已核包装里指定字体family。

原调用目标通过实际字符串与调用点闭合：默认皮肤包装RVA`C31770`解析`UnityEngine.GUIUtility::Internal_GetDefaultSkin(System.Int32)`；默认Font包装`C2FB90`解析`UnityEngine.GUIStyle::SetDefaultFont(UnityEngine.Font)`；GUIStyle两访问器解析相应get_font／set_font入口。它们缓存的是原生函数指针，**不是中文字体资源注册表**。下文已补固定UnityPlayer的两条真实注册及局部消费者，字体fallback注册来源仍未闭合，不因入口名、默认资源包存在Arial或当前Font为空便指定“微软雅黑”“Arial中文”或M+。

最小后续观测应在同一中文文字实际绘制处关联：真实语言码及skinMode、Internal_GetDefaultSkin返回的GUISkin与其Font对象、游戏GUIStyle.font及提交的默认Font；若这些为null或缺字，还须记录引擎为一段固定中文串实际请求的字体family／字形供给。对象名和窗口截图只能分别证明对象／最终外观，不能单独证明中文glyph来自哪个字体文件。本批没有动态对象，保留该缺口，不为其新增字体副本或猜测fallback表。

### 固定UnityPlayer的真实icall注册与默认皮肤路径

2026-10-14只读研究副本`DungeonVillageEXE/UnityPlayer.dll`，PE32/i386，文件版本`2021.3.11.679073`、产品版本`2021.3.11f1 (0a5ca18544bf)`，与[固定身份](../verification/STEAM_ASSESSMENT.md)一致。具名字符串并非单独命中就认证函数：原生注册入口RVA`14E090`在`1014E093–1014E0B0`逐项从名称表VA`114E0448`及函数表`114E07A0`取同一偏移，交`104F6850`，每次增加4字节、到`0x354`停止，共213项。该消费者继续调用槽`11554CBC`；其真实安装点`10506EE1`保存的是按字面`il2cpp_add_internal_call`查得的函数。名称表索引168是`UnityEngine.GUIStyle::SetDefaultFont`，对应函数RVA`14D4D0`；索引206是`UnityEngine.GUIUtility::Internal_GetDefaultSkin`，对应`14DA80`。原生注册名不带托管参数后缀；保留两侧各自真实名字，不把它们当两套功能。

SetDefaultFont的原生包装先处理对象桥接，各条正常分支交helper RVA`869650`。该helper在`10869653–1086966F`中，非空参数取其+4指针写全局`11568F54`，空参数则将此全局写0；**这条局部原生setter也没有把空值自动换成Arial或某个中文字库**。这里认证指针安装，尚未认证后续字形消费者在默认指针为空时如何选择Font或fallback。不能因setter已到原生就宣称glyph供给闭合。

Internal_GetDefaultSkin的包装RVA`14DA80`将当次整数交helper `869C30`；后者调用`867AF0`取得皮肤对象，再处理其对象状态／引用桥接。已核`867AF0`完整正常体不读取传入的skinMode，先检查缓存`11569020`及其对象有效性，缺失／失效才经`103E0450`请求**原字面路径`GameSkin/GameSkin.guiskin`**，成功后更新缓存。失败侧原字面为`Failed to find default skin in editor resources!`，不能仅凭这个错误文本把当前player路径说成Editor专用。此处已取得固定player的具名资源请求与缓存供给，仍未把返回实例ID直接配对为磁盘path10102的Font，更未取得中文glyph来源。

本样本仅导出UnityMain；PE调试目录指向`UnityPlayer_Win32_player_il2cpp_x86.pdb`，研究副本未附该PDB／map。现有iced解码足以沿上述注册表闭合入口及局部指针／资源请求，但没有具名字形解析符号或实际运行对象来认证fallback列表。二进制中虽有Font的OS字体枚举及fontNames访问器注册字符串，它们只能证明接口存在，不能证明某段中文实际使用哪项。本批不调用这些枚举、不读系统字库／偏好，也不盲扫全引擎；未知继续限定为真实Font实例、fallback供给及单串glyph选择。

### 普通测宽的原生Font对象选择

2026-10-16沿同一固定UnityPlayer的真实注册表继续核对10,320字节有限方法窗口。GameAssembly `GUIStyle.CalcSize RVA C2E790`与`Internal_CalcSize C2F670`经槽1115AEC0调用`Internal_CalcSize_Injected`；原生注册索引161对应RVA14D000。`1014D06D–1014D07F`先将GUIContent经862210转换，再交style的864810，以零约束进入864D10；非空文字且imagePosition不为ImageOnly(2)时，`10864DCB–10864E0B`调用8680E0，读取结果+2C／+30作为文字宽高，再组合图像、padding及fixedWidth／fixedHeight。因此CalcSize不是字体裸字宽，图像／样式不能在产品测宽适配中被悄悄丢弃。

文字消费者8680E0在`1086825E／1086828E`调用原生helper867750，按以下顺序寻找有效Font对象：当前native GUIStyle +FC→原生默认引用11568F54→独立兜底缓存11568F58→请求字面`Arial.ttf`并缓存返回对象+4引用。首项字段由注册索引115的`GUIStyle.set_font`→RVA14B590交叉：`1014B62C–1014B637`处理输入对象后写同一+FC。有效性经过200F50／D5C20及必要的2266C0对象解析，不是只判引用非零；失效引用继续兜底，最终仍可能返回null并记录`Couldn't load default font or font material!`。

资源请求`1085A6D0–1085A707`交已有资源入口3E02B0／3E0450，字符串1131AAEC为精确`Arial.ttf`。这是引擎内建资源请求，未与磁盘Font path10102或任何系统字体文件直接配对。此前SetDefaultFont(null)的合同不变：setter只写引用，兜底发生在**后续测宽消费者**。该对象级顺序不等于中文缺字的glyph／family回退；本链未认证中文字形文件、kerning／baseline或非BMP栅格化。GUIContent包装保留字符串对象，不能因未见主动拆代理对就宣称完整非BMP支持；原图、维护Noto及真实字形证据仍分开。

## OnGUI与DrawString的字体交接

`IApplication.OnGUI`（RVA `0x7889D0`）进入时检查静态`initFont_`。若Unity对象相等判断认为它为空，就读取当时`GUI.skin.font`并缓存；不是每轮重新发现系统字体，也没有在该段按中文创建命名字体。`GUISkin.get_font`与多个无关getter共用地址，本批按实际接收对象及元数据完整别名认证，不能采用探针注释的第一条JLocale／JThread别名。

OnGUI的多条正常／异常收尾调用`0x1077BDF0`。该116字节helper从当前GUI.skin取对象、把`initFont_`设回skin.font，然后结束Profiler采样；来源锚点是OnGUI中的直接call，已从OnGUI入口顺序核指令边界。它没有独立元数据名，因此证据单独标“直接调用目标helper”，不伪造原函数名。

`Graphics.DrawString(TextFormat,string,x,y,anchor)`（`0x7663D0`）中的已核关系：

1. 翻译／自动TextLayout准入仍按前批合同；带标签等条件转TextLayout后直接返回，不经过以下普通分支。
2. 先按TextFormat／本地化设置暂改字体，再处理anchor和逻辑坐标变换。当前Font的usedSize、fontScale和Graphics.fontScaleOffsetEnable分别参与位置；不能只用字符串长度和一个固定中文偏移替代。
3. `fontKeep_`为false时，把当前Font.GetFont()与FontStyle装入Graphics的GUIStyle，并置fontKeep；随后设置本轮GUI字号。
4. 读取当前GUI.skin.font，与GUIStyle.font作Unity对象不等比较；不同就把GUIStyle.font设进skin.font，包括GUIStyle.font为空的情形。
5. 之后才进入文字后端分支。正常路径恢复临时textColor；末尾helper的[矩阵／本次锁／请求字号清理](STEAM_FONT_DRAW.md#临时字号与本地化锁的清理)已补核，旧字号以`SetSize(old,true)`重算，正常和已见异常续体共同调用。它不恢复完整Font对象或撤销后端输出，不能合并成“整次绘制失败无修改”。

特别保留机器码边界：DrawString末端有`UnityEngine.Object.op_Inequality(null,null)`的调用及其条件设置空font分支；本批不将其润色成“用之前保存的skin字体恢复”。外层OnGUI的已证恢复负责另一时点。不能把旧GUI.skin状态、当前GUIStyle.font和最终字形回退合成同一个“字体字段”。

`IApplication.InitGUI`（`0x7869C0`）还会从当前GUISkin复制多类平台输入／对话用GUIStyle，按输入scale调整字号，并构造白纹理背景样式。该层不等同游戏木框／菜单SEB：本批没有把引擎GameSkin的box等样式认作原游戏所有弹窗素材。

## 文字后端选择的所有权

本批已核`Font.useFontTexture_`的主要局部读写链：

- `Graphics.Init(g,img,camera)`（`0x76F8E0`）在末段读取静态DefaultUseFontTexture，把值写进当前font_，并将useFontTextureCount归零。这是本次Graphics初始化时点的赋值，不能说每次DrawString都重新按平台选择。
- `PushFontTexture(use)`把当前font_的布尔值写进栈的当前索引，必要时扩容，然后增加计数并安装use。
- `PopFontTexture()`仅在计数>0时取上一项，减计数并恢复当前font_；空栈不改变flag。
- `Font._copy`和`_set`本身会复制来源Font的flag。但是`Graphics.SetFont(f)`先保存自己当前flag，执行Copy后又把旧flag写回，再调整字号。**切换Font不等于切换文字后端。**
- 静态DefaultUseFontTexture getter读取字段+0x3C，private setter原样写入调用参数；Graphics.cctor没有给该字段显式赋值。具名初始化／配置安装链已补核如下；仍不把默认零初始化升级为当前运行值必为false。

这里的栈是原表现层状态，不占用维护世界共同随机流，也不是产品应新增的第二个业务Owner。维护实现尚未接整套Steam文字后端；本批只交付原消费者规则。

### 默认开关的正常初始化上游与写者边界

2026-10-14从具名入口顺序补核。getter RVA`77BD60`在VA`1077BD96–1077BD99`读取Graphics静态区+0x3C；setter RVA`77BDA0`的两个正常分支在`1077BDE0`／`1077BDEB`写同一字节。类型槽`110F707C`已沿MetadataRegistration.types独立解析为`kairo.unity.ui.Graphics`（类型904），不是因其它类也有+0x3C便认成同一开关。setter没有平台、语言或Config条件，它只是写调用参数。固定DLL可执行段中未找到指向这两个访问器入口的直接call／jmp候选；这是直接调用检索结果，**不证明没有内联、反射或原生写者**。

三个Graphics构造的正常路径均进入`Init`：无参构造RVA`776740`于`10776FDB`，Image／Camera构造RVA`777000`于`10777899`，Offscreen构造RVA`7778C0`的两条分支汇入`1077819A`。`Init`于`1076FE38–1076FE46`内联读取上述静态字节、写当前Font+0x28并将useFontTextureCount清零；构造／已读Init没有在此前按平台另写默认开关。Graphics静态构造RVA`775990`初始化其它图形值和缓存、注册Updated清理，但未赋值+0x3C。因此在**没有额外写者的这条正常初始化路径**，当前Font会取得静态零初值false；再次Init会重新安装当次静态值，不保留之前PushFontTexture的临时选择。不能将这个有条件结论写成当前Steam窗口必走GUI.Label。

配置反射也已缩小范围。`Config.Create(Type)` RVA`83B420`先从来源配置类取字段，再在`1083B59A–1083B5AA`取得实际`kairo.common.cfg.Config`的Type，在`1083B683`按名字找其FieldInfo，于`1083B6D3`用SetValue安装值。槽`110FE58C`／`110F404C`分别独立解析为Config的类型句柄／类型信息（类型551）；目的不是Graphics Type，也不是对任意同名属性调用setter。Config字段目录不含DefaultUseFontTexture；已读Config.Create、Init（RVA`83BB30`）和静态构造（`83D7B0`）未提供该开关的实际writer。它们安装语言、平台、Editor等Config字段，不能把`Config.UNITY_EDITOR`在纹理插件消费者处的读取反推成“Config安装时打开纹理字体”。本节认证有限正常初始化链，保留其它间接调用、反射／原生修改与真实运行开关值；不穷举整个引擎来宣称全局不可写。

## 四类落点及插件边界

在普通画面分支且完成UnicodeToChanakya之后：

| 条件／落点 | 已核消费者 | 本批边界 |
| --- | --- | --- |
| useFontTexture=false、glRender=false | 更新GUI组后调用UnityEngine.GUI.Label(Rect,text,GUIStyle) | 最终字体栅格化／缺字回退由引擎消费，未认证 |
| useFontTexture=false、glRender=true | 准备Graphics.GLText、传入位置／文字／GUIStyle／clip并增加glTextCount | [队列最终消费／退休](STEAM_GLTEXT_LIFECYCLE.md)已核至GUI.Label；实际字形仍待证 |
| useFontTexture=true | 取得FontTexture后调用DrawScaledImage，透明度不足时配对RenderMode | 缓存年龄退休、UI调度及Unity销毁请求见下节；纹理生成器、实际释放时刻与最终采样仍待闭合 |
| 目标Image的另一分支 | 复制Font，建立Image.TextRenderTask并AddTextRenderTask | 不是即时GUI.Label；实际重放／引用保留见下节 |

`KairoPlugin.GetFontTextureSize(text,size)`（`0x8749A0`）构造闭包并以wait=true交IApplication.RunOnUiThread，要求取得可解箱SizeF的非空结果。闭包（`0x8870A0`）检查的是Config.UNITY_EDITOR（元数据静态字段+0x28）：false直接返回null；true才反射`WinLib`程序集的`kairo.windows.plugin.Utility.GetFontTextureSize`，把返回字节读成两个float。

因此不能把这个名字含native的入口无条件叫作“Steam当前Windows字体测宽”。本DLL的这条实现包含明确Editor资格；强行把普通运行的纹理开关打开不保证得到有效SizeF，wrapper对null有异常路径，并不会自动返回0或退回GUIStyle.CalcSize。本批不运行Editor、插件或反射调用。

## FontTexture缓存的实际退休消费者

2026-10-14补具名静态链。`Graphics.fontTextureList_`是静态列表，与GLText峰值池及Image文字任务不同。`FontTexture`保存Text、Image、Color、Size、LastFrame和Bold；构造RVA`77CE50`将LastFrame设为当时`IApplication`单例的`totalFrames_`。该钟不是世界tick、Draw次数或墙钟秒数。

普通`GetFontTexture` RVA`76D6E0`只匹配非Bold、文字相同、有效字号相同且RGB三分量相同的项；此匹配段不比较alpha。命中在VA`1076D921–1076D97F`刷新LastFrame，必要时从原位置移除再插到列表首，不Dispose命中图像。未命中时，只有**Count>100且尾项年龄>5**才在`1076D9B4–1076DA46`退掉一项，然后尝试生成新图并插到首位。故100是触发检查的阈值，不能写成硬容量上限；近期大量不同文字仍可能使列表继续增长。本段不认证Bold生成路径或纹理插件可用性。

完整倒序清理入口`RemoveExpiredFontTextures` RVA`772BC0`读取当前应用单例的totalFrames，遍历Count−1至0；`10772CC9–10772CD1`以有符号差值比较，**年龄恰40保留，严格大于40才先Dispose、后RemoveAt**（`10772CD6`／`10772D09`）。共享泛型使用槽已独立解析：`110FC3DC`为同一实例上下文的List.get_Item，`110FC384`为RemoveAt，`110FC358`为Insert，不能把模糊反汇编别名当缓存语义。

该消费者已有真实上游：`Graphics.cctor` RVA`775990`尾部通过方法槽`110E97E8`（metadata方法7796）取得RemoveExpiredFontTextures，构造实际`System.EventHandler`并调用当前应用的`add_Updated`（`10776686–1077669F`）。`IApplication.Update` RVA`78E3F0`在已核正常段`1078F96A`增加totalFrames，随后于`1078FABD–1078FAF5`取得并调用Updated；这里仅认证该执行路径，不把Unity的每次Update、失焦或提前返回都说成一次世界更新／缓存清理。

`FontTexture.Dispose` RVA`77CE20`在Image非空时调用`Image.Dispose` RVA`79B2A0`，正常返回后将自身Image引用设null（`1077CE27–1077CE44`）；它不清Text或其它值字段。缓存随后移除该FontTexture项，不能由此保证外部持有的Image／FontTexture立即回收。图像Dispose确实继续调用Release，调度与销毁请求如下；异常时该原链没有联合回滚保证。已闭合缓存年龄退休、事件入口和Image引用断开，保留其它间接持有者及全局容量缺口。

### Image释放有明确UI线程与Unity销毁请求

2026-10-14沿实际调用补核，不运行原程序。`Image.Dispose`先调用Unuse，再在`1079B338`调用`Image.Release` RVA`79EA80`。Release只在`image_ +8`非空时构造捕获自身的真实ThreadStart，方法槽`110EB4C4`独立解析为`<Release>b__136_0`；`1079EB3F–1079EB43`以**wait=true**交`IApplication.RunOnUiThread`。不能把它写成后台“发出即成功”的释放，也不能把方法默认参数false代入这个调用。

`RunOnUiThread(ThreadStart,bool)` RVA`78B100`先CheckUiThread：已在UI线程则直接调用；否则在`lockUITask_`锁内加入`uiMethodTasks_`，wait=true时轮询任务finished，并在完成后检查error。这里没有已证等待超时。`CheckUiThread` RVA`782EE0`通常交JThread.IsCurrentThread；UI线程为空且Unity不在playing时另有直接准入分支，不能泛称始终是后台线程。`ExecuteUIMethods` RVA`784C40`在锁内从首项取出并RemoveAt，再于锁外原序调用；正常／已见错误处理都将finished设真，错误保存供等待者回传。任务先从队列退休，捕获的Image仍由执行及等待局部持有到返回，不能等同当场GC。

实际释放闭包RVA`7A0A00`分所有权：若`atlasRegion_.Atlas.type_==Load(1)`，仅将本Image的image_清null，不Destroy共享图集。其余路径对RenderTexture先调用Release（`107A0ABB`），随后对Texture调用`UnityEngine.Object.Destroy`（`107A0ADC`），正常返回后清image_。两包装已核至实际原生入口字符串：RenderTexture RVA`BFF630`→`UnityEngine.RenderTexture::Release()`；Destroy RVA`C0B3D0`→`UnityEngine.Object::Destroy(UnityEngine.Object,System.Single)`，本调用传0.0f。**这是已证的引擎释放／销毁请求，不证明GPU显存当轮归还或原生对象已立即消失。** 本批止于原生调用边界，不追UnityPlayer完整GC实现。

Dispose在图集ID有效且管理器启用时，还调用`ImageAtlasManager.RemoveAtlas` RVA`794530`，同样wait=true交UI线程；闭包`7A3CD0`到`ImageAtlas.RemoveAtlas` RVA`798630`，正常尾执行AtlasRegion.Dispose及regions字典Remove，并置Image图集ID为−1。AtlasRegion.Dispose RVA`793390`断开Image.atlasRegion与自身Atlas／Image／Pixels引用，清Valid／Hint；这条区域退休链不Destroy整个共享图集。Dispose正常尾另清images／优化及源字节／文件等具名引用，清尺寸、置isDisposed，并对静态Image.all_执行List.Remove；不据此宣称全部外部引用已解除。文字／气泡任务数组和计数的保留边界仍按下节，合法持有与泄漏分别判断。

## Image.TextRenderTask是可重复重放的图像文字

2026-10-13补固定Steam静态局部链，不运行原程序、不改原档或复制字体。它与[GLText](STEAM_GLTEXT_LIFECYCLE.md)的active count归零不同：Image任务附在Image对象上，正常绘制不会消费清空，连续绘制同一Image会重放同一批文字。

`TextRenderTask`为24字节值类型，保存Text／X／Y／Anchor／Color／Font；ctor RVA7A2B20原样写字段及Text／Font引用，不自行复制Font。生产者Graphics.DrawString VA1076755A–10767576先由当前font构造独立Font副本，1076778E构造任务，107677B7调用Image.AddTextRenderTask。复制的是Font状态，文字及底层Unity Font对象引用不能由此推断为字体字节深拷贝。

Image构造7A1CF0在+A0建立零长任务数组，+A4计数采用零初值。Add79A2A0在count达到Length时新建Length+1数组，经Array.Copy复制全部旧Length，再将24字节任务写入当前槽，最后count++（1079A32A–1079A34F）。没有本局部硬上限，也不是GLText的峰值池复用。

实际消费者为Graphics._drawBitmap RVA778250：isGUI为真且Image任务count>0时，先把目的矩形压入clip，保存当前颜色／Font；备用fontBuf存在则Copy，否则新建Font副本。PushLocalizeSetting(false)后原序重放：

1. 令`r=min(目的宽/源宽,目的高/源高)`，保持原浮点阶段。
2. 工作Font复制任务Font，以`trunc(float32(usedSize×r+0.01))`调用SetSize(...,false)，不按preset转换；再切当前Font并保留原useFontTexture开关，调用AdjustFontSize(scale)。
3. 设任务颜色，按`x=(task.X−src.left)×r+dst.left`、同理y换算，调用DrawString(null,Text,x,y,Anchor)。该调用仍有自身GUI／GL／纹理分支，不等于直接写像素。

来源锚为107792CB–10779330的clip／保存，10779B7E语言设置栈、10779B91–10779BBF遍历，10779CCD字号及10779DA2实际DrawString。正常和异常续体都调用helper RVA75CB60恢复颜色／Font、保留后端开关并PopLocalizeSetting；只有正常路径10779DF8接PopClip，异常续体10779DC5–10779DEA绕过该步。因此可以确认部分状态恢复，不能宣称原绘制异常完整裁剪回滚或撤销已经绘制的像素。

重放末尾不清Image+A0／+A4、Text或Font。已独立读取Dispose79B2A0、Release79EA80及闭包7A0A00、Clear79AB50／79AC50：它们处理纹理、disposed标志、外部集合或像素；Dispose将touch count+B8清零，但**没有清文字任务count+A4**。若外部仍持有Image，文字／Font引用仍可能保留。本结论不外推全部GC／其它间接writer，也不把合法图像持有直接称为泄漏；产品接线须明确自己的图像与任务生命周期，不能机械套用GLText消费即归零。

## 可消费内容与剩余缺口

产品可按上述契约区分字体对象、后端开关、临时GUIStyle与外层GUISkin恢复，避免SetFont误改后端或认为一次DrawString已恢复所有外部状态。默认资源的Arial引用可用于后续取证，不能直接作为中文字体交付或以另选系统字体冒充原样。

仍缺：真实当前GUI.skin、原生默认皮肤返回对象及中文glyph fallback提供者、DefaultUseFontTexture其它间接／反射／原生写者及真实运行值、纹理实际回收时刻与全局持有者，以及完整游戏窗框的全部页面使用资格。默认开关的具名正常初始化及Config反射目的、缓存年龄退休、Updated事件入口、UI任务消费及Unity销毁请求已核，不能继续将它们整体列为未知；同样不能宣称缓存永久有界。托管空font到原生默认字体的交接已核，不再列为完全未知。TextLayout的14项标签局部生产／消费、普通断行及空串字段分工已有[字体合同](STEAM_FONT_CONSUMER.md)，全部语言禁则、运行注册绑定和实际字形仍未认证。普通位置变换见[文字绘制](STEAM_FONT_DRAW.md)，GUI组／clip见[窗框](STEAM_WINDOW_FRAME.md)，GLText最终消费者／保留对象见[生命周期](STEAM_GLTEXT_LIFECYCLE.md)。窗口观察可补最终画面尺寸和字形表现，但单张截图仍不能证明font对象身份。后续继续静态缩小这些边界，必要动态由既有外部提示词分支取得。
