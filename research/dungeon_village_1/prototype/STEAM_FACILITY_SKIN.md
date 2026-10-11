# Steam设施页只读皮肤计划

`steam_facility_skin.hpp/.cpp`属于现有应用库，使用标准C++17，不依赖raylib、字体对象或可写世界。入口覆盖raw81升级页和raw74普通实例两页；住宅、定义预览与装备店特殊皮肤另批。来源是[Steam74](../ui/STEAM_FACILITY_DETAIL.md)、[Steam81](../ui/STEAM_FACILITY_UPGRADE.md)及[共用helper](../ui/STEAM_FACILITY_DRAW_HELPERS.md)，不把APK同号图片当作Steam资源。

## 普通设施详情74

`steam_facility_detail_skin(state,page,options)`验证当前存活raw74、两页资格、实例/定义绑定、全部初始化Y及经济依赖后出图，缺字段显式返回空。第一页名称只读定义名；编号只拼到第二页来源名。实例四经营值、共享等级与到下级剩余人数读现有Owner，来源值读原y序，含隐藏行，不重算加成或使用道具。调用者提供语言、VIEW_Y、真实标题两次测宽、底部一次测宽、共享手形当前帧及局部选择/滚动；它们不新增Owner字段或wire格式。

`SteamFacilityDetailSkinPlan.operations`按原序保留嵌套通用图元、具名文字、SLIST触摸、标题箭头helper与滚动注册helper。第一页包含框、大图clip/Mapchip2、经营/效果/等级数字、MAX与橙底按钮；第二页包含维护费、五条来源或空提示。名称TextLayout、来源DrawString、富文本DrawString2与植物限框不能混成同一后端。`bonus_value`的argument指定标签属性，value保留字面signed串，消费者组成`<co=0065FF>译文</co>`再接value；不把值染蓝或擅自省略零。

标题箭头请求一并交给SEB型AddTouch消费者，不独立画图再重复注册。第二页原序是选中手形→行注册→图标→名称→值；滚动请求调用12后25，携带[count,5,0x20000]、Margin(3,20,0,0)及组件25的option4。后继已核12注册下层先绘轨道／滑块再登记组件，不能遗漏这个即时消费者。底部第一页为橙底→组件4→文字→手形，第二页只有说明。软标签固定[0,2]。

`steam_facility_bonus_cursor`保持上、下独立循环及五行追踪；窗口维护输入修正左右分别处理、确认优先于返回。键盘repeat、OS最终热区、完整TextLayout与全局输入声音仍不由raylib按键适配认证。详情Owner的翻页、75/79/80交接、第二页确认无效果和退休继续由building/pages套件主责；本批不重复其业务组合，也不把局部选择加入应用快照。

数值入口允许条件夹具；4096图元效果预算、65536来源/绝对索引预算和字符串4096字节是维护坏输入防护，不是原游戏内容上限。输出是短寿命值，不创建永久历史或一次性输出队列。[离线两页示例](../ui/examples/index.html)提供空、邻接和七行状态；CSS字体/尺寸是适配，第二页载荷明确为夹具，不能当原窗口或自然经营结果。

研究窗口的普通74现消费正式计划，使用已核初值VIEW_Y23和240×320画布的居中适配。PNG/SEB偏移、差异图片、数字、Mapchip2/clip、标签颜色/限框按请求执行；标题箭头复用已核GameView `buttoneffect` helper而非直画common74。选择/滚动按存活页ID保留，覆盖页返回恢复，退休页清理；行release选择后确认、翻页和底部点击仍是raylib输入适配，不宣称原Surface投递/按住repeat已验。手形明确静态SEB21帧0，不按页面frame猜原全局SEB动画。Noto正文/有限co标签及右返回框架适配仍非原字体后端；8字号中文可读性与字形警告保留，不为改善样图改变原字号。

