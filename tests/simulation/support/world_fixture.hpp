#pragma once

#include "ark/simulation/world/startup_world_runtime.hpp"
#include <stdexcept>

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
    if (raw == 60 || (raw >= 48 && raw <= 54) || raw == 90 || raw == 95) p.lifecycle = 0; // 新管理页须先经过真实Init。
    s.scripts.pages.push_back(p);
    if (raw == 60) s.human_detail_contexts.emplace(p.id, StartupHumanDetailContext{});
    s.scripts.executing_page = p.id;
    return s;
}
// 页面／codec共用的调用点夹具：只准备人物4已开放、九级与经验门槛，非自然经营。
// 实例、成长请求、70/94及职业开放均由现有真实消费者产生，不手造奖励页。
inline StartupWorldRuntimeState mastery_fixture() {
    auto s = world_fixture();
    s.arrival_counter = 1;
    s.human_presence.at(4) = 1;
    s.human_calendar.at(4).absent_months = 100;
    for (int event : {89, 109, 113, 119})
        s.scripts.event_calls[event] = 1; // 首次说明已经看过，避免遮住被测大师链。
    const auto adapter = startup_world_runtime_adapter();
    const auto arrived = adapter.arrival(s);
    if (!arrived || arrived->scene.world.world.ai.human_order.size() != 1)
        throw std::runtime_error("大师夹具：真实到访未创建唯一人物");
    s = *arrived;
    const auto actor = s.scene.world.world.ai.human_order.front();
    auto &ai = s.scene.world.world.ai;
    if (ai.battle.actors.at(actor).definition != 4 ||
        ai.growth.at(4).definition.current_profession != 4 || ai.professions.at(4).unlocked)
        throw std::runtime_error("大师夹具：人物4的原初战士职业及未开放状态失配");
    ai.growth.at(4).definition.profession_levels.at(4) = 9;
    ai.growth.at(4).experience = 284; // 原战士难度1，九级阈值3*9*9+4*9+5。
    ai.growth.at(4).pending = {1, 0};
    const auto growth = ref::prepare_actor_growth_commit(ai, actor);
    if (!growth.candidate)
        throw std::runtime_error("大师夹具：真实成长提交失败");
    ai = growth.candidate->state;
    s.scripts.executing_page = s.scripts.pages.front().id; // 原共同世界回调的插页锚点。
    auto result = adapter.prefix_effects(s, {actor, {}, growth.candidate->growth_requests});
    if (!result)
        throw std::runtime_error("大师夹具：Owner未消费实际成长请求");
    result->scripts.executing_page.reset();
    const auto framed = prepare_startup_world_runtime(*result);
    if (!framed.candidate)
        throw std::runtime_error("大师夹具：真实框架未初始化大师页面");
    return *framed.candidate;
}
} // namespace ark::simulation::test_support
