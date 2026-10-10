#include "ark/simulation/world/rules/world_random.hpp"

#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace ark::simulation::rules {
namespace {
constexpr std::uint64_t java_multiplier = 0x5DEECE66DULL;
constexpr std::uint64_t java_mask = (1ULL << 48) - 1;
} // namespace
WorldRandomStream::WorldRandomStream() : engine_(java_multiplier) {}
WorldRandomStream WorldRandomStream::from_raw(std::vector<std::int32_t> raw) {
    WorldRandomStream result;
    result.tape_mode_ = true;
    result.tape_ = std::move(raw);
    return result;
}
WorldRandomStream WorldRandomStream::from_java_seed(std::uint64_t seed) {
    WorldRandomStream result;
    result.engine_.seed((seed ^ java_multiplier) & java_mask);
    return result;
}
WorldRandomSnapshot WorldRandomStream::snapshot() const {
    // 标准引擎流接口导出内部状态；不推进引擎，也不读取其对象表示。
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << engine_;
    std::istringstream input(output.str());
    input.imbue(std::locale::classic());
    std::uint64_t state{};
    if (!(input >> state) || state > java_mask)
        throw std::runtime_error("invalid Java random engine snapshot");
    return {state, tape_, static_cast<std::uint64_t>(cursor_), tape_mode_};
}
std::optional<WorldRandomStream>
WorldRandomStream::from_snapshot(const WorldRandomSnapshot &snapshot) {
    if (snapshot.engine_state > java_mask ||
        snapshot.cursor > std::numeric_limits<std::size_t>::max() ||
        (snapshot.tape_mode && snapshot.cursor > snapshot.tape.size()) ||
        (!snapshot.tape_mode && !snapshot.tape.empty()))
        return {};
    WorldRandomStream result;
    result.engine_.seed(snapshot.engine_state); // c非零；seed(0)也原样恢复48位状态0。
    result.tape_ = snapshot.tape;
    result.cursor_ = static_cast<std::size_t>(snapshot.cursor);
    result.tape_mode_ = snapshot.tape_mode;
    return result;
}
WorldRandomDraw WorldRandomStream::draw(std::int32_t bound) {
    WorldRandomDraw result;
    result.ordinal = cursor_;
    if (tape_mode_) {
        if (cursor_ == tape_.size()) {
            result.error = WorldRandomError::exhausted;
            return result;
        }
        result.raw = tape_[cursor_];
    } else {
        const auto bits = engine_() >> 16;
        // 显式转换top32的补码值，避免uint32→int32越界转换依赖实现。
        const auto signed_value = bits >= (1ULL << 31)
                                      ? static_cast<std::int64_t>(bits) - (1LL << 32)
                                      : static_cast<std::int64_t>(bits);
        result.raw = static_cast<std::int32_t>(signed_value);
    }
    ++cursor_;
    if (bound == 0) {
        result.error = WorldRandomError::zero_bound;
        return result;
    }
    // Java MIN_VALUE%-1为0；提升到int64避免C++有符号除法溢出。
    const auto remainder = static_cast<std::int64_t>(result.raw) % static_cast<std::int64_t>(bound);
    result.ticket = static_cast<std::int32_t>(remainder < 0 ? -remainder : remainder);
    return result;
}
} // namespace ark::simulation::rules
