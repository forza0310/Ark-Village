#include "startup_world_codec.hpp"
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace dungeon_village_prototype::persistence_detail {
namespace {
constexpr std::size_t owner_budget = 64U * 1024U * 1024U;
void need(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
template <class T, class Enable = void> struct Codec;
template <class T> struct CodecFields;
template <class T> bool valid_enum(T) = delete;
struct Writer {
    std::vector<std::uint8_t> bytes;
    void raw(const void *data, std::size_t n) {
        need(n <= owner_budget - bytes.size(), "Owner编码超出64MiB预算");
        if (!n)
            return;
        const auto *p = static_cast<const std::uint8_t *>(data);
        bytes.insert(bytes.end(), p, p + n);
    }
    void integer(std::uint64_t n, unsigned width = 8) {
        for (unsigned i = 0; i < width; ++i) {
            const auto b = static_cast<std::uint8_t>(n >> (i * 8));
            raw(&b, 1);
        }
    }
    template <class T> void field(const char *, const T &value) { Codec<T>::write(*this, value); }
};
struct Reader {
    const std::vector<std::uint8_t> &bytes;
    CodecDecodeBudget &budget;
    std::size_t offset{};
    unsigned depth{};
    std::size_t remaining() const { return bytes.size() - offset; }
    std::uint64_t integer(unsigned width = 8) {
        need(width <= remaining(), "Owner字段截断");
        std::uint64_t value{};
        for (unsigned i = 0; i < width; ++i)
            value |= std::uint64_t(bytes[offset++]) << (i * 8);
        return value;
    }
    // min_wire为每项最小编码宽度，allocation为容器实际元素及保守节点开销。
    std::size_t count(std::size_t min_wire, std::size_t allocation) {
        const auto n = integer();
        need(min_wire > 0 && n <= remaining() / min_wire, "Owner容器计数超过载荷");
        need(n <= budget.nodes_remaining, "Owner累计节点预算耗尽");
        need(allocation > 0 && n <= budget.allocation_bytes_remaining / allocation,
             "Owner累计分配预算耗尽");
        budget.nodes_remaining -= static_cast<std::size_t>(n);
        budget.allocation_bytes_remaining -= static_cast<std::size_t>(n) * allocation;
        return static_cast<std::size_t>(n);
    }
    template <class T> void field(const char *name, T &value) {
        need(depth < 64, "Owner嵌套深度超预算");
        ++depth;
        try {
            Codec<T>::read(*this, value);
        } catch (const std::runtime_error &e) {
            --depth;
            throw std::runtime_error(std::string(name) + ": " + e.what());
        }
        --depth;
    }
};
template <class T, class Enable> struct Codec {
    static void write(Writer &w, const T &v) { CodecFields<T>::visit(w, v); }
    static void read(Reader &r, T &v) { CodecFields<T>::visit(r, v); }
    static std::size_t minimum() { return CodecFields<T>::minimum(); }
};
template <class T>
struct Codec<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
    static void write(Writer &w, T v) { w.integer(static_cast<std::uint64_t>(v)); }
    static void read(Reader &r, T &v) {
        const auto bits = r.integer();
        if constexpr (std::is_signed_v<T>) {
            // 用显式二补数解释，避免无符号大值转有符号的实现定义行为。
            const auto value = bits <= std::uint64_t(INT64_MAX)
                                   ? static_cast<std::int64_t>(bits)
                                   : -1 - static_cast<std::int64_t>(~bits);
            need(value >= std::numeric_limits<T>::min() && value <= std::numeric_limits<T>::max(),
                 "有符号整数字段越界");
            v = static_cast<T>(value);
        } else {
            need(bits <= std::numeric_limits<T>::max(), "无符号整数字段越界");
            v = static_cast<T>(bits);
        }
    }
    static std::size_t minimum() { return 8; }
};
template <> struct Codec<bool> {
    static void write(Writer &w, bool v) { w.integer(v ? 1 : 0, 1); }
    static void read(Reader &r, bool &v) {
        auto n = r.integer(1);
        need(n <= 1, "非法布尔值");
        v = n != 0;
    }
    static std::size_t minimum() { return 1; }
};
template <class T> struct Codec<T, std::enable_if_t<std::is_enum_v<T>>> {
    using Base = std::underlying_type_t<T>;
    static void write(Writer &w, T v) {
        need(valid_enum(v), "未知枚举值");
        Codec<Base>::write(w, static_cast<Base>(v));
    }
    static void read(Reader &r, T &v) {
        Base n{};
        Codec<Base>::read(r, n);
        v = static_cast<T>(n);
        need(valid_enum(v), "未知枚举值");
    }
    static std::size_t minimum() { return 8; }
};
template <class T> struct Codec<T, std::enable_if_t<std::is_floating_point_v<T>>> {
    using Bits = std::conditional_t<sizeof(T) == 4, std::uint32_t, std::uint64_t>;
    static_assert(std::numeric_limits<T>::is_iec559 && (sizeof(T) == 4 || sizeof(T) == 8));
    static void write(Writer &w, T v) {
        need(std::isfinite(v), "Owner浮点字段不是有限值");
        Bits b{};
        std::memcpy(&b, &v, sizeof(v));
        w.integer(b, sizeof(T));
    }
    static void read(Reader &r, T &v) {
        const auto b = static_cast<Bits>(r.integer(sizeof(T)));
        std::memcpy(&v, &b, sizeof(v));
        need(std::isfinite(v), "Owner浮点字段不是有限值");
    }
    static std::size_t minimum() { return sizeof(T); }
};
template <> struct Codec<std::string> {
    static void write(Writer &w, const std::string &v) {
        w.integer(v.size());
        w.raw(v.data(), v.size());
    }
    static void read(Reader &r, std::string &v) {
        const auto n = r.count(1, 1);
        v.assign(reinterpret_cast<const char *>(r.bytes.data() + r.offset), n);
        r.offset += n;
    }
    static std::size_t minimum() { return 8; }
};
template <class T> struct Codec<std::vector<T>> {
    static void write(Writer &w, const std::vector<T> &v) {
        w.integer(v.size());
        for (const auto &x : v)
            Codec<T>::write(w, x);
    }
    static void read(Reader &r, std::vector<T> &v) {
        const auto n = r.count(Codec<T>::minimum(), sizeof(T));
        v.clear();
        v.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            T x{};
            r.field("vector[]", x);
            v.push_back(std::move(x));
        }
    }
    static std::size_t minimum() { return 8; }
};
template <class T, std::size_t N> struct Codec<std::array<T, N>> {
    static void write(Writer &w, const std::array<T, N> &v) {
        for (const auto &x : v)
            Codec<T>::write(w, x);
    }
    static void read(Reader &r, std::array<T, N> &v) {
        for (auto &x : v)
            r.field("array[]", x);
    }
    static std::size_t minimum() { return N * Codec<T>::minimum(); }
};
template <class T> struct Codec<std::optional<T>> {
    static void write(Writer &w, const std::optional<T> &v) {
        Codec<bool>::write(w, v.has_value());
        if (v)
            Codec<T>::write(w, *v);
    }
    static void read(Reader &r, std::optional<T> &v) {
        bool present{};
        Codec<bool>::read(r, present);
        if (present) {
            v.emplace();
            r.field("optional", *v);
        } else
            v.reset();
    }
    static std::size_t minimum() { return 1; }
};
template <class A, class B> struct Codec<std::pair<A, B>> {
    static void write(Writer &w, const std::pair<A, B> &v) {
        Codec<A>::write(w, v.first);
        Codec<B>::write(w, v.second);
    }
    static void read(Reader &r, std::pair<A, B> &v) {
        r.field("pair.first", v.first);
        r.field("pair.second", v.second);
    }
    static std::size_t minimum() { return Codec<A>::minimum() + Codec<B>::minimum(); }
};
template <class K, class V> struct Codec<std::map<K, V>> {
    static void write(Writer &w, const std::map<K, V> &v) {
        w.integer(v.size());
        for (const auto &x : v) {
            Codec<K>::write(w, x.first);
            Codec<V>::write(w, x.second);
        }
    }
    static void read(Reader &r, std::map<K, V> &v) {
        const auto n = r.count(Codec<K>::minimum() + Codec<V>::minimum(),
                               sizeof(std::pair<const K, V>) + 4 * sizeof(void *));
        v.clear();
        for (std::size_t i = 0; i < n; ++i) {
            K k{};
            V x{};
            r.field("map.key", k);
            r.field("map.value", x);
            need(v.emplace(std::move(k), std::move(x)).second, "重复map键");
        }
    }
    static std::size_t minimum() { return 8; }
};
template <class T> struct Codec<std::set<T>> {
    static void write(Writer &w, const std::set<T> &v) {
        w.integer(v.size());
        for (const auto &x : v)
            Codec<T>::write(w, x);
    }
    static void read(Reader &r, std::set<T> &v) {
        const auto n = r.count(Codec<T>::minimum(), sizeof(T) + 4 * sizeof(void *));
        v.clear();
        for (std::size_t i = 0; i < n; ++i) {
            T x{};
            r.field("set[]", x);
            need(v.insert(std::move(x)).second, "重复set元素");
        }
    }
    static std::size_t minimum() { return 8; }
};
// 私有状态只经显式快照契约，不读取对象内存表示或推进随机流。
template <class T, class Snapshot> struct SnapshotCodec {
    static void write(Writer &w, const T &v) { Codec<Snapshot>::write(w, v.snapshot()); }
    static void read(Reader &r, T &v) {
        Snapshot snapshot{};
        r.field("private_snapshot", snapshot);
        auto value = T::from_snapshot(snapshot);
        need(value.has_value(), "非法私有状态快照");
        v = std::move(*value);
    }
    static std::size_t minimum() { return Codec<Snapshot>::minimum(); }
};
template <>
struct Codec<dungeon_village_reference::WorldRandomStream>
    : SnapshotCodec<dungeon_village_reference::WorldRandomStream,
                    dungeon_village_reference::WorldRandomSnapshot> {};
template <>
struct Codec<dungeon_village_reference::PeriodAccounting>
    : SnapshotCodec<dungeon_village_reference::PeriodAccounting,
                    dungeon_village_reference::PeriodAccountingSnapshot> {};
#include "startup_world_codec_fields.inc"
} // namespace

