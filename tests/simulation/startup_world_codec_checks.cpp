#include "startup_world_codec_checks.hpp"
#include "../../src/simulation/persistence/startup_world_codec.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace ark::simulation {
namespace {
namespace detail = persistence_detail;
using Bytes = std::vector<std::uint8_t>;
void check(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(std::string("codec: ") + message);
}
template <class F> void rejects(F operation, const char *message) {
    bool rejected = false;
    try {
        operation();
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    check(rejected, message);
}
std::size_t changed_byte(const Bytes &a, const Bytes &b) {
    check(a.size() == b.size(), "定长字段探针改变载荷长度");
    std::size_t offset = a.size(), count = 0;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i] != b[i]) {
            offset = i;
            ++count;
        }
    check(count == 1, "定长字段探针必须只定位一个字节");
    return offset;
}
std::size_t locate_u64(const Bytes &bytes, std::uint64_t value) {
    Bytes token;
    for (unsigned i = 0; i < 8; ++i)
        token.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
    const auto first = std::search(bytes.begin(), bytes.end(), token.begin(), token.end());
    check(first != bytes.end(), "wire标记缺失");
    check(std::search(first + 1, bytes.end(), token.begin(), token.end()) == bytes.end(),
          "wire标记不唯一");
    return static_cast<std::size_t>(first - bytes.begin());
}
void private_snapshots() {
    auto rng = ref::WorldRandomStream::from_java_seed(987654321);
    for (int i = 0; i < 37; ++i)
        rng.draw(17);
    auto restored = ref::WorldRandomStream::from_snapshot(rng.snapshot());
    check(restored.has_value(), "Java随机快照恢复");
    for (int i = 0; i < 50; ++i) {
        const auto a = rng.draw(i == 0 ? 0 : 19), b = restored->draw(i == 0 ? 0 : 19);
        check(a.raw == b.raw && a.ticket == b.ticket && a.error == b.error &&
                  a.ordinal == b.ordinal,
              "Java随机后续序列及零bound");
    }
    auto tape = ref::WorldRandomStream::from_raw({INT32_MIN, 7, -13});
    tape.draw(0);
    restored = ref::WorldRandomStream::from_snapshot(tape.snapshot());
    check(restored.has_value(), "磁带随机快照恢复");
    for (int i = 0; i < 4; ++i) {
        const auto a = tape.draw(11), b = restored->draw(11);
        check(a.raw == b.raw && a.error == b.error && a.ordinal == b.ordinal, "磁带游标及耗尽恢复");
    }
    auto broken = tape.snapshot();
    broken.cursor = broken.tape.size() + 1;
    check(!ref::WorldRandomStream::from_snapshot(broken), "磁带越界拒绝");
    broken = rng.snapshot();
    broken.engine_state = 1ULL << 48;
    check(!ref::WorldRandomStream::from_snapshot(broken), "随机引擎超48位拒绝");
    ref::PeriodAccounting ledger(100, 10);
    const ref::CashEntry income{1, 1, ref::CashCategory::facilities, ref::CashDirection::income,
                                50};
    const ref::ReportInput report{
        1, {{2, 1, ref::CashCategory::facilities, ref::CashDirection::expense, 20}}, {{1, 3, 2}}};
    check(ledger.post_cash(income) == ref::AccountingError::none &&
              ledger.prepare_report(report) == ref::AccountingError::none &&
              ledger.claim_report(1) == ref::AccountingError::none,
          "账本夹具成立");
    auto ledger_copy = ref::PeriodAccounting::from_snapshot(ledger.snapshot());
    check(ledger_copy && ledger_copy->funds() == 130 && ledger_copy->village_points() == 16,
          "账本恢复不重复扣款或领奖");
    check(ledger_copy->prepare_report(report) == ref::AccountingError::none &&
              ledger_copy->claim_report(1) == ref::AccountingError::none &&
              ledger_copy->funds() == 130 && ledger_copy->village_points() == 16,
          "账本恢复后重试幂等");
    auto invalid = ledger.snapshot();
    invalid.reports.at(1).displayed_net++;
    check(!ref::PeriodAccounting::from_snapshot(invalid), "错误报告总额拒绝");
    invalid = ledger.snapshot();
    invalid.entries.erase(2);
    check(!ref::PeriodAccounting::from_snapshot(invalid), "缺已付费用拒绝");
}
} // namespace
void run_startup_world_codec_checks() {
    private_snapshots();
    StartupSession reset;
    StartupWorldRuntimeSession session(reset.state(), ref::WorldRandomStream::from_java_seed(42));
    const auto bytes = detail::encode_state(session.state());
    const auto restored = detail::decode_state(bytes, *session.state().rules);
    check(restored.rules == session.state().rules && detail::encode_state(restored) == bytes,
          "真实新局逐字节往返及规则重绑");
    auto changed = session.state();
    changed.confirm_input = !changed.confirm_input;
    auto invalid = bytes;
    invalid[changed_byte(bytes, detail::encode_state(changed))] = 2;
    rejects([&] { detail::decode_state(invalid, *restored.rules); }, "非法bool拒绝");
    changed = session.state();
    changed.dungeon_labels.push_back({ref::DungeonCrewRequestKind::retreat, {}, 0, 0});
    auto enum_bytes = detail::encode_state(changed);
    changed.dungeon_labels.back().kind = ref::DungeonCrewRequestKind::progress_label231;
    enum_bytes[changed_byte(enum_bytes, detail::encode_state(changed))] = 255;
    rejects([&] { detail::decode_state(enum_bytes, *restored.rules); }, "未知枚举拒绝");
    changed = session.state();
    changed.camera[0] = std::numeric_limits<float>::infinity();
    rejects([&] { detail::encode_state(changed); }, "非有限浮点编码拒绝");
    changed.camera[0] = 123.25F;
    auto float_bytes = detail::encode_state(changed);
    changed.camera[0] = -123.25F;
    const auto float_sign = changed_byte(float_bytes, detail::encode_state(changed));
    check(float_sign >= 3, "浮点位模式存在");
    float_bytes[float_sign - 3] = 0;
    float_bytes[float_sign - 2] = 0;
    float_bytes[float_sign - 1] = 128;
    float_bytes[float_sign] = 127;
    rejects([&] { detail::decode_state(float_bytes, *restored.rules); }, "非有限浮点解码拒绝");
    changed.camera[0] = -0.0F;
    const auto signed_zero = detail::encode_state(changed);
    check(detail::encode_state(detail::decode_state(signed_zero, *restored.rules)) == signed_zero,
          "负零IEEE位模式保持");
    // 类型夹具只用于格式边界，不作为真实游戏路径或跨域合法状态。
    constexpr std::uint64_t marker1 = 0x3141592653589793ULL, marker2 = 0x3141592653589794ULL;
    changed = session.state();
    changed.human_pages_initialized = {marker1, marker2};
    invalid = detail::encode_state(changed);
    const auto set_first = locate_u64(invalid, marker1), set_second = locate_u64(invalid, marker2);
    std::copy_n(invalid.begin() + set_first, 8, invalid.begin() + set_second);
    rejects([&] { detail::decode_state(invalid, *restored.rules); }, "重复set元素拒绝");
    changed = session.state();
    changed.facility_original_ids.clear();
    changed.facility_original_ids.emplace(marker1, 1);
    changed.facility_original_ids.emplace(marker2, 2);
    invalid = detail::encode_state(changed);
    const auto map_first = locate_u64(invalid, marker1), map_second = locate_u64(invalid, marker2);
    std::copy_n(invalid.begin() + map_first, 8, invalid.begin() + map_second);
    rejects([&] { detail::decode_state(invalid, *restored.rules); }, "重复map键拒绝");
    changed = session.state();
    changed.shop_order = {marker1};
    invalid = detail::encode_state(changed);
    const auto element = locate_u64(invalid, marker1);
    check(element >= 8, "容器计数存在");
    std::fill_n(invalid.begin() + element - 8, 8, 255);
    rejects([&] { detail::decode_state(invalid, *restored.rules); }, "超大容器计数分配前拒绝");
    for (const auto cut :
         {std::size_t{0}, std::size_t{1}, std::size_t{7}, bytes.size() / 2, bytes.size() - 1}) {
        const Bytes truncated(bytes.begin(), bytes.begin() + cut);
        rejects([&] { detail::decode_state(truncated, *restored.rules); }, "截断载荷拒绝");
    }
    invalid = bytes;
    invalid.push_back(0);
    rejects([&] { detail::decode_state(invalid, *restored.rules); }, "尾随垃圾拒绝");
    detail::CodecDecodeBudget low_nodes{0, 128U * 1024U * 1024U};
    rejects([&] { detail::decode_state(bytes, *restored.rules, low_nodes); }, "节点预算拒绝");
    detail::CodecDecodeBudget low_memory{1000000, 1};
    rejects([&] { detail::decode_state(bytes, *restored.rules, low_memory); }, "分配预算拒绝");
    detail::CodecDecodeBudget used;
    const auto before = used;
    detail::decode_state(bytes, *restored.rules, used);
    const auto nodes = before.nodes_remaining - used.nodes_remaining;
    const auto allocation = before.allocation_bytes_remaining - used.allocation_bytes_remaining;
    std::cout << "codec startup Owner: wire_bytes=" << bytes.size() << " nodes=" << nodes
              << " allocation_budget_bytes=" << allocation << '\n';
    detail::CodecDecodeBudget shared{nodes * 2 - 1, allocation * 3};
    detail::decode_state(bytes, *restored.rules, shared);
    rejects([&] { detail::decode_state(bytes, *restored.rules, shared); },
            "多个Owner累计预算不重置");
    check(detail::encode_state(session.state()) == bytes, "失败解码不改当前Owner");
}
} // namespace ark::simulation
