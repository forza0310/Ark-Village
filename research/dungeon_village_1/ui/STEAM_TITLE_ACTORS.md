# Steam标题人物调度与基础绘制

2026-10-09。固定Steam2.56 `GameAssembly.dll`及metadata的具名方法静态交叉，补充[标题Draw](STEAM_TITLE_DRAW.md)。[证据清单](../work/steam-title-actors/EVIDENCE.json)和[复查方式](../work/steam-title-actors/README.md)保留来源身份、入口、字节范围与摘要；未启动游戏、读写内存或原档。APK合同见[标题调度](TITLE_PRESENTATION.md)与[人物皮肤](TITLE_ACTOR_SKIN.md)，两种来源分别认证。

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

Steam边缘参数：x<-14时`srcRatio=abs(-18-x)*64`；x>250时`srcRatio=(254-x)*64`。正常活跃范围内，边界−18/254为0，−17/253为64，−16/252为128，−15/251为192；中央区不主动设置边缘模式。外层先`SetRenderMode(1,srcRatio)`，再画该人物的阴影与人物助手，末尾调用无参`SetRenderMode()`。

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

`TitleForm.Update`固定前2048字节证明：父更新计数回卷后，titleFrame饱和递增、frame回卷；更新后的titleFrame<100时先查确认脉冲，可跳到100。仍<100则跳往后部结束；达到100后调用背景更新，之后才处理调试长按、指示SEB及菜单输入。背景调用点VA`0x1020C4A7`。

确认键mask`0x100000`不走JoystickIndex分支；无输入guard且可用Keypad时，`CheckKeyPulse`读取并清除该bit。故旧titleFrame=98时，同一确认脉冲可用于跳动画并在本轮更新人物，但不会再次供菜单确认；旧值=99自然加到100，不经过提前消费，后续确认仍可能命中。这里只认证这个方法及同一mask消费关系；本批没有追完Steam表单框架的栈顶／生命周期准入、guard状态变化、触摸映射或实际键位，不把它写成所有输入设备的完整合同。

## 排序、资源查询与图层

`Draw_titleAnime`（RVA`0x20A710`，完整1600字节）先把全部20槽的`(slot,y)`写入临时排序表，包含inactive槽；外层i递增、内层从末尾向i+1递减，仅前y>后y时交换。排序后才跳过inactive。它是非稳定交换排序，不能替换为`stable_sort`再宣称同深度等价。

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

`DrawDispPlayer`先调用`SetDrawParam`，当共享显示人物`objType_==0`且weaponId!=-1时画武器，最后画身体。因此主路径图层为**阴影→有条件武器→身体**。没有读取防具或饰品另叠衣服；这不代表其它人物效果没有额外层。

`SetDrawParam`的正常人类、action0、`haveObj_==-1`分支：身体SEB取`HUMAN_ANIME_SEB[0]+direct_`，帧取`HUMAN_ANIME_SEB_F[0][step]`，图片取bodyImg；身体偏移从`body_off[0]`取得。后续[数组与资源专题](../work/steam-actor-arrays/README.md)已独立闭合此分支的Steam常量与裁片，见下节。

`Draw_weapon`沿当前武器定义的`chargeType_`选`WEAPON_ANIME_F[chargeType][0][step]`，SEB取`WeaponData.WEAPON_SEB[chargeType]+direct`，图片取`weapon.imgId_`，锚点加`GetWeponOffset(0,chargeType,direct,step)`。标题传`zsort=false`，直接DrawSeb，不走世界绘制队列。action0不会进入action1／5的斩击光分支。`GetWeaponAnimeIndex(0)`已核返回0；完整`GetWeponOffset`证明action0选择`WEAPON_WALK_POS[type][direct][step]`，没有额外插值或随机。

## Steam数组、SEB与图片交叉闭合

2026-10-09续批：[独立证据](../work/steam-actor-arrays/EVIDENCE.json)。固定两具名cctor在成功分配／类型检查路径沿显式常量及真实fieldRef解读，目标字段全部发布后停止；Character2前10,513字节、WeaponData前25,697字节。未知值、指令或控制流显式失败；没有执行游戏代码，没有用APK常量补洞。三个编译器小helper由原调用点追到真实入口并分别核≤128字节的成功路径。

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

`SetDispPlayerData`只改共享显示人物的`seb_/bodyImg_/animeIndex_/direct_/sebCnt_`五项。`SetDrawParam`还写共享临时字段和`draw_param[5…10]`；`Draw_human`结尾写旧画面坐标。它们不是严格纯函数。

身体绘制仍包含相机选中、体力条、携物、倒地、调试、伤害和效果分支；`haveObj_`残留还会把行走SEB改用`HUMAN_ANIME_SEB[8]`。已读完整`Draw_human`不直接调SetRenderMode或Random，但它继续调用`DrawEffDamage/DrawEffect`等，本批未闭合其所有子树与共享状态初始化。所以不能宣称整个标题绘制无随机、任何历史进入标题都只有三层，或淡出必然不被深层效果覆盖。

本次交付可用于Steam模式的后续设计与差分断言，尚未增加C++切源策略、Owner字段或快照schema，也未修改已认证APK标题模式。正常人物数组和所需SEB／PNG身份现已独立核实；后续追共同显示人物初始化／残留资格、框架更新准入及JRandom交接，再在独立Steam表现模式验边缘与随机差异。窗口观测可交叉人物淡入／淡出与遮挡，但截图不能单独证明内部槽、抽数或逻辑tick。
