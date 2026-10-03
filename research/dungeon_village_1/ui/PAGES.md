# UI 截图与 APK 渲染映射

日期：2026-10-03。范围：R3-V；关联RQ01/RQ03/RQ04/RQ05及局部RQ07/RQ09。
本报告的映射研究按用户要求从代码补缺图页面、反查已有截图渲染出处；本轮文档整合与注释检查独立记入 [验证入口](../VERIFICATION.md)，不修改产品规则。
截图总入口见 [视觉参考](../references/README.md)，总体边界见 [视觉基线](README.md)。

## 来源与置信度

- APK固定为1.0.8/versionCode9，身份见 [证据索引](../EVIDENCE.md)；下文代码结论只属于此输入。
- 用户明确截图来自实际运行，版本与APK不同；代码与截图的对应是结构/功能映射，
  不认证截图版使用同一页面ID、相同资源、金额、汉化文案或坐标。
- “已定位”表示固定输入中有明确页面条件、字段或绘制调用；“候选”表示与截图组合相似，
  但尚未闭合全部消费者或跨版本身份；“未定位”不代表原作没有该功能。
- 生成源码在被忽略的work中保持只读，文档只记位置/字段/独立分析，不复制反编译实现。
  大方法中的跳转与副作用只记局部静态线索，不扩称完整模态栈、动态时序或全分支证明。
- [b/g渲染分支](../work/decompiled/sources/b/g.java)的i(o)在6926有重复块警告，
  a(o)和上层初始化/更新为大型重建方法。需精确时序的结论仍须低层/运行证据佐证。

## 全部截图索引

表中位置均为当前生成源码的1基行号；页码是g.f121a，不是标签数组下标。
同一页还受i（子页）、f126f（上下文）、设施类型和活动类别控制，不能仅按页面ID创建UI。

