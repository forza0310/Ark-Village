# Steam 标题背景、上边与 Logo 绘制

2026-10-09。只读固定Steam2.56研究副本，核`TitleForm.Draw`及`_draw`入口前4096字节，交叉`SmartBeginPaint`、动画常量真实字段和Logo资源选择链；不是完整`_draw`、标题菜单或原窗口复刻验收。方法和资源证据见专题包，图像身份沿[启动资源](STEAM_STARTUP_RESOURCES.md)，APK坐标独立保留于[启动皮肤合同](STARTUP_SKIN.md)。

已闭合的关键差异：Steam title00按实际Image宽高居中并贴底，upper在逻辑画面顶部按240宽重复，草边也横向重复；不能把APK240宽标题画布整体放大冒充Steam。此层仅发`DrawImage(image,x,y)`，没有给背景传入拉伸目标矩形，但底层Graphics／窗口缩放仍在边界之外。

## 坐标资格

以下`W/H`来自`GameView.GetInstance()`的`GetGameWidth/GetGameHeight`，`Wv/Hv`来自当前表单`view_`的同名虚方法，都是游戏逻辑尺寸。不是OS客户区、截图1067×910或Unity屏幕物理像素。两接收对象一般相关，本批不借字段名强制断言它们永远同一实例。

机器码通过IL2CPP虚表偏移`0x12C/0x134`调用，结合`SurfaceBase`声明的slot14／15定位；它们不同于非虚的`GetWidth/GetHeight`。图片字段`+0x14/+0x18`分别是`Image.width_/height_`，资源`+0x8`为`ResourceManager.img`。整数除2均为向零截断，下面记为`half(n)`。

`_draw`首个Fill之前没有清零原点：VA `1020E35A`设置颜色后直接在`1020E3D6`调用FillRect，**继承进入本方法时的Graphics原点**。首个SetOrigin在Fill之后的`1020E421`。`Graphics.SetOrigin`（RVA `774860`）直接写入originX／Y（`+188/+18C`）再更新，是绝对替换，不是累加或Push；不能擅自在首Fill前补一次SetOrigin0，也不能把表中历次原点相加。

## 已核层序与参数

| 顺序 | 图层／调用 | 原点与参数 |
| --- | --- | --- |
| 1 | 填背景色 | `GetColorOfRGB(41,198,255)`；`FillRect(0,0,Wv,Hv)`，继承入口原点 |
| 2 | title图片0 | 原点`(half(Wv-240),0)`；整图`DrawImage(img[0],half(240-imageWidth),H-imageHeight)` |
| 3 | title图片6 upper | 原点`(0,0)`；x从0开始每次加240，`x<W`继续；每次画`img[6]`于`(x,0)` |
| 4 | 标题人物 | 原点`(half(W-240),0)`；调用`Draw_titleAnime(g)` |
| 5 | title图片4草边 | 原点`(0,0)`；同样每240重复至`x<W`，y=`H-240+209=H-31` |
| 6 | title图片2 Logo | 无条件SetOrigin为`(half(W-240),0)`，之后才检查首动画阈值；局部x=2，y见下一节 |
| 7 | Logo后的确认注册 | 仅Logo绘制支执行`TouchOption.Create(2)`→无矩形重载`GameView.AddTouch(id2,opt)`；不由Logo图片尺寸造命中框 |
| 8 | 语言按钮及后续分支 | Logo分支后，原标题为栈顶且非存档选择状态时调用`LanguageForm.DrawButton(g,callback,20)`；再进入广告／菜单分支，未闭合全部路径 |

前景人物不会被upper覆盖：已核调用序是upper先于人物，草边后于人物；人物内部阴影／身体与淡出由[人物专题](STEAM_TITLE_ACTORS.md)独立认证，不能因APK已有合同就混写为Steam来源。

草边后`1020E63E`无条件设置中心原点，再在`1020E67D`检查`frame≥10`；因此frame小于10也留下中心原点。frame小于75的正常早退不恢复该原点；语言按钮的Push／Pop只恢复它进入时已有的中心原点。Logo后两次调用分别位于`1020E874`及`1020E898`，只证明无矩形确认注册请求，不在本段扩证该重载的最终范围。

