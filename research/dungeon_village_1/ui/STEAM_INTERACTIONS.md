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

### 设置12的四行键盘更新候选边界

2026-10-16重新核同一DLL的Update局部`1032692E–10326D73`及既有基类setter；本段只覆盖现皮肤对应的`Jp()==false && En()==true`四行，不拿它代替JP目录或实际平台语言资格。raw12先对已由通用Update推进的主钟再作`(+1)%20`，Jp／En均不命中时仍完成这次加钟，但不查询下表输入；独立纯候选若仅消费已准入四行body，不得再模拟上游wait或通用共享SEB。

| 原序 | 可直接表达的纯逻辑及原锚 |
| --- | --- |
| 上→下 | `103269A2/103269FE`均为Repeat，分别选择`(row+3)%4`／`(row+1)%4`，独立执行；先定最终行，再读该行当前实际值，不能先改旧行或把上／下改成互斥。 |
| 左→右 | 先把画质／速度实际值作`(value+1)%2`得到显示索引，音量原值不反转；`10326B50/10326BD4`均为Repeat，按本行MAX分别减1／加1绕环，最后才反转回存储值。音量0向左为5、5向右为0，不是夹限。速度行且J9为0在`10326B08–10326B37`直接越过两个方向查询；仅行选择变化、无方向或左右同轮净值不变都不调用setter。 |
| 改值→返回 | 净值改变才在`10326CEB`请求当前行setter；之后一律在`10326D03`查询soft2，没有确认键提交分支。soft2命中时请求顺序是SaveSystem→ResetOffscreen→Pop，并把实际Pop bool交还调用者；同轮改值在保存前已生效，不是返回撤销。 |

声音只随有效setter发生：BGM在`10201480/102014C3/102014CB`依次SetVolume(type0,value×51)→SetMute(type0,value==0)→Reflect；SE在`102011DD/10201220/10201228`用type1执行同序，再于`10201261`Play(resSound_[7])。归零仍保留试听请求，实际静音由播放器决定，不能因听不到便删请求；页内没有新增“方向声／确认声”，通用待发jingle仍归上游。速度setter`1020139C–102013DA`只写P13，没有额外J9资格或重绘；画质setter`10201509–1020156D`写P12后还调用Canvas后继与DrawAllForms，不用纯值变化冒称平台画质已经刷新。

现[steam_config_english_update](../prototype/include/dungeon_village_prototype/steam_config_skin.hpp)复用既有设置皮肤输入的四个实际值、当前行、speed_unlocked，另接四个具名Repeat与soft2查询结果，返回下一值／行、实际检查轨迹、单次setter及返回动作的有序请求；它不新增Owner／持久布局。setter以row、before、after的实际存储值表达，真实执行者独占后继声音／画质操作，不能同时执行setter与另一路拆分输出而重复试听。音量／画质后继、跨局权威J字段、系统文件写入失败与Pop结果均留给真正执行者，不能仅安装候选便宣称完整设置完成。原SaveSystem异常可能捕获后直接Pop（`10326DA2–10326E22`），不回滚已经改值；维护的原子提交与失败保留仍须独立契约，不照搬该原异常行为。Touch6／19即时setter及拖动行回写是既有另一条输入路径，不把键盘J9门控外推给它或重复消费一次。

候选值域沿现皮肤：P12／P13为0..1，J3／J4为0..5。按现明确提供的中文选项顺序，画质stored0／1分别显示“速度优先”／“画质优先”，速度stored0／1分别显示“普通”／“高速”；中文名称是当前适配文案，反向索引来自原数组。J9的原字段名为`SYSSAVEI_OPEN_HIGHSPEED`，资格是`!=0`而非恰等于1；最终行被锁时检查轨迹应为上、下、soft2，不虚报已经查询左右。`MyFormBase.Jp RVA200A20`和`SubForm.En RVA30E210`分别委托Language.Japanese／English，调用者须明确提供本分支资格，不能把“研究窗口显示中文”直接当作En真。

### 平台按钮事件与显式动作

