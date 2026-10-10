# Steam升级81：正式计划的CPU原素材示例

2026-10-10已构建并生成六面板，主会话实际查看[原尺寸拼图](steam-facility-upgrade/contact-sheet.png)。[清单](steam-facility-upgrade/MANIFEST.json)保留实际消费的PNG／SEB及每图、有序JSON的字节身份；单面板与绘制JSON同目录。Release visuals检查0.61秒通过，归档完整性与源hash见验证包（本地核对材料，不随仓库交付）。

本示例直接消费[只读皮肤接口](../../prototype/STEAM_FACILITY_SKIN.md)的`steam_facility_upgrade_skin`、`steam_facility_number_draws`和`steam_facility_mapchip2_draws`，用于产品审阅木窗框、数字、设施大图、左右角色、遮罩和原绘制次序。布局不是另写一套HTML或手工JSON；导出的每项请求来自实际C++调用。来源见[Steam81合同](../STEAM_FACILITY_UPGRADE.md)，数值与字宽为明确测试夹具，不是自然升级结果。

这是原素材组成的研究静态例图，**不是原游戏窗口截图，也不是完整中文皮肤**。当前没有接入Steam字体后端，文字位置以洋红十字标记；JSON保留文本语义角色、参数、锚、颜色、字号和语言分支。十字、灰色外背景与拼图间隔是研究适配，不能作为原UI素材迁入。示例中的80/80标题宽、12/30/4/30正文测宽仅为夹具输入，不冒充真实中文字宽。

## 生成与消费

既有`startup_world_visuals`测试可执行仍接受原素材根及可选启动拼图输出；现在可再给第三个路径参数，作为本例**尚不存在**的输出目录。不用创建新target、窗口或纹理，也不初始化游戏Owner。

```powershell
<visuals测试程序> research/dungeon_village_1/assets/original <work中的新启动拼图.png> <work中的新Steam81输出目录>
```

建议目录为`research/dungeon_village_1/work/steam-facility-ui-example/output/`；每次选择新目录，已有目录拒绝覆盖。输出为六张240×240 PNG、六份有序绘制JSON、一张744×504联系拼图及`MANIFEST.json`。实际构建／生成／图像查看状态由本批交付记录登记，拥有导出代码不等于图像已经验收。未经查看的示例不自动归入正式素材包。

拼图从左到右、从上到下：

| 示例 | phase/frame/frame2 | 主要观察 |
| --- | --- | --- |
| 第一阶段起点 | 0/0/0 | 正文clip高度0、背景设施和跳起角色 |
| 第一阶段展开 | 0/40/5 | 正文完整展开、文字锚及数字等级、小角色抛物偏移 |
| 第一阶段提示 | 0/50/20 | 继续箭头与静止小角色 |
| 第二阶段差值 | 1/4/5 | 价格增量已起跳、其它行延迟、金额和加号原SEB |
| 第二阶段计数 | 1/34/19 | 正式countAnime中间值、三行增量 |
| 第二阶段完成 | 1/55/20 | 最终三值、首行MAX、继续箭头 |

建筑使用定义35在已核目录中的真实mapchip身份；等级2及三行100→110、10→14、20→26只是正增量显示夹具，避免将尚未认证的负SEB帧解释成减号像素。页计划仍支持的其它数值边界由既有visuals断言负责，本示例不替代那些测试。

执行器只做末端操作：读已出版PNG／SEB，按记录frame取真实裁片、offset和已支持翻转，应用半开clip栈，按原序绘图；数字和Mapchip的展开全部调用正式接口。任何缺资源、错误图片身份、越界裁片、负帧或不支持的翻转显式失败，不补假图或夹到合法帧。SEB内部offset只加一次，金额dx−9只由正式数字helper处理。

`MANIFEST.json`登记实际消费的资源路径、SHA-256、字节数，以及输出PNG／有序请求JSON的hash和尺寸。图像引用Steam专属number05／number08差异包，不能改回APK同名图；其它同字节出版资源仍保留来源身份。产品可据PNG审阅比例与层次，据JSON逐项接入自己的渲染后端；真正中文文本、测宽、Owner绑定及物理输入热区另按正式合同完成。

CPU图片由本次RAII对象持有，临时裁片立即释放，资源缓存仅活到单次导出结束；不会留下GPU对象、窗口、后台进程或世界随机消费。输出文件是审核制品，不写入应用存档，不把六张静态例图称作完整动画或完整原皮肤。
