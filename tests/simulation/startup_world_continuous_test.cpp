#include "ark/simulation/facilities/startup_world_building.hpp"
#include "ark/simulation/facilities/startup_world_commerce.hpp"
#include "ark/simulation/facilities/startup_world_editing.hpp"
#include "ark/simulation/facilities/startup_world_facility_items.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include "ark/simulation/persistence/startup_world_persistence.hpp"
#include "ark/simulation/world/startup_world_runtime.hpp"
#include "ark/simulation/village/startup_world_tax.hpp"
#include "ark/simulation/village/startup_world_village_activity.hpp"
#include "ark/simulation/actors/rules/world_arrivals.hpp"
#include "startup_world_replay_driver.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
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
              !s.facility_month_age.count(id) && !s.facility_item_confirmations.count(id) &&
              !s.neighbourhood.count(id) && !s.neighbourhood_details.count(id) &&
              !s.dungeon_facilities.count(id) && !s.sites.count(id) && !s.shops.count(id) &&
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
              page_owned(s.activity_page_bindings) && page_owned(s.activity_page_lists) &&
              page_owned(s.activity_page_display_humans) && page_owned(s.activity_page_parents) &&
              page_owned(s.activity_page_answers) && page_owned(s.activity_page_selections) &&
              page_owned(s.activity_page_scroll) && page_owned(s.facility_item_page_items) &&
              page_owned(s.facility_item_page_lists) &&
              page_owned(s.facility_item_page_selections) && page_owned(s.commerce_page_data) &&
              page_owned(s.commerce_page_lists) &&
              page_owned(s.facility_definition_page_bindings) &&
              page_owned(s.facility_page_bindings) && page_owned(s.facility_page_neighbours) &&
              page_owned(s.build_page_catalogs) && page_owned(s.residence_page_candidates) &&
              std::all_of(s.facility_upgrade_initialized.begin(),
                          s.facility_upgrade_initialized.end(),
                          [&](auto id) { return pages.count(id) != 0; }) &&
              std::all_of(s.facility_item_pages_initialized.begin(),
                          s.facility_item_pages_initialized.end(),
                          [&](auto id) { return pages.count(id) != 0; }) &&
              std::all_of(s.commerce_pages_initialized.begin(), s.commerce_pages_initialized.end(),
                          [&](auto id) { return pages.count(id) != 0; }) &&
              std::all_of(s.activity_page_parents.begin(), s.activity_page_parents.end(),
                          [&](const auto &binding) { return pages.count(binding.second) != 0; }) &&
              std::all_of(s.activity_pages_initialized.begin(), s.activity_pages_initialized.end(),
                          [&](auto id) { return pages.count(id) != 0; }) &&
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
// 在真实晋级前缀之后执行编辑命令。候选从当时地图/定义寻找，不预设空地或维护ID。
template <class Require>
std::array<std::uint64_t, 2> natural_editing_commands(StartupWorldRuntimeSession &session,
                                                      Require &&require) {
    const auto &initial = session.state();
    check((initial.scripts.user_flags & 32U) != 0,
          "natural first-star95 really unlocked moving before editing commands");
    const auto road_definition = std::find_if(
        initial.rules->facilities.begin(), initial.rules->facilities.end(), [&](const auto &d) {
            return d.kind == 6 && (d.flags & 4) && initial.facility_presence.at(d.id) != 0;
        });
    check(road_definition != initial.rules->facilities.end(),
          "natural town has unlocked source road definition");
    const int road_id = road_definition->id;
    const auto quote = startup_world_build_quote(initial, road_id);
    check(quote.has_value(), "natural road uses current source construction quote");
    const auto &map = initial.scene.world.world.map;
    const auto fence = initial.rules->fences.at(initial.fence_level);
    const auto in_town = [&](ref::Position p) {
        return p.x > fence[0].x && p.x < fence[1].x && p.y > fence[1].y && p.y < fence[0].y;
    };
    std::optional<ref::Position> road_cell;
    for (int y = 0; y < map.height && !road_cell; ++y)
        for (int x = 0; x < map.width && !road_cell; ++x) {
            const auto &tile = map.cells.at(y * map.width + x);
            if (in_town({x, y}) && tile.legacy_state == 4 && !tile.facility)
                road_cell = ref::Position{x, y};
        }
    check(road_cell.has_value(), "natural map supplies a legal single road cell");
    const auto accepted = [&](const StartupBuildResult &r, const char *label) {
        require(r.error, label);
        if (r.denial != StartupBuildDenial::none)
            throw std::runtime_error(std::string("natural editing denied ") + label +
                                     " denial=" + std::to_string(static_cast<int>(r.denial)) + ' ' +
                                     snapshot(session.state(), -1));
    };
    const auto cash = initial.scene.world.world.ai.accounting.funds();
    const auto draws = initial.scene.random.draws();
    const auto index = static_cast<std::size_t>(road_cell->y * map.width + road_cell->x);
    accepted(session.begin_road(road_id), "enter natural road mode");
    accepted(session.confirm_edit(*road_cell, ref::FacilityOrientation::first),
             "choose road start");
    accepted(session.confirm_edit(*road_cell, ref::FacilityOrientation::first),
             "commit single road cell");
    check(session.state().scene.world.world.map.cells.at(index).legacy_state == 3 &&
              session.state().surface.at(index).definition == road_id &&
              session.state().scene.world.world.ai.accounting.funds() ==
                  cash - quote->construction_cost,
          "real road changes one tile and charges original current quote once");
    require(session.cancel_edit(), "leave road mode");
    accepted(session.begin_edit(false), "enter road removal mode");
    accepted(session.confirm_edit(*road_cell, ref::FacilityOrientation::first),
             "choose road removal start");
    accepted(session.confirm_edit(*road_cell, ref::FacilityOrientation::first),
             "remove single road cell");
    check(session.state().scene.world.world.map.cells.at(index).legacy_state == 4 &&
              session.state().surface.at(index).definition == session.state().ground_definition &&
              session.state().scene.world.world.ai.accounting.funds() ==
                  cash - quote->construction_cost,
          "real road removal restores ground without synthetic refund");
    require(session.cancel_edit(), "leave road removal mode");

    std::optional<std::uint64_t> old_id;
    std::optional<ref::Position> target;
    // 只选择正常kind2及其合法全占地空地；不拆正在经营/接待人物的设施来简化引用问题。
    const auto &before = session.state();
    for (const auto id : before.scene.world.facility_order) {
        const auto &f = before.scene.world.world.facilities.at(id);
        if (f.kind != 2 || f.status != 1 || !f.occupants.empty())
            continue;
        for (int y = 0; y < before.scene.world.world.map.height && !target; ++y)
            for (int x = 0; x < before.scene.world.world.map.width && !target; ++x) {
                const auto footprint = ref::facility_footprint(
                    f.placement.shape, f.placement.orientation, {x, y},
                    before.scene.world.world.map.width, before.scene.world.world.map.height);
                if (footprint.error != ref::GeometryError::none)
                    continue;
                if (std::all_of(footprint.cells.begin(), footprint.cells.end(), [&](const auto &c) {
                        const auto &tile = before.scene.world.world.map.cells.at(
                            c.position.y * before.scene.world.world.map.width + c.position.x);
                        return in_town(c.position) && tile.legacy_state == 4 && !tile.facility;
                    })) {
                    old_id = id;
                    target = ref::Position{x, y};
                }
            }
        if (target)
            break;
    }
    check(old_id && target, "natural town has an ordinary kind2 and disjoint legal move footprint");
    const auto placed = before.scene.world.world.facilities.at(*old_id).placement;
    const int raw = before.facility_original_ids.at(*old_id),
              ordinal = before.facility_ordinals.at(*old_id);
    const int age = before.facility_month_age.at(*old_id);
    const auto month_cash = before.facility_monthly_cash.at(*old_id);
    const int item_count = before.facility_item_confirmations.at(*old_id);
    const auto count = before.scene.world.world.facilities.size();
    const auto move_cash = before.scene.world.world.ai.accounting.funds();
    accepted(session.begin_edit(true), "enter genuinely unlocked move mode");
    accepted(session.confirm_edit(placed.anchor, placed.orientation),
             "select actual kind2 instance");
    const auto moved = session.confirm_edit(*target, placed.orientation);
    accepted(moved, "move actual kind2 instance");
    check(moved.created && *moved.created != *old_id &&
              !session.state().scene.world.world.facilities.count(*old_id) &&
              session.state().scene.world.world.facilities.size() == count &&
              session.state().facility_original_ids.at(*moved.created) == raw &&
              session.state().facility_ordinals.at(*moved.created) == ordinal &&
              session.state().facility_month_age.at(*moved.created) == age &&
              session.state().facility_monthly_cash.at(*moved.created) == month_cash &&
              session.state().facility_item_confirmations.at(*moved.created) == item_count &&
              session.state().scene.world.world.ai.accounting.funds() == move_cash - 300,
          "natural move rebuilds stable Owner identity while preserving source raw/ordinal/y/cash "
          "and charging source300");
    // 300来自b/c:1826–1890/a/o:613–635的原固定移动费，不是新设演示价格。
    require(session.cancel_edit(), "leave move mode");
    accepted(session.begin_edit(false), "enter moved-decoration removal mode");
    accepted(session.confirm_edit(*target, placed.orientation), "remove moved kind2 instance");
    require(session.cancel_edit(), "leave final removal mode");
    const auto &after = session.state();
    check(after.scene.world.world.facilities.size() + 1 == count &&
              !after.scene.world.world.facilities.count(*moved.created) &&
              after.scene.world.world.ai.accounting.funds() == move_cash - 300 &&
              after.scene.random.draws() == draws && !after.build_moving_facility &&
              !after.build_anchor,
          "natural moved facility removal retires entity without refund, random draw or stale "
          "editing selection");
    for (const auto id : {*old_id, *moved.created}) {
        check(!after.facility_original_ids.count(id) && !after.facility_ordinals.count(id) &&
                  !after.facility_details.count(id) && !after.facility_month_age.count(id) &&
                  !after.facility_item_confirmations.count(id) &&
                  !after.facility_monthly_cash.count(id) && !after.sites.count(id) &&
                  !after.dungeon_facilities.count(id) && !after.neighbourhood.count(id) &&
                  !after.neighbourhood_details.count(id) &&
                  std::none_of(after.scene.world.world.map.cells.begin(),
                               after.scene.world.world.map.cells.end(),
                               [id](const auto &cell) {
                                   return cell.facility && cell.facility->instance_id.value == id;
                               }),
              "natural editing leaves neither old nor replacement per-instance or map references");
    }
    managed_resource_references(after);
    return {*old_id, *moved.created};
}

