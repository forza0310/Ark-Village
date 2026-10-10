// Active player-file acceptance, distinct from frozen domain trajectories and FIFO tests.
// Serial real commands precede one real runtime update. Only wall-clock waits are removed.
#include "ark/app/world_report.hpp"
#include "ark/assets/sha256.hpp"
#include "support/world_fixture.hpp"
#include "world_active_late_strategy.hpp"
#include "world_active_pot_strategy.hpp"
#include "world_active_strategy.hpp"
#include "world_commands.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
namespace app = ark::app;
namespace sim = ark::simulation;
namespace ref = sim::rules;
using State = app::WorldState;
using Strategy = ark::test::ActiveVillageStrategy;
using LateStrategy = ark::test::ActiveLateVillageStrategy;
using PotStrategy = ark::test::ActivePotVillageStrategy;
using Kind = app::WorldCommandKind;
void require(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}
void good(const std::string &error) { require(error.empty(), error); }
const ref::WorldScriptPage *top(const State &state) {
    for (auto page = state.scripts.pages.rbegin(); page != state.scripts.pages.rend(); ++page)
        if (page->lifecycle != 4)
            return &*page;
    return nullptr;
}
int month(const State &state) {
    return state.scene.calendar.year * 12 + state.scene.calendar.month;
}
std::string bytes(const std::filesystem::path &file) {
    std::ifstream input(file, std::ios::binary);
    require(bool(input), "Cannot read " + file.string());
    return {std::istreambuf_iterator<char>(input), {}};
}

// WorldSession tests own transport. Refuse its menu/save/camera/speed metadata instead of
// bypassing those gates; decisions use the same private-candidate consumer as the worker.
app::WorldCommandResult apply(State &candidate, const app::WorldCommand &command) {
    app::WorldCommandResult result;
    result.kind = command.kind;
    result.page = command.page;
    switch (command.kind) {
    case Kind::acknowledge_page:
        require(!app::detail::is_decision_page(top(candidate)) &&
                    !candidate.task_abort_questions.count(command.page),
                "Decision page requires explicit action, never generic acknowledgement");
        result.runtime_error = sim::acknowledge_startup_world_runtime_page(candidate, command.page);
        break;
    case Kind::acknowledge_report:
        require(app::acknowledge_world_report(candidate, command.report_phase),
                "Report acknowledgement rejected");
        break;
    case Kind::open_build_menu:
    case Kind::select_build_menu:
    case Kind::cancel_build_menu:
    case Kind::confirm_build:
    case Kind::cancel_build:
    case Kind::confirm_edit:
    case Kind::cancel_edit:
    case Kind::open_facility:
    case Kind::facility_action:
    case Kind::open_human:
    case Kind::human_action:
    case Kind::tax_action:
    case Kind::open_village_activities:
    case Kind::village_activity_action:
    case Kind::open_commerce:
    case Kind::commerce_action:
    case Kind::open_magic_pot:
    case Kind::magic_pot_action:
    case Kind::facility_item_action:
    case Kind::facility_catalog_action:
    case Kind::residence_action:
    case Kind::open_task_control_menu:
    case Kind::open_task_menu:
    case Kind::task_action:
    case Kind::cancel_page:
    case Kind::award_action:
    case Kind::rank_action:
        app::detail::apply_world_decision(candidate, command, result);
        break;
    default:
        throw std::runtime_error("Unsupported campaign command " +
                                 std::to_string(int(command.kind)));
    }
    result.outcome = result.runtime_error == sim::StartupWorldRuntimeError::none &&
                             result.denial == ref::TaskCommandDenial::none &&
                             result.build_denial == sim::StartupBuildDenial::none
                         ? app::WorldCommandOutcome::applied
                         : app::WorldCommandOutcome::rejected;
    require(result.runtime_error == sim::StartupWorldRuntimeError::none ||
                result.runtime_error == sim::StartupWorldRuntimeError::invalid_page,
            "Fatal runtime decision error=" + std::to_string(int(result.runtime_error)));
    return result;
}

template <class PlayerStrategy> struct Route;
template <> struct Route<Strategy> {
    static constexpr const char *magic = "ARK_ACTIVE_CHECKPOINT_1";
    static constexpr int minutes = 100;
    static std::uint64_t budget(bool resume) { return resume ? 250000 : 25000; }
    static bool checkpoint(const Strategy &strategy, const State &state) {
        return strategy.construction_checkpoint(state);
    }
};
template <> struct Route<LateStrategy> {
    static constexpr const char *magic = "ARK_ACTIVE_LATE_CHECKPOINT_1";
    static constexpr int minutes = 100;
    static std::uint64_t budget(bool) { return 360000; }
    static bool checkpoint(const LateStrategy &strategy, const State &state) {
        return strategy.checkpoint(state);
    }
};
template <> struct Route<PotStrategy> {
    static constexpr const char *magic = "ARK_ACTIVE_POT_CHECKPOINT_1";
    static constexpr int minutes = 100;
    static std::uint64_t budget(bool) { return 360000; }
    static bool checkpoint(const PotStrategy &strategy, const State &state) {
        return strategy.checkpoint(state);
    }
};

