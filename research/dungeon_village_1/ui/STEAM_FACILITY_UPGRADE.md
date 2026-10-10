# Steam设施升级81的绘制与确认

2026-10-10。固定Steam2.56；[工作包与复算](../verification/steam-facility-upgrade/README.md)复用已冻结具名SubForm方法，补8个有限helper和一个直接调用数组读取段。APK既有[演出合同](EQUIPMENT_FACILITY_RENDER.md)只交叉，不填Steam缺口；没有新窗口、C++或产品验收。

## 入口与真正升级

[设施详情74合同](STEAM_FACILITY_DETAIL.md)已核场景选中tenant后先取共享tenantData并检查isLvUp，真则进入81，优先于施工状态拒绝；并非先展示74再由详情的确认开始升级。81同时持有实例tenant和共享定义tenantData，二者不能用同一个ID替代。

Init2的81分支0x1030F0E8调用SetTenantLvUpParam(tenantData,tenant)。该helper在初始化时实际修改共享定义：

1. UpdateTenantParam(tenantParamTUserUp)刷新旧值。
2. 对价格／品质／魅力三槽保存显示前值。当前定义tenantParam加选中实例bonusValue后，以定义各槽原范围上端×2做ClampMax；不是直接抄原表加成。
3. tenantUserNum减**旧等级**GetNextLvUpNum；lv<5时lv加1并刷新参数。单次仅升一级，不因剩余人数够多在本页循环连升。
4. 用同一实例bonus和封顶方式保存显示后值，差值=后−前，最后再次刷新定义参数。

显示三行保存在TenantData静态tenantItemParam[前／后／差]；原代码不是每个弹窗自带独立不可变三槽。选中实例提供邻接修正，等级与使用人数属于共享定义。Init2没有维护Owner的完整失败回滚保证，维护实现继续以已确认事务模型隔离。

**最终确认只清共享isLvUp并Pop，不再升等级、不再扣使用人数。** 不应为了展示安全把初始化的真实升级推迟到末次确认，也不能恢复页面时重复跑升级初始化。本页没有建设扣款或道具库存消费。

## 两阶段计数与输入

Steam cctor独立核出：TENANTITEM_ANIME_TIME2=[2,32,34,49,55]；81第一表C4=[40,50]，第二表C8=[55,65]，后者由共享表末项55及55+10构造。Update2调用的数组helper0x10002640读取传入索引，两个门槛调用实际传0；故确认门槛是40和55，不能取最后项50／65。

| 状态／动作 | 原实际行为 |
| --- | --- |
| page=0且frame=1 | Update2调用PlayJingle(20)，是更新消费者，不是绘制触发声音 |
| phase0确认且frame<40 | frame置40，保持phase0 |
| phase0确认且frame≥40 | page置1、frame置0；**frame2不清零** |
| phase1确认且frame<55 | frame置55，保持phase1 |
| phase1确认且frame≥55 | 清tenantData.isLvUp，Pop |
| 无确认 | 不自动推进或自动关闭 |

确认是pulse0x100000；存在与74相同的配置门控keyState0x40替代分支，未据此认定某个物理键。81本地Update2没有IsPushSoftLabel(2)或取消分支；softLabels[81]=[0,0]，两边均空，不能沿用74的右返回。通用平台退出／强制关闭不在这项局部合同内。

绘制结尾注册Touch组件2、TouchOption.Create(2)，与软标签为空并不冲突；这是原确认输入面，不据无坐标重载猜桌面全屏矩形。计数来自应用原回调顺序，不能把40/50/55/65换成秒或把截图间隔当tick。

## 窗框与绘制顺序

Draw分支0x1034E3C2–0x1034E9CA先DrawWindow(222,170,0,"レベルアップ")，再DrawBox(15,139,224,202)，完整木纹／标题／边线按[Steam窗框](STEAM_WINDOW_FRAME.md)。接着按phase画文字／属性；最后画两侧小角色和影子，再注册确认组件。没有按画面印象重排这些层。

phase0先画裁剪中的通知，再调用Draw_tenantStrength(tenantData,**−1**,false)；负frame使三属性行跳过，只留下演出背景与设施图。phase1则直接传当前frame，dispCombi仍false，所以**不画普通道具成功／失败的组合响应文字**。共用helper存在该分支不能证明81会显示它。

共同背景来自AppData.resEvent（偏移0x24，**不是resEffect**）：event图片14 event_getItem_back在(30,68)，图片16 event_BackMini02在(77,71)。随后ClipRect(80,74,80,64,true)，DrawMapchip2(g,120,105,定义mapchipId,0)，PopClip。图块仍使用74已核居中pattern修正；实例真实朝向不传入此页。

## 第一阶段文字：日文与非日文分开

裁剪请求为(0,158,240,H,true)，H=RateConvert(frame,5,40,0,48,true)，整数乘除按原顺序并向零截断。原通知正文色SC_WINDOW_BLACK=(92,51,31)，绘完PopClip；没有本页临时改字号调用，继承当前Font，不能用中文默认字号和固定字符数替换测宽。

日文支：`LT("<0>が",名称)`在(120,158)anchor2；“レベル”在(85,176)anchor1；当前共享lv用common SEB16于(130,175)、padding0、anchor2；“に”在(141,176)anchor1；“ﾚﾍﾞﾙｱｯﾌﾟしました!”在(120,194)anchor2。

非日文支：`LT("<0>",名称)`在(120,166)anchor2。令d=Font.StringWidth(lv.ToString())，p=Font.StringWidth(LT("レベル"))，s=Font.StringWidth(LT("!"))，

