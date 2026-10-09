# Steam设施详情74：定义预览、实例与两页内容

2026-10-10。固定Steam2.56的局部完整绘制分支、初始化及输入合同。直接来源是GameAssembly具名方法和metadata，见[复算工作包](../work/steam-facility-detail/README.md)。没有用APK补Steam字段，没有新增窗口实验；S043等只保留为独立画面观察。本文的坐标是原Graphics逻辑坐标／调用实参，不是桌面截图像素，字体翻译、原点、缩放与裁剪仍沿已有合同。

## 两个入口不能合并

| 入口 | 已核原载荷与前置分流 |
| --- | --- |
| 建设目录21的详情 | [前批合同](STEAM_BUILD_LIST_BUSINESS.md)核到构造74、value1=1、tenantData=选中定义；不写tenant实例。软键7只给type2/3/13，住宅type12不从该入口预览 |
| 场景当前选中设施 | `_update` 0x10305670从当前selectedTenant取实际实例，调用GetTenantData。**先看定义isLvUp**，真则打开81；否则施工state=0不打开详情；已入住住宅type12且myHomeCharaDataId≠−1直接开人物60、charaInfoChase=1；其余路径才打开74，写实例、定义、value1=0 |

场景升级81先于施工拒绝，是原实际检查顺序；不能改写为“点任意建筑都先打开74再提示升级”。既有说明事件先执行：type13查81，role2查87，武器／防具／饰品distinction1/4/5分别查84/85/86；未发生过则ExecEvent，随后仍走74构造。这里81是**事件ID**，与升级raw81不是一个命名空间。

住宅74绘制与输入确实存在，但不代表场景正常点已入住住宅可达该支。无居民住宅若被错误送入74住宅绘制会按−1访问人物定义，原代码并没有维护层的显式载荷拒绝。产品不得据“分支存在”创造一条不满足身份条件的入口。

74本身不收费、不投道具、不完成升级；它展示并交接至75/79/80，或住宅追踪。维护Owner仍要保留唯一提交者与失败回滚，不能把原私有对象引用布局当跨域事务保障。

## 初始化与引用身份

Init2的74支为0x1030F8C3–0x1030FC57：新建ary1；distinction1、4、5分别枚举已开放state≠0的武器、防具、饰品定义，6调用EnumerateMyHomeChara。它们用于“种类”／入住人数，不是邻接实例列表。

tenant非null时另外新建ary2，逐项加入tenant.tenantBonusList中的int[]对象；这一层是列表复制，**不是每条int[]深复制**。条目绘制读[0]邻接建筑定义ID、[1]编号（显示加1）。本批不把它改名为实例uid，也不猜其全部生产和退休时机。预览没有tenant则不生成该实例邻接表。

原Init共同入口按softLabels[type]装配软键，再委托Init2。标签2取消消费者已证；本批没有独立解出softLabels[74]的全部初值文字，因此不补写猜测的左标签名。页面select、scroll、page与字段初始化沿构造/父页生命周期；不能由单张第一页截图证明每次Resume全部归零。

## 五种布局与共同窗框

分派顺序是type12→住宅布局0；distinction1/4/5→装备店布局1；6→入住布局2；type2→植物布局4；其余→普通布局3。metadata还声明INN=5，但本绘制分支不选择5，不据枚举补一套旅馆专属布局。

普通布局3且value1=0时标题为`LT("施設情報 <0>/2",page+1)`，其他为`施設情報`。均调用DrawWindow(220,168,0)与DrawBox(17,74,219,184)。木纹、标题带、边框、文字色和角图按[窗框合同](STEAM_WINDOW_FRAME.md)，不能靠把截图背景缩放成整张弹窗替代。

只有普通实例调用Draw_titleBarArrow(220,168)：common SEB3 arrow02，左frame3、右frame0；Touch组件1的值分别16、18。令k=trunc((frame%16)/8)，逻辑点为左(26−3k,55)、右(214+3k,55)。这些是原帧字段，不换算秒。两边都持续注册；按当前逻辑两页循环，非第一页禁左／末页禁右。

页0标题下：设施小图标在(21,66)，Draw_icon(mode9,index=定义iconIndex)。它读common图片91 icon_tenantInfo，裁片(16×(index%10),0,16,16)。名称日文在(41,69)anchor1；非日文临时字号11，TextLayout(41,69,85,11)，anchor0x20、lineSpace0，随后ResetFontSize。名称色SC_WINDOW_BLACK；右侧标签为SC_WINDOW_BLUE。

