#include "dungeon_village_reference/world_random.hpp"

#include <utility>

namespace dungeon_village_reference {
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
} // namespace dungeon_village_reference
