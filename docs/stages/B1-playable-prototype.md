# B1：持续世界产品接入

[文档索引](../README.md) · [当前对照](../reference/REFERENCE_CHECKLIST.md#research-history-current-audit) · [完整历史](history/B1-implementation-log.md)

本文件只记录当前收口批次和历史导航。旧切片、Mac四配置验收、手动月报、二倍速等旧决定保留在历史中；当前政策以架构与后继ADR为准。

<a id="business-validation-dll-docs"></a>

## 经营流程、公共DLL与现状文档（2026-10-08，本地验收完成）

用户明确确认三线并行，产品起点aead864，冻结0a5b5e2的361项来源与670项素材。本批不导入research工作区在途实现；开始时98f8edc仅输入/绘制副作用合同，收口后继1e6b291已发布接口，作为后续独立产品调度设计/接入项（见本节结果）。开局创建主角、标题纪录和人物设施属性cd13头标继续等待正式交付。

| 工作线 | 范围与验收 | 当前证据边界 |
| --- | --- | --- |
| 真实经营流程 | 从真实经营状态串起购买→出店举物→装配，真实建设→邻接变化→实际经营属性/提示（自然搬迁另待解锁），保存→独立进程加载→继续经营；核对原序/实际装备/属性、无重复收费发奖与只读显示 | 前批27张窗口包含调用点夹具，不能替代本批自然触发链；测试驱动、实际OS输入与原游戏动态分别记录 |
| DLL防错 | 公共输入指纹和成功库产物哈希，消费者构建/启动检测混用；四配置仍共用Release库，玩家包不依赖Node或未使用维护库 | 在访问业务对象前拒绝已加载Ark库身份不符；系统loader先报缺DLL/缺导出不属于守卫可接管范围，见ADR-0062 |
| 现状文档 | README、架构、当前待办、研究对照/需求及模块说明统一到aead864；保留历史提交、来源与验收身份 | 文档仅检查差异/链接，不以数量或旧日志证明新功能完成 |

验收计划：实现收口后按AGENTS运行desktop-debug标准CTest；DLL/CMake边界补headless-debug，两套仅排除三个月。Release按经营链和启动风险定向验证及必要窗口检查，纯只读/DLL防错不机械重复同一三个月核心轨迹。最终结果在本节补记；未验证项目不提前标完成，未获推送授权，CI另记。

### 本批实现与结果

- 公共库/四套消费者均已完成首次守卫部署；四套仍共享同一Release DLL集合，headless-release本批只构建、不重复核心轨迹。desktop-debug标准174项通过（97.22秒），headless-debug标准150项通过（97.40秒），两套仅排除simulation.startup_world_continuous_test。该标准基线的注册/参数/断言保持，最终提交的Release完整CI待验证，未推送。
- 内容身份覆盖实际库源、公开/私有头及.inc/.ipp片段、编译数据、字形生成内容和工具链设置；完成记录核对DLL及importlib字节。每个DLL独立C导出，EXE在普通全局业务构造/main前检查实际已加载Ark依赖。首次全量链接发现MinGW显式导出抑制原自动业务导出，已以export-all-symbols修复，并加入未注解业务函数的真实DLL回归；不改冻结源码。构建检查及串行公共库约定保留，不承诺任意并发完整重建的所有竞争。
- 新增shared_library_contract作为独立进程/构建主责套件，覆盖未完成发布、头/片段/字形输入变化、DLL/importlib替换、缺指纹、旧EXE/新DLL、业务全局初始化前拒绝，以及文档不使身份失效。未改写活动build/bin库做坏输入实验。另实际运行一次本批自然形成的旧Release EXE/新DLL组合，退出78并打印重建提示、没有进入世界；随后重建消费者恢复正常。
- 自然经营使用既有UI测试target的两个手动场景，新增单文件承担完整窗口生命周期，不混用旧的注入状态夹具。真实seed1、从首轮使用逻辑视口{0,24,384,211}，仅执行真实菜单/建设和明示对话确认。建设66于{9,5}扣200G、31于{11,5}扣1000G，均第一朝向；实例3/4的邻接缓存、实际经营属性及提示同步改变。没有注入现金/解锁/装备/cd/提示。
- 驱动更新序号2464观测武器店收费400G，2528出现cd15，2569实际武器装配；2596收费320G，2660出现cd21，2691实际防具槽2装配。核对待装配命令、重选计数6、共享装备combat和演员武器；绘制多帧不改变资金/随机/提示/装备计数。按真实状态输出9张窗口图；独立取景相机不改变Owner镜头及过去可见性资格。
- 实际玩家档165451字节、保存资金5060G。第二个全新进程使用seed20261008读取同一隔离文件，耐久内容重新编码逐字节相同，保留冷会话随机；源世界步2664恢复，后续第26次更新自然发生300G设施收费。保存/恢复不重扣建设或装备费用。这是源事务驱动＋生产绘制的自然路径验收，不是OS鼠标/原APK动态认证；饰品cd22、自然flag32搬迁及魔法壶自然二星不在该短链结论内。
- 便携包按原PE导入闭包剥离分发副本，携带13个必要DLL、670项素材与字体，未带维护/测试DLL或Node。ZIP为4,936,312字节（约4.71MiB），解压11,971,393字节；从独立目录且PATH仅含Windows系统目录运行--check及真实标题窗口通过，存档目录隔离，标题world_updates=0。本地精简Python缺zlib，压缩/解压改用Windows .NET Optimal；不修改CI的标准Python压缩路径。包标记local-worktree-validation-based-on-aead864，属于本批本地验收制品，不冒称GitHub发布。SHA-256：25d2de796d1ccf3d7cbc297641680691489621f95d18bc7043bde2e6f630cfaa。
- 证据：build/validation/business-and-library-guard-20261008/，包含构建/CTest、经营页序和付款/装配日志、冷进程、失配拒绝、便携包及截图。12份现状文档304条本地路径/锚点、产品C++格式及diff检查通过；保留历史身份，关闭已接79/72、82、魔法壶及举物/建筑提示的重复请求。最终源码/库指纹检查通过；不重做本批无变动的三个月轨迹。
- 收口期间研究正式发布1e6b291（建设图块查询、Owner显式表现请求与独立回放），已只读补审并登记为另批产品调度/资源接线事项，没有夹带迁入。人物属性cd13、主角创建和标题纪录仍未在该提交交付，按用户要求等待。

<a id="research-a57958c-design"></a>

## a57958c/0a5b5e2：魔法壶、存档与经营反馈（2026-10-08）

沿用户确认的顺序接入a57958c魔法壶/村办5–6，再从正式Git 0a5b5e2冻结361项源/数据/测试。四项既有产品适配保留，壶数据和独立candidate回归显式重基；研究工作区后继在途改动不导入。唯一Owner持有40条配方进度与41–47页面，legacy_n/user_flags复用；村办子菜单与K走main_menu入口，107对话、93领取和219延迟新闻沿共同页栈。投入/处理/发现/开发分别消费，45没有返回，不从绘制发奖。

用户2026-10-08明确批准破坏旧档及新增资源/来源清单。玩家ARKSAVE1升级schema3，新增配方identity/status/pending_notice，已有壶13槽与flags继续保存；7类页载荷/评语/动画瞬态不落盘。恢复校验40项固定身份、状态及壶处理日期，保留当前随机/暂停/速度。旧schema1/2和旧数据身份明确拒绝，不迁移或自动删除。玩家数据身份为冻结snapshot SHA-256 e2e0d0d06765bea97f808cb8903637b4d209770aad404b47088985129df38884；维护AVRSAVE1的完整随机/Owner协议保持独立。

覆盖归属沿现有套件：源facility_items/pages/persistence保留所有规则、顺序及回滚断言；world_session覆盖专用FIFO/锁定入口/投入一次/错误与过期输入，world_commerce覆盖只读面板与门槛，world_save覆盖配方及13槽往返、7瞬态排除、原处理消费者续跑、坏身份/状态/日期与旧版拒绝。不新增一次性测试target。

<a id="equipment-facility-render-design"></a>

### 用户点名的举物与邻接反馈

用户在本轮明确要求“人物与装备店交互以后，出门以后……双手举着他购买的装备停顿一下”，并指出相邻设施的魅力/价格/品质加成；这是当前接入任务的具体要求，沿用“已有确认”的实现范围。研究随后正式发布0a5b5e2，只增加对应只读绘制及装备资源投影，不改变购买、等待、装配、邻接结算或存档schema。

方案限于该提交的公开StartupVisualDraw接口：既有cd15/21/22在人物身体之后绘制、按源年龄绑定旧/新装备；设施队首1–6以原f()投影进入稳定深度队列。产品从已提交的原始weapon目录复制所需素材，记录哈希/尺寸/用途；指定SEB层及图片override，不画原空记录或猜图标。79/72商品图标使用已发布18×18映射。验收复用冻结visuals套件、产品资源/场景测试和明确调用点窗口，断言绘制不改变资金/装备/计数/随机。此范围不包含尚未交付的人物属性cd13、主角创建或纪录页。

用户已明确授权新增资源与来源清单；产品从Git 0a5b5e2固定导入361项，保留四项既有产品适配。新增weapon目录51项后共670项素材。指定SEB层/图片override、人物举物与建筑队首1–6提示、79/72商品图标已接线；人物属性cd13和研究工作区在途修改不纳入本批。

编译输入发现新冻结C++头带UTF-8 BOM；不修改来源字节，字形扫描器只消费文件开头编码签名，仍按原字节哈希。现有Unicode套件补C++/头/JSON签名回归；本地固定Noto源重建1733字形、388268字节子集，未下载新依赖。

### 本批本地验收

- desktop-debug配置/构建及标准173项全部通过，81.25秒；headless-debug配置/构建及标准149项全部通过，85.62秒。两套仅排除simulation.startup_world_continuous_test，保留其注册/参数/断言。字段分类、维护完整codec、短三进程回放、玩家恢复/拒绝和FIFO均包含在标准套件内。
- 新源visuals使用完整Owner摘要作只读oracle，首次链接发现缺ark_world_persistence依赖；仅给该桌面测试目标补依赖，最终编译及原断言通过，游戏不因此依赖维护codec。冻结visuals覆盖113装备定义、132武器方向与设施各阶段资源，未删除空SEB原记录或放宽裁片校验。
- Release构建通过；world_save、world_commerce、simulation.startup_world_visuals_test与packaged_assets定向4项通过，17.79秒。a57958c数据版本此前原参数三个月通过63.99秒（证据build/validation/magic-pot-a57958c）；随后0a5只增加静态装备资源投影与只读查询，未重复同一核心长轨迹，不声称最终提交的完整Release CI已运行。
- Release实际窗口共27张：20张商品/魔法壶及既有设施页，3张新举物/建筑升降阶段图与1张既有P标复验，25%自然世界1张、隔离保存与另进程读取2张。商品图标在最小窗正确显示；效果覆盖四朝向、旧/新武器20门槛、防具/饰品和六种建筑升降。效果/页面使用明确调用点夹具，绘制不改计数/装备/资金/随机，不宣称自然购买、二星解锁或原APK动态认证。25%真实世界到三人自然到访、failed=0；冷加载generation=2，保留暂停。
- 361项冻结来源与670项素材哈希通过；weapon新增33 PNG、16 SEB、2 INF，共39,934字节。10份受影响文档285条链接检查通过，32份产品C++格式检查通过。weapon/INF新增-text规则，暂存区全部1031项冻结源/素材与工作区原字节一致，防止下次检出换行转换破坏哈希。新BOM字形回归与1733码点本地Noto子集沿前述处理，无新增依赖下载。
- 主要证据：build/validation/equipment-0a5b5e2/；研究后继98f8edc已只读补审，仅新增输入/绘制副作用合同与待确认Owner请求提案，无新增代码接口。人物设施属性cd13、主角配置与标题纪录仍待正式维护交付，不把在途文件夹带入本批。未推送、CI待验证、未生成新ZIP。

<a id="research-29f371d-progression"></a>

## 29f371d：设施事件与持续推进（2026-10-08）

沿用户已确认的接入顺序，以及“尽可能多的通知、活动、村子状态变迁”要求，先闭合已正式发布的79/72、opcode40→82。输入严格来自Git提交29f371db6c5a9a020e771a1b7ff4de580137e6a1，356项冻结；魔法壶在途修改不导入。原四项产品适配保留，新增原表测试路径仅映射到产品tenantData副本，不改变原断言。

本批范围：装备商店74的“商品”入口，79原交换排序/四行/两页/清整类NEW，72独立信息及父页返回，82真实设施/共享人物绑定、两段40门槛和最终头插人气请求。状态和选择都由Owner持有，桌面只读、FIFO提交，重复/过期输入拒绝；普通确认不能旁路业务。人气仍由后续共同消费者结算，绘制不发奖。

玩家ARKSAVE1/schema2不增加页面字段；加载清除四项新页面缓存，设施icon由固定定义legacy_icon纯重建，以便保存的设施脚本续体可以恢复opcode40。维护AVRSAVE1同步新字段及layout身份，旧维护前缀不视为兼容，不迁移旧档。用户新增25%缩放下限作为PC视图政策，默认100%、上限200%、鼠标锚定与5%滚轮步长保持，旧诊断仍50%下限。

| 契约与风险 | 主责验收 |
| --- | --- |
| 26条原设施程序、类别分派、原目录顺序、两个40门槛及延迟人气 | 迁入scripts/pages/building套件，保留所有原断言 |
| 专用命令、通用确认拒绝、过期输入、无重复消费 | 既有world_session中设施命令场景 |
| 精确恢复与坏父页/绑定拒绝；玩家延迟程序续跑 | 维护persistence/restore与既有world_save |
| 只读商品价/属性、四行命中、pending、25%拾取/投影 | 既有world_commerce/world_building/world_scene/launch_options |
| 新页面实际绘制、窄窗口及25%场景；持续世界不因新演出卡住 | Release窗口、Release三个月基线；不冒称OS输入或原游戏动态认证 |

79参考S020四行/价格/手形/滚条/信息软键，数值只取当前Owner；72/82仍为真实数据的简化表现，装备图标/完整举物或口碑动画未交付。主角创建、纪录、人物属性头标与邻接浮标的具体剩余边界见[本轮需求](../reference/RESEARCH_REQUESTS.md#player-feedback-20261008)。

本批本地验收完成：

- desktop-debug配置/构建及标准173项通过（79.79秒），headless-debug配置/构建及标准149项通过（74.17秒）；均仅排除三个月用例，注册/参数/断言保留。
- desktop-release最终构建通过，标准三个月seed1/speed0通过（64.53秒）。先前原表路径和通用确认拒绝的两项Release失败已修复，并重新定向通过；没有因慢缩短长测。新维护页往返/损坏引用与短三进程回放由上述标准套件覆盖，旧layout的38000前缀不再作为本批兼容证据。
- Release实际窗口：79普通/最小、72信息、82两阶段共5张新页面截图；同批复验既有10张设施/道具页面。使用明确页调用点及真实初局定义，由源初始化载荷，未声称自然口碑触发或OS输入。夹具统一到生产原生framebuffer路径，绘制前后现金/随机/时钟/页计数保持。
- 25%默认世界窗口从真实新局到三人自然到访，失败标志0，扩大视野可显示村界与外部战斗道路区域；几何/鼠标锚定/人物及格子拾取由现有套件验证，无OS拖动认证。无新增字体/素材下载。
- 356项冻结来源、619项素材及新字段分类检查通过；10份受影响文档282条本地链接通过，diff检查通过。日志和窗口证据：`build/validation/progression-29f371d/`。未推送、CI待验证，未生成新ZIP。

收口期间研究发布a57958c魔法壶/村办5–6，进入下一独立批次；本批继续固定29f371d，不能以本批验收认证下一版数据/存档布局。

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
