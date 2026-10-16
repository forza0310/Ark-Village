# Steam标题人物调度与基础绘制

2026-10-09。固定Steam2.56 GameAssembly及metadata的具名方法静态交叉，补[标题Draw](STEAM_TITLE_DRAW.md)，[来源身份](../EVIDENCE.md)统一维护。未运行原游戏、读写内存或原档；APK[标题调度](TITLE_PRESENTATION.md)和[人物皮肤](TITLE_ACTOR_SKIN.md)独立认证。

## 已证版本差异

| 项目 | 固定APK1.0.8 | 固定Steam2.56 |
| --- | --- | --- |
| direction=1出生x | 0 | -18 |
| direction=0出生x | 240 | 254 |
| 每次背景更新移动 | `2*direction-1` | 相同 |
| 活跃槽退休条件 | `x<-10 || x>250` | `x<-18 || x>254` |
| 绘制边缘处理 | 该标题方法无此Steam新增边缘混合 | x<-14或x>250时设置混合；4单位宽 |
| 完整横穿后退休的移动次数 | 251 | 273 |

最后一行是由出生、速度、严格越界比较推导的次数，不是墙钟时长，也不是新执行的原窗口长跑。Steam差异会改变活跃槽寿命、槽满概率与其后随机消耗；不能只在绘制器改坐标而仍把APK自然轨迹称为Steam轨迹。当前维护模式与已认证快照没有在本批切源。

Steam边缘参数：x<-14时`srcRatio=abs(-18-x)*64`；x>250时`srcRatio=(254-x)*64`。正常活跃范围内，边界−18/254为0，−17/253为64，−16/252为128，−15/251为192；对应dst为255／191／127／63。**只有边缘演员**先`SetRenderMode(1,srcRatio)`，再画阴影与人物助手，末尾调用无参`SetRenderMode()`。中央区`−14≤x≤250`既不设置也不重置模式，继承进入该演员时的环境模式：VA `1020AAFB`先设局部哨兵−1，`1020ACFB–1020AD07`仅哨兵被边缘计算替换时才执行尾部reset。不能为统一渲染流程对每个中央演员补reset。

两参数重载令`dstRatio=255-srcRatio`；三参数重载写`renderOperator_/renderSrcRatio_/renderDstRatio_`并刷新绘制缓存。无参重载恢复`(0,255,0,replace=true)`，**不是恢复调用前的模式栈**。这认证了源端混合参数，尚未将底层渲染器、任意残留人物特效及最终屏幕透明度全部认证为一条无条件等式。

## 调度和随机准入

`TitleForm.Update_titleAnime`（RVA`0x20D5D0`，完整1056字节）先遍历活跃槽：age加1按`INT_MAX`回卷，移动x，再严格越界退休。`ResetHobjExist`只清active，不清age、y等旧数据。出生也不重置age；新生本轮不再移动。

随后出现计数加1，到间隔时清计数，先抽新间隔，再找最小空槽。无空槽时已消耗间隔一抽，有空槽才继续后三抽：

| 顺序 | 抽样与消费 |
| --- | --- |
| 1 | `Random(100)`；`RateConvert(ticket,0,99,20,70,true)`，即20+整除`ticket*50/99` |
| 2 | `Random(TITLE_GUEST.Length)`，池已从Steam fieldRef与44字节blob哈希独立核成0…10 |
| 3 | `Random(2)`取direction，并决定−18/254出生边 |
| 4 | `Random(100)`映射y=210+整除`ticket*7/99` |

因此正常有空槽是`100→11→2→100`；20槽全满仍先抽间隔，不能优化成零抽。`SetAppearCnt`使用同一间隔公式。Steam `GameUtil.Random(n)`读静态共同`JRandom`，调用无参数`NextInt()`，有符号余数后交给`JMath.Abs`；本批没有认证Steam底层PRNG与APK Java序列等价，不能仅凭四个bound相同合并随机基线。

