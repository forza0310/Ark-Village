# Steam建设raw21绘制与注册

正式增量见[STEAM_BUILD_LIST](../../ui/STEAM_BUILD_LIST.md#steam后继完整raw21局部绘制分支)。本批只读固定Steam DLL、metadata、方法映射与既有资源索引；没有原窗口、存档操作、C++、CMake、产品改动或构建。

旧建设专题具体非入口续窗曾被拒，历史记录保持原样。本批先复用后来Steam保存菜单研究的`_draw`具名入口4096字节证据，看到type21分派至`0x10353799`；新探针继续**从同一具名方法入口顺序解码**8704字节。`layout/rows/registration`参数只是stdout过滤，不指定解码起点，没有自动跳表推导或从未知地址强行解码。

[inspect.cjs](inspect.cjs)固定三个具名入口，核DLL／方法索引hash、登记下一方法、可执行PE映射与边界；主前缀8704字节、DrawMapchip784字节、DrawVerticalScroll2共304字节。合计9792字节，raw21局部分支6133字节至真实RET；不分析登记跨度中的其它匿名函数，不把完整`_draw`标完成。

[audit.cjs](audit.cjs)保存[EVIDENCE.json](EVIDENCE.json)：三个窗口身份／摘要、20个已消费字段声明、7个固定literal、7个直接资源槽和5个SEB图片依赖、五行及类目／滚动输入实参。证据不包含反汇编全文、原存档、账号或复制素材。原资源引用现有发布包，不新增PNG/SEB。

```powershell
node research/dungeon_village_1/work/steam-build-list-render/inspect.cjs layout
node research/dungeon_village_1/work/steam-build-list-render/inspect.cjs rows
node research/dungeon_village_1/work/steam-build-list-render/inspect.cjs registration
node research/dungeon_village_1/work/steam-build-list-render/inspect.cjs mapchip
node research/dungeon_village_1/work/steam-build-list-render/inspect.cjs scroll
node research/dungeon_village_1/work/steam-build-list-render/audit.cjs
```

关键差异：Steam行marker16已实证，Margin仅(10,0,0,0)；三类基矩形宽60且从J起，不是APK的宽54／J+3；滚动helper实际注册组件12和25。raw21入口绘制会递增frame再夹3，不能塞进“任意重画都无副作用”的纯皮肤消费者。普通建筑通过mapchip定义、反向设施pattern及SEB分片绘制，不使用设施icon字段直接画整PNG。7个直接源槽同字节，但向下追SEB依赖发现图103 number05与图98 tenant_resident的Steam字节不同；严格同核初验据此失败，随后将这两项明确列为差异依赖，保持原7项同核断言，不将异图alias为相同。

未完成：Steam建设Init/Update业务、pattern数组全部值、Surface完整滚动/OS事件以及原窗口。已核图层与输入注册不替代自然建设或完整窗口验收。脚本同步结束，无监测器、播放器、构建树或后台任务；阶段审计与本地提交由主会话统一。
