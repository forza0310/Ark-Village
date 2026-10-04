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
    check(defaults.width == 1080 && defaults.height == 720 && !defaults.paused);
    check(defaults.tick_rate == 0);
    check(parse_arguments({"--tick-rate", "20"}).options->tick_rate == 20);
    check(parse_arguments({"--tick-rate", "240"}).options->tick_rate == 240);
    check(parse_arguments({"--check"}).options->mode == LaunchMode::check);
    check(parse_arguments({"--check-ai"}).options->mode == LaunchMode::check_ai);
    check(parse_arguments({"--ai-preview"}).options->ai_preview);
    check(parse_arguments({"--verify-play", "--frames", "3000"}).options->verify_play);
    check(parse_arguments({"--inspect-page", "ai", "--frames", "8"}).options->ai_preview);
    check(parse_arguments({"--help"}).options->mode == LaunchMode::help);
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
