#pragma once
#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dungeon_village_prototype::persistence_detail {
// 一个文件的当前Owner与全部审计Owner共享预算，避免独立分区放大分配。
struct CodecDecodeBudget {
    // 初始Owner约8516节点；既有244份合法审计已超过两百万，不能以百万上限截断历史。
    std::size_t nodes_remaining{16U * 1024U * 1024U};
    std::size_t allocation_bytes_remaining{128U * 1024U * 1024U};
};
std::vector<std::uint8_t> encode_state(const StartupWorldRuntimeState &state);
StartupWorldRuntimeState decode_state(const std::vector<std::uint8_t> &bytes,
                                      const StartupWorldRules &rules);
StartupWorldRuntimeState decode_state(const std::vector<std::uint8_t> &bytes,
                                      const StartupWorldRules &rules, CodecDecodeBudget &budget);
const char *codec_schema_identity();
} // namespace dungeon_village_prototype::persistence_detail
