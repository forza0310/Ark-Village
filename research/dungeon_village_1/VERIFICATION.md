# 验证记录

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

## R2-A 本轮结果（2026-10-02）

阶段为 InProgress，不标记 Completed。范围限于研究；没有修改产品构建、Godot、产品存档或产品规则。
本轮使用 Homebrew Clang 22.1.8、CLion CMake/Ninja、raylib 6.0.0、pkgconf 3.0.7，macOS arm64。

### 无人值守研究开始时的重新验证

2026-10-02 在当前工作树重新运行 CLion `cmake --build ... --parallel 2` 和 `ctest --test-dir ... --output-on-failure`：
`example-debug-llvm`、`example-release-llvm`、`tools-debug-llvm`、`tools-release-llvm`、
`prototype-debug-llvm`、`prototype-release-llvm` 六套构建均成功（无需重编译），CTest 分别
1/1、1/1、1/1、1/1、2/2、2/2 通过。本次没有重新执行窗口鼠标验收，不能用测试替代 R2-A 剩余验收。
本地版本管理仅纳入 research 维护成果；[研究忽略规则](../.gitignore) 独立排除 APK 与工作缓存，
不依赖其他智能体尚未提交的根忽略文件。

版本基线为 `14ff0f8`，不包含根忽略文件或兼容 R1 跳转的未提交变化；没有切换共享分支或推送。
原始素材局部 `-text` 属性生效后，以 Git blob SHA-1 与工作文件逐一核对，761/761 字节一致。
研究 Markdown 本地文件链接检查 61 项通过；检查脚本首轮工作目录处理错误，修正脚本后重新验证，
不是研究文件缺失。以上检查不改变 APK 和生成证据。

| 验证项 | 最新结果 |
| --- | --- |
| 工具 Debug/Release | 各编译通过，警告视为错误；CTest 各 1/1，通过 84 项检查 |
| 原型 Debug/Release | 各编译通过，警告视为错误；CTest 各 2/2，包含 814 项素材检查、50 项领域检查 |
| SEB 全量 | 341 个文件通过严格结构解析；346 个图层、1228 条记录、最大 60 帧 |
| PNG 全量 | 398 张实际解码通过；371 张含透明像素；字节重复副本 0 |
| 素材发布 | 761 个原始文件、5 个规范化副本、761 行来源记录；最终清单重新发布一致 |
| SHA-256 | 空串、abc、多块及空字节向量通过；每个来源文件写入源哈希 |
| APK 复核 | SHA-256 与基线 `1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5` 一致 |
| 真实图形运行 | Debug/Release 均创建 OpenGL 4.1 窗口，载入全部逻辑素材，演示完成后退出码 0，窗口和 GPU 资源释放 |
| 演示流程 | 放置、重叠拒绝、移动、撤除、预约、行走、进入 in_use、效果 5、回 idle 通过 |
| 替换根 | 同一 Release 二进制载入隔离变色旅店/人物，演示仍通过；原始图片不改写 |
| 最终截图 | 6 项检查通过；720×720，3×3 最近邻块一致，非黑像素 518400，换图差异像素 21807 |
| 格式与编辑器诊断 | 本轮维护的 13 个 C++ 文件通过 clang-format --dry-run --Werror；研究工具/原型无编辑器诊断 |
| 维护树与链接 | 研究 Markdown 本地链接可解析；未混入生成 Java、DEX、调用图或临时写入文件；APK/中间截图仍被忽略 |
| 进程收尾 | 最后 pgrep 未发现原型、提取、构建、测试或批量图片检查进程；本轮所有执行任务已退出 |

### 可复现命令

以仓库根目录为工作目录。工具独立构建用 `tools`，原型独立构建用 `prototype`；不进入根 CMake。

```sh
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake \
  -S research/dungeon_village_1/prototype \
  -B research/dungeon_village_1/work/prototype-debug-llvm \
  -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_MAKE_PROGRAM=/Applications/CLion.app/Contents/bin/ninja/mac/aarch64/ninja \
  -DPKG_CONFIG_EXECUTABLE=/opt/homebrew/bin/pkg-config
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake \
  --build research/dungeon_village_1/work/prototype-debug-llvm --parallel 2
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/ctest \
  --test-dir research/dungeon_village_1/work/prototype-debug-llvm --output-on-failure
```

工具使用相同工具链参数，将源改为 `tools`、构建目录改为 `work/tools-debug-llvm`，无需 pkg-config。
Release 分别改为 `-DCMAKE_BUILD_TYPE=Release` 与独立的 `*-release-llvm` 目录。

```sh
research/dungeon_village_1/work/tools-release-llvm/kairo_seb_inspect \
  research/dungeon_village_1/assets/original
research/dungeon_village_1/work/tools-release-llvm/kairo_asset_publish \
  research/dungeon_village_1/work/r2-assets/extracted research/dungeon_village_1/assets
research/dungeon_village_1/work/prototype-release-llvm/dungeon_village_prototype \
  --demo --screenshot research/dungeon_village_1/work/r2-assets/final-original.png
```

最终替换夹具在被忽略的 `work/r2-assets/replacement-anchors/`；重新生成时选择不存在的新目录，
不覆盖已有夹具。生成与运行方式见 [素材接入说明](ASSETS.md)。画面像素检查：

```sh
research/dungeon_village_1/work/prototype-release-llvm/dungeon_village_asset_tests \
  --compare research/dungeon_village_1/work/r2-assets/final-original.png \
  research/dungeon_village_1/work/r2-assets/final-replacement.png
```

最终截图位于被忽略的 `work/r2-assets/`：`final-original.png`、`final-replacement.png`、
`final-debug.png`。已实际查看地图、道路、两类建筑和角色；未以旧的 DPI 错位截图替代最终结果。
CTest 的每次素材检查在对应忽略构建目录中生成独立 `asset-fixture-*`，不修改原始素材。

### 限制与失败记录

- 系统级 screencapture 报告无法从显示器生成图像；桌面输入接口多次异常退出。已改用 raylib framebuffer
  自截图验证画面，但没有绕过系统权限，也不能据此声称鼠标输入驱动已经通过。
