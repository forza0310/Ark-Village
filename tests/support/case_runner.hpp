#pragma once

#include <exception>
#include <initializer_list>
#include <iostream>
#include <string_view>

namespace ark::test {
struct Case {
    std::string_view name;
    void (*run)();
};

// One explicit case per process preserves independent fixture/state lifetimes under CTest.
inline int run_case(int argc, char **argv, std::initializer_list<Case> cases) {
    if (argc != 2) {
        std::cerr << "Expected exactly one case name. Available cases:";
        for (const auto &entry : cases)
            std::cerr << ' ' << entry.name;
        std::cerr << '\n';
        return 2;
    }
    for (const auto &entry : cases) {
        if (entry.name != argv[1])
            continue;
        try {
            entry.run();
            return 0;
        } catch (const std::exception &error) {
            std::cerr << "FAIL " << entry.name << ": " << error.what() << '\n';
        } catch (...) {
            std::cerr << "FAIL " << entry.name << ": unknown exception\n";
        }
        return 1;
    }
    std::cerr << "Unknown case: " << argv[1] << ". Available cases:";
    for (const auto &entry : cases)
        std::cerr << ' ' << entry.name;
    std::cerr << '\n';
    return 2;
}
} // namespace ark::test
