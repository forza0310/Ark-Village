# 设施74／81皮肤：只读计划的最小接入边界

2026-10-10。按实际维护接口审计[Steam74](../../ui/STEAM_FACILITY_DETAIL.md)、[软标签补证](../../ui/STEAM_FACILITY_LABELS.md)、[窗框](../../ui/STEAM_WINDOW_FRAME.md)和[产品请求](../../../../docs/reference/RESEARCH_REQUESTS.md)。本包仅方案，不改代码／测试，不构建、提交或操作窗口。并行分支落盘后已实际阅读[Steam81合同](../../ui/STEAM_FACILITY_UPGRADE.md)，以下81边界以正式内容为依据，不只依赖阶段消息。

## 当前已有的消费能力

| 模块 | 已有能力 | 实际限制 |
| --- | --- | --- |
| `steam_startup_skin.hpp/.cpp` | 标准C++有序variant计划；SteamStartupText含角色、行值、点／矩形、anchor、颜色、字号、行距；窗口／Box／白角／Window2／3完整展开；Touch原组件／value／矩形或SEB注册引用 | Text role目前限标题／保存菜单；不是完整文本后端，也未持有裁剪栈、设施资源身份或任意富文本执行器 |
| `startup_world_visuals.hpp/.cpp` | 建筑分片、74效果图标及重复加号、设施小图标、人物头像、cd／X4等只读映射 | 原图身份默认APK；不能拿相同SEB号隐式绑定Steam差异PNG。建筑DrawMapchip不含74 DrawMapchip2的居中修正 |
| `startup_world_building.hpp/.cpp` | 真实74实例／定义绑定、合法性、价格／品质／魅力／维护费；第二页冻结来源顺序、编号后缀及原符号；81共享升级与冻结ap数据 | 业务数值只读消费即可；不能在绘图函数再次升级、重扫邻接、扣款或另建选择Owner |
| `startup_view.cpp` | ChineseFont::text／measure／paragraph，SourceSprites消费资源裁片和SEB；74已有简化文字／图块展示 | 未调用SteamStartupSkinPlan；ChineseFont是维护raylib字体与按码点换行，不能冒称Steam TextLayout、翻译或标签后端。74当前面板仍为简化纸色矩形，不能仅更换函数名宣称原皮肤 |
| 既有visuals套件 | `startup_world_visuals_test.cpp`、`startup_skin_checks.cpp`、`steam_startup_skin_checks.cpp`共用实际PNG／SEB和CPU图像生命周期 | 有合适目标可扩展，不需逐页新target；没有原窗口逐像素认证 |

结论：**有公开文字布局请求，没有已接research窗口的公共Steam文本执行器。**可以先交完整可消费的逻辑图元／文本／clip／touch计划，保持后端未知；不能把布局计划通过等同实际中文字形或PC输入验收通过。

## 第一片：74普通预览与普通实例两页

最小独立切片优先普通布局3：定义预览和实例页0／1。现有Owner足够提供绑定、页号、价格及邻接冻结列表；无需新增持久字段。装备／植物／入住三种静态布局在同模块按真实分派补齐，住宅动画单列其副作用边界，不用“全布局默认同一套”掩盖缺口。

建议新增实际职责模块`prototype/include/dungeon_village_prototype/steam_facility_skin.hpp`及对应src，仅加入既有startup_application库源清单，不建新target。入口分为短寿命的只读页面view和纯`steam_facility_detail_skin(view, context)`，不保存第二份Owner；context只接本次VIEW_Y／frame／语言分支／真实测宽结果等表现事实。页面绑定错误、预览却提供实例邻接、普通实例页号非法、退休来源／缺字段须显式拒绝，不能map.at异常或补默认0。

具体消费边界：

1. 窗框220×168与内容框按现Steam helper展开。页面外origin和VIEW_Y只在各自原调用点应用一次；文字标题阴影／正文仍需两次真实测宽，null和空串不合并。
2. 保留原绘制穿插顺序。新增有限clip请求类型表达`ClipRect(push=true)`／`PopClip`等实际操作，不能将全部图先画、文字再画；74大图clip必须准确包围Mapchip2，不能把后续名称／价格夹入。
3. 大图用真实Steam mapchip绑定与朝向0，在既有分片上加pattern0=(-30,0)、1=(-45,7)、2=(-30,15)的Mapchip2居中偏移；底板97／98宽的DrawRect实参与半开尺寸分开。不得按底板包围盒缩放整栋建筑PNG。
4. 数字应保留SEB、值、padding、anchor或展开为原数字图块；价格／品质／魅力／种类不能统一成普通系统字体字符串。定义值／实例值／上限／共享lv／剩余使用次数从当前维护查询取，绘制不重算业务交易；实例MAX与预览无MAX的资格分开。
5. 第二页沿初始化冻结来源原序，最多5行，读现有bonus_window，保留编号+1、植物双项、负数“+-3”和空列表。输出已核SLIST11原矩形与值；74没有75的marker16/Margin政策，不复制其二次点击资格。
6. `Draw_btmMsg`、滚动条、数字及富文本等helper若尚无完整已核展开，先具名保留实参和资源／测宽依赖，并将未展开项列为消费缺口；不能称这份混合helper计划已经是可直接逐图元渲染的完整皮肤。

文本复用应控制改动：现SteamStartupText的几何／字号／anchor合同可直接使用；新增设施角色应是具名有限枚举，不能随意把74标题冒充message_title或用任意整数塞role。若决定抽取共用布局字段，只在这两个实际消费者之间做小型值类型整理、保留启动接口适配和既有测试，不引入通用UI树／控件框架。该整理不应成为全库重命名或新Owner的理由。

