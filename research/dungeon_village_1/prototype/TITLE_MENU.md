# 标题与选档纯控制器

[接口](include/dungeon_village_prototype/startup_title_menu.hpp)与[实现](src/startup_title_menu.cpp)维护标题两项、两槽两行、raw20、raw1及纪录／配置的父关系。来源为[Steam标题菜单](../ui/STEAM_TITLE_MENU.md)、[Steam手动菜单](../ui/STEAM_SAVE_MENU.md)和[APK标题生命周期](../work/title-owner-contract/README.md)。本模块只准备候选，不持文件、世界、随机或播放器；应用Owner在文件事务与其它消费者均成功后才联合安装。

## 状态与一次性意图

`StartupTitleMenuState`保存根模式、标题选择、存档行、根ID／下一个ID和至多一个raw20／一个raw1或一个外部子页。`draft.slot`仍由应用独占；Context只读传入，候选返回应安装的新槽号。raw20另存打开时槽号及目录修订／摘要，用于拒绝页面打开后目标漂移，它是冻结载荷而非第二个可变选中槽。

所有State直接／嵌套字段都需要进入应用回放；尤其`returned`、预存`result`、确认原因、父ID、菜单frame及目录stamp不能根据现页重新推算。Context的2×2可见性和stamp来自应用系统目录，不由文件是否存在推导。缺stamp的raw20创建或消费显式拒绝；错误父页、缺载荷、越界选择、重复／过期页面输入不发布候选。

`Candidate.intent`是本次具名输出：打开纪录、打开配置、读取记录、隐藏记录或实际开始。它不是持久待发送队列。应用丢弃失败候选即可保留原菜单、序号、选择和答案，成功后只消费一次意图。隐藏意图本身既不改目录也不删除文件；覆盖“是”只进入配置，不产生start或提前覆盖世界。

## 返回与调度边界

raw1确认／取消只标自己的`returned/result`；当前有效栈顶转为raw20。父页再以自己的稳定ID发`consume_return`：是使raw20返回，否／取消清子引用而保留raw20选择及此前预存的1/2。raw20真正返回后，标题根才能再消费并产生文件或配置意图。原菜单取消显式把旧result改成-1，不能将被否决的1/2泄漏到标题。

这几个具名步骤不是自行推定的三个逻辑tick；应用可以在已核的框架准入和私有候选内按实际顺序组合，最后联合提交。存在待返回结果时，父页普通导航被拒绝，必须先消费；因此不会把原“子页返回优先”误作同轮再次确认。纪录／配置返回同样显式标记、由根消费；配置草稿的保留以及纪录页面内部翻页仍归它们各自消费者。

键输入只表达已经过平台／框架准入的repeat与pulse，不能用当前OS按住或绘制次数替代。标题上下以及选档左／右／上／下依原顺序各自独立判断；选档随后校正空中断，并令确认优先于软返回。raw20是up优先down优先confirm优先back，左右不代替确认／返回。raw1是right优先left优先confirm优先back，与纪录的left优先链不同。

两条Steam优先级在既有[机器码摘要](../work/steam-interaction-analysis/disassembly.json)中复核：`FrameMenu(int)`的`0x1030E2DB–0x1030E3A5`及分支返回；`SubForm.Update`的`0x10328261–0x10328427`。raw1／raw20父返回分支在`0x1032748F–0x10327585`。这些是具名方法内的有限窗口，未据方法名推广为完整Steam调度已证。

触摸请求只支持本批已核组件0／3／9的ENTER／UP。标题id0的UP只发确认，ENTER才写行；id3／9的UP先写行再确认。空中断没有实际触摸组件，伪造该行输入拒绝。箭头、平台键盘、OS焦点等其它事件应由对应适配器具名归一化，控制器不猜未核映射，也不套用设施75的首次UP标记规则。

菜单frame仅通过`advance_frame_menu`明确推进一次FrameMenu内部`+1/min(3)`，普通导航与查询不推进。它不包括SubForm.Update前置计数、自身绘制、手形SEB或背景人物时钟，不能宣称此请求就是完整游戏tick；应用后续接线须保留这层资格。

## 验收与规模

[测试](tests/startup_title_menu_checks.cpp)归既有application套件，使用明确的目录条件输入，不构建假原档。覆盖独立方向与优先级、ENTER／UP差异、默认否、否决后取消、分级结果消费、过期ID、缺目录身份、修订／摘要漂移、非法载荷和序号耗尽。文件失败、随机联合回滚、系统目录发布及声音保序由应用事务套件主责，本模块不能代替它们。

控制器不保留退休页历史或累计事件队列；活跃载荷上限为raw20＋raw1两份。单调next_id的合法增长不是内存增长。没有新增图片或资源副本；已接现有application套件，并随[五项集中验收](../work/title-menu-application-delivery/README.md)通过。独立控制器加应用文件事务已交付，完整原窗口／框架输入和自动中断产生者仍缺，不能据此宣称整个Steam标题已还原。