`TitleForm.Update`（RVA`20C230`，至下一具名入口`20D5D0`的5024字节，含末尾填充）现已顺序核完：父更新计数回卷后，titleFrame饱和递增、frame回卷；更新后的titleFrame<100时先查确认脉冲，可跳到100。仍<100则跳到`1020D2CD`直接返回true；达到100后调用背景更新，之后才处理调试长按、指示SEB及菜单输入。背景调用点VA`0x1020C4A7`。因此门槛前的提前返回也绕过尾部平台／调试输入，不是只暂停人物移动；[Draw的75门槛](STEAM_TITLE_MENU.md)与此处Update的100门槛独立。

确认键mask`0x100000`不走JoystickIndex分支；无输入guard且可用Keypad时，`CheckKeyPulse`读取并清除该bit。故旧titleFrame=98时，同一确认脉冲可用于跳动画并在本轮更新人物，但不会再次供菜单确认；旧值=99自然加到100，不经过提前消费，后续确认仍可能命中。它不单独证明所有输入设备的完整键位或脉冲生产链。

### Update的后半部与输入边界

2026-10-16补核剩余范围后，以下顺序可供产品接线；菜单／选档各项业务结果仍集中在[标题菜单合同](STEAM_TITLE_MENU.md)和[保存菜单合同](STEAM_SAVE_MENU.md)，不在此复制第二套规则。

| 顺序 | 原方法直接行为与边界 |
| --- | --- |
| 背景后 | 查询`CheckKeyState(0x800)`，未按下清POUND计数，按下递增且严格大于40才置STAR；本方法没有相应清STAR分支。随后推进common SEB22一次，再按state分流 |
| state0 | 先Up、再Down repeat，再处理已有dialog；之后依次查询软标签1、软标签5、脉冲`0x1000000`、确认`0x100000`。前三项分别走既有询问、`URL_SP`外链、LanguageForm；不应把同轮输入先合成一个无序按钮集合 |
| state1 | 按Left→Right→Up→Down查询repeat，随后把无中断档的中断行改为手动行；之后才检查已有dialog。无dialog才查询确认，确认未命中才查软标签2返回；返回调用`ChangeState(0)`后进入尾部，保留选择，详见菜单合同 |
| state1后继 | 空栏NewGame调用后回到尾部；非空手动栏Push raw20后立即返回，非空中断ContinueGame后立即返回。已有dialog结果分支处理及清引用后走独立false返回，不经过普通尾部 |
| 普通尾部 | `1020CE1D`先检查STAR专用`0x400`脉冲／原对话请求，再在`Config.APPLI_TYPE==4`时检查`0x800000`脉冲，并于`1020CF08`调用`SteamInputManager.ShowBindingPanel`，最后返回true |

尾部绑定面板请求不是仅state0可达：选档无确认、空栏NewGame返回及软标签2返回等路径也能走到它；新子页Push、直接Continue等提前返回则跳过。不能仅因[Draw只在栈顶state0显示平台图标](STEAM_TITLE_MENU.md)就把Update快捷请求也限制成相同资格。ShowBindingPanel具名入口`7B7E00–7B7EA0`另有`Config.USE_STEAM`与manager初始化检查，然后把当前64位handle交给SDK；该入口未额外检查handle非零，不能补用相邻其它方法的资格。本批只核请求链，不执行SDK或声称当前手柄可用。

STAR／`DEBUG_CMD_DEV`／`DEBUG`／`PTW`保留为原调试旁支，不能默认纳入普通经营接口：其中可见Http.Connect、RecordStore.WriteRecord、系统日期槽／字符串修改和SaveSystem；另一STAR路径调用SendSaveData。原方法内这些调用是有副作用的，不能以“标题背景”之名整体放进可重复只读Draw。上述静态分支存在不等于用户正常冷启动实际已准入；本批未调用网络、调试命令或原档。

