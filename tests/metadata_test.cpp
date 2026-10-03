#include "ark/assets/sprite.hpp"
#include "ark/assets/table.hpp"
#include <iostream>
#include <stdexcept>

void check(bool value) {
    if (!value)
        throw std::runtime_error("Metadata contract failed");
}
int main() {
    try {
        const std::vector<std::uint8_t> header{0, 1, 0, 6, 0, 0, 0, 6};
        const auto sprite = ark::assets::parse_legacy_seb(header);
        check(sprite.frame_count == 6 && sprite.layers[0].legacy_tag == 6 &&
              sprite.layers[0].parts.empty());
        for (auto bytes : {std::vector<std::uint8_t>{0, 1}, std::vector<std::uint8_t>{128, 0},
                           std::vector<std::uint8_t>{0, 0, 0, 0, 1}}) {
            bool rejected = false;
            try {
                ark::assets::parse_legacy_seb(bytes);
            } catch (const std::runtime_error &) {
                rejected = true;
            }
            check(rejected);
        }
        const auto rows = ark::assets::parse_tsv({'1', '\t', '\t', 'x', '\r', '\n'});
        check(rows.size() == 1 && rows[0].size() == 3 && rows[0][1].empty());
        bool rejected = false;
        try {
            ark::assets::parse_tsv({0xE4, 0xB8});
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        check(rejected);
        std::cout << "PASS metadata boundaries\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
