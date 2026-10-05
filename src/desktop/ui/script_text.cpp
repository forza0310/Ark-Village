#include "script_text.hpp"

#include <algorithm>
#include <cctype>

namespace ark::desktop::ui {
ScriptText decode_script_text(std::string_view source) {
    ScriptText result;
    for (std::size_t i = 0; i < source.size();) {
        if (source[i] == '<') {
            const auto end = source.find('>', i + 1);
            if (end != std::string_view::npos) {
                std::string tag(source.substr(i + 1, end - i - 1));
                std::transform(tag.begin(), tag.end(), tag.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (tag == "br" || tag == "br/" || tag == "br /") {
                    result.text += '\n';
                    i = end + 1;
                    continue;
                }
                const bool color = tag.size() == 9 && tag.compare(0, 3, "co=") == 0 &&
                                   std::all_of(tag.begin() + 3, tag.end(),
                                               [](unsigned char c) { return std::isxdigit(c); });
                if (color || tag == "/co" || tag == "po=cm") {
                    result.centered = result.centered || tag == "po=cm";
                    i = end + 1;
                    continue;
                }
            }
        }
        if (source[i] == '\r') {
            result.text += '\n';
            ++i;
            if (i < source.size() && source[i] == '\n')
                ++i;
        } else
            result.text += source[i++];
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