结合已核资源：背景600×380，因此其局部坐标为`(-180,H-380)`；upper240×9、草边240×18。x步长240来自代码，尺寸来自资源，两证合并才说明两者按原图宽度铺排。最后一片仍调用整图绘制，不在本方法显式裁短；越界部分如何裁剪由Graphics／surface决定，不能宣称已核最后一片裁片矩形。

`SmartBeginPaint`独立小方法也填同一蓝色并设置`half(Wv-240)`原点；当前`Draw`包装器实际直接调用`_draw`，不调用它。不能因为两个方法同样填色就把SmartBeginPaint额外插进一次Draw，以免重复修改绘制状态。包装器还处理异常日志；这不等于恢复全部绘制状态的事务保证。

## Logo 入场与弹跳

令`frame=titleFrame_`，源静态`title_anime`三个元素`A0/A1/A2`已独立核为**10／75／100**。证据链是`.cctor`调用`InitializeArray`实参槽`0x110F42C8`→fieldRef224→私有数据type4129／local224→field25362→metadata默认值区12字节。该12字节SHA-256与私有字段名`AE845E9C…3E6A3C58C`完整匹配，原值见ANIMATION.json（本地核对材料，不随仓库交付）；与APK一致是交叉结果，不是借APK填写。

当`frame≥A0`时，入口确认以下参数：

- `T(H)=H`（H≤240）；否则`T(H)=240+half(H-240)`。
- 纵向基项`B=H-182-T(H)`。
- 入场位移`R=GameUtil.RateConvert(frame,A0,A1-1,0,181,true)`；Steam整数helper已在[信息菜单](INFORMATION_MENU.md)独立核验：先截限，区间内整数乘除向零截断，不使用浮点插值。
- `r=(frame-A2)%44`采用有符号余数；r<6时p=r，6≤r<12时p=12-r，其余p=0。仅p>0计算弹跳。
- 弹跳请求分别调用`SetVStop(-6000.0f,6)`和`SetAStop(-6000.0f,6)`，返回值先向零截断为整数，再组成`V*p + half(A*(p+1)*p)`，结果向零除1000。
- 最终局部位置为`(2, B+R+弹跳)`。

`SetVStop/SetAStop`的Steam float32算术已由[金币绘制](STEAM_COIN_RENDER.md)独立核验；代入本页实参得到`V=-2400、A=400`。这是具名机器码与实参的推导，不是窗口测得的弹跳轨迹。

[维护接口](../prototype/include/dungeon_village_prototype/steam_startup_skin.hpp)的`steam_title_backdrop_skin`按上述顺序返回Fill、SetOrigin、整图、实际人物绘制插入点及无矩形确认注册，止于语言／菜单之前；`SteamTitleActors`要求调用者消费真实人物计划，不生成演示人物，也不推进随机或演员状态。尺寸1..65536及标题钟0..INT_MAX−1是维护预算，实际Logo变体由调用者提供；本批功能短测和四个有界研究窗口已通过，见[前段示例](examples/steam-title-backdrop.png)；示例没有演员／菜单，不是原窗口动态。大标题钟必须先截限入场位移，不能先乘181：INT_MAX−1时R=181、余数10、p=2、弹跳−3。独立坐标核对：W479／H481、Wv301／Hv277、背景600×380时，背景原点为30、局部位置`(−180,101)`，人物及最终原点为119；Logo在frame10／73／74／101／106分别为局部`(2,−61)`／`(2,117)`／`(2,120)`／`(2,118)`／`(2,114)`。这些是静态公式oracle，不是窗口测量。

这段Logo调用前未见APK的英语`-8`偏移。稍后的**广告**分支另读取Logo高度、`Language.English()`并减8，不能把广告修正移到Logo本体。`_draw`本身只拿当前`img[2]`，Logo语言选择发生在下面的资源加载链。

## 右上语言按钮

