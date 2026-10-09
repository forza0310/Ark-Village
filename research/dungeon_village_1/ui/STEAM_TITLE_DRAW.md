# Steam 标题背景、上边与 Logo 绘制

2026-10-09。只读固定Steam2.56研究副本，核`TitleForm.Draw`及`_draw`入口前4096字节，交叉`SmartBeginPaint`、动画常量真实字段和Logo资源选择链；不是完整`_draw`、标题菜单或原窗口复刻验收。方法和资源证据见[专题包](../work/steam-title-draw/README.md)，图像身份沿[启动资源](STEAM_STARTUP_RESOURCES.md)，APK坐标独立保留于[启动皮肤合同](STARTUP_SKIN.md)。

已闭合的关键差异：Steam title00按实际Image宽高居中并贴底，upper在逻辑画面顶部按240宽重复，草边也横向重复；不能把APK240宽标题画布整体放大冒充Steam。此层仅发`DrawImage(image,x,y)`，没有给背景传入拉伸目标矩形，但底层Graphics／窗口缩放仍在边界之外。

## 坐标资格

以下`W/H`来自`GameView.GetInstance()`的`GetGameWidth/GetGameHeight`，`Wv/Hv`来自当前表单`view_`的同名虚方法，都是游戏逻辑尺寸。不是OS客户区、截图1067×910或Unity屏幕物理像素。两接收对象一般相关，本批不借字段名强制断言它们永远同一实例。

机器码通过IL2CPP虚表偏移`0x12C/0x134`调用，结合`SurfaceBase`声明的slot14／15定位；它们不同于非虚的`GetWidth/GetHeight`。图片字段`+0x14/+0x18`分别是`Image.width_/height_`，资源`+0x8`为`ResourceManager.img`。整数除2均为向零截断，下面记为`half(n)`。

## 已核层序与参数

| 顺序 | 图层／调用 | 原点与参数 |
| --- | --- | --- |
| 1 | 填背景色 | `GetColorOfRGB(41,198,255)`；`FillRect(0,0,Wv,Hv)` |
| 2 | title图片0 | 原点`(half(Wv-240),0)`；整图`DrawImage(img[0],half(240-imageWidth),H-imageHeight)` |
| 3 | title图片6 upper | 原点`(0,0)`；x从0开始每次加240，`x<W`继续；每次画`img[6]`于`(x,0)` |
| 4 | 标题人物 | 原点`(half(W-240),0)`；调用`Draw_titleAnime(g)` |
| 5 | title图片4草边 | 原点`(0,0)`；同样每240重复至`x<W`，y=`H-240+209=H-31` |
| 6 | title图片2 Logo | 恢复原点`(half(W-240),0)`；到首动画阈值才画；局部x=2，y见下一节 |
| 7 | 语言按钮及后续分支 | Logo后，原标题为栈顶且非存档选择状态时调用`LanguageForm.DrawButton(g,callback,20)`；再进入广告／菜单分支，未闭合全部路径 |

前景人物不会被upper覆盖：本轮已核调用序是upper先于人物，草边后于人物；人物内部阴影／身体与淡出尚须`Draw_titleAnime`单独认证，不能因APK已有合同就写成Steam已证。

结合已核资源：背景600×380，因此其局部坐标为`(-180,H-380)`；upper240×9、草边240×18。x步长240来自代码，尺寸来自资源，两证合并才说明两者按原图宽度铺排。最后一片仍调用整图绘制，不在本方法显式裁短；越界部分如何裁剪由Graphics／surface决定，不能宣称已核最后一片裁片矩形。

`SmartBeginPaint`独立小方法也填同一蓝色并设置`half(Wv-240)`原点；当前`Draw`包装器实际直接调用`_draw`，不调用它。不能因为两个方法同样填色就把SmartBeginPaint额外插进一次Draw，以免重复修改绘制状态。包装器还处理异常日志；这不等于恢复全部绘制状态的事务保证。

## Logo 入场与弹跳

令`frame=titleFrame_`，源静态`title_anime`三个元素`A0/A1/A2`已独立核为**10／75／100**。证据链是`.cctor`调用`InitializeArray`实参槽`0x110F42C8`→fieldRef224→私有数据type4129／local224→field25362→metadata默认值区12字节。该12字节SHA-256与私有字段名`AE845E9C…3E6A3C58C`完整匹配，原值见[ANIMATION.json](../work/steam-title-draw/ANIMATION.json)；与APK一致是交叉结果，不是借APK填写。

当`frame≥A0`时，入口确认以下参数：

