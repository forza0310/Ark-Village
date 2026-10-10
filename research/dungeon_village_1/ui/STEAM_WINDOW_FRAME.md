# Steam窗口框、内容框与裁剪边界

2026-10-10[外部皮肤反馈接收](../verification/skin-observation-analysis/ANALYSIS.md)增加Steam2.56纪录两页木纹框及蓝色退出确认框外观。16项清单／11图完整性通过；原图含标题栏与纹理带，不把1067×910当逻辑客户区或由截图反定helper参数。经营详情三类仍未取得，本页静态身份不因可见版本号被追认为该运行DLL相同。

2026-10-09。固定Steam2.56的有界静态合同：19个具名方法、13,264字节，复用标题／raw1调用证据及5份已出版同字节资源。证据和复算入口见[工作包](../verification/steam-window-frame-contract/README.md)。本页不运行原游戏、不改C++或产品，不把APK同图当Steam同调用；本批已独立核到Steam自己的helper和Graphics消费者。

## 木纹标题窗的完整helper

`AppData.DrawWindow`默认重载RVA0x252050令off_y=0，实际入口RVA0x252080。参数GWidth=W、GHeight=H、GTop=v、off_y=s；`GameForm.VIEW_Y`来自其静态区偏移0x0C。整数除法分别向零截断：

```text
L = 120 - trunc(W/2)
I = H > 216 ? 0 : VIEW_Y + 2*s
T = trunc((I+240)/2) - trunc(H/2) + v
```

这是原逻辑240基坐标，页面原点转换仍由调用者执行。`off_y`虽然名为偏移，参与的是I分支，不等于无条件在最终T上加s；H>216时这项不使用。

按实际调用顺序展开：

| 层 | Steam实参／资源 |
| --- | --- |
| 外边线 | 色(89,103,91)，`DrawRect(L-1,T-2,W+1,H+2)` |
| 内边线 | 色(239,239,221)，`DrawRect(L,T-1,W-1,H)` |
| 木纹 | common图片28，源(0,0,min(40,剩余宽),H)，目标(L+40*i,T)；循环`trunc(W/40)+1`，保留原零宽尾请求事实 |
| 标题带 | common图片29，源(L+1,0,W-2,17)，目标(L+1,T)；沿横坐标裁取，不是缩放整图 |
| 标题阴影 | T_S非null时，当前Font.StringWidth整数测宽q，x=120-trunc(q/2)、y=T+3，SC_WINDOW_TITLE2=(44,54,105) |
| 标题正文 | 再次调用StringWidth，x同式、y=T+2，SC_WINDOW_TITLE=(247,253,247) |

两个测宽调用均真实存在；不能用当前PNG列数或字符串字节数替代。空字符串与null的分支不同：null跳过标题两次绘制，木纹和标题带仍绘制。helper没有自行设clip、弹clip、改字号、恢复旧颜色或恢复旧字体；实际文本仍走[Steam翻译／字体链](STEAM_FONT_CONSUMER.md)。长标题不因窗框宽度自动受该窗裁剪，也不从helper推出自动缩小字号。

同图资源已核：图片28 `wnd_back.png`为40×240、图片29 `wnd_bar.png`为240×17；Steam目录身份与[原出版副本](../assets/original/common/)逐字节一致，hash见证据。当前正常宽高使用范围能直接消费；原helper未提供维护C++的W≤240／H≤240等严格保护，不能把维护保护记为原规则。

## DrawRect不是默认线宽的stroke

Steam `Graphics.DrawRect` RVA0x763590首先经过BeginRenderMatrix、UpdateGLClip，再形成两个矩形：外侧(x,y,w+1,h+1)，内侧(x+1,y+1,w-1,h-1)。常量从源0x10DDE55C读得float32 1.0，不靠APK推断。

当前GUI路径用四个四边形连接外／内顶点；mesh路径加入8顶点及24索引，仍是四边环。`FillRect` RVA0x76A990则直接使用(x,y,w,h)单个矩形，不额外加1。后续屏幕矩阵、y翻转、gravity点及材质仍在Graphics/Unity层，因此这里只认证变换前逻辑几何，不把逻辑一单位许诺成任意DPI下恰一物理像素。

维护半开尺寸表示可据此将标题外线登记为(L-1,T-2,W+2,H+3)，内线为(L,T-1,W,H+1)；与APK已核标准框几何相容，但本页依据是Steam两层调用。直接用raylib默认居中stroke可能偏出半像素，不能只把两个DrawRect参数照搬成填充矩形。

## 内容框、白角与另外两个木纹helper

`AppData.DrawBox`默认重载RVA0x24B010传mode0，实际RVA0x24B040。输入为边界X1/Y1/X2/Y2；令w=X2-X1、h=Y2-Y1，两个Y均加trunc(VIEW_Y/2)。先填(247,253,247)，再外线(172,202,179)、内线(222,234,225)，然后画角：

| 图元 | 原实参 |
| --- | --- |
| 填充 | FillRect(X1,Y1,w,h) |
| 外线 | DrawRect(X1,Y1,w-1,h-1) |
| 内线 | DrawRect(X1+1,Y1+1,w-3,h-3) |
| 角 | common SEB6帧0/1/2/3，锚依序左上／右上／右下／左下 |

mode0使用SEB自带图片30 `wnd_conner.png`。mode1通过DrawSeb的显式Image重载替换为图片121 `wnd_conner01.png`，仍用同一SEB6的帧／裁片／偏移；`DrawBox_whiteConner` RVA0x24B4F0就是传mode1。其他mode完成填充／双线后直接返回，不绘角；它们不是自动回退mode0。

