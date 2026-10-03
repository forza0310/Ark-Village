# 可玩研究原型包

本包将已维护规则组装成可测试的建设与人物自主访问闭环，不是原版新局或完整 UI 复刻。
领域聚合不依赖窗口，raylib 只处理输入、格子投影、插值与图片绘制；不加入产品主构建。

## 职责与依赖

| 文件/目标 | 职责 |
| --- | --- |
| [village.hpp](include/dungeon_village_prototype/village.hpp)、[village.cpp](src/village.cpp) | 唯一可变聚合所有者；建设/时间输入候选事务，布局变更取消旧目标，到达收入和完成计数分开 |
| [catalog.cpp](src/catalog.cpp) | 通过工具包严格解析设施表，仅接入定义28/29/36 |
| [asset_manifest.hpp](include/dungeon_village_prototype/asset_manifest.hpp)、[asset_manifest.cpp](src/asset_manifest.cpp) | 七个逻辑键与替换素材契约；拒绝不安全路径、重复键和不存在的文件 |
| [main.cpp](src/main.cpp) | 参数、有界无窗口自检/窗口演示、工具选择、地图点击、相机和RAII纹理/画布 |
| dungeon_village_prototype_model | 标准C++17模型；依赖 [example](../example/README.md)，复用 [tools](../tools/README.md) 的表解析实现 |
| dungeon_village_prototype | raylib可执行程序，旁置打包的素材与必要数据表 |
| [tests](tests/) | 模型组合、事务失败、时间分段、素材像素/替换和CLI边界；含18项领域测试共24项CTest |

具体目标与打包步骤见 [CMake](CMakeLists.txt)。运行只读程序旁的素材与表，不访问 APK 或反编译结果。
详细规则见 [规则入口](../rules/README.md)，来源/帧/锚点与换图见 [素材说明](../assets/README.md)。

## 构建与运行

从仓库根目录执行，需已安装raylib及pkg-config：

```sh
cmake -S research/dungeon_village_1/prototype -B research/dungeon_village_1/work/prototype-debug-llvm -DCMAKE_BUILD_TYPE=Debug
cmake --build research/dungeon_village_1/work/prototype-debug-llvm --parallel 2
ctest --test-dir research/dungeon_village_1/work/prototype-debug-llvm --output-on-failure
research/dungeon_village_1/work/prototype-debug-llvm/dungeon_village_prototype --paused
```

前三个工具选择旅店、双格旅店、咖啡厅；移动工具先选建筑再选目标格，旋转工具切换朝向，撤除工具删除。
Esc取消选择，右键拖动视图，暂停按钮继续模拟。人物自主选择设施，不由玩家指挥移动。
退出不保存。详细操作验收见 [试玩清单](../stages/PLAYTEST.md)。

仅检查模型可执行：

```sh
research/dungeon_village_1/work/prototype-debug-llvm/dungeon_village_prototype --check
```

`--demo`执行自动演示并有界退出，`--frames N`限制窗口帧数，`--orientation 0/1`选择初始朝向。
`--screenshot`只用于有界窗口验收，`--asset-root`切换同契约素材，`--data-file`指定设施表。
本机工具链与桌面限制见 [验证入口](../VERIFICATION.md)，不能将无窗口自检描述为真实输入通过。

## 明确的夹具与缺口

7×7地图、10000初始资金、两个人物、速度和600个100ms tick的月份均为夹具；初始建造后余额7300。
独占预约、仅类别1/2、访问计数重置、完成后回到接近格和统一旋转收费是原型策略。
到达收费使用维护规则，完成只计数，不执行未知退出效果、共享升级、任务或存档。
画布240×240、站姿角色与固定道路首帧不代表原作 UI、动画或道路建设；截图来源版本也不等于固定APK。

后续产品建设切片需要真实地图/初值与建设链证据，由研究交付、产品智能体接入，不能把本包夹具迁入产品当原作默认值。
