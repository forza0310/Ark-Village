#include "ark/app/launch_options.hpp"

#include <charconv>

namespace ark::app {
namespace {
bool positive(const std::string &text, int maximum, int &value) {
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size() && value > 0 &&
           value <= maximum;
}
} // namespace

LaunchResult parse_arguments(const std::vector<std::string> &arguments) {
    LaunchOptions options;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        const auto &argument = arguments[i];
        if (argument == "--help") {
            options.mode = LaunchMode::help;
            return {options, {}};
        }
        if (argument == "--check") {
            options.mode = LaunchMode::check;
        } else if (argument == "--paused") {
            options.paused = true;
        } else if (argument == "--inspect-page") {
            if (++i >= arguments.size())
                return {std::nullopt, "--inspect-page requires a page"};
            const auto &page = arguments[i];
            if (page != "shops" && page != "plants" && page != "food" && page != "arrival" &&
                page != "visitor")
                return {std::nullopt, "Unknown inspection page"};
            options.inspect_page = page;
        } else if (argument == "--font") {
            if (++i >= arguments.size() || arguments[i].empty() || arguments[i][0] == '-')
                return {std::nullopt, "--font requires a TTF path"};
            options.font = arguments[i];
        } else if (argument == "--size") {
            if (i + 2 >= arguments.size() || !positive(arguments[i + 1], 4096, options.width) ||
                !positive(arguments[i + 2], 4096, options.height) || options.width < 240 ||
                options.height < 330) {
                return {std::nullopt, "--size requires width 240..4096 and height 330..4096"};
            }
            i += 2;
        } else if (argument == "--frames") {
            if (++i >= arguments.size() || !positive(arguments[i], 100000, options.frames)) {
                return {std::nullopt, "--frames requires an integer 1..100000"};
            }
        } else if (argument == "--screenshot") {
            if (++i >= arguments.size() || arguments[i].empty() || arguments[i][0] == '-') {
                return {std::nullopt, "--screenshot requires an output path"};
            }
            options.screenshot = arguments[i];
        } else {
            return {std::nullopt, "Unknown argument: " + argument};
        }
    }
    if (!options.screenshot.empty() && (options.frames == 0 || options.mode == LaunchMode::check)) {
        return {std::nullopt, "--screenshot requires a bounded window run with --frames"};
    }
    if (!options.inspect_page.empty() &&
        (options.frames == 0 || options.mode != LaunchMode::window))
        return {std::nullopt, "--inspect-page requires a bounded window run"};
    return {options, {}};
}

} // namespace ark::app
