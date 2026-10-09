# 标题与纪录人物基础皮肤

2026-10-09。固定汉化重签APK1.0.8的局部源码、原表、原SEB与PNG交叉。本文补充[标题调度](TITLE_PRESENTATION.md)及[启动皮肤](STARTUP_SKIN.md)，不把APK的方向号、图层和坐标认证成Steam2.56相同实现。[具名证据](../work/title-actor-skin/EVIDENCE.json)记录22个源码窗口、原表哈希和98个实际资源的身份。没有原窗口操作、内存读写或存档改写。

## 已核调用链和顺序

标题[b/h:129](../work/decompiled/sources/b/h.java:129)在原20槽排序后逐人执行；纪录[b/e:59](../work/decompiled/sources/b/e.java:59)使用开页时已生成的原序名单。二者共享`c/n.a`临时人物助手，但传入周期和阴影资格不同。

| 项目 | 标题背景人物 | 纪录插画人物 |
| --- | --- | --- |
| 定义 | q槽定义0…10，生人不筛职业／到访开放 | p=1池随机交换所得名单，或已核后备名单 |
| 步帧 | `(age%20)/5` | `(pageCounter%16)/4` |
| 朝向 | direction=1传1，否则2 | 固定1 |
| 锚点 | q.x、q.y+surfaceHeight−240，再叠标题外层横移 | `120−(min(count,5)−1)*13+26*i`、153，再叠纪录表单变换 |
| 主武器参数 | `e.v[0]` | `e.c().n`，`c()`返回`bt[v[0]]` |
| 基础图层调用序 | common25阴影→武器→human身体 | 武器→human身体；本窗口没有显式阴影调用 |

标题[只读投影](../prototype/include/dungeon_village_prototype/startup_title_presentation.hpp)的`body`请求需要展开武器和身体，不能当作“只画一张人物PNG”。逐人阴影／身体之间不插另一个人的图层；grass仍在所有标题人物之后。纪录没有这一阴影调用，不能因标题复用相同助手就自动补上。

## 职业、性别和武器身份

[a/e.b:717](../work/decompiled/sources/a/e.java:717)通过共享定义当前职业`t`取`bu[t]`；[a/h.a:102](../work/decompiled/sources/a/h.java:102)读取职业`f40e[e.f21c]`。所以身体图片由**当前职业及性别**决定，不是人物定义ID、基础职业、角色实例ID或头像序号。

`job.txt`零起列索引3、4就是两性别图片索引；原`human/img.inf`为0…31、33…37，**没有32**。原表23职业的两性别字段共引用37张实际图，其中部分职业不开放某性别，但原数组仍有对应值；此局部绘制查询不承担转职准入判断。

标题／纪录助手只向武器绘制传当前主武器，不读取防具1、防具2或饰品来叠加衣服。身体衣装由职业整张PNG选择承载；这只是本条调用链的事实，不能推成其它页面或所有人物效果都没有附加图层。临时人物不把e绑定为世界实例，其当前职业／性别／武器应由调用者读取权威定义，不能从创建时的actor metadata推断永远不变。

`c/n.a(canvas,x,y,weapon,step)`先计算W身体scratch，再调用[c/b.a:1079](../work/decompiled/sources/c/b.java:1079)直接画武器，最后画身体。`weapon==-1`在助手边界明确跳过武器；正式标题／纪录的正常主武器应来自真实定义。已出版`StartupWorldEquipment.render_image/render_style`分别对应`p.e/p.h`，不得把weapon定义ID、装备图标ID、攻击类别或武器名称当PNG/SEB序号。

## 行走帧、裁片与坐标

[c/n:730](../work/decompiled/sources/c/n.java:730)写`W.k=0、h=身体图片、i=step、j=facing、l=age`。
`c/b.a(shape,action,step,counter)`在action0、正常`N==-1`时取`bd[0]+facing`和`be[0][step]`。二者分别是方向SEB 0…3及原步帧0…3；这里不会按age重新计算帧。标题和纪录的步帧周期必须保留在各自调用者。

| 资源 | 图片覆盖／SEB | 已核请求的实际裁片 |
| --- | --- | --- |
| 阴影 | common图片3、SEB25 `shadow00.seb`、frame0 | `(0,0,12,2)`，SEB offset=(-6,-1) |
| 身体 | human当前职业性别图片、SEB `walk00/01/02/03` | 单层，18×24；四帧x依次0、18、0、36；y=`24*facing`；offset=(-9,-24)，无翻转 |
| 挥动类武器 | weapon当前图片、SEB `sword00…03` | 行走均frame0；21×25，x0、y=`25*facing` |
| 弓 | weapon当前图片、SEB `bow00…03` | frame0；21×25，x0、y=`25*facing` |
| 枪 | weapon当前图片、SEB `spear00…03` | frame0；26×28，x0、y=`28*facing` |
| 大剑 | weapon当前图片、SEB `greatSword00…03` | frame0；32×35，x0、y=`35*facing` |

