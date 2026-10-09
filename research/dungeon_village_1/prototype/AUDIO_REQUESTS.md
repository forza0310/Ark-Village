# 声音操作输出

本模块是唯一Owner的表现输出接口，来源为[原调用合同](../ui/AUDIO_REQUESTS.md)及[生产者审计](../work/audio-producer-audit/README.md)。音频字节在[独立音频发布包](../assets/audio/README.md)，播放设备不属于本模块。

`startup_audio.hpp`定义`StartupAudioRequest{operation,id}`。operation为ordinary_play、replace_bgm或jingle，分别保留原c、b、d调用资格；这些枚举是维护传输类型，不是原游戏存档字段。相同ID可以有不同操作，普通重复请求也不能去重。

`StartupWorldRuntimeSession::take_audio_requests()`和`StartupApplication::take_audio_requests()`一次领取完整原序请求。现有`take_sound_requests()`仍可用，但仅投影ID并消费**同一条**队列；先调用其中一个后，另一个为空。旧接口不适合实现完整播放后端，不提供隐式Request(int)或跨类型比较来掩盖操作丢失。

规则／玩家动作在私有候选中追加输出，唯一Owner成功才提交。新任务遭遇通过现有同步回写的瞬态请求桥，接真实创建结果的BGM2及通知24；通用通知到计数1才产生C11，不能把创建和提示音当同一时点。F与路径P的当前创建载体分别消费，已有遭遇绑定没有新请求；不按ID或整局历史永久去重。

声音队列可由读取者暂缓领取，因此不声称积压天然有界。标准轮末快照要求队列已消费；codec仍完整覆盖历史节点的操作与ID。世界语义3、应用语义5及对应Driver区分旧整数输出轨迹；原预算不变，旧样本只保历史，不迁移。实际验收与未完成项见[本批交付](../work/audio-owner-delivery/README.md)。

本批未接音频设备、标题独立队列、其它输入／绘制声音、Steam完整暂停／失焦与循环策略。调用者不能把这份世界输出接口存在当作整套原音频已还原；人物屏内声音资格、成功提交时点及一次领取都须沿既有消费者，不在绘图重画时再次发出。
