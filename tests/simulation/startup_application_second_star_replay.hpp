#pragma once
#include <filesystem>
#include "startup_application_active_replay.hpp"

// 二星策略首批：有限首星交接及真实下一任务；不宣称已经完成二星或住宅。
int run_startup_application_second_star_replay_cli(int argc, const char **argv);
int run_startup_application_second_star_driver_checks(const std::filesystem::path &parent);

namespace second_star_replay_support {
using Application = ark::simulation::StartupApplication;
using Metadata = ark::simulation::StartupApplicationReplayMetadata;
using Bytes = std::vector<std::uint8_t>;
using Command = std::array<std::int64_t, 5>;
struct Round { std::vector<Command> commands; std::vector<ark::simulation::StartupAudioRequest> sounds; };
struct Handoff { std::unique_ptr<Application> application; Metadata metadata; std::uint64_t next_task_month{}; };
// 工厂只返回完整成功的自持应用，不允许调用者传入可被半推进的应用。
Handoff prepare_v2_handoff(const std::filesystem::path &source, const std::filesystem::path &live,
                          const std::filesystem::path &unused, const std::vector<std::filesystem::path> &protected_paths);
std::string validate_v2(const Application &, const Metadata &);
std::string validate_v2_origin(const Metadata &);
std::string previous_handoff_json(const Metadata &);
enum class Modal { wait, acknowledge, confirm_task, depart_task, deadline, award, leave };
Modal inherited_modal(const Application &); // 只复用已接任务／说明页；拒绝新任务22/23。
Bytes read_bounded_file(const std::filesystem::path &);
std::string directory_digest(const std::filesystem::path &);
std::string residence_observation(const Application &);
std::string round_trace(const Application &, const Metadata &,
                       const ark::simulation::StartupApplicationReplayValidator &,
                       const Round &, const std::filesystem::path &system);
} // namespace second_star_replay_support
