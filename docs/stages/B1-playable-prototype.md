# B1：持续世界产品接入

[文档索引](../README.md) · [当前对照](../reference/REFERENCE_CHECKLIST.md#research-history-current-audit) · [完整历史](history/B1-implementation-log.md)

本文件只记录当前收口批次和历史导航。旧切片、Mac四配置验收、手动月报、二倍速等旧决定保留在历史中；当前政策以架构与后继ADR为准。

<a id="research-ee687bc-replay"></a>

## ee687bc按序接入：回放工具（2026-10-07）

用户已确认按“回放提效→79/72及82→魔法壶→精确表现/后期”顺序推进。当前正式交付ee687bc只修改回放runner和规则/证据文档，无新C++消费者、原表或UI截图。本批先更新这一项冻结工具；其余353项仍沿524415a来源及既有产品适配，不全量覆盖或更换玩家存档身份。

范围：natural_progression默认合同不变，新增natural_expansion、已认证前缀与明确标记的裸候选接续。原快照和证书只读，新捕获后再比较两个恢复进程；源身份/实际next_frame、场景、seed/speed、摘要、三路轨迹/stdout和各场景黄金终点断言保留。证书记录参考轨迹来源，候选后代不升级为完整新局认证。

产品适配继续限定临时/周期/输出及输入前缀/证书在产品build中，经真实祖先路径拒绝junction逃逸；只清理自己mkdtemp的成功现场，失败留证。源码从Git ee687bc固定，清单记录新增来源提交及产品路径适配，不导入research在途文件。

本批验收已完成：

- desktop-debug配置/构建及标准173项通过（81.45秒），headless-debug标准149项通过（76.08秒）；均只排除`simulation.startup_world_continuous_test`，保留注册/参数/断言，Release CI待验证。
- 冻结默认晋级短三进程保留原参数/断言。新增`simulation.replay_runner_contract`复用continuous二进制，以真实扩张420捕获/440终点、已认证及裸候选接续到444，共15个正反场景验证资格、源只读、证书/帧/场景拒绝、build边界和junction逃逸；两配置均通过。新套件负责产品CLI边界，不复制规则或仿造AVRSAVE1解析器。
- Release使用现有已认证38000晋级档，38001重新捕获、38002一帧尾段三路一致，三进程约1.386/0.609/0.611秒。兼容既有旧证书协议，明确标为resumed_reference_tail；没有重跑38000新局前缀、重新认证完整282帧尾段或执行扩张108700晚期链。
- 输入前缀使用产品build中已有35,087,165字节快照的只读NTFS硬链接，避免再复制35MB；证书保持原字节，成功临时输出已清理。353项原记录逐项不变，runner源/产品哈希独立登记，354项来源校验、Node语法、文档链接及diff检查通过。
- 证据：`build/validation/replay-ee687bc-20261007/`。没有UI/规则/codec/玩家档变化，不重复窗口或生成ZIP；未推送，CI待验证。收口时研究79/72及82相关源码/测试仍未提交，未导入或夹带；后续按已确认顺序等待正式维护交付。

<a id="title-start-flow"></a>

## 标题与开始入口（2026-10-07）

用户要求接入已研究的开始界面。正式来源：1832b8c归档S038标题、S039目录、S040手动栏子菜单及STEAM_INTERACTIONS；5eae10a仍列空档动态/第二页/标题动画为缺口。使用APK三份PNG（240×330背景、236×115 Logo、98×68书本），不裁截图或将Steam版本号写入产品。详见ADR-0060。

默认无界启动显示标题；开始→两个Ark手动栏位，空栏新游戏，有档→继续/新游戏。新游戏不覆盖文件；继续复用玩家校验/恢复，仅成功后启动模拟线程，冷启动随机/暂停政策保留。纪录/删除禁用，无自动档、Steam依赖或原档迁移。标题不推进时钟、AI、随机；尺寸/点击为PC适配，完整背景人物动画另待精确合同。

本批验收完成：

- desktop-debug标准172项通过，73.33秒；headless-debug标准148项通过，65.69秒，两套均只排除`simulation.startup_world_continuous_test`，三个月注册/参数/断言保留，Release CI待验证。
- 既有world_menu覆盖标题/空栏/有档子菜单、禁用项、逐级返回、当前随机/暂停、文件在展示后损坏时重新读取并保持Owner；launch_options覆盖三个标题诊断及拒绝旧切片组合。没有新增测试target或重复存档规则组合。
- Release最终7张实际窗口：横屏/最小标题、横屏/最小两栏目录、手动栏子菜单、隔离目录保存与另一进程冷载入。标题截图世界更新为0；保存/加载分别generation1/2。有档目录使用本批真实保存，不填演示余额。输入契约测试与窗口绘制分别成立，未认证OS鼠标或原游戏动态。
- 初次窗口发现Windows高DPI图片/文字错位，已抽取原有WorldCanvas供标题与世界共用原生画布，最终窗口复验通过；标题输入完成后清理本轮输入，避免同次点击/确认透传到世界。
- 三份PNG来自已提交研究字节，新增来源/用途/尺寸/哈希，当前619项素材检查通过。复用本地Noto生成字体，未下载新依赖；34份产品Markdown共456个本地链接/锚点检查、C++格式与diff检查通过。
- 证据：`build/validation/title-start-20261007/`。未推送、CI待验证、未生成新ZIP；research在途存取/经营文档和测试保持原样。玩家存档格式/随机/暂停政策不变，自动档、纪录、删除、完整标题人物动画仍未接。

<a id="research-save-organization"></a>

## 最新研究、存档审计与结构整理（2026-10-07）

### 来源与范围

产品基线5fc5d52；正式研究核对至6061a2c及后继归档5eae10a。6061a2c交付非空设施p实际重载/业务施工交叉和S062–S068，无新维护C++或规则表；5eae10a整理当前能力和历史入口，明确晚期自然认证仅晋级38000→38282，扩张控制器接线/字节往返不等于扩张晚期认证。冻结仍为524415a的354项；不导入研究未提交内容。

S067/S068只消费五行候选、头像/名称/贡献/勤奋度列、橙色选择/手形/蓝滚条和中止确认视觉；数值来自当前Owner。沿用已有先选人再授予、独立确认/拒绝、默认否中止及FIFO门槛。标题三页切换、信息软键、勋章图标与原Surface尺寸未完整接线，不假画可操作入口。S062的自动栏不替换玩家两栏手动政策；S063/S065不是施工帧/阈值合同。

维护性整理沿已批准工程建议：HUD与通用页面文本移至现有ui/world_panels，窗口仍独占输入/生命周期/线程协调，不新增target或Owner。B1旧记录、已完成任务台账和原审阅完整归档，保留旧锚点及相对链接；TODO改为待办，docs/README提供分类索引。

### 存档审计结论

玩家手动存读主链已实现，不能称为完整原版存档系统：

| 能力 | 当前结论 | 主责验证 |
| --- | --- | --- |
| 两栏/覆盖/读取确认 | 已实现，稳定主场景；月报/问答/授勋等模态拒绝 | world_save、world_save_commands、world_save_menu |
| 字节/版本/大小/引用/地图 | 校验、预算、数据集身份、悬空/重复引用及混合地表拒绝 | world_save_codec/restore；player_save_policy |
| 失败安全 | 同目录独占临时写、flush、Windows替换；失败保留旧档/Owner | 文件锁/目标阻塞、命令失败回滚 |
| 加载提交 | 当前随机/暂停/速度沿用；generation拒旧命令；不重复收费/发奖 | commands、自然服务/施工/跨月续跑 |
| 非空设施p | Ark按已批准政策保留，与原游戏读弃明确不同；施工/实例/经营字段独立保存 | 本批扩展既有施工往返，非空[3,9]和下一轮持久状态对照 |
| 维护精确回放 | 独立AVRSAVE1 normal/replay，完整随机/Owner/历史，不接玩家菜单 | persistence及短三进程replay标准回归 |
| 自动跨周/任意模态/标题继续 | 未实现；轮内续体仍缺正式维护消费者 | RQ12，不能用外层快照冒充 |
| 原档/旧schema迁移、退出自动保存、Steam服务 | 不在当前范围 | 不做兼容或隐式写档 |

6061a2c的最早后档距恢复点击353ms，与静态读弃p相容但不能独立排除自然消费；不是首帧。p清空不等于删除建筑，阶段/计数及相关业务子集保留并继续施工。此证据补审不改变玩家或维护格式。

### 本批验证

- 本地desktop-debug构建与标准CTest **172项全部通过，80.44秒**，只排除`simulation.startup_world_continuous_test`；包含玩家world_save、会话/文件拒绝、维护persistence、短三进程replay、字段分类及冻结来源检查。本批非空p施工往返/下一轮对照通过。
- 截图检查后压紧授勋布局、将中止问题改为独立白色正文区与单项橙色高亮；最终Debug/Release均重新构建，受影响6项定向CTest全部通过，0.25秒。没有重复全套或降低原断言。
- Release窗口：授勋名单/中止确认在540×360与240×256共4张截图；使用明确的7人名单调用点夹具，当前职业/贡献/勤奋度取真实新局Owner，不声称自然年度到达或截图中的原版数值。既有同依赖UI套件承接夹具，不新建测试target。
- Release存档窗口另3张：隔离目录保存完毕、另一进程冷加载、空第二栏读取失败；成功加载generation=2且保留当前暂停/随机，失败保持generation=1及旧Owner。窗口检查使用稳定初局；施工/服务/扩张/跨月等更丰富状态由存档回归覆盖，不把初局截图扩张为全部场景认证。
- 34份产品Markdown的本地文件/标题锚点检查通过；三份历史归档逐字对照原正文，仅新增归档提示与调整相对链接。C++格式和diff检查通过；本地既有Noto生成1732码点/387860字节子集，无新增依赖下载。
- 证据在`build/validation/research-save-organization-20261007/`。本批不改核心接口/依赖或CMake，不增加headless矩阵/年度长跑；三个月保留注册，交由Release CI。未推送，CI待验证；没有OS鼠标认证、原版动态复验或新ZIP。research后续在途存取文档/测试改动未导入或提交。

## 历史导航

以下保留原入口的标题和显式锚点，具体内容在完整历史中。

<a id="s057-catalogue-correction"></a>

## S057建设目录版式纠正（2026-10-07）

[查看历史记录](history/B1-implementation-log.md#s057建设目录版式纠正2026-10-07)

<a id="latest-ui-evidence"></a>

## 最新截图与设施交互子集（2026-10-07）

[查看历史记录](history/B1-implementation-log.md#最新截图与设施交互子集2026-10-07)

<a id="pc-pointer-input"></a>

## PC鼠标交互接线（2026-10-07）

[查看历史记录](history/B1-implementation-log.md#pc鼠标交互接线2026-10-07)

<a id="desktop-glyph-inventory"></a>

## 统一桌面字形需求（2026-10-07）

[查看历史记录](history/B1-implementation-log.md#统一桌面字形需求2026-10-07)

<a id="research-524415a-design"></a>

<a id="maintainability-ui-batch"></a>

## 工程收口与设施静态UI（2026-10-07）

[查看历史记录](history/B1-implementation-log.md#工程收口与设施静态ui2026-10-07)

### 实现与来源

[查看历史记录](history/B1-implementation-log.md#实现与来源)

### 本地验收

[查看历史记录](history/B1-implementation-log.md#本地验收)

## 524415a存取与精确回放接入设计（2026-10-07，玩家政策已确认）

[查看历史记录](history/B1-implementation-log.md#524415a存取与精确回放接入设计2026-10-07玩家政策已确认)

### 已确认的玩家政策与备选对照

[查看历史记录](history/B1-implementation-log.md#已确认的玩家政策与备选对照)

### 来源身份与验收

[查看历史记录](history/B1-implementation-log.md#来源身份与验收)

<a id="research-898b653-design"></a>

## 898b653接入设计与产品补丁逐项审阅（2026-10-06设计，2026-10-07本地验收）

[查看历史记录](history/B1-implementation-log.md#898b653接入设计与产品补丁逐项审阅2026-10-06设计2026-10-07本地验收)

<a id="pc-input-performance"></a>

## PC输入审阅与增长轨迹性能侦察（2026-10-06）

[查看历史记录](history/B1-implementation-log.md#pc输入审阅与增长轨迹性能侦察2026-10-06)

<a id="pending-level-ui"></a>

## 已发布稳定UI：经验期间P闪标（2026-10-06）

[查看历史记录](history/B1-implementation-log.md#已发布稳定ui经验期间p闪标2026-10-06)

<a id="normal-speed-audit"></a>

## 最新研究审计与玩家一倍速（2026-10-06）

[查看历史记录](history/B1-implementation-log.md#最新研究审计与玩家一倍速2026-10-06)

<a id="map-editing-ui"></a>

## 道路、移动、撤除与住宅重建（2026-10-06）

[查看历史记录](history/B1-implementation-log.md#道路移动撤除与住宅重建2026-10-06)

<a id="world-performance"></a>

## 窗口合并与世界复制优化（2026-10-06）

[查看历史记录](history/B1-implementation-log.md#窗口合并与世界复制优化2026-10-06)

<a id="research-2b479f6-integration"></a>

## 最新经营与编辑研究接续（2026-10-06）

[查看历史记录](history/B1-implementation-log.md#最新经营与编辑研究接续2026-10-06)

<a id="village-construction-ui"></a>

### 村办、奖励与建设视觉接线

[查看历史记录](history/B1-implementation-log.md#村办奖励与建设视觉接线)

<a id="items-commerce-ui"></a>

### 普通道具、设施强化与商会

[查看历史记录](history/B1-implementation-log.md#普通道具设施强化与商会)

## 已确认范围与研究覆盖

[查看历史记录](history/B1-implementation-log.md#已确认范围与研究覆盖)

## 恢复可玩实现的最小交付

[查看历史记录](history/B1-implementation-log.md#恢复可玩实现的最小交付)

## 本轮实际范围与差异

[查看历史记录](history/B1-implementation-log.md#本轮实际范围与差异)

## 模块与依赖

[查看历史记录](history/B1-implementation-log.md#模块与依赖)

## 状态与命令契约

[查看历史记录](history/B1-implementation-log.md#状态与命令契约)

## 自底向上的实施顺序

[查看历史记录](history/B1-implementation-log.md#自底向上的实施顺序)

## 验收与暂停条件

[查看历史记录](history/B1-implementation-log.md#验收与暂停条件)

## UI 对齐迭代设计（2026-10-03）

[查看历史记录](history/B1-implementation-log.md#ui-对齐迭代设计2026-10-03)

## 首个切片验证（2026-10-03，UI迭代前）

[查看历史记录](history/B1-implementation-log.md#首个切片验证2026-10-03ui迭代前)

## UI迭代验证（2026-10-03）

[查看历史记录](history/B1-implementation-log.md#ui迭代验证2026-10-03)

## 经营详情接入设计（2026-10-03）

[查看历史记录](history/B1-implementation-log.md#经营详情接入设计2026-10-03)

### 本轮追加：道路与地图缩放

[查看历史记录](history/B1-implementation-log.md#本轮追加道路与地图缩放)

### 本轮验收

[查看历史记录](history/B1-implementation-log.md#本轮验收)

## macOS缩放清晰度修正（2026-10-03）

[查看历史记录](history/B1-implementation-log.md#macos缩放清晰度修正2026-10-03)

## 加载后地图、访问与运动接入设计（2026-10-03）

[查看历史记录](history/B1-implementation-log.md#加载后地图访问与运动接入设计2026-10-03)

## 加载后地图、访问与运动验收（2026-10-03）

[查看历史记录](history/B1-implementation-log.md#加载后地图访问与运动验收2026-10-03)

## 人物AI接入方案（2026-10-03，已确认）

[查看历史记录](history/B1-implementation-log.md#人物ai接入方案2026-10-03已确认)

### 可先实现的决策底层

[查看历史记录](history/B1-implementation-log.md#可先实现的决策底层)

### 启用默认AI的最小缺口

[查看历史记录](history/B1-implementation-log.md#启用默认ai的最小缺口)

### 实施与验收

[查看历史记录](history/B1-implementation-log.md#实施与验收)

## 人物AI决策底层实施（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#人物ai决策底层实施2026-10-04)

## AI感知、优先级与控制前缀接入（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#ai感知优先级与控制前缀接入2026-10-04)

## 道路草心修正（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#道路草心修正2026-10-04)

## 桌面启动与暂停提示（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#桌面启动与暂停提示2026-10-04)

## 调度、效果、装备与成长接入（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#调度效果装备与成长接入2026-10-04)

## 真实首段AI与设施服务（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#真实首段ai与设施服务2026-10-04)

## 主程序人物AI可视预览（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#主程序人物ai可视预览2026-10-04)

## 人物脚底投影与步态修正（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#人物脚底投影与步态修正2026-10-04)

## 逻辑时钟与绘制分离（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#逻辑时钟与绘制分离2026-10-04)

## AI战斗决策与共用更新底层（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#ai战斗决策与共用更新底层2026-10-04)

## 最新研究对照与原版限速/探索底层（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#最新研究对照与原版限速探索底层2026-10-04)

## 正常村庄生活循环（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#正常村庄生活循环2026-10-04)

## 店内人物可见性修正（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#店内人物可见性修正2026-10-04)

## 栅栏与外部入口绘制（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#栅栏与外部入口绘制2026-10-04)

## 共同调度、真实路径及血条（2026-10-04，已验证接入切片）

[查看历史记录](history/B1-implementation-log.md#共同调度真实路径及血条2026-10-04已验证接入切片)

## 外部出口草地显示修正（2026-10-05）

[查看历史记录](history/B1-implementation-log.md#外部出口草地显示修正2026-10-05)

## 人物行走四向显示修正（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#人物行走四向显示修正2026-10-04)

## 完整新局世界接管方案（2026-10-04，待确认/研究稳定）

[查看历史记录](history/B1-implementation-log.md#完整新局世界接管方案2026-10-04待确认研究稳定)

### 问题与选择

[查看历史记录](history/B1-implementation-log.md#问题与选择)

### 模块与顺序

[查看历史记录](history/B1-implementation-log.md#模块与顺序)

### 本轮独立检查

[查看历史记录](history/B1-implementation-log.md#本轮独立检查)

### 接入验收

[查看历史记录](history/B1-implementation-log.md#接入验收)

## 完整世界分批接入（2026-10-05）

[查看历史记录](history/B1-implementation-log.md#完整世界分批接入2026-10-05)

### 本批实现和验收

[查看历史记录](history/B1-implementation-log.md#本批实现和验收)

## 持续世界成为默认入口（2026-10-05）

[查看历史记录](history/B1-implementation-log.md#持续世界成为默认入口2026-10-05)

## 启动事实、随机与日常决策接入（2026-10-04）

[查看历史记录](history/B1-implementation-log.md#启动事实随机与日常决策接入2026-10-04)

## 战斗、设施反馈与桌面调度（2026-10-05）

[查看历史记录](history/B1-implementation-log.md#战斗设施反馈与桌面调度2026-10-05)

## 已发布持续世界修正接入（2026-10-05）

[查看历史记录](history/B1-implementation-log.md#已发布持续世界修正接入2026-10-05)

## 成果统计与攻略标记（2026-10-05）

[查看历史记录](history/B1-implementation-log.md#成果统计与攻略标记2026-10-05)

## 当前世界菜单与设施详情设计（2026-10-05）

[查看历史记录](history/B1-implementation-log.md#当前世界菜单与设施详情设计2026-10-05)

## 任务自然闭环产品接入（2026-10-05）

[查看历史记录](history/B1-implementation-log.md#任务自然闭环产品接入2026-10-05)

## 菜单恢复与招募展示（2026-10-05）

[查看历史记录](history/B1-implementation-log.md#菜单恢复与招募展示2026-10-05)

<a id="e8f66d9产品接入设计2026-10-05实施中"></a>

## e8f66d9产品接入（2026-10-05）

[查看历史记录](history/B1-implementation-log.md#e8f66d9产品接入2026-10-05)

<a id="task-report-ui"></a>

## 任务、月报与升级通知表现接入（2026-10-05至06）

[查看历史记录](history/B1-implementation-log.md#任务月报与升级通知表现接入2026-10-05至06)

<a id="build-rotation-fix"></a>

## 单帧建筑旋转退出修复（2026-10-06）

[查看历史记录](history/B1-implementation-log.md#单帧建筑旋转退出修复2026-10-06)

<a id="human-management-integration"></a>

## 8f12654人物经营链接入（2026-10-06，已验收）

[查看历史记录](history/B1-implementation-log.md#8f12654人物经营链接入2026-10-06已验收)

### 本批验收记录

[查看历史记录](history/B1-implementation-log.md#本批验收记录)

<a id="file-persistence-design"></a>

## 文件存取接入方案（2026-10-06，已批准）

[查看历史记录](history/B1-implementation-log.md#文件存取接入方案2026-10-06已批准)

### 建议范围与所有权

[查看历史记录](history/B1-implementation-log.md#建议范围与所有权)

### 依赖与执行次序

[查看历史记录](history/B1-implementation-log.md#依赖与执行次序)

### 验收

[查看历史记录](history/B1-implementation-log.md#验收)

### 手动档实现与本地验收

[查看历史记录](history/B1-implementation-log.md#手动档实现与本地验收)

<a id="dynamic-linking-validation"></a>

## 单份公共Release库与缓存清理（2026-10-06）

[查看历史记录](history/B1-implementation-log.md#单份公共release库与缓存清理2026-10-06)
