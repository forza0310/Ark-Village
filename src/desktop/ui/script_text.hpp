#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace ark::desktop::ui {
struct ScriptText {
    std::string text;
    bool centered{};
};
// Decode the maintained script's presentation tags after rule-side placeholder substitution.
// Unknown tags and literal angle brackets remain visible instead of silently deleting content.
ScriptText decode_script_text(std::string_view source);
// Wrap already decoded UTF-8 text using the actual font metrics, preserving explicit line breaks.
std::vector<std::string> wrap_plain_text(const std::string &text, float width,
                                         const std::function<float(const std::string &)> &measure);
} // namespace ark::desktop::ui