正常标题栈顶且非选档状态时，在Logo后、菜单动画门槛前调用`LanguageForm.DrawButton(g,callback,20)`。本节独立顺序核对anchor重载RVA `1EB190`、坐标重载`1EACA0`、`IsLanguageSwitch` `812470`、`OnTouchEvent` `1EB400`及`DrawNewLanguage` `80D5C0`；不由窗口按钮外观推断输入事件。

两层DrawButton均先检查`Language.IsLanguageSwitch()`，其资格来自`langConfig_.GetLanguageSettings(GetLanguageCode()).Check(LanguageConfig.Flag.Switch=1)`；不能简化为非日文或Steam平台恒真。anchor位2优先水平居中、否则位4右对齐；位32优先垂直居中、否则位64下对齐；缺相应位时距左／上20。标题实参20（`RIGHT4|TOP16`）因此得到**`(W−20,20)`**。两层各自`PushOrigin(0,0,false)`并在正常尾配对恢复，不叠加标题`half(W−240)`原点。

| 项目 | 已核请求与边界 |
| --- | --- |
| 素材 | `IApplication.GetResource()`的managed `KairoLibrary.resource.kairolib.bytes`，`seb.inf`槽0=`button_language.seb`、`img.inf`槽0=`button_language.png,bin`。PNG为60×60；SEB一层一帧，image0、源矩形`(0,0,60,60)`、偏移`(−30,−30)`、末两字段0。身份沿[图像索引](../assets/IMAGE_COVERAGE.json)，不混用Unity同名载荷。 |
| 常态绘制 | `DrawSeb(g,x,y,0)`外套临时0.5矩阵，视觉为锚点中心30×30。 |
| 触摸高亮 | `CheckTouch(component3,value0x133A066)`为真时，外加`PushBlendMode(1,255,255,255,128)`与1.1矩阵，再套0.5矩阵；按相反顺序恢复。高亮不扩大触区。 |
| 触区 | `GetSprite(frame0,line0)`取尺寸／偏移，各向零除2；`AddTouch`注册component3、value`0x133A066`，基矩形`(x−15,y−15,30,30)`；标题即`(W−35,5,30,30)`。无显式TouchOption，使用`TOUCHPARAMS[3]`四边margin **左16／右16／上8／下8**，不是平台按钮显式15。屏幕裁切与组件碰撞调整仍交共同触区注册器。 |
| 手柄提示请求 | `SetJoystickButtonDraw(button9, X,Y, registerID2, scale100)`；`X=trunc(float32((x+14)×ratio/100))`、`Y=trunc(float32((y−14)×ratio/100))`，ratio来自`GetScaleRatio(false)`。不是平台控制器按钮的button8／register24。 |
| 输入 | `OnTouchEvent`匹配同一component／value后，仅`event.type==1（UP）`构造`LanguageForm`并`FormManager.Push`；匹配的其它事件也返回false，阻止通用分发。不匹配才返回true。不是CLICK5，也不在此直接切换语言或调用callback。 |

该Draw并非完全只读：外层先资格检查、PushOrigin及算锚，内层再检查资格，随后才写`LanguageForm.callback_`（VA `101EAD4F`），再执行第二层PushOrigin。图像矩阵恢复后另调用`Language.DrawNewLanguage(g,x,y,−1,8)`；只有`newState_==2`时绘制，先将`newFrame_=(newFrame_+1)%16`写回，再发`Kairolib.DrawNewText(g,float(x)−0.5,float(y−trunc(newFrame_/8)+16),frame8,scale0.75)`。NEW标记不随按钮0.5／1.1矩阵缩放，且每次合格Draw都会推进其静态帧，不能借标题逻辑钟替代。正常尾序为**NEW→注册触区→注册手柄提示→内层PopOrigin→返回后外层PopOrigin**；不能将NEW留在按钮高亮混合或缩放栈内。

