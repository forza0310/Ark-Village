# 声音资源、播放语义与维护请求

后继Owner交付在[声音操作模块](../prototype/AUDIO_REQUESTS.md)：原25点已显式分类为B／C／D，唯一typed队列与旧ID接口互消费；补接新遭遇BGM2和普通通知24，通知到计数1才C11。集中验收见[交付记录](../VERIFICATION.md#声音操作遭遇通知与steam菜单语言2026-10-09)，设备播放／标题独立队列及Steam完整生命周期仍未完成。下文原“整数队列缺口”保留发现时证据，不再代表当前在途接口类型。

2026-10-09。回应产品[信息／音频接入需求](../../../docs/reference/RESEARCH_REQUESTS.md#information-audio-animation-89f157c)。固定APK1.0.8的资源、局部Java与Steam2.56七个具名方法交叉，证据见[工作记录](../verification/audio-contract/README.md)及逐ID哈希（本地核对材料，不随仓库交付）。后继已发布[独立音频素材](../assets/audio/README.md)及[确定性清单](../assets/audio/MANIFEST.json)；没有播放、操作原游戏、改Owner或schema。

## 已闭合的APK资源链

`b/a.java:514`创建`AppData.C`的资源组，按`d/a.o[1][12]`加载`sound`；`kairo/android/ui/s.java:333`把该名转为`/res/raw`，读取`snd.inf`。**音频ID是实际清单中对应的声音对象槽位**；本样本26行无显式索引／路径，资源初始化按行填`C.f593c`。

文件名转换`kairo/a/a/a.java:30`把`.mld`变为`.ogg`；`s.b(String)`去扩展名并默认类型`raw`，用Android资源查找ID；`y.b(String)`保留此资源ID，播放时由`MediaPlayer.create`创建播放器。实际清单全为小写，不能据此宣称大小写任意互换。

`d/a.x`仅有4个BGM名及16个旧SE名，并含`MainA`等与实际raw不同的旧字符串；**它不是当前26槽的文件索引**。本批直接从固定APK ZIP读`snd.inf`和26个Ogg，再逐字节核研究副本，不按旧x数组顺序猜文件。

| ID | APK实际raw文件（均为.ogg） | 初始化通道／重复 |
| --- | --- | --- |
| 0 | kairoquest_title | BGM0／无限完成后重播 |
| 1 | kairoquest_main | BGM0／无限完成后重播 |
| 2 | kairoquest_battle | BGM0／无限完成后重播 |
| 3 | kairoquest_happy | BGM0／无限完成后重播 |
| 4 | kairoquest_yorokobi | SE1／单次 |
| 5 | kairoquest_yorokobishort | SE1／单次 |
| 6 | kairoquest_kanashimi | SE1／单次 |
| 7 | se_06 | SE1／单次 |
| 8 | se_00 | SE1／单次 |
| 9 | se_03 | SE1／单次 |
| 10 | se_07 | SE1／单次 |
| 11 | se_09 | SE1／单次 |
| 12 | kairoquest_seattackmnst1 | SE1／单次 |
| 13 | kairoquest_seattackmnst2 | SE1／单次 |
| 14 | kairoquest_seattacksword | SE1／单次 |
| 15 | kairoquest_seattacklance | SE1／单次 |
| 16 | kairoquest_semagicheal | SE1／单次 |
| 17 | kairoquest_semagicfire | SE1／单次 |
| 18 | kairoquest_semagicblizzard | SE1／单次 |
| 19 | kairoquest_semagicthunder | SE1／单次 |
| 20 | department_levelup | SE1／单次 |
| 21 | z_se07 | SE1／单次 |
| 22 | z_se08 | SE1／单次 |
| 23 | se_04 | SE1／单次 |
| 24 | se_05 | SE1／单次 |
| 25 | se_01 | SE1／单次 |

通道由`b/a.java:516–522`实际配置：y组0…3调用`w.d()`设loop=-1及`w.a(0)`；z组4…25调用`w.a(1)`，保留loop=0。这里的0／1是通道号，与资源ID1／2不是同一概念。文件共2,659,933字节；完整SHA-256、原ZIP条目、研究副本路径、声道数／采样率和容器granule时长均已登记。时长来自Ogg容器，不是播放实测、逻辑tick或完整解码认证。

## 播放入口不能合并为一种整数操作

| APK入口 | 已核调用语义 | 对维护输出的意义 |
| --- | --- | --- |
| `AppData.b(id)` | 特定ap前缀退出；目标已经state2则退出；否则停止y组中state2的曲目，再播放目标 | BGM替换与同曲幂等，不能每次请求重置乐曲 |
| `AppData.c(id)` | 同样ap前缀退出；直接交SoundPlayer播放对象 | 普通声音请求；同对象正在播放时底层会重头播放 |
| `AppData.d(id)` | 先`Main.a(sound)`登记jingle并把BGM通道音量设0，再调用c(id) | 除播放外还有压低／恢复BGM的副作用 |

资源ID可决定哪个文件与初始化通道，但**不能独自证明本次调用应走b、c还是d**。例如`b/g.java:4721、4881`确有`d(5)`；不能见5属于SE便一律普通播放。其它调用者仍需按已经解析的真实入口保留操作资格，不由后端按名称或猜测每个SE都压BGM。

`Main.a(sound)`写共享跟踪对象h及计数i=20，将BGM音量设0；恢复时调用`Main.a(null)`，按当前系统音量设置`J[3]*51`恢复。主循环只在h的状态为3或0时递减；旧i≤0才恢复，所以从20开始需21次符合条件的主循环检查。**不是播放开始20帧后自动恢复，更不是20秒**。后来的jingle覆盖跟踪对象和计数；这段没有同时停止上一段普通SE。

## APK底层重复、通道及生命周期

`kairo/android/ui/y.java`为当前Ogg对象。已存在MediaPlayer、状态2且实际isPlaying时，播放请求先反映音量／静音，再`seekTo(0)`返回；同对象不会分配第二个叠加实例。不是所有同ID请求都被去重：b包装主动忽略同曲，而c/d会进入这条重头路径。

新播放先停止／释放旧MediaPlayer；若SE通道静音则不创建。BGM静音仍可创建并以0音量推进。正常创建设置完成监听、状态2、注册播放端口后start；普通单次完成会停止并release，状态改3。loop非0的完成监听另起线程seekTo(0)／start重播，本样本没有设置`MediaPlayer.setLooping(true)`，不认证无缝循环。

SoundPlayer有4个播放端口，两类通道各保存音量／静音。当前Java中取端口先找空位，满时返回−1；本批没有观察到据priority抢占旧声音的实际分支，不能擅自实现高优先级必然抢占。音量为`level*51`再除255转为播放器系数，标题和设置页都按系统J[3]/J[4]应用；声音音量控制与暂停游戏逻辑不是同一事件。

Android `onStop`置前台标志false并调用SoundPlayer.d：先缓存looping播放名单，再停止所有已登记声音对象。恢复在框架`g()`确认未锁屏、应用前台且有窗口焦点后调用SoundPlayer.c，仅重新播放缓存的循环对象。由于停止已release且恢复重新创建，APK此链是从头恢复循环曲，不保留原播放位置；普通SE不在恢复名单。

**单纯失去窗口焦点不同**：框架只等待100ms并暂不继续该轮，没有在此分支调用SoundPlayer.d；`onPause`自身也没有直接停声调用。不能把“失焦即停全部／恢复位置”写成APK已核事实，更不能不经Steam交叉照搬成PC策略。资源组退休`s.a()`依次dispose声音、清槽并重置组；w注册表与端口生命周期有明确释放，文件枚举数量不是活动MediaPlayer数量。

## Steam已核局部与待核项

Steam metadata独立声明相同26个资源ID名及BGM0／SE1类型。七个完整具名方法共1,168字节：`AppData.PlayBgm`确认目标state2时忽略，遍历其BGM数组处理旧曲，再调用SoundPlayer.Play；`PlaySe`直接播放；`PlayJingle`真实调用`Main.SetJingle(sound,20)`后PlaySe；`StopBgm(id)`按对象交SoundPlayer.Stop。

Steam的SoundPlayer.Play还消费masterVolume与mute，不应从APK只有一层音量的实现推出完全同式。Steam `Suspend/Resume`这两个方法只改`suspend2_`标志，**不是APK同名操作的停止／重建实现**；后续消费点、Unity AudioSource、循环／重复／满端口政策、应用失焦和最终声音文件尚未闭合。metadata常量名相同不证明EXE AudioClip与上述APK Ogg字节相同。本批只给APK原文件身份，不假装已取得可直接分发的Steam音频包。

## 维护现状与下一份最小交付

当前`StartupWorldRuntimeSession::take_sound_requests()`以swap移走整数队列，`StartupApplication`只转发世界队列；CLI领取并打印，窗口领取后丢弃，没有播放。授勋／庆典产生3，恢复经营或战斗产生1／2，部分人物操作产生4／5／6；注释称“原c(sound)输出”不足以涵盖这些BGM与jingle来源。现有表现与回放测试覆盖请求顺序、拒绝无输出和轮末消费，但不等于音频设备或完整三类入口语义已验。

资源部分现已完成：[assets/audio](../assets/audio/README.md)提供26个Ogg原字节及ID／来源／hash／通道／循环／Vorbis清单，独立于旧761项视觉发布；[发布核验器](../tools/scripts/publish_audio.py)支持只读`--check`，已有异内容明确拒绝、不覆盖，产品可选择性复制并保留来源，不读取work。

下一份维护交付仍需逐真实调用来源保留BGM替换、普通播放、jingle及停止等操作，让成功Owner提交后一次领取，失败不产出、重复领取为空。桌面暂停／失焦及循环设备策略须依据Steam继续交叉后明确；声音资源身份不要求把MediaPlayer线程或平台对象放入世界存档。新增请求结构、backend与持久边界未在本批编码或擅自定案，素材发布与实际音频消费者验收分开记录。

## 后继生产者审计：操作资格与漏接来源

[逐生产者合同](../verification/audio-producer-audit/README.md)及源窗口清单（本地核对材料，不随仓库交付）已逐一映射现25个Owner整数队列写点：7处来自BGM包装b、9处普通c、9处jingle d；这是源码写点数，不是运行次数。人物共用C出口另细分延迟效果、法术目标／命中、救援、死亡、控制33拾物、施法等来源，并保留缓存坐标／屏内门槛。原调用者而非ID决定类别：81的20是D；53/59/93/94/95/96的5是D；事件消息4/6、成果30和设施演出82的4也是D；赠礼66显式绘制的8是C。

年度87与庆典50的3均为B，重复counter1请求不能重头播放正在进行的同曲。结束顺序分别保留：50为G→关闭；87主动结束为事件22→G→关闭，勋章耗尽为G→事件22→关闭；通关计分为关闭→G→事件6→纪录事件。G在该时点按当前任务选择B1/2，不提前统一到帧末。

**发现尚未维护的遭遇开始声音／提示**：原`c/b:365`新任务遭遇成功先B2再提示24。底层已产生`music2/notice24`，直接F留在`daily.task_entry`，路径P留在`daily.path`；外层目前安装world/facts/task及保存daily审计，却未向Owner声音／通知队列消费这两个标记。提示24是S末尾`{24,-1,80}`“战斗任务开始”，不是授勋脚本事件24；通用通知消费者虽已存在，仍缺此处生产。通知以后轮到前两项更新、计数恰1才C11，不在创建时立即补音效。单次daily旧状态0的P与5/11的F互斥，成功encounter写回后其它人物只绑定，不重复创建。不能拿关闭／取消时G恢复替代或按ID统一去重；修正应覆盖两个真实创建入口、一次提交及失败无输出，本静态审计未改实现。

标题B0、世界初始化G、raw76投放及魔法壶绘制C8也未由当前维护队列接出；当前显式表现声音消费是raw66，业务已维护不代表其全部绘制声音已接。APK／Steam生命周期停止、输入反馈和Unity后端继续分证据资格；不把没有消费者表述为原版没有声音。本次仅新增来源文档／审计，无播放、构建、格式或Owner变更。

证据限制：`s.java`的扩展名选择循环存在JADX结构异常（缺清晰退出），本批没有新DEX逐指令认证；26行与raw对象／通道初始化链、文件归一化和具体Ogg工厂可分别核实，不把异常Java重写成生产算法。若要把整个通用资源加载器认证为无缺口，应先对该具名DEX方法补低层验证。
