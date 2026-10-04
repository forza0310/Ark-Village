# Ark-Village

基于 `research/` 的逆向交付，以 **C++17 + raylib** 复刻《冒险迷宫村》一代的玩法、数值、UI与操作流程，并保留未来3D化的领域边界。

2026-10-03按用户要求重新建立产品。旧代码、文档、构建和Git历史已移出项目；research保持原样。
当前是有限的建造/首访原型：24×24加载后地图、5000G初值、七种单格设施建造、首名冒险者自动免费加入及属性查看。
数据来自research静态交付；UI已接入原版上下栏、五项主菜单、三分类目录和放置箭头，按截图及页面映射复核。
已保留加载后逻辑格与实例身份；围栏/外入口附加覆盖、默认人物AI及完整经营闭环尚未接入。
候选生成、两级选目标、感知/目标优先级、出发与本地控制队列已接入纯C++模块，默认自主运行的完整更新与使用退出事务仍等待研究补齐。
实时名单调度、人物效果时间线、装备选择、定义共享属性/职业成长已接入独立模块与组合测试；尚未接通到正常游戏中的完整人物更新。
寻路与连续运动已作为独立模块接入，可用有界检查查看显式目标运动：`./build/desktop-debug/bin/ark_village --inspect-page motion --frames 120`。

## 构建运行

需要CMake 3.21+、C++17、Node 18+（仅构建）、pkg-config及raylib 6.0+。macOS可用：

```sh
brew install cmake node pkg-config raylib
cmake --preset desktop-debug
cmake --build --preset desktop-debug --parallel 4
ctest --preset desktop-debug
./build/desktop-debug/bin/ark_village
```

无窗口资源检查：`./build/desktop-debug/bin/ark_village --check`。
默认窗口1080×720（3:2），可拖动调整，或用`--size 宽 高`覆盖；Retina按原生像素渲染。
有界运行：`./build/desktop-debug/bin/ark_village --frames 60`。
初局AI无窗口验证：`./build/desktop-debug/bin/ark_village --check-ai`。
该命令在私有会话中运行两个出生点×旅店/包子铺/武器店六条真实源表流程，验证自主选择、移动、收费、使用、退出与延迟效果；票号显式固定，正常窗口尚不运行这条链。
CLion打开本目录的CMakeLists.txt，选择desktop-debug；其他IDE同样可使用CMake。
资源来自可执行程序旁的assets，不依赖当前工作目录或研究工具。

菜单→建造→分类/条目（选中后再次点击或Enter）→点击地图格→再次点击同格或Enter建造。
旋转切换两朝向，四向箭头移动预览；返回/Esc逐层返回，右键拖动视图。
返回正常场景并继续后施工和到访计数才推进；首访两句提示逐次确认，人物已经加入，无需支付入住费用。
正常村庄画面左上角显示“运行中 · 暂停”或“已暂停 · 继续”，点击或按Space切换。
普通启动会自动推进；`--paused`仅用于主动暂停检查。菜单/建设/弹窗仍按原有资格暂停世界更新。
首名冒险者在420次有资格更新后到访，后续持续到访尚未接入。
菜单“信息”暂接已到访人物页；Tab切换1/2倍速，这是桌面适配入口。
道路/入住募集暂不可用，默认窗口尚未启用人物自主AI/后续访客；本轮在待研究月报前结束，可重新开始。
退出不保存。macOS默认使用系统Arial Unicode.ttf，其他环境加`--font /路径/中文.ttf`。
模块、研究差异与验收见[切片记录](docs/stages/B1-playable-prototype.md)。

## 目录

| 路径 | 用途 |
| --- | --- |
| research/ | 研究智能体维护的规则、示例、素材与截图；产品侧只读 |
| include/ark、src/world/facilities/people/economy/app | 不依赖raylib的格子、设施、人物与唯一应用聚合 |
| src/assets | 维护SEB/TSV解析子集，标准C++ |
| src/desktop | raylib进程入口、窗口和图像生命周期 |
| assets/ | 必要运行资源副本及来源记录 |
| scripts/ | 发布数据编译与显式素材导入，不执行逆向 |
| tests/ | 当前产品的测试，不运行research测试代替产品验收 |
| docs/ | 三份概览：目标/架构、开发流程、计划/决策 |
| docs/reference/ | 持续增长的原版对照清单与研究交接需求 |

从 [目标与架构](docs/ARCHITECTURE.md)、[开发流程](docs/CONTRIBUTING.md)、[计划与决策](docs/MILESTONES.md)开始阅读。
近期任务见 [TODO](TODO.md)，具体复刻条目见 [原版对照](docs/reference/REFERENCE_CHECKLIST.md)。