// All routes share the exact production transaction/update/player-save driver. Only
// player decisions, evidence serialization and business milestones vary by route.
template <class PlayerStrategy> struct CampaignRun {
    State state;
    app::WorldSystemState system;
    std::filesystem::path directory;
    PlayerStrategy strategy;
    std::uint64_t rounds{};

    void command(const app::WorldCommand &input) {
        auto candidate = state;
        const auto result = apply(candidate, input);
        if (result.runtime_error == sim::StartupWorldRuntimeError::none) {
            auto next = system;
            good(app::commit_world_system(directory, state, candidate, next));
            candidate.sound_requests.clear();
            strategy.observe(state, input, result, candidate);
            state = std::move(candidate);
            system = std::move(next);
        } else {
            strategy.observe(state, input, result, state); // Rejected candidate is not installed.
        }
        std::cout << "command=" << int(input.kind) << " page=" << input.page
                  << " definition=" << input.definition << " selection=" << input.selection
                  << " outcome=" << int(result.outcome) << " error=" << int(result.runtime_error)
                  << " task_denial=" << int(result.denial)
                  << " build_denial=" << int(result.build_denial) << std::endl;
    }
    void step() {
        const int before_month = month(state);
        const auto before_random = state.scene.random.draws();
        require(!app::world_clear_page(state),
                "Active acceptance cannot substitute date-clear for business goals");
        if (const auto input = strategy.next(state))
            command(*input);
        auto update = sim::prepare_startup_world_runtime(state);
        require(update.candidate.has_value(),
                "Runtime update rejected: runtime=" + std::to_string(int(update.error)) +
                    " world=" + std::to_string(int(update.world_error)));
        require(sim::update_startup_world_render_cache(*update.candidate), "Render cache rejected");
        auto next = system;
        good(app::commit_world_system(directory, state, *update.candidate, next));
        update.candidate->sound_requests.clear();
        strategy.observe_tick(state, *update.candidate);
        state = std::move(*update.candidate);
        system = std::move(next);
        ++rounds;
        require(state.scene.random.draws() >= before_random, "Random rewound within process");
        require(month(state) >= before_month && month(state) <= before_month + 1 &&
                    ref::valid_world_calendar_state(state.scene.calendar),
                "Natural calendar discontinuity");
        require(state.scene.speed_setting == 0 && !state.scene.framework_paused,
                "Active route must keep the normal single-speed runtime");
        if (month(state) != before_month)
            std::cout << "MONTH " << strategy.diagnose(state) << std::endl;
    }
    void run(bool resume) {
        const auto start = std::chrono::steady_clock::now();
        auto report = start;
        const std::uint64_t budget = Route<PlayerStrategy>::budget(resume);
        for (;;) {
            const bool achieved = resume ? strategy.complete(state)
                                         : Route<PlayerStrategy>::checkpoint(strategy, state);
            if (achieved && app::world_save_eligible(state))
                break;
            require(rounds < budget, "Active business step budget exhausted");
            step();
            const auto now = std::chrono::steady_clock::now();
            require(now - start < std::chrono::minutes(Route<PlayerStrategy>::minutes),
                    "Active business time budget exhausted");
            if (now - report >= std::chrono::seconds(30)) {
                std::cout << "PROGRESS " << strategy.diagnose(state) << std::endl;
                report = now;
            }
        }
        std::cout << "MILESTONE " << strategy.diagnose(state) << std::endl;
    }
    void save(int slot) {
        require(!std::filesystem::exists(app::world_save_slot_path(directory, slot)),
                "Campaign never overwrites an existing player slot");
        const auto captured = app::capture_world_save(state);
        require(captured.image.has_value(), captured.message);
        const auto saved = app::write_world_save_slot(directory, slot, *captured.image);
        require(saved.error == app::WorldSaveError::none, saved.message);
        const auto sidecar = directory / ("strategy" + std::to_string(slot) + ".txt");
        require(!std::filesystem::exists(sidecar), "Existing strategy evidence must be preserved");
        std::ofstream output(sidecar, std::ios::binary);
        output << Route<PlayerStrategy>::magic << '\n'
               << ark::assets::sha256_hex(bytes(app::world_save_slot_path(directory, slot)))
               << '\n';
        strategy.encode(output);
        output.close();
        require(bool(output), "Could not write strategy evidence");
        std::cout << "SAVE slot=" << slot << " bytes=" << captured.image->bytes.size() << " "
                  << strategy.diagnose(state) << std::endl;
    }
};
using Campaign = CampaignRun<Strategy>;
using LateCampaign = CampaignRun<LateStrategy>;
using PotCampaign = CampaignRun<PotStrategy>;