`<DrawButton>b__16_0` RVA7B9320与`<DrawKeyboardButton>b__18_0` RVA7B9580都是`void(TouchEvent)`，不返回是否传播；`SurfaceBase.DoEnd`在1088D88B–1088D914发现opt.onTouchEvent后调用该delegate并绕过普通hook／OnTouchEvent支。传给DrawButton的外层AddTouchDelegate是绘制时注册器：设好margin及事件delegate后恰调用一次（107B6BAE／107B70FB），事件回调不再调用它。不能套语言页bool handler或重复注册。

| 已分发事件 | 正常路径及实参 |
| --- | --- |
| 控制器CLICK5 | 107B94BC–107B9565按顺序检查USE_STEAM、initialized，再把共同64位controllerHandle原样交ShowBindingPanel；没有非零／HasController检查，API返回bool未消费 |
| 键盘CLICK5 | 107B9744–107B9796取GetLanguageCode，三串Concat为`https://kairosoft.net/steam/?_lh=`＋原码＋`#controlmap`，再OpenBrowser；不读USE_STEAM／initialized／handle，返回bool未消费 |
| 其它正常事件 | 不产生上述动作；回调不负责component／value再次匹配或传播判定 |

两callback的DEBUG旁支先于CLICK分支：ENTER2读取CurrentTimeMillis写**共同**downButtonTime；ON13重新取时，与旧值按原双字减法及分支比较；正常未溢出的时间差严格大于3000毫秒才new TrophyManagementForm→FormManager.Push→再读一次时间写共同字段（107B9394–107B94BA／107B961C–107B9742）。Push正常返回false仍到读钟；异常不据此假称完整提交。同一事件类型不可能同时为ON与CLICK。两按钮不能各存一个长按钟，3000不是逻辑tick或绘制帧数。

极端long边界保留固定x86的实际flags：controller在107B9400低位sub、9403高位sbb、9409 js拒绝、940F jg接受，否则9411低位cmp3000／9417 jbe拒绝；keyboard对应107B9688–107B969F。jg仍读sbb的OF／ZF，不是重新test高字。例如now位型8000000200000000、down7FFFFFFF00000000，相减高3／低0但OF1，源转低位比较而不触发；now0000000300000000／down0虽同样高3低0，OF0则触发。维护按无符号双字算术显式保留这些分支，不将C++有符号溢出或简化64位差当作来源。

URL两个literal分别由metadata槽110F417C／110FEACC核得，原调用没有编码、trim或大小写归一。String.Concat三串重载RVA8E5DF0在108E5E0E–108E5EE1把中段null／空串转为prefix＋suffix；实际返回null合法，尚未读取语言码却不是null证据。

[平台模块](../prototype/include/dungeon_village_prototype/steam_platform_button_skin.hpp)的`steam_platform_button_event`接已分发事件，返回最多两个按序的具名动作，不执行SDK／浏览器／时钟或修改Owner。已消费handle缺值拒绝但0合法；`language_code_resolved`区别未取值与源null，语言串4096字节为维护预算。DEBUG重记钟输出“执行到此再取钟”请求，不能用判断ON时的旧now替代。调用宿主须沿单一平台时钟状态按序消费，并另行处理外部调用失败；原bool未消费不等于平台调用成功。此处不新增世界持久字段、不认证OS投递、实际控制器或外链已打开。

### 平台键盘与控制器按钮的素材和局部绘制

图标来自平台`KairoLibrary.resource.kairolib.bytes`，不是游戏common包。原物理容器为`KairoGames_Data/il2cpp_data/Resources/KairoLibrary.dll-resources.dat`；`IApplication.Awake 1078293B–10782A0F`创建ResourceManager，以IApplication所属Assembly载入字面`kairolib`，给图片设置`SetExpiration(true,40)`，随后将该对象安装到`R.Library.Kairolib.resource`。实际赋值helper在`1077BEF0–1077BF43`，`IApplication.GetResource 10785DC0–10785E31`读取同一字段。资源命名、加载链和原归档目录三者独立交叉，不能只由PNG外观认定身份。

