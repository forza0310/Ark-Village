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
    check(parse_arguments({}).options->mode == LaunchMode::window);
    const auto defaults = *parse_arguments({}).options;
    check(defaults.width == 1080 && defaults.height == 720 && !defaults.paused && defaults.world);
    check(ark::app::LaunchOptions{}.world);
    check(defaults.tick_rate == 0);
    check(defaults.save_directory.empty());
    check(parse_arguments({"--save-dir", "test saves"}).options->save_directory == "test saves");
    check(parse_arguments({"--inspect-page", "world-load", "--frames", "8", "--save-dir", "test"})
              .options->world);
    check(!parse_arguments({"--inspect-page", "world-load", "--frames", "8"}).options);
    check(!parse_arguments({"--save-dir"}).options);
    check(!parse_arguments({"--save-dir", "--paused"}).options);
    check(!parse_arguments({"--legacy-slice", "--save-dir", "test"}).options);
    check(parse_arguments({"--legacy-slice", "--tick-rate", "20"}).options->tick_rate == 20);
    check(parse_arguments({"--tick-rate", "240", "--legacy-slice"}).options->tick_rate == 240);
    check(parse_arguments({"--check"}).options->mode == LaunchMode::check);
    check(parse_arguments({"--check"}).options->world);
    check(!parse_arguments({"--legacy-slice"}).options->world);
    check(!parse_arguments({"--check", "--legacy-slice"}).options->world);
    check(parse_arguments({"--check-ai"}).options->mode == LaunchMode::check_ai);
    check(parse_arguments({"--ai-preview"}).options->ai_preview);
    check(!parse_arguments({"--ai-preview"}).options->world);
    check(!parse_arguments({"--ai-preview", "--tick-rate", "20"}).options->world);
    check(!parse_arguments({"--check-ai"}).options->world);
    check(!parse_arguments({"--verify-play", "--frames", "3000"}).options->world);
    check(!parse_arguments({"--inspect-page", "shops", "--frames", "8"}).options->world);
    check(parse_arguments({"--inspect-page", "world-active", "--frames", "8"}).options->world);
    check(parse_arguments({"--inspect-page", "world-rank", "--frames", "8"}).options->world);
    for (const auto &page : {"world-task-added",    "world-level-up",      "world-month-income",
                             "world-month-defeats", "world-task-victory",  "world-task-popularity",
                             "world-combat",        "world-reward",        "world-exp",
                             "world-rest",          "world-rest-hp",       "world-news",
                             "world-break",         "world-speed",         "world-award",
                             "world-task-team",     "world-task-result",   "world-task-recruitment",
                             "world-menu",          "world-building",      "world-details",
                             "world-built",         "world-award-granted", "world-build-preview",
                             "world-build-rotated"}) {
        const auto result = parse_arguments({"--inspect-page", page, "--frames", "8"});
        check(result.options && result.options->world && result.options->inspect_page == page);
        check(
            !parse_arguments({"--legacy-slice", "--inspect-page", page, "--frames", "8"}).options);
    }
    check(parse_arguments({"--paused", "--frames", "8", "--size", "1080", "720"}).options->world);
    check(parse_arguments({"--world"}).options->world);
    check(parse_arguments({"--world", "--check"}).options->world);
    check(parse_arguments({"--world", "--inspect-page", "world-month", "--frames", "8"})
              .options->inspect_page == "world-month");
    check(parse_arguments({"--verify-play", "--frames", "3000"}).options->verify_play);
    check(parse_arguments({"--inspect-page", "ai", "--frames", "8"}).options->ai_preview);
    check(parse_arguments({"--help"}).options->mode == LaunchMode::help);
    const auto suite = parse_arguments(
        {"--inspect-page", "world-commerce-suite", "--frames", "8", "--screenshot", "suite.png"});
    check(suite.options && suite.options->world &&
          suite.options->inspect_page == "world-commerce-suite");
    check(!parse_arguments({"--inspect-page", "world-commerce-suite", "--frames", "8"}).options);
    check(!parse_arguments({"--legacy-slice", "--inspect-page", "world-commerce-suite", "--frames",
                            "8", "--screenshot", "suite.png"})
               .options);
    check(parse_arguments({"--paused", "--font", "a.ttf"}).options->paused);
    check(parse_arguments({"--font", "a.ttf"}).options->font == "a.ttf");
    check(parse_arguments({"--zoom-percent", "150"}).options->zoom_percent == 150);
    check(parse_arguments({"--inspect-page", "shops", "--frames", "8"}).options->inspect_page ==
          "shops");
    const auto bounded =
        parse_arguments({"--size", "720", "990", "--frames", "30", "--screenshot", "a.png"});
    check(bounded.options && bounded.options->width == 720 && bounded.options->height == 990 &&
          bounded.options->frames == 30 && bounded.options->screenshot == "a.png");
    for (const auto &arguments : std::vector<std::vector<std::string>>{
             {"--unknown"},
             {"--world", "--legacy-slice"},
             {"--legacy-slice", "--world"},
             {"--legacy-slice", "--inspect-page", "world-active", "--frames", "8"},
             {"--inspect-page", "world-month", "--frames", "8", "--legacy-slice"},
             {"--legacy-slice", "--inspect-page", "world-rank", "--frames", "8"},
             {"--tick-rate", "20"},
             {"--world", "--ai-preview"},
             {"--world", "--check-ai"},
             {"--world", "--tick-rate", "20"},
             {"--world", "--verify-play", "--frames", "3000"},
             {"--world", "--inspect-page", "shops", "--frames", "8"},
             {"--ai-preview", "--world"},
             {"--verify-play"},
             {"--verify-play", "--frames", "60", "--paused"},
             {"--verify-play", "--frames", "60", "--ai-preview"},
             {"--verify-play", "--frames", "60", "--inspect-page", "arrival"},
             {"--verify-play", "--frames", "60", "--tick-rate", "60"},
             {"--verify-play", "--frames", "60", "--check"},
             {"--verify-play", "--frames", "60", "--check-ai"},
             {"--tick-rate"},
             {"--tick-rate", "0"},
             {"--tick-rate", "-1"},
             {"--tick-rate", "241"},
             {"--tick-rate", "20x"},
             {"--tick-rate", "20.5"},
             {"--zoom-percent"},
             {"--zoom-percent", "49"},
             {"--zoom-percent", "201"},
             {"--zoom-percent", "100x"},
             {"--font"},
             {"--font", "--check"},
             {"--inspect-page"},
             {"--inspect-page", "unknown", "--frames", "8"},
             {"--inspect-page", "shops"},
             {"--check", "--inspect-page", "shops", "--frames", "8"},
             {"--check-ai", "--inspect-page", "shops", "--frames", "8"},
             {"--check-ai", "--frames", "1", "--screenshot", "a.png"},
             {"--check", "--ai-preview"},
             {"--check-ai", "--ai-preview"},
             {"--ai-preview", "--inspect-page", "motion", "--frames", "8"},
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
