#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace dungeon_village_reference {
enum class WorldRandomError { none, exhausted, zero_bound };
struct WorldRandomDraw {
    WorldRandomError error{WorldRandomError::none};
    std::int32_t ticket{}; // 仅none时有效；不是Java Random.nextInt(bound)。
    std::int32_t raw{};    // exhausted时无原始值，其余分支已消费nextInt。
    std::size_t ordinal{}; // 从零起的全局原始抽取位置，包括zero_bound。
};

// c/d.java:29,75：一个static Random，nextInt之后Java有符号余数再abs。
// 值拷贝包含引擎/tape游标，世界候选回滚不会推进真实所有者的随机流。
class WorldRandomStream {
  public:
    // 仅显式研究seed0；APK new Random()的默认种子未观测，不能据此复演新局。
    WorldRandomStream();
    static WorldRandomStream from_raw(std::vector<std::int32_t> raw);
    static WorldRandomStream from_java_seed(std::uint64_t seed);
    WorldRandomDraw draw(std::int32_t bound);
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
} // namespace dungeon_village_reference
