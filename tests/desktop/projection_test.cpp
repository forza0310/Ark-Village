// Input must invert the same transform at letterboxed and portrait/wide window sizes.
#include "projection.hpp"
#include "world_pointer.hpp"
#include <cmath>
#include <stdexcept>
int main() {
    // Same window-point gesture contract at every render DPI: only a short release clicks.
    using ark::desktop::WorldPointerGesture;
    const auto expect = [](bool value, const char *scenario) {
        if (!value)
            throw std::runtime_error(scenario);
    };
    WorldPointerGesture pointer;
    auto input = pointer.sample({100, 100}, true, true, false, true, true);
    expect(!input.click && input.pan.x == 0 && input.pan.y == 0,
           "Press must not open a facility before a possible drag");
    input = pointer.sample({102, 101}, false, false, true, true, true);
    expect(input.click && input.pan.x == 0 && input.pan.y == 0,
           "Small jitter remains a click without moving the camera");
    pointer.sample({100, 100}, true, true, false, true, true);
    input = pointer.sample({104, 103}, false, true, false, true, true);
    expect(!input.click && input.pan.x == 4 && input.pan.y == 3,
           "Drag threshold emits accumulated displacement once");
    input = pointer.sample({112, 109}, false, true, false, false, true);
    expect(!input.click && input.pan.x == 8 && input.pan.y == 6,
           "Captured map drag continues across the footer without clicking it");
    input = pointer.sample({100, 100}, false, false, true, true, true);
    expect(!input.click && input.pan.x == -12 && input.pan.y == -9,
           "Return to the origin and release must never select or buy");
    pointer.sample({10, 10}, true, true, false, false, true);
    input = pointer.sample({100, 100}, false, true, false, true, true);
    expect(!input.click && input.pan.x == 0 && input.pan.y == 0,
           "A press on UI cannot begin map panning by entering the scene");
    expect(!pointer.sample({100, 100}, false, false, true, true, true).click,
           "UI drag release cannot activate a different widget");
    pointer.sample({100, 100}, true, true, false, true, true);
    pointer.sample({105, 105}, false, true, false, true, false);
    expect(!pointer.sample({100, 100}, false, false, true, true, true).click,
           "Focus loss or a pending/modal barrier discards the release");
    pointer.sample({100, 100}, true, true, false, true, true);
    pointer.cancel();
    expect(!pointer.sample({100, 100}, false, false, true, true, true).click,
           "Generation/page/resize reset prevents a stale click in a new context");
    pointer.sample({100, 100}, true, true, false, true, true);
    expect(!pointer.sample({101, 100}, false, false, true, false, true).click,
           "Even a small release across UI/map boundary cannot change its target domain");
    expect(!pointer.sample({100, 100}, false, false, true, false, true).click,
           "A title/button release without a press in the current context cannot activate it");
    pointer.sample({100, 100}, true, true, false, false, true);
    input = pointer.sample({101, 100}, false, false, true, false, true);
    expect(input.click && input.pan.x == 0 && input.pan.y == 0,
           "A short UI press/release still activates its button without panning the world");
    for (auto size :
         {Vector2{480, 660}, Vector2{960, 512}, Vector2{1280, 600}, Vector2{240, 256}}) {
        const auto extent = ark::desktop::canvas_extent(int(size.x), int(size.y));
        const auto box = ark::desktop::viewport(int(size.x), int(size.y), extent);
        expect(extent.width >= 240 && extent.height >= 256 && box.width >= size.x - 2 &&
                   box.height >= size.y - 2,
               "Responsive view did not fill window");
        for (const Vector2 point : {Vector2{40, 60}, Vector2{120, 128},
                                    Vector2{extent.width - 10.F, extent.height - 20.F}}) {
            const Vector2 pixels{box.x + point.x * box.width / extent.width,
                                 box.y + point.y * box.height / extent.height};
            const auto logical = ark::desktop::logical_mouse(pixels, box, extent);
            expect(logical && std::abs(logical->x - point.x) < .01F &&
                       std::abs(logical->y - point.y) < .01F,
                   "Logical/window projection inverse mismatch");
            for (float dpi : {1.F, 1.5F, 2.F}) {
                const auto physical =
                    ark::desktop::viewport(int(size.x * dpi), int(size.y * dpi), extent);
                const auto rendered =
                    GetWorldToScreen2D(point, ark::desktop::canvas_camera(physical, extent));
                expect(std::abs(rendered.x - pixels.x * dpi) < .01F &&
                           std::abs(rendered.y - pixels.y * dpi) < .01F,
                       "Framebuffer/window DPI mapping mismatch");
                const auto hit =
                    ark::desktop::logical_mouse({rendered.x / dpi, rendered.y / dpi}, box, extent);
                expect(hit && std::abs(hit->x - point.x) < .01F &&
                           std::abs(hit->y - point.y) < .01F,
                       "Retina input applied DPI twice");
            }
        }
        expect(!ark::desktop::logical_mouse({-1, -1}, box, extent), "Letterbox input accepted");
    }
}