**guard限制输入，不直接暂停背景。** `CheckKeyPulse(int)`（747E60）与`CheckKeyRepeat(int)`（748250）在读取相应输入前均调用`Canvas.CheckInputGuard`（7479E0，完整512字节登记范围）；标题的`CheckKeyState`（7484E0）也先查同一guard。已核guard依次处理：any-key标志仍有效则拒绝，否则清该标志；非零截止时间仅在`deadline−10000≤now<deadline`时拒绝，过期或距离当前过远时清零；倒序剔除guard列表中mode4表单后，当前`GetTopForm`仍在列表才拒绝。时间来自CurrentTimeMillis，不是标题tick，10000是该异常期限比较常量，不能改写成标题动画时长。any-key所依赖框架静态标志的生产者、guard设置者与所有跨线程时点未在本批展开。

标题Update本身没有先以栈顶／生命周期或统一guard阻断三种时钟及人物更新；每次查询才分别读guard，guard检查还可能清期限／退休引用。Up／Down查询在guard拒绝前仍增加Canvas的`checkUpDownKeyCount_`，并非严格只读查询。调用者须保留[框架准入](STEAM_INTERACTIONS.md)与单轮原序，不能把一次guard结果缓存后任意重排全部查询；这里不证明非栈顶Title实际会被框架调用，也不扩大为完整设备／焦点或全部输入生产链已认证。

## 排序、资源查询与图层

`Draw_titleAnime`（RVA`0x20A710`，完整1600字节）先把全部20槽的`(slot,y)`写入临时排序表，包含inactive槽；外层i递增、内层从末尾向i+1递减，仅前y>后y时交换。排序后才跳过inactive。它是非稳定交换排序，不能替换为`stable_sort`再宣称同深度等价。

这里交换的是当前i／j两项，不是相邻冒泡。独立顺序样例：slot0／1均活跃y212、slot2不活跃y210、其余不活跃y217，最终可见顺序为slot1→slot0；先过滤inactive再排序会改变结果。

逐活跃槽的调用链如下；背景画完后草边才画，见标题Draw合同。

| 项目 | Steam已核源端请求 |
| --- | --- |
| 定义 | `AppData.characterData_[slot.definition]`；不是世界人物实例 |
| 步帧 | `(age%20)/5` |
| 朝向 | direction=1取1，否则2 |
| 锚点 | x=slot.x，y=slot.y+`view_.GetGameHeight()-240`；继承外层标题原点 |
| 阴影 | common SEB25、图片3、frame0；VA`0x1020AC0A` |
| 身体图片 | `jobs[character.jobId_].imgId_[character.gender_]` |
| scratch setter | `SetDispPlayerData(0,bodyImg,step,facing,age)`；VA`0x1020AC98` |
| 主武器 | `character.equipDataId_[0]`传入`DrawDispPlayer`；VA`0x1020ACF3` |

演员高度来自`TitleForm.view_`（VA `1020A9CB–1020A9EF`），不是外层标题取到的`GameView.H`；因此应将view高度与继承原点分别传递。外层先设`half(GameView.W−240)`横向原点，演员局部锚仅加`view_H−240`纵向位移；身体／武器SEB偏移后续各加一次，不重复居中或把两种高度强行合并。

`DrawDispPlayer`先调用`SetDrawParam`，当共享显示人物`objType_==0`且weaponId!=-1时画武器，最后画身体。因此主路径图层为**阴影→有条件武器→身体**。没有读取防具或饰品另叠衣服；这不代表其它人物效果没有额外层。

`SetDrawParam`的正常人类、action0、`haveObj_==-1`分支：身体SEB取`HUMAN_ANIME_SEB[0]+direct_`，帧取`HUMAN_ANIME_SEB_F[0][step]`，图片取bodyImg；身体偏移从`body_off[0]`取得。后续[数组与资源专题](../verification/steam-actor-arrays/README.md)已独立闭合此分支的Steam常量与裁片，见下节。