- 首轮高 DPI 截图为 1440×1440 且显示区域偏移；已移除高 DPI 标志，最终截图恢复完整 720×720。
- SEB 数量相等假设和夹具字段顺序错误曾导致失败，恢复授权见 [阶段记录](stages/R2-art-assets-cpp-prototype.md)。
  C++17 初始化/捕获、CTest 注册等机械错误按用户最新指令自行修正，最终结果均重新执行。
- 沙箱内 shasum 的 Perl 动态库加载被拒绝，已在沙箱外重新执行，APK 哈希一致。
- 只确认 APK 内容哈希不变；本轮没有保存提取开始前的修改时间快照，不能宣称前后 mtime 对照已经完成。
- 尚未完成真实鼠标逐项操作与用户试玩。建筑旋转/入口、原作道路连通与数值表也没有由此原型恢复。
- 原型只显示首帧，通用动画插值与精灵命令执行未实现；各语义限制见素材接入说明。
- 本轮不运行产品 Godot 或四套产品预设，产品侧已有其他智能体管理；这些研究结果不替代产品验收。

## R2-B 自主行动切片（2026-10-02）

当前阶段为 Blocked，仅新版窗口/画面验收阻塞；不标记 Completed。自主授权及设计见
[阶段记录](stages/R2-character-autonomy.md)，规则差异见 [自主行动报告](AUTONOMY.md)。

| 检查 | 当前结果 |
| --- | --- |
| 独立示例 Debug/Release | 编译通过、警告视为错误，CTest 各 3/3 通过 |
| 路径测试 | 1,354 项，含 200 个固定种子的随机地图与独立全量松弛最短路差分 |
| 自主模拟 | 534 项，含无需命令的生命周期、多人互斥、同类轮换、无候选重试、不可达恢复、移动/旋转/删除/阻断取消、重复反馈、时间分段/暂停和溢出整批回滚 |
| R1 回归 | 50 项保持通过，不改 R1 完成才应用一次效果契约 |
| 原型 Debug/Release | 编译通过，CTest 各 4/4（资源、领域、路径、模拟）通过 |
| 内存/未定义行为诊断 | 独立 `example-asan-llvm` Debug，`-fsanitize=address,undefined`，编译及 CTest 3/3 通过，无诊断 |
| 编辑器诊断 | 新增示例及修改原型无诊断 |
| 格式与链接 | R2-B 七个 C++ 文件通过格式检查；研究 Markdown 本地文件链接 72 项通过 |
| 当前真实窗口 | Debug 有界演示退出码 1：GLFW 无法确定显示器，平台初始化失败，未生成新版截图 |

内存诊断构建使用相同 CLion CMake/Ninja 与 Homebrew Clang，仅增加编译/链接 sanitizer 参数。
系统存在 WindowServer 不等于本执行上下文能够获取显示器；未以其他工具绕过会话/权限。
原型现在会在窗口初始化失败时立即报告，不继续上传纹理。窗口失败后没有残留原型进程。
加入窗口保护后两套原型再次构建通过；保护不改变已验证的核心/资源契约。
尚未验证新版实际画面、像素块、替换根和鼠标操作，旧截图只属于 R2-A 历史记录。
恢复条件为可用 macOS 图形会话，命令仍为 `prototype-*-llvm/dungeon_village_prototype --demo --screenshot ...`。

## R2-C 设施原始表切片（2026-10-02）

阶段仍 InProgress；本节仅记录表结构、必要数据与字段消费者的首个交付，不宣称完成几何或经营复刻。

- 工具 Debug/Release 重新构建通过，警告视为错误；CTest 各 3/3 通过。
- 表结构 44 项检查覆盖 UTF-8 码点/截断/控制字符、CRLF/空列、整数/列表边界、36 列、重复 ID 和未知形状。
- 固定数据 90 项检查：条目 SHA-256、85 条定义 ID 次序、75/7/3 形状分布、旅店/咖啡厅关键原字段。
- 原归档测试 84 项保持通过；4 个新增 C++ 文件格式检查通过，编辑器无诊断。
- 提交前研究 Markdown 本地文件链接 92 项通过。
- 发布 `data/original/tenantData.txt` 和 `data/SOURCE.tsv`，只在已验证、目标不存在时写入新临时目录，
  写完后改名为正式目录；现有目录不会覆盖，写入失败的 `.partial` 不视为交付。
- 设施定义原始表通过已知归档校验后恢复，字节/源哈希见清单；原始文本由局部 `-text` 保持。
- APK 内 `assets/xls.dat` 的只读 SHA-256 与来源清单一致；APK 本身哈希与固定基线一致。
  暂存原始设施表与恢复原件逐字节比较通过；重复发布已有目录按设计返回失败且不覆盖数据。
- 第 33 列效果表达式只做 UTF-8/结构保存，尚未验证解释器语义，不将“完整结构校验”说成所有字段已理解。
- 本批未启动新 GUI；R2-B 图形验收阻塞仍保留。未修改产品或现有演示经营参数。

只读复现：

```sh
research/dungeon_village_1/work/tools-release-llvm/kairo_table_inspect \
  research/dungeon_village_1/work/decompiled/resources/assets/xls.dat tenantData.txt
research/dungeon_village_1/work/tools-release-llvm/dungeon_village_table_tests \
  research/dungeon_village_1/data/original/tenantData.txt
```

首次发布用检查命令附加 `--publish 新目录`；正式数据已经存在时不要重复发布或手动覆盖。

### 独立几何规则

- 几何测试输出 `1488 checks passed`，覆盖全部形状/朝向在 7×7 地图上从 -2 到 8 的锚点、
  溢出坐标/地图保护、准确绑定顺序与分片、非法/重复身份、重叠和保持实例身份的移动预检。
- 独立示例 Debug/Release、AddressSanitizer/UndefinedBehaviorSanitizer 重新构建通过；CTest 各 4/4。
- 两套原型重新构建通过，CTest 各 5/5；增加几何规则没有改原型仍为单点的画面或经营夹具。
- 新版 GUI 仍未验收，不宣称几何已经用于可视放置，不替代产品构建/测试。

### 独立经营数值规则

