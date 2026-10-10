# 地图与路线基础

`rules/` 提供格几何、占地、可达性、导航及地图刷新；`startup_map` 恢复已发布初图，projection/expansion 负责世界地图投影和扩张。

这里使用逻辑格和世界坐标，不依赖 raylib 或屏幕拾取。建筑费用与移动/拆除操作在 facilities 提交；全世界地图由唯一 Owner 持有。