// 明确自动玩家策略：建设面包房、实际开展活动、点击自然升级提示，再申请月度晋级。
// 仅调用Session玩家命令；不写资金、点数、人气、日期、人物、设施使用数或rank。
void natural_progression(std::uint64_t seed, int speed, bool expansion = false,
                         const test::StartupWorldReplayOptions &options = {}) {
    StartupSession initial;
    StartupWorldRuntimeSession session(initial.state(),
                                       ref::WorldRandomStream::from_java_seed(seed));
    session.set_speed(speed);
    constexpr const char *controller_id = "natural-progression-expansion-v2";
    test::StartupWorldReplayDriver driver;
    driver.seed = seed;
    driver.speed = speed;
    driver.expansion = expansion;
    std::vector<StartupWorldOpaqueSection> retained_extensions;
    if (!options.load_file.empty()) {
        auto loaded = load_startup_world_file(options.load_file, *session.state().rules,
                                             StartupWorldSavePurpose::replay, controller_id);
        if (!loaded.snapshot)
            throw std::runtime_error("natural replay load failed: " + loaded.error);
        auto candidate = test::decode_startup_world_replay_driver(
            loaded.snapshot->metadata.controller_state);
        if (loaded.snapshot->metadata.next_frame != static_cast<std::uint64_t>(candidate.next_frame) ||
            candidate.expansion != expansion ||
            (options.seed_explicit && candidate.seed != seed) ||
            (options.speed_explicit && candidate.speed != speed))
            throw std::invalid_argument("natural replay controller or frame does not match request");
        test::validate_startup_world_replay_driver_world(candidate, loaded.snapshot->session.state());
        // Driver和世界均已在私有候选上校验；安装前不改自动玩家或当前Session。
        driver = std::move(candidate);
        session = std::move(loaded.snapshot->session);
        retained_extensions = std::move(loaded.snapshot->metadata.extensions);
        seed = driver.seed;
        speed = driver.speed;
        checks = driver.checks;
    }
    const int frame_limit = expansion ? 240000 : 180000;
    test::validate_startup_world_replay_options(options, driver.next_frame, frame_limit);
    if (driver.terminal)
        throw std::invalid_argument("natural replay snapshot already completed its scenario");
    std::ofstream trace;
    std::vector<StartupAudioRequest> round_audio; // 仅本轮已消费输出；捕获后不跨轮持有。
    if (!options.trace_file.empty()) {
        trace.open(options.trace_file, std::ios::binary | std::ios::trunc);
        if (!trace) throw std::runtime_error("cannot open natural replay trace");
    }
    bool saved{};
    const auto save = [&](const std::filesystem::path &file) {
        StartupWorldSaveMetadata metadata;
        metadata.purpose = StartupWorldSavePurpose::replay;
        metadata.producer_revision = options.producer_revision;
        metadata.controller_id = controller_id;
        metadata.next_frame = static_cast<std::uint64_t>(driver.next_frame);
        metadata.controller_state = test::encode_startup_world_replay_driver(driver);
        metadata.extensions = retained_extensions;
        const auto result = save_startup_world_file(file, session, metadata);
        if (!result.ok) throw std::runtime_error("natural replay save failed: " + result.error);
    };
    // 轮末唯一检查点：更新、该轮全部玩家命令、声音领取和原验收断言都已完成。
    const auto round_boundary = [&](int frame) {
        driver.next_frame = frame + 1;
        driver.checks = checks;
        if (!options.trace_file.empty() && frame >= options.trace_from.value_or(0)) {
            const auto bytes = test::encode_startup_world_replay_driver(driver);
            constexpr const char *hex = "0123456789abcdef";
            std::string controller_bytes;
            controller_bytes.reserve(bytes.size() * 2);
            for (const auto byte : bytes) {
                controller_bytes.push_back(hex[byte >> 4]);
                controller_bytes.push_back(hex[byte & 15]);
            }
            std::string audio_bytes;
            const auto audio_integer = [&](std::uint64_t value) {
                for (int n = 0; n < 8; ++n) {
                    const auto byte = static_cast<std::uint8_t>(value >> (n * 8));
                    audio_bytes.push_back(hex[byte >> 4]); audio_bytes.push_back(hex[byte & 15]);
                }
            };
            audio_integer(round_audio.size());
            for (const auto &request : round_audio) {
                audio_integer(static_cast<std::uint64_t>(request.operation));
                audio_integer(static_cast<std::uint64_t>(request.id));
            }
            trace << frame << ' ' << startup_world_session_digest(session) << ' '
                  << controller_bytes << ' ' << audio_bytes << '\n';
            if (!trace) throw std::runtime_error("cannot write natural replay trace");
        }
        if (options.save_at && frame == *options.save_at) {
            save(options.save_file);
            saved = true;
            std::cout << "natural replay saved next_frame=" << driver.next_frame << ' '
                      << snapshot(session.state(), frame) << std::endl;
        }
        if (test::startup_world_periodic_snapshot_due(options, frame)) {
            const auto directory = std::filesystem::path(options.save_directory);
            std::filesystem::create_directories(directory);
            const auto target = directory / ("prefix-" + std::to_string(frame) + ".awr");
            // staging目录独占，发布用硬链接create-only；竞态也不能覆盖已有有效证据。
            const auto staging = std::filesystem::path(target.string() + ".staging");
            if (std::filesystem::exists(target) || !std::filesystem::create_directory(staging))
                throw std::runtime_error("periodic snapshot target or staging already exists");
            const auto staged_file = staging / "snapshot.awr";
            const auto cleanup = [&] {
                std::error_code ignored;
                std::filesystem::remove(staged_file, ignored);
                std::filesystem::remove(staging, ignored);
            };
            try {
                save(staged_file);
                std::filesystem::create_hard_link(staged_file, target);
            } catch (...) { cleanup(); throw; }
            cleanup();
            std::cout << "natural replay periodic saved next_frame=" << driver.next_frame << ' '
                      << snapshot(session.state(), frame) << std::endl;
        }
        return options.stop_at && frame == *options.stop_at;
    };
    using E = StartupWorldRuntimeError;
    using A = StartupVillageActivityAction;
    auto &bakery = driver.bakery;
    auto &upgraded = driver.upgraded;
    auto &seen_upgrades = driver.seen_upgrades;
    auto &last_month = driver.last_month;
    auto &promoted_month = driver.promoted_month;
    auto &unlocked_month = driver.unlocked_month;
    auto &unlocked_income = driver.unlocked_income;
    auto &edited_month = driver.edited_month;
    auto &edited_income = driver.edited_income;
    auto &edited_retired = driver.edited_retired;
    auto &progression_complete = driver.progression_complete;
    auto &expanded_month = driver.expanded_month;
    auto &expanded_road_month = driver.expanded_road_month;
    auto &expanded_income = driver.expanded_income;
    auto &expanded_road = driver.expanded_road;
    auto &expanded_road_definition = driver.expanded_road_definition;
    auto &sounds = driver.sounds;
    const auto consume_audio = [&] {
        const auto requests = session.take_audio_requests();
        sounds += requests.size();
        if (trace.is_open()) round_audio.insert(round_audio.end(), requests.begin(), requests.end());
    };
    auto &peak_sounds = driver.peak_sounds;
    auto &peak_payloads = driver.peak_payloads;
    auto &peak_pages = driver.peak_pages;
    auto &peak_effects = driver.peak_effects;
    auto &peak_actors = driver.peak_actors;
    auto &peak_cash = driver.peak_cash;
    auto &peak_tasks = driver.peak_tasks;
    auto &peak_retired_actors = driver.peak_retired_actors;
    auto &peak_retired_encounters = driver.peak_retired_encounters;
    auto &next_activity_attempt = driver.next_activity_attempt;
    auto &completed = driver.completed;
    auto &unlocked_completed = driver.unlocked_completed;
    auto &next_task_month = driver.next_task_month;
    auto &previous_task = driver.previous_task;
    auto &observed_frame = driver.observed_frame;
    const auto require = [&](E error, const char *label) {
        if (error != E::none)
            throw std::runtime_error(
                std::string("natural progression command ") + label +
                " error=" + std::to_string(static_cast<int>(error)) +
                " raw=" + std::to_string(top_page(session.state())->legacy_page) +
                " page=" + std::to_string(top_page(session.state())->id) +
                " source=" + std::to_string(top_page(session.state())->source_record) + " " +
                snapshot(session.state(), observed_frame));
    };
    const auto income = [](const auto &s) {
        std::int64_t total{};
        for (const auto &entry : s.scene.world.world.ai.accounting.entries())
            if (entry.second.category == ref::CashCategory::facilities &&
                entry.second.direction == ref::CashDirection::income)
                total += entry.second.amount;
        return total;
    };
    const auto expansion_activity = std::find_if(session.state().rules->activities.begin(),
                                                 session.state().rules->activities.end(),
                                                 [](const auto &a) { return a.identity == 25; });
    if (expansion && options.load_file.empty())
        check(expansion_activity != session.state().rules->activities.end(),
              "fixed activity table contains original village expansion25");
    const int expansion_cost = expansion ? expansion_activity->parameters[4] : 0;
    // 扩张在原晋级/编辑完整前缀之后继续真实经营；只延长该可选模式的有限保护。
    const int limit = expansion ? 240000 : 180000;
    for (int frame = driver.next_frame; frame < limit; ++frame) {
        round_audio.clear();
        observed_frame = frame;
        const auto step = session.update();
        if (!step.candidate)
            throw std::runtime_error("natural progression update failed " +
                                     snapshot(session.state(), frame) +
                                     " last=" + diagnose(session.state()));
        const auto &s = session.state();
        managed_resource_references(s);
        const auto usage = startup_world_resource_usage(s);
        peak_payloads = std::max(peak_payloads, usage.page_payloads);
        peak_sounds = std::max(peak_sounds, usage.sound_outputs);
        peak_retired_actors = std::max(peak_retired_actors, usage.retired_actors);
        peak_retired_encounters = std::max(peak_retired_encounters, usage.retired_encounters);
        peak_pages = std::max(peak_pages, usage.pages);
        peak_effects = std::max(peak_effects, usage.effects);
        peak_actors = std::max(peak_actors, usage.live_actors + usage.retired_actors);
        peak_cash = std::max(peak_cash, s.scene.world.world.ai.accounting.entries().size());
        peak_tasks = std::max(peak_tasks, s.tasks.size());
        const auto current = top_page(s);
        check(current != nullptr, "progression preserves real framework page");
        const auto page = *current;
        const int month = s.scene.calendar.year * 12 + s.scene.calendar.month;
        // 明确的玩家经营策略：任务结束后留出一个完整自然月供人物恢复与设施营业。
        // 只限制下一次主动接受命令，不改变任务生成、人物AI或日历。
        if (previous_task && !s.active_task)
            next_task_month = month + 2;
        previous_task = s.active_task;
        const bool save_for_exhibition = s.rank >= 1 && unlocked_completed == 0;
        const bool expansion_tail = expansion && progression_complete;
        const bool expansion_open = expansion && s.scripts.activities.at(25).status == 1;
        if (month != last_month) {
            last_month = month;
            std::cout << "progression month popularity=" << s.popularity
                      << " peak_income=" << s.maximum_income << " points=" << s.village_points
                      << " F=" << s.events_held << " q=" << s.quarter_counter << " rank=" << s.rank
                      << ' ' << snapshot(s, frame) << std::endl;
        }
        if (s.rank >= 1 && promoted_month < 0) {
            promoted_month = month;
            check(bakery && s.events_held >= 2 && s.popularity >= 300 && s.maximum_income >= 5000 &&
                      s.scene.world.world.facilities.at(*bakery).placement.definition_id == 35 &&
                      s.scripts.activities.at(16).status == 1,
                  "first star follows actual source conditions and unlocks painting exhibition16");
            std::cout << "progression promoted " << snapshot(s, frame) << std::endl;
        }
        if (expansion_tail && page.kind == ref::WorldScriptPageKind::scene &&
            s.scene.scene_state == 0) {
            if (expanded_month >= 0 && !expanded_road) {
                check(s.fence_level == 1 && s.scene.world.world.map.cells.size() == 576,
                      "natural first expansion changes boundary without resizing source map");
                const auto &map = s.scene.world.world.map;
                const auto old_fence = s.rules->fences.at(0);
                const auto new_fence = s.rules->fences.at(s.fence_level);
                const auto inside = [](ref::Position p, const auto &fence) {
                    return p.x > fence[0].x && p.x < fence[1].x && p.y > fence[1].y &&
                           p.y < fence[0].y;
                };
                const auto road = std::find_if(
                    s.rules->facilities.begin(), s.rules->facilities.end(), [&](const auto &d) {
                        return d.kind == 6 && (d.flags & 4) && s.facility_presence.at(d.id) != 0;
                    });
                check(road != s.rules->facilities.end(), "expanded town retains real road unlock");
                expanded_road_definition = road->id;
                const auto quote = startup_world_build_quote(s, expanded_road_definition);
                check(quote.has_value(), "new-area road uses source construction quote");
                for (int y = 0; y < map.height && !expanded_road; ++y)
                    for (int x = 0; x < map.width && !expanded_road; ++x) {
                        const auto &tile = map.cells.at(y * map.width + x);
                        if (inside({x, y}, new_fence) && !inside({x, y}, old_fence) &&
                            tile.legacy_state == 4 && !tile.facility)
                            expanded_road = ref::Position{x, y};
                    }
                check(expanded_road.has_value(), "expansion exposes a genuinely new editable cell");
                const auto cash = s.scene.world.world.ai.accounting.funds();
                const auto draws = s.scene.random.draws();
                const auto accepted = [&](const StartupBuildResult &r, const char *label) {
                    require(r.error, label);
                    check(r.denial == StartupBuildDenial::none,
                          "source editing consumer accepts newly expanded interior");
                };
                accepted(session.begin_road(expanded_road_definition), "enter expanded-area road");
                accepted(session.confirm_edit(*expanded_road, ref::FacilityOrientation::first),
                         "select expanded-area road start");
                accepted(session.confirm_edit(*expanded_road, ref::FacilityOrientation::first),
                         "build expanded-area road");
                require(session.cancel_edit(), "leave expanded-area road mode");
                const auto &after = session.state();
                const auto n =
                    static_cast<std::size_t>(expanded_road->y * map.width + expanded_road->x);
                check(after.scene.world.world.map.cells.at(n).legacy_state == 3 &&
                          after.surface.at(n).definition == expanded_road_definition &&
                          after.scene.world.world.ai.accounting.funds() ==
                              cash - quote->construction_cost &&
                          after.scene.random.draws() == draws,
                      "new-area road charges original quote once without random consumption");
                expanded_road_month = month;
                expanded_income = income(after);
                std::cout << "expansion road=" << expanded_road->x << ',' << expanded_road->y << ' '
                          << snapshot(after, frame) << std::endl;
            } else if (expanded_month < 0) {
                // 留100原价点数及一个季度名额；开放25后停止另开任务，等当前任务真正结束。
                const bool can_expand = expansion_open && !s.active_task &&
                                        s.village_points >= expansion_cost && s.quarter_counter > 0;
                const bool can_promote = !expansion_open && s.popularity < 1000 &&
                                         s.village_points >= expansion_cost + 20 &&
                                         s.quarter_counter > 1;
                if ((can_expand || can_promote) && frame >= next_activity_attempt) {
                    require(session.open_village_activities(),
                            "open expansion-tail village office");
                    next_activity_attempt = frame + 300;
                } else if (!expansion_open && s.popularity < 1000 && !s.active_task &&
                           month >= next_task_month &&
                           std::any_of(
                               s.task_order.begin(), s.task_order.end(),
                               [&](auto id) {
                                   return s.rules->tasks.at(s.tasks.at(id).definition)
                                              .recruitment_fee <=
                                          s.scene.world.world.ai.accounting.funds();
                               })) {
                    require(session.open_task_menu(), "open real task toward expansion unlock");
                }
            }
        } else if (page.kind == ref::WorldScriptPageKind::scene && s.scene.scene_state == 0 &&
                   edited_month < 0) {
            const auto pending = std::find_if(
                s.scene.world.facility_order.begin(), s.scene.world.facility_order.end(),
                [&](auto id) {
                    return s.scene.world.world.facility_uses
                        .at(s.scene.world.world.facilities.at(id).placement.definition_id)
                        .upgrade_pending;
                });
            if (pending != s.scene.world.facility_order.end()) {
                require(session.open_facility_page(*pending), "open natural upgrade");
            } else if (!bakery) {
                const auto quote = startup_world_build_quote(s, 35);
                if (quote &&
                    s.scene.world.world.ai.accounting.funds() >= quote->construction_cost) {
                    require(session.open_build_menu(), "open build catalogue");
                    const auto selected =
                        session.select_build_menu(top_page(session.state())->id, 35);
                    require(selected.error, "select bakery35");
                    check(selected.denial == StartupBuildDenial::none,
                          "original bakery35 is genuinely available");
                    // 邻接原道路的空地优先；每个坐标仍交实际建设消费者检验完整占地。
                    const auto &map = session.state().scene.world.world.map;
                    std::vector<ref::Position> cells;
                    for (int y = 0; y < map.height; ++y)
                        for (int x = 0; x < map.width; ++x) {
                            const auto &cell = map.cells.at(y * map.width + x);
                            if (!cell.facility && cell.legacy_state == 4)
                                cells.push_back({x, y});
                        }
                    const auto distance = [&](ref::Position p) {
                        int nearest = map.width + map.height;
                        for (int y = 0; y < map.height; ++y)
                            for (int x = 0; x < map.width; ++x)
                                if (map.cells.at(y * map.width + x).legacy_state == 3)
                                    nearest =
                                        std::min(nearest, std::abs(p.x - x) + std::abs(p.y - y));
                        return nearest;
                    };
                    std::stable_sort(cells.begin(), cells.end(),
                                     [&](auto a, auto b) { return distance(a) < distance(b); });
                    for (const auto cell : cells) {
                        const auto built =
                            session.confirm_build(cell, ref::FacilityOrientation::first);
                        require(built.error, "place bakery35");
                        if (built.created) {
                            bakery = built.created;
                            break;
                        }
                    }
                    check(bakery.has_value(),
                          "player found legal bakery placement using current map");
                    require(session.cancel_build(), "finish placement");
                    std::cout << "progression bakery " << snapshot(session.state(), frame)
                              << std::endl;
                }
            } else if (s.quarter_counter > 0 &&
                       s.village_points >=
                           (save_for_exhibition ? s.rules->activities.at(16).parameters[4] : 20) &&
                       frame >= next_activity_attempt) {
                require(session.open_village_activities(), "open village activities");
                next_activity_attempt = frame + 300;
            } else if (!s.active_task && s.rank == 0 && s.popularity < 300 &&
                       month >= next_task_month &&
                       std::any_of(
                           s.task_order.begin(), s.task_order.end(),
                           [&](auto id) {
                               return s.rules->tasks.at(s.tasks.at(id).definition)
                                          .recruitment_fee <=
                                      s.scene.world.world.ai.accounting.funds();
                           })) {
                require(session.open_task_menu(), "open affordable adventure");
            }
        } else if (page.legacy_page >= 51 && page.legacy_page <= 54) {
            const auto view = inspect_startup_world_village_activity_page(s, page.id);
            check(view.has_value(), "actual village page has initialized payload");
            if (page.legacy_page == 51) {
                int selected = -1;
                for (int n = 0; n < static_cast<int>(view->entries.size()); ++n) {
                    const auto &a = s.rules->activities.at(view->entries[n]);
                    if (expansion_tail) {
                        if (expansion_open) {
                            if (a.identity == 25 && !s.active_task &&
                                a.parameters[4] <= s.village_points && s.quarter_counter > 0) {
                                selected = n;
                                break;
                            }
                        } else if (a.parameters[2] == 2 && s.quarter_counter > 1 &&
                                   a.parameters[4] <= s.village_points - expansion_cost) {
                            // 原序内优先本次人气增量高的可支付活动，不改费用或原表。
                            if (selected < 0 ||
                                a.parameters[5] >
                                    s.rules->activities.at(view->entries[selected]).parameters[5])
                                selected = n;
                        }
                        continue;
                    }
                    if (save_for_exhibition && a.identity != 16)
                        continue;
                    if (a.parameters[2] <= 2 && a.parameters[4] <= s.village_points &&
                        s.quarter_counter > 0) {
                        selected = n;
                        if (a.identity == 16)
                            break; // 晋级后实际消费新开放活动。
                    }
                }
                if (selected < 0)
                    require(session.cancel_page(page.id), "leave unavailable activities");
                else {
                    require(session.act_village_activity_page(page.id, A::select, selected),
                            "select affordable activity");
                    require(session.acknowledge_page(page.id), "open activity offer");
                }
            } else if (page.legacy_page == 53) {
                if (view->counter >= 120) {
                    const int activity = *view->activity;
                    const auto before_random = s.scene.random.draws();
                    const int before_points = s.village_points, before_slots = s.quarter_counter,
                              before_held = s.events_held;
                    require(session.acknowledge_page(page.id), "complete real activity");
                    ++completed;
                    if (activity == 25) {
                        const auto &after = session.state();
                        check(expansion_tail && expanded_month < 0 && after.fence_level == 1 &&
                                  after.village_points == before_points &&
                                  after.quarter_counter == before_slots - 1 &&
                                  after.events_held == before_held &&
                                  after.activity_counts.at(25) == 1 &&
                                  after.scene.random.draws() == before_random &&
                                  top_page(after)->legacy_page != 54,
                              "natural expansion completes once at53 without duplicate52 payment "
                              "or human-result random draws");
                        expanded_month = month;
                        std::cout << "expansion completed " << snapshot(after, frame) << std::endl;
                    }
                    if (activity == 16) {
                        ++unlocked_completed;
                        if (unlocked_month < 0) {
                            unlocked_month = month;
                            unlocked_income = income(session.state());
                        }
                    }
                    std::cout << "progression activity=" << activity << ' '
                              << snapshot(session.state(), frame) << std::endl;
                }
            } else
                require(session.acknowledge_page(page.id), "confirm activity offer/result");
        } else if (page.legacy_page == 22) {
            const auto &list = s.task_page_lists.at(page.id);
            const auto affordable = std::find_if(list.begin(), list.end(), [&](auto id) {
                return s.rules->tasks.at(s.tasks.at(id).definition).recruitment_fee <=
                       s.scene.world.world.ai.accounting.funds();
            });
            check(affordable != list.end(), "natural management chooses an actual affordable task");
            require(session
                        .act_task_page(page.id, StartupWorldTaskAction::confirm,
                                       static_cast<int>(affordable - list.begin()))
                        .error,
                    "select actual adventure");
        } else if (page.legacy_page == 23) {
            require(session.act_task_page(page.id, StartupWorldTaskAction::confirm).error,
                    "accept actual adventure");
        } else if (page.legacy_page == 25) {
            require(session.act_task_page(page.id, StartupWorldTaskAction::depart).error,
                    "depart real recruited team");
        } else if (page.legacy_page == 28) {
            if (s.page_phases.at(page.id) == 0)
                require(session.act_task_page(page.id, StartupWorldTaskAction::confirm).error,
                        "confirm departure animation");
        } else if (page.legacy_page == 33) {
            require(session
                        .act_task_page(
                            page.id, StartupWorldTaskAction::confirm,
                            s.scene.world.world.ai.accounting.funds() >= page.legacy_f ? 0 : 1)
                        .error,
                    "renew affordable task or explicitly abort");
        } else if (page.legacy_page == 81) {
            if (seen_upgrades.insert(page.id).second) {
                const auto facility = s.facility_page_bindings.at(page.id);
                const int definition =
                    s.scene.world.world.facilities.at(facility).placement.definition_id;
                check(s.scene.world.world.facility_uses.at(definition).level > 1,
                      "natural usage threshold really raises shared facility level");
                upgraded.insert(definition);
                std::cout << "progression upgraded=" << definition << ' ' << snapshot(s, frame)
                          << std::endl;
            }
            require(session.acknowledge_page(page.id), "confirm upgrade presentation");
        } else if (page.legacy_page == 48) {
            require(session.act_rank_page(page.id, 0, edited_month >= 0),
                    "apply rank or return post-edit report");
        } else if (page.legacy_page == 87) {
            if (s.medal_count > 0) {
                require(session.act_award_page(page.id, ref::WorldAwardAction::request_award, 0),
                        "request annual medal");
                require(session.act_award_page(page.id, ref::WorldAwardAction::confirm_award),
                        "award annual medal");
            }
        } else if (page.legacy_page == 83)
            require(session.cancel_page(page.id), "leave shop");
        else if (page.kind != ref::WorldScriptPageKind::scene && page.legacy_page != 16 &&
                 page.legacy_page != 24 && page.legacy_page != 56 && page.legacy_page != 57 &&
                 page.legacy_page != 97 && page.legacy_page != 98)
            require(session.acknowledge_page(page.id), "confirm actual world event");
        consume_audio();
        check(session.state().sound_requests.empty(),
              "progression sink consumes sound outputs once per frame");
        if (edited_month < 0 && unlocked_month >= 0 && month > unlocked_month &&
            !upgraded.empty() && unlocked_completed > 0 &&
            s.scripts.pages.back().kind == ref::WorldScriptPageKind::scene &&
            s.scene.calendar.units >= 27) {
            check(income(s) > unlocked_income && completed >= 2 &&
                      s.activity_pages_initialized.empty(),
                  "newly unlocked activity finishes and post-rank world keeps earning income with "
                  "retired pages");
            std::cout << "progression original prefix completed " << snapshot(s, frame)
                      << std::endl;
            edited_retired = natural_editing_commands(session, require);
            edited_month = month;
            edited_income = income(session.state());
            peak_sounds = std::max(peak_sounds, session.state().sound_requests.size());
            consume_audio();
            check(session.state().sound_requests.empty(),
                  "editing command sound outputs consumed once");
            std::cout << "progression edited old=" << edited_retired[0]
                      << " moved=" << edited_retired[1] << ' ' << snapshot(session.state(), frame)
                      << std::endl;
        }
        if (!progression_complete && edited_month >= 0 && month > edited_month &&
            s.scene.calendar.units >= 27 && top_page(s) &&
            top_page(s)->kind == ref::WorldScriptPageKind::scene) {
            check(income(s) > edited_income &&
                      !s.scene.world.world.facilities.count(edited_retired[0]) &&
                      !s.scene.world.world.facilities.count(edited_retired[1]) &&
                      !s.facility_month_age.count(edited_retired[0]) &&
                      !s.facility_month_age.count(edited_retired[1]) && !s.build_moving_facility &&
                      !s.build_anchor,
                  "post-edit natural month earns new facility income and keeps retired references "
                  "absent");
            std::cout << "progression summary completed=" << completed
                      << " unlocked=" << unlocked_completed << " upgraded=" << upgraded.size()
                      << " sounds=" << sounds << " peak_pages=" << peak_pages
                      << " peak_payloads=" << peak_payloads << " peak_effects=" << peak_effects
                      << " peak_sounds=" << peak_sounds
                      << " peak_retired_actors=" << peak_retired_actors
                      << " peak_retired_encounters=" << peak_retired_encounters
                      << " peak_actors=" << peak_actors << " peak_cash=" << peak_cash
                      << " peak_tasks=" << peak_tasks << " edited_month=" << edited_month
                      << " checkpoints=" << session.checkpoints().size() << ' '
                      << snapshot(s, frame) << '\n';
            if (!expansion)
                driver.terminal = true;
            else {
                // 原模式的全部断言和终点先完成；可选尾段从下一帧才改变玩家命令。
                if (seed == 1 && speed == 0)
                    check(frame == 38282 && s.scene.world.world.ai.accounting.funds() == 23388 &&
                              s.scene.random.draws() == 322697,
                          "expansion retains the accepted first-star/editing golden prefix");
                progression_complete = true;
            }
        }
        if (expansion_tail && expanded_road_month >= 0 && month >= expanded_road_month + 2 &&
            s.scene.calendar.units >= 27 && top_page(s) &&
            top_page(s)->kind == ref::WorldScriptPageKind::scene && !s.active_task) {
            const auto n = static_cast<std::size_t>(
                expanded_road->y * s.scene.world.world.map.width + expanded_road->x);
            check(s.fence_level == 1 && s.activity_counts.at(25) == 1 &&
                      s.scene.world.world.map.cells.size() == 576 &&
                      s.scene.world.world.map.cells.at(n).legacy_state == 3 &&
                      s.surface.at(n).definition == expanded_road_definition &&
                      income(s) > expanded_income && s.activity_pages_initialized.empty() &&
                      !s.build_anchor && !s.build_moving_facility,
                  "expanded town keeps new-area road and earns through a full natural month "
                  "with retired village pages");
            // 本批优化前完整自然轨迹的维护黄金值；不是APK原始随机seed或注入夹具。
            if (seed == 1 && speed == 0)
                check(frame == 111808 && s.scene.world.world.ai.accounting.funds() == 138463 &&
                          s.scene.random.draws() == 1047804 && s.popularity == 1024,
                      "performance changes retain the accepted natural expansion endpoint");
            std::cout << "expansion summary expanded_month=" << expanded_month
                      << " road_month=" << expanded_road_month << " sounds=" << sounds
                      << " peak_pages=" << peak_pages << " peak_payloads=" << peak_payloads
                      << " peak_effects=" << peak_effects << " peak_sounds=" << peak_sounds
                      << " peak_retired_actors=" << peak_retired_actors
                      << " peak_retired_encounters=" << peak_retired_encounters
                      << " peak_actors=" << peak_actors << " peak_cash=" << peak_cash
                      << " peak_tasks=" << peak_tasks
                      << " checkpoints=" << session.checkpoints().size() << ' '
                      << snapshot(s, frame) << '\n';
            driver.terminal = true;
        }
        const bool stopped = round_boundary(frame);
        if (driver.terminal || stopped) {
            if (!options.save_file.empty() && !saved)
                throw std::runtime_error("natural scenario ended before requested snapshot frame");
            if (trace.is_open()) {
                trace.flush();
                if (!trace) throw std::runtime_error("cannot flush natural replay trace");
            }
            if (stopped && !driver.terminal)
                std::cout << "natural replay bounded prefix next_frame=" << driver.next_frame
                          << ' ' << snapshot(session.state(), frame) << std::endl;
            return;
        }
    }
    throw std::runtime_error("natural progression limit reached " +
                             snapshot(session.state(), limit));
}