| 按钮 | 平台SEB／图片索引及原part | 局部目的矩形（普通态） |
| --- | --- | --- |
| 控制器绑定 | seb.inf[23] `button_gamepad.seb`，img.inf[27] `button_gamepad.png`；唯一part裁`(0,0,64,60)`、偏移`(−32,−30)` | `(x−16,y−15,32,30)` |
| 键盘说明 | seb.inf[25] `button_keyboard.seb`，img.inf[29] `button_keyboard.png`；唯一part裁`(0,0,64,42)`、偏移`(−32,−21)` | `(x−16,y−10.5,32,21)` |

这两组PNG／SEB已按原字节出版到[平台素材目录](../assets/steam-platform/)，各SEB均为一层、一帧、一part。`DrawKeyboardButton RVA7B6C50`在`107B6DEF–107B6E01`却取**gamepad的SEB23**第0帧第0层作为命中源，实际绘制才于`107B6EDF–107B6F14`取SEB25。二者触摸基框始终为`(x−16,y−15,32,30)`，四边Margin15；键盘`(21,51)`因此基框`(5,36,32,30)`，不能改用键盘21高的图像框。

两个helper都先`PushOrigin(0,0,false)`，结束恢复原点，不叠表单原点；检查的值分别为控制器`0x13465BA`、键盘`0x13489A9`，component由调用者传入（现标题／raw12均2）。普通态push单精度0.5矩阵；CheckTouch命中时，先push `BlendMode(1,255,255,255,128)`及单精度1.1矩阵，再push0.5。`Seb.Draw RVA7BF970`先以part偏移作局部Translate，`Graphics.BeginRenderMatrix RVA75D570`再把调用锚写入base矩阵；`Matrix.PostConcat RVA7AC110`独立核乘序。因此锚不缩放，高亮只将偏移／尺寸乘`1.1F*0.5F`；图像矩阵全部pop后才注册原基框，高亮不扩大命中。BlendMode1是具名颜色混合请求，不能未经后端核验简化成整张半透明。

资格分层保留：`DrawButton`只检查Platform.x86=4；`DrawKeyboardButton`还执行Xbox排除查询，本固定DLL的相关共享小方法返回false。helper均不自行判断顶页，标题顶页且state0、raw12顶页且APPLI_TYPE4是各调用者先做的检查。`USE_STEAM`和输入初始化只约束控制器CLICK后的平台动作，不约束图标绘制。控制器另外在`107B6BD8–107B6C2D`请求`Canvas.SetJoystickButtonDraw(button8, trunc((x−14)*scaleRatio/100), trunc((y−14)*scaleRatio/100), registerID24, scale100)`；键盘没有这项后继，尚未消费该glyph后端时须显式保留缺口。纯DTO应分别表达按钮kind、锚、component、平台资格、CheckTouch状态、原图／SEB身份、颜色混合、原点恢复及独立gamepad命中源；输入动作只产生具名平台意图，不由皮肤打开浏览器或绑定面板。以上为静态与素材认证，本批没有执行真实平台动作。

正式[平台按钮纯计划](../prototype/include/dungeon_village_prototype/steam_platform_button_skin.hpp)保留合法隐藏／非法输入的区别；控制器必须显式提供有限正值的GetScaleRatio(false)，键盘不读取它。坐标、component与比例上界是维护预算，不是原规则。研究设置窗口消费无附加手柄glyph的键盘计划；标题另有语言／平台组合接口和[条件参考](examples/README.md)。入口按钮图像与独立登记请求分开消费，标题示例显式采用登记表不存在的条件，不伪造当前控制器glyph。网页与绑定面板动作没有由研究窗口执行。

