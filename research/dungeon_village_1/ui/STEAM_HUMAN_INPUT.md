# Steam人物60：外层更新、软标签与触摸值

2026-10-10。[人物四页合同](STEAM_HUMAN_DETAIL.md)的后继局部认证；[研究包](../verification/steam-human-input/README.md)与[证据清单](../verification/steam-human-input/EVIDENCE.json)。固定Steam DLL，未执行原窗口点击，不以APK填补动态平台行为。

## 翻页不会在已核Update路径改写标签

外层`SubForm.Update`先更新共有字段、表现帧，再按type分派。raw60走默认`0x10321E97→Update2()`，随后直接返回；这个实际路径没有`SetSoftLabel`或page条件标签写入。此前已核raw60的Update2整个局部也不写软标签，只有分页、选择、确认与标签输入监听。

因此不能因页1监听标签11“転職”，便自动把左软按钮改成“转职”。本批闭合的初始与更新链是：

1. `Init`通用`0x10312643–0x103126F2`：用type索引SubForm两列静态表，再以这两个数字索引AppData字符串表；`SetSoftLabel(left,right,null,null)`。
2. raw60表项为`[8,2]`，“追跡／戻る”；外层Init随后`0x10312808→Init2()`。
3. raw60 Init2在`charaInfoChase_==0`时改为`[0,2]`，左为空；并执行教程111／战斗缓存重算。此链见人物四页原合同。
4. 在已核普通翻页及选择的Update/Update2路径中保留该标签状态；没有“每到页1自动重配label11”的代码。收到标签11输入能走职业目录61，是监听资格事实，单独不能证明label11在本页真实可见。

这里只闭合上述实际回路，不能升级成“任何平台／任何外部回调都绝不改标签”。绘制wrapper和`_draw`前缀已核没有相应标签写入，但后续`_draw1_*`分派链及平台展示覆盖不是本包完整分母。原窗口实际底栏仍可独立交叉。

外层Update还每轮对common SEB22、21、23调用`Frame()`，包括人物装备页使用的SEB21手指。不能把原SEB手指永远固定成frame0，或只因世界暂停就停掉所有表单表现帧；调用频率与墙钟秒数仍不同。

## 触摸值的选择与确认是两层

人物装备绘制的四行注册ID_LIST10、value=`0x20000|row`，原调用只创建普通TouchOption并设置Margin，见人物四页合同。本批新增核验如下：

| 路径 | 已核行为 |
|---|---|
| `SubForm.GetTouchValue` | 高16位`0x20000→select_`、`0x40000→scroll_`、`0x50000→page_`；其他委托基类 |
| `SubForm.SetTouchValue` | 对上述三类直接写对应字段；不是把行index当页号 |
| `GameView.Set(int,component)` | 取顶层表单，分离高16位类型和**有符号低16位**值，委托TouchValueAccessor；可处理组件附加值数组 |
| `SubForm.OnTouchEvent` | 特殊建设更新仅type21；人物60在一般路径返回true，让GameView继续。raw60不会调用`UpdateSoftKeyBuildWIndow` |
| `GameView.OnTouchEvent` | 先询问顶层MyFormBase.OnTouchEvent；返回false则停止。跳表中component3、10、11均到`0x1023CA42`列表分支 |

列表分支在事件1（UP）和2（ENTER）先写选择；仅UP继续确认检查。若组件没有marker位16，则走确认；若有marker位16，当前组件key等于此前标记key才确认，否则只记录新key。key由component ID/value构成，不依赖每帧新建对象的地址，也没有在这里做“双击时间间隔”判断。确认键默认`0x100000`，存在option自定义key数组时用其首项。

`OnTouchDlgSel`、`OnTouchList`、`OnTouchScrollList`在方法索引中共享RVA `0x23C730`，同样实现上述先选择／后判marker／再发KeyClick。组件10不是人物专用新协议，不能把普通列表和SLIST11的不同调用option合并成统一“必须二次点击”。人物四行调用本身没有请求marker16；组件最终flags默认合成、焦点、拖拽取消及完整平台输入时序仍按[通用交互合同](STEAM_INTERACTIONS.md)和其后继认证处理，本包不冒充真实单击验收。

原Get/Set对传入值没有新增范围夹取，低16位还可能为负。维护Owner／Session的严格载荷验证不能因此取消；非法row/page的显式拒绝属于已确认维护安全边界，不能复制原未校验赋值后靠数组异常兜底。

## 逻辑原点与范围

`SubForm.Draw`wrapper读取表单基础原点；屏幕超过240逻辑尺寸且不属于所列特殊底层表单／type组合时，将`trunc((width-240)/2)`、`trunc((height-240)/2)`加到原点，`SetOrigin`后调用`_draw`。raw60不在wrapper特殊type名单中。`_draw`有限前缀再按type把60委托`_draw1_1`；本包没有顺着整个巨型方法扫描其他窗口。

因此人物合同的`(5,y,225,18)`等坐标都不是桌面绝对坐标，触摸命中还经过绘制原点、平台缩放与组件裁剪。未得到运行时尺寸／DPI证据时不能直接输出可执行点击坐标。

## 当前认证边界

已完成本次具体疑问：**外层Update不是人物页按page动态改软标签的来源**；初始标签、无追踪覆盖、三种触摸值及列表选择／确认分层有静态证据。未声称完整绘制分派、最终组件flags、原窗口实操、字体像素或全部UI皮肤完成。

本包无新增图像、C++或世界状态实现；没有规则随机调用结论。共享手指SEB推进、触摸组件注册与回收分别交给Owner表现／框架生命周期，不能把方法局部没有容器增长当作整个应用永久有界。
