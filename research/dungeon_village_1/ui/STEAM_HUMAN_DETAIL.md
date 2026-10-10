# Steam人物详情60：四页布局与输入

后继：[外层更新与触摸补证](STEAM_HUMAN_INPUT.md)已确认普通Update／Update2翻页链不按页重配label11；本页原“外层Update尚未闭合”属于初批边界。完整平台覆盖仍未验，不扩大成任何外部回调均不改标签。

2026-10-10。固定Steam DLL、metadata身份及字节锚见[研究包](../verification/steam-human-detail/README.md)和[证据清单](../verification/steam-human-detail/EVIDENCE.json)。本页只认证具名方法中的局部静态消费者；没有本轮原窗口截图或完整皮肤像素验收。既有[人物UI示例](examples/README.md)是APK示意，不能反过来证明Steam坐标或文字。

## 范围与所有权

`SubForm._draw2_1`中`type==60`进入`0x1033D214`，局部绘制止于`0x1033F02C`。四页共用上半区域，`page_`为0、1、2、3；并非四个独立子窗口。窗口为`DrawWindow(220,175,0,"冒険者情報")`，内容框`DrawBox(17,52,219,190)`。坐标是原逻辑绘制空间，不能直接当桌面客户区像素。

- `charaData_ +0x104`指向共享`CharacterData`：姓名、当前职业、等级经验、六属性、四战斗缓存、当前装备、住宅状态及魔法。它不是按截图临时拼装的预览对象。
- 绘制每次通过`GetCharaInTownFromCharaInfoWindow`查找实际`Character2`。该helper顺序遍历`AppData +0xF0`列表，比对各实例的`GetCharacterData().id_`与目标定义ID，返回第一个匹配或空；不能把`chara_ +0x100`直接当本页唯一实例。
- 当前HP／最大HP和倒下状态来自实际实例；没有实例仍可绘制人物定义、装备、职业和属性。概览四值读取`equipParam_ +0x5C`，其中HP不是当前受伤HP。
- `Init2`先检查/触发教程111，再在`charaInfoChase_==0`时改软标签，最后调用该定义`UpdateBattleParam()`。初始化并非纯展示；不能将重算挪入每帧只读维护皮肤。
- 已证场景住宅入口见[设施74合同](STEAM_FACILITY_DETAIL.md)：已入住住宅打开60，并设置可追踪入口。此次没有认证所有人物目录／场景入口和父栈组合。

## 共用上半区域

| 项目 | 静态参数与消费者 |
|---|---|
| 名称 | `GetName()`，`(124,71)`、anchor=1，窗口棕色 |
| 职业底色 | `(119,84,91,25)` RGB(215,249,214)，下条`(119,109,91,13)` RGB(172,235,169) |
| 职业名 | 当前`jobId_`取职业定义`name_`；日文`(123,87)`；非日文进入53宽度阈值及字号9／8／7分支，见下文 |
| 等级 | image38 `wnd_lv`：非日文`(177,88)`、日文`(172,88)`；非大师SEB12数字，非日文右锚`(208,88)`、日文`(206,88)`；等级10显示image129 MAX，分别`(195,90)`／`(191,90)` |
| 经验底条 | image87 `wnd_expBar`，源`(1,0,81,5)`→`(123,102)` |
| 经验填充 | 源`(2,5,width,3)`→`(124,103)`；`width=RateConvert(exp,0,GetNextLvUpExp(currentLv,currentJob),0,79,true)`，等级10强制79 |
| 经验标签 | SEB60/image50 frame0锚`(130,112)`；image88 `wnd_ato`非日文`(149,113)`、日文`(148,112)` |
| 尚需经验 | 非等级10才画`nextExp-exp`，SEB15、右锚`(201,110)`；这不是总经验 |
| 人物底图 | image39 `mp_back`源`(0,5,90,57)`→`(25,69)`，再RGB(148,148,148)框`(25,69,89,56)` |
| 住宅与奖章 | `myHouse_[2]==1`叠image94 `myHomeBack(27,82)`；`Draw_medal(83,70,medalNum_)` |

非日文职业名先以原Font测宽，默认size9、y=88；宽≥53时先选择size8、y=89，再次用该Font测宽，仍≥53时改size7；之后才调用`SetFontSize`，绘制后`ResetFontSize`。这里记录原调用顺序，不擅自替换为“逐字号试排直到适配”。字体平台实际字宽与[窗口字体合同](STEAM_WINDOW_FRAME.md)分别核验。

普通人物交给`Draw_charaInfoChara(g,liveOrNull,definition,frame_)`。实际实例`state_==7`时改走倒下展示：`GetImgId(definition)`；`SetDispPlayerData(7,body,0,1,0)`；简版`DrawDispPlayer(75,121)`；`Draw_fukidashi("ﾀﾞｳﾝ中",75,79)`。

