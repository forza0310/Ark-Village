# 建筑与经营

`rules/` 负责到达收费、退出效果、邻接、店铺使用、住宅和魔法壶规则；本层 `startup_world_building/editing` 与商品/道具/商会消费者把动作提交到同一世界。

收费发生在到达，成长/装备等退出效果单独消费。地图与邻接经济一并准备后提交；仅刷新地表不能改变价格。公开接口位于 `include/ark/simulation/facilities`，规则仍链接 `ark_world_rules`，运行时接线仍链接 `ark_world_runtime`。
