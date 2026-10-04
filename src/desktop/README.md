# desktop

raylib表现层依赖app只读状态、素材元数据和旁置PNG/SEB，不修改人物或设施规则。

- main.cpp：参数、无窗口资源/首访检查、窗口入口。
- game_view.cpp：Retina窗口/原生像素画布、帧调度、输入采集和相机过渡。
- scene.cpp：加载后地表/补块/设施/人物共享深度排序、连接道路、占地与建筑/箭头预览。
- road_render.cpp：当前未占用道路的2×2/上下边缘标记及原PNG尺寸/锚点偏移，quad优先，第二遍按y降/x升提交。
- ui/：共享布局、导航控制、原版皮肤、HUD和页面，详见[模块说明](ui/README.md)。
- projection.cpp：格与连续世界坐标等距投影/拾取/响应视口/鼠标锚点缩放，绘制和输入共享转换。
- resources.cpp：RAII纹理/字体、SEB图片绑定、实际绘制帧校验。
- desktop_session.cpp：macOS桌面可用性检查。

默认1080×720横屏，窗口改变时扩展逻辑视口而非固定竖屏留黑；最小逻辑240×256与60FPS是适配政策，不宣称原版分辨率/时钟。
启用FLAG_WINDOW_HIGHDPI；窗口点坐标决定布局/鼠标，GetRenderWidth/Height决定画布物理像素。
canvas_camera将逻辑布局直接映射到原生画布，呈现时与framebuffer像素一一对应；最近邻素材只在最终尺寸采样。
Text::prepare按物理UI倍率增长字形图集，缩放地图不重建字体；窗口缩放/跨DPI屏幕由下一帧重新计算。
地图精灵原点与地块中心分开，拾取/轮廓使用原SEB的60×29中心。首访人物停在出生格，未知AI不补造。
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
