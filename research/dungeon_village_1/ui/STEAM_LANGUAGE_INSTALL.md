# Steam语言包安装、选择与资源目录

2026-10-09。固定Steam2.56研究副本的静态增量，核8个具名方法、14窗口、32个固定字符串槽和4个TextAsset，[身份入口](../EVIDENCE.md)。未运行游戏、读用户偏好／账号／存档、安装语言包或导出整份译文。与[字体消费者](STEAM_FONT_CONSUMER.md)、[标题资源选择](STEAM_TITLE_DRAW.md)、[手动档翻译](STEAM_SAVE_MENU.md)共同使用，APK汉化仍独立。

已闭合到**选定某个包后语言码与资源候选的来源**，尚未证明用户窗口当前选中包、实际中文字形对象或所有翻译回调注册者。资源存在、安装方法可达与当前运行选择是三个层级。

## 固定资源中的语言包

`resources.assets`的TextAsset `language`（pathID1127）经已核XOR／归档读取器解得12条CSV，没有该归档内部的`config.inf`。只登记哈希、字节数及六项声明头，未发布完整翻译表。

| 条目 | `@language` | `@title` | `@scale` |
| --- | --- | --- | --- |
| DungeonVillage_de.csv | de | Deutsch | auto |
| DungeonVillage_es.csv | es | Español | auto |
| DungeonVillage_fr.csv | fr | Français | auto |
| DungeonVillage_hi.csv | hi | हिंदी | auto |
| DungeonVillage_it.csv | it | Italiano | auto |
| DungeonVillage_ko.csv | ko | 한국어 | auto |
| DungeonVillage_pt.csv | pt | Português | auto |
| DungeonVillage_ru.csv | ru | Русский | auto |
| DungeonVillage_SC.csv | zh-CN | 簡体字 | auto |
| DungeonVillage_TC.csv | zh | 繁体字 | auto |
| DungeonVillage_TH.csv | th | ไทย | auto |
| DungeonVillage_tr.csv | tr | Türkçe | auto |

繁体包声明的是`zh`，不能凭文件名改写成`zh-TW`。TH采用双引号包围的CSV头，列表表述是去引号后的含义；机器证据保留原行。`@version`中的1.00／2.02是包内声明，不能当作当前EXE版本。没有日／英CSV不代表缺少日／英文；它们还有特殊选择和内置／模板路径。

另外独立核了原始TextAsset `language_pack_template_en`、`language_pack_template_ja`及`language_pack_format`。后二类模板文本无需套用`language`归档的XOR。format共23行，列分隔与记录分隔声明不同，如`evtmsgs,124,38`、`text,10,9`；本批只登记输入身份，不将这些声明直接认证为完整翻译匹配算法。

## 安装调用的真实职责

`Language.SetLanguagePack(file,data,save=true)`（RVA `0x81E280`）不仅返回一个语言码。它先处理基础语言优先级与特殊日／英选择，再加载和解析，成功清翻译缓存；选择和持久偏好是不同阶段。

普通包的输入分流：

1. `file`非空时，用固定正则`^[a-zA-Z0-9_\-\(\)]+\.csv(,[a-zA-Z0-9_\-\(\)]+\.csv)?$`区分内置包名和自定义位置。正则命中才属于内置命名支路，不把任意文件路径拼进资源目录。
2. 有显式`data`时用该字节；无数据且属于自定义位置时，检查／读取Storage media1的`_custom_lang_pack.csv`。
3. 无数据且属于内置命名支路时，Storage media3读取`language.dat`，构造JarInflater。文件名含两个逗号分段时分别取第1与第2条目，交给`LoadLanguagePack(data,partialData)`；普通单文件仅传第一份。
4. 内置安装之后把author写为固定`Kairosoft`，并可依据语言码的标题查找结果覆盖包标题；因此包头标题和最终菜单标题也要分开。
5. `save=true`时，自定义支路写自定义位置偏好并保存CSV；普通和自定义都会写当前包偏好并调用PlayerPrefs.Save。最后更新`langPackFile_`，清缓存，返回true。

特殊日／英选择不走CSV字节分支；设置相应`ja-JP`／`en-US`、标题／作者与scale，再以空字节调用`LoadLanguagePack`清理／重建，最后清缓存。特殊选择字符串字段的完整初始化值未在本批解出，不能只凭名字把传入字面值写死。

