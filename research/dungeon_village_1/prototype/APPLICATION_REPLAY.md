# 完整应用研究回放

本模块实现[2026-10-09已确认方案](../stages/in-progress/APPLICATION_REPLAY_DESIGN.md)，用于保存应用内存、完整世界Session与实际测试Driver。它是研究专用`AVRAPP01`，不兼容APK／Steam原档，不扩大正常玩家存档的页面范围，不包含外部两栏文件系统镜像。

## 所有权与捕获边界

应用仍是跨域协调者，Session仍是世界唯一Owner。新增[接口](include/dungeon_village_prototype/startup_application_replay.hpp)只提供捕获、恢复和完整状态摘要；不暴露任意字段setter。恢复通过封闭私有候选构造，不执行普通构造的系统读取，不调用install_loaded重写世界现金镜像，也不推进页面／重抽随机。

捕获在完整研究外层轮末，声音和Driver一次性输出必须已经消费，`scripts.executing_page`为空。快照保存系统副本、草稿、模式／页面、原序纪录装饰名单、请求序号、完整标题随机／历史交接、可选世界及其全部原序审计历史，以及计分六行／完整阶段状态／稳定页ID。路径重新绑定，构造错误只接受为空。

`StartupApplicationReplayMetadata.controller_state`是外部Driver的唯一规范状态字节，不能在恢复之后再靠旧局部变量继续策略。调用者必须提供纯验证器，完整解码、检查语义版本、页面关系、下一轮／命令与输出消费；空验证器或空载荷拒绝。库检查通用封装与应用关系，具体Driver负责已登记场景语义，不能传一个始终成功的回调宣称已验证策略。

当前真实Driver为`application-clear-conditions-v1`，资格明确是raw17条件入口。它记录实际阶段、下一轮／命令、页ID、捕获纪录、随机／历史基线、事件4／5／6、声音序列、失败／成功及检查计数。非首行row2计数44→45使用无确认更新，避免同一脉冲直接转到下一阶段；stage6计数64下一次更新自动结算。文件锁故障是测试环境，不保存OS句柄。自然经营Driver尚未交付，条件档不能换标签当自然通关档。

## 格式和严格校验

头含magic、格式／应用语义／捕获边界版本、dataset、world schema和应用字段schema。分区1为metadata，2为原AVRSYS01完整字节，3为应用控制字段，4仅在world存在时保存原AVRSAVE1 replay完整字节，5为Driver；ID≥1024是可选附属段，保留原序、版本和任意字节。每段和全文件均有SHA-256；摘要仅校验完整性。

整体128MiB、应用控制／Driver各1MiB、系统4MiB、可选段每段1MiB且最多60段。world与全部历史共用原16Mi节点／128MiB估计分配预算，不按分区重置，也不删历史。读时容器、分区字节与对象可共存，这些阈值不是整个进程RSS上限。

逐项验证页面与world存在资格、纪录名单与实际human_presence池、系统现金镜像、计分三元组成组、raw17栈顶／counter／phase一致、六行仍等于世界只读投影、累计与已计行关系、捕获最高分与系统相等。恢复复用只读`validate_startup_clear_score_page`，不通过推进一次来验证。无控制器的raw17只允许未推进入口；finished控制器不能保留并再领奖。

[应用字段清单](src/startup_application_replay_fields.json)由[Clang检查](scripts/check_application_replay_fields.mjs)登记全部15个直接成员的类型与保存／重绑／要求为空策略，独立schema为`6c0dd5f0f74fd9ffff54e346b7de0622ba531e9d5f93634fb15f0a8488830d90`。既有world codec coverage同时执行此检查，新应用成员未分类必须失败；world schema仍为a1ca开头的既有身份。该检查不是广告、原标题q或未来新增UI状态已实现的证明。

## 文件隔离和联合安装

应用库编译绑定本研究包的`work`目录；调用者另传其下已存在的专用研究目录。路径检查规范父组件、大小写／硬链接别名、符号／重解析点及Windows特殊名称，拒绝越界和与当前应用文件重叠。`protected_paths`可声明trace、证书和其它只读输入；它不进入快照。纯预检不创建目录或文件。

捕获目标必须不存在。恢复的固定`system.avr`、`world0.avr`、`world1.avr`也必须全部不存在；目录内无关文件保留。读容器→完整候选及Driver校验→准备可无异常移动的最终对象→再次检查路径→无覆盖发布一份系统文件→noexcept联合安装应用和Driver。任何发布前错误保留旧应用、Driver及源文件；恢复不写两栏世界文件。

无覆盖发布复用独占临时文件、刷新和回读校验。Windows最终MoveFileExW不带REPLACE_EXISTING；POSIX采用同目录link发布，目标存在即失败。该出口与正常单文件替换职责分开，禁止用exists预检代替最终无覆盖语义。实际Windows双写者竞争已测试；POSIX代码尚未在本机执行。路径预检不声称能抵御恶意进程并发替换父目录，掉电耐久性也不等于文件替换原子性。

## 摘要、复算与适用范围

`startup_application_replay_digest`比较完整规范状态，组合全部应用控制／系统／Driver及原序扩展和完整Session摘要；它不是容器字节hash，也不以摘要成功代替完整恢复校验或128MiB文件预算通过。每轮无需重复构造并解码磁盘容器；真实save仍执行同一decoder自检，restore仍完整校验。Session摘要复用不可变历史的弱引用缓存，退休引用不会因缓存延寿。

扩展现有应用、持久化、codec覆盖和进程隔离套件，不新增CTest或逐函数target。标准条件回放覆盖计分前后、结算失败／重试、等分不换纪录主人和一次收尾；四个捕获点各用两次新进程比较完整应用、Session历史、Driver、实际声音／事件与隔离系统摘要。标题／配置／纪录及保留世界的应用状态另有独立往返与损坏拒绝检查。

最终验收、复用候选和规模见[验证入口](../VERIFICATION.md)。本机四个已认证条件快照位于[证书目录](../work/snapshots/application-clear-v1/CERTIFICATE.json)，9文件1395611字节；结算前64档只需2次请求，失败后档只需1次重试。全套生成／验证可在既有进程runner显式增加`--application-exe`和`--application-snapshot-directory`（后者必须为work内的新目录），默认CTest仍回收临时制品。完整命令与证书摘要见[交付记录](../work/application-replay-delivery/README.md)。这些本地二进制条件档不提交为玩家存档。

自然180月、五星／全部任务／BOSS、原标题人物控制器和完整Steam皮肤仍独立推进；这批短回放只消除了“计分中不能恢复”的维护缺口。