`Draw_weapon`沿当前武器定义的`chargeType_`选`WEAPON_ANIME_F[chargeType][0][step]`，SEB取`WeaponData.WEAPON_SEB[chargeType]+direct`，图片取`weapon.imgId_`，锚点加`GetWeponOffset(0,chargeType,direct,step)`。标题传`zsort=false`，直接DrawSeb，不走世界绘制队列。action0不会进入action1／5的斩击光分支。`GetWeaponAnimeIndex(0)`已核返回0；完整`GetWeponOffset`证明action0选择`WEAPON_WALK_POS[type][direct][step]`，没有额外插值或随机。

## Steam数组、SEB与图片交叉闭合

2026-10-09续批：独立证据（本地核对材料，不随仓库交付）。固定两具名cctor在成功分配／类型检查路径沿显式常量及真实fieldRef解读，目标字段全部发布后停止；Character2前10,513字节、WeaponData前25,697字节。未知值、指令或控制流显式失败；没有执行游戏代码，没有用APK常量补洞。三个编译器小helper由原调用点追到真实入口并分别核≤128字节的成功路径。

| Steam字段 | 正常action0已核值 | 静态发布VA |
| --- | --- | --- |
| HUMAN_ANIME_SEB[0] | 0；因此SEB=direction方向号 | `0x10294B8C` |
| HUMAN_ANIME_SEB_F[0] | `[0,1,2,3]` | `0x10294F8E` |
| body_off[0] | `[0,0]` | `0x10295E6E` |
| WEAPON_ANIME_F[type][0] | type0…3均为`[0,0,0,0]` | `0x102958D6` |
| WEAPON_SEB | `[0,4,8,12]` | `0x1023029E` |
| WEAPON_WALK_POS | 下表的4类×4方向×4步帧×2坐标 | `0x1022BFA8` |

表中发布点不是消费点；消费顺序仍以上节实际Draw链为准。行走偏移的128个整数额外逐项直接检查原机器码`mov [ebx+disp8],imm32`与有符号立即数，独立于cctor静态解释器。int[]新建后的未写零槽是语言初始化语义；引用数组缺子数组则拒绝，不能把未知值当零。

| chargeType | direction0 | direction1 | direction2 | direction3 |
| --- | --- | --- | --- | --- |
| 0 | (-3,-28) | (-3,-28) | (-18,-28) | (-18,-28) |
| 1 | (-7,-22) | (-7,-22) | (-15,-22) | (-15,-22) |
| 2 | (-5,-28) | (-5,-28) | (-20,-28) | (-20,-28) |
| 3 | (-9,-38) | (-9,-37) | (-22,-37) | (-23,-38) |

表为step0/2；step1/3只把y加1。Steam本次读数与APK正常行走偏移相同；这是独立读数后的比较。标题实际只传direction1/2，但数组的四方向均核，不把它们转换成未经证明的世界朝向名称。

从Steam `resources.assets`的human／weapon／common TextAsset分别解码归档，独立解析`img.inf`、`seb.inf`和原SEB。共复核98项：human37图、weapon33图、阴影1图、21个所需SEB和6份目录；96项与APK原件字节相同，common两份目录整体不同，**所消费的common图片3和SEB25实际文件及其槽位已核一致**。不因目录整体不同否认已核局部，也不因局部相同声称整组相同。

| 图层 | Steam独立解析的资源与裁片 |
| --- | --- |
| 阴影 | common图片3、SEB25；frame0裁片`(0,0,12,2)`，offset=(-6,-1) |
| 身体 | human SEB0…3，walk00…03；18×24，四帧x=0、18、0、36，y=24×direction，offset=(-9,-24)，无翻转 |
| 武器type0/1 | sword／bow方向SEB，frame0；21×25，x=0、y=25×direction，SEB offset=(0,0) |
| 武器type2 | spear方向SEB，frame0；26×28，x=0、y=28×direction，SEB offset=(0,0) |
| 武器type3 | greatSword方向SEB，frame0；32×35，x=0、y=35×direction，SEB offset=(0,0) |

