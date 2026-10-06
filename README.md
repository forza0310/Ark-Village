# Ark-Village

以 **C++17 + raylib** 复刻《冒险迷宫村》一代的玩法、数值、UI和操作顺序，并保留未来3D化的调整空间。

当前默认运行持续世界：真实新局地图和完整定义目录、多人/怪物共同调度、设施使用和收入、脚本页栈、日期、月报与人气由同一个世界所有者管理。多人到访、实际设施收入与跨月结算已验收。界面已接入原版地表、道路拼块、栅栏/外部入口、人物/怪物动作、血条、现金浮标及事件页面。

当前产品已迁入研究 **`8f12654` 的315项源/测试/数据**，人物四页详情、转职、四槽装备赠礼与住宅税收已接线，本批验收通过（结果见B1）。正常世界可经菜单“建设”进入目录，预览完整占地并连续放置；点击建筑查看真实经营详情、符合资格时入住/升级；活动任务可追加队员、查看成员及请求中止。年度页支持选择人物授勋，晋级页支持实际申请与庆典；这些动作都提交当前模拟线程，共用地图、资金、人物和随机。

原五项菜单当前开放“建设、冒险”，村办/情报/系统禁用。旅馆两阶段条已接当前职业/性别头像；按5aa5c37新增秘书通知77皮肤/富文本、成长头像/ap前两项、raw30两阶段胜利窗及月报左上头像/收支简表，验收记录见B1。raw67/88/96仍为真实数据的文字展示，完整演出、胜利跳跃/手持武器合成、通知32特殊条及道路建设/移动/拆除另接。旧建设切片仅保留为显式诊断，当前世界建设不会启动第二个Game。产品测试和截图不等于原APK动态行为或OS鼠标验收。

此前已补齐伤害数字、死亡金币、经验/升级标签、旅馆休息/HP条和通知换行，并接独立模拟线程。此前2倍速600帧窗口绘制间隔中位数16.96ms、95分位17.12ms；旧手动月报的冻结结果保留为历史证据，其政策已被本次自动月报决定取代，不能代替当前验收。交互气泡具体文案、人气动态填充、设施精确L锚点、旗帜/连击/手持武器及部分战斗特效仍按已发布研究补齐。

历史接入与验收集中在[B1记录](docs/stages/B1-playable-prototype.md)：33ee056任务闭环冻结298项源/数据，四套588项标准测试及6次长测通过；随后测试整理与菜单/招募展示四套590项通过。此前e8接入通过四套构建、602项标准测试、6次额外自然/年度长测和7个真实窗口检查，最终结果已记入B1。测试结构见[tests/README](tests/README.md)。

## 构建运行

当前以Windows x64为构建和分发平台。[GitHub Actions](.github/workflows/ci.yml)仅在Windows runner执行desktop-release构建及全部标准CTest，成功后提供`ark-village-windows10-x64.zip`及SHA-256。解压后直接运行`ark_village.exe`：raylib和C++运行库静态链接，中文字体与许可随包提供。字体按当前产品字形生成子集，包内只保留运行文件；Actions制品保留7天。Debug和headless预设继续用于本地四套阶段验收。

