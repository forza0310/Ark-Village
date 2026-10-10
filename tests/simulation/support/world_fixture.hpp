#pragma once

#include "ark/simulation/world/startup_world_runtime.hpp"

namespace ark::simulation::test_support {
// 每个测试可执行文件只安装一次真实空人物新局；每次返回独立副本，不共享可写Owner。
inline StartupWorldRuntimeState
world_fixture(ref::WorldRandomStream random = ref::WorldRandomStream::from_java_seed(1)) {
    static const auto initial = [] {
        StartupSession startup;
        StartupWorldRuntimeSession runtime(startup.state(), ref::WorldRandomStream{});
        return runtime.state();
    }();
    auto s = initial;
    s.scene.random = std::move(random);
    return s;
}
// 页面明确是调用点夹具；地图、目录和其他新局字段来自真实初始化。
inline StartupWorldRuntimeState page_fixture(int raw) {
    auto s = world_fixture();
    s.scripts.pages.front().lifecycle = 3;
    ref::WorldScriptPage p;
    p.id = s.scripts.next_page_id++;
    p.kind = ref::WorldScriptPageKind::raw_page;
    p.legacy_page = raw;
    s.scripts.pages.push_back(p);
    s.scripts.executing_page = p.id;
    return s;
}
} // namespace ark::simulation::test_support
