# 原始研究数据

本目录只保存固定一代 APK 必要的研究数据，不包含反编译实现。

- [来源清单](SOURCE.tsv) 固定归档、条目 SHA-256 和字节/行/列数量。
- [新局数据包](startup/README.md)交付24×24源地图、六张完整源表、指定对话与有前置的静态状态；不是原版运行快照。
- [设施表原件](original/tenantData.txt) 保持恢复出的 UTF-8 字节，不编辑、不换行规范化。
- [精灵绑定](SPRITE_BINDINGS.tsv) 保存普通/双格旅店和咖啡厅经地图显示记录定位的帧、图片、偏移及翻转。
- [字段、几何与经营说明](../rules/FACILITIES.md#definitions) 区分已证实语义与未知列。
- [设施事件说明](../rules/FACILITY_EFFECTS.md#events) 解释第 33 列结构、26 条延迟展示记录与实例门槛，保留未知指令。
- [道具原件](items/original/item.txt) 和 [独立来源清单](items/SOURCE.tsv)保存 36 行25列道具，
  [道具报告](../rules/FACILITY_EFFECTS.md#items)维护已解释字段/类别适配，其余字段仍保留。
- [严格读取器](../tools/include/dungeon_village_tools/table.hpp) 及测试独立构建，不进入产品运行时。

当前表有 85 行、36 列。[收敛版原型](../stages/history/R2-prototype.md#convergence)严格读取该表，只消费 ID 28/29/36
的已解释字段，建设/消费/维护、形状与两朝向已接入。旧 R1/R2-B 独立示例仍保留原夹具。
未知列不执行，不能把三类接入描述为全表/完整原作玩法已经实现。
导入产品前需要完整规则和映射；原包属于用户提供的汉化重签输入，本目录不声明再发行授权。
