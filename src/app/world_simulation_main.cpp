// Bounded standard-C++ entry for the canonical researched world. Frames count framework calls,
// not renders or seconds. Page confirmation is an explicit test-user strategy, never implicit AI.
#include "ark/simulation/startup_world_runtime.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>

namespace {
namespace simulation = ark::simulation;
namespace rules = ark::simulation::rules;
struct Options {
    int frames{1000};
    std::optional<int> months;
    std::uint64_t seed{1}; // Reproducible input; the APK's actual default seed was not observed.
    int speed{};
    bool auto_confirm{};
    bool end_awards{};
};
constexpr const char *usage =
    "Usage: ark_world_simulation [--frames 1..100000] [--months 1..24] [--seed unsigned64]\n"
    "                            [--speed 0|1] [--auto-confirm] [--end-awards]\n"
    "Framework updates are unpaced. --months sets a goal within the --frames budget.\n"
    "Ordinary pages wait unless --auto-confirm supplies explicit test-user confirmations.\n"
    "Timed pages 16/56/57/97/98 advance themselves. Task, human and tax decisions wait for\n"
    "explicit input; auto-confirm never accepts, departs, renews or cancels these pages.\n"
    "Annual termination additionally needs --end-awards;\n"
    "that test policy requests and confirms termination, retaining unused medals.\n"
    "Exit codes: 0 completed budget/goal, 1 runtime failure, 2 invalid arguments, 3 unmet goal.\n";

// from_chars must consume the entire unsigned decimal token; signs and whitespace are invalid.
std::uint64_t number(const std::string &text, const std::string &option) {
    std::uint64_t value{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
        throw std::invalid_argument(option + " requires an unsigned decimal integer");
    return value;
}
Options parse(int argc, char **argv) {
    Options options;
    std::set<std::string> seen;
    for (int i = 1; i < argc; ++i) {
        const std::string option = argv[i];
        if (option != "--frames" && option != "--months" && option != "--seed" &&
            option != "--speed" && option != "--auto-confirm" && option != "--end-awards")
            throw std::invalid_argument("Unknown argument: " + option);
        if (!seen.insert(option).second)
            throw std::invalid_argument("Duplicate argument: " + option);
        if (option == "--auto-confirm") {
            options.auto_confirm = true;
            continue;
        }
        if (option == "--end-awards") {
            options.end_awards = true;
            continue;
        }
        if (++i == argc)
            throw std::invalid_argument(option + " requires a value");
        const auto value = number(argv[i], option);
        if (option == "--frames") {
            if (value < 1 || value > 100000)
                throw std::invalid_argument("--frames must be in 1..100000");
            options.frames = static_cast<int>(value);
        } else if (option == "--months") {
            if (value < 1 || value > 24)
                throw std::invalid_argument("--months must be in 1..24");
            options.months = static_cast<int>(value);
        } else if (option == "--speed") {
            if (value > 1)
                throw std::invalid_argument("--speed must be 0 or 1");
            options.speed = static_cast<int>(value);
        } else
            options.seed = value;
    }
    return options;
}
const rules::WorldScriptPage *top_page(const simulation::StartupWorldRuntimeState &state) {
    const auto found = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                    [](const auto &page) { return page.lifecycle != 4; });
    return found == state.scripts.pages.rend() ? nullptr : &*found;
}
std::int64_t month_index(const simulation::StartupWorldRuntimeState &state) {
    return static_cast<std::int64_t>(state.scene.calendar.year) * 12 + state.scene.calendar.month;
}
void print_state(const char *event, const simulation::StartupWorldRuntimeState &state, int frames) {
    const auto &date = state.scene.calendar;
    const auto &world = state.scene.world.world;
    const auto *page = top_page(state);
    std::cout << event << " frames=" << frames << " logical_rounds=" << state.simulation_steps
              << " date=" << date.year << '/' << date.month << '/' << date.subperiod << '/'
              << date.units << " month_ticks=" << date.month_ticks
              << " cash=" << world.ai.accounting.funds()
              << " humans=" << world.ai.human_order.size()
              << " monsters=" << world.ai.monster_order.size()
              << " retired=" << world.ai.retired_actors.size()
              << " tasks=" << state.task_order.size()
              << " random_draws=" << state.scene.random.draws()
              << " scene=" << state.scene.scene_state
              << " arrival_counter=" << state.arrival_counter << " report=" << state.report_state
              << '/' << state.report_counter << " pages=" << state.scripts.pages.size();
    if (page)
        std::cout << " top_page=" << page->id << " page_kind=" << static_cast<int>(page->kind)
                  << " legacy_page=" << page->legacy_page << " source=" << page->source_record;
    std::cout << '\n';
}
void add_amount(std::int64_t &sum, std::int64_t value) {
    if (value < 0 || sum > std::numeric_limits<std::int64_t>::max() - value)
        throw std::overflow_error("Ledger summary exceeds signed64 range");
    sum += value;
}
void print_summary(const simulation::StartupWorldRuntimeState &state, int frames,
                   std::int64_t completed_months, int confirmations, const char *result) {
    std::int64_t income{}, expenses{}, facility_income{};
    const auto &ledger = state.scene.world.world.ai.accounting;
    for (const auto &[id, entry] : ledger.entries()) {
        (void)id;
        if (entry.direction == rules::CashDirection::income) {
            add_amount(income, entry.amount);
            if (entry.category == rules::CashCategory::facilities)
                add_amount(facility_income, entry.amount);
        } else
            add_amount(expenses, entry.amount);
    }
    print_state("summary", state, frames);
    std::cout << "ledger result=" << result << " months=" << completed_months
              << " income=" << income << " expenses=" << expenses
              << " facility_income=" << facility_income << " entries=" << ledger.entries().size()
              << " confirmations=" << confirmations << '\n';
}

int run(const Options &options) {
    // The reset owner exists only while the runtime takes over its source-validated initial state.
    auto session = [&] {
        simulation::StartupSession initial;
        return simulation::StartupWorldRuntimeSession(
            initial.state(), rules::WorldRandomStream::from_java_seed(options.seed));
    }();
    session.set_speed(options.speed);
    const auto initial_month = month_index(session.state());
    auto last_month = initial_month;
    int frames{}, confirmations{};
    std::set<std::uint64_t> observed_pages;
    std::cout << "policy seed=" << options.seed << " seed_type=explicit_java"
              << " frame_budget=" << options.frames << " month_goal=" << options.months.value_or(0)
              << " auto_confirm=" << options.auto_confirm << " end_awards=" << options.end_awards
              << " speed=" << options.speed << " pacing=unpaced task_input=manual\n";
    print_state("initial", session.state(), 0);
    while (frames < options.frames) {
        const auto result = session.update();
        ++frames;
        if (!result.candidate || result.error != simulation::StartupWorldRuntimeError::none) {
            std::cerr << "runtime_error frame=" << frames
                      << " runtime=" << static_cast<int>(result.error)
                      << " scene=" << static_cast<int>(result.scene_error)
                      << " world=" << static_cast<int>(result.world_error) << '\n';
            print_summary(session.state(), frames, month_index(session.state()) - initial_month,
                          confirmations, "runtime_failed");
            return 1;
        }
        (void)
            session.take_sound_requests(); // Explicit silent consumer, not retained world history.
        const auto *page = top_page(session.state());
        if (!page) {
            std::cerr << "runtime_error frame=" << frames << " missing_active_page\n";
            print_summary(session.state(), frames, month_index(session.state()) - initial_month,
                          confirmations, "missing_active_page");
            return 1;
        }
        const auto current_month = month_index(session.state());
        if (current_month != last_month) {
            print_state("month", session.state(), frames);
            last_month = current_month;
        }
        if (page->kind != rules::WorldScriptPageKind::scene &&
            observed_pages.insert(page->id).second)
            print_state("page", session.state(), frames);
        // Automatic waiting/camera pages never receive fabricated confirmation. Annual-page
        // termination is a separate opted-in test input, not an implication of auto-confirm.
        const bool raw = page->kind == rules::WorldScriptPageKind::raw_page;
        const bool automatic =
            raw && (page->legacy_page == 16 || page->legacy_page == 56 || page->legacy_page == 57 ||
                    page->legacy_page == 97 || page->legacy_page == 98);
        // The source generic raw33 confirmation chooses renewal. Do not reuse it as an
        // ordinary acknowledgement, and never invent recruitment/departure/cancellation policy.
        const bool decision = raw && ((page->legacy_page >= 22 && page->legacy_page <= 28) ||
                                      page->legacy_page == 33 || page->legacy_page == 83 ||
                                      (page->legacy_page >= 60 && page->legacy_page <= 66) ||
                                      page->legacy_page == 68 || page->legacy_page == 70 ||
                                      page->legacy_page == 73 || page->legacy_page == 90);
        const bool annual = raw && page->legacy_page == 87;
        if (annual && options.end_awards) {
            const auto id = page->id;
            auto error = session.act_award_page(id, rules::WorldAwardAction::request_termination);
            if (error == simulation::StartupWorldRuntimeError::none) {
                ++confirmations;
                error = session.act_award_page(id, rules::WorldAwardAction::confirm_termination);
            }
            if (error != simulation::StartupWorldRuntimeError::none) {
                std::cerr << "award_input_error frame=" << frames << " page=" << id
                          << " error=" << static_cast<int>(error) << '\n';
                print_summary(session.state(), frames, current_month - initial_month, confirmations,
                              "award_input_failed");
                return 1;
            }
            ++confirmations;
        } else if (options.auto_confirm && !automatic && !annual && !decision &&
                   page->kind != rules::WorldScriptPageKind::scene) {
            const auto id = page->id;
            const auto acknowledged = session.acknowledge_page(id);
            if (acknowledged != simulation::StartupWorldRuntimeError::none) {
                std::cerr << "page_error frame=" << frames << " page=" << id
                          << " error=" << static_cast<int>(acknowledged) << '\n';
                print_summary(session.state(), frames, current_month - initial_month, confirmations,
                              "page_failed");
                return 1;
            }
            ++confirmations;
        }
        (void)session.take_sound_requests(); // Successful acknowledgement may also emit a sound.
        if (options.months && current_month - initial_month >= *options.months) {
            print_summary(session.state(), frames, current_month - initial_month, confirmations,
                          "month_goal_reached");
            return 0;
        }
    }
    print_summary(session.state(), frames, month_index(session.state()) - initial_month,
                  confirmations, options.months ? "month_goal_unmet" : "frame_budget_complete");
    return options.months ? 3 : 0;
}
} // namespace

int main(int argc, char **argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << usage;
        return 0;
    }
    Options options;
    try {
        options = parse(argc, argv);
    } catch (const std::invalid_argument &error) {
        std::cerr << "argument_error: " << error.what() << '\n' << usage;
        return 2;
    }
    try {
        return run(options);
    } catch (const std::exception &error) {
        std::cerr << "runtime_exception: " << error.what() << '\n';
        return 1;
    }
}
