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
3. **Ark 建议**：[example/](example/) 中独立作出的设计选择，不声称属于原作行为。

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

自主行动新增定位与证据限制集中在 [人物自主行动报告](AUTONOMY.md)，不提交上述生成 Java。

设施表加载、字段消费者和两朝向几何定位集中在 [设施报告](FACILITIES.md)。
维护数据来源见 [清单](data/SOURCE.tsv)，原始字段与定义 ID 不作为产品自行推测规则的授权。

周期费用、五类统计、双资源与报表局部时序定位见 [周期账本报告](ACCOUNTING.md)。
其中金币/点数消费者为小函数交叉证据；月报完整顺序和讨伐退出仍按带警告控制流推断处理。

地图状态/路径类别、占用格活动目标、全图连通标记和到达身份定位见 [地图访问报告](MAP_ACCESS.md)。
辅助周边表用于邻接修正，不作为入口证据；主场景完整建设条件仍带警告限制。

`a/o.java:399-490` 的重算/去重/道路魅力消费者及第 26/27 列解释见 [邻接属性报告](NEIGHBOURHOOD.md)。
维护示例用稳定实例 ID 记录来源，不复刻定义加同类序号的显示引用或变化提示动画。

设施事件语法、小型计数门槛、延迟记录和传说商店展示定位见 [设施事件报告](FACILITY_EVENTS.md)。
opcode 6/40 与 UI 的整体关系按带警告局部推断，小函数/固定原表分别核对；不复制完整解释器。

道具表、按类别适配、共享改良与库存消费者见 [道具改良报告](FACILITY_ITEMS.md)。
小型属性/库存函数与 UI 局部顺序分级记录，候选事务是 Ark 安全设计，不是照搬原作实现。

小型类别计划、访问计数与带偏差随机 helper 见 [人物活动类别报告](ACTIVITY_CHOICE.md)。
多格候选收集和特例过滤仍保留巨型方法限制；不以类别纯函数验收替代完整 AI 证明。

补充证据使用同版 JADX 的 `--config none --decompilation-mode fallback --no-res` 单类输出：
`work/activity-choice-fallback/Character.java` 与 `work/user-data-fallback/UserData.java`。
生成文件仍只在忽略目录，行号与常规输出不同，不替换上述原始定位。
局部寄存器/跳转交叉核对见 [多格选择报告](RANKED_FACILITY_CHOICE.md)和
[人气队列核对](FACILITY_EVENTS.md#人气队列低层核对)。这些局部结论不消除整个巨型方法的警告。

`work/candidate-filter-fallback/RouteSearch.java` 是 R2-J 对 `c.l` 的独立低层输出。
[活动候选报告](ACTIVITY_CANDIDATES.md)记录扫描资格、事件追加、交换排序、类别 4 两套视图及寻路平局
局部证据；原型/原作路径完全一致仍未认证，不能从候选层通过外推。

R2-K 使用已有 `Character` fallback 的 4036 起较小到达记账 helper，与常规 986–1062 和三个
独立金币/角色统计/实例销售消费者交叉核对；具体分支、状态投影和未实现路径见
[普通到达报告](FACILITY_ARRIVAL.md)。原有状态机警告仍保留，不将候选验证当全状态认证。

R2-L 连接已有 `Character` fallback 9437 收集与 3585/3609/3641 的普通选择消费者，
保留完整候选/阶段 1 活动实例两套视图，见 [快照选择](SNAPSHOT_FACILITY_CHOICE.md)。
局部计权包括重复/不可达事件，不借此声称完整事件使用路径已恢复。

R3-A 继续复用已有 Character fallback `Lc7c/Ld72/Lda0`，与较小的 `a/o.java:833-839` 使用累计、
`a/e.java:270-276,485-487` 满足度、`c/d.java:118-131` 整数插值交叉核对。
UI 81 初始化/关闭及升级 helper 只记录局部顺序，不把它当自动升级或已实现存取，见
[退出报告](FACILITY_EXIT.md)。本阶段没有运行新反编译任务。

R3-B 复用 Character fallback `17566 g(int)`、`17800 s()` 和 `16495 d(int)`，核对六槽生命值
协议；巨型 `c()` 的 `L969→L995` 只作旅店恢复调用点局部证据。常规小函数、静态计数表与实际
绘制读取分别交叉记录，见 [生命值报告](CHARACTER_HP.md)。不从显示协议外推全游戏时钟。
