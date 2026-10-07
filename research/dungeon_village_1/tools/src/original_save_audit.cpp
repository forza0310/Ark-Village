// 固定原档字段布局的只读审计；不调用世界Owner，不执行原加载修复。
#include "dungeon_village_tools/original_save_audit.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace dungeon_village_tools {
namespace {
constexpr std::size_t max_dynamic_count = 10000;
constexpr std::size_t max_dynamic_items = 131072;

struct Budget {
    std::size_t remaining{max_dynamic_items};
};

// 游标直接读取容器中的分区切片；字符串/浮点只消费原字节，不重编码或保存正文。
class Reader {
  public:
    Reader(const std::vector<std::uint8_t> &bytes, std::size_t begin, std::size_t size,
           int partition, Budget &budget)
        : bytes_(bytes), begin_(begin), at_(begin), partition_(partition), budget_(budget) {
        if (begin > bytes.size() || size > bytes.size() - begin)
            fail("分区范围超出容器");
        end_ = begin + size;
    }
    [[noreturn]] void fail(const std::string &message) const {
        throw std::runtime_error("原档分区" + std::to_string(partition_) + "偏移" +
                                 std::to_string(at_ - begin_) + "：" + message);
    }
    std::size_t remaining() const { return end_ - at_; }
    std::size_t consumed() const { return at_ - begin_; }
    std::size_t booleans() const { return noncanonical_booleans_; }
    void take(std::size_t size, const char *field) {
        if (size > remaining())
            fail(std::string(field) + "截断");
        at_ += size;
    }
    std::int32_t integer(unsigned width, const char *field) {
        if (width > remaining())
            fail(std::string(field) + "截断");
        std::uint32_t value{};
        for (unsigned i = 0; i < width; ++i)
            value = (value << 8U) | bytes_[at_++];
        const std::int64_t modulus = width == 2 ? 65536LL : 4294967296LL;
        const auto signed_value = value < modulus / 2 ? static_cast<std::int64_t>(value)
                                                      : static_cast<std::int64_t>(value) - modulus;
        return static_cast<std::int32_t>(signed_value);
    }
    std::int32_t h(const char *field) { return integer(2, field); }
    std::int32_t i(const char *field) { return integer(4, field); }
    void boolean(const char *field) {
        if (remaining() == 0)
            fail(std::string(field) + "截断");
        const auto value = bytes_[at_++];
        if (value != 0 && value != 1)
            ++noncanonical_booleans_;
    }
    void values(char type, std::size_t count, const char *field) {
        const std::size_t width = type == 'H' ? 2 : type == 'I' ? 4 : type == 'V' ? 12 : 1;
        if (count > remaining() / width)
            fail(std::string(field) + "序列截断");
        if (type == 'B') {
            for (std::size_t n = 0; n < count; ++n)
                boolean(field);
        } else {
            take(count * width, field);
        }
    }
    void schema(const char *types, const char *field) {
        for (const char *type = types; *type; ++type)
            values(*type, 1, field);
    }
    void string(const char *field) {
        const auto size = i(field);
        if (size < 0)
            fail(std::string(field) + "字符串长度为负");
        take(static_cast<std::size_t>(size), field);
    }
    std::size_t count(char type, std::size_t minimum, const char *field) {
        const auto raw = type == 'H' ? h(field) : i(field);
        if (raw < 0 || static_cast<std::size_t>(raw) > max_dynamic_count)
            fail(std::string(field) + "数量超出0..10000");
        const auto n = static_cast<std::size_t>(raw);
        if (minimum == 0 || n > remaining() / minimum)
            fail(std::string(field) + "数量超过剩余字节");
        if (n > budget_.remaining)
            fail(std::string(field) + "跨分区动态项预算耗尽");
        budget_.remaining -= n;
        return n;
    }
    std::vector<std::int32_t> references(char type, const char *field) {
        const auto n = count(type, 4, field);
        std::vector<std::int32_t> result;
        result.reserve(n);
        for (std::size_t index = 0; index < n; ++index)
            result.push_back(i(field));
        return result;
    }
    void finish() const {
        if (at_ != end_)
            fail("未恰好消费到分区末尾");
    }

