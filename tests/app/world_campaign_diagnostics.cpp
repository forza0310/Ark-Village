#include "world_campaign_diagnostics.hpp"
#include "../../src/simulation/persistence/startup_world_codec.hpp"
#include "../support/world_fixture.hpp"
#include "ark/assets/sha256.hpp"
#include "ark/simulation/persistence/startup_world_persistence.hpp"
#include "ark_library_contract.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace codec = sim::persistence_detail;
void require(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error("Campaign diagnostic: " + message);
}
std::string read(const std::filesystem::path &path) {
    require(std::filesystem::file_size(path) <= 64U * 1024U * 1024U, "file exceeds codec budget");
    std::ifstream stream(path, std::ios::binary);
    require(bool(stream), "cannot read " + path.string());
    return {std::istreambuf_iterator<char>(stream), {}};
}
void write_new(const std::filesystem::path &path, const std::string &bytes) {
    require(!std::filesystem::exists(path), "refuse to overwrite " + path.string());
    std::ofstream stream(path, std::ios::binary);
    stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    stream.close();
    require(bool(stream), "cannot write " + path.string());
}
const char *stage_name(sim::rules::WorldScheduleStage stage) {
    using S = sim::rules::WorldScheduleStage;
    switch (stage) {
    case S::arrival_front:
        return "arrival_front";
    case S::popularity:
        return "popularity";
    case S::decision:
        return "decision";
    case S::prefix_effects:
        return "prefix_effects";
    case S::carry_expression:
        return "carry_expression";
    case S::control:
        return "control";
    case S::actor_tail_cache:
        return "actor_tail_cache";
    case S::projectile:
        return "projectile";
    case S::object:
        return "object";
    case S::encounter:
        return "encounter";
    case S::facility:
        return "facility";
    case S::finalize:
        return "finalize";
    }
    return "unknown";
}
template <class Values> void list(std::ostream &out, const Values &values) {
    out << '[';
    bool first = true;
    for (const auto value : values) {
        if (!first)
            out << ',';
        out << value;
        first = false;
    }
    out << ']';
}
bool activity_actor(const app::WorldState &s, sim::rules::CharacterId id) {
    const auto &actors = s.scene.world.world.ai.battle.actors;
    const auto found = actors.find(id);
    if (found == actors.end() || found->second.kind != sim::rules::ActorKind::human)
        return false;
    const auto &control = found->second.control;
    return control.state == 0 || control.state == 8 ||
           (!control.queue.empty() && !control.queue.front().empty() &&
            (control.queue.front().front() == 0 || control.queue.front().front() == 8));
}
// The short form is the event key. Movement and waypoint changes are context only, so an
// otherwise unchanged travelling actor cannot flood the bounded observation log every tick.
std::string activity_actor_fields(const app::WorldState &s, sim::rules::CharacterId id,
                                  bool context) {
    const auto &world = s.scene.world.world;
    const auto found = world.ai.battle.actors.find(id);
    if (found == world.ai.battle.actors.end())
        return "live=0";
    const auto &actor = found->second;
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<float>::max_digits10)
        << "live=1 definition=" << actor.definition << " state=" << actor.control.state
        << " action=" << actor.control.action << " flags=" << actor.control.flags
        << " queue_size=" << actor.control.queue.size() << " head=";
    if (actor.control.queue.empty())
        out << "none";
    else
        list(out, actor.control.queue.front());
    const auto spending = world.human_spending.find(actor.definition);
    out << " spending=";
    if (spending == world.human_spending.end())
        out << "missing";
    else
        out << spending->second;
    const auto route = world.actors.find(id);
    if (route != world.actors.end()) {
        const auto &r = route->second;
        if (r.binding)
            out << " binding_instance=" << r.binding->instance_id.value
                << " binding_definition=" << r.binding->definition_id
                << " binding_goal=" << r.binding->goal.x << ',' << r.binding->goal.y;
        else
            out << " binding=none";
        out << " destination=";
        if (r.destination)
            out << r.destination->x << ',' << r.destination->y;
        else
            out << "none";
        out << " path_pending=" << r.path_pending << " visits=";
        list(out, r.visits.legacy_visit_counts);
        out << " category_one_count=" << r.visits.legacy_category_one_count
            << " category_six_counter=" << r.visits.legacy_category_six_counter
            << " last_visit="
            << r.visits.last_visited_instance.value_or(sim::rules::BuildingId{}).value
            << " actor_total=" << r.visits.legacy_actor_total;
        if (context) {
            out << " waypoint=" << r.waypoint << " route_kind=";
            const auto *path = r.journey ? &r.journey->route
                                        : r.unbound_route ? &*r.unbound_route : nullptr;
            out << (r.journey ? "facility" : r.unbound_route ? "ground" : "none");
            if (path) {
                out << " route_error=" << int(path->error) << " route_cost=" << path->cost
                    << " route_steps=[";
                for (std::size_t n = 0; n < path->steps.size(); ++n) {
                    if (n)
                        out << ';';
                    out << path->steps[n].x << ',' << path->steps[n].y;
                }
                out << ']';
            }
        }
    } else
        out << " route_context=missing";
    if (context) {
        out << " position=" << actor.position.x << ',' << actor.position.height << ','
            << actor.position.z << " state_counter=" << actor.state_counter
            << " action_counter=" << actor.control.action_counter;
        const auto cached = world.ai.contexts.find(id);
        if (cached != world.ai.contexts.end())
            out << " cached_cell=" << cached->second.cell.x << ',' << cached->second.cell.y;
    }
    return out.str();
}
void activity_facility_fields(std::ostream &out, const sim::rules::RescueFacility *facility) {
    if (!facility) {
        out << "live=0";
        return;
    }
    out << "live=1 definition=" << facility->placement.definition_id
        << " sales=" << facility->sales << " occupants=[";
    for (std::size_t n = 0; n < facility->occupants.size(); ++n) {
        if (n)
            out << ',';
        out << facility->occupants[n].value;
    }
    out << ']';
}
} // namespace

