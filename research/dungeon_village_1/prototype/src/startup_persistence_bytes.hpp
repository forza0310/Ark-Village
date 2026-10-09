#pragma once

#include "dungeon_village_prototype/startup_system_records.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"

namespace dungeon_village_prototype::persistence_detail {
// 研究容器的内部字节桥：复用原格式及完整校验，不读写文件、不安装应用或推进世界。
// 编码先由同一解码器自检；失败抛具名异常，调用方负责发布前处理。
std::vector<std::uint8_t>
encode_world_session_bytes(const StartupWorldRuntimeSession &, const StartupWorldSaveMetadata &);
StartupWorldSavedSession decode_world_session_bytes(std::vector<std::uint8_t>,
                                                     const StartupWorldRules &,
                                                     StartupWorldSavePurpose,
                                                     const std::string &controller);
std::vector<std::uint8_t> encode_system_records_bytes(const StartupSystemRecords &);
StartupSystemRecords decode_system_records_bytes(std::vector<std::uint8_t>);
} // namespace dungeon_village_prototype::persistence_detail
