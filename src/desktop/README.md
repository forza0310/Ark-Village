# desktop

raylib表现层依赖app只读状态、素材元数据和旁置PNG/SEB，不修改人物或设施规则。

- main.cpp：参数、无窗口资源/首访检查、窗口入口。
- game_view.cpp：深度排序、目录/预览/详情/到访/名单、输入和相机过渡。
- projection.cpp：等距投影/拾取/留边缩放，绘制和输入共享转换。
- resources.cpp：RAII纹理/字体、SEB图片绑定、实际绘制帧校验。
- desktop_session.cpp：macOS桌面可用性检查。

240×330画布与60FPS是适配政策，不宣称原版时钟。首访人物停在出生格，未知AI不补造。
到访列表仅列场景人物，不冒充原版四页名单。inspect-page只检查渲染，不算真实鼠标测试。
依据：[研究画面](../../research/dungeon_village_1/prototype/src/startup_view.cpp)、[UI](../../research/dungeon_village_1/ui/PAGES.md)。
