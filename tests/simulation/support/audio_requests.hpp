#pragma once

#include "ark/simulation/startup_audio.hpp"
#include <vector>

namespace ark::simulation::test_support {
// 保留历史ID顺序oracle的显式投影；调用点另核操作资格，不提供隐式跨类型比较。
inline std::vector<int> audio_ids(const std::vector<StartupAudioRequest> &requests) {
    std::vector<int> result;
    result.reserve(requests.size());
    for (const auto &request : requests)
        result.push_back(request.id);
    return result;
}
} // namespace ark::simulation::test_support
