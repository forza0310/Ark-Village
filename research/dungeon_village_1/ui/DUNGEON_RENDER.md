# 迷宫攻略进度与路线表现

日期：2026-10-04。固定APK为1.0.8/versionCode9，见[证据](../EVIDENCE.md)。
[S032](../references/screenshots/2026-10-04-dungeon/README.md)和既有S012为独立视觉参考，
不认证同版、同一任务或连续帧。本页区分固定输入静态合同与下述维护桥，未改产品。
队伍推进、挑战、撤退、完成与奖励沿用[探索规则](../rules/ai/DUNGEONS.md)，不重复建模。

## 三组显示的来源

| 可见部分 | 固定APK入口 | 字段与边界 |
| --- | --- | --- |
| 野外攻略条/标签/百分比 | [c/n:2710](../work/decompiled/sources/c/n.java)活动任务提交类型14→[d/a:3855](../work/decompiled/sources/d/a.java)→[c/m:487](../work/decompiled/sources/c/m.java) | 当前全局任务绑定的实例；阶段1且g≥0画红条，g/h映射百分比，不是选中人物HP |
| 目标菱形 | [c/n:2721](../work/decompiled/sources/c/n.java)类型6独立命令 | 任务种类0、有绑定实例；普通cursor_rect02帧0，定义flags524288另用cursor_rect03；不与临时选中框混合 |
| 底部路线/头像/攻略百分比 | [b/c:987](../work/decompiled/sources/b/c.java)调用[c/n:4208](../work/decompiled/sources/c/n.java)的e(o) | 全局活动任务种类0；路线读挑战k与人物o/X/Z/aa，右端读实例g，不是人物等级/伤害/奖励栏 |

生成Java对数值helper有重载歧义，精确参数使用Tenant低层（历史中间文件，当前未保留）1689–1818交叉：
百分比传入long，实际调用[d/a:2784](../work/decompiled/sources/d/a.java)百分比helper，
不是同名2734的整数星数绘制。底栏经UserData低层（历史中间文件，当前未保留）13614–14165交叉，
包括真实画布裁剪、取当前任务实例及百分比调用；这些仍是静态证据，不是运行帧认证。

## 场景锚点、进度条与菱形

设D为任务格地图基准经过镜头转换后的地表锚点。
类型14深度D.y，调用锚点A=`(D.x+4,D.y-21)`；它不读取屏幕中任意洞穴的自行选中状态。
无绑定实例时只画cursor_rect02帧1，位置A+(25,4)，不画进度条；有实例才调用实例渲染。

实例渲染只在阶段1且g≥0时显示。令T为占地首格显示定义所用tenant图片的高度，V=38−T：

| 绘制部分 | 资源/源矩形 | 左上角或显示计算 |
| --- | --- | --- |
| 红条背景 | common图片8，dungeon_bar.png `(0,0,52,10)` | A+(0,V) |
| 红条填充 | 同图 `(1,10,width,8)` | A+(1,V+1)，width将g从0..h映射为0..50 |
| 百分比数字 | common SEB12/图片103，number05；SEB76/图片85帧0补% | g从0..h映射为0..100；helper输入右端在A+(60,V−12)，另一语言分支X再+4；数字实际右端先−9 |
| 攻略中标签 | common SEB66/图片65，cursor_rect02帧1 `(1,1,37,14)`，偏移(-18,-14) | 调用A+(12,V−1)，另一语言分支X−4；最终左上角再叠SEB偏移 |
| 普通目标菱形 | 同SEB帧0 `(38,1,56,29)`，偏移(2,0) | 调用D+(-2,0)，最终左上角为D；深度D.y+20 |
| 特殊目标轮廓 | common SEB100/图片154，cursor_rect03帧0 `(0,0,56,16)`，偏移(0,13) | 定义flags524288分支调用D，最终D+(0,13)，同深度；不是据S032认定该flags |

百分比使用原有界整数比例与向零截断；h=0时helper返回映射起点0，不能擅自除零。
图片宽度映射与百分比各算一次，显示50%不认证源g精确等于h/2，也不由截图反推h、更新频率或剩余秒数。
common图片、SEB行号与tenant资源组独立；高度T不是菱形高度，不能固定V或重复加设施图偏移。
类型14与菱形分别进入共享深度队列，不把整个标记无条件画在所有人物之上。
固定[c/a:249](../work/decompiled/sources/c/a.java)临时选中框另用cursor_rect00；本图橙黄任务轮廓与该来源分开。

## 底部探索路线