**高亮混色的生成源码与适配边界。** `Graphics.SetBlendMode`在`107738B5–10773922`按常量255归一RGBA；`ShaderBuilder.Init`的`1073DDA9–1073DDBC`只在非clipRegion且blendMode1时启用blendColor，`GetNameId`加0x20（有图像再加0x10），`GetCode`于`1073D002–1073D018`同时匹配这两位后取原shader槽`110FC810`。该字符串先`constant * texture alpha`，再`previous lerp(constant) texture`（constant alpha为_BlendAlpha），最后`previous * primary`。`MaterialManager`的`10730065–107300EF`上传_BlendAlpha及RGB、强制_BlendColor.alpha=1。设原纹理T、顶点色P、归一混色alpha=a，按固定函数组合推导：`RGB=(T.rgb*(1-a)+Blend.rgb*T.a*a)*P.rgb`，`A=T.a*P.a`；本按钮Blend.rgb为白、a=128/255。只有不透明纹理像素才等于通常的对白色插值，不能用alpha tint替代。

当前研究执行器用上述公式的独立GLSL适配，校验专用uniform存在，拒绝编译失败后的默认shader回落；只在首次高亮加载、窗口结束释放。原MaterialManager会优先查找现成shader，找不到才生成；本批没有认证运行时具体compiled shader、GPU色域、DPI或原窗口像素。四态参考中的键盘高亮因此仍是具名研究适配，不升级为Steam动态认证。

**普通mode1透明合成与srcRatio。** 2026-10-14窄核标题空中断行黑罩／人物边缘所需的普通链，不运行原GPU。`SetRenderMode` RVA7749D0原样保存op／src／dst，只有op1且和为256才将dst减1；两参数774A80提供dst=255−src。`FillRect` RVA76A990在1076AAC1–AAD0将当前颜色交GetRenderPass，随后BeginShape的输出实际交GL.Color或MeshBuilder.AddVertex。GetRenderPass RVA76E2D0在无自定义Material、普通非clip资格且op1／src+dst=255时，1076E743–E7CA建立单pass：colorRatio255、alphaRatio=src；`RenderPass.BeginShape` RVA791300的10791505–152C按float32使顶点alpha乘src／255，RGB不因该比例变暗。纹理普通Begin路径同样消费pass的alphaRatio，不能把dst当作颜色alpha。

ShaderBuilder.Init／GetNameId（73DD30／73D730）对该模式选择位0x2040；GetCode RVA73CCC0在1073D06A–D092取实际字符串槽110F3E14，独立metadata解码为`Blend SrcAlpha OneMinusSrcAlpha, One OneMinusSrcAlpha`。普通有图、无附加BlendMode且非全不透明优化时，1073D01A–D029取110F603C的`SetTexture [_MainTex] { combine texture * primary }`；原shader模板还以BindChannels绑定vertex／texcoord／color。因此设T为纹理RGBA、P为原颜色／顶点RGBA、s=src／255、D为目的像素，普通源alpha为`A=T.a×P.a×s`，源RGB为`C=T.rgb×P.rgb`；生成shader式推导`out.rgb=C×A+D.rgb×(1−A)`、`out.a=A+D.a×(1−A)`。纯Fill去掉T，采用其原颜色；不透明黑色src128则`A=128／255`，目的RGB保留127／255，**不是alpha=255−src，也不是精确0.5**。

研究raylib普通适配可把不透明颜色的8位alpha设为src，对纹理保持原纹理alpha再乘该tint；需要保留透明RenderTexture输出时还应维持上述独立RGB／alpha混合因子，而不凭默认名称假定两者相同。原src0／64／128／192分别为0／64／128／192除255的源权重。已核生成链支持此局部适配，不认证当前预编译Material、GPU色域或像素一致性；特殊Image.alphaType3的分pass、自定义Material、clip／offscreen和额外BlendMode不由此方程通用覆盖，人物深层效果也可能另改模式。保持具名原请求与研究适配分层，不用这次结论宣称整套标题人物GPU已验。

**控制器附加按键图标的登记与消费。** `Canvas.SetJoystickButtonDraw RVA751DC0`全960字节已核，写入`joystickButtonDraw_`的七整数记录`{button,registerID,disp,x,y,life,scale}`，不是即时绘制或另一个Touch组件。以`(button,registerID)`找首个匹配；复用仅令disp=1并更新位置，保留life及旧scale，不执行新记录的后继调用（`10752049–10752077`）。新记录追加`{button,registerID,1,x,y,4,scale}`后，若存在栈顶表单，调用`SetAutoCancelJoystick(button)`（`10752147–1075215B`）。后者RVA7E3AC0仅button0..3可能写自动取消槽，仍受自己的全局开关／槽资格限制；8和9不写该槽。不能把8样例的无副作用推给0..3。

