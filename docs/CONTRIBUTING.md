# 开发流程

从AGENTS、[目标与架构](ARCHITECTURE.md)、[TODO](../TODO.md)与 [计划和决策](MILESTONES.md)开始。检查research最新维护交付、来源和工作区；只读研究内容，不修改其结论或执行逆向。
阶段先写明范围、问题/方案、数据所有权、接口、操作次序、依赖缺口及验收，用户确认后编码。
不改变已确认规则、契约或范围的小型编译、断言、实现错误自行修复并重新验证，不逐项询问；测试预期修正必须有来源依据，不能删有效断言、放宽校验或修改原表凑通过。
仅遇来源/规则与方案冲突、关键契约歧义、范围扩大或需要新决策时暂停对应工作，保留现场并说明依据与方案，确认后恢复。无人值守沿用此边界及权限要求。完成后同步文档、验证记录并提交checkpoint。

## 测试时机

以已确认、可独立验收的功能批次为阶段：先明确行为与验收，完成本批实现后，再集中补齐/整理测试并统一验证。无需等整个B1/B2完成，也不把每个函数、小改动或提交拆成一次完整验收。
开发中按需编译；只有排查已知失败、疑似回归或高风险事务/随机/并发问题时做最小定向复验，不默认每改一处就新建测试和跑四套。测试通过后才标记阶段完成、保存交付checkpoint。
纯文档阶段只检查差异、链接及约定一致性；代码、构建、资源或测试阶段按下面的改动范围选择检查，不再每批执行四套矩阵。真实窗口、OS输入、原版对照仍按实际范围另验。

## 构建检查

保留四套预设，普通代码阶段默认只在本地配置、编译desktop-debug并验收，排除耗时的三个月连续模拟：

```sh
cmake --preset shared-libraries
cmake --build --preset shared-libraries --parallel 4
cmake --preset desktop-debug
cmake --build --preset desktop-debug --parallel 4
ctest --preset desktop-debug -E '^simulation\.startup_world_continuous_test$'
```

首次用选定工具链完成上述公共库与消费者配置后，日常开发可使用统一重建入口：

```sh
node scripts/build_product.mjs desktop-debug --parallel 4
```

它严格先完成`shared-libraries`，再构建指定消费者；失败不继续。该入口复用已配置的预设，不选择新工具链、不运行测试。不要只重建消费者后运行测试，因为消费者导入的DLL不会自动重建其源文件；修改公共接口、实现或数据时同样使用此顺序。

另三套为desktop-release、headless-debug、headless-release，按下表需要替换预设名执行配置/编译。headless不查找raylib，Release测试也保留有效断言，警告视为错误。配置选择如下：

| 场景 | 阶段收口检查 |
| --- | --- |
| 普通代码阶段 | 本地desktop-debug配置/编译，标准CTest仅用上述`-E`精确排除三个月用例，其余全部执行 |
| main流水线 | desktop-release配置/编译、完整标准CTest（含三个月基线），通过后打包发布 |
| 核心接口、模块依赖或CMake调整 | 补headless-debug配置/编译及标准CTest，默认同样排除三个月用例，验证核心不依赖图形层 |
| 长期模拟、性能或持续世界回归 | 按风险使用headless-release，执行相关标准/显式长测，不重复同一核心轨迹 |
| 界面、资源或打包调整 | 补实际窗口、资源或解压启动检查；试玩优先desktop-release，不要求重复无关整套回归 |
| 仅CI发布配置调整 | 检查工作流、脚本和相关发布路径；构建/打包逻辑改动另补受影响验证，不机械触发四套游戏回归 |
| 仅文档修改 | 检查差异、链接及一致性，不跑游戏测试 |

按2026-10-06用户决定，本地Debug三个月测试不作为每批必跑项，三个月行为覆盖依赖流水线的Release测试；仅定位Debug特有问题、相关失败或用户明确要求时定向补跑。该用例仍注册在四套CTest中，直接运行不带`-E`的Debug预设会执行它；不修改用例、断言、月份、种子或超时。额外自然/年度长跑按风险显式开启，优先Release，Debug长跑仅作必要定向诊断。