| 截图 | 可见内容          | 固定APK渲染出处/条件                                                                                                      | 映射边界                                                          |
| ---- | ----------------- | ------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------- |
| S001 | 主菜单            | [b/g:539](../work/decompiled/sources/b/g.java)，b(o,int)，页面3；初始化10551                                               | 已定位菜单行/图标/选中；七标签不是七个实际菜单项，见后文          |
| S002 | 建设目录          | [b/g:9591](../work/decompiled/sources/b/g.java)，a(o)，页面21，i为三分类                                                   | 已定位分类/列表/报价；开放集合由状态决定                          |
| S003 | 放置预览          | [b/c:1006](../work/decompiled/sources/b/c.java)，场景f102a=1绘制a.o.ad；[a/o:69](../work/decompiled/sources/a/o.java)提示表 | 已定位提示/模式；箭头和确认全过程仍未闭合                         |
| S004 | 冒险通信          | [b/g:9483](../work/decompiled/sources/b/g.java)，a(o)，页面15                                                              | 标题/多页箭头/正文偏移已定位；非页面36收支情报                    |
| S005 | 场景/HUD          | [b/c:657](../work/decompiled/sources/b/c.java)上栏；[c/n:3634](../work/decompiled/sources/c/n.java)设施/人物底栏            | 已定位日期/金币来源；不提供新局初值或全地图                       |
| S006 | 无入住申请者      | [b/g:6952](../work/decompiled/sources/b/g.java)，i(o)，页面74，设施活动类别g=6                                             | 空申请者来自X.size()==0；不是普通经营详情                         |
| S007 | 城镇晋级条件      | [b/g:2921](../work/decompiled/sources/b/g.java)，f(o)，页面48/49                                                           | 条件/星槽结构候选；未将截图直接认证为48或49                       |
| S008 | 设施选中/气泡     | [c/a:249](../work/decompiled/sources/c/a.java)选中轮廓；[c/b:233](../work/decompiled/sources/c/b.java)气泡文本表            | 轮廓调用已定位；具体气泡绘制/施工帧身份仍需追踪                   |
| S009 | 冒险者到访        | [b/g:3261](../work/decompiled/sources/b/g.java)，f(o)，页面59                                                              | 人物到访插画/文本候选，APK标题“活动”与截图“事件”不同          |
| S010 | 局部战斗连击/短条 | [人物战斗代码](../work/decompiled/sources/c/b.java)和效果消费者待继续追踪                                                  | 本轮未闭合“2连击”的渲染出处，不擅认数字10语义                   |
| S011 | 征集冒险者结果    | [b/g:870](../work/decompiled/sources/b/g.java)，c(o)，页面24                                                               | 角色/等级/合计as结构候选；页面23为前置募集页，不能混用            |
| S012 | 野外攻略进度      | [c/m:487](../work/decompiled/sources/c/m.java)，a(o,x,y)，状态f209e=1、g>=0                                                | 进度条及g/h到百分比消费者候选；“攻略中”标签与目标菱形未全部绑定 |
| S013 | 赠礼结果          | [b/g:6440](../work/decompiled/sources/b/g.java)，h(o)，页面66；[c/n:2913](../work/decompiled/sources/c/n.java)属性过渡      | 插画/属性显示候选；截图标题不同，增量不等于最终值                 |
| S014 | 阿南商会物品      | [b/g:7951](../work/decompiled/sources/b/g.java)，j(o)，页面84，f126f=0、i=0                                                | APK为“南瓜商会”，同布局候选；购买/持有页与出售上下文另分        |
| S015 | 选择入住者        | [b/g:7387](../work/decompiled/sources/b/g.java)，i(o)，页面80                                                              | 列表/费用/余额色分支已定位；不覆盖截图版资格/收费                 |
| S016 | 舒适小屋完成      | [b/g:8507](../work/decompiled/sources/b/g.java)，j(o)，页面96                                                              | APK标题“自宅完成”，同插画/属性模板候选；不是81等级升级          |
| S017 | 旅店详情1/2       | [b/g:6952](../work/decompiled/sources/b/g.java)，i(o)，74普通分支、f126f=0、i=0                                            | 已定位字段/图像/命令，数值只消费APK状态，不用截图常量             |
| S018 | 防具店详情        | [b/g:7001](../work/decompiled/sources/b/g.java)，74装备店分支g=4                                                           | 种类/设施图/出售中/查看商品已定位，不用普通详情模板               |
| S019 | 花店详情1/2       | [b/g:7042](../work/decompiled/sources/b/g.java)，74普通分支、i=0                                                           | 右下效果由z/A绘制，交叉餐具具体语义未认证                         |
| S020 | 武器商品1/2       | [b/g:7427](../work/decompiled/sources/b/g.java)，i(o)，79、f126f=1、i=0                                                    | 四行/价格/信息入口已定位；非商会、非防具店下一帧                  |
| S021 | 设施强化列表      | [b/g:7259](../work/decompiled/sources/b/g.java)，i(o)，页面75                                                              | 库存z/选中/三项分档提示已定位；底部+不是精确效果                  |
| S022 | 1名入住申请者     | [b/g:7140](../work/decompiled/sources/b/g.java)，74、g=6、X.size()>0                                                       | 与S006同模板不同状态；不认证两图连续转变                          |

## 主菜单修正

[初始化10551](../work/decompiled/sources/b/g.java)默认加入标签0/1/2/5/6，即建设/冒险/村办/情报/系统；
仅当bG.k(1)满足才加入标签3“开发”。该局部没有加入标签4“交易”。
因此[标签池109](../work/decompiled/sources/b/g.java)里的七个标签不等于七项均显示，
S001五项菜单与默认结构相符；“五项对七标签”不能单独证明版本差异。
版本不同仍以用户明确说明为依据，不因此改称同版，也不推断交易在所有情境都没有入口。

菜单行间距28，选中背景由SEB menu绘制，主菜单图标由SEB wnd_menuIcon按标签映射。
[输入区599](../work/decompiled/sources/b/g.java)主菜单与子菜单注册方式不同，
需叠加当前窗口/绘图偏移，28不是截图物理像素。

