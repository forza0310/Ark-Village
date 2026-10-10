# Steam设施图块、朝向和真实资源

2026-10-09。接续[建设列表绘制](STEAM_BUILD_LIST.md#建筑字段到mapchip图层)，本页闭合固定Steam2.56的两个pattern数组、85设施定义到85 mapchip／SEB的静态对应，以及common图片98、103的实际Steam资源。证据与复算在[工作包](../verification/steam-mapchip-patterns/README.md)。这是源端静态交付，未修改C++／产品，也没有原窗口验收。

## 数组来自Steam初始化

`TenantData.cctor`具名RVA0x2253D0；从入口解释成功分配路径，两个目标发布后立即停止于VA0x102264ED，共4381字节。静态字段`SETPATTERN_MAPCHIP +0x48`发布于0x10225D9D，`SETMAPCHIP_ADDRESS +0x4C`发布于0x102264EA。整型数组未写入元素是语言规定的0；引用数组空洞、未知调用／寄存器／分支均拒绝，未用APK数组填值。两个四元素blob另走真实metadata fieldRef/default链并核私有字段名等于blob SHA-256。

每个piece按表中顺序绘制，地址(u,v)转为锚点偏移`(30*(u+v),15*(u-v))`：

| pattern | 朝向 | frame序列 | 地址序列 | 锚点偏移序列 |
| --- | --- | --- | --- | --- |
| 0 | 0 | 0 | (0,0) | (0,0) |
| 0 | 1 | 1 | (0,0) | (0,0) |
| 1 | 0 | 0,2 | (0,1),(0,0) | (30,-15),(0,0) |
| 1 | 1 | 1,3 | (-1,0),(0,0) | (-30,-15),(0,0) |
| 2 | 0 | 0,2,4,6 | (-1,1),(-1,0),(0,1),(0,0) | (0,-30),(-30,-15),(30,-15),(0,0) |
| 2 | 1 | 1,3,5,7 | (-1,1),(0,1),(-1,0),(0,0) | (0,-30),(30,-15),(-30,-15),(0,0) |

pattern2的朝向转换包含中间两片绘制顺序交换，不能只把整栋PNG水平翻转。原`DrawMapchip`对道路type6仍覆盖frame：朝向1用1，其他朝向用11；地址仍取上表。建筑分片内部还有SEB自己的裁片、偏移和翻转，应在上述锚点偏移之外消费一次。

## 定义到SEB的完整静态对应

本批独立核`TenantData.Load` RVA0x2217C0与`MapchipData.Load` RVA0x21BFB0，列号均从0计：

| 原表列 | Steam字段 | 后继用途 |
| --- | --- | --- |
| tenantData列9 | mapchipDataId_ +0x3C | 查mapchip定义，不能直接作为图片槽 |
| tenantData列10 | mapchipPattern_ +0x40 | 查两个pattern数组 |
| mapchip_main列3 | seb_ +0x24 | resMapChip_内SEB槽 |
| mapchip_main列5 | tenantDataId_ +0x2C | DrawMapchip反查设施、取pattern与type |

Steam `resources.assets`的TextAsset1144（xls）内Japanese.lproj两表各85行，逐字节核到已有Steam解出副本；设施表36列、mapchip表7列。85次“设施→mapchip→反向设施”身份全部回到自身，pattern分布为0类75项、1类7项、2类3项。逐定义、逐朝向、逐片、逐SEB层的图片槽／路径／crop／offset／flip及资源哈希集中在[RESOURCES.json](../verification/steam-mapchip-patterns/RESOURCES.json)，不手抄85行形成第二份易漂移目录。

图块归档是`KairoGames_Data/resources.assets`的TextAsset1143 `image`，对象偏移7653624、大小232512字节；载荷SHA-256 `cd0f5b294557c663c01ec7cc9c63d3ccb79028ec94e2a6f1b1fb7ae7c0c9bfd2`。本批直接解读其img.inf／seb.inf，85个图块SEB及其图片与现有APK出版副本字节一致；这个一致性是逐资源核验结果，不能推广到common或所有语言覆盖。`resMapChip_`的动态安装过程、语言替换及运行时自定义图片字典仍需独立合同，当前表对应不代替这些加载消费者。

### 源端异常与调用范围

原SEB存在名义帧数以外的实际关键帧，不能按头部帧数删记录。例如plain00.seb名义3帧，实际有frame3；casino名义2帧，仍有frame2/3。解析器保留所有原记录，并与已有全素材索引的异常列表核对。

本批发现并显式冻结以下源事实，不修改原图、原表或原SEB：

- image SEB27 plain00的frame3裁片(0,87,60,29)越出180×29原图。
- image SEB38 sea00的frame1／2裁片(60,0,60,29)／(120,0,60,29)越出60×29原图。
- image SEB67 t_casino00的frame2／3引用图片43／44，image/img.inf中没有这两个槽；它们不在当前pattern请求0／1内。

建设raw21已核固定传朝向0。85定义的该方向请求均有精确关键帧，引用图片存在且裁片在图内。若无条件枚举朝向1，27定义请求会超出SEB各层实际首尾帧范围，定义19则请求sea00的越界frame1。不能据此宣称全部85定义均可旋转，也不能补一张演示建筑图让枚举“通过”。旋转按钮准入和地图世界绘制另验。

`DrawFrame` RVA0x7BC710以lineNo=-1转`_draw`→`GetSpritesLocal`→逐层`GetSpriteLocal`。后者在VA0x107C1A6D／0x107C1A83确认请求小于首帧或大于尾帧时走0x107C1EF6，返回null；`Draw`在0x107BF9BE对null跳过绘制。这里的范围依据是**该层实际关键帧**，不是SEB头部名义帧数。缓存命中、插值、合成及全部SEB格式仍不在本批全量认证范围内；本索引对需要插值而无精确帧的请求会拒绝。

## 两张common差异图与消费者

两图在`KairoGames_Data/resources.assets`的TextAsset1148 `common`：对象偏移8314752、大小1021784字节，载荷SHA-256 `9178fcbb4bf387affd3fb1d23d02aa21295e6af1a68267d64876aa3fadd36092`。以下路径表示归档条目，不冒称已存在独立Steam PNG文件。

| 图片槽／条目 | Steam字节与尺寸 | Steam SHA-256 | 已核消费者 |
| --- | --- | --- | --- |
| 98／common/tenant_resident.png | 512字节，50×15 | `1a02c85d1b51db3da5789a6469d60bcf72a44de346f5ec251e9bb56169adb7c7` | raw21 type13、有入住者时common SEB81 frame0；裁(0,0,50,15)，内部偏移(-25,-15)，无翻转 |
| 103／common/number05.png | 701字节，100×21 | `9d94dd8c289f00459f4d9e7586942b5d60c130c5da381fa08b087e5194512b6d` | raw21 type12住宅数量`Draw_multiValue`使用common SEB12；21个原关键帧详见资源摘要 |

原raw21的resident调用锚为(J+42,K+40+37r)，加SEB偏移后的图片左上角为(J+17,K+25+37r)。数字消费者锚为(J+75,K+47+37r)；SEB12中数字0–9裁片为`(9+8*d,0,8,10)`，偏移(0,0)，其余符号帧保留原记录，不根据PNG列数猜含义。

两图不但PNG字节不同，RGBA像素hash也分别不同于APK出版副本。SEB12／81本身与APK同字节，不能因此替换为APK图片。未来正式资源交付应发布这两个Steam条目并登记来源；本批只核证，不复制PNG、不修改旧冻结皮肤包。

## 验收与仍待闭合

本批8个具名方法窗口共10685字节（包含cctor必要前缀），19个固定调用／赋值／边界锚。输出覆盖85定义、87个SEB、194个既有资源身份；异常裁片、缺图片槽、超层范围请求都显式保留。没有构建、窗口或产品接入结果。

尚待：Steam实际resMapChip安装／全部语言覆盖、建设Init/Update与旋转准入、世界地图深度／遮挡、完整SEB插值及Graphics对超图裁片的运行时政策。源端闭合的两个pattern数组与朝向0资源对应可直接用于后续维护计划；其余资格不能从本批静态枚举补出。
