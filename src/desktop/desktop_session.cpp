#include "desktop_session.hpp"

#include <stdexcept>

#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#include <array>
#endif

namespace ark::desktop {

void require_display() {
#ifdef __APPLE__
    std::array<CGDirectDisplayID, 64> displays{};
    uint32_t count = 0;
    if (CGGetOnlineDisplayList(static_cast<uint32_t>(displays.size()), displays.data(), &count) !=
        kCGErrorSuccess) {
        throw std::runtime_error("Cannot query macOS displays");
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (!CGDisplayIsAsleep(displays[i])) {
            return;
        }
    }
    throw std::runtime_error(
        "No accessible awake display; run from the logged-in desktop or use --check");
#endif
}

} // namespace ark::desktop