void inspect_campaign_activity(const app::WorldState &before, const app::WorldState &after,
                               std::uint64_t successful_rounds, std::ostream &out) {
    const auto &old_world = before.scene.world.world;
    const auto &new_world = after.scene.world.world;
    const auto prefix = [&](const char *kind) {
        out << kind << " view=actual_update_boundary round=" << successful_rounds + 1
            << " world_steps=" << before.simulation_steps << "->" << after.simulation_steps;
    };
    std::vector<sim::rules::CharacterId> humans = old_world.ai.human_order;
    for (const auto id : new_world.ai.human_order)
        if (std::find(humans.begin(), humans.end(), id) == humans.end())
            humans.push_back(id);
    for (const auto id : humans) {
        if ((!activity_actor(before, id) && !activity_actor(after, id)) ||
            activity_actor_fields(before, id, false) == activity_actor_fields(after, id, false))
            continue;
        for (const bool previous : {true, false}) {
            prefix("ACTOR_ACTIVITY");
            out << " actor=" << id.value << " phase=" << (previous ? "before" : "after") << ' '
                << activity_actor_fields(previous ? before : after, id, true) << '\n';
        }
    }
    // Shop occupancy and billing are independent of the actor filter: an intervening arrival
    // can leave neither boundary in state8/0, yet its committed sale must still be visible.
    std::set<std::uint64_t> facilities;
    for (const auto &[id, facility] : old_world.facilities)
        if (facility.kind == 3)
            facilities.insert(id);
    for (const auto &[id, facility] : new_world.facilities)
        if (facility.kind == 3)
            facilities.insert(id);
    for (const auto id : facilities) {
        const auto old = old_world.facilities.find(id);
        const auto next = new_world.facilities.find(id);
        const auto *old_facility = old == old_world.facilities.end() ? nullptr : &old->second;
        const auto *new_facility = next == new_world.facilities.end() ? nullptr : &next->second;
        if (old_facility && new_facility && old_facility->sales == new_facility->sales &&
            old_facility->occupants == new_facility->occupants)
            continue;
        for (const bool previous : {true, false}) {
            prefix("SHOP_ACTIVITY");
            out << " instance=" << id << " phase=" << (previous ? "before" : "after") << ' ';
            activity_facility_fields(out, previous ? old_facility : new_facility);
            out << '\n';
        }
    }
    const auto &old_entries = old_world.ai.accounting.entries();
    for (const auto &[id, entry] : new_world.ai.accounting.entries()) {
        if (old_entries.count(id))
            continue;
        prefix("ACTIVITY_LEDGER_ADDED");
        out << " event=" << entry.event_id << " period=" << entry.period
            << " category=" << int(entry.category) << " direction=" << int(entry.direction)
            << " amount=" << entry.amount << " cash_before=" << old_world.ai.accounting.funds()
            << " cash_after=" << new_world.ai.accounting.funds() << '\n';
    }
}