template <class PlayerStrategy = Strategy>
CampaignRun<PlayerStrategy> load(const std::filesystem::path &directory, int slot = 0) {
    const auto old_slot = bytes(app::world_save_slot_path(directory, slot));
    std::ifstream metadata(directory / ("strategy" + std::to_string(slot) + ".txt"),
                           std::ios::binary);
    std::string magic, digest;
    require(bool(std::getline(metadata, magic)) && magic == Route<PlayerStrategy>::magic &&
                bool(std::getline(metadata, digest)) && digest == ark::assets::sha256_hex(old_slot),
            "Strategy sidecar does not match this exact player file");
    auto strategy = PlayerStrategy::decode(metadata);
    auto fresh = ark::test::initial_world(20261009);
    const auto records = app::read_world_system(directory);
    require(records.records.has_value(), records.error);
    auto saved = app::read_world_save_slot(directory, slot);
    require(saved.state.has_value(), saved.message);
    std::string reason;
    require(app::prepare_world_save_candidate(*saved.state, fresh, reason) ==
                app::WorldSaveError::none,
            reason);
    // Check all persisted bytes. Audit logs, pages and random use the player's policy.
    const auto recaptured = app::capture_world_save(*saved.state);
    require(recaptured.image && std::string(recaptured.image->bytes.begin(),
                                            recaptured.image->bytes.end()) == old_slot,
            "Cold player restore changed persisted business state");
    saved.state->cash_peak = records.records->cash_peak;
    saved.state->cash_peak_village = records.records->cash_village;
    auto expected_random = fresh.scene.random;
    auto restored_random = saved.state->scene.random;
    require(saved.state->scene.random.draws() == fresh.scene.random.draws() &&
                restored_random.draw(197).ticket == expected_random.draw(197).ticket,
            "Cold player load must use the new process stream");
    require(sim::startup_world_human_profile(*saved.state, 0)->name == "经营验收主角",
            "Main profile survives the business checkpoint");
    strategy.reconcile(*saved.state);
    require(Route<PlayerStrategy>::checkpoint(strategy, *saved.state),
            "Loaded active business milestone lost");
    return {std::move(*saved.state), {*records.records, {}}, directory, std::move(strategy)};
}

std::int64_t facility_sales(const State &state) {
    std::int64_t total{};
    for (const auto &[id, facility] : state.scene.world.world.facilities) {
        (void)id;
        total += facility.sales;
    }
    return total;
}

// Scalar observations of the first checkpoint, not a retained second world or ledger.
struct BusinessCheckpoint {
    std::uint64_t world_steps{}, observed_ticks{}, commands{};
    std::int64_t sales{};
};
template <class PlayerStrategy>
BusinessCheckpoint checkpoint(const CampaignRun<PlayerStrategy> &campaign) {
    return {campaign.state.simulation_steps, campaign.strategy.stats().ticks,
            campaign.strategy.stats().commands, facility_sales(campaign.state)};
}

// Historical command receipts alone cannot certify a live playable end state. This
// check also runs on cold-loaded final files; it neither sends input nor writes files.
void verify_endpoint(const Campaign &campaign, const BusinessCheckpoint &before) {
    const auto &state = campaign.state;
    const auto &stats = campaign.strategy.stats();
    require(campaign.strategy.complete(state) && app::world_save_eligible(state),
            "Final player world does not satisfy the complete stable business milestone");
    const auto exhibition = state.activity_counts.find(16);
    require(state.rank >= 1 && exhibition != state.activity_counts.end() && exhibition->second > 0,
            "Current Owner lost its first star or actual exhibition consumption");
    require(stats.task_successes > 0 && state.task_progress.successes >= stats.task_successes,
            "Current Owner task successes do not support observed victories");
    require(!stats.upgraded_definitions.empty(), "No observed actual facility upgrade");
    for (const auto definition : stats.upgraded_definitions) {
        const auto progress = state.scene.world.world.facility_uses.find(definition);
        require(progress != state.scene.world.world.facility_uses.end() &&
                    progress->second.level > 1,
                "Current Owner lost upgraded facility definition=" + std::to_string(definition));
    }
    require(state.simulation_steps > before.world_steps && stats.ticks > before.observed_ticks &&
                stats.commands > before.commands,
            "Resumed process must perform actual world rounds and business commands");
    const auto current_sales = facility_sales(state);
    require(current_sales > before.sales,
            "Persisted cumulative facility sales must grow after the player restart");
    std::cout << "ENDPOINT world_rounds=" << before.world_steps << "->" << state.simulation_steps
              << " observed_ticks=" << before.observed_ticks << "->" << stats.ticks
              << " cumulative_facility_sales=" << before.sales << "->" << current_sales
              << " rank=" << state.rank << " exhibition_count=" << exhibition->second
              << " task_successes=" << state.task_progress.successes
              << " upgraded_definitions=" << stats.upgraded_definitions.size() << std::endl;
}

