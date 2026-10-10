#include "world_audio.hpp"
#include "world_audio_policy.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <raylib.h>

namespace ark::desktop {
namespace {
// Formal snd.inf order, registered in assets/SOURCES.json; not the older SE name table.
constexpr std::array<const char *, 26> files{"kairoquest_title.ogg",
                                             "kairoquest_main.ogg",
                                             "kairoquest_battle.ogg",
                                             "kairoquest_happy.ogg",
                                             "kairoquest_yorokobi.ogg",
                                             "kairoquest_yorokobishort.ogg",
                                             "kairoquest_kanashimi.ogg",
                                             "se_06.ogg",
                                             "se_00.ogg",
                                             "se_03.ogg",
                                             "se_07.ogg",
                                             "se_09.ogg",
                                             "kairoquest_seattackmnst1.ogg",
                                             "kairoquest_seattackmnst2.ogg",
                                             "kairoquest_seattacksword.ogg",
                                             "kairoquest_seattacklance.ogg",
                                             "kairoquest_semagicheal.ogg",
                                             "kairoquest_semagicfire.ogg",
                                             "kairoquest_semagicblizzard.ogg",
                                             "kairoquest_semagicthunder.ogg",
                                             "department_levelup.ogg",
                                             "z_se07.ogg",
                                             "z_se08.ogg",
                                             "se_04.ogg",
                                             "se_05.ogg",
                                             "se_01.ogg"};
} // namespace
class WorldAudio::Impl : public WorldAudioDevice {
  public:
    explicit Impl(const std::filesystem::path &assets) {
        InitAudioDevice();
        ready = IsAudioDeviceReady();
        if (!ready) {
            std::cerr << "Ark-Village: audio device unavailable; continuing without playback\n";
            return;
        }
        try {
            for (int id = 0; id < 26; ++id) {
                const auto path = (assets / "audio" / files[id]).string();
                if (id < 4) {
                    music[id] = LoadMusicStream(path.c_str());
                    valid[id] = IsMusicValid(music[id]);
                    music[id].looping = false; // Explicit completion restart, not seamless looping.
                } else {
                    sounds[id - 4] = LoadSound(path.c_str());
                    valid[id] = IsSoundValid(sounds[id - 4]);
                }
                if (!valid[id])
                    std::cerr << "Ark-Village: audio resource unavailable: " << path << '\n';
            }
        } catch (...) {
            release();
            throw;
        }
        std::cout << "World audio: device=ready resources="
                  << std::count(valid.begin(), valid.end(), true) << "/26\n";
    }
    ~Impl() override { release(); }
    void release() noexcept {
        if (!ready)
            return;
        for (int id = 0; id < 26; ++id)
            if (valid[id]) {
                if (id < 4)
                    UnloadMusicStream(music[id]);
                else
                    UnloadSound(sounds[id - 4]);
            }
        CloseAudioDevice();
        ready = false;
    }
    bool playing(int id) const override {
        return valid[id] && (id < 4 ? music_active[id] : IsSoundPlaying(sounds[id - 4]));
    }
    void play(int id) override {
        if (!valid[id])
            return;
        if (id < 4) {
            StopMusicStream(music[id]);
            PlayMusicStream(music[id]);
            music_active[id] = true;
        } else {
            StopSound(sounds[id - 4]);
            PlaySound(sounds[id - 4]);
        }
    }
    void stop(int id) override {
        if (id < 4) {
            StopMusicStream(music[id]);
            music_active[id] = false;
        } else
            StopSound(sounds[id - 4]);
    }
    void bgm_volume(float volume) override {
        for (int id = 0; id < 4; ++id)
            if (valid[id])
                SetMusicVolume(music[id], volume);
    }
    void update(double now) {
        if (!ready)
            return;
        for (int id = 0; id < 4; ++id)
            if (valid[id] && music_active[id]) {
                UpdateMusicStream(music[id]);
                if (!IsMusicStreamPlaying(music[id])) {
                    StopMusicStream(music[id]);
                    PlayMusicStream(music[id]);
                }
            }
        policy.update(now, *this);
    }
    bool ready{};
    std::array<bool, 26> valid{};
    std::array<bool, 4> music_active{};
    std::array<Music, 4> music{};
    std::array<Sound, 22> sounds{};
    WorldAudioPolicy policy;
};
WorldAudio::WorldAudio(const std::filesystem::path &assets)
    : impl_(std::make_unique<Impl>(assets)) {}
WorldAudio::~WorldAudio() = default;
void WorldAudio::consume(const std::vector<simulation::StartupAudioRequest> &requests) {
    if (impl_->ready)
        for (const auto &request : requests)
            impl_->policy.request(request, *impl_);
}
void WorldAudio::update(double now) { impl_->update(now); }
} // namespace ark::desktop
