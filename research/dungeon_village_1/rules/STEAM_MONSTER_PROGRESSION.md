# Steam普通怪物开放、介绍与任务资格

2026-10-10。固定Steam2.56两处怪物方法及共享状态helper静态交叉，共1584字节，[身份入口](../EVIDENCE.md)。这不是原动态或整套Steam任务选择器认证。APK具名字段写者合入下文，任务目录／选择器归[路线合同](PROGRESSION_ROUTES.md)，不要求研究自然经营到后期。

## 普通怪物按定义顺序开放

`Character2.GetAppearMonsterDataId()` RVA0x27E850先清临时候选Vector。随后沿原怪物定义顺序比较相邻两项`previous,next`，寻找同时满足以下条件的第一对：

- next的**怪物**flags不含4（BOSS），不是任务flags4。
- next.monsterRank≤当前UserData.questRank，即探索章节条件，不是村子星级。
- previous.state=1，next.state=0。

找到这对后检查previous.dieCnt_total≥8：满足才调用next.ChangeState(1)，随后结束这轮开放扫描；不足8也直接结束，不继续寻找后面可能达标的定义。因此不是一次批量开放所有满足阶段的怪物，也不是每八次全局胜利固定开放一个。代码读取的是该前一怪物定义的累计计数，和世界任务成功总数分开。

共享`ChangeState` RVA0x2116C0先写新state；只有旧0→新1时置NEW。IL2CPP把多种数据类的同实现合并到这个地址，反汇编打印为AsEventData或RankUpTerm只是同地址别名，不能据标签误判调用对象。本批同时核MonsterData具名别名及真实32字节状态写法。

## 选择池与介绍时点

开放扫描之后，逆原定义序收集state1、怪物flags不含4的项，最多5个，再调用一次`GameUtil.Random(candidate_count)`选择其一。刚开放的next参与本次候选；未介绍过不是入池条件，已介绍与未介绍项都可入池。本批只核这个调用位置与参数，未由此认证Random内部算法或原始seed。

选择后同时满足`!isAppearWindow && !flags1 && !UserData.IsSelectQuest()`才进行介绍：

1. execsScript非空时先实际调用定义脚本，popup=true、line0、talkCharaType2、talkCharaIndex为所选定义ID。
2. 创建SubForm类型89，monsterData绑定到该定义，并交AppData.Push。
3. 紧接着写isAppearWindow=true，返回所选定义ID。

**介绍资格在发出页面请求时就产生，不等待89确认，也不等待第一次击杀。** 正在进行所选任务时跳过介绍，不因此阻止选出怪物定义。原局部代码在Push返回后直接置标志，没有检查其bool返回；维护Owner的整体失败回滚是已有安全策略，不能反写为原程序也保证相同事务。

固定怪物列15有八个非空介绍程序，定义3／4／9／13／14／20／23／25分别调用talk125／126／129／130／127／131／132／128。它们的原对话记录默认speaker为0／−1，实际身份来自介绍外层的类型2／所选怪物，不能靠后面的raw89绑定代替。维护实际请求消费者仅给本次新增、没有指定说话者的对话补此身份；旧页及明确指定的说话者保留。两字段已有codec载荷，恢复保持保存值，不按原talk默认值重新初始化；没有新字段／schema或原表改写。八条、显式绑定、晚错回滚、页退休归任务套件，编码及严格恢复归现有codec套件；这是规则接线修正，不认证原Steam窗口皮肤。

字段映射与APK局部对应：

| 意义 | APK | Steam字段／偏移 |
| --- | --- | --- |
| 已开放状态 | a/k.p | BaseData.state_／0x10 |
| 介绍已发出 | a/k.y | MonsterData.isAppearWindow_／0x68 |
| 此怪物累计成长／击败计数 | a/k.v | MonsterData.dieCnt_total_／0x5C |
| 怪物需要的探索章节 | a/k.h | MonsterData.monsterRank_／0x30 |
| 当前探索章节 | c/n.x | UserData.questRank_／0x60 |

不能把state1当成isAppearWindow=true。它们的用途、产生时点及持久化字段不同。

## raw89介绍页的输入、绘制与共享请求

2026-10-14补核固定Steam `form.SubForm` 的Init／Update／Draw分派和显示怪物helper，与固定APK `b/g.java` 的89分支交叉；本节没有原窗口动态认证。载荷是怪物**定义**（Steam页字段`monsterData_ +0x198`），不需要生成世界怪物实例。前述介绍脚本、Push及introduced写入已在生产者完成，89确认不重复这些效果。