## 设施详情的条件与字段

[页面74分类6955](../work/decompiled/sources/b/g.java)按顺序区分：定义f82e=12住宅；
活动g=1/4/5装备商店；g=6募集入住；f82e=2周边增益设施；其余普通分支。
普通分支且f126f=0才绘制“两页”标题/左右箭头；建设目录信息入口使用f126f=1，
是定义预览，不可假设有场景实例/月度数据，也不显示同样的使用道具交互。

| 可见字段       | APK数据/绘制来源                                                                                                  | 局部坐标与边界                                                     |
| -------------- | ----------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------ |
| 名称/图标      | 定义o.f80c/f81d；[b/g:6973](../work/decompiled/sources/b/g.java)                                                   | 图标(21,66)，名称(41,69)；图标9类来源独立于设施大图                |
| 价格           | 实例上下文o.a(0,m)，定义预览o.e(0)；[b/g:6991](../work/decompiled/sources/b/g.java)                                | 数字右端参数(223,70)，数字SEB15；不是建设价h()                     |
| 大图           | 定义o.k→bs[k]→分片SEB；[a/j:37](../work/decompiled/sources/a/j.java)                                             | 普通图区x=23，特殊居中x=71，y=88，宽97高74；通过裁剪绘制           |
| 等级           | 共享定义o.G；[b/g:7022](../work/decompiled/sources/b/g.java)                                                       | wnd_lv与数字；G=5显示max素材，不是实例等级                         |
| 品质/魅力      | 实例o.a(1,m)/o.a(2,m)，预览o.e(1)/o.e(2)；[b/g:7067](../work/decompiled/sources/b/g.java)                          | 右上框(123,88,90,34)，两行间距15，语言分支调整文字/数字位置        |
| 效果图标/+     | 定义o.z[]图标类别7、o.A[]个SEB15帧14；[b/g:7102](../work/decompiled/sources/b/g.java)                              | 右下框(123,124,90,38)，效果行间距17；不能从图标直接命名未证规则    |
| 到下级剩余人数 | o.d()-o.K；[b/g:7177](../work/decompiled/sources/b/g.java)                                                         | 文本(45,172)、数字右端(188,172)与人数符号；不是累计人数            |
| 商店种类       | 页面初始化筛出的X.size()；[b/g:3729](../work/decompiled/sources/b/g.java)                                          | 武器bt/防具bz/饰品bA，仅p!=0项，不等于全表长度                     |
| 入住申请者     | o.a(X)生成候选，渲染按X.size()；[b/g:3749](../work/decompiled/sources/b/g.java)                                    | 空/非空文字分支在7140，动作标签APK为“入住希望者”                 |
| 场景底栏利润   | c/n设施分支→m.h()；[c/n:3696](../work/decompiled/sources/c/n.java)、[c/m:903](../work/decompiled/sources/c/m.java) | 仅定义f82e=9或3；v[0..当前月]收入列减费用列累加，不是单次收入/余额 |

底栏利润的写入消费者[c/m:851](../work/decompiled/sources/c/m.java)分别按当前月写v[][0]/v[][1]。
因此固定APK静态证据可以解释其来源范围，但截图“收益3,720G”等仍不能认证为同一口径或同版数值。
源码显示负净额使用取绝对值和不同数字样式，不假设一定绘制负号。

## 无截图页面的静态补齐

以下不等待截图才登记源码事实，但不生成猜测画面，也不写成动态验收通过。

