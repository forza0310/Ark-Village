#include "ark/app/random.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::app;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}

void signed_remainder_and_failure_order() {
    constexpr auto minimum = std::numeric_limits<std::int32_t>::min();
    auto stream = RandomStream::from_raw({-11, 11, minimum, minimum, minimum, -7});
    check(stream.draw(10).ticket == 1, "negative raw remainder is made positive");
    check(stream.draw(-10).ticket == 1, "negative divisor preserves Java helper behavior");
    check(stream.draw(-1).ticket == 0, "MIN/-1 has no C++ undefined behavior");
    check(stream.draw(minimum).ticket == 0, "MIN%MIN is zero");
    check(stream.draw(7).ticket == 2, "remainder precedes absolute value");
    const auto zero = stream.draw(0);
    check(zero.error == RandomError::zero_bound && zero.raw == -7 && zero.ordinal == 5 &&
              stream.draws() == 6 && stream.raw_cursor() == 6,
          "zero divisor consumes nextInt before arithmetic failure");
    const auto exhausted = stream.draw(0);
    check(exhausted.error == RandomError::exhausted && exhausted.ordinal == 6 &&
              stream.draws() == 6,
          "exhaustion precedes zero bound and does not consume raw");
    auto empty = RandomStream::from_raw({});
    check(empty.draw(3).error == RandomError::exhausted && empty.draws() == 0,
          "empty observed tape is not replaced with a seeded fixture");
    auto extreme_bound = RandomStream::from_raw({std::numeric_limits<std::int32_t>::max()});
    check(extreme_bound.draw(minimum).ticket == std::numeric_limits<std::int32_t>::max(),
          "negative minimum bound preserves the largest valid ticket");
}

void known_java_sequence() {
    const std::int32_t expected[]{-1155484576, -723955400,  1033096058, -1690734402, -1557280266,
                                  1327362106,  -1930858313, 502539523,  -1728529858, -938301587};
    auto stream = RandomStream::from_java_seed(0);
    RandomStream default_fixture;
    for (std::size_t i = 0; i < 10; ++i) {
        const auto a = stream.draw(1000);
        const auto b = default_fixture.draw(1000);
        check(a.error == RandomError::none && a.raw == expected[i] && a.ordinal == i,
              "Java seed0 known nextInt sequence and ordinal");
        check(a.raw == b.raw && a.ticket == b.ticket, "default is the explicit seed0 fixture");
        const auto remainder = static_cast<std::int64_t>(expected[i]) % 1000;
        check(a.ticket == (remainder < 0 ? -remainder : remainder),
              "helper consumes one full nextInt rather than rejection sampling");
    }
}

void private_candidate_rollback_and_seed_mask() {
    auto owner = RandomStream::from_java_seed(1234);
    auto candidate = owner;
    candidate.draw(0);
    check(owner.draws() == 0 && candidate.draws() == 1,
          "failed candidate does not advance the owner's engine");
    auto copy = owner;
    for (int i = 0; i < 10000; ++i) {
        const auto a = owner.draw(701);
        const auto b = copy.draw(701);
        check(a.raw == b.raw && a.ticket == b.ticket && a.ordinal == b.ordinal,
              "value copy preserves exact engine state");
    }
    auto lower = RandomStream::from_java_seed(999);
    auto upper = RandomStream::from_java_seed(999 + (1ULL << 48));
    for (int i = 0; i < 1000; ++i)
        check(lower.draw(100).raw == upper.draw(100).raw, "Java seed retains only low48 bits");
    auto tape_owner = RandomStream::from_raw({8, 9});
    auto tape_candidate = tape_owner;
    tape_candidate.draw(4);
    check(tape_owner.draw(4).raw == 8 && tape_candidate.draw(4).raw == 9,
          "observed tape cursor is also owned by value");
    tape_owner = tape_candidate;
    check(tape_owner.draw(4).error == RandomError::exhausted && tape_owner.draws() == 2,
          "committing the candidate also commits its exact exhausted cursor");
}
} // namespace

int main() {
    try {
        signed_remainder_and_failure_order();
        known_java_sequence();
        private_candidate_rollback_and_seed_mask();
        std::cout << "random: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "random: " << e.what() << '\n';
        return 1;
    }
}
