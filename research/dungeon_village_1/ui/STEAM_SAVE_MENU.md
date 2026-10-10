# Steam手动档操作菜单与确认

2026-10-09。本合同接续[标题与选档](STEAM_TITLE_MENU.md)，只核固定Steam样本raw20及其raw1确认链。不操作原存档、不执行删除／覆盖，不将确认文字当成文件已经写入。新方法与旧证据交叉见[工作入口](../verification/steam-save-menu-contract/README.md)。

## 页面与结果生命周期

raw20是`SubForm.TYPE_SAVE_MENU`，FormBase ID仍为4；它不是系统保存raw14。标题非空手动行创建raw20，x136、y144，`value1_`携带所选槽号。构造置`select_=0`、`result_=-1`、`untilActiveHide_=true`；Init安装软标签索引`[0,2]`、清`frame_`，再按原序加入三项：

| 行／菜单ID | 原始字符串 | state | 选择后的动作 |
|---|---|---:|---|
| 0／25 | `続きから` | 1 | 写raw20.result=0，直接请求Pop |
| 1／26 | `最初から` | 1 | 写result=1，推raw1覆盖询问，默认选“否” |
| 2／27 | `削除する` | 1 | 写result=2，推raw1删除询问，默认选“否” |

这些是metadata原literal，经Steam绘制翻译链消费，不能直接把表中日文或维护中文译名当用户运行页面的最终文字。

raw1两按钮字符串为`はい\tいいえ`，按Tab拆分，默认选择索引1。覆盖询问是`今のセーブデータに<br>上書きされます`；删除询问是`今のセーブデータは<br>消えてしまいます`。两种操作都先挂在raw20.dialog_，原菜单还存在；询问本身未调用保存、删除或新局。

raw1确认把当前选择写入自身result后Pop。返回raw20时，子result为0才再Pop raw20；子结果为1或取消保留的-1，清dialog_引用后留在原菜单。父菜单选择没有在此分支重置。raw20预先写下的1/2只是待返回动作；只有退出raw20、标题再次消费该结果时，才分别推raw91新局配置或清对应目录日期并SaveSystem。正常raw20取消由`FrameMenu(-1)`明确重写result=-1后Pop，不能让上次被否决的result1/2泄漏给标题。

继续没有额外raw1确认：标题消费result0后调用ContinueGame，再按[存档合同](../rules/PERSISTENCE.md)读入。覆盖“是”只是进入raw91配置，不能在这一步提前初始化世界／写游戏档；删除“是”对应标题目录处理，不能把它泛化成清空全部手动／中断栏或文件迁移。

`Pop`不是直接delete对象：具名FormManager.Pop通过UI线程处理，目标`formProcMode_`标记为4，并从栈尾找未标4的页恢复四个软标签；遇父页模式3则置1。物理列表退休和后续生命周期由框架继续完成。本合同以页面结果和恢复父页为边界，不把索引指针清零写成对象已经释放，也不宣称框架线程事务等价维护Owner事务。

## 输入：菜单行与询问按钮分开

raw20使用`FrameMenu(-1)`：Up/Down的repeat位为`0x20000/0x80000`，在三行内循环；确认只检查`CheckKeyPulse(0x100000)`。Right作为确认、Left作为返回的快捷分支只列3/4/5/7/8/9/10，**不包含raw20**。raw20返回使用软标签2。默认确认键映射继续沿用[Steam输入合同](STEAM_INTERACTIONS.md)，不能把这页补成所有方向键都确认。

raw20行注册`ID_SUBMENU=9`，value=`0x20000|行号`。已核分发表第9项直达`0x1023C9CD`：ENTER2或UP1先把行号写给当前栈顶，只有UP继续发默认确认脉冲。此支没有marker比较，DOWN0不在行处理器中确认。不是设施75的首次UP标记，也不是主菜单ID8寻找底层raw3并弹出旧子菜单的分支。

绘制末尾另注册`ID_KEYCLICK=4,value=22,TouchOption.Create(2)`；该处理器消费CLICK5并发`1<<22`逻辑键。它与行UP分别登记，不能把注册全局组件等同“任意按下即取消”；最终覆盖／hit分流仍由已有surface合同决定。

raw1询问的Left/Right repeat切换按钮，确认写选择后关闭；软标签2直接关闭，不改初始result=-1。询问按钮注册ID_DLGSEL3，无显式marker option，ENTER/UP写选择、UP确认。不要复用raw20的上下列表导航给横向“是／否”。原SubForm.ProcessBackKey有其它演出特判，但未给raw20注入确认，不能用它替代软标签取消合同。

## raw20菜单皮肤

