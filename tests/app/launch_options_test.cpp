#include "ark/app/launch_options.hpp"

#include <cstdlib>
#include <iostream>

namespace {
void check(bool condition) {
    if (!condition) {
        std::cerr << "Launch argument contract failed\n";
        std::exit(1);
    }
}
} // namespace

int main() {
    using ark::app::LaunchMode;
    using ark::app::parse_arguments;
    const auto defaults = *parse_arguments({}).options;
    check(defaults.mode == LaunchMode::window && defaults.width == 1080 &&
          defaults.height == 720 && !defaults.paused && defaults.zoom_percent == 100 &&
          defaults.save_directory.empty());
    check(parse_arguments({"--check"}).options->mode == LaunchMode::check);
    check(parse_arguments({"--world"}).options.has_value());
    check(parse_arguments({"--world", "--check"}).options->mode == LaunchMode::check);
    check(parse_arguments({"--help"}).options->mode == LaunchMode::help);
    check(parse_arguments({"--save-dir", "test saves"}).options->save_directory == "test saves");
    check(parse_arguments({"--inspect-page", "world-load", "--frames", "8", "--save-dir", "test"})
              .options.has_value());
    check(!parse_arguments({"--inspect-page", "world-load", "--frames", "8"}).options);
    check(!parse_arguments({"--save-dir"}).options);
    check(!parse_arguments({"--save-dir", "--paused"}).options);
    check(!parse_arguments({"--inspect-page", "world-speed", "--frames", "8"}).options);
    check(parse_arguments({"--paused", "--frames", "8", "--size", "1080", "720"}).options->paused);
    check(parse_arguments({"--world", "--inspect-page", "world-month", "--frames", "8"})
              .options->inspect_page == "world-month");
    for (const auto *page : {
             "world-active", "world-rank", "world-task-added", "world-level-up",
             "world-month-income", "world-month-defeats", "world-task-victory",
             "world-task-popularity", "world-combat", "world-reward", "world-exp",
             "world-rest", "world-rest-hp", "world-news", "world-break", "world-award",
             "world-task-team", "world-task-result", "world-task-recruitment", "world-menu",
             "world-building", "world-details", "world-built", "world-award-granted",
             "world-build-preview", "world-build-rotated", "world-road-start", "world-road-end",
             "world-road-built", "world-road-remove", "world-demolished", "world-home-credit",
             "world-home-rebuilt", "world-expansion-catalogue", "world-expansion-start",
             "world-expansion-completed", "world-title", "world-title-slots",
             "world-title-actions", "world-title-records", "world-title-cash",
             "world-title-configure", "world-title-configure-female", "world-title-text",
             "world-title-clear"}) {
        const auto result = parse_arguments({"--inspect-page", page, "--frames", "8"});
        check(result.options && result.options->inspect_page == page);
        check(!parse_arguments({"--inspect-page", page}).options);
        check(!parse_arguments({"--check", "--inspect-page", page, "--frames", "8"}).options);
    }
    for (const auto *page : {"world-commerce-suite", "world-home-suite"}) {
        const auto suite = parse_arguments(
            {"--inspect-page", page, "--frames", "8", "--screenshot", "suite.png"});
        check(suite.options && suite.options->inspect_page == page);
        check(!parse_arguments({"--inspect-page", page, "--frames", "8"}).options);
    }
    check(parse_arguments({"--paused", "--font", "a.ttf"}).options->paused);
    check(parse_arguments({"--font", "a.ttf"}).options->font == "a.ttf");
    for (const auto *percent : {"25", "49", "150", "200"}) {
        const auto zoomed = parse_arguments({"--zoom-percent", percent});
        check(zoomed.options && zoomed.options->zoom_percent == std::stoi(percent));
    }
    const auto bounded =
        parse_arguments({"--size", "720", "990", "--frames", "30", "--screenshot", "a.png"});
    check(bounded.options && bounded.options->width == 720 && bounded.options->height == 990 &&
          bounded.options->frames == 30 && bounded.options->screenshot == "a.png");
    // Retired entry points must fail explicitly instead of silently opening another world.
    for (const auto *option : {"--legacy-slice", "--check-ai", "--ai-preview", "--verify-play",
                               "--tick-rate"}) {
        const auto result = parse_arguments({option});
        check(!result.options && !result.error.empty());
    }
    for (const auto *page : {"shops", "plants", "food", "arrival", "visitor", "menu",
                             "placement", "detail", "bonuses", "equipment", "booster",
                             "motion", "ai"})
        check(!parse_arguments({"--inspect-page", page, "--frames", "8"}).options);
    for (const auto &arguments : std::vector<std::vector<std::string>>{
             {"--unknown"},
             {"--zoom-percent"},
             {"--zoom-percent", "24"},
             {"--zoom-percent", "201"},
             {"--zoom-percent", "100x"},
             {"--font"},
             {"--font", "--check"},
             {"--inspect-page"},
             {"--inspect-page", "unknown", "--frames", "8"},
             {"--frames"},
             {"--frames", "0"},
             {"--frames", "-1"},
             {"--frames", "10x"},
             {"--frames", "999999999999999999999"},
             {"--size", "239", "330"},
             {"--size", "240", "255"},
             {"--size", "480"},
             {"--size", "4097", "660"},
             {"--screenshot"},
             {"--screenshot", "--check"},
             {"--screenshot", "a.png"},
             {"--check", "--frames", "1", "--screenshot", "a.png"}}) {
        const auto result = parse_arguments(arguments);
        check(!result.options && !result.error.empty());
    }
    std::cout << "PASS launch arguments\n";
    return 0;
}
