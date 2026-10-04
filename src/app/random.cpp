#include "ark/app/random.hpp"

#include <utility>

namespace ark::app {
namespace {
constexpr std::uint64_t java_multiplier = 0x5DEECE66DULL;
constexpr std::uint64_t java_mask = (1ULL << 48) - 1;
} // namespace

RandomStream::RandomStream() : engine_(java_multiplier) {}

RandomStream RandomStream::from_raw(std::vector<std::int32_t> raw) {
    RandomStream result;
    result.tape_mode_ = true;
    result.tape_ = std::move(raw);
    return result;
}

RandomStream RandomStream::from_java_seed(std::uint64_t seed) {
    RandomStream result;
    result.engine_.seed((seed ^ java_multiplier) & java_mask);
    return result;
}

RandomDraw RandomStream::draw(std::int32_t bound) {
    RandomDraw result;
    result.ordinal = cursor_;
    if (tape_mode_) {
        if (cursor_ == tape_.size()) {
            result.error = RandomError::exhausted;
            return result;
        }
        result.raw = tape_[cursor_];
    } else {
        const auto bits = engine_() >> 16;
        // Decode top32 as two's complement without an implementation-defined narrowing.
        const auto signed_value = bits >= (1ULL << 31)
                                      ? static_cast<std::int64_t>(bits) - (1LL << 32)
                                      : static_cast<std::int64_t>(bits);
        result.raw = static_cast<std::int32_t>(signed_value);
    }
    ++cursor_;
    if (bound == 0) {
        result.error = RandomError::zero_bound;
        return result;
    }
    // Java MIN_VALUE % -1 is zero; widened operands avoid C++ division overflow.
    const auto remainder = static_cast<std::int64_t>(result.raw) % static_cast<std::int64_t>(bound);
    result.ticket = static_cast<std::int32_t>(remainder < 0 ? -remainder : remainder);
    return result;
}
} // namespace ark::app