普通价格标签日文(136,69)，非日文字号10、(133,70)。数值以Draw_money(x223,y70,seb15)；装备店改显示“種類”与ary1.size，DrawNumImage(seb15,x214,y70,padding0,anchor4)。不能将种类数当库存总量。

## 定义预览与实例数值

普通页0价格／品质／魅力分别为TP0/1/2。value1=1走GetTenantParam，不加实例bonus；value1=0走GetTenantParamAndBonus(…,tenant)，并与GetTenantParamLimit比较，达到或超过上限画common图片129 wnd_max。MAX是显示标记，不证明本页重新封顶或扣费。

品质／魅力行基准y=93、108。标签日文x136，非日文临时字号8、x130、y再加2；数值SEB15，日文x205、非日文x207，y仍93/108，padding0、anchor4。MAX点为(160,y+3)。价格MAX另在(160,72)。预览不走这些实例MAX判断。

大图先画绿色底板：普通布局x=23，住宅／装备／入住／植物x=71；外框DrawRect(x,88,97,74)，色(181,219,166)，内填(x+1,89,96,73)，色(219,249,207)。ClipRect(x,88,98,75,true)后调用**DrawMapchip2(g,x+49,125,mapchipId,0)**，随后PopClip。无论实例朝向如何，本页传的方向都是0。

DrawMapchip2使用已核[pattern和格偏移](STEAM_MAPCHIP_PATTERNS.md)，但比建设锚点另加居中偏移：pattern0=(-30,0)，1=(-45,7)，2=(-30,15)。每块实际点为中心点＋此偏移＋(30(u+v),15(u−v))，再走原SEB裁片偏移。不能沿用建设DrawMapchip而漏掉大图居中修正，也不把底板宽高当整座图的强制缩放。

普通布局附common图片38 wnd_lv于(25,151)。lv≠5用SEB12数字，日文(53,151)、非日文(55,151)，anchor4；lv=5用wnd_max于日文(44,154)、非日文(46,154)。这仍读取共享定义lv，不是每栋实例独立等级。

右侧两块浅黄色区：外框(123,88,90,34)、(123,124,90,38)，色(255,207,99)；内填(124,89,89,33)、(124,125,89,37)，色(255,250,213)。使用效果逐行从effectTUseType/Value读取：图标mode7在(136,127+17i)，common图片37第二行16×16；类型5特殊取x96，其余16×(类型%10)。数量以common SEB15 frame14由x192向左每次8，y=130+17i重复，不能误认成普通数字字符串。无效果画`---`于(168,137)、anchor2。

lv≠5下方显示“距下一等级”：剩余=GetNextLvUpNum()−tenantUserNum，SEB16于(188,172)，后接SEB12搭配**Steam**图片103 number05、frame18于(189,175)。En()为真另画图片178 ppl于(191,174)。lv=5换成最高等级提示。预览同样显示定义成长信息，但未认证当前截图中的具体剩余数字。

## 页1：邻接加成而非普通详情继续按钮

普通实例第2页上方画“施設ボーナス”(27,69)、anchor1；“維持費”(132,69)，TP3 GetTenantParamAndBonus以Draw_money(214,70,seb15)展示。列表空时(120,97)居中显示“ボーナスはありません”。

ary2最多5可见行，y=97+19×(绝对index−scroll)。选中行画SEB21 finger_r于(17,y+8)。行Touch SLIST11的矩形是(-1,y−2,231,18)、value=0x20000|绝对index，**本页没有raw75那套marker16/Margin(29…)调用**。只复用通用SLIST分派，不能复制道具选择行的二次点击资格。

每行以条目[0]查tenantData、mode9图标(26,y−2)，名称拼接条目[1]+1；非日文字号10于(44,y+1)，日文(44,y)。增益文本来自邻接定义effectAroundTenantValue：普通显示魅力＋第0项；type2植物另外显示价格＋第0项、品质＋第1项。原字符串包含`<co=0065FF>`与`</co>+`，要交给原文本标签规则，不能把标签字面显示出来或先用当前TP重算另一套数值。

