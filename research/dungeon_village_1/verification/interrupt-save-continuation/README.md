# 自动中断保存与恢复：下一批最小合同

2026-10-09。只读复核既有APK／Steam来源、维护调度和存取边界；没有重新解析25分区，没有读用户实时档、运行原游戏、修改Owner／格式或构建。本文是下一批预研，**没有批准或实现新的轮内恢复格式**。固定APK仍为世界规则基线，Steam独立交叉；系统2／应用6的当前交付另见[应用存储](../../prototype/APPLICATION_STORAGE.md)和[应用回放](../../prototype/APPLICATION_REPLAY.md)。

## 结论与最小范围

下一批可先补“原时点产生中断候选、系统四目录提交、读档重新激活”这一条链；不能把已支持的中断行normal条件文件改名为自然自动保存。现有日历审计检查点已处于正确原始保存位置，但不是可直接继续完整外层轮的Session，也不代表文件已经写入。

**普通玩家中断恢复与精确轮内回放需要不同合同。** 若明确采用原版重入主场景的恢复行为，不需要把调用栈／外层剩余轮数伪装为原档字段；若要求从保存调用之后精确续算，则必须显式保存和消费尚未执行的调度阶段。现有`processing_phase=1`／`processing_subphase=0`不足以表达它。建议先落实原时点自动候选与正常重新激活，精确回放继续使用已经实现的完整外层轮末应用快照；不要为了自动栏加载先引入通用轮内回放框架。

自动候选与当前normal边界的关系、失败如何共同回滚当前更新、多个内部轮的候选如何提交仍需具体实现设计，本文只列约束和最小验证。用户已批准的完整标题目录方案明确把原自动产生者／raw14轮内保存留作后续，不能借本静态预研直接改格式。

## 已证的产生时点

| 来源 | 位置与已证行为 |
| --- | --- |
| APK日期推进 | [b/c.java](../../work/decompiled/sources/b/c.java:149)的`c(int)`：先oldtime=time，试加advance；到10800时暂将time还原oldtime，调用`av.a(true)`，再恢复试加值；随后才UserData.t++及周／月／年归一化 |
| APK世界先行 | [低层MainScene](../../work/construction-render-fallback/MainScene.java)的`b()`：L29d共同世界更新，L2df显示／菜单尾部，Leb0在scene0才调日期；详见[APK日历复核](../../work/restore-behavior-analysis/apk-calendar/README.md) |
| Steam世界先行 | `GameForm._update` RVA0x303430：VA0x103052E0调用UserData.Update；0x10305BA2才调用ProgressTime；普通尾部资格成立才到达，不是每次外层Update必定走日历 |
| Steam跨界保存 | `GameForm.ProgressTime` RVA0x2FD080：0x102FD247旧值，0x102FD2A6阈值，0x102FD309还原，0x102FD31C调用SaveGame(true)，0x102FD34F恢复试加，0x102FD360才monthCnt++ |
| 多内部轮 | Steam外层Update RVA0x301010在0x103017B4循环调用_update(frameCount)，可提前停；APK维护普通加速为两内部轮，不能只记录“这一外层轮需要保存”就丢掉先后顺序 |

Steam指令、原方法hash及范围复用[日历机器码证据](../../work/restore-behavior-analysis/steam-calendar/README.md)、[反汇编](../../work/restore-behavior-analysis/steam-calendar/calendar-disassembly.txt)。本次没有新增反汇编或用截图推调用计数。

