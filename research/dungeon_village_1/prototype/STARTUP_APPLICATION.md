# 标题、配置、跨局纪录与应用事务

2026-10-09。标题菜单控制器、系统2四目录、应用级声音及语义6回放已验，五项相关检查通过103.30秒，结果见[集中交付](../work/title-menu-application-delivery/README.md)。本模块沿[启动纪录设计](../stages/in-progress/STARTUP_RECORDS_DESIGN.md)及[已确认的标题目录方案](../work/title-menu-application-design/README.md)，原事实与Steam差异分别见[纪录来源](../rules/STARTUP_RECORDS.md)、[Steam标题](../ui/STEAM_TITLE_MENU.md)和[raw20／raw1](../ui/STEAM_SAVE_MENU.md)。

`dungeon_village_startup_application`是无raylib依赖的研究维护消费者，不是产品窗口或原版档兼容器。[StartupApplication](include/dungeon_village_prototype/startup_application.hpp)协调一份系统观察、标题状态、草稿、音频队列及至多一个世界Session，只向调用方公开只读世界。正常文件协议见[应用存储](APPLICATION_STORAGE.md)，完整回放见[应用回放](APPLICATION_REPLAY.md)，避免在本页复制其线格式和预算。

## 菜单与配置

[纯标题控制器](include/dungeon_village_prototype/startup_title_menu.hpp)保存根菜单／两栏模式、选择、稳定递增页ID和有限子页：raw20、默认否的询问、纪录／配置外部页。槽号唯一来自`draft.slot`；目录显示资格来自系统固定四项的日期，不检查世界文件存在性猜UI。

中断非空直接请求加载，空中断不能确认并归手动；手动非空打开“继续／重新开始／删除”，空手动进配置。重新开始和隐藏先经过默认否询问；答案先返回，父页通过`consume_return`实际消费，不能将“已回答”当“业务已提交”。否／取消留父选择，raw20取消覆盖旧result为-1；错页、错父、过期目录stamp、越界选择及未消费返回载荷的错误输入明确拒绝。

`apply_title_request(expected_page_id, request)`在候选应用中处理一次具名方向／确认／返回／ENTER／UP／返回消费／菜单frame请求，再执行必要存储意图并共同安装。纯控制器不读写磁盘、不给世界setter，也不发声音。实际加载或新局成功后退休标题子页；world→title再建立新根ID，不重用旧页身份。

`request_new_game`、`answer_overwrite`、`cancel_configuration`、`start_game`等便利接口沿同一控制器路径。重新开始“是”仅进配置，不提前覆盖档案；真正start在私有候选安装定义0资料与继承，仍不创建定义0实例、不改定义1首访。系统last_slot成功提交后才安装新世界；start本身不写world blob。

村名／姓名采用合法UTF-8、无控制字符、4096字节维护预算；原14／12参数和全角7／6限制尚未全部闭合。用户已授权维护草稿策略：custom_name后切性别保名，取消配置保留draft，重新进入继续编辑，实际start才安装。Steam“短姓名→切性别→取消重进”动态链仍未闭合，不能把维护决策补成原窗口事实。

## 系统、世界与显式刷新

两页纪录只读系统最高通关分／村名、限期最高资金／村名，不由当前所持金币推算。真实现金提交后的峰值变更同步系统；加载按系统跨局权威重绑世界纪录镜像，不把历史世界档当新纪录事件。维护选择峰值发生时立即保存，原AddMoney只改内存而不自行SaveSystem，两者分开。

应用路径现在只有显式`root`，系统版本2持有两槽×中断／手动四条目录，正常世界为内容摘要不可变文件。保存沿单system提交点；隐藏仅日期=-1、继续引用旧blob。系统修订／摘要与应用旧观察冲突时返回错误，不自动加载新档、不合并写者。`save_world()`显式保存当前手动项，`load_world(slot, kind)`确认后才验证实际载荷。

调用方可显式`refresh_title_storage()`接纳新系统：只在标题侧／无计分时执行，退休旧子页和返回值、分配新根ID，回到两栏。草稿、背景、随机和现有音频保留；保留世界仅重绑跨局资金纪录镜像，实体、资金、随机和历史不换成其他栏世界。失败不改变应用，成功也不重放旧失败意图或发B0／G。

