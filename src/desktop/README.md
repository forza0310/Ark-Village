# desktop

默认 `world_view` 只负责输入/60FPS绘制，使用 `WorldSession` 的不可变快照；普通行走按快照时间插值，瞬移/进出/状态变化直接呈现，深度排序与绘制用同一位置。镜头先本地响应，再按序写回唯一世界，保留源可见性影响。

`world_overlay`定义标准C++绘制计划，`world_combat_visuals`读取累计伤害、X2/X3金币、cd24经验和cd14升级；`world_rest_visuals`按占用名单前4项/flags32生成两阶段休息条，live或retired人物引用均有效。头像采用e8f66d9的`startup_world_visuals`计划，读取当前职业/性别并裁剪walk01帧0。`world_overlay_render`才调用raylib；布局不扣款/加经验/推进计数，设施精确L锚点和完整恢复特效仍未交付。

默认`ark_village`及`--world`使用`WorldSession`模拟线程独占唯一State，`world_view`负责真实页ID输入、窗口生命周期、快照绘制与插值，不创建旧Game。`world_scene`只读当前surface、名册和元数据，按职业/性别/怪物体型绘制原动作帧，复用道路拼块、栅栏/门柱、血条和现金浮标。

- `world_management`：建设/设施/授勋/晋级及人物/税收页面的预览与FIFO接线；只保留表现状态和在途序号，拒绝反馈来自模拟线程，人物与税收列表选择由源Owner持有。
- `world_build_placement`：与场景共享相机/zoom的格拾取和完整占地红绿预览；读取当前报价与占用，最终合法性仍由worker重新验证。
- `world_menu`：五项原素材菜单，开放建设/冒险，其余三项禁用。M/菜单打开，Esc/M关闭，冻结worker且保留显式暂停；动作等待FIFO确认，菜单/待回执阻止地图输入旁路。生命周期是桌面适配，不伪造raw3。
- `ui/world_building`：21建设目录、74设施详情、80入住候选及81升级值；B打开建设，点击地图锁位、R旋转、Enter/按钮提交，成功连续放置、Esc退出。无道路建设、移动或拆除入口。
- `ui/world_tasks`：T进入任务目录/活动队伍，26/27追加并打开绑定60成员详情，X打开4中止管理，绑定1默认否并把答案交给父页消费。
- `ui/world_human`：实际人物点击/raw60概况、六属性、装备、魔法；P转职/G赠礼，61/62目录预览、63阶段、64四槽/65确认/66评价/68对比、70最终选择与73只读详情。只读page_ready、当前职业及其等级，63目标另列；不绘制初始化、随机或奖励。完整转职/评价/大师动画另接。
- `ui/world_tax`：90源序居民、当前G金额、五行滚动/合计和确认，无取消；98自动入账不画确认按钮。税收页面与左上自动月报不同。
- `ui/world_award`、`ui/world_progression`：年度按排名索引请求/确认授予或终止；48晋级/条件解释与49只读分开，50/67/88/96沿源计数开放输入，当前演出为数据/文字适配。
- `ui/world_panels`：普通页模板、源notice前两条、77皮肤/富文本与成长ap属性映射；不重复计时，32特殊条仍为文字回退。日期年/月/周及周内条读取当前calendar，比例仅为`units/10800`；原皮肤尺寸/方向未证。

`world_reports`显示任务胜利与左上月报。月报恢复源70/70自动生命周期，无确认按钮，不冻结世界或阻断正常输入；其他源模态/场景资格与玩家显式暂停照常生效。绘制不推进counter、日历或奖励，诊断skip不参与正常操作。

`world_rank`保留纯条件查询和既有契约，新的页面操作经`world_progression`向同一Owner提交。完整投射物/一般特效及其他经营/信息菜单另接。e8接线与5aa表现批次的历史验收见B1；当前8f12654人物/税收、日期和自动月报改动验收通过（结果见B1）。

实际场景视口反缩放后写入运行时，画面/可见性使用同一相机；渲染到Retina原生画布，鼠标仍用窗口点。`world-menu`经真实到访预运行和FIFO检查菜单冻结；`world-building/details/built/award-granted`分别检查真实目录、建筑详情、实际建设和授勋消费。`world-active/month/rank`等诊断从首轮使用实际宽屏视口，自动确认前序页面只是测试输入，不能替代OS鼠标验收。

以下描述`--legacy-slice`建设切片及其诊断的既有桌面模块。

正常启动显示首访后真实人物生活，场景读取Game投影的实际连续位置，HUD/人物页读取共享资金和人物HP/装备；玩家建设与人物活动使用当前世界。未知活动的局部handoff须显示原因，日期/施工仍推进，首个月报前总体保护另行显示。
--ai-preview保留为严格初局寻路/设施交互诊断，使用同一表现层；它固定布局/日历，不用于正常建设验收。
菜单/设施页按现有资格阻止Game.update，暂停/倍速/缩放仍走共享controller；建设在预览禁用。--inspect-page ai安排同一模式并固定种子用于有界截图，不注入假位置或收入。

