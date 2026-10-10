# 音频请求与原文件身份

2026-10-09。正式结论：[AUDIO_REQUESTS](../../ui/AUDIO_REQUESTS.md)。原静态批次登记身份与调用语义；后继按用户产品需求已完成[独立音频发布](../../assets/audio/README.md)，维护音频接口仍未交付。下面EVIDENCE.json保留原静态批次资格与检查结果，不将其“未发布”字段解释为后继素材现状。

`audit.py`直接验证固定APK SHA-256，从ZIP只读26条snd.inf与26个Ogg，逐字节比较既有研究副本。独立读取Ogg页边界、单逻辑流、序号、Vorbis identification及结束granule；没有播放、解码PCM或检验Ogg CRC，时长不当作游戏逻辑计数。实际原文件合计2,659,933字节，没有复制音频或新增资源缓存。

Java窗口记录源文件hash及行范围，加载C→sound目录→snd.inf→raw扩展名转换／MediaPlayer→通道和loop配置分别定位。旧x文件名数组不是26槽实际索引。JADX扩展名匹配循环结构异常明确留作DEX待核，不把输出片段直接翻译为维护实现。

`steam-inspect.cjs`限定7个具名方法及真实下一入口，校验固定DLL／metadata和PE范围，单窗≤4096字节，指令只输出stdout。七窗共1,168字节，已消费PlayBgm／PlaySe／PlayJingle／StopBgm及SoundPlayer.Play／Suspend／Resume；EXE音频加载、字节身份、AudioSource和焦点消费尚未闭合。

核对命令属于本地研究过程，不随仓库交付。

首条仅更新本目录EVIDENCE.json（本地核对材料，不随仓库交付）：26个完整文件hash、实际raw路径、通道、循环资格、容器头、21个Java窗口及7个Steam窗口。正式合同逐项消费；没有保存新反编译全文／机器码全文。自动校验3609项仅是输入与结构检查，不能代替原窗口、真实听音、C++播放或回放语义验收。

后继发布由[小发布器](../../tools/scripts/publish_audio.py)从固定APK直接读取，26份Ogg及清单位于assets/audio；不修改original视觉包／761分母。同内容重复发布新增0项，`--check`通过；项目内隔离冲突探针确认异内容MANIFEST拒绝、原字节不变且新音频0项，探针目录已清理。发布清单SHA-256为`7a4661f9603f0edd39856286091bd49feacfbc0b1bd5be1b9a8d5714d8a43ebd`。

本轮没有构建、播放、窗口输入、内存／存档读写或后台任务；未修改Owner、schema或产品。产品要求的音频文件已交付，保留操作类别的维护消费者及backend仍待后续；当前一次性整数领取不应宣称整个音频功能可用。