公共Init（VA `103125BE–103126F2`）先调用基类、绑定UserData、DecideKeyState、令主钟`frame_ +0xA0=0`并安装该类型软标签；随后进入Init2。89没有专用Init2分支，走成功尾`10312333–1031233B`。Steam cctor在`1032A777–1032A7BB`新建长度2的零int数组并直接放入目录89，故软标签为`{0,0}`，与APK `bH[89]`一致；不要补常驻“确定／返回”按钮。

Update2在`10317C7F–10317C82`分派89，局部`10318EAD–10318F44`只读确认脉冲mask`0x100000`，或DEBUG_CMD开启时的调试长按mask`0x40`。主钟小于40时确认只置40，满40再确认才走公共Pop `10320A3D`；没有取消、选择或奖励分支。公共Update在分派前先推进主／副钟（按INT_MAX回卷），处理等待、loopCounter、暂存SE并更新common SEB22／21／23（`10321B63–10321CF6`）。因此“89局部没有随机或PlaySe调用”不能扩写成整轮没有声音；待发公共声音和框架进出声音仍沿原共同消费者。

raw89绘制在`1034775E–10347761`分派至`1034AFD2–1034B395`，随后使用公共文字／箭头及Touch尾。调用顺序如下，坐标均是原240逻辑平面，不是窗口物理像素：

| 顺序 | 原请求 |
| --- | --- |
| 窗框／内框 | DrawWindow宽222、高170、top0；标题Steam字面`新種ﾓﾝｽﾀｰ`；DrawBox(15,139,224,202) |
| 背景 | common图片39 `mp_back.png`：锚(30,67)裁(0,0,93,82)、锚(109,67)裁(1,0,91,82)、锚(189,67)裁(1,0,20,82)，再用RGB148画Rect(30,67,179,81) |
| 阴影 | common SEB25／图片3，锚(120,133)，frame取怪物定义`shadowFrame_ +0x2C`，不固定为0 |
| 怪物 | 先写共享显示怪物体型及图片，再SetDispMonsterData(0,body×30+variant,(frame%16)/4,2,frame)，最后DrawDispMonster(g,120,132,外层步帧) |
| 两行文字 | clip(0,158,240,RateConvert(frame,5,40,0,48,true))；SC_WINDOW_BLACK；名字模板Steam为`<0>が`，锚(120,165)、anchor2；`出現しました!!`锚(120,187)、anchor2；末PopClip |
| 继续箭头／触摸 | frame≥40才Draw_btmCursor(frame−40)；锚(212,204)，余数`(frame−40)%25<15`时画common SEB2／图片72；公共尾`103477DA–10347824`注册Touch id2、option2 |

Steam metadata三个字面槽分别为`110F745C`（标题）、`110EA494`（名字模板）、`110F5604`（第二行）；具名类型及literal独立解码，不能按APK文字代填。APK89绘制（`b/g.java`约8322–8347）使用“新型怪物”、模板`<0>`及“出现了!!”，其余已列局部坐标／门槛同义。中文维护皮肤使用已核汉化APK字面属于研究适配，不认证Steam中文翻译。

**共享显示调用不是纯裁片函数。** `UserData.dispMonster_`为static+0x18，区别于人物scratch +0x14。页先写其`objSize_ +0x14`和`bodyImg_ +0x18`；SetDispMonsterData（RVA `2E6AE0`，`102E6B19–102E6B7E`）只写`seb_/bodyImg_/animeIndex_/direct_/sebCnt_`五项，不清其它历史状态。DrawDispMonster（RVA `2CCD70`，`102CCDA9–102CCDF6`）不读取外层animeIndex，先以`seb×3`调用SetDrawParam再Draw_monster；正常monster action0会重新算身体SEB为`body×4+2`、帧为`(frame%(speed×4))/speed`。Steam cctor `10294567–102945E8`独立给出speed `[6,6,8,10]`，不能把外层四槽步帧直接裁PNG。

SetDrawParam会写Character2共享临时字段及scratch `draw_param[5…10]`；Draw_monster（RVA `27D280`）在`1027D710–1027D739`写`effect_pos[0…1]`。它还包含选中高亮、血条、伤害、效果和调试显示；本节基础皮肤的充分条件在下文明确，不能只凭setter的五项覆盖宣称任意旧显示状态相同。

