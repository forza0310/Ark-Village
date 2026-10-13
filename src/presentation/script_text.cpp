#include "ark/presentation/script_text.hpp"

#include <algorithm>
#include <cctype>

namespace ark::desktop::ui {
namespace {
// The source scans C# strings: the tag's bracket index difference is in UTF-16 units.
std::size_t tag_code_units(std::string_view text) {
    std::size_t units{};
    for (const unsigned char byte : text) {
        if ((byte & 0xc0U) != 0x80U)
            units += byte >= 0xf0U ? 2 : 1;
    }
    return units;
}
} // namespace
ScriptText decode_script_text(std::string_view source) {
    ScriptText result;
    std::optional<std::uint32_t> color;
    const auto append = [&](char c) {
        result.text += c;
        if (result.runs.empty() || result.runs.back().rgb != color)
            result.runs.push_back({{}, color});
        result.runs.back().text += c;
    };
    for (std::size_t i = 0; i < source.size();) {
        if (source[i] == '<') {
            const auto end = source.find('>', i + 1);
            if (end != std::string_view::npos && tag_code_units(source.substr(i, end - i)) <= 32) {
                std::string tag(source.substr(i + 1, end - i - 1));
                // Published tags keep their spelling; HTML aliases are not source tokens.
                if (tag == "br") {
                    append('\n');
                    i = end + 1;
                    continue;
                }
                // This adapter consumes the maintained six-digit hex form, not all TextLayout
                // parameters.
                const bool opens_color =
                    tag.size() == 9 && tag.compare(0, 3, "co=") == 0 &&
                    std::all_of(tag.begin() + 3, tag.end(),
                                [](unsigned char c) { return std::isxdigit(c); });
                if (opens_color || tag == "/co" || tag == "po=cm") {
                    if (opens_color)
                        color = static_cast<std::uint32_t>(std::stoul(tag.substr(3), nullptr, 16));
                    else if (tag == "/co")
                        color.reset(); // Source closes to the layout base color, without a nested
                                       // stack.
                    result.centered = result.centered || tag == "po=cm";
                    i = end + 1;
                    continue;
                }
            }
        }
        if (source[i] == '\r') {
            append('\n');
            ++i;
            if (i < source.size() && source[i] == '\n')
                ++i;
        } else
            append(source[i++]);
    }
    return result;
}

std::vector<std::string> wrap_plain_text(const std::string &text, float width,
                                         const std::function<float(const std::string &)> &measure) {
    std::vector<std::string> result;
    std::string line;
    for (std::size_t i = 0; i < text.size();) {
        const auto byte = static_cast<unsigned char>(text[i]);
        const std::size_t count = byte < 0x80 ? 1 : byte < 0xe0 ? 2 : byte < 0xf0 ? 3 : 4;
        const auto next = text.substr(i, std::min(count, text.size() - i));
        if (next == "\n" || (!line.empty() && measure(line + next) > width)) {
            result.push_back(line);
            line.clear();
        }
        if (next != "\n")
            line += next;
        i += next.size();
    }
    result.push_back(line);
    return result;
}
} // namespace ark::desktop::ui
