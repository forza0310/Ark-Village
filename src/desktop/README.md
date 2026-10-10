# desktop

当前目录按职责分为 `application/`（标题和窗口协调）、`input/`（经营/存档控制与指针手势）、`scene/`（投影、深度绘制、拾取与放置预览）、`resources/`（精灵/字体/校验）、`platform/`（窗口平台与音频设备）、`inspection/`（显式窗口验收）。`ui/` 再按 facilities、actors、tasks、village、system、common 分组。

无 raylib 的战斗/休息/任务表现计划与脚本文字已移到 [presentation](../presentation/README.md)。建筑页面拆为 `world_building_view.cpp`、`world_building_input.cpp`、`world_building_render.cpp`；资源职责见 [resources](resources/README.md)。`world_view` 是窗口协调器，经营规则、AI、战斗和世界提交均由 simulation/app 承担。

普通无界启动先由`world_title`显示S038–S040标题/两栏手动目录/继续与新游戏；它在worker创建前运行，没有世界更新。继续重新读取玩家档、候选校验后交给唯一WorldSession，失败保留初始Owner和标题。新游戏先配置定义0主角，取消保留草稿，开始只写系统栏位纪录而不写/删除世界旧档；标题和系统菜单开放两页纪录，删除禁用。标题图片从产品assets/title读取，尺寸/PC布局不冒充Steam动画或原触控坐标。

e73bb31静态背景、顶部与草边改用已发布Steam三PNG，共215618字节；原Logo、两栏手动档及静态主角草稿预览保持。完整标题20槽已在维护层，但正常桌面宿主准入/随机交接未闭合，不以60FPS驱动。玩家schema4本批数据身份更新，旧身份档显示明确拒绝，不静默补任务池或迁移。收支只读查询已在runtime，信息页面入口仍禁用。

`world_canvas`复用原生framebuffer的RAII分配/缩放，标题与世界共用；图片和延迟文字在同一原生相机中合成后再输出窗口，鼠标仍按窗口点转换，避免Windows DPI导致黑边和文字错位。

默认 `world_view` 负责窗口协调、输入/60FPS绘制，使用 `WorldSession` 的不可变快照；普通行走按快照时间插值，瞬移/进出/状态变化直接呈现，深度排序与绘制用同一位置。镜头先本地响应，再按序写回唯一世界，保留源可见性影响。

玩家窗口固定一倍速，底栏倍速按钮与旧切片Tab切换已移除；旧档加载沿用当前会话速度。`world-active`可验正常运行，旧`world-speed`参数明确拒绝。下方旧切片速度组合说明仅为领域/测试适配历史，不再是玩家操作。

建设目录、设施详情、商会/强化中的建筑图统一经`world_build_graphic`转换维护`startup_world_building_draws`的分片和偏移；候选闪烁/光标范围由`startup_world_building_preview_draws`核准。预览定义须与Owner当前选择一致，审批仍检查完整占地/报价；绘制只转换raylib坐标，不重新实现分片表。显式表现请求接口目前仅由维护回放调用，普通桌面不在60FPS循环中抽随机或清任务。

`ui/world_building`的设施74使用S019/用户第二页参考的紧凑两页布局：标题字段位于木纹行，原number08/number05数字、arrow02金色左右帧、独立屏幕返回与页内居中动作。第二页消费72a5bf4已验证来源行，保留实例后缀/重复项/原序，显示类别9图标和各项加成；第一页显示类别9/7与原数量加号。长文本按实测字体缩放，保持224×172与五行滚动，不从旧Game或累计值补造。cd13与购买举物按record_index合并，文字在场景相机内即时绘制，保持背景→文字→数字及人物间遮挡顺序。头标使用Noto size16实测数字6px，与原加号测宽公式匹配；文字和计划测宽必须使用相同字号。

设施74底栏由绑定实例提供名称和kind3/9累计净收益（0..当前月），定义预览不造收益；number05/number12区分正负，负数显示幅度，长数字按可用宽度缩放。详情页隐藏人气扇面及暂停/菜单鼠标按钮，`world_hud_buttons_visible`同步绘制与鼠标准入；Space仍走原手动暂停，返回仍按绑定页提交关闭。

`world_overlay`定义标准C++绘制计划，`world_combat_visuals`读取累计伤害、X2/X3金币、cd24经验和cd14升级；`world_rest_visuals`按占用名单前4项/flags32生成两阶段休息条，live或retired人物引用均有效。头像采用e8f66d9的`startup_world_visuals`计划，读取当前职业/性别并裁剪walk01帧0。`world_overlay_render`才调用raylib；布局不扣款/加经验/推进计数，设施精确L锚点和完整恢复特效仍未交付。

