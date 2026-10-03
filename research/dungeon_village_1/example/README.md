# 独立领域规则包

本包只有标准 C++17，不依赖 raylib、窗口、图片、APK 或平台坐标。
公开接口位于 [include](include/dungeon_village_reference/)，实现位于 [src](src/)，对应回归位于 [tests](tests/)。
[CMake](CMakeLists.txt)提供 `dungeon_village_reference` 库及21个 CTest 测试程序，不加入产品主构建。

## 模块与依据

| 模块 | 职责与边界 | 规格 |
| --- | --- | --- |
| domain、simulation | R1事务、独占预约、单点自主模拟夹具；当前窗口不使用旧模拟 | [状态与建设](../rules/STATE_CONSTRUCTION.md)、[人物](../rules/CHARACTERS.md) |
| geometry、facility_economy、neighbourhood | 格坐标占地、经营推导、来源实例去重与道路魅力 | [设施](../rules/FACILITIES.md) |
| navigation、map_access | 加权寻路、完整设施绑定、距离场和到达身份 | [地图访问](../rules/MAP_ACCESS.md) |
| activity_choice、activity_candidates | 类别计划、候选生成/排序，票号由调用方注入 | [活动选择](../rules/ACTIVITY.md) |
| ranked_facility_choice、snapshot_facility_choice、regional_choice | 保留不同历史契约；重复计权、抽中项/目标格、六次区域回退 | [活动选择](../rules/ACTIVITY.md) |
| facility_departure | 普通设施分支的两级选择/回溯/绑定/初始朝向组合；不决定任务/救援/物体优先级、不执行运动或失败清理 | [首次活动](../rules/CHARACTERS.md#first-activity) |
| character_motion | 6.7世界单位位移、入口路点±40、矩形到达、向零截断和逻辑格进入；不收费/使用、不含表现投影 | [连续运动](../rules/CHARACTERS.md#continuous-motion) |
| facility_arrival、facility_exit、character_hp | 到达统计/现金候选、共享使用数/满足度请求、生命值目标与显示协议 | [设施使用](../rules/FACILITY_USE.md) |
| facility_use | 普通食物/旅店的占用请求、同轮等待扣减、旧计数170恢复与待处理退出；不提交完整退出或下一活动 | [使用计数](../rules/FACILITY_USE.md#use-timing) |
| facility_events、facility_items | 实例事件门槛、定义共享改良与库存候选 | [事件与道具](../rules/FACILITY_EFFECTS.md) |
| accounting | 即时金币、周期费用、报表快照、延迟点数与幂等身份 | [周期账本](../rules/ACCOUNTING.md) |

`prepare_*` 纯函数返回候选值，调用方负责跨域原子提交及事件去重。
`GlobalState` 是早期安全夹具，不能与当前原型聚合或原作初值混用；R1净额结算不能与即时现金账本叠加。
头文件注释维护输入、所有权、失败与返回语义，非直观计权/时点在实现处补注释；不逐行描述赋值。

## 构建与检查

从仓库根目录执行：

```sh
cmake -S research/dungeon_village_1/example -B research/dungeon_village_1/work/example-debug-llvm -DCMAKE_BUILD_TYPE=Debug
cmake --build research/dungeon_village_1/work/example-debug-llvm --parallel 2
ctest --test-dir research/dungeon_village_1/work/example-debug-llvm --output-on-failure
```

Release使用独立目录。本机显式工具链路径见 [环境基线](../verification/BASELINE.md)，
本轮结果见 [当前验证](../VERIFICATION.md)。测试覆盖严格错误、边界、重放、随机差分及组合链，
不替代固定 APK 动态认证。