- 新增纯函数和 80 项检查：旅店/咖啡厅 1–5 级、整数取整、改良/实例修正顺序、两组价格标志优先级、
  属性上限、负改良、维护费不加实例修正、装饰折扣、建造加速封顶、定义共享升级阈值与溢出拒绝。
- 当前独立示例 Debug/Release 和 ASan/UBSan 构建通过，CTest 各 5/5；原型两套构建与 CTest 各 6/6。
  没有改 R1 模拟/可视夹具的金额、完成时序或几何接入。
- 首轮 Debug CTest 和单独测试在百分比期望夹具失败：原测试将 311 经两组倍率误算为 409。
  只读核对后保留实现，夹具改为 304 经 120%/110% 得 400，清楚区别一次倍率得 401。
  按用户自主迭代授权修正机械期望值，以上结果均为修正后重新执行，不隐藏失败历史。
- 本轮编辑器诊断请求未返回，已终止该只读检查编排；不能记录编辑器检查通过。
  编译器严格警告、完整 CTest 和内存诊断结果独立有效。
- 三个经营 C++ 文件格式检查通过，研究 Markdown 本地链接 98 项通过。

### 普通旅店帧绑定纠正

- 核对定义 28 → 地图显示 48 → SEB 48 → 图片 25，普通旅店使用 `tenant10`，
  不再使用定义 29 的双格旅店 `t_inn00` 分片。维护八条旅店/双格旅店/咖啡厅帧绑定。
- 新规范化 PNG 与原件逐字节一致；重新发布 761 原件、五个活动逻辑键，清单与维护清单逐字节一致。
  旧旅店规范化 PNG 仅保留历史，不在活动清单内。
- 工具 Debug/Release 重新编译通过，CTest 各 3/3；原型两套重新编译通过，CTest 各 6/6。
  单独素材换图夹具通过 814 项检查、398 张 PNG 解码，371 张含透明像素、无字节重复。
- 新 `--row` 查询对负下标、85 和非整数均返回预期失败；`--parts` 对目录输入返回预期失败。
  索引为表行下标，不将定义 ID 当作行号。SEB 的 `legacy_tag` 仍不解释或与记录数比较。
- 首轮原型构建因测试调用未限定命名空间失败；补齐命名空间后，两套构建与测试均重新执行通过。
  四个修改 C++ 文件格式检查通过。普通旅店 PNG 已实际查看，完整非空；不以查看素材代替窗口验收。
- 本批没有启动 GUI，R2-B 当前图形阻塞保持；没有修改产品文件或原始 APK。

### 周期账本与双资源切片

- 核对金币加减、五分类统计、小型点数函数及菜单资源类型路径，纠正“月报关闭时把累计奖励入资金”的
  旧结论；明确月内报表准备与日历跨月分离。带警告巨型方法只作局部推断，边界见
  [周期账本报告](ACCOUNTING.md)。
- 新增独立 C++17 账本，明确即时金币、费用事务批次、前三分类报表快照、讨伐点数与 999 上限。
  冻结周期、稳定事件 ID 和同载荷幂等是 Ark 自有契约，不冒充原作存档重放保障。
- 首轮编译与测试通过，无本批编译/测试失败。账本 8,898 项检查，含 100 组固定种子分类账差分、
  重复投递、载荷冲突、费用整批回滚、封账边界、负余额、金币/点数溢出和点数领取顺序。
- 当前独立示例 Debug/Release 重新编译通过，CTest 各 6/6；ASan/UBSan 构建及 CTest 6/6通过，
  无内存或未定义行为诊断。两套原型重新构建通过，CTest 各 7/7（含资源与六项领域回归）。
- 未把新账本接入 R1 或原型，不新增存档功能；未启动 GUI，新版图形验收仍 Blocked。
- 三个新增 C++ 文件格式检查通过；研究 Markdown 本地链接 117 项通过；工具 Debug/Release 当前 CTest
  各 3/3 再次通过。761 个原始素材与已提交 Git blob 逐字节核对一致，不依赖文本过滤后的哈希。
- 最终交接补入后，本地链接检查 123 项通过；APK、设施原表与普通旅店副本 SHA-256 均与固定清单一致。
  最终研究程序/测试/JADX 进程检查无匹配；所有执行工具调用已结束，不保留后台任务。

## R2-D 地图访问切片（2026-10-02）

- 本次继续用户已授权无人值守研究，仅修改 research；产品与根文件既有变化不纳入研究提交。
- 实现严格基础地图/设施绑定、五类有向准入、显示状态首步退出、无设施展开排除、全图距离与前驱、
  1/2/4 格可达占用格报告和坐标/定义/实例到达身份检查。辅助周边表不再被误称入口。
- 6,339 项检查：全部 25 对准入及首步状态差异、九种绑定消费者、多格/两朝向分片、商店不可穿行、
  非锚点可达、预算/非法输入拒绝、身份失效和八种损坏场；200 个固定种子随机图与独立松弛差分。
- 首轮编译和测试均通过。示例 Debug/Release、ASan/UBSan 构建与 CTest 各 7/7；
  原型两套构建和 CTest 各 8/8；没有内存或未定义行为诊断。
- 三个新 C++ 文件格式检查通过；初次研究 Markdown 本地链接 128 项通过。
- 独立规则切片完成，不修改旧模拟，不运行 GUI，不宣称可视多格或全部原作活动资格已经恢复。

## R2-E 邻接修正切片（2026-10-02）

- 维护有序 8/10/12 格外环、邻接三属性与来源身份完整重算；来源种类 2/3 只作用商店 3，
  同来源→同目标实例去重、同类多来源分别累加；道路每外环格魅力 +2。
- 6,280 项检查，含六种完整外环次序、各边界锚点裁边、原表旅店/咖啡厅/向日葵值、空效果来源、
  多格去重、同类叠加、旋转/移除、正负和溢出拒绝、经营接口受检查转换和后置封顶。
- 200 组固定种子随机布局以独立 Chebyshev 邻接集合判定差分，不用被测外环函数生成期望。
- 工具固定数据检查增加至 238 项，确认 85 行的第 26/27 列列表长度/槽位及 51 条非空记录；
  原表字节/哈希不变，未知列保留契约不改。
