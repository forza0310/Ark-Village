# people

公开接口`include/ark/people/adventurer.hpp`区分定义ID与场景UID，保留首访职业、属性、装备和HP。
first_visit按已证初值创建角色，标志为2|8192，位置为出生格中心，pending_activity保留0；出生点由app选择，UI不能注入属性。
人物先创建再触发对话；住宅入住、任务征集与使用设施的到达均不是这里的招募。
motion.hpp/cpp独立实现6.7单位运动、入口±40偏移、向零取整和移动前按逻辑格检查进入。
plan_travel/advance_travel仅组合显式目标与路径；返回候选和一次性进入信号，不预约、收费、选目标或退出。
过期身份/被阻断的路径安全停止是产品保护策略；自动重规划未实现。默认人物仍停在出生格。
依赖world；完整AI、后续访客/成长待研究，不沿用夜骑士演示。
依据：[首名人物与加入顺序](../../research/dungeon_village_1/rules/STARTUP.md)、[普通运动](../../research/dungeon_village_1/example/src/character_motion.cpp)。
