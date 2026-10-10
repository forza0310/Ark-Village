# 美术素材与原型接入

原始素材发布包以固定的一代汉化重签 APK 为输入。身份与限制见 [证据索引](../EVIDENCE.md)，
实施授权见 [R2-A 阶段记录](../stages/history/R2-prototype.md#assets)。这里只描述研究交付，产品接入单独验收。

2026-10-08新增[APK／EXE全容器图像覆盖](IMAGE_COVERAGE.md)和[资源代码反查](RESOURCE_CODE_INDEX.md)。
下文398 PNG／761文件是既有11视觉归档发布范围，**不是全部APK／EXE图像分母**；平台、引擎、语言及重复别名另在新清单登记。

## 当前交付

- [Steam共用差异资源](steam-common)：六张PNG及一个SEB共4794字节。既有`menuRT01.png`（image85，57×10／469字节）、`icon_objRoots.png`（image128，148×36／1432字节）、`icon_result00.png`（image31，63×48／1000字节）、`number12.png`（image109，100×21／636字节）；菜单状态栏新增`menuRT00.png`（image84，59×45／945字节）及其SEB75（88字节／四帧）、`menuRT04.png`（image151，11×11／224字节）。均为固定`resources.assets`的common对象pathID1148原条目，源身份沿[全量索引](IMAGE_COVERAGE.json)的`EXE:resources.assets:1148:common:<文件名>`记录；同名APK有差异，不复制其它同字节SEB。image31用于35贡献及菜单怪物摘要，image109用于39负利润，84／151用于菜单HUD与剩余期；均未裁切或重绘。消费者见[信息菜单](../ui/INFORMATION_MENU.md)，正式[资源路径查询](../prototype/include/dungeon_village_prototype/steam_information_skin.hpp)供产品复制后使用；不另建指纹清单，语言替换与原字体另验。

- [Steam人物详情差异图](steam-human-common/README.md)：image88 wnd_ato，10×7／187字节；明确源条目／RGBA／APK差异及raw60经验区消费者，不扩称全部语言替换已核。

- [Steam设施详情差异图](steam-facility-common/README.md)：common图片37／105独立发布，共2,438字节；SEB15仅引用已有同字节副本，3逻辑资源共2,866字节。两图均与APK像素不同，消费者见[设施详情74](../ui/STEAM_FACILITY_DETAIL.md)，操作补证见[左右软标签](../ui/STEAM_FACILITY_LABELS.md)；不把静态资源发布当作窗口或产品接入验收。
- [Steam启动／选档子集](steam-startup/README.md)：38逻辑条目，21项同字节复用原包、17项差异载荷独立发布；保留完整原INF但只允许加载显式发布条目，不含未知中文字体。
- [APK音频](audio/README.md)：26个原始Ogg与实际ID／通道清单，独立于视觉761项；播放操作与设备后端另验。
- [原始素材](original)：11 个视觉归档，761 个文件，包含 398 张 PNG、341 个 SEB、22 个 INF。
- [规范化素材](normalized)：七个活动逻辑键使用的 PNG；完整图集字节不变，帧在清单中指定。
- [来源清单](MANIFEST.tsv)：每个原始文件的归档名、内部条目名、SHA-256、图片尺寸和转换说明。
- [独立原型](../prototype)：C++17 + raylib；通过 [状态聚合层](../prototype/include/dungeon_village_prototype/village.hpp)
  组装 [维护规则](../example)，只读必要设施表，不依赖 APK 或反编译 Java。
- [提取工具](../tools)：循环 XOR、游戏专用 CRC、归档边界检查、SEB 解析与确定性发布。

全部 PNG 可解码；371 张含透明像素，没有字节相同的 PNG 副本。未发现 PAL、OPT、未知图片编码或
压缩视觉条目，因此无需额外图像转换。所有归档均按存在 `img.inf` 检测，而不是只处理地图资源。

原始素材用于用户授权的内部学习，权利仍属于原权利人；本说明不授予公开再分发许可。
`original/` 保留只读参考，不应作为换图的编辑目标。

## 归档与图片索引

归档先验证固定输入的游戏 CRC，再在内存副本上循环 XOR；条目头使用大端字段。
标准 `crc32` 与非标准 `game_crc32` 分开，不能互换。发布器再次解析 SEB 并核对 PNG 头；
PNG 的实际像素解码由原型素材测试验证。

`img.inf` 是 CRLF 文本，形式为 `图片索引<TAB>文件名`，索引可能稀疏。
原始索引内的 `.gif` 在当前包中对应实际 `.png`；不能把文件名扩展名当成图片编码。
`seb.inf` 给出 SEB 加载顺序，还可能带实例修饰参数；它不等于动画时间表。
当前恢复并保留全部原始索引，但没有实现通用索引修饰符执行器。

## SEB 格式与证据边界

本次 341 个 SEB 全部为旧格式；精灵读取与绘制的直接证据是 JADX 输出中的
`kairo/android/ui/t.java`：读取方法第 98—170 行，绘制方法第 359 行起。
这些生成文件不属于维护源码。

所有整数按大端读取。旧格式结构：

| 层级 | 字段 | 处理 |
| --- | --- | --- |
| 文件头 | 图层数量、总帧数量，各 16 位 | 按有符号计数拒绝负值与超限 |
| 图层头 | 记录数量、`legacy_tag`，各 16 位 | 后者按原始位保存，不当作数量 |
| 每条记录 | 帧号、图片索引、源 x/y、宽/高、偏移 x/y、两个附加字段，共十个 16 位整数 | 完整保留 |

非负图片索引的两个附加字段用于水平/垂直翻转；负图片索引可代表绘图命令，附加字段还有颜色等含义，
不能对所有记录一律限制为布尔值。当前解析器严格验证结构、计数、读取边界、截断和尾随字节，
不声称已验证所有命令的运行时语义。原型选中的帧另检查实际 PNG 边界。

`common/finger_d.seb` 为 6 帧、4 条关键帧、`legacy_tag = 6`；旧 Java 读取器直接丢弃该字段。
因此不能要求它等于记录数量，也不能把“图层跨度”这一猜测写成事实。用户已确认保留未解释标签。
`image/plain00.seb` 还含不用于首帧显示的额外记录，故不能把每条记录都当成可独立渲染的 PNG 切片。

全量结果：346 个图层、1228 条记录，最大总帧数为 60。压缩 SEB 格式明确拒绝，尚未实现。
本轮不复刻关键帧插值、复合精灵或完整动画播放器。

## 七个逻辑键

清单的 `frame` 是源图上的 `x,y,width,height`，`anchor` 是相对帧左上角的 `x,y`。
原型按“屏幕锚点减 anchor”放置图片。

| 逻辑键 | 来源 PNG | 首帧 | 原型锚点 |
| --- | --- | --- | --- |
| `terrain.grass` | `image/plain00.png` | `0,0,60,29` | `30,14` |
| `terrain.road` | `image/road00.png` | `0,0,60,29` | `30,14` |
| `building.inn` | `image/tenant10.png` | `0,0,60,60` | `30,45` |
| `building.cafe` | `image/t_cafe.png` | `0,0,60,60` | `30,45` |
| `building.inn.pair.front` | `image/t_inn00.png` | `0,0,60,48` | `30,33` |
| `building.inn.pair.back` | `image/t_inn01.png` | `0,0,60,51` | `30,36` |
| `character.night` | `human/chara_night00.png` | `0,0,18,24` | `9,24` |

图片索引由对应归档的 `img.inf` 确认，源矩形由同名 SEB 的首记录确认。
角色使用 `human/walk00.seb`，其中图片索引 0 对应夜骑士而非冒险者，不能仅按图片名字猜角色绑定。
地表取菱形中心 `(30,14)`；建筑采用同一地表中心与 SEB 偏移相减：普通旅店和咖啡館 y 偏移均为 -31，
换算后为 `(30,45)`；人物取负绘制偏移 `(9,24)`。
这些屏幕锚点属于原型坐标适配，不是原作设施入口、占地或完整几何规则。

R2-C 已纠正旅店绑定：定义 28 是单格普通旅店，通过地图显示记录 48 使用 `tenant10.seb`/图片索引 25；
早期 R2-A 的 `t_inn00` 实际是定义 29 双格旅店的一个分片，不能用一帧代表完整双格设施。
当前规范化路径为 `normalized/building.inn.single.png`；旧 `normalized/building.inn.png` 仅为历史快照保留，
没有逻辑键指向它。收敛版补了双格旅店两片的活动逻辑键，奇数分片按绑定表水平翻转。
完整已核对的两朝向/双格分片见 [精灵绑定表](../data/SPRITE_BINDINGS.tsv) 和 [设施报告](../rules/FACILITIES.md#definitions)。
两片锚点同样以地表中心减 SEB 的 y 偏移 -19/-22 得到，不能当成设施入口。
收敛版只修改研究交付，不改产品中已有素材副本。

## 构建与操作

从仓库根目录按[单Release动态构建约定](../README.md#构建缓存管理)使用研究聚合入口，工具链及运行命令见[原型说明](../prototype/README.md#构建与运行)，实际验证环境见[验证记录](../VERIFICATION.md)。Debug只在定位问题时临时使用，不为素材说明另建常驻Debug树。

构建会在程序旁边打包必要规范化图片、清单与设施表，资源变化也会触发更新。
可直接在 CLion 中单独打开 [研究 CMake](../prototype/CMakeLists.txt)，不要用产品主 target 代替研究原型。

工具栏前三个图标选择旅店、双格旅店或咖啡厅；移动工具依次选择建筑和目标格，垃圾桶撤除建筑；
旋转工具点击建筑切换两朝向，选择旋转工具也切换下一次建设朝向；Esc 取消移动选择。
最后的暂停/继续图标控制模拟时间。人物自行选择设施、行走、使用，不提供手动指挥移动。
R2-A 旧版的 `Visit` 调试工具已由 R2-B 移除，证据和规则差异见 [自主行动报告](../rules/CHARACTERS.md)。
预览绿色表示可放置、红色表示拒绝；右键拖动查看边缘地块。
悬停显示工具名称，窗口标题显示当前工具/建设朝向/命令结果；按窗口关闭按钮结束进程。
余额、当前夹具月份、上期经营净额在工具栏下显示；初始资金 10000，三座初始设施扣款后余额 7300。

自动演示会创建三类建筑、拒绝多格重叠、移动/旋转并撤除临时建筑，再由自主模拟选择并预约设施、
行走至真实占用格、进入 `in_use`、到达收入入账、完成后再次决策，并跨越一次夹具月界。
截图包含使用状态与上期报表；演示通过后自动退出，15 秒超时或提前关闭返回失败。

```sh
research/dungeon_village_1/work/prototype-debug-llvm/dungeon_village_prototype \
  --demo --screenshot research/dungeon_village_1/work/r2-assets/demo.png
```

画布是 `240x240`，窗口/framebuffer 是 `720x720`，最近邻 3 倍缩放；不启用 Retina 双倍 framebuffer。
示例地图为 7×7，边缘允许裁切并可拖动；地图/资金/人物种子、角色速度和使用时长是演示夹具。
每 600 个 100ms tick 为一个夹具月份，不代表原作日历。原型没有存档，退出不保存。
寻路使用真实占用格绑定、准入和四邻接道路/空地离开成本；道路不是必经条件。
道路图像仍只显示固定首帧，不执行原作邻接掩码或道路建设规则。单格/双格与两朝向消费维护几何，
不再使用四个外侧接近点。所有模拟状态由标准 C++ 模块拥有，raylib 只插值和绘制。
建设价格为 1000/1000/700，移动或旋转扣款 300，撤除零退款；消费价格和维护费使用表端点、邻接与
经营推导。人物选择使用已有候选/两级权重，但仅开放类别 1/2，排除已预约者、重置访问计数和完成后
回到接近格都是有限原型策略，不声称完整原作 AI。完成仅记录次数，不执行未知退出效果或共享升级。
人物当前移动但只显示一个站姿帧。

收敛版 Debug/Release/ASan+UBSan 构建与回归已通过，实际 OpenGL 两朝向场景和换图已验收。
启动需要登录可用桌面和唤醒显示器；有界测试可临时使用 caffeinate，不改永久设置。
鼠标验证已由用户免验，用户试玩未运行，状态与边界见 [收敛记录](../stages/history/R2-prototype.md#convergence)。
原版页面、地图及触摸基线独立进入 [R3-V](../stages/R3-original-visual-baseline.md)，
素材与消费者的首批索引见 [视觉基线](../ui/README.md)，不把上述原型截图当原版运行画面。

## 替换自有素材

为另一套素材准备同格式 `MANIFEST.tsv` 与图片，保留七个逻辑键；修改规范化路径、帧与锚点即可。
新图片的帧必须在 PNG 内，不应修改 `original/`。清单的源哈希始终指向固定 APK 的来源文件，
不应伪造为自有图片哈希；自有素材另外记录来源与转换。

原型只读取有逻辑键的清单行，不要求替换根包含全部原始素材。用 `--asset-root` 切换，无须改 C++。
测试程序可生成隔离的变色旅店/人物夹具（目标目录必须不存在）：

```sh
research/dungeon_village_1/work/prototype-debug-llvm/dungeon_village_asset_tests \
  research/dungeon_village_1/assets research/dungeon_village_1/work/r2-assets/my-replacement
research/dungeon_village_1/work/prototype-debug-llvm/dungeon_village_prototype \
  --asset-root research/dungeon_village_1/work/r2-assets/my-replacement --demo \
  --screenshot research/dungeon_village_1/work/r2-assets/my-replacement.png
```

变色夹具只是替换契约测试，不是修改原始美术或拟定正式美术方案。

## 当前待验收

代码与自动验收已闭环；用户已明确不做鼠标验证，记为免验而非通过，用户试玩仍未运行。
不再重试 computer use、不修改权限，不用直接领域命令冒充真实输入。
[试玩清单](../stages/PLAYTEST.md)保留人工参考；整个 R2 保留试玩未运行边界，不阻塞新研究。
原版动态基线不属于该免验范围，仍需实际 Android 运行证据。
