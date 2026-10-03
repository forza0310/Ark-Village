# 资源与数据研究工具包

本包使用标准 C++17，负责严格读取资源，不负责 UI、人物、模拟或产品状态。
[接口](include/dungeon_village_tools/)、[实现与命令行入口](src/)、[测试](tests/)保持同一包边界。
[CMake](CMakeLists.txt)提供 `dungeon_village_archive` 库、四个命令行工具及5个 CTest 条目。

## 入口与格式

| 入口/源文件 | 职责 | 依据与边界 |
| --- | --- | --- |
| archive、sha256 | 原始字节、游戏CRC、循环XOR、大端归档、视觉条目提取、来源哈希 | [素材说明](../assets/README.md)；标准CRC和游戏CRC不能互换 |
| sprite | 旧SEB图层/记录读取 | [SEB规格](../assets/README.md#seb-格式与证据边界)；`legacy_tag`保留原位，不比较记录数 |
| table | UTF-8/TSV、整数列表、事件矩阵、36列设施/25列道具投影 | [数据来源](../data/README.md)；未知字段保留，不推定业务语义 |
| kairo_asset_extract / main | 解码并列举归档，或导出视觉条目 | `--list-only`只读，非视觉地图不能经视觉发布获得 |
| kairo_seb_inspect / seb_inspect | 单文件或目录批量结构检查 | 不执行压缩SEB、复合绘图命令或关键帧动画 |
| kairo_table_inspect / table_inspect | 设施表字段投影与检查 | 不修改源表 |
| kairo_asset_publish / publish_assets | 确定性发布清单、原始视觉文件和七个逻辑键 | 源哈希、PNG头/帧与SEB绑定核对；像素解码由原型素材测试负责 |

解析失败通过异常报告，不返回部分解析成功结果；文件写入拒绝覆盖不同内容。
APK、源素材和生成 Java 保持只读，工具输出到独立工作目录。地图文件内部格式尚未作为本包交付。

## 构建与检查

从仓库根目录执行：

```sh
cmake -S research/dungeon_village_1/tools -B research/dungeon_village_1/work/tools-debug-llvm -DCMAKE_BUILD_TYPE=Debug
cmake --build research/dungeon_village_1/work/tools-debug-llvm --parallel 2
ctest --test-dir research/dungeon_village_1/work/tools-debug-llvm --output-on-failure
```

Release使用独立目录。本机路径见 [环境基线](../verification/BASELINE.md)，结果见 [当前验证](../VERIFICATION.md)。
测试分别覆盖归档、设施表、道具表的错误输入和实际源表；不把通过结构解析当作完整运行语义认证。
