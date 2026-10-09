# 应用主动首星Driver：本批具体方案

2026-10-10。接线checkpoint `01f5996`之后，按用户持续授权实现独立 `application-active-progression-v1`，不是被动日期Driver改名。源码位于既有prototype测试包，复用同一persistence可执行；不新增target、容器、业务持久字段或预算。世界4／应用7及全部原表保持。

每轮先一次真实应用更新，再按旧`natural_progression`的优先级处理当前栈顶：升级→面包房35目录与合法位置→可支付村办→可支付任务。建设与51选择可包含有限次相邻真实命令，完整记录每一次输入；场景无动作也写wait。原等待/自动页继续等待，未知页面立即失败，不能generic确认购买或任意跳页。原任务结束后保留完整营业恢复月；下一任务资格仍由实际日期、人物/任务Owner决定。年度沿已有实际授勋命令，不混用被动终止。

首星以后优先开展真实解锁活动16。真正terminal要求：真实面包房、首星四条件成立并晋级、至少一次自然设施升级、活动16实际完成，且到后续自然月份产生新设施收入并回到主场景。任务接受/出发/成功分别记录，未取得的自然BOSS／二星不因terminal被宣称完成。420→440是有界早期回放，可正常结束但必须报告`terminal=false`。

2026-10-10新月收入门槛纠正：此前“跨活动完成月＋累计设施收入高于活动完成时”可能由前月已增长的收入满足，不能证明新月份营业。`step`和terminal validator保留上述条件，再共同要求权威`monthly_cash[当前原月][facilities][income] > 0`；无新增Driver字段、Owner或预算。月份不在0–11时显式拒绝，诊断`progress.current_month_facility_income`给出真实当月值。原34419证书仍保留其旧terminal条件和三路一致事实，不能据此补宣称新门槛已过；由新的真实尾段认证证明新月份收入。现有Driver检查增加累计已涨／当月仍0的条件反例及坏月份拒绝，不把该条件夹具当自然经营；正向由真实尾段覆盖。

所有跨轮Driver字段编码到既有opaque `controller_state`：seed1／speed0／策略边界、下一轮与命令、面包房身份、升级页去重、活动重试轮／完成计数、任务冷却月／旧任务身份、晋级/活动16月份和收入基准、日期/随机/历史/步骤、栈顶及下一策略、音频累计摘要、资源峰值。读取拒绝截断、尾段、非法集合、下一步与实际Owner不符、未消费输出。没有指针、路径或替代的业务字段。

CLI：`EXE application-active-progression-v1 --work-dir DIR --trace-file FILE --stop-at N`；可带`--save-file FILE --save-at 420 --tail-after-save 20`或`--load-file FILE`，可用`--trace-from`限定非捕获观察。现只按显式轮捕获，不加按星级/月份参数。每次目录独占、文件create-only，所有恢复目标沿现应用路径审查；快照在本轮命令、音频和检查完成后捕获。

`--producer-revision`默认`unspecified`，记录本次生成实现，必须与证书/metadata相同；它不承担策略版本身份。恢复先按源原producer完整验证，随后以本次CLI值登记后继证据；reference与双恢复传同一当前值。策略仍由固定Controller和Driver版本1决定。新增Driver检查沿现persistence套件验证截断、下一轮/命令、页界、缺失面包房、错误下一策略、溢出命令总数及来源身份拒绝；未新增target。

trace每行记录`frame/next_frame/next_command`、完整`commands`五整数数组`[命令ID,page,arg0,arg1,arg2]`、Driver规范字节hex及其SHA-256、完整应用digest、实际系统文件digest与typed声音。wait为`[0,0,0,0,0]`，不增加命令序号；非wait每次已完成命令都增加。完整应用digest已覆盖世界和包括隐藏项的文件引用，不重复逐帧解码全部blob。

正式冻结前审阅修正：原输入ID0–13保持，新增14明确表示`leave_commerce_page`，不再与ID9的通用`cancel_page`混写。Driver命令计数数组同步增加该槽，坏载荷检查覆盖新槽超出next_command的拒绝。之前尚未提交的420初验证书对应旧草稿布局，仅保留draft历史，不能接续；本次不改已发布world4/app7身份。raw83条件输入映射由最小源路径审阅覆盖，未为此注入商会页面或宣称自然到访。