`save=false`**不是完全无副作用／只读模式**：若已有`initialized_`且`file`非null，前段仍会设置`_language_pack_userchanged=1`并Save；解析前也已经调整基础语言优先级，部分路径会清文字／软键表或加载翻译表。这一点仅为原静态事实，本批没有调用原方法修改设置。

失败也不能概括成全局事务。`LoadLanguagePack`入口保存并在异常续体恢复records、folders、author、code、title、scale六项；但`SetLanguagePack`外围的优先级／偏好／缓存写入不因此自动回滚。原方法有false返回和关闭JarInflater路径；不能拿它代替维护Owner的失败无部分提交保证。

## 语言码与资源目录产生者

`LoadLanguagePack`（RVA `0x8129F0`）识别`@author/@language/@title/@scale/@appli`。本批核与资源选择直接相关的赋值：

- `@language`先用`[^a-zA-Z\-]`移除其它字符；不是任意locale字符串原样保存。
- 以`-`拆分后**恰为两段**才把首段转小写、第二段转大写，再用`-`拼回。不是三段语言码也只取前两段；这一点与字体资源路径查找的“至少两段取前二段”不同。
- 标题为空时回退语言码。
- `@scale,auto`置-1进入后续自动测宽路径；不能把所有auto直接写成100。本批已见Font.StringWidth采样、结果归一和下限分支，完整样本选择与最后请求helper仍未闭合。

随后重建`langPackFolders_`，共享泛型helper已用实际MethodInfo槽独立核为`.ctor/Add/ToArray`，不借同址别名猜操作。顺序是：

| 当前规范码 | 原序加入的资源目录值 |
| --- | --- |
| 普通两段，如zh-CN | zh-CN，zh |
| zh-SG | zh-SG，zh-CN，zh |
| zh-HK | zh-HK，zh-TW，zh |
| zh-MO | zh-MO，zh-TW，zh |
| 单段，如zh | zh |

首个值总是完整码；特定地区追加别名；有至少两段再追加第一段。本层不会无条件加入`English.lproj`；后者来自AssetReader随后追加的基础语言文件夹，顺序沿[标题候选合同](STEAM_TITLE_DRAW.md)。不在此处把目录值加成`.lproj`或替换成系统locale。

`GetLanguagePackFolders()`（`0x80E0F0`）直接返回该字段。`GetLanguageCode()`（`0x80DFB0`）优先返回非空`langPackCode_`；否则`Get()==0`返回`ja-JP`，其它值返回`en-US`。因此文本码、基础0／1语言和资源搜索目录不是同一个字段。

对固定SC／TC包，**在该包成功装入且没有后续切换的条件下**可推得码分别为`zh-CN`／`zh`，目录前缀分别为`[zh-CN,zh]`／`[zh]`。由字体专题中缺失的中文／通用default字体目录仍不能推出最终中文Font；还要核Unity空font回退及插件。标题Logo同理：若基础语言目录此时为English.lproj，则它可在上述中文目录都未命中后选得英语Logo；不能因画面有中文就把根Logo标成唯一中文Logo。

## 启动选择与翻译注册的边界

`Language.Init`（`0x80F820`）有9952字节登记范围。本批核到它重置静态状态、设置偏好键、建立语言目录／设置集合、读取默认locale和保存的包偏好、执行匹配分支，然后将偏好值传`SetLanguagePack(file,null,false)`。失败后有`SetLanguagePack(null,null,true)`回退。它还注册IApplication.Updated事件并维护语言变化检查。

这不等于“默认一定简体”或“Steam语言与Windowslocale完全等价”：系统locale的实际来源、所有Config条件、目录筛选／匹配优先级还未逐项闭合；语言页的普通结果消费后继见下节，本批未读用户偏好。原静态初始化大方法`Language.cctor`未盲扫。

前批已核`LoadTranslateTable`会优先调用`translateTableMethod_`，没有回调才走默认表读取。本批核`RegistTranslateTable(ThreadStart)`（`0x81DC40`）仅写该字段；`InitStaticFields`先将它清空。对固定DLL可执行节只搜索指向这一地址的直接E8／E9，结果为0；`system.KairoText.cctor`也没有注册调用，仅初始化自己的静态数组。

**注册者尚未找到，不是证明不存在回调。** 直接调用搜索不覆盖内联写字段、反射与间接委托。下一步应从实际启动宿主／翻译表创建者追有限写入与委托绑定，不能把这次零候选写成“所有游戏只读xls.dat”。

## 语言页的选择、结果与标题回调

