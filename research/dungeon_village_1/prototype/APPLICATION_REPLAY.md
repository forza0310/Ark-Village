# 完整应用研究回放

2026-10-09。应用语义6、标题菜单控制器与四槽文件视图已验，完整Release构建及五项相关检查通过103.30秒。当前使用`AVRAPP01`格式1／应用语义6／轮末边界1，内嵌系统版本2与世界语义3。旧应用1–5、旧系统1及旧标题控制器不迁移；声音批语义5的短前缀在当前代码中已退休，历史文件与证书保持原字节。当前420轮／首月及17命令文件操作尾段见[集中交付](../work/title-menu-application-delivery/README.md)，不等于完整Steam窗口／调度。

模块沿[应用快照方案](../stages/in-progress/APPLICATION_REPLAY_DESIGN.md)及[已确认的完整目录设计](../work/title-menu-application-design/README.md)。它是维护专用回放，不是APK／Steam原档兼容，也不扩大正常玩家存档的页面范围。存储事务另见[应用存储](APPLICATION_STORAGE.md)，应用行为见[标题与跨局纪录](STARTUP_APPLICATION.md)。

## 所有权与完整轮末

[接口](include/dungeon_village_prototype/startup_application_replay.hpp)提供保存、恢复和规范状态摘要；不开放任意字段setter。应用协调系统／文件／表现输出，Session仍是世界唯一Owner。恢复使用封闭私有候选构造，不执行普通初始化，不调用install_loaded重写世界镜像，不重抽随机、不发标题B0或激活G。

捕获要求健康应用、无执行中页面、应用音频和当前world音频均已消费，Driver也在完整外层轮末。当前维护状态完整保存：系统records、草稿、模式／页面、纪录名单与请求数、标题随机及历史交接、背景`l/f132f/s/t`及20槽全部字段、标题根模式／选择／稳定ID、raw20／询问／外部子页完整载荷，以及可选Session、全部原序审计历史和计分三元组。returned子页尚未被父消费时仍保存，不“修复”为初始菜单；inactive标题槽的age/y也保留。

应用输出仅允许空队列捕获，控制段显式保存已消费位；恢复队列为空。环境路径不保存，`cleanup_pending`不跨环境带入，新根恢复为false。storage的missing和digest是磁盘观察：发布system后重绑为missing=false及该原字节实际摘要；系统修订和内容保留。raw20目录stamp仍是必须验证的输入资格，不能省略。

具名菜单请求各自是事务；原标题背景更新仍有独立显式入口，不能把两次成功API称作同一原版输入轮。未闭合的Steam框架时序、纪录动画、完整皮肤和原自动中断轮内阶段并不因“完整应用”命名而自动获得实现。

## 格式与校验

头包含magic、格式／应用语义／捕获边界、dataset、world schema和应用schema。每段及全容器都有SHA-256，提供完整性校验而非防篡改身份认证。

| 分区 | 内容 |
| --- | --- |
| 1，必需 | Driver身份、producer、下一轮及下一命令 |
| 2，必需 | `AVRSYS01`版本2实际原字节，保留合法分区原序 |
| 3，必需 | 应用控制字段、496字节标题背景、完整菜单及空音频边界 |
| 4，world存在时必需 | `AVRSAVE1` replay完整Session及全部原序历史 |
| 5，必需 | 实际Driver规范状态字节 |
| 6，必需 | 四目录引用的唯一normal blob集合；无引用时也须有空段 |
| ≥1024，可选 | 原序、版本及任意字节保留 |

分区6按32字节摘要字典升序保存引用身份和原blob字节，不保存任意路径。集合必须恰好等于系统四目录的引用闭包，包括日期-1隐藏项；缺失、重复、额外、元数据冲突、用途错误或摘要不符均拒绝。正常世界仅临时解码验证，保留原blob字节，不把解码后已清输入的对象重编码覆盖原载荷。

整个容器128MiB，控制／Driver各1MiB，系统4MiB，可选段最多60且各1MiB，含可选world最多66段。活动Session、其历史和最多四个唯一blob共享原16Mi节点／128MiB估计分配预算，单Owner64MiB约束保持；不逐档重置预算、不删历史或隐藏记录凑通过。容器、分区和解码对象可共存，因此这不是进程RSS上限。

