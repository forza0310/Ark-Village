# Steam人物详情：肖像调度、血条、奖章与气泡

同日后继[危险图标偏移](STEAM_HUMAN_PINCH.md)已独立核出`OFF_EFPINCE={{−14,−28},{5,−28}}`及实际消费者；下文“本批未解”保留当批边界，当前可沿后继合同取得精确锚点。原窗口像素与完整人物树仍未认证。

2026-10-10。本页补齐[人物详情60](STEAM_HUMAN_DETAIL.md)委托的局部表现helper。来源是固定Steam GameAssembly与metadata的具名方法和初始化blob，不是原窗口观测；APK1.0.8及维护原型保持独立证据等级。复现工具、方法／调用锚及资源哈希见[研究包](../verification/steam-human-presentation/README.md)与EVIDENCE.json（本地核对材料，不随仓库交付）。

## 普通肖像的100槽调度

`UserData.Draw_charaInfoChara`在`0x102CFF00`读取`UserData.MOVE_DATA`（静态字段偏移`0x128`）。本批从`UserData..cctor`六次`InitializeArray`调用反查metadata的原16字节默认blob，并以字段名中的SHA-256交叉校验内容，得到每行`[direction, xStart, xEnd, duration]`：

| 段 | 非负frame的周期内区间 | direction | xStart→xEnd | duration |
| --- | --- | ---: | --- | ---: |
| 0 | 0–9 | 1 | 75→75 | 10 |
| 1 | 10–39 | 1 | 75→98 | 30 |
| 2 | 40–49 | 1 | 98→98 | 10 |
| 3 | 50–59 | 2 | 98→98 | 10 |
| 4 | 60–89 | 2 | 98→75 | 30 |
| 5 | 90–99 | 2 | 75→75 | 10 |

方法先求六段duration总和100，再用输入frame取余；累计时长首次严格大于余数的行是当前段，段内时间为`余数−此前累计时长`。x使用原`RateConvert(segmentTime,0,duration,xStart,xEnd,true)`，不是浮点正弦运动，也不是实际人物的世界位置。区间来自原表及循环条件的直接推导，不代表100秒或100个桌面渲染帧；调用方的更新／绘制频率需要另外认证。

动画索引由`Character2.GetAnimeIndex(0,完整输入frame)`求得，传入的不是段内时间或frame%100。因此换段／循环不自行重置步行动画。绘制顺序为：当前职业的`GetImgId(definition)`→`SetDispPlayerData(0,body,animeIndex,direction,完整frame)`→读取当前武器定义ID→`DrawDispPlayer(x+offset,121,weaponId,animeIndex)`。

`myHouse_[2]==1`时offset为0，其余为−12。因此无住宅的基础水平范围是63至86；有住宅时75至98。保留direction原值1／2，不在未读对应绘制方向表时擅自称左／右。`DrawDispPlayer`的人物／武器树和`GetAnimeIndex`内部索引仍是各自已有或后继专题，本页不由PNG列数猜具体身体帧。

`SetDispPlayerData`是共享表现对象写入。维护只读皮肤应产出人物身份／动画／方向／武器的显式请求，由已授权Owner表现请求／独立回放机制处理；不得在绘制回调中重新计算人物业务属性、消费世界随机或移动冒险者。

## 没有活动实例、普通实例与倒下实例

人物详情先查同定义的实际`Character2`，可能为空。三条分支不能合并：

| 条件 | 本页已证行为 |
| --- | --- |
| 没有实际实例 | 仍按共享定义绘制上述肖像及当前武器；在肖像helper的`0x102D0281`空判断直接结束，不调用power-bar，不画危险图标 |
| 有实例且非倒下展示分支 | 普通肖像之后调用实例血条，再按独立字段／闪烁条件决定危险图标 |
| 调用者已见`state_==7` | raw60外层改走倒下展示：`SetDispPlayerData(7,body,0,1,0)`、简版`DrawDispPlayer(75,121)`、`Draw_fukidashi("ﾀﾞｳﾝ中",75,79)`；这不是普通MOVE_DATA支 |

“没有活动实例”不等于人物定义不存在，不意味着隐藏职业、当前装备、六属性或四战斗缓存；也不应填一条100%血条假装当前HP已知。通用power-bar helper内部虽有`chara==null`时填宽18的分支，raw60普通肖像调用点已先挡住空实例，所以该fallback不是raw60的空实例行为。

## 血条使用PB_NOW，危险图标使用PB_AFTER

`Character2`映射直接登记`PB_NOW=1`、`PB_AFTER=3`；`powerBar_`字段偏移`0x104`。数组元素1与3不能按名字相近合并成一个“当前HP”。

raw60实际调用`Draw_charaPowerBarToSubForm(g,x,121,item=null,chara=live,eff=false,frame=0)`。这个调用没有道具加值数字，也不运行可选effect动画。对一般输入锚`(dx,dy)`，绘制顺序如下：

