# Ark-Village

基于 `research/` 的逆向交付，以 **C++17 + raylib** 复刻《冒险迷宫村》一代的玩法、数值、UI与操作流程，并保留未来3D化的领域边界。

2026-10-03按用户要求重新建立产品。旧代码、文档、构建和Git历史已移出项目；research保持原样。
当前是有限的建造/首访生活原型：24×24加载后地图、5000G初值、七种单格设施建造、首名冒险者自动免费加入及属性查看。
数据来自research静态交付；UI已接入原版上下栏、五项主菜单、三分类目录和放置箭头，按截图及页面映射复核。
正常启动会推进首访；关闭两句教程后，人物自主选设施、寻路、收费、使用、退出并续活动。人物读取当前地图、设施等级/邻接与共享资金，玩家建设后重校验行程，日期与施工继续按既有资格推进。
仍是有限生活链：抽到未接活动或装备选择时保留实际请求并暂停该人物，窗口显示原因，日期/施工继续；这属于产品缺分支交接，不是原版等待行为。后续访客、完整月报、野外和围栏/外入口附加覆盖尚未接通。
人物脚底已对齐道路地块中心，移动播放源四帧步态，暂停冻结。寻路按研究优先道路但允许草地；原版朝向/帧速、建筑正门绑定和进出演出等待研究补齐。
候选生成、两级选目标、出发、运动与普通服务已接入正常Game；感知、战斗、救援、探索等独立模块不等于完整世界已运行。已交付规则优先组合到可玩路径，研究缺口与产品待迁入分别登记。
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
正常玩法使用不带参数的启动命令；等待首访并依次关闭教程，可观察人物生活链、建设、暂停/继续、倍速及缩放。
逻辑时钟与60FPS绘制独立：默认按原版整数47ms最小开始间隔更新，卡顿后不补算；人物每步仍移动6.7单位。`--tick-rate 1..240`仅用于显式固定频率实验（含最多8次补算），不是默认原版节奏；原版设备实际FPS/走格耗时尚待动态核对。
`--ai-preview`保留为严格未改图初局诊断：直接进入首访后，布局/日历固定、建设禁用；遇未接分支或1000轮诊断上限后结束，可重新开始。该上限不用于正常生活循环。
初局AI无窗口验证：`./build/desktop-debug/bin/ark_village --check-ai`。
该命令在私有会话中运行两个出生点×旅店/包子铺/武器店六条真实源表流程，验证自主选择、移动、收费、使用、退出与延迟效果；票号显式固定。本地随机消费与APK仍有差异，不用该诊断代替正常玩法验收。
正常窗口有界检查：`./build/desktop-debug/bin/ark_village --verify-play --frames 3000 --screenshot /tmp/ark-playable.png`。按真实47ms/420步首访运行，通过共享controller确认教程、建设包子铺及暂停/恢复，再检查人物访问/收入/退出和施工；这是引擎内坐标控制检查，不等于OS鼠标或原APK验收。
CLion打开本目录的CMakeLists.txt，选择desktop-debug；其他IDE同样可使用CMake。
资源来自可执行程序旁的assets，不依赖当前工作目录或研究工具。

菜单→建造→分类/条目（选中后再次点击或Enter）→点击地图格→再次点击同格或Enter建造。
旋转切换两朝向，四向箭头移动预览；返回/Esc逐层返回，右键拖动视图。
返回正常场景并继续后施工和到访计数才推进；首访两句提示逐次确认，人物已经加入，无需支付入住费用。
正常村庄画面左上角显示“运行中 · 暂停”或“已暂停 · 继续”，点击或按Space切换。
普通启动会自动推进；`--paused`仅用于主动暂停检查。菜单/建设/弹窗仍按原有资格暂停世界更新。
首名冒险者在420次有资格更新后到访，后续持续到访尚未接入。
菜单“信息”暂接已到访人物页；Tab切换1/2倍速，这是桌面适配入口。
道路/入住募集暂不可用，后续访客尚未接入；正常生活、日期与施工在首个月报准备前1456步结束本轮，可重新开始。未知人物分支只暂停该人物，不冒充完整经营世界。
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