// 同一continuous套件中的控制器协议边界；仅显式模式执行，不增加自然黄金轨迹checks。
void replay_driver_contract() {
    using D = test::StartupWorldReplayDriver;
    D d;
    d.seed = 123;
    d.expansion = true;
    d.speed = 1;
    d.next_frame = 1234;
    d.observed_frame = 1233;
    d.checks = 5678;
    d.bakery = 101;
    d.upgraded = {28, 35};
    d.seen_upgrades = {201, 202};
    d.last_month = 15;
    d.promoted_month = 11;
    d.unlocked_month = 12;
    d.unlocked_income = 301;
    d.edited_month = 13;
    d.edited_income = 401;
    d.edited_retired = {501, 502};
    d.progression_complete = true;
    d.expanded_month = 14;
    d.expanded_road_month = 15;
    d.expanded_income = 601;
    d.expanded_road = ref::Position{4, 3};
    d.expanded_road_definition = 18;
    d.sounds = 701; d.peak_sounds = 702; d.peak_payloads = 703; d.peak_pages = 704;
    d.peak_effects = 705; d.peak_actors = 706; d.peak_cash = 707; d.peak_tasks = 708;
    d.peak_retired_actors = 709; d.peak_retired_encounters = 710;
    d.next_activity_attempt = 1240;
    d.completed = 7;
    d.unlocked_completed = 2;
    d.next_task_month = 16;
    d.previous_task = 801;
    const auto bytes = test::encode_startup_world_replay_driver(d);
    const auto restored = test::decode_startup_world_replay_driver(bytes);
    check(restored.seed == 123 && restored.speed == 1 && restored.expansion &&
              restored.next_frame == 1234 && restored.observed_frame == 1233 &&
              restored.checks == 5678 && !restored.terminal,
          "replay driver retains scenario identity and exact next complete round");
    check(restored.bakery == d.bakery && restored.upgraded == d.upgraded &&
              restored.seen_upgrades == d.seen_upgrades && restored.last_month == 15 &&
              restored.promoted_month == 11 && restored.unlocked_month == 12 &&
              restored.unlocked_income == 301 && restored.edited_month == 13 &&
              restored.edited_income == 401 && restored.edited_retired == d.edited_retired &&
              restored.progression_complete && restored.expanded_month == 14 &&
              restored.expanded_road_month == 15 && restored.expanded_income == 601 &&
              restored.expanded_road && restored.expanded_road->x == 4 &&
              restored.expanded_road->y == 3 && restored.expanded_road_definition == 18,
          "replay driver preserves natural progression command stages and income baselines");
    check(restored.sounds == 701 && restored.peak_sounds == 702 &&
              restored.peak_payloads == 703 && restored.peak_pages == 704 &&
              restored.peak_effects == 705 && restored.peak_actors == 706 &&
              restored.peak_cash == 707 && restored.peak_tasks == 708 &&
              restored.peak_retired_actors == 709 && restored.peak_retired_encounters == 710 &&
              restored.next_activity_attempt == 1240 && restored.completed == 7 &&
              restored.unlocked_completed == 2 && restored.next_task_month == 16 &&
              restored.previous_task == d.previous_task,
          "replay driver preserves output accounting, resource peaks, and command cooldowns");
    const auto rejected = [](const auto &operation) {
        try { operation(); } catch (const std::invalid_argument &) { return true; }
        return false;
    };
    test::StartupWorldReplayOptions periodic;
    test::validate_startup_world_replay_options(periodic, 0, 180000);
    check(!test::startup_world_periodic_snapshot_due(periodic, 5000),
          "periodic replay snapshots are disabled unless explicitly requested");
    periodic.save_every = 5000;
    check(rejected([&] { test::validate_startup_world_replay_options(periodic, 0, 180000); }),
          "periodic replay cadence requires an explicit output directory");
    periodic.save_directory = "periodic-contract-only";
    test::validate_startup_world_replay_options(periodic, 5001, 180000);
    check(!test::startup_world_periodic_snapshot_due(periodic, 0) &&
              !test::startup_world_periodic_snapshot_due(periodic, 4999) &&
              test::startup_world_periodic_snapshot_due(periodic, 5000) &&
              !test::startup_world_periodic_snapshot_due(periodic, 5001) &&
              test::startup_world_periodic_snapshot_due(periodic, 10000),
          "periodic snapshots use completed global frames and retain cadence after resume");
    periodic.save_at = 38000;
    periodic.save_file = "primary-contract-only.awr";
    test::validate_startup_world_replay_options(periodic, 0, 180000);
    check(periodic.save_at == 38000 && !test::startup_world_periodic_snapshot_due(periodic, 38000),
          "periodic snapshots do not replace the independent certification save frame");
    for (const int invalid : {0, -1, 180000}) {
        periodic.save_every = invalid;
        check(rejected([&] { test::validate_startup_world_replay_options(periodic, 0, 180000); }),
              "periodic replay rejects nonpositive or out-of-range cadence");
    }
    periodic.save_every.reset();
    check(rejected([&] { test::validate_startup_world_replay_options(periodic, 0, 180000); }),
          "periodic replay directory alone does not imply permission to save");
    for (std::size_t size = 0; size < bytes.size(); ++size) {
        const std::vector<std::uint8_t> truncated(bytes.begin(), bytes.begin() + size);
        check(rejected([&] { (void)test::decode_startup_world_replay_driver(truncated); }),
              "truncated replay driver is rejected before installation");
    }
    auto corrupt = bytes;
    corrupt.push_back(0);
    check(rejected([&] { (void)test::decode_startup_world_replay_driver(corrupt); }),
          "replay driver rejects trailing bytes");
    corrupt = bytes; corrupt[0] ^= 1;
    check(rejected([&] { (void)test::decode_startup_world_replay_driver(corrupt); }),
          "replay driver rejects another schema identity");
    corrupt = bytes; corrupt[24] = 2; // v1 expansion的8字节布尔字段。
    check(rejected([&] { (void)test::decode_startup_world_replay_driver(corrupt); }),
          "replay driver rejects noncanonical boolean");
    corrupt = bytes;
    std::copy(corrupt.begin() + 88, corrupt.begin() + 96, corrupt.begin() + 96);
    check(rejected([&] { (void)test::decode_startup_world_replay_driver(corrupt); }),
          "replay driver rejects duplicate upgraded definitions rather than dropping them");
    for (int scenario = 0; scenario != 7; ++scenario) {
        auto bad = d;
        switch (scenario) {
        case 0: bad.next_frame = 0; break;
        case 1: --bad.observed_frame; break;
        case 2: bad.speed = 2; break;
        case 3: bad.checks = -1; break;
        case 4: bad.unlocked_completed = bad.completed + 1; break;
        case 5: bad.expanded_road.reset(); break;
        case 6: bad.expansion = false; break;
        }
        check(rejected([&] { (void)test::encode_startup_world_replay_driver(bad); }),
              "replay driver rejects invalid frame, mode, stage, or statistics");
    }
    // 普通晋级终点只置terminal；扩张终点同时保留progression_complete前缀标志。
    d.terminal = true;
    check(test::decode_startup_world_replay_driver(
              test::encode_startup_world_replay_driver(d)).terminal,
          "expansion endpoint remains a valid complete-round snapshot");
    d.expansion = false;
    d.progression_complete = false;
    d.expanded_month = d.expanded_road_month = -1;
    d.expanded_income = 0;
    d.expanded_road.reset();
    d.expanded_road_definition = -1;
    check(test::decode_startup_world_replay_driver(
              test::encode_startup_world_replay_driver(d)).terminal,
          "progression endpoint remains a valid complete-round snapshot");
    StartupSession initial;
    StartupWorldRuntimeSession session(initial.state(), ref::WorldRandomStream::from_java_seed(1));
    const auto step = session.update();
    check(step.candidate.has_value(), "driver ownership fixture completes one real framework round");
    (void)session.take_sound_requests();
    D first;
    first.next_frame = 1;
    first.observed_frame = 0;
    first.checks = 1;
    first.last_month = session.state().scene.calendar.year * 12 + session.state().scene.calendar.month;
    test::validate_startup_world_replay_driver_world(first, session.state());
    for (int scenario = 0; scenario != 8; ++scenario) {
        auto bad = first;
        switch (scenario) {
        case 0: bad.bakery = session.state().next_facility_identity; break;
        case 1: bad.previous_task = session.state().next_task_identity; break;
        case 2: bad.seen_upgrades.insert(session.state().scripts.next_page_id); break;
        case 3: bad.upgraded.insert(35); break;
        case 4: bad.edited_retired = {1, 2}; break;
        case 5: bad.speed = 1; break;
        case 6: ++bad.last_month; break;
        case 7: bad.checks = 0; break;
        }
        check(rejected([&] {
                  test::validate_startup_world_replay_driver_world(bad, session.state());
              }),
              "driver cannot install dangling or contradictory progress into valid Owner");
    }
}

