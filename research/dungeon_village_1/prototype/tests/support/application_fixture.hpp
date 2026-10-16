#pragma once

#include "dungeon_village_prototype/startup_application.hpp"
#include <utility>

namespace dungeon_village_prototype {
// 测试显式选取两边均大于480的环境；640是夹具输入，不冒称原Steam外框/DPI或默认尺寸。
// 正常宿主必须自行提供真实应用语义的width/height，缺值时start_game仍拒绝。
// 一次夹具开始命令将显式环境与开始放在同一临时候选，失败不改已有比较基线。
inline std::string start_application_fixture_game(StartupApplication &application) {
    auto candidate=application;
    auto error=candidate.set_new_game_environment({640,640});
    if(error.empty())error=candidate.start_game();
    if(error.empty())application=std::move(candidate);
    return error;
}
} // namespace dungeon_village_prototype
