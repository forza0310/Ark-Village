# 只读表现计划

`world_combat_visuals`、`world_rest_visuals`、`world_dungeon_visuals` 和 `world_overlay` 将真实世界字段转换为绘制计划；`script_text` 处理脚本标签和按调用方测宽换行，`world_rank` 输出原条件说明。公开头位于 `include/ark/presentation`。

这一层不加载 raylib、不持有纹理、不推进动画计数、随机或奖励。源像素锚点由 desktop 的场景绘制器转换成屏幕位置；现有 C++ 命名空间与调用协议保持。前三类计划和文字处理继续由 `ark_world_visuals` 提供，升星说明保留当前消费者链接方式。
