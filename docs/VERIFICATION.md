# 产品验证

## B0：2026-10-03重置

Completed。新产品检查单独记录，不引用旧工程日志；research验证由其维护入口管理。
AppleClang21、CLion内置CMake/CTest、pkg-config3.0.7、raylib6.0.0；未验证其他平台。

| 预设 | 配置/编译（警告即错误） | CTest |
| --- | --- | --- |
| headless-debug | 通过，不查找raylib | 1/1 |
| headless-release | 通过，不查找raylib | 1/1 |
| desktop-debug | 通过 | 2/2 |
| desktop-release | 通过 | 2/2 |

分别实际执行cmake --preset、cmake --build --preset --parallel 4和ctest --preset。
Release程序从/private/tmp运行完整路径--check返回0，实际解码程序旁240×330背景。
源/产品资源SHA-256一致，见assets/README；没有从research读取运行资源。

真实窗口：Debug480×660和Release960×600各30帧，由caffeinate -d -u包裹后正常退出0。
截图保存在忽略目录build/verification/b0/portrait.png与landscape.png；已查看，背景非空、等比例完整显示，横向窗口有留边。
这是窗口/资源检查，不是游戏标题菜单/触摸/玩法验收；临时防休眠随进程退出释放。

15份新产品文档的24个本地链接、20个research到产品的外部链接/锚点通过检查。
少量外部历史锚点保留为定位，不维护旧产品决策正文或旧平台教程。
新Git索引中的920个research路径与旧已提交快照的blob哈希/文件模式逐项相同；当前研究修改和第三批图片未夹带。
新产品从无父提交的main起始，远程配置保留，远程历史未改写。旧工程和.git在仓库外可恢复备份。
本阶段没有领域模型/玩法/存档/3D；已清理半完成旧重构，不以启动背景宣称复刻完成。
