#include "ark/app/launch_options.hpp"
#include "desktop_session.hpp"

#include <raylib.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {

class BackgroundImage {
  public:
    explicit BackgroundImage(const std::filesystem::path &path) {
        image_ = LoadImage(path.string().c_str());
        if (!image_.data || image_.width != 240 || image_.height != 330) {
            if (image_.data) {
                UnloadImage(image_);
            }
            throw std::runtime_error("Missing or invalid packaged title background: " +
                                     path.string());
        }
    }
    ~BackgroundImage() { UnloadImage(image_); }
    BackgroundImage(const BackgroundImage &) = delete;
    BackgroundImage &operator=(const BackgroundImage &) = delete;
    Image get() const { return image_; }

  private:
    Image image_{};
};

class Window {
  public:
    explicit Window(const ark::app::LaunchOptions &options) {
        ark::desktop::require_display();
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(options.width, options.height, "Ark-Village");
        if (!IsWindowReady()) {
            throw std::runtime_error("Cannot initialize raylib window");
        }
        SetWindowMinSize(240, 330);
        SetTargetFPS(60);
    }
    ~Window() { CloseWindow(); }
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
};

class BackgroundTexture {
  public:
    explicit BackgroundTexture(Image image) {
        texture_ = LoadTextureFromImage(image);
        if (texture_.id == 0) {
            throw std::runtime_error("Cannot upload background texture");
        }
        SetTextureFilter(texture_, TEXTURE_FILTER_POINT);
    }
    ~BackgroundTexture() { UnloadTexture(texture_); }
    BackgroundTexture(const BackgroundTexture &) = delete;
    BackgroundTexture &operator=(const BackgroundTexture &) = delete;
    Texture2D get() const { return texture_; }

  private:
    Texture2D texture_{};
};

void run_window(const ark::app::LaunchOptions &options, Image image) {
    const Window window(options);
    const BackgroundTexture background(image);
    int frame = 0;
    while (!WindowShouldClose() && (options.frames == 0 || frame < options.frames)) {
        const auto texture = background.get();
        const auto scale = std::min(GetScreenWidth() / 240.0f, GetScreenHeight() / 330.0f);
        const Rectangle destination{(GetScreenWidth() - 240 * scale) / 2,
                                    (GetScreenHeight() - 330 * scale) / 2, 240 * scale,
                                    330 * scale};
        BeginDrawing();
        ClearBackground(Color{35, 38, 37, 255});
        DrawTexturePro(texture, {0, 0, 240, 330}, destination, {0, 0}, 0, WHITE);
        EndDrawing();
        ++frame;
    }
    if (!options.screenshot.empty()) {
        if (frame != options.frames) {
            throw std::runtime_error("Window closed before the bounded check completed");
        }
        const auto screenshot = LoadImageFromScreen();
        const auto saved = screenshot.data && ExportImage(screenshot, options.screenshot.c_str());
        if (screenshot.data) {
            UnloadImage(screenshot);
        }
        if (!saved) {
            throw std::runtime_error("Cannot export screenshot");
        }
    }
}

} // namespace

int main(int argc, char **argv) {
    try {
        const auto parsed = ark::app::parse_arguments({argv + 1, argv + argc});
        if (!parsed.options) {
            throw std::runtime_error(parsed.error);
        }
        const auto &options = *parsed.options;
        if (options.mode == ark::app::LaunchMode::help) {
            std::cout << "ark_village [--check] [--size W H] [--frames N] [--screenshot PNG]\n";
            return 0;
        }
        SetTraceLogLevel(LOG_WARNING);
        const auto path =
            std::filesystem::path(GetApplicationDirectory()) / "assets/title-background.png";
        const BackgroundImage background(path);
        if (options.mode == ark::app::LaunchMode::check) {
            std::cout << "PASS packaged startup image (240x330); gameplay not implemented\n";
            return 0;
        }
        run_window(options, background.get());
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Ark-Village: " << error.what() << '\n';
        return 1;
    }
}
