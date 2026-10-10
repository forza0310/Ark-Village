#pragma once
#include "ark/simulation/presentation/startup_audio.hpp"
#include <filesystem>
#include <memory>
#include <vector>

namespace ark::desktop {
// Main-thread lifetime spans title -> world; audio resources retire before the device.
class WorldAudio {
  public:
    explicit WorldAudio(const std::filesystem::path &assets);
    ~WorldAudio();
    WorldAudio(const WorldAudio &) = delete;
    WorldAudio &operator=(const WorldAudio &) = delete;
    void consume(const std::vector<simulation::StartupAudioRequest> &requests);
    void update(double now);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace ark::desktop