`DrawNewText`（RVA `8055B0`）已补核：它不调用字体或翻译，取managed kairolib的SEB21 `new.seb`，以`frame%16、layer−1`显式调用`Seb.Draw`；scale不为1时在此前Push `ScaleTemporary(scale,scale,0,0)`，之后Pop。`new.png`为image24、28×22；SEB一层、16帧、tag16，关键帧0／7为源`(0,0,28,11)`，8／15为`(0,11,28,11)`，偏移均`(−14,−5)`、末两字段0。标题固定传8，因此使用第二行裁片；`newFrame_`只影响本调用的纵向摆动，不轮换NEW的图像帧。0.75是图像局部矩阵缩放，不是字体字号；锚／偏移的合成沿[平台按钮共同矩阵合同](STEAM_INTERACTIONS.md)，不由截图估算像素。显式帧调用不执行当前帧Draw的自动登记，见[表现请求](INPUT_RENDER_REQUESTS.md)。

触区默认值来源独立核至`GameView..cctor`：VA `10240317`以使用槽`110F5D00`初始化六整数，`1024034C`写入`TOUCHPARAMS[3]`；使用槽经FieldRef到私有字段25442的默认值为`(16,16,8,8,1,1)`。`_addTouch` RVA `23E010`在无opt覆盖时取该行，后两值分别控制碰撞和屏幕约束。这里保留基矩形及margin请求，不把扩大后的矩形冒充经过其它组件避让后的最终命中范围。

维护交付：[平台按钮模块](../prototype/include/dungeon_village_prototype/steam_platform_button_skin.hpp)的`steam_language_button_skin`接显式Switch资格、原anchor、窗口逻辑尺寸、比例、触摸与NEW状态，返回原序steps及图元／注册参数；callback替换、NEW下一帧是显式请求，不在查询中写静态状态。`body`各部分必须按steps分别消费，绘按钮时不能提前注册触摸／手柄；两层原点由steps管理，body.reset_origin=false，不能额外叠第三层。`steam_language_button_touch`仅分类传播或UP入页意图，不实际切语言。按钮与NEW四原文件已出版，复用现资源身份；尺寸1..65536、比例(0,10000]及NEW帧0..15是维护预算，合法隐藏不消费其它字段。尚未维护语言页内部选择、设置保存、callback最终消费或完整平台恢复；实际会话资格仍由调用方提供。尾部开发警告提示支路不计入普通按钮合同。

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

`_draw`登记范围11008字节，本专题初批只从固定方法入口顺序解码前4096，尾部6912字节当时未处理；末端截断指令不作事实依据。后继[标题菜单与选档合同](STEAM_TITLE_MENU.md)已顺序补读至方法尾部，闭合正常菜单底图／光标／矩形文字、选档小窗／两行图标／可用性与确认路径，以及正常尾部不恢复原点、debug块局部保存恢复、末尾共享stub仅RET 0的边界。初批范围保留，不再把整个尾部列为未知。

后继[raw20菜单](STEAM_SAVE_MENU.md)及[共用窗框](STEAM_WINDOW_FRAME.md)已有局部合同和维护皮肤，平台设置／键盘具名helper及纯计划见[交互合同](STEAM_INTERACTIONS.md)。这些交付仍不代表广告／debug全部资格、完整平台组合或最终Graphics尺度已闭合。

本批背景／Logo计划、Steam演员投影及局部菜单／选档已组合为[六状态研究窗口参考](examples/steam-title-menu-slots.png)，六入口各8帧有界检查通过。两个中央普通演员、空目录／两行非空均为显示条件；Noto／中文、静态手形帧0和VIEW_Y23属于维护适配。它可展示已核图层／原点与小窗组合，不证明原Steam动态、物理输入、实际语言资源选择或完整平台标题。

后续优先核实际语言包安装／切换、真实字体与窗口尺度／裁剪、完整平台组合。[人物专题](STEAM_TITLE_ACTORS.md)已核出生／退休、边缘set/reset与中央继承模式、正常基础图层，以及共同随机的三种后端算法；未知的是当前实际随机模式、初始化／流交接、深层效果消费与完整框架准入，不再把底层PRNG整体列为未解。窗口智能体可补不同尺寸下的背景居中／铺条与人物淡入淡出，但截图不能直接证明逻辑W/H、阈值、资源槽或随机抽数；继续使用既有外部任务，不在本会话操作用户原游戏。
