# 声音生产者：维护入队点与APK实际操作

2026-10-09。接续[声音合同](../../ui/AUDIO_REQUESTS.md)，只读逐项核维护声音入队点、底层来源、输出领取和仍未维护的原声音来源。没有C++／schema／原表改动，没有构建、播放、原窗口或存档操作。

EVIDENCE.json（本地核对材料，不随仓库交付）由audit.cjs（本地核对材料，不随仓库交付）登记所有现`prototype/src`的`state.sound_requests.push_back/insert`位置及源码哈希，并逐类关联APK具名窗口。它是来源审计，不把“查到整数”算作实际听见声音；现窗口仍消费后丢弃，CLI只打印。

## 操作分类必须来自调用者

| 记号 | 原调用 | 已核意义 |
| --- | --- | --- |
| B | `d/a.b(id)` | 同BGM已state2则忽略；否则先停止其它正在播放的BGM，再播放目标 |
| C | `d/a.c(id)` | 普通播放；正在播放同对象时可重头，不能统一去重 |
| D | `d/a.d(id)` | 先登记jingle／压低BGM，再普通播放；恢复由原Main完成状态驱动 |
| G | `d/a.g()` | 根据当前任务／遭遇选B(1)或B(2)，不是独立音频资源或地图刷新 |

BGM资源0…3和SE资源4…25只说明初始通道，不能替代B/C/D。尤其当前3来自B，20来自D，8在若干绘制／拾物来源来自C；“所有非BGM均普通SE”会丢失jingle压BGM。现`vector<int>`只保留ID，未保留这些操作资格。

## 当前每个入队来源的归属

以下行覆盖生产代码的全部整数队列写点。具体文件行号及哈希由EVIDENCE逐条冻结；表中用模块和动作保持可读性，不以行号推断职责。

