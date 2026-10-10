# 二星到魔法壶入口：功能覆盖审计

2026-10-10。按最新职责，research负责规则／功能／逻辑组合，产品负责真实世界长跑。本审计没有运行自然Driver或把历史自然轨迹作为新功能前置。固定来源仍为汉化重签APK1.0.8；Steam配方数字表相同不意味着整条Steam动态链已验。

## 现有主责已覆盖什么

仓库并不存在独立的`startup_world_village_activity_test.cpp`或`startup_world_magic_pot_test.cpp`；当前主责均在既有`prototype/tests/startup_world_pages_test.cpp`，继续扩该职责，不建逐功能target。

| 行为 | 现有主责／证据 | 审计结论 |
| --- | --- | --- |
| 第二星800/10/4/12条件，晋级事件选择 | `example/tests/world_calendar_tasks_test.cpp`的二星条件及五级晋升测试；`prototype/tests/startup_world_pages_test.cpp::rank_conditions/rank_promotion` | 纯条件与事件序已测；Owner原晋级组合主要使用首星夹具，不把它说成特定43→30链已测 |
| raw95类型11开放活动且不开展 | `example/tests/world_gift_page_test.cpp`、pages的`unlock_rewards` | 已覆盖通用类型11/NEW语义、满40确认、非现金副作用；Owner例子绑定活动0，未覆盖二星实际生成30 |
| 村办51/52/53/54生命周期 | pages的`village_activity_initialization/village_activity_pages` | 季度优先于点数、扣点与m/F时点、53满120、父答案、坏字段、暂停、退休已测，不重复排列 |
| 类型5/6壶效果 | pages的`village_magic_pot_pages` | 已覆盖28/29扩级及30导入、52先扣100点、119拒效／120提交、104晚失败整体回滚、219延迟、重复确认拒绝；30开放态在该测试是显式夹具 |
| 主菜单／开发菜单入口 | pages的`magic_pot_entry_and_retirement` | 已覆盖主菜单清bit2／开发入口保留、进入前真实process、初始化完整载荷、坏输入和四类载荷退休；解锁u低两位来自该测试夹具 |
| 投入、处理、发现、炼制及低层奖励 | 同文件其余`magic_pot_*`与example现facility_items规则套件；现persistence/restore套件 | 已有独立主责，本次不复制其数值、目录和回滚组合 |

实际消费者与正式[魔法壶合同](../../rules/MAGIC_POT.md)、[进程路线](../../rules/PROGRESSION_ROUTES.md)一致：rank1→2的事件43为`6,30&2,46&33,30`；脚本33生成95的`legacy_r=11/legacy_s=30`；满40确认开放活动30。活动30本身是kind6／100村子点数，只有51→52支付→53满120才置u bit1/bit2、执行104及首次219。**95的11、活动kind6、地图扩张kind3是三个不同命名空间／含义**。

## 真正缺少的组合与本批补充

原各套件分别给“已晋级”“活动已开放”“壶已开放”夹具，尚无单次Owner组合验证这三处分界接线。新增同文件`second_rank_magic_pot_unlock_chain`，起点只设置已初始化raw48的rank1、四条件缓存和200点开展预算；这明确是条件世界，不证明自然取得四宅十二成功八百人气。

此后不写活动30状态或壶权限，全部沿真实消费者：

1. raw48真正晋级到2；断言10/24按星级开放，而30仍未开放。
2. 有界页面回调推进原庆祝／延迟／对话；取得实际95/r11/s30，早确认只快进40，满40确认开放30，但不扣100点、不增开展次数、不置壶权限。
3. 返回场景后壶入口仍拒绝；从实际51目录找到30并经52扣点／增加m/F，53尚未完成时权限仍关闭。
4. 真实53更新到120并确认，置两权限、季度扣一次，沿104及219，再经父51正常返回。
5. 从真实主菜单入口开41，仅清提示bit2；实际ready41可返回，村办与壶的临时页载荷各自退休，保留共享壶状态和配方目录。

页面等待每个节点最多600次回调，53至120另设120上限；这是原模态计数的有限功能测试，不是年份经营或自然路线。只允许已核庆祝、消息与自动页面，不用任意generic确认掩盖未知页。typed声音保留到明确消费点；不改旧有效断言、不改表、不插演示数值来凑自然成功。

## 本批状态与剩余限制

已完成消费者与测试归属审计及上述一项组合。单Release既有pages目标编译通过；本批最后加入博物馆64商会精确边界后，定向CTest `dungeon_village_prototype.startup_world_pages`通过，2.08秒，日志见[独立末次输出](../../work/progression-feature-audit/pages-verified.log)。此前pages-final.log复制了被并发CTest覆盖的共享LastTest，不能作为pages证据；保留原文件并使用本次独立stdout结果。未新增target、CMake、业务状态、格式、自然Controller或存档；构建和测试进程均已退出，未跑其它世界轨迹。

实现排错保留两项具体原因：二星事件54的原`5,6`生成raw11/source_record6，不能只按普通dialogue识别，测试增加这一个已证事件消息消费者；返回场景时只找“首个非退休页”仍会提前越过95的实际载荷退休，现等待真实框架入口清理life4后才判场景就绪。没有放宽未知页兜底或删除退休断言；异常继续给出目标raw、实际raw/kind/lifecycle/ID。

原Steam完整字体／皮肤、物理输入、真实窗口交叉和产品世界持续经营属于不同验收。当前应用`StartupApplication`公开接口未转发魔法壶管理，Session与研究窗口已有入口；这是产品／应用接入层的明确边界，不应在本次Owner条件测试中伪称已完成应用转发，也不为此新增后期自然Controller。