经验期间共享定义P真时，cd24自己的计数按12周期/6半周期选择ef_lvUp两帧，55后与经验条继续共存、72结束；不使用FPS或真正cd14时钟，不触发升级/HP恢复。普通身体锚点沿现有适配，弹跳/完整bl及金币X4精确运动缺口另列研究需求。0a5b5e2的建筑队首kind1–6以实例f()原投影绘制升降提示；world_overlay_render执行指定SEB层和PNG override，只应用一次各级偏移。cd15/21/22购买后举物与身体动作9共用原逻辑计数，不在绘制时装配。

默认`ark_village`及`--world`使用`WorldSession`模拟线程独占唯一State，`world_view`负责真实页ID输入、窗口生命周期、快照绘制与插值。旧Game切片、AI预览及其专属controller/演示动画已退役。`world_scene`只读当前surface、名册和元数据，按职业/性别/怪物体型绘制原动作帧，复用道路拼块、栅栏/门柱、血条和现金浮标。

`main`的`--check`保留真实资源校验及完整世界初始化检查。`resources`读取正式`startup_evidence`和旁置PNG/SEB，不依赖旧数据生成器；`projection`只保留画布/视口/DPI转换，世界格坐标与缩放由`world_scene`/`world_build_placement`负责。`road_render`读取Owner已有道路标记并返回原PNG尺寸/偏移，`boundary_render`读取当前surface标记返回围栏/门柱片与深度；两者均由实际场景调用并保留独立素材oracle。`character_status`只消费当前规则HP值生成矩形，不持有或更新人物。

默认1080×720、最小逻辑240×256，60FPS绘制与模拟线程47ms门槛分离。地图缩放25%～200%，左键拖动，短点松开后选择，右键打开菜单／返回；目录和详情滚轮只滚动列表。`--frames`和`--inspect-page world-*`为有界诊断，实际窗口、程序化输入与原版动态一致性分别验收。纹理使用point采样，文字按物理倍率准备图集；画布使用framebuffer尺寸而鼠标使用窗口点坐标。

- `world_inspection`：显式有界诊断的自然预运行、隔离存档FIFO检查与非空截图导出；不参与普通玩家的世界更新。suite继续复用一次自然检查点，每张截图使用独立分支。
- `world_render_statistics`：仅`--frames`有界诊断保留热身后逐帧样本，样本数受帧预算限制，退出原地排序一次；普通试玩关闭采样，不累积渲染历史。
- `ui/world_panels`集中只读HUD和通用页面文本，输入分页与绘制共用同一文本投影；`world_view`传入当前页和菜单门槛，继续负责窗口、FIFO和失焦处理，不由绘制查询修改页面。
- `world_management`：建设/设施/授勋/晋级及人物/税收页面的预览与FIFO接线；只保留表现状态和在途序号，拒绝反馈来自模拟线程，人物与税收列表选择由源Owner持有。
- `world_build_placement`：与场景共享相机/zoom的格拾取和完整占地红绿预览；读取当前报价与占用，最终合法性仍由worker重新验证。
- `world_menu`：五项原素材菜单，开放建设/冒险/村办/系统，情报禁用。M/菜单打开，Esc/M关闭，冻结worker且保留显式暂停；动作等待FIFO确认，菜单/待回执阻止地图输入旁路。生命周期是桌面适配，不伪造raw3。
- `ui/world_building`：21建设目录、74设施详情、80入住候选及81升级值；B打开建设，点击地图锁位、R旋转、Enter/按钮提交，成功连续放置、Esc退出。道路/移动/撤除入口由world_editing接线，住宅H资格与金币分开。
- `ui/world_facility_catalog`：79四行商品/两页、72装备信息、82设施口碑；读Owner选择与两段计数，专用FIFO动作提交清提示/返回/延迟人气，不从绘制初始化或购买。79/72商品图标使用已发布18×18映射，武器商品图标与地图PNG索引独立；完整口碑动画另待精确合同。
- `ui/world_magic_pot`：41–47的菜单、投入、处理、发现与开发；复用五行经营布局，按源门槛提供确认/返回，45不提供返回。规则/配方/库存/评语均由Owner持有；没有绘制线程产出或自动炼制。
- `ui/world_tasks`：T进入任务目录/活动队伍，26/27追加并打开绑定60成员详情，X打开4中止管理，绑定1默认否并把答案交给父页消费。
- `ui/world_human`：实际人物点击/raw60概况、六属性、装备、魔法；P转职/G赠礼，61/62目录预览、63阶段、64四槽/65确认/66评价/68对比、70最终选择与73只读详情。只读page_ready、当前职业及其等级，63目标另列；不绘制初始化、随机或奖励。完整转职/评价/大师动画另接。
- `ui/world_tax`：90源序居民、当前G金额、五行滚动/合计和确认，无取消；98自动入账不画确认按钮。税收页面与左上自动月报不同。
- `ui/world_award`、`ui/world_progression`：年度按排名索引请求/确认授予或终止；48晋级/条件解释与49只读分开，50/67/88/96沿源计数开放输入，当前演出为数据/文字适配。
- `ui/world_panels`：普通页模板、源notice前两条、77皮肤/富文本与成长ap属性映射；不重复计时，32特殊条仍为文字回退。日期年/月/周及周内条读取当前calendar，比例仅为`units/10800`；原皮肤尺寸/方向未证。