显示怪物由`UserData.Setting`（RVA `2E89D0`）在`102EA2B2–102EA32A`创建独立Character2、调用Init(1,0,0)并将uid设−1；不是UserData cctor创建，也不加入世界人物／怪物容器。构造明确分配`draw_param`、`effect_pos`、空效果列表及6槽powerBar；新标量和int数组槽按语言语义置零。Init绑定共同上下文／对象身份并初始化动作、朝向、flag和部分状态／计数（`1028418D–1028421D`）。89及setter不清uid、powerBar、伤害计数、miss或效果列表，但以下已核正常显示使用者也不改这些槽。

2026-10-14续核从以下四个具名方法入口顺序解码至实际调用边界，已把上一批E8候选收为真实指令，局部直接写集如下；不把顺序解码代替全程序指针别名证明。

| 调用者／来源 | 本局部共享显示写入及消费 |
| --- | --- |
| UserData.Draw_bossAppear，RVA `2CE9E0` | `102CEF90–102CEFB8`写size／bodyImg；`102CF00C/102CF021`依次setter(action0)／draw |
| UserData.Draw_ma，RVA `2D2820` | `102D2CE8–102D2D16`写size／bodyImg；`102D2D3B/102D2D4D`依次setter(action0)／draw |
| SubForm._draw1_2，RVA `332DB0`的raw33期限分支 | `10332FEB–10332FEE`将33送至`10336228`；`103370AC–103370D4`写size／bodyImg；`103370FD/1033710E`依次setter(action0)／draw |
| SubForm._draw2_4，RVA `3472B0`的raw92怪物信息分支 | `10347779–1034777C`将92送至`1034937C`；`10349820–1034984B`写size／bodyImg；`10349873/10349885`依次setter(action0)／draw |

四处经相同SetDrawParam和Draw_monster消费；正常monster action0的SetDrawParam只做数组查询、算帧及上述派生写入，不向世界AI／战斗／任务更新传出scratch。Draw_monster把scratch传给血条、伤害、效果及调试helper，但以下准入会跳过相关绘制和写入，身体的AppData.DrawSeb只收到图像／SEB／帧／坐标，不收到Character2。这闭合的是正常显示链，不涉及世界怪物实例更新。

| 正常基础皮肤的充分条件 | 来源与保持关系 |
| --- | --- |
| 已完成Setting的独立显示初始化；`uid=-1`、powerBar[4]为0、hitTime为0、effects为空 | 构造／Init／Setting如上；setter及四个显示调用者不写这些槽。DrawEffDamage在`10276C2B–10276C33`读`effD_hitTime +0x114`，≤0直接返回`10276F86`，因此不读miss或画伤害；DrawEffect在`1027720D–10277231`读`+0x19C`长度，空列表直接返回`1027ADAF` |
| 相机仅选合法世界对象，或没有选中对象 | Camera.Collision（RVA `29DB60`）从AppData的人物／怪物Vector建命中名单，`1029E624–1029E655`按objType绑定subPlayer／subMonster；不会加入独立显示scratch。GetHumanUniqueId（`27FB40`）／GetMonsterUniqueId（`281AF0`）从0找最小未用身份，在正常有限世界规模内非负，因此不匹配scratch的−1。Draw_monster的`1027D2D5–1027D37F`高亮及`1027D575–1027D5FC`选中血条分支不成立；powerBar[4]为0也跳主动血条 |
| 普通根游戏的非调试显示，Map.DISP_ENEMYAREA为0 | metadata槽`110FBACC`独立解析为game.Map，static+0x20；Map.cctor `102BAF46`明确置0。Draw_monster在`1027D61D–1027D624`仅值恰1才调用Draw_stateInfo／Draw_actInfo，不能把它误称Config.DEBUG直接准入 |

