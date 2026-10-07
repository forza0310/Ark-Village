#include "startup_world_replay_driver.hpp"

#include <limits>
#include <stdexcept>
#include <type_traits>

namespace ark::simulation::test {
void validate_startup_world_replay_options(const StartupWorldReplayOptions &o, int next_frame,
                                         int frame_limit) {
    for (const auto frame : {o.save_at, o.stop_at, o.trace_from})
        if (frame && (*frame < next_frame || *frame >= frame_limit))
            throw std::invalid_argument("natural replay option is outside remaining frame range");
    if (o.save_at.has_value() != !o.save_file.empty() ||
        o.save_every.has_value() != !o.save_directory.empty() ||
        (o.save_every && (*o.save_every <= 0 || *o.save_every >= frame_limit)) ||
        (o.trace_from && o.trace_file.empty()) ||
        (o.save_at && o.stop_at && *o.save_at > *o.stop_at))
        throw std::invalid_argument("natural replay requires matching save/trace options");
}

bool startup_world_periodic_snapshot_due(const StartupWorldReplayOptions &o, int frame) {
    // 原外层帧号的正整数倍；不在新局尚无完整轮时保存，也不因恢复改变调度相位。
    return o.save_every && *o.save_every > 0 && frame > 0 && frame % *o.save_every == 0;
}

namespace {
constexpr std::uint64_t magic = 0x315652445741ULL; // AWDRV1，维护测试协议。

struct Writer {
    std::vector<std::uint8_t> bytes;
    void integer(std::uint64_t value) {
        for (int n = 0; n != 8; ++n)
            bytes.push_back(static_cast<std::uint8_t>(value >> (n * 8)));
    }
    void field(bool &v) { integer(v ? 1 : 0); }
    template <typename T> void field(T &v) { integer(static_cast<std::uint64_t>(v)); }
    void field(std::optional<std::uint64_t> &v) {
        integer(v ? 1 : 0);
        if (v) integer(*v);
    }
    void field(std::optional<ref::Position> &v) {
        integer(v ? 1 : 0);
        if (v) { field(v->x); field(v->y); }
    }
    template <typename T> void field(std::set<T> &v) {
        integer(v.size());
        for (auto item : v) field(item);
    }
    template <typename T, std::size_t N> void field(std::array<T, N> &v) {
        for (auto &item : v) field(item);
    }
};

struct Reader {
    const std::vector<std::uint8_t> &bytes;
    std::size_t position{};
    [[noreturn]] static void fail() { throw std::invalid_argument("invalid natural replay driver"); }
    std::uint64_t integer() {
        if (bytes.size() - position < 8) fail();
        std::uint64_t value{};
        for (int n = 0; n != 8; ++n)
            value |= static_cast<std::uint64_t>(bytes[position++]) << (n * 8);
        return value;
    }
    void field(bool &v) {
        const auto value = integer();
        if (value > 1) fail();
        v = value != 0;
    }
    template <typename T> void field(T &v) {
        const auto value = integer();
        if constexpr (std::is_signed<T>::value) {
            // 避免无符号超范围到有符号转换的实现定义行为。
            const auto signed_value = value <= static_cast<std::uint64_t>(INT64_MAX)
                                          ? static_cast<std::int64_t>(value)
                                          : -1 - static_cast<std::int64_t>(UINT64_MAX - value);
            if (signed_value < std::numeric_limits<T>::min() ||
                signed_value > std::numeric_limits<T>::max()) fail();
            v = static_cast<T>(signed_value);
        } else {
            if (value > std::numeric_limits<T>::max()) fail();
            v = static_cast<T>(value);
        }
    }
    void field(std::optional<std::uint64_t> &v) {
        bool present{}; field(present);
        if (present) v = integer(); else v.reset();
    }
    void field(std::optional<ref::Position> &v) {
        bool present{}; field(present);
        if (present) { ref::Position p{}; field(p.x); field(p.y); v = p; } else v.reset();
    }
    template <typename T> void field(std::set<T> &v) {
        const auto size = integer();
        if (size > (bytes.size() - position) / 8) fail();
        for (std::uint64_t n = 0; n < size; ++n) {
            T item{}; field(item);
            if (!v.insert(item).second) fail();
        }
    }
    template <typename T, std::size_t N> void field(std::array<T, N> &v) {
        for (auto &item : v) field(item);
    }
};

// 显式字段登记同时约束写入和读取，避免调度器新增跨帧字段而只更新单向路径。
template <typename Archive> void fields(Archive &a, StartupWorldReplayDriver &d) {
    a.field(d.seed); a.field(d.speed); a.field(d.expansion);
    a.field(d.next_frame); a.field(d.observed_frame); a.field(d.checks); a.field(d.terminal);
    a.field(d.bakery); a.field(d.upgraded); a.field(d.seen_upgrades);
    a.field(d.last_month); a.field(d.promoted_month); a.field(d.unlocked_month);
    a.field(d.unlocked_income); a.field(d.edited_month); a.field(d.edited_income);
    a.field(d.edited_retired); a.field(d.progression_complete);
    a.field(d.expanded_month); a.field(d.expanded_road_month); a.field(d.expanded_income);
    a.field(d.expanded_road); a.field(d.expanded_road_definition);
    a.field(d.sounds); a.field(d.peak_sounds); a.field(d.peak_payloads); a.field(d.peak_pages);
    a.field(d.peak_effects); a.field(d.peak_actors); a.field(d.peak_cash); a.field(d.peak_tasks);
    a.field(d.peak_retired_actors); a.field(d.peak_retired_encounters);
    a.field(d.next_activity_attempt); a.field(d.completed); a.field(d.unlocked_completed);
    a.field(d.next_task_month); a.field(d.previous_task);
}

void validate(const StartupWorldReplayDriver &d) {
    const int limit = d.expansion ? 240000 : 180000;
    if ((d.speed != 0 && d.speed != 1) || d.next_frame < 1 || d.next_frame > limit ||
        d.observed_frame != d.next_frame - 1 || d.checks < 0 || d.last_month < 0 ||
        d.promoted_month < -1 || d.unlocked_month < -1 || d.edited_month < -1 ||
        d.expanded_month < -1 || d.expanded_road_month < -1 || d.next_task_month < 0 ||
        d.next_activity_attempt < 0 || d.completed < 0 || d.unlocked_completed < 0 ||
        d.unlocked_completed > d.completed || d.unlocked_income < 0 || d.edited_income < 0 ||
        d.expanded_income < 0 || (d.bakery && *d.bakery == 0) ||
        (d.previous_task && *d.previous_task == 0) ||
        (d.expanded_road.has_value() != (d.expanded_road_month >= 0)) ||
        (d.expanded_road && (d.expanded_month < 0 || d.expanded_road_definition < 0 ||
                             d.expanded_road->x < 0 || d.expanded_road->y < 0)) ||
        (!d.expansion && (d.progression_complete || d.expanded_month >= 0 || d.expanded_road)) ||
        (d.progression_complete && d.edited_month < 0) ||
        (d.terminal && (d.expansion ? !d.expanded_road : d.edited_month < 0)))
        Reader::fail();
    for (auto id : d.upgraded) if (id < 0) Reader::fail();
    for (auto id : d.seen_upgrades) if (id == 0) Reader::fail();
}
} // namespace

std::vector<std::uint8_t> encode_startup_world_replay_driver(const StartupWorldReplayDriver &d) {
    validate(d);
    Writer w; w.integer(magic);
    auto copy = d;
    fields(w, copy);
    return std::move(w.bytes);
}

StartupWorldReplayDriver decode_startup_world_replay_driver(const std::vector<std::uint8_t> &bytes) {
    Reader r{bytes};
    if (r.integer() != magic) Reader::fail();
    StartupWorldReplayDriver d;
    fields(r, d);
    if (r.position != bytes.size()) Reader::fail();
    validate(d);
    return d;
}

void validate_startup_world_replay_driver_world(const StartupWorldReplayDriver &d,
                                               const StartupWorldRuntimeState &s) {
    validate(d);
    const auto require = [](bool valid, const char *message) {
        if (!valid) throw std::invalid_argument(message);
    };
    require(s.rules && d.speed == s.scene.speed_setting && s.sound_requests.empty(),
            "natural replay driver does not match world speed/output boundary");
    const int month = s.scene.calendar.year * 12 + s.scene.calendar.month;
    require(d.last_month == month && d.checks >= d.next_frame,
            "natural replay driver has incompatible month or check progress");
    for (const auto recorded : {d.promoted_month, d.unlocked_month, d.edited_month,
                                d.expanded_month, d.expanded_road_month})
        require(recorded <= month, "natural replay driver milestone lies after world date");
    if (d.bakery) {
        const auto found = s.scene.world.world.facilities.find(*d.bakery);
        require(found != s.scene.world.world.facilities.end() &&
                    found->second.placement.definition_id == 35,
                "natural replay bakery does not reference the actual definition35 instance");
    }
    require(d.upgraded.size() <= d.seen_upgrades.size(),
            "natural replay upgrade definitions have no observed presentation pages");
    for (const auto id : d.upgraded) {
        const auto found = s.scene.world.world.facility_uses.find(id);
        require(found != s.scene.world.world.facility_uses.end() && found->second.level > 1,
                "natural replay upgrade refers to absent or unupgraded definition");
    }
    // seen_upgrades是历史去重集合，页面允许已退休；只校验曾分配身份的范围。
    for (const auto id : d.seen_upgrades)
        require(id < s.scripts.next_page_id,
                "natural replay observed upgrade page was never allocated");
    // 前一轮任务可已完成，不能错误要求它等于当前active_task。
    if (d.previous_task)
        require(*d.previous_task < s.next_task_identity && s.tasks.count(*d.previous_task) != 0,
                "natural replay previous task has no retained task record");
    if (d.promoted_month >= 0)
        require(d.bakery.has_value() && s.rank >= 1,
                "natural replay promotion milestone has no world prerequisite");
    if (d.unlocked_month >= 0)
        require(d.promoted_month >= 0 && d.unlocked_month >= d.promoted_month &&
                    d.unlocked_completed > 0,
                "natural replay exhibition milestone has no promotion prefix");
    if (d.edited_month >= 0) {
        require(d.unlocked_month >= 0 && d.edited_month > d.unlocked_month &&
                    d.edited_retired[0] != d.edited_retired[1],
                "natural replay editing milestone has no completed exhibition prefix");
        for (const auto id : d.edited_retired)
            require(id > 0 && id < s.next_facility_identity &&
                        s.scene.world.world.facilities.count(id) == 0 &&
                        s.facility_month_age.count(id) == 0,
                    "natural replay retired editing identity is live or was never allocated");
    } else
        require(d.edited_retired == std::array<std::uint64_t, 2>{},
                "natural replay driver has retired identities before editing");
    if (d.expanded_month >= 0) {
        const auto count = s.activity_counts.find(25);
        require(d.progression_complete && d.expanded_month >= d.edited_month &&
                    s.fence_level == 1 && count != s.activity_counts.end() && count->second == 1,
                "natural replay expansion milestone does not match real activity25");
    }
    if (d.expanded_road) {
        const auto &map = s.scene.world.world.map;
        const auto p = *d.expanded_road;
        require(p.x < map.width && p.y < map.height &&
                    d.expanded_road_month >= d.expanded_month,
                "natural replay expanded road is outside the current map or phase");
        const auto index = static_cast<std::size_t>(p.y * map.width + p.x);
        require(index < map.cells.size() && index < s.surface.size() &&
                    map.cells[index].legacy_state == 3 &&
                    s.surface[index].definition == d.expanded_road_definition,
                "natural replay expanded road does not match current world surface");
    }
    std::int64_t income{};
    for (const auto &entry : s.scene.world.world.ai.accounting.entries())
        if (entry.second.category == ref::CashCategory::facilities &&
            entry.second.direction == ref::CashDirection::income) {
            require(entry.second.amount >= 0 &&
                        entry.second.amount <= std::numeric_limits<std::int64_t>::max() - income,
                    "natural replay retained facility income cannot be represented");
            income += entry.second.amount;
        }
    require(d.unlocked_income <= income && d.edited_income <= income && d.expanded_income <= income,
            "natural replay income baseline exceeds retained facility income");
    const auto exhibition = s.activity_counts.find(16);
    require(d.unlocked_completed == 0 ||
                (exhibition != s.activity_counts.end() && d.unlocked_completed <= exhibition->second),
            "natural replay completed exhibition count exceeds actual world count");
}
} // namespace ark::simulation::test