普通更新与页面动作仍通过唯一世界Owner，包含Session的轮末`render_position/cached_screen_position`更新；缓存影响声音可见性和后续表现，不能用回放自洽掩盖漏接。`cached_view`沿自身既有逻辑尾部更新。

raw17真正出现后，应用只读六类计分并逐阶段推进。新纪录严格大于捕获最高分；收尾关闭17、请求主BGM、事件6及事件4／5，再提交系统并安装候选。失败保留页面／随机／事件／纪录，成功退休计分行、控制器和页ID，不能重复领奖。`act_award_page`、`return_rank_page`、`leave_commerce_page`沿真实栈顶87／48／83和稳定ID执行已有消费者，仍须区分条件接线与自然触发。

## 输出、随机与恢复

健康冷构造和world→title真正重入各入队一次`replace_bgm(0)`。纪录／配置／raw20／询问返回不重新初始化，不重复B0。真实start／load激活成功后，沿G的任务／遭遇状态追加B1或B2，排在该事务较早实际输出之后；失败不发，不按ID去重。首次B0尚未领取就start时，B0、G均按原序保留。

应用公开命令成功边界把当前world声音转入应用唯一typed队列，world当前队列归空，历史checkpoint原声音仍保存。`take_audio_requests()`与兼容ID接口领取的是同一队列，任一领取后另一接口不能再取到同一请求。记录operation＋ID；只比较ID会丢失播放、切换等语义。CLI逐命令／更新消费输出，捕获还要求Driver已消费；精确恢复不重发B0／G。

`logic`不推进标题背景／装饰随机。`title_presentation`通过独立显式`advance_title_background`推进APK的`l/f132f/s/t`与20槽，子页尚未退休时冻结父背景。状态和随机共同提交，任一抽取失败均不推进。只读人物投影给出原序shadow/body等计划，不自动绘制完整皮肤。

开纪录按p==1池交换、最多5人，空池按flags1后备；重画和翻页零抽。start通过handoff公开完整交接。world→title复制世界随机，重置q/l/s/t并保留f132f；配置／纪录返回保留背景。背景与菜单为分别具名的事务，Steam75绘制门槛不能覆盖APK100更新门槛，原完整更新／输入相位仍有未证项，不能把API调用数称为原逻辑tick。

独立标题内存控制器现为`startup-title-v3`：无世界时保存背景、菜单、草稿、页面、装饰、随机、请求和当前目录身份；恢复先核与当前应用的目录观察一致，不重新读取磁盘、不加载文件或发初始化输出。旧v1／v2拒绝。完整磁盘应用使用AVRAPP01语义6，19个直接成员及43个嵌套字段分类，另含完整四目录引用文件视图；恢复到尚不存在的新研究根后联合安装。旧应用1–5和系统1均拒绝，不改旧证书迁移。独立世界语义3／原字段schema保持，已推进raw17仍不能只用世界档从零恢复应用计分。

原自动中断产生者、raw14轮内保存、纪录动画完整调度、精确Steam窗口／字体／全部皮肤和自然通关尚未因此完成。正常世界保存仅支持稳定normal场景，应用快照不替它偷偷扩大资格。

## 有界CLI与交付边界

`work/release/bin/dungeon_village_startup_application_cli.exe --root <已存在研究根>`，可选`--seed`与`--title-presentation`。旧`--system/--slot0/--slot1`显式拒绝，不静默迁移；退出或EOF不自动保存。

基础命令保留new／overwrite／village／name／sex／cancel／start／records／next／previous／view／step／confirm／save／load／title／quit。菜单具名命令为left／right／up／down／activate／back、consume、frame-menu，冲突接纳使用refresh。`activate`操作标题菜单，`confirm`继续世界／计分页，两者不是同一入口。step每批最多10000次，逐轮领取并打印声音操作和编号，不实际播放音频。它用于审核维护操作次序，不证明Steam键鼠坐标和像素皮肤已还原。

当前实现及新增检查已集中通过，17命令文件操作与420轮／首月各自双恢复一致。历史[声音批](../work/audio-owner-delivery/README.md)的语义5短前缀保留历史资格，在新应用语义6下不可接续；当前自然前缀目录为`natural-application-menu-v1`，后期仍按[研究路线](../work/natural-application-route/README.md)独立认证。仅保留一份活动世界、退休菜单或逐轮消费声音都不保证永久有界；正常账本、任务历史、审计和磁盘候选规模按批记录。
