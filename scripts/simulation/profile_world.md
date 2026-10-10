# 世界运行时性能采样

这是显式离线诊断，不属于标准 CTest 或玩家程序。seed1、原240×320视口、speed0、原日历27、真实新局逐框架推进，每帧消费 render cache 和声音；普通非决策页使用与 CLI 相同的显式确认，自动页自行推进。无47ms等待、跳轮、跳消费者或演示初值。2000帧包含首次人物到访、战斗/退休、跨月及4个原时点不可变检查点；不是桌面宽屏商会的同一轨迹。

## 复现

先按项目说明构建公共 Release 库。以下 PowerShell 命令只编译独立诊断 EXE，不重建或替换 DLL：

```powershell
$compiler = 'build/local-tools/llvm-mingw-20250305-ucrt-x86_64/bin/x86_64-w64-mingw32-clang++.exe'
& $compiler -std=c++17 -O3 -DNDEBUG -Wall -Wextra -Werror -Iinclude scripts/simulation/profile_world.cpp -Lbuild/shared-libraries/lib -lark_world_runtime -lark_world_rules -lark_world_save -o build/bin/ark_profile_world.exe
& build/bin/ark_profile_world.exe 2000
```

逐250帧记录 prepare、render、最终 commit-copy；这三列为该区间累计毫秒。wall为进程累计采样时间，包含确认、输出和探针。probe-copy为当前同一个真实Owner的20次复制/销毁平均毫秒；probe-adapter为20次实际adapter构造/销毁平均毫秒。结果用于定位开销，不能解释为稳态FPS、桌面调度速率或原APK速度。

模板复制定位需要显式生成插桩副本：

```powershell
node scripts/simulation/profile_world_generate.mjs
& $compiler -std=c++17 -O3 -DNDEBUG -DARK_PROFILE_INSTRUMENTED -Wall -Wextra -Werror -Ibuild/validation/world-performance/instrumented -Iinclude scripts/simulation/profile_world.cpp -Lbuild/shared-libraries/lib -lark_world_runtime -lark_world_rules -lark_world_save -o build/bin/ark_profile_instrumented.exe
& build/bin/ark_profile_instrumented.exe 2000
```

生成器只读取产品文件，将入口和5个模板头的诊断副本写入忽略的 `build/validation/world-performance/instrumented`；正常产品/CMake不使用该目录。它计量显式Owner复制构造及adapter回调，不覆盖DLL内部的所有复制；各项可能嵌套，不能直接相加解释总占比。插桩调用、额外时间查询和模板在EXE中的编译会扰动耗时。

较长轨迹可使用 `ark_profile_world.exe 12000 bakery`：同一真实新局在第2000帧起，首次稳定主场景且原资金足够时，从真实目录选择35号面包店，按冻结 `natural_progression` 中相同的原道路距离排序，经建设消费者寻找合法完整占地并退出工具；只建一次。设施/人物/任务的其它增长均由原消费者产生，不注入人物、资金、任务或地图。`passive` 为默认策略，保持历史2000帧输入。它们都不自动处理任务、年度授勋等决策页，因此月份/任务数和停点须看实测，不能把指定帧数称为无限经营或高人口压力测试。

新增列记录设施数、任务Owner/顺序名单数、耐久存档FNV-1a摘要与未来8个随机原值的滚动摘要。摘要只作快速同输入核对；当前工具只在capture明确返回ineligible时记耐久摘要0，其它捕获失败抛出错误码和消息，不伪造保存资格，也不是完整瞬态状态等价证明。`probe_snapshot_ms`为每个采样点20次 `WorldFrame` 分配及 `make_shared<const WorldState>(move(candidate))` 的平均毫秒；准备完整candidate和销毁均在计时外，不含worker锁、通知、读线程或绘制，所以只能定位快照构造成本。

插桩stderr现在逐250帧记录累计指标，可相减取得区间值。`domain`覆盖 `prepare_owned_world_runtime_domain`（不等于整个prepare；包括其内部Owner复制及消费者）；`calendar`覆盖日历入口；`adapter_construct`只覆盖prepare中实际adapter构造，外部20次adapter探针仍包含销毁。它们与其它指标嵌套，不能求和。性能采样前检查其它游戏、构建和研究长测进程；若有并发，仅记录侦察结果，不把前后墙钟差异解释为受控收益。保留命令、产品HEAD、源/EXE/DLL哈希、运行前后CPU累计及原始CSV/stderr。

