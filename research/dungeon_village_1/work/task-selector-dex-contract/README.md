# 任务选择器的固定DEX分支合同

2026-10-09。本批用当前固定APK的DEX替代缺失`UserData`历史fallback文件，重核真实任务选择入口。**六个特殊BOSS存在默认模式v0的普通产生路径；v1没有找到正向启用器不等于特殊BOSS不可达。** 当前维护选择算法与下列已核分支相符，没有据此修改代码、原表或黄金轨迹。

实际具名入口是`Lc/n;::f(I)Lc/k;`，不是凭旧命名猜测的`e(int)`。原表共81定义，选择器有三条直接出口路径：显式模式取u、默认G门槛特殊任务、普通探索／战斗。星级与探索chapter仍分开，不能把升星直接当BOSS选择条件。

## 证据与复算

[dex-audit.cjs](dex-audit.cjs)只读固定DEX、原表和维护文件，生成[DEX_SELECTOR.json](DEX_SELECTOR.json)。不创建伪造fallback，不把普通Java坏反编译重新作为期望值。

```powershell
node --check research/dungeon_village_1/work/task-selector-dex-contract/dex-audit.cjs
node research/dungeon_village_1/work/task-selector-dex-contract/dex-audit.cjs
```

| 身份 | 本轮值 |
| --- | --- |
| DEX SHA-256 | `b4386a0de612fe18196a390633607ef36c69c2a63881244832314e3bf4902d1a` |
| 方法 | `Lc/n;::f(I)Lc/k;`，method index1008，code offset546136 |
| 指令区 | 540个code unit／1080字节 |
| 指令区SHA-256 | `67776f0dde52ee6c76828502b2242f3724a7635a0c810813995a5fe05a4d2536` |
| 分支 | 54个直接跳转，全部目的地通过指令边界检查 |
| 有限核验 | 6个职责窗口、10项关键分支约束、唯一具名直接调用者的局部窗口 |
| 输出 | JSON 78299字节，工具限制96KiB；不导出第二份DEX |

pc以16位code unit计。`La/m.f`是任务chapter、`d`是kind、`o`是flags、`t`是任务完成次数；`Lc/n.x/w/G`分别是当前探索chapter、普通探索累计、任务池进度；`La/k.v/y`是怪物成长／胜利计数与介绍资格。`a/m.v`静态特殊选择模式与怪物`a/k.v`不是同一个字段。

固定APK与DEX在[产生者专题](../task-selection-producers/README.md)已做本轮逐字节绑定。这里8个源文件另存hash，81条仅派生ID／kind／chapter／monster／flags索引，不复制整张定义文本。APK静态、维护消费者和自然实跑分别计级。

## 分支合同与维护对照

| pc／窗口 | 当前DEX直接约束 | 对照现有维护实现 |
| --- | --- | --- |
| 18–38 | `a/m.v==1 && kind==1`才取`u[w]`，然后跳共用创建入口34 | `special_selection_mode/index/special_selection`分支一致；仍仅条件消费者，不认证自然启用器 |
| 39–74、537–539 | 按全部定义原序扫描当前chapter的flags2；不匹配时保留上个候选，因此最后匹配者获选 | `current=&d`原序覆盖一致；固定表每chapter恰一个特殊任务 |
| **60–62** | pc60读m.t到v5；pc62立即把flags读入同一v5，之间没有分支或其他消费者 | **没有“任务t未完成才可选”的条件**。不能从坏Java里多余局部变量推导资格；当前C++不以t过滤正确 |
| 75–96 | G<500或kind≠1走普通分支；否则当前特殊任务对应怪物k.v≤0直接选当前BOSS | 默认v0完整正向入口；不检查event60、y或星级 |
| 97–176 | 当前BOSS的k.v>0时，重新按全部定义原序收集flags2；以首项怪物v为cap，所有大于cap的怪物v降至cap | `special`目录、cap及共享monster_growth写回一致；包括未来chapter的特殊任务，不只当前chapter |
| 177–219、533–536 | 从最大整数开始；严格更小的怪物v才更新最优索引；相等跳过更新；选目录该项后跳34 | 严格`<`、平局保原序一致；这不是随机选择六BOSS |
| 221–333、530–532 | kind0先取`bx[min(w,5)]`，抽100；遍历同chapter／同kind且非flags2。非flags8追加普通池；flags8且命中时清w，pc305直接跳34 | rare阈值`0,0,10,20,30,100`与早返回一致；**没有再消费普通目录抽签** |
| 335–351 | 非kind0抽100；票≥40或事件60不为true时进普通战斗回退449 | `<40 && event60`短路次序一致；没有特殊BOSS分支时才消费这张票 |
| 353–447 | 清临时复发池，逆序扫描静态x；任务chapter≤当前chapter且对应怪物y；最多6项；非空才按池size抽签，空则449 | `replay_order.rbegin`、chapter／introduced投影、最多6及空池回退一致 |
| 449–528 | 清普通池，原序同chapter／同kind、排除flags2和flags4，然后按池size抽签 | pc479的`if-ne ->507`证明**不同chapter跳过**，不是普通Java错误显示的`chapter != current`准入 |

