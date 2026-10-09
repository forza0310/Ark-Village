# Steam标题与选档局部绘制计划

接口为[steam_startup_skin.hpp](include/dungeon_village_prototype/steam_startup_skin.hpp)，实现为[steam_startup_skin.cpp](src/steam_startup_skin.cpp)。本模块把[Steam标题／选档合同](../ui/STEAM_TITLE_MENU.md)和[手动菜单／询问合同](../ui/STEAM_SAVE_MENU.md)中的已核局部调用组织为只读C++17计划。它与[APK启动图块桥](STARTUP_SKIN.md)分开，不创建Owner、打开文件、推进动画、读取随机或产生声音。

四个查询分别返回标题两项、选档小窗、raw20三行和raw1询问。`draws`保留图片、填充、文字和窗口helper的穿插次序，不能先画全部图片再画全部文字；`origin`是Steam游戏逻辑坐标原点。TitleForm横向按`(W-240)/2`向零截断；标题根页下SubForm只要任一维大于240，就同时加两个居中偏移，另一窄维可以产生负值（Draw包装`0x1030CEA3–0x1030CF88`）。W/H不是截图或OS客户区像素。

## 文本、图块与交互交接

- 文字计划给出语义角色／行号，实际名称、日期、资金和译文由已校验的只读文本适配器提供。窗口标题属于`SteamStartupWindow`的标题角色及参数，不生成虚假的`(0,0)`文字请求。窗口／内容框保留Steam原helper实参，不能擅自用APK木框展开替换。
- raw20文字宽取实际`StringWidth`整数；超出日文74／非日文80时只对该次文字请求字号11，之后恢复。raw1共同按钮宽取两个`StringWidthF`的最大值加2，并保留float运算及向零取整；没有用字符数估计字体。无穷、NaN、负测宽及不可安全转换的数值拒绝。
- `rgb`／`anchor`没有具名证据的文字项保持空值，表示仍须源样式适配器处理，不默认填棕色或猜anchor位值。`placement`单独保留已核居中、右对齐或矩形布局语义。这是局部绘制计划，尚不能称完整字体／文字皮肤已消费。
- `steam_startup_resource`返回真实图槽／SEB槽及原文件名。左手SEB22引用`finger_r.png`70；存读档图177保留语言变体资格，实际语言路径须由调用方明确提供，不能默认把APK图当Steam中文图。原文件沿[Steam启动清单](../assets/steam-startup/MANIFEST.json)；menu图片25及SEB0是[原公共素材](../assets/original/common/menu.png)中已经跨版核同的资源。
- 图片`frame=-1`表示外部当前SEB帧，查询不推进手形。raw20保留`imul32(frame,1000)`回卷后有符号除3的原局部缩放；不把FrameMenu更新中的夹3规则偷放进绘制查询。允许查询frame4或极值不证明这些状态在自然UI中可达。
- `touches`只是原注册请求，包含组件ID、value和已核基矩形。箭头通过`image_draw`引用对应SEB helper，保留其注册资格而不猜热区大小；raw20末尾KEYCLICK保留option2且没有臆造全屏矩形。surface仍负责margin、缩放、裁剪、重叠及事件分发。标题id0、选档／询问id3、菜单id9的ENTER／UP语义及软标签取消沿正式合同；此模块不把触摸计划变成Owner动作。

## 状态与遮挡

标题菜单和选档都保留75的绘制准入；选档空中断没有行注册且不能作为有效选择输入，Owner必须先按原输入合同纠正选择。非空与空手动均保留实际行。选档的箭头注册需要标题栈顶且focus，选中手形需要标题栈顶；菜单覆盖时保留选档，询问覆盖时整块隐藏。raw20被询问覆盖同样不输出旧菜单。调用者仍须根据已核父子栈提供真实状态，查询不自行退休页面。

资源和输出规模固定：7种资源引用；单份标题／选档／raw20／raw1计划最多分别5／17／7／6条绘制和2／4／4／2条触摸请求。计划按值返回、由调用者本轮消费并释放，不保留跨轮队列或文件缓存；没有新增PNG、声音、SEB或资源副本。真实Owner、文件目录、快照、B0／初始化G和平台按钮不属于本次增量。

## 检查归属

[steam_startup_skin_checks.cpp](tests/steam_startup_skin_checks.cpp)由既有`startup_skin_checks`调用，仍属于同一个visuals target；拆文件只为将Steam坐标／菜单合同与APK原图CPU合成职责分开，不新增CTest。检查覆盖75门槛、奇数向零截断、有效／空目录行、原裁片偏移、层遮挡、窄窗口修正、真实测宽阈值、float按钮矩形、坏输入及原SEB2/3和手形引用。没有修改原自然轨迹或声音断言。

单Release动态树构建通过，既有visuals与application两项CTest通过（合计22.18秒）。正式记录见[集中交付](../work/steam-skin-delivery/README.md)；静态计划与源资源检查不等于原窗口输入、Steam字体运行选择或完整可操作标题已经验收。