标题根页的FormBase ID为1。原SubForm.Draw取x_/y_为基原点：当逻辑W或H大于240且非根id9特例时，加`trunc((W-240)/2)`、`trunc((H-240)/2)`。标题手动菜单因此从`(136,144)`加居中偏移；这些仍是逻辑尺寸，不是截图像素。Draw包装有未具名finally helper，本批不以它猜测返回原点复原语义。

raw20满足IsMenu，调用`DrawMenu2(g,type=1)`。该菜单**按common SEB0的整行图绘制**，不是DrawWindow木框。三行高度依据`3*28+5=89`做边界适配；行间隔28。

令进入DrawMenu2的原点为`Ox/Oy`，游戏逻辑尺寸为W/H。菜单宽参数m：日文84，非日文90。日文WIDTH_DATA[0]由Steam默认数组独立解析为`[63,84,42]`，非日文[1]的机器码初始化为`[91,90,42]`，本页取索引1；不借APK填写。

raw20不加无边框safe-area起始padding。局部修正dx初始0，若`Ox+m>W`则`dx=W-m-Ox`；dy初始0，若`Oy+89>H`则`dy=H-89-Oy`。以下r=0/1/2，`Y=dy+28*r`，均在上述原点之内：

| 元素 | 原参数与分支 |
|---|---|
| 行底图 | common SEB0，选中frame2、非选中frame3，位置`(dx+1,Y+1)`；横纵scale均`trunc(frame_*1000/3)` |
| 行文字 | `frame_>=3`才画；`DrawString(MENU_STR[id],dx+7,Y+9)`；选中RGB(76,58,50)，非选中RGB(255,242,220) |
| 过长文字 | 实测当前Font.StringWidth若大于`m-10`，临时请求字号11，画后ResetFontSize；不是固定裁短或无条件缩放 |
| 行触摸基矩形 | id9，`(dx+40,Y,m-40,28)`，value=`0x20000|r`；后续_addTouch仍处理margin／原点／重叠／尺度 |
| 选中手形 | `frame_>=3`且本页栈顶；dx<120时common SEB22在`(dx+m+6,Y+13)`，否则SEB21在`(dx-2,Y+13)`；使用当前SEB帧 |

FrameMenu先加一并钳`frame_<=3`，Update前段还会增加frame_；不要由scale公式倒推出从首次可见帧开始必须经历0/1/2/3四张截图。3只是局部动画门槛，不转换为固定秒数。

common/menu.seb和menu.png在Steam与现有APK发布副本逐字节相同。SEB共4帧，图像ID25；本页frame2源裁片`(68,0,89,29)`，frame3为`(68,29,89,29)`，偏移0。**宽参数84/90是菜单布局参数，裁片宽89是资源事实，两者不能互相改值凑齐。**两项当前可消费路径为[menu.seb](../assets/original/common/menu.seb)、[menu.png](../assets/original/common/menu.png)，对应Steam证据见工作EVIDENCE；它们不在先前38项启动包清单内，不改变已发布包分母。手形SEB21/22共用70号finger_r.png。

raw20还可走DrawMenu2后部的任务摘要准入判断，但已核新标题初始化世界不含选中任务；不能把该通用helper的游戏内摘要条当标题手动菜单必画内容。

## raw1询问皮肤与遮挡

原菜单上方出现raw1时，raw20的菜单绘制资格因“栈顶为非菜单SubForm”而退出；标题选档窗也只保留到栈顶为菜单，故询问页不是透明叠加三个仍可交互的原菜单。标题背景／Logo和表单整栈其他绘制仍按对应合同。

raw1构造后InitDialog设置x/y=0、select=0，再由MakeDialog覆盖select=1。其Draw调用`DrawWindow(210,110,0,"メッセージ")`与`DrawBox(26,90,213,148)`。正文TextLayout矩形`(26,115,187,32)`、lineSpace6、anchor0x22，色RGB(92,51,31)，保留`<br>`交给文本布局。

两按钮中心分别x84和156（一般公式`120-36*(N-1)+72*r`），文字y167、anchor2。令b为所有按钮当前`StringWidthF`的最大值再加2：选中背景`(center-b/2-5,164,b+10,17)`，色RGB(255,153,55)。按钮触摸基矩形x=`trunc(center-b/2-105)`、y64、宽=`trunc(b+210)`、高217。宽触摸区域来自原调用实参，最终交叠裁剪须经surface，不直接作为产品OS热区。

## raw14保存页的局部皮肤

