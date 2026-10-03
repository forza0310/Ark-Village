# Ark-Village

基于 `research/` 的逆向交付，以 **C++17 + raylib** 复刻《冒险迷宫村》一代的玩法、数值、UI与操作流程，并保留未来3D化的领域边界。

2026-10-03按用户要求重新建立产品。旧代码、文档、构建和Git历史已移出项目；research保持原样。
当前只有启动底座：显示研究交付的标题背景、校验打包资源、验证启动参数。**尚无游戏世界、人物或经营玩法**，该画面不是已还原的原版标题菜单。

## 构建运行

需要CMake 3.21+、C++17编译器、pkg-config和raylib 6.0+。macOS可用：

```sh
brew install cmake pkg-config raylib
cmake --preset desktop-debug
cmake --build --preset desktop-debug --parallel 4
ctest --preset desktop-debug
./build/desktop-debug/bin/ark_village
```

无窗口资源检查：`./build/desktop-debug/bin/ark_village --check`。
有界运行：`./build/desktop-debug/bin/ark_village --frames 60`。
CLion打开本目录的CMakeLists.txt，选择desktop-debug；其他IDE同样可使用CMake。
资源来自可执行程序旁的assets，不依赖当前工作目录或研究工具。

## 目录

| 路径 | 用途 |
| --- | --- |
| research/ | 研究智能体维护的规则、示例、素材与截图；产品侧只读 |
| include/ark/app、src/app | 不依赖raylib的启动参数契约 |
| src/desktop | raylib进程入口、窗口和图像生命周期 |
| assets/ | 必要运行资源副本及来源记录 |
| tests/ | 当前产品的测试，不运行research测试代替产品验收 |
| docs/ | 新产品的目标、架构、决策、计划、对照及验证 |

从 [目标](docs/REPLICA_TARGET.md)、[架构](docs/ARCHITECTURE.md)、[里程碑](docs/MILESTONES.md)开始阅读。
近期任务见 [TODO](TODO.md)，开发流程见 [CONTRIBUTING](docs/CONTRIBUTING.md)。