898b653接入后，诊断生成器在实际调用级calendar consumer计时，并保留原入口委托；prepare中的只读adapter只在首次调用构造，包装回调也只发生在该诊断对象初始化中，不修改生产共享adapter。`adapter_construct`因此只记录首次构造，独立adapter探针仍构造新值，两者含义不同。以下2b479f6历史测量不据此改写或宣称新版本提速。

新模板/入口与尚未重建的旧DLL可做同输入逐帧对照（必须在重建旧DLL之前执行）：

```powershell
& $compiler -std=c++17 -O3 -DNDEBUG -DARK_PROFILE_INSTRUMENTED -DARK_PROFILE_COMPARE -Wall -Wextra -Werror -Ibuild/validation/world-performance/instrumented -Iinclude scripts/simulation/profile_world.cpp -Lbuild/shared-libraries/lib -lark_world_runtime -lark_world_rules -lark_world_save -o build/bin/ark_profile_compare.exe
& build/bin/ark_profile_compare.exe 2000
```

比较每帧日期、世界/框架计数、现金、页面全部字段、页计数/phase、声音、未来8个随机原值；正常可保存时逐字节比较全部耐久存档，检查点同样比较。暂不可保存的帧不伪造存档资格，此对照不是全部瞬态字段形式化等价证明。对照版wall含双份计算与序列化，不能用于优化加速比。原规则/运行时的独立拒绝、回滚、顺序及三个月基线仍须运行。

## 2026-10-06 初始测量

当前机器、LLVM-MinGW 20250305、Release共享库，一次2000帧普通采样32.664s，终点1980round、cash5480、random6207、4个checkpoint。首250帧prepare2.860s，1251–1500帧5.122s；演员从0增长为3人/2怪，最终3人/0怪/2退休。Owner复制探针0.089–0.101ms；最终commit-copy约0.10ms/帧，render不到0.001ms/帧；adapter构造/销毁约1.3ms/帧。仅移动最外层candidate无法解决主要开销。

旧模板插桩运行34.931s；旧模板7个显式Owner复制点实际执行59,105次，累计5.969s，约29.6次/框架（不含其它隐式复制）。scene_other调用13,848次，其入口也曾无条件复制Owner；routes读10,703次/0.393s、写11,275次/0.408s。原与插桩逐250帧的世界、随机、日期、资金及名单/账本/checkpoint摘要一致。优化把无条件复制挪进各自需要它的分支，所以当前模板静态插桩点为9个，动态执行次数反而减少；不以源码点数代替复制量。

最小适配保持所有输入、公开布局、返回候选、回滚、审计及随机顺序；不修改adapter目录所有权，不改变Session::update返回完整独立candidate的契约。源身份和导入哈希由现有 `--record-patch` 登记机制保留，不引入生产构建补丁器：

| 产品文件 | 登记理由 |
| --- | --- |
| `include/ark/simulation/world/rules/world_runtime.hpp` | 只在需要可变候选的分支复制Owner；纯转发仍传const输入，最后使用的局部Owner/审计改移动，原消费者与拒绝顺序不变 |
| `include/ark/simulation/ai/rules/world_actor_schedule.hpp` | 回调消费后的私有Owner及最后的schedule结果移动，事件/表现/遭遇重投影顺序和返回审计不变 |
| `include/ark/simulation/world/rules/world_scene.hpp` | 完成scratch写回后移动即将销毁的场景audit，保留全部返回字段 |
| `include/ark/simulation/ai/rules/world_schedule.hpp` | 完成scratch写回后移动即将销毁的schedule audit，保留全部返回字段 |
| `src/simulation/world/startup_world_runtime.cpp` | 暂停页、页面候选与最终owned结果在最后使用时移动；公开Session返回候选仍保留独立副本 |
| `tests/simulation/startup_world_runtime_test.cpp` | 新增成功Session返回完整candidate、相同未来随机与独立可变性检查；原失败回滚等断言不删改 |

原始日志位于 `build/validation/world-performance/`。阶段最终性能、等价性和标准验收结果需分别登记；未运行的CI不视为通过。

优化后的独立模板/入口插桩运行30.949s，相比同样插桩的34.931s减少11.4%；显式Owner复制39,751次，较59,105次减少32.7%（19,354次）。这是单次短轨迹、旧DLL消费者不变的受控测量，不承诺其它场景相同比例。

优化入口对旧DLL独立oracle的2000帧逐帧对照已通过（exit0），终点、4个checkpoint及所有实际可保存帧的耐久字节一致；对照wall65.890s包含双份计算，不列入加速比。原始 `compare-2000.csv/.stderr` 保留；这不替代阶段统一标准测试与Release三个月基线。