| 缺图页面/操作      | APK静态内容与出处                                                                                                                                                                                                                             | 仍需核对                                                          |
| ------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------- |
| 设施信息2/2        | [b/g:7006](../work/decompiled/sources/b/g.java)左“设施奖励”、右“维护费”o.a(3,m)；[b/g:7195](../work/decompiled/sources/b/g.java)Y列表，最多5行，行间距19，定义名加来源层级、价格/品质或魅力奖励；空列表“没有奖励”；底部“周围设施的奖励” | 标题/文案与截图版本、实际来源列表和字体；不能照抄截图第1页做第2页 |
| 商品2/2            | [b/g:7441](../work/decompiled/sources/b/g.java)列头属性图标；[b/g:7518](../work/decompiled/sources/b/g.java)武器s[1]/s[3]，防具/饰品h[0]/h[2]，值大于0才画；同列表最多4行、行间距24                                                             | 装备属性图标的完整语义/本帧数据与截图外观                         |
| 设施强化演出76     | [b/g:7558](../work/decompiled/sources/b/g.java)横向场景、设施图、人物与演出计数；[初始化3773](../work/decompiled/sources/b/g.java)执行改良helper                                                                                                | 精确动画帧/实际动态时长；不能把原计数当秒                         |
| 强化结果77         | [b/g:7370](../work/decompiled/sources/b/g.java)“设施强化”，调用c/n设施属性结果绘制；[b/g:5609](../work/decompiled/sources/b/g.java)确认输入可跳到展示末段或关闭                                                                               | 属性结果图、回到详情的栈恢复；不预建二次确认弹窗                  |
| 设施等级提升81     | [b/g:7815](../work/decompiled/sources/b/g.java)“等级上升”，两段正文/属性展示；[初始化3840](../work/decompiled/sources/b/g.java)执行升级helper，关闭5703清提示                                                                                 | 运行重入/完整输入与显示；S016住宅完成不能代替                     |
| 建设设备分类       | [b/g:10647](../work/decompiled/sources/b/g.java)W[0]按状态过滤，插入-1撤除、条件k(32)插入-2配置更替；[绘制9688](../work/decompiled/sources/b/g.java)报价分别0/300                                                                               | 实际道路条目/开放存档、截图版命名；不由0报价推全部退款规则        |
| 道路/撤除/移动提示 | [a/o:69](../work/decompiled/sources/a/o.java)8个模式提示；[b/g:11595](../work/decompiled/sources/b/g.java)进入模式3撤除、6移动、1道路起点、0建筑；[b/c:1006](../work/decompiled/sources/b/c.java)绘制当前提示                                    | 完整预览、拒绝、确认/返回、位置/报价和所有热区                    |
| 入住余额不足       | [b/g:5681](../work/decompiled/sources/b/g.java)余额不够调用消息11并返回false；[d/a:3968](../work/decompiled/sources/d/a.java)直接比较金币与费用                                                                                                 | 消息11正文资源、截图版拒绝画面；不由静态消息号编造中文提示        |
| 设置页12           | [b/g:8794](../work/decompiled/sources/b/g.java)与8904两套语言布局，画面/速度/BGM/效果音/画面旋转，数值与选项组来自顶部数组                                                                                                                     | 实际语言分支、设置副作用与显示；未执行系统设置修改                |

### 强化列表不是精确增量展示

[页面75初始化3760](../work/decompiled/sources/b/g.java)只加入by数组中z>0的道具，空列表调用消息15并退出。
[绘制7271](../work/decompiled/sources/b/g.java)最多5行，行y=97+19×可见行号；高亮矩形(23,y-2,191,16)。
道具名称f32c、图标g、剩余z。底部价格/品质/魅力三个提示读取k[0..2]：

| 项目               | 原值到加号数量的静态分档                 |
| ------------------ | ---------------------------------------- |
| 价格k[0]           | 0→0；非零且<50→1；50至<100→2；其余→3 |
| 品质k[1]、魅力k[2] | 0→0；非零且<3→1；3至<6→2；其余→3     |

