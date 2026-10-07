# 装备举物与设施属性演出

2026-10-08。本专题交付固定汉化重签APK1.0.8的装备举物、商品图标及设施属性增长／减幅头标映射。
维护原型已接只读查询和实际素材绘制；完整赠礼／升级模态皮肤与原窗口动态认证仍分开登记。
本批没有修改产品、原表或素材，没有新增资源发布，也没有改变持久化schema。

## 来源与维护边界

固定APK身份见[证据清单](../EVIDENCE.md)，SHA-256为`1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5`。
原PNG／SEB／INF哈希见[素材清单](../assets/MANIFEST.tsv)。图片按`img.inf`显式ID，SEB按`seb.inf`零基行序；两种索引不可互换。

[装备工作包](../work/equipment-lift-render-mapping/README.md)和[机器映射](../work/equipment-lift-render-mapping/MAPPING.json)
逐条记录113装备定义、132个武器方向绑定、姿态偏移、SEB裁片及实际PNG边界；
[DEX证据](../work/equipment-lift-render-mapping/DEX_EVIDENCE.json)读取固定DEX中四个明确帮助器，指令字节数分别994／532／202／96，
DEX SHA-256为`b4386a0de612fe18196a390633607ef36c69c2a63881244832314e3bf4902d1a`。
这些帮助器认证商品图标、武器姿态／帧和override绘制；不认证巨型人物／页面方法的全部控制流。

[设施工作包](../work/facility-growth-render-mapping/README.md)和[机器映射](../work/facility-growth-render-mapping/MAPPING.json)
保存有限源窗口、9份SEB、6份PNG和原工具逐记录交叉。设施绘制链本批依据固定APK普通Java和原资源交叉，尚无单方法DEX认证；
普通巨型Java的JADX警告及生成稿身份差异保留，不将辅助定位稿覆盖已冻结低层证据。
Steam及用户异版本截图只作另行参考，不参与本页帧号、数值或时点证明；本页不声明原游戏窗口动态通过或产品已消费。

正式维护接口见[startup_world_visuals.hpp](../prototype/include/dungeon_village_prototype/startup_world_visuals.hpp)、
[只读实现](../prototype/src/startup_world_visuals.cpp)、[窗口适配](../prototype/src/startup_view.cpp)及[现有visuals套件](../prototype/tests/startup_world_visuals_test.cpp)。
查询只消费唯一Owner已有cd／notices：不推进年龄、装备、邻接、奖励、随机、页栈或声音。
缺定义、方向越界、缺载荷或数值溢出明确拒绝；没有像素输出与输入非法分开。
原型适配保留SEB层及相对锚点，CPU资源验证和有界窗口接线的结果由[阶段入口](../stages/README.md)及[本轮验证](../VERIFICATION.md)登记。

## 武器举物与商品图标

`weapon.txt`零基列2为商品图标d，列3为图片索引e，列6为风格h；这是三个独立字段。
weapon图片显式ID域为0–5、10–15、20–25、30、40–46、50–56；定义生成与查询均拒绝7等空洞ID，不能仅检查0–56。
原`c.b.a(canvas,x,y,dir,action4,weapon,phase0,subphase0,false)`使用`a.p.B[h]+dir`，`B=[0,4,8,12]`，
绑定weapon目录剑／弓／枪／大剑各四方向SEB，举物frame固定0。
绘制以`weapon/img.inf[e]`的真实PNG替换SEB内部非负图片索引；斧／杖借对应风格裁片，不能硬绑SEB内部image0。

| 风格h | dir0姿态偏移 | dir1 | dir2 | dir3 |
| --- | --- | --- | --- | --- |
| 0 剑 | (-5,-36) | (-4,-36) | (-19,-37) | (-17,-36) |
| 1 弓 | (-4,-35) | (-4,-35) | (-17,-35) | (-17,-34) |
| 2 枪 | (-5,-36) | (-4,-36) | (-22,-37) | (-20,-36) |
| 3 大剑 | (-9,-45) | (-8,-46) | (-24,-46) | (-23,-46) |

先取人物脚底屏幕点，减举物高度，再加上表姿态偏移和SEB内部offset各一次。
实际矩形与SEB偏移逐定义／方向保存在机器映射中，不用统一中心偏移替代。
例如短剑定义0的图片索引50→`weapon/sword00.png`，风格0／dir0为sword00.seb/frame0，源(0,0,21,25)、SEB偏移(0,0)。
炎斧定义9图片索引2→ax02.png，用同风格裁片，其商品图标25属于另一路。