void inspect_campaign_progress(const app::WorldState &s, std::ostream &out) {
    require(s.rules != nullptr, "progress inspection needs source definitions");
    auto rank = s;
    require(sim::refresh_startup_world_runtime_rank(rank), "rank query rejected prefix");
    const auto table = sim::rules::fixed_calendar_task_rank_terms();
    const auto found = table.find(s.rank);
    if (found != table.end())
        for (std::size_t n = 0; n < found->second.size(); ++n)
            out << "RANK_REQUIREMENT target=" << s.rank + 1 << " index=" << n
                << " type=" << found->second[n].type << " value=" << rank.rank_values.at(n)
                << " threshold=" << found->second[n].threshold << " met=" << rank.rank_met.at(n)
                << '\n';
    if (s.active_task) {
        const auto &task = s.tasks.at(*s.active_task);
        out << "ACTIVE_ADVENTURE instance=" << *s.active_task << " definition=" << task.definition
            << " kind=" << s.rules->tasks.at(task.definition).factory.kind
            << " difficulty=" << task.difficulty << " participants=" << s.participants.size()
            << " facility=" << task.facility.value_or(0) << '\n';
        if (task.facility && s.dungeon_facilities.count(*task.facility)) {
            const auto &progress = s.dungeon_facilities.at(*task.facility);
            out << "ADVENTURE_PROGRESS facility=" << *task.facility
                << " updates=" << progress.updates << " progress=" << progress.progress
                << " extent=" << progress.extent << " percent=" << progress.percent
                << " challenges=" << progress.challenges.size() << '\n';
        }
    }
    const auto &ai = s.scene.world.world.ai;
    for (const auto &[id, actor] : ai.battle.actors) {
        const bool human = actor.kind == sim::rules::ActorKind::human;
        out << "ACTOR_PROGRESS id=" << id.value << " human=" << human
            << " definition=" << actor.definition << " state=" << actor.control.state
            << " baseline=" << actor.baseline << " flags=" << actor.control.flags
            << " action=" << actor.control.action << " state_counter=" << actor.state_counter
            << " hp=" << actor.hp.target << " displayed_hp=" << actor.hp.displayed
            << " capacity=" << actor.capacity << " position=" << actor.position.x << ','
            << actor.position.z << " down_timer=" << actor.down_timer
            << " rescue=" << actor.rescue.value_or(sim::rules::CharacterId{}).value
            << " encounter=" << actor.encounter.value_or(0)
            << " attack_count=" << actor.attack_count << " hits=" << actor.hit_count
            << " queue=" << actor.control.queue.size();
        if (!actor.control.queue.empty()) {
            out << " head=";
            for (const int value : actor.control.queue.front())
                out << value << ',';
        }
        if (human) {
            const auto &growth = ai.growth.at(actor.definition);
            const int job = growth.definition.current_profession;
            out << " job=" << job << " level=" << growth.definition.profession_levels.at(job)
                << " combat=";
            for (const int value : growth.derived.combat)
                out << value << ',';
            const bool roster_member = std::find(s.participants.begin(), s.participants.end(),
                                                 actor.definition) != s.participants.end();
            out << " task_roster_member=" << roster_member
                << " task_member=" << (s.active_task && roster_member);
        }
        if (const auto context = s.scene.world.world.actors.find(id);
            context != s.scene.world.world.actors.end()) {
            const auto &route = context->second;
            out << " path_pending=" << route.path_pending << " journey=" << bool(route.journey)
                << " waypoint=" << route.waypoint << " no_path=" << route.no_path_updates;
            if (route.destination)
                out << " destination=" << route.destination->x << ',' << route.destination->y;
        }
        out << '\n';
    }
}

std::string describe_campaign_failure(const sim::StartupWorldRuntimeResult &result) {
    std::ostringstream out;
    out << "runtime=" << int(result.error) << " scene=" << int(result.scene_error)
        << " world=" << int(result.world_error);
    if (result.failure) {
        const auto &failure = *result.failure;
        out << " stage=" << stage_name(failure.stage) << " id=";
        if (failure.id)
            out << *failure.id;
        else
            out << "none";
        out << " layer=" << failure.layer << " detail=" << failure.error;
    } else {
        out << " stage=unavailable";
    }
    return out.str();
}

