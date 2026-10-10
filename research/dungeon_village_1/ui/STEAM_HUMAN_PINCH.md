# Steam人物危险图标：偏移表与实际锚点

2026-10-10。本页补齐[人物表现合同](STEAM_HUMAN_PRESENTATION.md)此前保留的`Character2`静态`+0xA0`偏移表。来源是固定Steam DLL中的直接常量初始化及raw60普通肖像消费者，原APK仅做局部Java交叉；没有窗口像素实测。方法身份、地址、指令锚及资源摘要见[研究包](../work/steam-human-pinch-offset/README.md)和[EVIDENCE.json](../work/steam-human-pinch-offset/EVIDENCE.json)。

## 原表不是运行时推算

Il2CppDumper字段映射为`game.Character2.OFF_EFPINCE`，类型`int[][]`，静态偏移`0xA0`；拼写是原程序的`EFPINCE`，不重命名后反推其它同名表。

`Character2..cctor`具名方法起点RVA `0x293560`。本表初始化局部为VA `0x102968DA–0x102969F0`（半开区间，278字节）：分配长度2的外层数组、各长度2的内层数组，四条立即数分别写入内层元素0／1，再按顺序挂到外层元素0／1，最后在`0x102969D7`写入静态字段`+0xA0`。

| 行 | x偏移 | y偏移 | 原立即数写入地址 |
| --- | ---: | ---: | --- |
| 0 | −14 | −28 | 0x1029691A、0x1029692B |
| 1 | 5 | −28 | 0x1029698A、0x1029699B |

负值来自原signed int32立即数`0xFFFFFFF2`／`0xFFFFFFE4`，不是按截图估算。此局部没有`RuntimeHelpers.InitializeArray`调用，不存在本表待解的RuntimeFieldHandle默认blob；与同为静态二维表的MOVE_DATA初始化方式不同。

## raw60怎样选择并使用

普通肖像consumer沿[原MOVE_DATA六段](STEAM_HUMAN_PRESENTATION.md)：读取当前段direction，当direction为2或3取偏移行1，否则取行0。原比较指令位于`0x102D030B／0x102D0310`；行1赋值位于`0x102D0315`。本批已核六段只产生direction1／2，direction3是helper保留分支，不表示该循环实际出现第三方向。

令`X`为原段内RateConvert得到的肖像x，`H=0`表示`myHouse_[2]==1`，否则`H=−12`。危险图标最终调用：

```text
row = (direction == 2 || direction == 3) ? 1 : 0
DrawSeb(SEB39, image22, frame0,
        x = X + H + OFF_EFPINCE[row][0],
        y = 121 + OFF_EFPINCE[row][1])
```

因此y恒为93；direction1时x为`X+H−14`，direction2时为`X+H+5`。例如原段0的X75／direction1：有住宅为(61,93)，无住宅为(49,93)；原段3的X98／direction2：有住宅为(103,93)，无住宅为(91,93)。这些是由已核表和调用式计算的逻辑锚点示例，**不是本轮原窗口截图测量**；段内实际整数插值仍沿原RateConvert，不自行换浮点曲线。

它只会在原普通肖像的危险显示条件通过后调用：存在实际`Character2`，非负frame的每12槽前6槽，原整数`100*powerBar_[PB_AFTER]/GetHpMax() <= 30`。血条长度使用PB_NOW，危险提示使用PB_AFTER；本页仅补偏移，不改变前页已核门槛。无实际实例时在本判断之前返回，不能给定义肖像硬补危险图标；state7倒下展示由raw60外层另走，不套这个普通循环。

SEB39的frame0对应image22 `ef_pinch.png`源(0,0,11,13)，SEB局部x/y偏移均0，所以该分片左上位置就是上述锚点。PNG为22×13，另有frame1的右半裁片；raw60此调用明确固定frame0，不能因有两帧就让它随闪烁交替。

## APK局部交叉与使用边界

固定APK生成Java中，`c/b.java:141`的`bD`同为`{{−14,−28},{5,−28}}`；`c/n.java:1645–1660`对应人物详情肖像消费者也按direction2／3选行1，把x偏移加到段内肖像x及住宅偏移、y偏移加121，使用SEB39/image22/frame0。这是**表值和具体调用链两项分别核对后**的局部一致，不是因类名、字段名或资源同名推断相同。

本包记录两份生成Java全文hash及17行有限窗口hash，没有新增DEX指令审计。Steam结论以本地机器码常量与consumer为独立依据，APK交叉不替代其低层证据；也不据此宣称两版整个Character2危险表现相同。

本合同供未来只读皮肤生成明确图元请求：人物当前段direction、独立肖像位置、住宅状态以及PB_AFTER条件均有来源。完整人物／武器树、GetAnimeIndex、世界地图人物的危险特效入口、窗口缩放与像素对照仍是独立范围；未改C++、产品代码、原游戏或存档。