原生命周期与维护文件桥见[保存合同](../rules/PERSISTENCE.md#steam手动保存页的局部交叉)。`SubForm.InitDialog` RVA `3123C0`置x／y／selection为0，空按钮参数替换为“了解”再按Tab拆分；raw14只有这一项。标题原文“メッセージ”、正文“ゲームセーブ中”／“ゲームセーブ完了”经过原翻译链，维护中文与原Unity字体分开登记。

`_draw` RVA `352E20`在VA `10353076–103530D7`先检查untilActiveHide／wait>0；raw14不在IsMenu集合，非顶页整页不画。`103531AF`分派到`10356DE9`，依次在`10356E0E/10356E33/10356EB1`绘制与上述raw1相同的210×110木窗、内容框及正文布局。state2才进入`10356EC2`后按钮段；前两阶段不测按钮文字，也不登记按钮触摸。ESI来源为`10353193`的GetFont，后续非空检查是字体资格，不是第二个顶页条件。

单按钮中心(120,167)、anchor2、色(92,51,31)。令b为实际StringWidthF("了解")+2，橙色块为(120−b/2−5,164,b+10,17)；组件3、value0x20000的触摸基矩形为(trunc(120−b/2−105),64,trunc(b+210),217)。按钮的原调用锚为`10356F47/10357085/103570F6/10357171`；没有按钮PNG、手形或raw20的KEYCLICK共尾。大触摸范围保留在计划中，平台仍须负责命中和事件消费，不能让一次点击同时落入背景控件与按钮。

Draw包装RVA `30CE30`在任一画布维大于240时共同加两轴居中偏移；raw14不属于root ID9菜单的免居中集合，`1030CFB4` Translate后进入_draw。本路径不设置字号。**GameForm已核静态初值VIEW_Y=23**，仅Window／Box helper消费它，正文与按钮不再加一次：木窗76..186，内容框(26,101,213,159)，按钮164..181位于木框内；不能按VIEW_Y0条件图的越界观感改变原210／110／0实参。

正文的(26,115,187,32)是布局区域，y115为上边；anchor0x22令每行横向居中，并使内容高不足32时垂直居中。普通同字号无标签的n行总高为`n*Font.usedSize_+(n-1)*6`，首行y为`115+max(0,(32-总高)/2)`，不以白框90..148直接居中。`TextLayout._makeTextBlocks` RVA `7CD750`的VA `107CF3A3–107CF3DC`、`107CF510–107CF55F`分别给横向、垂直与负偏移夹零；`107CF06D–107CF07D`加行高／间距，`107CF4C2–107CF4D4`末行不加尾间距。静态Draw RVA `7CA340`在`107CA97F–107CA9C2`另加`(调用者字号−第一块字号)*0.25`，同字号为0；块绘制用anchor0x11。混合标签、缩字和原字体后端仍不能套用这个普通文本简式。

维护`steam_save_page_skin`与raw1复用已核对话图元，阶段／隐藏／真实测宽由输入提供，不写Owner、文件或随机。研究窗口已消费原木框／单按钮计划，使用Noto12及普通中文正文适配；[三状态例图](examples/steam-save-page.png)为明确条件输入和替代字体，均非原Steam截图或GUI保存全链认证。

## 文本翻译的已核边界

原菜单字符串先进入Font.StringWidth和Graphics.DrawString；后者并非直接把日文画到屏幕，而是调用`Language.TranslateText(text,true)`后才处理文本标签／字体／绘制。直接LT包装也走翻译、参数替换与缓存，因此UI显示语言、基础Language.Get、资源目录选择不能混成一件事。

本轮已核有限翻译链：缓存命中先返回；未命中走CallTranslateText，依据translateEnable_处理整段或Tab分段、换行／标签规范化；`_translateText2`先查当前textTable_，再按基础语言查内置表，必要时回退第1列；当langPackRecords_存在且内置行至少3列时改取第2列；之后还有softlabel／console与强制正则替换支路。外层另保留内置多语言覆盖条件，不能简化成“中文取所有表第2列”。

LoadTranslateTable允许已注册回调替代默认入口；默认才是Storage媒体3的`xls.dat`，读`softkey.txt`与`text.txt`。本批只认证此调用路径，未执行Storage，也未把默认入口当当前Steam中文会话真实来源。当前语言包的安装／选择、翻译回调注册者、实际中文译文与字体fallback仍需后续具名消费者／原窗口交叉；没有解码Language.cctor的77,600字节来猜字体。

本轮可交付raw20原序、结果/确认/取消链、两类输入、菜单与询问皮肤的已列参数；没有新增窗口验收，没有实现产品菜单。维护Owner应以具名子页与已消费结果协调加载／新局／删除，非法槽号、错误父页、过期答案显式拒绝；原result预存和框架标记退休机制不是维护事务实现模板。