void capture_campaign_failure(const std::filesystem::path &directory, const app::WorldState &before,
                              const sim::StartupWorldRuntimeResult &result,
                              std::uint64_t successful_rounds) {
    require(!result.candidate, "only rejected updates belong in failure evidence");
    const auto encoded = codec::encode_state(before);
    const std::string payload(encoded.begin(), encoded.end());
    std::ostringstream metadata;
    metadata << "ARK_CAMPAIGN_FAILURE_1\n"
             << codec::codec_schema_identity() << '\n'
             << sim::startup_world_persistence_dataset() << '\n'
             << assets::sha256_hex(payload) << '\n'
             << describe_campaign_failure(result) << '\n'
             << ARK_LIBRARY_CONTRACT << '\n';
    write_new(directory / "failure-state.bin", payload);
    write_new(directory / "failure-state.txt", metadata.str());
    std::cout << "FAILURE_SNAPSHOT rounds=" << successful_rounds
              << " world_steps=" << before.simulation_steps
              << " random_draws=" << before.scene.random.draws() << " bytes=" << payload.size()
              << ' ' << describe_campaign_failure(result) << std::endl;
    if (result.failure && result.failure->id) {
        const sim::rules::CharacterId id{*result.failure->id};
        const auto &actors = before.scene.world.world.ai.battle.actors;
        if (const auto found = actors.find(id); found != actors.end())
            std::cout << "PRE_UPDATE_ACTOR id=" << id.value
                      << " definition=" << found->second.definition
                      << " state=" << found->second.control.state << std::endl;
    }
}

void replay_campaign_failure(const std::filesystem::path &directory, bool expect_fixed) {
    std::istringstream metadata(read(directory / "failure-state.txt"));
    std::string magic, schema, dataset, digest, expected, producer;
    require(bool(std::getline(metadata, magic)) && magic == "ARK_CAMPAIGN_FAILURE_1" &&
                bool(std::getline(metadata, schema)) && schema == codec::codec_schema_identity() &&
                bool(std::getline(metadata, dataset)) &&
                dataset == sim::startup_world_persistence_dataset() &&
                bool(std::getline(metadata, digest)) && bool(std::getline(metadata, expected)) &&
                bool(std::getline(metadata, producer)),
            "snapshot schema/data identity is unsupported");
    const auto payload = read(directory / "failure-state.bin");
    require(assets::sha256_hex(payload) == digest, "snapshot bytes changed");
    const auto initial = initial_world();
    const auto state = codec::decode_state({payload.begin(), payload.end()}, *initial.rules);
    const auto before = sim::startup_world_state_digest(state);
    const auto result = sim::prepare_startup_world_runtime(state);
    if (expect_fixed) {
        require(result.candidate.has_value(),
                "fixed runtime still rejects: " + describe_campaign_failure(result));
        require(sim::startup_world_state_digest(state) == before,
                "fixed update mutated input Owner");
        std::cout << "PASS formerly failing exact Owner/random update now commits; prior="
                  << expected << " producer_library=" << producer
                  << " current_library=" << ARK_LIBRARY_CONTRACT << std::endl;
        return;
    }
    require(!result.candidate && describe_campaign_failure(result) == expected,
            "single update differs: " + describe_campaign_failure(result) + "; expected " +
                expected);
    require(sim::startup_world_state_digest(state) == before,
            "rejected update mutated the input Owner/random");
    std::cout << "PASS exact Owner/random single-update failure replay: " << expected
              << " producer_library=" << producer << " current_library=" << ARK_LIBRARY_CONTRACT
              << std::endl;
    if (result.failure && result.failure->stage == sim::rules::WorldScheduleStage::projectile) {
        // Secondary probe of the public adapter, after the exact production replay above.
        // Forward every request unchanged; inspect the failing transaction, never install it.
        auto adapter = sim::startup_world_runtime_adapter();
        const auto consumer = adapter.nonactors.request;
        const auto identity = result.failure->id;
        adapter.nonactors.request = [consumer, identity](auto &owner, const auto &request) {
            const auto out = consumer(owner, request);
            if (identity && request.identity == *identity) {
                std::cout << "NONACTOR_REQUEST kind=" << int(request.kind)
                          << " identity=" << request.identity
                          << " caster=" << (request.caster ? request.caster->value : 0)
                          << " target=" << (request.target ? request.target->value : 0)
                          << " visual=" << request.visual
                          << " hit=" << (request.hit ? int(request.hit->kind) : -1)
                          << " parameter=" << (request.hit ? request.hit->parameter : -1)
                          << " accepted=" << bool(out);
                if (request.target) {
                    const auto &ai = owner.scene.world.world.ai;
                    std::cout << " target_live=" << ai.battle.actors.count(*request.target)
                              << " target_retired=" << ai.retired_actors.count(*request.target)
                              << " target_context=" << ai.contexts.count(*request.target)
                              << " target_metadata=" << owner.actor_metadata.count(*request.target);
                }
                if (out)
                    std::cout << " facing=" << out->target_facing.value_or(-1)
                              << " effects=" << bool(out->target_effects) << " effects_valid="
                              << (!out->target_effects ||
                                  sim::rules::valid_actor_effect_state(*out->target_effects));
                std::cout << std::endl;
            }
            return out;
        };
        const auto probe =
            sim::rules::prepare_owned_world_runtime(state, {state.calendar_advance, true}, adapter);
        require(!probe.state && probe.failure && probe.failure->stage == result.failure->stage &&
                    probe.failure->id == identity && probe.failure->error == result.failure->error,
                "adapter probe differs from exact production failure; do not attribute its trace");
        require(sim::startup_world_state_digest(state) == before, "adapter probe mutated input");
        std::cout << "PASS unchanged-adapter secondary failure trace" << std::endl;
    }
}

