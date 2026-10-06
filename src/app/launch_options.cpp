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
    bool explicit_world{}, explicit_legacy{};
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        const auto &argument = arguments[i];
        if (argument == "--help") {
            options.mode = LaunchMode::help;
            return {options, {}};
        }
        if (argument == "--check") {
            options.mode = LaunchMode::check;
        } else if (argument == "--check-ai") {
            options.mode = LaunchMode::check_ai;
        } else if (argument == "--paused") {
            options.paused = true;
        } else if (argument == "--ai-preview") {
            options.ai_preview = true;
        } else if (argument == "--world") {
            explicit_world = true;
        } else if (argument == "--legacy-slice") {
            explicit_legacy = true;
        } else if (argument == "--verify-play") {
            options.verify_play = true;
        } else if (argument == "--inspect-page") {
            if (++i >= arguments.size())
                return {std::nullopt, "--inspect-page requires a page"};
            const auto &page = arguments[i];
            if (page != "shops" && page != "plants" && page != "food" && page != "arrival" &&
                page != "visitor" && page != "menu" && page != "placement" && page != "detail" &&
                page != "bonuses" && page != "equipment" && page != "booster" && page != "motion" &&
                page != "ai" && page != "world-active" && page != "world-month" &&
                page != "world-rank" && page != "world-combat" && page != "world-reward" &&
                page != "world-exp" && page != "world-rest" && page != "world-rest-hp" &&
                page != "world-news" && page != "world-break" && page != "world-award" &&
                page != "world-task-team" && page != "world-task-result" &&
                page != "world-task-recruitment" && page != "world-menu" &&
                page != "world-building" && page != "world-build-preview" &&
                page != "world-build-preview-hidden" && page != "world-build-rotated" &&
                page != "world-details" && page != "world-built" && page != "world-award-granted" &&
                page != "world-task-added" && page != "world-level-up" &&
                page != "world-month-income" && page != "world-month-defeats" &&
                page != "world-task-victory" && page != "world-task-popularity" &&
                page != "world-human" && page != "world-human-attributes" &&
                page != "world-human-equipment" && page != "world-human-spells" &&
                page != "world-professions" && page != "world-profession-preview" &&
                page != "world-profession-cancel" && page != "world-profession-change" &&
                page != "world-profession-complete" && page != "world-gift-cancel" &&
                page != "world-gifts" && page != "world-gifts-armor" &&
                page != "world-gifts-shield" && page != "world-gifts-accessory" &&
                page != "world-gift-confirm" && page != "world-equipment-info" &&
                page != "world-gift-result" && page != "world-equipment-change" &&
                page != "world-tax" && page != "world-tax-collected" && page != "world-village" &&
                page != "world-village-start" && page != "world-village-results" &&
                page != "world-expansion-catalogue" && page != "world-expansion-start" &&
                page != "world-expansion-completed" && page != "world-reward95" &&
                page != "world-save" && page != "world-load-error" && page != "world-load" &&
                page != "world-item-gift" && page != "world-facility-items" &&
                page != "world-facility-item-result" && page != "world-commerce" &&
                page != "world-commerce-buy" && page != "world-commerce-receipt" &&
                page != "world-commerce-facilities" && page != "world-commerce-facility-info" &&
                page != "world-commerce-facility-reward" && page != "world-village-menu" &&
                page != "world-commerce-suite" && page != "world-road-start" &&
                page != "world-road-end" && page != "world-road-built" &&
                page != "world-road-remove" && page != "world-demolished" &&
                page != "world-home-credit" && page != "world-home-rebuilt" &&
                page != "world-home-suite")
                return {std::nullopt, "Unknown inspection page"};
            options.inspect_page = page;
        } else if (argument == "--save-dir") {
            if (++i >= arguments.size() || arguments[i].empty() || arguments[i][0] == '-')
                return {std::nullopt, "--save-dir requires a directory"};
            options.save_directory = arguments[i];
        } else if (argument == "--font") {
            if (++i >= arguments.size() || arguments[i].empty() || arguments[i][0] == '-')
                return {std::nullopt, "--font requires a TTF path"};
            options.font = arguments[i];
        } else if (argument == "--size") {
            if (i + 2 >= arguments.size() || !positive(arguments[i + 1], 4096, options.width) ||
                !positive(arguments[i + 2], 4096, options.height) || options.width < 240 ||
                options.height < 256) {
                return {std::nullopt, "--size requires width 240..4096 and height 256..4096"};
            }
            i += 2;
        } else if (argument == "--zoom-percent") {
            if (++i >= arguments.size() || !positive(arguments[i], 200, options.zoom_percent) ||
                options.zoom_percent < 50)
                return {std::nullopt, "--zoom-percent requires an integer 50..200"};
        } else if (argument == "--tick-rate") {
            if (++i >= arguments.size() || !positive(arguments[i], 240, options.tick_rate))
                return {std::nullopt, "--tick-rate requires an integer 1..240"};
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
    if (!options.screenshot.empty() &&
        (options.frames == 0 || options.mode != LaunchMode::window)) {
        return {std::nullopt, "--screenshot requires a bounded window run with --frames"};
    }
    if (!options.inspect_page.empty() &&
        (options.frames == 0 || options.mode != LaunchMode::window))
        return {std::nullopt, "--inspect-page requires a bounded window run"};
    if (options.ai_preview && options.mode != LaunchMode::window)
        return {std::nullopt, "--ai-preview requires a window run"};
    if (options.inspect_page == "ai")
        options.ai_preview = true;
    const bool world_inspection = options.inspect_page.rfind("world-", 0) == 0;
    // Select a single owner after parsing, so argument order cannot silently change the world.
    // Named legacy diagnostics remain explicit opt-ins; generic size/pause/check options do not.
    const bool legacy_diagnostic = options.ai_preview || options.verify_play ||
                                   options.mode == LaunchMode::check_ai ||
                                   (!options.inspect_page.empty() && !world_inspection);
    if (explicit_world && explicit_legacy)
        return {std::nullopt, "--world and --legacy-slice are mutually exclusive"};
    if (explicit_world && legacy_diagnostic)
        return {std::nullopt, "--world cannot be combined with legacy slice diagnostics"};
    if (world_inspection && (explicit_legacy || legacy_diagnostic))
        return {std::nullopt, "World inspection cannot be combined with legacy slice diagnostics"};
    options.world = !explicit_legacy && !legacy_diagnostic;
    if ((options.inspect_page == "world-commerce-suite" ||
         options.inspect_page == "world-home-suite") &&
        options.screenshot.empty())
        return {std::nullopt, "Inspection suite requires --screenshot filename prefix"};
    if (!options.world && !options.save_directory.empty())
        return {std::nullopt, "--save-dir requires the continuous world"};
    if (options.inspect_page == "world-load" && options.save_directory.empty())
        return {std::nullopt, "world-load inspection requires --save-dir"};
    if (options.world && options.tick_rate != 0)
        return {std::nullopt, "--tick-rate requires --legacy-slice or a legacy diagnostic"};
    if (options.ai_preview && !options.inspect_page.empty() && options.inspect_page != "ai")
        return {std::nullopt, "--ai-preview cannot be combined with a different inspection page"};
    if (options.verify_play &&
        (options.frames == 0 || options.mode != LaunchMode::window || options.paused ||
         options.ai_preview || !options.inspect_page.empty() || options.tick_rate != 0))
        return {std::nullopt, "--verify-play requires --frames and normal original-paced startup"};
    return {options, {}};
}

} // namespace ark::app
