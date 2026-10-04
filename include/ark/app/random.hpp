// Adapted from maintained research example/world_random; product runtime is independent.
#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace ark::app {
enum class RandomError { none, exhausted, zero_bound };
struct RandomDraw {
    RandomError error{RandomError::none};
    std::int32_t ticket{}; // Valid only on success; this is not Java nextInt(bound).
    std::int32_t raw{};    // Exhaustion has no raw value; zero_bound has consumed one.
    std::size_t ordinal{}; // Zero-based global raw draw position, including zero_bound.
};

// Source c/d.java:29,75 shares one Random: nextInt(), signed remainder, then abs().
// Copy the stream with a private world candidate; commit it only with that candidate.
// Both engine state and observed-tape cursor roll back when a candidate is discarded.
class RandomStream {
  public:
    // Explicit research seed0 fixture, not the unobserved APK new Random() default seed.
    RandomStream();
    static RandomStream from_raw(std::vector<std::int32_t> raw);
    static RandomStream from_java_seed(std::uint64_t seed);

    // Call lazily at the source consumer, rather than pre-drawing unused branch tickets.
    // Even a zero bound consumes nextInt() before returning its arithmetic failure.
    RandomDraw draw(std::int32_t bound);
    std::size_t draws() const { return cursor_; }
    std::size_t raw_cursor() const { return cursor_; }

  private:
    using JavaEngine =
        std::linear_congruential_engine<std::uint64_t, 0x5DEECE66DULL, 0xBULL, (1ULL << 48)>;
    JavaEngine engine_;
    std::vector<std::int32_t> tape_;
    std::size_t cursor_{};
    bool tape_mode_{};
};
} // namespace ark::app
