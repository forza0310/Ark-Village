# Steam图块资源安装、旋转资格与差异图发布

2026-10-10。正式合同：[STEAM_BUILD_RESOURCE_INSTALL](../../ui/STEAM_BUILD_RESOURCE_INSTALL.md)。父会话统一归档提交；本分支只写独立工作包、独立合同、两图发布目录和限白名单发布器，不覆盖[上一批冻结候选](../steam-mapchip-patterns/README.md)。

## 来源与复算

[inspect.cjs](../../tools/steam-build-resource-install/inspect.cjs)固定20个具名窗口共9344字节。分别核GameForm.Init／Finish／ChangeState、软标签与逻辑脉冲、RecordStore资源分支、Storage media3、AssetReader具名回调及ResourceManager字节入口。所有入口按方法索引和PE边界核验，输出只到stdout，不保存原实现全文。

[audit.cjs](../../tools/steam-build-resource-install/audit.cjs)核固定DLL／metadata、41个真实调用／参数／赋值锚、3个literal、85原定义及46项“可建设＋可旋转”子集，两方向裁片继续消费上一批完整来源；输出[EVIDENCE.json](EVIDENCE.json)。AppData.Init／cctor及GameForm更新复用旧具名反汇编包，所需窗口另核原DLL字节hash，不重拷大包。

```powershell
node research/dungeon_village_1/tools/steam-build-resource-install/inspect.cjs manifest
node research/dungeon_village_1/tools/steam-build-resource-install/inspect.cjs gameInit
node research/dungeon_village_1/tools/steam-build-resource-install/inspect.cjs gameState
node research/dungeon_village_1/tools/steam-build-resource-install/audit.cjs
python -B research/dungeon_village_1/tools/scripts/publish_steam_build_common.py --check
```

首轮按具名BootForm.Load／Download及初始化线索探查，发现resMapChip不在BootForm.Load安装；最终正式入口是GameForm.Init。探索stdout未另落盘，也未把未闭合的语言候选／LoadReady全部流程列入20方法交付窗口。无盲扫、原游戏执行或构建。

## 发布消费与边界

[两图正式包](../../assets/steam-build-common/README.md)只有1213字节新原图、2个同字节SEB alias。清单共4逻辑资源1669字节，记录来源、像素和消费合同；`--check`只读、再次发布created0。两张输出原图已通过图片工具查看，未裁剪、重编码或改色。发布器复用既有预检、独占创建及按本轮身份回收协议，不复制整套资源或读取work；包内独立Git属性保持图片字节。

原资源安装先Dispose旧对象、先写新引用再Load；原GameForm.Finish则Dispose并清引用。研究Owner已有原子事务要求继续适用，不能把原失败行为当维护保证。TextAsset成功／异常均Unload、JarInflater成功／异常均Close已核；完整ResourceManager内部、GPU释放与语言选择仍不称已完。

flags32只是旋转软标签资格；57项含该位，46项还含建设位4，不代表46项全部自然解锁。脉冲检查会消费位，不能作为可反复调用的只读UI查询。旧地图pattern与源异常完整保留，不补帧、不改原图。

探针、发布和检查均同步退出；无后台任务、窗口、存档或新增构建缓存。新增静态摘要和两图均有正式消费者，历史世界记录合法增长仍单独审计，不宣称永久内存有界。