| 维护生产者／触发 | 当前入队ID | APK真实调用及来源 | 当前输出含义／限制 |
| --- | --- | --- | --- |
| [runtime_pages](../../prototype/src/startup_world_runtime_pages.cpp:127) raw50庆典phase0、counter1 | 3 | B(3)，`b/g:4625–4641` | BGM幂等不能因再次见3而重头 |
| 同文件raw50最终确认 | 当前任务2，否则1 | G→B，`b/g:4639` | 先恢复，再退休50；不能先清任务再选曲 |
| 同文件`consume_award`消费`WorldAwardEffectKind::sound` | 当前只有3 | B(3)，`b/g:5909–5918`；底层`world_award_page:138` | 年度87每次counter1均会请求，原B包装忽略正在播的同曲 |
| 同文件年度87 `refresh` | 当前任务2，否则1 | G→B，`b/g:5921/5945` | 与事件22的先后依终止分支不同，见下节 |
| 同文件raw59解锁人物counter1 | 5 | D(5)，`b/g:4879–4884` | jingle，不是普通C5 |
| 同文件raw96住宅税收phase0 counter1 | 5 | D(5)，`b/g:6152–6158` | 仅该阶段首轮；不因显示税额每帧发声 |
| 同文件raw11事件消息command0／1首轮 | 4／6 | D(4)／D(6)，`b/g:11368–11377` | 其它command没有该声音，不由对话文本猜 |
| 同文件raw30成功成果phase0 counter1 | 4 | D(4)，`b/g:11921–11927` | 任务庆祝jingle；不是遭遇开始BGM2 |
| 同文件raw94／95礼物页结果的可选sound | 5 | D(5)，`b/g:6116–6122`，底层`world_gift_page:68` | 共用页初始化门槛，不因grant业务重复发声 |
| [village_activity](../../prototype/src/startup_world_village_activity.cpp:464) raw53 counter70 | 5 | D(5)，`b/g:4719–4725`及aW[1] | 发生于演出中途，不是52扣点即播放 |
| [facility_catalog](../../prototype/src/startup_world_facility_catalog.cpp:391) raw82 phase0 counter1 | 4 | D(4)，`b/g:5720–5723` | 不把三设施/商品数量当播放次数 |
| [building](../../prototype/src/startup_world_building.cpp:1055) raw81升级phase0 counter1 | 20 | D(20)，`b/g:5699–5702` | 升级jingle需压BGM；不得C20替代 |
| 同文件普通收费建设成功 | 11 | C(11)，`b/c:1736` | 建设事务内排输出，晚期失败不外发 |
| [editing](../../prototype/src/startup_world_editing.cpp:175)道路铺／除；移动确认；撤除／对应提交 | 11／21 | C(11/21)，`b/c:1786/1809/1824/1887` | 本文件3个入队点均C，其中道路点区分铺／除；不因光标预览播放 |
| [commerce](../../prototype/src/startup_world_commerce.cpp:343)成交后开86 | 25 | C(A[B])，`b/g:5839`；固定初始B0、A[0]=25 | 现实现固定初始B来源，不能将25猜成全部商会提示 |
| 同文件raw93领取页counter1 | 5 | D(5)，`b/g:6086–6092` | 和成交25的操作不同 |
| [runtime_tasks](../../prototype/src/startup_world_runtime_tasks.cpp:606)遭遇生命周期`refresh_global` | 当前任务2，否则1 | G→B，原恢复消费者 | 保留请求在clear_task后的原序，不用旧遭遇状态选2 |
| [runtime_deadline](../../prototype/src/startup_world_runtime_deadline.cpp:275)终止类型1且存在遭遇 | 1 | `c/n:4697–4711`清任务后G→B1 | 无遭遇分支不制造恢复请求；不是全类型取消均播1 |
| [application](../../prototype/src/startup_application.cpp:347)通关计分页最终确认 | 当前任务2，否则1 | `b/g:6894–6904`先退计分再G | 系统纪录／事件更新失败时候选不安装，声音不提前播放 |
| [runtime_scene](../../prototype/src/startup_world_runtime_scene.cpp:30)全局延迟效果Y出队，type4/5/6 | 17／18／19 | C(17/18/19)，`d/a.f:3993–4010` | **全局**效果，无人物bm屏内门槛，不与下一行合并 |
| [runtime_nonactors](../../prototype/src/startup_world_runtime_nonactors.cpp:122)共用人物声音出口，包含prefix effects、法术接触、命中、救援、死亡、控制33、施法 | 动态ID见下节 | `c/n.a(id,bm):806–810`屏内才C(id) | 共用1个入队点、多个具名生产路径；可见性门槛已经由Owner应用，不留给后端重算 |
| [runtime_scene](../../prototype/src/startup_world_runtime_scene.cpp:296)通知输出合并 | 11 | C(11)，`d/a.q:4234–4249`，`world_notices:26` | 通知计数恰1；insert仅转交候选声音，不能再次当新通知生产 |
| [presentation](../../prototype/src/startup_world_presentation.cpp:109)raw66赠礼显式表现准入counter45 | 8 | C(8)，`b/g:6516–6518` | 每次准入请求可重复，预览／声音暂停守卫独立；只重画冻结计划不得再次发声 |

**普通人物详情／转职／装备提交代码不直接意味着有声音**：本次扫描`startup_world_human.cpp`没有入队生产，声音来自其脚本插页或显式表现分支；不能看到“升级／授予”动作就补一个5。raw67／88年度人物结果没有本次新增入队源，不等于全部演出绝对无声；87背景音乐及插入消息另有来源。

## 共用人物C出口的逐路来源

现`emit_startup_world_actor_sound`先验证actor metadata，再用缓存`bm`与参考视窗判屏内，成立才入队；它保留的是普通C资格，绝非BGM/Jingle通用入口。