两角图均8×8。SEB共4个单层帧，源依序(0,0,4,4)、(4,0,4,4)、(4,4,4,4)、(0,4,4,4)，偏移依序(0,0)、(-4,0)、(-4,-4)、(0,-4)，无翻转。标准角资源hash `15f03b08…`，白角`01c1490a…`，SEB `cfab2deb…`；完整hash和图片槽映射见证据，不把文件名white误推成纯白填充。

`DrawWindow2` RVA0x251D10仅平铺木纹，无两条边线／标题带／文字；T用上述s=0公式。`DrawWindow3` RVA0x251E70以显式dx/dy画外线(dx-1,dy-1,W+1,H+2)、内线(dx,dy,W-1,H)，随后木纹裁高为H+1，无标题带／文字。这两个helper不能用“普通窗口换空标题”替代；后者仍会画标题带。

## 当前菜单计划的留空样式与缺口

对照[Steam只读计划](../prototype/include/dungeon_village_prototype/steam_startup_skin.hpp)，下列原调用已经具名，可在后继实现批替换留空字段；本静态批不修改其接口或已冻结包。

| 当前role／helper | 可闭合内容 | 原证据 |
| --- | --- | --- |
| SteamStartupWindow | 上述完整木纹／双线／标题两层；消息窗style0，选档窗style-12 | 0x10252080及已有raw1／TitleForm调用 |
| SteamStartupBox | 默认mode0、三矩形＋4角；右／下是边界 | 0x1024B010／0x1024B040 |
| slot_type | RGB(60,100,200)、anchor2；点(120,V+69) | TitleForm 0x1020F824–0x1020F8AD |
| slot_name | RGB(30,30,30)，原TextLayout anchor0x22保持 | 0x1021025C–0x10210321 |
| slot_date | RGB(30,30,30)，经TextFormat的原重载；不因右端资金anchor4而给日期也填4 | 0x1021037A–0x1021054B |
| slot_cash | RGB(30,30,30)、anchor4，点(201,R+24) | 0x102105D7–0x102105F4 |
| empty_slot两行 | RGB(30,30,30)、anchor2，点(126,R+15) | 0x1020FB0F–0x1020FB50、0x1020FF0E–0x1020FF51 |
| raw1 answer两按钮 | 每个按钮绘字前重新SetColor(SC_WINDOW_BLACK)=(92,51,31)，anchor2；选中橙底不会泄漏为文字色 | 0x1035708D–0x103570F6 |

新增发现：**空手动栏且被选中时，原TitleForm还画RGB(200,200,255)底色(85,R+14,82,14)**，证据0x1020F955–0x1020F9C8。当前`steam_save_selector_skin`只给非空栏加底色，空手动分支缺这一图元。它与非空栏日文宽94／非日文宽102的矩形不同，不能复用其位置来凑齐。本页登记具体缺口，未称已修复。

## clip从逻辑矩形到字体队列

DrawWindow／DrawBox自己不创建内容裁剪范围；图形与文字继承调用方clip。标题选档的特殊调用尤其容易误读：VA0x1020F867调用`ClipRect(0,V+65,240,20,true)`，**0x1020F86F立即PopClip，之后0x1020F8AD才画类型说明**，中间没有文字绘制。不能把这个临时clip错误套到slot_type或整个保存窗。

本批核到的裁剪合同为：

- `SetClip`经过BeginRenderMatrix和GetTransRect，最后替换clip，不与旧值相交。GetTransRect先形成两对角边界，经当前screenMatrix.MapPoints分别变换，再交给RectangleF.Set；本批不补写所有旋转／归一化内部政策。
- `ClipRect(push=true)`先PushClip，再变换请求矩形，与当前浮点clipRect计算左／上max、右／下min，差值夹不小于0；最后调用同一`_setClip`。
- `_setClip`保存四个浮点值，转整数边界时是对x、y、x+w、y+h分别**先float32加约0.01、再向零截断**，不是四舍五入0.5，也不是直接对w/h单独取整。原常量位型0x3C23D70A，VA0x10DDE54C。
- PushClip把四float复制到按深度复用的槽；PopClip深度≤0不操作，否则减深度并沿`_setClip`恢复。它不销毁保留栈容量；不能据当前深度0宣称相关资源全部释放。
- UpdateGUIGroup在figure clip设置／使用期间直接返回；否则可复用相等旧区域。切换时先EndGroup旧组，再按整数clipRegion开启GUI.BeginGroup。GLText flush逐条恢复其入队时复制的clip并调用此消费者，最后恢复调用前clip和GUI样式，详见[GLText生命周期](STEAM_GLTEXT_LIFECYCLE.md)。

图形DrawRect／FillRect走UpdateGLClip；它与GUI.BeginGroup不是同一后端。UpdateGLClip可能用即时GL遮罩、meshOnly flush＋FillClipRect或Offscreen flush路径，受clipEnable／isGUI／glRender／offscreen条件控制；本批只认证此分派和字段读取，不冒称全部材质／shader／Unity裁剪已经还原。文字字体的实际中文fallback、glyph测宽与栅格化仍沿[字体后端限制](STEAM_FONT_BACKEND.md)，不能用上述clip公式补一个“原Steam字体”。

## 交付边界

已闭合的是当前保存／询问窗使用的helper几何、原资源、显式颜色／对齐与裁剪顺序；Steam框架物理坐标、实际GUI后端、TextLayout全部标签／基线和所有页面调用仍独立未决。这里没有修改维护世界、随机、菜单所有权或文件格式，原窗口观察与离线计划仍是不同验收等级。

资源未重复发布，5项均复核现有副本和Steam索引；新证据只保存方法／窗口hash及必要常量，没有落盘整段反编译代码。查询输出、clip池容量与世界历史的增长分别审核，不宣称本批证明整个游戏永久有界。