// Re-open both player slots and check live Owner relationships, rather than accepting
// the sidecar's historical counters as a substitute for a still-operational village.
void verify_late_endpoint(const LateCampaign &campaign, const BusinessCheckpoint &before) {
    const auto &state = campaign.state;
    const auto &stats = campaign.strategy.stats();
    require(campaign.strategy.complete(state) && app::world_save_eligible(state),
            "Second-star endpoint is not a complete stable business milestone");
    int houses{}, shops{};
    bool new_shop_traded{};
    for (const auto &[id, facility] : state.scene.world.world.facilities) {
        if (facility.status != 1)
            continue;
        if (facility.kind == 12)
            ++houses;
        if (facility.kind == 3 || facility.kind == 9) {
            ++shops;
            if (stats.buildings.count(id) && facility.sales > 0)
                new_shop_traded = true;
        }
    }
    const auto activity = state.activity_counts.find(30);
    require(
        state.rank == 2 && houses >= 4 && shops >= 10 && state.task_progress.successes >= 12 &&
            activity != state.activity_counts.end() && activity->second > 0,
        "Current Owner lost second star, completed houses/shops, task victories or activity 30");
    // Popularity is checked at the real rank application by the strategy; it may fluctuate
    // afterwards. The final Owner still has to retain the physical and unlock outcomes.
    require(!stats.residents.empty() && stats.admissions > 0 && new_shop_traded &&
                stats.new_shop_income > 0 && stats.task_successes > 0 &&
                state.task_progress.successes >= stats.initial_successes + stats.task_successes,
            "Current Owner cannot support the observed new residence, trade and victories");
    for (const auto human : stats.residents) {
        const auto home = state.human_homes.find(human);
        require(home != state.human_homes.end() && home->second[2] != 0,
                "Admitted human no longer has a real home");
        bool housed{};
        for (const auto &[id, resident] : state.facility_residents) {
            const auto facility = state.scene.world.world.facilities.find(id);
            if (resident == human && facility != state.scene.world.world.facilities.end() &&
                facility->second.kind == 12 && facility->second.status == 1)
                housed = true;
        }
        require(housed, "Admitted human has no matching finished residential facility");
    }
    require(state.simulation_steps > before.world_steps && stats.ticks > before.observed_ticks &&
                stats.commands > before.commands && facility_sales(state) > before.sales,
            "Second-star restart did not perform actual commands, updates and continued trade");
    std::cout << "LATE_ENDPOINT world_rounds=" << before.world_steps << "->"
              << state.simulation_steps << " cumulative_facility_sales=" << before.sales << "->"
              << facility_sales(state) << " rank=" << state.rank << " houses=" << houses
              << " shops=" << shops << " successes=" << state.task_progress.successes
              << " activity30=" << activity->second << " admissions=" << stats.admissions
              << " new_shop_income=" << stats.new_shop_income
              << " commerce_open=" << ((state.scripts.user_flags & 16U) != 0) << std::endl;
}

// Receipts certify the transient element charge and healing, while cold Owner checks
// certify the discovered recipe, unlocked pot and continued operation still persist.
void verify_pot_endpoint(const PotCampaign &campaign, const BusinessCheckpoint &before) {
    const auto &state = campaign.state;
    const auto &stats = campaign.strategy.stats();
    require(campaign.strategy.complete(state) && app::world_save_eligible(state),
            "Magic-pot endpoint is not a complete stable business milestone");
    const auto recipe = state.magic_pot_recipes.find(stats.recipe);
    const auto item = state.items.find(stats.item);
    const auto activity = state.activity_counts.find(30);
    require(state.rank >= 2 && (state.scripts.user_flags & 1U) != 0 &&
                activity != state.activity_counts.end() && activity->second > 0 &&
                recipe != state.magic_pot_recipes.end() && recipe->second.status == 1 &&
                item != state.items.end(),
            "Current Owner lost the unlocked pot, discovered recipe or resulting item");
    require(stats.deposits > 0 && stats.processed > 0 && stats.discoveries > 0 &&
                stats.crafted > 0 && stats.used > 0 && stats.healed > 0 &&
                stats.hp_after > stats.hp_before && stats.recipient >= 0 &&
                sim::startup_world_human_profile(state, stats.recipient).has_value(),
            "Pot endpoint lacks observed deposit, processing, discovery, craft or actual healing");
    require(state.simulation_steps > before.world_steps && stats.ticks > before.observed_ticks &&
                stats.commands > before.commands && facility_sales(state) > before.sales,
            "Pot restart did not perform actual commands, updates and continued trade");
    std::cout << "POT_ENDPOINT world_rounds=" << before.world_steps << "->"
              << state.simulation_steps << " cumulative_facility_sales=" << before.sales << "->"
              << facility_sales(state) << " recipe=" << stats.recipe << " item=" << stats.item
              << " deposits=" << stats.deposits << " processed=" << stats.processed
              << " discoveries=" << stats.discoveries << " crafted=" << stats.crafted
              << " used=" << stats.used << " healed=" << stats.healed << " hp=" << stats.hp_before
              << "->" << stats.hp_after << std::endl;
}

