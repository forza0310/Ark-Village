# 建设目录21：Steam局部合同与APK交叉边界

2026-10-08。本专题服务已确认的Steam 2.56 UI／操作还原目标；固定汉化重签APK1.0.8仍作为规则基线。
这里只读既有局部探针及冻结Java来源，不启动游戏、不改存档、不修改维护Owner或产品。
**Steam目录行注册、图块绑定与精确绘制尚未闭合**；下文APK坐标和输入参数不能自动转记为Steam事实。

## 来源与证据范围

| 输入 | 身份／范围 |
| --- | --- |
| Steam GameAssembly.dll | SHA-256 `9cf4bb10d55afe6898bf9b82d9016d328cce623a7e4743623eb3df720b55ab1a` |
| Steam global-metadata.dat | SHA-256 `80e17b3c1f7b7a844d64be27918e16cacfab05d70d7f33c0a61e45d830d7a369` |
| Steam methods.json | SHA-256 `399156eff1f2623623d489b3e36ab9a83b9695d311c7c91e21b013dafc665c42` |
| Steam dump.cs | SHA-256 `1bf5680223c495b5d24aa5934831a17e9ecf03b95ba4ddb3ad4b9459c3c81912`；SubForm字段及TYPE_BUILD=21声明 |
| APK b/g.java | SHA-256 `0eecb47f1996abc34e6fa1ffcbbeeb40d2a597f46168a2583833ed6fe1e33081`；下列具名局部窗口 |