human图片索引没有32，现有37图均核实际像素／尺寸及所需身体裁片边界。weapon33图逐个核字节与像素身份；本工具未重解Steam武器表的每条定义→图片／chargeType配对，不能把所有类别SEB交叉套用到每张武器图。body第2帧重复站立列，不等于取PNG第三列；武器frame固定0，不随身体步帧切到第1…3帧。人物锚点、武器行走偏移与SEB内部偏移应各加一次。

本节闭合正常action0的源端常量和资源身份，仍不证明共享scratch在任意历史中都处于该前提。完整其它动作、特效及产品实际绘制应用继续各自验收，不能由解析器通过或文件hash相同替代。

## scratch副作用与交付边界

`SetDispPlayerData`只改共享显示人物的`seb_/bodyImg_/animeIndex_/direct_/sebCnt_`五项。`SetDrawParam`还写共享临时字段和`draw_param[5…10]`；`Draw_human`结尾写`effect_pos[0/1]`，不能误标为另一份`old_screen_pos_`。它们不是严格纯函数。

2026-10-14已闭合**重新执行标题Init时的scratch构造来源**：TitleForm.Init（RVA20ADF0，完整1936字节）的正常完成路径在`1020B322`实际调用UserData.Setting（2E89D0），不是仅由方法名推定。Setting在`102EA205–102EA2A8`新建Character2、调用其构造29D2E0，把新引用写入`UserData.dispHuman_ static+14`，再执行Init(0,0,0)、uid=−1、jobId=3；接着独立建立dispMonster_，两者不是同一对象，也不是标题20个槽各有一个scratch。此处每次重新构造替换旧static引用，不是清空／复用旧Character2；旧引用的全部别名及GC退休未由该赋值证明。

Character2.Init（284080）的`10284193–1028421D`显式写objType=0、动画／朝向／seb=0、flag／state／相关计数清0、**haveObj=−1**，并绑定共同AppData／Canvas／UserData；Init本身没有把所有历史字段清零。上述新构造另外分配draw_param／old_screen_pos／effect_pos、两空效果列表和6槽powerBar；effD_hitTime／effD_miss及未显式赋值的新标量、数组槽按语言分配语义为零。因此“成功完成新标题Init并尚未被其它使用者改写”已有明确objType0／haveObj−1／空效果的正常来源，能支持本页正常action0数组／裁片前提；不能再笼统说其初始化完全未知。五字段setter仍不恢复这些槽，父页返回与新标题Init分开。

**标题纪录返回已核不重Init。** 具名原类型为`form.RankForm`；其Update RVA203420在软标签2命中后于`102034A0`直接转FormManagerBase.Pop（7E7D40），再经7E7D60的UI线程委托7F29E0令子mode4、逆序找到原父；父mode3于`107F2DBC–2DC2`转PREUPDATE1。`_executeInitAndFinish`（7E9360）只有mode0走Init，mode4走Finish／移除；`_execute`（7E98D0）在`107EA297–A309`仅复核同一栈顶、调用PreUpdate并转mode2，PreUpdate（7E7EB0）消费Canvas输入。这里不会把原Title设mode0或再调用Setting。Title.Resume虽有声明，实际入口1EDFA0仅返回true，也不清scratch；不能凭Resume名字补一次初始化。Title.NewGame（20B8E0）明确Push新SubForm91；若91经同一通用Pop恢复Title，同样不重Init，但其具体取消／最终开始分支不由本批通用框架代为认证。

