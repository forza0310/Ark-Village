# people

公开接口`include/ark/people/adventurer.hpp`区分定义ID与场景UID，保留首访职业、属性、装备和HP。
first_visit按已证初值创建角色并设置8192标志；出生点由app选择，UI不能注入属性。
人物先创建再触发对话；住宅入住、任务征集与使用设施的到达均不是这里的招募。
依赖world；完整AI、后续访客/成长待研究，不沿用夜骑士演示。
依据：[首名人物与加入顺序](../../research/dungeon_village_1/rules/STARTUP.md)。
