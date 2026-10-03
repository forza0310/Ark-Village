# macOS窗口检查

raylib/GLFW需要可访问的登录桌面和唤醒显示器，WindowServer进程存在不等于当前上下文能访问显示器。
程序在InitWindow前通过CoreGraphics检查显示器，无可用显示器时立即返回错误，避免进入无法验证的窗口初始化。

无窗口验证：`ark_village --check`。真实桌面使用有界运行，例如`ark_village --frames 60`。
必要时`caffeinate -d -u <程序> --frames 60`只在运行期间防止休眠，不改变永久电源设置，也不授予额外桌面权限。
截图失败/焦点丢失和业务异常分别记录；环境无法运行窗口时仍执行构建/CTest，窗口验收明确记未执行。
