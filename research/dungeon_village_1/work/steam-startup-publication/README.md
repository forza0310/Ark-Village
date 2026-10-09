# Steam启动图块最小发布方案

2026-10-09。下文保留最初的最小集合方案；后继已按授权完成[正式发布](../../assets/steam-startup/README.md)，不修改APK原761文件。原始输入身份沿用[Steam启动资源](../../ui/STEAM_STARTUP_RESOURCES.md)，消费合同见[标题绘制](../../ui/STEAM_TITLE_DRAW.md)与[菜单／选档](../../ui/STEAM_TITLE_MENU.md)。

最新交付：17差异文件加MANIFEST／README，共19文件335,725字节；21别名仍指向原包。已用[正式发布工具](../../tools/scripts/publish_steam_startup.py)重新解档核字节、PNG像素和SEB依赖；重复发布零新增、只读check、异内容预检不留部分写入、缺项拒绝、同步失败清理本次新建树均通过。38条发布路径与全部761个APK原文件hash复核通过，无未登记文件；隔离探针已清理，无后台任务。结果见[验证摘要](PUBLICATION_VERIFICATION.json)。

## 当前交付缺口

方案提出时`assets`正式发布目录为`original`、`normalized`、`audio`。当时20项启动资源交叉表只在证据里登记Steam身份，尚无Steam运行素材包。其中16项与APK已发布条目同字节；4项不同（title00、两种title_logo、title_grass）。后继`steam-startup`已补Steam新增upper与中文等saveload语言变体的正式路径。产品不应读取Unity容器、APK或work证据目录补齐资源。

原素材可以按已核hash复用，但清单必须明确其Steam来源相同；不能因PNG名称相同就将APK图替代Steam差异图。完整语言列表仍由消费者／运行配置决定，方案保留全部小型saveload变体，不猜用户中文会话的实际fallback。

## 最小集合与依赖闭合

[MANIFEST_DRAFT.json](MANIFEST_DRAFT.json)提供38条拟发布逻辑身份，总359,293字节：25PNG、7SEB、6INF。32项有效素材来自既有20项、upper1项、saveload10项和新补finger_l.seb1项；目录6项另计。此集合覆盖已核启动基础图块与选档，不是完整标题人物／字体／通关皮肤。

| 集合 | PNG／SEB／INF | 用途及来源差异 |
|---|---:|---|
| title | 7／0／2 | title00、window、默认与英语logo、cursor、grass、upper；四旧条目不同，upper仅Steam；空seb.inf仍保留原身份 |
| event | 2／0／2 | backGlad、medelCelemony_back2及两目录，全部同APK |
| common窗框／箭头／手形 | 6／7／2 | wnd_back/bar/conner、arrow01/02、finger_r图；六同名SEB加finger_l；素材同APK，两INF不同 |
| common存读档图 | 10／0／0 | 根目录、de、es、fr、hi、it、pt、ru、tr、zh-CN；根图同APK，其余语言变体独立 |

七SEB均先核Steam记录与APK发布副本SHA相同，再解析当前字节检查引用闭合：

| SEB | 图片ID及真实成员 |
|---|---|
| wnd_back | 28 → wnd_back.png |
| wnd_bar | 29 → wnd_bar.png |
| wnd_conner | 30 → wnd_conner.png |
| arrow01 | 72 → arrow01.png |
| arrow02 | 72 → arrow01.png，74 → arrow02.png |
| finger_r、finger_l | 70 → finger_r.png |

标题选择图标用common SEB22=`finger_l.seb`，原20项表只含finger_r.seb，因此需要补这一个SEB；**没有finger_l.png依赖**。选档两裁片使用177号saveload，而非该SEB。全部裁片仍以消费者合同为准，SEB帧数不能替代时钟。

保留六份INF原字节作为来源与ID证据，不能删无关行或重编号。其余INF条目（例如title广告图、common其他101项SEB）不在本最小集合中；清单须注明这是部分发布，产品只加载清单列出的逻辑条目，不能让旧“整组Load”隐式要求所有INF引用都存在。已选七个SEB的正图片引用已全部在集合内。

## 建议的正式发布结构

建议新增`assets/steam-startup/MANIFEST.json`、`README.md`及差异文件子树`original/<group>/<原条目路径>`。清单保留来源容器／TextAsset pathID、原条目、SHA、尺寸、语言路径、APK等同性与相对发布路径；产品根据清单把所需实际文件复制到自己的assets，不读取研究输入容器。

- 21条同字节身份直接别名引用现有`assets/original`，不复制第二套素材；包括18PNG／SEB和3INF。
- 17条需新文件，304,770字节，包括14张Steam差异PNG和3份Steam INF。其余54,523字节已由原发布包提供。
- 未来如产品需要完全独立游戏包，打包器按清单收集本批38逻辑项；生产包文件可按hash去重，不能把相对研究路径当运行路径。

上述结构已实际发布，未新增产品加载器或CMake。原APK761包的数量、hash和用途保持原定义；Steam集合另设分母，不混入761。MANIFEST_DRAFT保留最初方案资格，实际交付以正式MANIFEST及验证摘要为准。

实施已沿用已有纯归档读取器，先核resources.assets容器、对象、payload与条目hash，再核PNG CRC／RGBA后发布白名单；该Unity内嵌归档不虚构额外游戏CRC字段。不复制整个resources.assets，不导出字体。不同内容的已存在目标拒绝，不覆盖；同内容重复运行零新增；`--check`只读核验实际文件及全部别名。隔离小目录测试冲突拒绝、缺项、重复运行和七SEB引用完整性；无需要运行游戏或全量CTest。

## 证据规模与限制

`audit.py`只读既有图像清单／启动证据／APK发布副本，写本目录清单草稿，没有提取Steam载荷。它核38项唯一身份、现有别名实际hash与七SEB的图片引用；**不把未实际发布的17项标成已核落盘**。后续发布器须重新解档核字节，不能仅相信旧清单。

字体sfnt四载荷、标题人物98项交叉集合、音频26项、通关秘书／大臣完整皮肤不纳入这38项；它们分别已有专题或独立发布，不能由“startup”目录名推断全部启动表现已齐全。Chinese字体fallback、标题实际语言目录、平台按钮和最终窗口尺度仍待独立交叉。

脚本无后台进程、不改原档、不构建、不提交。本批正式交付与最初方案分开登记，由主会话统一归档；产品加载与窗口另验。