| 维护路由 | 原来源 | ID／时点 |
| --- | --- | --- |
| runtime `prefix_effects`→`actor_effects` | `c/b.d(int):5450–5469`延迟ce释放→`n.a(id,bm)` | 17/18/19；延迟到期，不靠渲染FPS |
| nonactors 法术目标visual≤6 | `c/b:4754–4779`→`n.a` | 16/17/18/19，参数visual+13；更高级的延迟子效果后续各自消费 |
| projectile_hit及近战contact的`attack_sound` | `c/b:1228–1248`→`n.a` | 人物武器种类14/15/12、怪物13；前一层还按攻击者旧o和当前镜头裁剪，后层读目标旧bm |
| actor `cached_sound`救援 | `c/b:5282–5295`→`n.a` | 7；成功绑定救援后，沿原缓存坐标 |
| actor lifecycle普通死亡 | `c/b:5095–5106`→`n.a` | 23；旧u投影bm后，取消死亡路径不套此音效 |
| actor `r.sound`控制33 | `c/b:3989–3998`→`n.a`；`world_misc_control:80–85` | 8；取当前缓存u和当前镜头投影，已经发物后的独立命令，不重复授予 |
| actor `cast_sound` | `c/b:4988–4997`→`n.a`；`combat_commit:289` | 10；施法真实阶段，不能与伤害时的17/18/19合并 |

它们可能在同轮连续产生同ID；原C允许同对象重头，typed交付不能按ID排序、去重或替换成集合。资源通道满时的原后端政策是另一级，不应删除Owner的真实请求来伪造“只听见一次”。

## 年度／庆典结束恢复音乐的原顺序

- raw50：最终确认达门槛→G选1/2→退休50。维护`consume_rank_celebration`同序。
- raw87用户确认提前结束：原先请求事件22，再G，再退休87；底层`WorldAwardAction::confirm_termination`效果序列为event22→refresh→close，Owner按序消费。
- raw87勋章耗尽：原先G，再事件22，再退休87；底层效果序列为refresh→event22→close。两个分支不可合并成同一事件先后。
- 年度counter1先B3再判断是否插事件23；后续恢复counter1可以再请求B3，声音后端需保留原同曲幂等。
- 类型1任务中止：原遭遇结算／地图清理→清h→G，因此恢复1。成功任务清理的`refresh_global`也必须在现有ordered requests的位置消费，不提前在胜利判定时替代。
- raw17通关计分最终结束：退计分→G→事件6→纪录事件4或5；当前应用在候选中完成关闭、入队和事件／系统提交。普通的“阶段加分”没有当前已核直接音效生产，不能自加每行叮声。

## 遭遇开始BGM2：已有规则候选，Owner漏接