Steam身份、方法登记范围及哈希继承[既有UI证据](../work/steam-ui-coverage/README.md)，
对应[方法清单](../work/steam-ui-coverage/methods.json)与[检查记录](../work/steam-ui-coverage/CHECK.json)。
本次仅用既有`analyze.cjs --inspect`消费SetTouchValue及Update_scrollValue有限指令窗口，没有新增机器码探针。
外层`SubForm._draw`登记范围197,616字节，本次没有解码；不能宣称全部方法或21绘制已认证。
APK巨型Java方法的普通反编译警告仍有效，图块绘制已另以低层窗口交叉，见[图块合同](PAGES.md#固定apk候选建筑两朝向与目录裁剪)。

## Steam已证的页面字段与输入

`SubForm`metadata字段：`type_ +0x84`、`scrollValue_ +0x88`、`select_ +0x90`、
`page_ +0xD8`、`buildList_ +0x168`；`value1_ +0xB8`、`value2_ +0xBC`是其它字段。
以下映射由实际DLL消费者交叉，不能把类目索引写成value1_。

| 合同 | 实际机器码位置 | 已证内容／限制 |
| --- | --- | --- |
| 建设类目事件 | OnTouchEvent RVA0x3160B0；VA0x10316103–0x103161E5 | TYPE21、组件3且value高16位0x50000处理UP／ENTER；类目循环模3，并清scroll／select。名称及注册矩形未知 |
| 类目写入 | SetTouchValue RVA0x317600；VA0x10317612–0x10317626 | type高16位0x50000写page_；此方法本身不限定21页 |
| 列表选择写入 | 同方法VA0x10317653–0x10317667 | type高16位0x20000写select_；不证明21行已注册此value |
| 滚动写入 | 同方法VA0x10317668–0x1031767C | type高16位0x40000写scrollValue_；不证明21滚动组件绑定 |
| 通用选择跟窗 | Update_scrollValue RVA0x3284B0；VA0x103284E3–0x103285B0 | select不小于集合长度先归0；scroll高于select则下调；选择越过可见尾则上移窗；scroll过count−disp时归max(0,count−disp)，并夹选择到0…count−1。**21调用及disp实参尚未核** |

通用DLGSEL3／LIST10／SLIST11的marker16共享消费者已证：首次未标记UP选择并记key，
第二次同key UP发逻辑确认，没有点击间隔判定；键盘会清marker。
见[Steam输入合同](STEAM_INTERACTIONS.md#输入选中标记与确认分开)。
**尚无Steam21行注册marker16的机器码证据**，不能据通用消费者称“21必须双击”。
Steam适配事件CANCEL=10；APK原始Android ACTION_CANCEL=3，两个事件层保持分开。

## APK21目录及特殊边界

以下是固定APK事实，Steam交叉栏未核部分保留缺口。坐标是原画布局部坐标，不是物理DPI热区。

| 项目 | APK局部合同／来源 | Steam交叉状态 |
| --- | --- | --- |
| 类目／开放集合 | [b/g:10647](../work/decompiled/sources/b/g.java:10647)：3个Vector；定义开放p非0、flags4，住宅kind12还需H>0，按定义j入类；第0类索引1插撤除−1，flag32开放后索引2插移动−2 | Steam只有三类输入消费已证；同集合生成待核 |
| 类目文字 | [b/g:9604](../work/decompiled/sources/b/g.java:9604)：设备／一般／饮食 | 不能从模3推出Steam中文名称 |
| 目录原点／外框 | [b/g:9591](../work/decompiled/sources/b/g.java:9591)：J=((c.m+c.o)/2)−87−画布偏移X，K=((c.n+c.p)/2)−104−画布偏移Y；外框175×210 | 未核Steam21绘制 |
| 行／图块 | [b/g:9655](../work/decompiled/sources/b/g.java:9655)：最多5个实际条目；行距37；图块左上(J+10,K+25+37row)，64×32裁剪，RGB(190,242,230)，固定朝向0按原分片画 | 未核Steam实际mapchip绑定、锚点与裁剪 |
| 特殊图标 | 撤除−1图片17偏移(4,3)，移动−2图片119偏移(1,0)；住宅H、NEW147及kind13提示81另叠加 | 未核Steam21资源绑定 |
| 报价 | [b/g:9688](../work/decompiled/sources/b/g.java:9688)：撤除0G、移动300G、普通定义报价h()；资金不足换价色，0G仍绘 | Steam报价／颜色未核 |
| 每行输入登记 | [b/g:9730](../work/decompiled/sources/b/g.java:9730)：SLIST11，矩形(J+2,K+22+37row,165,37)，value=(scroll+row)\|0x20000，marker16，扩命中(10,10,0,0) | Steam相应登记未核 |
| 类目输入登记 | [b/g:9742](../work/decompiled/sources/b/g.java:9742)：DLGSEL3，矩形(J+3+57cat,K+2,54,20)，value=0x50000\|cat，扩命中(0,0,30,0) | Steam相应登记未核；已有高位类目消费者不证明矩形 |
| 滚动绘制实参 | [b/g:9740](../work/decompiled/sources/b/g.java:9740)：(J+170,K+21,189,count−1,5)传滚动helper | Steam21实参／最终组件热区未核 |

APK需要保留两个易被普通列表规则抹掉的边界：

1. **输入登记始终5行**，实际条目绘制才受`scroll+row<count`限制；集合不足5项时，空行也有登记。
2. [b/g:11537](../work/decompiled/sources/b/g.java:11537)的键选择上界是`max(count−1,4)`，
   允许选择空行；首尾循环、左右换类、跟窗采用21自己的分支。确认时才检查`select<count`。
   因而不能将Steam通用`Update_scrollValue`的count裁剪直接套为21精确合同。

## APK选择、预检查与建设提交分开

[b/g:11571](../work/decompiled/sources/b/g.java:11571)至11629：选择变化刷新对应软键；
未收到确认时，取消退出，动作7仅对非负定义打开74详情并以value1=1限制详情模式。
确认且选择在真实条目范围内时，先比报价与资金；不足拒绝，这一页尚不实际扣款。
撤除−1进入建设模式3，移动−2进入模式6；道路kind6进入模式1，普通flags4进入模式0。
普通选中设置定义ID与朝向0，清定义新标r，再切GameForm建设场景；提示旗标与真实扣款属于后续消费者。
已有[普通候选绘制与旋转](PAGES.md#固定apk候选建筑两朝向与目录裁剪)不是本页选中输入已完整认证。

Steam建设候选闪烁、朝向传递及建设确认消费另见[覆盖清单](STEAM_UI_COVERAGE.md#建设定位拖动旋转与提交)，
与本页目录21的行确认不混合。现有维护查询及原型／产品缺图修复也不当成Steam原窗口证明。

## 下一闭合范围

先精确定位外层`_draw`的21分派，再核有限块中的类目文本、五行绘制、图块／价格、
行marker16／滚动组件注册实参，并交叉Init2和Update的21专用分支。
每个窗口须有已核指令边界、方法登记范围、地址／哈希及预算；大型外层不得按完整方法覆盖报告。
自然窗口仍需在原程序单独认证最终命中区、滚轮／键盘投递及空行输入，不由静态事实替代。

新有界探针当前因自动审批拒绝暂停，详见[专题工作包](../work/steam-build-list-contract/README.md)。
这不影响上述既有证据整理；本次不新增探针、不增加构建缓存、不留下后台进程。
