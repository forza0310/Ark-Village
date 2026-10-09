# 完整应用研究回放

2026-10-09最新初始化修正已通过12项相关CTest与420轮／首月双恢复：补齐六特殊／三十复发任务目录、实际reset后的设施共享人气20／30。现世界语义2、应用语义4，旧应用1／2／3与世界语义1拒绝、不迁移，原表及Owner字段布局不变。此前自然24月等快照保留修前证据，36月续跑已在29月中止并收齐研究测试进程；当前最终前缀为natural-application-v4。详情见[初始化交付](../work/task-pool-delivery/README.md)，不将日期回放自洽代替后期规则正确。

本模块实现[2026-10-09已确认方案](../stages/in-progress/APPLICATION_REPLAY_DESIGN.md)，用于保存应用内存、完整世界Session与实际测试Driver。它是研究专用`AVRAPP01`，不兼容APK／Steam原档，不扩大正常玩家存档的页面范围，不包含外部两栏文件系统镜像。

## 所有权与捕获边界

应用仍是跨域协调者，Session仍是世界唯一Owner。新增[接口](include/dungeon_village_prototype/startup_application_replay.hpp)只提供捕获、恢复和完整状态摘要；不暴露任意字段setter。恢复通过封闭私有候选构造，不执行普通构造的系统读取，不调用install_loaded重写世界现金镜像，也不推进页面／重抽随机。

捕获在完整研究外层轮末，声音和Driver一次性输出必须已经消费，`scripts.executing_page`为空。快照保存系统副本、草稿、模式／页面、原序纪录装饰名单、请求序号、完整标题随机／历史交接、背景`l/f132f/s/t`及20槽全部六字段、可选世界及其全部原序审计历史，以及计分六行／完整阶段状态／稳定页ID。退休槽age/y仍保存，恢复不重置或补更新；logic模式只接受初始背景。路径重新绑定，构造错误只接受为空。

标题背景通过Owner显式请求与随机共同提交；开纪录和新局仍为独立菜单API，文件记录的是实际已完成请求边界，不替它们提供同一原版输入轮的联合事务。`h/j/o`、raw20／91返回载荷、方向输入及纪录动画`f110a`尚未实现，不在当前载荷中。“完整应用”指完整保存当前维护应用，不能解释为原版所有UI状态已复刻。

`StartupApplicationReplayMetadata.controller_state`是外部Driver的唯一规范状态字节，不能在恢复之后再靠旧局部变量继续策略。调用者必须提供纯验证器，完整解码、检查语义版本、页面关系、下一轮／命令与输出消费；空验证器或空载荷拒绝。库检查通用封装与应用关系，具体Driver负责已登记场景语义，不能传一个始终成功的回调宣称已验证策略。

计分Driver为`application-clear-conditions-v1`，资格明确是raw17条件入口。它记录实际阶段、下一轮／命令、页ID、捕获纪录、随机／历史基线、事件4／5／6、声音序列、失败／成功及检查计数。非首行row2计数44→45使用无确认更新，避免同一脉冲直接转到下一阶段；stage6计数64下一次更新自动结算。文件锁故障是测试环境，不保存OS句柄。另有已实现的`application-natural-clear-v1`被动自然Driver，其观察范围、输出预算及语义修正见[自然路线记录](../work/natural-application-route/README.md)；修后自然前缀待重新认证，条件档不能换标签当自然通关档。

## 格式和严格校验

头含magic、格式／应用语义／捕获边界版本、dataset、world schema和应用字段schema。当前magic仍为`AVRAPP01`，格式1、应用语义4、捕获边界1；旧应用语义1／2／3明确拒绝，不迁移。分区1为metadata，2为原AVRSYS01完整字节，3为应用控制字段，4仅在world存在时保存原AVRSAVE1 replay完整字节，5为Driver；ID≥1024是可选附属段，保留原序、版本和任意字节。语义2曾在控制分区末尾追加496字节标题背景（4个int32及20×6个int32）；语义3保留这份字段布局与偏移。每段和全文件均有SHA-256；摘要仅校验完整性。

语义3修正应用普通`update()`漏接Session轮末`update_startup_world_render_cache`的问题：现在同样先更新候选人物`render_position/cached_screen_position`，成功才联合提交世界与系统。两个缓存影响后续声音可见性及表现，不能仅因字段／world schema未变就沿用旧应用前缀。此修正不把`cached_view`误作漏更新字段；缓存函数本身不额外抽随机，原规则及预算保持。旧文件和证书保持原字节，不补写缓存或偷改版本头迁移。

整体128MiB、应用控制／Driver各1MiB、系统4MiB、可选段每段1MiB且最多60段。world与全部历史共用原16Mi节点／128MiB估计分配预算，不按分区重置，也不删历史。读时容器、分区字节与对象可共存，这些阈值不是整个进程RSS上限。

逐项验证页面与world存在资格、纪录名单与实际human_presence池、系统现金镜像、计分三元组成组、raw17栈顶／counter／phase一致、六行仍等于世界只读投影、累计与已计行关系、捕获最高分与系统相等。恢复复用只读`validate_startup_clear_score_page`，不通过推进一次来验证。无控制器的raw17只允许未推进入口；finished控制器不能保留并再领奖。

