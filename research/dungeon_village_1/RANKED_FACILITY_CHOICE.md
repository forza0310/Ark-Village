# 已排序设施候选格与多格计权

更新：2026-10-02；阶段 [R2-I](stages/R2-ranked-facility-choice.md)。承接 [类别计划](ACTIVITY_CHOICE.md)。
原来的常规 Java 输出中，多格目标循环缺失，不能只凭它推断去重/最小格。此次对固定 APK 使用同版
JADX 1.5.6 fallback 单类输出，保留原输出不动；新文件仅在忽略目录，不作为维护源码。

## 低层证据与复现

```sh
jadx --config none --decompilation-mode fallback --no-res --threads-count 2 \
  --single-class c.b \
  --single-class-output research/dungeon_village_1/work/activity-choice-fallback/Character.java \
  --output-dir research/dungeon_village_1/work/activity-choice-fallback \
  research/dungeon_village_1/maoxianmigongcun.apk
```

已存在输出时不要覆盖，复现改用新的工作目录。下述行号只对应这次低层输出，原常规行号保持不变。
fallback 保留寄存器/跳转标签，局部结论依两种输出交叉核对；仍不是原机运行验证或完整控制流证明。

| 局部 | 低层锚点 | 结论 |
| --- | --- | --- |
| 收集候选 | `Character.java:9437`，`o(int)` 的 `L6a→L9d` | 每个候选格追加定义；实例阶段 1 时追加实例并加类别计数，没有实例去重；没有实例的候选也加类别计数 |
| 权重累计 | `Character.java:3585`，目标 helper 的 `L12c→L149` | 每次出现加该定义 `I[2]`，即定义魅力 |
| 抽取出现项 | `Character.java:3609`，`L157→L180` | 按同类别定义魅力累计半开区间，抽出一个列表出现项 |
| 最终目标格 | `Character.java:3641`，`L191→L1c6` | 遍历所选实例全部 `z`，比较候选格坐标，保留最小候选列表下标 |
| 第一层特别分支 | `Character.java:3794`，`Lc2→Lf5` | 再次佐证门槛后检查类别 4 存在、调用类别 3，不修改 R2-H 结论 |

常规对应为 `c/b.java:2379-2407,824-881,930-938`。最小候选下标证据现在补齐，但上游候选
过滤、野外事件仍未全部恢复。后续 [R2-J](ACTIVITY_CANDIDATES.md)已核对普通候选过滤与交换排序的
具体平局次序；不稳定，不能改为稳定排序。寻路前驱与角色上层优先级仍有独立限制。

## 概率与目标格是两件事

同一实例若有两个合格可达占用格，就在列表中出现两次；四个则四次。定义魅力相同的两个实例，
候选出现数为 1 和 4 时，其权重贡献为 `charm` 与 `4 * charm`，不能按实例去重后各给一次权重。
这是局部列表构造与消费者得到的计权，不是整个游戏全场景概率：仍受前一级类别、资格和随机偏差影响。

抽中该实例较晚的出现项，最终走向的仍是该实例在候选排序里最靠前的占用格；
因此“随机抽格”和“随机抽实例后选优先格”不能混为同一结果。
选择使用定义魅力，不使用含邻接道路/来源修正的实例魅力。成本用于候选排序，不直接乘入魅力权重。

## 独立规则边界

输入为已经筛选和排序的设施格，不是所有可达格都自动有资格。示例验证身份、共享字段、坐标唯一和
成本非递减；同一实例可以多个坐标，阶段 1 才参与。其他阶段保存但不计权。
支持普通类别 1/2/6/8；3（住所/出口）、-1（地域回退）、4（按格计数）不借用普通加权规则。
负魅力、非法票号、总和溢出显式拒绝；这些安全处理不复刻 Java 回绕/异常后的部分状态。

调用方提供平局次序和物理绑定保证。组合测试用已维护地图模块生成候选，并以格下标打破成本平局，
后者仅是 Ark 夹具；此函数本身不重排输入。旧可视模拟仍未接入多格、真实数值或该选择规则。

维护的 [接口](example/include/dungeon_village_reference/ranked_facility_choice.hpp)、
[实现](example/src/ranked_facility_choice.cpp)和 [测试](example/tests/ranked_facility_choice_test.cpp)
通过 1,026 项检查，包括全部普通支持类别、多格重复计权、100 组固定种子票号展开差分和地图到达组合链。

后续 [完整候选快照选择](SNAPSHOT_FACILITY_CHOICE.md)另行处理重复坐标/无实例/不可达事件，
不改变本阶段旧接口的严格输入契约；只有合法有限子集才可用于两接口差分。
