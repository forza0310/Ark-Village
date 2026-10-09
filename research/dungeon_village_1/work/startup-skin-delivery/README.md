# 启动皮肤图块交付与窗口决策收口

2026-10-09，接续`daed97e`。用户取消自定义姓名→切性别→取消重进的原窗口强制补测，采用既有保名／保草稿维护策略；冻结窗口报告和Steam未知项保持。[接收分析](../startup-ui-analysis/ANALYSIS.md)已更新，旧提示词标注停用该链。

## 本批交付与验证

[只读图块模块](../../prototype/STARTUP_SKIN.md)提供APK标题五图目录、纪录／新局／计分三个背景、计分静止和兴奋角色及继续箭头。原始PNG和SEB未改，不增加Owner、缓存、随机调用或持久字段。扩展既有visuals测试；独立源码文件用于隔离应用皮肤包身份，不新增target／CTest。

单Release动态树集中构建最终通过。首次链接发现测试误用了未链接的归档读文件函数与摘要符号，改为测试局部只读输入，并显式链接既有hash库；没有引入整个归档库、删断言或修改原数据。失败记录保留于`build.log`，最终见`build-final.log`。四份命令日志统一UTF-8／LF并去行尾空白后计算归档hash，未改变诊断内容。

7项受影响CTest全部通过，共38.09秒：startup_application、startup_world_persistence、startup_world_replay_driver、startup_world_replay_process、startup_world_visuals、startup_world_data、assets。Owner实现和格式未改，未重新生成大型AST或运行无关多年长测；Debug／产品／原程序窗口本批未执行。

额外调用既有visuals导出[CPU素材拼图](skin-pieces.png)，退出0，322939检查（其中大量为逐像素检查，不能当作独立行为数量）。实际查看600×660输出：五张标题图、三个页面背景裁片、三组角色及7×5箭头可见；标题含固定汉化APK自己的Logo，这不是Steam标题。标准CTest不生成此文件，导出运行多两项路径／输出检查。

图片桥从原包索引校验PNG和SEB身份、哈希、裁片、页面锚点及SEB内部偏移仅一次。角色图像通过RAII释放，无窗口／GPU纹理／后台服务；可选输出已由审阅与文档消费。输出只是素材拼图，没有UI字号、按钮命中、完整页面或业务演出时序认证。

## 审计与剩余范围

[audit.cjs](audit.cjs)复核17个旧局部源窗口及17项素材身份、变更文档链接、Owner／schema／原表无修改、输出hash与Release规模，结果见[VALIDATION.json](VALIDATION.json)。schema保持`a1ca189f3b9290b18390812af91f601eaac9f295154ab1de3601590d0fbda2d7`。Release树1387文件99576185字节，无.pending；只有既有Release树，没有新建Debug或第二构建缓存，首次失败日志作为必要诊断保留。

图片请求为固定大小值对象，调用结束退休；既有页与角色引用仍由原Owner管理，本批不新增实体引用。现金流水、任务和审计历史仍可合法增长，不能据纯查询无分配宣称整个应用永久有界。

下一步仍为完整皮肤的框／字体与Steam实际资源、原标题背景人物调度和随机交接，再结合[自然路线预算](../natural-clear-route/README.md)细化通关尾段。完整应用快照的[具体设计](../../stages/in-progress/APPLICATION_REPLAY_DESIGN.md)状态未因改名链决定改变。本批没有操作原游戏／实时档案，也不关闭已交还用户的旧PID；构建、测试及本轮子任务均已结束，只保存本地checkpoint，不推送。
