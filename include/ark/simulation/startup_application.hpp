#pragma once

#include "ark/simulation/startup_system_records.hpp"
#include "ark/simulation/startup_world_clear_score.hpp"
#include "ark/simulation/startup_world_profile.hpp"
#include <array>

namespace ark::simulation {
enum class StartupApplicationMode { logic, title_presentation };
enum class StartupApplicationPage { title, overwrite, configure, records, world };
struct StartupApplicationPaths {
    std::filesystem::path system;
    std::array<std::filesystem::path, 2> worlds;
};
struct StartupTitleDraft {
    std::string village{"口袋冒险村"};
    StartupWorldHumanProfile main_character{"冒险太郎", 0, false};
    int slot{};
};
struct StartupRecordView {
    std::string label, village, unit;
    std::int64_t value{};
    int page{};
};
// 只捕获无世界的标题控制器；系统纪录由独立文件验证，世界有自己的快照协议。
struct StartupTitleReplay {
    std::string controller{"startup-title-v1"};
    StartupApplicationMode mode{StartupApplicationMode::logic};
    StartupTitleDraft draft;
    StartupApplicationPage page{StartupApplicationPage::title};
    int record_page{};
    std::vector<int> decorations;
    ref::WorldRandomSnapshot random;
    std::uint64_t requests{};
};
// 应用协调系统文件与世界候选；从未向UI暴露可写世界或任意纪录setter。
class StartupApplication {
  public:
    StartupApplication(StartupApplicationPaths paths, ref::WorldRandomStream random,
                       StartupApplicationMode mode = StartupApplicationMode::logic);
    const std::string &error() const { return error_; }
    const StartupSystemRecords &records() const { return records_; }
    const StartupTitleDraft &draft() const { return draft_; }
    StartupApplicationPage page() const { return page_; }
    const StartupWorldRuntimeSession *world() const { return world_ ? &*world_ : nullptr; }
    const std::vector<int> &decorations() const { return decorations_; }
    const std::optional<StartupClearScorePageState> &clear_page() const { return clear_; }
    const std::optional<StartupClearScoreRows> &clear_rows() const { return clear_rows_; }
    const std::optional<ref::WorldRandomSnapshot> &handoff_random() const { return handoff_; }
    std::string request_new_game(int slot);
    std::string answer_overwrite(bool yes);
    std::string edit_village(std::string name);
    std::string edit_main_name(std::string name);
    std::string edit_sex(int sex);
    std::string cancel_configuration();
    std::string start_game();
    std::string return_to_title();
    std::string open_records(); // 一次显式表现请求；重画record_view不抽取。
    std::string turn_record_page(int direction);
    StartupRecordView record_view() const;
    std::optional<StartupTitleReplay> capture_title_replay() const;
    std::string restore_title_replay(const StartupTitleReplay &snapshot);
    std::string update(bool confirm = false);
    std::string acknowledge_page(std::uint64_t page);
    std::vector<int> take_sound_requests();
    std::string save_world(); // 仅世界文件，不暗含系统保存。
    std::string load_world(int slot);
    // 显式研究入口：沿现有已校验世界回放协议载入，拒绝已推进的raw17。
    std::string load_world_replay(const std::filesystem::path &, const std::string &controller);

  private:
    std::string install_loaded(StartupWorldLoadResult result, int slot);
    std::string commit_world(StartupWorldRuntimeSession candidate, bool save_system = false);
    std::string update_clear(bool confirm);
    StartupApplicationPaths paths_;
    StartupSystemRecords records_;
    StartupTitleDraft draft_;
    ref::WorldRandomStream random_;
    StartupApplicationMode mode_;
    StartupApplicationPage page_{StartupApplicationPage::title};
    int record_page_{};
    std::vector<int> decorations_;
    std::uint64_t requests_{};
    std::optional<ref::WorldRandomSnapshot> handoff_;
    std::optional<StartupWorldRuntimeSession> world_;
    std::optional<StartupClearScoreRows> clear_rows_;
    std::optional<StartupClearScorePageState> clear_;
    std::optional<std::uint64_t> clear_id_;
    std::string error_;
};
} // namespace ark::simulation
