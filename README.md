# Ark-Village

基于 `research/` 的逆向交付，以 **C++17 + raylib** 复刻《冒险迷宫村》一代的玩法、数值、UI和操作顺序，并保留未来3D化的领域边界。

当前默认运行持续世界：真实新局地图和完整定义目录、多人/怪物共同调度、设施使用和收入、脚本页栈、日期、月报与人气由同一个世界所有者管理。多人到访、实际设施收入与跨月结算已验收。界面已接入原版地表、道路拼块、栅栏/外部入口、人物/怪物动作、血条、现金浮标及事件页面。

完整世界已实现任务入口、接受/征集/出发/期限及成果返回。当前恢复原五项菜单外观，先开放“冒险”进入任务目录，菜单打开冻结世界、关闭保留显式暂停；其余四项禁用。建筑点击详情/建设、完整菜单子页、完整投射物/一般特效或文件存取仍未提供。当前Owner的只读设施查询底层已备妥，建筑详情窗口的桌面适配方案待确认。旧建设切片保留为显式诊断入口，不能把其中的建设界面当作持续世界已经完成的功能。产品测试和截图也不等于原APK动态行为或OS鼠标验收。

已补齐伤害数字、死亡金币、经验/升级标签、旅馆休息/HP条、通知换行及原素材模板，并改为月报手动确认与独立模拟线程。此前的2倍速600帧窗口绘制间隔中位数16.96ms、95分位17.12ms，月报等待120帧时世界更新0次。交互气泡具体文案、人气动态填充、旅馆头像/精确锚点及部分战斗特效仍待研究补齐。