四类武器的行走帧0均无SEB内部offset和翻转；身体frame2重复frame0的站立列，不能按“每帧向右取下一格”实现。[d/a:334](../work/decompiled/sources/d/a.java:334)的单图重载用调用者传入PNG覆盖SEB记录默认图片，保留记录的裁片和偏移。例如短剑图片50配`sword01.seb`，不能误用SEB原默认图片0而画成斧。

武器SEB序号是`4*render_style+facing`；行走`bf[style][0][step]`全为0，武器**不**随身体step切SEB帧。武器锚点额外采用[a/p.x](../work/decompiled/sources/a/p.java:34)：

| style | 朝向0 | 朝向1 | 朝向2 | 朝向3 |
| --- | --- | --- | --- | --- |
| 0 挥动 | (-3,-28) | (-3,-28) | (-18,-28) | (-18,-28) |
| 1 弓 | (-7,-22) | (-7,-22) | (-15,-22) | (-15,-22) |
| 2 枪 | (-5,-28) | (-5,-28) | (-20,-28) | (-20,-28) |
| 3 大剑 | (-9,-38) | (-9,-37) | (-22,-37) | (-23,-38) |

表是step0/2的偏移；step1/3只把y加1。所有值为原逻辑像素，不是任意分辨率下的物理像素。人物锚点、上述武器偏移和SEB内部偏移各加一次；不把武器的`p.G`深度排序辅助值加到本条直接绘制坐标，也不套世界场景的绘制队列排序。action0不会触发挥动命中光，所以基础计划不附common40…43光斩层。

## 原scratch副作用及维护边界

原助手不是严格纯函数。它写共享W的`k/h/i/j/l`，身体计划写静态`de…di`及`W.bl[5…10]`，身体绘制结束又写`W.bx`。`W`初始化由[c/n:1983](../work/decompiled/sources/c/n.java:1983)执行`new b → a(0,0,0) → f143e=-1 → f()`；`a()`使N=-1，刚创建对象的am/aq/cd等为零或空。

但[c/b:4522](../work/decompiled/sources/c/b.java:4522)四参数绘制仍含全局选中、HP、携物N、倒地、调试、伤害及cd效果分支。五参数scratch setter没有清这些字段；[b/c:1962](../work/decompiled/sources/b/c.java:1962)还有调试场景对同一W调用`d()`和攻击助手。**本批没有证明任意历史路径进入标题时W全部附加字段必为零。** 正式交付因此只命名为正常W的基础图层查询，不复制整个原W绘制，也不把任意残留效果静默认证为“不存在”。

在已明确的正常初始化、action0、无上述叠加条件内，基础图片计算不需要随机、声音、资金、装备装配或创建世界人物。维护[只读复合计划](../prototype/include/dungeon_village_prototype/startup_title_actor_skin.hpp)只查冻结规则及显式参数，按shadow可选→weapon可选→human body返回；身体仍由既有human SEB适配器绘制。它不访问Owner、存档、文件、输入系统或可写随机，不扩应用schema。

坏职业、性别、主武器、step或方向返回空结果；图片32不补透明fallback。职业／性别转换及装备更换由已有规则Owner负责，调用者应该把已提交的当前值传入；本接口不替换整个标题状态机、不自动将草稿装配到世界，也不处理世界人物全部叠层。

## 验证资格与后续

本次静态审计解析4个身体SEB、16个武器SEB和阴影SEB，对37个身体图片及33个武器定义的实际请求裁片核边界，登记源码、原表、目录和PNG哈希；结果见[工作记录](../work/title-actor-skin/README.md)。既有visuals套件扩展原图CPU合成与独立帧/偏移期望，包含身体最后覆盖、两性别、无武器和拒绝分支；实际构建与回归结果由主会话集中验收后记录，不以静态脚本通过代替C++测试通过。

还需在完整标题／纪录表现消费者中接当前定义读取、原顺序和页面锚点；本批CPU图块拼图不是完整UI验收。Steam对应素材和实际角色叠层尚待其自身代码／窗口交叉，不能因共同名字或PNG相同就将本文升格为Steam调用事实。完整W的异常／调试残留资格单独记录，不作为正常基础皮肤交付的隐含保证。

主会话集中Release构建及visuals检查已通过，最终331315检查；已实际查看[760×1230合成图](../work/title-actor-skin/actor-pieces.png)，新增四行展示四武器风格／双向／四步。回归、资源规模和完整限制见[验证入口](../VERIFICATION.md)，不将检查数当覆盖比例。
