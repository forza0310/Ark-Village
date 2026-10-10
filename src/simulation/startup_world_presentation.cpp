// 实际绘制调用的局部业务消费者；像素适配器仅消费返回计划，不访问共同随机。
#include "ark/simulation/startup_world_presentation.hpp"
#include "ark/simulation/startup_world_human.hpp"

#include <algorithm>
#include <set>

namespace ark::simulation {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
void remove_first(std::vector<std::uint64_t> &order, std::uint64_t id) {
    const auto found = std::find(order.begin(),order.end(),id);
    if (found != order.end()) order.erase(found); // 原Vector.removeElement，重复引用不全删。
}
bool clear_missing_task(State &s, ref::DungeonFinishTask &task) {
    auto &world = s.scene.world.world;
    if (!task.site || !ref::valid_legacy_map(world.map) ||
        s.surface.size() != world.map.cells.size() ||
        s.scene.world.surface.size() != world.map.cells.size() ||
        task.site->x < 0 || task.site->y < 0 || task.site->x >= world.map.width ||
        task.site->y >= world.map.height || !s.rules || s.task.encounter) return false;
    const auto ground = std::find_if(s.rules->facilities.begin(),s.rules->facilities.end(),
        [&](const auto &d) { return d.id == s.ground_definition; });
    if (ground == s.rules->facilities.end()) return false;
    const auto index = static_cast<std::size_t>(task.site->y)*world.map.width+task.site->x;
    auto &tile = world.map.cells[index];
    // 正常缺绑定单格已有空x；若仍指向活跃其它实体，拒绝维护载荷，不留半个占地。
    if (tile.facility && world.facilities.count(tile.facility->instance_id.value)) return false;
    tile.facility.reset(); tile.legacy_state = 4; tile.category = ref::RouteCategory::ground;
    auto &surface = s.surface[index];
    surface.definition = s.ground_definition; surface.updates = 0;
    surface.instance = surface.fragment = -1;
    // 原tile.a(false)保留h/i/k，不额外c/d/邻接、烟雾10或完成奖励。
    s.scene.world.surface[index] = static_cast<int>(ref::RouteCategory::ground);
    remove_first(s.task_order,task.identity);
    if (s.human_flags.size() != s.rules->humans.size()) return false;
    for (const auto &human : s.rules->humans) {
        const auto flags = s.human_flags.find(human.identity);
        if (flags == s.human_flags.end()) return false;
        flags->second &= ~std::uint32_t{2};
    }
    // 原定义位是人物派生读取事实，同步现有运行别名而不重置动作/清参与者。
    for (const auto &[id,actor] : world.ai.battle.actors) {
        if (actor.kind != ref::ActorKind::human) continue;
        const auto context = world.actors.find(id);
        if (!s.human_flags.count(actor.definition) || context == world.actors.end()) return false;
        context->second.definition_task_flag = false;
    }
    if (s.active_task) remove_first(s.task_order,*s.active_task);
    s.active_task.reset(); world.ai.task_active = false; s.task = {};
    return true;
}
bool footer(State &s, std::uint64_t page, StartupPresentationPlan &plan) {
    if (!s.active_task) return true;
    const auto task = s.tasks.find(*s.active_task);
    if (task == s.tasks.end() || task->second.identity != task->first) return false;
    const auto definition = s.task_progress.definitions.find(task->second.definition);
    if (definition == s.task_progress.definitions.end()) return false;
    const auto source=std::find_if(s.rules->tasks.begin(),s.rules->tasks.end(),
        [&](const auto &d) { return d.factory.identity==task->second.definition; });
    if (source==s.rules->tasks.end() || source->factory.kind != definition->second.kind) return false;
    if (definition->second.kind != 0) return true;
    // k.b按当前设施名单找引用：null与已退休/不再活跃的引用均走原缺绑定分支。
    if (!task->second.facility || !s.scene.world.world.facilities.count(*task->second.facility)) {
        const auto id = task->first;
        if (!clear_missing_task(s,task->second)) return false;
        plan.cleared_task = id;
        return true;
    }
    const auto facility = *task->second.facility;
    const auto progress = s.dungeon_facilities.find(facility);
    if (progress == s.dungeon_facilities.end()) return false;
    const auto &challenges = progress->second.challenges;
    for (std::size_t n = challenges.size(); n > 0; --n) {
        const auto &challenge = challenges[n-1];
        if (challenge[0]<0 || challenge[1]<0 || challenge[1]>1 || challenge[2]<0 ||
            challenge[2]>3 || challenge[3]<0 || challenge[4]<0 ||
            (challenge[1]==0 && (challenge[4]>3 || challenge[5]<0))) return false;
        if (challenge[1] != 1 || challenge[2] != 1 || challenge[3] < 10) continue;
        const auto draw = s.scene.random.draw(3);
        if (draw.error != ref::WorldRandomError::none) return false;
        plan.dungeon_jitters.push_back({page,facility,n-1,draw.ticket-1});
    }
    return true;
}
bool gift(State &s, const ref::WorldScriptPage &page, const StartupPresentationRequest &request,
          StartupPresentationPlan &plan) {
    const auto id = page.id;
    const auto counter = s.page_counters.find(id), phase = s.page_phases.find(id);
    const auto human = s.page_human_bindings.find(id);
    const auto choice = s.human_equipment_choices.find(id);
    if (!s.human_pages_initialized.count(id) || counter == s.page_counters.end() ||
        phase == s.page_phases.end() || counter->second < 0 || phase->second < 0 || phase->second > 1 ||
        human == s.page_human_bindings.end() || !startup_world_human_details(s,human->second) ||
        choice == s.human_equipment_choices.end() || choice->second[0] < 0 || choice->second[0] > 4 ||
        !s.human_page_selections.count(id) || !s.human_gift_scores.count(id) ||
        !s.human_gift_messages.count(id)) return false;
    const int slot = choice->second[0], definition = choice->second[1];
    if (slot == 4) {
        if (std::none_of(s.rules->items.begin(),s.rules->items.end(),
                        [=](const auto &d) { return d.identity == definition; })) return false;
    } else {
        const int kind = slot == 0 ? 1 : slot == 3 ? 3 : 2;
        if (std::none_of(s.rules->equipment.begin(),s.rules->equipment.end(),
                        [=](const auto &d) { return d.shop.kind == kind && d.shop.id == definition; })) return false;
    }
    if (counter->second == 45 && !request.application_preview && !request.sound_paused) {
        s.sound_requests.push_back({StartupAudioOperation::ordinary_play, 8}); ++plan.sound_requests;
    }
    return true;
}
} // namespace
std::optional<std::vector<std::uint64_t>> startup_world_presentation_pages(
    const State &s, StartupPresentationMode mode) {
    if ((mode != StartupPresentationMode::full_redraw && mode != StartupPresentationMode::top_only) ||
        s.scripts.pages.empty() || s.scripts.executing_page || s.scripts.page_mutations_locked) return {};
    std::set<std::uint64_t> ids;
    for (const auto &page : s.scripts.pages)
        if (!page.id || page.id >= s.scripts.next_page_id || !ids.insert(page.id).second ||
            page.lifecycle < 0 || page.lifecycle > 4 ||
            page.kind < ref::WorldScriptPageKind::scene || page.kind > ref::WorldScriptPageKind::raw_page)
            return {};
    std::vector<std::uint64_t> result;
    for (std::size_t n=0;n<s.scripts.pages.size();++n) {
        const auto &page=s.scripts.pages[n];
        if (mode == StartupPresentationMode::top_only && n+1 != s.scripts.pages.size()) continue;
        if (page.lifecycle != 0 && page.lifecycle != 4) result.push_back(page.id);
    }
    return result;
}
StartupPresentationResult prepare_startup_world_presentation(
    const State &s, const StartupPresentationRequest &request) {
    const auto pages = startup_world_presentation_pages(s,request.mode);
    if (!request.ordinal || !s.rules || !pages || *pages != request.expected_pages)
        return {Error::invalid_page,{},{}};
    if (request.gift_wrapper_ready) {
        const auto &top=s.scripts.pages.back();
        if (top.kind != ref::WorldScriptPageKind::raw_page || top.legacy_page != 66 ||
            std::find(pages->begin(),pages->end(),top.id)==pages->end())
            return {Error::invalid_page,{},{}};
    }
    auto next = s;
    StartupPresentationPlan plan;
    plan.ordinal=request.ordinal; plan.pages=*pages; plan.random_before=s.scene.random.draws();
    // 不保存iterator/执行页根。后项失败也丢弃此前随机/清理/声音和全部计划。
    for (const auto id : *pages) {
        const auto page=std::find_if(next.scripts.pages.begin(),next.scripts.pages.end(),
                                     [=](const auto &p) { return p.id==id; });
        if (page == next.scripts.pages.end()) return {Error::invalid_page,{},{}};
        if (page->kind == ref::WorldScriptPageKind::scene) {
            if (!footer(next,id,plan)) return {Error::missing_source,{},{}};
        } else if (page->kind == ref::WorldScriptPageKind::raw_page && page->legacy_page == 66 &&
                   request.gift_wrapper_ready && id == next.scripts.pages.back().id) {
            if (!gift(next,*page,request,plan)) return {Error::missing_source,{},{}};
        }
    }
    plan.random_after=next.scene.random.draws();
    return {Error::none,std::move(next),std::move(plan)};
}
StartupPresentationResult StartupWorldRuntimeSession::present(const StartupPresentationRequest &request) {
    auto result=prepare_startup_world_presentation(state_,request);
    if (result.candidate) state_=*result.candidate;
    return result;
}
} // namespace ark::simulation
