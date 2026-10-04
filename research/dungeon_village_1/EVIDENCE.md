# 证据清单与符号映射

## 输入与工具

| 项目 | 观测值 | 证据等级 |
| --- | --- | --- |
| 输入 | 研究目录内的 `maoxianmigongcun.apk` | 直接事实 |
| SHA-256 | `1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5` | 直接事实 |
| 包名 | `net.kairosoft.android.bouken_ja` | 直接事实 |
| 版本 | `1.0.8`，版本代码 9 | 直接事实 |
| 运行时形态 | 单个 Dalvik/Java 应用，不含原生库 | 直接事实 |
| 签名/来源限制 | 包含汉化内容并使用 Android 调试签名证书 | 直接事实 |
| 反编译器 | Homebrew 安装的 JADX 1.5.6 | 直接事实 |
| 反编译规模 | 188 个类、1,953 个方法、295,672 条指令，并生成调用图 | 工具报告 |

包身份和哈希将所有结论固定到这一输入。调试证书和汉化字符串意味着不能把该安装包描述为未经修改的
官方发行版。

新局七类缺口的本轮研究见 [新局、到访与建设](rules/STARTUP.md)，原始字节与推导分开发布到
[新局数据包](data/startup/README.md)。地图保留未消费的两个零字节，不作为完整格式已闭合的声明。
首名自动到访是人物表flags8命中的ID1；首次教程对话ID69与页面69、普通解锁页59分别登记。
既有低层 `UserData.java` 的 `L206→L244→L450→L469→L4c9`佐证创建、加入和事件89的局部顺序。
加载后格/身份的静态重建使用新增 `work/startup-map-fallback/Map.java`，
JADX参数为 `--single-class c.h --single-class-output <路径> --decompilation-mode fallback --no-res --log-level warn`。
输出SHA-256为 `faa6ac955724a8d32937a60b45a40c87cace6c0274a9a2a5ca6b3f1c0f84f6fe`，仍不提交生成Java。
低层修正常规g()循环跳转误读，c()/d()/实例分配小函数交叉定位见[新局报告](rules/STARTUP.md#加载后逻辑显示快照)。
运动/路点/逻辑格进入复用既有Character低层输出，数值与巨型P()局部证据分级见[连续运动](rules/CHARACTERS.md#continuous-motion)。

本轮新增单类fallback：`work/first-visit-fallback/MainScene.java`（b.c）与
`work/weapon-choice-fallback/Weapon.java`（a.p），参数均为
`--config none --decompilation-mode fallback --no-res --single-class <类> --single-class-output <路径>`。
SHA-256依次为`cadcb21544111c305214c67f4e25724a17ada111b3c728f7ed20b9772ddf21bb`、
`abeb2ed48ccac67c702cbca1c6113d9e5e678c9addfea35273b750e1cba3c66d`。
主场景自动扫描/延迟局部见[首访前置](rules/CHARACTERS.md#first-activity)，
武器消费者及A[0]赋6见[武器报告](rules/FACILITY_USE.md#weapon-choice)，
道路补块复用既有Map低层与小型图片/深度消费者，见[绘制交付](ui/README.md#road-patches)。
生成Java保持忽略、不提交；局部交叉证据不消除整套巨型方法的警告。

2026-10-04人物/怪物AI专项新增`work/ai-fallback/Encounter.java`（c.f）和
`work/ai-fallback/Projectile.java`（c.j），参数为
`--config none --decompilation-mode fallback --no-res --log-level warn --single-class <类> --single-class-output <路径>`。
SHA-256分别为`6926d7ff344accf558986f1c2fa1e44d97227a1208ee5f5298490c626f87799c`、
`b386c299543a294b012361a42ab3256132a7f557dc1bb6e226fa1f2ee84d18c6`。
复用Character低层哈希`4bd10321f4a6a256664f5700dcc9dd71638e3c66c4fb9cb5634b1d96d712a3bd`和既有Map低层。
[AI入口](rules/ai/README.md)逐功能记录普通/低层定位、维护C++职责、未闭合副作用与组合条件。
F/G差异、o活动6条件、d的8且9条件、攻击整数除法、影响场除2掩码已有低层交叉；
事件完整奖励已与低层740–1513交叉，e.K近期击杀数和N/O职业成长消费者另由小方法佐证；
状态setter、漫游与装备提交扩展见[控制规格](rules/ai/CONTROL.md#状态setter和装备提交)。
完整全局运行仍不以一份普通输出宣称等价。生成文件只留忽略work，不纳入发布源或Git。

同参数新增`work/ai-fallback/WorldObject.java`（c.e），SHA-256
`9db7e7d2375b48d14ed3183ff2f62e82f6e8b05a80e3bd2829a3655140d90075`。
物体奖励先增加计数再判20/60、普通物品提示先于授予、装备相反、全局E取模与事件151条件已低层交叉。
攻击14共同尾部/弓无目标早停、17旧au及救援递归到达先清引用后复制位置，复用上述Character低层核对。
聚合更新复用`UserData.java:13364–13587/L548→L709`，纠正常规人物d删除条件和旅店S可用判断，
证实怪物d正向删除跳过与同轮追加可见性，见[调度规格](rules/ai/LIFECYCLE.md#聚合遍历与同轮新建)。
显示/延迟队列及表情复用`Character.java:14683–14744,16515–17045`，
防具/饰品复用`Weapon.java:1171–1287`，修正常规防具rank条件反写并保留不同空当前回退。
对应[控制时间线](rules/ai/CONTROL.md#显示与延迟效果的实际推进)及
[装备选择](rules/FACILITY_USE.md#防具饰品选择与收费前置)与独立C++同步维护。

人物定义新增`work/ai-fallback/HumanDefinition.java`（a.e），参数同上，SHA-256为
`41dfeff5e26bb642743ae090aec4523ccfb75ec40fbc99c8e5760c3d56e6a75e`。
属性重算1098–1393、职业成长1677–1964，与常规`a/e.java:146–152,304–396,498–613`
及`c/b.java:5856–5868`成长显示交叉；逐职业截断、满级扣费、最后一级提示与职业解锁顺序见
[定义成长](rules/ai/CONTROL.md#人物定义重算与职业成长)。生成Java仍只留忽略work。

同日继续复用上述生成证据，没有重新反编译或修改输入。
完整服务安排/退出前部见[设施事务](rules/FACILITY_USE.md#设施服务组合与首段私有所有者)，
状态18及d尾部复用`Character.java:16350–16443,17030–17365`，见[共用更新](rules/ai/LIFECYCLE.md#共用更新前缀与尾部)。
重复参战匹配、定义p只清J/K及全局N只追加ID，分别核对命中低层`Ld0→Lf2/L19a→L1bc`、
`a/e.java:817–820`、`c/n.java:2483–2485`，见[战斗事务](rules/ai/COMBAT.md#跨所有者战斗提交)。
授予目录消费者`a/g.java:173–188,a/p.java:182–192,a/b.java:60–67,a/a.java:60–67`及商店通知
`a/o.java:711–735,c/m.java:832–840`为小方法直接事实；直接授予/拾取入口差异见[拾物提交](rules/ai/LIFECYCLE.md#拾物与目录实际提交)。
怪物创建`c/n.java:518–535,1663–1669`、原UID首个空槽`c/b.java:2202–2227`、
尸体旧B12与先事件d/后增长e、组返回值忽略/引用保留、完成量f215e以及定义成长共享容量见
[奖励所有者](rules/ai/ENCOUNTERS.md#死亡战斗组与奖励所有者组合)。月度A槽扣减见`b/c.java:259–268`，
不擅自挪到每次访问或AI tick。有限组合/稳定ID/异常拒绝仍与原版完整世界、Java回绕及动态轨迹分级。

本批继续复用同一生成输入，普通创建守卫/数量/实例安装见`c/f.java:58–153,c/k.java:106–147,c/b.java:2248–2289`，
与[创建事务](rules/ai/ENCOUNTERS.md#普通遭遇创建事务)及C++同步。原事件ID0/取模、前期强制一只仍消费抽取逐项记录。
普通救援返程的活动4用Character低层`L32c→L363`核对成本严格最小、首平局及完整dk，而不是随机dl；
递归到达/共享B2与普通旅店使用见[救援所有者](rules/ai/PERCEPTION.md#普通旅店世界事务)。
R/S/db/dc、组及投射引用寿命与当前名单身份分开，见[对象可达性](rules/ai/ENCOUNTERS.md#对象引用可达性)。
当前攻击控制14–17继续对照既有低层；武器p.h与当前动作k时间端点独立，原`bc`表121–124交叉。
回复小函数682–690为直接事实：显示追加、HP改变及倒地B提前；大控制消费者仍保留局部控制流推断等级，
见[实际控制提交](rules/ai/COMBAT.md#控制14至17的世界提交)。本批没有新反编译、原表修改或固定APK动态认证。

本轮继续复用上述固定生成输入：共同c前段`c/b.java:4807–4880`，d前段/物理投影
`5446–5675`，K`476–500`，e`5754–5816`、J`461–474`，转换`c/a.java:148–167`。
普通e/F/事件计数比较原`f164b`而非维护稳定对象身份，退休引用按原对象关系保留；
怪物初始中心缓存/随后偏移分开见`c/n.java:1663–1669,518–535`。
小函数n只写k/l（`c/b.java:2300–2303`）；低层`Character.java:9371–9377,14422–14465`
交叉b不清i，opcode3消费者却另清i。由此修正维护基线与救援修复的实现/预期。
九方向移动`c/b.java:1881–1944,2030–2107`，半格中心`c/h.java:308–315`；
影响场快照`c/f.java:327–333,459–466`、初始零数组`42–45`。
任务配额`a/m.java:105–107`、地图bit2刷新`b/c.java:2227–2249`为小函数直接事实；
F新建后调用顺序/事件完整组合仍保留巨型方法局部推断等级。详见
[共同感知](rules/ai/PERCEPTION.md#共同世界感知与缓存)、[实际执行前段](rules/ai/LIFECYCLE.md#实际执行前段与解释器续行)、
[策略世界提交](rules/ai/COMBAT.md#状态1策略的世界提交)和[任务与场事务](rules/ai/ENCOUNTERS.md#任务创建与影响场地图事务)。
无新增截图输入或反编译实现复制；六套C++验收与APK动态/窗口验收仍分别记录。

## 稳定符号映射

下列名称是根据残留诊断字符串、序列化职责和调用关系建立的工作别名，不是恢复出的 Java 原始类型名。

| 混淆类型 | 工作别名 | 直接锚点 |
| --- | --- | --- |
| `c.b` | `Character` | `c/b.java:132` 的状态标签表；3271-3287 的 `Character.java setGoTenant()` 诊断；4205/4310 的序列化 |
| `c.m` | `Tenant` / 设施实例 | `c/m.java:96` 的 `Tenant.java deserialize2()` 诊断；111 的实例初始化；223/279 的序列化 |
| `c.n` | `UserData` | `c/n.java:3950/3971` 的 `UserData.java update()` 诊断；2492/2573 的序列化 |
| `b.c` | 主场景与建造控制器 | `b/c.java:136,601` 的建造模式提示；1359 的校验；1702-1890 的提交分支 |
| `b.g` | 菜单/事件 UI 状态机 | `b/g.java:109` 的主菜单标签；1880 的月报入口；7010 的设施维护费显示 |
| `d.a` | 资源/存档/运行时协调器 | `d/a.java:3194-3268` 将角色和运行时向量写入存档分区 |
| `c.l` | `RouteSearch` 工作别名 | `c/b.java:3265-3267` 调用；3271 的失败诊断直接命名 |
| `c.i` | 地图格 | `c/i.java:6` 路径类别矩阵；56-133 的清空、设施和道路绑定 |

## 证据规则

1. **直接事实**：在固定 JADX 输出中可见的字段读写、调用、分支条件、序列化操作或字面量。
2. **控制流推断**：将多项直接事实组合成职责或时序结论。混淆名称和带反编译警告的方法必须使用该等级。
3. **Ark 建议**：[example/](example) 中独立作出的设计选择，不声称属于原作行为。

大型方法 `Character.o(int)`、`Character` 控制方法、`UserData.e()` 和 `b.c.b()` 带有 JADX 的重复代码块、
移除指令、嵌套 `try` 或类型推断警告。诊断字符串和较小被调方法可以佐证其总体职责，但具体分支优先级
不是直接事实。存档读写方法和较小的 `Tenant` 方法反编译较干净，可信度更高。

## 生成证据位置

生成证据有意保持在忽略目录中：

```text
work/decompiled/sources/c/b.java       Character
work/decompiled/sources/c/m.java       Tenant
work/decompiled/sources/c/n.java       UserData
work/decompiled/sources/c/l.java       加权寻路与前驱回溯
work/decompiled/sources/c/i.java       地图格类别与准入矩阵
work/decompiled/sources/c/d.java       邻接方向与随机 helper
work/decompiled/sources/b/c.java       主场景/建造控制器
work/decompiled/sources/b/g.java       UI 状态机
work/decompiled/sources/d/a.java       存档/运行时协调器
work/decompiled/callgraph.json         JADX 调用图
```

本研究中的行号对应由固定 APK 和 JADX 1.5.6 生成的输出。更换参数或 JADX 版本重新生成后，行号可能变化。

自主行动新增定位与证据限制集中在 [人物自主行动报告](rules/CHARACTERS.md)，不提交上述生成 Java。

设施表加载、字段消费者和两朝向几何定位集中在 [设施报告](rules/FACILITIES.md#definitions)。
维护数据来源见 [清单](data/SOURCE.tsv)，原始字段与定义 ID 不作为产品自行推测规则的授权。

周期费用、五类统计、双资源与报表局部时序定位见 [周期账本报告](rules/ACCOUNTING.md)。
其中金币/点数消费者为小函数交叉证据；月报完整顺序和讨伐退出仍按带警告控制流推断处理。

地图状态/路径类别、占用格活动目标、全图连通标记和到达身份定位见 [地图访问报告](rules/MAP_ACCESS.md)。
辅助周边表用于邻接修正，不作为入口证据；主场景完整建设条件仍带警告限制。

`a/o.java:399-490` 的重算/去重/道路魅力消费者及第 26/27 列解释见 [邻接属性报告](rules/FACILITIES.md#neighbourhood)。
维护示例用稳定实例 ID 记录来源，不复刻定义加同类序号的显示引用或变化提示动画。

设施事件语法、小型计数门槛、延迟记录和传说商店展示定位见 [设施事件报告](rules/FACILITY_EFFECTS.md#events)。
opcode 6/40 与 UI 的整体关系按带警告局部推断，小函数/固定原表分别核对；不复制完整解释器。

道具表、按类别适配、共享改良与库存消费者见 [道具改良报告](rules/FACILITY_EFFECTS.md#items)。
小型属性/库存函数与 UI 局部顺序分级记录，候选事务是 Ark 安全设计，不是照搬原作实现。

小型类别计划、访问计数与带偏差随机 helper 见 [人物活动类别报告](rules/ACTIVITY.md#categories)。
多格候选收集和特例过滤仍保留巨型方法限制；不以类别纯函数验收替代完整 AI 证明。

补充证据使用同版 JADX 的 `--config none --decompilation-mode fallback --no-res` 单类输出：
`work/activity-choice-fallback/Character.java` 与 `work/user-data-fallback/UserData.java`。
生成文件仍只在忽略目录，行号与常规输出不同，不替换上述原始定位。
局部寄存器/跳转交叉核对见 [多格选择报告](rules/ACTIVITY.md#ranked)和
[人气队列核对](rules/FACILITY_EFFECTS.md#events-人气队列低层核对)。这些局部结论不消除整个巨型方法的警告。

`work/candidate-filter-fallback/RouteSearch.java` 是 R2-J 对 `c.l` 的独立低层输出。
[活动候选报告](rules/ACTIVITY.md#candidates)记录扫描资格、事件追加、交换排序、类别 4 两套视图及寻路平局
局部证据；原型/原作路径完全一致仍未认证，不能从候选层通过外推。

R2-K 使用已有 `Character` fallback 的 4036 起较小到达记账 helper，与常规 986–1062 和三个
独立金币/角色统计/实例销售消费者交叉核对；具体分支、状态投影和未实现路径见
[普通到达报告](rules/FACILITY_USE.md#arrival)。原有状态机警告仍保留，不将候选验证当全状态认证。

R2-L 连接已有 `Character` fallback 9437 收集与 3585/3609/3641 的普通选择消费者，
保留完整候选/阶段 1 活动实例两套视图，见 [快照选择](rules/ACTIVITY.md#snapshot)。
局部计权包括重复/不可达事件，不借此声称完整事件使用路径已恢复。

R3-A 继续复用已有 Character fallback `Lc7c/Ld72/Lda0`，与较小的 `a/o.java:833-839` 使用累计、
`a/e.java:270-276,485-487` 满足度、`c/d.java:118-131` 整数插值交叉核对。
UI 81 初始化/关闭及升级 helper 只记录局部顺序，不把它当自动升级或已实现存取，见
[退出报告](rules/FACILITY_USE.md#exit)。本阶段没有运行新反编译任务。

R3-B 复用 Character fallback `17566 g(int)`、`17800 s()` 和 `16495 d(int)`，核对六槽生命值
协议；巨型 `c()` 的 `L969→L995` 只作旅店恢复调用点局部证据。常规小函数、静态计数表与实际
绘制读取分别交叉记录，见 [生命值报告](rules/FACILITY_USE.md#hp)。不从显示协议外推全游戏时钟。