void verify_third_endpoint(const LateCampaign &campaign, const BusinessCheckpoint &before) {
    const auto &s = campaign.state;
    const auto &v = campaign.strategy.stats();
    require(v.target_rank == 3 && campaign.strategy.complete(s) && app::world_save_eligible(s),
            "Third-star endpoint lacks completed conditions, school business or full-month trade");
    const auto &western = s.scene.world.world.facilities.at(v.western);
    const auto &school = s.scene.world.world.facilities.at(v.school);
    require(western.placement.definition_id == 40 && western.status == 1 &&
                western.sales > v.western_initial_sales && school.placement.definition_id == 63 &&
                school.status == 1 && school.sales == v.school_sales && school.sales > 0 &&
                s.maximum_income >= 35000 && s.events_held >= 15 && s.activity_counts.at(7) > 0 &&
                v.school_activity_paid &&
                s.task_progress.successes >= v.initial_successes + v.task_successes &&
                v.task_successes > 0,
            "Cold third-star Owner lost real restaurant, school, activity or task outcomes");
    require(s.simulation_steps > before.world_steps && v.ticks > before.observed_ticks &&
                v.commands > before.commands && facility_sales(s) > before.sales,
            "Third-star restart did not perform real commands, updates and trade");
    std::cout << "THIRD_ENDPOINT world_rounds=" << before.world_steps << "->" << s.simulation_steps
              << " cumulative_facility_sales=" << before.sales << "->" << facility_sales(s)
              << " rank=" << s.rank << " income_record=" << s.maximum_income
              << " events=" << s.events_held << " school=" << v.school
              << " school_sales=" << school.sales << " activity7=" << s.activity_counts.at(7)
              << " post_activity_full_month=1" << std::endl;
}

std::filesystem::path isolated_directory(const char *executable, const char *supplied,
                                         const char *marker = "ark-active-first-star-v1\n") {
    const auto build = std::filesystem::canonical(executable).parent_path().parent_path();
    require(build.filename() == "build",
            "Campaign binary must be in the product build/bin directory");
    const auto directory = std::filesystem::canonical(supplied);
    const auto relative = directory.lexically_relative(build);
    require(!relative.empty() && relative != "." && !relative.is_absolute() &&
                *relative.begin() != ".." && std::filesystem::is_directory(directory),
            "Campaign files must stay strictly below the real product build directory");
    require(bytes(directory / "ACTIVE_CAMPAIGN") == marker,
            "Use the isolated process runner to create a fresh campaign directory");
    return directory;
}

