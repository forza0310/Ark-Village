# Steam设施页只读皮肤计划

`steam_facility_skin.hpp/.cpp`属于现有应用库，使用标准C++17，不依赖raylib、字体对象或可写世界。当前入口仅覆盖已核raw81升级页；74接线方案见[后继边界](../verification/facility-skin-integration/README.md)。来源是[Steam81合同](../ui/STEAM_FACILITY_UPGRADE.md)及[共用helper](../ui/STEAM_FACILITY_DRAW_HELPERS.md)，不把APK同号图片当作Steam资源。

底层数值接口接收已初始化页面的定义／mapchip／共享等级、冻结三属性前后差与上限、phase／frame／独立frame2、语言分支、VIEW_Y及原调用对应的真实测宽。两个标题宽必填；非日文第一阶段另填四次测宽，不能用字符串长度估算。接口检查缺测量、阶段／等级／计数、差额关系和整数范围，非法输入返回空；单凭手填数值不证明真实Owner绑定。

正式Owner桥为`steam_facility_upgrade_skin(state,page,options)`：经`inspect_startup_world_facility_upgrade`核真实raw81、初始化、实例／定义绑定及完整载荷，再读取当前共享等级、原上限和冻结值；调用方只提供语言、VIEW_Y及测宽。Owner的`facility_upgrade_display`按`[前/后/差][属性]`保存，桥显式转置成数值接口的`[属性][前/后/差]`，不重新升级或从新等级倒推前值。未知、未初始化、已关闭或缺计数的页面返回空。

独立frame2复用已编码的`page_secondary_counters`，首次初始化0、实际获准页面更新递增并按INT_MAX回卷；确认快进和phase切换不改它。框架暂停保持两计数，世界逻辑暂停不能替代页面更新资格。真实退休沿既有映射清理，资源规模统计已计入该映射。无新wire字段或格式；已初始化81缺phase／任一计数的旧不完整快照明确拒绝，不猜值补档。正常稳定世界档不包含正在展示的81。

返回有序variant计划：原木窗框／内框、文本位置与颜色、clip push/pop、event两背景、设施Mapchip2请求、数字请求、MAX图、箭头、左右影子／角色，以及双空软标签和原无矩形的确认组件。顺序必须保持，不能先汇总图片再画文字；phase0的提示40可确认但50才画，phase1从55起提示，frame2不随切段归零。

数值动画保留Steam的特殊整数计数与逐步float32抛物取整。差为±1时34–48仍旧值，49才切新值；较小正差提前结束，下降不镜像上升。MAX比较当前显示值。具名资源映射明确区分Steam number05／number08与APK同名图，其余引用已有同字节出版资源，不复制图像。

页面仍保留具名数字／Mapchip2请求；后继`steam_facility_number_draws`已将普通数字、金额、加号展开为有序SEB请求，金额只在这里减9，不能在页面计划及执行器重复偏移。普通数字传入对应SEB首帧真实步宽，使用anchor位判断；金额／加号沿固定8及原逗号覆盖次序。负普通值可请求负商帧，signed串负号可请求−3；保留原请求不等于认证负帧最终像素，不把它替换成猜测的减号裁片。

`steam_facility_mapchip2_draws`已沿固定85项已核定义／mapchip／SEB映射展开分片请求，复用完整世界定义目录和原pattern几何。中心偏移仅加一次，SEB内部offset仍由末端执行；道路不套普通建设helper的frame11覆盖。查询两朝向只记录原helper请求，不能由存在请求证明该定义可旋转或末端裁片有效；已知源异常仍按[Steam图块合同](../ui/STEAM_MAPCHIP_PATTERNS.md)保留。

文字仍需真实翻译／字形／测宽后端，SEB请求仍需最终原图绑定、裁剪和绘制。此模块不是完整像素渲染器，没有接入研究窗口或认证OS热区，不宣称完整原皮肤已完成。

后继[升级81 CPU示例](../ui/examples/STEAM_FACILITY_UPGRADE.md)已在现visuals可选输出中执行PNG／SEB、数字、Mapchip2及裁剪并查看六阶段原尺寸图，导出请求JSON供产品审阅。文字仅锚点标记，数值和测宽是明确夹具；这不替代真实字体后端、研究窗口接线或原游戏动态。

绘制不会重新执行升级、扣费、递增计数、抽随机、发声或退休页面；这些继续由唯一Owner负责。返回向量是本次短寿命值，不进入存档或第二输出队列。现有visuals套件扩`steam_facility_skin_checks.cpp`，覆盖帧边界、取整、层序、裁剪平衡、输入拒绝、Steam资源和重复查询规模；升级业务仍由原building/pages套件主责，不重复其整组断言。

2026-10-10：初批单Release目标构建及visuals通过0.60秒；[数字后继](../VERIFICATION.md#steam设施数字请求展开2026-10-10)构建和visuals通过0.58秒。Owner桥后继通过building、persistence与visuals三套定向检查，覆盖双计数、暂停／拒绝、精确往返、矩阵转置、只读输出及退休。原窗口逐像素、完整中文字体与74全部布局仍未由此认证，实际结果见[交付](../VERIFICATION.md)。