e(o)只要求有全局任务且任务种类0，没有额外阶段1守卫；不能用上方条的显示条件替代底栏资格。
设U为窗口边距，B=`(sceneLeft+sceneWidth)/2−127`，Y为[UserData.j(0):4569](../work/decompiled/sources/c/n.java)的底栏滑入位置。
外裁剪固定为场景底18高；活动任务将Y在前18个计数映射上移18，不将这18换成秒。

- 背景用common图片127的dungeonProgressBarBack.png；源Y=任务定义f70e×18，每行高18，从图片右侧截取能容纳的宽度。
  S032的洞穴样式不直接认证任务f70e/定义ID，不能根据一张缩放图选整张背景。
- 挑战按k逆序：阈值0..100映射到−8..153，再加B+16；宝箱以SEB27帧3/5区分阶段，
  奖励类别0用图片6、其余用图片97。状态3先在第一遍绘制，第二遍不重复画。
- 挑战怪物按定义种类/变体取monster图片和对应SEB帧0，另有阴影；状态1且年龄≥10会下降，
  并从3票号抽水平抖动−1/0/+1。这是绘制期随机调用，完整随机流复演必须包含它，不能仅记录逻辑更新抽选。
- 人物o逆序绘制头像，X映射到−8..153再加B；Z1撤退时aa的10..15映射成向上18像素。
  这是头像轨迹，不是场景n坐标或玩家直接控制人物行走。
- 显示特效aW另走common SEB93/图片141；阶段<4用帧0，其余帧1，不与人物EXP+或战斗数字共用队列。
- 右段先画menuRT02图片86，额外空白用其`(64,0,最多13,18)`分片平铺。
  “攻略”是SEB75/图片84帧0 `(0,0,59,14)`，不是普通系统字体；百分比数字为SEB10的number03，
  %为SEB76/图片85帧0 `(0,0,10,10)`。另一语言分支只将数字和%右移6，不猜截图平台。

右段百分比调用[c/m.a(int):67](../work/decompiled/sources/c/m.java)，取**当前全局任务实例h**为分母，输入为该实例g。
不用缓存i、任意人物X、怪物击杀比例或经过的时间替代。正常场景上方/底栏同源g/h，但布局和素材独立。
底栏若发现任务绑定实例丢失，会恢复任务格并清任务；这是绘制入口的实际副作用，不是仅隐藏UI或发奖励。

维护定义`StartupWorldTask.dungeon_background_row`现直接投影`questData.txt`零基第4列。
固定APK构造`a/m.java`给`f70e`赋此列，`c/n.java:4230`再取该值乘18作为源Y；
81条定义的行0…5计数分别48／10／3／8／6／6，与原PNG的217×108六行匹配。
这是不可变表现元数据，不保存到Owner，也不改变任务kind／探索阶段、页钟、随机或文件身份；
其它版本须独立核其定义与素材，不能从相同图片名推同一行。
当前仅补背景行的产品缺口，不代表完整底栏只读计划／滑入布局或原Draw准入已经交付；
后者仍需结合既有显式表现请求消费冻结抖动与缺绑定清理，不能在重复绘图时自行抽票或清任务。

### Steam底栏的独立静态交叉

2026-10-16，固定Steam2.56具名`UserData.DungeonBar_draw`（RVA`2DA2B0`至`2DB3A0`，4336字节）、
`GetOyBtmBar`（`2DC760`，288字节）及右侧铺片helper（`2D1D50`，320字节）均核完整登记范围。
本节是原DLL静态调用、字段与顺序证据，没有运行原游戏或认证素材像素／动态；不把APK行号直接套入Steam。

令`L/T/W/H=GameForm.VIEW_X/VIEW_Y/VIEW_W/VIEW_H`，U为当前`GameView.GetSideSoftWidth()`的返回值，
`R(v,a,b,c,d)`代表原有界整数比例`RateConvert(v,a,b,c,d,true)`。本方法的几何式如下：

