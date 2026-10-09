#pragma once

#include "dungeon_village_prototype/startup_audio.hpp"
#include <vector>

namespace dungeon_village_prototype::test_support {
// 保留历史ID顺序oracle的显式投影；调用点另核操作资格，不提供隐式跨类型比较。
inline std::vector<int> audio_ids(const std::vector<StartupAudioRequest> &requests) {
    std::vector<int> result;
    result.reserve(requests.size());
    for (const auto &request : requests)
        result.push_back(request.id);
    return result;
}
} // namespace dungeon_village_prototype::test_support
