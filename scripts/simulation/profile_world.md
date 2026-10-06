# 世界运行时性能采样

这是显式离线诊断，不属于标准 CTest 或玩家程序。seed1、原240×320视口、speed0、原日历27、真实新局逐框架推进，每帧消费 render cache 和声音；普通非决策页使用与 CLI 相同的显式确认，自动页自行推进。无47ms等待、跳轮、跳消费者或演示初值。2000帧包含首次人物到访、战斗/退休、跨月及4个原时点不可变检查点；不是桌面宽屏商会的同一轨迹。

## 复现

先按项目说明构建公共 Release 库。以下 PowerShell 命令只编译独立诊断 EXE，不重建或替换 DLL：

```powershell
$compiler = 'build/local-tools/llvm-mingw-20250305-ucrt-x86_64/bin/x86_64-w64-mingw32-clang++.exe'
& $compiler -std=c++17 -O3 -DNDEBUG -Wall -Wextra -Werror -Iinclude scripts/simulation/profile_world.cpp -Lbuild/shared-libraries/lib -lark_world_runtime -lark_world_rules -o build/bin/ark_profile_world.exe
& build/bin/ark_profile_world.exe 2000
```

逐250帧记录 prepare、render、最终 commit-copy；这三列为该区间累计毫秒。wall为进程累计采样时间，包含确认、输出和探针。probe-copy为当前同一个真实Owner的20次复制/销毁平均毫秒；probe-adapter为20次实际adapter构造/销毁平均毫秒。结果用于定位开销，不能解释为稳态FPS、桌面调度速率或原APK速度。

模板复制定位需要显式生成插桩副本：

```powershell
node scripts/simulation/profile_world_generate.mjs
& $compiler -std=c++17 -O3 -DNDEBUG -DARK_PROFILE_INSTRUMENTED -Wall -Wextra -Werror -Ibuild/validation/world-performance/instrumented -Iinclude scripts/simulation/profile_world.cpp -Lbuild/shared-libraries/lib -lark_world_runtime -lark_world_rules -o build/bin/ark_profile_instrumented.exe
& build/bin/ark_profile_instrumented.exe 2000
```

生成器只读取产品文件，将入口和5个模板头的诊断副本写入忽略的 `build/validation/world-performance/instrumented`；正常产品/CMake不使用该目录。它计量显式Owner复制构造及adapter回调，不覆盖DLL内部的所有复制；各项可能嵌套，不能直接相加解释总占比。插桩调用、额外时间查询和模板在EXE中的编译会扰动耗时。

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
| `include/ark/simulation/rules/world_runtime.hpp` | 只在需要可变候选的分支复制Owner；纯转发仍传const输入，最后使用的局部Owner/审计改移动，原消费者与拒绝顺序不变 |
| `include/ark/simulation/rules/world_actor_schedule.hpp` | 回调消费后的私有Owner及最后的schedule结果移动，事件/表现/遭遇重投影顺序和返回审计不变 |
| `include/ark/simulation/rules/world_scene.hpp` | 完成scratch写回后移动即将销毁的场景audit，保留全部返回字段 |
| `include/ark/simulation/rules/world_schedule.hpp` | 完成scratch写回后移动即将销毁的schedule audit，保留全部返回字段 |
| `src/simulation/startup_world_runtime.cpp` | 暂停页、页面候选与最终owned结果在最后使用时移动；公开Session返回候选仍保留独立副本 |
| `tests/simulation/startup_world_runtime_test.cpp` | 新增成功Session返回完整candidate、相同未来随机与独立可变性检查；原失败回滚等断言不删改 |

原始日志位于 `build/validation/world-performance/`。阶段最终性能、等价性和标准验收结果需分别登记；未运行的CI不视为通过。

优化后的独立模板/入口插桩运行30.949s，相比同样插桩的34.931s减少11.4%；显式Owner复制39,751次，较59,105次减少32.7%（19,354次）。这是单次短轨迹、旧DLL消费者不变的受控测量，不承诺其它场景相同比例。

优化入口对旧DLL独立oracle的2000帧逐帧对照已通过（exit0），终点、4个checkpoint及所有实际可保存帧的耐久字节一致；对照wall65.890s包含双份计算，不列入加速比。原始 `compare-2000.csv/.stderr` 保留；这不替代阶段统一标准测试与Release三个月基线。

共用Release DLL重建后，用原普通诊断EXE再次采样2000帧，期间不并发编译或测试：30.071568s（exit0），相较32.663625s减少7.94%；prepare累计32.117240→29.499849s，减少8.15%。逐250帧的8个采样点，frame/round、人物/怪物/退休、账本条数、catalog、checkpoint、random/cash、年/月/units与确认数共14列逐值一致。原始 `optimized-2000.csv/.stderr` 和 `comparison.json` 保留。这是相同短轨迹的一次前后对照；共享目录构造和其余投影/复制成本仍存在，不声称获得稳态FPS或解决全部增长开销。