日文普通魅力位置x147，植物价格x121／品质x171；非日文字号9，普通魅力(x151,y+2)，植物分别TextLayout(130,y+2,35,9)、(168,y+2,35,9)，anchor0x20。末尾恢复字号。滚动条调用DrawVerticalScroll2(221,85,4,110,scroll,count−1,5)，上界实参是count−1；空列表也走此helper，边界规则沿其消费者。

页1上/下重复脉冲独立检查，select按count循环且只在count>0时处理；scroll跟随保持5行。符合普通实例资格时，左和右各自令page=(page+1)%2；同轮两者都命中会切两次。原代码没因切页清select/scroll。第2页确认直接返回false，不推75、不选择邻接设施、更不消费道具。

## 特殊布局与底部说明

住宅布局通过实例myHomeCharaDataId查人物定义，以职业GetImgId和当前武器绘人物；动画索引trunc((frame%16)/4)，SetDispPlayerData传seb0、direction1、updateCnt=frame；人物绘于(145,155)，底部姓名说明居中(120,172)。这含显式共享表现请求，不是无副作用的纯函数。

装备店底部将distinction1/4/5翻译为武器／防具／饰品后填“正在出售”；入住布局按ary1.size显示当前申请人数或无人；植物布局显示effectAroundTenantValue的价格和品质，并在y199说明影响周围。植物非日文临时字号10，日文字号11；不能把日文和中文排版当同一路。

普通页0底部Draw_btmMsg“使用道具”，y197、select=true、cursor=true、widthMax200；页1为周围设施加成说明，select=false、cursor=false。同一helper还用于装备店“查看商品”、入住“入住申请者”。value1≠0跳过这些互动提示。预览value1=1最后登记Touch组件2、TouchOption.Create(2)；其最终物理热区及关闭键分派仍由框架决定，不根据无坐标重载猜成某个屏幕矩形。

## 确认、取消与行为交接

确认脉冲0x100000优先；另有配置布尔门控的keyState0x40替代确认路径，其配置的用户可见含义未在本批认证。未确认再检查softlabel2，命中Pop。确认时顺序为：

1. value1恰1：直接Pop，回建设父页；不扣款、不转道具页。
2. page恰1：返回false，不执行后续业务。
3. type12：从当前人物实例列表找定义ID等于住宅myHomeCharaDataId的人，写相机追踪人物、清追踪设施，ChangeState(6)，再Pop；找不到同样Pop。
4. distinction1/4/5：构造79，只写value1=distinction；本支不传tenant实例。
5. distinction6：构造80，复制tenant和tenantData；普通非植物构造75，同样复制两者。
6. type2植物：无确认业务，返回共享true。

Pop与Push的本地调用已证，不代表完整FormManager退休、所有父级选中保留和最终OS投递均已认证。页面私有坏值不能靠原代码的非null崩溃分支作维护错误协议；原实例、共享定义、邻接引用与历史对象仍要分别校验。

## 资源与交付资格

本包登记20项common引用，含SEB间接图片依赖；18项逐字节对应已有出版副本，number05必须使用[Steam专用副本](../assets/steam-build-common/README.md)。两张尚无Steam出版副本，均已核与APK同名图片字节和RGBA不同：

| 图片 | Steam身份 | 已证消费者 |
| --- | --- | --- |
| 37 icon_param00.png | 112×32、1790字节；SHA-256 `2b99ce3fa12e25e4d1fab78e06b6cd0ada87f8e3f4bf68c94c8421d7172c22e0` | Draw_icon(mode7)使用效果图标，第二行16×16，类型5选x96 |
| 105 number08.png | 100×21、648字节；SHA-256 `8914321921983ba8508b86db5c499cb580df1a5468860d354a09cede9d9f8361` | SEB15的价格／维护费／品质／魅力／种类数字与使用效果frame14点数 |

不能取APK副本冒充这两个Steam依赖。本批只登记源身份，不复制素材；完整SEB裁片／偏移及图片依赖已随EVIDENCE登记并检查范围。

本批覆盖局部raw74绘制0x103453B8–0x1034728A及上述具名helper，不声称完整皮肤完成：softLabels74具体表值、未出版效果图、最终中文字体／TextLayout标签后端、场景输入到selectedTenant全部路径、表单退休仍有独立缺口。既有S043/S047/S054只能证明当时画面；本批没有原窗口逐像素验收、住宅74自然可达认证、维护C++测试或产品接入。