- `T(H)=H`（H≤240）；否则`T(H)=240+half(H-240)`。
- 纵向基项`B=H-182-T(H)`。
- 入场位移`R=GameUtil.RateConvert(frame,A0,A1-1,0,181,true)`。这里只核调用实参，不以APK同名helper替代Steam内部算术认证。
- `r=(frame-A2)%44`采用有符号余数；r<6时p=r，6≤r<12时p=12-r，其余p=0。仅p>0计算弹跳。
- 弹跳请求分别调用`SetVStop(-6000.0f,6)`和`SetAStop(-6000.0f,6)`，返回值先向零截断为整数，再组成`V*p + half(A*(p+1)*p)`，结果向零除1000。
- 最终局部位置为`(2, B+R+弹跳)`。

这段Logo调用前未见APK的英语`-8`偏移。稍后的**广告**分支另读取Logo高度、`Language.English()`并减8，不能把广告修正移到Logo本体。`_draw`本身只拿当前`img[2]`，Logo语言选择发生在下面的资源加载链。

## Logo资源选择链与中文资格

已从具名入口串起：`TitleForm.Init`读取`RecordStore.ReadRecord(1,11)`→新`ResourceManager.Load(bytes,3)`→`JarInflater`→`LoadReady/LoadStart`→UI线程图像任务→`JarInflater.GetData`→`_search`→`AssetReader.GetAccessFilesList`。新资源管理器构造明确`rsId_=rcId_=-1`，`_init`置`loaded_=false`；这里的1／11是标题资源入口，不是读取玩家存档操作，本批没有执行这些原方法。

标题`img.inf`第2槽仍写`title_logo.gif`，无逗号修饰参数。实际加载器先处理目录／文件名修饰，再由扩展转换将Steam平台的`.gif`换为`.png`。图像任务将选得字节送入图集候选／普通Image加载支路；`_search`把候选转小写，按候选序寻找归档条目并返回**第一个命中**，不会随机选Logo。

文件候选由两层列表组合：

1. 语言文件夹列表：`langPackFolders_`存在且`UseResourceLanguage`允许时先按原序加入这些目录，之后追加`Language.GetFolder()+"/"`，最后空前缀。
2. DPI文件夹：从当前`dpi_`到`DPI_FOLDERS`末尾正向组合，再从`dpi_-1`回到0逆向组合，最后组合不带DPI目录的版本。每个DPI层内部保持上述语言目录优先序。

对无目录的`title_logo.png`，候选形如`<DPI>/<语言>/title_logo.png`，最后才是`<语言>/title_logo.png`及根目录条目。本归档Logo条目只有根目录236×115和`English.lproj/title_logo.png`236×132；没有DPI子目录或中文名Logo候选。因而当语言候选实际包含`English.lproj/`时，它会先于根目录命中英语变体；其它不存在的语言目录会继续回退，而不是因为UI显示中文就必然选根目录。

`GetFolder()`实际索引`FOLDERS[Language.Get()]`。`Get()`优先显式`priorityLanguage_`，其次已缓存基础语言，再按Config.LANGUAGE／翻译开关／系统locale建立基础语言；它不等于`GetLanguageCode()`的多语言文本标签。`langPackFolders_`又可排在基础目录之前。因此本批**已闭合候选顺序和首命中规则，尚未认证用户中文会话的实际目录列表**。`Language.cctor`登记77600字节，其完整FOLDERS／语言包初始化未解；未为凑中文结论盲扫该大方法或把metadata字段名当实值。当前原窗口Logo的具体变体仍须最小运行证据或进一步具名语言设置消费者交叉。

`JarInflater.GetData`还保留外部目录／custom resource优先路径；图集也可能影响最终Image实例。上述两个Logo的哈希与尺寸是固定研究包的归档事实，不能覆盖用户另行安装的语言包或运行时定制资源。本批未读用户配置、原档或内存。

## 未闭合项与后续最小工作

`_draw`登记范围11008字节，本批只从固定方法入口顺序解码前4096，尾部6912字节未处理；末端截断指令不作事实依据。后面的语言相关菜单、标题窗口图／光标、存档小窗、返回时原点复原和所有广告资格均未完整认证。现阶段可交付上述背景／upper／人物调用／草边／Logo层序，不能交付整张可操作Steam标题皮肤。

后续优先核实际语言包文件夹的安装／切换消费者、`Draw_titleAnime`人物淡出与最终Graphics尺度／裁剪。窗口智能体可采不同窗口尺寸下的背景居中、顶部重复条与草边，但截图不能直接证明逻辑W/H、数组阈值或资源槽号；使用既有外部窗口任务，不在本会话运行游戏或修改用户会话。
