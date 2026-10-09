# 固定APK音频发布

2026-10-09。26个Ogg保留固定汉化重签APK1.0.8的原始字节，供产品按已核合同选择性复制；运行时不用读取APK或research/work。它们独立于`assets/original`的761项视觉素材，不改变原视觉清单或图像分母。

[MANIFEST.json](MANIFEST.json)按声音ID原序记录APK／snd.inf身份、ZIP条目、原清单名、发布文件、SHA-256、字节、通道、循环资格及Vorbis容器属性。26个声音合计2,659,933字节；ID0…3为BGM通道0并循环，ID4…25为SE通道1且单次。名称从实际snd.inf归一化，未按旧x数组补猜。

**文件已交付不代表播放功能已交付。** [声音合同](../../ui/AUDIO_REQUESTS.md)区分BGM替换／同曲忽略、普通SE重头、jingle压BGM及生命周期；整数ID不能替代这些操作语义。Steam音频文件与完整AudioSource／失焦策略尚未认证，不把APK文件称为Steam同一字节。

从仓库根目录运行独立发布器：

```powershell
python research/dungeon_village_1/tools/scripts/publish_audio.py
python research/dungeon_village_1/tools/scripts/publish_audio.py --check
```

发布器只读固定APK，先验证全部待发布条目与所有已存在目标；同内容保留、异内容明确拒绝，独占创建缺少的文件，不覆盖。`--check`不创建／修改文件，重新验证固定APK、确定性清单及26份声音字节。目录不接受未登记项（本README除外）或符号链接／目录联接；不清理他人的文件。

本轮已完成首次发布、相同内容重复发布（新增0项）、只读check及项目内隔离冲突探针：已有不同MANIFEST被拒绝且字节不变，未写入任何音频，探针已清理。没有播放、PCM解码或设备验证；Vorbis属性只核Ogg页／标识／EOS，容器时长不等于逻辑tick。产品backend、带操作类型请求和成功提交后领取语义仍需独立接线验收。

原声音用于用户授权的内部研究与本机接入，权利仍属于原权利人；沿用[素材说明](../README.md)的来源与用途边界。