void contract() {
    auto state = ark::test::initial_world();
    app::WorldCommand command;
    command.kind = Kind::open_build_menu;
    require(apply(state, command).outcome == app::WorldCommandOutcome::applied,
            "Real construction menu should open");
    const auto *page = top(state);
    require(page != nullptr, "Real menu page missing");
    command.page = page->id;
    command.kind = Kind::acknowledge_page;
    bool refused{};
    try {
        apply(state, command);
    } catch (const std::runtime_error &) {
        refused = true;
    }
    require(refused, "Driver cannot generic-ack a real decision menu");
    command.kind = Kind::cancel_build_menu;
    require(apply(state, command).outcome == app::WorldCommandOutcome::applied,
            "Real construction menu return should succeed");
    Strategy strategy;
    strategy.reconcile(state);
    std::ostringstream encoded;
    strategy.encode(encoded);
    std::istringstream input(encoded.str());
    auto restored = Strategy::decode(input);
    restored.reconcile(state);
    std::ostringstream encoded_again;
    restored.encode(encoded_again);
    require(encoded.str() == encoded_again.str(), "Strategy evidence roundtrip differs");
    require(!restored.complete(state) && !restored.construction_checkpoint(state),
            "A fresh village cannot pass business milestones");
    std::istringstream corrupt("not-a-strategy\n");
    refused = false;
    try {
        Strategy::decode(corrupt);
    } catch (const std::exception &) {
        refused = true;
    }
    require(refused, "Malformed strategy evidence must be rejected");
    LateStrategy late;
    std::ostringstream late_encoded;
    late.encode(late_encoded);
    std::istringstream late_input(late_encoded.str());
    const auto late_restored = LateStrategy::decode(late_input);
    std::ostringstream late_again;
    late_restored.encode(late_again);
    require(late_encoded.str() == late_again.str() && !late_restored.complete(state) &&
                !late_restored.checkpoint(state),
            "Fresh late strategy must roundtrip without claiming business milestones");
    refused = false;
    try {
        late.reconcile(state);
    } catch (const std::exception &) {
        refused = true;
    }
    require(refused, "Late route must refuse an initial unranked village");
    refused = false;
    std::istringstream wrong_route(encoded.str());
    try {
        LateStrategy::decode(wrong_route);
    } catch (const std::exception &) {
        refused = true;
    }
    require(refused, "Late strategy must refuse first-star strategy evidence");
    PotStrategy pot;
    std::ostringstream pot_encoded;
    pot.encode(pot_encoded);
    std::istringstream pot_input(pot_encoded.str());
    const auto pot_restored = PotStrategy::decode(pot_input);
    std::ostringstream pot_again;
    pot_restored.encode(pot_again);
    require(pot_encoded.str() == pot_again.str() && !pot_restored.complete(state) &&
                !pot_restored.checkpoint(state),
            "Fresh pot strategy must roundtrip without claiming business milestones");
    refused = false;
    try {
        pot.reconcile(state);
    } catch (const std::exception &) {
        refused = true;
    }
    require(refused, "Pot route must refuse an initial unranked village");
    refused = false;
    std::istringstream wrong_pot_route(late_encoded.str());
    try {
        PotStrategy::decode(wrong_pot_route);
    } catch (const std::exception &) {
        refused = true;
    }
    require(refused, "Pot strategy must refuse second-star strategy evidence");
    LateStrategy third(3);
    std::ostringstream third_encoded;
    third.encode(third_encoded);
    std::istringstream third_input(third_encoded.str());
    const auto third_restored = LateStrategy::decode(third_input);
    require(third_restored.stats().target_rank == 3 && !third_restored.complete(state) &&
                !third_restored.checkpoint(state),
            "Third-star evidence lost target or invented progress");
    refused = false;
    try {
        third.reconcile(state);
    } catch (const std::exception &) {
        refused = true;
    }
    require(refused, "Third-star route must refuse an unranked world");
    // A bounded decision fixture, not a natural second-star certificate. Keep 119
    // points for the real 120-point offer, then exercise actual 83/85/93 consumers.
    auto restaurant_state = ark::test::initial_world();
    restaurant_state.rank = 2;
    restaurant_state.village_points = 119;
    restaurant_state.quarter_counter = 1;
    // Explicit chamber-entry eligibility for this isolated decision fixture. The
    // real route must cold-load this flag from the independently verified prefix.
    restaurant_state.scripts.user_flags |= 16U;
    require(restaurant_state.rules->facility_initial.at(40).capacity == 120 &&
                restaurant_state.facility_presence.at(40) == 0,
            "Restaurant fixture differs from the published 120-point locked offer");
    LateStrategy restaurant_budget(3);
    restaurant_budget.reconcile(restaurant_state);
    const auto reserved = restaurant_budget.next(restaurant_state);
    require(!reserved || (reserved->kind != Kind::open_village_activities &&
                          reserved->kind != Kind::open_commerce),
            "Restaurant budget must not spend its reserved points or open an unaffordable offer");
    restaurant_state.village_points = 120;
    LateStrategy restaurant(3);
    restaurant.reconcile(restaurant_state);
    for (int step = 0; step < 300 && !restaurant.stats().western_unlock_claimed; ++step) {
        if (const auto action = restaurant.next(restaurant_state)) {
            auto candidate = restaurant_state;
            const auto result = apply(candidate, *action);
            restaurant.observe(restaurant_state, *action, result, candidate);
            restaurant_state = std::move(candidate);
        }
        if (restaurant.stats().western_unlock_claimed)
            break;
        auto candidate = sim::prepare_startup_world_runtime(restaurant_state);
        require(candidate.candidate.has_value(), "Restaurant quick fixture update failed");
        require(sim::update_startup_world_render_cache(*candidate.candidate),
                "Restaurant quick fixture render cache failed");
        restaurant.observe_tick(restaurant_state, *candidate.candidate);
        restaurant_state = std::move(*candidate.candidate);
    }
    require(restaurant.stats().western_unlock_paid && restaurant.stats().western_unlock_claimed &&
                restaurant.stats().western_unlock_points == 120 &&
                restaurant_state.village_points == 0 &&
                restaurant_state.facility_presence.at(40) == 2,
            "Actual restaurant chamber purchase/claim did not complete exactly once");
    const auto old_late_end = late_encoded.str().find("WESTERN_UNLOCK_1");
    require(old_late_end != std::string::npos, "Current late evidence lacks receipt extension");
    std::istringstream verified_second_prefix(late_encoded.str().substr(0, old_late_end));
    require(LateStrategy::decode(verified_second_prefix).stats().target_rank == 2,
            "Verified target-two evidence must remain readable without a third-star extension");
    refused = false;
    std::istringstream unextended_third(
        third_encoded.str().substr(0, third_encoded.str().find("WESTERN_UNLOCK_1")));
    try {
        LateStrategy::decode(unextended_third);
    } catch (const std::exception &) {
        refused = true;
    }
    require(refused, "Old target-three evidence cannot claim the new restaurant receipt contract");
    ark::test::ActiveTradeEvidence trade;
    trade.revenue.emplace(9, 100);
    require(!trade.full_month_after(9, 11), "Milestone-month sales cannot certify the next month");
    trade.revenue.emplace(10, 100);
    require(!trade.full_month_after(9, 10) && trade.full_month_after(9, 11),
            "Only a completed following month can certify continued trade");
    std::cout << "PASS active campaign driver contract" << std::endl;
}
} // namespace