固定DLL的`LanguageForm.Init/Update/OnTouchEvent`（RVA `1EB390/1EB9B0/1EB400`）已从具名入口顺序核对。
TypeInfo槽`110FB090/110FB03C/110F404C`独立对应LanguageForm／Language／Config，
标题委托槽`1110041C`解为TitleForm的`<_draw>b__51_0`（MethodDef33478），不按同址别名猜接收者。
以下是原静态调用顺序，本批没有实际切换语言、读偏好或执行外链／下载。

| 局部步骤 | 已核消费者与边界 |
| --- | --- |
| 入页 | 已有[原标题按钮合同](STEAM_TITLE_DRAW.md#右上语言按钮)的匹配UP事件创建表单908并Push；Draw保存的是共享静态callback，本次UP不调用它 |
| 初始化 | 先MyFormBase.Init；mode0且`Language.IsLanguageCreate()`假时改mode2。该资格方法`812420`读Config.DEBUG，不是从“有CSV”推导编辑功能开放 |
| 首次Update | 先MyFormBase.Update，将`start_ +0x88`置true；从Language静态`systemLangPackFiles_ +0x68`复制有序行至`languages_ +0x78`，各行[0]交平台作标题、[1]作安装文件。按文件与当前`langPackFile_ +0x64`相等选择索引，初值0，多个相同匹配保留最后一项 |
| 自定义候选 | 若GetCustomLanguagePackLocation非null且media1的`_custom_lang_pack.csv`存在，读取其包头；以标题及`@作者`构造额外行，文件仍取该自定义位置。此处只列候选，不安装，不把归档12条直接当作全部当前菜单 |
| 平台面板 | 调`KairoPlugin.StartLanguagePanel(titles,index,customLocation,mode)`（`87BF60`）；后续Update等IsLanguagePanelFinish（`878C10`），完成才取GetLanguagePanelResult（`8754E0`）。传入菜单索引与内部定义ID无关 |
| 非负选择 | result[0]解析为int索引，再取冻结行[1]；与GetLanguagePackFile相同就请求Pop。不同才`SetLanguagePack(file,null,true)`；成功后调用非null共享callback，随后请求Pop。false返回则不调用callback、仍请求Pop，不能补成原子失败回滚 |
| 空结果 | result[0]为null直接请求Pop，没有本分支安装或callback；这只认证空结果消费者，未证明所有桌面取消／关窗输入都生成相同结果，也不把空字符串、负数或格式错误合并为取消 |

普通成功的顺序是**语言安装及原偏好保存／清缓存→callback→Pop请求**，不是先退页后异步保存。
维护[平台接口](../prototype/include/dungeon_village_prototype/steam_platform_button_skin.hpp)现提供
`steam_language_panel_result`：接已解析的平台结果与冻结行文件、已取得当前文件，输出最多三项有序请求。
null结果和相同包仅Pop；不同包为`SetLanguagePack(file,null,true)`请求→安装true时调用当前共享callback→Pop。
消费者须按序执行，安装false跳过callback但仍Pop，安装或callback异常停止余项；该纯计划不实现原偏好写入，
也不承诺原安装事务或Boot已经成功重建。callback必须在执行该步时重新读取，不能冻结成入页私有委托。
冻结文件及当前文件各保留null，按文件串精确比较，不按标题／locale／存在性替换。
平台result[0]的Int32.Parse仍由适配层负责，尚未取得结果的默认unresolved显式拒绝，空串／坏格式不能冒充null；本接口拒绝负结果和越界，
因此HTTP／模板／外链不由普通结果接口隐式启用。4096行及实际消费字符串4096字节为维护预算；
未消费目录不补校验。现visuals短测覆盖分支、原序、null文件、大小写、拒绝与重复只读，没有新增Owner或持久字段。
标题实际传入的callback（RVA `20BFD0`）在`1020BFE3`调用`AppData.StopBgm(0)`，
随后`1020C002`调用`FormManagerBase.ChangeCurrentForm(apdat_.frmBoot_,false)`。
接收者是AppData**实例**字段`+0x08`的BootForm；TitleForm在`+0x0C`、GameForm在`+0x10`，
不能把此处的`+0x08`误认成标题页，或与AppData静态区同偏移的UserData混读。
callback本身不直接加载资源，其正常Boot→Title后继见下节；ChangeCurrentForm请求不等于重建已成功。
callback抛异常时本页随后的线性Pop不执行；
已发生的语言／偏好副作用不会因这一局部自动撤销。本页消费的是Draw时共享callback，不是冻结到每个表单的私有委托。

负结果各有独立职责：−1把result[1]存共享下载位置、清本页下载字节、Push带两个具名回调的HttpForm，
再请求Pop；−2按result[1]是否`ja`选模板生成方向并Push另一HttpForm链；−3调用
`Canvas.OpenBrowser(http://kairopark.jp/language_pack_en.html)`后请求Pop。不能用普通选择合同隐含支持这些下载／上传／外链动作。
下载完成回调`<Update>b__13_3 RVA1EB850`只有ResultData的complete为真才以共享下载位置及本页下载字节调用
`SetLanguagePack(...,true)`，成功后再消费共享callback；未完成则创建mode1语言页及错误提示。
完整HTTP失败、平台取消产生者和模板上传副作用未由此局部穷尽，不主动执行它们。

菜单目录上游已缩小到`Language.Init RVA80F820`：字段非null时跳过重建；否则读取media3的`language.dat`，
可读取归档内`config.inf`，枚举归档文件并取包头，结合LanguageConfig设置构造／排序／筛选，
最后在`10810FF8`发布`systemLangPackFiles_`。本固定归档无config.inf是资源事实；
不能因此跳过源代码的条件路径，或把存在12包认证成每个实际运行都显示同序12项。

### 正常标题切语言后的Boot→Title重建

`AppData`的TypeInfo槽`110F1FFC`独立解为TypeDef4061；以上实例字段又与具名callback的
`[this+0x4C]→[app+0x08]`调用链交叉。原程序复用AppData保存的Boot／Title**表单对象**，
callback没有new这两个表单；表单复用不等于复用其资源管理器或显示scratch。

`ChangeCurrentForm RVA7E4A50`先查目标是否在栈、移除旧目标，再把剩余页`formProcMode_`置FINISH4。
目标未在栈时走`Push(null,target)`（`107E4E19`）；Push委托`7F2580`在`107F2778`明确置INIT0。
已在栈且不是原栈顶则只重新追加并RefreshSoftLabel，不自动重Init；原栈顶还会走Push支。
因此“ChangeCurrentForm总重Init”不成立。下面针对正常初始启动已完成、Boot已退休离栈、
Title为当前父页的普通语言选择，保留这一前提，不套调试／嵌套Boot状态。

callback返回后的`LanguageForm.Pop`是对本页对象的请求；委托`7F29E0`把它置4，
从全栈末端找首个非4页，已追加的Boot保持INIT0，不会因本次Pop改成退休页。
框架`_executeInitAndFinish RVA7E9360`先处理FINISH4，成功且仍为4才移出引用，随后处理INIT0。
Title.Finish（`20AD50`）在`1020AD9D`Dispose旧`resTitle_ +0x78`，并在`1020ADA8`清引用；
不是用普通纪录返回的PREUPDATE路径恢复旧标题资源。

Boot.Init（`1FB500`）先MyFormBase.Init，再LoadSystem，false才InitApp，随后Init2；
Init2（`1FADD0`）建立新的加载画面ResourceManager、置boot state0、清错误／注册字节并新建JThread启动Run。
Run（`1FE4E0`）正常进入state1后在`101FE8FC`执行Load（`1FBE90`）；
Load读取资源组1的记录10并重建定义目录，且依原序Dispose／重建公共资源管理器，例如入口在
`101FC188`Dispose原resCom。这个加载链含系统写入及后台任务，不是纯菜单翻译或全局事务。
正常加载／等待完成后`101FEC49`置state4；Boot.Update（`1FF530`）在无需版本提示的普通支
于`101FF74F`调用`ChangeCurrentForm(app.frmTitle_,false)`，这次读的是实例`+0x0C`。

旧Title已成功Finish并离栈时，这次Push重新置其INIT0，随后真正执行Title.Init（`20ADF0`）：
`1020AF21`绑定新ResourceManager，`1020AF4C`读取资源组1／记录11，`1020AF62`调用Load(bytes,3)。
所以标题图片重新选择发生在**Boot重载完成后重新初始化Title的资源加载**，不是首次callback或Pop时；
其语言目录／Logo选择沿既有[资源加载合同](STEAM_TITLE_DRAW.md#logo资源选择链与中文资格)，
条件仍是先前SetLanguagePack成功且期间没有其它语言变更。Title.Init随后调用UserData.Setting，
重建显示scratch的边界见[演员合同](STEAM_TITLE_ACTORS.md#scratch副作用与交付边界)，不重新创建AppData或表单对象。

Language.ClearCache（`80CA00`）只在锁内清翻译缓存列表；其Updated回调RemoveExpiredTranslateCache（`81DCA0`）
同样是缓存寿命消费者，不充作ResourceManager重建回调。原翻译表／语言目录已由SetLanguagePack变更，
实际逐字文本及字体重选仍须由各消费者认证。下面仅补Boot加载失败与局部线程交接，不穷尽注册／版本提示、
平台线程实现、Finish失败或目标仍在栈的分支；已发生的偏好、旧页退休和资源替换不保证回滚。

#### 加载异常、错误保留与线程交接

Run的实际异常表沿入口SEH指向的FuncInfo `11084C10`核对：try state6覆盖
`101FE8F8–101FE91A`的Load及随后SaveSystem，接异常处理器`101FEA8A`，不是从反汇编邻近位置猜捕获范围。
其System.Exception检查槽`110F5E20`独立解为该类型；匹配后经`101FE820–101FE87D`把原异常存
`e_ +0xA4`，将`error_ +0x9C`设为槽`110F9ADC`的精确字面`読み込みに失敗しました。`，
再调用ChangeState(3)，清`th_ +0xA0`并返回。这里没有继续执行正常尾部的ChangeState(4)，
也没有撤回Load此前的资源替换或系统写入。ChangeState只按状态装软标签、写state_并清frame_wait，
不是重新加载或失败事务。

Update在`101FF606–101FF609`把state3分到错误UI：按当前GameForm是否存在使用平台Dialog或SubForm，
后继RemoveAllForms；特定按键还能把保留的e_交SpForm。该错误分支没有切Title。
只有state4进入已有的Title／版本提示分支；不能将后台线程返回或th_为空当成成功资格。
Init2的`101FAFA2–101FAFC5`仅清error_与regist_，整方法未清e_；本批所核正常Run尾及Update同样未清它。
因此复用Boot时旧异常对象可能仍被持有，直到新异常覆盖；不把清错误文字写成完整释放失败结果。

其它异常不能并入这一结论：注册请求自己的捕获改用GetHttpErrorText；注册返回解析的捕获
`101FEB8C`只把bowWait置0，不写加载错误，仍可进入state4，而Update会再次解析该返回字节。
外层state1–11的异常续体`101FEC6E–101FECB0`清th_后重抛，未保证已经安装error_或state3。
这批不把上述路径认证为平台网络容错或所有异常均安全停页。

Init2实际构造JThread并在`101FB00A`调用Start；JThread的JRunnable构造
`849FB0`绑定委托、创建System.Threading.Thread，Start `849E90`将非空内部线程交Thread.Start。
Run正常尾先在`101FEC49`发布state4，随后`101FEC5D`清th_；Update只读state_，不等th_清空，
也未调用Join／IsAlive。Run的短等待是bowWait=5后循环JThread.Sleep(50)，每轮减计数，确认pulse可提早清零；
Update普通尾另有Sleep(50)。JThread.Sleep `849E20`转Thread.Sleep(int)，随后检查共享abort_并可请求当前线程Abort，
不是等待加载线程完成的Join。Boot这两个方法和ChangeState中的状态／引用交接是普通字段读写，
未见局部Monitor、Interlocked或显式屏障；引用写屏障不等于已核线程完成协议。
原生调度、内存可见性及ThreadAbort运行时语义仍在SDK边界，不照搬成无同步C++共享状态，
也不由清th_宣称底层线程已回收或全部加载副作用已原子发布。

## 交付与后继

产品可消费以上语言输入区分、条件资源目录顺序、中文包身份及失败／保存副作用边界。当前不提供完整多语言播放器、字体替代、已观测当前中文文字全集或可操作语言页；也没有改变APK业务、C++Owner、应用快照格式或发布素材。

后继最小研究是：翻译注册者、LanguageConfig／locale选择的实际输入、平台线程可见性与回收、平台取消结果产生者、最终Graphics文字基线与Unity／插件字形回退。上述正常Boot→Title资源重建及局部加载失败分支不再列为整体未知；外部窗口可记录当前语言菜单和同一标题的文字／Logo，但不得为验证本合同自行覆盖用户语言偏好。

## 静态来源范围

注册者搜索仅限固定RegistTranslateTable的E8／E9候选，并从具名调用者入口核边界；零直接候选不排除内联／间接注册。未扫描Language完整静态初始化，尚未闭合的回调与当前实际中文字体／选择不能由资源清单补猜。