std::vector<std::uint8_t> encode_state(const StartupWorldRuntimeState &state) {
    Writer writer;
    writer.field("owner", state);
    return std::move(writer.bytes);
}
StartupWorldRuntimeState decode_state(const std::vector<std::uint8_t> &bytes,
                                      const StartupWorldRules &rules, CodecDecodeBudget &budget) {
    need(bytes.size() <= owner_budget, "Owner载荷超出64MiB预算");
    need(budget.nodes_remaining > 0 &&
             budget.allocation_bytes_remaining >= sizeof(StartupWorldRuntimeState),
         "Owner根预算耗尽");
    --budget.nodes_remaining;
    budget.allocation_bytes_remaining -= sizeof(StartupWorldRuntimeState);
    StartupWorldRuntimeState candidate;
    Reader reader{bytes, budget};
    reader.field("owner", candidate);
    need(reader.remaining() == 0, "Owner有尾随载荷");
    candidate.rules = &rules;
    return candidate;
}
StartupWorldRuntimeState decode_state(const std::vector<std::uint8_t> &bytes,
                                      const StartupWorldRules &rules) {
    CodecDecodeBudget budget;
    return decode_state(bytes, rules, budget);
}
const char *codec_schema_identity() { return schema_identity; }
} // namespace dungeon_village_prototype::persistence_detail
