# 信息菜单合同：先闭合收支36

2026-10-09。只读产品[需求](../../../../docs/reference/RESEARCH_REQUESTS.md#information-audio-animation-89f157c)，与研究UI索引、主菜单、周期账本和自动月报去重后，选取未维护的raw36闭合原静态链；正式结果见[INFORMATION_MENU](../../ui/INFORMATION_MENU.md)。

本批明确了raw9原序15/14/16/17/18、选择16后压入36并退休菜单3/9、36双页输入与返回场景、完整12×5×2统计及跨年清零。现Owner已有全部字段，不需从ledger／前三类月报拼造。低层分支解决了JADX遗漏金额int→long转换的问题，保留千分组、负号和全角Ｇ。

[EVIDENCE.json](EVIDENCE.json)以用途／源码窗口／完整源hash／LF窗口hash登记；[audit.cjs](../../tools/information-menu-contract/audit.cjs)核源窗口及关键目录/字段关系。39个源码窗口、45项审计通过；JSON约15KiB，实际字节数由脚本输出。没有保存反编译实现副本，没有新原图／存档或窗口操作，没有本分支后台任务。

资格：APK1.0.8原局部静态＋已有低层反编译交叉；Steam仅已存在页面常量声明，未把它升格为相同实现。[startup_information](../../prototype/include/dungeon_village_prototype/startup_information.hpp)已交最小纯投影，既有[startup_skin_checks](../../prototype/tests/startup_skin_checks.cpp)新增条件桶／只读及回卷oracle，主会话统一接线、编译并验收，本分支未自行构建／运行CTest。C++入口／翻页／退休消费者尚未实现，此合同不能冒称完整信息菜单交付；34/39、35/60、37/38仍有独立缺口。

复现：`node research/dungeon_village_1/tools/information-menu-contract/audit.cjs`。本目录仅派生审计与报告，证据由正式合同消费；文档与索引由主会话统一收口，不修改产品文件、不提交Git。

## 集中验收与同批交付

主会话已登记源文件，单Release完整构建通过；既有应用／visuals两项CTest通过26.66秒（26.65／0.52秒），日志为[build.log](../../work/information-menu-contract/build.log)、[ctest.log](../../work/information-menu-contract/ctest.log)。仅新增纯查询和既有套件的边界oracle，没有Owner字段、页面载荷、随机、schema或新target。前述“本分支未构建”仅描述子分支工作，不代表当前集中交付未验。

同批归档[Steam基础动作数组](../steam-actor-arrays/README.md)、[31活动产生者](../progression-unlock-producers/README.md)、[音频合同](../audio-contract/README.md)，并认证当前语义3自然12→24月。24月源与结果完整证书摘要、日志hash、文件规模和链接检查见[VALIDATION.json](VALIDATION.json)；本地41.6MB快照留在ignored工作区，摘要不替代恢复时完整文件校验。

24月reference约689秒，双恢复20轮各约2秒，说明后续局部回归可以复用当前前缀。96份审计历史、313流水和3保留任务属于合法增长；终点页1、峰值5，声音已消费。文件仍在128MiB内，后继36月按现预算采样，不提高预算或删除历史。当前阶段构建／测试／24月进程已收齐，下一阶段采样独立继续。