共用Release DLL重建后，用原普通诊断EXE再次采样2000帧，期间不并发编译或测试：30.071568s（exit0），相较32.663625s减少7.94%；prepare累计32.117240→29.499849s，减少8.15%。逐250帧的8个采样点，frame/round、人物/怪物/退休、账本条数、catalog、checkpoint、random/cash、年/月/units与确认数共14列逐值一致。原始 `optimized-2000.csv/.stderr` 和 `comparison.json` 保留。这是相同短轨迹的一次前后对照；共享目录构造和其余投影/复制成本仍存在，不声称获得稳态FPS或解决全部增长开销。

## 2026-10-06 增长轨迹侦察

产品HEAD `2acf7e1`、冻结研究 `2b479f6`、LLVM-MinGW 20250305 x64 UCRT、公共Release DLL，使用上述 `12000 bakery` 策略。没有导入研究在途性能代码，也没有修改生产代码、完整candidate返回、快照所有权、规则或47ms开始门槛。原始输入哈希、CPU快照、CSV/stderr、汇总在 `build/validation/late-world-performance/`；本次不登记product_patch。

两次都与研究长测PID15556并发；该进程在插桩/普通采样区间分别实际消耗296.781/272.359 CPU秒。普通采样约10250帧后还叠加了产品构建/CTest。因此以下是热点侦察及状态核对，**不是受控前后性能对照**，不能用302.938s与277.819s推导优化收益。

同一新局自然从0人物/8设施/0任务推进到3人物/10设施/3任务：2000帧真实建面包店，之后原任务消费者生成场所；采样峰值3人/4怪。终点11926轮、原日期0/10/8262、cash17675、random72123、92笔账本、29个原时点检查点。没有接受任务或执行村办/晋级策略，因此不代表高人口、多年经营或活动任务压力。48个采样点的19项计数/名单摘要、资金、日期、未来随机摘要全部相同；其中43点成功捕获的耐久摘要也全部相同。历史测量版本在另外5点只记了0而未留存capture错误类别，这5点不认证为ineligible或耐久等价；采样间隙也不由摘要对照认证。

| 插桩区间（框架） | prepare平均ms/框架 | Owner复制探针ms | 最终commit-copy平均ms/框架 | 快照构造探针ms |
| --- | ---: | ---: | ---: | ---: |
| 1–2000 | 17.152 | 0.106 | 0.109 | 0.001 |
| 5001–7000 | 24.343 | 0.113 | 0.122 | 0.001 |
| 10001–12000 | 32.343 | 0.118 | 0.138 | 0.001 |

插桩prepare共299.213s；domain153159次/98.511s、显式模板Owner复制301198次/38.417s、实际adapter构造11926次/16.831s。domain与复制嵌套，不能相加；其它未插桩的规则、投影、审计复制和销毁仍在prepare内。前/后2000帧显式Owner复制39751/58349次，说明调用次数增长也是成本来源，而非只有单次Owner变大。普通采样prepare共274.104s、最终commit-copy1.498s、render cache0.012s；这些是离线计算和探针，不能解释为实际绘制耗时、稳定FPS或47ms尾延迟保证。

本批不追加生产优化：快照已移动完整候选，外层复制和快照构造不是主要热点；静态目录改共享引用涉及adapter所有权契约，尚不以这份并发小人口样本推动该改造。优先保留既有597e642适配。后续若继续优化，应先在更丰富的已证经营轨迹上细分domain/审计和规则内投影，并在独立CPU窗口做相同输入的优化前后比较，保持当前回滚、检查点、完整候选和未来随机oracle。

另外以 `ARK_PROFILE_COMPARE` 运行2500帧bakery策略：每帧由插桩模板入口与原DLL从同一输入计算，核对日期/计数/资金、全部页面字段、声音、未来8个随机原值、capture错误/结果及实际成功捕获的耐久字节，逐一比较6个检查点；完成至2479轮/cash6280/random6602，无差异并打印正常结束标记。这个范围不等于12000帧逐帧认证，也不证明所有瞬态字段等价。收口修正了hash探针对非ineligible捕获失败的诊断；最小独立夹具复验真实新局可保存、活动场景ineligible及非法日历invalid_world三条分支通过，不重新跑12k或添加标准CTest。性能工具两种构建与比较构建均使用 `-O3 -DNDEBUG -Wall -Wextra -Werror`；标准游戏验收由本阶段统一记录，本工具结果不替代原拒绝/回滚或长期回归。
