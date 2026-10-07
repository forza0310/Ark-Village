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
    ark::world::SourceMap map{24, 24, {}};
    for (auto size :
         {Vector2{480, 660}, Vector2{960, 512}, Vector2{1280, 600}, Vector2{240, 256}}) {
        const auto extent =
            ark::desktop::canvas_extent(static_cast<int>(size.x), static_cast<int>(size.y));
        const auto box =
            ark::desktop::viewport(static_cast<int>(size.x), static_cast<int>(size.y), extent);
        if (extent.width < 240 || extent.height < 256 || box.width < size.x - 2 ||
            box.height < size.y - 2)
            throw std::runtime_error("Responsive view did not fill window");
        for (float zoom : {0.5F, 1.0F, 1.05F, 1.5F, 2.0F})
            for (auto cell : {ark::world::Cell{9, 3}, ark::world::Cell{12, 5}}) {
                const Vector2 camera{30.0F * (cell.x + cell.y) + 40, 15.0F * (cell.y - cell.x) + 5};
                const auto p = ark::desktop::tile_center(cell, camera, extent, zoom);
                const auto continuous = ark::desktop::project_position(
                    {cell.x * 100.0F + 50, cell.y * 100.0F + 50}, camera, extent, zoom);
                const auto origin = ark::desktop::project(cell, camera, extent, zoom);
                if (std::abs(continuous.x - p.x) > 0.01F || std::abs(continuous.y - p.y) > 0.01F ||
                    std::abs(continuous.x - origin.x - 30 * zoom) > 0.01F ||
                    std::abs(continuous.y - origin.y - 14.5F * zoom) > 0.01F)
                    throw std::runtime_error(
                        "Actor feet do not coincide with ground diamond centre");
                const ark::world::Cell next{cell.x, cell.y + 1};
                const auto road_end = ark::desktop::tile_center(next, camera, extent, zoom);
                const auto halfway = ark::desktop::project_position(
                    {cell.x * 100.0F + 50, cell.y * 100.0F + 100}, camera, extent, zoom);
                if (std::abs(halfway.x - (p.x + road_end.x) / 2) > 0.01F ||
                    std::abs(halfway.y - (p.y + road_end.y) / 2) > 0.01F)
                    throw std::runtime_error("Continuous feet drift from road centreline");
                const Vector2 pixels{box.x + p.x * box.width / extent.width,
                                     box.y + p.y * box.height / extent.height};
                const auto logical = ark::desktop::logical_mouse(pixels, box, extent);
                if (!logical || std::abs(logical->x - p.x) > 0.01F ||
                    std::abs(logical->y - p.y) > 0.01F ||
                    ark::desktop::pick(*logical, camera, map, extent, zoom) != cell)
                    throw std::runtime_error("Projection inverse mismatch");
                // Physical render resolution must not change window-point hit testing.
                for (float dpi : {1.0F, 1.5F, 2.0F}) {
                    const auto physical = ark::desktop::viewport(
                        static_cast<int>(size.x * dpi), static_cast<int>(size.y * dpi), extent);
                    const auto rendered =
                        GetWorldToScreen2D(p, ark::desktop::canvas_camera(physical, extent));
                    if (std::abs(rendered.x - pixels.x * dpi) > 0.01F ||
                        std::abs(rendered.y - pixels.y * dpi) > 0.01F)
                        throw std::runtime_error("Framebuffer/window DPI mapping mismatch");
                    const auto hit = ark::desktop::logical_mouse(
                        {rendered.x / dpi, rendered.y / dpi}, box, extent);
                    if (!hit || ark::desktop::pick(*hit, camera, map, extent, zoom) != cell)
                        throw std::runtime_error("Retina hit testing applied DPI twice");
                }
            }
        if (ark::desktop::logical_mouse({-1, -1}, box, extent))
            throw std::runtime_error("Letterbox input accepted");
        Vector2 camera{426, -72};
        const auto start = camera;
        float zoom = 1;
        const auto anchor = ark::desktop::tile_center({12, 5}, camera, extent, zoom);
        ark::desktop::zoom_at(anchor, extent, 1, camera, zoom);
        const auto after = ark::desktop::tile_center({12, 5}, camera, extent, zoom);
        if (std::abs(after.x - anchor.x) > 0.001F || std::abs(after.y - anchor.y) > 0.001F ||
            std::abs(zoom - 1.05F) > 0.001F)
            throw std::runtime_error("Small-step anchored zoom failed");
        ark::desktop::zoom_at(anchor, extent, -1, camera, zoom);
        if (std::abs(zoom - 1) > 0.001F || std::abs(camera.x - start.x) > 0.001F ||
            std::abs(camera.y - start.y) > 0.001F)
            throw std::runtime_error("Zoom round trip moved camera");
        ark::desktop::zoom_at(anchor, extent, 10000, camera, zoom);
        if (zoom != 2)
            throw std::runtime_error("Zoom upper bound failed");
        ark::desktop::zoom_at(anchor, extent, -10000, camera, zoom);
        if (zoom != 0.5F)
            throw std::runtime_error("Zoom lower bound failed");
    }
}