| 图元 | 逻辑矩形／参数 | 颜色RGB |
| --- | --- | --- |
| 外框 | DrawRect(dx−11,dy−26,21,5) | (246,246,246) |
| 内框 | DrawRect(dx−10,dy−25,19,3) | (39,53,74) |
| 绿色部分 | FillRect(dx−9,dy−24,width,2) | (83,255,0) |
| 剩余部分 | FillRect(dx−9+width,dy−24,18−width,2) | (68,100,104) |

`width=RateConvert(powerBar_[PB_NOW],0,live.GetHpMax(),0,18,true)`。最大值由当前实例方法取得；不拿raw60概览的`equipParam_[0]`替代绘制期实例输入，也不以道具恢复量直接生成长度。对raw60的dy121，这些y分别为95、96、97；外框宽21、内框宽19、填充总宽18应分别保留，不能“对齐”成统一宽度。

危险图标在普通肖像之后独立判断：对正常非负输入frame，`trunc((frame%12)/6)==0`，即每12槽的前6槽才继续；然后计算原整数`100*powerBar_[PB_AFTER]/GetHpMax()`，结果**≤30**时绘制SEB39/image22的frame0。阈值是整数除法后的结果，不能改成严格HP比例<30%；也不能拿绿色宽度或PB_NOW判定。

图标偏移另取`Character2`静态表`+0xA0`：direction为2或3用索引1，否则索引0，再叠肖像坐标及住宅偏移。该偏移表具体数值本批没有解码，故只登记依赖，不能根据截图估一个坐标冒称全闭合。SEB39虽然有两帧，此调用点明确用frame0，不按闪烁槽交替frame0／1。

上述整数除法、正最大HP和正常frame是原游戏合法状态下的消费者说明；本页没有新增负frame、乘法溢出或HP最大值0时的安全策略。维护错误处理应沿Owner既有输入契约，不能把安全防护编成原代码事实。

## 奖章：1至4个图标，5起显示乘号与数字

`AppData.Draw_medal(g,dx,dy,num)`在`0x10254F70`，raw60调用锚为(83,70)。num为0时不画；1至4按`Draw_icon(mode6,index1)`重复，依次锚在`dx+20−10*i,dy`，i从0到num−1。它不是从dx向右逐枚排列。

num≥5时令`R=dx+30`、`W=8*GetFig(num)`：奖章图标锚`(R−W−22,dy)`；乘号用SEB12/image103 **frame16**，锚`(R−W−10,dy+4)`；数字用`DrawNumImage`的SEB12、空前后缀、padding0、anchor4，锚`(R,dy+4)`。这里的8用于数字宽度，不得用徽章图的14像素宽代替。

SEB12是`number05.seb`，资源页为Steam匹配的image103 `number05.png`。frame16裁片为源(61,10,9,10)，不是根据字符字体临时写一个“×”；它与普通数字函数共同形成多枚奖章显示。负num不是本页研究的业务输入，不能把循环意外路径当正常UI设计。

## 气泡：60宽度分支及中心尾部

`AppData.Draw_fukidashi`在`0x10253230`。先使用当前Font的`StringWidth(text)`得到W，正常非负宽度以`H=trunc(W/2)`、`L=dx−H`定位；本地函数不改变字体大小。以下坐标都是原逻辑绘制坐标：

- **W≤60**：image7源`(36−H,0,W,21)`绘至`(L,dy)`，通过选取原中段保留气泡尾部。
- **W>60**：从L开始反复取image7源x6／y0，每段宽`min(27,remaining)`、高21，铺满W；再把源`(33,0,5,21)`绘至`(dx−2,dy)`补中心尾部。不是拉伸整张气泡PNG，也不把尾部重复到每块。
- 两支均追加SEB28/image7 frame0于`(L−6,dy)`，frame1于`(dx+H,dy)`。SEB裁片分别为左(0,0,6,18)、右(65,0,7,18)，主体／尾部高21而端帽高18，是原资源差异。
- 最后设黑色，文字`DrawString(text,dx,dy+3,anchor2)`。这里保留原anchor数值，实际字宽仍由当前字体消费者决定，不用文字字符数乘固定宽度。

局部中虽在绘制文字前有空字符串替代分支，测宽发生在它之前，不能由此声称任意null输入都安全。没有本地clip push/pop；父窗口裁剪是否已经生效需要外层合同，不能由“本函数没裁剪”推断整个气泡不受裁剪。

## 素材和交付范围

本包7个明确common依赖是4图＋3SEB，不包含完整动态人物／武器树：image7 `fukidashi_back`、35 `icon_medal00`、22 `ef_pinch`、103 `number05`；SEB28、39、12。审计沿EXE `img.inf/seb.inf`逻辑索引定位原资源记录，再核本地发布路径的字节一致性；即使路径在`assets/original/common`，也是本批逐文件证明与EXE一致后才复用，不是按同名推断两版相同。

窗口、字体、四页其它图元及输入分别沿[人物详情](STEAM_HUMAN_DETAIL.md)、[人物输入](STEAM_HUMAN_INPUT.md)、[窗口字体](STEAM_WINDOW_FRAME.md)。本批不生成新图片、不注册CMake、不改产品侧、不接C++渲染；原窗口像素、实际输入、完整皮肤、危险图标偏移表及所有人物／武器帧仍分开验收。
