#include "ark/simulation/rules/world_random.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
void raw_rules() {
    constexpr auto minimum = std::numeric_limits<std::int32_t>::min();
    auto stream = WorldRandomStream::from_raw({-11, 11, minimum, minimum, minimum, -7});
    check(stream.draw(10).ticket == 1, "negative raw remainder is made positive");
    check(stream.draw(-10).ticket == 1, "negative divisor preserves Java helper behavior");
    check(stream.draw(-1).ticket == 0, "MIN/-1 has no C++ undefined behavior");
    check(stream.draw(minimum).ticket == 0, "MIN%MIN is zero");
    check(stream.draw(7).ticket == 2, "Java remainder before absolute value, not abs raw");
    const auto zero = stream.draw(0);
    check(zero.error == WorldRandomError::zero_bound && zero.raw == -7 && zero.ordinal == 5 &&
              stream.draws() == 6 && stream.raw_cursor() == 6,
          "zero divisor consumes nextInt before arithmetic error");
    const auto exhausted = stream.draw(0);
    check(exhausted.error == WorldRandomError::exhausted && exhausted.ordinal == 6 &&
              stream.draws() == 6,
          "exhaustion does not invent or consume raw even for zero bound");
    auto empty = WorldRandomStream::from_raw({});
    check(empty.draw(3).error == WorldRandomError::exhausted && empty.draws() == 0,
          "empty observed tape is not replaced with seeded fixture");
}
void known_java_sequence() {
    const std::int32_t expected[]{-1155484576, -723955400,  1033096058, -1690734402, -1557280266,
                                  1327362106,  -1930858313, 502539523,  -1728529858, -938301587};
    auto stream = WorldRandomStream::from_java_seed(0);
    WorldRandomStream default_fixture;
    for (std::size_t i = 0; i < 10; ++i) {
        const auto a = stream.draw(1000);
        const auto b = default_fixture.draw(1000);
        check(a.error == WorldRandomError::none && a.raw == expected[i] && a.ordinal == i,
              "Java seed0 known nextInt sequence and ordinal");
        check(a.raw == b.raw && a.ticket == b.ticket, "default explicitly equals research seed0");
        const auto remainder = static_cast<std::int64_t>(expected[i]) % 1000;
        check(a.ticket == (remainder < 0 ? -remainder : remainder),
              "bounded helper consumes one full nextInt, not rejection sampling");
    }
}
void copies_and_seed_mask() {
    auto owner = WorldRandomStream::from_java_seed(1234);
    auto candidate = owner;
    candidate.draw(0);
    check(owner.draws() == 0 && candidate.draws() == 1,
          "failed candidate does not advance owner's random state");
    auto copy = owner;
    for (int i = 0; i < 10000; ++i) {
        const auto a = owner.draw(701);
        const auto b = copy.draw(701);
        check(a.raw == b.raw && a.ticket == b.ticket && a.ordinal == b.ordinal,
              "value copy preserves exact engine state");
    }
    auto lower = WorldRandomStream::from_java_seed(999);
    auto upper = WorldRandomStream::from_java_seed(999 + (1ULL << 48));
    for (int i = 0; i < 1000; ++i)
        check(lower.draw(100).raw == upper.draw(100).raw, "Java constructor keeps low48 seed bits");
    auto raw_owner = WorldRandomStream::from_raw({8, 9});
    auto raw_candidate = raw_owner;
    raw_candidate.draw(4);
    check(raw_owner.draw(4).raw == 8 && raw_candidate.draw(4).raw == 9,
          "raw tape cursor is also owned by value");
}
} // namespace
int main() {
    try {
        raw_rules();
        known_java_sequence();
        copies_and_seed_mask();
        std::cout << "world_random: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "world_random: " << e.what() << '\n';
        return 1;
    }
}
