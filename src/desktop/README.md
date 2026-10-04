# desktop

默认`ark_village`即运行完整世界，`--world`保留为别名。`world_view`拥有唯一运行时State，按完整候选加渲染缓存的提交边界更新，
处理真实页ID确认、暂停/倍速和窗口生命周期；不创建旧Game，不保留bootstrap。
`world_scene`只读实际surface、名册和元数据，按职业/性别与怪物体型绘制原动作帧，复用道路拼块、栅栏/门柱、血条和现金浮标。
`world_rank`只读实际rank条件缓存和原条件表/设施名供页49显示；确认交回页栈消费者，仅返回，不晋级/收费。
当前窗口范围是查看连续世界；玩家建设、完整任务交互与投射物/一般特效另接。
实际场景视口反缩放后写入运行时投影，画面/可见性使用同一相机；渲染到Retina原生画布，鼠标仍用窗口点。
`world-active/world-month/world-rank`是显式自动确认页面的画面检查策略，预运行前即安装实际宽屏视口，不冒充OS输入。

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
