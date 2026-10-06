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

日常从仓库根目录使用[研究聚合入口](../CMakeLists.txt)，与example、prototype共用一套Release构建树：

```sh
cmake -S research/dungeon_village_1 -B research/dungeon_village_1/work/release -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
cmake --build research/dungeon_village_1/work/release --parallel 2
ctest --test-dir research/dungeon_village_1/work/release -LE long-world --output-on-failure
```

聚合构建需要原型使用的Node、动态raylib及pkg-config，配置方式见[原型构建说明](../prototype/README.md#构建与运行)。
本包5项测试与87项领域、31项原型测试合计123项唯一标准测试；prototype包含的领域测试不重复注册。
日常标准测试排除`long-world`，长测按风险显式启用并单独执行。Debug仅按需使用`work/debug`，不再每批重复六套验收。
本包[CMake入口](CMakeLists.txt)仍可独立配置，不引入reference或窗口业务依赖；独立检查完成后不常驻重复缓存。
本机路径见 [环境基线](../verification/BASELINE.md)，新入口实际结果见 [当前验证](../VERIFICATION.md)。
测试分别覆盖归档、设施表、道具表的错误输入和实际源表；不把通过结构解析当作完整运行语义认证。

本地Debug／Release默认共享`dungeon_village_archive`，由[共同构建模块](../cmake/ResearchLibraries.cmake)管理。
工具与运行DLL统一放当前构建的`bin/`，导入库放`lib/`，四个工具不各自嵌入一份静态领域库。
不同构建树不共写DLL目录，按需Debug构建自己的匹配库；收齐进程后再清理不需要的缓存。
Windows复制并校验目标架构的运行库；LLVM-MinGW i686使用目标`i686-w64-mingw32/bin`，不是宿主x64的同名DLL。
旧`-static`缓存需在进程结束后显式清空（例如`-DCMAKE_EXE_LINKER_FLAGS=`）；配置不会悄悄覆盖它。
只有独立向玩家分发的Release制品才显式选择静态策略，本工具包日常Release继续动态链接。