[应用字段清单](src/startup_application_replay_fields.json)由[Clang检查](scripts/check_application_replay_fields.mjs)登记全部16个直接成员及11个标题嵌套字段（`StartupTitleSlot`六项、`StartupTitlePresentation`五项），当前schema为`77fcbdfefcb6e2ad562e95d081c6d9ba3a086612dab44aa49d72faca9dae84c2`。此次schema身份随应用语义4更新，成员数量和布局不变。两次AST过滤分别核应用成员与标题类型，新外层或嵌套字段未分类均失败，不只比较`title_`类型名；其它嵌套字段沿具名codec及world/system身份约束。既有world codec coverage同时执行此检查；world schema仍为a1ca开头的既有身份。该检查不证明广告、完整标题菜单或未来UI状态已实现。

## 文件隔离和联合安装

应用库编译绑定本研究包的`work`目录；调用者另传其下已存在的专用研究目录。路径检查规范父组件、大小写／硬链接别名、符号／重解析点及Windows特殊名称，拒绝越界和与当前应用文件重叠。`protected_paths`可声明trace、证书和其它只读输入；它不进入快照。纯预检不创建目录或文件。

捕获目标必须不存在。恢复的固定`system.avr`、`world0.avr`、`world1.avr`也必须全部不存在；目录内无关文件保留。读容器→完整候选及Driver校验→准备可无异常移动的最终对象→再次检查路径→无覆盖发布一份系统文件→noexcept联合安装应用和Driver。任何发布前错误保留旧应用、Driver及源文件；恢复不写两栏世界文件。

无覆盖发布复用独占临时文件、刷新和回读校验。Windows最终MoveFileExW不带REPLACE_EXISTING；POSIX采用同目录link发布，目标存在即失败。该出口与正常单文件替换职责分开，禁止用exists预检代替最终无覆盖语义。实际Windows双写者竞争已测试；POSIX代码尚未在本机执行。路径预检不声称能抵御恶意进程并发替换父目录，掉电耐久性也不等于文件替换原子性。

## 摘要、复算与适用范围

`startup_application_replay_digest`比较完整规范状态，组合全部应用控制／系统／Driver及原序扩展和完整Session摘要；它不是容器字节hash，也不以摘要成功代替完整恢复校验或128MiB文件预算通过。每轮无需重复构造并解码磁盘容器；真实save仍执行同一decoder自检，restore仍完整校验。Session摘要复用不可变历史的弱引用缓存，退休引用不会因缓存延寿。

扩展现有应用、持久化、codec覆盖和进程隔离套件，不新增CTest或逐函数target。标准条件回放覆盖计分前后、结算失败／重试、等分不换纪录主人和一次收尾；四个捕获点各用两次新进程比较完整应用、Session历史、Driver、实际声音／事件与隔离系统摘要。标题／配置／纪录及保留世界的应用状态另有独立往返与损坏拒绝检查。

最终验收、复用候选和规模见[验证入口](../VERIFICATION.md)。语义1的四个条件快照位于[历史证书目录](../work/snapshots/application-clear-v1/CERTIFICATE.json)，当时9文件1395611字节；当时结算前64档只需2次请求，失败后档只需1次重试。旧语义1／2字节不能由当前语义3解码，须由当前代码重新生成并认证，不改旧证书伪装兼容。全套生成／验证可在既有进程runner显式增加`--application-exe`和`--application-snapshot-directory`（后者必须为work内的新目录），默认CTest仍回收临时制品。上批命令与证书摘要见[历史交付记录](../work/application-replay-delivery/README.md)。这些本地二进制条件档不提交为玩家存档，本页不代替本批最终回归记录。

标题批次曾生成并认证语义2的四个计分条件档，见[语义2历史证书](../work/snapshots/application-clear-v2/CERTIFICATE.json)，9文件1397567字节。原标题独立Driver `title-background-requests-v2`当时也认证600→680请求尾段，两次新进程恢复与reference一致，完整保留20槽、实际确认消费／抽数及绘制投影；这些是对应版本的历史结果，见[标题批次交付](../work/title-owner-delivery/README.md)。当前四个条件档已在`work/snapshots/application-clear-v3`重生成并认证；历史证书不改写。

`work/snapshots/natural-application-v1`下420轮、首月和第12月三份前缀均产生于轮末缓存修正前。它们曾完成各自20轮三路一致，但现在只保留为修前诊断，不是修后可复用的当前自然档。修后自然候选目录为`work/snapshots/natural-application-v2`，尚待重新生成和认证；不从旧档补缓存续跑来代替自然来源。

自然180月、五星／全部任务／BOSS、完整原标题输入和完整Steam皮肤仍独立推进。标题背景Owner与快照已接线，完整人物资源展开和纪录动画仍有缺口；条件短回放不能换标签成为自然通关证明。

当前语义3已在[本批交付](../work/natural-application-delivery/README.md)认证：`natural-application-v2`内420轮和首月两档各20轮双恢复，`application-clear-v3`内四个计分条件档重新认证。自然第12月仅有修前诊断，当前后期路线仍待推进；新Driver与原独立world策略分别登记。

后继[等价窄投影与12月采样](../work/natural-application-performance/README.md)已认证当前语义3的`natural-application-v2/month12.avra`：19295→19315三路，19,662,928字节。此更新取代上一批“当前仅早期”的状态；旧语义2诊断继续保留，不重命名为修后档。下一步先采24月预算，schema不变。