[d/a:2725](../work/decompiled/sources/d/a.java)按数量绘制SEB15帧14。
这些是原值的展示分档，不是类别倍率后的最终效果，也不能由一个+推精确品质+1。
[局部更新5574](../work/decompiled/sources/b/g.java)确认输入调用消耗helper后打开76、更新实例事件计数；
未看到在75与76之间另开独立确认页，不因缺“确认截图”预设必有二次确认。
76到77依计数切换，75/77取消或退出通过框架m()关闭当前页；完整父页刷新另待核对。

### 入住链路

固定APK局部结构为74的g=6分支→80→余额检查→拆原候选设施并新建住宅→进入施工/实例绑定。
入口在[b/g:5547](../work/decompiled/sources/b/g.java)，选中费用来自人物e.h。
[页面80绘制7411](../work/decompiled/sources/b/g.java)按金币足够选择SEB15，不足选SEB19；
这可解释固定APK价格色分支，不能认证截图版本每种红色都表示禁用。
80绘制最多5行，但更新5676的滚动窗口条件使用4；保留原局部差异，不擅自统一为5。

[实例施工完成c/m:538](../work/decompiled/sources/c/m.java)在u=1、定义f82e=12、绑定人物t!=-1时
排入96，另有满足/努力结果辅助请求。96绘制标题“自宅完成”、住宅插画、人物与结果字段，
确认输入分两段推进/关闭（[b/g:6152](../work/decompiled/sources/b/g.java)）。
截图S022/S015/S016仅提供对应页外观；未观察上述动作，不从900G→650G求入住费用。

## 绘图坐标与资源链

[b/g:9119](../work/decompiled/sources/b/g.java)先叠加bK/bL和窗口居中偏移，之后才进入页面渲染。
标准窗外框由[d/a:2763](../work/decompiled/sources/d/a.java)按宽/高居中，
还有b/c.n及调用参数偏移；白色内容框由[d/a:473](../work/decompiled/sources/d/a.java)加n/2。
因此以上是函数局部坐标/矩形参数，不是物理热区，也不能把所有页面统一为240×240。

加载链：[b/a:469](../work/decompiled/sources/b/a.java)将归档1放p，归档7放q，
归档9放t；[d/a:117](../work/decompiled/sources/d/a.java)的归档表对应common/common2/event。
[资源加载器](../work/decompiled/sources/kairo/android/ui/s.java)分别读取img.inf和seb.inf：
图片列表使用显式索引并重排，SEB列表本批所用项按0基列表位置。
不可将图片下标、SEB下标、逻辑页码或维护清单行号混用。
INF保留.gif逻辑名，实际归档条目与维护副本为PNG，映射按既有发布清单核对，不另做图像转换。

