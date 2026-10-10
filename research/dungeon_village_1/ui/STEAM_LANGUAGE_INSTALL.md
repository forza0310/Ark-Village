# Steam语言包安装、选择与资源目录

2026-10-09。固定Steam2.56研究副本的只读静态增量。核8个具名方法、14个有界窗口、32个固定字符串槽及4个TextAsset，证据与复算见[专题包](../verification/steam-language-install-contract/README.md)。没有运行原游戏、读取用户偏好／账号／存档、安装语言包或导出整份译文。与[字体消费者](STEAM_FONT_CONSUMER.md)、[标题资源选择](STEAM_TITLE_DRAW.md)及[手动档翻译入口](STEAM_SAVE_MENU.md)共同使用；APK汉化文字仍是独立来源。

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

这不等于“默认一定简体”或“Steam语言与Windowslocale完全等价”：系统locale的实际来源、所有Config条件、目录筛选／匹配优先级和语言页选择输入还未逐项闭合；本批未读用户偏好。原静态初始化大方法`Language.cctor`未盲扫。

前批已核`LoadTranslateTable`会优先调用`translateTableMethod_`，没有回调才走默认表读取。本批核`RegistTranslateTable(ThreadStart)`（`0x81DC40`）仅写该字段；`InitStaticFields`先将它清空。对固定DLL可执行节只搜索指向这一地址的直接E8／E9，结果为0；`system.KairoText.cctor`也没有注册调用，仅初始化自己的静态数组。

**注册者尚未找到，不是证明不存在回调。** 直接调用搜索不覆盖内联写字段、反射与间接委托。下一步应从实际启动宿主／翻译表创建者追有限写入与委托绑定，不能把这次零候选写成“所有游戏只读xls.dat”。

## 交付与后继

产品可消费以上语言输入区分、条件资源目录顺序、中文包身份及失败／保存副作用边界。当前不提供完整多语言播放器、字体替代、已观测当前中文文字全集或可操作语言页；也没有改变APK业务、C++Owner、应用快照格式或发布素材。

后继最小研究是：启动宿主与回调字段写入者、LanguageConfig／locale选择的实际输入、语言页确认后的重载链、最终Graphics文字基线与Unity／插件字形回退。外部窗口可记录当前语言菜单和同一标题的文字／Logo，但不得为验证本合同自行覆盖用户语言偏好。
