#include "ark/app/world_save.hpp"
#include "world_save_fields.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace ark::app {
namespace save_detail {
static_assert(sizeof(int) == 4, "Save schema requires 32-bit world integers");
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
              "Save schema requires IEEE-754 binary32");
constexpr std::uint32_t schema_version = 2;
constexpr std::size_t max_entries = 1000000;
constexpr char magic[] = "ARKSAVE1";
struct Failure {
    WorldSaveError error;
    const char *message;
};
[[noreturn]] void fail(WorldSaveError error, const char *message) { throw Failure{error, message}; }

template <class T> struct Vector : std::false_type {};
template <class T, class A> struct Vector<std::vector<T, A>> : std::true_type {};
template <class T> struct Map : std::false_type {};
template <class K, class V, class C, class A> struct Map<std::map<K, V, C, A>> : std::true_type {};
template <class T> struct Set : std::false_type {};
template <class T, class C, class A> struct Set<std::set<T, C, A>> : std::true_type {};
template <class T> struct Optional : std::false_type {};
template <class T> struct Optional<std::optional<T>> : std::true_type {};
template <class T> struct Array : std::false_type {};
template <class T, std::size_t N> struct Array<std::array<T, N>> : std::true_type {};
template <class T> struct Pair : std::false_type {};
template <class A, class B> struct Pair<std::pair<A, B>> : std::true_type {};

class Encoder {
  public:
    static constexpr bool reading = false;
    std::vector<std::uint8_t> bytes;
    template <class... T> void operator()(T &...values) { (value(values), ...); }
    void raw(const void *source, std::size_t count) {
        if (count > world_save_max_bytes - bytes.size())
            fail(WorldSaveError::too_large, "Save exceeds byte budget");
        if (!count)
            return;
        const auto *begin = static_cast<const std::uint8_t *>(source);
        bytes.insert(bytes.end(), begin, begin + count);
    }
    template <class T> void integer(T v) {
        static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>);
        using U = std::make_unsigned_t<T>;
        U bits{};
        std::memcpy(&bits, &v, sizeof(bits));
        for (std::size_t i = 0; i < sizeof(bits); ++i) {
            const auto byte = static_cast<std::uint8_t>((bits >> (i * 8)) & U{255});
            raw(&byte, 1);
        }
    }
    void count(std::size_t n) {
        if (n > max_entries || entries_ > max_entries - n)
            fail(WorldSaveError::too_large, "Save exceeds entry budget");
        entries_ += n;
        integer(static_cast<std::uint32_t>(n));
    }
    template <class T> void value(T &v) {
        using V = std::remove_cv_t<T>;
        if constexpr (std::is_same_v<V, bool>) {
            integer(static_cast<std::uint8_t>(v ? 1 : 0));
        } else if constexpr (std::is_enum_v<V>) {
            integer(static_cast<std::int32_t>(v));
        } else if constexpr (std::is_integral_v<V>) {
            integer(v);
        } else if constexpr (std::is_same_v<V, float>) {
            static_assert(sizeof(float) == sizeof(std::uint32_t));
            if (!std::isfinite(v))
                fail(WorldSaveError::invalid_world, "Non-finite world coordinate");
            std::uint32_t bits{};
            std::memcpy(&bits, &v, sizeof(bits));
            integer(bits);
        } else if constexpr (std::is_same_v<V, std::string>) {
            count(v.size());
            raw(v.data(), v.size());
        } else if constexpr (Vector<V>::value) {
            count(v.size());
            for (std::size_t i = 0; i < v.size(); ++i) {
                if constexpr (std::is_same_v<typename V::value_type, bool>) {
                    bool entry = v[i];
                    value(entry);
                } else {
                    value(v[i]);
                }
            }
        } else if constexpr (Map<V>::value) {
            count(v.size());
            for (auto &entry : v) {
                auto key = entry.first;
                value(key);
                value(entry.second);
            }
        } else if constexpr (Set<V>::value) {
            count(v.size());
            for (auto entry : v)
                value(entry);
        } else if constexpr (Optional<V>::value) {
            bool exists = v.has_value();
            value(exists);
            if (exists)
                value(*v);
        } else if constexpr (Array<V>::value) {
            for (auto &entry : v)
                value(entry);
        } else if constexpr (Pair<V>::value) {
            value(v.first);
            value(v.second);
        } else {
            fields(*this, v);
        }
    }

  private:
    std::size_t entries_{};
};