raylib表现层依赖app只读状态、素材元数据和旁置PNG/SEB，不修改人物或设施规则。

- main.cpp：参数、无窗口资源/首访检查、窗口入口。
- game_view.cpp：Retina窗口/原生像素画布、帧调度、输入采集和相机过渡。
- scene.cpp：加载后地表/补块/设施/人物共享深度排序、连接道路、占地与建筑/箭头预览。kind5外部出口仅用当前草地及external_direction门柱，不再叠画定义里的entranceOut灰色地板；kind4城镇入口照常提交设施图元，显示过滤不改变逻辑实例或寻路。
- road_render.cpp：当前未占用道路的2×2/上下边缘标记及原PNG尺寸/锚点偏移，quad优先，第二遍按y降/x升提交。
- boundary_render.cpp：按BOUNDARY规格映射六栅栏片/三皮肤与四方向外部门柱，返回地表SEB锚点的提交偏移/深度；scene第一遍按当前有效地表及视口资格提交common图元，sprites只应用一次SEB记录偏移。
- character_animation.cpp：按获准轮次与连续位置的原始整数投影选择walk00..03四方向，每方向四帧；双轴均变才改普通朝向，单轴不变/静止保留，初始/控制转向读取当前人物，状态4/20不被普通移动覆盖。镜头/zoom/DPI不参与判向，静止回帧0、暂停冻结、重启清状态；每6移动轮次换帧仍属桌面播放策略，完整原动作时钟/武器合成另行接入。
- character_visibility.cpp：用户要求的店内暂时隐藏适配；仅真实state14、有效active_facility且该实例占用名单包含人物时不提交精灵，退出释放后恢复。普通/预览共用，显式motion检查独立显示；不是原版bit1隐藏谓词或正门动画认证。
- character_status.cpp：按[战斗显示契约](../../research/dungeon_village_1/ui/COMBAT_RENDER.md)生成只读HP条矩形计划。正常/预览读取真实HP、容量与动作，显示活动或真实人物选中时出现，动作7和店内隐藏时不画；当前UI尚无人物选中命令。人物绿条/红过渡与怪物蓝条/黄过渡共用接口，正回复不套伤害色。scene按人物锚点与zoom提交、沿用人物深度和场景裁剪；不在绘制时推进HP计数，不为检查运动或尚未创建的怪物注入数据。伤害数字、武器/完整战斗动作及原APK动态对照另验。
- ui/：共享布局、导航控制、原版皮肤、HUD和页面，详见[模块说明](ui/README.md)。
- projection.cpp：格与连续世界坐标等距投影/拾取/响应视口/鼠标锚点缩放，绘制和输入共享转换。
- resources.cpp：RAII纹理/字体、SEB图片绑定、实际绘制帧校验。
- desktop_session.cpp：macOS桌面可用性检查。

