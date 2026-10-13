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

SetDrawParam会写Character2共享临时字段及scratch `draw_param[5…10]`；Draw_monster（RVA `27D280`）还画条件状态／伤害／效果并在`1027D710–1027D739`写`effect_pos[0…1]`。本批已读其主体调用，但未穷尽这些特效子树及任意旧scratch残留，不能宣称整页绘制永远零随机。维护按现有Owner显式表现请求／独立消费方案保留上述顺序，纯基础皮肤只认证正常身体；不能为达到该前提创建实体或把共有显示monster请求当任务99／100的BD随机目录。

当前请求没有覆盖所有被读字段。显示怪物由`UserData.Setting`（RVA `2E89D0`）在`102EA2B2–102EA32A`创建Character2、调用Init(1,0,0)并将uid设−1；不是UserData cctor创建。Character2构造明确分配`draw_param`、`effect_pos`、空效果列表及6槽powerBar；Init绑定共同上下文／对象身份并初始化动作、朝向、flag和部分状态／计数（`1028418D–1028421D`），不重置上述效果列表和伤害计数。89及setter没有覆盖`uid_ +0x0C`（选择高亮／选中血条）、`powerBar_ +0x104`的第4槽（主动血条）、`effD_hitTime +0x114`／`effD_miss +0x118`（DrawEffDamage RVA `276BF0`入口）、效果列表`+0x19C`（DrawEffect RVA `277150`），也没有覆盖调试Draw_stateInfo／Draw_actInfo读取的状态／行动字段。因此最小后续问题是：**Setting后各个显示monster使用者是否只改当次已覆盖字段，能否证明这些附效在89准入时仍为空／未启用？** 不能把未追的共享历史补0当正常状态，也不能据此承诺当次显式请求就足以完整恢复。现Owner还没有此共享monster scratch载荷；先沿89这条具名链补闭包，再决定复用独立表现请求是否足够，不扩大成全世界actor重构。

后继最小调用者候选为UserData.Draw_bossAppear（RVA `2CE9E0`）、Draw_ma（`2D2820`）、SubForm._draw1_2（`332DB0`）和_draw2_4（`3472B0`，另对调用在VA `10349873/10349885`）；这里只按E8相对目标列出SetDispMonsterData／DrawDispMonster成对候选，尚未逐个认证完整指令边界和分派，不是穷尽写集。

资源复用现[全量索引](../assets/IMAGE_COVERAGE.json)：两版common图片3／39／72、SEB25／2，以及monster方向2的四个walk SEB及全部35张monster PNG在该索引中同字节。索引的Steam monster／common容器分别为resources.assets TextAsset1129／1148；这仅认证这些条目身份，不把整个common目录或所有显示效果都宣称同版本。图片索引为body×30+variant，方向SEB为body×4+2，沿正式img.inf／seb.inf检验缺图和帧界，不按PNG列数猜动作。

当前Owner生产者已有`monster_definition`，通用89确认使用`page_counters`实现早确认快进40／晚确认关闭；这是现有局部业务接线。89尚无专用副钟及共同SEB调度，不能称原公共两钟完整维护；其generic确认也未专门校验怪物定义，并以map下标访问主钟，缺定义的夹具仍可确认。后续需补明确身份和载荷验证：缺定义、孤立载荷、缺钟、错顶页或坏素材显式拒绝，并按本合同实现表现；这些是待维护要求，不能把现generic成功当已通过严格校验。完整恢复仍需先闭合共享scratch历史及表现请求身份，不能重新触发介绍脚本、introduced写入、随机抽样或奖励。原窗口的物理输入与最终画面未由本次静态分析取得。

## 新局与复发关系

`MonsterData.NewGame()` RVA0x21C8D0清state、NEW、四个计数字段及isAppearWindow；flags1定义才同时置state1／NEW和isAppearWindow=true。这与APK reset的结果相符，Steam不重复执行APK源码中的第二次清y语句，不要求指令数量一致。

APK的a/k.y所有具名直接写点已核为新局、介绍及读档恢复；复发任务选择在event60、40票号、任务章节及逆序目录筛选之外还使用该y。维护`monster_growth.introduced`由介绍消费者写回，Owner再投影到`TaskCreationMonster.replay_available`。这条APK及维护消费链已有证据；本批Steam只交叉到介绍资格产生，不将其扩大为Steam完整复发选择算法已相同。

APK精确写点（pc为16位code unit）：`La/k;::b()V`的pc6／16重复清y，flags1分支pc29写真；`La/k;::a(InputStream)V`的pc40安装读取的bool；`Lc/b;::k()I`的pc190在介绍脚本及raw89请求之后写真。后者完整195 code unit已核，选择器`Lc/n;::f(I)Lc/k;`的pc399读取y；开放p、介绍y和首次击杀不是同一时点。以上是固定DEX具名直接写范围，不将读取／序列化当新产生者。

现功能测试主责为`encounter_creation_test::creation/synchronous_intro`及任务选择套件：覆盖开放参与本次选择、介绍在spawn之前、活动任务抑制介绍、相同定义只介绍一次、晚失败回滚和复发空池回退。本批没有改业务或新增重复测试；这些是维护契约覆盖，不是Steam动态证据。真实经营到各怪物、BOSS胜利与完整任务复发交产品长跑验证。

后继[Steam任务选择器交叉](PROGRESSION_ROUTES.md#steam任务选择器交叉)已确认该isAppearWindow字段实际参与逆序复发候选；这是对上述局部边界的补充，目录初始化全值与原窗口自然路线仍未由此认证。

## 静态来源范围

原开放／介绍范围为MonsterData.NewGame（RVA0x21C8D0，160字节）、Character2.GetAppearMonsterDataId（0x27E850，1392字节）和共享ChangeState（0x2116C0，32字节）。本次另补raw89的SubForm Init／Init2、Update／Update2、_draw2_4局部分派及共用尾、软标签cctor条目，沿SetDispMonsterData／DrawDispMonster、SetDrawParam正常monster action0、Draw_monster主体及Setting／Character2构造／Init的上述具名区间交叉；不是将这些大方法全部分支认证。边界按最近更高具名入口核对，共享别名须结合字段／接收对象；GetMonsterUniqueId分配实例身份，不是定义选择器。显示scratch全部写者／特效子树、完整Steam任务选择、全部反射／外部写点、Random内部及原窗口连续经营仍未闭合。
