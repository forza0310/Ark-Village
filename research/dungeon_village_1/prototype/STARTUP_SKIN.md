# 启动皮肤图块桥

本模块把[固定APK启动绘制合同](../ui/STARTUP_SKIN.md)中的图片请求转为只读C++17计划，随现有`dungeon_village_startup_application`库交付。接口见[startup_skin.hpp](include/dungeon_village_prototype/startup_skin.hpp)，实现见[startup_skin.cpp](src/startup_skin.cpp)。它不持有Owner、资源缓存、窗口、文件或随机状态。

## 消费方式

`startup_skin_image`区分title、event、common包身份，返回标题前五图和纪录／新局／计分背景。标题五图的offset为零，只用于资源目录；实际标题位置、Logo滑入和人物遮挡见源合同，不能按目录顺序拼完整标题。纪录与新局共用event9，但分别使用整图和(0,9,180,63)裁片，页面锚点也不同。

`startup_clear_skin(stage, stage_counter, page_counter)`返回计分背景、按原序排列的右大臣／左秘书及可选继续箭头。阶段计数与页面计数不可混用：stage4按页面计数四步切换角色；继续箭头只在0／3／6阶段恰满门槛且页面相位大于20时请求。非法阶段、负计数和超阶段门槛显式返回空结果；页面计数INT_MAX合法取模。

`sprite<0`时直接裁PNG；否则按指定sprite／frame／layer解析原SEB，并在外部锚点上**只加一次**SEB偏移。角色的(-7,-22)是脚底偏移；箭头frame0只占14×5原图的左侧7×5。大臣SEB历史10／11帧越界不属于raw17请求，不能放宽素材校验让其通过。

绘制顺序需由调用方维持背景→内框→右大臣／左秘书→阶段文字／提示→累计与最高分的源层关系；背景、角色、标记分开返回，便于插入尚未实现的窗口和文字。stage6满门槛是否被框架绘制、stage7何时退休仍由原调度资格决定，局部查询不承诺这些状态可见。

## 验证与边界

后继新增`startup_window_skin`和`startup_content_skin`，分别返回标题窗的边线／木纹块／标题带／测宽后的文字锚点，以及标准内容框的填充／双线／四角。所有矩形采用半开像素大小；outline为内侧1像素线，源o.d的差1转换只做一次。字体测宽由真实平台适配器传入，本模块不补近似字体。详细原尺寸和使用顺序见[图元合同](../verification/startup-frame-font-research/README.md)。

扩展既有startup_world_visuals套件，[检查文件](tests/startup_skin_checks.cpp)用独立资源身份、哈希、尺寸、SEB记录和边界输入验证，不新增target。标准运行只读CPU图像并释放；可选第二参数目前导出760×930素材／窗框拼图（前批600×660原件保留），拼图坐标是展示布局，不是游戏UI坐标：

```powershell
research/dungeon_village_1/work/release/bin/dungeon_village_startup_world_visuals_tests.exe research/dungeon_village_1/assets/original research/dungeon_village_1/work/startup-frame-font-delivery/frame-pieces.png
```

后继[人物基础图层](../ui/TITLE_ACTOR_SKIN.md)新增`startup_title_actor_skin`：显式职业／性别／主武器及步帧／朝向，返回可选阴影、可选武器和身体。顺序为shadow→weapon→body，身体仍沿human SEB适配器；不创建临时世界人物或修改Owner。当前可选CPU图扩为760×1230，[实际合成](../verification/title-actor-skin/actor-pieces.png)末四行展示四武器风格、两方向、四步；这不是原窗口截图。

来源摘要见[静态证据](../verification/startup-skin-contract/EVIDENCE.json)，本批结果见[验证入口](../VERIFICATION.md)。原PNG没有复制或修改；图片桥与CPU拼图不是完整原皮肤／窗口验收。Steam实际资源／布局与字体已有具名局部合同，完整菜单／动态仍独立研究；应用快照和标题背景Owner已交付，不能再整体标为未实施。产品迁入时应登记固定研究提交及素材来源，运行不读取research/work。