此前已迁入研究40972a9的全局遭遇、定时/镜头/怪物介绍页及年度贡献/结束流程，历史验收见[B1记录](docs/stages/B1-playable-prototype.md#已发布持续世界修正接入2026-10-05)。raw49已有条件读取/显示/确认，未接晋级收费；年度完整授予/raw88仍待研究。随后已接成果页队员统计和活动任务的场景攻略条/目标轮廓；通知底栏尚缺队列生命周期桥，不能直接绘制永久消息。该批任务显示采用明确夹具验证，历史四套配置共570项标准CTest通过；更早一批完成三个12/6/24个月长期回归。

上一批已按研究33ee056迁入任务自然闭环及救援/携物/路线/名单修正，冻结298项源/数据，保留勋章共享字段的产品适配记录。任务操作通过模拟线程向同一Owner提交。四套编译及588项标准CTest、6次额外长测全部通过，真实新局队伍/自然成果窗口已检查；两种子任务链包含自然成功、成果返回和后续任务。详见[B1本批验收](docs/stages/B1-playable-prototype.md#任务自然闭环产品接入2026-10-05)。

本轮已完成测试目录/套件整理与菜单、招募人物展示修正，四套构建和590项标准CTest通过；真实菜单/征集窗口已检查。测试结构见[tests/README](tests/README.md)，当前差异与验收边界见[B1最新记录](docs/stages/B1-playable-prototype.md#菜单恢复与招募展示2026-10-05)。

## 构建运行

main上的[GitHub Actions](.github/workflows/ci.yml)分别在macOS ARM与Windows runner执行desktop-release构建及全部标准CTest，成功后提供`ark-village-macos-arm64`和`ark-village-windows10-x86`的Release压缩包及SHA-256。Debug和headless预设保留，但不在日常CI中重复执行。下载入口在Actions运行页的Artifacts，保留14天；Windows目标是32位、实际CI系统为Windows Server 2022。构建/测试仅在GitHub runner上执行，首次远程结果待验证；制品内容、字体启动方式和验收边界见[CI说明](docs/CONTRIBUTING.md#github-ci与制品)。

需要CMake 3.21+、C++17、Node 18+（仅构建）、pkg-config及raylib 6.0+。macOS可用：

```sh
brew install cmake node pkg-config raylib
cmake --preset desktop-release
cmake --build --preset desktop-release --parallel 4
ctest --preset desktop-release
./build/desktop-release/bin/ark_village
```

默认启动和显式 `--world` 都运行持续世界；`--check` 无窗口校验资源和世界初始化。
确认启动介绍和后续事件页后，世界才按原有资格继续推进，首名冒险者在420次有资格的到访更新后自动免费加入。页面等待输入时不会自动跳过教程或奖励。

窗口默认1080×720（3:2），支持 `--size 宽 高`、调整窗口大小、右键拖动镜头和滚轮约5%步长缩放。底部按钮或Space暂停/继续，右侧按钮切换1/2倍速。Retina使用原生framebuffer，素材最近邻采样，文字按显示密度生成。

逻辑由独立模拟线程提交，主线程读取不可变快照以60FPS绘制，普通行走插值平滑。当前47ms独立更新间隔是产品桌面调度政策，卡顿不补算；它不是原Android框架生命周期或实际FPS的等价认证。完整候选事务在Debug中较慢，正常体验建议Release，断点调试选择 `desktop-debug`。CLion打开本目录的 `CMakeLists.txt` 即可。

月报等待确认时暂停世界：第1页点击“下一页”，第2页点击“确定”后恢复；Enter也可确认。如果此前手动暂停，关闭月报后仍保持暂停。人物、日期、设施与随机在等待期间均不推进，最终确认发放一次村子点数。

主场景无其他页面/月报时，点击底部“菜单”或按M打开原五项菜单；M、Esc或再次点击菜单关闭。菜单期间冻结世界，关闭不改变手动暂停。未暂停且无活动任务时选择“冒险”打开任务目录，也可在主场景直接按T。点击列表或用上下键选择，点击确认或按Enter接受；征集页按住Enter或“加速”按钮加快原征集计数，松开、切页、暂停或失焦即释放。队伍页可选择追加队员并支付实际费用，或选择出发；出发演出开始后不能取消。期限页选择继续或中止再确认，成果页显示真实队员统计后确认返回。一般返回用取消或Esc；设施强化页83目前仅提供取消，解锁页59需原计数满70才可确认。原版完整菜单生命周期尚未交付，当前开关属于已确认的桌面适配。招募页已恢复按源名单切换的人物展示，完整入场/表情动画仍待研究绘制计划。

年度贡献页显示真实排名与持有勋章；“结束授勋”先询问是/否，选择否继续停留，选择是按真实消费者结束并保留未用勋章。完整授予操作尚未接入。

程序从可执行文件旁的 `assets/` 加载资源，不依赖当前工作目录、research或APK。macOS默认使用系统Arial Unicode.ttf，其他环境通过 `--font /路径/中文.ttf` 指定字体。退出不保存。

## 验证与诊断

标准C++世界入口在headless配置中也可构建：

```sh
cmake --preset headless-release
cmake --build --preset headless-release --parallel 4
./build/headless-release/bin/ark_world_simulation --seed 1 --frames 20000 --months 3 --auto-confirm
```

`--seed` 是明确的可重复Java种子输入，未认证原APK默认种子；`--auto-confirm` 是测试用户输入策略。`--speed 0|1` 对应正常/双倍源轮数；跨年度测试另加 `--end-awards`，明确请求并确认结束授勋，普通 `--auto-confirm` 不替代年度选择。`--frames` 为无节拍框架调用预算，`--months` 是预算内目标，未达到时返回非零退出码。

| 入口 | 用途与边界 |
| --- | --- |
| `ark_village --check` | 无窗口资源和持续世界初始化检查 |
| `ark_village --frames 60` | 有界默认世界窗口，可加 `--screenshot /tmp/ark.png` |
| `ark_village --inspect-page world-menu --frames 120` | 真实三人新局后经FIFO打开菜单；保留显式未暂停，检查独立菜单冻结 |
| `ark_village --inspect-page world-task-recruitment --frames 120` | 真实接受任务后停在已显示源队首人物的征集页 |
| `ark_village --inspect-page world-active --frames 8` | 真实新局预运行至三人到访，再检查画面 |
| `ark_village --inspect-page world-month --frames 8` | 真实新局预运行至月报，再检查画面 |
| `ark_village --inspect-page world-award --frames 120` | 从真实新局预运行至年度贡献页；可能耗时数分钟，前序确认仅为测试输入 |
| `ark_village --inspect-page world-task-team --frames 8` | 有界真实新局输入，接受当前可负担任务并征集到队伍页；可能耗时数分钟 |
| `ark_village --inspect-page world-task-result --frames 8` | 接续真实接受/出发，自然运行到同一任务成果页，不注入任务或奖励 |
| `ark_village --inspect-page world-rank --frames 8` | 等待实际raw49条件页初始化，再检查四项条件 |
| `ark_village --inspect-page world-combat --frames 8` | 停在真实受击数字；另有world-reward/world-exp/world-rest/world-rest-hp |
| `ark_village --inspect-page world-news --frames 8` | 真实冒险通信；world-break检查带换行标记的通知 |
| `ark_village --inspect-page world-speed --frames 600` | 三人正常场景后以2倍速持续运行，输出绘制/模拟耗时 |
| `ark_village --legacy-slice` | 旧有限建设/首访生活切片 |
| `--ai-preview`、`--check-ai`、`--verify-play`、旧 `--inspect-page` 名称 | 自动选择旧切片的显式诊断，不作为默认世界验收 |

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

| 路径 | 用途 |
| --- | --- |
| `research/` | 研究侧维护的规则、原型、素材与截图，产品侧只读 |
| `include/ark/simulation/rules/`、`src/simulation/rules/` | 标准C++领域规则，`ark_world_rules` |
| `include/ark/simulation/`、`src/simulation/` | 初始化、唯一世界Owner及跨域运行时，`ark_world_runtime` |
| `src/app/` | 启动参数、时钟、世界模拟线程/手动报告、只读设施查询、无窗口入口及旧切片协调 |
| `src/desktop/` | raylib窗口、资源、输入、投影及UI |
| `src/world/`、`src/facilities/`、`src/people/`、`src/economy/` | 旧建设切片保留的标准C++模块 |
| `src/assets/`、`assets/` | 素材解析、运行副本及来源清单 |
| `scripts/`、`tests/` | 数据编译/显式素材导入、按职责组织的测试与来源校验，见[测试说明](tests/README.md) |
| `docs/` | 架构、流程、计划；长期对照在 `reference/`，历史阶段在 `stages/` |

从 [架构](docs/ARCHITECTURE.md)、[开发流程](docs/CONTRIBUTING.md)、[计划与决策](docs/MILESTONES.md)开始阅读。近期任务见 [TODO](TODO.md)，研究覆盖与产品缺口见 [原版对照](docs/reference/REFERENCE_CHECKLIST.md)及 [研究需求](docs/reference/RESEARCH_REQUESTS.md)。
