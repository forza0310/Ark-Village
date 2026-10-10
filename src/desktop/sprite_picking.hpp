#pragma once

// Last presented scene, with no simulation ownership. Picking replays actual texture
// blits on demand so GPU sampling, flips and transparent holes agree at every zoom.
#include <cstdint>
#include <optional>
#include <raylib.h>
#include <vector>

namespace ark::desktop {
struct SpritePickTarget {
    enum class Kind { blocker, facility, human };
    Kind kind{Kind::blocker};
    std::uint64_t id{};
};
class SpritePickMap {
  public:
    SpritePickMap() = default;
    ~SpritePickMap();
    SpritePickMap(const SpritePickMap &) = delete;
    SpritePickMap &operator=(const SpritePickMap &) = delete;
    void reset(Rectangle clip, Camera2D raster = {{0, 0}, {0, 0}, 0, 1}, int width = 0,
               int height = 0);
    // Texture handles stay owned by the longer-lived scene Sprites cache.
    void add(Texture2D texture, Rectangle source, Rectangle destination, SpritePickTarget target);
    std::optional<SpritePickTarget> pick(Vector2 point) const;

  private:
    struct Entry {
        Texture2D texture;
        Rectangle source, destination;
        SpritePickTarget target;
    };
    void rasterize() const;
    Rectangle clip_{};
    Camera2D raster_{{0, 0}, {0, 0}, 0, 1};
    int width_{}, height_{};
    std::vector<Entry> entries_;
    mutable RenderTexture2D buffer_{};
    mutable Shader shader_{};
    mutable Image pixels_{};
};
} // namespace ark::desktop
