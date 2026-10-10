# Steam 2.56 UI／操作覆盖清单

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
| 3 | MAIN_MENU | 菜单 | 静态：[缓存重入、目录／NEW、父位置、ID8及DrawMenu2(type0)](PAGES.md#steam主菜单缓存输入与局部皮肤)已核；维护：逐行图标／NEW／GET／期间块纯计划及壶只读查询已接，真实raw3 Owner仍缺。动态：S042仅外观，原字体／整栈输入／光标与Review独立待验 |
| 4 | ADVENTURE_MENU | 菜单 | 仅声明；更新／绘制／动态链待逐项 |
| 5 | DEVELOP_MENU | 菜单 | 仅声明；更新／绘制／动态链待逐项 |
| 6 | SHOPPING_MENU | 菜单 | 仅声明；更新／绘制／动态链待逐项 |
| 7 | ACTION_MENU | 菜单 | 仅声明；更新／绘制／动态链待逐项 |
| 8 | SETTING_MENU | 菜单 | 仅声明；更新／绘制／动态链待逐项 |
| 9 | INFO_MENU | 菜单 | 静态：[五标签／输入、菜单及HUD摘要](INFORMATION_MENU.md)已核；维护：Owner、五行与HUD摘要只读计划已接。动态：无本页完整窗口认证；实际父原点、字体／触摸保护和一次性光标消费仍独立 |
| 10 | SYSTEM_MENU | 菜单 | 静态：[Steam五tags20～24、系统结果与type1绘制](PAGES.md#steam主菜单缓存输入与局部皮肤)已核，末项与APK28不同。维护：正式只读行计划及[四条件示例](examples/steam-system-menu.png)已接；真实Owner、保存／RankForm／结束与平台请求仍缺。动态：S049局部外观，不认证全输入或结果 |
| 11 | EVENT | 事件／系统 | 仅声明；更新／绘制／动态链待逐项 |
| 12 | CONFIG | 事件／系统 | 仅声明；更新／绘制／动态链待逐项 |
| 13 | MANUAL | 事件／系统 | 仅声明；更新／绘制／动态链待逐项 |
| 14 | SAVE | 事件／系统 | 保存状态机局部静态＋S041/S064 |
| 15 | NEWS | 事件／系统 | 仅声明；更新／绘制／动态链待逐项 |
| 16 | WAIT | 事件／系统 | 仅声明；更新／绘制／动态链待逐项 |
| 17 | CLEARPOINT | 事件／系统 | 静态：[SetClearPoint六类及Update阶段](../rules/STARTUP_RECORDS.md#通关六类计分固定apk及steam有限交叉)有Steam有限交叉；维护：计分／应用回放已接，现图片计划沿APK合同。动态：通关页未取得；Steam完整Draw、快速分支资格及真实输入仍缺 |
| 18 | ENDING | 事件／系统 | 仅声明；更新／绘制／动态链待逐项 |
| 19 | NO_MYMENU | 事件／系统 | 仅声明；更新／绘制／动态链待逐项 |
| 20 | SAVE_MENU | 事件／系统 | 三项原序、raw1父子消费、取消清结果、SEB2/3／测宽／触摸窄矩形已核，见[合同](STEAM_SAVE_MENU.md)；已有S040不代替全链动态，维护文件17命令另验 |
| 21 | BUILD | 建设目录 | Init／Update、绘制／marker／滚动、85定义pattern、资源安装／旋转准入静态已核；完整父栈、最终OS输入／落点后续待证 |
| 22 | QUEST_SELECT | 任务 | 仅声明；更新／绘制／动态链待逐项 |
| 23 | QUEST_GATHER | 任务 | 仅声明；更新／绘制／动态链待逐项 |
| 24 | QUEST_GATHERANIME | 任务 | 仅声明；更新／绘制／动态链待逐项 |
| 25 | QUEST_GATHERLIST | 任务 | 仅声明；更新／绘制／动态链待逐项 |
| 26 | QUEST_GATHERLIST_SELECTING | 任务 | 仅声明；更新／绘制／动态链待逐项 |
| 27 | QUEST_GATHERLIST_ADD | 任务 | 仅声明；更新／绘制／动态链待逐项 |
| 28 | QUEST_START | 任务 | 仅声明；更新／绘制／动态链待逐项 |
| 29 | QUEST_INFO | 任务 | 仅声明；更新／绘制／动态链待逐项 |
| 30 | QUEST_FINISH | 任务 | 仅声明；更新／绘制／动态链待逐项 |
| 31 | QUEST_FINISH_MONSTER | 任务 | 仅声明；更新／绘制／动态链待逐项 |
| 32 | QUEST_FINISH_DUNGEON | 任务 | 仅声明；更新／绘制／动态链待逐项 |
| 33 | QUEST_CONTINUE | 任务 | 仅声明；更新／绘制／动态链待逐项 |
| 34 | INFO_TOWN | 情报／赠礼入口 | 静态：[统计／34→39及完整局部Draw](INFORMATION_MENU.md)已核；维护：Owner与语言／五星／测宽底栏计划已接。动态：无本页完整认证；字体／热区和Steam平台成就实际授予仍未证 |
| 35 | INFO_MEMBERLIST | 情报／赠礼入口 | 静态：[非0目录、贡献／NEW、四页及60追踪来源](INFORMATION_MENU.md)已核；维护：Owner、恢复与四页有序裁剪／人物计划已接。动态：未全页认证；字体、共享scratch附加效果与平台成就仍独立 |
| 36 | INFO_INCOME | 情报／赠礼入口 | 静态：[月年统计、独立左右／关闭、完整局部Draw](INFORMATION_MENU.md)已核；维护：Owner及金额文本／端点线／箭头计划已接。动态：未全页认证；字体、OS输入和实际测宽仍独立 |
| 37 | INFO_ITEM | 情报／赠礼入口 | 静态：[正库存原序、空15、关闭清NEW及局部Draw](INFORMATION_MENU.md)已核；维护：Owner与五行／说明／滚动计划已接。动态：未全页认证；原字体、最终触摸与动态SEB执行仍独立 |
| 38 | INFO_EQUIP | 情报／赠礼入口 | 静态：[Steam flags过滤、未知行、四页与局部Draw](INFORMATION_MENU.md)已核；维护：Owner与图标／属性／GET／滚动计划已接。动态：未全页认证；字体、实际点击和动态手形仍独立 |
| 39 | INFO_TENANT | 情报／赠礼入口 | 静态：[kind3原序、编号／截至当月利润、scene7及局部Draw](INFORMATION_MENU.md)已核；维护：稳定ID目录／恢复、镜头交接及五行／正负金额计划已接。动态：未全页认证；字体／物理输入仍缺，确认不冒称打开商店详情 |
| 40 | MEMBER_PRESENT | 情报／赠礼入口 | 仅声明；更新／绘制／动态链待逐项 |
| 41 | DEVELMP_TOP | 魔法壶 | 仅声明；更新／绘制／动态链待逐项 |
| 42 | DEVELMP_ITEMSELECT | 魔法壶 | 仅声明；更新／绘制／动态链待逐项 |
| 43 | DEVELMP_RECIPESELECT | 魔法壶 | 仅声明；更新／绘制／动态链待逐项 |
| 44 | DEVELMP_ANIME1 | 魔法壶 | 仅声明；更新／绘制／动态链待逐项 |
| 45 | DEVELMP_OPEN_PARAM | 魔法壶 | 仅声明；更新／绘制／动态链待逐项 |
| 46 | DEVELMP_GETRECIPE | 魔法壶 | 仅声明；更新／绘制／动态链待逐项 |
| 47 | DEVELMP_CREATE | 魔法壶 | 仅声明；更新／绘制／动态链待逐项 |
| 48 | AS_RANKUP_TOWN | 村办／晋级 | 仅声明；更新／绘制／动态链待逐项 |
| 49 | AS_RANKUP_TOWN_AUTO | 村办／晋级 | 仅声明；更新／绘制／动态链待逐项 |
| 50 | AS_RANKUP_TOWN_ANIME | 村办／晋级 | 仅声明；更新／绘制／动态链待逐项 |
| 51 | AS_EVENT_SELECT | 村办／晋级 | 仅声明；更新／绘制／动态链待逐项 |
| 52 | AS_EVENT_INFO | 村办／晋级 | 仅声明；更新／绘制／动态链待逐项 |
| 53 | AS_EVENT_ANIME | 村办／晋级 | 仅声明；更新／绘制／动态链待逐项 |
| 54 | AS_EVENT_AFTER | 村办／晋级 | 仅声明；更新／绘制／动态链待逐项 |
| 55 | CAMERAMOVE_MA | 相机／来访 | 仅声明；更新／绘制／动态链待逐项 |
| 56 | CAMERAMOVE_MAA | 相机／来访 | 仅声明；更新／绘制／动态链待逐项 |
| 57 | CAMERAMOVE_DUNGEON | 相机／来访 | 仅声明；更新／绘制／动态链待逐项 |
| 58 | CAMERAMOVE_SELECTQUEST | 相机／来访 | 仅声明；更新／绘制／动态链待逐项 |
| 59 | MANYAPPEAHUMAN | 相机／来访 | 仅声明；更新／绘制／动态链待逐项 |
| 60 | CHARA_INFO | 人物／成长／礼物 | 静态：[四页布局](STEAM_HUMAN_DETAIL.md)、[输入](STEAM_HUMAN_INPUT.md)及[肖像／HP／奖章](STEAM_HUMAN_PRESENTATION.md)已核，追踪见[信息合同](INFORMATION_MENU.md)。维护：Owner与基础肖像／危险图标／气泡计划已接；四页完整文字皮肤、scratch附加效果及原窗口全链仍未认证 |
| 61 | CHARA_JOBCHANGE_LIST | 人物／成长／礼物 | 仅声明；更新／绘制／动态链待逐项 |
| 62 | CHARA_JOBCHANGE_INFO | 人物／成长／礼物 | 仅声明；更新／绘制／动态链待逐项 |
| 63 | CHARA_JOBCHANGE_ANIME | 人物／成长／礼物 | 仅声明；更新／绘制／动态链待逐项 |
| 64 | CHARA_PRESENT_ITEMSELECT | 人物／成长／礼物 | 仅声明；更新／绘制／动态链待逐项 |
| 65 | CHARA_PRESENT_ITEMSELECT_INFO | 人物／成长／礼物 | 仅声明；更新／绘制／动态链待逐项 |
| 66 | CHARA_PRESENT_ANIME | 人物／成长／礼物 | 仅声明；更新／绘制／动态链待逐项 |
| 67 | CHARA_PUTUP_BONUS | 人物／成长／礼物 | 仅声明；更新／绘制／动态链待逐项 |
| 68 | CHARA_EQUIP_EFFECT | 人物／成长／礼物 | 仅声明；更新／绘制／动态链待逐项 |
| 69 | CHARA_ITEM_EFFECT | 人物／成长／礼物 | 仅声明；更新／绘制／动态链待逐项 |
| 70 | CHARA_LVMAX | 人物／成长／礼物 | 仅声明；更新／绘制／动态链待逐项 |
| 71 | EQUIP_INFO | 装备详情 | 仅声明；更新／绘制／动态链待逐项 |
| 72 | EQUIP_INFO_FROM_TENANT | 装备详情 | 仅声明；更新／绘制／动态链待逐项 |
| 73 | EQUIP_INFO_FROM_PRESENT | 装备详情 | 仅声明；更新／绘制／动态链待逐项 |
| 74 | TENANT_INFO | 设施 | 设施详情第1页局部静态＋S043/S047/S054 |
| 75 | TENANT_ITEMSELECT | 设施 | 道具列表局部静态＋S044/S046/S055 |
| 76 | TENANT_ANIME1 | 设施 | 初始化／49自动转页静态，未认证连续演出 |
| 77 | TENANT_ANIME2 | 设施 | 49快进／55关闭静态＋S045结果角色 |
| 78 | TENANT_SERVICE | 设施 | 仅声明；更新／绘制／动态链待逐项 |
| 79 | TENANT_EQUIPLIST | 设施 | 仅声明；更新／绘制／动态链待逐项 |
| 80 | TENANT_RESIDENT_LIST | 设施 | 仅声明；更新／绘制／动态链待逐项 |
| 81 | TENANT_LVUP_ANIME | 设施 | 静态：[Init真正升级、40／55确认、独立frame2及完整局部Draw](STEAM_FACILITY_UPGRADE.md)已核；维护：Owner与数字动画／角色／框及触摸计划已接。动态：无连续原窗口认证；原字体、平台热区仍独立 |
| 82 | TENANT_STREN_BONUS | 设施 | 仅声明；更新／绘制／动态链待逐项 |
| 83 | SHOPPING_TOP | 商会 | 仅声明；更新／绘制／动态链待逐项 |
| 84 | SHOPPING_ITEMSELECT | 商会 | 仅声明；更新／绘制／动态链待逐项 |
| 85 | SHOPPING_TENANT | 商会 | 仅声明；更新／绘制／动态链待逐项 |
| 86 | SHOPPING_ACT_AFTER | 商会 | 仅声明；更新／绘制／动态链待逐项 |
| 87 | MEDAL_LIST | 授勋／怪物／住宅 | 仅声明；更新／绘制／动态链待逐项 |
| 88 | MEDAL_CELEMONY | 授勋／怪物／住宅 | 仅声明；更新／绘制／动态链待逐项 |
| 89 | MONSTER_NEWAPPEAR | 授勋／怪物／住宅 | 仅声明；更新／绘制／动态链待逐项 |
| 90 | MYHOME_TAX | 授勋／怪物／住宅 | 仅声明；更新／绘制／动态链待逐项 |
| 91 | NEWGAME | 新局／奖励／解锁／事件 | 静态：[NewGame／默认参数小方法](../rules/STARTUP_RECORDS.md#steam小方法交叉及设计决策)及[标题空栏入口](STEAM_TITLE_MENU.md)已核，完整Steam Update／Draw未核。维护：已有草稿／正式开始与APK图片计划；动态：空栏配置、默认联动／输入面板局部已见，未读运行页号；自定义保名／取消保留属用户批准策略，非Steam实测 |
| 92 | MONSTER_INFO | 新局／奖励／解锁／事件 | 仅声明；更新／绘制／动态链待逐项 |
| 93 | GET_OBJ | 新局／奖励／解锁／事件 | 仅声明；更新／绘制／动态链待逐项 |
| 94 | GET_OBJ_FROM_CHARA | 新局／奖励／解锁／事件 | 仅声明；更新／绘制／动态链待逐项 |
| 95 | GET_OBJ_FROM_AWARD | 新局／奖励／解锁／事件 | 仅声明；更新／绘制／动态链待逐项 |
| 96 | GET_MYHOME | 新局／奖励／解锁／事件 | 仅声明；更新／绘制／动态链待逐项 |
| 97 | TP_LOCKOFF | 新局／奖励／解锁／事件 | 仅声明；更新／绘制／动态链待逐项 |
| 98 | TAX_INCOME | 新局／奖励／解锁／事件 | 仅声明；更新／绘制／动态链待逐项 |
| 99 | BOSS_APPEAR | 新局／奖励／解锁／事件 | 仅声明；更新／绘制／动态链待逐项 |
| 100 | MANYMONSTER_APPEAR | 新局／奖励／解锁／事件 | 仅声明；更新／绘制／动态链待逐项 |

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
| 3 DLGSEL、10 LIST、11 SLIST | 同入口0x1023CA42；标记后确认 | 75及21行注册分别已核；21的Margin不同，不能混用全部输入政策 |
| 4 KEYCLICK、20 PLUS、21 MINUS | OnTouchKeyClick | raw9组件4/value22/flag2无矩形、34测宽底栏value20已核并有计划；不能推广全屏热区，其它参数仍逐页核 |
| 5 KEYPRESS | OnTouchKeyPress | 同上 |
| 6 KEYDOWN、7 KEYUP | 通用内联分支 | 位拆分及完整资格链待核 |
| 8 MAINMENU | OnTouchMainMenu | 完整菜单／解锁待核 |
| 12 VSCROLLBAR、14 VSCROLLBAR2、15 VSCROLLBAR3 | OnTouchVScrollBar | 8相对／9绝对滚动及35／37–39组件12注册已核，维护保留110／111差异；其它页注册及最终输入仍待核 |
| 13 VSCROLLBTN | OnTouchVScrollButton | ON上下边界已核，注册待核 |
| 16 UNMARKER | OnTouchUnMarker | 完整生命周期待核 |
| 17 CAMERA | GameForm.OnTouchEvent→OnTouchCamera | 主世界／建设局部已核；OS投递及全模式待核 |
| 19 TRACKBAR | 通用轨条分支 | 具体用途待核 |
| 22 OVER、24 RECT | 各自通用内联分支 | 用途及注册未知 |
| 23 ARROW_UD | 默认入口0x1023CA3B | 顶页／实际注册未知 |
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
