# 装备举物与设施通知演出

2026-10-08初批交付固定汉化重签APK1.0.8的装备举物、商品图标及设施属性增长／减幅头标映射。
2026-10-16补Steam2.56设施队首kind0完成／住宅和kind7商品NEW，复用既有Owner通知；只读查询及研究窗口消费完整0–7，原窗口动态与产品验收仍分别登记，不新增持久字段。

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
上述初批数值仍以APK为源；本页后继Steam具名入口与常量交叉在对应段落独立登记，用户异版本截图不参与帧号、数值或时点证明。本页不声明原游戏窗口动态通过或产品已消费。

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
L升级提示common79/tenant_bonus.seb属于另一分支，不由通知队首查询替代。旧`startup_world_facility_growth_draws`保持只返回kind1–6，遇0／7返回合法空向量；完整入口`startup_world_facility_notice_draws`复用该查询并补下述0／7，研究窗口使用完整入口，不叠画两份增长头标。

## Steam队首完成／住宅与商品NEW

固定Steam `Tenant.Draw` RVA`2C2590`在`102C2987–102C29BD`只取实例`tenantFukidashi +0x48`第0项，`102C29F6–102C2A3A`按其`[0]`类型分流；`[1]`是该条年龄，不是UserData公共帧。锚L由`GetCenterSpos` RVA`2C3D70`再经`Camera.ConvertScreenToCamera` RVA`29F3C0`取得，与前述增长头标相同；下表外偏移尚未含SEB内部偏移。

| 队首kind | 原请求 | 帧与外偏移 | 分支资格 |
| --- | --- | --- | --- |
| 0完成 | common SEB68 `eff_open00` | 首个满足age小于阈值的索引，过尾保持4；外偏移(0,−37)，`lineNo=−1`画全层 | 当前定义kind非12；`102C3041–102C31DD` |
| 0住宅完成 | common SEB91 `eff_open02` | 同帧、同外偏移、同全层 | 当前定义kind恰12，不按detail、名称或首次入住标志判定；`102C3113／102C31DF–102C320D` |
| 7商品NEW | common SEB97 `eff_equipStock` | frame0、layer0；外偏移Y=`−29−2×((age%12)/6)` | `102C2A49–102C2B41`，使用通知自身计数 |

完成阈值`C=[4,8,12,16,44]`已由Steam `Tenant.cctor` RVA`2C7370`独立解析：`102C742E`槽`110F4A54`经`InitializeArray`写静态`EFF_TENANT_OPEN_TIME +0`，fieldRef247→TypeDef4129／field25385，默认数据偏移3127453。因此age0–3／4–7／8–11／12–15／16起分别frame0／1／2／3／4，不能把末阈值44当成第五帧出现时刻。UP／DOWN两表分别在静态+4／+8，均由槽`110F5138`解析为既有`[2,4,6,9,12,15,43]`；本批不改1–6的已证公式。

`TF_END_TIME +0xC`由同cctor的`102C74CE–102C7668`构造为`[44,43,43,43,43,43,43,60]`。`Update_fukidashi` RVA`2C53E0`在`102C5490`先将队首年龄加1，再于`102C54CE–102C54E5`达到对应寿命时删除第0项，尾项本轮不推进。原Draw没有44／60上界守卫：正常Owner的完成年龄0–43、NEW年龄0–59；单独查询更大的非负年龄仍按末帧／12周期出图，不将这种条件输入冒称自然可达。APK `c/m.java:348–405,842–847`的队首、C／F和偏移局部一致；无新增计时、奖励、声音或随机调用。

完成SEB68／91各两层五帧，逐帧均有显式记录。layer0普通完成用image68的44×18条，frame0／1分别取x0／44，frame2–4取x88，内部偏移(−22,−18)；住宅用image134的32×14条，前三图x0／32／64，frame2–4共用末图，内部X为−16、Y依次−14／−16／−18。layer1共用image69：frame0–3源矩形依次(19,19,16,16)、(63,9,36,36)、(3,57,48,48)、(54,54,54,54)，内部偏移(−8,−13)、(−18,−25)、(−24,−33)、(−27,−36)；frame4为显式零宽高记录，星爆消失，不继承frame3。NEW单层源image150的(0,0,60,21)，内部偏移(−30,−21)，合外偏移后为(−30,−50／−52)。完整查询将0／7展开为裁片及最终offset，执行器不能再次加SEB偏移。

固定Steam common对象1148的SEB68／91／97与已出版APK同文件；image69／134也同字节。根image68与150则PNG字节和RGBA均不同，已分别出版[Steam完成图](../assets/steam-common/eff_open00.png)132×18及[Steam NEW图](../assets/steam-common/eff_equipStock.png)60×21，不能用同名APK图替代。这里只消费root自然尺寸；INF的`eff_open00.gif,e132x18`不是无条件缩放或语言选择声明。其标准尺寸仅基础`Language.English()`成立时由公共加载器设置，资源语言及线程gate沿[公共加载合同](DUNGEON_RENDER.md)独立处理；当前完整通知查询不新增语言上下文／缩放请求，也不宣称全语言变体、当前用户语言或原后端像素已认证。

## 设施77／81与人物66／68／69的现合同

原共享`o.ap[0/1/2][价格/品质/魅力]`是前值／后值／可见差，与人物六属性分开。75先消耗道具，76初始化提交定义共享J并保存ap，77不再加属性；81初始化扣旧等级阈值K、G至多+1并保存同ap。每实例邻接s独立，ap包含实际封顶／职业倍率／选中实例修正，不能直接用道具表增量或邻接裸增量代替。

[设施道具Owner](../prototype/src/startup_world_facility_items.cpp)维护这些事实及精确快照载荷；75目录、76投物、77结果的有序皮肤／持久首行／真实消费示例已交付，见[页面合同](PAGES.md)、[输入](INPUT_RENDER_REQUESTS.md)及[示例入口](examples/README.md)。81初始化、双阶段确认及已交付计划见[升级合同](STEAM_FACILITY_UPGRADE.md)，不再列为仅静态定位。APK `aI=[0,0,15,29,39,39,49]`、`aJ=[2,32,34,49,55]`及三行Y158+18×slot仍是有效局部交叉；价格差右190、品质／魅力差右198，值达到槽上限才显示MAX。Steam响应图、数字／差额及窗口资格以各现合同为准。

APK77的`ar=0／1／2／−1→frame1／2／0／3`使用common SEB92／image136，锚(120,149+map(counter,0,6,17,0,true))、clip(85,132,70,17)；不能把这张APK响应图直接指定为Steam翻译素材。APK81 `aK=[40,50]`，phase0首帧声音20，phase1计数归0、满55确认才清L关闭；现业务仍将真正升级与最终确认分开。

65只回传答案，父64恢复后才扣款／库存与奖励；地图商店cd15和66赠礼演出是不同序列。APK66的common73两帧（45前后切换）、effect6闪光和45–74商品窗口仍保留；闪光frame=(counter−45)/3仅0–2时绘。Steam66人物／回复／赠礼布局及68／69冻结结果皮肤已交付，现状和精确时序见[人物合同](STEAM_HUMAN_DETAIL.md)。66计数恰45的原Draw声音8由[显式表现请求](INPUT_RENDER_REQUESTS.md#框架准入与赠礼声音)隔离消费，包装守卫资格未由初始化状态代替；普通只读查询不发声。原OS、完整字体、整场景遮挡和自然玩家路线仍分别验收，CPU素材消费不等于原APK动态或产品验收。
