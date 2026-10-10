#pragma once

// Native framebuffer ownership shared by title and world rendering. Input continues to use
// window points; artwork and deferred text must use the same framebuffer camera before blit.
#include "projection.hpp"
#include <stdexcept>

namespace ark::desktop {
struct WorldCanvas {
    RenderTexture2D texture{};
    Extent size{};
    WorldCanvas() = default;
    WorldCanvas(const WorldCanvas &) = delete;
    WorldCanvas &operator=(const WorldCanvas &) = delete;
    ~WorldCanvas() {
        if (texture.id)
            UnloadRenderTexture(texture);
    }
    void resize(Extent next) {
        if (texture.id && next.width == size.width && next.height == size.height)
            return;
        auto created = LoadRenderTexture(next.width, next.height);
        if (!created.id)
            throw std::runtime_error("Cannot allocate world framebuffer");
        SetTextureFilter(created.texture, TEXTURE_FILTER_POINT);
        if (texture.id)
            UnloadRenderTexture(texture);
        texture = created;
        size = next;
    }
};
} // namespace ark::desktop
