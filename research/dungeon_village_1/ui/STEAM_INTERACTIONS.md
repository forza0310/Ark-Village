# Steam 2.56：存档入口与设施道具交互

2026-10-07，范围为存档栏、菜单保存、设施详情／道具／结果的交互关系。
依据本机 Steam 原版窗口实验与同身份 IL2CPP 局部机器码；固定 APK 1.0.8 仍是规则基线。
本批没有再次启动游戏、操作当前原档或修改产品；“EXE 交互”实际定位于 `GameAssembly.dll`，不是启动器。

## 来源与证据等级

- Windows：实验标题显示 ver 2.56、中文，appID 1859360、buildID 14371088。
  DLL SHA-256 `9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a`；
  metadata SHA-256 `80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369`。
- 窗口原记录：[RESULT.md](../work/window-restore-observation/20261007-102645-backup-restore-trials/RESULT.md)、
  [ACTIONS.json](../work/window-restore-observation/20261007-102645-backup-restore-trials/ACTIONS.json)；
  设计素材见[截图归档](../references/screenshots/2026-10-07-steam-restore/README.md)。
  没有录像，动作日志时间不等于内部调用计数。截图尺寸不是已测逻辑分辨率或 DPI。
- 静态定位：复用 Il2CppDumper 6.7.46 方法映射，用 iced-x86 1.21.0 解码19个白名单方法、89,840字节，
  详细身份、参数、机器码哈希及地址见[本地证据](../work/steam-interaction-analysis/README.md)。
  元数据只认证类型／字段名，下面标为静态的分支同时检查了机器码。