| 量／请求 | Steam原式与顺序 |
| --- | --- |
| B | `trunc((L+W)/2)−127`；不是`L+W/2−127`，也不把W改成右边界 |
| 固定底边与外clip | 先写`UserData.DBAR_OY=T+H−18`，再`ClipRect(U,DBAR_OY,W−2*U,18,push=true)` |
| 活动任务滑入Y | `GetOyBtmBar(0)=T+H+R(cnt_dungeonBar_,0,18,0,−18)`；只检查全局任务存在、定义type0，不加阶段1门槛 |
| 其它底栏type | `type!=0 && isDispBtmBar`才再加`R(cntDispBtmBar,0,6,0,−22)`；这项不加到本探索条的type0 |
| 背景裁片 | 图片127宽Iw；实际C=`B+177−U`，在`(U,Y)`画源`(Iw−C,dungeonSearchBack_*18,C,18)`。该Steam入口虽调用ClampMax，却没有消费返回值，详见下文版本差异 |
| 人物内clip | 两遍挑战后再push`(U,Y,B+177−U,18)`；人物画完Pop一次 |
| 正常末尾 | 内clip Pop后依次画效果、右段与百分比，最后再Pop外clip；不得用“恢复全屏”替代两次各自恢复 |

`QuestData.type_ +24`、`dungeonSearchBack_ +28`分别消费任务种类和背景行；
`UserData.selectQuest_ +24`及其`Quest.dungeon_ +20`绑定当前实例。
**背景右裁宽有已证版本差异。** Steam的`102DA51D`保存原C，`102DA520`调用ClampMax(C,Iw)，
但`102DA525`立即用背景行覆盖返回寄存器，`102DA52B`重读原C交给DrawImage；
ClampMax（24A000，16字节）只返回较小值，不改实参或局部。因此不能把APK确实消费的`min(C,Iw)`套入Steam，
也不在本批替原代码补钳限。源矩形越界时Graphics后端如何处理仍未由这个调用者证明。
上述外clip先于实例缺失检查；缺绑定分支实际调用`Mapchip.DestructTenant(false)`、从任务容器移除该Quest、
`Quest.EndSelectQuest()`并提前返回，本方法内没有对应Pop。维护仍由既有Owner表现请求承接清理，
不能在只读计划里改世界，也不能把维护的整份验证／裁剪恢复保证登记成原方法已有finally。

**挑战两遍均倒序，垂直偏移不是逐条独立。** 六整数记录仍是阈值／类型／状态／年龄／第五／第六字段；
本Draw读取前五项，第六项未用于这条显示链。令`Xc=B+16+R(threshold,0,100,−8,153)`：

- 第一遍仅画类型0且状态3的宝箱，在`(Xc+7,Y+17)`请求common SEB27帧5；第五项0取图片6，否则图片97。
- 第二遍类型0状态3不重画且清dy=0；状态1取`dy=R(age,0,5,0,−18)`／帧3，状态2取dy0／帧5，
  其它状态取dy0／帧3；锚`(Xc+7,Y+17+dy)`，图片选择与第一遍一致。
- 第二遍类型1以第五项取MonsterData，图片索引`size_*30+sizeIndex_`，SEB取`faceIconSeb_`、帧0。
  状态1且age≥10时才请求`Random(3)−1`水平抖动，并更新dy=`R(age,10,40,0,18)`；否则抖动0且**沿用上一条dy**。
  第二遍开始前dy0，但它在循环外保留（VA`102DA720`、`102DA89E/8A3`），不能把普通怪物的dy总重置0。
  先在`(Xc−1+jitter,Y+dy)`请求`SubForm.Draw_icon(mode5,index0)`，再在同x、y+1绘怪物；不是深度队列排序。

人物使用实例`tenantUserList_ +44`倒序；`dungeonProgressEva_ +C0`对应APK X，
`dungeonState_ +C8`／`dungeonStateCnt_ +CC`对应Z／aa。
头像锚x=`B+R(X,0,100,−8,153)`、y=`Y+1+(Z==1 ? R(aa,10,15,0,−18) : 0)`。
先`Draw_charaBackRect(x,y,16,16)`，再`Draw_charaClip(x,y+1,当前职业性别图片,direct1,15,14)`；
其绿底16×16、灰边15×15、普通clip／ImageClip分离及清除语义沿已核[人物摘要helper](INFORMATION_MENU.md)。
本次另从实际调用复核：它传`SetDispPlayerData(action0,body,anime0,direct1,update0)`，人物身体锚为剪裁锚+(8,21)，
`DrawDispPlayer`二参数位置重载（2CD000）补weaponId=−1、animeIndex=0，**不是按成员aa选择行走帧**；
深层共享scratch历史资格仍沿[人物绘制边界](STEAM_TITLE_ACTORS.md)，不宣称任意历史均只有这一裁片。

内clip恢复后，`UserData.db_chargeEff static+114`（APK aW）倒序读`[phase,x,y,…]`：
common SEB93／图片141，phase<4帧0否则帧1，锚`(B+x,y)`，y已是显示坐标，不再叠Y。
Draw不在此推进／删除该列表；不能把它混入人物经验条。右段随后按以下式消费：

