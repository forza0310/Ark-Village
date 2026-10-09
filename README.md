# Ark-Village

以 **C++17 + raylib** 复刻《冒险迷宫村》一代的玩法、数值、UI和操作顺序，为后续3D表现保留边界。以research的固定APK维护交付为规则来源，Windows原游戏截图用于视觉参考，不接Steam软件。

默认先显示标题，可新建或继续已有手动档；进入后运行持续世界：地图、设施、人物/怪物、资金、任务、日期和页面由同一Owner持有。已接建设/道路/移动/撤除、人物经营、任务与授勋、晋级/扩张、村办0–3及5/6、商会/魔法壶/普通道具，以及两栏手动存读档。情报菜单、部分经营入口和完整动画仍待接。

规则/运行时冻结至 **89f157c维护闭包，共391项来源记录**，保留商品79/72、设施口碑82、魔法壶41–47与村办5/6、购买后举物与建筑升降提示。建设图块复用研究只读查询；维护回放新增显式表现请求，桌面不按FPS触发随机或任务清理。玩家使用ARKSAVE1/schema4，保存40条配方进度及定义0的主角资料，旧档明确拒绝且不迁移；完整AVRSAVE1/AVRAPP01仅用于维护与精确回放；独立system.arksys保存跨局纪录及继承。当前批次状态与表现边界见[B1](docs/stages/B1-playable-prototype.md#research-89f157c-integration)。

[文档分类索引](docs/README.md) · [当前待办](TODO.md) · [架构](docs/ARCHITECTURE.md) · [研究/产品差距](docs/reference/REFERENCE_CHECKLIST.md#research-history-current-audit) · [历史验收](docs/stages/history/B1-implementation-log.md)

## 构建运行

当前以Windows x64为构建和分发平台。[GitHub Actions](.github/workflows/ci.yml)仅在Windows runner执行desktop-release构建及全部标准CTest，成功后提供`ark-village-windows10-x64.zip`及SHA-256。解压后直接运行`ark_village.exe`：所需Ark、raylib和C++运行库DLL、中文字体与许可随包提供，无需另外安装依赖。开发与测试共用DLL，避免静态代码重复进入每个测试程序。字体按当前产品字形生成子集，包内只保留运行文件；Actions制品保留7天。只有desktop-release发布玩家游戏包，Debug/headless均为开发配置。本地默认desktop-debug验收，排除三个月测试；核心边界或长模拟检查按需使用headless，详见[构建检查](docs/CONTRIBUTING.md#构建检查)。

成功构建会发布到[GitHub Releases](https://github.com/forza0310/Ark-Village/releases/latest)，直接下载Windows ZIP及SHA-256附件，无需容器工具；历史版本按提交保留。实际CI系统为Windows Server 2022，Windows 10真机另验。CI按上述分工执行，首次Releases发布仍待main运行确认；工具版本、存储策略和下载方式见[CI说明](docs/CONTRIBUTING.md#github-ci与制品)。

开发需要CMake 3.21+、Ninja、Node 18+（仅构建）、pkg-config、LLVM-MinGW x64及共享raylib 6.0。设置x64编译器和raylib的`PKG_CONFIG_PATH`后，在PowerShell中运行（完整依赖准备见[Windows本地构建](docs/CONTRIBUTING.md#windows本地构建)）：

```powershell
cmake --preset shared-libraries -G Ninja -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-clang++ -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres -DCMAKE_EXE_LINKER_FLAGS= -DARK_DESKTOP_FONT="$PWD/build/local-tools/fonts/default.otf" -DARK_DESKTOP_FONT_LICENSE="$PWD/build/local-tools/fonts/OFL.txt"
cmake --build --preset shared-libraries --parallel 4
cmake --preset desktop-release -G Ninja -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-clang++ -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres -DCMAKE_EXE_LINKER_FLAGS= -DARK_DESKTOP_FONT="$PWD/build/local-tools/fonts/default.otf" -DARK_DESKTOP_FONT_LICENSE="$PWD/build/local-tools/fonts/OFL.txt"
cmake --build --preset desktop-release --parallel 4
ctest --preset desktop-release
.\build\bin\ark_village-desktop-release.exe
```

默认启动和显式 `--world` 都先进入标题，再启动持续世界；`--check` 无窗口校验资源和世界初始化。标题点“开始游戏”选择两栏手动档，空栏进入城镇名称／冒险者姓名／性别／开始游戏配置，有档可“继续游戏／新游戏”；取消保留草稿，开始不覆盖旧档。右键或Esc逐级返回，Enter/Space确认，上下键选择。标题和游戏内系统菜单可查看两页跨局纪录，左右翻页；删除仍禁用。有界`--frames`世界诊断仍直达世界；`--inspect-page world-title|world-title-slots|world-title-actions|world-title-records|world-title-cash|world-title-configure|world-title-text|world-title-clear`用于有界截图；计分页是明确阶段夹具。来源/边界见[标题批次](docs/stages/B1-playable-prototype.md#title-start-flow)。
确认启动介绍和后续事件页后，世界才按原有资格继续推进，首名冒险者在420次有资格的到访更新后自动免费加入。页面等待输入时不会自动跳过教程或奖励。

窗口默认1080×720（3:2），支持 `--size 宽 高`、调整窗口大小、左键拖动地图和滚轮约5%步长缩放（25%～200%）；也可用`--zoom-percent 25`启动全景视图，默认仍100%。左键短点在松开时选择，右键在主场景打开菜单、在菜单/已有返回页逐级退出。底部按钮或Space暂停/继续，试玩固定一倍速，读取旧档沿用当前速度。Retina使用原生framebuffer，素材最近邻采样，文字按显示密度生成。

逻辑由独立模拟线程提交，主线程读取不可变快照以60FPS绘制，普通行走插值平滑。当前47ms独立更新间隔是产品桌面调度政策，卡顿不补算；它不是原Android框架生命周期或实际FPS的等价认证。四套程序共用无调试符号的Release核心库；`desktop-debug`用于调试应用和测试消费者，不能完整单步调试库内部。CLion打开本目录的 `CMakeLists.txt` 即可。

左上月报自动显示击倒/点数和收支两阶段，无需确认，也不暂停世界或阻断菜单、人物与任务操作。每阶段沿原消费者累计70次有资格更新，关闭时只发放一次村子点数，维护费不重复扣除；其他源模态与场景资格照常生效。玩家显式暂停仍冻结世界与报告，只有明确继续才恢复。日期显示年/月/周，周内进度条只映射已证的`units/10800`；精确原版皮肤、尺寸和方向尚待研究。

主场景无其他模态页面时，点击底部“菜单”或按M打开五项菜单；可见月报不阻止开菜单。M、Esc或再次点击菜单关闭。菜单期间冻结世界，关闭不改变手动暂停。未暂停时可选择“建设”、“冒险”或“村办”，“系统”在手动暂停时也可用；情报禁用。原版完整菜单生命周期尚未交付，开关属于已确认的桌面适配。

“系统→保存/读取”提供两个手动栏位，显示村名、日期与资金；覆盖已有档和读取均须明确确认。首版只接受稳定主场景，未结束的月报、事件/问答页或离村处理中不能存读档。默认目录为`%LOCALAPPDATA%/Ark-Village/saves`，可用`--save-dir PATH`指定隔离目录。保存失败保留旧档，读取失败保留当前世界；读取保留玩家当前手动暂停。共同随机不写入文件，同进程沿用当前流，冷启动使用新流。文件是当前版本Ark格式，不兼容原APK/旧demo档；自动中断档仍待轮内恢复合同。退出不自动保存。

- **建设**：菜单→建造或B打开三分类目录，左侧显示原尺度64×32建筑裁片、右侧名称和当前报价；点击列表/上下键选择、左右键切换分类，点击行或Enter进入完整建筑与占地预览。目录按S057采用窄五行、顶部分类、名称橙底/手形和右侧滚动条，右下返回；不再显示页内“建设”大按钮。点击地图锁定位置，R或“旋转”切换朝向，Enter或“建设”提交；成功后保持连续放置，Esc/返回退出。候选按源场景计数闪烁，缺少朝向帧时空绘；预览不占地或扣款，模拟线程提交时重新验证。施工计时已接，地基/工人/脚手架等分阶段动画待精确资源合同。
- **村办与商会**：菜单→村办选择活动、南瓜商会或魔法壶；V直达活动、C在商会解锁后直达商会、K在魔法壶解锁后直达魔法壶。该子菜单层级为桌面适配，交易资格仍读源flag16。活动确认才扣村子点数，执行满120源计数后可结束，结果显示原序参与人物及实际旧/新属性。商会支持金币买卖道具、村子点数兑换设施及只读设施预览；兑换先付点数、领取再授予建设权限，实际建设仍收费。奖励95沿原40计数确认门槛领取，绘制不发奖。
- **地图编辑**：“建设→道路植物”可铺路或撤除；首星奖励开放“配置更替”后可移动设施。点击锁定格、Enter提交；铺路分起点/终点，按较长轴生成线段，平局选纵轴，只按实际改格数收费。移动分选旧设施/选新址，保留旧朝向，可按R旋转，成功才付300G。Esc先中止当前线段/落点，再返回主场景；已完成操作保留，撤除不退款。住宅拆除返还H建设次数，目录显示H与800G造价，重建仍收费并重新绑定原住户。
- **设施**：点击当前建筑打开详情，左右键翻页；普通设施可使用自有道具，显示真实价格/品质/魅力变化。道具列表先点击选中，再次点击同一已提交行使用；Enter/Space/小键盘Enter也可确认，键盘或换页清标记。装备商店点“商品”进入四行目录，上下选择、左右切页，I/“情报”查看装备，确认或返回清整类NEW而不购买。设施口碑按两个40计数阶段确认/返回，最终人气由世界后续结算。邻接加成和人物设施属性增长已结算；购买后举物、商品图标及建筑价格/品质/魅力升降提示已接。设施详情类别/效果/来源图标、逐来源加成与普通道具图标、人物属性增长cd13头标已接线，当前验收见B1；完整口碑动画仍待合同。
- **任务**：菜单→冒险或T打开目录；有活动任务时打开当前队伍。征集页按住Enter或“加速”按钮加快源计数，松开/切页/暂停/失焦释放。队伍页支持追加、成员详情和出发；出发演出开始后不可取消。X打开活动任务中止管理，先请求、再在默认“否”的问题页确认；期限页沿原继续/中止流程。完整招募入场/表情动画仍待绘制合同。
- **人物与税收**：点击当前人物打开概况/六属性/装备/魔法四页；P转职、G赠礼，四类装备与第五类普通道具分开。转职预览不改变职业，装备赠礼确认由父页恢复后实际消费现金或共享库存；普通道具使用自有库存，效果显示共享成长属性变化，不把HP上限增长当回血。住宅税收页按原序显示五行和合计，确认只关页，随后自动消费者入账；它与无确认的左上月报不同。完整转职/评价/大师动画及道具图标另待精确合同。
- **年度与晋级**：年度贡献页选择人物后请求授勋，再确认是/否；结束授勋有独立问题。raw48可申请晋级或查看四项条件解释，raw49仅查看，庆典按源计数开放确认。一般返回用取消/Esc，设施强化83目前仅取消，解锁59满70计数可确认。

程序从可执行文件旁的 `assets/` 加载资源，不依赖当前工作目录、research或APK。字体优先使用 `--font`，否则查找程序旁 `fonts/default.otf`、`fonts/default.ttf`，保留macOS历史回退。Windows便携包附Noto CJK衍生字形子集及OFL许可，直接运行exe即可；本地通过 `ARK_DESKTOP_FONT`、`ARK_DESKTOP_FONT_LICENSE` 配置复制。raylib不能直接读取Windows TTC集合，部分系统TTF缺原文符号；不会静默忽略缺字。字体属于桌面依赖，不改研究素材清单。退出不保存。

## 验证与诊断

标准C++世界入口在headless配置中也可构建：

```sh
cmake --preset headless-release
cmake --build --preset headless-release --parallel 4
./build/bin/ark_world_simulation-headless-release.exe --seed 1 --frames 20000 --months 3 --auto-confirm
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
| `ark_village --inspect-page world-active --frames 600`                        | 三人正常场景后以一倍速持续运行，输出绘制/模拟耗时                     |
| `ark_village --inspect-page world-commerce-suite --frames 8 --screenshot suite.png` | 一次自然预运行，独立分支验收六个商会页面，输出六张带页面名后缀的PNG |
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

长回归保持研究原断言：`annual_world` 核对12/6/24个月和自然任务数，`natural_tasks` 覆盖seed1双轮及seed20261005单轮的真实接受至自然成功/后续任务链。两类均由 `ARK_LONG_WORLD_TESTS` 显式开启；自然任务测试保留四套构建支持，按风险选择所需配置，优先headless-release，避免重复相同纯核心长跑。标准三个月基线保留注册，本地Debug默认跳过，三个月行为覆盖依赖main CI的desktop-release完整测试；当前CI不跑Debug或额外长测。仅必要诊断或用户明确要求时补跑Debug长测，命令与记录要求见[构建检查](docs/CONTRIBUTING.md#构建检查)。建议与正常试玩错开；60FPS绘制不代表世界每47ms都能完成计算。

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
