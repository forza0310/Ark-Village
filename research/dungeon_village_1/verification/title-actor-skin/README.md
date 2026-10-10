# 标题／纪录人物基础图层交付

2026-10-09。本分支完成固定APK1.0.8的标题／纪录人物基础资源链，不操作窗口、Steam存档或原进程。事实与限制统一见[正式合同](../../ui/TITLE_ACTOR_SKIN.md)。

- 标题逐人原顺序为阴影→武器→身体；纪录为武器→身体。身体按当前职业和性别选择PNG，武器按当前主武器定义选择PNG／动作风格。
- 原human四向SEB帧顺序为列0、1、0、2；武器行走固定frame0、位置按四类原表随step奇偶上下变化。原身体与武器不会共用相同帧参数。
- 原W有scratch写入及调试复用，不能保证所有历史入口无残留叠加。本次C++只交基础图层查询，不声称复制完整W.draw。

[audit.cjs](../../tools/title-actor-skin/audit.cjs)输出[EVIDENCE.json](EVIDENCE.json)，本次2791项静态检查通过：22个源码窗口、3张原表身份、98个实际资源文件，覆盖37个身体图片、33个武器定义、4个身体SEB／16个武器SEB／1个阴影SEB。没有复制原图或新建构建树；派生证据约52KB。资源数是本批引用规模，不是新增图片数量。

新增模块为[头文件](../../prototype/include/dungeon_village_prototype/startup_title_actor_skin.hpp)和[实现](../../prototype/src/startup_title_actor_skin.cpp)。调用者传当前职业、性别、主武器、已决定的步帧和朝向，结果的阴影／武器沿既有`StartupVisualDraw`消费，身体沿既有human适配器消费，固定原顺序。它不改Owner或应用schema。

验证扩展在既有[startup_skin_checks.cpp](../../prototype/tests/startup_skin_checks.cpp)，没有新target。实际原图CPU裁片、冻结SEB顺序哈希、33武器全方向步态及拒绝集中在该套件；可选素材拼图扩为760×1230，新增底部四行只展示原图块组合，无业务数值和猜测字体。主会话集中构建／运行后登记结果；本分支没有自行构建，因此本记录不声称C++验收已通过。

可复现静态检查：`node research/dungeon_village_1/tools/title-actor-skin/audit.cjs`。脚本只读取固定源码／表／资源并更新本目录派生证据；源窗口同时记完整文件与LF规范窗口哈希，不保存反编译实现副本。EVIDENCE由正式合同、C++字面期望和本工作入口消费，不留无人引用的截图或缓存。本分支无后台进程、无子任务、未提交Git。