登记资格在查找旧记录之前判定：表不存在直接返回；`InitJoystick 1074CCF3–1074CD5A`仅在主机或摇杆仿真资格下建表，否则置空。非`APPLI_TYPE4`不查控制器；type4调用`HasController RVA7B7D10`，它检查64位controllerHandle是否非零。有控制器允许继续；没有时仅button2／3直接允许，button7须当前`GetSoftLabelR()`精确等于`戻る`、`メニュー`或`ﾒﾆｭｰ`之一，其它button均拒绝（`10751EB5–10751F18`）。因此绑定8／24与语言9／2的**登记资格相同**，不能因为语言按钮可见就认定其glyph已登记。原方法没有button、registerID或scale范围钳位；维护拒绝范围是独立预算。

type4通过上述准入后，源先读取Screen宽高及`IApplication.GetWidth/GetHeight`，再检查`steamOffsetEnable_`；为真才各加`trunc((Screen尺寸−IApplication尺寸)/2)`（`10751F1E–10751FB2`）。负奇数差也向零，例如−201得到−100，不是floor；不取GetGameWidth，不替换成OS外框尺寸。非type4不走该偏移，禁止将ratio换算重复加到已登记锚。

`FormManagerBase.OnDraw`在BeginPaint成功后先于`107E7669`调用Reset；页面绘制及触摸登记重新标活，屏幕指针之后才在`107E77AB`调用DrawJoystickButtons，随后画触摸效果。Reset（RVA751B70）全192字节仅将全部disp清0；Raise（RVA751960）全192字节仅将全部life置4，包括隐藏记录。二者不删除记录，不改其它字段；同键反复登记不逐帧新增对象，life0也不会因重新登记而自动回4。

释放路径位于`DecideKeyState`闭包RVA752BB0的`10754EA6–1075501D`：按index0..23原序查询`GetJoystickUp(index)`，为真时遍历整表，对button匹配且disp非零的**全部**记录将life减1并夹0，不按registerID只减一项。button8／9还可先向各自映射键发送OnKeyUp；该物理输入桥不属于登记表纯消费者。更早的button11释放／DEBUG分支可先Raise，再进入这段循环，不能把Release与Raise合成同一动作。Reset、登记和普通绘制都不减life；它是使用次数相关的显隐量，不是四帧倒计时。Canvas初始化可以重建表，合法保留条目不等于泄漏；注册不同键可使表增长，不能宣称全局永久有界或迁入世界存档。

`DrawJoystickButtons RVA749220`全1424字节已核。表不存在立即返回；表存在时基础比例初为1，仅`APPLI_TYPE4`改为逐步float32的`((GetScaleRatio(false)/100)*240)/1080`，然后所有平台在`screenCursorDrawFrame_>=168`时再乘float32的1.1。基础比例不等于1先全表外PushScale，即使表空或本轮没有可见记录仍有这对正常Push／Pop。以下按登记原序逐条执行，不把scale预乘成整数或缩放锚点：

1. 仅disp恰1继续。effective life先取记录life；若`USE_STEAM`或Xbox查询为真、无控制器且button7，当前右软标签为`戻る`时改用`rightClickButtonCount_[0]`，两种菜单串改用`[1]`（`107494BF–107495B9`），其它情形不替换。此为局部有效值，不回写记录life；本固定DLL的Xbox共享小方法返回false。
2. effective life≤0跳过；否则记录scale不等于100时再内PushScale(float32(scale)/100)，然后以`Graph.Easing(0,255,4,effective,0)`设置`SetRenderMode(1,alpha)`。life1／2／3／4的alpha独立值为63／127／191／255，不作四舍五入；Easing入口RVA8A4A40在frame≥4时直接返回255。
3. 调用`Kairolib.DrawButtons(g,record.x,record.y,record.button,layer0,autoKeyReverse=true)`，再无参SetRenderMode清默认`(0,255,0)`，再按实际内Push弹矩阵；全部记录结束才弹外矩阵。原catch不构成完整图形栈异常回滚证明。