  private:
    const std::vector<std::uint8_t> &bytes_;
    std::size_t begin_{}, at_{}, end_{};
    int partition_{};
    Budget &budget_;
    std::size_t noncanonical_booleans_{};
};

struct Actor {
    std::int32_t uid{}, human{}, monster{}, encounter{};
};
struct Encounter {
    std::int32_t uid{}, state{}, counter{}, group_state{}, group_counter{}, turn_time{};
    std::vector<std::int32_t> humans, monsters;
};
struct Task {
    std::int32_t uid{}, facility{}, encounter{};
};
struct Facility {
    std::int32_t uid{}, definition{}, serial{};
    std::vector<std::int32_t> humans;
    std::vector<std::pair<std::int32_t, std::int32_t>> history;
};
struct Parsed {
    std::array<std::vector<Actor>, 2> actors;
    std::vector<Encounter> encounters;
    std::vector<Task> tasks;
    std::vector<Facility> facilities;
    std::int32_t user_task{};
    std::vector<std::int32_t> user_people;
    std::array<std::array<std::int32_t, 2>, 14> tp_clear{};
    std::size_t bubbles{}, exploration_rows{};
};

void character_definition(Reader &r) {
    r.string("character.az");
    r.schema("HIHIBHH", "character.c..u");
    r.values('H', 4, "character.v");
    r.h("character.m");
    r.values('B', 4, "character.s");
    r.values('I', 12, "character.y/z");
    r.values('H', 4, "character.A");
    r.values('I', 3, "character.B");
    r.h("character.C");
    r.values('I', 4, "character.D");
    r.schema("HIIHHIHI", "character.E..L");
    r.values('H', 23, "character.M");
    r.schema("IIB", "character.N/O/P");
    r.values('H', 23, "character.R");
}

Actor character(Reader &r) {
    r.h("actor.definition");
    Actor result;
    result.uid = r.i("actor.uid");
    r.values('H', 6, "actor.f..k");
    r.i("actor.l");
    const auto rows = r.count('H', 2, "actor.m_count");
    for (std::size_t row = 0; row < rows; ++row)
        r.values('I', r.count('H', 4, "actor.m.row_count"), "actor.m.row");
    r.values('V', 5, "actor.n..r");
    r.values('H', 8, "actor.s..v");
    r.schema("BHHIHIIHHH", "actor.w..F");
    r.values('H', 2 * r.count('H', 4, "actor.G_count"), "actor.G");
    r.h("actor.H");
    r.values('H', 6, "actor.I");
    r.i("actor.J");
    r.values('H', 10, "actor.K..Q");
    result.human = r.i("actor.aC");
    result.monster = r.i("actor.aD");
    r.values('H', 2, "actor.T/U");
    r.values('I', 7, "actor.V..ab");
    r.values('H', 6, "actor.ac..ah");
    r.schema("BIB", "actor.ai/aj/ak");
    result.encounter = r.i("actor.aE");
    r.boolean("actor.al");
    r.values('I', 10, "actor.am..aq");
    r.schema("BIHVIHBH", "actor.ar..ay");
    return result;
}

Encounter encounter(Reader &r) {
    Encounter result;
    result.uid = r.i("encounter.uid");
    r.values('H', 4, "encounter.position/d/e");
    r.i("encounter.f");
    r.values('H', 2, "encounter.g/target");
    result.state = r.h("encounter.k");
    result.counter = r.i("encounter.l");
    r.i("encounter.m");
    result.humans = r.references('H', "encounter.humans");
    result.monsters = r.references('H', "encounter.monsters");
    result.group_state = r.h("encounter.group.d");
    result.group_counter = r.i("encounter.group.e");
    r.h("encounter.group.f");
    r.i("encounter.group.g");
    result.turn_time = r.i("encounter.group.h");
    return result;
}

void user(Reader &r, Parsed &parsed) {
    r.values('I', 4, "user.c..f");
    parsed.user_task = r.i("user.P");
    r.values('I', 4, "user.i..l");
    parsed.user_people = r.references('I', "user.Q");
    r.values('I', 13, "user.n");
    r.values('I', 11, "user.o..y");
    r.values('I', 120, "user.z");
    r.values('I', 2, "user.A/B");
    r.values('I', 300, "user.C");
    r.values('I', 4, "user.D..G");
    r.h("user.H");
    r.values('H', 3 * r.count('H', 6, "user.I_count"), "user.I");
    for (auto &date : parsed.tp_clear) {
        date[0] = r.h("user.J.year");
        date[1] = r.h("user.J.month");
    }
    r.values('I', 12, "user.K/L");
    r.values('B', 6, "user.M");
    r.values('H', r.count('I', 2, "user.N_count"), "user.N");
    r.values('H', 5, "user.O");
}

Facility facility(Reader &r, Parsed &parsed) {
    r.values('H', 2, "facility.anchor");
    Facility result;
    result.uid = r.i("facility.b");
    result.serial = r.i("facility.c");
    result.definition = r.h("facility.d");
    r.h("facility.e");
    r.values('I', 5, "facility.f..j");
    const auto rows = r.count('H', 2, "facility.k_count");
    parsed.exploration_rows += rows;
    for (std::size_t row = 0; row < rows; ++row)
        r.values('H', r.count('H', 2, "facility.k.row_count"), "facility.k.row");
    r.values('H', 2, "facility.l/m");
    r.i("facility.n");
    result.humans = r.references('H', "facility.A");
    const auto bubbles = r.count('H', 4, "facility.p_count");
    parsed.bubbles += bubbles;
    r.values('H', 2 * bubbles, "facility.p");
    r.values('H', 3, "facility.q/r");
    r.values('I', 3, "facility.s");
    r.values('H', 2, "facility.t/u");
    r.values('I', 24, "facility.v");
    const auto history = r.count('H', 8, "facility.w_count");
    result.history.reserve(history);
    for (std::size_t index = 0; index < history; ++index) {
        const auto definition = r.i("facility.w.definition");
        const auto serial = r.i("facility.w.serial");
        result.history.emplace_back(definition, serial);
    }
    r.values('I', 3, "facility.x/y");
    return result;
}

std::size_t partition(Reader &r, int id, Parsed &parsed) {
    static constexpr std::array<std::size_t, 14> definitions{0,  30, 50,  31, 25, 36, 23,
                                                             40, 36, 100, 81, 5,  85, 33};
    static constexpr std::array<const char *, 14> schemas{
        "",   "HBH",     "HBH", "IHBI", "",   "HBIIB",           "HB",
        "HB", "HBIIIIB", "HB",  "HBI",  "HB", "HIBHHHHHHIBBHBB", "HBH"};
    if (id >= 1 && id <= 13) {
        for (std::size_t row = 0; row < definitions[id]; ++row) {
            if (id == 4)
                character_definition(r);
            else
                r.schema(schemas[id], "definition");
        }
        return definitions[id];
    }
    if (id == 14) {
        r.values('V', 3, "camera");
        return 1;
    }
    if (id == 21) {
        user(r, parsed);
        return 1;
    }
    if (id == 23) {
        r.take(24U * 24U * 10U, "map24x24");
        return 576;
    }
    const std::size_t minimum = id == 0                ? 28
                                : id == 15 || id == 16 ? 296
                                : id == 17             ? 70
                                : id == 18             ? 50
                                : id == 19             ? 64
                                : id == 20             ? 27
                                : id == 22             ? 8
                                : id == 24             ? 182
                                                       : 0;
    if (!minimum)
        r.fail("未登记分区");
    const auto records = r.count('I', minimum, "record_count");
    for (std::size_t row = 0; row < records; ++row) {
        if (id == 0) {
            r.values('I', 5, "delay.header");
            r.string("delay.text");
            r.values('I', r.count('I', 4, "delay.params_count"), "delay.params");
        } else if (id == 15 || id == 16) {
            parsed.actors[id - 15].push_back(character(r));
        } else if (id == 17) {
            r.take(70, "ground_effect");
        } else if (id == 18) {
            parsed.encounters.push_back(encounter(r));
        } else if (id == 19) {
            r.take(64, "projectile"); // 身份源/目标判别尚未证，不猜引用域。
        } else if (id == 20) {
            Task task;
            task.uid = r.i("task.uid");
            r.values('H', 4, "task.type/definition/position");
            task.facility = r.i("task.facility");
            task.encounter = r.i("task.encounter");
            r.schema("HIB", "task.h/i/j");
            parsed.tasks.push_back(task);
        } else if (id == 22) {
            r.values('H', 4, "facility.preview");
        } else if (id == 24) {
            parsed.facilities.push_back(facility(r, parsed));
        }
    }
    return records;
}

const OriginalSaveField &field(const OriginalSaveInspection &inspection, const std::string &path,
                               const char *type) {
    const auto found = std::find_if(inspection.fields.begin(), inspection.fields.end(),
                                    [&](const auto &entry) { return entry.path == path; });
    if (found == inspection.fields.end() || found->type != type)
        throw std::runtime_error("不匹配原世界存档布局：" + path);
    return *found;
}
std::int64_t number(const OriginalSaveField &field) {
    std::int64_t value{};
    const auto parsed =
        std::from_chars(field.value.data(), field.value.data() + field.value.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != field.value.data() + field.value.size())
        throw std::runtime_error("容器数值字段不合法：" + field.path);
    return value;
}
void require_group(const OriginalSaveInspection &inspection, const std::string &path,
                   std::int64_t count) {
    if (number(field(inspection, path, "group")) != count)
        throw std::runtime_error("不匹配已登记profile的容器数量：" + path);
}

template <class Records>
std::set<std::int32_t> identities(OriginalSaveAudit &audit, int partition, const Records &records) {
    std::set<std::int32_t> result;
    for (const auto &record : records)
        result.insert(record.uid);
    const auto prefix = "partition." + std::to_string(partition);
    audit.facts[prefix + ".unique_ids"] = static_cast<std::int64_t>(result.size());
    audit.facts[prefix + ".duplicate_ids"] =
        static_cast<std::int64_t>(records.size() - result.size());
    return result;
}
void reference(OriginalSaveReferenceAudit &audit, std::int32_t value,
               const std::set<std::int32_t> &target) {
    if (value == -1)
        return;
    ++audit.checked;
    if (!target.count(value)) {
        ++audit.missing;
        if (audit.missing_values.size() < 16)
            audit.missing_values.push_back(value);
    }
}

// 诊断按实际引用域检查；重复项仍逐次计数，缺目标不改变任何原载荷。
void audit_references(OriginalSaveAudit &audit, const Parsed &parsed) {
    const auto humans = identities(audit, 15, parsed.actors[0]);
    const auto monsters = identities(audit, 16, parsed.actors[1]);
    const auto encounters = identities(audit, 18, parsed.encounters);
    const auto tasks = identities(audit, 20, parsed.tasks);
    const auto facilities = identities(audit, 24, parsed.facilities);
    const auto add = [&](int id, const char *name) -> OriginalSaveReferenceAudit & {
        audit.references.push_back({id, name, 0, 0, {}, {}});
        return audit.references.back();
    };
    for (int kind = 0; kind < 2; ++kind) {
        auto &human = add(15 + kind, "aC");
        for (const auto &actor : parsed.actors[kind])
            reference(human, actor.human, humans);
        auto &monster = add(15 + kind, "aD");
        for (const auto &actor : parsed.actors[kind])
            reference(monster, actor.monster, monsters);
        auto &encounter = add(15 + kind, "aE");
        for (const auto &actor : parsed.actors[kind])
            reference(encounter, actor.encounter, encounters);
    }
    auto &group_humans = add(18, "people");
    for (const auto &encounter : parsed.encounters)
        for (const auto id : encounter.humans)
            reference(group_humans, id, humans);
    auto &group_monsters = add(18, "monsters");
    for (const auto &encounter : parsed.encounters)
        for (const auto id : encounter.monsters)
            reference(group_monsters, id, monsters);
    auto &task_facility = add(20, "facility");
    for (const auto &task : parsed.tasks)
        reference(task_facility, task.facility, facilities);
    auto &task_encounter = add(20, "encounter");
    for (const auto &task : parsed.tasks)
        reference(task_encounter, task.encounter, encounters);
    reference(add(21, "P"), parsed.user_task, tasks);
    std::set<std::int32_t> human_definitions;
    for (std::int32_t id = 0; id < 25; ++id)
        human_definitions.insert(id);
    auto &user_people = add(21, "Q");
    for (const auto id : parsed.user_people)
        reference(user_people, id, human_definitions);
    auto &facility_humans = add(24, "A");
    for (const auto &facility : parsed.facilities)
        for (const auto id : facility.humans)
            reference(facility_humans, id, humans);
    std::set<std::pair<std::int32_t, std::int32_t>> pairs;
    for (const auto &facility : parsed.facilities)
        pairs.emplace(facility.definition, facility.serial);
    auto &history = add(24, "w");
    for (const auto &facility : parsed.facilities)
        for (const auto &pair : facility.history) {
            ++history.checked;
            if (!pairs.count(pair)) {
                ++history.missing;
                // 原w没有标量UID的-1哨兵合同；保存完整definition/serial，不压成伪UID。
                if (history.missing_pairs.size() < 16)
                    history.missing_pairs.push_back(pair);
            }
        }
    audit.facts["partition.19.reference_audit_covered"] = 0;
}

std::int32_t month_index(std::int64_t year, std::int64_t month) {
    // 原Java/Steam int32运算溢出回绕；用无符号计算避免C++有符号溢出。
    const auto raw =
        static_cast<std::uint32_t>(year) * std::uint32_t{12} + static_cast<std::uint32_t>(month);
    return raw <= 0x7fffffffU
               ? static_cast<std::int32_t>(raw)
               : static_cast<std::int32_t>(static_cast<std::int64_t>(raw) - 4294967296LL);
}
void calendar_facts(OriginalSaveAudit &audit, const Parsed &parsed) {
    // APK d/a:1753及Steam 1025D7BC：最大初值0，但候选索引是原J[0][0]。
    // 全部年月<=0时不得合成0/0；负root仍可能触发按首条原日期回写的条件。
    std::int32_t maximum{};
    std::size_t selected{};
    for (std::size_t n = 0; n < parsed.tp_clear.size(); ++n) {
        const auto value = month_index(parsed.tp_clear[n][0], parsed.tp_clear[n][1]);
        if (maximum < value) {
            maximum = value;
            selected = n;
        }
    }
    const auto year = audit.facts.at("root.year"), month = audit.facts.at("root.month");
    const bool repair = month_index(year, month) < maximum;
    const auto &date = parsed.tp_clear[selected];
    audit.facts["user.tp_clear.max_month_index"] = maximum;
    audit.facts["user.tp_clear.max_year"] = date[0];
    audit.facts["user.tp_clear.max_month"] = date[1];
    audit.facts["user.tp_clear.selected_section"] = static_cast<std::int64_t>(selected / 7);
    audit.facts["user.tp_clear.selected_row"] = static_cast<std::int64_t>(selected % 7);
    audit.facts["user.tp_clear.repair_needed"] = repair;
    audit.facts["user.tp_clear.candidate_year"] = repair ? date[0] : year;
    audit.facts["user.tp_clear.candidate_month"] = repair ? date[1] : month;
}
} // namespace