## 四页与底部说明

### 页0：概览

左下底块`(25,153,89,36)`、RGB(193,238,247)。满意文字`(30,157)`、SEB12右锚数值`(108,158)`读取`niconicoValue_`；笑脸SEB84/image115 frame0锚`(68,169)`。努力文字通过`TextLayout(30,175,60,11,anchor=0x20)`，数值SEB12右锚`(108,176)`读取`putUp_`。

四个当前装备图标在`(32+18*i,129)`，顺序武器、防具1、防具2、饰品。武器用`GetWeapon().imgId_`、mode2；防具用`GetArmour()[i-1].imgId_`、mode3；饰品用`GetAccessory().imgId_`、mode4。防具／饰品空时用mode5/index7占位，不把缺装备变成缺定义。

右侧四战斗值顺序来自静态标签`ＨＰ/攻撃/防御/魔法`，第i行y=`123+18*i`。框`(119,y,47,18)`和`(166,y,40,18)`、RGB(173,209,221)；mode8/index i图标锚`(128,y+2)`；SEB15数字右锚`(203,y+5)`读取`equipParam_[i]`。当前实例HP另由人物下方power-bar helper显示，不用当前HP覆盖这里的缓存最大HP。

底部默认“プレゼント”，`Draw_btmMsg(text,202,true,true,200)`。仅当存在实际实例、持有回复道具且`GetHp()<trunc(GetHpMax()/2)`或该实例state7时，改“回復ｱｲﾃﾑをﾌﾟﾚｾﾞﾝﾄ”。括号关系为`live && hasRecovery && (hp<half || down)`。

### 页1：六属性

顺序为`体力/力/器用/丈夫/魔力/運`，分别读取`baseParam_[0..5]`，不是战斗缓存，也不在绘制中另加装备／职业值。i对应`x=29+92*trunc(i/3)`、`y=134+19*(i%3)`，先填`(x-4,y-2,93,20)` RGB(208,242,249)，再框`(x-4,y-2,92,19)` RGB(149,206,217)。

mode7/index i图标在`(x,y)`，标签`(x+18,y+2)`，SEB15数值右锚`(x+83,y+3)`。index5特殊取image37的第6列，不可机械取第5列。底部“転職する”，选中和指针均true。

### 页2：四装备与选中装备加值

四行依次武器、防具1、防具2、饰品；`y=128+17*i`，填`(25,y,185,18)`、框`(25,y,184,17)`，颜色同六属性。标题非日文以size10 `TextLayout(29,y+3,45,12,anchor=0x20)`；日文`DrawString(37,y+3)`。装备图标锚`(79,y)`，正文从x112起、宽90；武器y+2高12，防具／饰品y+3高11，TextLayout anchor0x20。后两类空则画占位及“装備なし”。

每行绘制还注册`TouchComponent id10`，原矩形`(5,y,225,18)`、value=`0x20000|i`、Margin(left0,right−20,top0,bottom0)。不能只实现PNG而遗漏触摸行，也不能未核通用输入转换便声称单击一定直接购买。

选中指针SEB21当前帧，非英语x24、英语x21，y=`138+17*select_`。底部展示该**装备定义**的加值：武器取`param_[1]`／`param_[3]`并画mode8/index1、3图标；防具／饰品取`param_[0]`／`param_[2]`，图标index0、2。图标锚`(31,202)`和`(132,202)`，正数才分别`Draw_plusValue(108,205,value,12)`与`(209,205,value,12)`。两值均−1或所选防具／饰品不存在时改为“<btn=0,0>でプレゼントできます”，非选中、无指针。零和负数不是自动“+0”，也不画负数加值。

### 页3：已学魔法

标题“習得魔法”中心锚`(120,134)`。四项`火の魔法/氷の魔法/雷の魔法/回復魔法`；i对应`x=29+92*trunc(i/2)`、`y=153+19*(i%2)`，背景矩形和颜色同六属性。读取`magic_[i]`，未学显示`---`、中心锚`(x+42,y+8)`；已学mode14/index i（i=3改index4），正文日文`(x+18,y+2)`、非日文size9 `TextLayout(x+18,y+3,65,9,anchor0x20)`。

底部统计整个`magic_`布尔数组的true个数，直接索引五项说明：0为“特定の職業をﾏｽﾀｰすると習得”，经`Draw_btmMsg(202,false,false,200)`；1–4为“１つ…/２つ…/３つ…/あらゆる魔法を使えます”，中心锚`(120,204)`。魔法页确认在本局部不推进子页。

四页末尾都调用`DrawVerticalScroll2(221,69,4,127,page_,3,1)`。该helper[已证只注册触摸组件而不画图元](STEAM_FACILITY_DRAW_HELPERS.md)，不能凭函数名补造可见滚动条。

## 输入、软标签与初始化副作用

