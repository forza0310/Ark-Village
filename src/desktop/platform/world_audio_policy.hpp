#pragma once
#include "ark/simulation/presentation/startup_audio.hpp"
#include <array>
#include <optional>
#include <stdexcept>

namespace ark::desktop {
// Device-neutral playback decisions; neither snapshots nor render rate produce requests.
class WorldAudioDevice {
  public:
    virtual ~WorldAudioDevice() = default;
    virtual bool playing(int id) const = 0;
    virtual void play(int id) = 0;
    virtual void stop(int id) = 0;
    virtual void bgm_volume(float volume) = 0;
};
class WorldAudioPolicy {
  public:
    void request(simulation::StartupAudioRequest request, WorldAudioDevice &device) {
        using Operation = simulation::StartupAudioOperation;
        const int id = request.id;
        if (id < 0 || id >= 26)
            throw std::invalid_argument("Audio request outside published 26-slot catalogue");
        if (request.operation == Operation::replace_bgm) {
            if (device.playing(id))
                return;
            for (int other = 0; other < 4; ++other)
                if (device.playing(other))
                    device.stop(other);
        } else if (request.operation == Operation::jingle) {
            jingle_ = id;
            remaining_ = 20;
            device.bgm_volume(0);
        } else if (request.operation != Operation::ordinary_play) {
            throw std::invalid_argument("Unknown typed audio operation");
        }
        // Same object restarts; a full set of four ports cannot steal another object.
        int playing{};
        for (int other = 0; other < 26; ++other)
            playing += device.playing(other);
        if (device.playing(id) || playing < 4)
            device.play(id);
    }
    void update(double now, WorldAudioDevice &device) {
        // PC adaptation: independent 47ms checks, no accumulated catch-up or world ticks.
        if (now < next_check_)
            return;
        next_check_ = now + .047;
        if (jingle_ && !device.playing(*jingle_) && remaining_-- <= 0) {
            jingle_.reset();
            device.bgm_volume(1);
        }
    }

  private:
    std::optional<int> jingle_;
    int remaining_{};
    double next_check_{};
};
} // namespace ark::desktop