阶段收口分别记录本地配置/通过项、Debug排除项、按需追加检查及对应提交的CI链接/结果。当前CI只运行desktop-release，不得把Release结果写成Debug三个月通过；CI未运行或未结束时标待验证。完成上述本地范围可先保存checkpoint，不为等待CI而补跑四套或本地Debug三个月，也不自动推送；完整交付仍需记录流水线结果。修复后按实际影响重验本批所需配置及用例，共用源码改动不自动升级为四套全测；通过后无新修改、失败或未决风险不重复全套。

四套是消费者配置，不是四份玩家制品：Debug调试消费者程序，Release优化消费者程序，headless用于无窗口验证/模拟。用户选择公共库统一用Release，不保留库内部调试符号；四套消费者导入同一份库，不分别编译核心。维护AVRSAVE的`ark_world_persistence`/`ark_world_hash`与正常runtime分开，回放测试显式链接；玩家ARKSAVE仍使用原`ark_world_save`，递归玩家收包不会带入未使用的维护库。独立`shared-libraries`预设在`build/shared-libraries`编译库，消费者对象仍位于`build/<预设名>-shared`。全部EXE/DLL同放`build/bin/`，EXE带配置名后缀避免互相覆盖，导入库只在公共库树`lib/`。每次公共库构建补齐C++运行库、raylib、资源和字体；修改库源码后先重建公共库，再构建消费者。只向用户发布desktop-release游戏包，打包才复制必要DLL并去掉游戏EXE后缀。main CI不重复Debug或headless构建，本地验证持续允许。
构建需要Node 18+，只在构建期JSON.parse交叉校验固定发布数据，生成只读标准C++；运行无需Node。
资源更改须核对源/副本哈希、实际解码和任意工作目录启动；界面更改须实际画面/输入验收；存储用隔离档，不做旧档迁移。
格式按clang-format，公开头在include/ark，实现在src；CMake显式登记文件。只建立有实际职责的模块。

## Windows本地构建

长测与窗口预运行已经逐框架直接计算，不等待47ms；游戏倍速0/1仍是原消费者规则，不能用跳tick加速测试。性能采样入口与计量边界见[采样说明](../scripts/simulation/profile_world.md)。合并商会窗口验收使用`build/bin/ark_village-desktop-release.exe --inspect-page world-commerce-suite --frames 8 --screenshot build/validation/world-performance/suite.png`：一次自然到达83，独立分支输出六页截图和耗时；目录需先创建。保留各页实际输入与资金/点数检查，不将独立分支当连续交易。正常窗口worker节拍不变。

当前目标为Windows 10+ x64，使用LLVM-MinGW 20250305 UCRT，CMake 3.21+、Ninja、Node 18+、pkg-config。工具可解包到忽略的`build/local-tools`，不必系统安装；编译器、CMake/Ninja、Node和pkg-config加入当前终端PATH。更换已配置的32位编译器时先以`cmake --fresh --preset ...`重新配置，不混用旧对象和raylib库。

raylib使用下节固定提交；示例命令假定源码位于`build/local-tools/raylib`，LLVM的bin已在PATH。先构建x64共享库：

```powershell
cmake -S build/local-tools/raylib -B build/local-tools/raylib-x64-shared-build -G Ninja -DCMAKE_C_COMPILER=x86_64-w64-mingw32-clang -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_INSTALL_PREFIX="$PWD/build/local-tools/raylib-x64-shared-install" '-DPKG_CONFIG_LIBS_EXTRA=-lwinmm -lgdi32 -lopengl32'
cmake --build build/local-tools/raylib-x64-shared-build --parallel 4
cmake --install build/local-tools/raylib-x64-shared-build
$env:PKG_CONFIG_PATH="$PWD/build/local-tools/raylib-x64-shared-install/lib/pkgconfig"
python -m pip install fonttools==4.59.0
python scripts/prepare_windows_font.py --output-dir build/local-tools/fonts
```