registerID只作登记键，不是SEB帧或绘制层；8和9在这层没有独立过滤，深层设备图像资格继续由`_drawButtons`处理。button8为原ZL／`Buttons.TriggerL`，不能因此把所有记录的frame固定成8，或把button7有效life的特例忽略掉。

**局部缩放不缩放登记锚。** Steam `Seb.Draw`（RVA7BF970）在`107BFA4D`检查局部矩阵；存在时于`107BFAA7–107BFAB0`将SEB偏移作为TranslateTemporary压入局部栈，DrawImage仍传原x／y。`_drawBitmap`在`1077887F`调用`Graphics.BeginRenderMatrix`（75D570）；后者于`1075D772–1075D7AB`将原x／y PreTranslate到worldMatrix，并把本次绘制坐标清0。`UpdateScreenMatrix`（7757F0）先组合局部栈，再PostConcat(world)；PreTranslate（107AD075–107AD0CC）和PostConcat的实际数组运算按行向量次序，结果是**登记锚＋局部缩放×SEB偏移／顶点**。因此平台消费者不能把提示表的外层PushScale直接当作围绕屏幕原点的全局缩放，否则会把glyph移离按钮。维护计划的pivot0是局部图元原点；raylib适配应在每个DrawButtons处先定位登记锚，再作用局部缩放，不能为修正位置反改ratio或登记坐标。这一局部矩阵交叉不认证所有Graphics屏幕变换、DPI或SDK动态图后端。

`Kairolib._drawButtons RVA805D60`会再次拒绝Steam无控制器的button8。`USE_STEAM`有效时于`1080602A–1080604D`读取`Buttons.TriggerL.GetImage()`；有有效尺寸的动态图片以最大边60等比居中绘制，不同时叠底版。`Buttons..cctor 107A696F–107A69AA`独立核其对象为`DigialButton`、action名`TriggerL`；其`GetImage RVA7A6D70`依据当前action set／controller／digital action查询`GetDigitalActionOrigins`，取首个origin交`GetGlyphForActionOrigin_Legacy`路径；路径变化时退役旧Image，再读文件并加载新图，不能冻结为游戏内固定PNG。正常返回但没有有效动态图时走Kairolib原包后备SEB：InputType5／13取`buttons_ps4`（SEB19／图片22），1／2／3取`buttons_xbox`（SEB29／图片33），其余取`buttons_switch`（SEB13／图片15），均frame8／layer0；文件读取异常不能擅自改写成正常后备结果。后备身份与`IMAGE_COVERAGE.json`已有原包条目交叉；本批未取得实际Steam返回路径、设备按键映射或动态图文件，不将后备图冒充当前设备图。

维护[提示表纯接口](../prototype/include/dungeon_village_prototype/steam_joystick_prompts.hpp)显式输入表是否可用、APPLI_TYPE／控制器、原记录表、比例／尺寸差及光标显示计数，输出私有候选表及有序DrawButtons请求；不新建世界Owner或快照字段。4096条、button0..43及scale0..10000是维护预算，坏旧表／重复键／实际消费字段缺失／坐标偏移溢出明确拒绝，不留下部分修改。0..3新登记返回自动取消后继请求；button7读取右软标签／有效计数；释放遍历只处理0..23，不能保证更大button自行消退。表项按值持有、不保存页面／设备引用，Reset隐藏并不退休条目；宿主结束平台会话时释放整表。Draw最多5×4096＋2项，按原序交平台消费者，SDK动态图和跨设备缓存仍是下层职责。标题组合窗口此前在显式无登记表条件下只消费入口图像和登记意图，不等于已接完整动态后端。

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

### 虚拟屏幕光标：模式、位置与合成Touch

