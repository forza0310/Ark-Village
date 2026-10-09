# 公共窗口框、Steam资源与原标题调度收口

2026-10-09，接续4ef4a98，保持产品并行修改。本批三个专题分别是[APK框／字体底层](../startup-frame-font-research/README.md)、[Steam资源身份](../steam-startup-resource-map/README.md)、[原标题调度](../title-schedule-handoff/README.md)。原事实、维护图元策略与窗口未知分开。

公共框与标准内框接既有只读startup_skin模块，Release集中构建通过；应用／visuals／assets三项通过5.82秒。后续应用快照代码变更后的相关最终复验见[应用交付](../application-replay-delivery/README.md)，不以本节旧二进制代替最终结果。

已实际查看[760×930 CPU拼图](frame-pieces.png)，下方显示三种真实原尺寸框／内框，未补虚构字体和业务值。导出运行322992检查，主要为像素比较，不当行为数量；标准不输出图片。APK背景／Logo与Steam不同，不能把此拼图当Steam原窗口截图。

Steam20条映射含14 PNG／6 SEB，16条字节相同、10条PNG像素相同；四嵌入字体完成sfnt范围与表校验，共22870结构检查。标题背景600×380、Logo／草边与APK不同；中文运行字体及实际Draw选择未认证。原标题合同补age不归零、inactive参与非稳定排序、98／99确认脉冲区别及栈顶准入；这些是APK静态链，未加入新的标题Owner。

[外部提示词](../window-restore-observation/NEXT_SKIN_OBSERVATION_PROMPT.md)及[反馈模板](../window-restore-observation/SKIN_OBSERVATION_FEEDBACK_TEMPLATE.md)已准备，本会话未启动原游戏。取消的改名链不重派；计分仅等现成前提。所有本轮CPU图像／子任务已释放，无新增构建树；有限图元查询最多7图片请求，内容框3矩形＋4角，不承诺全世界历史有界。来源哈希、输出、链接与资源规模汇总于[审核](../application-replay-delivery/VALIDATION.json)。
