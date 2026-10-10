// Independent fake-device lifetime: no graphics context or sound hardware is required.
#include "../../src/desktop/platform/world_audio_policy.hpp"
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool result, const char *message) {
    if (!result)
        throw std::runtime_error(message);
}
struct Device : ark::desktop::WorldAudioDevice {
    std::array<bool, 26> active{};
    std::vector<std::string> actions;
    float volume{1};
    bool playing(int id) const override { return active[id]; }
    void play(int id) override {
        active[id] = true;
        actions.push_back("play" + std::to_string(id));
    }
    void stop(int id) override {
        active[id] = false;
        actions.push_back("stop" + std::to_string(id));
    }
    void bgm_volume(float value) override {
        volume = value;
        actions.push_back(value == 0 ? "mute" : "restore");
    }
};
} // namespace
int main() {
    using Operation = ark::simulation::StartupAudioOperation;
    ark::desktop::WorldAudioPolicy policy;
    Device device;
    policy.request({Operation::replace_bgm, 1}, device);
    policy.request({Operation::replace_bgm, 1}, device);
    policy.request({Operation::ordinary_play, 11}, device);
    policy.request({Operation::ordinary_play, 11}, device);
    policy.request({Operation::replace_bgm, 2}, device);
    check(device.actions == std::vector<std::string>{"play1", "play11", "play11", "stop1", "play2"},
          "BGM is idempotent; duplicate ordinary output restarts in order");
    policy.request({Operation::jingle, 5}, device);
    check(device.volume == 0 && device.active[2], "Jingle mutes without stopping BGM");
    for (int n = 0; n < 30; ++n)
        policy.update(n * .05, device);
    check(device.volume == 0, "An active jingle never restores BGM based on elapsed time alone");
    device.active[5] = false;
    for (int n = 30; n < 50; ++n)
        policy.update(n * .05, device);
    check(device.volume == 0, "Twenty stopped checks are insufficient");
    policy.update(2.5, device);
    check(device.volume == 1, "The twenty-first stopped check restores BGM");
    policy.request({Operation::jingle, 5}, device);
    policy.request({Operation::jingle, 4}, device);
    device.active[5] = false;
    policy.update(200, device);
    check(device.volume == 0 && device.active[4], "Later jingle replaces the tracked sound");
    device.active[4] = false;
    for (int n = 0; n < 100; ++n)
        policy.update(200, device);
    check(device.volume == 0, "Repeated rendering cannot spend jingle recovery checks");
    for (int n = 1; n <= 21; ++n)
        policy.update(200 + n * .05, device);
    check(device.volume == 1, "New jingle gets its complete recovery eligibility");
    Device full;
    ark::desktop::WorldAudioPolicy ports;
    for (int id : {1, 7, 8, 9})
        ports.request({Operation::ordinary_play, id}, full);
    ports.request({Operation::ordinary_play, 10}, full);
    check(!full.active[10] && full.actions.size() == 4,
          "Full ports do not steal an existing sound");
    ports.request({Operation::ordinary_play, 8}, full);
    check(full.actions.size() == 5 && full.actions.back() == "play8",
          "Existing object still restarts when ports are full");
    ports.request({Operation::replace_bgm, 2}, full);
    check(!full.active[1] && full.active[2], "BGM replacement reuses its released port");
    bool invalid{};
    try {
        ports.request({Operation::ordinary_play, 26}, full);
    } catch (const std::invalid_argument &) {
        invalid = true;
    }
    check(invalid, "Unknown asset IDs reject explicitly");
}
