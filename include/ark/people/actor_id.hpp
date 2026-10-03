#pragma once
// Nonzero stable AI snapshot identity, distinct from a raw original UID (where zero is valid).
// The aggregate owns mapping and lifetime; this type never allocates or guesses identities.
#include <cstdint>
namespace ark::people {
struct ActorId {
    std::uint64_t value{};
};
inline bool operator==(ActorId a, ActorId b) { return a.value == b.value; }
inline bool operator<(ActorId a, ActorId b) { return a.value < b.value; }
} // namespace ark::people
