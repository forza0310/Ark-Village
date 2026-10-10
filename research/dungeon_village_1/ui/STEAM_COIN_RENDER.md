# Steam金币X4：独立局部交叉

2026-10-10。固定Steam2.56的独立机器码与资源证据见[工作包](../verification/steam-coin-render/README.md)及[复算清单](../verification/steam-coin-render/EVIDENCE.json)。这是只读静态交叉，不运行原游戏，不修改已验APK维护接口、世界随机或文件格式。APK正式合同继续见[COMBAT_RENDER](COMBAT_RENDER.md)，两者不因同图自动等价。

## 具名入口与绘制资格

`main.AppData.DrawEffect(Graphics)` RVA0x24BB30注册范围13,488字节。入口从效果列表实例偏移0x68依索引0向后读取；VA0x1024BC49–0x1024BC51的kind4分派跳到0x1024E2D0。该分支至0x1024E4E2前共530字节，独立确认：

- count来自记录索引1，严格负数跳过。没有count23绘制上界守卫；寿命退休由UpdateEffect负责。
- 索引2／3直接传入`Camera.ConvertScreenToCamera(int,int,int[])`，不读取活人物坐标、不再次做等距投影。
- 索引4／5是运动参数。线性项为`velocity*count`，二次项经整数乘法后除2，再相加除100；带符号移位／修正体现向零截断。
- 随后count达到`EFF_COIN_TIME[0]/2`则用`EFF_COIN_DIS_Y[0]`覆盖dy；`cmovg`将正dy改0，再加到转换后的Y。
- frame独立为`(count%14)/2`，不是末帧保持。common SEB槽0x5E=94、图片槽0x90=144，经显式Image重载`DrawSeb`绘制。
- 末尾返回列表共同尾部，索引加1再读下一项。X4与同X列表其他种类保原序；本节不把它推广为整个场景顶层。

Steam实际算式与APK在合法载荷下对应。机器码乘法保32位结果；维护接口的溢出拒绝仍是维护安全约束，不能以本交叉宣布异常溢出输入也完全等价。

## 常量、生产与寿命

metadata独立声明`EFF_COIN=4`、`EFF_COIN_DIS_Y`静态偏移0xA4、`EFF_COIN_TIME`偏移0xA8。复用既有具名AppData `.cctor`证据，当前DLL字节再次匹配其整个已登记方法hash；仅审读0x102692AD–0x10269344窗口，确认两个单元素数组分别为[-16]和[23]。没有重新导出该大方法全文。

`AddEffectCoin(sx,sy,wait)` RVA0x246040创建六项数组`{4,-wait,sx,sy,v,a}`后追加同一列表。v和a分别调用具名`GameUtil.SetVStop`／`SetAStop`，输入距离-1600及时间11。前者float32计算`2*dis/(t-1)`，后者`-2*dis/(t*(t-1))`，写数组时向零转整数，因此v=-320、a=29。这里证明的是此方法自身；死亡调用链传入wait是否恰2尚未独立核完，不能借APK初始-2填为Steam已证调用事实。

`UpdateEffect()` RVA0x2660D0的kind4分支先count加1，再读取TIME数组末项，达到23后删当前索引；共享循环仍增加索引，因此被移到当前位置的下一项本轮不推进。Draw和Update各自保持原职责，不把一次重画当作count推进。

| count | frame | dy | 加已核SEB偏移后的左上角 |
| --- | --- | --- | --- |
| 0 | 0 | 0 | 转换锚点+(-5,-10) |
| 2 | 1 | -5 | 转换锚点+(-5,-15) |
| 12 | 6 | -16 | 转换锚点+(-5,-26) |
| 22 | 4 | -16 | 转换锚点+(-5,-26) |

样本由本版已核生产参数与局部机器码求值，不是窗口截图采样，也不是任意GPU帧到逻辑计数的换算。

## 相机和资源身份

`Camera.ConvertScreenToCamera(int sx,int sy,int[] out)` RVA0x29F310独立读取静态`pos`的float x/y，以`cvttsd2si`向零取整；其逻辑输出为：

```text
x = DRAW_CENTER_X + sx - trunc(pos.x)
y = DRAW_VIEW_Y + DRAW_VIEW_H - (DRAW_CENTER_Y + sy - trunc(pos.y))
```

本批没有以字段名推定VIEW_H就是物理客户区高度，也没有核全部SetDrawCenter调用或OS／DPI矩阵。金币dy在该转换以后叠加，不反向改变保存的sx/sy。

Steam资源来自独立`EXE:resources.assets:1148:common`记录：img.inf显式144绑定`eff_coin.png`，seb.inf槽94绑定`eff_coin.seb`。二者与当前APK出版副本逐字节一致，因此本局部可以复用原文件，而非看到同名就alias。

- PNG：70×10、478字节，SHA-256 `64809d749638a027339c2fccbf712c937e5744e3de54f5bb9ec3fd8ae5aaa8ae`。
- SEB：148字节，SHA-256 `7a1c24539389ceabb23e875255e5a2344e5616e52a91faccde3a1de350b084d4`；单层7帧，各帧引用图144，源`(10*frame,0,10,10)`，偏移(-5,-10)，无翻转。

本批不新增素材副本。正式维护X4计划已经依APK合同交付；本交叉只提高上述局部的Steam来源等级，不改该接口或宣称Steam死亡wait、完整DrawEffect调用相位、暂停／模态下准入、声音、实际帧率与全场景遮挡已完成。
