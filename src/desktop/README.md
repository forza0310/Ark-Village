# desktop

raylib表现层依赖app只读状态、素材元数据和旁置PNG/SEB，不修改人物或设施规则。

- main.cpp：参数、无窗口资源/首访检查、窗口入口。
- game_view.cpp：窗口/可变画布、帧调度、输入采集和相机过渡。
- scene.cpp：源格/设施/人物深度排序、连接道路、占地与建筑/箭头预览。
- ui/：共享布局、导航控制、原版皮肤、HUD和页面，详见[模块说明](ui/README.md)。
- projection.cpp：等距投影/拾取/响应视口/鼠标锚点缩放，绘制和输入共享转换。
- resources.cpp：RAII纹理/字体、SEB图片绑定、实际绘制帧校验。
- desktop_session.cpp：macOS桌面可用性检查。

默认960×512横屏，窗口改变时扩展逻辑视口而非固定竖屏留黑；最小逻辑240×256与60FPS是适配政策，不宣称原版分辨率/时钟。
地图精灵原点与地块中心分开，拾取/轮廓使用原SEB的60×29中心。首访人物停在出生格，未知AI不补造。
场景滚轮每格约5%，范围50%～200%，鼠标下世界位置保持；右键拖动补偿缩放，HUD/面板尺寸不变。
目录和详情页滚轮继续滚动列表，不同时缩放地图；`--zoom-percent 50..200`用于复现初始视图/有界截图。
到访列表仅列场景人物，不冒充原版四页名单。inspect-page只检查渲染，不算真实鼠标测试。
依据：[研究画面](../../research/dungeon_village_1/prototype/src/startup_view.cpp)、[UI](../../research/dungeon_village_1/ui/PAGES.md)。
