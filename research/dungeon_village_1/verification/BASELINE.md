# R1 环境与历史基线

以下为当时记录，不代替当前检查。

本记录只适用于该目录的维护研究快照及其被忽略的 `work/` 输出，不代表 Ark 产品构建状态。

## APK 与反编译器

2026-10-02 已验证：

- 输入：`maoxianmigongcun.apk`
- SHA-256：`1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5`
- JADX：1.5.6
- 包名：`net.kairosoft.android.bouken_ja`
- 版本：1.0.8，版本代码 9
- 工具报告规模：188 个类、1,953 个方法、295,672 条指令
- 生成位置：`work/decompiled/`，包括 `callgraph.json`

APK 是带 Android 调试证书的汉化重签包。结果只标识这一输入，不代表《冒险迷宫村》的所有发行版。
带 JADX 重复代码块、移除指令、嵌套 `try` 或类型推断警告的方法，只用于得到经过佐证的总体职责和
控制流推断，不作为精确分支顺序的证据。

## 独立 C++ 示例

示例已在 Debug 和 Release 两种配置下分别完成配置、编译和测试：

```sh
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake \
  -S research/dungeon_village_1/example \
  -B research/dungeon_village_1/work/example-debug-llvm \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_MAKE_PROGRAM=/Applications/CLion.app/Contents/bin/ninja/mac/aarch64/ninja \
  -DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake \
  --build research/dungeon_village_1/work/example-debug-llvm --parallel 2
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/ctest \
  --test-dir research/dungeon_village_1/work/example-debug-llvm --output-on-failure

# Release 使用相同步骤，并改用 work/example-release-llvm。
```

最新执行使用 Homebrew Clang 22.1.8 和 CLion 自带的 CMake/Ninja。两套测试程序均输出
`50 checks passed`，两套 CTest 均通过 1/1。AppleClang、Clang 和 GNU 编译器配置均将警告视为错误。

## 源码与仓库卫生

- 示例头文件、实现和测试通过 `clang-format --dry-run --Werror`。
- 维护树检查未发现 `.class`、`.dex`、`.jar`、生成的 Java、调用图或反编译缓存；唯一例外是研究目录内
  有意保留的 APK，以及被忽略的 `work/` 树。
- 维护 Markdown 的本地链接均可解析。
- APK 是只读输入；分析命令只向 `work/` 写入。
- 反编译实现和生成缓存不进入维护研究源码；R2-A 用户授权的原始/规范化视觉素材已单独维护并记录来源。

## 尚未验证

- 未执行动态插桩、运行时 Hook 或 APK 修改。
- 未与未经修改的官方安装包进行逐字节或逐行为对照。
- 未恢复完整数值平衡表和状态机的每个分支。
- C++ 示例是独立编写的 Ark 设计参考，不声称与原作行为等价，也不是反编译程序的翻译。