字体工具仅用于构建。先按README配置/构建`shared-libraries`，再选择消费者预设：日常开发用`desktop-debug`，试玩/打包用`desktop-release`；两套headless按需配置，不查找raylib/字体。仅核心开发可用`shared-libraries`加`-DARK_BUILD_DESKTOP=OFF`构建核心库，桌面使用前须以ON重新构建公共库。共享配置不能沿用`-static`、`-static-libstdc++`或`-static-libgcc`；编译器身份、版本、路径和指针宽度必须与公共库一致。pkg-config不在PATH时可显式传`-DPKG_CONFIG_EXECUTABLE=<pkgconf.exe路径>`。路径含空格时保留PowerShell引号，CLion也可将相同编译器/缓存项配置在CMake profile。公共库阶段将`ARK_DESKTOP_FONT`/`ARK_DESKTOP_FONT_LICENSE`指定的OTF/许可复制至`build/bin/fonts/`。Windows可执行文件嵌入`asInvoker`，避免带update字样的测试程序被系统误认为安装器。

## GitHub CI与制品

[Build and test](../.github/workflows/ci.yml)在push到main时运行，也支持在main上手动触发。CI构建与测试在GitHub runner上执行，按[构建检查](#构建检查)承担三个月行为覆盖并提供其余额外检查。用户持续允许本地配置、编译、测试和窗口验收；两类结果分别记录，不以本地结果代替远程验收。

| 制品 | GitHub runner / 工具链 | 目标与验收边界 |
| --- | --- | --- |
| `ark-village-windows10-x64` | `windows-2022` / LLVM-MinGW 20250305 UCRT、x86_64 | Windows 10+ 64位目标；检查PE32+/AMD64，实际CI为Windows Server 2022，不代表Windows 10真机/图形验收 |

按2026-10-06用户决定，构建与发布job均只在Windows运行；停止macOS/x86制品。配置、编译desktop-release并执行全部标准CTest，保留三个月基线，额外`ARK_LONG_WORLD_TESTS`关闭。desktop包含核心程序及核心测试，日常CI不重复Debug或headless；本地以desktop-debug为默认验收配置，headless按需追加。任何步骤失败即失败，不跳过失败或降低断言；同一main的新提交取消旧流水线。

Windows检出关闭Git自动CRLF转换，保留冻结源/资源的字节和哈希。先构建公共Release库，再构建desktop-release消费者。`packaged_frame_contract`临时副本保留原可执行文件名（Windows保留`.exe`），并携带共用DLL；原有全部断言、变异输入及产品实现保持不变。

raylib固定到6.0提交`dbc56a87da87d973a9c5baa4e7438a9d20121d28`，单独共享构建；通过`PKG_CONFIG_LIBS_EXTRA`补齐`winmm/gdi32/opengl32`系统链接依赖。编译器显式选择`x86_64-w64-mingw32-clang++`，开发程序共用目标架构的C++运行库DLL。打包递归读取PE导入，仅复制Release游戏所需的Ark/raylib/C++ DLL并剥离分发副本符号，拒绝未随包提供的非系统依赖。解压后不需要安装Node/CMake/raylib或额外C++运行库。

desktop-release全部标准测试成功后，打包剥离调试符号的桌面程序及依赖DLL、616项清单资源、字体子集/来源/OFL许可、raylib与运行库许可及启动说明。simulation数据已编译进运行库，源码副本、测试、CLI、导入库、调试符号和工具链不进入游戏包。字体来自固定Noto Sans CJK SC2.004，使用固定fonttools版本按产品源码/数据提取并验证字形；每次构建重新生成，新增文案不会沿用旧字形清单。直接运行exe或启动脚本即可，保留`--font`覆盖；不猜测系统TTC支持。打包前检查PE32+/导入DLL、字体与资源哈希，并在独立工作目录执行制品`--check`。实际字体窗口加载由本地窗口验收单独记录。

Actions运行页保留7天的ZIP、SHA-256及独立诊断artifact；ZIP使用最高常规压缩等级，上传时关闭二次压缩。失败仍上传构建日志、CTest日志/JUnit，工具链和构建树不作为制品存储。通过后独立publish job用Windows runner自带的GitHub CLI，将单份ZIP及校验文件作为GitHub Release附件上传；仅此job授予`contents: write`及下载artifact所需的`actions: read`，build job只有读取权限。不再安装ORAS或申请`packages: write`。

每次发布使用`sha-<完整提交SHA>`标签，显式指向已测试的提交。先创建草稿并上传附件，从Release回读并逐字节核对ZIP/校验两个文件后才公开；当前main提交标为Latest，重跑旧提交不会替换Latest。失败使流水线失败，未公开的草稿可在重跑时续传；已公开附件不覆盖，重跑须与本次产物逐字节一致，否则失败。Release历史不受Actions的7天期限影响；已有GHCR包不自动删除。

在[最新Release](https://github.com/forza0310/Ark-Village/releases/latest)的Assets中下载`ark-village-windows10-x64.zip`及`.zip.sha256`，解压后运行`ark_village.exe`。也可使用GitHub CLI：

```sh
gh release download --repo forza0310/Ark-Village --pattern 'ark-village-windows10-x64.zip*'
```

指定历史构建时，在`gh release download`后追加`sha-<完整提交SHA>`，或从[Releases列表](https://github.com/forza0310/Ark-Village/releases)选择版本。Actions发布步骤摘要记录Release链接和提交标签。CI创建Release及对应标签，不推送源码提交、不执行研究工具；标准CTest、真实窗口/OS输入与原APK动态对照分别报告。

历史[运行37317066693](https://github.com/forza0310/Ark-Village/actions/runs/37317066693)验证了`ba68e90`旧两平台流程。Windows-only x64游戏包的历史本地构建、ZIP生成/解压及真实启动记录于[B1](stages/B1-playable-prototype.md#task-report-ui)。本次Releases发布迁移的验收见[ADR-0043](MILESTONES.md#adr-0043)；首次远程创建/附件回读/Latest更新待包含新配置的main运行确认，本地不主动推送。

平台依据：[GitHub托管runner列表](https://github.com/actions/runner-images#available-images)、[LLVM-MinGW 20250305工具链与UCRT说明](https://github.com/mstorsjo/llvm-mingw/blob/20250305/README.md)。

## 测试设计与组织

### 初始审计快照（2026-10-05）

以下为测试整理前的静态盘点，保留历史基线；未把research测试算入产品，也未运行游戏回归。当前整理见本节末尾，不用旧文件数描述本批结构。

| 范围 | C++测试文件 | 当前职责 |
| --- | ---: | --- |
| `tests/`根目录 | 54 | 旧Game诊断、时钟/启动、当前世界查询/会话及桌面表现混排 |
| `tests/simulation/rules/` | 77 | 完整世界规则和规则间组合 |
| `tests/simulation/`直属 | 9 | 新局、运行时、路由、页面及连续世界 |

共140个C++测试文件、36,915行，每个文件自带`main()`，133个自定义`check()`；另有3个Node测试、2个CMake测试脚本和2个公共辅助头。17对根目录/规则目录测试同名；抽查`character_hp_test.cpp`，除来源注释、头文件和命名空间外正文相同，但分别验证两套实现。独立main或短文件本身不等于无效测试；当前同时存在11个不足100行和1个超过1,000行的文件，不能用统一行数阈值决定拆合。

该审计时根CMake存在重复注册和重复列出桌面支持源码；`cmake/WorldSimulation.cmake`已用显式清单和循环注册，但仍每文件一个进程且统一链接运行时。应优先改进职责归属、重复辅助代码和构建样板，而不是直接减少测试断言。`assets/simulation/SOURCES.json`冻结了87个测试文件，迁入测试整理必须保留来源与适配记录。当时仅额外长期世界测试有`long_world`标签；通用主责标签仍是后续整理目标，本批新增的长期标签见下。

### 以契约和风险分层

阶段设计在已有阶段记录中用一张小表列出“行为契约/来源、风险与代表输入、主责套件、验收方式”，无需逐个断言建台账。优先查找现有套件；缺少覆盖才补用例。期望值来自已证规则、固定发布数据或独立不变量，未知规则登记研究缺口。

| 测试层 | 主责 | 用例取舍 |
| --- | --- | --- |
| 规则 | 计算、边界、合法/非法输入、无副作用 | 等价类+边界值，同逻辑多输入表驱动；有明确性质时用固定种子的性质检查 |
| 运行时 | 唯一Owner、跨域次序、原子提交、失败回滚、共享随机 | 少量跨域成功/失败场景；断言资金、占用、名册、随机等关联结果 |
| 适配与表现 | 命令映射、快照、坐标/布局、只读显示 | 纯映射尽量headless；真实窗口与OS输入单独验收 |
| 端到端/来源 | 真实新局组合、打包路径、发布数据/哈希、持续推进 | 保留标准三个月基线；额外12/6/24个月按阶段风险显式开启 |

例如“设施到达只收费一次”：规则套件负责价格/资格边界，运行时套件负责重复提交及后续失败时资金/占用一起回滚，表现套件只验证读到实际收入，不再枚举价格组合。跨层共同出现某行为不算冗余，重复相同输入/结果而无新增风险才是合并候选。

### 文件、夹具与构建

- 以稳定职责组织逻辑套件，如设施、人物、运行时事务、页面、表现；明确当前世界和legacy归属。优先扩展现有文件中的具名场景；职责、依赖、来源或隔离需求不同才拆文件，不按阶段/bug编号建一次性套件。过大文件按行为拆分，不能改成串行依赖的巨型流程。
- 产品自有测试逐步抽取最小断言/失败诊断；领域夹具独立于通用断言，限定在真正使用它的套件。第二处语义相同的使用出现时再考虑共享，不预建万能世界工厂或测试框架。每个用例重新创建可变状态，seed、逻辑tick、自动确认策略显式给出，避免墙钟sleep和执行顺序依赖。
- 夹具准备输入，断言验证结果；不在辅助函数中藏业务分支，也不用被测计算产出自己的期望值。表驱动失败输出场景名、输入/seed和预期/实际差异；不要用Release会关闭的裸`assert`承载测试检查。
- CMake复用轻量注册函数或显式清单，集中配置警告、超时及标签，源文件仍显式登记；只链接需要的模块。文件可共享一个职责套件target，具名场景仍可定位，必要时可筛选独立执行。需要窗口、独立进程状态、不同超时或冻结来源的测试保持隔离；不强推整个项目单一可执行文件，也不默认引入新框架。
- 整理时逐步采用主责标签`rules/runtime/presentation/e2e/provenance/legacy`，保留额外长跑`long_world`；标准CTest仍完整执行，标签用于定位和统计。迁入测试与产品补充场景分清来源；不得为统一风格直接改写冻结测试或重算清单掩盖差异。

### 阶段末去重与验收

1. 按本阶段契约表检查正常路径、关键边界和失败路径，缺陷回归放入其行为主责套件；不补仅验证私有布局、转发代码或低风险可逆改动的机械测试。
2. 核对新用例是否提供不同风险覆盖；重复输入改为表驱动，重复夹具局部共享，上层只保留组合保障。旧诊断仍有入口时保留其独立回归，不凭同名删除。
3. 合并/移除测试须给出旧契约→保留场景映射，保持有效断言、错误拒绝、独立oracle与来源依据。涉及冻结迁入结构先明确机械适配及校验方式，禁止修改research或放宽哈希校验来完成整理。
4. 实现和测试收口后按[构建检查](#构建检查)执行默认desktop-debug及改动范围要求的追加检查，Debug默认排除三个月用例；记录配置选择、失败、修复、排除项与对应提交的CI结果。长期回归按实际风险使用headless-release。记录新增/合并理由与显著耗时变化，不追求文件数、断言数或覆盖率指标；代码检查、产品行为、真实窗口和原版一致性分别报告。

本批已落地产品自有支撑整理并通过四套标准测试及集中长测：`tests/support/world_fixture.hpp`以真实新局初始化、seed1每进程不可变缓存和逐用例独立副本复用准备过程，`same_world_clock`统一六个日期字段及世界/模拟/到访计数比较，业务和随机断言由各套件保留。显示边界用的小型合成夹具保持局部；不隐藏自动确认或额外推进。`ark_world_ui_test_support`共同编译布局、皮肤、资源和投影，轻量CMake注册保留独立UI套件与全部断言。冻结研究测试不为风格整理改写，未因同名删除旧切片覆盖。

长测显式分层：标准三个月连续基线仍在四套CTest内注册，本地Debug按上述策略排除，CI在Release中完整执行。`ARK_LONG_WORLD_TESTS=ON`额外注册`long_world`，其中`annual_world`为12/6/24个月三种子/倍速，`natural_tasks`为seed1双轮及seed20261005单轮的实际任务接受至自然成功链。自然任务目标保留四套构建支持，实际只编译所选配置，按风险优先使用headless-release，不默认要求Debug长链；避免按桌面/headless重复相同核心轨迹，保留原参数、逻辑预算与断言。CI默认不开启额外长测，不得称其已覆盖这些轨迹；需要时单独登记执行范围与结果。

## 本轮测试目录与套件整理（2026-10-05）

以下为该批历史方案与实测结果；后续执行范围以[构建检查](#构建检查)的本地Debug排除政策为准。

本轮沿用户明确授权实施上文整理规则。主要风险是搬移漏注册、来源哈希变化、case合并后误共享状态或吞掉失败；对应保留旧CTest清单、冻结文件哈希、各case独立进程，以及原断言表达式/消息/顺序。

| 契约 | 方案与验收 |
| --- | --- |
| 原有覆盖与定位 | 61个产品文件按职责移动，文件名保留；配置后比对原139/155项标准名称，无缺失；后续菜单修复额外增加world_menu，桌面为156，原参数与超时核对 |
| 来源与隔离 | research只读；tests/simulation路径/正文/SOURCES哈希不变，标签由CMake设置 |
| 月报/勋章与世界UI | 6份独立正文共享两个显式case入口，CTest名称和进程隔离保留；102处静态断言调用表达式/消息/次序保持，运行时循环次数不改 |
| 构建重复 | 产品注册集中到cmake/ProductTests.cmake，复用target/case配置；UI共享依赖和实现只编译/链接所需次数 |
| 验收时机 | 完成本批改动后统一四套构建/标准CTest，每套三个月基线；长测只核对注册与不变参数，不重复前批6条自然/年度执行 |

详细目录、原target→新case映射和运行命令见[tests/README](../tests/README.md)。最终合并代码四套构建和590项标准CTest均通过（139/139/156/156）；各套三个月基线通过。本次没有重跑参数/正文未变的额外自然/年度长链。72份产品C++格式、原102处断言顺序/表达式保留、既有CTest名称/属性、冻结来源与新增标签均复核通过。两个聚合runner缺参/未知case共4项拒绝检查通过。用户中途追加菜单/可见问题，菜单FIFO和raw24人物展示随本批验收；中断前不完整日志单独标为pre-interruption，不计入最终结果。

/tmp清理按项目命名、内容归属及活跃进程/打开文件记录检查：已删除434个顶层临时项（7121文件，2,945,283,183字节），包括旧Godot导入缓存、产品/研究临时构建、探针、日志和截图；其他应用临时文件未处理。旧阶段/tmp路径是当时证据位置，现已按本次要求清理；历史数值保留在阶段文档。当前清理明细和验收日志放忽略的`build/validation/test-organization/`，阶段末已删除本次新建三个临时构建/配置目录约3.54GB，当前日志/清单/截图保留在上述忽略目录。

## Git与研究边界

新main是产品重置起点。小步本地提交，不夹带research或其他智能体未完成改动；构建/个人IDE/研究work不提交。
推送或远程历史改写需用户明确指令；不能把本次本地重置自动推成force-push。
research已有外部链接的UI/C/RQ编号保留为交接身份。按已提交维护源冻结、迁入和核对哈希，不夹带在途研究改动；研究测试结果与产品验收分别记录。

当前默认启动、`--world`别名及`--check`都选择持续世界。旧建设切片通过`--legacy-slice`或命名旧诊断进入，`--tick-rate`只用于这类旧入口；`--verify-play`保留原节拍。raw49只接晋级条件读取/四项显示/确认；e8批次另经原raw48消费者申请晋级，不能从raw49或普通确认旁路收费。长期回归通过 `-DARK_LONG_WORLD_TESTS=ON` 显式开启，建议Release、`ctest -L long_world --parallel 1`，避免与试玩/其他长跑争抢CPU。自然任务生成、玩家接受后自然成功及无限运行分别验收，不能互相代替。

## macOS窗口检查

窗口必须在InitWindow前启用FLAG_WINDOW_HIGHDPI，画布按GetRenderWidth/Height分配；逻辑布局尺寸不用于纹理分辨率。
有界运行会打印window/framebuffer/canvas尺寸。默认1080×720在2×Retina下framebuffer/canvas为2160×1440；以实际显示器倍率为准。
缩放清晰度对比使用同一窗口/镜头/zoom-percent；PNG物理尺寸与窗口点尺寸不同，查看时保留原图像素。

raylib/GLFW需要可访问的登录桌面和唤醒显示器，WindowServer存在不等于当前上下文能访问显示器。
程序在InitWindow前用CoreGraphics检查显示器，无可用显示器时立即返回错误。
无窗口检查使用`ark_village --check`；真实桌面使用`ark_village --frames 60`等有界运行。
必要时`caffeinate -d -u <程序> --frames 60`只在进程期间防止休眠，不改永久电源设置，也不授予桌面权限。
沙箱可能无法枚举真实显示器。应在已登录终端运行或获准使用独立窗口检查，不移除显示器保护、不把沙箱报错记成游戏崩溃。
截图/焦点问题和业务异常分别记录；环境无法运行窗口时仍做构建/CTest，窗口验收明确记未执行。

世界页面检查：`ark_village --inspect-page world-active --frames 8 --screenshot /tmp/ark-world.png`，也支持world-month/world-rank/world-award；从实际窗口视口预运行真实世界，明确给予前序页面测试确认输入，停在实际多人/月报/晋级条件/年度贡献状态。年度预运行可能耗时数分钟，不能用合成状态替换真实新局轨迹。
任务诊断使用`--inspect-page world-task-recruitment`、`world-task-team`或`world-task-result`，仍需`--frames`限制窗口。它们有界预运行真实新局，以明确测试输入接受可负担任务、征集/出发并停在同一任务征集/队伍/成果页，不能改成合成人物/任务/奖励夹具；失败保留错误和输入身份，截图不是OS输入验收。
旧页面检查如`--inspect-page shops/motion`自动选择旧切片。motion覆盖检查用行程，不修改领域人物/资金，也不代表默认AI。缩放检查可加`--zoom-percent 50..200`。
页面快照只验渲染，不等于正常新局/OS输入。共享controller坐标测试单独记录，不能代替真实鼠标验收。原生自动化连接裸raylib或隔离bundle可能阻塞，优先有界截图与坐标测试，不反复连接或移除桌面保护。
SEB检查须区分结构和使用帧：完整记录/legacy_tag原样保留，PNG矩形只检查实际请求帧；不能为未用草地/海面记录修改原图或全面放宽边界。

## 文档维护

docs根目录只维护目标/架构、开发流程、计划/决策三份概览。原版对照与研究需求集中在reference子目录。
近期可执行任务维护在根TODO；概览链接到它，不重复复制待办。
短决策直接追加到计划文档，当前不为每个ADR新建文件；单个决策需要长篇设计或数量明显增长时再提取到decisions子目录。
详细阶段设计或对照案例按需分别放stages、reference/cases，只有实际文档时才建目录；概览保留索引与状态，不重复正文。
移动/合并文档要同步产品入口；研究侧外部引用由研究维护者处理，产品侧不修改其规格、证据或状态。

<a id="godot-首次导入与自动退出"></a>

此历史research链接只保留定位；当前运行约束见 [macOS窗口检查](#macos窗口检查)，没有旧平台依赖或导入流程。