成功构建会发布到[GitHub Releases](https://github.com/forza0310/Ark-Village/releases/latest)，直接下载Windows ZIP及SHA-256附件，无需容器工具；历史版本按提交保留。实际CI系统为Windows Server 2022，Windows 10真机另验。CI是本地验收以外的额外检查，首次Releases发布仍待main运行确认；工具版本、存储策略和下载方式见[CI说明](docs/CONTRIBUTING.md#github-ci与制品)。

开发需要CMake 3.21+、Ninja、Node 18+（仅构建）、pkg-config、LLVM-MinGW x64及静态raylib 6.0。设置x64编译器和raylib的`PKG_CONFIG_PATH`后，在PowerShell中运行（完整依赖准备见[Windows本地构建](docs/CONTRIBUTING.md#windows本地构建)）：

```powershell
cmake --preset desktop-release -G Ninja -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-clang++ -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres -DCMAKE_EXE_LINKER_FLAGS=-static -DARK_DESKTOP_FONT="$PWD/build/local-tools/fonts/default.otf" -DARK_DESKTOP_FONT_LICENSE="$PWD/build/local-tools/fonts/OFL.txt"
cmake --build --preset desktop-release --parallel 4
ctest --preset desktop-release
.\build\desktop-release\bin\ark_village.exe
```

默认启动和显式 `--world` 都运行持续世界；`--check` 无窗口校验资源和世界初始化。
确认启动介绍和后续事件页后，世界才按原有资格继续推进，首名冒险者在420次有资格的到访更新后自动免费加入。页面等待输入时不会自动跳过教程或奖励。

窗口默认1080×720（3:2），支持 `--size 宽 高`、调整窗口大小、右键拖动镜头和滚轮约5%步长缩放。底部按钮或Space暂停/继续，右侧按钮切换1/2倍速。Retina使用原生framebuffer，素材最近邻采样，文字按显示密度生成。

逻辑由独立模拟线程提交，主线程读取不可变快照以60FPS绘制，普通行走插值平滑。当前47ms独立更新间隔是产品桌面调度政策，卡顿不补算；它不是原Android框架生命周期或实际FPS的等价认证。完整候选事务在Debug中较慢，正常体验建议Release，断点调试选择 `desktop-debug`。CLion打开本目录的 `CMakeLists.txt` 即可。

左上月报自动显示击倒/点数和收支两阶段，无需确认，也不暂停世界或阻断菜单、人物与任务操作。每阶段沿原消费者累计70次有资格更新，关闭时只发放一次村子点数，维护费不重复扣除；其他源模态与场景资格照常生效。玩家显式暂停仍冻结世界与报告，只有明确继续才恢复。日期显示年/月/周，周内进度条只映射已证的`units/10800`；精确原版皮肤、尺寸和方向尚待研究。

主场景无其他模态页面时，点击底部“菜单”或按M打开五项菜单；可见月报不阻止开菜单。M、Esc或再次点击菜单关闭。菜单期间冻结世界，关闭不改变手动暂停。未暂停时可选择“建设”或“冒险”，其他三项禁用；原版完整菜单生命周期尚未交付，开关属于已确认的桌面适配。

- **建设**：菜单→建设或B打开三分类目录，左侧显示完整建筑缩略图、右侧名称和当前报价；点击列表/上下键选择、左右键切换分类，确认后进入完整建筑与占地预览。点击地图锁定位置，R或“旋转”切换朝向，Enter或“建设”提交；成功后保持连续放置，Esc/返回退出。当前预览静态半透明，精确原版闪烁节拍待研究；预览不占地或扣款，模拟线程提交时重新验证。
- **设施**：点击当前建筑打开详情，左右键翻页；入住候选、升级和返回沿实际页面资格提交。道路建设、移动、拆除及未交付商品/道具操作不提供。
- **任务**：菜单→冒险或T打开目录；有活动任务时打开当前队伍。征集页按住Enter或“加速”按钮加快源计数，松开/切页/暂停/失焦释放。队伍页支持追加、成员详情和出发；出发演出开始后不可取消。X打开活动任务中止管理，先请求、再在默认“否”的问题页确认；期限页沿原继续/中止流程。完整招募入场/表情动画仍待绘制合同。
- **人物与税收**：点击当前人物打开概况/六属性/装备/魔法四页；P转职、G赠送装备，目录选择后明确确认。转职预览不改变职业，赠礼确认由父页恢复后实际消费现金或共享库存。住宅税收页按原序显示五行和合计，确认只关页，随后自动消费者入账；它与无确认的左上月报不同。完整转职/评价/大师动画及普通道具另接。
- **年度与晋级**：年度贡献页选择人物后请求授勋，再确认是/否；结束授勋有独立问题。raw48可申请晋级或查看四项条件解释，raw49仅查看，庆典按源计数开放确认。一般返回用取消/Esc，设施强化83目前仅取消，解锁59满70计数可确认。

程序从可执行文件旁的 `assets/` 加载资源，不依赖当前工作目录、research或APK。字体优先使用 `--font`，否则查找程序旁 `fonts/default.otf`、`fonts/default.ttf`，保留macOS历史回退。Windows便携包附Noto CJK衍生字形子集及OFL许可，直接运行exe即可；本地通过 `ARK_DESKTOP_FONT`、`ARK_DESKTOP_FONT_LICENSE` 配置复制。raylib不能直接读取Windows TTC集合，部分系统TTF缺原文符号；不会静默忽略缺字。字体属于桌面依赖，不改研究素材清单。退出不保存。

## 验证与诊断

标准C++世界入口在headless配置中也可构建：

```sh
cmake --preset headless-release
cmake --build --preset headless-release --parallel 4
./build/headless-release/bin/ark_world_simulation --seed 1 --frames 20000 --months 3 --auto-confirm
```

`--seed` 是明确的可重复Java种子输入，未认证原APK默认种子；`--auto-confirm` 是测试用户输入策略。`--speed 0|1` 对应正常/双倍源轮数；跨年度测试另加 `--end-awards`，明确请求并确认结束授勋，普通 `--auto-confirm` 不替代年度选择。`--frames` 为无节拍框架调用预算，`--months` 是预算内目标，未达到时返回非零退出码。

| 入口                                                                            | 用途与边界                                                            |
| ------------------------------------------------------------------------------- | --------------------------------------------------------------------- |
| `ark_village --check`                                                         | 无窗口资源和持续世界初始化检查                                        |
| `ark_village --frames 60`                                                     | 有界默认世界窗口，可加`--screenshot /tmp/ark.png`                   |
| `ark_village --inspect-page world-menu --frames 120`                          | 真实三人新局后经FIFO打开菜单；保留显式未暂停，检查独立菜单冻结        |
| `ark_village --inspect-page world-task-recruitment --frames 120`              | 真实接受任务后停在已显示源队首人物的征集页                            |
| `ark_village --inspect-page world-building --frames 120` | 预运行调用源建设目录消费者，检查分类/当前报价 |
| `ark_village --inspect-page world-details --frames 120` | 预运行调用源设施消费者，打开当前建筑详情 |
| `ark_village --inspect-page world-built --frames 120` | 预运行调用源目录/建设事务，检查同一地图与账本 |
| `ark_village --inspect-page world-award-granted --frames 120` | 预运行经源年度消费者请求并确认授勋，停在真实奖励展示 |
| `ark_village --inspect-page world-active --frames 8`                          | 真实新局预运行至三人到访，再检查画面                                  |
| `ark_village --inspect-page world-month --frames 8`                           | 真实新局预运行至月报，再检查画面                                      |
| `ark_village --inspect-page world-award --frames 120`                         | 从真实新局预运行至年度贡献页；可能耗时数分钟，前序确认仅为测试输入    |
| `ark_village --inspect-page world-task-team --frames 8`                       | 有界真实新局输入，接受当前可负担任务并征集到队伍页；可能耗时数分钟    |
| `ark_village --inspect-page world-task-result --frames 8`                     | 接续真实接受/出发，自然运行到同一任务成果页，不注入任务或奖励         |
| `ark_village --inspect-page world-rank --frames 8`                            | 等待实际raw49条件页初始化，再检查四项条件                             |
| `ark_village --inspect-page world-combat --frames 8`                          | 停在真实受击数字；另有world-reward/world-exp/world-rest/world-rest-hp |
| `ark_village --inspect-page world-news --frames 8`                            | 真实冒险通信；world-break检查带换行标记的通知                         |
| `ark_village --inspect-page world-speed --frames 600`                         | 三人正常场景后以2倍速持续运行，输出绘制/模拟耗时                      |
| `ark_village --legacy-slice`                                                  | 旧有限建设/首访生活切片                                               |
| `--ai-preview`、`--check-ai`、`--verify-play`、旧 `--inspect-page` 名称 | 自动选择旧切片的显式诊断，不作为默认世界验收                          |

`world-building/details/built/award-granted`的诊断预运行直接调用源消费者，不经过窗口worker FIFO或OS鼠标；它们检验实际源状态的窗口呈现。正常UI命令的FIFO、回执与事务契约由产品会话测试覆盖，不能把这些截图称为窗口自动点击验收。

`world-active/world-month/world-rank` 不需要额外写 `--world`；它们按实际窗口视口预运行并自动确认前序真实页面，仅作为渲染检查。`--verify-play` 使用共享controller的引擎内坐标，不能替代OS鼠标测试。`--tick-rate 1..240` 可用于 `--legacy-slice` 或允许覆盖时钟的命名旧诊断（如 `--ai-preview`）；默认世界及 `--world` 不接受覆盖，`--verify-play` 仍须原节拍。

年度/多种子/倍速长回归默认不纳入日常CTest，可在Release中显式开启：

```sh
cmake --preset headless-release -DARK_LONG_WORLD_TESTS=ON
cmake --build --preset headless-release --parallel 4
ctest --preset headless-release -L long_world --parallel 1
```

长回归保持研究原断言：`annual_world` 核对12/6/24个月和自然任务数，`natural_tasks` 覆盖seed1双轮及seed20261005单轮的真实接受至自然成功/后续任务链。两类均由 `ARK_LONG_WORLD_TESTS` 显式开启；自然任务测试在四套配置中编译，阶段末按Debug/Release分别执行，避免仅因渲染后端重复相同纯核心长跑。标准三个月基线仍在每套日常CTest中执行。建议与正常试玩错开，Debug完整候选复制和并行长测会显著拉长逻辑推进时间；60FPS绘制不代表世界每47ms都能完成计算。

旧切片七种单格建设、局部人物交接及1456步月前保护的设计与历史验收集中保留在 [B1记录](docs/stages/B1-playable-prototype.md)，这些限制不适用于默认持续世界。

## 目录与阅读入口

| 路径                                                                   | 用途                                                                            |
| ---------------------------------------------------------------------- | ------------------------------------------------------------------------------- |
| `research/`                                                          | 研究侧维护的规则、原型、素材与截图，产品侧只读                                  |
| `include/ark/simulation/rules/`、`src/simulation/rules/`           | 标准C++领域规则，`ark_world_rules`                                            |
| `include/ark/simulation/`、`src/simulation/`                       | 初始化、唯一世界Owner及跨域运行时，`ark_world_runtime`                        |
| `src/app/`                                                           | 启动参数、时钟、世界模拟线程/报告诊断、只读设施查询、无窗口入口及旧切片协调     |
| `src/desktop/`                                                       | raylib窗口、资源、输入、投影及UI                                                |
| `src/world/`、`src/facilities/`、`src/people/`、`src/economy/` | 旧建设切片保留的标准C++模块                                                     |
| `src/assets/`、`assets/`                                           | 素材解析、运行副本及来源清单                                                    |
| `scripts/`、`tests/`                                               | 数据编译/显式素材导入、按职责组织的测试与来源校验，见[测试说明](tests/README.md) |
| `docs/`                                                              | 架构、流程、计划；长期对照在`reference/`，历史阶段在 `stages/`              |

从 [架构](docs/ARCHITECTURE.md)、[开发流程](docs/CONTRIBUTING.md)、[计划与决策](docs/MILESTONES.md)开始阅读。近期任务见 [TODO](TODO.md)，研究覆盖与产品缺口见 [原版对照](docs/reference/REFERENCE_CHECKLIST.md)及 [研究需求](docs/reference/RESEARCH_REQUESTS.md)。
