#include "ui/script_text.hpp"

#include <stdexcept>

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
} // namespace

int main() {
    using namespace ark::desktop::ui;
    const auto news = decode_script_text("大家的冒险通信<br>发行了");
    check(news.text == "大家的冒险通信\n发行了", "Published script newline leaked into text");
    check(decode_script_text("<po=cm>村庄<co=0066cc>升级</co><br/>条件").text == "村庄升级\n条件",
          "Known presentation markup changed visible content");
    check(decode_script_text("<po=cm>村庄").centered, "Source centered paragraph ignored");
    check(decode_script_text("A<BR>B<br />C\r\nD\rE").text == "A\nB\nC\nD\nE",
          "Explicit break variants or CRLF produced literal markup/duplicate rows");
    check(decode_script_text("<0> 2<3 <unknown> <co=xyz>尾").text == "<0> 2<3 <unknown> <co=xyz>尾",
          "Decoder silently removed unknown source content");
    // Uniform codepoint widths let the test independently check UTF-8 boundaries and hard breaks.
    const auto measure = [](const std::string &value) {
        float width{};
        for (const unsigned char c : value)
            if ((c & 0xc0U) != 0x80U)
                width += 1;
        return width;
    };
    check(wrap_plain_text(news.text, 20, measure) ==
              std::vector<std::string>{"大家的冒险通信", "发行了"},
          "Font wrapping joined an explicit script break");
    check(wrap_plain_text("中文A\n\n尾\n", 2, measure) ==
              std::vector<std::string>{"中文", "A", "", "尾", ""},
          "Wrapping split UTF-8 characters or lost empty lines");
    check(wrap_plain_text("", 20, measure) == std::vector<std::string>{""},
          "Empty script paragraph has no layout row");
}
