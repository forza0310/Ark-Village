# 开发流程

从AGENTS、目标、架构、TODO与里程碑开始。检查research最新维护交付、来源和工作区；只读研究，不修改其结论或执行逆向。
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
资源更改须核对源/副本哈希、实际解码和任意工作目录启动；界面更改须实际画面/输入验收；存储用隔离档，不做旧档迁移。
格式按clang-format，公开头在include/ark，实现在src；CMake显式登记文件。只建立有实际职责的模块。

## Git与研究边界

新main是产品重置起点。小步本地提交，不夹带research或其他智能体未完成改动；构建/个人IDE/研究work不提交。
推送或远程历史改写需用户明确指令；不能把本次本地重置自动推成force-push。
research已有外部链接的UI/C/RQ编号保留为交接身份，新产品接入状态全部从零开始。

<a id="godot-首次导入与自动退出"></a>

此历史research链接只保留定位；当前运行约束见 [raylib桌面检查](troubleshooting/macos-raylib-window.md)，没有旧平台依赖或导入流程。