```text
half = trunc((p+s+trunc(10*d/6)+7)/2)
prefix: (120-half,184), anchor1
lv:     (124+再次测得的prefix宽-half,183), SEB16, padding0, anchor1
suffix: (120+half,184), anchor4
```

四次StringWidth调用包含前缀二次测宽。数字SEB宽度与Font对数字文本测宽不是同一资源，原式保留10/6和+7，不能把数字画成字体文本后假称同版。

phase0的提示箭头资格为frame≥50，并且frame%25<15；点(212,204)，common SEB2 arrow01搭配图片72、frame0。**40已经允许切段，50才开始提示箭头**，二者不是同一门槛。

## 第二阶段三属性与特殊计数插值

三行y=158+18×slot，slot0/1/2为价格／品质／魅力。标签色SC_WINDOW_BLUE=(0,100,255)；日文x48、y不变；非日文PushFontSize(9)，x39、y+1，然后BackFontSize。实际字符串来自TenantData.TENANTITEM_NAME，未用PNG字形替代语言字符串。

| frame范围 | 主数值／差值 |
| --- | --- |
| <2 | 前值；差值隐藏 |
| 2–31 | 前值；每槽的差值起跳时间为frame−2−6×slot，负时隐藏；其他时候使用GetParabolaValue(8,12,局部计数)，向零截断后从行y减去 |
| 32–33 | 前值；非零差值位于行y |
| 34–48 | RateConvert_countAnime(frame,34,49,前,后,true)；差值位于行y |
| ≥49 | 后值；差值位于行y |

差值只在实际差≠0时绘制。价格主值Draw_money(134,y,值,seb15)；品质／魅力主值DrawNumImage(seb15,值,134,y,padding0,anchor4)。MAX检查比较**当前显示值**与GetTenantParamLimit(slot)，达到上限在(72,y+3)画图片129 wnd_max；不是只依据最终新值提前点亮。

差值使用SEB12 number05：价格Draw_plusValue(190,差值y,差,12)，再在同点画frame20货币字；品质／魅力Draw_plusValue(198,差值y,差,12)。对应图片103必须用Steam建设差异包，主数值SEB15图片105必须用[Steam详情差异包](../assets/steam-facility-common/README.md)。

RateConvert_countAnime是独立整数策略，**不能替换成普通lerp**。对本页34–49递增计数区间，令old/new是前后值、a=old+sign(new−old)：

- old=new、或a=new时直接返回old，因此差为±1在34–48仍保持旧值，到49外层分支才显示new。
- 其他情况令end=clamp(34+3×(new−a),34,49)；end=34时也返回old。
- 有效区间内返回a+trunc((new−a)×(frame−34)/(end−34))，超过有效end直接new。较小正差会提前结束计数；下降路径不能擅自镜像正差动画。

抛物helper使用float32中间运算。TIME20／12时n=trunc(TIME/2)，初速2×DIS/(n−1)，加速度−2×DIS/(n×(n−1))，返回max(0,初速×t+加速度×t×(t+1)×0.5)。调用方向零截断高度，不能用双精度对称抛物线或四舍五入替换后宣称逐帧一致。

phase1箭头在frame≥55时调用Draw_btmCursor(212,204,frame−55)，同样模25的前15槽可见。C8的65在本页已核Update2／Draw分支中没有关闭用途，不添加一个65自动退出。

## 两侧人物与独立frame2

令t=frame2%30。t<20时h=trunc(GetParabolaValue(6,20,t))，shadow=1、pose=1；否则h=0、shadow=0、pose=0。左锚x52，右x188，基准y144。

原顺序：左影(52,144)→左角色(52,144−h)→右影(188,144)→右角色(188,144−h)。全用common SEB80 chara_mini和图片93。影子frame=shadow；Draw_spChara的ID5在pose0/1取frame3/5，ID6取frame2/4。direction与cnt调用参数都是0，不拿人物走路方向代填。

SEB裁片／偏移：影frame0为(2,51,13,2)、offset(−7,−1)，frame1为(21,51,11,2)、offset(−6,−1)；角色frame2/3/4/5分别(1,0,15,24)/(1,24,15,25)/(17,0,19,25)/(17,24,19,26)，offset分别(−8,−24)/(−8,−25)/(−10,−25)/(−10,−26)。当前计数的画面锚再加这些内部offset。阶段切换只清frame而非frame2，不能让小角色每次切段重启跳跃。

这段局部绘制及已展开helper未见共同随机调用、升级写入或声音调用；不据此声称整个Graphics/SEB后端没有缓存、变换或平台副作用。表现计划应保存原显示参数，声音继续从更新请求消费。

## 资源、引用与剩余边界

本包登记17项已有出版资源，包含common框／数字／箭头／角色及event两背景；SEB所有图片引用和裁片范围已检查，无新增图片副本。资源存在不等于原窗口显示已逐像素验收，中文glyph、实际测宽与OS事件仍独立待证。

页面持有实例和定义，原三槽静态数组可被其它演出覆盖；本批未扩整框架退休、重复Init防护或自然玩家升级全路径。维护快照必须保留已有独立载荷与身份校验，不能在恢复时重新从当前定义推前值，不能以合法历史列表增长宣称泄漏或永久有界。

APK的初始化升级、40/50、55门槛、首帧声音20和共享三槽与本批Steam相符；Steam非日文文本测宽、mini角色资源及特殊countAnime已独立认证。未逐帧比较维护C++数字动画输出，也未完成APK对应helper与Steam机器码的逐算术交叉；不能只因“整数插值”旧概述较简略就宣布来源冲突或改测试预期。本包作为后继只读皮肤接线输入，未改原表、有效断言、Owner或旧冻结记录。