`world_reports`显示任务胜利与左上月报。月报恢复源70/70自动生命周期，无确认按钮，不冻结世界或阻断正常输入；其他源模态/场景资格与玩家显式暂停照常生效。绘制不推进counter、日历或奖励，诊断skip不参与正常操作。

`world_rank`保留纯条件查询和既有契约，新的页面操作经`world_progression`向同一Owner提交。完整投射物/一般特效及其他经营/信息菜单另接。e8接线与5aa表现批次的历史验收见B1；当前8f12654人物/税收、日期和自动月报改动验收通过（结果见B1）。

实际场景视口反缩放后写入运行时，画面/可见性使用同一相机；渲染到Retina原生画布，鼠标仍用窗口点。`world-menu`经真实到访预运行和FIFO检查菜单冻结；`world-building/details/built/award-granted`分别检查真实目录、建筑详情、实际建设和授勋消费。`world-active/month/rank`等诊断从首轮使用实际宽屏视口，自动确认前序页面只是测试输入，不能替代OS鼠标验收。

普通世界点击使用`SpritePickMap`记录上一实际呈现帧的图块、翻转、裁剪及绘制顺序，设施屋顶和多格分片都归属实例；角色/怪物/前景建筑可遮挡，透明留白不截获。复用已加载纹理，仅点击时以同一相机/裁剪离屏绘制身份并读回，不逐帧读GPU或重复解码PNG；地图格只用于建设/编辑选址。快照换代清命中，FIFO重验身份；浮动文字/状态效果不扩大实体热区。这是用户确认的PC交互适配。

`world_audio`在主线程持有raylib音频设备、4条流和22个音效资源，标题到经营共用其RAII生命周期。`world_audio_policy`只消费带operation的源请求，分开同曲幂等、普通重播与jingle静音/恢复，设备缺失明确诊断并继续游戏。WorldSession单次FIFO独立于快照，只有成功提交的输出可领取；系统写入失败待重试的输出不提前播放。PC明确适配为失焦/游戏暂停不停声、完成后重启BGM、jingle停止后以独立47ms门槛无补算检查21次恢复；此设备调度未认证为Steam循环/生命周期等价，APK音频字节也不宣称等于Steam资源。默认两个通道均为全音量，尚无声音设置页；设备/播放位置不持久化。

`world_dungeon_visuals`为无raylib的任务场景计划：读取唯一活动任务/实例g与h，返回进度条与目标菱形的独立深度层；调用方提供首占地格实际PNG高度和明确语言偏移。它不推进任务、绘制期随机或底栏队伍，不持有第二份进度。

`world_task_inspection` 只供显式 `--inspect-page world-task-team/world-task-result`：真实新局选择当前可负担的实际任务，调用原征集/出发消费者，自然运行到队伍或成果页。普通页/月报复用已有诊断确认；不写人物、任务、奖励或资金，不参与正常窗口策略。

`Sprites::map_frame`统一预览、缩略图、地表和地图图片高度的帧解释：沿2b479f6合同保留非负逻辑朝向，缺失帧为空绘而不回退frame0；负帧明确拒绝。不回写世界fragment或逻辑朝向。建设窗口诊断优先实际目录的募集地块，`world-built`通过真实事务确认朝向1，覆盖此前只检查旅馆遗漏的单帧路径。

公共桌面字形产物仅在 `build/shared-libraries/desktop-generated/` 生成一次：`desktop_glyphs.json` 记录需求/来源，`desktop_glyphs.hpp` 提供运行字符串。公共库的 `ark_desktop_glyphs` 每次构建扫描当前源码和产品目录，内容相同不改文件时间；四套消费者不复制清单。先配置、构建 `shared-libraries`，再配置桌面消费者；缺生成头/JSON会明确拒绝并提示公共构建入口。核心规则/运行时及headless消费者不包含这个桌面头。`--font`、ASCII、显式额外字形、缺字报错和按密度扩图集政策保留。

`ui/world_startup`执行启动图块桥，分开title/event/common索引与阶段/页面计数。raw17读WorldFrame.system.clear，确认由WorldSession专用计分事务消费；系统写失败显示重试，所有世界输入冻结。`ui/skin`在源支持的宽高范围消费89f157c公共窗框/标题裁片及内容框双线/四角计划，宽PC窗口与彩色选择行保留桌面适配。标题20槽维护控制器已交付，正常桌面宿主动态及纪录装饰人物仍未接线，不以60FPS补造随机或动画。Text按需增加自定义名字的字形，分发字体保留固定Noto cmap并剥离无用表；不支持的字符明确拒绝。
