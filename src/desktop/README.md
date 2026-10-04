# desktop

--ai-preview在同一主程序显示真实初局人物寻路/设施交互，场景读取Game投影的实际连续位置，HUD/人物页读取实际会话资金/HP/装备。
菜单/设施页按现有资格阻止Game.update，暂停/倍速/缩放仍走共享controller；建设在预览禁用。--inspect-page ai安排同一模式并固定种子用于有界截图，不注入假位置或收入。

raylib表现层依赖app只读状态、素材元数据和旁置PNG/SEB，不修改人物或设施规则。

- main.cpp：参数、无窗口资源/首访检查、窗口入口。
- game_view.cpp：Retina窗口/原生像素画布、帧调度、输入采集和相机过渡。
- scene.cpp：加载后地表/补块/设施/人物共享深度排序、连接道路、占地与建筑/箭头预览。
- road_render.cpp：当前未占用道路的2×2/上下边缘标记及原PNG尺寸/锚点偏移，quad优先，第二遍按y降/x升提交。
- character_animation.cpp：按实际位移/获准轮次播放walk00四帧，静止回帧0、暂停冻结、重启清状态；每6移动轮次换帧暂属桌面播放策略，原朝向/武器合成另待研究。
- ui/：共享布局、导航控制、原版皮肤、HUD和页面，详见[模块说明](ui/README.md)。
- projection.cpp：格与连续世界坐标等距投影/拾取/响应视口/鼠标锚点缩放，绘制和输入共享转换。
- resources.cpp：RAII纹理/字体、SEB图片绑定、实际绘制帧校验。
- desktop_session.cpp：macOS桌面可用性检查。

默认1080×720横屏，窗口改变时扩展逻辑视口而非固定竖屏留黑；最小逻辑240×256与60FPS是适配政策，不宣称原版分辨率/时钟。
GetTime单调时钟经app/SimulationClock才调用Game.update，默认采用原版整数47ms最小开始间隔，卡顿不补算；倍速仍由Game每次推进1/2个有资格步骤，移动仍每步6.7单位。
game_view关闭raylib的隐式帧末限速，用最早逻辑/60FPS绘制截止WaitTime；两次绘制之间也可更新，输入只在绘制分支采集一次。没有忙等或第二逻辑线程，渲染仍使用Retina原生像素。
暂停/菜单/非正常模式阻止Game但默认门槛继续运行，不累积工作；重开清时钟。`--tick-rate 1..240`仅显式固定频率实验，阻塞清积累/恢复丢弃跨阻塞间隔、每次最多8次补算，教程/研究边界出现即停止。单调时钟、绘制频率和窗口生命周期是桌面适配，实际APK帧率另验。
步态逐次观察获准更新，多绘制帧不推进步态；motion检查使用相同逻辑时钟、暂停/倍速，但仅推进检查行程，不改变领域状态。
`--frames`继续表示绘制帧数，同帧数不保证同逻辑次数；有界运行输出Simulation的pacing/tick_rate_override/outer_updates/elapsed_seconds/minimum_gap_ms以便核对。
启用FLAG_WINDOW_HIGHDPI；窗口点坐标决定布局/鼠标，GetRenderWidth/Height决定画布物理像素。
canvas_camera将逻辑布局直接映射到原生画布，呈现时与framebuffer像素一一对应；最近邻素材只在最终尺寸采样。
Text::prepare按物理UI倍率增长字形图集，缩放地图不重建字体；窗口缩放/跨DPI屏幕由下一帧重新计算。
地图精灵原点与地块中心分开，拾取使用原SEB的60×29中心；连续人物脚底在同一中心投影，不复用地表左上绘制锚点。普通建设模式首访静止，显式AI预览按真实位置运动。
场景滚轮每格约5%，范围50%～200%，鼠标下世界位置保持；右键拖动补偿缩放，HUD/面板尺寸不变。
目录和详情页滚轮继续滚动列表，不同时缩放地图；`--zoom-percent 50..200`用于复现初始视图/有界截图。
到访列表仅列场景人物，不冒充原版四页名单。inspect-page只检查渲染，不算真实鼠标测试。
`--inspect-page motion --frames 120`在有界窗口显式选择旅店展示连续运动，只覆盖渲染位置；领域人物/金币不变。
它不证明默认首访会选择旅店，不含方向动画/武器合成或使用退出；手动新局重置会清除检查行程。
依据：[研究画面](../../research/dungeon_village_1/prototype/src/startup_view.cpp)、[UI](../../research/dungeon_village_1/ui/PAGES.md)。


道路补块按[研究绘制规则](../../research/dungeon_village_1/ui/README.md#road-patches)画整张PNG：
road4block00为30×20、相对SEB锚点(14,21)；road4block01为27×15、偏移(20,19)，均深度D.y-10。
不得叠加SEB内部偏移或菱形中心偏移。road_render从当前terrain重算，建筑覆盖任一角后不保留静态quad标志。
scene从发布mapchip列4/6读取显示深度偏移/flags，基础深度D.y+15+偏移，flags1则D.y-50+偏移。
补块和地表/物件共享稳定深度队列，人物保留当前深度适配；不无条件把补块最后叠到前景上。
当前绘制全部地图，不做独立补块裁剪；未复刻原10命令满桶溢出协议，不能把本批填心验收扩张为全场景原版渲染等价。
