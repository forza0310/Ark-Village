// A fixed C boundary remains callable even when C++ object layouts have changed.
// Each DLL embeds its own copy, so a partial/replaced module cannot hide behind
// another library's version. No world state is accessed here.
#include "ark_library_contract.hpp"

#if defined(_WIN32)
#define ARK_CONTRACT_EXPORT __declspec(dllexport)
#else
#define ARK_CONTRACT_EXPORT __attribute__((visibility("default")))
#endif

extern "C" ARK_CONTRACT_EXPORT const char *ark_library_contract_v1() noexcept {
    return ARK_LIBRARY_CONTRACT;
}
