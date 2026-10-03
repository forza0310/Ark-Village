# 开发流程

从AGENTS、[目标与架构](ARCHITECTURE.md)、TODO与 [计划和决策](MILESTONES.md)开始。检查research最新维护交付、来源和工作区；只读研究内容，不修改其结论或执行逆向。
阶段先写明范围、问题/方案、数据所有权、接口、操作次序、依赖缺口及验收，用户确认后编码。
非预期规则/编译/测试问题停止并保留现场，报告原因与恢复方案。完成后同步文档、验证记录并提交checkpoint。

## 构建检查

四套预设分别执行：

```sh
cmake --preset desktop-debug
cmake --build --preset desktop-debug --parallel 4
ctest --preset desktop-debug
```

另三套替换预设名。headless不查找raylib，测试在Release也执行，警告视为错误。
构建需要Node 18+，只在构建期JSON.parse交叉校验固定发布数据，生成只读标准C++；运行无需Node。
资源更改须核对源/副本哈希、实际解码和任意工作目录启动；界面更改须实际画面/输入验收；存储用隔离档，不做旧档迁移。
格式按clang-format，公开头在include/ark，实现在src；CMake显式登记文件。只建立有实际职责的模块。

## Git与研究边界

新main是产品重置起点。小步本地提交，不夹带research或其他智能体未完成改动；构建/个人IDE/研究work不提交。
推送或远程历史改写需用户明确指令；不能把本次本地重置自动推成force-push。
research已有外部链接的UI/C/RQ编号保留为交接身份，新产品接入状态全部从零开始。

## macOS窗口检查

窗口必须在InitWindow前启用FLAG_WINDOW_HIGHDPI，画布按GetRenderWidth/Height分配；逻辑布局尺寸不用于纹理分辨率。
有界运行会打印window/framebuffer/canvas尺寸。Retina 960×512窗口预期framebuffer/canvas为1920×1024；以实际显示器倍率为准。
缩放清晰度对比使用同一窗口/镜头/zoom-percent；PNG物理尺寸与窗口点尺寸不同，查看时保留原图像素。

raylib/GLFW需要可访问的登录桌面和唤醒显示器，WindowServer存在不等于当前上下文能访问显示器。
程序在InitWindow前用CoreGraphics检查显示器，无可用显示器时立即返回错误。
无窗口检查使用`ark_village --check`；真实桌面使用`ark_village --frames 60`等有界运行。
必要时`caffeinate -d -u <程序> --frames 60`只在进程期间防止休眠，不改永久电源设置，也不授予桌面权限。
沙箱可能无法枚举真实显示器；本轮沙箱外同一有界程序成功运行。应在已登录终端运行或获准使用独立窗口检查，不移除显示器保护、不把沙箱报错记成游戏崩溃。
截图/焦点问题和业务异常分别记录；环境无法运行窗口时仍做构建/CTest，窗口验收明确记未执行。

页面检查：`ark_village --inspect-page shops --frames 8 --screenshot /tmp/ark-shops.png`，支持menu/shops/plants/food/placement/detail/bonuses/equipment/booster/arrival/visitor/motion。
缩放检查可加`--zoom-percent 50..200`；常规场景滚轮每格约5%，目录/设施第二页滚轮用于列表。
motion显式指定旅店并只推进检查用行程，渲染位置覆盖不修改领域人物/资金，也不代表默认AI。
该模式先安排模型状态再暂停，只验渲染，不验真实输入或正常新局。ui_navigation使用同一controller与实际Game验证坐标命中，但不算OS鼠标验收。
原生自动化连接裸raylib/隔离bundle可能长时间阻塞；本轮连接耗时约8分钟，窗口有界进程已结束，人工输入保持待验。不要反复连接或移除桌面保护，用有界截图和坐标测试记录独立结果。
SEB检查须区分结构和使用帧：完整记录/legacy_tag原样保留，PNG矩形只检查实际请求帧；不能为未用草地/海面记录修改原图或全面放宽边界。

## 文档维护

docs根目录只维护目标/架构、开发流程、计划/决策三份概览。原版对照与研究需求集中在reference子目录。
近期可执行任务维护在根TODO；概览链接到它，不重复复制待办。
短决策直接追加到计划文档，当前不为每个ADR新建文件；单个决策需要长篇设计或数量明显增长时再提取到decisions子目录。
详细阶段设计或对照案例按需分别放stages、reference/cases，只有实际文档时才建目录；概览保留索引与状态，不重复正文。
移动/合并文档要同步入口与研究侧引用。本次research只做链接路径调整，不修改其规格、证据或状态。

<a id="godot-首次导入与自动退出"></a>

此历史research链接只保留定位；当前运行约束见 [macOS窗口检查](#macos窗口检查)，没有旧平台依赖或导入流程。
