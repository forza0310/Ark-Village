# Steam 2.56 UI／操作覆盖清单

2026-10-13：48—54与95补[具名局部皮肤／输入](PAGES.md#steam-village-progress)及维护只读计划；保留原窗口、字体与OS事件缺口。功能短测与条件示例不计为自然晋级或全解锁验收，具体结果仍查[验证](../VERIFICATION.md)。

2026-10-10更新：按正式合同与维护接口修正已闭合条目；静态消费者、维护绘制计划与原窗口局部分列。信息9／34–39、人物60基础表现及设施81不再笼统标“仅声明”，未证字体、物理输入及附加效果仍保留缺口。

2026-10-10[建设资源装载与旋转](STEAM_BUILD_RESOURCE_INSTALL.md)补地图manager安装／退休、image来源与flags32软标签；[两张差异素材](../assets/steam-build-common/README.md)已正式发布。静态素材范围不替代列表Init／Update、全语言覆盖或原窗口输入。

2026-10-08。按本轮已确认范围，以Steam EXE版UI与操作为还原目标，固定汉化重签APK1.0.8暂保留规则基线。
本表列研究证据分母，不代表产品已实现，也不把规则已维护当作Steam页面已认证。
证据来自固定metadata、真实GameAssembly.dll局部消费者及冻结窗口报告；维护代码只证明其已接范围，不替代原程序动态观察。

后继具体消费者见[属性头标](ATTRIBUTE_GAIN_RENDER.md)与[普通道具图标](ITEM_ICON_RENDER.md)：cd13绘制链及type1分类／裁图已局部核对，数字加号定位与APK不同。页面64／75／84直接调用和原窗口仍未认证，以下页面分母不因此整体升级。
[建设目录21专题](STEAM_BUILD_LIST.md)已闭合局部绘制、marker16与滚动；[业务合同](STEAM_BUILD_LIST_BUSINESS.md)补Init／Update三目录、空行拒绝及BUILD载荷，[图块pattern全集](STEAM_MAPCHIP_PATTERNS.md)已核85定义及两数组。完整父栈、最终OS输入与落点后续仍待证。[公共窗框](STEAM_WINDOW_FRAME.md)是Steam自己的helper合同，不以APK同图替代。

2026-10-09外部原窗口反馈已补[启动局部动态](../verification/startup-ui-analysis/ANALYSIS.md)：标题纪录单击进入、两页右向循环／返回保留选择、2/2空栏进入新局配置、默认性别联动及输入面板。内部页号未由窗口读取；自动输入未实际改名、配置取消未完成、计分页未取得，不能将关联TYPE声明整体升级为完整认证。

## 分母与证据分层

DLL与metadata身份沿用[Steam交互合同](STEAM_INTERACTIONS.md#来源与证据等级)。分母来自已核metadata声明，行为证据入口为各行正式合同；本地过程清单不作为阅读或维护本表的必要输入。

| 分母 | 声明数 | 含义／限制 |
| --- | ---: | --- |
| SubForm.TYPE_* | 101（0–100） | 页面／演出／相机辅助类型；声明不证明当前版本可达，也不等于101个独立视觉窗口 |
| GameForm.STATE_* | 8（0–7） | 主世界、建设、追踪等场景状态；与SubForm页号分开 |
| TitleForm.STATE_* | 2（0–1） | 标题菜单／存档栏；与手动栏子菜单20分开 |
| GameView.ID_* | 26（0–25） | 触控组件类别；共享消费者，未证明每项实际注册 |
| TouchEvent.TYPE_* | 14（0–13） | Steam适配层事件类别，不是Android原始MotionEvent常量 |
| TouchOption.FLAG_* | 14（1–8192位） | 命中／标记／取消／优先级标志；声明不证明全部组合 |
| form.*类型 | 9 | 包括FormManager及MyFormBase，不能当9个可见界面 |

**声明**只认metadata名称和值；**静态局部**认实际消费者的指定分支；**动态局部**认冻结截图／动作链。
任一层都不自动升级为完整页面、全输入模式或全部状态组合。既有19、24、5方法批与本批19方法有重叠，不相加冒充独立覆盖率。

## 101项页面分母

每项有独立Steam声明。“仅声明”不表示没有APK合同或维护消费者，只表示本清单没有Steam行为认证。
101项声明分母保留，不因过程清单清理而缩减；数值相同不能从APK自动填上Steam布局或操作结论。

| 页值 | Steam TYPE名称 | 功能组 | 当前Steam证据／缺口 |
| --- | --- | --- | --- |
| 0 | TALK | 对话 | 自然对话截图只证角色，未绑定TYPE |
| 1 | DIALOG | 对话 | raw20重开／删除询问的默认否、横向输入、父页结果及局部绘制已核，见[合同](STEAM_SAVE_MENU.md)；其它调用者／原窗口组合仍待逐项 |
| 2 | DIALOG_CHARA | 对话 | 仅声明；更新／绘制／动态链待逐项 |
| 3 | MAIN_MENU | 菜单 | 静态：缓存重入／目录／NEW／父位置／type0计划已核；维护：真实Owner与任务NEW／建设缓存已接。原字体、整栈输入和虚拟光标仍缺，见[菜单合同](PAGES.md#steam主菜单缓存输入与局部皮肤) |
| 4 | ADVENTURE_MENU | 菜单 | 静态及维护：可变type1菜单、真实任务入口与父子返回已交付；原OS投递另验，见[菜单合同](PAGES.md) |
| 5 | DEVELOP_MENU | 菜单 | 仅声明；更新／绘制／动态链待逐项 |
| 6 | SHOPPING_MENU | 菜单 | 仅声明；更新／绘制／动态链待逐项 |
| 7 | ACTION_MENU | 菜单 | 静态及维护：可变type1目录、真实Owner及活动／晋级／商会入口已接；原字体与OS动态另验，见[菜单合同](PAGES.md) |
| 8 | SETTING_MENU | 菜单 | 仅声明；更新／绘制／动态链待逐项 |
| 9 | INFO_MENU | 菜单 | 静态：[五标签／输入、菜单及HUD摘要](INFORMATION_MENU.md)已核；维护：Owner、五行与HUD摘要只读计划已接。动态：无本页完整窗口认证；实际父原点、字体／触摸保护和一次性光标消费仍独立 |
| 10 | SYSTEM_MENU | 菜单 | 静态：五tags20–24及type1已核，APK末项28差异保留；维护：真实Owner、14保存与13说明已接；设置／RankForm／结束平台闭环另验，见[菜单合同](PAGES.md) |
| 11 | EVENT | 事件／系统 | 仅声明；更新／绘制／动态链待逐项 |
| 12 | CONFIG | 事件／系统 | 静态：真实菜单入口、Init冻结画质、Jp／En绘制分派、高速Touch门控及返回仅SaveSystem已核，区别于标题平台按钮，见[设置生命周期](STEAM_INTERACTIONS.md#游戏设置与标题平台按钮)。维护：非日文四行纯皮肤、音量控件原图与源／实际热区及四态参考已交；平台键盘原图、固定热区与高亮原请求已接研究绘制；跨局权威设置／输出与快照未接，字体／OS／平台动作未验 |
| 13 | MANUAL | 事件／系统 | 静态：Steam局部Draw、正文LT差异、人物冻结与箭头纹理已核；维护：真实菜单／Owner、正文／关于页和恢复已接。完整字体／TextLayout／OS动态另验，见[说明合同](PAGES.md#说明13固定正文输入消费与装饰随机) |
| 14 | SAVE | 事件／系统 | 静态保存状态机与局部皮肤已核；维护：真实菜单应用保存及冷载已验；S041/S064仅原动态局部，不代表自动中断上游，见[保存合同](STEAM_SAVE_MENU.md) |
| 15 | NEWS | 事件／系统 | 仅声明；更新／绘制／动态链待逐项 |
| 16 | WAIT | 事件／系统 | 仅声明；更新／绘制／动态链待逐项 |
| 17 | CLEARPOINT | 事件／系统 | 静态：[计分规则](../rules/STARTUP_RECORDS.md#通关六类计分固定apk及steam有限交叉)与[Steam局部Draw](STARTUP_SKIN.md#steam计分17的独立绘制交叉)已核，独立主钟／阶段钟、六类空单位及Touch2分开。维护：正式Steam计划／应用只读桥／窗口、wait0自然入口的独立主钟和联合恢复已接，Init切BGM0一次；通用非零wait仍未接。动态：通关页、快速分支资格及真实输入仍缺 |
| 18 | ENDING | 事件／系统 | 仅声明；更新／绘制／动态链待逐项 |
| 19 | NO_MYMENU | 事件／系统 | 仅声明；更新／绘制／动态链待逐项 |
| 20 | SAVE_MENU | 事件／系统 | 三项原序、raw1父子消费、取消清结果、SEB2/3／测宽／触摸窄矩形已核，见[合同](STEAM_SAVE_MENU.md)；已有S040不代替全链动态，维护文件17命令另验 |
| 21 | BUILD | 建设目录 | Init／Update、绘制／marker／滚动、85定义pattern、资源安装／旋转准入静态已核；完整父栈、最终OS输入／落点后续待证 |
| 22 | QUEST_SELECT | 任务 | [局部合同](PAGES.md#steam任务选择与募集22–24)：目录／原任务名／NEW／费用／宝箱等级及软6场景5→4已接；静态／维护短测与原窗口动态分开，字体／OS和特殊subQuest待证 |
| 23 | QUEST_GATHER | 任务 | [局部合同](PAGES.md#steam任务选择与募集22–24)：单募集确认／费用／取消与整轮方向输入已接；静态／维护短测与原窗口动态分开，字体／OS和特殊subQuest待证 |
| 24 | QUEST_GATHERANIME | 任务 | [局部合同](PAGES.md#steam任务选择与募集22–24)：真实X/Y报名演出／冻结发言／肖像／勋章／人数与held已接；静态／维护短测与原窗口动态分开，字体／OS和特殊subQuest待证 |
| 25 | QUEST_GATHERLIST | 任务 | [原序皮肤与Owner桥](PAGES.md#steam参加名单追加与出发25–28)／资源及条件示例已交；UI选择／冻结名单／页签／父一次返回及窗口接线已维护，原动态另验 |
| 26 | QUEST_GATHERLIST_SELECTING | 任务 | [原序皮肤与Owner桥](PAGES.md#steam参加名单追加与出发25–28)／资源及条件示例已交；UI选择／冻结名单／页签／父一次返回及窗口接线已维护，原动态另验 |
| 27 | QUEST_GATHERLIST_ADD | 任务 | [原序皮肤与Owner桥](PAGES.md#steam参加名单追加与出发25–28)／资源及条件示例已交；UI选择／冻结名单／页签／父一次返回及窗口接线已维护，原动态另验 |
| 28 | QUEST_START | 任务 | [原序皮肤与Owner桥](PAGES.md#steam参加名单追加与出发25–28)／资源及条件示例已交；UI选择／冻结名单／页签／父一次返回及窗口接线已维护，原动态另验 |
| 29 | QUEST_INFO | 任务 | [独立helper／严格桥](PAGES.md#steam地图候选信息与raw29不同入口)和条件例图已交；实际scene4信息／热区已接，raw29菜单开页入口及原动态未证 |
| 30 | QUEST_FINISH | 任务 | [两段／双钟及人物合同](PAGES.md#steam任务成果与期限的后继定位)、纯计划及严格Owner只读桥已接；完整summary／当前m／副钟拒绝边界与float32抛物分开核，奖励业务已维护，原动态另验 |
| 31 | QUEST_FINISH_MONSTER | 任务 | [五行战绩纯皮肤](PAGES.md#steam任务成果与期限的后继定位)及示例已交，合法重复X／独立数字单位已验；Owner选择／首行持久化和严格窗口桥已接，原动态另验 |
| 32 | QUEST_FINISH_DUNGEON | 任务 | [奖励五行纯皮肤](PAGES.md#steam任务成果与期限的后继定位)及示例已交，四类图标／0价格／仅首行滚动已核；Owner首行持久化和严格窗口桥已接，原动态另验 |
| 33 | QUEST_CONTINUE | 任务 | [按钮／期限／三人演出合同](PAGES.md#steam任务成果与期限的后继定位)与纯计划／例图已交，原男职业图、首次同定义实例动作／HPafter、全队倒下、普通Mapchip和怪物传入step／最终计时帧分开；Owner按钮持久化／严格来源桥及窗口已接，原动态另验 |
| 34 | INFO_TOWN | 情报／赠礼入口 | 静态：[统计／34→39及完整局部Draw](INFORMATION_MENU.md)已核；维护：Owner与语言／五星／测宽底栏计划已接。动态：无本页完整认证；字体／热区和Steam平台成就实际授予仍未证 |
| 35 | INFO_MEMBERLIST | 情报／赠礼入口 | 静态：[非0目录、贡献／NEW、四页及60追踪来源](INFORMATION_MENU.md)已核；维护：Owner、恢复与四页有序裁剪／人物计划已接。动态：未全页认证；字体、共享scratch附加效果与平台成就仍独立 |
| 36 | INFO_INCOME | 情报／赠礼入口 | 静态：[月年统计、独立左右／关闭、完整局部Draw](INFORMATION_MENU.md)已核；维护：Owner及金额文本／端点线／箭头计划已接。动态：未全页认证；字体、OS输入和实际测宽仍独立 |
| 37 | INFO_ITEM | 情报／赠礼入口 | 静态：[正库存原序、空15、关闭清NEW及局部Draw](INFORMATION_MENU.md)已核；维护：Owner与五行／说明／滚动计划已接。动态：未全页认证；原字体、最终触摸与动态SEB执行仍独立 |
| 38 | INFO_EQUIP | 情报／赠礼入口 | 静态：[Steam flags过滤、未知行、四页与局部Draw](INFORMATION_MENU.md)已核；维护：Owner与图标／属性／GET／滚动计划已接。动态：未全页认证；字体、实际点击和动态手形仍独立 |
| 39 | INFO_TENANT | 情报／赠礼入口 | 静态：[kind3原序、编号／截至当月利润、scene7及局部Draw](INFORMATION_MENU.md)已核；维护：稳定ID目录／恢复、镜头交接及五行／正负金额计划已接。动态：未全页认证；字体／物理输入仍缺，确认不冒称打开商店详情 |
| 40 | MEMBER_PRESENT | 情报／赠礼入口 | Steam页面仍仅声明；维护赠礼人物目录及进入64已有Owner消费者，不等于35皮肤自动适用于40。独立Steam绘制／输入与原动态待核，见[页面边界](PAGES.md) |
| 41 | DEVELMP_TOP | 魔法壶 | [局部合同](STEAM_MAGIC_POT.md)：主页壶／知识／四元素、按钮及整轮输入已接；静态来源／维护短测与原窗口动态分开，字体／OS待证 |
| 42 | DEVELMP_ITEMSELECT | 魔法壶 | [局部合同](STEAM_MAGIC_POT.md)：五行原序库存／元素等级、Steam独立margin及marker16已接；静态来源／维护短测与原窗口动态分开，字体／OS待证 |
| 43 | DEVELMP_RECIPESELECT | 魔法壶 | [局部合同](STEAM_MAGIC_POT.md)：未知／已知两页、成本、无marker及上下后左右顺序已接；静态来源／维护短测与原窗口动态分开，字体／OS待证 |
| 44 | DEVELMP_ANIME1 | 魔法壶 | [局部合同](STEAM_MAGIC_POT.md)：投物／闪光／评语、独立副钟与Owner单次SE8已接；静态来源／维护短测与原窗口动态分开，字体／OS待证 |
| 45 | DEVELMP_OPEN_PARAM | 魔法壶 | [局部合同](STEAM_MAGIC_POT.md)：处理报告五属性、知识及快进门槛已接，无返回输入；静态来源／维护短测与原窗口动态分开，字体／OS待证 |
| 46 | DEVELMP_GETRECIPE | 魔法壶 | [局部合同](STEAM_MAGIC_POT.md)：发现配方的奖励图标／成本／等待确认已接，未提前置位；静态来源／维护短测与原窗口动态分开，字体／OS待证 |
| 47 | DEVELMP_CREATE | 魔法壶 | [局部合同](STEAM_MAGIC_POT.md)：制作确认／不足分支及真正扣元素后继已接；静态来源／维护短测与原窗口动态分开，字体／OS待证 |
| 48 | AS_RANKUP_TOWN | 村办／晋级 | 静态：四条件／五选择、星与底栏helper已核。维护：Owner选择／说明／申请及有序皮肤已接。动态：旧S007仅布局候选；原字体／OS输入与完整原窗口仍缺，见[合同](PAGES.md#steam-village-progress) |
| 49 | AS_RANKUP_TOWN_AUTO | 村办／晋级 | 静态：只读条件、底栏与48差异已核。维护：只读皮肤／返回已接；不提供申请命令。动态：未完整认证，见[合同](PAGES.md#steam-village-progress) |
| 50 | AS_RANKUP_TOWN_ANIME | 村办／晋级 | 静态：三段／冻结布局、两人物层与独立frame2步帧已核。维护：双钟、X、绘图和140确认已接，查询不重洗名单。动态：连续庆典／共享scratch／原字体仍缺，见[合同](PAGES.md#steam-village-progress) |
| 51 | AS_EVENT_SELECT | 村办／晋级 | 静态：目录／准入顺序、五行／季度／th分支、Touch11独立margin与111滑条已核。维护：Owner首行／整轮输入及有序皮肤已接。动态：最终OS滚动／点击及连续原窗口仍缺，见[合同](PAGES.md#steam-village-progress) |
| 52 | AS_EVENT_INFO | 村办／晋级 | 静态：支付时点、插画、说明夹190、按钮夹60与六行边距已核。维护：两选项输入／纯计划已接。动态：原字体／OS热区实测仍缺，见[合同](PAGES.md#steam-village-progress) |
| 53 | AS_EVENT_ANIME | 村办／晋级 | 静态：[60,70,110,120]、jingle5及效果时点、进度／mask／mini和类型1语言分支已核。维护：六类结果计划、120确认及Owner效果已接；类型4拒绝。动态：连续原窗口／字体及条件held后备资格仍缺，见[合同](PAGES.md#steam-village-progress) |
| 54 | AS_EVENT_AFTER | 村办／晋级 | 静态：冻结开放人物、当前减ao、五行／clip／111滑条及无option行已核。维护：Owner姓名／性别覆盖、只读结果及关闭已接。动态：原字体／OS输入与附加scratch仍缺，见[合同](PAGES.md#steam-village-progress) |
| 55 | CAMERAMOVE_MA | 相机／来访 | 仅声明；更新／绘制／动态链待逐项 |
| 56 | CAMERAMOVE_MAA | 相机／来访 | 仅声明；更新／绘制／动态链待逐项 |
| 57 | CAMERAMOVE_DUNGEON | 相机／来访 | 仅声明；更新／绘制／动态链待逐项 |
| 58 | CAMERAMOVE_SELECTQUEST | 相机／来访 | 仅声明；更新／绘制／动态链待逐项 |
| 59 | MANYAPPEAHUMAN | 相机／来访 | 仅声明；更新／绘制／动态链待逐项 |
| 60 | CHARA_INFO | 人物／成长／礼物 | 静态：[四页及输入](STEAM_HUMAN_DETAIL.md)已核；维护：四页完整局部计划、四槽同轮输入／滚动及Owner桥已交付。完整字体、共享scratch与原窗口全链未认证 |
| 61 | CHARA_JOBCHANGE_LIST | 人物／成长／礼物 | 静态及维护：职业五行两页、首行／选择、预览返回及整轮输入已交付；原字体／OS动态另验，见[人物合同](STEAM_HUMAN_DETAIL.md) |
| 62 | CHARA_JOBCHANGE_INFO | 人物／成长／礼物 | 静态及维护：职业预览、当前／可转职分支、真实测宽及63入口已交付；原动态另验，见[人物合同](STEAM_HUMAN_DETAIL.md) |
| 63 | CHARA_JOBCHANGE_ANIME | 人物／成长／礼物 | 静态及维护：55改职业／197最终确认、演出资源与共享人物钟已接；原字体／scratch／OS另验，见[人物合同](STEAM_HUMAN_DETAIL.md) |
| 64 | CHARA_PRESENT_ITEMSELECT | 人物／成长／礼物 | 静态及维护：装备／普通道具目录、五槽／四行／首行、回复预选及整轮输入已接；原动态另验，见[人物合同](STEAM_HUMAN_DETAIL.md) |
| 65 | CHARA_PRESENT_ITEMSELECT_INFO | 人物／成长／礼物 | 静态及维护：装备赠礼确认皮肤及答案回父64，父恢复才支付／装配；原动态另验，见[人物合同](STEAM_HUMAN_DETAIL.md) |
| 66 | CHARA_PRESENT_ANIME | 人物／成长／礼物 | 静态及维护：赠礼演出、类别结果和原资源计划已交付；Owner显式表现请求与原Draw准入分开，完整scratch／字体／OS仍缺，见[人物合同](STEAM_HUMAN_DETAIL.md) |
| 67 | CHARA_PUTUP_BONUS | 人物／成长／礼物 | [合同与有序皮肤](STEAM_HUMAN_DETAIL.md#human-reward-followups)：与68共用四战斗行，冻结努力百分比／严格Owner桥／窗口及短恢复已接；原字体与OS动态仍待证 |
| 68 | CHARA_EQUIP_EFFECT | 人物／成长／礼物 | 静态及维护：冻结装备槽／新ID、当前与差额动画、有序计划与真实消费参考已交付；完整原动态另验，见[人物合同](STEAM_HUMAN_DETAIL.md) |
| 69 | CHARA_ITEM_EFFECT | 人物／成长／礼物 | 静态及维护：普通道具结果、冻结前值、当前能力动画与实际消费参考已交付；完整原动态另验，见[人物合同](STEAM_HUMAN_DETAIL.md) |
| 70 | CHARA_LVMAX | 人物／成长／礼物 | [大师合同](STEAM_HUMAN_DETAIL.md#职业大师70与人物奖励后继)：三段原序皮肤、双钟、同轮Owner输入／严格恢复与窗口已接；Steam软标签0/0，APK取消合同不扩成Steam Esc，原字体／成就旁支与动态另验 |
| 71 | EQUIP_INFO | 装备详情 | 仅声明；更新／绘制／动态链待逐项 |
| 72 | EQUIP_INFO_FROM_TENANT | 装备详情 | 静态及维护：单项装备说明、原1/1／双箭头和79真实父绑定已接；原动态另验，见[商品合同](../rules/COMMERCE.md) |
| 73 | EQUIP_INFO_FROM_PRESENT | 装备详情 | 静态及维护：只读装备信息、资源／数值与赠礼目录返回已交付；原字体／OS另验，见[人物合同](STEAM_HUMAN_DETAIL.md) |
| 74 | TENANT_INFO | 设施 | 静态：[普通两页／特殊布局与交接](STEAM_FACILITY_DETAIL.md)及GameView即时滚动条已核；维护普通实例／85定义预览、装备店与植物计划／窗口已接，入住人数模板／严格资格查询与现80交接已接并短验。住宅模板文字／帧及共享副作用已局部核，三个已核正常producer未产生有效住宅74，维护不造入口或演员请求；不等于全程序不可达。动态：S043/S047/S054及[S069–S071](../references/README.md#steam普通设施两页2026-10-11)补两行来源、鼠标翻页／选择／返回；空／长列表、OS拖动及键输入消费仍未闭合，HTML视觉未验 |
| 75 | TENANT_ITEMSELECT | 设施 | 静态及维护：五行道具／marker16、Owner首行与整轮输入／恢复已交付；S044/S046/S055仅局部动态，见[页面合同](PAGES.md) |
| 76 | TENANT_ANIME1 | 设施 | 静态及维护：投物、共享J／价格刷新、49自动转页与正式皮肤已接；38声音为Owner一次请求适配，原连续演出另验，见[效果合同](../rules/FACILITY_EFFECTS.md) |
| 77 | TENANT_ANIME2 | 设施 | 静态及维护：49快进／55关闭、前后数字与角色资源计划已接；S045仅结果局部，完整OS动态另验，见[效果合同](../rules/FACILITY_EFFECTS.md) |
| 78 | TENANT_SERVICE | 设施 | 仅声明；更新／绘制／动态链待逐项 |
| 79 | TENANT_EQUIPLIST | 设施 | 静态及维护：四行两页／NEW、商品信息72与真实Owner整轮输入已交付；确认清标记不购买，原动态另验，见[商品合同](../rules/COMMERCE.md) |
| 80 | TENANT_RESIDENT_LIST | 设施 | [局部合同与纯皮肤](PAGES.md#steam住宅候选80与授勋目录87)及显式游标helper已接，保留四行更新／五行绘制及marker16；现有入住事务，Owner游标／完整窗口与原动态待接 |
| 81 | TENANT_LVUP_ANIME | 设施 | 静态：[Init真正升级、40／55确认、独立frame2及完整局部Draw](STEAM_FACILITY_UPGRADE.md)已核；维护：Owner与数字动画／角色／框及触摸计划已接。动态：无连续原窗口认证；原字体、平台热区仍独立 |
| 82 | TENANT_STREN_BONUS | 设施 | 静态及维护：两阶段口碑／人气、有序皮肤、独立双钟／恢复与条件参考已交付；未认证自然36个月触发、完整scratch或原OS动态，见[效果合同](../rules/FACILITY_EFFECTS.md) |
| 83 | SHOPPING_TOP | 商会 | Steam完整局部合同、只读皮肤／Owner NEW查询及同轮输入已接研究窗口：三入口、等级、私有南瓜、两NEW、上→下→确认优先返回；见[合同](../rules/COMMERCE.md#steam-commerce-top)与[接口](../prototype/include/dungeon_village_prototype/steam_commerce_skin.hpp)。字体／共享手形调度及原OS动态独立，未由条件图认证 |
| 84 | SHOPPING_ITEMSELECT | 商会 | Steam局部Init／输入／绘制与APK差异已核，见[目录合同](../rules/COMMERCE.md#steam-commerce-items)。已有交易Owner；冻结反馈金额缺载荷，不能用当前选项价替代。皮肤与整轮反馈次序接线待补，原动态未验 |
| 85 | SHOPPING_TENANT | 商会 | Steam三行／Mapchip2／点价／滚动／NEW、详情74及先扣点再93领取已核，见[设施合同](../rules/COMMERCE.md#steam-commerce-facilities)。维护原序皮肤／三行滚动／marker接线与同轮Owner输入已接，保留p0策略及85付点→93独立领取。普通／装备店／植物74定义预览已接独立皮肤与只读桥，原动态未验 |
| 86 | SHOPPING_ACT_AFTER | 商会 | Steam完整局部已核：已交易后创建，下一获准更新直接Pop，虽画了解但不等待确认；所持金标签旁实际绘本笔差额。见[反馈合同](../rules/COMMERCE.md#steam-commerce-result)。维护只读皮肤／真实图标／实际测宽及研究窗口已接，保留原自动关闭；原动态未验 |
| 87 | MEDAL_LIST | 授勋／怪物／住宅 | [局部合同与纯计划](PAGES.md#steam住宅候选80与授勋目录87)及显式游标helper已接：三页签、五行、各数值列／勋章与住宅图；现奖励消费者不等于原dialog父子页，Owner游标／页签／问答接线及原动态待补 |
| 88 | MEDAL_CELEMONY | 授勋／怪物／住宅 | [合同与有序皮肤](STEAM_HUMAN_DETAIL.md#human-reward-followups)：Steam背景／演员／奖章／气泡／陪衬和mentalUp偏移135、严格Owner桥／窗口／恢复已接；共享scratch附效、字体及原动态待证 |
| 89 | MONSTER_NEWAPPEAR | 授勋／怪物／住宅 | Steam静态产生者：介绍脚本→绑定所选怪物的89请求→立即置介绍标志已核；维护同步介绍／对话身份与回滚已接。89自身SteamInit／Update／Draw局部与素材已核；现单主钟的真实Init、身份／缺载荷拒绝、确认与恢复／退休已短验。维护皮肤／副钟及共同SEB／共享monster scratch闭包及原动态仍缺，见[怪物介绍合同](../rules/STEAM_MONSTER_PROGRESSION.md) |
| 90 | MYHOME_TAX | 授勋／怪物／住宅 | SteamInit／Update／Draw、头像／两种SEB金额、五行滚动及soft0／0已核；维护只读皮肤、研究窗口和整轮输入已接，严格初始化／恢复不重枚举丢失名单，确认不入账。共享人物附效、原字体／OS动态仍未认证，见[税收合同](../rules/ACCOUNTING.md) |
| 91 | NEWGAME | 新局／奖励／解锁／事件 | 静态：[NewGame／默认参数小方法](../rules/STARTUP_RECORDS.md#steam小方法交叉及设计决策)及[标题空栏入口](STEAM_TITLE_MENU.md)已核，[Steam Draw](STARTUP_SKIN.md#steam新局91的独立绘制交叉)已核，完整Update仍待核。维护：已有草稿／正式开始及独立Steam纯计划、四态参考；配置选择／主钟的Owner恢复尚未接；动态：空栏配置、默认联动／输入面板局部已见，未读运行页号；自定义保名／取消保留属用户批准策略，非Steam实测 |
| 92 | MONSTER_INFO | 新局／奖励／解锁／事件 | 仅声明；更新／绘制／动态链待逐项 |
| 93 | GET_OBJ | 新局／奖励／解锁／事件 | [共享领取合同](../rules/ACCOUNTING.md#script-rewards)：商会已维护r3的原序皮肤、严格Owner桥及窗口已接；无给予人物，85先付点、93满40确认才开放，原动态另验 |
| 94 | GET_OBJ_FROM_CHARA | 新局／奖励／解锁／事件 | [共享领取合同](../rules/ACCOUNTING.md#script-rewards)：已维护r3–8的真实给予人、装备／道具图标、原序皮肤及严格Owner桥已接；新职业早已开放与实际领取分开，完整字体／动态另验 |
| 95 | GET_OBJ_FROM_AWARD | 新局／奖励／解锁／事件 | 静态：共93／94绘制模板的七类图文、SetClip替换及r11择一后缀已核。维护：真实r/s/t只读皮肤、等待／40领取及拒绝已接，范围见[合同](../rules/ACCOUNTING.md#script-rewards)。动态：原窗口／字体与94其它分支仍缺 |
| 96 | GET_MYHOME | 新局／奖励／解锁／事件 | [局部合同与维护计划](STEAM_HUMAN_DETAIL.md#human-reward-followups)：Mapchip2、独立副钟、人物抛物、mentalUp／jingle5、严格Owner桥及窗口已接；短拒绝／恢复／退休已验，完整字体与原OS动态待证 |
| 97 | TP_LOCKOFF | 新局／奖励／解锁／事件 | Steam静态：第一次实际Update即清人气条锁并Pop，无确认／计数门槛已核；维护人气奖励后继已接。未由此认证Draw或完整人气演出动态，见[人气合同](../rules/ACCOUNTING.md#人气100条奖励与超过10000后的重放) |
| 98 | TAX_INCOME | 新局／奖励／解锁／事件 | Steam页面仍仅声明；APK自动重新合计／其它收入／清全人物F/G合同及维护Owner事务已接，无玩家确认。Steam自身Update／Draw及原动态仍待核，见[税收合同](../rules/ACCOUNTING.md) |
| 99 | BOSS_APPEAR | 新局／奖励／解锁／事件 | Steam静态产生者：特殊任务成功创建后134→99及后继事件顺序已核；APK满40确认／维护演出消费者不代证Steam自身Update／Draw。独立Steam皮肤与原动态仍缺，见[后期任务合同](../rules/PROGRESSION_ROUTES.md) |
| 100 | MANYMONSTER_APPEAR | 新局／奖励／解锁／事件 | Steam静态产生者：复发任务成功创建后133→100及167守卫已核；APK演出数组／随机及40确认、维护消费者与Steam自身Update／Draw分开。独立Steam皮肤与原动态仍缺，见[后期任务合同](../rules/PROGRESSION_ROUTES.md) |

标题菜单0／存档栏1的[菜单／选档静态合同](STEAM_TITLE_MENU.md)与维护纯计划已接；S038／S039/S062及[后继窗口](../verification/startup-ui-analysis/ANALYSIS.md)只认证局部入口／显示，不证明完整覆盖、删除或配置最终开始。独立RankForm的[双页Update及纪录getter](../rules/STARTUP_RECORDS.md#steam小方法交叉及设计决策)有Steam静态证据，[两轮窗口](../verification/skin-observation-analysis/ANALYSIS.md)补纪录外观／翻页；维护纪录计划仍有APK来源部分，Steam完整Draw与字体未核，不与SubForm的3 MAIN_MENU混号。
GameForm 0 MAIN、1 BUILD有局部窗口证据；6 CHASE_CHARA、7 CHASE_TENANT另有[具名追踪／确认与镜头静态合同及维护接线](INFORMATION_MENU.md)，不再仅有声明，但原窗口追踪全链未认证。2 USER、3 BACK、4 DUNGEON_SELECT、5 DUNGEON_MOVE在此仍仅登记声明。
S066–S068自然对话不按旧标签补写具名详情，见[非空p实验](../work/window-restore-observation/20261007-131546-nonempty-p-reload/RESULT.md)。

## 26组件与14事件分母

GameView先让顶页处理事件，再通用分派；跳表接受ID0–24，25走默认。17相机由GameForm顶页消费者处理，不能因通用默认项认作缺功能。

| ID／组件 | 实际路由或候选 | 限制 |
| --- | --- | --- |
| 0 TITLEMENU | 通用标题分支 | 路由已核；完整输入未知 |
| 1 DLGPAGE、18 ARROW | 同入口0x1023CCBE | 36／38等页箭头注册及逻辑翻页已有合同；其它页、最终物理输入仍另核 |
| 2 DLGCMD、9 SUBMENU | 同入口0x1023C9CD | raw9基矩形／值及ENTER选行、UP请求确认已核，见[输入后继](INFORMATION_MENU.md#steam9选行确认与虚拟光标后继静态已核)；维护只接解析后输入，完整命中／guard和其它页仍独立 |
| 3 DLGSEL、10 LIST、11 SLIST | 同入口0x1023CA42；有marker才标记后确认，无marker的UP直接确认 | 75／21有marker；48／51／52／54无marker注册及不同Margin已核，不能混用全部输入政策 |
| 4 KEYCLICK、20 PLUS、21 MINUS | OnTouchKeyClick | raw9组件4/value22/flag2无矩形、34测宽底栏value20已核并有计划；不能推广全屏热区，其它参数仍逐页核 |
| 5 KEYPRESS | OnTouchKeyPress | 同上 |
| 6 KEYDOWN、7 KEYUP | 通用内联分支 | 位拆分及完整资格链待核 |
| 8 MAINMENU | OnTouchMainMenu | 完整菜单／解锁待核 |
| 12 VSCROLLBAR、14 VSCROLLBAR2、15 VSCROLLBAR3 | OnTouchVScrollBar | 8相对／9绝对滚动及35／37–39、51／54组件12注册已核，维护保留110／111差异；其它页注册及最终输入仍待核 |
| 13 VSCROLLBTN | OnTouchVScrollButton | ON上下边界已核，注册待核 |
| 16 UNMARKER | OnTouchUnMarker | 完整生命周期待核 |
| 17 CAMERA | GameForm.OnTouchEvent→OnTouchCamera | 主世界／建设局部已核；OS投递及全模式待核 |
| 19 TRACKBAR | 通用轨条分支 | 具体用途待核 |
| 22 OVER、24 RECT | 各自通用内联分支 | 用途及注册未知 |
| 23 ARROW_UD | 默认入口0x1023CA3B | 51上／下组件17／19及独立margin已核并有计划；最终OS输入及其它页仍缺 |
| 25 SCROLLBG | 超通用跳表上限 | 35／37–39的背景注册／flag4已核并有计划；不能据注册推成通用跳表已处理，最终命中仍独立 |

14事件为DOWN0、UP1、ENTER2、LEAVE3、MOVE4、CLICK5、LOST6、DRAG7、GRID_DRAG8、CELL_DRAG9、CANCEL10、PINCH_START11、PINCH12、ON13。
本批核到建设DOWN／UP／CLICK／GRID_DRAG及滚动8／9／13局部分支，其它事件不因声明存在报完成。
TouchOption十四标志是已核声明分母；marker16有75注册证据，其余优先级／取消组合按实际消费者另核，不依赖本地inventory清单。
Steam CANCEL=10，而APK原始Android ACTION_CANCEL=3；事件适配层不同，不能直接复制数值。

## 建设定位、拖动、旋转与提交

| 合同 | Steam真实机器码位置 | 新事实及边界 |
| --- | --- | --- |
| 相机事件资格 | GameForm.OnTouchEvent RVA0x2FBFF0 | 组件17调用OnTouchCamera且返回false，阻止通用二次处理；其它有效组件允许通用路由 |
| 建设定位与点击 | OnTouchCamera RVA0x2FADA0；0x102FB97F／0x102FBB9D／0x102FBCC1 | DOWN在BUILD状态、投影格与当前候选格双坐标相同且模式1或6时可发确认0x100000；不是所有DOWN均建设 |
| 释放 | 同方法0x102FBE85–0x102FBFBC | UP支按相机拖动记录算惯性并清标志，没有直接ProcBuild或KeyClick确认；不能普遍写“释放立即建造” |
| CLICK／GRID_DRAG | 0x102FB090–0x102FB09C、0x102FB258–0x102FB484、0x102FB493–0x102FB4D7 | CLICK有候选格／对象资格再确认；GRID_DRAG受buildLock和起点差约束，OS投递顺序仍未知 |
| 三类建设目录 | SubForm.OnTouchEvent RVA0x3160B0；0x10316103–0x103161E5 | 21页组件3及value高16位0x50000处理类目UP/ENTER，循环模3并清scroll/select；类别名称不由模3推定 |
| 旋转 | GameForm._update RVA0x303430；0x10304739–0x10304772 | 软标签9生效后tenantBuildInversion=1−旧值；标签参数不是具体桌面键 |
| 闪烁与朝向 | Draw_buildObj RVA0x2F7110；0x102F7DB5–0x102F7E5F | 普通候选局部路径stateCnt_%20<10绘图块，实际mapchipId及tenantBuildInversion传入DrawMapchip；20是状态计数，不换秒 |
| 提交消费 | _update 0x103046E4／0x10304D1F／0x10304D4A／0x10304E71 | 确认脉冲后按朝向占地、ChBuild、模式及费用资格，再ProcBuild；输入逻辑请求与领域提交分开 |

这是实际DLL消费者证据；已定位调用不等于ProcBuild各模式完整规则认证。
道路／移动／撤除有独立调用锚点，规则保留APK已有合同，Steam费用／人气／引用生命周期待独立交叉。
本批未把维护Owner原子提交策略改成原游戏实现，也未修改产品。

建设21初批从具名_draw入口顺序解码8704字节前缀，闭合局部分支6133字节、marker16及mapchip→pattern→SEB绑定；后继已核pattern全值与Init／Update，见前述正式合同。没有把23–40分派当作21证据；外层197,616字节未因此完成全量解码，最终OS热区与落点后续仍独立。

## 列表标记、滚动及最小后续

75的marker／5行／局部矩形已交付于[交互合同](STEAM_INTERACTIONS.md#输入选中标记与确认分开)。
第一次未标记UP只选中／标记，再次同key UP确认；没有点击间隔条件，键盘会清marker。
历史“单击→Return→双击”不是受控比较，不能称必须双击；75机制不自动适用于建设21。

通用垂直滚动条：GRID_DRAG8相对变化、CELL_DRAG9绝对值；拒绝INT_MAX，约束0…maximum−largeChange。
ON13按钮按低16位0/1方向作一步变化；SubForm.Update_scrollValue使选中跟随显示窗口。
21、35、37–39等页注册参数已有各自静态合同与部分维护计划；其它页、鼠标滚轮适配、拖动取消仍未全面闭合，metadata的Unity ScrollWheel只说明引擎能力。

下一批按独立短链推进：

1. 沿已核21的pattern、Init／Update与BUILD载荷继续父栈／落点后续及最终OS输入，不重复已闭合静态分支或经营长前缀。
2. 固定已有条件，观察DOWN／移动／UP／重复同格输入、旋转／取消／一次确认；记录候选格、方向与实体分别何时变，键盘与鼠标分开。
3. 同一列表隔离悬停、首击、移开再点、长间隔同key再次点击、键盘确认及滚轮／拖条，每次只改一条件。
4. 从101清单按功能批次补Init／Update／Draw／输入载荷与局部动态；空栏／删除等另按授权实验范围执行。

早期分母取证批范围为19方法66,192字节、最大18,512字节；这只是当批20方法／96KiB／32KiB限制内的记录，不是后继全表累计覆盖率。
本表按各正式合同区分静态、维护与窗口范围；完整UI和真实键鼠等价尚未认证，文档状态更新不替代对应实现的验收记录。
