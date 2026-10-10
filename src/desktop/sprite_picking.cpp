#include "sprite_picking.hpp"
#include <cmath>
#include <stdexcept>

namespace ark::desktop {
SpritePickMap::~SpritePickMap() {
    if (pixels_.data)
        UnloadImage(pixels_);
    if (buffer_.id)
        UnloadRenderTexture(buffer_);
    if (shader_.id)
        UnloadShader(shader_);
}
void SpritePickMap::reset(Rectangle clip, Camera2D raster, int width, int height) {
    clip_ = clip;
    raster_ = raster;
    width_ = width > 0 ? width : static_cast<int>(std::ceil(clip.x + clip.width));
    height_ = height > 0 ? height : static_cast<int>(std::ceil(clip.y + clip.height));
    entries_.clear();
    if (pixels_.data) {
        UnloadImage(pixels_);
        pixels_ = {};
    }
}
void SpritePickMap::add(Texture2D texture, Rectangle source, Rectangle destination,
                        SpritePickTarget target) {
    if (pixels_.data) {
        UnloadImage(pixels_);
        pixels_ = {};
    }
    entries_.push_back({texture, source, destination, target});
}
void SpritePickMap::rasterize() const {
    if (entries_.size() >= 0xffffffU)
        throw std::runtime_error("Scene exceeds pixel picking identity capacity");
    if (!shader_.id) {
        shader_ = LoadShaderFromMemory(nullptr, R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
out vec4 finalColor;
void main() {
    if (texture(texture0, fragTexCoord).a == 0.0) discard;
    finalColor = vec4(fragColor.rgb, 1.0);
}
)");
        if (!shader_.id || GetShaderLocation(shader_, "texture0") < 0)
            throw std::runtime_error("Cannot initialize visible-sprite picking shader");
    }
    if (!buffer_.id || buffer_.texture.width != width_ || buffer_.texture.height != height_) {
        const auto next = LoadRenderTexture(width_, height_);
        if (!next.id)
            throw std::runtime_error("Cannot allocate visible-sprite picking buffer");
        if (buffer_.id)
            UnloadRenderTexture(buffer_);
        buffer_ = next;
    }
    BeginTextureMode(buffer_);
    ClearBackground(BLANK);
    BeginMode2D(raster_);
    BeginScissorMode(static_cast<int>(std::floor(raster_.offset.x + clip_.x * raster_.zoom)),
                     static_cast<int>(std::floor(raster_.offset.y + clip_.y * raster_.zoom)),
                     static_cast<int>(std::ceil(clip_.width * raster_.zoom)),
                     static_cast<int>(std::ceil(clip_.height * raster_.zoom)));
    BeginShaderMode(shader_);
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        const auto &entry = entries_[i];
        const auto id = static_cast<unsigned>(i + 1);
        const Color color{static_cast<unsigned char>(id & 255),
                          static_cast<unsigned char>((id >> 8) & 255),
                          static_cast<unsigned char>((id >> 16) & 255), 255};
        DrawTexturePro(entry.texture, entry.source, entry.destination, {0, 0}, 0, color);
    }
    EndShaderMode();
    EndScissorMode();
    EndMode2D();
    EndTextureMode();
    // Read back only for a click (or a fixture's first query), never on normal render frames.
    pixels_ = LoadImageFromTexture(buffer_.texture);
    if (!pixels_.data)
        throw std::runtime_error("Cannot read visible-sprite picking result");
    ImageFlipVertical(&pixels_);
}
std::optional<SpritePickTarget> SpritePickMap::pick(Vector2 point) const {
    if (!std::isfinite(point.x) || !std::isfinite(point.y) || entries_.empty() ||
        point.x < clip_.x || point.y < clip_.y || point.x >= clip_.x + clip_.width ||
        point.y >= clip_.y + clip_.height || width_ <= 0 || height_ <= 0)
        return {};
    const auto pixel = GetWorldToScreen2D(point, raster_);
    const int x = static_cast<int>(std::floor(pixel.x));
    const int y = static_cast<int>(std::floor(pixel.y));
    if (x < 0 || y < 0 || x >= width_ || y >= height_)
        return {};
    if (!pixels_.data)
        rasterize();
    const auto color = GetImageColor(pixels_, x, y);
    const unsigned index = color.r | (unsigned(color.g) << 8) | (unsigned(color.b) << 16);
    return index && index <= entries_.size() ? std::optional{entries_[index - 1].target}
                                             : std::nullopt;
}
} // namespace ark::desktop