已核一个真实子页writer：RankForm._draw（203660，完整3680字节）在`10203FB3`按当前展示定义的职业／性别图、步帧、dir1和完整页钟调用SetDispPlayerData(action0)，`10203FDC`再传当前武器绘人物。该局部没有直接写dispHuman.objType／haveObj或重新构造它；下一标题演员的五字段setter会覆盖这些预览参数。DrawDispPlayer／SetDrawParam／Draw_human仍沿共享后端写派生数组与旧坐标，不能由Rank局部无直接写者外推其全部传递调用或任意子页历史都保留空效果。因此纪录返回的生命周期已证，全部残留资格仍须按具体writer和附效子树逐项核，不能对返回动作隐式清零。

2026-10-16进一步窄核**新Setting的scratch尚未被其它使用者改写**这一前提，沿`Draw_human RVA27BCF0`实际后继闭合下列资格；不外推纪录返回或任意历史。

| 分支 | 新对象能排除的内容／仍需显式的环境前提 |
| --- | --- |
| 伤害／miss | `DrawEffDamage RVA276BF0`在`10276C2B–10276C33`先判`effD_hitTime +0x114<=0`直接返回`10276F86`。新标量0使后续数字、miss和附效分支均不进入，没有图像或随机请求；不必再将这条新对象路径整体列为未知。 |
| 效果列表 | `DrawEffect RVA277150`在`1027720D`读`effect_ +0x19C`，空表count0于`1027722F–10277231`直接到`1027ADAF`返回。本路径不读第二列表，不执行列表内图像、随机或渲染模式调用；不能外推非空效果条目。 |
| 携物／倒地 | `1027C075–1027C087`的haveObj−1跳过携物；`1027C213–1027C217`的state0跳过倒地表现。旧haveObj可改变正常行走SEB的既有风险保持。 |
| 相机选中／HP | `1027BD63–1027BE0D`从AppData.camera.subPlayer检查非null、uid相同且objType0后画选中图；HP在`1027BFC8–1027C067`以`powerBar[4]==1`或相机subPlayer同uid准入。新powerBar零只排前一资格，scratch uid−1也不替代相机检查。无该两项附图的充分环境前提是subPlayer为null或其uid不等于−1；仅objType不同不足以排掉HP。`Draw_powerBar`普通seb0即使powerBar全零也先在`1027D819/1027D867`画边框，不能靠零HP推定无图。 |
| 调试叠层／收尾 | `1027C408–1027C428`的门控是**Map.DISP_ENEMYAREA==1**，槽110FBACC经metadata独立绑定game.Map静态+0x20，不是Config.DEBUG。未命中才跳过Draw_stateInfo／Draw_actInfo；新scratch不决定这个全局值。正常末尾`1027C514–1027C53F`仍写effect_pos，两次重复Draw并非无状态。 |

因此正常新标题Init已给出空伤害／空效果、无携物／倒地的真实来源；再显式具备上述相机不匹配与Map门控不为1时，本次所核人物附效分支不出图、不抽随机。这是局部充分前提，未认证Title.Init清除了相机选择或该全局开关，也不认证整棵Title.Draw／底层GPU无随机或像素一致。共享对象全部别名／历史写者、非空效果子树及其它父页恢复仍保留；不能据此宣布任意历史的标题人物永远只有基础三层或淡出模式不会被深层覆盖。

维护[标题人物接口](../prototype/include/dungeon_village_prototype/startup_title_actor_skin.hpp)新增`steam_title_actor_projection`：借既有六整数槽DTO，独立校验Steam活跃x范围−18..254与退休保留端点−19／255，全部20槽按源顺序排序后输出人物助手实参及可选边缘模式；输出最多20人，view高度1..65536为维护预算。现visuals已短验非稳定次序、边缘四级模式、退休保留及拒绝边界，[标题组合示例](examples/steam-title-menu-slots.png)仅消费两名中央普通演员。中央无模式请求，边缘请求要求消费方在阴影／人物助手之后清默认模式；投影不展开深层scratch，不调用APK更新／校验器，不新增Owner或快照模式，也不把已认证APK自然轨迹切换成Steam。