`Update2` raw60分支完整局部`0x1031E06F–0x1031E63C`：先把`scroll_`同步到`page_`，末尾将page写回scroll。键值上`0x20000`、下`0x80000`使用repeat，确认`0x100000`使用pulse；上、下检查不是互斥else分支。

- 页0/1/3，上下分别`(page+3)%4`／`(page+1)%4`。进入页2时，从页1进入选择装备0，从页3进入选择装备3。scroll同步导致进入页2时也做同样选择修正。
- 页2，上在选择>0时减1、选择0时转页1；下在选择<3时加1、选择3时转页3。因此装备列表方向先走四槽，不能每次方向都翻页。此分支没有读取左右键。
- 确认在页0开赠礼64；满足前述回复条件时设子页`page_=4`并按已持有的回复道具前缀选择首项。页1检查/触发教程112后开职业目录61。页2开赠礼64并将所选装备槽传给子页page。页3局部直接返回未处理。
- 确认路径还存在平台静态布尔`+0x83`与`CheckKeyState(0x40)`替代入口，当前只登记指令，未将该字段命名为某个具体鼠标按钮。
- 软标签2命中Pop；软标签11走教程112／职业61；软标签8仅`charaInfoChase_==1`时调用`ProcChaseCommandFromInfoWindow(definition)`。这里的追踪不等于玩家直接移动冒险者。

SubForm静态表60默认`[8,2]`，AppData标签8原文字“追跡”、2“戻る”；Init2在`charaInfoChase_==0`改为`[0,2]`（0为空）。标签11原文字“転職”。此次确认了初始表与局部输入监听，**外层Update是否逐页重新配置标签11尚未闭合**，不能声明所有页实际左软标签始终“追跡”或始终“転職”。

## 图像映射与表现副作用

[EVIDENCE](../verification/steam-human-detail/EVIDENCE.json)登记25个common逻辑引用：18图＋7SEB，包括SEB派生图，限定本批出版白名单有24个字节匹配路径；image88 `wnd_ato.png`（Steam 10×7）不在该历史白名单。其后已由[Steam人物素材包](../assets/steam-human-common/README.md)独立发布，当前不再缺此图；原静态清单不随新包增多改写历史匹配数。这是本批直接common分母，不是完整人物皮肤素材总数；窗口、字体、奖章、气泡、power-bar及动态人物／武器资源仍有委托边界。

| Draw_icon模式 | 本页用途与资源 |
|---|---|
| 2 | 武器：common12 `icon_weapon00`，18×18格；common24 `icon_back00`先垫底 |
| 3 | 防具：common20 `icon_armour00`，18×18格；同样先垫common24 |
| 4 | 饰品：common21 `icon_accessry00`，18×18格；同样先垫common24 |
| 5/index7 | common24的18×18空槽裁片 |
| 7 | Steam common37 `icon_param00`，y16、16×16；index5改x96 |
| 8 | SEB98/image37按传入frame绘制，不用mode7裁片代替 |
| 14 | common118 `icon_magicpot`，16×16格，回复魔法用index4 |

装备图标编号的十列取模／整除来自`imgId_`，不是装备定义ID；底色使用ItemData静态分类色表的末项，该表值本批未展开。common37与APK同名不同像素，必须用[Steam设施资源包](../assets/steam-facility-common/README.md)；SEB15数字也经该包匹配image105，SEB12经Steam建设资源包匹配image103。

普通人物helper按UserData静态`+0x128`的若干行`[direction,xStart,xEnd,duration]`求总时长、`frame%sum`、当前段和段内插值x，再根据住宅状态作x修正；它不是取实例当时世界坐标。`GetAnimeIndex(0,frame)`求动画，`SetDispPlayerData(0,body,anime,direction,frame)`写共享表现对象，随后按当前武器定义ID绘制；无实际实例时到这里结束。有实例才调用power-bar，并在局部闪烁槽及低HP条件下画SEB39/image22危险图标。静态调度表的具体行值和power-bar内部本批未展开，不能由截图估计整套循环周期。

这页绘制会改变共享展示对象并注册本帧触摸区域。维护层应沿用已授权的Owner显式表现请求／独立回放边界；只读皮肤不应调用原式共享写入或重算世界。每帧触摸清理／页面退休由框架另证，不能将本页静态资源数解释成全生命周期永久有界。

## 未闭合与后续最小范围

1. 外层Init/Update的触摸输入转换、软标签动态刷新、初页与返回父选择恢复；不在此以APK填空。
2. 人物静态调度行、power-bar／奖章／气泡及动态human/weapon依赖集；优先具名helper闭合，不重扫巨型绘制方法。
3. 原窗口实际四页、触摸及字号折行证据；本批源码坐标与原画面分开认证。
4. image88已独立发布，最终完整皮肤接线仍需验收；本批未写C++、未修改原游戏／存档、未做游戏回归。