默认1080×720横屏，窗口改变时扩展逻辑视口而非固定竖屏留黑；最小逻辑240×256与60FPS是适配政策，不宣称原版分辨率/时钟。
GetTime单调时钟经app/SimulationClock才调用Game.update，默认采用原版整数47ms最小开始间隔，卡顿不补算；倍速仍由Game每次推进1/2个有资格步骤，移动仍每步6.7单位。
game_view关闭raylib的隐式帧末限速，用最早逻辑/60FPS绘制截止WaitTime；两次绘制之间也可更新，输入只在绘制分支采集一次。没有忙等或第二逻辑线程，渲染仍使用Retina原生像素。
暂停/菜单/非正常模式阻止Game但默认门槛继续运行，不累积工作；重开清时钟。`--tick-rate 1..240`仅显式固定频率实验，阻塞清积累/恢复丢弃跨阻塞间隔、每次最多8次补算，教程/研究边界出现即停止。单调时钟、绘制频率和窗口生命周期是桌面适配，实际APK帧率另验。
步态与显示朝向逐次观察获准更新，倍速拆为两次观察以保留中间转弯，多绘制帧不推进步态；motion检查使用相同逻辑时钟、暂停/倍速，但仅推进检查行程，不改变领域状态。当前生活切片尚未持有原版u/v缓存，因此显示朝向保存在CharacterAnimation，不反写AI控制状态。
`--frames`继续表示绘制帧数，同帧数不保证同逻辑次数；有界运行输出Simulation的pacing/tick_rate_override/outer_updates/elapsed_seconds/minimum_gap_ms以便核对。
启用FLAG_WINDOW_HIGHDPI；窗口点坐标决定布局/鼠标，GetRenderWidth/Height决定画布物理像素。
canvas_camera将逻辑布局直接映射到原生画布，呈现时与framebuffer像素一一对应；最近邻素材只在最终尺寸采样。
Text::prepare按物理UI倍率增长字形图集，缩放地图不重建字体；窗口缩放/跨DPI屏幕由下一帧重新计算。
地图精灵原点与地块中心分开，拾取使用原SEB的60×29中心；连续人物脚底在同一中心投影，不复用地表左上绘制锚点。普通建设模式与显式AI预览均读取真实生活位置。
场景滚轮每格约5%，范围50%～200%，鼠标下世界位置保持；右键拖动补偿缩放，HUD/面板尺寸不变。
目录和详情页滚轮继续滚动列表，不同时缩放地图；`--zoom-percent 50..200`用于复现初始视图/有界截图。
到访列表仅列场景人物，不冒充原版四页名单。inspect-page只检查渲染，不算真实鼠标测试。
`--verify-play --frames 3000 --screenshot /tmp/ark-playable.png`是正常新局的有界controller检查，固定产品随机种子但不预推进首访、不注入目标/收入；默认47ms等待真实420步，再用共享坐标控制关闭教程、建设食品类包子铺、暂停30绘制帧并恢复。
该检查观测人物移动/访问/收费/占用/退出、真实使用期间隐藏与退出再显示、施工完成和现金账本一致性；打印Village的steps/date/life_rounds/visits/completions/funds/ledger_funds/error/pending_category/actor_visible及Playability结果，不达标退出失败。约50秒绘制预算、60秒硬上限，不可与预览/检查页面/暂停启动或固定频率覆盖混用。
这是产品内部正常模型/controller检查，不代表OS鼠标事件或原APK行为等价；实际通过记录集中在阶段文档。
`--inspect-page motion --frames 120`在有界窗口显式选择旅店展示连续运动，只覆盖渲染位置；领域人物/金币不变。
它不证明默认首访会选择旅店；使用相同四向行走显示，不含武器合成或使用退出；手动新局重置会清除检查行程。
依据：[研究画面](../../research/dungeon_village_1/prototype/src/startup_view.cpp)、[UI](../../research/dungeon_village_1/ui/PAGES.md)。


道路补块按[研究绘制规则](../../research/dungeon_village_1/ui/README.md#road-patches)画整张PNG：
road4block00为30×20、相对SEB锚点(14,21)；road4block01为27×15、偏移(20,19)，均深度D.y-10。
不得叠加SEB内部偏移或菱形中心偏移。road_render从当前terrain重算，建筑覆盖任一角后不保留静态quad标志。
scene从发布mapchip列4/6读取显示深度偏移/flags，基础深度D.y+15+偏移，flags1则D.y-50+偏移。
补块和地表/物件共享稳定深度队列，人物保留当前深度适配；不无条件把补块最后叠到前景上。
当前遍历全部地图，不做独立补块可见格裁剪；栅栏/门柱按D+(0,-31)的60×60与scene相交门槛提交，所有场景图元最终在Retina物理画布按共享scene视口裁剪。未复刻原10命令满桶溢出协议，不能把填心/栅栏验收扩张为全场景原版渲染等价。
Layout.scene_clip绘制到21单位底栏边缘；Layout.scene命中仍停在29单位角按钮上方。两者不能混用，避免8单位空白带或改变旧鼠标命中。
初局boundary_index来自已校验STATE.reset.active_boundary_index=0，34片低木栅栏和两个方向2/3门柱依加载后格标记绘制；通行仍读取原边界blocked/access状态。扩张必须同步更新通行/标记，不能只切三皮肤纹理；本轮未添加扩张命令。
依据：[栅栏与入口契约](../../research/dungeon_village_1/ui/BOUNDARY.md)。原版动态遮挡/扩张及截图跨版本差异另行验收。

`world_dungeon_visuals`为无raylib的任务场景计划：读取唯一活动任务/实例g与h，返回进度条与目标菱形的独立深度层；调用方提供首占地格实际PNG高度和明确语言偏移。它不推进任务、绘制期随机或底栏队伍，不持有第二份进度。

`world_task_inspection` 只供显式 `--inspect-page world-task-team/world-task-result`：真实新局选择当前可负担的实际任务，调用原征集/出发消费者，自然运行到队伍或成果页。普通页/月报复用已有诊断确认；不写人物、任务、奖励或资金，不参与正常窗口策略。

`Sprites::map_frame`统一预览、缩略图、地表和地图图片高度的帧解释：单帧素材请求朝向1时复用帧0，其他越界明确拒绝并报告资源名/帧号。不回写世界fragment或逻辑朝向；原版单帧转向外观仍待研究。建设窗口诊断优先实际目录的募集地块，`world-built`通过真实事务确认朝向1，覆盖此前只检查旅馆遗漏的单帧路径。