int main(int argc, char **argv) {
    std::optional<Campaign> campaign;
    std::optional<LateCampaign> late_campaign;
    std::optional<PotCampaign> pot_campaign;
    try {
        if (argc == 2 && std::string(argv[1]) == "--contract") {
            contract();
            return 0;
        }
        require(argc == 3, "Expected campaign phase and runner-created isolated directory");
        const std::string mode = argv[1];
        if (mode == "inspect-third-input" || mode == "check-third-layout") {
            const auto directory =
                isolated_directory(argv[0], argv[2], "ark-active-second-star-v1\n");
            const auto before = checkpoint(load<LateStrategy>(directory, 0));
            auto prefix = load<LateStrategy>(directory, 1);
            verify_late_endpoint(prefix, before);
            std::cout << prefix.strategy.diagnose_construction(prefix.state) << std::endl;
            if (mode == "check-third-layout") {
                const auto old_slot = bytes(app::world_save_slot_path(directory, 1));
                LateStrategy strategy(3);
                strategy.reconcile(prefix.state);
                for (int n = 0; n < 1000 && !strategy.stats().layout_complete; ++n) {
                    if (const auto action = strategy.next(prefix.state)) {
                        auto candidate = prefix.state;
                        const auto result = apply(candidate, *action);
                        strategy.observe(prefix.state, *action, result, candidate);
                        prefix.state = std::move(candidate);
                    }
                    if (strategy.stats().layout_complete)
                        break;
                    auto next = sim::prepare_startup_world_runtime(prefix.state);
                    require(next.candidate.has_value(), "Real-prefix road smoke update rejected");
                    require(sim::update_startup_world_render_cache(*next.candidate),
                            "Real-prefix road smoke render cache rejected");
                    strategy.observe_tick(prefix.state, *next.candidate);
                    prefix.state = std::move(*next.candidate);
                }
                require(
                    strategy.stats().layout_complete && strategy.stats().layout_road_cells == 6 &&
                        strategy.stats().layout_cost == 360 &&
                        bytes(app::world_save_slot_path(directory, 1)) == old_slot,
                    "Actual road strategy failed its bounded real-prefix smoke or modified input");
                std::cout << "LAYOUT_SMOKE " << strategy.diagnose(prefix.state) << std::endl;
                std::cout << strategy.diagnose_construction(prefix.state) << std::endl;
                std::cout
                    << "PASS actual command road-branch smoke in temporary Owner; no files saved"
                    << std::endl;
                return 0;
            }
            std::cout << "PASS read-only third-star construction input inspection; no updates or "
                         "mutations"
                      << std::endl;
            return 0;
        }
        const bool pot = mode == "pot-new" || mode == "pot-resume" || mode == "verify-pot";
        if (pot) {
            const auto directory =
                isolated_directory(argv[0], argv[2], "ark-active-magic-pot-v1\n");
            if (mode == "pot-new") {
                require(!std::filesystem::exists(app::world_save_slot_path(directory, 0)) &&
                            !std::filesystem::exists(app::world_save_slot_path(directory, 1)),
                        "New pot route cannot overwrite previous player slots");
                const auto input = std::filesystem::canonical(directory / "input");
                require(input.parent_path() == directory && input.filename() == "input",
                        "Second-star input directory must be an isolated local copy");
                const auto before = checkpoint(load<LateStrategy>(input, 0));
                auto prefix = load<LateStrategy>(input, 1);
                verify_late_endpoint(prefix, before);
                require(bytes(app::world_system_path(directory)) ==
                            bytes(app::world_system_path(input)),
                        "Pot route system record differs from the verified prefix");
                pot_campaign.emplace(
                    PotCampaign{std::move(prefix.state), std::move(prefix.system), directory, {}});
                pot_campaign->strategy.reconcile(pot_campaign->state);
                pot_campaign->run(false);
                require(pot_campaign->strategy.stats().crafted == 0 &&
                            pot_campaign->strategy.stats().used == 0,
                        "Pot business checkpoint must precede crafting and item use");
                pot_campaign->save(0);
            } else {
                const auto before = [&] {
                    const auto saved = load<PotStrategy>(directory, 0);
                    require(saved.strategy.stats().crafted == 0 && saved.strategy.stats().used == 0,
                            "Cold pot checkpoint must precede crafting and item use");
                    return checkpoint(saved);
                }();
                if (mode == "verify-pot") {
                    pot_campaign.emplace(load<PotStrategy>(directory, 1));
                    verify_pot_endpoint(*pot_campaign, before);
                    std::cout << "PASS read-only magic-pot cold-file endpoint verification"
                              << std::endl;
                } else {
                    pot_campaign.emplace(load<PotStrategy>(directory, 0));
                    const auto first_slot = bytes(app::world_save_slot_path(directory, 0));
                    const auto first_strategy = bytes(directory / "strategy0.txt");
                    pot_campaign->run(true);
                    verify_pot_endpoint(*pot_campaign, before);
                    pot_campaign->save(1);
                    require(bytes(app::world_save_slot_path(directory, 0)) == first_slot &&
                                bytes(directory / "strategy0.txt") == first_strategy,
                            "Pot restart must preserve its first player checkpoint and evidence");
                    std::cout << "PASS active magic-pot campaign: deposit, date processing, "
                                 "discovery, cold restart, paid craft, item consumption, healing, "
                                 "continued trade"
                              << std::endl;
                }
            }
            return 0;
        }
        const bool third = mode == "third-new" || mode == "third-resume" || mode == "verify-third";
        const bool late =
            third || mode == "late-new" || mode == "late-resume" || mode == "verify-late";
        if (late) {
            const auto directory = isolated_directory(argv[0], argv[2],
                                                      third ? "ark-active-third-star-v1\n"
                                                            : "ark-active-second-star-v1\n");
            if (mode == "late-new" || mode == "third-new") {
                require(!std::filesystem::exists(app::world_save_slot_path(directory, 0)) &&
                            !std::filesystem::exists(app::world_save_slot_path(directory, 1)),
                        "New late route cannot overwrite previous player slots");
                const auto input = std::filesystem::canonical(directory / "input");
                require(input.parent_path() == directory && input.filename() == "input",
                        "First-star input directory must be an isolated local copy");
                // The first Owner is discarded after scalar observations. Cold-load the exact
                // verified endpoint, then move its sole Owner into a fresh late strategy.
                require(bytes(app::world_system_path(directory)) ==
                            bytes(app::world_system_path(input)),
                        "Late route system record differs from the verified prefix");
                if (third) {
                    const auto before = checkpoint(load<LateStrategy>(input, 0));
                    auto prefix = load<LateStrategy>(input, 1);
                    require(prefix.strategy.stats().target_rank == 2,
                            "Third-star prefix is not the second-star route");
                    verify_late_endpoint(prefix, before);
                    late_campaign.emplace(LateCampaign{std::move(prefix.state),
                                                       std::move(prefix.system), directory,
                                                       LateStrategy{3}});
                } else {
                    const auto before = checkpoint(load(input, 0));
                    auto prefix = load(input, 1);
                    verify_endpoint(prefix, before);
                    late_campaign.emplace(LateCampaign{std::move(prefix.state),
                                                       std::move(prefix.system), directory,
                                                       LateStrategy{}});
                }
                late_campaign->strategy.reconcile(late_campaign->state);
                late_campaign->run(false);
                late_campaign->save(0);
            } else {
                const auto before = checkpoint(load<LateStrategy>(directory, 0));
                if (mode == "verify-late" || mode == "verify-third") {
                    late_campaign.emplace(load<LateStrategy>(directory, 1));
                    if (third)
                        verify_third_endpoint(*late_campaign, before);
                    else
                        verify_late_endpoint(*late_campaign, before);
                    std::cout << "PASS read-only rank-route cold-file endpoint verification"
                              << std::endl;
                } else {
                    late_campaign.emplace(load<LateStrategy>(directory, 0));
                    require(late_campaign->strategy.stats().target_rank == (third ? 3 : 2),
                            "Saved route target differs from requested phase");
                    const auto first_slot = bytes(app::world_save_slot_path(directory, 0));
                    const auto first_strategy = bytes(directory / "strategy0.txt");
                    late_campaign->run(true);
                    if (third)
                        verify_third_endpoint(*late_campaign, before);
                    else
                        verify_late_endpoint(*late_campaign, before);
                    late_campaign->save(1);
                    require(bytes(app::world_save_slot_path(directory, 0)) == first_slot &&
                                bytes(directory / "strategy0.txt") == first_strategy,
                            "Late restart must preserve its first player checkpoint and evidence");
                    std::cout << (third ? "PASS active third-star campaign: restaurant, victories, "
                                          "cold restart, promotion, school construction, paid "
                                          "activity 7, full-month trade"
                                        : "PASS active second-star campaign: new shops, residence, "
                                          "victories, cold restart, promotion, paid activity 30, "
                                          "continued trade")
                              << std::endl;
                }
            }
            return 0;
        }
        require(mode == "new" || mode == "resume" || mode == "verify",
                "Unknown active campaign phase");
        const auto directory = isolated_directory(argv[0], argv[2]);
        if (mode == "verify") {
            // Dispose of the first decoded Owner after extracting read-only scalar evidence.
            const auto before = checkpoint(load(directory, 0));
            campaign.emplace(load(directory, 1));
            verify_endpoint(*campaign, before);
            std::cout << "PASS read-only cold-file endpoint verification; no route rerun or "
                         "per-tick revalidation claimed"
                      << std::endl;
        } else if (mode == "new") {
            require(!std::filesystem::exists(app::world_system_path(directory)) &&
                        !std::filesystem::exists(app::world_save_slot_path(directory, 0)),
                    "New campaign cannot overwrite previous files");
            campaign.emplace(Campaign{ark::test::initial_world(), {}, directory, {}});
            app::WorldNewGameDraft draft;
            draft.human.name = "经营验收主角";
            draft.human.custom_name = true;
            good(app::start_world_draft(campaign->state, campaign->system, draft, directory));
            campaign->strategy.reconcile(campaign->state);
            campaign->run(false);
            campaign->save(0);
        } else {
            campaign.emplace(load(directory));
            const auto before = checkpoint(*campaign);
            const auto old_slot = bytes(app::world_save_slot_path(directory, 0));
            campaign->run(true);
            verify_endpoint(*campaign, before);
            campaign->save(1);
            require(bytes(app::world_save_slot_path(directory, 0)) == old_slot,
                    "Restarted business must preserve the first checkpoint");
            std::cout << "PASS active first-star campaign: construction, trade, cultivation, "
                         "task victory, promotion, paid exhibition, continued trade, cold restart"
                      << std::endl;
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Active campaign failed: " << error.what() << std::endl;
        if (campaign)
            std::cerr << "STATE " << campaign->strategy.diagnose(campaign->state) << std::endl;
        if (late_campaign)
            std::cerr << "STATE " << late_campaign->strategy.diagnose(late_campaign->state)
                      << std::endl;
        if (pot_campaign)
            std::cerr << "STATE " << pot_campaign->strategy.diagnose(pot_campaign->state)
                      << std::endl;
        return 1;
    }
}
