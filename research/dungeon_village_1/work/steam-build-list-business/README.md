# Steam raw21初始化与确认业务

2026-10-10。正式合同见[STEAM_BUILD_LIST_BUSINESS](../../ui/STEAM_BUILD_LIST_BUSINESS.md)。独立新包，不修改既有raw21绘制、mapchip数组、差异图或资源安装证据，不改共同索引／C++／产品，不构建或操作原游戏。

## 固定窗口与复算

[inspect.cjs](inspect.cjs)登记8个具名方法，核DLL／metadata、方法下一入口、PE可执行范围。Init从入口0x10312460顺序解码9899字节；Update从入口0x10321990顺序解码18761字节，display_ranges仅控制stdout显示，绝不从中间地址初始化解码器。其余6个是软键、OnTouchEvent、ChangeTBMode、SetHelpMsgInit、类型构造和Pop小方法；总30276字节。

旧专题曾拒绝从非入口续窗，本批不执行旧被拒方案。由当前具名入口的真实分派直接定位Init→0x10314793和Update→0x1032599A，再保持入口顺序解码；Update分支距入口16394字节，完整后继必要前缀18761字节，固定上限20KiB，显示只限已见raw21分支／已见返回调用目标。本包不改变旧专题16KiB账本、不进行自动跳表扫描或导出指令全文。

实际Update分支地址为**0x1032599A**，结束0x103262D9；Init分支0x10314793–0x10314B0B。两个局部共3255字节；其它prefix字节只用于入口顺序与边界，不声称其他页面已认证。

[audit.cjs](audit.cjs)复核52个真实E8调用／参数／赋值锚和两条提示literal，保存[EVIDENCE.json](EVIDENCE.json)。旧通用marker输入、raw21登记和共享true返回复用已有源hash；APK仅保固定源hash与具名行引用，未再落盘反编译全文。

```powershell
node research/dungeon_village_1/work/steam-build-list-business/inspect.cjs init
node research/dungeon_village_1/work/steam-build-list-business/inspect.cjs update
node research/dungeon_village_1/work/steam-build-list-business/inspect.cjs softBuild
node research/dungeon_village_1/work/steam-build-list-business/inspect.cjs touch
node research/dungeon_village_1/work/steam-build-list-business/audit.cjs
```

## 增量和重要限制

Steam三目录过滤／住宅库存、撤除移动插入、空行选择与确认拒绝、独立五行跟窗、软键／74载荷、资金预检及BUILD交接已证。空行哨兵−99、max(count−1,4)、上绕时先余数0后跟窗不能改成普通列表裁剪。DrawAllForms在frame3门槛之前，原绘制会变frame，不是纯UI查询。

特殊撤除／移动确认只切模式和场景，未写tenantBuildId／朝向；ChangeTBMode也只改模式并刷新提示。普通选择才写定义ID、朝向0、清NEW。没有实际收费、施工或世界事务发生在本目录确认分支，后继独立验。

原Init依赖合法私有状态，不证明缺字段、错误父页或坏索引可安全恢复。维护Owner已有显式拒绝与事务要求不变。触摸value1字段的长期写入者、完整FormManager退休及原OS窗口仍未知；没有为了“完整”补演示行为。

探针同步退出，无后台任务、缓存、临时图像或原档副本。新证据被正式合同消费，旧资源引用保留，父会话统一链接／差异／规模验收及提交。