2026-10-14补固定Steam2.56具名静态分支，不操作窗口。这里的`screenCursor_`是Canvas内部虚拟光标，区别于物理鼠标／Unity Touch。真正生产者为`DecideKeyState` RVA`748780`经`RunOnUiThread(wait=true)`提交的`<DecideKeyState>b__325_0` RVA`752BB0`；实际委托槽`110FB634`独立解码为metadata method7605，与具名入口交叉。`OnInputUpdate`的物理Touch循环不能代替这条生产链。

Canvas.cctor在`10757754–10757796`建立真实`CONTROLLER_TABLE=[0,2]`，含Joystick与ScreenCursor，没有表内Remocon项。新Canvas的controllerIndex采用零初值，因此初始索引0不是虚拟光标；`IsScreenCursor` RVA`74E6D0`实际检查表值等于2。输入lambda在`10754BEE–10754CF5`遇button10释放，或无平台GUI对话（`guiMode_=0`）且键盘P按下时，令索引加1模表长、清三项摇杆指针，再检查是否进入光标模式。这是普通Steam样本里的可达切换分支，不要求Editor；未读取当前窗口索引或实际输入，不能声称用户会话已经启用。实际设备button10映射及guard／焦点仍是平台输入前提。

| 局部消费者 | 已证效果与边界 |
| --- | --- |
| InitScreenCursor `74CD80`及Init内联 `1074DA96–1074DBBA` | 独立建立screenPointer／screenCursor／previous三个两元素float数组，click=−1、移动／停顿／恢复／绘制计数清0。Init把Screen整数中心先装入pointer，再交SetScreenCursor；不是以物理鼠标当前位置初始化 |
| SetScreenCursor `752200`／JumpScreenCursor `74EA30` | 保存float位置，限制到横向`[8,KairoPlugin.GetWidth()−8]`、纵向`[8,Canvas.GetHeight()−8]`；原比较／赋值没有按格吸附。Jump随后将恢复计数置12，不自行发送点击。宽高不足、非有限值不由此认证为安全输入 |
| lambda移动 `10754497–10754866` | 仅ScreenCursor模式移动；无平台GUI对话且某轴为0时才可用左右/A/D或上下/W/S以−0.5／+0.5补该轴。读取Steam StickL分量并除220，按模长与joystickPointerAsobi比较；低于阈值时用方向按钮，连续计数≤2／>2分别选构造中的速度4／8，乘Screen.height/240后截断；模拟轴路径以`trunc(16×Screen.height/240)`乘原轴分量。两路都更新已有位置并交SetScreenCursor，不以秒数计算距离 |
| lambda合成事件 `10754869–10754BEB` | 非Joystick模式且guiMode=0才进入；当前顶页A/B映射及反转选出的候选按钮、符合顶页映射的Y、Return或KeypadEnter（Unity值271，不是RightControl）可维持点击。click=−1首次按住发原MotionEvent action0；已按住且两轴位移各向零截断后的平方和>0才发action2；松开已有点击发action1并清click。每次以pointerID0、当前float坐标、null delta和`isMousePointer=true`构造MotionEvent，交Canvas.OnTouchEvent；这些action不是GameView的TouchEvent TYPE值 |

APPLI_TYPE4的合成坐标再各加`trunc((Screen尺寸−Canvas尺寸)/2)`，与光标绘制的客户区域补偿保持分层。`OnTouchEvent` RVA`751000`倒序把事件交现有surface的虚方法；此入口本身不直接KeyClick确认。最终组件命中、guard、marker及世界／菜单消费仍走原surface链，不因虚拟事件存在就视为全页输入认证。

显示入口为FormManagerBase.OnDraw的`107E77A0`，在页面绘制后、附加控制器图标和Touch效果前调用`DrawScreenCursor` RVA`7497B0`。该方法先令独立drawFrame加1，达到192归0，即使Joystick模式随后跳过图像也已推进；非Joystick且位置数组存在才画图。位置两轴**先各向零截断为int**，APPLI_TYPE4再加`trunc((Screen尺寸−Canvas尺寸)/2)`，Canvas.GetWidth `74CB90`已核直调native GetWidth；之后把整数锚升float传Seb.Draw。该方法本身无PushOrigin或首层Translate，不预缩屏幕目的矩形。平台Kairolib.ScreenCursor getter`807370`实际取resource.seb[10]／图片12，原图与SEB已出版到[平台素材目录](../assets/steam-platform/)；显式frame0／1、layer1基础Draw继承调用前的矩阵与混合状态。

