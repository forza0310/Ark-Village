# Steam基础人物数组与资源

2026-10-09。正式消费：[标题人物合同](../../ui/STEAM_TITLE_ACTORS.md)。本专题只做固定Steam样本的静态解析和APK资源身份差分，不修改Owner、schema、维护模式或产品。

## 来源与读取边界

- `arrays.cjs`固定Character2.cctor与WeaponData.cctor，验证DLL／metadata身份、具名入口／下一入口、PE范围，最多41,000字节／15,000指令，目标字段齐全立即停止。当前分别停在VA0x10295E71／0x102302A1，只读10,513／25,697字节，未继续整个方法。
- 工具是初始化成功分配／类型检查路径的窄静态常量解释器；不加载原DLL，不调用原函数，不运行游戏。new int[]未写元素为语言规定的零；寄存器、局部或静态字段未知则拒绝，引用数组空洞拒绝。未知算术、helper、指令、flags或跳出具名范围均失败。
- 源方法中的编译器helper0x100025D0／0x10002BB0／0x10002BE0分别由已见call追到入口，人工核了类型检查、引用数组写入、整数数组写入的成功路径。记录≤128字节hash再支持，未将未知helper默认无效；不因此认证异常运行时分配失败行为。
- `InitializeArray`使用真实fieldRef链，验证私有字段blob名等于数据SHA-256。`human.json`和`weapon.json`只保目标字段、值、分配／写入／发布锚点和blob身份；不是原指令全文。
- `inspect.cjs`保留固定两cctor入口4096字节与完整416字节GetWeponOffset的只读stdout入口，便于独立复查。入口输出在会话消费，不另存反汇编；截止处不足一条指令的字节不作为已核语义。

## 实际取得

6个目标字段：HUMAN_ANIME_SEB、HUMAN_ANIME_SEB_F、body_off、WEAPON_ANIME_F、WEAPON_WALK_POS、WEAPON_SEB。正常action0的身体方向／四步帧、四类武器固定frame0及全部行走偏移独立闭合；完整数组保留供后续动作逐条认证，不能把已有数据枚举称为所有动作消费者都完成。

`resources.py`只读现有纯归档／PNG解析函数，重新验证Steam容器、TextAsset和payload身份后解码human／weapon／common三组。图片不导出，仅写`RESOURCES.json`：98项中96项与APK字节同一，差异是common两目录整体；当前消费的图片3／SEB25仍独立解析一致。正文列出Steam裁片，APK比较在独立解析之后完成。

`verify-evidence.cjs`没有重用cctor读数算法：对行走128个坐标的实际机器码立即数逐项验证；对Steam解析后的16身体帧／16武器帧核独立裁片期望。hash、坐标锚点、资源摘要见[EVIDENCE.json](EVIDENCE.json)。本次没有C++构建、原窗口观察或自然路线测试。

```powershell
node research/dungeon_village_1/work/steam-actor-arrays/arrays.cjs human
node research/dungeon_village_1/work/steam-actor-arrays/arrays.cjs weapon
python research/dungeon_village_1/work/steam-actor-arrays/resources.py
node research/dungeon_village_1/work/steam-actor-arrays/verify-evidence.cjs
```

前两条只输出结构化摘要，需归档时保存到对应human.json／weapon.json；后两条只更新本目录派生JSON。没有导出原游戏图片／二进制、没有存档读取或进程操作，没有额外构建缓存或后台任务。全部证据由正式合同消费，旧归档与其它会话文件不退休、不清理。

## 剩余边界

完整共享scratch初始化与残留效果、绘制随机子树、Steam框架准入／PRNG交接仍需独立追踪。Steam武器定义→图片／chargeType的全表配对未在此工具重解；图片身份不自动等于每个定义已在当前进程可用。源端基础皮肤闭合不代表完整原版窗口、其它动作、产品接入或窗口对照已经验收。