OriginalSaveAudit audit_original_save(const OriginalSaveInspection &inspection,
                                      OriginalSaveAuditProfile profile) {
    if (inspection.empty_record || inspection.container_bytes.empty())
        throw std::runtime_error("空记录不是可审计世界存档");
    if (profile != OriginalSaveAuditProfile::apk108 &&
        profile != OriginalSaveAuditProfile::steam_9cf4bb10)
        throw std::runtime_error("未知原档审计profile");
    // 复用已有容器验证：公开Inspection可由调用者构造，不能信任过期offset/value元数据。
    // 只允许世界P的非null分区；容器再次验证不执行原档外壳解密或读取用户身份。
    const auto verified =
        inspect_original_save(inspection.container_bytes, OriginalSaveFormat::container);
    static constexpr std::array<int, 5> shape{1, 16, 3, 1, 25};
    for (int tag = 0; tag < 5; ++tag)
        require_group(verified, "$/" + std::to_string(tag), shape[tag]);
    field(verified, "$/0/0", "container");
    const auto events = profile == OriginalSaveAuditProfile::apk108 ? 200 : 228;
    for (int tag = 0; tag < 5; ++tag)
        require_group(verified, "$/0/0/" + std::to_string(tag), tag == 1 ? events : 0);
    OriginalSaveAudit result;
    result.profile = profile == OriginalSaveAuditProfile::apk108 ? "apk108" : "steam_9cf4bb10";
    const std::array<const char *, 5> calendar{"year", "month", "week", "time", "old_time"};
    for (std::size_t index = 0; index < calendar.size(); ++index)
        result.facts[std::string("root.") + calendar[index]] =
            number(field(verified, "$/1/" + std::to_string(index + 1), "int32"));
    result.facts["root.cash"] = number(field(verified, "$/2/0", "int64"));
    Budget budget;
    Parsed parsed;
    for (int id = 0; id < 25; ++id) {
        const auto &blob = field(verified, "$/4/" + std::to_string(id), "opaque-bytes");
        Reader reader(verified.container_bytes, blob.offset, blob.size, id, budget);
        const auto records = partition(reader, id, parsed);
        reader.finish();
        result.partitions.push_back({id, blob.size, reader.consumed(), records, reader.booleans()});
        result.facts["partition." + std::to_string(id) + ".records"] =
            static_cast<std::int64_t>(records);
    }
    audit_references(result, parsed);
    result.facts["facility.bubbles"] = static_cast<std::int64_t>(parsed.bubbles);
    result.facts["facility.exploration_rows"] = static_cast<std::int64_t>(parsed.exploration_rows);
    std::map<std::int32_t, std::size_t> encounter_counts;
    for (const auto &encounter : parsed.encounters)
        ++encounter_counts[encounter.uid];
    for (const auto &encounter : parsed.encounters) {
        if (encounter_counts.at(encounter.uid) != 1)
            continue; // 不能任取重复UID某行冒充唯一状态。
        const auto prefix = "encounter." + std::to_string(encounter.uid);
        result.facts[prefix + ".state"] = encounter.state;
        result.facts[prefix + ".counter"] = encounter.counter;
        result.facts[prefix + ".group_state"] = encounter.group_state;
        result.facts[prefix + ".group_counter"] = encounter.group_counter;
        result.facts[prefix + ".turn_time"] = encounter.turn_time;
    }
    calendar_facts(result, parsed);
    return result;
}
} // namespace dungeon_village_tools