发行配置须区分基类默认与游戏覆盖：基础库Config.cctor先令DEBUG=true（`1083D964`），DEBUG_CMD从DEBUG复制（`1083DCC2`）；游戏`cfg.MyConfig.cctor`却令DEBUG=false（`10228D49`）并据此令DEBUG_CMD=false（`10228F14–10228F1B`）。IApplication.Awake在`107815FE–1078169C`查找Config派生类型并调用Config.Create；后者通过同名字段GetValue→基类GetField→SetValue复制（`1083B66C–1083B6D3`）。根应用正常复制这些游戏值；非根应用会跳过含DEBUG／DEBUG_CMD的SKIP_OVERRIDE_STATIC_FIELD_NAMES，因此本合同不承诺子应用配置。GameForm.Update在`1030156C–10301573`要求DEBUG_CMD后，才允许CheckKeyRepeat(mask8)在`10301608–10301638`以1−旧值切换DISP_ENEMYAREA；正常根游戏的玩家输入不进入这条调试切换。这里是固定发行静态启动链，未读取用户运行期配置。

在上述初始化、合法相机对象、正常根游戏非调试及资源有效的前提内，本次显示只改可重新计算的绘制参数和effect_pos，**不消耗游戏随机、不发声音、不改uid／血条／伤害／效果列表，也不创建世界实体**。图像／SEB／帧的纯基础计划可以独立消费，不必为这些派生缓存新增Owner持久字段；这是已证正常链的维护策略，不是把未知附效补0。原公共Update待发SE和框架进出声音仍沿前文消费者，不能从本次Draw零声音推成整轮静音。调试态、损坏／外部改写的身份和附效状态不在该纯计划范围，不能静默回退为普通身体。

资源复用现[全量索引](../assets/IMAGE_COVERAGE.json)：两版common图片3／39／72、SEB25／2，以及monster方向2的四个walk SEB及全部35张monster PNG在该索引中同字节。索引的Steam monster／common容器分别为resources.assets TextAsset1129／1148；这仅认证这些条目身份，不把整个common目录或所有显示效果都宣称同版本。图片索引为body×30+variant，方向SEB为body×4+2，沿正式img.inf／seb.inf检验缺图和帧界，不按PNG列数猜动作。

当前Owner生产者绑定并校验`monster_definition`，89仅在真实栈顶首次准入时建立现有`page_counters`；暂停或被覆盖的pending页不提前Init。统一validator拒绝缺定义、重复／未知身份、ready缺钟、非法计数及误挂phase／副钟，框架在清理关闭页之前也校验；恢复入口复用该规则。确认只接就绪栈顶，早确认快进40、后次关闭，不重触发介绍脚本、introduced、随机抽样或奖励；主钟沿已核INT_MAX回卷，关闭后下一框架入口共同退休页及主钟。这些是当前单主钟业务与维护拒绝策略，不是原程序损坏档案的安全保证。

研究[有序皮肤](../prototype/include/dungeon_village_prototype/steam_monster_skin.hpp)及实现`steam_monster_skin.cpp`按上述正常前提只读生成背景、阴影、身体、文字、箭头和Touch2请求；Owner桥严格读真实就绪栈顶89、定义与现主钟，缺图／坏帧拒绝，不修改scratch或世界随机。研究窗口已接经营中的89以及早期／就绪两种例图，本轮visuals短测及两种8帧窗口通过；这是维护绘制／接线验证，不是原Steam动态认证。当前仍没有原公共副钟／SEB更新、完整原字体和物理Touch2动态证据；这些独立补证，不阻碍正常只读基础皮肤。[两态例图](../ui/examples/steam-monster-introduction.png)与接口已供产品消费，后继接下述92，不再扩大扫描世界actor或新增共享显示存档。

## raw92怪物信息：共享定义入口与局部皮肤

Steam `GameForm._update`在VA `103052F3–10305305`接确认脉冲`0x100000`，按已选人物→怪物→设施顺序处理。怪物分支`10305985–103059F7`创建92，由`Character2.GetMonsterData()`取共享定义写入页`+0x198`后Push；不是冻结当前怪物实例。APK `b/c.java`约1600行同为选中实例→定义→`g(92).ah`。公共Init归零主钟；Init2没有92专支，cctor `1032A85B–1032A8B0`给软标签`{0,2}`。Update在`10318645–1031865D`确认立即转Pop，另在`1031F640–1031F680`接受soft2；无本页等待门槛、目录／滚动、奖励或随机，公共Update／框架声音仍独立。

