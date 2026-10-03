# Ark-Village

基于 `research/` 的逆向交付，以 **C++17 + raylib** 复刻《冒险迷宫村》一代的玩法、数值、UI与操作流程，并保留未来3D化的领域边界。

2026-10-03按用户要求重新建立产品。旧代码、文档、构建和Git历史已移出项目；research保持原样。
当前是有限的建造/首访原型：24×24源地图、5000G初值、七种单格设施建造、首名冒险者自动免费加入及属性查看。
数据来自research静态交付，画面参照最新prototype；尚非完整原版初始化快照、人物AI或经营闭环。

## 构建运行

需要CMake 3.21+、C++17、Node 18+（仅构建）、pkg-config及raylib 6.0+。macOS可用：

```sh
brew install cmake node pkg-config raylib
cmake --preset desktop-debug
cmake --build --preset desktop-debug --parallel 4
ctest --preset desktop-debug
./build/desktop-debug/bin/ark_village --paused
```

无窗口资源检查：`./build/desktop-debug/bin/ark_village --check`。
有界运行：`./build/desktop-debug/bin/ark_village --frames 60`。
CLion打开本目录的CMakeLists.txt，选择desktop-debug；其他IDE同样可使用CMake。
资源来自可执行程序旁的assets，不依赖当前工作目录或研究工具。

建设→分类/条目→点击地图格→确定；旋转切换两朝向，返回/Esc取消，右键拖动视图。
返回正常场景并继续后施工和到访计数才推进；首访两句提示逐次确认，人物已经加入，无需支付入住费用。
底部“冒险者”查看已到访人物。道路/入住募集暂不可用，人物首段AI/后续访客未接入；本轮在待研究月报前结束，可重新开始。
退出不保存。macOS默认使用系统Arial Unicode.ttf，其他环境加`--font /路径/中文.ttf`。
模块、研究差异与验收见[切片记录](docs/stages/B1-playable-prototype.md)。

## 目录

| 路径 | 用途 |
| --- | --- |
| research/ | 研究智能体维护的规则、示例、素材与截图；产品侧只读 |
| include/ark、src/world/facilities/people/app | 不依赖raylib的格子、设施、人物与唯一应用聚合 |
| src/assets | 维护SEB/TSV解析子集，标准C++ |
| src/desktop | raylib进程入口、窗口和图像生命周期 |
| assets/ | 必要运行资源副本及来源记录 |
| scripts/ | 发布数据编译与显式素材导入，不执行逆向 |
| tests/ | 当前产品的测试，不运行research测试代替产品验收 |
| docs/ | 三份概览：目标/架构、开发流程、计划/决策 |
| docs/reference/ | 持续增长的原版对照清单与研究交接需求 |

从 [目标与架构](docs/ARCHITECTURE.md)、[开发流程](docs/CONTRIBUTING.md)、[计划与决策](docs/MILESTONES.md)开始阅读。
近期任务见 [TODO](TODO.md)，具体复刻条目见 [原版对照](docs/reference/REFERENCE_CHECKLIST.md)。