通用校验还核页面与菜单栈映射、world资格、raw20槽号与draft.slot、实际系统摘要／修订、稳定父子ID、返回结果、纪录名单资格、系统现金镜像、计分三元组、raw17栈顶及counter／phase、六行投影及累计。非法关系拒绝，不能通过推进一轮补状态。world激活后标题子页必须已退休；无计分控制器的raw17只允许未推进入口。

[字段清单](src/startup_application_replay_fields.json)由[Clang脚本](scripts/check_application_replay_fields.mjs)登记19个应用直接成员及43个嵌套字段，应用schema为`309d5698d851da2d505796fa258316ff3dd3c1a00b99c6b1ead44784266f5ec0`。嵌套覆盖标题背景、菜单各载荷、catalog stamp及storage观察字段；新成员未分类就失败。世界语义3／schema不因标题目录改变；其独立codec coverage继续负责世界嵌套字段。

## 新根恢复与失败边界

捕获输出必须位于指定的已存在研究目录且文件尚不存在。恢复源须是research/work内普通无硬链接文件；目标是其下**尚不存在的新应用根**，parent须已存在。路径检查复用存储层规则，保护当前应用整个根、源和调用方声明的trace／证书等路径，拒绝大小写别名、硬链接、符号／重解析点及Windows特殊名称。预检不建目录。

恢复顺序为完整读解／Driver校验、预分配最终观察值、固定父目录句柄、独占同卷临时根、写闭／刷新／回读system与全部blob、复核目标／源／保护路径、以Windows目录句柄无覆盖原子改名发布完整新根，最后仅noexcept联合安装应用与Driver。没有引用也发布空worlds目录。最终目录发布是`FileRenameInfo`且`ReplaceIfExists=false`，不是先exists再逐文件复制。其他平台尚无同等实现，明确拒绝。

发布前失败保留旧应用、Driver、源容器及竞争写者的既有目标。只清理本次登记文件和空临时目录，不递归删除未知内容；清理失败报告残留。成功后不再读取或分配才安装内存。正常单文件原子替换与本恢复整根发布是两个职责，不能用历史单文件测试替代本批目录发布验收；掉电耐久性也不等同于原子可见性。

## 摘要、Driver与验证归属

`startup_application_replay_digest`是纯内存计算，组合规范system、完整控制字段、包括隐藏项的按摘要去重完整引用身份、Session／历史、Driver及原序可选段。同摘要的长度／用途冲突拒绝；不读取路径、取得租约或创建锁文件，因而也可校验尚未发布的私有恢复候选。它不是容器文件hash或实时磁盘审计，不证明磁盘blob当前仍完整；真正save／restore另完整核实际字节及世界语义。逐轮不重复读取大blob；不可变历史沿弱引用摘要缓存，缓存不延寿退休历史。

实际save经完整decoder自检；restore逐层验证。Driver验证器必须纯检查，空验证器或空载荷拒绝；下一轮／命令、策略阶段和一次性输出由对应具名Driver证明，不能用恒成功回调替代策略认证。计分条件入口、自然经营和标题背景／菜单命令轨迹分开登记，条件中断档不能改称自然自动保存。

现有应用、持久化、codec和进程隔离套件集中覆盖标题／配置／纪录／计分及保留世界、raw20／询问／返回载荷、四槽隐藏引用、重签坏容器与坏blob、联合预算、临时根失败清理、双恢复和实际音频序列。新增测试代码存在与本批测试通过分开记录；当前集中验收中，尚不能把旧证书作为语义6通过结果。

历史按身份保留：[最初应用交付](../work/application-replay-delivery/README.md)、[标题背景交付](../work/title-owner-delivery/README.md)、[轮末缓存修正](../work/natural-application-delivery/README.md)、[12月性能采样](../work/natural-application-performance/README.md)、[初始化修正](../work/task-pool-delivery/README.md)、[声音Owner交付](../work/audio-owner-delivery/README.md)。旧自然12／24月与各代条件档不可换头接续；语义5的420轮／首月历史认证也不等于当前语义6认证。下一批沿当前代码重新生成最短前缀，避免重复从头长跑。

自然180月、五星／全部任务／BOSS、精确Steam完整皮肤仍须独立推进；计分条件短回放不证明自然通关。正常账本、任务历史、审计和外部快照允许合法增长，每批记录规模、引用退休与输出消费，不宣称永久有界。
