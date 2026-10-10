# Steam GLText队列的绘制与退休

2026-10-09。接续[文字后端](STEAM_FONT_BACKEND.md)，本批选择GLText这一条较短消费者链，核到最终托管绘制调用与消费后保留对象；不扩大为Image.TextRenderTask、完整GL栅格化或所有字体缓存审计。来源是固定Steam2.56 DLL／metadata，新增7个具名方法4368字节，复用已核DrawString生产者4096字节窗口。证据见专题包。没有运行原程序、访问系统字体／用户设置或修改产品。

关键结论：**这里的GLText不是逐字生成GL网格。它先保存文字命令，FlushGLRender提交网格后，仍逐条调用Unity GUI.Label。文字消费以active count归零表示，池和文字／Font引用没有同时清空。**

## 入队保存了什么

前批已核`Graphics.DrawString`在`useFontTexture=false && glRender=true`时进入GLText分支（VA `0x10767043`）：

- 若`glTextCount_ >= glTextList_.Count`，新建GLText并Add；否则取已有第glTextCount项复用。共享泛型槽已独立解析为同一List实例类型的Add／get_Item。
- 用本轮已算好的Rect、翻译及字符变换后的text、当前GUIStyle与clip调用GLText.Init，然后增加glTextCount。不会为每条命令持有一个完整GUIStyle副本。
- `GLText.Init`复制Rect、字号、FontStyle、textColor；分别保留当前文字String与GUIStyle.font的对象引用。
- 构造函数为每个槽建一个独立Rectangle。clip非null时，把四个数值字段拷到这个Rectangle，并令IsClip=true；clip为空时只令IsClip=false，旧Rectangle数值可保留但本次不消费。
- Log标志也在入队时从原Log设置采样；不是flush时重新决定每条是否计入日志。

因此后续改变外部GUIStyle或原clip矩形，不会重写已入队命令的字号／样式／颜色／四边。Font与String是对象引用，不是本批已证的字体字节深复制；原UnityFont对象自身生命周期仍由引擎管理。

## Flush的准入、原序和正常恢复

`Graphics.FlushGLRender(meshOnly=false)`（RVA `0x76BCD0`）仅在offscreen按Unity对象比较为空、glRender=true且isGUI=true时工作。任一前提不满足即返回，本批不把它当作所有图形目标通用的文字消费函数。

通过前提后：

1. 按meshManList原数组顺序，对非null项EndMesh、RenderMesh。矩阵合成与相机深度在该阶段；本批只核先网格后文字的调用顺序，不认证完整矩阵数学。
2. `meshOnly=true`直接跳过文字。文字的glTextCount及槽内内容保留，留给后续完整flush。
3. `meshOnly=false && glTextCount>0`时，保存当前clipRegion和GUIStyle的font／fontStyle／fontSize／normal.textColor。
4. 对`[0,glTextCount)`依次读取GLText。根据IsClip把Graphics.clipRegion临时换成该槽的Clip或null，调用UpdateGUIGroup；再按槽内快照设置GUIStyle的font／style／size／color。
5. 使用槽内Position与Text，实际调用`UnityEngine.GUI.Label(Rect,string,GUIStyle)`（VA `0x1076C127`）。需要时记录一次文字DrawCall。
6. 全部文字正常完成后将glTextCount置0，恢复此前GUIStyle四项和clipRegion，再次UpdateGUIGroup。
7. 调GLClear，参数是clearDepth=true、clearColor=false、clearStencil=false、color=-1；之后重新BeginMesh，准备后续网格。这里没有“擦掉刚画文字”的清色调用。

暂存及恢复的是Graphics当前GUIStyle和clip引用；这与外层OnGUI恢复GUISkin.font是不同层。最终GUI.Label的字形选择、null font回退、抗锯齿、底层字体纹理仍停在Unity边界，不因GLText名含GL而认定独立中文字体算法已找到。

## Begin／End不是每次一条命令

`BeginGLRender(clipEnable)`（`0x75CFA0`）同样要求无offscreen且isGUI。首次进入时BeginMesh、清active文字计数并置glRender=true；已在GL模式时先完整FlushGLRender(false)。随后增加嵌套计数、重置render cache，并压入／安装本次clipEnable。

`EndGLRender(force=false)`（`0x768E00`）要求当前GL模式有效；先PopClipEnable、减嵌套计数。计数≤0或force=true才完整flush并把嵌套计数与glRender清零；否则保留外层GL模式。最后重置render cache。这里的force不是已经证明能一次展开所有嵌套clip栈，不应把它改写成“无条件清掉所有图形栈”。

原方法没有维护Owner式事务：若某条Label／setter／分组调用抛异常，已产生的画面不会回滚；active count在整个文字循环结束前还没有归零。异常续体只见Profiler结束与重新抛出，不包含正常路径完整的GUIStyle／clip恢复。因此不能原样重试一次失败flush并声称前几条不会重复，也不能把这些表现副作用塞进维护世界事务保证。

## 退休与资源规模

| 时点 | active文字数 | GLText对象池／引用 |
| --- | --- | --- |
| 普通入队 | 增加1 | 超过已有Count才新建；否则覆盖旧槽 |
| meshOnly flush | 保留 | 保留全部待消费数据 |
| 成功完整flush | 置0 | list.Count及槽内Text／Font引用仍保留 |
| 下一批较少文字 | 从第0槽覆盖 | 没用到的旧高位槽仍可保留上一批引用 |
| Graphics.Dispose(false) | 本方法没有清理文字计数的操作 | 仅清MeshManager并重建mesh数组，不清GLText池 |

沿已核链条，GLText池跟随历史未消费文字峰值复用，不是每一轮无限Add；但**没有给出全局固定上限**。消费count为0不代表字符串与Font引用已经释放；合法池保留、容量扩张、异常后未消费与实际泄漏必须分开。Graphics整体对象被外部持有多久及GC最终释放不在本批覆盖内。

`ResetRender`的有限方法也未见GLText池清空；它重置GUI与图形状态，不能仅因名称Reset就当作完成文字引用退休。该发现不会改变既有研究资源预算，更不允许删审计历史或放宽128MiB回放门槛。

## 产品消费及后继

可以据此建立独立表现命令的快照边界：保存每条文字所需的Font身份、字号、样式、颜色、位置及独立clip值，区分待消费数量和复用容量；成功消费后的引用释放／容量政策如与原池不同，应明确登记为维护策略。本批没有新增C++接口、快照字段或产品实现。

这条GLText链已闭合到最后托管GUI.Label及原局部退休政策。剩余的是Unity实际字形／栅格化、UpdateGUIGroup的最终clip坐标、上游完整矩阵、全Graphics对象生命周期，以及独立Image.TextRenderTask分支；不再把“GLText最终调用者完全未知”重复登记为缺口。