class Decoder {
  public:
    static constexpr bool reading = true;
    Decoder(const std::vector<std::uint8_t> &bytes, std::size_t end) : bytes_(bytes), end_(end) {}
    template <class... T> void operator()(T &...values) { (value(values), ...); }
    void raw(void *destination, std::size_t count) {
        if (count > end_ - cursor_)
            fail(WorldSaveError::malformed, "Truncated save");
        if (count)
            std::memcpy(destination, bytes_.data() + cursor_, count);
        cursor_ += count;
    }
    template <class T> T integer() {
        static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>);
        using U = std::make_unsigned_t<T>;
        U bits{};
        for (std::size_t i = 0; i < sizeof(U); ++i) {
            std::uint8_t byte{};
            raw(&byte, 1);
            bits |= static_cast<U>(static_cast<U>(byte) << (i * 8));
        }
        T result{};
        std::memcpy(&result, &bits, sizeof(result));
        return result;
    }
    std::size_t count(std::size_t element_size) {
        const auto n = integer<std::uint32_t>();
        if (n > max_entries || entries_ > max_entries - n || n > end_ - cursor_ ||
            (element_size && n > (world_save_max_bytes - allocations_) / element_size))
            fail(WorldSaveError::too_large, "Save exceeds allocation or entry budget");
        entries_ += n;
        allocations_ += static_cast<std::size_t>(n) * element_size;
        return n;
    }
    std::size_t cursor() const { return cursor_; }
    std::size_t remaining() const { return end_ - cursor_; }
    template <class T> void value(T &v) {
        using V = std::remove_cv_t<T>;
        if constexpr (std::is_same_v<V, bool>) {
            const auto byte = integer<std::uint8_t>();
            if (byte > 1)
                fail(WorldSaveError::malformed, "Invalid boolean");
            v = byte != 0;
        } else if constexpr (std::is_enum_v<V>) {
            v = static_cast<V>(integer<std::int32_t>());
        } else if constexpr (std::is_integral_v<V>) {
            v = integer<V>();
        } else if constexpr (std::is_same_v<V, float>) {
            const auto bits = integer<std::uint32_t>();
            std::memcpy(&v, &bits, sizeof(bits));
            if (!std::isfinite(v))
                fail(WorldSaveError::malformed, "Non-finite coordinate");
        } else if constexpr (std::is_same_v<V, std::string>) {
            const auto n = count(1);
            v.resize(n);
            raw(v.data(), n);
        } else if constexpr (Vector<V>::value) {
            const auto n = count(sizeof(typename V::value_type));
            v.clear();
            v.reserve(n);
            for (std::size_t i = 0; i < n; ++i) {
                typename V::value_type entry{};
                value(entry);
                v.push_back(std::move(entry));
            }
        } else if constexpr (Map<V>::value) {
            const auto n = count(sizeof(typename V::value_type) + 4 * sizeof(void *));
            v.clear();
            for (std::size_t i = 0; i < n; ++i) {
                typename V::key_type key{};
                typename V::mapped_type entry{};
                value(key);
                value(entry);
                if (!v.emplace(std::move(key), std::move(entry)).second)
                    fail(WorldSaveError::malformed, "Duplicate map identity");
            }
        } else if constexpr (Set<V>::value) {
            const auto n = count(sizeof(typename V::value_type) + 4 * sizeof(void *));
            v.clear();
            for (std::size_t i = 0; i < n; ++i) {
                typename V::value_type entry{};
                value(entry);
                if (!v.insert(std::move(entry)).second)
                    fail(WorldSaveError::malformed, "Duplicate set identity");
            }
        } else if constexpr (Optional<V>::value) {
            bool exists{};
            value(exists);
            v.reset();
            if (exists) {
                v.emplace();
                value(*v);
            }
        } else if constexpr (Array<V>::value) {
            for (auto &entry : v)
                value(entry);
        } else if constexpr (Pair<V>::value) {
            value(v.first);
            value(v.second);
        } else {
            fields(*this, v);
        }
    }

  private:
    const std::vector<std::uint8_t> &bytes_;
    std::size_t end_{};
    std::size_t cursor_{};
    std::size_t entries_{};
    std::size_t allocations_{};
};

std::uint64_t checksum(const std::vector<std::uint8_t> &bytes, std::size_t end) {
    std::uint64_t result = 14695981039346656037ULL;
    for (std::size_t i = 0; i < end; ++i) {
        result ^= bytes[i];
        result *= 1099511628211ULL;
    }
    return result;
}
WorldSaveMetadata metadata(const simulation::StartupWorldRuntimeState &state) {
    const auto &calendar = state.scene.calendar;
    return {
        state.scripts.village_name, calendar.year,  calendar.month,
        calendar.subperiod,         calendar.units, state.scene.world.world.ai.accounting.funds()};
}
template <class Archive> void fields(Archive &io, WorldSaveMetadata &x) {
    io(x.village, x.year, x.month, x.week, x.units, x.funds);
}
} // namespace save_detail