// 新局实际库存先赠人/投设施，真实任务开放商会后买卖，再留出完整自然月营业。
// 这是显式玩家策略；不改其它自然链与默认黄金轨迹，也不补钱、人物、库存或flags。
void natural_tools(std::uint64_t seed, int speed) {
    StartupSession initial;
    StartupWorldRuntimeSession session(initial.state(),
                                       ref::WorldRandomStream::from_java_seed(seed));
    session.set_speed(speed);
    using E = StartupWorldRuntimeError;
    using H = StartupHumanPageAction;
    using C = StartupCommerceAction;
    check(session.state().items.at(0).inventory == 2 && session.state().items.at(29).inventory == 3,
          "natural tools begins with original two potatoes and three medicines");
    std::optional<int> recipient, bought_item;
    bool person_gift{}, facility_gift{}, facility_effect{}, bought{}, sold{};
    int commerce_month = -1, complete_month = -1, last_month = -1, observed_frame{};
    std::int64_t commerce_income{};
    const int old_improvement = session.state().scripts.facilities.at(33).improvements[1];
    const auto &start = session.state();
    const auto original_bun = std::find_if(
        start.scene.world.facility_order.begin(), start.scene.world.facility_order.end(),
        [&](auto id) {
            return start.scene.world.world.facilities.at(id).placement.definition_id == 33;
        });
    check(original_bun != start.scene.world.facility_order.end(),
          "natural source map contains original bun shop");
    const auto bun = *original_bun;
    std::size_t sounds{}, peak_sounds{}, peak_payloads{}, peak_pages{}, peak_effects{},
        peak_actors{}, peak_cash{}, peak_tasks{}, peak_retired_actors{}, peak_retired_encounters{};
    const auto require = [&](E error, const char *label) {
        if (error == E::none)
            return;
        const auto *p = top_page(session.state());
        throw std::runtime_error(std::string("natural tools command ") + label +
                                 " error=" + std::to_string(static_cast<int>(error)) +
                                 " raw=" + std::to_string(p ? p->legacy_page : -1) + ' ' +
                                 snapshot(session.state(), observed_frame));
    };
    const auto income = [](const auto &s) {
        std::int64_t total{};
        for (const auto &entry : s.scene.world.world.ai.accounting.entries())
            if (entry.second.category == ref::CashCategory::facilities &&
                entry.second.direction == ref::CashDirection::income)
                total += entry.second.amount;
        return total;
    };
    const auto affordable_task = [](const auto &s, std::uint64_t id) {
        return s.rules->tasks.at(s.tasks.at(id).definition).recruitment_fee <=
               s.scene.world.world.ai.accounting.funds();
    };
    const auto affordable_stock = [](const auto &s) {
        return std::any_of(s.rules->items.begin(), s.rules->items.end(), [&](const auto &i) {
            return s.shop_item_stock.at(i.identity).quantity > 0 &&
                   s.scene.world.world.ai.accounting.funds() >= i.commerce_price;
        });
    };
    constexpr int limit = 180000;
    for (int frame = 0; frame < limit; ++frame) {
        observed_frame = frame;
        const auto old_draws = session.state().scene.random.draws();
        const auto step = session.update();
        if (!step.candidate) {
            const auto &failed = session.state();
            std::ostringstream details;
            details << "natural tools update failed " << snapshot(failed, frame)
                    << " error=" << static_cast<int>(step.error)
                    << " scene_error=" << static_cast<int>(step.scene_error)
                    << " world_error=" << static_cast<int>(step.world_error) << " bought=" << bought
                    << " sold=" << sold;
            for (const auto &p : failed.scripts.pages) {
                const auto counter = failed.page_counters.find(p.id);
                details << " page=" << p.id << ":raw" << p.legacy_page << ":life" << p.lifecycle
                        << ":source" << p.source_record << ":f/r/s=" << p.legacy_f << '/'
                        << p.legacy_r << '/' << p.legacy_s << ":counter"
                        << (counter == failed.page_counters.end() ? -1 : counter->second)
                        << ":initialized=" << failed.commerce_pages_initialized.count(p.id) << '/'
                        << failed.human_pages_initialized.count(p.id) << '/'
                        << failed.facility_item_pages_initialized.count(p.id);
            }
            for (const auto &[id, item] : failed.items) {
                const auto c = failed.catalog.find({0, id});
                if (c == failed.catalog.end() || c->second.inventory != item.inventory ||
                    c->second.status != item.status ||
                    c->second.unlock_counter != item.unlock_counter ||
                    c->second.newly_unlocked != item.newly_unlocked) {
                    details << " item-mismatch=" << id << ":items=" << item.inventory << '/'
                            << item.status << '/' << item.unlock_counter << '/'
                            << item.newly_unlocked;
                    if (c != failed.catalog.end())
                        details << ":catalog=" << c->second.inventory << '/' << c->second.status
                                << '/' << c->second.unlock_counter << '/'
                                << c->second.newly_unlocked;
                }
            }
            throw std::runtime_error(details.str() + " last=" + diagnose(failed));
        }
        const auto &s = session.state();
        managed_resource_references(s);
        check(s.scene.random.draws() >= old_draws &&
                  ref::valid_world_calendar_state(s.scene.calendar),
              "natural tools retains common monotonic random and normalized calendar");
        const auto usage = startup_world_resource_usage(s);
        peak_sounds = std::max(peak_sounds, usage.sound_outputs);
        peak_payloads = std::max(peak_payloads, usage.page_payloads);
        peak_pages = std::max(peak_pages, usage.pages);
        peak_effects = std::max(peak_effects, usage.effects);
        peak_actors = std::max(peak_actors, usage.live_actors + usage.retired_actors);
        peak_retired_actors = std::max(peak_retired_actors, usage.retired_actors);
        peak_retired_encounters = std::max(peak_retired_encounters, usage.retired_encounters);
        peak_cash = std::max(peak_cash, s.scene.world.world.ai.accounting.entries().size());
        peak_tasks = std::max(peak_tasks, s.tasks.size());
        const int month = s.scene.calendar.year * 12 + s.scene.calendar.month;
        if (month != last_month) {
            last_month = month;
            std::cout << "tools month person=" << person_gift << " facility=" << facility_effect
                      << " success=" << s.task_progress.successes << " bought=" << bought
                      << " sold=" << sold << ' ' << snapshot(s, frame) << std::endl;
        }
        if (facility_gift && !facility_effect &&
            s.scripts.facilities.at(33).improvements[1] == old_improvement + 2) {
            facility_effect = true;
            check(s.facility_item_confirmations.at(bun) == 1,
                  "natural original potato reaches76 and advances actual bun-shop instance count");
            std::cout << "tools facility improved " << snapshot(s, frame) << std::endl;
        }
        const auto current = top_page(s);
        check(current, "natural tools preserves real framework page");
        const auto page = *current;
        if (page.kind == ref::WorldScriptPageKind::scene && s.scene.scene_state == 0) {
            if (!person_gift && !s.scene.world.world.ai.human_order.empty()) {
                const auto actor = s.scene.world.world.ai.human_order.front();
                recipient = s.scene.world.world.ai.battle.actors.at(actor).definition;
                require(session.open_human_page(*recipient), "inspect natural arrival");
            } else if (person_gift && !facility_gift) {
                require(session.open_facility_page(bun), "inspect original bun shop");
            } else if (facility_effect && !(s.scripts.user_flags & 16U) && !s.active_task &&
                       std::any_of(s.task_order.begin(), s.task_order.end(),
                                   [&](auto id) { return affordable_task(s, id); })) {
                require(session.open_task_menu(), "open natural available adventure");
            } else if (facility_effect && s.task_progress.successes > 0 && !s.active_task &&
                       !sold && (s.scripts.user_flags & 16U)) {
                // 探索成功不保证开放商会；持续真实任务直至原事件／讨伐链给出开放位。
                // 开放后等待真实商会补货或下一次自然月补货。
                if (bought || affordable_stock(s))
                    require(session.open_commerce(), "open genuinely unlocked commerce");
            }
        } else if (page.legacy_page == 60) {
            if (recipient && !person_gift)
                require(session.act_human_page(page.id, H::gifts), "open natural recipient gifts");
            else
                require(session.cancel_page(page.id), "return human details");
        } else if (page.legacy_page == 64) {
            if (person_gift) {
                require(session.cancel_page(page.id), "return after one ordinary gift");
            } else {
                require(session.act_human_page(page.id, H::equipment_slot, 4),
                        "open ordinary items");
                const auto &list = session.state().equipment_page_catalogs.at(page.id)[4];
                const auto potato = std::find(list.begin(), list.end(), 0);
                check(potato != list.end(), "original potato appears in ordinary item inventory");
                require(session.act_human_page(page.id, H::select,
                                               static_cast<int>(potato - list.begin())),
                        "select potato");
                const auto cash = session.state().scene.world.world.ai.accounting.funds();
                const auto old_extra =
                    session.state().scene.world.world.ai.growth.at(*recipient).definition.extra[0];
                require(session.act_human_page(page.id, H::confirm), "give original potato");
                check(session.state().items.at(0).inventory == 1 &&
                          session.state().scene.world.world.ai.accounting.funds() == cash &&
                          session.state()
                                  .scene.world.world.ai.growth.at(*recipient)
                                  .definition.extra[0] == old_extra + 4,
                      "natural64 consumes original item and applies actual m4 without cash "
                      "injection");
                person_gift = true;
                std::cout << "tools human gifted=" << *recipient << ' '
                          << snapshot(session.state(), frame) << std::endl;
            }
        } else if (page.legacy_page == 74) {
            require(session.act_facility_page(page.id, facility_gift
                                                           ? StartupFacilityPageAction::cancel
                                                           : StartupFacilityPageAction::confirm),
                    "use or return original facility details");
        } else if (page.legacy_page == 75) {
            if (facility_gift) {
                require(session.cancel_page(page.id), "return facility item catalogue");
            } else {
                const auto &list = s.facility_item_page_lists.at(page.id);
                const auto potato = std::find(list.begin(), list.end(), 0);
                check(potato != list.end(), "remaining original potato appears in75");
                require(session.act_facility_item_page(page.id, StartupFacilityItemAction::select,
                                                       static_cast<int>(potato - list.begin())),
                        "select facility potato");
                require(session.acknowledge_page(page.id), "apply facility potato");
                check(session.state().items.at(0).inventory == 0 &&
                          session.state().scripts.facilities.at(33).improvements[1] ==
                              old_improvement,
                      "natural75 exhausts potato before actual76 improvement");
                facility_gift = true;
            }
        } else if (page.legacy_page == 22) {
            const auto &list = s.task_page_lists.at(page.id);
            const auto task = std::find_if(list.begin(), list.end(),
                                           [&](auto id) { return affordable_task(s, id); });
            check(task != list.end(), "natural tools selects affordable actual task");
            require(session
                        .act_task_page(page.id, StartupWorldTaskAction::confirm,
                                       static_cast<int>(task - list.begin()))
                        .error,
                    "select task");
        } else if (page.legacy_page == 23) {
            require(session.act_task_page(page.id, StartupWorldTaskAction::confirm).error,
                    "accept task");
        } else if (page.legacy_page == 25) {
            require(session.act_task_page(page.id, StartupWorldTaskAction::depart).error,
                    "depart recruited team");
        } else if (page.legacy_page == 28) {
            if (s.page_phases.at(page.id) == 0)
                require(session.act_task_page(page.id, StartupWorldTaskAction::confirm).error,
                        "confirm departure");
        } else if (page.legacy_page == 33) {
            require(session
                        .act_task_page(
                            page.id, StartupWorldTaskAction::confirm,
                            s.scene.world.world.ai.accounting.funds() >= page.legacy_f ? 0 : 1)
                        .error,
                    "renew affordable adventure or explicitly abort");
        } else if (page.legacy_page == 83) {
            // 事件36也可在本轮调度尾部刚插入83；与主动入口一样等待真实首次初始化。
            if (!s.commerce_pages_initialized.count(page.id)) {
                // 仍执行本轮末尾声音消费和资源检查。
            } else if (s.task_progress.successes == 0 || sold ||
                       (!bought && !affordable_stock(s))) {
                require(session.cancel_page(page.id), "return commerce");
            } else {
                require(session.act_commerce_page(page.id, C::select, bought ? 1 : 0),
                        "select buy or sell catalogue");
                require(session.act_commerce_page(page.id, C::confirm), "open trade catalogue");
            }
        } else if (page.legacy_page == 84) {
            const auto view = inspect_startup_world_commerce_page(s, page.id);
            check(view.has_value(), "natural commerce catalogue has initialized payload");
            if ((view->mode == 0 && bought) || (view->mode == 1 && sold)) {
                require(session.cancel_page(page.id), "return completed trade catalogue");
            } else {
                int index = -1;
                for (int n = 0; n < static_cast<int>(view->entries.size()); ++n) {
                    const auto id = view->entries[n];
                    if (view->mode == 1 ? bought_item == id
                                        : s.rules->items.at(id).commerce_price <=
                                              s.scene.world.world.ai.accounting.funds()) {
                        index = n;
                        break;
                    }
                }
                check(index >= 0,
                      "natural trade has affordable stock or the genuinely purchased item");
                const int item = view->entries[index];
                const int old_stock = s.items.at(item).inventory;
                const int old_shop = s.shop_item_stock.at(item).quantity;
                const auto cash = s.scene.world.world.ai.accounting.funds();
                const int price =
                    s.rules->items.at(item).commerce_price / (view->mode == 1 ? 2 : 1);
                require(session.act_commerce_page(page.id, C::select, index),
                        "select real stocked item");
                require(session.act_commerce_page(page.id, C::confirm),
                        "commit actual buy or sale");
                const bool sale = view->mode == 1;
                check(session.state().items.at(item).inventory == old_stock + (sale ? -1 : 1) &&
                          session.state().shop_item_stock.at(item).quantity ==
                              old_shop - (sale ? 0 : 1) &&
                          session.state().scene.world.world.ai.accounting.funds() ==
                              cash + (sale ? price : -price),
                      "natural trade exactly changes real cash, owned stock and distinct commerce "
                      "stock");
                if (sale) {
                    sold = true;
                    commerce_month = month;
                    complete_month = month + 2; // 跨过成交月尾及下一整月，不把半个月当完整经营月。
                    commerce_income = income(session.state());
                } else {
                    bought = true;
                    bought_item = item;
                }
                std::cout << "tools trade mode=" << view->mode << " item=" << item << ' '
                          << snapshot(session.state(), frame) << std::endl;
            }
        } else if (page.legacy_page == 77 && !s.facility_item_pages_initialized.count(page.id)) {
            // 76本次更新刚换入77；玩家等下一次真实初始化，不能在首轮载荷创建前确认。
        } else if (page.legacy_page == 48) {
            require(session.act_rank_page(page.id, 0, true), "return rank conditions");
        } else if (page.legacy_page == 87) {
            if (s.medal_count > 0) {
                require(session.act_award_page(page.id, ref::WorldAwardAction::request_award, 0),
                        "request annual medal");
                require(session.act_award_page(page.id, ref::WorldAwardAction::confirm_award),
                        "award annual medal");
            }
        } else if (page.kind != ref::WorldScriptPageKind::scene && page.legacy_page != 16 &&
                   page.legacy_page != 24 && page.legacy_page != 56 && page.legacy_page != 57 &&
                   page.legacy_page != 76 && page.legacy_page != 86 && page.legacy_page != 97 &&
                   page.legacy_page != 98) {
            require(session.acknowledge_page(page.id), "confirm source event or item result");
        }
        sounds += session.take_sound_requests().size();
        check(session.state().sound_requests.empty(),
              "natural tools consumes transient sound outputs exactly once");
        const auto &after = session.state();
        const auto *remaining = top_page(after);
        if (sold && month >= complete_month && after.scene.calendar.units >= 27 && remaining &&
            remaining->kind == ref::WorldScriptPageKind::scene) {
            check(person_gift && facility_effect && bought && after.task_progress.successes > 0 &&
                      income(after) > commerce_income &&
                      after.facility_item_pages_initialized.empty() &&
                      after.facility_item_page_lists.empty() &&
                      after.facility_item_page_items.empty() &&
                      after.commerce_pages_initialized.empty() &&
                      after.commerce_page_data.empty() && after.commerce_page_lists.empty(),
                  "real gifts, task restock and buy/sale retire pages and continue a complete "
                  "income month");
            std::cout << "tools summary success=" << after.task_progress.successes
                      << " bought=" << *bought_item << " commerce_month=" << commerce_month
                      << " sounds=" << sounds << " peak_pages=" << peak_pages
                      << " peak_payloads=" << peak_payloads << " peak_effects=" << peak_effects
                      << " peak_sounds=" << peak_sounds << " peak_actors=" << peak_actors
                      << " peak_retired_actors=" << peak_retired_actors
                      << " peak_retired_encounters=" << peak_retired_encounters
                      << " peak_cash=" << peak_cash << " peak_tasks=" << peak_tasks
                      << " checkpoints=" << session.checkpoints().size() << ' '
                      << snapshot(after, frame) << '\n';
            return;
        }
    }
    throw std::runtime_error(
        "natural tools frame limit reached person=" + std::to_string(person_gift) +
        " facility=" + std::to_string(facility_effect) + " bought=" + std::to_string(bought) +
        " sold=" + std::to_string(sold) + ' ' + snapshot(session.state(), limit));
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
        if (argc == 2 && std::string(argv[1]) == "replay_driver_contract") {
            replay_driver_contract();
            std::cout << "natural replay driver: " << checks << " checks\n";
            return 0;
        }
        if (argc >= 2 && std::string(argv[1]) == "natural_tools") {
            if (argc > 4)
                throw std::invalid_argument("expected natural_tools [seed [speed]]");
            if (argc >= 3)
                parse(argv[2], seed);
            if (argc >= 4)
                parse(argv[3], speed);
            if (speed != 0 && speed != 1)
                throw std::invalid_argument("invalid natural_tools speed");
            natural_tools(seed, speed);
            std::cout << "natural tools: " << checks << " checks\n";
            return 0;
        }
        if (argc >= 2 && (std::string(argv[1]) == "natural_progression" ||
                          std::string(argv[1]) == "natural_expansion")) {
            test::StartupWorldReplayOptions options;
            int arg = 2;
            if (arg < argc && std::string(argv[arg]).rfind("--", 0) != 0) {
                parse(argv[arg++], seed);
                options.seed_explicit = true;
            }
            if (arg < argc && std::string(argv[arg]).rfind("--", 0) != 0) {
                parse(argv[arg++], speed);
                options.speed_explicit = true;
            }
            std::set<std::string> seen_options;
            while (arg < argc) {
                const std::string name = argv[arg++];
                if (!seen_options.insert(name).second || arg == argc)
                    throw std::invalid_argument("duplicate or incomplete natural replay option");
                const std::string value = argv[arg++];
                if (value.empty()) throw std::invalid_argument("empty natural replay option");
                if (name == "--save-file") options.save_file = value;
                else if (name == "--save-directory") options.save_directory = value;
                else if (name == "--load-file") options.load_file = value;
                else if (name == "--trace-file") options.trace_file = value;
                else if (name == "--producer-revision") options.producer_revision = value;
                else if (name == "--save-at" || name == "--stop-at" || name == "--trace-from" ||
                         name == "--save-every") {
                    int frame{};
                    parse(value.c_str(), frame);
                    if (name == "--save-at") options.save_at = frame;
                    else if (name == "--stop-at") options.stop_at = frame;
                    else if (name == "--save-every") options.save_every = frame;
                    else options.trace_from = frame;
                } else
                    throw std::invalid_argument("unknown natural replay option: " + name);
            }
            if (speed != 0 && speed != 1)
                throw std::invalid_argument("speed must be 0 or 1");
            const auto same_path = [](const std::string &a, const std::string &b) {
                if (a.empty() || b.empty()) return false;
                std::error_code error;
                if (std::filesystem::equivalent(a, b, error) && !error) return true;
                auto x = std::filesystem::absolute(a).lexically_normal().generic_string();
                auto y = std::filesystem::absolute(b).lexically_normal().generic_string();
#ifdef _WIN32
                // 尚未存在的目标也按Windows路径大小写规则拒绝冲突。
                std::transform(x.begin(), x.end(), x.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                std::transform(y.begin(), y.end(), y.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
#endif
                return x == y;
            };
            if (same_path(options.trace_file, options.save_file) ||
                same_path(options.trace_file, options.load_file))
                throw std::invalid_argument("natural trace must not overwrite snapshot input/output");
            natural_progression(seed, speed, std::string(argv[1]) == "natural_expansion", options);
            std::cout << checks << " checks passed\n";
            return 0;
        }
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