10800是逻辑进度，非秒。固定APK默认advance为27；Steam`.cctor`的DAY_SECOND1=80及FRAME_TIME1=43200/(80×20)=27已在[正式合同](../../rules/PERSISTENCE.md#回到完整更新入口不是回到原保存调用之后)定位，但全部运行期赋值未穷尽，不能无条件套到所有Steam配置。

因此中断档含“本内部轮人物／怪物／设施等已推进，日历还在旧日期／旧time”的状态。它不含这一轮调用阶段、剩余内部轮数或框架输入已消费位置。不能由再次调用world update断言某一笔收费必然重复；业务动作计数已保存，各消费者可能自行判断资格。日历消费者尚未执行，更不能把读档后第一次跨周叫作必然重复结算。

## 保存内容与恢复激活

25分区已完成范围不重做，直接复用[存档合同](../../rules/PERSISTENCE.md)。需要在自动路径维持的边界是：延迟脚本opcode6续体保存程序身份、下一指令、等待值、上下文和参数；人物FIFO、状态／动作计数、连续坐标，物体／投射物进度会保存。原页面栈、页面选择／问答／局部计数、通知／显示队列和即时声音不保存。APK共同Random seed／游标没有持久化，同进程延用流、冷进程重新初始化；不能为原版补写固定seed。

原加载不是精确恢复调用游标：APK标题加载后切主场景并初始化scene0，下一次合格更新重新走脚本→世界→显示／菜单→日历。Steam `TitleForm.ContinueGame`在0x1020A4EE调用LoadGame、0x1020A514切GameForm；LoadGame先NewGame和写系统选槽，再读游戏，失败不具备维护候选回滚。

本次只读复核已存在的Steam `GameForm.Init`完整1200字节（RVA0x2FA820，方法hash `70ea02b609128bee3a7a94dfca18417cf05bd0fd0b8b8e47b7a94a18d8ac30ca`）：

1. 重建主场景临时容器／菜单、资源和视图；VA0x102FAA25调用`ChangeState(0)`，其已核入口将scene计数清0。
2. 重建显示向量后，VA0x102FABF1调用`AppData.PlayBgmMain()`（G，RVA0x25F620）。因此激活G在场景重置及资源／临时视图之后；不是LoadGame刚返回就从旧页面续发。
3. 后续VA0x102FAC4C检查事件210，0x102FAC8D可能将系统int[16]置0。metadata明确int[16]是`SYSSAVEI_REVIEW`，**不是11／12两条中断日期**；不能把此清零称作中断目录清除。

APK [b/c.a()](../../work/decompiled/sources/b/c.java:1160)同样在`a(0)`与视图重建之后调用`av.g()`；[d/a.g()](../../work/decompiled/sources/d/a.java:4154)按活动任务且遭遇非空选B2，否则B1。两边方法身份和调用顺序独立登记，不把Steam Init的额外review逻辑回填APK。

现应用已消费G并将当前world音频转入应用单队列，真实start／load成功才发；精确应用恢复不调用Init或G。这部分可复用，无需第二队列或用B1替换所有旧输出。首次自然自动恢复是否触发异常／迁移／年月纠正分支仍应按具体样本验证；Steam tpClear年／月修正已知，不等于time被重置。

## 单槽／多槽与目录清除：已证与未知

固定APK与当前Steam标题均有两槽×中断／手动四目录。保存写选定游戏后，只更新选定日期／资金／村名再保存系统；APK写者在[d/a.java](../../work/decompiled/sources/d/a.java:3300)，索引分别为自动`11+slot`、手动`6+slot`。标题删除分支仅将当前所选日期置-1，未擦除载荷，见[b/h.java](../../work/decompiled/sources/b/h.java:528)及[Steam标题合同](../../ui/STEAM_TITLE_MENU.md)。

本次没有找到足以认证“读取中断即清该栏”“手动保存同时清当前中断”“单槽清一条而多槽清全部”的具体写者；已复核GameForm.Init的int[16]修改不能支持它们。也不能凭当前两槽UI推断通用单槽平台政策。后续只应定向跟踪**已定位的LoadGame、SaveGame、GameForm.Init、TitleForm继续／删除**对sys.int[6/7/11/12]的实际写及条件，必要再查明确常量分支，不做无界全镜像扫描。

在缺该证据前，维护当前隐藏／保存分别只动一目录的已确认合同继续保持，不擅自新加“读后删除自动档”。原SaveAll另有true→false→system顺序，不能由“写过手动”推定自动目录必然消失。

## 可复用维护边界与缺口

| 当前组件 | 可复用内容 | 不能据此声称 |
| --- | --- | --- |
| [world_calendar.cpp](../../example/src/world_calendar.cpp) | `checkpoint_before_normalize`在旧time／旧日期、month_ticks递增之前；失败返回候选失败 | 已写出自然中断文件 |
| [world_scene.cpp](../../example/src/world_scene.cpp) | normal scripts／delayed／world／input及显示尾部后才调日历；保留实际begun_rounds | 已持久化当前round索引、scheduled_rounds和剩余日历阶段 |
| [startup_world_runtime.cpp](../../prototype/src/startup_world_runtime.cpp:1140) | 检查点callback设置save_marker=1，保留完整不可变Owner审计，临时去掉executing_page | 检查点可作活动Session从外层入口恢复；此时simulation_steps及轮末render cache尚未最终提交 |
| [正常存取](../../prototype/PERSISTENCE.md) | 严格world校验、normal主场景、维护随机、输入复采集及候选加载 | 可直接保存原轮内候选；已有声音／模态／调度资格可能不满足normal |
| [应用存储](../../prototype/APPLICATION_STORAGE.md) | 两槽四目录、不可变blob、短租约、单system提交点、旧引用退休与隐藏保留 | 原自动触发者存在；normal用途即可解释新轮内语义 |
| [应用回放](../../prototype/APPLICATION_REPLAY.md) | 完整外层轮末、实际Driver、共享预算及四目录文件视图 | 任意轮内恢复或原APK／EXE档导入 |

自动候选应由唯一Owner在原检查点位置产生，只含已完成副作用；外层协调文件提交。不能在纯日历消费者里直接写文件后又让后半轮失败，把旧内存与已发布目录拆开。最小设计须先决定：整个外层候选完成后才发布当时捕获字节，还是将自动保存边界提升成可提交的独立事务；两者对失败语义和多内部轮有不同影响。原版逐文件非事务行为不是照抄的理由。

若下一批要精确轮内继续，最小续体至少说明：捕获位置、原advance／待归一化进度、当前内部轮／剩余轮、哪些脚本／世界／显示已完成、calendar后剩余消费者、输入和一次性输出消费边界、simulation_steps／缓存最终收尾资格；这些在现有场景字段里没有完整表示，不能只把`processing_phase`从1改-1骗过读器。

## 后续集中验证与证据限制

优先用已认证外层轮末短前缀，自然运行至一次跨周：核自动候选旧time==oldtime、世界确已推进、日历未消费、只有当前自动目录变化；同时核相邻不跨界轮不会重复产生请求。覆盖加速两内部轮、脚本／模态阻断、磁盘冲突／写失败、重试、最后候选覆盖顺序和无多次声音。原重入读取与精确尾段回放分别报告；normal条件中断不能升级自然等级。

Steam非空p恢复及10773/10773样本已经有真实端点证据，见[非空中断恢复](../../work/nonempty-p-reload-analysis/ANALYSIS.md)和[首次窗口分析](../../work/window-save-analysis/ANALYSIS.md)。它们不含首帧全部调用、随机状态或确切保存次数，没必要重复制造相同非空p样本来回答调度缺口。原窗口若必要，另输出最小提示词，不在本批启动。

另一个自动槽产生入口是Steam预留保存：`FormManagerBase._execute`尾部→预留分派→KairoService.SaveAll，已证true／false／system及三次异常尝试，见[正向上游](../../work/saveall-upstream/README.md)。全部请求产生者及退出／失焦时是否还有消费机会未闭合；“中断槽”不单独证明10800来源，也不能许诺退出必存。

本批只新增本README；无原档副本、素材、缓存、后台／原游戏进程。链接与文件规模由同步检查登记；不改旧历史或当前验收状态。资源审计继续分正常历史增长、隐藏引用、退休页／输出及失败临时文件，不宣称永久有界。