- APK：对应[设施道具合同](../rules/FACILITY_EFFECTS.md#items-ui-调用与消费次序)及[UI入口](README.md)。
  Steam 的 `SubForm.TYPE_*` 数值是本样本 metadata 独立核出的常量；与 APK raw 页码相同不代表全部行为等价。
- 后续静态收口：[输入链](../work/steam-input-closure/README.md)24方法／93,104字节，
  [设施初始化及动画表](../work/facility-animation-closure/README.md)5方法／45,536字节。
  两批均以同身份机器码验证；动画表另从metadata原字节独立读取，不借用APK数值。

窗口实验曾请求启动复制目录，但 Steam 实际转回原安装。实验随后按用户授权采用备份／正常操作／退出后恢复，
本地原件恢复已由实验报告核验，云端状态未认证。因此本批只分析被冻结的实验文件，不重用“复制启动即隔离”的假设。

## 可消费的操作关系

| 入口和输入 | Steam 对象／条件 | 结果与返回边界 | 证据 |
| --- | --- | --- | --- |
| 标题进入存档选择，选择有记录的手动栏 | `TitleForm` 存档选择状态，`selectSaveType_ != 0` | 创建 `TYPE_SAVE_MENU=20`，传入栏位；继续选项回到标题处理，再调用 `ContinueGame` | 窗口02→03→04；静态 `TitleForm.Update` |
| 选择有记录的“自动”栏 | `selectSaveType_ == 0` | 直接调用 `ContinueGame`，没有手动栏的继续／新游戏／删除子菜单 | 窗口32→33；同一静态分支 |
| 存档条目为未使用标志 | 标题目录对应项等于 `-1` | 转 `NewGame`；不能把所有栏位都当继续入口 | 静态；本次未动态覆盖空栏 |
| 主菜单选“系统” | 菜单项6 → `TYPE_SYSTEM_MENU=10` | 推入系统菜单；菜单项编号与页面类型是不同字段 | 窗口23→24；静态 `SubForm.Update` |
| 系统选“保存” | 菜单项20 → `TYPE_SAVE=14` | 保存页内部状态0→1；下一次该页更新调用 `AppData.SaveGame()`；正常返回后设状态2 | 窗口35→36及05；静态保存状态机 |
| 保存完成页确认／对应软键 | 保存状态2 | 关闭保存页；此确认关闭提示，不是等到点击此时才执行保存 | 静态；异常保存分支与底层文件提交另见[存档合同](../rules/PERSISTENCE.md) |
| 普通设施详情第一页确认 | `TYPE_TENANT_INFO=74`，排除住宅／特定详情类型／种类2植物等独立分支 | 推入 `TYPE_TENANT_ITEMSELECT=75`，传设施实例和定义；第二详情页确认不走道具消费 | 窗口10→11；静态74分支 |
| 道具页上下选择 | 当前列表与 `select_`、`scrollValue_` | 上下循环选中，显示窗口按5行跟随；选中本身不消费 | 静态75分支；窗口只覆盖局部列表 |
| 道具页收到确认键脉冲 | `TYPE_TENANT_ITEMSELECT=75` | 调 `PresentItem`，更新列表位置，创建76并传设施／定义／道具，再增实例计数并检查强化奖励窗口；耗尽列表时关闭75，已排入76仍保留 | 静态；窗口库存2→1，保存审计已证道具减少 |
| 强化演出初始化与推进 | `TYPE_TENANT_ANIME1=76` | 首次`Init2`调用`SetTenantItemParam`应用定义共享改良；`frame_`达到49后推入77并关闭76；更新不再次扣库存或改良 | 静态初始化／原表／消费者；窗口15→16 |
| 强化结果确认 | `TYPE_TENANT_ANIME2=77` | 小于49时快进49；49–54等待；达到55才关闭，不再次改良或扣库存 | 静态原表／消费者；窗口16→17返回道具列表 |
| 道具列表返回，再退出设施详情 | 75→74→地图 | 实验已观察库存1、回详情、回地图；完整取消键映射仍未认证 | 窗口17→18→19 |

设施业务变化的本次观察为咖啡店价格490→510G、品质21不变、魅力25→27、坐垫库存2→1。
真实保存的共享改良和库存差异已由[只读审计](../work/window-save-analysis/ANALYSIS.md)交叉；
两个保存之间有自然经营，不能把全部差异归给这一操作。人物“好满足！”和设施“升级↑”不是已识别的序列化 `p` 冒泡。

## 游戏设置与标题平台按钮

2026-10-14按本页固定DLL／metadata身份重新核对调用链。**游戏内系统菜单tag21确实打开SubForm raw12**：Update在VA`103276CD`比较21，`10327858–10327874`创建对象并传类型12，公共`10328228–10328248`依次构造SubForm、Push、退休菜单。这不是只有`TYPE_CONFIG=12`声明而没有入口的旧页；四项非日文目录及返回SaveSystem的边界见[设置12](PAGES.md#设置12值所属与返回边界)。本次未实际打开或修改设置。

标题左上此前按外观统称“设置”的按钮应标为**Steam控制器绑定入口**。`TitleForm._draw`在`102108F7–1021095A`检查标题为顶层且state0，再以`(20,20,id=2)`调用`SteamInputManager.DrawButton`；helper只在`Platforms.x86=4`注册。delegate槽`110EE040`从metadata原字节解析为`<DrawButton>b__16_0`，`107B94BC–107B9565`只在CLICK5且`Config.USE_STEAM`、`initialized_`成立时，以当前controllerHandle调用`Steamworks.SteamInput.ShowBindingPanel`。这条链不创建raw12，不能用标题图标证明游戏画质／音量页已观察；是否成功显示Steam面板仍需对应运行环境，未动态验证。

相邻键盘按钮在`1021098D`以`(21,51,id=2)`调用`DrawKeyboardButton`，delegate槽`110EE06C`对应`<DrawKeyboardButton>b__18_0`；CLICK5在`107B9744–107B9796`拼接`https://kairosoft.net/steam/?_lh=`、当前语言码及`#controlmap`并调用`Canvas.OpenBrowser`，属于外部操作说明链接。两个helper另外含`Config.DEBUG`限定的TrophyManagementForm旁支，不能当作普通设置入口。本次只读解码，不点击平台面板、外链或调试分支；图标资源身份、OS投递和平台实际行为另验。

raw12同身份静态子批进一步核定以下生命周期；字段含义及四项数组沿[设置12](PAGES.md#设置12值所属与返回边界)，不由按钮外观补齐平台功能。

| 环节 | Steam机器码所证及维护边界 |
| --- | --- |
| Init | `10312755–10312758`将类型12送至`10314B3B–10314B77`，只把进入时世界P.int[12]复制到页字段`+B8`；通用`10312616`清主钟。没有建立可取消的完整设置副本。 |
| Draw | `1035321A–1035321D`分派raw12；`10355DCA–10355E37`先Jp、再En，分别调用`DrawConfig_jp`／`DrawConfig_en`。后者`1030601F–1030603E`实参窗宽210、高186，按四项CONFIG_EN目录绘制；本次不把语言helper名直接等同当前中文平台的实际语言配置。两helper末尾的键盘按钮仍走前述外部说明入口，不是第五项游戏设置。 |
| 高速准入 | 非日文`1030673E–10306778`在速度行、选项0、J.int[9]==0时跳过该项Touch6注册；JP对应`103078FF–1030793F`。文字仍绘制，灰色路径分别在`103065F8–1030661A`／`103077C3–103077E5`，普通行选择热区也仍保留。选项值经过REVERSE后才交setter；键盘禁改整速度行、绘制只禁高速选项、基类setter本身没有相同门控，是三个不同层次，不能相互替代。 |
| Update与返回 | 通用`10321B63–10321B84`推进主副钟，raw12在`1032692E–1032693D`对主钟再加1模20。软标签2在`10326D03`／`10327139`进入返回；非日文依次SaveSystem、ResetOffscreen、Pop（`10326D28/49/5B`），JP保存后到公共Pop。捕获异常的路径也可进入`10326E07–10326E0A`关闭，未回滚即时设置；不把该原行为当作维护事务保证。 |

`AppData.SaveSystem`在`102640DC–10264119`打开RecordStore0并写record0，取`sysSave_`（`+40`），没有顺带序列化世界P。本批独立只读计划消费下述目录、触摸注册与值映射；完整设置消费者仍需确定系统音量／通关资格的跨局权威字段、画质刷新与音频输出、返回写失败策略及快照归属。原字体、实际OS输入、平台初始值及中文语言配置未在本批动态认证；不扩展为平台Config／Option窗口已实现。

非日文独立纯计划位于[设置皮肤接口](../prototype/include/dungeon_village_prototype/steam_config_skin.hpp)及[实现](../prototype/src/steam_config_skin.cpp)。输入显式提供P12／P13的0..1与J3／J4的0..5实际值、当前行、J9资格、两音量行的左右CheckTouch结果、Graphics原点、当前语言文字和标题两次实测宽度；不借现Owner补设置字段。主钟没有本页绘制消费者，不作为纯计划输入。Steam `.cctor`在`10328E2A–10328E78`独立安装raw12软标签`[0,2]`。

| 原序与部分 | 非日文DrawConfig_en及组件实际消费 |
| --- | --- |
| 外框／内框 | `1030601F–1030603E`的DrawWindow参数为210×186、side4／top4／bottom39／bar80／BOX4。`MyFormBase.DrawWindow2`的`102000B4–10200379`将实际外框偏移−12，返回布局却为top43／left15／right225／bottom213；内框原请求为`(19,35,221,162)`，再由DrawBox加VIEW_Y/2。正文不能再加该偏移。 |
| 行名／行选择 | 四行左29、y为57／81／105／129；当前行名称底`(28,y−1,56,14)`为RGB159/189/255。先底色，再Touch6原基`(−120,y−6,179,24)`、值`0x20000|row`，再文字RGB30。SE名称独用40×11的TextLayout、anchor32，其余普通字体。 |
| 二选当前值 | 画质／速度显示索引均`(stored+1)%2`。`GetEvenlyDivisionArea RVA257710`将94..206以padding2分两区，整数中心121／178；当前值底54×14，与当前行选择独立。选项Touch6分别`(59,y−6,92,24)`／`(149,y−6,240,24)`，主值为类型`0x60000/0x70000`加实际存储值1／0，扩展值为当前行key。J9锁定只抑制高速选项0的触摸并染灰，仍允许显示它的当前值底。 |
| 音量数与左右 | 当前数字用普通字体long，右锚`(82,y)`；随后先左21、再右20。基矩形为`(59,y−6,55,24)`／`(186,y−6,240,24)`，主值为行key、扩展值分别`0x10010/0x10012`。`_addTouch`在登记前即时画IMAGE_DATA4/6锚`(99,y+6)`、5/7锚`(201,y+6)`，两态只来自实际CheckTouch，不拿页面main猜帧。 |
| 音量轨道／触摸 | `GameView._addTouch 1023E5BF–1023E723`先画RGB30的`(120,y+4,60,4)`再`(119,y+5,62,2)`，IMAGE_DATA11锚`(119+12×volume,y+6)`；12来自`(62−2)/(6−1)`先整除。Touch19基`(119,y−6,62,24)`真正登记为`(104+12×volume,y−6,32,24)`；Drag为`{119+originX,originY,62,0,6,0}`、Flag4，主值类型`0x80000/0x90000`、扩展值行key。源与实际热区、绝对Drag原点分别保留。 |
| 说明／平台请求 | 两行在`(34,178)`／`(34,196)`、anchor17；速度第二行独为RGB200/0/0、180×11的TextLayout、anchor32，不因已解锁隐藏。最后仅平台Steam且本页顶层请求`DrawKeyboardButton(21,51,id2)`；纯计划明确输出平台请求，不画假图标或触发浏览器。 |

控制组件引用GameView的`buttoneffect.png`（复用已出版`assets/steam-common`副本），IMAGE_DATA条目`4/5/6/7/11`分别是`{0,−11,−11,1,85,22,21}`、`{0,−11,−11,24,85,22,21}`、`{0,−14,−14,47,85,28,28}`、`{0,−14,−14,76,85,28,28}`、`{0,−2,−13,73,57,9,23}`，含资源索引、偏移与裁片；不是common图片编号。源裁片按实际图宽/128乘后加0.5截断，目的偏移与大小按0.75缩放，密度不再次放大目的矩形。拖动原消费者`OnTouchTrackBar RVA23D670`只在event9先应用主设置值，再由`Set RVA23DE60`回写扩展行选择，低16位有符号；这不是原事务。本批仅交付图元／注册描述，不新增Owner设置或文件事务。

`DrawWindow2 102000E4–102000FC`在顶层仍推进独立arwFrame；raw12只有BOX标志，所核局部不读取它绘箭头。维护纯查询不推进它，不能反称原Draw无副作用。设置主体及平台键盘的研究窗口见示例；原字体／平台语言、OS输入、真实设置保存仍分别验收，独立皮肤通过不等于完整设置消费者已完成。

### 平台键盘与控制器按钮的素材和局部绘制

图标来自平台`KairoLibrary.resource.kairolib.bytes`，不是游戏common包。原物理容器为`KairoGames_Data/il2cpp_data/Resources/KairoLibrary.dll-resources.dat`；`IApplication.Awake 1078293B–10782A0F`创建ResourceManager，以IApplication所属Assembly载入字面`kairolib`，给图片设置`SetExpiration(true,40)`，随后将该对象安装到`R.Library.Kairolib.resource`。实际赋值helper在`1077BEF0–1077BF43`，`IApplication.GetResource 10785DC0–10785E31`读取同一字段。资源命名、加载链和原归档目录三者独立交叉，不能只由PNG外观认定身份。

| 按钮 | 平台SEB／图片索引及原part | 局部目的矩形（普通态） |
| --- | --- | --- |
| 控制器绑定 | seb.inf[23] `button_gamepad.seb`，img.inf[27] `button_gamepad.png`；唯一part裁`(0,0,64,60)`、偏移`(−32,−30)` | `(x−16,y−15,32,30)` |
| 键盘说明 | seb.inf[25] `button_keyboard.seb`，img.inf[29] `button_keyboard.png`；唯一part裁`(0,0,64,42)`、偏移`(−32,−21)` | `(x−16,y−10.5,32,21)` |

这两组PNG／SEB已按原字节出版到[平台素材目录](../assets/steam-platform/)，各SEB均为一层、一帧、一part。`DrawKeyboardButton RVA7B6C50`在`107B6DEF–107B6E01`却取**gamepad的SEB23**第0帧第0层作为命中源，实际绘制才于`107B6EDF–107B6F14`取SEB25。二者触摸基框始终为`(x−16,y−15,32,30)`，四边Margin15；键盘`(21,51)`因此基框`(5,36,32,30)`，不能改用键盘21高的图像框。

两个helper都先`PushOrigin(0,0,false)`，结束恢复原点，不叠表单原点；检查的值分别为控制器`0x13465BA`、键盘`0x13489A9`，component由调用者传入（现标题／raw12均2）。普通态push单精度0.5矩阵；CheckTouch命中时，先push `BlendMode(1,255,255,255,128)`及单精度1.1矩阵，再push0.5。`Seb.Draw RVA7BF970`先以part偏移作局部Translate，`Graphics.BeginRenderMatrix RVA75D570`再把调用锚写入base矩阵；`Matrix.PostConcat RVA7AC110`独立核乘序。因此锚不缩放，高亮只将偏移／尺寸乘`1.1F*0.5F`；图像矩阵全部pop后才注册原基框，高亮不扩大命中。BlendMode1是具名颜色混合请求，不能未经后端核验简化成整张半透明。

资格分层保留：`DrawButton`只检查Platform.x86=4；`DrawKeyboardButton`还执行Xbox排除查询，本固定DLL的相关共享小方法返回false。helper均不自行判断顶页，标题顶页且state0、raw12顶页且APPLI_TYPE4是各调用者先做的检查。`USE_STEAM`和输入初始化只约束控制器CLICK后的平台动作，不约束图标绘制。控制器另外在`107B6BD8–107B6C2D`请求`Canvas.SetJoystickButtonDraw(button8, trunc((x−14)*scaleRatio/100), trunc((y−14)*scaleRatio/100), registerID24, scale100)`；键盘没有这项后继，尚未消费该glyph后端时须显式保留缺口。纯DTO应分别表达按钮kind、锚、component、平台资格、CheckTouch状态、原图／SEB身份、颜色混合、原点恢复及独立gamepad命中源；输入动作只产生具名平台意图，不由皮肤打开浏览器或绑定面板。以上为静态与素材认证，本批没有执行真实平台动作。

正式[平台按钮纯计划](../prototype/include/dungeon_village_prototype/steam_platform_button_skin.hpp)保留合法隐藏／非法输入的区别；控制器必须显式提供有限正值的GetScaleRatio(false)，键盘不读取它。坐标、component与比例上界是维护预算，不是原规则。研究设置窗口只消费无附加手柄glyph的键盘计划；返回的图元、命中和原点契约供产品接入，网页与绑定面板动作没有由研究窗口执行。

**高亮混色的生成源码与适配边界。** `Graphics.SetBlendMode`在`107738B5–10773922`按常量255归一RGBA；`ShaderBuilder.Init`的`1073DDA9–1073DDBC`只在非clipRegion且blendMode1时启用blendColor，`GetNameId`加0x20（有图像再加0x10），`GetCode`于`1073D002–1073D018`同时匹配这两位后取原shader槽`110FC810`。该字符串先`constant * texture alpha`，再`previous lerp(constant) texture`（constant alpha为_BlendAlpha），最后`previous * primary`。`MaterialManager`的`10730065–107300EF`上传_BlendAlpha及RGB、强制_BlendColor.alpha=1。设原纹理T、顶点色P、归一混色alpha=a，按固定函数组合推导：`RGB=(T.rgb*(1-a)+Blend.rgb*T.a*a)*P.rgb`，`A=T.a*P.a`；本按钮Blend.rgb为白、a=128/255。只有不透明纹理像素才等于通常的对白色插值，不能用alpha tint替代。

当前研究执行器用上述公式的独立GLSL适配，校验专用uniform存在，拒绝编译失败后的默认shader回落；只在首次高亮加载、窗口结束释放。原MaterialManager会优先查找现成shader，找不到才生成；本批没有认证运行时具体compiled shader、GPU色域、DPI或原窗口像素。四态参考中的键盘高亮因此仍是具名研究适配，不升级为Steam动态认证。

**控制器附加按键图标的登记与消费。** `Canvas.SetJoystickButtonDraw RVA751DC0`写入`joystickButtonDraw_`的七整数记录`{button,registerID,disp,x,y,life,scale}`，不是即时绘制或另一个Touch组件。以`(button,registerID)`查重；首次8/24登记为`{8,24,1,x,y,4,100}`，再次只更新disp和位置，保留life与scale（`10752049–10752105`）。存储不存在时直接返回；`InitJoystick 1074CCF3–1074CD5A`仅在主机或摇杆仿真资格下建表，否则置空。Steam无控制器时button8也直接拒登记；`HasController RVA7B7D10`检查64位controllerHandle非零，不把“图标可见”当成已有控制器。`steamOffsetEnable_`为真时另给坐标加`trunc((Screen尺寸−IApplication尺寸)/2)`，不能只用窗口外框替代这些输入。

`FormManagerBase.OnDraw`在BeginPaint成功后先于`107E7669`调用Reset，将旧记录disp全清0；页面绘制及触摸登记重新标活，屏幕指针之后才在`107E77AB`调用DrawJoystickButtons，随后画触摸效果。Reset不删记录，没再登记的旧项当轮隐藏；此固定8/24键反复出现复用同一记录，不逐帧新增对象。`RaiseJoystickButtonDraw`只把全部life重置4；所核输入回调`10754EBC–10755016`在对应按钮GetJoystickUp且disp非零时将life减1并夹0，普通重绘不减life。它是使用次数相关的显隐量，不是四帧倒计时；Canvas重新初始化可重建／清空登记表，不能把合法保留条目称作泄漏或迁进世界存档。

`DrawJoystickButtons RVA749220`只消费disp=1且life>0，先按顺序计算单精度`(GetScaleRatio(false)/100)*240/1080`；`screenCursorDrawFrame_>=168`再乘1.1，记录scale非100再叠其百分比矩阵。透明度沿`Graph.Easing(0,255,4,life,0)`交`SetRenderMode(1,alpha)`，用登记位置调用`Kairolib.DrawButtons(buttonFrame8,layer0,autoKeyReverse=true)`，之后恢复渲染模式及矩阵。registerID24只作登记键，不是SEB帧或绘制层；button8是原ZL／`Buttons.TriggerL`，不参与仅0/1的确认取消反转。SetAutoCancelJoystick虽被调用，所核`107E3B42–107E3B65`不对button8写0..3的自动取消槽。

`Kairolib._drawButtons RVA805D60`会再次拒绝Steam无控制器的button8。`USE_STEAM`有效时于`1080602A–1080604D`读取`Buttons.TriggerL.GetImage()`；有有效尺寸的动态图片以最大边60等比居中绘制，不同时叠底版。`Buttons..cctor 107A696F–107A69AA`独立核其对象为`DigialButton`、action名`TriggerL`；其`GetImage RVA7A6D70`依据当前action set／controller／digital action查询`GetDigitalActionOrigins`，取首个origin交`GetGlyphForActionOrigin_Legacy`路径；路径变化时退役旧Image，再读文件并加载新图，不能冻结为游戏内固定PNG。正常返回但没有有效动态图时走Kairolib原包后备SEB：InputType5／13取`buttons_ps4`（SEB19／图片22），1／2／3取`buttons_xbox`（SEB29／图片33），其余取`buttons_switch`（SEB13／图片15），均frame8／layer0；文件读取异常不能擅自改写成正常后备结果。后备身份与`IMAGE_COVERAGE.json`已有原包条目交叉；本批未取得实际Steam返回路径、设备按键映射或动态图文件，不将后备图冒充当前设备图。

可实施边界是独立的平台登记器与只读图标计划：显式输入表是否可用、控制器资格、8/24旧记录、运行时比例及Screen／Application尺寸差、光标显示计数、动态图有效尺寸或后备InputType；输出当轮可见图元和更新后的该条记录。每轮Reset→页面重新登记→统一绘制的次序、按钮释放降低life、初次和重复登记的差异须保留。动态Steam图标provider及跨设备缓存仍是平台资源职责；当前研究窗口只消费键盘按钮，控制器附加请求尚未伪造为已完成动态后端。

## 后续业务重载观察

用户交回[第二次窗口实验](../work/window-restore-observation/20261007-120056-business-reload-p/RESULT.md)，
已实际加载候选、查看咖啡店详情及坐垫列表、正常保存，未再次使用坐垫。
[S054／S055](../references/screenshots/2026-10-07-steam-business-reload/README.md)显示510G／21／27、等级1与库存1；
前后档独立核库存1与定义36共享J[20,0,2,0]保留。该批业务恢复已覆盖；当时非空p尚未覆盖，后续结果见下一节。
只替换游戏记录而保留当前系统目录，选择页日期／资金与读入世界不同，是已记录的目录／游戏档分离案例，
不据此判加载失败；世界运行后的画面和后档不能认证精确首帧或随机回放。

撤除画面[S057–S061](../references/screenshots/2026-10-07-steam-business-reload/README.md)补充设备目录、
“要撤除哪里”编辑提示、提交前后及返回地图。`removal-selection`图仍高亮道路，
`flower-target`不认证向日葵，`removal-confirmation`实际为撤除后编辑地图，未出现具名确认弹窗。
本次局部观察不外推所有撤除路径或把截图坐标当逻辑格；对象与引用差分见[分析](../work/business-reload-analysis/ANALYSIS.md)。

## 非空p中断恢复的后续观察

[本次实验](../work/window-restore-observation/20261007-131546-nonempty-p-reload/RESULT.md)实际从“自动”栏恢复非空p候选，
目录3年12月1周／1448G，最早取得世界已为下一周同现金，随后正常保存为3年12月3周／3285G。
首个自动后档与后续手动档是不同采样点，p空但同一目标邻接／共享业务字段保持，详情未取得。
[S062–S068](../references/screenshots/2026-10-07-steam-nonempty-p-reload/README.md)补目录、最早世界、保存、施工外观及自然授勋／中止参考。
旧“facility-detail”“restaurant-detail”时序标签实际为自然对话，不认证具名详情、UID或旧p图标。
字节端点与限制见[存取合同](../rules/PERSISTENCE.md#非空p中断前档实际重载已收口)；不为已完成链重复窗口操作。

## 输入：选中、标记与确认分开

`surface.GameView.OnTouchEvent`先允许顶层表单处理事件，再按组件ID分派。
`ID_DLGSEL=3`、`ID_LIST=10`、`ID_SLIST=11`共享 `OnTouchDlgSel` 的实现。
该方法对 `TYPE_ENTER=2` 与 `TYPE_UP=1`写入选中值；只有 UP 继续处理确认。
若组件带 `FLAG_MARKER=16`，第一次 UP 命中尚未标记的组件只记录组件key；
再次 UP 命中同一标记key才发送确认，未带标记的组件可直接发送确认。
默认发送 Canvas 的逻辑确认位 `0x100000`，组件的自定义键数组非空时改用首项。
`GameView.OnKeyDown`调用`ClearMarker`，说明键盘输入还能改变鼠标标记上下文；
实验在单击和最终双击之间插入Return，因此尤其不能把这几次输入当成相同前提下的单击／双击对照。

这段输入处理没有检查两次点击间隔，因而是**条件性的标记后确认**，不能写成 Windows 的限时“双击协议”。
后续静态核对已定位75的实际行注册：`_draw2_3` 的75分支对最多5个可见行，使用
`TouchOption.Create(16).Margin(29,0,0,0)`，注册 `ID_SLIST=11`，选择值为 `0x20000 | 绝对行index`。
因此设施道具列表确实参加上述标记机制；仍不能把它断言为该实验首次单击失败的唯一原因。
实验中一次点击、一次 Return 没取得执行结果，最后双击成功，只能记录这些动作及其可见结果；
焦点、命中位置、标记状态和事件投递没有被受控隔离，不能得出“Return无效”或“必须双击”。

设施75／77消费的是逻辑 `CheckKeyPulse(0x100000)`。已核 Steam `Canvas.GetKeyID` 和 `OnKeyDown`：
Unity KeyCode 经允许的焦点Keypad写入 `1 << keyID`，再由 `DecideKeyState` 将按下边沿转换为脉冲，
`CheckKeyPulse` 查询后消费该位。原始keyCode事件可被表面处理器消费，禁用位、输入guard和焦点也能阻止后续执行。

| Steam输入 | 静态映射／分支 | 消费边界 |
| --- | --- | --- |
| Return、Space、KeypadEnter | ID20 → `0x100000`确认 | 不是“Return无效”；需实际事件送达且当前表单有可消费的确认分支 |
| Left／Up／Right／Down箭头 | ID16／17／18／19 → `0x10000/0x20000/0x40000/0x80000` | 基础键位已核；控制器／滚动模式有额外分流，不能宣称所有模式都移动同一列表 |
| Escape、Backspace | 查当前四项显示软标签与 `backSoftLabels_` 的匹配，产生对应soft1／2／3／4位 | 返回软键随当前标签和运行时配置变化，不能全局固定为一个逻辑位 |
| 无标签匹配的Escape／Backspace | 满足应用返回资格和输入guard等条件后调用 `ProcessBackKey` | Escape传 `terminate=true`，Backspace传false；所有具体表单最终处理未认证 |
| 鼠标右键 | 先检查右键组件；其他路径可调用菜单软键，或合成Backspace8的down／up | 有组件、菜单和配置分支，不能写为全局Escape别名 |

75行的基注册矩形为 `x=23、y=95+19×(index-scroll)、w=191、h=16`；左margin为29。
注册过程还处理绘制／表面缩放、裁剪和偏移。矩形命中将输入换回表面局部坐标并整数截断，
左／上含边、右／下不含边；这些数值不是桌面或截图像素。标记比较使用组件id与value组成的key，
不要求复用同一个组件对象。完整优先级、拖拽取消和窗口焦点链仍待核。
精确VA／RVA、机器码哈希、预算及核对锚点见[输入链证据](../work/steam-input-closure/README.md)。
物理键到上述基础逻辑位已有静态证据；实际窗口投递、全部手柄别名和每个页面的返回软件键仍分别待验证。
产品消费时应保留“选择／确认／返回”语义，并按目标平台适配输入；Steam键位不直接赋给固定APK。

## 设施初始化与动画原表

Steam `SubForm.Init2`的76分支在VA `0x1030F7BB`调用`TenantData.SetTenantItemParam`（RVA `0x222E90`）。
三项增量写入定义共享`tenantParamTUseUp_`后重算，实例参与前后属性计算；显示差值全零仍继续写共享改良。
该消费函数不减库存，库存已由75消费。正常75→76→77流程的更新不重复消费，
但直接重复76初始化仍会再次改良：函数本身非幂等，未认证所有异常重入。
框架在mode0请求初始化、成功且仍为0后置1；通用派发帮助器未完整解析，不能据此承诺任意重试安全。

本版metadata独立取出的TIME1为`[0,0,15,29,39,39,49]`，TIME2为`[2,32,34,49,55]`。
分别对应FieldRef119／4、绝对字段25257／25142及原数据偏移`0x2FAACE`／`0x2FA036`，
原字节哈希、cctor初始化和49／55消费者均见[设施证据](../work/facility-animation-closure/README.md)。
这些阈值是内部页面更新计数；完整演出绘制、音效及真实输入投递仍未认证。

## APK交叉与剩余边界

| 合同 | 本次对照 | 可下结论的范围 |
| --- | --- | --- |
| 74→75→76→77 | Steam metadata 分别为 `TYPE_TENANT_INFO / ITEMSELECT / ANIME1 / ANIME2`，更新分支已核 | 页面角色和本表所列转移相容；不是全页视觉或所有条件已等价 |
| 75先消费、排入76、实例计数／强化奖励检查 | 两版局部更新次序相容 | 原程序不是一个原子提交函数；维护Owner的全失败回滚仍是独立安全策略 |
| 76首次初始化应用共享改良 | 两版独立核到初始化消费；Steam已核`Init2`该支及完整效果函数 | 局部次序相容；显示无差值仍写改良，函数非幂等，不外推异常重入 |
| 76自动切77、77确认快进／等待／关闭 | 两版原表与消费者分别核出49／55 | 局部阈值相同；不等于绘制／音效等价，不换算墙钟秒 |
| 每次页面更新计数 | Steam `SubForm.Update`在分派前递增 `frame_`和`frame2_` | 是该方法调用计数；不换算为墙钟秒数或Unity绘制帧 |
| 手动栏与自动栏入口 | Steam静态与本次窗口相容 | 不等于已认证原APK动态入口、首次更新边界或所有存储错误路径 |

下一次补证优先使用短输入实验：同一列表先悬停、单击、移开再点、间隔较长再次点击、键盘确认，记录选中和执行分别发生在哪一步。
需要已有明确输入及备份／隔离方案；本批不启动该实验。基础物理键映射和75行标记注册已静态闭合，
完整模式／焦点分支与实际窗口投递保持待验，无需重跑自然经营长链。
后续非空p输入恢复端点已收口，见[存取合同](../rules/PERSISTENCE.md#非空p中断前档实际重载已收口)。
它与道具业务恢复分别认证，不要求重复同一窗口链。

## 本批验证

19方法／89,840字节保持24方法／96KiB预算内；输入DLL身份校验通过，生成输出有SHA-256清单，
已核对上述分支地址及元数据字段。仅增加研究文档和忽略目录证据，无游戏代码变更，不运行无关CTest。
本批无后台任务、游戏进程或监测进程；不操作用户当前游戏。完整原程序键鼠等价和窗口回放仍未验收。

后续输入批仅静态文档与专属work证据：24方法／93,104字节，单方法最大30,528字节，满足
24方法／96KiB总量／32KiB单方法预算；48项身份、方法哈希、跳表、指令及输出消费核对通过。
没有C++变更或新原窗口实验，不追加无关CTest；本批没有后台或游戏进程。

设施续批5个完整方法／45,536字节，最大25,168字节；两个动画字段原字节与声明长度、cctor创建长度及消费者交叉。
证据哈希与本轮统一检查见[验证入口](../VERIFICATION.md)；静态初始化和工具自比均不能代替设施候选实际重载。