同模块`startup_title_actor_skin`仍读取固定APK维护定义，只提供正常基础阴影／武器／身体图层；其正常action0数组和裁片已有本页独立Steam交叉，但不能据此声称Steam每条定义配对、任意scratch历史或完整人物子树已维护。后续继续查不重Init的共同显示残留／别名资格、框架准入及JRandom交接；窗口观测可交叉淡入／淡出与遮挡，截图不能单独证明内部槽、抽数或逻辑tick。

## 共同随机来源

固定Steam的`GameUtil.Random(int)`（RVA `0x2A44E0`）在`0x102A4526`调用共同对象的无参`JRandom.NextInt()`，随后有符号`% n`再取`JMath.Abs`（RVA `0x7F7B50`）。这不是调用有界NextInt(n)，没有因余数偏差重新抽签。标题与已核世界任务消费者经此入口共享`GameUtil.random +0x40`；不据此推断所有引擎／界面随机都使用它。

`GameUtil.cctor`在`0x102A6796`调用无参JRandom构造，`0x102A67A5`保存共同对象。`JRandom.ctor`（RVA `0x7F8050`）先取`JSystem.CurrentTimeMillis`，把结果低32位作为种子，同时构造下面三个后端；`fix_`为true时改为种子0。CurrentTimeMillis（RVA `0x849830`）读取UtcNow、减静态时间原点、取TotalMilliseconds再转整数；本批未展开该原点初始化，不把它擅自写成具体纪元。

`JRandom.NextInt`（RVA `0x7F7E60`）每次按静态`Logic`选择对象，再虚调用Next。枚举与实际分支均已核：

| Logic | 后端及已核内部行为 |
| --- | --- |
| 0 Default（分支中其它值也走此路） | `System.Random`。Next（RVA `0xA17780`）转InternalSample（`0xA174F0`）：56元素数组使用1..55槽，双游标递增回卷、两槽相减；恰INT_MAX改为INT_MAX−1，负值补INT_MAX，再写回。构造成功路径`0x10A17890–0x10A17A2C`将游标设0／21；不是Java48位LCG |
| 1 Unity2018 | `JRandom.Random2018`继承上述Random且没有重写Next；Init（RVA `0x808E70`）按原metadata字符串`_inextp`反射查字段，找到后在`0x10808F29`设置31，未找到则跳过。名称不代表调用UnityEngine.Random |
| 2 Xorshift128 | 独立四uint状态；Next（RVA `0x80B090`）以左移11和无符号右移异或推进并返回32位结果。SetSeed（`0x80B140`）令x=seed&0x7fffffff，y/z/w为原固定常量，不能混用前两支状态 |

`JRandom.SetSeed(long)`（RVA `0x7F7F20`）也取低32位、遵守fix_并重建三个后端；它不是只改当前选择后端的一个整数。`set_Logic`（`0x7F8300–0x7F832F`）则只在`107F832B`写共同静态字段+4，不调用SetSeed、不重建三个后端。故切模式与重种是两种操作：各JRandom实例的后端状态保留，而模式选择是这些实例共同读取的静态值。此处未执行任何切换。

**正常根启动的固定种子资格。** 2026-10-14沿Awake入口补核：`1078212E–10782133`先读`Config.UNITY_EDITOR`（类型551静态+28）；仅为true才检查`ActionReplay.CheckReplayFile()`或`PlayerPrefs.GetString("DEVCMD_REPLAY_RECORD","off")=="on"`，满足其一才在`10782182–1078218A`写`JRandom.fix_=true`。三字符串由本DLL使用槽与metadata字面表核对，未读取本机PlayerPrefs或回放文件。基础Config.cctor确实在`1083DA48`先把UNITY_EDITOR设true，但游戏MyConfig.cctor在`10228F62`写自己的同名字段false；沿[已核根应用配置链](../rules/PERSISTENCE.md)，Awake的`1078169C`先执行Config.Create同名字段复制，随后才到此资格检查。因此**正常根游戏启动不会因为存在回放设置就自动固定seed0**，不能用框架基类默认true代替游戏配置。该局部只有置true，没有false分支清除旧fix_；既有同进程历史、子应用或外部改写不由正常冷启动结论覆盖。修改fix_也不立即重置既有对象，替换仍发生在构造／SetSeed。

