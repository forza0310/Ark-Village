// 系统设置的跨局即时权威与raw12应用提交；文件观察、世界及一次性输出联合安装。
#include "dungeon_village_prototype/startup_application.hpp"

#include <exception>
#include <type_traits>

namespace dungeon_village_prototype {
StartupSystemRecords StartupApplication::system_records_candidate() const {
    auto records = storage_.records;
    records.settings = live_settings_; // expected storage_始终是原磁盘观察，不能原地调音。
    return records;
}

void StartupApplication::queue_title_settings() {
    // 标题Init无同值过滤，也不额外Reflect/试听；资源播放由宿主实际执行。
    presentation_requests_.emplace_back(SteamConfigVolume{0, live_settings_.bgm * 51});
    presentation_requests_.emplace_back(SteamConfigMute{0, live_settings_.bgm == 0});
    presentation_requests_.emplace_back(SteamConfigVolume{1, live_settings_.se * 51});
    presentation_requests_.emplace_back(SteamConfigMute{1, live_settings_.se == 0});
    presentation_requests_.emplace_back(StartupAudioRequest{StartupAudioOperation::replace_bgm, 0});
}

std::string StartupApplication::set_new_game_environment(StartupNewGameEnvironment environment) {
    if (!error_.empty()) return error_;
    if ((page_ != StartupApplicationPage::title && page_ != StartupApplicationPage::configure) || clear_ ||
        environment.width <= 0 || environment.height <= 0)
        return "新局环境需要标题或配置页及显式正整数应用宽高";
    new_game_environment_ = environment;
    return {};
}

std::string StartupApplication::commit_config(StartupWorldConfigResult result) {
    if (result.error != StartupWorldRuntimeError::none)
        return "设置命令拒绝：" + std::to_string(static_cast<int>(result.error));
    if (!result.candidate || !world_) return "设置缺完整世界候选";
    auto live = live_settings_;
    live.bgm = result.system_volumes[0]; live.se = result.system_volumes[1];
    if (live.bgm < 0 || live.bgm > 5 || live.se < 0 || live.se > 5) return "设置音量候选越界";
    std::vector<StartupApplicationPresentationRequest> effects;
    effects.reserve(result.effects.size() + (result.reset_offscreen ? 1U : 0U));
    for (const auto &effect : result.effects) {
        std::visit([&](const auto &operation) {
            using T = std::decay_t<decltype(operation)>;
            if constexpr (!std::is_same_v<T, SteamConfigPropertyWrite>)
                effects.emplace_back(operation); // 属性已在世界/系统候选安装，不再次执行setter。
        }, effect);
    }
    if (result.reset_offscreen)
        effects.emplace_back(StartupApplicationPresentationAction::reset_offscreen);
    if (!update_startup_world_render_cache(*result.candidate)) return "设置渲染缓存候选拒绝";
    StartupWorldRuntimeSession candidate;
    candidate.state_ = std::move(*result.candidate);
    candidate.checkpoints_ = world_->checkpoints_;
    // 返回时即使设置相同也保存；发布失败前不安装本轮音量/选择/钟/Pop或输出。
    return commit_world(std::move(candidate), result.save_system, live, std::move(effects));
}

std::string StartupApplication::input_config_page(std::uint64_t page, const StartupConfigInput &input) {
    const auto eligibility = world_action_error();
    if (!eligibility.empty()) return eligibility;
    try {
        return commit_config(prepare_startup_world_config_input(world_->state(), page,
            {live_settings_.bgm, live_settings_.se, live_settings_.high_speed != 0}, input));
    } catch (const std::exception &error) { return error.what(); }
}

std::string StartupApplication::set_config_value(std::uint64_t page, int row, int value) {
    const auto eligibility = world_action_error();
    if (!eligibility.empty()) return eligibility;
    try {
        return commit_config(prepare_startup_world_config_setting(world_->state(), page,
            {live_settings_.bgm, live_settings_.se, live_settings_.high_speed != 0}, row, value));
    } catch (const std::exception &error) { return error.what(); }
}

std::string StartupApplication::set_config_selection(std::uint64_t page, int selection) {
    const auto eligibility = world_action_error();
    if (!eligibility.empty()) return eligibility;
    try {
        auto candidate = *world_;
        const auto error = set_startup_world_config_selection(candidate.state_, page, selection);
        if (error != StartupWorldRuntimeError::none)
            return "设置选择拒绝：" + std::to_string(static_cast<int>(error));
        return commit_world(std::move(candidate));
    } catch (const std::exception &error) { return error.what(); }
}
} // namespace dungeon_village_prototype
