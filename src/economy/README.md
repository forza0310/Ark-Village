# economy

cash.hpp/cpp从研究d7ca763的accounting提取即时现金契约，核心只依赖标准C++17。
CashLedger拥有余额与按event_id索引的已提交记录；相同事件重试幂等，冲突ID、非法金额/类别或溢出不会改写余额/记录。
sequence为调用方顺序标签，初局AI使用获准轮次，不把它解释为日历月份。原契约允许支出后余额为负，建设审批另有余额检查。
app/initial_ai将账本与人物、占用、统计共同放在整轮候选中提交；不修改Game现有建设支出记录，不实现月报/结转/持久化。
该事件身份与原子提交是维护示例的所有者契约，不是新增玩法。
依据：[经济研究](../../research/dungeon_village_1/rules/ACCOUNTING.md)。
