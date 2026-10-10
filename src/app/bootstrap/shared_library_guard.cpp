// Runs before main and therefore before any player/session object is constructed.
// The OS loader still owns missing DLL / missing imported-symbol errors, which can
// occur before any application code executes. This guard covers loadable ABI drift.
#include "ark_library_contract.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdio>
#include <cstring>

#if defined(_MSC_VER)
#pragma init_seg(lib)
#endif

namespace {
struct SharedLibraryGuard {
    SharedLibraryGuard() {
        for (const char *name : ark_library_modules) {
            const HMODULE module = GetModuleHandleA(name);
            // Only check the executable's loaded dependency closure. The player
            // package deliberately excludes maintenance and test-only modules.
            if (!module)
                continue;
            using Identity = const char *(*)() noexcept;
            const FARPROC address = GetProcAddress(module, "ark_library_contract_v1");
            Identity identity{};
            static_assert(sizeof(identity) == sizeof(address),
                          "Windows function pointer representation");
            std::memcpy(&identity, &address, sizeof(identity));
            const char *actual = identity ? identity() : nullptr;
            if (actual && std::strcmp(actual, ARK_LIBRARY_CONTRACT) == 0)
                continue;
            std::fprintf(stderr,
                         "Ark-Village: executable and DLL do not match: %s\n"
                         "Install/extract the complete Release package together. Developers: run\n"
                         "node scripts/build_product.mjs <consumer-preset>\n"
                         "Expected %.12s; loaded %.12s. World startup was refused.\n",
                         name, ARK_LIBRARY_CONTRACT, actual ? actual : "no contract");
            std::fflush(stderr);
            ExitProcess(78);
        }
    }
};
#if defined(__GNUC__)
// Check before ordinary executable static constructors as well as before main.
__attribute__((init_priority(101)))
#endif
const SharedLibraryGuard guard;
} // namespace
#endif