void campaign_diagnostic_contract(const std::filesystem::path &build) {
    const auto directory =
        build / "validation" /
        ("campaign-diagnostic-contract-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory.parent_path());
    require(std::filesystem::create_directory(directory), "fresh contract directory required");
    auto state = initial_world();
    const auto progress_digest = sim::startup_world_state_digest(state);
    std::ostringstream progress;
    inspect_campaign_progress(state, progress);
    require(progress.str().find("RANK_REQUIREMENT target=1 index=0") != std::string::npos &&
                sim::startup_world_state_digest(state) == progress_digest,
            "progress inspection must query real rank terms without mutating Owner/random");
    // Prepare a live record through the real arrival producer and its unchanged initial
    // countdown. These arrival-only calls are a formatting fixture, not natural campaign
    // progression; the original new-world Owner and every downstream assertion stay intact.
    auto activity_before = state;
    const auto arrival = sim::startup_world_runtime_adapter().arrival;
    require(bool(arrival), "activity fixture needs the production arrival consumer");
    const int arrival_calls = activity_before.arrival_counter;
    for (int n = 0; n < arrival_calls && activity_before.scene.world.world.ai.human_order.empty();
         ++n) {
        auto arrived = arrival(activity_before);
        require(arrived.has_value(), "activity fixture source arrival preparation rejected");
        activity_before = std::move(*arrived);
    }
    require(sim::startup_world_state_digest(state) == progress_digest,
            "activity fixture preparation must preserve the original new-world Owner/random");
    auto &world = activity_before.scene.world.world;
    require(!world.ai.human_order.empty(), "activity fixture needs the real initial visitor");
    const auto actor = world.ai.human_order.front();
    world.ai.battle.actors.at(actor).control.state = 8;
    auto movement = activity_before;
    movement.scene.world.world.ai.battle.actors.at(actor).position.x += .25F;
    ++movement.scene.world.world.actors.at(actor).waypoint;
    const auto input_digest = sim::startup_world_state_digest(activity_before);
    const auto movement_digest = sim::startup_world_state_digest(movement);
    std::ostringstream quiet;
    inspect_campaign_activity(activity_before, activity_before, 7, quiet);
    inspect_campaign_activity(activity_before, movement, 7, quiet);
    require(quiet.str().empty() && sim::startup_world_state_digest(activity_before) == input_digest &&
                sim::startup_world_state_digest(movement) == movement_digest,
            "unchanged or movement-only activity inspection stays quiet and preserves both Owners");
    auto changed = movement;
    auto &next_world = changed.scene.world.world;
    next_world.ai.battle.actors.at(actor).control.state = 0;
    const auto shop = std::find_if(next_world.facilities.begin(), next_world.facilities.end(),
                                   [](const auto &entry) { return entry.second.kind == 3; });
    require(shop != next_world.facilities.end(), "activity fixture needs an initial shop");
    shop->second.sales += 11;
    shop->second.occupants.push_back(actor);
    require(next_world.ai.accounting.post_cash(
                {next_world.ai.next_cash_id, next_world.ai.period,
                 sim::rules::CashCategory::facilities, sim::rules::CashDirection::income, 11}) ==
                sim::rules::AccountingError::none,
            "activity fixture records one explicit ledger entry");
    const auto output_digest = sim::startup_world_state_digest(changed);
    std::ostringstream activity;
    inspect_campaign_activity(activity_before, changed, 7, activity);
    require(activity.str().find("ACTOR_ACTIVITY view=actual_update_boundary round=8") !=
                std::string::npos &&
                activity.str().find("phase=before live=1") != std::string::npos &&
                activity.str().find("phase=after live=1") != std::string::npos &&
                activity.str().find("SHOP_ACTIVITY view=actual_update_boundary") != std::string::npos &&
                activity.str().find("ACTIVITY_LEDGER_ADDED view=actual_update_boundary") !=
                    std::string::npos &&
                sim::startup_world_state_digest(activity_before) == input_digest &&
                sim::startup_world_state_digest(changed) == output_digest,
            "actual-boundary format labels both sides, independent shops and ledger without mutation");
    // Billing visibility must not depend on the two boundary actors qualifying as state8/0.
    activity_before.scene.world.world.ai.battle.actors.at(actor).control = {};
    activity_before.scene.world.world.ai.battle.actors.at(actor).control.state = 1;
    changed.scene.world.world.ai.battle.actors.at(actor).control = {};
    changed.scene.world.world.ai.battle.actors.at(actor).control.state = 1;
    std::ostringstream shop_only;
    inspect_campaign_activity(activity_before, changed, 7, shop_only);
    require(shop_only.str().find("ACTOR_ACTIVITY") == std::string::npos &&
                shop_only.str().find("SHOP_ACTIVITY") != std::string::npos &&
                shop_only.str().find("ACTIVITY_LEDGER_ADDED") != std::string::npos,
            "shop occupancy and ledger remain visible outside the actor filter");
    capture_campaign_observation(directory, state);
    require(sim::startup_world_state_digest(read_campaign_observation(directory)) ==
                sim::startup_world_state_digest(state),
            "observation must preserve the full Owner/random");
    state.scripts.pages.clear(); // Deliberately invalid page root, not a reachable business prefix.
    const auto result = sim::prepare_startup_world_runtime(state);
    require(!result.candidate, "invalid page fixture must reject");
    capture_campaign_failure(directory, state, result, 0);
    replay_campaign_failure(directory);
    const auto original = read(directory / "failure-state.bin");
    bool refused{};
    try {
        capture_campaign_failure(directory, state, result, 0);
    } catch (const std::exception &) {
        refused = true;
    }
    require(refused && read(directory / "failure-state.bin") == original,
            "capture must preserve an existing failure snapshot");
    auto corrupt = original;
    corrupt.at(0) ^= 1;
    std::ofstream damaged(directory / "failure-state.bin", std::ios::binary);
    damaged.write(corrupt.data(), static_cast<std::streamsize>(corrupt.size()));
    damaged.close();
    refused = false;
    try {
        replay_campaign_failure(directory);
    } catch (const std::exception &) {
        refused = true;
    }
    require(refused, "altered snapshot must reject before replay");
    std::filesystem::remove_all(directory); // Only this exclusively created fixture directory.
}

// Observation snapshots retain experimental endpoints even if a business assertion fails.
// They contain no player-strategy certificate and are never installed into player slots.
void capture_campaign_observation(const std::filesystem::path &directory,
                                  const app::WorldState &state) {
    const auto encoded = codec::encode_state(state);
    const std::string payload(encoded.begin(), encoded.end());
    std::ostringstream metadata;
    metadata << "ARK_CAMPAIGN_OBSERVATION_1\n"
             << codec::codec_schema_identity() << '\n'
             << sim::startup_world_persistence_dataset() << '\n'
             << assets::sha256_hex(payload) << '\n'
             << ARK_LIBRARY_CONTRACT << '\n';
    write_new(directory / "observation.bin", payload);
    write_new(directory / "observation.txt", metadata.str());
}
app::WorldState read_campaign_observation(const std::filesystem::path &directory) {
    std::istringstream metadata(read(directory / "observation.txt"));
    std::string magic, schema, dataset, digest;
    require(bool(std::getline(metadata, magic)) && magic == "ARK_CAMPAIGN_OBSERVATION_1" &&
                bool(std::getline(metadata, schema)) && schema == codec::codec_schema_identity() &&
                bool(std::getline(metadata, dataset)) &&
                dataset == sim::startup_world_persistence_dataset() &&
                bool(std::getline(metadata, digest)),
            "unsupported observation identity");
    const auto payload = read(directory / "observation.bin");
    require(assets::sha256_hex(payload) == digest, "observation bytes changed");
    const auto initial = initial_world();
    return codec::decode_state({payload.begin(), payload.end()}, *initial.rules);
}
} // namespace ark::test