| 右段 | 原请求 |
| --- | --- |
| 底图与补白 | 在`(B+177,Y)`画图片86；E=`max(0,W−U−(图片86.width+B+177))`，再用`UserData.IMG_DATA`四槽铺到右边 |
| 补白每片 | 剩余r>0时片宽`Clamp(r,1,IMG_DATA[2])`，源`(IMG_DATA[0],IMG_DATA[1],片宽,IMG_DATA[3])`；每次前进片宽，末片不拉伸 |
| 标签 | K=`trunc(E/2)+3`；common SEB75／图片84帧0，锚`(B+177+K,Y+2)` |
| 数字和% | 日文数字右端`(B+177+K+47,Y+3)`、%锚x再+1／y=Y+6；非日文这两个x各再+6。数字helper为SEB10、padding0、anchor4，%为SEB76／图片85帧0 |

百分比原调用`Tenant.GetDungeonSearchRate(value)`（2C40D0）取当前实例`dungeonCnt_ +24`为value，
分母仍经全局选中Quest取得其实例`dungeonCntMax_ +28`，与APK g/h的归属一致，不能直接取人物进度。
Steam `IMG_DATA`现已独立核为`[64,0,13,18]`：UserData.cctor局部`102F46DE–102F4705`创建四槽int数组，
经`RuntimeHelpers.InitializeArray`填充后发布到static+150。实际句柄槽`110F3C8C`解到fieldRef205，
其typeIndex5607与类型4129的byvalTypeIndex一致；相对字段205→绝对字段25343，默认数据位于metadata`2FB4E2`，
16字节独立按小端解析为上述四值，并与原编译器数据字段名校验一致。这是独立交叉后与APK相同，未读写运行内存，
未复核无关cctor字段或把整段大构造器宣称为已完整解释。

`Draw_icon(5,0)`也已闭合：具名入口30C350在`1030C39D–C3A3`直接分流mode5，
`1030CDB8–CE16`消费common图片24，源`(index*18,0,18,18)`，目标坐标不再偏移，**不经过SEB**。
Steam自身common `img.inf`将24映射为`icon_back00.gif`（实际归档图为PNG），与[已核图标helper](STEAM_HUMAN_DETAIL.md)一致；
此处index0，因此是`(0,0,18,18)`的**图标背景垫片**，不能混成common3／SEB25的地面阴影。
上文APK“阴影”的概括在这条函数链中也应以同一mode5背景请求为准，不由观感另选shadow00。

**越界源矩形目前只闭合直接转交，未认证最终像素。** `Graphics.DrawImage(Image,dx,dy,sx,sy,w,h)`
（760C50，240字节）没有以图片宽高钳源；它把w/h同时作目标尺寸与源尺寸，于`10760D27`交`_drawImage`。
该具名后端的已核普通路径先处理独立ImageClip、翻转及可选优化缩放；无ImageClip、无翻转且`optimizePass_==0`时，
`1077AC73–ACD3`仍将原sx/sy/sw/sh递交`_drawBitmap`（778250），不是自动套APK的min宽。
本次只展开`_drawImage`至该普通转交点，及`_drawBitmap`至`107786FF`的入口／图集／特殊模式缓存前缀，
没有完成其后所有缓存、分块纹理和GPU提交；底栏127当前实际优化／图集资格也未动态取得。
因此越界最终是裁掉、透明、边缘延伸或其它效果仍不定论，产品不能据此把普通clip或ImageClip当作源界限修复。
Steam各图片／SEB实际裁片、中文资源及这条底层越界行为继续分别补证。
已核直接Draw中的抖动请求点`102DA870`；帧冻结与一次消费沿现表现请求，不由重复生成只读计划重新抽签。

## 完成、奖励与验收边界

场景进度条不负责发奖。g≥h由设施更新切阶段2并给成员错峰退出；任务收尾、经验显示/成长、成果页30/32、
完成量/延迟人气仍按[完成消费者](../rules/ai/DUNGEONS.md#completion-consumers)各自时序执行。
S032没有100%/退出/成果页，不能据这张图判断奖励已到手或点击底栏会结算。

本轮素材实际查看，SEB的legacy_tag保留原值且不与记录数比较；原PNG/源表未改。
后续需要同一任务身份的挑战/撤退/完成连续对照；固定APK触控、动画、随机流和完整结算动态未验收。