- 首轮构建/测试通过。示例 Debug/Release、ASan/UBSan 构建与 CTest 各 8/8；
  原型两套构建及 CTest 各 9/9，工具两套各 3/3；无内存或未定义行为诊断。
- 没有修改 R1/R2-B 模拟或产品，没有启动 GUI；纯规则完成不替代可视接入/新版图形验收。
- 六个修改 C++ 文件格式检查通过；中文研究 Markdown 本地链接 156 项通过。

## R2-F 设施事件切片（2026-10-02）

- 严格解析事件整数矩阵，支持 ASCII trim、正负 32 位整数与空脚本；预算保护/空参数/空指令/溢出拒绝。
  原始字段与未知 opcode 保留，不因结构通过宣称完整解释器已恢复。
- 工具两套构建与 CTest 各 3/3，结构 73 项、固定原始表 350 项、归档回归保持；
  85 行完整核对确认只有定义 33–58 有 `6,100&40,<本定义ID>`，共 26 条，图标 2/3。
- 实例道具确认/跨月计数和两条受限计划有 42,472 项检查，含 100 组各 200 步随机标量差分、
  200 组月份分段差分、门槛/重置/独立实例、空/未知脚本、身份与溢出拒绝。
- 首轮构建和测试全部通过。示例 Debug/Release、ASan/UBSan 各 9/9；原型两套各 10/10。
  无内存或未定义行为诊断；两份修改实现的编辑器诊断均为空。
- 没有接入旧模拟/图形原型/产品，不执行完整解释器或直接加人气，不新增事件队列/存档。
  当时原作人气队列局部倒计数分支存在 JADX 控制流疑点，未依此复制精确时序；后续 R2-I 低层核对
  消除该局部条件疑点，完整调度仍未恢复。
- 七个修改/新增 C++ 文件格式检查通过；中文研究 Markdown 本地链接 173 项通过，差异无空白错误。

## R2-G 道具表与共享改良切片（2026-10-02）

- 已严格读取并发布 36 行25列原始道具和独立来源清单；固定条目 3,327 字节/SHA-256 与归档一致。
  全部设施 12 项适配列表按道具类别索引，值 0/1/2；不是按道具 ID，也不将 0 当作零效果。
- 道具结构 39 项、固定数据/跨表 2,225 项检查；既有事件结构 73 项、设施数据 350 项保持通过。
- 独立候选事务 16,047 项检查，含 2,000 组固定种子独立缩放/累加/实际属性/库存/计数差分。
  覆盖正负半倍取整、共享定义对另一实例的影响、封顶/零效果仍消费、维护费不变、无库存/溢出整批拒绝。
- 示例 Debug/Release、ASan/UBSan 构建及 CTest 各 10/10；工具两套各 5/5；原型两套各 11/11。
  无内存或未定义行为诊断；道具规则与表读取实现编辑器诊断为空。未启动 GUI、未接入旧模拟/产品。
- 首轮工具构建/测试通过后，只读打印发现类型化道具名称未赋值；原字段/原件哈希保持。
  按自主机械迭代授权补齐名称与断言，以上构建/测试均为随后当前代码再次执行结果，不隐去发现。
- 重复发布现有道具目录按设计退出码 1、不覆盖；随后固定字节/跨表 2,225 项再次通过。
  道具原件 Git text 属性为 unset；七份 C++ 格式检查、190 项中文 Markdown 本地链接与差异空白检查通过。

## R2-H 活动类别计划切片（2026-10-02）

- 小型 0/2/6 类别计划与外部票号选择实现；保留标志 8192、六项访问计数、零权重列表项和数字类别。
  强制类别 3 与类别 4 存在条件独立，不按猜测改成“强制类别 4”。
- 63,908 项检查：2,352 组资格真值表、每组 0/2 等价与所有合法票号区间、200 组随机权重显式展开差分、
  最大计数/无权重/负数/未知操作/后续权重溢出整批拒绝。
- 固定设施检查增加至 436 项，类别分布为 `20/36/2/6/4/13/0/0/0/3/1`（类别 0–10）。
  类别 6/7/8 非空测试明确为合成输入，不能冒充固定包真实设施。
- 首轮编译/测试通过：示例 Debug/Release、ASan/UBSan 各 11/11；原型两套各 12/12；工具两套各 5/5。
  内存诊断无报告，规则实现与测试编辑器无诊断。不启动 GUI、不替换旧单点/轮换模拟或产品。
- 四份 C++ 格式检查通过；中文研究 Markdown 本地链接 202 项及差异空白检查通过。

## R2-I 已排序设施格选择切片（2026-10-02）

- 固定 APK、JADX 1.5.6 fallback 单类输出核对 `Character` 的候选收集、按出现项计权和多格最小
  列表下标；命令、低层行号/标签与限制见 [选择报告](RANKED_FACILITY_CHOICE.md)。
- 新增独立选择接口，保留同实例的多个格，不以实例去重改变概率；返回抽中项与最终目标格两种下标。
  完整结构、共享字段、成本顺序与票号拒绝不留下目标输出；上游资格和原作排序平局仍未恢复。
- 1,026 项检查，含普通类别 1/2/6/8、100 组固定种子重复出现项票号展开差分、1:4 多格权重、地图绑定→距离场→实际
  可达格→排序夹具→选择→路径和到达身份组合。组合中的平局格索引是 Ark 夹具，不是原作事实。
- 示例 Debug/Release、ASan/UBSan 当前构建成功，CTest 各 12/12；原型 Debug/Release 构建成功、
  CTest 各 13/13。实现与测试编辑器诊断为空。未运行 GUI，R2-B 的显示会话阻塞不变。
- `UserData` 独立 fallback 核对消除 R2-F 人气倒计数的局部疑点：剩余大于零跳过，耗尽才应用并
  移除。记录提示标记、同次到期顺序和阈值副作用，不复制完整队列，不承诺秒数/暂停/全场景时序。
- 两次单类 JADX 执行均已结束，生成 Java 留在忽略目录，没有进入维护源码。
- 类别覆盖终检补入后重新构建三套示例/两套原型并运行全部 CTest，结果保持各 12/12、13/13；
  工具两套当前 CTest 各 5/5。三份 C++ 格式检查、中文 Markdown 本地链接 223 项与差异空白检查通过。

