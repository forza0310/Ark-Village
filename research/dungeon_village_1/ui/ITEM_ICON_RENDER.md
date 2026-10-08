# 普通道具图标：定义字段、分类底框与目录复用

2026-10-08。回应产品RQ03／RQ05及人物赠礼、商会目录的图标缺口。固定汉化重签APK1.0.8原表与原帮助器是本页维护来源；后继Steam共享图标帮助器及原列字段已完成静态局部交叉，各页面布局／自然输入和原窗口另验，详见末节。

## 原字段与资源桥

原`a/g.java:61–71`构造器将item第5列（零基）写入`g.g`。它不是道具定义ID、业务类别`g.e`或属性效果`g.f`。36条定义现由`StartupWorldItem::render_icon`完整投影；原表、库存和持久化字段不改。

例如定义0“北国马铃薯”图标5，定义20图标36，定义25图标34，定义29图标78；按定义ID直接裁图会错。原`a.g.C`有89项图标分类，`D=[1,4,6,2,3]`给分类背景槽；不能从人物／设施业务类别猜底色。

| 顺序 | 原type1帮助器行为 | 资源／矩形 | 相对调用锚点 |
| --- | --- | --- | --- |
| 1 | 背景槽`D[C[icon]]` | common图片24，`icon_back00.png`，144×18；`(slot×18,0,18,18)` | (-1,-1) |
| 2 | 道具前景 | common图片9，`tresureIcon00.png`，240×96；`(icon%15×16,icon/15×16,16,16)`，整数除法 | (0,0) |

以上是PNG裁片，不是SEB同号帧。使用原尺寸绘制；锚点是前景左上角，背景多一圈。若产品整体缩放，统一变换两个命令，不能分别按不同中心缩放。

类型0只绘前景、类型5只绘指定背景，装备2／3／4另用18×18图集；这些不是普通type1的可互换别名。此接口只维护本页type1，不为未知类型返回默认图。

## 各页面共享帮助器，布局分别保留

| 用途 | 直接来源 | 第1行前景位置／行距／可见行 |
| --- | --- | --- |
| 人物礼物64、slot4普通道具 | `b/g.java:6293–6296`委派`c/n.java:1113–1133`，调用1129 | (31,95)，24，4行 |
| 设施道具75 | `b/g.java:7264–7285`，调用7281 | (30,95)，19，5行 |
| 商会84 | `b/g.java:7984–8015`，调用8004 | (30,95)，19，5行 |
| 赠礼66／能力结果69 | `b/g.java:6500／6644` | 同type1图块；66有独立运动计数，69标题锚点依文本宽度，不能直接套列表坐标 |
| 设施投放76 | `b/g.java:7629` | 同type1图块；抛物／人物／背景／计数完整演出仍独立待交付 |

图标共享不意味着各列表行数、marker边距、滚动、选中和确认顺序相同。研究窗口只接现有目录行的图标，当前简化布局仍标适配；本批不将其改称精确原页面，也不由图标绘制扣库存、收款或发效果。

## 维护接口与验收范围

`startup_world_item_icon_draws(state, item_definition)`在[startup_world_visuals.hpp](../prototype/include/dungeon_village_prototype/startup_world_visuals.hpp)声明，返回两个按原序排列的`StartupVisualDraw`。使用稳定定义身份查找，不以map.at异常代替无定义拒绝。无rules、未知定义、图标越界明确返回失败，不取模夹取坏输入。

定义投影由[编译器](../prototype/scripts/compile_startup_world.mjs)读取原表列5并验证0–88；该上限来自原分类表C，而非前景图集最大可裁90格。静态rules不进Owner codec；本批没有新增存档字段／更改原表身份或随机调用。

[现有visuals套件](../prototype/tests/startup_world_visuals_test.cpp)以36个原图标字面量核投影和矩形，以四个独立样本核四类背景，加载实际PNG核边界，检查坏载荷拒绝和完整Owner摘要不变。数据生成测试保留原魔法壶元素断言并加入同一记录的图标5预期，没有删有效断言。

