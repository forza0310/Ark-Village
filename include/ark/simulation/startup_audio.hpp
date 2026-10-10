#pragma once

namespace ark::simulation {
// 维护表现输出的操作资格，不是原存档字段。由原调用者显式选择，不能由声音ID推断。
// Owner只持一份待领取队列；旧ID接口显式丢弃operation，仍消费同一份输出。
// 完整快照捕获仍要求输出已消费，不把播放器/播放位置或平台生命周期放入世界存档。
enum class StartupAudioOperation { ordinary_play, replace_bgm, jingle };
struct StartupAudioRequest {
    StartupAudioOperation operation{StartupAudioOperation::ordinary_play};
    int id{};
};
// 同类输出比较保留操作与ID；刻意不提供int隐式构造或跨类型比较。
constexpr bool operator==(const StartupAudioRequest &a, const StartupAudioRequest &b) {
    return a.operation == b.operation && a.id == b.id;
}
constexpr bool operator!=(const StartupAudioRequest &a, const StartupAudioRequest &b) {
    return !(a == b);
}
} // namespace ark::simulation