## 输入计划与业务Owner分工

74软标签固定[0,2]：左空、右返回。逻辑确认0x100000独立，不创建左“确定”按钮。普通实例两页箭头仍分别Touch组件1值16／18；两次同轮左右输入可切两次，皮肤不能把页面改成自己管理的单向状态。页1确认无业务，预览确认Pop，真正道具／商品／入住动作由既有Owner执行。

纯计划只输出已核逻辑触摸矩形和无坐标helper注册事实。输入相位、当前软标签脉冲消费、原组件标记以及OS缩放／hit test属于独立输入消费者，不在皮肤中根据鼠标hover直接更改page/selection。没有原矩形的Touch组件2 Create(2)只保注册请求，禁止猜成全窗口或底部按钮。

住宅74已有SetDispPlayerData写共享表现对象的调用，不能放进声称只读的函数；该页自然入口也不能忽略已入住住宅通常转60的实际分流。先把图元计划标条件分支并沿显式表现请求取得冻结人物计划，或者明确该子分支待接；不要为了全布局数量声称这一副作用已消费。

## 第二片：81升级页的依赖边界

现Owner通过`consume_startup_world_facility_upgrade`在首次初始化真正升级共享定义并冻结三属性旧／新／差额；`facility_upgrade_initialized`、页面phase／counter及定义ap是业务权威。皮肤只读取已初始化状态，不能因为绘制phase0又升级一次，不能由“差值已显示”推导收费／升级提交。

正式81合同已具名闭合：两阶段背景event14／16，大图Mapchip2 clip(80,74,80,64)，phase0文字滑入clip和非日文四次真实测宽，双空软标签[0,0]、确认组件2及40／55门槛；三属性使用特定`RateConvert_countAnime`，不是普通线性插值。page0 frame1的jingle20在update，不进只读draw；页面第二计数frame2与phase计数不可合并。上述可以作为下一实现批独立oracle，不再登记为未知等待项。

81最小输入是已初始化Owner的设施定义／实例ID、当前共享lv、冻结ap三槽、当前phase／frame和独立frame2，另加语言与实测文本宽。只读计划无需完整人物Owner：两侧秘书／大臣是common SEB80／图93的已核固定ID5／6，frame2模30驱动影0/1及角色3/5、2/4；float32抛物高度应逐运算保持来源，不借标题人物调度或当前人物动作。

应在现visuals中集中补的81边界是：phase0 frame39/40只影响输入准入，箭头到50才可见；phase1 frame54/55及55后mod25；属性frame1/2、31/32、33/34、48/49；差值0与±1在34–48仍保持old，较小正差可提前结束，下降分支不能镜像正差；小角色frame2=19/20/29/30与切段frame2不清。这些只验证投影，阶段切换／真正升级／末次清提示继续由Owner原套件负责。

实现仍放同`steam_facility_skin`模块，分`steam_facility_upgrade_skin`独立入口，复用74确实同义的资源／clip／数字图元。若原动态文本组成需要多次StringWidth，context必须按各原调用／字号提供实际值或只读测量回调，不能字符串长度乘字宽。角色演出与资源帧只在原调用身份／偏移完整时展开，不因同PNG而套标题人物调度。

## 资源、测试与接窗条件

Steam差异资源已有独立[设施包](../../assets/steam-facility-common/README.md)：图片37／105两PNG合2438字节，SEB15与APK同字节且仅引用，不复制；建设包另有不同于APK的图片98／103。所有设施计划必须保留Steam资源包／槽身份，不能让现SourceSprites::Binding::common自动从APK目录取同名图。没有必要再复制图像。

最小测试主责：

| 职责 | 扩展位置 | 验收重点 |
| --- | --- | --- |
| 纯布局／输入描述与语言分支 | 既有`steam_startup_skin_checks.cpp`或同visuals可执行中的职责文件`steam_facility_skin_checks.cpp` | 页0／1、预览／实例、MAX、5行与空列表、箭头frame边界、文字实测宽与clip栈原序、不能生成虚构确认按钮；不新增target |
| 原资源与分片 | 既有visuals CPU资源生命周期 | Steam差异PNG manifest/hash绑定、SEB裁片、Mapchip2居中、同ID跨包拒错；不重测世界经济算法 |
| Owner只读／退休／升级业务 | 既有building套件主责保持 | 仅新增调用计划前后完整Owner／随机／声音digest不变、错误绑定拒绝等接线检查；不复制原升级／邻接全组合 |
| 实际窗口与OS输入 | 后继独立接窗批 | 原图与实际文字测宽、色彩／裁剪／原点；原通道不可用时输出外部提示词，不把CPU图替原窗口 |

当前不建议直接修改`startup_view.cpp`覆盖整个74／81：它没有通用Steam计划执行器，且全场景／页栈绘制相位和字体后端仍有限制。先交可审阅只读计划及CPU示例；待有明确可消费helper／资源／层序后，只在对应页面现有位置接入，不挪整个renderer层序。输出计划为本次短寿命值，不进Owner、快照、第二事件队列；测试记录图元规模、每页引用数量、临时CPU图释放和原页面退休，正常历史增长另审。

本包不承诺具体完成比例。可直接实现的是已核静态布局／原图／只读数值到计划的桥；需要独立原动态或后端工作的包括实际字体／TextLayout标签、OS热区、原页面生命周期全部相位、住宅表现副作用以及完整窗口视觉一致性。
