# 图像与容器全量盘点

本清单区分固定 APK 1.0.8 与用户提供的 Steam EXE 目录，不以 EXE 新资源补写 APK 原表，也不扩大原素材发布范围。它回答“本机固定输入中有哪些资源、如何定位与核验”，不证明每张图的精确消费者、显示条件、动画时间或原型接入完成。

机器清单为 [IMAGE_COVERAGE.json](IMAGE_COVERAGE.json)，正式复算工具为 [image_coverage.py](../tools/scripts/image_coverage.py)，运行 `py -3 research/dungeon_village_1/tools/scripts/image_coverage.py`。本机执行摘要与边界见 [盘点工作记录](../work/image-coverage-audit/README.md)。现有 [MANIFEST.tsv](MANIFEST.tsv) 的 761 个原素材文件仍是发布分母；[资源与代码索引](RESOURCE_CODE_INDEX.md) 的词法反查是另一层证据。

## 输入与分母

固定输入 `maoxianmigongcun.apk` 的 SHA-256 为 `1e52408aeaebec2d92a5e50b8c0d964566d0038456bc9b081e18b7d3446a4ef5`。脚本核对 APK 身份、全部 ZIP 条目 CRC，以及条目与 Kairo 归档边界。EXE 分母为 `DungeonVillageEXE/` 当前的 25 个文件，逐文件记录字节数、SHA-256 与 PNG 签名候选数；不涉及实时存档或游戏进程。

| 证据层 | APK | EXE | 含义 |
| --- | ---: | ---: | --- |
| Kairo 归档 | 15 | 20 | EXE 为 18 个 Unity TextAsset 归档及 2 个 IL2CPP 托管归档 |
| PNG 条目 | 432 | 955 | EXE 的 917 个 TextAsset PNG 与 38 个托管归档 PNG分别计数 |
| SEB 条目 | 341 | 578 | 均通过当前样本的非压缩旧结构解析；不等于动画语义已完成 |
| INF 条目 | 23 | 30 | 全部 `img.inf`／`seb.inf` 目录目标可解析；修饰符保留但不执行 |
| Unity SerializedFile | — | 7 | 3152 个对象按边界、class ID、path ID 登记 |
| Unity Texture2D | — | 114 | 69 个外部流载荷、45 个内联载荷；原始纹理像素尚未解码 |
| Unity Sprite | — | 70 | 全部局部纹理引用与矩形边界通过；这是裁片对象，不能加到 PNG 数量 |
| 包级 PNG／DIB | — | 29 | PE 资源为 7 PNG＋16 DIB，托管 System.Drawing 的六个 ICO各有一个 DIB帧 |

现有发布的 11 个视觉归档共 398 PNG、341 SEB、22 INF，总计 761 文件，已逐项直接核 APK 条目和 `assets/original/` 原文件 SHA-256。**APK 全量图片比此发布分母多 34 张**：`system` 12、`lineup` 16、APK `res/` 6。它们已登记，未复制到素材发布目录；原来的 398 张不能再写成 APK 所有图片数量。

EXE 的 114 Texture2D 含五个零尺寸 `Font Texture` 运行时占位；占位已明确登记，不能当作丢图。格式计数为 1:1、2:34、3:19、4:27、10:16、12:17。载荷声明长度、外部流位置、完整载荷 SHA-256 已核对，DXT 等格式的实际像素仍是未完成项。

## APK／EXE 比较

APK 的 432 PNG 全部字节唯一。EXE 的 955 PNG 有 847 个唯一字节载荷、50 个同字节别名组，重复超额 108。别名以原容器和条目身份保留，不删除语言变体或不同库版本。

EXE PNG 先按资源组和条目名匹配 APK，语言 `.lproj` 前缀单独记录：382 个同名候选字节与 RGBA 像素相同，74 个同名候选不同，499 个没有同组同名候选。这个分母保留所有语言条目；不能将 499 当成 499 个新增画面，也不能把字节一致当作两版本消费者相同。

35人物目录后继消费已核：Steam common31的`icon_result00.png`以原条目出版到[共用差异包](steam-common/icon_result00.png)，63×48、1000字节；SEB44沿既有同字节副本，实际frame4也有透明像素RGBA差异，不将两版整图混为同一身份。该页其它已出版图片及human包67项已逐字节交叉，数字SEB11／20分别实际引用image102／108，详见[35资源对应](../ui/INFORMATION_MENU.md#steam35完整局部绘制合同)。此增量不改变上表原容器分母，也不认证索引语言后缀或字体后端。

`kairolib` 在 Unity TextAsset 与 IL2CPP managed 两种容器中都存在，分别有 20 和 38 张 PNG。清单保留不同容器 ID 和 `archive_origin`，不能只凭逻辑组名合并。此前仅数 TextAsset 会漏掉托管归档，此次已补齐。

## 已验证与未验证

- **存在与定位**：全部当前输入文件身份、Kairo 归档条目、Unity 对象、托管资源外壳、PE 视觉资源均登记。Kairo 与托管外壳解析无失败；未解析的七个托管 `collation.*.bin` 是独立列出的非图像载荷，未把无 PNG 签名当作“已完全理解”。
- **图片结构与像素**：APK 432 PNG、EXE 955 PNG及包级七个 PNG 全部完成签名、chunk CRC、zlib、扫描行滤波与 RGBA 解码/hash。DIB 22 个仅核头部尺寸；Unity 114 个纹理仅核结构和载荷，未混记为像素通过。
- **索引与切片结构**：全部旧 SEB 完整读取、`img.inf`／`seb.inf` 目标缺失为 0；70 个 Unity Sprite 的纹理局部引用及矩形边界通过。SEB 图槽、负 ID 绘制命令、实例修饰符和复合层运行时语义需继续沿代码消费者解析，不能用结构通过替代精确切片认证。
- **精确消费者、条件与时钟**：需要逐消费者确认调用、图片槽、源矩形、翻转、锚点、显示条件、逻辑 tick／绘制计数及随机顺序。本盘点没有运行游戏、没有用词法命中代替这一级证据。动态生成纹理、字体实际字符图集、音频、Shader 和外部加载条件未在此图像盘点中完成语义研究。

七个非 Kairo TextAsset 已登记名称、长度、hash 与 PNG 签名数：`BillingMode`、`applist.txt`、`language_pack_template_ja`、`language_pack_format`、`snd`、`filelist`、`language_pack_template_en`。它们没有 PNG 签名，并非不支持的图片编码；它们的业务语义也不在本轮认定完成。

## 保留的原始诊断

APK 与 EXE 各 16 个 SEB 的部分 keyframe 超过或等于声明 `frame_count`。清单保留 `keyframes_outside_nominal_count`；原读取器的逐层记录语义仍需按消费者研究，不能擅自添加 `frame < frame_count` 校验、修改原表或删记录凑通过。相关文件包括 `down00–03`、`jampGlad00–03`、`buildAnime00–02`、`develWeapon01`、`number10`、`plain00`、`t_casino`、`t_casino00`。

APK `title_logo.png` 在 IEND 后还有两个字节。原始完整 hash 与尾随 hash 单独保留，PNG 解码只读取到 IEND，不裁切或改写原文件。

资源规模与输出消费检查：此次只新增文本清单及标准库分析器，不生成像素／解包缓存、不复制原程序；同字节别名仍保存独立引用。清单统一UTF-8／LF，当前3783898字节，规模按输入容器与记录数增长。它是静态来源索引，不提供运行时引用退休或永久内存有界的证明。
