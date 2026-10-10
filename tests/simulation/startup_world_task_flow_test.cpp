#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"

#include <algorithm>
#include <charconv>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

using namespace ark::simulation;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
// 无窗口自然轨迹也须消费一次性输出；只检查并统计请求，不模拟播放器或改动世界业务。
struct HeadlessAudioSink {
    std::uint64_t count{};
    std::size_t peak{};
    void consume(StartupWorldRuntimeState &s) {
        for (const auto &request : s.sound_requests) {
            const int operation = static_cast<int>(request.operation);
            check(operation >= 0 && operation <= 2,
                  "natural task audio output operation stays within typed contract");
            check(request.id >= 0 && request.id <= 25,
                  "natural task audio output ID stays within original 26-entry table");
        }
        peak = std::max(peak, s.sound_requests.size());
        count += s.sound_requests.size();
        s.sound_requests.clear();
    }
};
const ref::WorldScriptPage &top(const StartupWorldRuntimeState &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                [](const auto &v) { return v.lifecycle != 4; });
    if (p == s.scripts.pages.rend())
        throw std::runtime_error("lost main page");
    return *p;
}
std::string describe(const StartupWorldRuntimeState &s, int frame) {
    std::ostringstream out;
    out << "frame=" << frame << " date=" << s.scene.calendar.year << '/' << s.scene.calendar.month
        << " raw=" << top(s).legacy_page << " cash=" << s.scene.world.world.ai.accounting.funds()
        << " successes=" << s.task_progress.successes << " active=" << s.active_task.value_or(0)
        << " draws=" << s.scene.random.draws();
    for (const auto &p : s.dungeon_facilities)
        if (s.scene.world.world.facilities.at(p.first).category == 5)
            out << " dungeon=" << p.first << ':' << p.second.progress << '/' << p.second.extent
                << ":state" << s.scene.world.world.facilities.at(p.first).status;
    for (const auto id : s.scene.world.world.ai.human_order) {
        const auto &a = s.scene.world.world.ai.battle.actors.at(id);
        out << " human=" << a.definition << ":state" << a.control.state << ":action"
            << a.control.action;
        if (!a.control.queue.empty())
            out << ":op" << a.control.queue.front().front();
    }
    if (s.task.encounter) {
        const auto event = s.scene.world.world.ai.encounters.find(*s.task.encounter);
        if (event != s.scene.world.world.ai.encounters.end())
            out << " battle=" << *s.task.encounter << ":spawned" << event->second.runtime.spawned
                << ":linked" << event->second.linked_monsters << ":quota"
                << event->second.runtime.quota;
    }
    out << " monsters=" << s.scene.world.world.ai.monster_order.size();
    return out.str();
}
std::string diagnose(const StartupWorldRuntimeState &s) {
    auto adapter = startup_world_runtime_adapter();
    std::string last;
    auto admitted = s;
    admitted.scripts.pages.erase(std::remove_if(admitted.scripts.pages.begin(),
                                                admitted.scripts.pages.end(),
                                                [](const auto &p) { return p.lifecycle == 4; }),
                                 admitted.scripts.pages.end());
    admitted.scene.top_is_main = top(admitted).kind == ref::WorldScriptPageKind::scene;
    admitted.scripts.executing_page = top(admitted).id;
    const auto arrival = adapter.arrival;
    adapter.arrival = [&](const auto &owner) {
        last = "arrival";
        auto value = arrival(owner);
        if (!value)
            last += " failed";
        return value;
    };
    const auto before_common = adapter.before_common;
    adapter.before_common = [&](const auto &owner) {
        last = "before_common";
        return before_common(owner);
    };
    const auto other = adapter.nonactors.other;
    adapter.nonactors.other = [&](const auto &owner, const auto &call, const auto &field) {
        last = "nonactor=" + std::to_string(static_cast<int>(call.stage));
        return other(owner, call, field);
    };
    const auto decision = adapter.actors.decision;
    adapter.actors.decision = [&](const auto &owner, auto id) {
        last = "decision=" + std::to_string(id.value);
        auto input = decision(owner, id);
        const auto original_input = input;
        if (!input)
            last += " missing";
        else {
            auto routes = startup_world_runtime_routes(owner);
            auto private_owner = owner;
            input->event = [&](const auto &projected,
                               int event) -> std::optional<ref::WorldActorRoutesState> {
                if (!write_startup_world_runtime_routes(private_owner, projected))
                    return {};
                const auto consumed = adapter.actors.event(private_owner, event);
                last +=
                    " event=" + std::to_string(event) + ':' + std::to_string(consumed.has_value());
                if (!consumed)
                    return {};
                private_owner = *consumed;
                return startup_world_runtime_routes(private_owner);
            };
            input->presentation =
                [&](const auto &projected,
                    const auto &request) -> std::optional<ref::WorldActorRoutesState> {
                if (!write_startup_world_runtime_routes(private_owner, projected))
                    return {};
                const auto consumed = adapter.actors.presentation(private_owner, request);
                last += " presentation=" + std::to_string(consumed.has_value());
                if (request.shop)
                    last += ":shop" + std::to_string(static_cast<int>(request.shop->kind));
                if (!consumed)
                    return {};
                private_owner = *consumed;
                return startup_world_runtime_routes(private_owner);
            };
            input->encounter =
                [&](const auto &projected,
                    const auto &request) -> std::optional<ref::WorldActorRoutesState> {
                if (!write_startup_world_runtime_routes(private_owner, projected))
                    return {};
                const auto consumed = adapter.actors.encounter(private_owner, request);
                last += " encounter=" + std::to_string(consumed.has_value());
                if (!consumed)
                    return {};
                private_owner = *consumed;
                return startup_world_runtime_routes(private_owner);
            };
            const auto result = ref::prepare_world_actor_decision(routes, *input);
            last += " domain=" + std::to_string(static_cast<int>(result.error));
            const auto &path = routes.world.actors.at(id);
            if (path.binding)
                last += " bound=" + std::to_string(path.binding->instance_id.value) + ":def" +
                        std::to_string(path.binding->definition_id) + ":goal" +
                        std::to_string(path.binding->goal.x) + ',' +
                        std::to_string(path.binding->goal.y);
            const auto cell = routes.world.ai.contexts.at(id).cell;
            last += " cell=" + std::to_string(cell.x) + ',' + std::to_string(cell.y);
        }
        return original_input;
    };
    const auto command = adapter.actors.owned_command;
    adapter.actors.owned_command = [&](const auto &owner, const auto &routes, auto id,
                                       const auto &op) {
        last = "command=" + std::to_string(id.value) + ":op" + std::to_string(op.front());
        const auto input = command(owner, routes, id, op);
        if (!input)
            last += " missing";
        else {
            const auto result = ref::prepare_world_actor_control(
                routes, id, [input](const auto &, auto, const auto &) { return input; });
            last += " domain=" + std::to_string(static_cast<int>(result.error)) +
                    " control=" + std::to_string(static_cast<int>(result.control_error));
        }
        return input;
    };
    const auto facility = adapter.facility;
    adapter.facility = [&](const auto &owner, const auto &request) {
        last = "facility=" + std::to_string(static_cast<int>(request.kind));
        auto result = facility(owner, request);
        if (!result)
            last += " failed";
        return result;
    };
    (void)ref::prepare_owned_world_runtime(admitted, {s.calendar_advance, true}, adapter);
    return last;
}
// 真实新局+显式玩家菜单选择；世界生成、招募随机、人物AI、挑战及奖励均保持自然路径。
void natural_flow(std::uint64_t seed, int speed) {
    StartupSession initial;
    StartupWorldRuntimeSession installed(initial.state(),
                                         ref::WorldRandomStream::from_java_seed(seed));
    auto s = installed.state();
    s.scene.speed_setting = speed;
    std::set<std::uint64_t> accepted;
    std::set<std::uint64_t> departed;
    std::set<std::uint64_t> generated;
    std::set<int> success_kinds;
    std::map<std::uint64_t, std::vector<ref::Position>> original_cells;
    bool later_task{};
    std::optional<std::uint64_t> followup_task;
    int last_success{};
    HeadlessAudioSink audio;
    for (int frame = 0; frame < 150000; ++frame) {
        const auto prior_active = s.active_task;
        auto r = prepare_startup_world_runtime(s);
        if (!r.candidate) {
            throw std::runtime_error(
                "natural task runtime failed error=" + std::to_string(static_cast<int>(r.error)) +
                " scene=" + std::to_string(static_cast<int>(r.scene_error)) +
                " world=" + std::to_string(static_cast<int>(r.world_error)) + ' ' +
                describe(s, frame) + " last=" + diagnose(s));
        }
        s = std::move(*r.candidate);
        check(update_startup_world_render_cache(s), "render cache uses same committed world");
        for (const auto id : s.task_order)
            if (generated.insert(id).second) {
                std::cout << "generated task=" << id << " definition=" << s.tasks.at(id).definition
                          << ' ' << describe(s, frame) << std::endl;
                if (last_success > 0) {
                    later_task = true;
                    followup_task = id;
                }
            }
        if (s.task_progress.successes != last_success) {
            check(s.task_progress.successes == last_success + 1,
                  "natural completion increments successes once");
            last_success = s.task_progress.successes;
            later_task = false; // 必须在最近一次成功以后再由真实任务工厂生成，不复用旧观察。
            followup_task.reset();
            check(!s.active_task && !s.scene.world.world.ai.task_active,
                  "natural success clears active task and derived AI flag together");
            check(prior_active && departed.count(*prior_active) &&
                      std::find(s.task_order.begin(), s.task_order.end(), *prior_active) ==
                          s.task_order.end(),
                  "success belongs to this actual departed task, never an earlier aborted task");
            const auto &task = s.tasks.at(*prior_active);
            success_kinds.insert(s.task_progress.definitions.at(task.definition).kind);
            if (task.facility) {
                check(!s.scene.world.world.facilities.count(*task.facility),
                      "success removes actual exploration facility");
                for (const auto cell : original_cells.at(*prior_active)) {
                    const auto &map = s.scene.world.world.map;
                    const auto index = static_cast<std::size_t>(cell.y * map.width + cell.x);
                    check(s.surface.at(index).definition == s.ground_definition &&
                              !map.cells.at(index).facility,
                          "every occupied tile restored to true ground with no orphan binding");
                }
            }
            std::cout << "natural success " << describe(s, frame) << std::endl;
        }
        const auto page = top(s);
        const bool aftermath_done =
            std::none_of(s.scripts.continuations.begin(), s.scripts.continuations.end(),
                         [](const auto &continuation) {
                             return continuation.event == 126 || continuation.event == 128 ||
                                    continuation.event == 201 || continuation.event == 205 ||
                                    continuation.event == 92 || continuation.event == 80;
                         });
        if (later_task && success_kinds.count(0) && success_kinds.count(1) &&
            page.kind == ref::WorldScriptPageKind::scene && !s.active_task && aftermath_done) {
            check(s.task_progress.successes >= 2 && accepted.size() >= 2 && departed.size() >= 2,
                  "both kinds naturally succeed, result pages return, and later tasks generate");
            const auto cash = s.scene.world.world.ai.accounting.funds();
            const auto draws = s.scene.random.draws();
            check(followup_task &&
                      open_startup_world_runtime_task_menu(s) == StartupWorldRuntimeError::none,
                  "completed world really reopens task menu for a later generated task");
            const auto menu = top(s);
            const auto &list = s.task_page_lists.at(menu.id);
            const auto entry = std::find(list.begin(), list.end(), *followup_task);
            check(entry != list.end(), "followup retains its actual new task identity in menu");
            check(act_startup_world_runtime_task_page(s, menu.id, StartupWorldTaskAction::confirm,
                                                      static_cast<int>(entry - list.begin()))
                              .error == StartupWorldRuntimeError::none &&
                      top(s).task_identity == followup_task,
                  "player opens actual followup offer, not a retired previous-task page");
            check(
                act_startup_world_runtime_task_page(s, top(s).id, StartupWorldTaskAction::cancel)
                            .error == StartupWorldRuntimeError::none &&
                    act_startup_world_runtime_task_page(s, menu.id, StartupWorldTaskAction::cancel)
                            .error == StartupWorldRuntimeError::none &&
                    top(s).kind == ref::WorldScriptPageKind::scene && !s.active_task &&
                    s.scene.world.world.ai.accounting.funds() == cash &&
                    s.scene.random.draws() == draws,
                "explicit followup cancellation returns real scene without fee or random");
            audio.consume(s); // 成功提前返回也在最终玩家动作完成后领取，不留下待消费输出。
            std::cout << "task flow summary seed=" << seed << " speed=" << speed
                      << " accepted=" << accepted.size() << " departed=" << departed.size()
                      << " generated=" << generated.size() << " audio_count=" << audio.count
                      << " audio_peak=" << audio.peak << ' ' << describe(s, frame) << '\n';
            return;
        }
        const auto affordable =
            std::find_if(s.task_order.begin(), s.task_order.end(), [&](const auto id) {
                return s.rules->tasks.at(s.tasks.at(id).definition).recruitment_fee <=
                       s.scene.world.world.ai.accounting.funds();
            });
        // 两类自然成功以后只等待真实后续任务，不再启动新任务延长验收链。
        if (page.kind == ref::WorldScriptPageKind::scene && !s.active_task &&
            !(success_kinds.count(0) && success_kinds.count(1)) &&
            affordable != s.task_order.end()) {
            check(open_startup_world_runtime_task_menu(s) == StartupWorldRuntimeError::none,
                  "explicit player opens real task menu");
        } else if (page.legacy_page == 22) {
            const auto &list = s.task_page_lists.at(page.id);
            check(affordable != s.task_order.end(), "player menu entry remains affordable");
            const auto selected = std::find(list.begin(), list.end(), *affordable);
            check(selected != list.end(), "player selection is actual affordable list entry");
            check(act_startup_world_runtime_task_page(s, page.id, StartupWorldTaskAction::confirm,
                                                      static_cast<int>(selected - list.begin()))
                          .error == StartupWorldRuntimeError::none,
                  "player chooses actual task list entry");
        } else if (page.legacy_page == 23) {
            const auto id = *page.task_identity;
            const auto before = s.scene.world.world.ai.accounting.funds();
            auto cancelled = s;
            const auto draws = s.scene.random.draws();
            check(act_startup_world_runtime_task_page(cancelled, page.id,
                                                      StartupWorldTaskAction::cancel)
                              .error == StartupWorldRuntimeError::none &&
                      cancelled.scene.world.world.ai.accounting.funds() == before &&
                      cancelled.scene.random.draws() == draws && !cancelled.active_task,
                  "cancelling original offer consumes no fee/random/task activation");
            auto blocked = s;
            blocked.scripts.page_mutations_locked = true;
            check(
                act_startup_world_runtime_task_page(blocked, page.id,
                                                    StartupWorldTaskAction::confirm)
                            .error == StartupWorldRuntimeError::missing_source &&
                    blocked.scene.world.world.ai.accounting.funds() == before &&
                    blocked.scene.random.draws() == draws && !blocked.active_task &&
                    top(blocked).id == page.id,
                "late page consumer failure rolls back fee and recruitment random with same Owner");
            const auto result =
                act_startup_world_runtime_task_page(s, page.id, StartupWorldTaskAction::confirm);
            check(result.error == StartupWorldRuntimeError::none && result.accepted,
                  "real new-game cash pays original recruitment fee");
            check(accepted.insert(id).second && !s.active_task &&
                      before - s.scene.world.world.ai.accounting.funds() ==
                          s.rules->tasks.at(s.tasks.at(id).definition).recruitment_fee,
                  "accept charges once and does not prematurely activate task");
            const auto paid = s.scene.world.world.ai.accounting.funds();
            check(act_startup_world_runtime_task_page(s, page.id, StartupWorldTaskAction::confirm)
                              .error == StartupWorldRuntimeError::invalid_page &&
                      s.scene.world.world.ai.accounting.funds() == paid,
                  "stale offer retry cannot charge original fee twice");
        } else if (page.legacy_page == 25) {
            check(!s.participants.empty() && s.participants.size() <= 9 && !s.active_task,
                  "page24 really recruits eligible definitions before team confirmation");
            const auto id = *page.task_identity;
            if (s.tasks.at(id).facility)
                original_cells[id] = s.sites.at(*s.tasks.at(id).facility).occupied_cells;
            check(act_startup_world_runtime_task_page(s, page.id, StartupWorldTaskAction::depart)
                          .error == StartupWorldRuntimeError::none,
                  "player opens departure prompt");
        } else if (page.legacy_page == 28 && s.page_phases[page.id] == 0) {
            const auto id = *page.task_identity;
            check(act_startup_world_runtime_task_page(s, page.id, StartupWorldTaskAction::confirm)
                          .error == StartupWorldRuntimeError::none,
                  "player confirms departure animation");
            check(!s.active_task && departed.insert(id).second,
                  "confirmation starts animation, not immediate activation");
        } else if (page.legacy_page == 33) {
            const int selection =
                s.scene.world.world.ai.accounting.funds() >= page.legacy_f ? 0 : 1;
            check(act_startup_world_runtime_task_page(s, page.id, StartupWorldTaskAction::confirm,
                                                      selection)
                          .error == StartupWorldRuntimeError::none,
                  "explicit player renews when affordable or chooses source abort, no cash "
                  "injection");
        } else if (page.kind != ref::WorldScriptPageKind::scene && page.legacy_page != 24 &&
                   page.legacy_page != 28 && page.legacy_page != 56 && page.legacy_page != 57 &&
                   page.legacy_page != 16 && page.legacy_page != 97) {
            auto error = StartupWorldRuntimeError::none;
            const auto before_cash = s.scene.world.world.ai.accounting.funds();
            const auto before_success = s.task_progress.successes;
            if (page.legacy_page == 87) {
                error = act_startup_world_runtime_award_page(
                    s, page.id, ref::WorldAwardAction::request_termination);
                if (error == StartupWorldRuntimeError::none)
                    error = act_startup_world_runtime_award_page(
                        s, page.id, ref::WorldAwardAction::confirm_termination);
            } else if (page.legacy_page == 83)
                error = cancel_startup_world_runtime_page(s, page.id);
            else
                error = acknowledge_startup_world_runtime_page(s, page.id);
            if (error != StartupWorldRuntimeError::none)
                throw std::runtime_error("player page acknowledgement failed " +
                                         describe(s, frame));
            if (page.legacy_page == 30 || page.legacy_page == 31 || page.legacy_page == 32)
                check(
                    s.scene.world.world.ai.accounting.funds() == before_cash &&
                        s.task_progress.successes == before_success,
                    "real result confirmation never pays reward or commits success a second time");
        }
        audio.consume(s); // update和本轮玩家命令均完成后消费，同轮次序保留而不跨轮积压。
        if (frame % 5000 == 0)
            std::cout << "progress " << describe(s, frame) << std::endl;
    }
    throw std::runtime_error("natural flow limit reached " + describe(s, 150000));
}
} // namespace
int main(int argc, const char **argv) {
    try {
        std::uint64_t seed = 1;
        int speed = 1;
        const auto parse = [](const char *text, auto &value) {
            const std::string input(text);
            const auto result = std::from_chars(input.data(), input.data() + input.size(), value);
            if (result.ec != std::errc{} || result.ptr != input.data() + input.size())
                throw std::invalid_argument("invalid task flow argument");
        };
        if (argc > 3)
            throw std::invalid_argument("expected [seed [speed]]");
        if (argc >= 2)
            parse(argv[1], seed);
        if (argc == 3)
            parse(argv[2], speed);
        if (speed != 0 && speed != 1)
            throw std::invalid_argument("speed must be zero or one");
        natural_flow(seed, speed);
        std::cout << "startup world task flow checks: " << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
