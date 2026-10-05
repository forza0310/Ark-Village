# 开发流程

从AGENTS、[目标与架构](ARCHITECTURE.md)、[TODO](../TODO.md)与 [计划和决策](MILESTONES.md)开始。检查research最新维护交付、来源和工作区；只读研究内容，不修改其结论或执行逆向。
阶段先写明范围、问题/方案、数据所有权、接口、操作次序、依赖缺口及验收，用户确认后编码。
不改变已确认规则、契约或范围的小型编译、断言、实现错误自行修复并重新验证，不逐项询问；测试预期修正必须有来源依据，不能删有效断言、放宽校验或修改原表凑通过。
仅遇来源/规则与方案冲突、关键契约歧义、范围扩大或需要新决策时暂停对应工作，保留现场并说明依据与方案，确认后恢复。无人值守沿用此边界及权限要求。完成后同步文档、验证记录并提交checkpoint。

## 构建检查

四套预设分别执行：

```sh
cmake --preset desktop-debug
cmake --build --preset desktop-debug --parallel 4
ctest --preset desktop-debug
```

另三套为desktop-release、headless-debug、headless-release。headless不查找raylib，测试在Release也执行，警告视为错误。每批记录当前代码的结果，本轮最终四套产品验证完成前不得写全部通过。
构建需要Node 18+，只在构建期JSON.parse交叉校验固定发布数据，生成只读标准C++；运行无需Node。
资源更改须核对源/副本哈希、实际解码和任意工作目录启动；界面更改须实际画面/输入验收；存储用隔离档，不做旧档迁移。
格式按clang-format，公开头在include/ark，实现在src；CMake显式登记文件。只建立有实际职责的模块。

## Git与研究边界

新main是产品重置起点。小步本地提交，不夹带research或其他智能体未完成改动；构建/个人IDE/研究work不提交。
推送或远程历史改写需用户明确指令；不能把本次本地重置自动推成force-push。
research已有外部链接的UI/C/RQ编号保留为交接身份。按已提交维护源冻结、迁入和核对哈希，不夹带在途研究改动；研究测试结果与产品验收分别记录。

当前默认启动、`--world`别名及`--check`都选择持续世界。旧建设切片通过`--legacy-slice`或命名旧诊断进入，`--tick-rate`只用于这类旧入口；`--verify-play`保留原节拍。本轮raw49只接晋级条件读取/四项显示/确认，不接收费晋级。长期回归通过 `-DARK_LONG_WORLD_TESTS=ON` 显式开启，建议Release、`ctest -L long_world --parallel 1`，避免与试玩/其他长跑争抢CPU。自然任务生成、玩家接受后自然成功及无限运行分别验收，不能互相代替。

## macOS窗口检查

窗口必须在InitWindow前启用FLAG_WINDOW_HIGHDPI，画布按GetRenderWidth/Height分配；逻辑布局尺寸不用于纹理分辨率。
有界运行会打印window/framebuffer/canvas尺寸。默认1080×720在2×Retina下framebuffer/canvas为2160×1440；以实际显示器倍率为准。
缩放清晰度对比使用同一窗口/镜头/zoom-percent；PNG物理尺寸与窗口点尺寸不同，查看时保留原图像素。

raylib/GLFW需要可访问的登录桌面和唤醒显示器，WindowServer存在不等于当前上下文能访问显示器。
程序在InitWindow前用CoreGraphics检查显示器，无可用显示器时立即返回错误。
无窗口检查使用`ark_village --check`；真实桌面使用`ark_village --frames 60`等有界运行。
必要时`caffeinate -d -u <程序> --frames 60`只在进程期间防止休眠，不改永久电源设置，也不授予桌面权限。
沙箱可能无法枚举真实显示器。应在已登录终端运行或获准使用独立窗口检查，不移除显示器保护、不把沙箱报错记成游戏崩溃。
截图/焦点问题和业务异常分别记录；环境无法运行窗口时仍做构建/CTest，窗口验收明确记未执行。

世界页面检查：`ark_village --inspect-page world-active --frames 8 --screenshot /tmp/ark-world.png`，也支持world-month/world-rank/world-award；从实际窗口视口预运行真实世界，明确给予前序页面测试确认输入，停在实际多人/月报/晋级条件/年度贡献状态。年度预运行可能耗时数分钟，不能用合成状态替换真实新局轨迹。
旧页面检查如`--inspect-page shops/motion`自动选择旧切片。motion覆盖检查用行程，不修改领域人物/资金，也不代表默认AI。缩放检查可加`--zoom-percent 50..200`。
页面快照只验渲染，不等于正常新局/OS输入。共享controller坐标测试单独记录，不能代替真实鼠标验收。原生自动化连接裸raylib或隔离bundle可能阻塞，优先有界截图与坐标测试，不反复连接或移除桌面保护。
SEB检查须区分结构和使用帧：完整记录/legacy_tag原样保留，PNG矩形只检查实际请求帧；不能为未用草地/海面记录修改原图或全面放宽边界。

## 文档维护

docs根目录只维护目标/架构、开发流程、计划/决策三份概览。原版对照与研究需求集中在reference子目录。
近期可执行任务维护在根TODO；概览链接到它，不重复复制待办。
短决策直接追加到计划文档，当前不为每个ADR新建文件；单个决策需要长篇设计或数量明显增长时再提取到decisions子目录。
详细阶段设计或对照案例按需分别放stages、reference/cases，只有实际文档时才建目录；概览保留索引与状态，不重复正文。
移动/合并文档要同步产品入口；研究侧外部引用由研究维护者处理，产品侧不修改其规格、证据或状态。

<a id="godot-首次导入与自动退出"></a>

此历史research链接只保留定位；当前运行约束见 [macOS窗口检查](#macos窗口检查)，没有旧平台依赖或导入流程。