窗口在礼物64普通道具、设施75及商会84目录消费计划；每个调用锚点仍属研究布局。实际构建、集中测试与有界窗口结果见[验证记录](../VERIFICATION.md)。完整66／76演出、原APK／Steam动态和产品接线另验。

## 来源身份

[只读证据](../work/item-icon-contract/EVIDENCE.json)核36定义、89分类、两张原PNG、三个目录及结果页的明确调用坐标。
复用已冻结`Lb/g;::c(Lkairo/android/ui/o;IIII)V`的code_item偏移228576、994指令字节，指令hash为`c5ddbe5c418800ac0342ab54cd1bd878d506d7a86ec768406ba690c20484c6f8`；原DEX身份保持`b4386a0de612fe18196a390633607ef36c69c2a63881244832314e3bf4902d1a`。本批重读字节复核，并核C／D字段引用；未复制反编译实现或重新生成整类源码。
PNG hash：背景`c80d01870c41132cfd9f96b74d241d1aee412024c059a7db17f7fc379c17df30`，前景`c40493918c5fadeedb3a74afb2ee05b27902c296fa245309515e22ba20ba0982`；全部已有[原素材清单](../assets/MANIFEST.tsv)记录，无新素材副本。

## Steam字段／分类与共享帮助器交叉

[Steam工作包](../work/steam-attribute-item-render/README.md)与 [有限合同摘要](../work/steam-attribute-item-render/CONTRACTS.json)只读固定真实DLL／metadata，不使用dummy DLL实现或图像同名推测调用。

| 链路 | 本轮实际证据 |
| --- | --- |
| 原表列5→图标字段 | `data.ItemData.Load` RVA0x21A170，0x1021A23A–0x1021A251校第5列、ParseInt并写`icon_`偏移0x2C；其它type1_/2_/3_分别占0x20/0x24/0x28，未与业务类别混用 |
| 分类数组初始化 | `ItemData..cctor` RVA0x21A610分别建89项、5项数组，用InitializeArray填充后写静态`ITEMBACK_ROOT`／`ITEMBACK_INDEX`偏移0x8/0xC |
| 初始化载荷身份 | 使用槽0x110F5718／0x110F22A4→metadata FieldRef→注册类型5607／TypeDef4129→字段默认载荷；356 B位于metadata0x2FBC85，20 B位于0x2FABAA，全部值逐项与APK C／D一致，不仅按二进制搜索命中 |
| type1实际绘制 | `form.SubForm.Draw_icon` RVA0x30C350，type1分派到0x1030CBEC；分支先读`ITEMBACK_INDEX[ITEMBACK_ROOT[icon]]`，图24槽×18底框18×18、X/Y−1，再图9按15列裁16×16前景。两个实际DrawImage调用在0x1030CCA4／0x1030CD1A |

英／日两份Steam item表各36×25，稳定ID与全部列5图标值逐项等于固定APK；本地化列另保留。89分类和 `[1,4,6,2,3]`背景顺序也已沿真实默认数据链核对。对应图24／图9与APK字节相同；这里是“字段含义＋分类默认值＋type1消费者＋资源身份”的局部组合，不能宣布所有道具业务与所有页面相同。

本轮没有认证Steam64／75／84等页面实际调用的位置、行距、缩放、点击／确认或自然库存路径；APK页面表仍独立有效。也没有原Steam窗口图标测试，不把下面的维护原型窗口当作Steam动态。

## 后继维护窗口与验收层级

研究原型的有界 `world-item-gift` 窗口沿自然商会购买→人物60→目录64 slot4运行，实际读取并显示原道具前景和分类底框。证据为 [窗口截图](../work/item-icon-contract/world-item-gift.png)及 [日志](../work/item-icon-contract/window-item-gift.log)，八帧取样。raw64预运行steps15912，终点4人物／138338次共同随机抽取／2270G；日志末端`updates497`是窗口内部更新计数，不能当成总预运行长度。

这补齐“维护计划已接线并有自然目录路径”层级；完整66／76演出、精确原页面布局、原APK／Steam动态及产品验收仍各自登记。旧窗口日志和上批验收保留，后继证据不覆盖历史。
