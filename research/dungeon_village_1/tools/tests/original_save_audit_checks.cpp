// 固定APK合同的独立手工夹具；不是原版新局，不读取用户存档或复制审计算法。
#include "dungeon_village_tools/original_save_audit.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace dungeon_village_tools;

namespace {
using Bytes = std::vector<std::uint8_t>;
using Parts = std::array<Bytes, 25>;
int checks = 0;

void check(bool condition, const std::string &message) {
    ++checks;
    if (!condition) {
        std::cerr << "失败: 原档25分区审计: " << message << '\n';
        std::exit(1);
    }
}

template <typename F> void rejects(F operation, const std::string &message) {
    ++checks;
    try {
        operation();
    } catch (const std::exception &) {
        return;
    }
    std::cerr << "失败: 原档25分区审计应拒绝: " << message << '\n';
    std::exit(1);
}

void i32(Bytes &out, std::int32_t value) {
    const auto v = static_cast<std::uint32_t>(value);
    for (const unsigned shift : {24U, 16U, 8U, 0U})
        out.push_back(static_cast<std::uint8_t>(v >> shift));
}

void i16(Bytes &out, std::int16_t value) {
    const auto v = static_cast<std::uint16_t>(value);
    out.push_back(static_cast<std::uint8_t>(v >> 8U));
    out.push_back(static_cast<std::uint8_t>(v));
}

void zeros(Bytes &out, std::size_t size) { out.insert(out.end(), size, 0); }
void append(Bytes &out, const Bytes &value) { out.insert(out.end(), value.begin(), value.end()); }
void patch32(Bytes &out, std::size_t at, std::int32_t value) {
    Bytes encoded;
    i32(encoded, value);
    std::copy(encoded.begin(), encoded.end(), out.begin() + static_cast<std::ptrdiff_t>(at));
}
void patch16(Bytes &out, std::size_t at, std::int16_t value) {
    Bytes encoded;
    i16(encoded, value);
    std::copy(encoded.begin(), encoded.end(), out.begin() + static_cast<std::ptrdiff_t>(at));
}

// 非空m行和G行各一条，固定其独立合同总长310；UID和引用使用不连续稳定值。
Bytes character(std::int32_t uid, std::int32_t human, std::int32_t monster,
                std::int32_t encounter) {
    Bytes out;
    i16(out, 0); // d
    i32(out, uid);
    zeros(out, 16); // f..k,l
    i16(out, 1);    // m_count
    i16(out, 2);    // m[0].count
    i32(out, 7);
    i32(out, 9);
    zeros(out, 101); // 五个V、s/t/u/v及w..F
    i16(out, 1);     // G_count
    i16(out, 12);
    i16(out, 34);
    zeros(out, 38);      // H..Q
    i32(out, human);     // aC
    i32(out, monster);   // aD
    zeros(out, 50);      // T..ak
    i32(out, encounter); // aE
    zeros(out, 69);      // al..ay
    return out;
}

Bytes tenant(std::int32_t uid, std::int16_t definition, bool queues) {
    Bytes out;
    i16(out, definition == 5 ? 1 : 2); // 锚x
    i16(out, 1);                       // 锚y
    i32(out, uid);
    i32(out, 0); // 同定义序号c
    i16(out, definition);
    i16(out, 0);              // e
    zeros(out, 20);           // f..j
    i16(out, queues ? 1 : 0); // k_count
    if (queues) {
        i16(out, 2);
        i16(out, 3);
        i16(out, 4);
    }
    zeros(out, 8);            // l,m,n
    i16(out, queues ? 2 : 0); // A_count，保留相同人物的两次引用
    if (queues) {
        i32(out, 10);
        i32(out, 10);
    }
    i16(out, queues ? 1 : 0); // p_count，原加载会丢弃，但审计必须消费
    if (queues) {
        i16(out, 8);
        i16(out, 9);
    }
    zeros(out, 22);           // q,r[2],s[3],t,u
    zeros(out, 96);           // v[12][2]
    i16(out, queues ? 1 : 0); // w_count
    if (queues) {
        i32(out, 6);
        i32(out, 0);
    }
    zeros(out, 12); // x,y[2]
    return out;
}

Bytes user_data() {
    Bytes out;
    for (const auto value : {1, 2, 3, 4, 40, 5, 6, 7, 8})
        i32(out, value); // c,d,e,f,P,i,j,k,l
    i32(out, 2);
    i32(out, 0);
    i32(out, 1);    // Q中的人物定义
    zeros(out, 52); // n[13]
    for (const auto value : {0, 5, 6, 2, 7, 17, 0, 0, 0, 0, 0})
        i32(out, value);             // o..y，包括r=2,s=7,t=17
    zeros(out, 480 + 8 + 1200 + 16); // z,A/B,C,D/E/F/G
    i16(out, 0);                     // H
    i16(out, 1);                     // I_count
    i16(out, 2);
    i16(out, 3);
    i16(out, 4);
    zeros(out, 56 + 48 + 6); // J,K/L,M
    i32(out, 2);             // N_count
    i16(out, 1);
    i16(out, 2);
    zeros(out, 10); // O
    return out;
}

// 固定目录按已证每条宽度准备零字段；动态段使用手写值而非审计返回值。
Parts fixture_parts() {
    Parts p;
    const std::array<std::pair<int, std::size_t>, 12> fixed = {{{1, 150},
                                                                {2, 250},
                                                                {3, 341},
                                                                {5, 432},
                                                                {6, 69},
                                                                {7, 120},
                                                                {8, 720},
                                                                {9, 300},
                                                                {10, 567},
                                                                {11, 15},
                                                                {12, 2465},
                                                                {13, 165}}};
    for (const auto &[id, length] : fixed)
        zeros(p[static_cast<std::size_t>(id)], length);
    for (int row = 0; row < 25; ++row) {
        i32(p[4], row == 0 ? 3 : 0);
        if (row == 0)
            append(p[4], {'A', 0, 255}); // 字符串是原始字节，不要求UTF-8
        zeros(p[4], 242);
    }
    i32(p[0], 1);
    for (const auto value : {3, 4, 5, 6, 7})
        i32(p[0], value);
    i32(p[0], 1);
    p[0].push_back('x');
    i32(p[0], 2);
    i32(p[0], -123);
    i32(p[0], 456);
    zeros(p[14], 36);
    // V保留IEEE754位型，不以整型坐标代替：第一个float=1.5。
    patch32(p[14], 0, 0x3fc00000);
    i32(p[15], 2);
    append(p[15], character(10, 11, 20, 30));
    append(p[15], character(11, -1, -1, -1));
    i32(p[16], 1);
    append(p[16], character(20, 10, -1, 30));
    i32(p[17], 1);
    i16(p[17], 2);
    zeros(p[17], 68); // V*4,I,H[2],H,I,H*3
    i32(p[18], 1);
    i32(p[18], 30);
    zeros(p[18], 4); // 位置
    i16(p[18], 2);   // d/state
    i16(p[18], 7);   // e
    i32(p[18], 9);   // f/counter
    i16(p[18], 0);
    i16(p[18], -1); // 原目标t未在加载二阶段恢复
    i16(p[18], 0);
    i32(p[18], 12);
    i32(p[18], 13);
    i16(p[18], 2);
    i32(p[18], 10);
    i32(p[18], 10);
    i16(p[18], 1);
    i32(p[18], 20);
    i16(p[18], 1); // group.d
    i32(p[18], 6); // group.e
    i16(p[18], 0);
    i32(p[18], 2);
    i32(p[18], 80); // group.h
    i32(p[19], 1);
    i16(p[19], 1);
    zeros(p[19], 12);
    i16(p[19], 10); // 来源人物
    i16(p[19], 20); // 目标怪物
    zeros(p[19], 46);
    i32(p[20], 1);
    i32(p[20], 40);
    i16(p[20], 3);
    i16(p[20], 0);
    zeros(p[20], 4);
    i32(p[20], 50);
    i32(p[20], 30);
    i16(p[20], 0);
    i32(p[20], 0);
    p[20].push_back(1);
    p[21] = user_data();
    i32(p[22], 1);
    for (const std::int16_t value : {5, 1, 1, 2})
        i16(p[22], value);
    zeros(p[23], 5760);
    i32(p[24], 2);
    append(p[24], tenant(50, 5, true));
    append(p[24], tenant(51, 6, false));
    return p;
}

Bytes full_container(const Parts &parts, int event_count = 200, int year = 2, int month = 3) {
    Bytes child;
    i32(child, 5);
    for (int tag = 0; tag != 5; ++tag) {
        i32(child, tag);
        i32(child, tag == 1 ? event_count : 0);
        if (tag == 1)
            zeros(child, static_cast<std::size_t>(event_count) * 4);
    }
    Bytes out;
    i32(out, 5);
    i32(out, 0);
    i32(out, 1);
    i32(out, static_cast<std::int32_t>(child.size()));
    append(out, child);
    i32(out, 1);
    i32(out, 16);
    for (const auto value : {0, year, month, 1, 10773, 10773, 0, 0, 255, -1, 0, 0, 1, 0, 0, 1})
        i32(out, value);
    i32(out, 2);
    i32(out, 3);
    i32(out, 1);
    i32(out, 12345); // long资金4294979641，超过32位，防止截窄
    zeros(out, 16);
    i32(out, 3);
    i32(out, 1);
    i32(out, 0); // 空名称
    i32(out, 4);
    i32(out, 25);
    for (const auto &part : parts) {
        i32(out, static_cast<std::int32_t>(part.size()));
        append(out, part);
    }
    return out;
}

OriginalSaveAudit audit(const Parts &parts,
                        OriginalSaveAuditProfile profile = OriginalSaveAuditProfile::apk108,
                        int event_count = 200, int year = 2, int month = 3) {
    return audit_original_save(
        inspect_original_save(full_container(parts, event_count, year, month),
                              OriginalSaveFormat::container),
        profile);
}

std::int64_t fact(const OriginalSaveAudit &result, const std::string &key) {
    const auto found = result.facts.find(key);
    check(found != result.facts.end(), "缺事实 " + key);
    return found->second;
}

const OriginalSaveReferenceAudit &reference(const OriginalSaveAudit &result, int partition,
                                            const std::string &field) {
    const auto found =
        std::find_if(result.references.begin(), result.references.end(),
                     [&](const auto &r) { return r.partition == partition && r.field == field; });
    check(found != result.references.end(),
          "缺引用诊断 " + std::to_string(partition) + "." + field);
    return *found;
}

void exact_layout_and_profiles(const Parts &parts) {
    // 固定期望来自合同手算，不使用被测解析器或夹具size()生成期望。
    const std::array<std::size_t, 25> lengths = {{41,  150, 250, 341,  6153, 432,  69,  120, 720,
                                                  300, 567, 15,  2465, 165,  36,   624, 314, 74,
                                                  66,  68,  31,  1986, 12,   5760, 394}};
    const std::array<std::size_t, 25> counts = {
        {1, 30, 50, 31, 25, 36, 23, 40, 36, 100, 81, 5, 85, 33, 1, 2, 1, 1, 1, 1, 1, 1, 1, 576, 2}};
    const auto result = audit(parts);
    check(result.partitions.size() == 25, "完整P有25个分区结果");
    for (std::size_t i = 0; i < lengths.size(); ++i) {
        const auto &part = result.partitions[i];
        const auto context = "分区" + std::to_string(i);
        check(part.partition == static_cast<int>(i), context + "索引");
        check(part.bytes == lengths[i] && part.consumed == lengths[i], context + "精确消费到尾");
        check(part.records == counts[i], context + "记录数量");
        check(part.noncanonical_booleans == 0, context + "规范布尔");
    }
    check(fact(result, "root.time") == 10773 && fact(result, "root.old_time") == 10773,
          "time/oldtime读取而不跨周归一化");
    check(fact(result, "root.year") == 2 && fact(result, "root.month") == 3 &&
              fact(result, "root.week") == 1 && fact(result, "root.cash") == 4294979641LL,
          "原索引年/月/周/64位现金保留");
    check(fact(result, "partition.15.unique_ids") == 2, "人物UID唯一数");
    check(fact(result, "partition.19.reference_audit_covered") == 0,
          "非空投射物有边界覆盖，不伪造引用语义认证");
    const auto &people = reference(result, 18, "people");
    check(people.checked == 2 && people.missing == 0, "遭遇重复引用按次数统计，不去重");
    const auto &users = reference(result, 24, "A");
    check(users.checked == 2 && users.missing == 0, "设施使用人物重复引用保留");
    check(reference(result, 24, "w").checked == 1 && reference(result, 24, "w").missing == 0,
          "设施定义/序号配对解析");
    check(reference(result, 20, "facility").missing == 0 &&
              reference(result, 20, "encounter").missing == 0,
          "任务两个独立引用字段");
    const auto steam = audit(parts, OriginalSaveAuditProfile::steam_9cf4bb10, 228);
    check(steam.partitions.size() == 25, "明确Steam profile接受228事件的同布局合成P");
    rejects([&] { audit(parts, OriginalSaveAuditProfile::apk108, 228); }, "APK不能接受228事件");
    rejects([&] { audit(parts, OriginalSaveAuditProfile::steam_9cf4bb10, 200); },
            "Steam不能接受200事件");
    rejects([&] { audit(parts, static_cast<OriginalSaveAuditProfile>(99)); }, "未知profile");
}

void malformed_partitions(const Parts &parts) {
    // 每段各剪末字节/加尾字节，证明边界归属；不写25个重复target。
    for (std::size_t id = 0; id < parts.size(); ++id) {
        auto changed = parts;
        changed[id].pop_back();
        rejects([&] { audit(changed); }, "分区" + std::to_string(id) + "截断");
        changed = parts;
        changed[id].push_back(0);
        rejects([&] { audit(changed); }, "分区" + std::to_string(id) + "尾字节");
    }
    struct BadInteger {
        std::size_t part, offset;
        bool narrow;
        const char *name;
    };
    for (const auto &field : std::array<BadInteger, 7>{{{0, 0, false, "脚本记录负数"},
                                                        {0, 24, false, "脚本文本负长"},
                                                        {0, 29, false, "脚本参数负数"},
                                                        {4, 0, false, "人物名负长"},
                                                        {15, 26, true, "人物m负数"},
                                                        {15, 28, true, "人物m行负数"},
                                                        {21, 36, false, "UserData.Q负数"}}}) {
        auto changed = parts;
        if (field.narrow)
            patch16(changed[field.part], field.offset, -1);
        else
            patch32(changed[field.part], field.offset, -1);
        rejects([&] { audit(changed); }, field.name);
    }
    auto excessive = parts;
    excessive[0].clear();
    i32(excessive[0], 10001);
    zeros(excessive[0], 10001U * 28U);
    rejects([&] { audit(excessive); }, "单动态计数10000预算，载荷充足也拒绝");
    excessive[0].clear();
    i32(excessive[0], 10000);
    for (int i = 0; i < 10000; ++i) {
        zeros(excessive[0], 24); // 五int和空串长度
        i32(excessive[0], 14);
        zeros(excessive[0], 56);
    }
    rejects([&] { audit(excessive); }, "每个计数合法但总动态项超过131072预算");
}

void diagnostics_preserve_input(const Parts &parts) {
    auto changed = parts;
    patch32(changed[15], 4 + 310 + 2, 10); // 第二人物UID重复
    patch32(changed[15], 4 + 179, 99);     // 第一人物aC，310布局的固定引用位置
    patch32(changed[18], 36, 99);          // 双方名单前两个人物均缺失，次数不得合并
    patch32(changed[18], 40, 99);
    changed[1][2] = 2; // 原boolean读法仅1为true，工具诊断不能直接拒绝
    auto inspection = inspect_original_save(full_container(changed), OriginalSaveFormat::container);
    const auto before = inspection;
    const auto result = audit_original_save(inspection, OriginalSaveAuditProfile::apk108);
    check(fact(result, "partition.15.records") == 2 &&
              fact(result, "partition.15.unique_ids") == 1 &&
              fact(result, "partition.15.duplicate_ids") == 1,
          "重复UID仅诊断，不重编号");
    const auto &people = reference(result, 18, "people");
    check(people.checked == 2 && people.missing == 2 &&
              people.missing_values == std::vector<std::int32_t>({99, 99}),
          "缺目标重复引用保留两次，不执行原加载丢弃");
    check(result.partitions[1].noncanonical_booleans == 1, "非规范bool诊断");
    check(inspection.container_bytes == before.container_bytes &&
              diff_original_saves(before, inspection).empty(),
          "审计不修改原字节或字段");
    auto many_missing = parts;
    patch16(many_missing[18], 34, 20);
    many_missing[18].erase(many_missing[18].begin() + 36, many_missing[18].begin() + 44);
    Bytes absent_ids;
    for (int i = 0; i < 20; ++i)
        i32(absent_ids, 99);
    many_missing[18].insert(many_missing[18].begin() + 36, absent_ids.begin(), absent_ids.end());
    const auto limited = audit(many_missing);
    const auto &missing = reference(limited, 18, "people");
    check(missing.checked == 20 && missing.missing == 20 && missing.missing_values.size() == 16 &&
              std::all_of(missing.missing_values.begin(), missing.missing_values.end(),
                          [](std::int32_t uid) { return uid == 99; }),
          "缺目标总数不截断，明细最多前16次");
}

// 本夹具Q有2项、I有1项，因此J起点是1858；J是2×7组年月short，不能误读I队列。
void set_tp_date(Parts &parts, std::size_t section, std::size_t row, std::int16_t year,
                 std::int16_t month) {
    const std::size_t at = 1858 + (section * 7 + row) * 4;
    patch16(parts[21], at, year);
    patch16(parts[21], at + 2, month);
}

void calendar_repair_is_only_a_candidate(const Parts &parts) {
    struct Scenario {
        std::int16_t year, month;
        int repair, candidate_year, candidate_month;
    };
    // 根2年/3月=27，早于/等于/晚于它的最大J分别独立检查。
    for (const auto &test :
         std::array<Scenario, 3>{{{2, 2, 0, 2, 3}, {2, 3, 0, 2, 3}, {3, 4, 1, 3, 4}}}) {
        auto changed = parts;
        set_tp_date(changed, 1, 2, test.year, test.month);
        const auto result = audit(changed);
        check(fact(result, "user.tp_clear.max_month_index") == test.year * 12 + test.month,
              "J的最大年月索引来自2×7个二元组");
        check(fact(result, "user.tp_clear.selected_section") == 1 &&
                  fact(result, "user.tp_clear.selected_row") == 2,
              "J最大值位置");
        check(fact(result, "user.tp_clear.repair_needed") == test.repair &&
                  fact(result, "user.tp_clear.candidate_year") == test.candidate_year &&
                  fact(result, "user.tp_clear.candidate_month") == test.candidate_month,
              "只在根年月更早时产生候选");
        check(fact(result, "root.year") == 2 && fact(result, "root.month") == 3 &&
                  fact(result, "root.week") == 1 && fact(result, "root.time") == 10773 &&
                  fact(result, "root.old_time") == 10773,
              "候选不执行年月修复或跨周");
    }
    auto tied = parts;
    set_tp_date(tied, 0, 5, 3, 4);
    set_tp_date(tied, 1, 2, 3, 4);
    const auto first = audit(tied);
    check(fact(first, "user.tp_clear.selected_section") == 0 &&
              fact(first, "user.tp_clear.selected_row") == 5,
          "同最大值保留首次严格增大位置");

    auto nonpositive = parts;
    for (std::size_t section = 0; section < 2; ++section)
        for (std::size_t row = 0; row < 7; ++row)
            set_tp_date(nonpositive, section, row, -3, 0);
    set_tp_date(nonpositive, 0, 0, -2, 3);
    const auto original_edge = audit(nonpositive, OriginalSaveAuditProfile::apk108, 200, -1, 0);
    check(fact(original_edge, "user.tp_clear.max_month_index") == 0 &&
              fact(original_edge, "user.tp_clear.selected_section") == 0 &&
              fact(original_edge, "user.tp_clear.selected_row") == 0,
          "原算法最大初始0，全部非正保留首行");
    check(fact(original_edge, "user.tp_clear.repair_needed") == 1 &&
              fact(original_edge, "user.tp_clear.candidate_year") == -2 &&
              fact(original_edge, "user.tp_clear.candidate_month") == 3 &&
              fact(original_edge, "root.year") == -1,
          "负根索引按原分支产生首行候选，不擅自美化为取真正最大值");
}

void missing_facility_pairs_keep_both_slots(const Parts &parts) {
    for (const auto pair :
         std::array<std::pair<std::int32_t, std::int32_t>, 3>{{{-1, 777}, {6, -1}, {6, 999}}}) {
        auto changed = parts;
        patch32(changed[24], 192, pair.first);  // 第一设施w[0].definition
        patch32(changed[24], 196, pair.second); // w[0].serial
        const auto result = audit(changed);
        const auto &history = reference(result, 24, "w");
        check(history.checked == 1 && history.missing == 1 && history.missing_values.empty() &&
                  history.missing_pairs == std::vector<std::pair<std::int32_t, std::int32_t>>{pair},
              "历史pair无单值-1哨兵，缺失诊断完整保留两槽");
    }
    auto repeated = parts;
    patch16(repeated[24], 190, 20); // w_count
    repeated[24].erase(repeated[24].begin() + 192, repeated[24].begin() + 200);
    Bytes keys;
    for (int i = 0; i < 20; ++i) {
        i32(keys, 6);
        i32(keys, 999);
    }
    repeated[24].insert(repeated[24].begin() + 192, keys.begin(), keys.end());
    const auto result = audit(repeated);
    const auto &history = reference(result, 24, "w");
    check(history.checked == 20 && history.missing == 20 && history.missing_pairs.size() == 16 &&
              history.missing_values.empty() &&
              std::all_of(history.missing_pairs.begin(), history.missing_pairs.end(),
                          [](const auto &pair) { return pair.first == 6 && pair.second == 999; }),
          "重复缺pair次数不合并，明细最多16对且保留序号");
}

void reject_non_game_records() {
    const auto empty = inspect_original_save({}, OriginalSaveFormat::apk_record);
    rejects([&] { audit_original_save(empty, OriginalSaveAuditProfile::apk108); }, "空档不是新局");
    // 系统J：0/16/4/12/4，手写最小容器，不把它当作游戏P。
    Bytes system;
    i32(system, 5);
    for (int tag = 0; tag < 5; ++tag) {
        i32(system, tag);
        const std::array<int, 5> n = {{0, 16, 4, 12, 4}};
        i32(system, n[static_cast<std::size_t>(tag)]);
        zeros(system,
              static_cast<std::size_t>(n[static_cast<std::size_t>(tag)]) * (tag == 2 ? 8 : 4));
    }
    const auto inspection = inspect_original_save(system, OriginalSaveFormat::container);
    rejects([&] { audit_original_save(inspection, OriginalSaveAuditProfile::apk108); },
            "系统J不是游戏P");
}
} // namespace

int original_save_audit_checks() {
    const auto parts = fixture_parts();
    exact_layout_and_profiles(parts);
    malformed_partitions(parts);
    diagnostics_preserve_input(parts);
    calendar_repair_is_only_a_candidate(parts);
    missing_facility_pairs_keep_both_slots(parts);
    reject_non_game_records();
    return checks;
}
