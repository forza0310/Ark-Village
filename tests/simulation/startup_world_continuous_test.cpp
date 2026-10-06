#include "ark/simulation/startup_world_building.hpp"
#include "ark/simulation/startup_world_human.hpp"
#include "ark/simulation/startup_world_runtime.hpp"
#include "ark/simulation/startup_world_tax.hpp"
#include "ark/simulation/rules/world_arrivals.hpp"

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
            page->legacy_page != 57 && page->legacy_page != 16 && page->legacy_page != 97) {
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

ref::Position housing_anchor(const StartupWorldRuntimeState &s) {
    const auto &map = s.scene.world.world.map;
    const auto bounds = s.rules->fences.at(s.fence_level);
    for (int y = bounds[1].y + 1; y < bounds[0].y; ++y)
        for (int x = bounds[0].x + 1; x < bounds[1].x; ++x)
            if (!map.cells.at(y * map.width + x).facility)
                return {x, y};
    throw std::runtime_error("natural housing has no free original town cell");
}
void retired_recruitment(const StartupWorldRuntimeState &s, std::uint64_t id) {
    check(!s.scene.world.world.facilities.count(id) && !s.facility_original_ids.count(id) &&
              !s.facility_ordinals.count(id) && !s.facility_residents.count(id) &&
              !s.facility_difficulties.count(id) && !s.facility_flags.count(id) &&
              !s.facility_details.count(id) && !s.facility_monthly_cash.count(id) &&
              !s.facility_month_age.count(id) && !s.neighbourhood.count(id) &&
              !s.neighbourhood_details.count(id) && !s.dungeon_facilities.count(id) &&
              !s.sites.count(id) && !s.shops.count(id) &&
              std::find(s.shop_order.begin(), s.shop_order.end(), id) == s.shop_order.end(),
          "natural residence retires old recruitment instance and all no-longer-owned auxiliary "
          "records");
}
void managed_resource_references(const StartupWorldRuntimeState &s) {
    std::set<std::uint64_t> pages;
    for (const auto &page : s.scripts.pages)
        pages.insert(page.id);
    const auto page_owned = [&](const auto &records) {
        return std::all_of(records.begin(), records.end(),
                           [&](const auto &record) { return pages.count(record.first) != 0; });
    };
    check(page_owned(s.human_page_catalogs) && page_owned(s.equipment_page_catalogs) &&
              page_owned(s.human_page_selections) && page_owned(s.page_job_bindings) &&
              page_owned(s.human_page_parents) && page_owned(s.human_page_answers) &&
              page_owned(s.human_equipment_choices) && page_owned(s.human_gift_scores) &&
              page_owned(s.human_gift_messages) && page_owned(s.tax_page_residents) &&
              page_owned(s.tax_page_selection) && page_owned(s.tax_page_scroll) &&
              std::all_of(s.human_pages_initialized.begin(), s.human_pages_initialized.end(),
                          [&](const auto id) { return pages.count(id) != 0; }),
          "human and tax payload scale is bounded by real retained pages, never orphan history");
    check(std::all_of(s.shops.begin(), s.shops.end(),
                      [&](const auto &shop) {
                          return s.scene.world.world.facilities.count(shop.first) != 0;
                      }) &&
              std::all_of(s.shop_order.begin(), s.shop_order.end(),
                          [&](const auto id) { return s.shops.count(id) != 0; }),
          "shop records and order are owned by currently retained facility instances");
}
// 长轨迹沿既有continuous职责：真实新局，仅玩家命令，不注入C/资金/人物/日期/奖励。
void natural_housing(std::uint64_t seed, int speed) {
    StartupSession initial;
    StartupWorldRuntimeSession session(initial.state(),
                                       ref::WorldRandomStream::from_java_seed(seed));
    session.set_speed(speed);
    const auto anchor = housing_anchor(session.state());
    check(session.open_build_menu() == StartupWorldRuntimeError::none,
          "natural player opens actual construction catalogue");
    const auto catalogue = top_page(session.state());
    check(catalogue && catalogue->legacy_page == 21,
          "natural catalogue is actual raw21, not an injected page");
    check(session.select_build_menu(catalogue->id, 24).error == StartupWorldRuntimeError::none,
          "actual initial catalogue permits original recruitment24");
    const auto before_build = session.state().scene.world.world.ai.accounting.funds();
    const auto placed = session.confirm_build(anchor, ref::FacilityOrientation::first);
    check(placed.created &&
              session.state().scene.world.world.ai.accounting.funds() == before_build - 100 &&
              session.cancel_build() == StartupWorldRuntimeError::none,
          "natural recruitment costs original100, returns real scene without injected cash");
    const auto recruitment = *placed.created;
    std::optional<std::uint64_t> home;
    std::optional<int> resident;
    bool recruitment_ready{};
    bool home_ready{};
    int grants{};
    int tax_receipts{};
    std::optional<std::uint64_t> tax_page;
    int tax_month{-1};
    int tax_year{-1};
    int income_after_tax{};
    std::size_t max_pages{}, max_notices{}, max_sounds{}, max_actors{}, max_tasks{}, max_entries{};
    std::size_t max_retired_actors{}, max_retired_encounters{}, max_payloads{}, max_effects{};
    std::size_t consumed_sounds{};
    int gifts{};
    bool gifting{};
    int last_month = session.state().scene.calendar.month;
    constexpr int frame_limit = 180000;
    for (int frame = 0; frame < frame_limit; ++frame) {
        const auto &old = session.state();
        const auto previous = top_page(old);
        const bool paying = previous && previous->kind == ref::WorldScriptPageKind::raw_page &&
                            previous->legacy_page == 98;
        const auto paying_id = paying ? previous->id : 0;
        const auto old_cash = old.scene.world.world.ai.accounting.funds();
        const auto old_draws = old.scene.random.draws();
        const int old_income = old.monthly_cash.at(old.scene.calendar.month)[4][0];
        std::int64_t expected_tax{};
        if (paying)
            for (const auto &human : old.rules->humans)
                if (old.human_presence.at(human.identity) != 0 &&
                    old.human_homes.at(human.identity)[2] == 1)
                    expected_tax += old.human_calendar.at(human.identity).legacy_G;
        const auto updated = session.update();
        if (!updated.candidate)
            throw std::runtime_error("natural housing update failed runtime=" +
                                     std::to_string(static_cast<int>(updated.error)) + ' ' +
                                     snapshot(session.state(), frame) +
                                     " last=" + diagnose(session.state()));
        const auto &s = session.state();
        managed_resource_references(s);
        const auto usage = startup_world_resource_usage(s);
        max_retired_actors = std::max(max_retired_actors, usage.retired_actors);
        max_retired_encounters = std::max(max_retired_encounters, usage.retired_encounters);
        max_payloads = std::max(max_payloads, usage.page_payloads);
        max_effects = std::max(max_effects, usage.effects);
        check(s.scene.random.draws() >= old_draws &&
                  ref::valid_world_calendar_state(s.scene.calendar),
              "natural housing keeps one monotonic random stream and normalized calendar");
        max_pages = std::max(max_pages, s.scripts.pages.size());
        max_notices = std::max(max_notices, s.scripts.notices.size());
        max_sounds = std::max(max_sounds, s.sound_requests.size());
        max_actors = std::max(max_actors, s.scene.world.world.ai.battle.actors.size());
        max_tasks = std::max(max_tasks, s.tasks.size());
        max_entries = std::max(max_entries, s.scene.world.world.ai.accounting.entries().size());
        if (paying) {
            const auto paid = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                           [&](const auto &p) { return p.id == paying_id; });
            check(paid != s.scripts.pages.end() && paid->lifecycle == 4 &&
                      s.scene.world.world.ai.accounting.funds() == old_cash + expected_tax &&
                      s.monthly_cash.at(s.scene.calendar.month)[4][0] ==
                          old_income + expected_tax &&
                      s.scene.random.draws() == old_draws,
                  "natural98 automatically posts exact current tax to other-income and closes "
                  "without random");
            check(std::all_of(
                      s.rules->humans.begin(), s.rules->humans.end(),
                      [&](const auto &human) {
                          const auto id = human.identity;
                          return s.human_calendar.at(id).legacy_G == 0 &&
                                 s.scene.world.world.ai.battle.humans.at(id).battle_reward_stat ==
                                     0;
                      }),
                  "natural98 clears every real definition's F/G, not only the chosen resident");
            check(tax_page && expected_tax > 0,
                  "natural resident really earned positive tax before original annual collection");
            ++tax_receipts;
            tax_month = s.scene.calendar.month;
            tax_year = s.scene.calendar.year;
            income_after_tax = s.monthly_cash.at(tax_month)[0][0];
            std::cout << "housing tax amount=" << expected_tax << ' ' << snapshot(s, frame)
                      << std::endl;
        }
        if (!home) {
            const auto built = s.scene.world.world.facilities.find(recruitment);
            check(built != s.scene.world.world.facilities.end(),
                  "natural recruitment remains until the actual successful residence command");
            recruitment_ready = recruitment_ready || built->second.status == 1;
        } else {
            retired_recruitment(s, recruitment);
            const auto &built = s.scene.world.world.facilities.at(*home);
            if (!home_ready && built.status == 1) {
                home_ready = true;
                check(resident && s.facility_residents.at(*home) == *resident &&
                          s.human_homes.at(*resident)[2] == 1 &&
                          ref::world_script_seen(s.scripts, 202),
                      "common world naturally completes actual home and source first-home event");
                std::cout << "housing complete human=" << *resident << ' ' << snapshot(s, frame)
                          << std::endl;
            }
        }
        if (s.scene.calendar.month != last_month) {
            last_month = s.scene.calendar.month;
            std::cout << "housing resources pages=" << s.scripts.pages.size()
                      << " notices=" << s.scripts.notices.size()
                      << " sounds=" << s.sound_requests.size()
                      << " actors=" << s.scene.world.world.ai.battle.actors.size()
                      << " task_records=" << s.tasks.size()
                      << " cash_entries=" << s.scene.world.world.ai.accounting.entries().size()
                      << " checkpoints=" << session.checkpoints().size() << ' '
                      << " C1=" << s.shop_humans.at(1).satisfaction << ' ' << snapshot(s, frame)
                      << std::endl;
        }
        const auto current = top_page(s);
        check(current, "natural housing preserves a real framework page");
        const auto page = *current;
        const auto failure = [&](StartupWorldRuntimeError error, const char *command) {
            if (error != StartupWorldRuntimeError::none)
                throw std::runtime_error(std::string("natural housing input failed ") + command +
                                         " error=" + std::to_string(static_cast<int>(error)) + ' ' +
                                         snapshot(session.state(), frame));
        };
        if (page.kind == ref::WorldScriptPageKind::scene && !home && recruitment_ready) {
            const auto eligible = std::find_if(
                s.rules->humans.begin(), s.rules->humans.end(), [&](const auto &human) {
                    return s.human_presence.at(human.identity) != 0 &&
                           s.human_homes.at(human.identity)[2] == 0 &&
                           s.shop_humans.at(human.identity).satisfaction >=
                               human.residence_threshold &&
                           s.scene.world.world.ai.accounting.funds() >= human.residence_fee;
                });
            if (eligible != s.rules->humans.end()) {
                resident = eligible->identity;
                failure(session.open_facility_page(recruitment), "open recruitment");
            } else if (s.human_presence.at(1) != 0 &&
                       s.shop_humans.at(1).satisfaction <
                           s.rules->humans.at(1).residence_threshold &&
                       s.scene.world.world.ai.accounting.funds() >=
                           s.rules->humans.at(1).residence_fee + 150) {
                // 明确玩家策略：赠送原初始短剑提升C，留足原入住费；不写C或补现金。
                gifting = true;
                failure(session.open_human_page(1), "open actual gift recipient");
            }
        } else if (gifting && page.legacy_page == 60) {
            failure(session.act_human_page(page.id, StartupHumanPageAction::gifts),
                    "open equipment gifts");
        } else if (gifting && page.legacy_page == 64) {
            if (s.human_page_answers.count(page.id)) {
                // K0必须由下一真实父更新消费；此刻不重开65。
            } else if (s.shop_humans.at(1).satisfaction >=
                           s.rules->humans.at(1).residence_threshold ||
                       s.scene.world.world.ai.accounting.funds() <
                           s.rules->humans.at(1).residence_fee + 150) {
                failure(session.act_human_page(page.id, StartupHumanPageAction::cancel),
                        "return gift catalogue");
                gifting = false;
            } else {
                const auto &list = s.equipment_page_catalogs.at(page.id)[0];
                const auto selected = std::find(list.begin(), list.end(), 0);
                check(selected != list.end(),
                      "source initial short sword0 remains available to player gifts");
                failure(session.act_human_page(page.id, StartupHumanPageAction::select,
                                               static_cast<int>(selected - list.begin())),
                        "select original short sword");
                failure(session.act_human_page(page.id, StartupHumanPageAction::confirm),
                        "open65 quote");
            }
        } else if (gifting && page.legacy_page == 65) {
            failure(session.acknowledge_page(page.id), "confirm original equipment gift");
            ++gifts;
        } else if (!gifting && page.legacy_page == 60) {
            failure(session.cancel_page(page.id), "return human details after gifts");
        } else if (page.legacy_page == 74) {
            if (!home && s.facility_page_bindings.at(page.id) == recruitment)
                failure(session.act_facility_page(page.id, StartupFacilityPageAction::confirm),
                        "open resident candidates");
            else
                failure(session.act_facility_page(page.id, StartupFacilityPageAction::cancel),
                        "return facility details");
        } else if (page.legacy_page == 80) {
            check(resident && std::find(s.residence_page_candidates.at(page.id).begin(),
                                        s.residence_page_candidates.at(page.id).end(),
                                        *resident) != s.residence_page_candidates.at(page.id).end(),
                  "natural reached threshold appears in real candidate list");
            const auto chosen = session.act_residence_page(page.id, *resident);
            failure(chosen.error, "choose actual resident");
            check(chosen.created && chosen.denial == StartupBuildDenial::none,
                  "natural affordable resident replaces recruitment without a synthetic rebate");
            home = chosen.created;
            std::cout << "housing admitted human=" << *resident << ' '
                      << snapshot(session.state(), frame) << std::endl;
        } else if (page.legacy_page == 87) {
            if (s.medal_count > 0) {
                const int human = s.award_rankings.at(page.id).front();
                const int before = s.human_calendar.at(human).celebrations;
                failure(session.act_award_page(page.id, ref::WorldAwardAction::request_award, 0),
                        "request real annual medal");
                failure(session.act_award_page(page.id, ref::WorldAwardAction::confirm_award),
                        "confirm real annual medal");
                check(session.state().human_calendar.at(human).celebrations == before + 1,
                      "natural annual award actually increments chosen human's source medal count");
                ++grants;
            }
        } else if (page.legacy_page == 90) {
            check(
                home_ready && s.scene.calendar.year > 0 && s.scene.calendar.month == 3,
                "natural completed residence reaches original tax date without calendar injection");
            const auto view = inspect_startup_world_tax_page(s, page.id);
            check(view && !view->rows.empty(), "actual90 displays real resident tax list");
            tax_page = page.id;
            const auto cash = s.scene.world.world.ai.accounting.funds();
            failure(session.acknowledge_page(page.id), "confirm tax list");
            check(session.state().scene.world.world.ai.accounting.funds() == cash,
                  "natural tax-list confirmation itself does not pay tax");
        } else if (page.legacy_page == 83) {
            failure(session.cancel_page(page.id), "return unlocked shop entry");
        } else if (page.kind != ref::WorldScriptPageKind::scene && page.legacy_page != 16 &&
                   page.legacy_page != 56 && page.legacy_page != 57 && page.legacy_page != 97 &&
                   page.legacy_page != 98) {
            failure(session.acknowledge_page(page.id), "confirm actual event");
        }
        const auto &after = session.state();
        consumed_sounds += session.take_sound_requests().size();
        check(session.state().sound_requests.empty(),
              "natural headless presentation sink consumes transient sound outputs each frame");
        const auto next_page = top_page(after);
        const int elapsed_months = after.scene.calendar.year * 12 + after.scene.calendar.month -
                                   (tax_year * 12 + tax_month);
        if (tax_receipts == 1 && elapsed_months >= 1 && after.scene.calendar.units >= 27 &&
            next_page && next_page->kind == ref::WorldScriptPageKind::scene) {
            check(home_ready && recruitment_ready && grants > 0 &&
                      after.monthly_cash.at(tax_month)[0][0] > income_after_tax,
                  "natural housing, annual award and tax return to continuing paid facility "
                  "business");
            check(after.tax_page_residents.empty() && after.tax_page_selection.empty() &&
                      after.tax_page_scroll.empty(),
                  "real tax pages retire their transient lists and selection records");
            retired_recruitment(after, recruitment);
            std::cout << "housing summary seed=" << seed << " speed=" << speed
                      << " resident=" << *resident << " home=" << *home << " grants=" << grants
                      << " taxes=" << tax_receipts << " gifts=" << gifts
                      << " peak_pages=" << max_pages << " peak_notices=" << max_notices
                      << " peak_sounds=" << max_sounds << " peak_actors=" << max_actors
                      << " consumed_sounds=" << consumed_sounds
                      << " peak_task_records=" << max_tasks << " peak_cash_entries=" << max_entries
                      << " peak_retired_actors=" << max_retired_actors
                      << " peak_retired_encounters=" << max_retired_encounters
                      << " peak_page_payloads=" << max_payloads << " peak_effects=" << max_effects
                      << " checkpoints=" << session.checkpoints().size() << ' '
                      << snapshot(after, frame) << '\n';
            return;
        }
    }
    throw std::runtime_error(
        "natural housing frame limit reached home=" + std::to_string(home.has_value()) +
        " ready=" + std::to_string(home_ready) + " grants=" + std::to_string(grants) +
        " taxes=" + std::to_string(tax_receipts) + ' ' + snapshot(session.state(), frame_limit));
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
        if (argc >= 2 && std::string(argv[1]) == "natural_housing") {
            if (argc > 4)
                throw std::invalid_argument("expected natural_housing [seed [speed]]");
            if (argc >= 3)
                parse(argv[2], seed);
            if (argc >= 4)
                parse(argv[3], speed);
            if (speed != 0 && speed != 1)
                throw std::invalid_argument("speed must be 0 or 1");
            natural_housing(seed, speed);
            std::cout << "natural housing continuous checks: " << checks << '\n';
            return 0;
        }
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
