#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_reference/world_arrivals.hpp"

#include <algorithm>
#include <charconv>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

using namespace dungeon_village_prototype;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
const ref::WorldScriptPage *top_page(const StartupWorldRuntimeState &s) {
    const auto found = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                    [](const auto &p) { return p.lifecycle != 4; });
    return found == s.scripts.pages.rend() ? nullptr : &*found;
}
std::string snapshot(const StartupWorldRuntimeState &s, int frame) {
    std::ostringstream out;
    const auto &date = s.scene.calendar;
    const auto &world = s.scene.world.world;
    out << "frame=" << frame << " date=" << date.year << '/' << date.month << '/' << date.subperiod
        << '/' << date.units << " month_tick=" << date.month_ticks
        << " cash=" << world.ai.accounting.funds() << " scene=" << s.scene.scene_state
        << " B=" << s.arrival_counter << " humans=" << world.ai.human_order.size()
        << " tasks=" << s.task_order.size() << " random=" << s.scene.random.draws()
        << " report=" << s.report_state << '/' << s.report_counter;
    for (auto id : world.ai.human_order) {
        const auto &a = world.ai.battle.actors.at(id);
        out << " actor=" << id.value << ":def" << a.definition << ":state" << a.control.state
            << ":action" << a.control.action << ":B" << a.state_counter << ":cell"
            << world.ai.contexts.at(id).cell.x << ',' << world.ai.contexts.at(id).cell.y;
        if (!a.control.queue.empty())
            out << ":op" << a.control.queue.front().front();
    }
    return out.str();
}
std::string diagnose(const StartupWorldRuntimeState &s) {
    auto adapter = startup_world_runtime_adapter();
    std::string last;
    const auto arrival = adapter.arrival;
    adapter.arrival = [&](const auto &owner) {
        last = "arrival";
        auto result = arrival(owner);
        if (!result) {
            ref::WorldArrivalsState projected;
            projected.finish = startup_world_runtime_finish(owner);
            projected.scripts = startup_world_runtime_scripts(owner);
            projected.random = owner.scene.random;
            projected.spawn_cells = owner.scene.world.spawn_cells;
            projected.arrival_counter = owner.arrival_counter;
            projected.camera_delay = owner.camera_delay;
            projected.camera_follow = owner.camera_follow;
            for (const auto &h : owner.rules->humans) {
                const auto &spells =
                    owner.scene.world.world.ai.growth.at(h.identity).derived.available_spells;
                projected.definitions.push_back({h.identity, owner.human_presence.at(h.identity),
                                                 owner.human_calendar.at(h.identity).absent_months,
                                                 owner.human_flags.at(h.identity),
                                                 std::vector<bool>(spells.begin(), spells.end())});
            }
            const auto probe = ref::prepare_world_arrivals(projected, adapter.catalog);
            last += " domain-error=" + std::to_string(static_cast<int>(probe.error));
            if (probe.candidate) {
                auto private_owner = owner;
                const bool finish = write_startup_world_runtime_finish(
                    private_owner, probe.candidate->state.finish);
                const bool scripts = finish && write_startup_world_runtime_scripts(
                                                   private_owner, probe.candidate->state.scripts);
                last += " finish-write=" + std::to_string(finish) +
                        " scripts-write=" + std::to_string(scripts);
            }
            const auto script_probe =
                ref::prepare_world_script_continuations(adapter.catalog, projected.scripts, false);
            last += " script-validate=" + std::to_string(static_cast<int>(script_probe.error));
            last += " eventcalls-match=" +
                    std::to_string(projected.finish.event_calls == projected.scripts.event_calls);
            last += " pending-match=" +
                    std::to_string(projected.finish.dungeon.world.ai.pending_completion ==
                                   projected.scripts.pending_completion);
            for (auto id : projected.finish.dungeon.world.ai.human_order) {
                const auto &a = projected.finish.dungeon.world.ai.battle.actors.at(id);
                last += " roster=" + std::to_string(id.value) +
                        ":actor-id=" + std::to_string(a.id.value) +
                        ":kind=" + std::to_string(static_cast<int>(a.kind)) +
                        ":uid=" + std::to_string(a.legacy_id);
            }
        }
        return result;
    };
    const auto decision = adapter.actors.decision;
    adapter.actors.decision = [&](const auto &owner, auto id) {
        last = "decision-input actor=" + std::to_string(id.value);
        const auto &actor = owner.scene.world.world.ai.battle.actors.at(id);
        last += " state=" + std::to_string(actor.control.state) +
                " action=" + std::to_string(actor.control.action) +
                " kind=" + std::to_string(static_cast<int>(actor.kind)) +
                " metadata=" + std::to_string(owner.actor_metadata.count(id));
        auto input = decision(owner, id);
        if (!input)
            last += " provider=null";
        else if (actor.kind == ref::ActorKind::monster) {
            const auto routes = startup_world_runtime_routes(owner);
            auto probing_input = *input;
            // 调度在decision-provider之后才安装Owner事件消费者，诊断必须做同样的桥接。
            probing_input.event = [&](const auto &event_routes,
                                      int event) -> std::optional<ref::WorldActorRoutesState> {
                auto private_owner = owner;
                if (!adapter.actors.event ||
                    !write_startup_world_runtime_routes(private_owner, event_routes))
                    return {};
                const auto consumed = adapter.actors.event(private_owner, event);
                last += " owned-event=" + std::to_string(event) +
                        " consumed=" + std::to_string(consumed.has_value());
                if (!consumed)
                    return {};
                const auto result = startup_world_runtime_routes(*consumed);
                last +=
                    " battle-seen=" + std::to_string(result.world.ai.battle.events.count(event)) +
                    " script-seen=" +
                    std::to_string(ref::world_script_seen(consumed->scripts, event));
                return result;
            };
            const auto probe = ref::prepare_world_actor_decision(routes, probing_input);
            last += " route-error=" + std::to_string(static_cast<int>(probe.error));
            if (actor.control.state == 8 || actor.control.state == 9) {
                auto daily = probing_input.daily;
                daily.actor = id;
                daily.event = [&](const auto &world,
                                  int event) -> std::optional<ref::RescueWorldState> {
                    last += " daily-event=" + std::to_string(event);
                    if (!probing_input.event)
                        return {};
                    auto event_routes = routes;
                    event_routes.world = world;
                    const auto consumed = probing_input.event(event_routes, event);
                    last += " event-consumer=" + std::to_string(consumed.has_value());
                    return consumed ? std::optional<ref::RescueWorldState>(consumed->world)
                                    : std::nullopt;
                };
                const auto daily_probe =
                    ref::prepare_world_daily_c({routes.world, routes.facts, routes.task}, daily);
                last += " daily-error=" + std::to_string(static_cast<int>(daily_probe.error)) +
                        " counter=" + std::to_string(actor.state_counter) +
                        " mode=" + std::to_string(routes.world.actors.at(id).monster_mode);
            }
        }
        return input;
    };
    const auto popularity = adapter.popularity.read;
    adapter.popularity.read = [&](const auto &owner) {
        last = "popularity-read value=" + std::to_string(owner.popularity);
        return popularity(owner);
    };
    const auto write_popularity = adapter.popularity.write;
    adapter.popularity.write = [&](auto &owner, const auto &result) {
        last = "popularity-write value=" + std::to_string(result.popularity);
        return write_popularity(owner, result);
    };
    const auto command = adapter.actors.owned_command;
    adapter.actors.owned_command = [&](const auto &owner, const auto &routes, auto id,
                                       const auto &op) {
        last = "control-input actor=" + std::to_string(id.value) +
               " opcode=" + std::to_string(op.front());
        return command(owner, routes, id, op);
    };
    const auto write_routes = adapter.actors.write_routes;
    adapter.actors.write_routes = [&](auto &owner, const auto &routes) {
        const bool written = write_routes(owner, routes);
        if (written && !ref::valid_world_schedule_owner(owner.scene.world)) {
            const auto &world = owner.scene.world.world;
            last += " invalid-owner actors=" + std::to_string(world.ai.battle.actors.size()) +
                    "/humans=" + std::to_string(world.ai.human_order.size()) +
                    "/monsters=" + std::to_string(world.ai.monster_order.size()) +
                    " projectile=" + std::to_string(world.ai.projectile_order.size()) + '/' +
                    std::to_string(world.ai.projectiles.size()) +
                    " object=" + std::to_string(world.object_order.size()) + '/' +
                    std::to_string(world.ai.battle.objects.size()) +
                    " encounter=" + std::to_string(world.ai.encounter_order.size()) + '/' +
                    std::to_string(world.ai.encounters.size()) +
                    " facility=" + std::to_string(owner.scene.world.facility_order.size()) + '/' +
                    std::to_string(world.facilities.size());
            for (const auto &a : world.actors)
                if (a.second.journey && a.second.unbound_route)
                    last += " duplicate-path=" + std::to_string(a.first.value);
            for (const auto id : world.ai.projectile_order)
                last += " projectile-order=" + std::to_string(id);
            for (const auto &p : world.ai.projectiles)
                last += " projectile-map=" + std::to_string(p.first);
        }
        return written;
    };
    const auto facility = adapter.facility;
    adapter.facility = [&](const auto &owner, const auto &request) {
        last = "facility-request kind=" + std::to_string(static_cast<int>(request.kind));
        return facility(owner, request);
    };
    const auto tail = adapter.actors.tail_cache;
    adapter.actors.tail_cache = [&](const auto &owner, auto id, const auto &projected) {
        last = "tail-cache actor=" + std::to_string(id.value);
        return tail(owner, id, projected);
    };
    const auto scene = adapter.scene_other;
    adapter.scene_other = [&](const auto &owner, const auto &call) {
        last = "scene-stage=" + std::to_string(static_cast<int>(call.stage));
        auto result = scene(owner, call);
        if (!result && call.stage == ref::WorldSceneStage::global_display) {
            for (const auto &effect : owner.visual_effects) {
                last += " visual=[";
                for (auto value : effect)
                    last += std::to_string(value) + ',';
                last += ']';
            }
            for (const auto &effect : owner.delayed_effects) {
                last += " delayed=[";
                for (auto value : effect)
                    last += std::to_string(value) + ',';
                last += ']';
            }
        }
        return result;
    };
    const auto script_read = adapter.scripts.read;
    adapter.scripts.read = [&](const auto &owner) {
        auto value = script_read(owner);
        const auto valid = ref::prepare_world_script_continuations(adapter.catalog, value, false);
        last = "scripts-read validate=" + std::to_string(static_cast<int>(valid.error));
        for (const auto &continuation : value.continuations)
            last += " continuation=" + std::to_string(continuation.event) + ':' +
                    std::to_string(continuation.next_instruction) + ':' +
                    std::to_string(continuation.remaining_updates);
        return value;
    };
    const auto script_write = adapter.scripts.write;
    adapter.scripts.write = [&](auto &owner, const auto &scripts) {
        const bool written = script_write(owner, scripts);
        if (!written)
            last += " scripts-write-failed";
        return written;
    };
    const auto entry_read = adapter.entry.read;
    adapter.entry.read = [&](const auto &owner) {
        last = "entry-read";
        return entry_read(owner);
    };
    const auto entry_write = adapter.entry.write;
    adapter.entry.write = [&](auto &owner, const auto &value) {
        last = "entry-write";
        const bool written = entry_write(owner, value);
        if (!written)
            last += " failed";
        return written;
    };
    const auto report_read = adapter.report.read;
    adapter.report.read = [&](const auto &owner) {
        last = "report-read";
        return report_read(owner);
    };
    const auto report_write = adapter.report.write;
    adapter.report.write = [&](auto &owner, const auto &value) {
        last = "report-write";
        const bool written = report_write(owner, value);
        if (!written)
            last += " failed";
        return written;
    };
    const auto before_common = adapter.before_common;
    adapter.before_common = [&](const auto &owner) {
        last = "before-common";
        auto value = before_common(owner);
        if (!value)
            last += " failed";
        return value;
    };
    // 只重放失败候选作读诊断，保留原消费者；不添加任何默认成功/状态推进。
    (void)ref::prepare_owned_world_runtime(s, {s.calendar_advance, true}, adapter);
    return last;
}
void continuous(int months, std::uint64_t seed, int speed) {
    StartupSession original;
    StartupWorldRuntimeSession session(original.state(),
                                       ref::WorldRandomStream::from_java_seed(seed));
    session.set_speed(speed);
    check(session.state().popularity == 50 && session.state().maximum_popularity == 0,
          "source n.J initializes popularity50/peak0; do not invent peak50 to satisfy an invalid "
          "invariant");
    const int opening_month = session.state().scene.calendar.month;
    const int opening_year = session.state().scene.calendar.year;
    std::set<std::uint64_t> pages;
    std::set<std::uint64_t> task_ids;
    std::set<int> report_states;
    std::size_t max_humans{};
    std::size_t max_occupants{};
    std::int64_t facility_income{};
    std::int64_t cash_expenses{};
    int month_transitions{};
    int last_month = opening_month;
    bool finished{};
    // 有界测试保护，不参与世界规则、不跳过真实人物/页面/日期消费者。
    const int frame_limit = std::max(20000, months * 10000);
    for (int frame = 0; frame < frame_limit; ++frame) {
        const auto before_random = session.state().scene.random.draws();
        const auto result = session.update();
        if (!result.candidate) {
            std::ostringstream error;
            error << "continuous update failed runtime=" << static_cast<int>(result.error)
                  << " scene_error=" << static_cast<int>(result.scene_error)
                  << " world_error=" << static_cast<int>(result.world_error) << ' '
                  << snapshot(session.state(), frame) << " last=" << diagnose(session.state());
            throw std::runtime_error(error.str());
        }
        const auto &s = session.state();
        check(s.scene.random.draws() >= before_random,
              "single Java stream never rewinds on committed updates");
        check(ref::valid_world_calendar_state(s.scene.calendar),
              "calendar stays normalized during true continuous run");
        max_humans = std::max(max_humans, s.scene.world.world.ai.human_order.size());
        for (const auto &f : s.scene.world.world.facilities)
            max_occupants = std::max(max_occupants, f.second.occupants.size());
        for (auto id : s.task_order)
            task_ids.insert(id);
        report_states.insert(s.report_state);
        if (s.scene.calendar.month != last_month) {
            ++month_transitions;
            last_month = s.scene.calendar.month;
            std::cout << "month " << month_transitions << ' ' << snapshot(s, frame) << std::endl;
        }
        const auto page = top_page(s);
        check(page != nullptr, "source framework preserves main page");
        if (page->kind != ref::WorldScriptPageKind::scene && page->legacy_page != 56 &&
            page->legacy_page != 57 && page->legacy_page != 16) {
            const auto id = page->id;
            const auto legacy_page = page->legacy_page;
            const auto source_record = page->source_record;
            if (pages.insert(id).second)
                std::cout << "page id=" << id << " kind=" << static_cast<int>(page->kind)
                          << " legacy=" << page->legacy_page << " source=" << page->source_record
                          << ' ' << snapshot(s, frame) << std::endl;
            // 明确的测试用户一次确认；多阶段成果页保留自身真实计数/阶段，绝不直接删页。
            auto acknowledged = StartupWorldRuntimeError::none;
            if (legacy_page == 87) {
                // 明确测试玩家选择终止→确认，不自动授勋、清勋章或冒充普通确认。
                acknowledged =
                    session.act_award_page(id, ref::WorldAwardAction::request_termination);
                if (acknowledged == StartupWorldRuntimeError::none)
                    acknowledged =
                        session.act_award_page(id, ref::WorldAwardAction::confirm_termination);
            } else
                acknowledged = session.acknowledge_page(id);
            if (acknowledged != StartupWorldRuntimeError::none) {
                std::ostringstream error;
                error << "page confirmation failed error=" << static_cast<int>(acknowledged)
                      << " legacy=" << legacy_page << " source=" << source_record << ' '
                      << snapshot(s, frame);
                throw std::runtime_error(error.str());
            }
        }
        if (month_transitions >= months &&
            s.scene.calendar.year == opening_year + (opening_month + months) / 12 &&
            s.scene.calendar.month == (opening_month + months) % 12 &&
            s.scene.calendar.units >= 27) {
            finished = true;
            break;
        }
    }
    const auto &s = session.state();
    for (const auto &entry : s.scene.world.world.ai.accounting.entries()) {
        if (entry.second.category == ref::CashCategory::facilities &&
            entry.second.direction == ref::CashDirection::income)
            facility_income += entry.second.amount;
        if (entry.second.direction == ref::CashDirection::expense)
            cash_expenses += entry.second.amount;
    }
    if (!finished)
        throw std::runtime_error("continuous frame limit reached " + snapshot(s, frame_limit));
    check(month_transitions == months,
          "normal real new game crosses requested month boundaries without injected waits");
    check(s.scene.calendar.year == opening_year + (opening_month + months) / 12,
          "continuous target includes actual year normalization rather than repeated month only");
    check(max_humans > 0 && max_occupants > 0 && facility_income > 0,
          "real autonomous visitors actually use facilities and pay arrival income");
    check(cash_expenses > 0 && report_states.count(2),
          "source fees and cash report occur during same continuous world");
    check(!session.checkpoints().empty(),
          "real calendar call saves immutable pre-normalization checkpoints");
    check(s.scene.random.draws() > 0, "real Java48 stream used by live branches");
    std::cout << "continuous summary months=" << months << " seed=" << seed << " speed=" << speed
              << " humans=" << max_humans << " occupancy=" << max_occupants
              << " income=" << facility_income << " expenses=" << cash_expenses
              << " cash=" << s.scene.world.world.ai.accounting.funds() << " pages=" << pages.size()
              << " tasks=" << task_ids.size() << " random=" << s.scene.random.draws()
              << " checkpoints=" << session.checkpoints().size() << '\n';
}
} // namespace
int main(int argc, const char **argv) {
    try {
        int months = 2;
        std::uint64_t seed = 1;
        int speed = 0;
        const auto parse = [](const char *text, auto &value) {
            const std::string input(text);
            const auto result = std::from_chars(input.data(), input.data() + input.size(), value);
            if (result.ec != std::errc{} || result.ptr != input.data() + input.size())
                throw std::invalid_argument("invalid continuous test argument");
        };
        if (argc > 4)
            throw std::invalid_argument("expected [months [seed [speed]]]");
        if (argc >= 2)
            parse(argv[1], months);
        if (argc >= 3)
            parse(argv[2], seed);
        if (argc >= 4)
            parse(argv[3], speed);
        if (months < 1 || months > 36 || (speed != 0 && speed != 1))
            throw std::invalid_argument("months must be 1..36, speed must be 0 or 1");
        continuous(months, seed, speed);
        std::cout << "startup world continuous checks: " << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
