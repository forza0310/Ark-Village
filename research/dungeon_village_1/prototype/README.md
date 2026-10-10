# 可玩研究原型包

[应用存储](APPLICATION_STORAGE.md)与[标题控制器](TITLE_MENU.md)已接系统2四目录、显式菜单输入和完整文件视图；世界语义4／应用语义7修复建设、移动、撤除后的到达价格缓存，当批Driver v3与字段布局未变，验收见[收费缓存交付](../VERIFICATION.md#邻接价格与语义4应用7修正2026-10-09)。本批37／38目录另行扩展Owner字段，旧布局研究普通档及精确快照均拒绝，见[存取模块](PERSISTENCE.md)。CLI使用显式`--root`，旧三路径接口拒绝且不迁移；自动中断原轮内产生者／raw14及原游戏档兼容仍未实现。

旧`natural-application-menu-v1`世界3／应用6的首月、12月前缀仅保历史，不能加载后暗补price接续或重签证书。新`natural-application-economy-v1`首月正在验证，尚未认证通过。价格在完整邻接刷新中同步所有实例，品质在退出时由当前邻接即时派生，没有同类长期实例缓存；原版依据与条件到达覆盖见[设施使用](../rules/FACILITY_USE.md#arrival-cache)。

[Steam启动局部皮肤](STEAM_STARTUP_SKIN.md)提供标题／选档／手动菜单／询问的只读绘制与触摸注册计划，已过visuals和application验收；它不修改应用文件契约或代替完整可操作标题。原资源、真实测宽、未知字体及平台热区边界分别保留。

人物60基础委托位于[steam_human_skin](include/dungeon_village_prototype/steam_human_skin.hpp)，复用现应用库和visuals套件。`steam_human_detail_skin(state,page,down_text_width)`读取已初始化Owner视图，返回奖章→人物／武器→血条和危险提示或倒下气泡的有序计划；100槽位置与16槽步帧独立，HP显示值／目标值及当前共享最大值分开。已挂起父页仍可只读投影，可绘制不等于可交互。数字绑定Steam差异PNG，倒下文字宽度由平台实测；4096像素是气泡维护输出预算，不是原游戏字符串上限。模块无raylib依赖，不新增持久字段，也不推进随机／计数／声音；完整scratch附加效果、字体和原窗口像素仍独立未验，来源见[人物表现合同](../ui/STEAM_HUMAN_PRESENTATION.md)。

信息页面由[startup_world_information](include/dungeon_village_prototype/startup_world_information.hpp)维护。Session／Application提供`open_information_menu`和`input_information_page`；接收已解析输入，raw9上下优先互斥，raw36左右依次执行。五项原序保留，现开放36／37／38，34／35拒绝且状态不变；选子页先压页再退休菜单，主场景快捷入口仍属维护适配。37／38在既有phase／counter之外增加typed目录载荷，分别冻结一组／四组ID及选择／滚动；38采用Steam目录过滤，关闭后由框架统一退休。完整皮肤与OS输入沿[信息合同](../ui/INFORMATION_MENU.md)分别交付，本批短测结果另记。

收支36的[steam_information_skin](include/dungeon_village_prototype/steam_information_skin.hpp)复用Steam窗框／内框，输出完整局部有序图元、字体金额、源端点线与箭头触摸引用；不修改Owner，不生成数字SEB。调用者提供语言分支、VIEW_Y与标题两次真实测宽，并负责原页面坐标及字体后端；[布局例图](../ui/examples/steam-income.png)使用明确替代字体，仅作产品布局参考。raw9菜单皮肤和其余信息子页仍未由此补齐。

[startup_information](include/dungeon_village_prototype/startup_information.hpp)提供37正库存原序／原说明和38四类装备的纯目录查询，保留显式APK／Steam版本选择；维护38页面固定使用Steam过滤／占位语义。37正常关闭在Owner候选中清全部道具NEW，空目录初始化则请求事件15并退休、不清NEW；38依次处理左右、最终换页重置、上下和关闭，不清装备NEW。装备列表图标复用`startup_world_equipment_icon_draws`，与武器身体PNG分开。新增页面map使codec布局身份自动变化，旧精确快照拒绝、不迁移；普通稳定场景存档政策不变。

默认运行已发布的新局建设/首访保护切片；`--world`显式运行完整目录的共同世界AI。
旧7×7自主访问演示只在`--fixture`运行，三者不共享可写世界，也不冒充完整原版复刻。
本包独立于产品主构建，只修改研究代码。

最新[声音操作输出](AUDIO_REQUESTS.md)提供同一Owner队列的typed／旧ID两种一次领取接口；完整播放使用操作＋ID，旧接口不代表完整音频语义。新遭遇BGM2及通知24已接，真实设备与标题剩余声音另验。

设施74第二页现提供[逐来源只读行及五行视窗](../ui/FACILITY_BONUS_ROWS.md)，配套[类别9／属性7图块与效果加号](../ui/FACILITY_BONUS_ICONS.md)。按原序显示实例后缀与固定y值，不从累计分摊；不会因读页再次计算或提交加成。
有界`--world --inspect-page world-facility-bonuses --frames 8`仅打开真实新局已有来源详情，不构造示例数值；窗口来源行／标题已接，第一页面完整效果皮肤及原EXE动态另验。

2026-10-08：[普通道具图标](../ui/ITEM_ICON_RENDER.md)按原item列5投影，并接礼物64／设施75／商会84；
[六属性头标](../ui/ATTRIBUTE_GAIN_RENDER.md)通过字体度量回调生成只读计划，与举物按原cd索引归并。接口不推进计数、累计属性或随机。
18项受影响Release测试和8帧共同世界窗口通过；窗口未捕获cd13或道具目录，原身体bl偏移／精确原布局／Steam消费者仍未认证，详见[需求回应](../verification/PRODUCT_REQUESTS.md)。
证据与缺口集中在[新局报告](../rules/STARTUP.md)，不以截图或演示初值补齐未知原版规则。

`2b479f6`普通道具／商会／道路与编辑消费者已验，Release标准、五项长测与三个有界研究窗口通过；聚合Release入口123项标准及迁移窗口也通过。
随后第一次自然地图扩张／新区经营、道路保留特殊实例引用组合及同轨迹性能优化已独立验收：
单Release标准123项、4条专项长测及两个既有有界窗口通过；第二次扩张／封顶仍只属组合验收。
`9897d64`对应此前村办、95和自然晋级基线，
本批新增路径不能沿用其424次CTest或窗口结果宣称通过；独立检查进度见[验证入口](../VERIFICATION.md)。

## 职责与依赖

| 文件/目标 | 职责 |
| --- | --- |
| [startup.hpp](include/dungeon_village_prototype/startup.hpp)、[startup.cpp](src/startup.cpp) | 标准C++17新局聚合；消费加载后身份与建设地表，免费初始化、建设事务、施工、首访和模态调度 |
| [startup_map.hpp](include/dungeon_village_prototype/startup_map.hpp)、[startup_map.cpp](src/startup_map.cpp) | 576格/8实例静态重建、入口编号、逻辑/显示分离、边界/道路刷新与非零寻路身份投影；不是可变运行地图 |
| [dump_startup_map.cpp](src/dump_startup_map.cpp) | 导出[发布TSV](../data/startup/README.md#加载后快照)，不访问APK；全字段对账是独立CTest |
| [数据编译脚本](scripts/README.md) | 构建期用Node内置JSON解析生成只读C++；完整地图字节哈希、原表和派生契约交叉验证，运行不依赖Node |
| [startup_view.hpp](include/dungeon_village_prototype/startup_view.hpp)、[startup_view.cpp](src/startup_view.cpp) | raylib新局画面、中文替代字体、源SEB/图片绑定、目录/放置/提示与有界页面检查；核心不接收屏幕坐标 |
| [village.hpp](include/dungeon_village_prototype/village.hpp)、[village.cpp](src/village.cpp) | 保留旧夹具的建设、多格布局、自主访问、到达收入和周期事务，不混入默认新局 |
| [catalog.cpp](src/catalog.cpp) | 旧夹具设施表投影，仍只接28/29/36 |
| [asset_manifest.hpp](include/dungeon_village_prototype/asset_manifest.hpp)、[asset_manifest.cpp](src/asset_manifest.cpp) | 旧夹具的七键替换素材契约 |
| [main.cpp](src/main.cpp) | CLI入口，显式分流默认保护切片、共同世界和旧夹具 |
| [startup_world_projection.hpp](include/dungeon_village_prototype/startup_world_projection.hpp)、[实现](src/startup_world_projection.cpp) | 真实新局一次性接管、完整目录和稳定维护身份，不保存第二个可写StartupState |
| [startup_world_runtime.hpp](include/dungeon_village_prototype/startup_world_runtime.hpp)、[实现](src/startup_world_runtime.cpp) | 唯一世界/台账/随机/日历所有者；队首时读取最新事实，整帧事务和不可变内存检查点 |
| [场景](src/startup_world_runtime_scene.cpp)、[日历](src/startup_world_runtime_calendar.cpp)、[到访](src/startup_world_runtime_arrival.cpp) | 原入口资格/全局表现、跨月各域和真实人物创建/89/Q刷新时序 |
| [独立焦点](src/startup_world_runtime_focus.cpp) | 主场景2的W单独所有权、按键运动/共享成长/FIFO/物理与留存，不混入自主人物名单；普通入口守卫不放宽 |
| [任务](src/startup_world_runtime_tasks.cpp)、[非人物](src/startup_world_runtime_nonactors.cpp)、[页面](src/startup_world_runtime_pages.cpp) | 真实任务工厂/探索地图/奖励、投射/拾物/遭遇消费，以及成果/赠礼页的独立更新和输入 |
| [玩家任务页](src/startup_world_runtime_task_pages.cpp) | 页22原名单、23付费接受、24真实征集、25队伍、27追加、28正式出发；临时领域投影同步消费真实脚本/页栈，不另存任务世界 |
| [期限与返回](src/startup_world_runtime_deadline.cpp) | 页33续费/中止演出、栈外原关闭页引用及真正主场景返回；当前报价/现金/任务清理/地图恢复整轮提交 |
| [只读世界表现](include/dungeon_village_prototype/startup_world_visuals.hpp)、[实现](src/startup_world_visuals.cpp) | 当前职业/性别头像、旅馆前四占用引用、169/170计时/HP切换；cd15／21／22举物及设施队首kind1–6原帧/图层；不推进计数、装配、邻接或随机 |
| [显式表现请求](include/dungeon_village_prototype/startup_world_presentation.hpp)、[实现](src/startup_world_presentation.cpp) | Session按物理页栈准入及原序，事务提交任务栏共同抖动／缺绑定恢复和显式栈顶66声音；即时返回冻结计划，与只读像素绘制分开 |
| [共同世界建设](include/dungeon_village_prototype/startup_world_building.hpp)、[实现](src/startup_world_building.cpp) | 当前普通目录/报价/全占地准入与实例初始化、raw74稳定绑定、募集24→入住80→住宅25/96、共享等级81；刷新/账本/脚本整轮回滚，不复用旧建设世界 |
| [共同世界编辑](include/dungeon_village_prototype/startup_world_editing.hpp)、[实现](src/startup_world_editing.cpp) | 道路起终点／撤除／移动选择与落点、原模式返回、全占地刷新及旧实例退休；住宅解除绑定及H重建资格仍归同一Owner，重建仍付原800G |
| [地图扩张](include/dungeon_village_prototype/startup_world_expansion.hpp)、[实现](src/startup_world_expansion.cpp) | 村办类型3的边界级别、原序设施替换／退休、地面／双入口重建与旧ax人物重置；地图仍为576格，逐次刷新和最后c/d分开，失败不提交部分Owner |
| [设施道具](include/dungeon_village_prototype/startup_world_facility_items.hpp)、[实现](src/startup_world_facility_items.cpp) | 74→75库存目录、75消费与实例门槛／原程序、76首次共享改良／49换页、77结果确认；各同定义实例独立重算价格，载荷随页退休 |
| [商会](include/dungeon_village_prototype/startup_world_commerce.hpp)、[实现](src/startup_world_commerce.cpp) | 83入口、84真实买卖／86反馈、85点数购买与93确认领取；金币、村子点数、商会A库存、人物可用z库存和建筑H次数分开；85定义预览只读共享定义 |
| [人物管理](include/dungeon_village_prototype/startup_world_human.hpp)、[实现](src/startup_world_human.cpp) | 当前四页／装备73、61/62预览与拒绝、63中点及最终提交、64/65装备延迟赠礼、64普通道具及66/68/69结果／回复、70大师奖励；只维护同一Owner的共享进度和页附属记录 |
| [税收](include/dungeon_village_prototype/startup_world_tax.hpp)、[实现](src/startup_world_tax.cpp) | 90居民引用／实时税额和98自动其它收入、清全人物F/G；确认与入账分开，无额外随机 |
| [村办活动](include/dungeon_village_prototype/startup_world_village_activity.hpp)、[实现](src/startup_world_village_activity.cpp) | 51—54初始化／只读投影／动作／更新，类型0／1／2及人物结果的有放回抽签；类型3在53提交地图扩张，不插54或额外抽签。同一Owner提交资源、人物、脚本和页面，严格拒绝失配载荷并随真实页退休辅助记录 |
| dungeon_village_startup_model | 标准C++17静态证据与新局规则，无raylib/字体/JSON运行依赖 |
| dungeon_village_startup_world | 标准C++17共同世界与完整原表；不依赖raylib，不在运行时读取APK/研究临时文件 |
| dungeon_village_prototype_model | 旧夹具聚合，复用[领域示例](../example/README.md)与[工具](../tools/README.md)表解析 |
| [测试目录](tests/) | 新局/共同世界/任务/页面/连续运行、发布表/错误输入及旧夹具；当前计数以[本轮验证](../VERIFICATION.md)为准 |

[CMake](CMakeLists.txt)把所需原始图集/SEB及人物、怪物、common目录复制到程序旁。
武器举物同时复制weapon目录，按定义图片覆盖专用SEB，不以商品图标代替。
两类新增只读绘制的来源、边界及本轮验收见[表现合同](../ui/EQUIPMENT_FACILITY_RENDER.md)和[验证入口](../VERIFICATION.md)。
运行只读该素材目录；设施新局数据编译进程序，不访问APK、反编译结果或发布JSON。
源路径/哈希见[素材清单](../assets/MANIFEST.tsv)，不重复发布或修改原图。

## 构建与运行

构建需要C++17、CMake、Node、动态raylib和pkg-config。日常从仓库根目录使用[研究聚合入口](../CMakeLists.txt)：

```sh
cmake -S research/dungeon_village_1 -B research/dungeon_village_1/work/release -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
cmake --build research/dungeon_village_1/work/release --parallel 2
ctest --test-dir research/dungeon_village_1/work/release -LE long-world --output-on-failure
research/dungeon_village_1/work/release/bin/dungeon_village_prototype --paused
```

同一依赖图只构建一份reference领域库；prototype包含的87项领域测试不重复注册，
加上35项原型及5项工具测试，共127项唯一标准测试。长测按本批风险显式启用，以`-L long-world`单独执行。
三包独立CMake入口保留用于依赖边界检查，但日常只保留`work/release`，不常驻重复缓存。
Debug仅在定位问题时将上述构建目录改为`work/debug`、配置改为`-DCMAKE_BUILD_TYPE=Debug`，
可只构建所需target；完成后收齐进程并清理，不再每批要求三包各跑Debug与Release。

本机工具链路径见[环境基线](../verification/BASELINE.md)。暂停开始用`--paused`，
无窗口新局自检为`--check`，有界运行用`--frames N`，`--screenshot`仅用于有界截图。
默认字体是本机`/System/Library/Fonts/Supplemental/Arial Unicode.ttf`，未复制或分发；
其他环境用`--font /绝对路径/中文字体.ttf`指定字体，缺少字形明确失败，不悄悄画方框。

[共同构建策略](../cmake/ResearchLibraries.cmake)让本地Debug与Release都使用共享项目库和动态raylib。
同一顶层构建的程序及DLL放`bin/`、导入库放`lib/`；三包共用该位置。
不同构建树不共写同一个DLL目录；按需Debug使用自己构建的匹配库，不混用Release DLL。
素材复制使用真实可执行目录，故仍在`bin/assets`及`bin/data`，CTest按target路径运行，不依赖当前工作目录找DLL。
Windows运行库仅从目标LLVM-MinGW导入库定位并验证PE架构；不会取宿主x64编译器旁的同名DLL来运行i686程序。
raylib只从PkgConfig的`RAYLIB_PREFIX`寻找DLL，静态`.a`或缺DLL会明确失败，不回退静态链接。

本机i686动态raylib安装在`research/dungeon_village_1/work/local-tools/raylib-shared`。
PowerShell配置聚合入口前显式设置其pkg-config路径。若在已有静态缓存内切换，先结束对应进程，
再清除旧链接与pkg-config缓存值；不要把旧包级构建树直接改为不同的CMake源目录：

```powershell
$env:PKG_CONFIG_PATH = (Resolve-Path research/dungeon_village_1/work/local-tools/raylib-shared/lib/pkgconfig).Path
cmake -S research/dungeon_village_1 -B research/dungeon_village_1/work/release -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DCMAKE_EXE_LINKER_FLAGS= '-URAYLIB_*' '-Upkgcfg_lib_RAYLIB_*' -U__pkg_config_checked_RAYLIB
```

Release日常目录采用同一动态策略；只有独立向玩家分发的Release制品才在另一个构建目录显式设置
`BUILD_SHARED_LIBS=OFF`及匹配的静态raylib／运行库选项，不因配置名是Release自动静态。
聚合入口123项标准及迁移窗口通过，旧六目录已清理；
实际测试与缓存体积见[验证入口](../VERIFICATION.md)，旧回归不能替代当前验收。

## 共同世界入口

```sh
research/dungeon_village_1/work/release/bin/dungeon_village_prototype --world
research/dungeon_village_1/work/release/bin/dungeon_village_prototype --world --check
research/dungeon_village_1/work/release/bin/dungeon_village_prototype --world --inspect-page world-month --frames 12 --screenshot research/dungeon_village_1/work/world-month.png
```

`--world`从真实空人物新局开始：介绍、自动到访、89提示/镜头、自主设施选择/移动/使用/退出，
之后继续执行共同世界、日历、后续到访、遭遇、任务生成、费用和月报。
Enter或确认点击逐段处理当前页；暂停同时冻结世界和栈顶页；倍速只改变框架内1/2逻辑轮数，
不把移动速度乘二。右键拖动是表现视图偏移，不能直接控制人物。
退出不写正常存档；在原自动保存调用时点仅保存完整不可变Owner内存检查点，这是明确研究策略。

任务自然出现后，点击底部“任务”或按T打开列表；上下键/点击选择，Enter确认，Esc取消。
接受页按原表扣征集费，征集页自主计数，按住Enter可按原held规则加速；队伍页选队员确认出发，
末行“追加队员”进入按实际职业等级报价的付费候选页。出发确认先播放计数阶段，到门槛才安装任务；
此后人物自行寻路、进入设施/战斗，玩家不控制移动。原型列表为简化240×320表现适配，不宣称原UI皮肤一致。
活动任务按T进入raw26当前队伍，可追加raw27并按原报价聘请，不能再次出发；队员详情raw60接当前四页、首次111、装备礼物和转职。
按X打开raw4主动中止菜单：真实raw1是非页保存答案，恢复父页后才执行中止/地图恢复、
80/首次162、完成量请求-10及通知26。期限页33仍走独立费用/返回链，不把主动中止伪装成期限收费。

主场景建设按钮/B进入当前目录：三分类按实际开放状态包含道路、撤除和已解锁移动；住宅重建需有真实H建设资格。
普通设施、募集24的报价、扣款与全占地来自同一Owner。
确认放置可连续建设，Esc退出不退款；施工仅在回到正常场景后推进。点击现有设施开raw74，普通两页箭头/返回已接。
募集的入住按钮进入raw80，以人物原费用替换为住宅25并绑定同一定义；建成后奖励/96/67由真实消费者提交。
已绑定住宅点击转到原居民raw60，不误开普通设施74或创建人物；正常场景也可直接点击可见冒险者查看详情，不控制移动。
有升级提示时先开81，第一次初始化真正升共享等级；40/55分段确认最终才清提示。
道路先定起点再终点，沿原较长轴选线、等长取纵轴；Esc先退回起点模式，再返回场景。
移动要求真实用户flag32，选中现有设施后定落点，重建维护身份但保留原数字ID／序号及来源快照字段，原移动费300G。
撤除不退款；住宅释放居民绑定并返还H建设资格，后续重建仍支付原800G造价，沿实际目录、原居民状态与施工恢复绑定。
H不是造价减免，不因返还资格重复收取募集入住费，也不伪造一次新入住奖励。
精确规则见[共同世界建筑合同](../rules/STATE_CONSTRUCTION.md#共同世界建筑合同)。

人物详情分概况／六属性／四槽装备／四魔法。职业名单与性别／当前开放／村子点／人物勋章取真实共享定义，
确认62扣点并奖励后进入63；第55计数改职业，满197确认才重算经营及清当前经验L和待发N，保留O。
装备65只回传答案，64实际恢复后重新报价、扣款或减免费份数、奖励及装配；取消不支付。
礼物不按职业拒绝装备，同装备仍奖励并设置重选锁6；当前人物HP与详情最大HP分开。
64第五类已接普通道具：正库存目录、职业适配／品质评价、奖励与消耗、属性结果69及学习／回复分支。
普通道具直接确认使用，区别于装备65回传答案；66的实际恢复沿原计数推进，不通过绘制或重复确认多次回血。
设施74第一详情页进入75，道具先消费，再于76首次初始化改良定义；77的49快进／55关闭不重复奖励，也不提供取消领取。
精确合同见[人物管理](../rules/CHARACTERS.md)与[设施道具](../rules/FACILITY_EFFECTS.md)。
税收90确认只关列表，事件123之后的98自动结算；按原月份3／年份>0，不以自然住宅建成就立即收税。
研究窗口未实现音频，明确领取并丢弃一次性声音请求；其他适配可通过`take_sound_requests()`按原序领取一次。
世界完整检查点仍是研究审计历史，不是文件存档；原介质／分区／不保存的随机和页面见[存档专题](../rules/PERSISTENCE.md)。

脚本奖励95接入原计数40快进／确认领取，覆盖固定脚本实际生成的现金、点数、建筑、职业、配置更替、勋章和活动七类型。
窗口只读展示奖励名称；领取后才回写唯一现金台账及共享进度，不复用脚本奖励通知来多发一次。
原94既有支持范围保持。通用确认与专用动作均拒绝框架暂停中的输入；合同见[脚本奖励页](../rules/ACCOUNTING.md#script-rewards)。

主场景“村办”按钮／V打开51原序目录，上下键或点击选择，确认进入52开展／取消。
51先检查季度次数再检查点数；52开展才扣点并增加定义m／全局F，季度q和人物效果留到53满120确认。
53恰70更新请求声音5，提前确认不快进；类型0／1随后进入54只读结果，类型2只请求全局人气。
类型3地图扩张已接本批维护实现，满120确认后更新边界与地图，不插人物结果54；当前验收状态见验证入口。
类型5／6已接魔法壶级别及开放事件并验条件组合；类型4仍明确拒绝，不显示虚假成功。
所有修改由唯一Owner在私有候选内组合，缺绑定／名单／计数、越界选择、错误父页或晚期随机／脚本失败
拒绝整轮，不留下部分扣款、随机或实体修改；页面附属记录随真实页退休。具体来源见[村办合同](../rules/ACCOUNTING.md#下一批来源村办活动与晋级前置)。
这些是简化原型入口与维护合同；`2b479f6`普通工具与编辑路径已通过Release标准、五项长测与三个有界窗口，聚合Release入口123项标准及迁移窗口也已通过。本批扩张独立验收，结果见[当前验证](../VERIFICATION.md)。

`--world --inspect-page task-team --frames 12 --screenshot 路径`从真实新局自然等候任务，
提供明确的测试玩家接受输入，停在真实队伍页作有界窗口检查，不注入任务、人物或资金。

显示使用原人物职业/性别、怪物体型/变体索引及SEB，逐格消费建筑分片/道路补块/边界/入口。
旅馆两阶段头像按原walk01帧0裁剪；通知底栏读取唯一队列的前两项，不由绘制FPS推进。
通知当前皮肤为研究简化显示，恢复特效未接；精确合同及独立边界见[页面映射](../ui/PAGES.md)。
月报读取唯一Owner已提交的快照，不重复扣维护费；页面30/31/32关闭不再次发奖励。
成果后的事件消息页11保留40计数快进/关闭门槛；首次战斗成功延迟出现的商会83可买入、出售或购买设施。
主场景S／商会按钮仍受真实开放flag16限制。84逐件交易、86反馈自动返回；85扣村子点数，93满40确认才增加H建设次数并开放定义，
领取不是立即把设施放进地图。85设施定义预览已通过本批Release合同验收，只读共享定义，不套用实例邻接、月度统计或设施道具操作。
人气奖励终点页97等待实际页面更新自动清R/返回，
不接受玩家确认，不重复奖励。页面只是当前原脚本链，不能直接清整栈恢复世界。
人物解锁页59首轮音效5，满70次页面更新后确认才提高到访优先值并返回；不快进或直接创建人物。
`world-active`/`world-month`检查先在同一真实世界逐轮确认页面，到达三人物/月报时再开画面；
这是明确输入策略，不改世界数值，但也不是人工操作或固定APK动态捕获。
`visitor`是旧首访静态接管检查，不混入自然长跑结果。

默认seed1仅为明确可重放研究输入，固定APK的实际默认seed未观测。

### 可持续世界验收入口

[连续测试](tests/startup_world_continuous_test.cpp)接受`月份数 种子 速度`，月份1..36、速度0/1。
默认仍是seed1两个月；框架更新后检查日期、随机和真实页栈，不注入人物、任务、资金或等待状态。
raw16定时等待、raw56/57镜头页及raw97人气奖励终点自动推进，不以确认删除；
raw49条件页不执行晋级；raw89保留40快进门槛。
年度raw87在明确测试输入中选择“终止→确认”，未用勋章保留，事件22与背景音乐按原序消费。
这些输入只用于自然轨迹验收，不代表玩家的选择。窗口已接简化年度名单/授予与终止是非、88/67、
任务管理/住宅/设施升级显示。完整原皮肤与OS输入仍未认证；正常文件已支持稳定主场景的命令行加载和退出保存，
原两栏菜单在独立StartupApplication控制器接入，图形原型未自动接完整菜单；日历自动文件保存仍缺，范围见[存取模块](PERSISTENCE.md)和[应用存储](APPLICATION_STORAGE.md)。
raw48已接真正晋级/条件说明/返回，raw50三段计数庆典不以早确认跳过；raw49仍只查看条件。

```sh
research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests 12 1 0
research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests 6 20261005 1
```

配置时加`-DDUNGEON_VILLAGE_LONG_WORLD_TESTS=ON`登记六个可重复长期CTest：`world_year`、
`world_speed_seed`、`natural_housing`、`natural_progression`、`natural_tools`、`natural_expansion`（名称均带`dungeon_village_prototype.`前缀），
以`ctest --test-dir <构建目录> -L long-world --output-on-failure`执行；原五项3600秒、扩张10800秒仅是墙钟保护，不参与原规则。
当前跨年、多种子、插桩与构建实际结果见[验证](../VERIFICATION.md)，不从单条路径推导全部状态可达或无限期认证。

无窗口长测直接循环调用`StartupWorldRuntimeSession::update()`，不使用sleep、FPS或墙钟节拍，已经按CPU可承受速度运行。
`speed=1`仅表示原版主场景在框架入口保存两轮逻辑；自动玩家在每次框架更新后观察并下达命令，
因此改成1可能改变页面、命令和随机消费的交错，不能代替`seed=1/speed=0`的黄金轨迹验收。
非人物商店消费者使用窄投影，与完整人物routes共用任务旗标引用遍历和商店notice投影，不再构造随后丢弃的完整人物路由。缺human flag或RescueActorContext仍在原时点拒绝；原序notice、空覆盖及缺details保留不变。历史条件微基准中位数由57.3482ms降至25.676ms，仅代表单次投影，不能当整局加速倍数。

增大CTest超时不会提速。保持同轨迹的性能工作应先采样Owner候选复制、投影、分配与验证成本，
再以相同结果、操作时点、随机抽数及回滚断言验收优化；目前没有额外的等价加速参数。

用户已将按改动缩小长测范围、中后期采样与可校验的中途回放快照列为必要后续工作，
同时要求正常存读档持续可用。原APK不保存随机游标／页面栈，测试精确恢复需额外状态与明确事务边界；
无损保留和持续回归要求见[存档约束](../rules/PERSISTENCE.md#必要后续工作持续可用的存读档与测试快照)。
正常维护文件和晋级跨进程回放已经实现并验收，命令及范围见[PERSISTENCE](PERSISTENCE.md)；扩张晚期认证及其它控制器仍按缺口登记。

Windows混合核心机器可单独配置测试进程的CPU亲和性，保持正常优先级，不改变游戏步长。
本机拓扑确认逻辑核心0—11为6个带超线程的性能核、12—19为能效核；本机性能核掩码为4095，
仅在已核对的本机诊断／验收进程使用。它不是跨机器默认值，也不是游戏内倍速；测量时应记录亲和性与宿主负载。
本批同核3000步对照的CPU均值减少22.4%，该数据与随后长测的核心调度调整分别记录在[验证](../VERIFICATION.md)。

同套件保留显式`natural_housing [seed [speed]]`模式：原募集建设、合法短剑赠礼提高满足度、74/80入住、
共同施工、年度真正授予、90/98及收税后继续一个月；不注入人物／现金／满足度／日期或奖励。
长测选项同时登记该模式，每月输出对象、页栈、输出队列、现金记录和检查点规模；合法审计增长与泄漏分别登记。
每轮核对人物／税收页载荷属于真实留存页面、商店记录属于真实留存设施；募集退休时连同商店索引清理。
页面载荷、音效输出、退役人物／遭遇和特效记录单独记录峰值；账本、任务历史和内存检查点不冒充原文件存档。
赠礼只走60→64→65→父64，读原报价、库存、C奖励和锁；保留入住费，不手工改C。它是明确玩家策略，不代表无操作也会同年入住。
第一星还要求人气300、最高月收入5000、两次村办活动和指定设施35；活动操作是自然晋级的依赖，不能靠自动等待代替。

显式`natural_progression [seed [speed]]`模式，默认`1 0`：从真实空人物新局建设35面包房，
实际开展可支付活动和任务，点击自然设施升级提示，经48申请第一星；再完成新解锁16绘画展览，
继续到后续月份并检查设施收入增加。策略只调用真实Session玩家命令，不注入资金、点数、人气、F、日期或rank，
任务结束后留一个完整自然月正常营业，人气达到300后不再接新任务；晋级后为活动16保留其原价点数。
这是显式验收玩家策略，不是原版自动行为；最高月收入不包含95的其它收入现金奖励。
`2b479f6`在原晋级／活动16／后续收入完成断言之后追加编辑尾段：真实flag32已解锁才铺设并撤除单格道路，
移动并撤除一个正常kind2设施，检查原数字身份保留与新旧维护引用退休，再经营到下一月份并检查新增收入。
不重复新建第二条从新局到晋级的长测，不改变住宅模式和原自然任务黄金轨迹；该批Release长测已通过，原frame36385前缀保持。
编辑消耗道路10G及移动300G，不抽随机，旧实例6→新实例15→撤除；终点frame38282、23388G／322697抽，共154284项检查。
180000框架调用与3600秒是验收保护；每轮检查真实页载荷／实体引用及声音消费，输出资源峰值和检查点规模。
现金流水、任务历史和完整内存检查点允许合法增长，有限轨迹不能证明永久有界。

```sh
research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests natural_progression 1 0
research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests natural_tools 1 0
```

显式`natural_expansion [seed [speed]]`复用`natural_progression`函数及全部原晋级／编辑断言，
seed1／speed0先验证既有frame38282、23388G／322697抽的前缀，再继续玩家经营，不改变旧模式轨迹。
尾段通过真实任务和已开放的人气活动达到1000，预留原100村子点和一个季度名额，等待原奖励解锁25后走51→52→53。
完成后在旧边界外的新严格内圈实际铺一格原价道路，保留道路并经营到一个完整自然月结束，检查新增设施收入、
地图仍576格、页面／实例引用退休和声音输出消费。只执行Session命令，不注入人气、点数、日期、人物或解锁旗标。
新模式采用240000框架调用的有限保护，旧模式180000不变；保护不是游戏时限。
本批优化后实际通过451725项检查，终点frame111808、138463G／1047804抽；109条里程碑与优化前逐字一致。
本批可只跑更长扩张模式以覆盖已有完整前缀；第二次扩张及封顶由Owner组合验收，不能写成已走两次自然玩家链。
现金流水、任务历史和完整内存检查点仍可能合法增长，有限自然轨迹不证明永久有界。

```sh
research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_continuous_tests natural_expansion 1 0
```

新增`natural_tools`从真实初始库存开始：自然人物到访后赠送普通道具，再给实际包子铺使用设施道具，
真实任务自然成功及商会补货后买入一件、售回同件，再经营一个完整自然月份。
它只发Session玩家命令，不注入钱／人物／库存／日期／flags；与任务黄金、住宅黄金及晋级编辑链各自验收。
新模式沿180000调用／3600秒保护，每轮检查页载荷／设施归属与输出消费，并报告资源峰值和退休情况。
`2b479f6`的Release长测已通过：真实补货后买入400G、售回200G，终点frame16263、10770G／106409抽，共81340项检查；详见[当前验证](../VERIFICATION.md)。

[自然任务流程测试](tests/startup_world_task_flow_test.cpp)接受可选`种子 速度`（默认`1 1`），
从真实空人物新局等待任务，按真实现金选择可支付任务，显式接受、等待征集并确认出发。
世界自行创建人物、寻路、探索或战斗，测试不注入成功/奖励。两类自然成功后不再接受新任务，
继续确认实际成果页与脚本，等待126/128/201/205/92/80相关任务续体退出、
成功后的真实任务工厂生成并返回主场景，才算闭环完成。
期限能支付则续费，否则明确选择原中止；取消、旧页重试、晚期页锁失败及成果重复奖励另有断言。
150000框架调用和3600秒是验收保护，不是原任务期限或游戏时钟；普通两月检查墙钟保护为1800秒。
并行构建导致CPU限流时先减少研究长测并发，不能改日期目标、删断言或补成功来缩短检查。

完整目录/共同AI模式与旧保护建设切片分开：本入口已接上述普通道具、商会与编辑消费者，
85定义预览已通过本批Release合同验收，仅读共享定义且没有实例邻接；完整主菜单、原UI皮肤与正常文件存取仍有边界；
不能从真实AI连续运行推出全部菜单、音频、图层桶溢出或跨版本截图视觉等价。
图标/文字、逻辑240×320及窗体480×640都是表现适配，验收边界见[当前验证](../VERIFICATION.md)。

2026-10-07新增[正常文件存取与精确回放](PERSISTENCE.md)：正常档支持稳定主场景，
`--world --save-file <路径>`在退出时保存，`--load-file <路径>`恢复，合用`--check`只读校验文件。
测试快照另存完整随机／页面／审计与自然玩家控制器，`natural_progression`／`natural_expansion`可保存和恢复外层轮边界。
标准新增集中codec／文件恢复、控制器、短跨进程和字段覆盖检查；中后期认证及尾段复用命令见模块说明。
后继StartupApplication已接四目录菜单及完整应用文件视图，独立world窗口不自动获得该接线；日历自动文件写入仍未接，原APK／Steam存档不兼容；当前日历检查点继续只作内存审计。

`--world --inspect-page world-building`和`world-details`从真实新局用显式命令打开目录/初始旅店详情；
`world-human`自然等到首个冒险者，再打开60并实际确认首次说明后停在详情，不注入人物或跳过初始化。
`world-award`以最多50000次真实框架更新、前序页面确认输入等到首个年度raw87，不补人物/年月/勋章。
`world-activities`从真实新局打开村办，按实际更新／确认处理首次说明后停在51，不注入点数或活动次数。
这些检查均要求`--frames N`有界运行，截图存项目内work；失败明确报告，不用合成Owner替代自然轨迹。
例如`--world --inspect-page world-activities --frames 12 --screenshot research/dungeon_village_1/work/world-activities.png`，
Windows使用本机构建的`.exe`并以`--font`指定可用中文字体；有界截图检查不等于真实鼠标／键盘验收。

默认窗口为240×320逻辑画布、480×640物理像素。建设→目录分类→条目→地图格→确定。
返回或Esc取消，放置模式支持连续建设；右键拖动视图，暂停/继续和1倍/2倍按钮控制模拟资格。
施工只在正常模式推进。首访两句提示逐次确认，然后镜头移到人物；没有人物移动命令。
退出不保存。详细人工清单见[试玩说明](../stages/PLAYTEST.md)。

页面截图检查不模拟鼠标，必须指定有界帧数：

```sh
research/dungeon_village_1/work/release/bin/dungeon_village_prototype --inspect-page shops --frames 8 --screenshot research/dungeon_village_1/work/startup-shops.png
```

支持`roads`、`shops`、`food`、`arrival`、`visitor`；它们明确安排检查状态，不能作为正常新局运行轨迹。

## 默认保护切片的来源边界

- 源24×24格、8设施种子、5000金币/10点数/50人气、1年4月、空人物场景与初期9项目录来自发布包。
  核心已重建576逻辑/显示格、8实例身份和向量顺序，建设使用加载后地表；原始ID0通过独立映射保留。
  这是静态重建，不是原版动态捕获；此旧窗口与共同世界的新逐格绘制入口分开登记。
- 普通商店280逻辑计数、入住募集1、向日葵立即可用；职业影响按已解锁定义计算，不按活人计算。
  募集建好后的住宅/入住副作用未实现，不由“施工完成”推导居民加入。
- 自动首访420次有资格更新，先创建UID0/定义1丰田龟次郎再触发事件89，不收取1500入住费。
  属性/装备及装配后的武器重选计数6保留；出生点由本地可重放策略选择，不还原APK随机消费。
  最终朝向/武器合成表现仍未知，介绍事件7和逐字对话节奏尚未接入。
- 首访实例标志为2与8192的组合，保留待处理活动0；人物仍停留出生格，暂不套用旧自主AI。
  原版可在教程触发同轮执行首次选路，原型延后请求是研究保护，不是原版“关闭教程才启动AI”。
  不能把当前静止当作原版行为，也不安排后续普通来客；[出发规则](../rules/CHARACTERS.md#first-activity)可独立组合验证。
  [地图组合](tests/startup_map_test.cpp)已覆盖两出生点到三个商店的连续行走与逻辑格进入，但只用于无玩家修改的新局测试，
  不在窗口自动执行；武器店未从候选删除，武器单类选择已交付，但装备到达/完整使用仍受保护。
  [使用组合](tests/startup_use_test.cpp)已连接两出生点的逻辑进入、原表基础端点、到达收入、包子60/旅店200等待、
  旧计数170恢复和共享完成/满足度候选；以显式目标及零实例修正为前置，只做无窗口条件测试。
  条件测试另覆盖单格出口保留、包子属性尾部与活动0顺序、首名武器计数正时不消耗抽选票号。
  [首段私有AI](include/dungeon_village_prototype/startup_ai.hpp)及[回归](tests/startup_ai_test.cpp)另已连接自主两级选择、
  邻接价格、占用/等待/完整退出、下一活动、包子属性累计和武器实际装配，覆盖两个出生点。
  它不接默认窗口/日历/玩家改图；表情/装备显示保留请求，不伪造随机变体或像素载荷。
  运行`dungeon_village_startup_ai_tests`可执行此无窗口组合；不能登记为默认全局AI或固定APK轨迹认证。
- [道路绘制参数](include/dungeon_village_prototype/road_render.hpp)交付整图绑定/尺寸、SEB锚点偏移及原深度；
  [CPU素材组合](tests/road_render_test.cpp)验证2×2原草心、原表道路flags1深度与前景遮挡。
  表现参数只属于原型表现边界，不引入领域库；共同世界窗口已消费，旧保护切片仍单独登记。
- 每正常逻辑步日历加27，每子周期10800、月4子周期；展示帧目标60FPS只是适配节奏。
  目录/放置/提示/镜头不推进模拟；2倍速每外层最多2步，不换算成原版真实秒。
- 首个月界进入受保护的月末状态，不扣夹具维护费、不给演示收益，等待跨月组合时序闭合。
- 新增/撤除后的地表暂用显示记录27，道路暂用37的源首帧；占用/扣款可测，邻接刷新不是原版重建。
  原型连续放置、撤除退款为0、窗体尺寸/物理热区是适配边界。
- 草地、建筑、入口与人物使用原始美术；首访使用秘书，而非后续解锁插画。
  字体、窗体和图标缩放是替代，不宣称固定APK或不同版本截图的视觉等价。

## AI表现适配交付

[facility_projection.hpp](include/dungeon_village_prototype/facility_projection.hpp)维护类别8休息/类别6特殊入口的
旧s→原60×30投影偏移→h.e世界坐标截断。领域只消费世界目标，不出现原像素位置；
[回归](tests/facility_projection_test.cpp)检查四方向×576格并连接完整使用队列。

[loop_pacing.hpp](include/dungeon_village_prototype/loop_pacing.hpp)维护固定APK默认47ms绘制路径门槛、
等待后观测提交和普通模式倍速迭代上限；[回归](tests/loop_pacing_test.cpp)覆盖整数除法、长停顿无补算及时钟回拨。
来源/限定见[限速规格](../rules/ai/LIFECYCLE.md#原版墙钟限速)。这两个适配均无窗口、无线程、无真实sleep，
交付给后续产品接入，不擅自修改当前窗口节奏或解开AI/月界保护。
框架调用链纠正了旧“栈顶更新前限速”的解释：更新路径先返回，下一g才限速/刷新输入/绘制。
纯等待数值函数未变；不能把其门槛应用在每个1/2逻辑轮之前。

## 设施商品与口碑页面

[startup_world_facility_catalog.hpp](include/dungeon_village_prototype/startup_world_facility_catalog.hpp)及其实现维护79商品目录、
72装备信息与82两段口碑演出；它属于既有唯一世界target，无raylib依赖。
四个附属字段保存初始化、目录／选择／首行、绑定与真实父页，共用计数／phase；
关闭提示及I请求在Owner候选提交，下一框架入口退休载荷。详细合同见[商品](../rules/COMMERCE.md#设施商品79来源已核维护消费者待接)、
[演出](../rules/FACILITY_EFFECTS.md#设施程序40与演出82来源已核维护消费者待接)。

研究窗口方向键选择／翻页，I查看信息，Enter及Escape调用各原页面消费者；82返回和确认同路径，不能取消奖励。
`--world --inspect-page world-goods --frames 8`及`world-equipment-info`从真实新局现存武器店74进入79／72；
没有现存实例时只按真实建设／施工继续，拒绝不补资金或实体。它们是有界维护路径检查，不是原APK动态或OS输入。
全部窗口仍需可用中文字体，原图标和动画并未随本批完整还原。

## 魔法壶与村办5／6

[startup_world_magic_pot](include/dungeon_village_prototype/startup_world_magic_pot.hpp)在唯一Owner中复用`legacy_n`和用户标志，
维护40配方共享进度、aM／aN／aQ全局展示事实及41–47真实目录／绑定／父页／计数。
菜单入口按日期处理，42当下扣持有道具及一次共同评语随机；44取消不退款，45只展示处理结果，46才发现，47才扣元素并兑现低层奖励。
设施奖励107对话先于93领取、最终恢复43；不直接建造。只初始化实际栈顶，关闭载荷在下一框架入口退休。
村办53满120后先扣q，5升壶级上限3，6开放用户标志／104及首次219延迟；类型4排除不变。

简化窗口开放后用P／“魔法壶”发送主菜单入口；方向键／列表选择、左右配方翻页、Enter／Escape沿各页消费者，45不能取消。
有界`world-magic-pot`诊断可只读加载已校验normal档并调用真实入口，不能与写档混用：

```powershell
& research/dungeon_village_1/work/release/bin/dungeon_village_prototype.exe --world --load-file research/dungeon_village_1/work/release/prototype/persistence-tests/magic-pot-window.avrs --inspect-page world-magic-pot --frames 8 --font build/local-tools/NotoSansCJKsc-Regular.otf --asset-root research/dungeon_village_1/assets
```

上述文件由现有持久化套件产生并读回校验，是明确壶解锁／持有库存前提的组合夹具，不能冒充自然第二星获得壶的路线。
完整原皮肤、绘制随机、真实OS输入与自然后期路线未验，实际检查见[验证](../VERIFICATION.md#魔法壶与村办56维护消费者2026-10-07)。

## 旧夹具

`--fixture`显式进入旧7×7场景；`--demo`隐含该模式并自动退出。
`--fixture --check`执行原自主访问/周期闭环；`--orientation 0/1`与`--data-file`仅作用于旧夹具。
`--asset-root`是两个模式共同的程序旁素材根目录，新局要求保留`original`结构，旧模式要求七个清单键。

旧模式10000初始金币、三次建设后7300、两位夜骑士、600个100ms tick月份都是夹具。
仅类别1/2、独占预约、完成回接近格、统一旋转收费、周期报表仍是原型策略。
它保留此前自主访问回归，不用于认证首名人物行为或产品新局初值。
