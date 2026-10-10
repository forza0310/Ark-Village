# 产品源码导航

按游戏职责查找实现，公开接口在 `include/ark` 下对应目录。目录分组不增加世界所有者，也不等于一个目录对应一个 DLL。

| 入口 | 职责 |
| --- | --- |
| [simulation](simulation/README.md) | 建筑经营、人物、AI、战斗、任务、地图、村庄进程及唯一世界运行时 |
| [app](app/README.md) | 模拟线程、FIFO 命令、玩家存档、启动参数与只读查询 |
| [presentation](presentation/README.md) | 无 raylib 的只读表现计划和脚本文字处理 |
| [desktop](desktop/README.md) | Windows/raylib 资源、输入、场景绘制、页面 UI 与窗口诊断 |
| [assets](assets/README.md) | 标准 C++ 的素材元数据解析 |

依赖方向为 desktop → app / presentation → simulation。规则代码不依赖窗口、纹理或屏幕坐标；正常世界只有 `StartupWorldRuntimeState` 一个可变聚合所有者。模块间事务仍在原来的运行时/会话边界提交。

新增研究文件先在 `scripts/simulation/module_paths.mjs` 明确模块归属；导入器维护源路径与显式 CMake 清单。重排不改变源命名空间、存档字段、规则数值或随机调用顺序。重要注释说明所有权、输入身份、失败/回滚和调用顺序，不机械复述函数名称。
