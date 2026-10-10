# Steam普通怪物开放、介绍与任务资格

2026-10-10。固定Steam2.56样本的两处怪物方法及共享状态helper静态交叉，共1,584字节；不是原窗口动态或整套Steam任务选择器认证。逐指令、字段、原文件hash和复算入口见[证据包](../verification/steam-monster-progression/README.md)。固定APK的完整具名字段扫描已在[任务产生者](../verification/task-selection-producers/README.md)完成，本批不重复它，也不要求研究自然经营到后期。

## 普通怪物按定义顺序开放

`Character2.GetAppearMonsterDataId()` RVA0x27E850先清临时候选Vector。随后沿原怪物定义顺序比较相邻两项`previous,next`，寻找同时满足以下条件的第一对：

- next的**怪物**flags不含4（BOSS），不是任务flags4。
- next.monsterRank≤当前UserData.questRank，即探索章节条件，不是村子星级。
- previous.state=1，next.state=0。

找到这对后检查previous.dieCnt_total≥8：满足才调用next.ChangeState(1)，随后结束这轮开放扫描；不足8也直接结束，不继续寻找后面可能达标的定义。因此不是一次批量开放所有满足阶段的怪物，也不是每八次全局胜利固定开放一个。代码读取的是该前一怪物定义的累计计数，和世界任务成功总数分开。

共享`ChangeState` RVA0x2116C0先写新state；只有旧0→新1时置NEW。IL2CPP把多种数据类的同实现合并到这个地址，反汇编打印为AsEventData或RankUpTerm只是同地址别名，不能据标签误判调用对象。本批同时核MonsterData具名别名及真实32字节状态写法。

## 选择池与介绍时点

开放扫描之后，逆原定义序收集state1、怪物flags不含4的项，最多5个，再调用一次`GameUtil.Random(candidate_count)`选择其一。刚开放的next参与本次候选；未介绍过不是入池条件，已介绍与未介绍项都可入池。本批只核这个调用位置与参数，未由此认证Random内部算法或原始seed。

选择后同时满足`!isAppearWindow && !flags1 && !UserData.IsSelectQuest()`才进行介绍：

1. execsScript非空时先实际调用定义脚本，popup=true、line0、talkCharaType2、talkCharaIndex为所选定义ID。
2. 创建SubForm类型89，monsterData绑定到该定义，并交AppData.Push。
3. 紧接着写isAppearWindow=true，返回所选定义ID。

**介绍资格在发出页面请求时就产生，不等待89确认，也不等待第一次击杀。** 正在进行所选任务时跳过介绍，不因此阻止选出怪物定义。原局部代码在Push返回后直接置标志，没有检查其bool返回；维护Owner的整体失败回滚是已有安全策略，不能反写为原程序也保证相同事务。

字段映射与APK局部对应：

| 意义 | APK | Steam字段／偏移 |
| --- | --- | --- |
| 已开放状态 | a/k.p | BaseData.state_／0x10 |
| 介绍已发出 | a/k.y | MonsterData.isAppearWindow_／0x68 |
| 此怪物累计成长／击败计数 | a/k.v | MonsterData.dieCnt_total_／0x5C |
| 怪物需要的探索章节 | a/k.h | MonsterData.monsterRank_／0x30 |
| 当前探索章节 | c/n.x | UserData.questRank_／0x60 |

不能把state1当成isAppearWindow=true。它们的用途、产生时点及持久化字段不同。

## 新局与复发关系

`MonsterData.NewGame()` RVA0x21C8D0清state、NEW、四个计数字段及isAppearWindow；flags1定义才同时置state1／NEW和isAppearWindow=true。这与APK reset的结果相符，Steam不重复执行APK源码中的第二次清y语句，不要求指令数量一致。

APK的a/k.y所有具名直接写点已核为新局、介绍及读档恢复；复发任务选择在event60、40票号、任务章节及逆序目录筛选之外还使用该y。维护`monster_growth.introduced`由介绍消费者写回，Owner再投影到`TaskCreationMonster.replay_available`。这条APK及维护消费链已有证据；本批Steam只交叉到介绍资格产生，不将其扩大为Steam完整复发选择算法已相同。

现功能测试主责为`encounter_creation_test::creation/synchronous_intro`及任务选择套件：覆盖开放参与本次选择、介绍在spawn之前、活动任务抑制介绍、相同定义只介绍一次、晚失败回滚和复发空池回退。本批没有改业务或新增重复测试；这些是维护契约覆盖，不是Steam动态证据。真实经营到各怪物、BOSS胜利与完整任务复发交产品长跑验证。