实际维护文件是[world_task_creation.cpp](../../example/src/world_task_creation.cpp)内局部`select()`及`prepare_world_task_creation()`；当前仓库没有名为`task_selector`的独立模块。Owner[任务适配](../../prototype/src/startup_world_runtime_tasks.cpp)负责提供真实定义、当前完成数、怪物v/y、共同随机与目录，并联合写回。维护对非法kind、缺目录、越界索引的显式拒绝是保护契约，不宣称原APK原生具备相同回滚保证。

任务完成次数t仍由真正任务成功增加，也在任务命名、后续难度和日历首次复发提醒中使用。这里只证明它**不是这个选择入口的未完成过滤器**，不将其删掉或从持久化移除。

## 默认v0的六BOSS正向链

普通任务真正成功沿`c/k.a(m)`增加全局成功数、普通探索累计和定义t；无仍存在的特殊任务引用阻挡时，探索给G加50、战斗加100。`G`跨300/400/500分别请求原提示29/30/31。维护对应[world_dungeon.cpp](../../example/src/world_dungeon.cpp)的`prepare_dungeon_task_success`，成功路径与页面提示不同阶段。

日历生成入口仍受日期、任务上限、重试和普通目录条件约束；进入选择前，`b/c.c(I)`在G≥500时把kind强制为1。但第一年显示月份≤8的早期探索限制在这个强制之后，不能只看G就说任意日期立刻生成BOSS。DEX唯一具名直接调用为`Lb/c::c(I)V` pc1307，证据附pc1266起的G门槛及早期覆盖窗口；维护同样先G、后早期规则。

在合法生成时点，给定`mode0、G>=500、kind1、当前chapter所对怪物v<=0`，选择器存在以下六条直接正向路径：

| 当前chapter | 所选任务ID | 怪物ID | 结果 |
| --- | ---: | ---: | --- |
| 0 | 39 | 30 | 当前首个特殊BOSS |
| 1 | 47 | 31 | 当前第二个特殊BOSS |
| 2 | 55 | 32 | 当前第三个特殊BOSS |
| 3 | 63 | 33 | 当前第四个特殊BOSS |
| 4 | 72 | 34 | 当前第五个特殊BOSS |
| 5 | 80 | 35 | 当前第六个特殊BOSS |

这条默认策略扫描全部定义，**不通过u[w]**。u已补齐是保留显式模式的必要接线，不能误称它是自然BOSS唯一入口。自然复发任务需要x/y，特殊BOSS的这条直接选择不需要它们。

实际特殊任务生成后，原日历消费者请求大臣对话／raw99、首次BOSS脚本与后续提示，才将G归0；没有实际生成地点／任务时不能提前清G。真正特殊任务成功先记原年月并把chapter加1封顶5；之后仍按清理前任务引用判断是否给G加点，不能提前移除当前特殊任务让它再次加100。高星／六BOSS全过程仍需要自然玩家路径认证，静态存在这六条路径不等于已经跑完。

## 81定义的分组覆盖与剩余界限

JSON按原表零基列3/5/11/16派生五组：普通探索27、稀有flags8探索6、普通战斗12、flags4复发30、flags2特殊6，总81。当前chapter普通池用相等，复发池用小于等于；BOSS重复策略扫描全部6项。不能把这三个范围合成同一个“所有已解锁任务池”。

本批已经重新证明历史坏Java的两项关键修正：稀有早返回、不等chapter跳过；同时说明m.t死读和默认v0特殊入口。没有发现需要改变现行维护算法的差异。缺失的旧fallback仍保持缺失，不以新造同名文件掩盖证据来源变化；以后复查可使用本专题具名方法hash与分支约束。

语法、10项字节／分支约束、54跳转边界和81定义分组检查通过。未构建、未运行CTest、未新跑游戏或长测、未提交；没有后台任务。来源scope不含Steam同逻辑认证，也不含反射／JNI特殊模式穷尽，更不覆盖原游戏自然通关。后续重点应是符合真实前置的积极任务路线与已认证快照尾段，而非为了走BOSS而伪造v1或修改G／chapter／金币。