const char *world_save_dataset() {
    return "f34787eabc2e6e1556fe971b57c4d6adbf2f35d79b2aa9c81e35cddbf8ec6b04";
}

WorldSaveCapture capture_world_save(const simulation::StartupWorldRuntimeState &state) {
    std::string reason;
    if (!world_save_eligible(state, &reason))
        return {WorldSaveError::ineligible, std::move(reason), {}};
    try {
        const auto validation = validate_world_save_candidate(state, reason);
        if (validation != WorldSaveError::none)
            return {validation, std::move(reason), {}};
        auto copy = state;
        copy.save_marker = 1;
        save_detail::Encoder payload;
        payload(copy);
        auto summary = save_detail::metadata(state);
        save_detail::Encoder file;
        file.raw(save_detail::magic, 8);
        auto version = save_detail::schema_version;
        std::string dataset = world_save_dataset();
        auto length = static_cast<std::uint32_t>(payload.bytes.size());
        file(version, dataset, summary, length);
        file.raw(payload.bytes.data(), payload.bytes.size());
        auto check = save_detail::checksum(file.bytes, file.bytes.size());
        file(check);
        return {
            WorldSaveError::none, {}, WorldSaveImage{std::move(summary), std::move(file.bytes)}};
    } catch (const save_detail::Failure &failure) {
        return {failure.error, failure.message, {}};
    } catch (const std::bad_alloc &) {
        return {WorldSaveError::too_large, "Save allocation failed", {}};
    } catch (const std::exception &failure) {
        return {WorldSaveError::invalid_world, failure.what(), {}};
    }
}

WorldSaveCandidate decode_world_save(const std::vector<std::uint8_t> &bytes) {
    if (bytes.size() > world_save_max_bytes)
        return {WorldSaveError::too_large, "Save exceeds byte budget", {}, {}};
    if (bytes.size() < 24)
        return {WorldSaveError::malformed, "Truncated save header", {}, {}};
    try {
        std::uint64_t recorded{};
        for (std::size_t i = 0; i < 8; ++i)
            recorded |= static_cast<std::uint64_t>(bytes[bytes.size() - 8 + i]) << (8 * i);
        if (recorded != save_detail::checksum(bytes, bytes.size() - 8))
            save_detail::fail(WorldSaveError::malformed, "Save checksum mismatch");
        save_detail::Decoder file(bytes, bytes.size() - 8);
        char marker[8]{};
        file.raw(marker, sizeof(marker));
        if (!std::equal(std::begin(marker), std::end(marker), save_detail::magic))
            save_detail::fail(WorldSaveError::malformed, "Invalid save signature");
        std::uint32_t version{};
        file(version);
        if (version != save_detail::schema_version)
            save_detail::fail(WorldSaveError::unsupported_version, "Unsupported save version");
        std::string dataset;
        file(dataset);
        if (dataset != world_save_dataset())
            save_detail::fail(WorldSaveError::dataset_mismatch, "Save dataset mismatch");
        WorldSaveMetadata summary;
        std::uint32_t length{};
        file(summary, length);
        if (length != file.remaining())
            save_detail::fail(WorldSaveError::malformed, "Save payload length mismatch");
        simulation::StartupWorldRuntimeState candidate;
        candidate.rules = &simulation::startup_world_rules();
        file(candidate);
        if (file.remaining())
            save_detail::fail(WorldSaveError::malformed, "Trailing save payload");
        const auto actual = save_detail::metadata(candidate);
        if (summary.village != actual.village || summary.year != actual.year ||
            summary.month != actual.month || summary.week != actual.week ||
            summary.units != actual.units || summary.funds != actual.funds)
            save_detail::fail(WorldSaveError::malformed, "Save summary differs from world");
        std::string reason;
        const auto result = validate_world_save_candidate(candidate, reason);
        if (result != WorldSaveError::none)
            return {result, std::move(reason), {}, {}};
        return {WorldSaveError::none, {}, std::move(candidate), std::move(summary)};
    } catch (const save_detail::Failure &failure) {
        return {failure.error, failure.message, {}, {}};
    } catch (const std::bad_alloc &) {
        return {WorldSaveError::too_large, "Save allocation failed", {}, {}};
    } catch (const std::exception &failure) {
        return {WorldSaveError::malformed, failure.what(), {}, {}};
    }
}
} // namespace ark::app