## 第二批无人值守收尾（2026-10-02）

- 约两小时有效研究与六个切片检查点见 [独立交接](stages/无人值守研究-2026-10-02-第二批.md)，
  第一批记录保持历史，不覆盖旧时段。
- 最终研究 Markdown 本地文件链接 235 项通过；格式/差异空白检查通过，维护树扫描无生成 Java、
  CLASS、DEX、JAR 或调用图。原件与生成证据不作为 C++ 产品源码。
- APK、设施原表、道具原表哈希与固定来源一致；素材清单覆盖的 761 个唯一原始文件 SHA-256 全部匹配。
- 沙箱内系统 Git 无法找到开发工具、系统 shasum 的 Perl 动态库被阻止；按已授权方式在沙箱外重跑
  只读检查成功。这是工具环境限制，不改动系统工具链配置或研究输入。
- 最终研究程序/JADX/构建/CTest 进程检查无匹配，所有本轮执行会话退出；未启动 GUI 或无限重试。
  共享工作区的两项顶层改动不纳入研究提交，未推送远程。

## R2-J 候选生成与类别 4（2026-10-02）

- 接续工作区已有候选切片，审阅实现/测试与 `RouteSearch`、`Character` 低层输出，不覆盖生成证据。
  普通过滤、城内外扫描、事件直接追加/只删首项、交换排序和三种列表视图见
  [候选报告](ACTIVITY_CANDIDATES.md)。原排序平局的局部疑点已消除，寻路节点平局仍有 Ark 差异。
- 当前 5,478 项检查，含 200 组固定种子收集/排序差分及地图→候选→类别→普通选择→路径/到达链。
  类别 4 保留统计范围与扫描范围不同的原观察，不擅自改为只选择阶段 1。
- 使用 CLion CMake `--build work/<构建目录> --parallel 2` 和 CTest `--output-on-failure`：
  示例 Debug/Release、ASan/UBSan 各构建成功、13/13；原型 Debug/Release 各构建成功、14/14。
  测试运行在对应忽略目录，不读写正式存档。编辑器测试入口返回无已注册测试，实际结果以 CTest 为准。
- 三份候选 C++ 文件 `clang-format --dry-run --Werror` 通过，`git diff --check` 通过。
  本批未运行图形窗口，完整人物决策和新候选原型接入尚未完成。

## R2-K 普通到达与即时收入（2026-10-02）

- 交叉核对较小 `Character` 到达 helper、已有 fallback 与金币/角色统计/设施月销售消费者，
  规则、行号、分支与所有权见 [到达报告](FACILITY_ARRIVAL.md)。只维护普通 N=-1、非装备细分类路径。
- 新增纯候选，先更新上次访问和访问统计，再根据正价格/人物类型/标志决定收入；不将 B[2] 当钱包，
  不增加共享定义使用次数或应用退出效果。严格验证与 int32 累加保护，unsupported/失败没有部分快照。
- 25,816 项检查，包含 7,920 组分支与收费组合、1,000 组固定种子状态差分、各修改位置溢出、
  有符号统计/零定义/月份端点/免费访问和经营→到达→账本→月报组合。
- CLion CMake `--build work/<构建目录> --parallel 2`，CTest `--output-on-failure`：
  示例 Debug/Release、ASan/UBSan 当前构建和 CTest 各 14/14；原型 Debug/Release 各 15/15。
  三份新 C++ 的格式检查通过，编辑器诊断为空。只链接/回归原型，未改运行玩法、未运行 GUI。
- 文档同步首轮补丁的末尾锚点与当前证据文件不匹配，工具拒绝整个补丁；只读确认未部分改写后，
  按现有文本重新应用。不改源码/证据来掩盖问题，编译/测试没有失败。
- 原作 helper 不是幂等现金事件：重复计数与现金重试必须在未来聚合层一起处理，包括零金额访问。
  本阶段没有新增全局存档或声称实现了跨所有者提交事务。

## 本轮持续研究收尾（2026-10-02）

- R2-J/K 本地切片检查点及下一项契约缺口见 [续作交接](stages/持续研究续作-2026-10-02.md)。
  普通到达六类修改位置的溢出与全部不相关字段保留已验证，不宣称跨所有者提交已实现。
