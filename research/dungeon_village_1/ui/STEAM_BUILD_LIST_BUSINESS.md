# Steam建设目录21的初始化、选择与确认

2026-10-10。接续[raw21绘制](STEAM_BUILD_LIST.md#steam后继完整raw21局部绘制分支)和[图块／旋转资源](STEAM_MAPCHIP_PATTERNS.md)。本页独立核固定Steam2.56的Init与Update局部分支；APK只作末尾交叉，不用APK填Steam空白。来源见[工作包](../verification/steam-build-list-business/README.md)，未改维护C++／产品、未操作原窗口或存档。

## 目录生成与打开时的副作用

`SubForm.Init` RVA0x312460在VA0x10312767识别TYPE_BUILD=21，进入0x10314793，至0x10314B0A返回。进入该分支前已调用MyFormBase.Init、取得UserData、DecideKeyState并清frame。类型构造本身置select=0；page／scroll仍不可由“看到一个默认0截图”推断所有恢复情形都重置。

初始化按原定义数组顺序建立3个Vector：

- BaseData.state_必须非0，flag_包含FLAG_BUILDABLE=4。
- 住宅type12另外要求haveNum_>0。
- 加入`buildList_[category_]`，保存定义ID，不保存PNG号或实例ID。
- 第0类索引1插入撤除−1；UserData.ChEventFlg(32)通过时，索引2再插移动−2。
- 随后调用`Update_residentFlg(false)`，以当前page/select直接取条目并更新软键。

原代码依赖合法目录与当前索引；它没有为全部空类或越界私有页载荷提供维护层的显式错误协议。维护消费者继续应校验并明确拒绝，不靠map.at或补虚构首项掩盖。打开21也不是绝对只读：其中存在入住标志刷新与输入状态决定调用。

原表category来自tenantData列8，字段+0x38；flags来自列35，字段+0x0C。固定85定义中50项带建设位4，分类数量13／25／12，住宅候选是25、27。这只是定义候选全集，实际state和haveNum是运行时条件，不能将50项当新局目录。位8是FLAG_FENCE，不是本页开放过滤；设施type、category与mapchipPattern也不能互换。

## 五行范围、空行与键盘顺序

Update入口RVA0x321990在VA0x10321DA9识别type21，分支为0x1032599A–0x103262D8。先DrawAllForms，**之后**才检查frame_≥3；绘制本身在上一合同已证会推进并夹frame，不能把这个顺序换成“先三逻辑tick禁画／禁输入”。未满足门槛直接走共享true返回。

本分支不用通用Update_scrollValue。令`count=buildList_[page].size()`、`upper=max(count-1,4)`，真实顺序如下：

1. 保存更新前选中定义；select≥count时用内部哨兵−99。
2. 上重复脉冲0x20000令select减1，再检查下重复0x80000令其加1。
3. select恰−1时设为upper，scroll先设`select%upper`（因此这里为0）；恰upper+1时select和scroll都归0。随后仍继续执行跟窗，不能只保留上绕时scroll=0当最终结果。
4. 左重复0x10000产生类目增量−1，右重复0x40000后检查、命中则覆盖为+1。非0增量使page=(page+3+增量)%3，并清scroll/select。
5. select≥scroll+5时scroll=select−4；select<scroll时scroll=select。
6. 再取得选中定义或−99；前后定义ID不同才刷新软键。

因此少于5个实际条目的类目允许选中已登记的空行。五行原输入组件不因条目不足而消失，空行也可能参与marker；但确认后的范围检查会阻止把空行当建筑。

原分支只对精确的−1／upper+1处理首尾循环，并不是对任意坏整数取模或夹取。可维护实现的载荷拒绝仍独立，不能将负数私有选择塞入原算法后声称安全等价。

维护提供[纯游标候选](../prototype/include/dungeon_village_prototype/startup_world_building.hpp)`prepare_startup_build_menu_cursor`：显式接收三类长度、类目／选择／首行及同轮四方向，不读取或修改Owner，不处理确认、软键、时钟及平台marker。宽整数中间运算与坏初态拒绝属于维护保护；函数保留合法尾窗，不用`count−5`重夹首行。已批准的Owner与窗口接线沿同一算法，完整持久布局见[已确认设计](../prototype/PERSISTENCE.md#建设21目录游标与74预览恢复设计)。

## 鼠标标记和类目触摸

上一[绘制合同](STEAM_BUILD_LIST.md#实际marker滚动和箭头注册)已核21每行注册SLIST11、marker16和绝对select值；结合已核[通用OnTouchDlgSel](STEAM_INTERACTIONS.md#输入选中标记与确认分开)，ENTER／UP写选择，首次未标记UP只记组件key，再次相同key的UP才发0x100000确认。没有双击时间阈值；键盘会清marker，不能把键盘确认夹在两次点击之间当相同前提。

SubForm.OnTouchEvent针对21的组件3、value高16位0x50000，在UP=1／ENTER=2计算目标类目差。变化时模3换page、清scroll/select，并尝试给新类目组件发触摸效果；该类目分支返回false，阻止通用组件二次处理。列表行保留通用处理；组件18在事件5另有方向效果路径，不能混成所有组件都换类。

触摸末段按`select_`与`value1_`比较或类目变化决定是否刷新软键；本方法没有把value1当成类别字段，也没有在本段更新value1。当前已证来源不足以替它设计新的长期“前一次选择缓存”；只按实际字段与后续回调顺序消费。

## 软键、详情与取消

具名`UpdateSoftKeyBuildWIndow` RVA0x320D60对普通非负定义读取type：type2／3／13用左软标签7、右2；其他type或任意负哨兵用左0、右2。文字来自AppData.SOFTLABELS，实际翻译仍在字体／语言链。

确认0x100000优先。没有确认时，标签2调用Pop；否则标签7且选中ID≥0时构造raw74详情，将value1_=1、tenantData_=当前定义，再Push。2026-10-14沿原Update21局部补核`10325CDD–10325D7F`：此ID是**本轮方向、换类与五行跟窗之后**取出的当前`buildList_[page_][select_]`，不是上一轮ID、定义类别或首行；其前段范围不足时为−99。`10325D43`构造74，`10325D4E`写原value1_=1（+B8），`10325D58–10325D5C`绑定tenantData_（+10C），`10325D65`Push。此链不写tenant实例，也不写父21的page_／select_／scrollValue_（+D8/+90/+88）；新子页的页签／选择独立。维护74的`legacy_f`承载定义ID、`legacy_g=1`表达预览，是已有映射策略，不能误抄成原value2_=1。

正常软键7仅由type2／3／13提供；结合初始化，目标必须属于已冻结三类目录、state≠0并带建设位4。没有额外distinction筛选，因此符合入口的普通／装备店／入住／植物布局沿[74自身分派](STEAM_FACILITY_DETAIL.md#五种布局与共同窗框)；住宅type12、道路type6及撤除／移动／空行没有该预览按钮。标签7消费分支本身只重查ID≥0、不重复type条件，这是原合法UI前提，维护应拒绝伪造按钮或错类条目，不能据强行注入soft7创造住宅预览。它也不要求商会85的state=0或saleRank条件，已开放设施即使已有实例仍预览共享定义，不能沿用85专用“未取得定义”校验。

74确认原value1_=1在`1031B5EF–1031B5F9`优先于第二页、住宅追踪和75／79／80业务，跳直接Pop；未确认时soft2也Pop。此局部没有资金、库存、实体、建设模式、定义NEW或目录NEW写入，回到21后也没有从该支重建目录／重新刷新入住标志。沿[现框架合同](INFORMATION_MENU.md#steam9与36输入及框架身份交叉)，子页Pop将其置退休并使真实父页进入PREUPDATE，SubForm.Resume／Finish（RVA1EDFA0）只返回true；父对象仍持原三字段，不能把返回改成新建21并归零。背景重画可夹21的frame到3，不推进主场景经营；确认真正进入BUILD使用ChangeCurrentForm，和预览Pop是两条不同路径。上述是静态局部保持，不认证原窗口动态或最终物理按键。

维护使用`build_page_ui`保存21的绝对选择／独立首行，类目、完整冻结目录及主副钟沿既有容器。新`input_build_menu`把原序方向、平台行／类目／滚动请求、确认／取消／详情放在同一候选；坏初态／错父／缺键拒绝。父21只允许当前开放且带建设位4的type2／3／13，父85仍保留未取得及售卖资格，不把一条放宽套到另一条。74返回保持父选择／首行，不重新枚举目录；退休21保留合法冻结历史至Finish，释放目录／UI／双钟。具体拒绝与旧布局政策以[持久合同](../prototype/PERSISTENCE.md#建设21目录游标与74预览恢复设计)为准。

`draw_build_menu`只在明确获准Draw时使主钟加一钳3，覆盖中的父21可绘，暂停保持；公共Update另推进主副钟。皮肤只读，不能代Draw推进。研究窗口在实际绘制完成后提交该请求、下一帧输入读取已画钟；与原Update内部先DrawAllForms的物理调度区分，不认证原OS同帧事件。窗口仅保留短期marker／拖动手势，不持第二份选择；键盘整轮、类目点击、同一行二次UP、滚轮／拖条及详情都交Owner。完整框架Touch缓存、共享手形时钟及原物理输入仍是独立边界。

## 确认只进入建设状态，实际扣款在后续

确认消费后先检查select<count；空行直接返回true，未创建实体或切BUILD。合法条目的报价为撤除0、移动300、普通GetBuildCost；只有报价>0时与`AppData.save_.lngs[SAVEL_MONEY=0]`比。金额不足调用ExecEvent(11,null,null)并返回false，本页不扣钱。

| 选中条目 | 模式调用 | 本分支额外写入与交接 |
| --- | --- | --- |
| 撤除−1 | ChangeTBMode(3) | GameForm.ChangeState(BUILD=1)、ChangeCurrentForm(game,false)，然后显式设置撤除软键；**本分支没有写tenantBuildId或朝向** |
| 移动−2 | ChangeTBMode(6) | 同样转BUILD并ChangeCurrentForm；**本分支没有写tenantBuildId或朝向** |
| 道路type6 | ChangeTBMode(1) | 写tenantBuildId=定义ID、朝向0，再转BUILD并ChangeCurrentForm |
| 普通flags4 | ChangeTBMode(0) | 同道路的定义ID／朝向写入；真实位置与扣款仍交后继 |

`ChangeTBMode` RVA0x2202A0只写tenantBuildMode并调用SetHelpMsgInit；后者按模式取TBMODE_HELPMSGS、清提示计数。不能把它推断为自动将tenantBuildId改成−1／−2的隐藏消费者。特殊分支继续使用已有状态的事实需保留，维护Owner的显式模式模型与原静态字段布局是两个层次。

普通定义路径在ChangeCurrentForm之后清该定义BaseData.new_，调用SetIsExistBuildNew重新统计目录NEW。道路首次提示看UserData.flag_的64位掩码，普通建设看128；首次时先置标志，再MakeTalkForm并Push说明。这里位掩码64／128表示提示已消费，不是实体施工已完成。

维护新同轮输入已组合资金不足event11及首次64／128说明：不足资金仍保留本轮选择和原提示，但不扣款；产页失败连同选择、标志和建设模式一起回滚。旧显式定义`select_build_menu`接口仍保留已有无修改denial合同，不把研究控制器直接调用冒充完整UI输入。Steam `MakeTalkForm`（RVA315DE0）为raw0、无人物、无纪录号；普通提示实际字面为`<btn=16,1>/<btn=29,0>で画面をスクロールしながら建てる場所を選びます。`（literal1163／槽110EB510），道路为literal9914／槽110F13D0。维护对白沿固定APK `b/g.java:11620/11624`原中文，普通“旋转画面／点击微调”不冒称Steam译文；标志与产页顺序两版交叉，文本输入分别登记。

当前合法初始化确保普通条目有flags4；若外部错误载荷绕过初始化塞入其他非道路定义，原分支并没有普遍重新拒绝：可能保持旧模式后继续写ID。维护侧必须拒绝坏载荷，不能照搬这一不完整防护。原本页也没有世界Owner事务；原顺序和维护失败回滚策略分别记录。

## APK交叉与验收资格

固定APK `b/g.java:10647`、`:11537`及相邻已核窗口，与Steam目录过滤、五行空行、左右顺序、确认报价、特殊模式不写ID、普通写ID／朝向0及首次提示逻辑相符。Steam本页结论来自自己的机器码消费者；不同版本行margin／类目矩形差异仍保留，不以业务相近抵消视觉差异。

8个具名窗口／必要前缀合计30276字节；Init的21分支888字节，Update的21分支2367字节，52个直接调用或赋值锚可复算。共享true返回只复用旧已核Update证据，未新扫整段方法。本段静态研究没有原窗口或自然建设证据；维护短验另记[当前验证](../VERIFICATION.md)，不代产品接入结果。

剩余：FormManager.ChangeCurrentForm的完整页栈退休、触摸value1生命周期、当前语言／最终OS投递、建设落点／道路／移动／撤除的后继事务及所有世界绘制遮挡；74预览Pop回原21的上述静态局部已核，原窗口动态仍独立。本批已证内容可作为后续实现合同，不把这些独立缺口补成成功。