按下图选择与合成Touch不同：顶页AKey=KeypadEnter271才允许候选button0；BKey=Backspace8才允许候选button1。reverse=false取A候选、true取B候选，该候选存在且`GetJoystickButton(candidate,false)`为true即按下；否则`[(guiMode=0且键盘Y按住)或Steam.Buttons.Y为true]且顶页YKey=271`才按下。按下用frame1，其余frame0；这里不把Return／KeypadEnter直接添为按下图条件，也不猜布尔查询背后的物理设备映射。

恢复N>0的正常调用严格为：基础Draw→`PushMatrix(ScaleTemporary(s,s,0,0))`→`SetRenderMode(1,a)`→**同一原锚、frame、layer1**再次Draw→无参SetRenderMode→PopMatrix（`10749B4E–10749C80`）。`s=Graph.Easing(1.f,12.f,12.f,float(N),−80.f)`，`a=Graph.Easing(0,160,12,N,−80)`；N不由Draw递减。mode1包装`774A80`传`(op1,src=a,dst=255−a)`，无参`774980`清为`(op0,src255,dst0)`并置dirty，**不是恢复旧ambient模式**。Pop只弹本次scale，原异常路径未证明总能配对完成；维护RAII和拒绝属于另行安全策略。

Easing浮点／整数入口分别`8A4B20`／`8A4A40`：frame<0返回v0，frame≥length返回v1；中段令F为逐步float32舍入，`t=F(frame/length)`、`angle=F(F(F(t×πf)×180f)/πf)`，πf为40490FDB；FastMath.Sin `870E50`把angle升double，经ToRadians `7F7E20`**先乘doubleπ、再除double180**，调用与System.Math.Sin `A09410`相同的原生Sin目标，再降F。不能先把π/180合成常量或把整式改成double。令`u=F(F(Sin(angle)×F(−80f/314.1592712402344f))+t)`，scale为`clamp(F(F(u×11f)+1f),1,12)`；alpha为`clamp(trunc(F(u×160f)),0,160)`，其原整数端点／差值仍按int32运算。这里的Sin／x87到double交接不保证跨编译器或平台逐位像素等价。

可独立计算的源公式短测oracle：N=1／3／6／11时scale约为1.19168162346／1.76930403709／3.69887304306／10.3583488464，alpha为2／11／39／136；N=12直接scale12、alpha160；N=0不输出叠加请求。合法位置(17.9,22.9)、Screen(1001,701)、Canvas(801,600)在type4的锚为(117,72)，非4为(17,22)；旧drawFrame191下一值0，隐藏也推进。数值由已核公式单独计算，未执行原DLL或取得原窗口像素。lambda的stop计数无活动时增到101，活动时清0、达到100后再活动置恢复12；**Draw不读取stop计数隐藏基础光标**，不能由SCREENCURSOR_HIDE_COUNT名字推“100帧后消失”。

维护[纯计划接口](../prototype/include/dungeon_village_prototype/steam_cursor_skin.hpp)／[实现](../prototype/src/steam_cursor_skin.cpp)分开显式按下资格与几何输入，返回有序Draw／矩阵／模式请求及next_draw_frame；坐标±65536、消费尺寸1..65536和恢复0..12是维护预算，隐藏未消费字段不校验，不称原规则夹限。它不合成Touch、修改Owner或新增持久字段，也不发现原设备。真实准入后才提交next值，冻结计划重复展示不能再推进drawFrame；完整模式切换、float位置／previous／click及恢复调度仍属共同平台上下文，现研究窗口没有接整套原虚拟输入。原OS投递、设备映射、全部guard及框架次数仍待证，纯计划／素材交付不升级为原窗口已验。

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