- 复用已有 `UserData` fallback 对月报人物/设施费用、分类汇总和状态 2/3 领取顺序补充局部证据，
  见 [账本报告](ACCOUNTING.md#后续低层核对费用过滤与领取局部顺序)。不从 UI 状态转移推导存档事务。
- 工具 Debug/Release 当前构建成功，CTest 各 5/5；原始 APK、设施/道具原表哈希与固定来源一致，
  761 个唯一原始美术文件 SHA-256 与清单全部匹配，未新增反编译/解码工作进程。
- 最终中文 Markdown 本地文件链接 292 项通过，差异空白检查通过；维护树无生成 Java/CLASS/DEX/JAR
  或调用图。进程收尾检查没有研究程序/JADX/构建/CTest 匹配，所有本轮执行会话已结束。
- 本轮不推送、不切分支、不运行 GUI；共享顶层忽略配置和 R1 兼容记录的其他改动保持原样。

## R2-L 完整候选快照选择（2026-10-02）

- 复用已有 Character 低层输出，核对 dl 普通权重与完整候选最终目标的映射；报告见
  [快照选择](SNAPSHOT_FACILITY_CHOICE.md)。没有另起 JADX/解码过程或提交原始实现。
- 暴露已有快照结构校验给新选择器，既有类别 4 复用同一入口；R2-I 接口不放宽、不删事件去重适配。
- 6,914 项检查：完整/活动/类别下标差异、所有普通支持类别、重复项权重、可空成本、严格结构拒绝、
  4096/4097 上限、总和/票号边界、200 组固定种子展开差分、R2-I 合法子集与地图全链组合。
- 示例 Debug/Release、ASan/UBSan 当前构建成功、CTest 各 15/15；原型 Debug/Release 构建成功、
  CTest 各 16/16；共享校验触及的候选、类别 4、普通选择回归全部通过，编译/测试没有失败。
- 新接口/实现/测试及候选实现编辑器诊断为空；原型只是链接/测试回归，未接入快照选择或运行 GUI。

## R2-M 区域回退与测试恢复（2026-10-03）

- 首轮新增区域组合夹具设置零高度城镇，候选生成正确返回 invalid_input；Debug CTest 15/16，
  已按约定停止并只读定位。用户确认修正后，改为合法上下边界，增加明确错误码断言，
  下游使用同一个 town.bottom；不修改规则校验或抽签算法。
- [区域测试](example/tests/regional_choice_test.cpp)14,043 项通过：五次提前命中、第六次新抽回退、
  已消费/未消费票号、整数边界、1,000 组全前缀差分和候选/类别/区域/路径组合。
- 示例 Debug/Release/ASan+UBSan 当前构建成功，CTest 各 16/16；原型 Debug/Release 当前构建成功，
  CTest 各 17/17。新测试/实现编辑器诊断为空。本条证据在新收敛模型加入之前取得，
  不用来宣称新模型或窗口已验收。
- 使用既有 CLion CMake/CTest 与 Homebrew LLVM，沙箱外执行；没有修改产品或运行新逆向工具。
- 后续按 [R2 收敛设计](stages/R2-convergence.md)接入维护交付，完整细节顺延。

## R2 收敛模型与窗口接入（2026-10-03）

### 当前代码与回归

- 独立标准 C++ 原型模型连接地图绑定/距离场、候选、普通类别权重、完整快照选择、邻接/经营推导、
  普通到达与周期账本；没有改 R1 旧接口、APK、生成 Java 或产品源码。
- [新模型组合测试](prototype/tests/village_test.cpp)550 项通过：三类真实数值/两格绑定、两朝向/移动 300/
  零退款、全状态拒绝、两人独占、自主访问/到达/完成、免费访问、取消、暂停、时间分批一致、
  后续到达计数溢出整批回滚，以及同 tick 到达收入先于费用/冻结/清零的夹具月界。
- 最终原型 Debug/Release/ASan+UBSan 均重新构建成功，CTest 各 22/22，包括原有 16 项示例、
  新模型、两个朝向的无窗口自检、非法参数拒绝与素材检查。研究测试不替代四套产品预设，未运行产品。
- 当前工具 Debug/Release 构建成功、CTest 各 5/5；七键发布器分别在同一隔离目录生成 761 个原始文件/
  七个逻辑键，第二次保持相同内容，生成 MANIFEST 与维护版本 cmp 逐字节一致。
- 素材实际解码/替换 816 项通过，398 PNG、371 含透明像素、无字节重复；新增两张双格规范化 PNG
  字节哈希等于清单中原始条目。固定 APK/设施表/道具表哈希再次与来源相同。

工具链仍为 CLion CMake/CTest/Ninja 与 Homebrew LLVM，沙箱外执行。新内存诊断配置命令：

```sh
/Applications/CLion.app/Contents/bin/cmake/mac/aarch64/bin/cmake \
  -S research/dungeon_village_1/prototype \
  -B research/dungeon_village_1/work/prototype-asan-llvm -G Ninja \
  -DCMAKE_MAKE_PROGRAM=/Applications/CLion.app/Contents/bin/ninja/mac/aarch64/ninja \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_CXX_FLAGS=-fsanitize=address,undefined \
  -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined
```

对 prototype-debug-llvm/prototype-release-llvm/prototype-asan-llvm 分别 cmake --build --parallel 2、
ctest --test-dir --output-on-failure；对 tools-debug-llvm/tools-release-llvm 同样操作。

### 实际窗口与替换

- 使用临时 caffeinate -d -u 包住 --demo；窗口超时上限 15 秒，正常约 2.2 秒退出 0，电源断言随之释放。
- 最终 Debug/Release 各两朝向截图：work/r2-convergence/final-first.png、final-debug-second.png、
  final-release-first.png、final-second.png。三类分片、人物使用、M2 与上期 Net 可见；图片已实际查看。
- Release 两朝向从 /private/tmp 运行，不指定素材/数据路径，仍加载程序旁资源包并正常截图/退出。
- 替换素材不改模型；final-replacement.png 与 final-first.png 的六项截图检查通过：720×720、
  非黑像素 518,400、改变像素 23,256、3 倍最近邻；不是凭文件存在判定画面通过。
- --check 不初始化窗口；--frames 1–3600 支持有界静态/交互观察，--paused 支持人工建设验收起点，
  --orientation 0/1 只改变初始/建设朝向；非法参数有预期拒绝测试。

### 失败、局部修复与剩余项

- 首次窗口接入编译报 Rectangle 重名（素材清单和 raylib），明确两处 ::Rectangle 后构建通过。
  用户明确简单逻辑修复无需再次确认；没有改变领域规则或接口。
- 首次 ASan 配置找不到 PATH 中 Ninja，沿用现有 CLion Ninja 明确路径恢复，不安装新依赖。
- 项目外目录首轮窗口/素材已正常，但 TakeScreenshot 将工作目录拼到绝对路径前，导出失败/退出 1；
  已用 LoadImageFromScreen/ExportImage 修复并重新通过。Cocoa 换缓冲后读图可能显示前一帧，
  演示截图等待连续两帧处于使用/有报告，避免拿到达前的画面当作到达证明。
- 受限环境原生自动化工具因 OpenSSL 配置不可读启动失败；JXA 未暴露鼠标权限查询 API，改用系统 Swift
  只读查询 CGPreflightPostEventAccess，结果 false。未申请权限、未注入鼠标事件或修改系统设置。
- 真实鼠标与用户试玩未运行，见 [试玩清单](stages/R2-playtest.md)。R2-C 有限交付收尾，
  整个 R2 仍 InProgress；深层任务/装备/日历/存取与完整 AI 不再扩大本轮收敛范围。

### 维护检查

- 全部 761 个唯一原始素材 SHA-256、七个规范化/再生成映射逐项相同，固定输入不变。
- 七份修改 C++ 文件 clang-format --dry-run --Werror 通过，编辑器未报告新增编译/诊断问题。
- 研究 Markdown 本地文件链接 351 项通过，git diff --check -- research 通过。
- 本轮没有启动新的 JADX/解码任务，不推送、不切分支、不修改顶层产品文件；共享工作区的
  产品和兼容文档改动保持原样。窗口与临时电源断言正常退出，构建/CTest 执行会话逐项收尾。

## R3-A 退出辅助规则（2026-10-03）

- 复用已有常规 Java 与 Character fallback 核对 opcode 24 局部顺序，定位较小共享使用/满足度消费者，
  见 [退出报告](FACILITY_EXIT.md)。没有运行新 JADX、改动原件或把反编译实现纳入维护源码。
- 共享定义使用先加 1，门槛满足只锁存升级提示；满级仍计数，不自动升级/归零，也不清已有提示。
  普通商店满意判断由实例品质/职业门槛/外部票号准备，实际满意变化和人气请求分别返回。
- 新检查 124,230 项：2,000 组独立标量差分、所有十票号门槛前/等于/后、封顶仍请求人气、降序
  整数取整、负品质、极值宽算术、等级/阈值/已有提示/共享累计以及完整拒绝；经济派生品质组合通过。
- 示例 Debug/Release/ASan+UBSan 当前构建成功、CTest 各 17/17；原型三套当前构建成功、
  CTest 各 23/23，涵盖既有模型/资源/无窗口自检及全部领域回归，没有启动 GUI 或更改原型策略。
- 使用既有 CLion CMake/CTest/Ninja 和 Homebrew LLVM，沙箱外执行。分别对六个忽略构建目录执行
  `cmake --build <目录> --parallel 2` 与 `ctest --test-dir <目录> --output-on-failure`；无编译/测试失败。
- 三份 C++ `clang-format --dry-run --Werror` 通过。编辑器诊断调用未返回，已终止这项检查编排；
  不记录编辑器通过，实际编译/运行测试为本次有效证据。
- 原型保留有限完成计数/回退策略；共享升级 UI、人物恢复/装备、属性 opcode、完整人气阈值/队列和
  退出重入/存取不在此切片完成范围。不标整个 R2、第三批目标或完整复刻为完成。

## R3-B 生命值目标/显示协议（2026-10-03）

- 使用既有 Character fallback 与常规小函数交叉核对生命值六槽、旅店状态 14 的计数 170 恢复、
  住所退出直接赋值；不重新运行反编译或改动原证据。详细行号/标签见 [生命值报告](CHARACTER_HP.md)。
- 新纯函数保留真实目标先更新、动画从真实目标重启、直接赋值不清动画、正/负过渡分母差异以及
  停止不自动补最终显示。容量、分类守卫、时钟/暂停与全人物身份仍由上游负责。
- 200,496 项检查：1,000 组各 100 步混合六槽独立差分，30 步短序列和全部分段点、负值/零值/封顶、
  大整数差值/乘积、目标加法溢出、非法状态/预算及无部分结果。原型没有接入这些协议。
- 示例 Debug/Release/ASan+UBSan 当前构建成功、CTest 各 18/18；原型三套当前构建成功、
  CTest 各 24/24。使用同一现有工具链和隔离目录，首轮至终轮无编译/测试失败。
- 三份 C++ 格式检查和 `git diff --check -- research` 通过；没有请求新的编辑器检查或运行 GUI。
  所有构建和测试句柄已读取到终止状态，不保留后台测试进程。

## 本机 computer use 尝试、用户免验与 R3 设计（2026-10-03）

- 用户允许调用已有本机 Codex Computer Use 做鼠标验证。内置 cua_repl 与 node_repl 在受限环境
  均因无法读取系统 OpenSSL 配置退出，未执行任何点击。
- 沙箱外运行现有 SkyComputerUseClient mcp，initialize 与 tools/list 成功；两次 list_apps 均返回
  bootstrapTimedOut。本机安装服务虽启动并出现 IPC socket，也不能据此记为连接成功或鼠标通过。
- 用户随后明确“不做这个验证”，已停止工具诊断，没有启动原型窗口、发送鼠标事件、
  安装工具或修改系统权限。试玩清单的鼠标项记为 Waived，用户试玩仍 Pending。
- 已终止本轮客户端；中断后只剩本轮启动的服务 PID 37430，经只读核对后定点发送 SIGTERM。
  随后 pgrep -alf 'SkyComputerUse|Codex Computer Use|dungeon_village_prototype|caffeinate' 无匹配，
  没有遗留本轮原型或辅助进程，不终止其他智能体或用户进程。
- R2 研究交付无剩余编码任务；整个阶段保留用户试玩未运行的边界，不将免验写成 Completed。
  新建 [R3-B 设计](stages/R3-calendar-report-scheduling.md)，只读核对已有日历/报表消费者，
  当前 DesignReview，未进行低层顺序认证、实现或新阶段构建测试；不修改产品文件。
- 共享目录在本轮出现另一项 R3-A 设施退出辅助规则及 C++ 改动，全部保留并排除本轮提交。
  全研究链接初查发现该新增报告指向尚未创建的测试文件，不自行修补其他工作的文件。
- 隔离检查本轮八份文档的本地链接 113 项，失败 0 项；编辑器未报告研究目录错误。
  本轮只改文档，没有运行构建/CTest，不将 R2 旧测试结果挪作 R3 验收证据。

## R3-V 原版视觉基线首批静态定位（2026-10-03）

- 用户明确先做原版页面/地图/视觉交互基线，再逐步恢复人物完整优先级、进入/退出效果和成长。
  已调整维护入口和研究顺序，日历候选暂缓；只改 research 文档，不修改产品文件或共享人物代码。
- 读取既有 b/a、b/c、b/g、c/h、c/a 及触摸/字体消费者，建立首批页面入口与资源索引，
  巨型菜单更新/初始化的局部跳转仍标推断；未运行新 JADX 或复制生成实现。
- 分别运行既有 tools-debug-llvm/tools-release-llvm 下 kairo_asset_extract，参数为
  `--input research/dungeon_village_1/work/decompiled/resources/assets/map.dat --list-only`。
  两次退出 0，均报告单一 game.gmap 条目、2356 字节、flags=0、非视觉归档。未提取或编辑地图原件。
- 沙箱外 shasum -a 256 核对 APK 哈希仍为 1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5；
  map.dat 为 8fe8916453538f6f1ee4e61af8f5660ff2f665f0405666980dcf25457e9ff05b，
  与 unzip -p 从固定 APK 读取的 assets/map.dat 哈希相同。不是解码条目 game.gmap 哈希。
- 实际查看原始 title00.png 和 wnd_menuIcon.png，仅认证素材可见，不记为原版运行画面。
- 沙箱外 command -v 找到 /opt/homebrew/bin/jadx，未找到 adb/emulator；只读检查常见 SDK、
  .android、Android Studio 路径均不存在。不穷举自定义安装位置，不安装新依赖或操作真实设备。
- 原版动态采集 Blocked，恢复需明确的 Android 运行环境；首批静态索引见
  [视觉报告](VISUAL_BASELINE.md)，范围、场景、验收与恢复条件见 [R3-V](stages/R3-original-visual-baseline.md)。
  没有原版截图/录像、地图逐格解码或新 C++ 实现，不标本阶段 Completed。
- 本轮没有启动窗口、模拟器、computer use 或后台服务；没有运行构建/CTest。
- 已读取产品 RQ01/RQ02 等交接需求，在研究报告内记录接收、静态覆盖与动态缺口，不改产品状态表。
- 本轮十份文档本地链接 159 项通过，git diff --check -- research 通过；两个新增 Markdown
  编辑器诊断无错误。没有新实现，不把既有工具运行记作本阶段 C++ 构建或原版运行验收。

## R3-V 用户实况截图首批归档（2026-10-03）

- 用户指定 BV1Jc411E7UP 为 UI 参考，不希望安装模拟器；随后提供 5 张截图，要求分类保存、
  需要时主动提出补图需求。已改为用户截图与固定 APK 静态索引分别登记，不再将缺模拟器作为
  页面观察的必要前置；固定 APK 运行、新局与连续触摸/动画验收仍未执行。
- 此前浏览器能读取视频标题、作者、发布时间和 11 个分集目录；播放器 readyState=0、
  videoWidth/videoHeight=0，并发生媒体请求中止，未成功播放或观察视频帧。
  后续既有浏览器页不可用，用户改为提供截图后没有继续重试、安装下载工具或获取完整视频。
- [首批归档](references/screenshots/2026-10-03-bv1jc411e7up/README.md)保存 S001–S005 原始 PNG，
  分类为菜单、建设、事件和场景；3 张可读视频时间点为 05:27、05:32、06:53，
  另外 2 张时间未知。P1 关联只来自页面分集目录与截图 38:14 总时长匹配。
- 对附件目录的沙箱内复制被权限拒绝，随后经沙箱外授权只读取这 5 个明确附件并复制到 research。
  原附件保持原样；没有修改权限、覆盖旧归档或访问无关附件。
- Node 只读逐字节对比 5 份源附件/归档副本全部相同；尺寸、字节数和 SHA-256 与
  [清单](references/screenshots/2026-10-03-bv1jc411e7up/MANIFEST.tsv)5/5 相符。
  系统 sips 成功识别全部 5 张 PNG，像素尺寸逐项相同；实际查看上传的 5 张图，
  并从归档再次查看通信弹窗。没有裁切、重采样、去水印或生成派生图。
- 已记录布局/可见字段、网页/播放器遮挡、5 个可见菜单与 APK 7 标签索引的待核对差异；
  不根据截帧差额推断交易/维护费，不把 5 月场景记为初始村庄，不猜预览建筑定义 ID。
- 六份观察/入口/阶段文档的本地链接目标 115 项存在；git diff --check -- research 通过。
  本轮只增加截图和中文记录，没有 C++ 修改，没有运行构建/CTest，也不引用旧测试作为本次证据。
- 已请求下一批最小补图：设施详情、建设设备分类、放置确认/建成与实际出现的拒绝提示。
  RQ01 新增静帧部分覆盖；RQ02 仍缺新局与原始地图。没有改产品文件或产品验收状态。
- 本次复制和只读检查均已退出，没有启动窗口、模拟器、播放器、caffeinate 或后台服务；
  不保留本轮执行会话，不终止其他智能体/用户进程。

## R3-V 用户实况截图第二批归档（2026-10-03）

- 延续用户分类保存和主动请求补图的要求，新增 [第二批9张图片](references/screenshots/2026-10-03-bv1jc411e7up-02/README.md)，
  编号 S006–S014，累计14张；原件按设施、城镇、场景、事件、战斗、冒险、人物和物品分类保存。
  原附件只读，研究副本不裁切、不重编码、不去水印，不作为产品运行纹理。
- 沙箱外授权复制9个明确附件；Node 逐字节源/副本比对9/9相同，尺寸、字节数与 SHA-256
  和 [清单](references/screenshots/2026-10-03-bv1jc411e7up-02/MANIFEST.tsv)逐项相符。
  系统 sips 成功识别9张 PNG并确认尺寸；实际查看用户提供的9张图片，并从归档再看城镇条件页。
- 本批两个可读视频时间点为12:35、13:07，总时长38:14与P1目录匹配；其余时间未知。
  S010为520×356用户局部战斗图，裁切矩形/缩放未知；S008底部画面截断，均不作完整热区基线。
- 新记录特殊募集入住空申请者、城镇★1条件、到访、连击/短条、征集/攻略、赠礼结果和商店列表。
  不把募集入住页当经营属性页，不把合计810G当正收益，不由赠礼5/+5、1/+1求最终属性，
  不由同帧人气+20认证统一触发规则，不由日期或上传顺序串接交易。
- RQ01扩展静帧覆盖；RQ04经营属性、RQ02新局、建设设备/确认与连续输入仍缺。
  RQ07–RQ11有相关外观但没有新增上层规则认证；日历候选仍暂缓，不因此启动新实现。
- 六份观察/入口/阶段文档的本地链接目标128项存在，git diff --check -- research通过；
  四份主要Markdown编辑器诊断无错误。本轮没有C++改动，没有运行构建/CTest或原版APK。
- 未修改顶层产品文件；本轮只使用已退出的复制/只读检查工具，没有后台窗口、模拟器、播放器、
  caffeinate或服务，没有遗留执行会话，不处理其他智能体/用户的进程。