**具名重种调用与共同实例分开。** 可执行节对SetSeed的E8相对调用候选只找到`LineupManager.Start`（RVA7247A0）的一处；沿该方法入口核到`10725594–107255B2`先分配并构造新的局部JRandom，`107255BD`再取CurrentTimeMillis，`107255CC`给这个局部对象SetSeed，随后`10725649`用于推荐列表抽选。它不是读取或重种`GameUtil.random`。对set_Logic的同类候选扫描仍为零；字节候选须回到具名入口验证，零候选只说明未找到这种直接调用，不排除内联写字段、委托、反射或原生写者。Config.Create的已核反射目标是Config字段，也不能把它想象成JRandom属性setter。

**标题到新世界不是已证的新随机零点。** `GameUtil.Random`在`102A4505–102A4519`检查类型初始化后读取静态+40。`UserData.NewGame`在`102E0489`调用Init；Init的`102DF272–102DF28B`确实新建一个GameUtil，但其**实例构造**（RVA2A6C50）只重建SIN表及三组角度缓存，`102A6CB9`写的是静态+44，不是random的+40，也不调用JRandom构造或SetSeed。实例构造中的类型初始化检查只在静态类尚未初始化时触发cctor；不能把每次NewGame里的`new GameUtil`误读成每次新建共同随机。

本次同时沿TitleForm.Init／NewGame／SetAppearCnt、AppData.NewGame、UserData.NewGame／Init／Setting及AppData.LoadGame的具名方法检查直接调用边界：标题SetAppearCnt于`1020BD88`继续调用同一GameUtil.Random，未在这些入口发现JRandom重种或Logic设置调用。结合上面的真实实例构造，已证的普通交接没有“从标题拷贝seed并建立世界专用流”这一步；在共同对象未被其它写者替换时，世界继续消费它已有的后端状态，标题等待与已准入的抽签属于前缀。这里没有穷尽上述方法的所有传递调用或计算前缀总抽数，也没有把原存档未保存随机状态扩大成所有运行中恢复都不可能改静态字段。Awake另有`Config.MULTIPLE_PACKAGE`为真才进入的`ApplicationManager.BackupStaticFields/RestoreStaticFields`路径（`10782690–1078272A`）；基础cctor在`1083DB23`设false，MyConfig无同名覆盖，本批不将这条正常根启动跳过的框架能力认证成游戏档案随机恢复。

未见JRandom静态构造器，Logic静态零默认与Default分支一致；**静态默认0不等于实际运行模式已经观察**。精确冷启动首次触发时点、未闭合的模式／反射写者、标题深层绘制全部随机消费及完整世界前缀仍各自保留。此处已有字节依据否定“名字叫java.util就必是APK Java随机序列”的推论；不修改APK共同随机实现、黄金轨迹或现有快照身份，也不以相同bound或seed宣称两版逐抽等价。

## 静态来源范围

原批21个窗口现均覆盖完整登记范围；TitleForm.Update由2048字节前缀补至5024字节登记边界，正文至`1020D5C1`，其后至下一入口为填充。另沿具名Canvas输入查询／CheckInputGuard和GetJoystickIndex补核局部准入，未穷尽设备树或传递调用。TITLE_GUEST经cctor槽0x110F2100的fieldRef→私有数据字段→默认44字节blob独立得出0…10，不用APK代替；共同随机后端算法见上节，实际模式／流交接、完整框架准入及scratch历史资格仍未闭合。来源身份与数组证据沿本页既有入口维护，不另存重复指纹。