| 用途/调用              | 固定APK索引                 | 维护资源                                                                                                                                                                                              |
| ---------------------- | --------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 标题木纹窗底/横条      | p图片28/29，d/a:2772        | [wnd_back.png](../assets/original/common/wnd_back.png)、[wnd_bar.png](../assets/original/common/wnd_bar.png)                                                                                            |
| 白色内容框角           | p SEB6，d/a:485             | [wnd_conner.seb](../assets/original/common/wnd_conner.seb)；主体颜色/细框线是代码绘制                                                                                                                  |
| 手形指示               | p SEB21→图片70，g:7285/481 | [finger_r.seb](../assets/original/common/finger_r.seb)、[finger_r.png](../assets/original/common/finger_r.png)                                                                                          |
| 蓝色价格/属性数字      | p SEB15                     | [number08.seb](../assets/original/common/number08.seb)，不是文字字体                                                                                                                                   |
| 普通暖色数字/金币      | p SEB12→图片103            | [number05.seb](../assets/original/common/number05.seb)、[number05.png](../assets/original/common/number05.png)                                                                                          |
| 不足费用/负利润色样    | p SEB19→图片109            | [number12.seb](../assets/original/common/number12.seb)、[number12.png](../assets/original/common/number12.png)                                                                                          |
| 设施标题图标           | p图片91，g.c类型9           | [icon_tenantInfo.png](../assets/original/common/icon_tenantInfo.png)，横向16×16裁片                                                                                                                   |
| 道具图标/分类底色      | p图片9/24，g.c类型1         | [tresureIcon00.png](../assets/original/common/tresureIcon00.png)、[icon_back00.png](../assets/original/common/icon_back00.png)                                                                          |
| 商品武器/防具/饰品图标 | p图片12/20/21，g.c类型2/3/4 | [icon_weapon00.png](../assets/original/common/icon_weapon00.png)、[icon_armour00.png](../assets/original/common/icon_armour00.png)、[icon_accessry00.png](../assets/original/common/icon_accessry00.png) |
| 设施效果图标           | p图片37，g.c类型7           | [icon_param00.png](../assets/original/common/icon_param00.png)，第二排16×16，具体规则另证                                                                                                             |
| 等级/上限              | p图片38/129                 | [wnd_lv.png](../assets/original/common/wnd_lv.png)、[wnd_max.png](../assets/original/common/wnd_max.png)                                                                                                |
| 详情/商品翻页箭头      | p SEB3                      | [arrow02.seb](../assets/original/common/arrow02.seb)；不当作建设预览黄色箭头                                                                                                                           |
| 配置更替目录图         | p图片119                    | [moveTenant.png](../assets/original/common/moveTenant.png)                                                                                                                                             |
| 新条目标记             | p图片147                    | [wnd_new.png](../assets/original/common/wnd_new.png)，类别/条目状态分别控制                                                                                                                            |
| 小屋完成横图背景       | p图片39与设施/人物叠加      | [mp_back.png](../assets/original/common/mp_back.png)，不是整张完成页面位图                                                                                                                             |

橙色高亮T为RGB(255,153,55)，由代码填矩形；按钮文字宽度经当前字体测量。
[g:470](../work/decompiled/sources/b/g.java)对“使用道具”等动态计算高亮及注册输入区域，
手形另经SEB绘制；不能把背景高亮当作一张固定尺寸按钮素材。
列表滚动条、中文字体、最终触摸换算与完整设施定义到分片映射仍需下一轮细化。

## 可复查身份与本轮检查

当前读取的关键生成文件SHA-256如下，JADX再生成后行号可能变化；先核对固定APK与文件身份。

| 文件     | SHA-256                                                          |
| -------- | ---------------------------------------------------------------- |
| b/g.java | 1b46873ef42121303cb238794c57213844eca67a99495371dd8d7e4981b3c877 |
| b/c.java | e20eeba16c56c35b1634e9e2970042314d2e994141fafbb564f3a8c44b1b2e1b |
| c/m.java | ef76ae372cd33f4c898dc5228f2fa3242ac6ceab6c77063b44e79713d7d4bfd2 |
| c/n.java | 4fccf7efee95ed0c4931bc00c21ef510dbd037c9e915f984d639679a83a0757e |
| d/a.java | e4c36b0e8b70a32ca63b6581392247ef3562fb99643e12ceb0d7f312dee95770 |

本轮只读既有生成源码与INF/素材，已有SEB检查器对finger_r及number05/08/12逐记录解析成功，
手形为6帧4记录，三个数字SEB均为21帧21记录；number08使用图片105，已实际查看蓝色数字及+字形。
finger_r图像70的偏移为(-8,-5)/(-6,-5)，
不把SEB稀疏记录数当帧数，不把帧或原计数当实测秒数。
上述页面映射首次检查未运行新构建、游戏或Git；检查器进程均已退出。
后续轮次的维护验证见 [当前验证](../VERIFICATION.md)，历史检查见 [归档](../verification/2026-10-03.md)。

本轮 [新局与建设](../rules/STARTUP.md#6-初始画面素材与输入)已补首名人物图集、初始镜头、
首次对话与普通解锁插画的区别、模态更新资格及输入坐标链。
仍需细化滚动条、物理热区、字体替代度量、建设取消与战斗反馈；不以补图作为唯一推进路径。
