#pragma once

#include "dungeon_village_tools/original_save.hpp"
#include <map>
#include <utility>

namespace dungeon_village_tools {
// 显式绑定已核布局；Steam名称对应固定GameAssembly哈希前缀，不表示任意更新版。
enum class OriginalSaveAuditProfile { apk108, steam_9cf4bb10 };
struct OriginalSavePartitionAudit {
    int partition{};
    std::size_t bytes{};
    std::size_t consumed{};
    std::size_t records{};
    std::size_t noncanonical_booleans{};
};
struct OriginalSaveReferenceAudit {
    int partition{};
    std::string field;
    std::size_t checked{}; // 标量排除原-1空引用；历史pair按完整键检查，保留重复次数。
    std::size_t missing{};
    std::vector<std::int32_t> missing_values; // 有界诊断，至多前16个。
    std::vector<std::pair<std::int32_t, std::int32_t>>
        missing_pairs; // 设施历史缺失二元键，至多16个。
};
struct OriginalSaveAudit {
    std::string profile;
    std::vector<OriginalSavePartitionAudit> partitions;
    std::vector<OriginalSaveReferenceAudit> references;
    // 固定命名数值事实：数量、状态、UID唯一数及日历；不保存名字、账号或解码明文。
    std::map<std::string, std::int64_t> facts;
};
// 只审计已知布局，不执行原加载的缺引用省略、年月修复或场景更新。
// 结构截断/预算/不匹配布局抛错；缺失引用和非规范bool作为诊断，不冒充坏档判定。
OriginalSaveAudit audit_original_save(const OriginalSaveInspection &inspection,
                                      OriginalSaveAuditProfile profile);
} // namespace dungeon_village_tools