2026-10-11[原两条来源窗口](../references/screenshots/2026-10-11-steam-facility/S070-bonus-page2.jpg)右侧蓝竖条触发补核，[GameView实际消费者](../ui/STEAM_FACILITY_DRAW_HELPERS.md#drawverticalscroll2实际是两项组件注册)及`steam_facility_scroll_skin(request,first,first_touch)`已闭合：同一请求位置展开暗轨道→蓝／触摸橙滑块，另返回原组件12基矩形。空至五行不隐藏，七行顶／底按原整数比例。窗口按本页真实first执行蓝条；首次触摸橙色只在纯计划覆盖，OS拖动仍待认证。无新Owner／格式，拒绝未知请求／越界first，输出短寿命；更新后的参考图保持实际研究输出。

`--world --inspect-page world-facility-details`与`world-facility-bonuses`取真实新局同一已营业且有邻接的设施，分别展示第一/第二页，不注入来源、资金或数值。[两页参考图](../ui/examples/steam-facility-detail.png)来自这两个有界窗口；其他特殊74模板仍用旧研究展示。普通分支不再覆盖旧通用白面板或叠画“确定”。窗口消费一次性声音请求但未播放，不据此认证原输入声音。

底层数值接口接收已初始化页面的定义／mapchip／共享等级、冻结三属性前后差与上限、phase／frame／独立frame2、语言分支、VIEW_Y及原调用对应的真实测宽。两个标题宽必填；非日文第一阶段另填四次测宽，不能用字符串长度估算。接口检查缺测量、阶段／等级／计数、差额关系和整数范围，非法输入返回空；单凭手填数值不证明真实Owner绑定。

正式Owner桥为`steam_facility_upgrade_skin(state,page,options)`：经`inspect_startup_world_facility_upgrade`核真实raw81、初始化、实例／定义绑定及完整载荷，再读取当前共享等级、原上限和冻结值；调用方只提供语言、VIEW_Y及测宽。Owner的`facility_upgrade_display`按`[前/后/差][属性]`保存，桥显式转置成数值接口的`[属性][前/后/差]`，不重新升级或从新等级倒推前值。未知、未初始化、已关闭或缺计数的页面返回空。

独立frame2复用已编码的`page_secondary_counters`，首次初始化0、实际获准页面更新递增并按INT_MAX回卷；确认快进和phase切换不改它。框架暂停保持两计数，世界逻辑暂停不能替代页面更新资格。真实退休沿既有映射清理，资源规模统计已计入该映射。无新wire字段或格式；已初始化81缺phase／任一计数的旧不完整快照明确拒绝，不猜值补档。正常稳定世界档不包含正在展示的81。

返回有序variant计划：原木窗框／内框、文本位置与颜色、clip push/pop、event两背景、设施Mapchip2请求、数字请求、MAX图、箭头、左右影子／角色，以及双空软标签和原无矩形的确认组件。顺序必须保持，不能先汇总图片再画文字；phase0的提示40可确认但50才画，phase1从55起提示，frame2不随切段归零。

数值动画保留Steam的特殊整数计数与逐步float32抛物取整。差为±1时34–48仍旧值，49才切新值；较小正差提前结束，下降不镜像上升。MAX比较当前显示值。具名资源映射明确区分Steam number05／number08与APK同名图，其余引用已有同字节出版资源，不复制图像。

页面仍保留具名数字／Mapchip2请求；后继`steam_facility_number_draws`已将普通数字、金额、加号展开为有序SEB请求，金额只在这里减9，不能在页面计划及执行器重复偏移。普通数字传入对应SEB首帧真实步宽，使用anchor位判断；金额／加号沿固定8及原逗号覆盖次序。负普通值可请求负商帧，signed串负号可请求−3；保留原请求不等于认证负帧最终像素，不把它替换成猜测的减号裁片。

`steam_facility_mapchip2_draws`已沿固定85项已核定义／mapchip／SEB映射展开分片请求，复用完整世界定义目录和原pattern几何。中心偏移仅加一次，SEB内部offset仍由末端执行；道路不套普通建设helper的frame11覆盖。查询两朝向只记录原helper请求，不能由存在请求证明该定义可旋转或末端裁片有效；已知源异常仍按[Steam图块合同](../ui/STEAM_MAPCHIP_PATTERNS.md)保留。

文字仍需真实翻译／字形／测宽后端，原OS热区与全局表现调度另验；普通74已接研究窗口，81仍保留纯计划与CPU示例。模块不宣称所有设施模板或完整原皮肤已完成。

后继[升级81 CPU示例](../ui/examples/STEAM_FACILITY_UPGRADE.md)已在现visuals可选输出中执行PNG／SEB、数字、Mapchip2及裁剪并查看六阶段原尺寸图，导出请求JSON供产品审阅。文字仅锚点标记，数值和测宽是明确夹具；这不替代真实字体后端、研究窗口接线或原游戏动态。

绘制不会重新执行升级、扣费、递增计数、抽随机、发声或退休页面；这些继续由唯一Owner负责。返回向量是本次短寿命值，不进入存档或第二输出队列。现有visuals套件扩`steam_facility_skin_checks.cpp`，覆盖帧边界、取整、层序、裁剪平衡、输入拒绝、Steam资源和重复查询规模；升级业务仍由原building/pages套件主责，不重复其整组断言。

2026-10-10：初批单Release目标构建及visuals通过0.60秒；[数字后继](../VERIFICATION.md#steam设施数字请求展开2026-10-10)构建和visuals通过0.58秒。Owner桥后继通过building、persistence与visuals三套定向检查，覆盖双计数、暂停／拒绝、精确往返、矩阵转置、只读输出及退休。原窗口逐像素、完整中文字体与74全部布局仍未由此认证，实际结果见[交付](../VERIFICATION.md)。