Draw分派`10347779–1034777C→1034937C`，取定义name`+1C`、flags`+0C`、body／variant／shadow`+20/+28/+2C`、description`+50`、累计击杀`+5C`与倒下人数`+64`。普通显示Lv按累计击杀100／200／300／400分1–5档；BOSS为`min(count+1,10)`。`GetExp` RVA21C580和`GetDropMoney` RVA21C4D0分别从基础`+40/+4C`按flags4选普通／BOSS奖励倍率，沿[已核奖励合同](ai/ENCOUNTERS.md)，不把Lv档数直接当收益倍率。这些具名helper不消费随机或写共享进度。

窗220×168、Box(17,74,219,184)，名字(26,69)，Lv为common38及SEB12，common39地板(23,89)，定义shadow帧在(70,158)，身体(70,157)面向1并沿原speed数组计步，经验(42,178)／数值(95,177)，掉钱图标(147,189)／数值(200,177)，说明(120,198)。Steam字面独立解码：标题槽110F3EC0为“モンスター情報”、110F7190为“支給”，110F4DB0／110F4D5C为“倒した数…”／“倒された人数…”。本节中文仅为说明，维护中文显示仍须注明汉化APK适配。

非JP布局先将两条LT标签按空格分段，只有**两条都至少两段**（`10349906–10349924`）才临时用字号10：击杀文字(125,98)/(125,112)、count3(210,104)，倒下文字(125,132)/(125,146)、count4(210,138)，随后ResetFont。分段不足则走JP后备`10349AD5–10349BA7`：两标签分别(125,95)/(125,129)，计数(210,110)/(210,144)。APK非JP直接索引前两段，没有此后备，不能为两版统一去掉Steam资格检查。92与89复用共享显示对象，但方向／统计布局／退出门槛独立；当前只完成上述局部合同，尚无92维护目录入口、皮肤或原窗口动态认证。

## 新局与复发关系

`MonsterData.NewGame()` RVA0x21C8D0清state、NEW、四个计数字段及isAppearWindow；flags1定义才同时置state1／NEW和isAppearWindow=true。这与APK reset的结果相符，Steam不重复执行APK源码中的第二次清y语句，不要求指令数量一致。

APK的a/k.y所有具名直接写点已核为新局、介绍及读档恢复；复发任务选择在event60、40票号、任务章节及逆序目录筛选之外还使用该y。维护`monster_growth.introduced`由介绍消费者写回，Owner再投影到`TaskCreationMonster.replay_available`。这条APK及维护消费链已有证据；本批Steam只交叉到介绍资格产生，不将其扩大为Steam完整复发选择算法已相同。

APK精确写点（pc为16位code unit）：`La/k;::b()V`的pc6／16重复清y，flags1分支pc29写真；`La/k;::a(InputStream)V`的pc40安装读取的bool；`Lc/b;::k()I`的pc190在介绍脚本及raw89请求之后写真。后者完整195 code unit已核，选择器`Lc/n;::f(I)Lc/k;`的pc399读取y；开放p、介绍y和首次击杀不是同一时点。以上是固定DEX具名直接写范围，不将读取／序列化当新产生者。

现功能测试主责为`encounter_creation_test::creation/synchronous_intro`及任务选择套件：覆盖开放参与本次选择、介绍在spawn之前、活动任务抑制介绍、相同定义只介绍一次、晚失败回滚和复发空池回退。本批没有改业务或新增重复测试；这些是维护契约覆盖，不是Steam动态证据。真实经营到各怪物、BOSS胜利与完整任务复发交产品长跑验证。

后继[Steam任务选择器交叉](PROGRESSION_ROUTES.md#steam任务选择器交叉)已确认该isAppearWindow字段实际参与逆序复发候选；这是对上述局部边界的补充，目录初始化全值与原窗口自然路线仍未由此认证。

## 静态来源范围

原开放／介绍范围为MonsterData.NewGame（RVA0x21C8D0，160字节）、Character2.GetAppearMonsterDataId（0x27E850，1392字节）和共享ChangeState（0x2116C0，32字节）。本次另补raw89的SubForm Init／Init2、Update／Update2、_draw2_4局部分派及共用尾、软标签cctor条目，沿SetDispMonsterData／DrawDispMonster、SetDrawParam正常monster action0、Draw_monster主体及Setting／Character2构造／Init的上述具名区间交叉；不是将这些大方法全部分支认证。边界按最近更高具名入口核对，共享别名须结合字段／接收对象；GetMonsterUniqueId分配实例身份，不是定义选择器。显示scratch全部写者／特效子树、完整Steam任务选择、全部反射／外部写点、Random内部及原窗口连续经营仍未闭合。