| 商品图标kind | common图片 | 原定义字段 | 矩形 |
| --- | --- | --- | --- |
| 2／10 武器 | 12 / icon_weapon00.png | weapon列2/d | (18×(id%10),18×(id/10),18,18) |
| 3／11 防具 | 20 / icon_armour00.png | armour列3/e | 同上 |
| 4／12 饰品 | 21 / icon_accessry00.png | accessory列3/e | 同上 |

背景先画common24/icon_back00.png的(54,0,18,18)。这是直接PNG裁片，不是同编号SEB；列表64／79、详情72与赠礼66的商品图标不能替代地图武器举物。

### cd15／21／22年龄与装配时点

业务与cd来源沿[人物控制](../rules/ai/CONTROL_COMPOSITION.md#27与29的装备显示记录)，查询遍历原cd正序并保留重复记录；原人物flags bit1总守卫抑制绘制。
负年龄保持待启动且没有举物像素，不通过绘制改变它。

cd15由控制27产生，`cx=[0,20]、cy=[8,32,40]`，速度／加速度记录为571／-71。
令`P(a)=((a+1)*(cd[7]*a)/2+cd[6]*a)/100`，按原整数先乘后除、两次向零截断。
年龄0–7高度P(age)，8–31固定20，32–39高度P(age-32+8)，40由逻辑退休。
年龄<20显示cd[4]旧武器，≥20显示cd[5]新武器；真实装配仍在后续控制28，绘制切换不能提前提交装备。

cd21／22由控制29产生，分别读取防具／饰品真实图标字段。
年龄<12高度`(cd[2]*age+cd[3]*age*(age+1)/2)/100+16`，速度／加速度218／-18；年龄≥12固定28，22由逻辑退休。
背景与图标锚点同为脚底相对(-8,-height-16)，最终控制30装配独立。
异常载荷拒绝属于维护安全合同，不重现Java整数回绕，不补默认帧。

## 设施邻接增长与减幅头标

[邻接合同](../rules/FACILITIES.md#neighbourhood)已维护来源身份、道路魅力、实例s及完整重算；
原`a.o.a(boolean)`比较新s−旧G，正差价格／品质／魅力发kind1／2／3，负差发4／5／6。
原`c.m.c`同kind去重，`c.m.d`每逻辑更新只推进队首；kind1–6的age≥43退休。
维护绘制只读`facility_details.notices.front()`，后项不参与本次绘制，不再次发属性。

锚点L来自`c.a.a(instance.f(),L)`原投影；它包含镜头变换，不能直接改为格中心、PNG左上或设施图片顶部。
只读查询输出L相对offset，原型窗口再转换当前镜头；完整原场景深度桶、镜头和覆盖顺序认证另计。

| kind | 含义 | common SEB | 图片／主裁片 | SEB内部offset |
| --- | --- | --- | --- | --- |
| 1 | 价格上升 | 69 / eff_tenantUse00.seb | image49，(41,15*r,42,15) | (-20,-15) |
| 2 | 品质上升 | 70 / eff_tenantUse01.seb | image49，(0,15*r,41,15) | (-20,-15) |
| 3 | 魅力上升 | 71 / eff_tenantUse02.seb | image49，(83,15*r,41,15) | (-20,-15) |
| 4 | 价格下降 | 85 / eff_tenantUse04.seb | image120，(43,15*stage,44,15) | (-22,-15) |
| 5 | 品质下降 | 85 / eff_tenantUse04.seb | image120，(0,15*stage,43,15) | (-22,-15) |
| 6 | 魅力下降 | 85 / eff_tenantUse04.seb | image120，(87,15*stage,43,15) | (-22,-15) |

image49为[eff_tenantUse00.png](../assets/original/common/eff_tenantUse00.png)124×45，image81为[eff_tenantUse01.png](../assets/original/common/eff_tenantUse01.png)68×111，
image120为[eff_tenantUse04.png](../assets/original/common/eff_tenantUse04.png)130×45。没有同名eff_tenantUse02.png；该SEB实际引用49／81。

正负阈值`D=E=[2,4,6,9,12,15,43]`，从前向后找第一个age小于阈值的索引，全部经过则钳6。

| age | 正SEB frame | 正主图行r | 原正图层 | 负stage |
| --- | --- | --- | --- | --- |
| 0–1 | 0 | 0 | 0 | 0 |
| 2–3 | 1 | 1 | 0 | 0 |
| 4–5 | 2 | 2 | 0 | 1 |
| 6–8 | 3 | 2 | 0、1 | 1 |
| 9–11 | 4 | 2 | 0、1 | 2 |
| 12–14 | 5 | 2 | 0、1 | 2 |
| 15–42 | 6 | 2 | 0 | 2 |

负frame=`(kind-4)*3+stage`。正layer1只在frame3／4／5绘，image81裁(0,37*(frame-3),68,37)、内部offset(-34,-30)。
正调用Y偏移`map(age,0,6,-24,-31,true)`，负为`map(age,0,6,-32,-28,true)`；原map先钳输入，再整数乘除向零。
age0..6正偏移[-24,-25,-26,-27,-28,-29,-31]，负偏移[-32,-32,-31,-30,-30,-29,-28]，以后固定-31／-28。
不得用浮点插值或floor替代，不重复叠加SEB内部offset。

正layer1的frames0／1／2是image=-1无像素记录，frame6是image81零宽高记录，均不在原选层调用中绘制。
正式查询明确返回layer0及需要的layer1，窗口按指定层读取；有效裁片仍严格校验，不删原空记录、不复制首帧或放宽PNG越界。
`tf_price00/quality00/charm00.seb`虽是common57／58／59，实际邻接消费者调用69／70／71，不能用相似文件名替换。
L升级提示common79/tenant_bonus.seb、kind0开业提示和kind7商品NEW属于独立分支，本接口不宣称全部头标已接。

## 77／81三属性页：合同已定位，完整皮肤待接

原共享`o.ap[0/1/2][价格/品质/魅力]`是前值／后值／可见差，与人物六属性分开。
75已消耗道具，76初始化提交定义共享J并保存ap，77不再加属性；81初始化扣旧等级阈值K、G至多+1并保存同ap。
每实例邻接s独立，ap包含实际封顶／职业倍率／选中实例修正，不能直接把道具表增量或邻接裸增量显示成结果。
已有[道具规则接口](../example/include/dungeon_village_reference/facility_items.hpp)与[Owner](../prototype/src/startup_world_facility_items.cpp)维护这些事实及精确快照载荷。

76 `aI=[0,0,15,29,39,39,49]`，计数≥49自动创建77并退休；举物角色、抛物轨迹、光效和完整背景尚未恢复。
77与81 phase1共用`c.n.a`，`aJ=[2,32,34,49,55]`：<2旧值且无差；2–31差值按slot×6错开弹动；32–33旧值；34–48原整数插值；≥49新值。
三行Y=158+18×slot，实际非零差才绘；价格差右界190及货币符号，品质／魅力差右界198。
值达到槽上限才在(72,Y+3)画image129/wnd_max.png，不以原增量或库存判定MAX。

77适配文字`ar=0→frame1、1→2、2→0、-1→3`，用common SEB92／image136 eff_tenantStren，
锚点(120,149+map(counter,0,6,17,0,true))、裁剪(85,132,70,17)。四帧裁片及内部offset见机器映射。
81 `aK=[40,50]`，phase0不足40确认快进，否则phase1计数归0；phase1不足55快进，≥55才清L关闭。phase0首帧声音20。
上述响应文字、字体语言分支、数字弹动、原皮肤与全部声音只完成静态定位，不以当前文字原型宣布完整还原。

## 赠礼66及剩余缺口

65只回传购买／赠礼答案，父64恢复后才扣款／库存与奖励；地图商店cd15和66赠礼演出是不同序列。
66静态映射已有common73/present00.seb两帧（<45帧0，≥45帧1）及effect6/dropItem00.seb三帧闪光。
计数45–74显示18格商品图标，闪光frame=(counter-45)/3仅0–2时绘；原绘制在counter恰45调用声音8。
声音8存在绘制重复风险，不能无依据移到更新期或借只读查询播放。
完整人物身体／表情／HP响应、赠礼字布局、68／69属性结果皮肤、OS输入、绘制随机及自然玩家路线仍待独立接线认证。
本批正式接口只覆盖已有cd15／21／22和设施队首1–6，不创建新演出状态，也不把CPU素材消费当原APK动态或产品验收。