页面生命周期审阅修正：52确认刚建53时只绑定活动，没有初始化计数；Driver只对`life0的栈顶53←刚退休且已初始化的52←选中同活动且answer0的活51`合法链返回wait，另核活动定义与开展记录。已初始化53仍必须通过完整inspect；缺counter/binding不能伪装等待。最小条件检查仅设20点／3次，由真实Owner创建51、经过说明、选23、确认52、再框架初始化53；不直接造页，不当自然进度。另覆盖错父／缺binding／initialized缺counter拒绝。

51／52／81的计划阶段不读取缺省payload，真正动作在下一次应用update之后执行，未增加提前初始化。28不同：`startup_world_runtime_task_pages.cpp:commit`在25→28返回前已经写phase／prediction／acceleration，原调用没有缺phase窗口；Driver改显式find与范围检查，缺项拒绝，不靠map.at异常或补默认0。

集中套件重跑修正：Driver检查使用规范测试父目录下独占时间戳子目录，成功仅清理本轮树，失败打印并保留准确路径。不会删除旧固定`active-driver-contract`或任何先前失败目录；新增53条件链只使用这一次Owner内存，没有第二套固定输出目录。此项修复测试工作区生命周期，不改变业务断言。

2026-10-10月度/终点观察增量：月日志增加`progress={...}`，summary增加同名对象，列当前现金、人气、最高月收入、点数、实际活动F、季度次数、任务成功数、kind3/9设施数及kind12住宅数。两种计数复用`prepare_world_rank_status`第二星条件模板，只投影其所读字段，不复制统计算法或全Session。只在月变/打印终点时查询，未写Driver、rank缓存、世界、随机或音频，不改变trace和已有前缀身份；旧证书缺这些附加诊断不意味着其快照语义失效。新报告三路对比该对象，不从旧报告补造缺失数字。

2026-10-10首次任务展示页准入：20007轮实际raw100曾被严格未知页守卫挡住；按[任务展示合同](../../rules/ai/DUNGEONS.md#演出与成果后的阻塞点)和[页面合同](../../ui/PAGES.md)将99/100明确纳入`acknowledge`白名单。仍每轮先应用update再确认；原`consume_task_display`负责初始化身份、共享8×9表及共同随机，100首次E合计19抽，F只由update推进，99不运行E/F。若本轮刚插新页，首次真实confirm可完成源允许的初始化；同页后续确认不重初始化、不重跑F、不额外抽19。早确认保持原计数，至少40次页面更新后确认才关闭；Driver不自己快进、不改计数，也不加入额外音效。

本改动不扩未知页通用准入，未改Driver字段/格式。可从已保存20000前缀认证20001起含首次raw100的短尾段，不重跑自然前缀。已有`dungeon_village_prototype.startup_world_pages`的`task_display`覆盖真实怪物绑定缺失拒绝、首次19抽、重复早确认无额外抽取、40轮世界/日历冻结及关闭；`dungeon_village_reference.world_task_display`覆盖纯领域随机/溢出。新增风险限于应用Driver接线，由既有`startup_world_replay_process`及20000来源短三路重放覆盖，不重复建立新测试target。以上是本次修复与验收建议，不提前登记通过。

summary前缀`application-active-summary`，记录controller、capture_frame/capture_rank、完成轮/下一命令、rank/months/date、digest、sound_count/hash、random、资源与峰值、phase/terminal、任务/活动/升级计数及保存恢复耗时。跨进程runner独立验证420捕获、reference继续440和双恢复同尾段；本文件记录方案，不预报验证结果。原世界Driver和历史黄金不修改。

2026-10-10后继从已认证2000轮继续时，在7348轮／自然4月遇到实际任务成果raw30，按未知页边界停止，原失败现场保留。复核[任务报告合同](../../ui/TASK_REPORT_RENDER.md#2-任务完成raw30两阶段胜利页)与Owner的30／31／32具名消费者后，补这三页的确认策略：30早确认到40、阶段0转1、阶段1关闭；31初始化统计后关闭；32关闭既有摘要。奖励在任务收尾已提交，不借确认重复发奖，不使用任意raw页兜底。沿旧自然世界策略的实际成果确认，不改之前420／2000已发生的命令、Driver布局、世界或应用语义；新段须重新认证并登记本次producer。