原[c/b:359–369](../../work/decompiled/sources/c/b.java:359)仅在任务g原本为空、创建type2遭遇成功并写入配额后，先B(2)，再提示24；失败不产生两者，后续人物绑定已有g不重复播放。这个事实已见[遭遇合同](../../rules/ai/ENCOUNTERS.md#任务创建与影响场地图事务)。

维护[world_encounters:87](../../example/src/world_encounters.cpp:87)成功候选同时置`music2=notice24=true`，有两条载体：直接F经[world_daily:126–159](../../example/src/world_daily.cpp:126)保存完整`daily.task_entry`；路径P经[world_departure:798–802](../../example/src/world_departure.cpp:798)验证并传递到`daily.path`。随后[world_actor_routes:257–273](../../example/src/world_actor_routes.cpp:257)安装world/facts/task及event_requests、保留daily审计载荷；**生产代码中music2只在world_encounters／world_departure显式生产、校验、复制，没有prototype消费者或声音入队，两条daily载体也未消费这两个标记**。这不是原版没有战斗音乐，也不是另一个refresh1/2已经消费了创建事件。

`notice24`是**通知编号24“战斗任务开始”**，不是脚本事件24。原[d/a.b(int,String):3452](../../work/decompiled/sources/d/a.java:3452)取`c/n.az[24]=1`，交[d/a.a(int,String,int):1039](../../work/decompiled/sources/d/a.java:1039)，立即在通知S末尾追加`{24,-1,80}`和`ay[24]`本地化文本，参数null没有替换内容。它发生在成功配额写入及B2请求之后；现有script event24却是授勋相关事件，不能拿它来填补。

创建通知本身不立即播放11。后续`d/a.q`只轮到前两条通知、按逆序推进；该通知从-1→0时无声音，下一次获准推进到1才C11，超过80+16退休。前方有排队通知时不能换算成“创建后固定两帧”。已有`prepare_world_notices`和场景common_display_tail已维护通用消费，缺的是**这个创建时追加通知**。

本次不只搜索字段名：直接F的world_daily回写只含world/facts/task并保留task_entry；路径P只回写path状态，event_requests只追加实际event116；world_actor_routes随后消费event_requests但不投影创建提示。再核prototype所有通知追加点和原文本，未见新任务遭遇对应24或同文本的等价生产。这个限定链中应补通知生产，但不能把所有其它“任务开始”消息视为重复并删除。

单次`prepare_world_daily_c`按进入时旧状态switch：0只走路径P；5/11走直接F，二者在这一次调用中互斥。首个成功结果写回唯一Owner的`task.encounter`；同一外层frame的后续人物仍可能调用F，但[ai_perception:43–46](../../example/src/ai_perception.cpp:43)遇已有encounter只绑定，不请求新建，因此不应再发B2／notice24。规则定义允许之后退休并重新创建另一个真实遭遇；不能按“本帧见过2”或“本任务历史发过2”永久去重。

本批未修代码；后续应在一次成功的新遭遇Owner事务中消费实际创建路径的原顺序，覆盖直接F与路径P且不双发，重复F绑定不得发第二次。已有任务关闭／取消G不是这项请求，不能在那里补发2或用“进入任务状态就每帧播放2”的推断弥补。

## 成功提交、领取与仍未覆盖来源

现新增声效都先写局部候选Owner／规则候选；frame在[Session::update](../../prototype/src/startup_world_runtime.cpp:1157)完整准备与末尾render-cache校验成功后才安装。玩家动作也在完整候选中写入并于成功结尾提交；中途失败无候选输出。计分涉及系统文件时沿应用既有提交边界，本审计不新定义文件事务。

[Session::take_sound_requests](../../prototype/src/startup_world_runtime.cpp:993)通过swap一次领取；[Application](../../prototype/src/startup_application.cpp:280)只转发已有world队列，标题没有自己的声音队列。CLI调用后仅打印；`startup_view`预运行／经营循环领取后丢弃；没有音频设备播放认证。现[正常捕获边界](../../prototype/src/startup_world_persistence.cpp:122)要求队列为空，应用快照同样要求轮末已消费；typed请求不能无意改成保存平台播放器或未消费历史声音。

原已见但本次维护队列未接的来源还包括：

- 标题`b/h:298`的B(0)，世界初始化`b/c:1181`的G；不能因应用只在world存在时转发就称标题没有音乐。
- raw76道具投放绘制`b/g:7619`的C(8)、魔法壶原绘制`c/n:3060`的C(8)。当前显式表现声音生产仅有raw66，不能从已维护投放／魔法壶业务规则推断这些绘制声音已接。
- 原B包装隐含停止旧BGM、Android生命周期停止／恢复、资源退休及Steam明确StopBgm入口；当前整数队列没有独立停止操作。上面已归B的替换内部停止不应再额外发送重复stop；生命周期/Steam细节沿正式合同待继续，不能统一当静音或世界暂停。
- 输入反馈、平台UI声音和Unity设备后端本次未逐调用闭合，不用“无入队点”证明原始没有声音。

后续设计需要保留操作类别＋资源身份及现有请求原序、Origin准入与一次领取；是否新增枚举／结构、如何升级回放描述和应用标题输出由主会话统一设计，本分支不擅自编码。已有严格声音顺序／空输出断言应保留并扩展类别检查，不能删去以适配播放后端。

资源／输出消费：只新增中文报告、审计脚本与小型JSON，音频0新增、播放器0、后台任务0；全部原资源继续引用已发布音频包。JSON由本报告和正式合同消费；队列按消费时点清空，不宣称积压无限输入时天然有界，更不把账本／任务合法历史增长算声音泄漏。

本次静态核验83项、51个具名源码窗口、25个队列写点（B7／C9／D9）；EVIDENCE共25,864字节。两份文档38个本地链接及diff检查通过。复现：`node research/dungeon_village_1/tools/audio-producer-audit/audit.cjs`。这些检查不代替后续typed输出回归或设备播放验收。
