# 完整世界构建数据

两份维护编译器读取产品`assets/simulation`，验证固定原表、APK身份、表尺寸和交叉引用，生成标准C++只读目录与脚本。
Node只用于构建。脚本不会提取APK，也不会在构建时更新research或产品输入。

显式升级使用上一级`import_world_research.mjs`：先`--snapshot`冻结维护源码/测试/数据，再`--import`导入产品；
源与产品哈希记录于`assets/simulation/SOURCES.json`。`verify_sources.mjs`无需research即可校验全部副本。
命名空间、include路径与测试数据路径是机械适配，规则算法与回归断言保持维护版本。

e8f66d9接入新增建设/通知/只读头像及原测试，原型共享夹具保留在`tests/simulation/support`。
头像CPU测试的SEB/TSV头与命名空间映射到现有`ark::assets`等价API，保留全部图片裁剪断言；
该测试只在桌面配置链接raylib，运行时及其他规则测试保持标准C++依赖。
上游已完整交付勋章共享Owner桥，移除此前runtime.cpp的产品补丁记录，以原维护实现为准。

显式离线性能入口见[性能采样](profile_world.md)；采样副本只写忽略的build目录，正常构建不使用插桩头。计算优化通过既有product_patch登记保留研究/首次导入身份，不改变原消费者、随机和失败回滚。

524415a维护存取另用`compile_persistence_identity.mjs`生成真实15文件数据身份；逻辑文件名保持研究协议，只映射到产品assets读取。`generate_owner_codec.mjs --check`用Clang AST核对92结构/11枚举、源规范JSON及机械namespace翻译inc。运行游戏不依赖这些工具。

精确文件回放在现有continuous套件，`replay_file_test.mjs`短三进程场景进入标准CTest。较长认证档先核对数据/布局/语义和前缀资格，按风险复用相关尾段；完整外层轮是唯一续跑边界，日历轮内审计不能重跑完整框架。命令及证据见[B1](../../docs/stages/B1-playable-prototype.md#research-524415a-design)。

ee687bc增量迁入只更新该runner，354项清单以`source_revision`与`incremental_imports`追溯；原完整`snapshot_sha256`仍指524415a基础冻结，不能将其解释为本次新全量快照。未改规则/数据/codec，玩家数据集身份及维护数据/布局身份保持。

## 场景与既有前缀接续

默认仍是`natural_progression`；`--scenario natural_expansion`使用自己的上限及完整终点oracle。所有临时、周期、保留快照和输入前缀/证书必须位于产品`build`，并拒绝junction/symlink逃逸。程序与参数路径支持空格，不使用shell拼接。不要指向research/work或玩家存档目录。

```powershell
# 首次小段三路认证，留下主快照及相邻 .awr.json 证书。
node tests/simulation/replay_file_test.mjs --exe build/bin/ark_simulation_startup_world_continuous_test-desktop-release.exe --work-dir build/replay-work --scenario natural_expansion --save-at 420 --stop-at 440 --snapshot-file build/replay-prefixes/expansion420.awr
# 从该前缀继续，仅比较新捕获后的441..460；不重复新局前缀。
node tests/simulation/replay_file_test.mjs --exe build/bin/ark_simulation_startup_world_continuous_test-desktop-release.exe --work-dir build/replay-work --scenario natural_expansion --load-prefix build/replay-prefixes/expansion420.awr --save-at 440 --stop-at 460
```

已认证输入默认读取同名`.json`并核快照hash、场景、seed/speed、实际next_frame及旧证书范围。裸周期档必须显式加`--prefix-status candidate --prefix-next-frame N`，N需匹配文件；它只能获得新捕获后尾段认证，不能升级成已从新局无中断验证。两种输入和证书始终只读，输出独占创建，不覆盖原有效证据。周期输出加`--save-every N --save-directory build/...`，与认证主快照分别登记。

标准的`simulation.replay_runner_contract`在既有continuous二进制上验证扩张/两种接续及路径、证书、帧身份拒绝；默认晋级三进程测试保持原参数与断言。扩张108700晚期前缀尚未认证，不因工具支持就宣称预算或自然路线已通过。此次产品结果见[B1](../../docs/stages/B1-playable-prototype.md#research-ee687bc-replay)。
