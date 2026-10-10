#pragma once

#include "dungeon_village_prototype/startup_system_records.hpp"
#include "dungeon_village_prototype/startup_audio.hpp"
#include "dungeon_village_prototype/startup_world_clear_score.hpp"
#include "dungeon_village_prototype/startup_world_profile.hpp"
#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_title_presentation.hpp"
#include "dungeon_village_prototype/startup_title_menu.hpp"
#include "dungeon_village_prototype/startup_application_storage.hpp"
#include <array>
#include <utility>

namespace dungeon_village_prototype {
enum class StartupApplicationMode { logic, title_presentation };
enum class StartupApplicationPage { title, overwrite, configure, records, world };
struct StartupApplicationPaths {
    std::filesystem::path root; // 显式研究存储根；内部固定system与不可变worlds，不接原档路径。
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
    std::string controller{"startup-title-v3"};
    StartupApplicationMode mode{StartupApplicationMode::logic};
    StartupTitleDraft draft;
    StartupApplicationPage page{StartupApplicationPage::title};
    int record_page{};
    std::vector<int> decorations;
    ref::WorldRandomSnapshot random;
    std::uint64_t requests{};
    StartupTitlePresentation title;
    StartupTitleMenuState menu;
    StartupTitleCatalogStamp catalog;
};
struct StartupTitleApplyResult {
    std::string error;
    bool confirm_consumed{}, menu_confirm_ready{};
    std::size_t random_draws{};
};
// 命令回执，不属于存档字段。error表示事务未提交；denial是原业务拒绝，
// 可能已经提交不足资金提示等合法页面，调用者不能据此自动重试付款。
struct StartupApplicationBuildResult {
    std::string error;
    StartupBuildDenial denial{StartupBuildDenial::none};
    std::optional<std::uint64_t> created;
};
struct StartupApplicationTaskResult {
    std::string error;
    ref::TaskCommandDenial denial{ref::TaskCommandDenial::none};
    bool accepted{}, departed{};
};
// 应用协调系统文件与世界候选；从未向UI暴露可写世界或任意纪录setter。
class StartupApplication {
  public:
    StartupApplication(StartupApplicationPaths paths, ref::WorldRandomStream random,
                       StartupApplicationMode mode = StartupApplicationMode::logic);
    const std::string &error() const { return error_; }
    const StartupSystemRecords &records() const { return storage_.records; }
    const StartupTitleDraft &draft() const { return draft_; }
    StartupApplicationPage page() const { return page_; }
    StartupApplicationMode mode() const { return mode_; }
    const StartupWorldRuntimeSession *world() const { return world_ ? &*world_ : nullptr; }
    const std::vector<int> &decorations() const { return decorations_; }
    const std::optional<StartupClearScorePageState> &clear_page() const { return clear_; }
    const std::optional<StartupClearScoreRows> &clear_rows() const { return clear_rows_; }
    const std::optional<ref::WorldRandomSnapshot> &handoff_random() const { return handoff_; }
    const StartupTitlePresentation &title_presentation() const { return title_; }
    const StartupTitleMenuState &title_menu() const { return title_menu_; }
    bool storage_cleanup_pending() const { return cleanup_pending_; }
    bool has_pending_audio_requests() const noexcept { return !audio_requests_.empty(); }
    // 明确接收外部目录的新修订并退休旧标题子页；不自动重试失败的文件意图。
    std::string refresh_title_storage();
    std::uint64_t title_page_id() const { return startup_title_menu_top_id(title_menu_); }
    // 规范输入已获框架准入；控制器、目录意图及必要文件写入在一个候选中共同提交。
    std::string apply_title_request(std::uint64_t expected_page_id,
                                   const StartupTitleMenuRequest &request);
    // 明确的一次背景更新请求，不从Draw/FPS推导，不代替h/j/o菜单路由。
    StartupTitleApplyResult advance_title_background(StartupTitleUpdateRequest request);
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
    // 窗口桥仅提交明确设置，不开放可写Session；同值输入通过资格检查后不复制世界。
    std::string set_paused(bool paused);
    std::string set_speed(int setting);
    std::string set_page_confirm_held(bool held);
    std::string acknowledge_page(std::uint64_t page);
    std::string act_award_page(std::uint64_t page, ref::WorldAwardAction action, int selection = 0);
    std::string return_rank_page(std::uint64_t page);
    std::string leave_commerce_page(std::uint64_t page);
    // 首星/住宅/二星管理命令：复用Session资格与唯一应用提交点，不开放可写world。
    std::string open_build_menu();
    StartupApplicationBuildResult select_build_menu(std::uint64_t page, int definition);
    std::string cancel_build_menu(std::uint64_t page);
    StartupApplicationBuildResult confirm_build(ref::Position anchor,
                                                ref::FacilityOrientation orientation);
    std::string cancel_build();
    StartupApplicationBuildResult begin_build(int definition);
    StartupApplicationBuildResult begin_road(int definition);
    StartupApplicationBuildResult begin_edit(bool move);
    StartupApplicationBuildResult confirm_edit(ref::Position position,
                                               ref::FacilityOrientation orientation);
    std::string cancel_edit();
    std::string open_facility_page(std::uint64_t facility);
    std::string act_facility_page(std::uint64_t page, StartupFacilityPageAction action);
    StartupApplicationBuildResult act_residence_page(std::uint64_t page, int human,
                                                     bool cancel = false);
    std::string open_village_activities();
    std::string open_commerce();
    std::string act_commerce_page(std::uint64_t page, StartupCommerceAction action,
                                   int selection = 0);
    std::string act_facility_item_page(std::uint64_t page, StartupFacilityItemAction action,
                                       int selection = -1);
    std::string act_facility_catalog_page(std::uint64_t page, StartupFacilityCatalogAction action,
                                          int selection = -1);
    std::string act_tax_page(std::uint64_t page, StartupWorldTaxAction action, int selection = 0);
    std::string act_village_activity_page(std::uint64_t page, StartupVillageActivityAction action,
                                          int selection = 0);
    // 魔法壶沿Session两种原入口与41–47载荷提交，不另建应用层壶状态。
    std::string open_magic_pot(StartupMagicPotEntry entry);
    std::string act_magic_pot_page(std::uint64_t page, StartupMagicPotAction action,
                                   int selection = 0);
    std::string open_task_menu();
    std::string open_task_control_menu();
    StartupApplicationTaskResult act_task_page(std::uint64_t page, StartupWorldTaskAction action,
                                               int selection = 0);
    std::string open_human_page(int human);
    std::string open_information_menu();
    std::string open_main_menu();
    std::string input_menu_page(std::uint64_t page, const StartupWorldMenuInput &input);
    std::string input_information_page(std::uint64_t page, const StartupInformationInput &input);
    std::string act_human_page(std::uint64_t page, StartupHumanPageAction action, int selection = 0);
    std::string act_rank_page(std::uint64_t page, int selection = 0, bool cancel = false);
    std::string cancel_page(std::uint64_t page);
    std::vector<StartupAudioRequest> take_audio_requests();
    // 兼容旧ID消费者；同一输出只会被任一领取接口消费一次。
    std::vector<int> take_sound_requests();
    std::string save_world(); // 普通轮末：不可变世界字节与手动目录经单系统提交点发布。
    std::string load_world(int slot, StartupSaveKind kind = StartupSaveKind::manual);
    // 显式研究入口：沿现有已校验世界回放协议载入，拒绝已推进的raw17。
    std::string load_world_replay(const std::filesystem::path &, const std::string &controller);

  private:
    friend struct StartupApplicationReplayAccess;
    // 已校验持久候选专用构造：不得读取外部系统或建立默认纪录。
    struct RestoreTag {};
    StartupApplication(RestoreTag, StartupApplicationPaths paths, ref::WorldRandomStream random,
                       StartupApplicationMode mode)
        : paths_(std::move(paths)), random_(std::move(random)), mode_(mode) {}
    std::string install_loaded(StartupWorldLoadResult result, int slot);
    std::string start_game_candidate();
    std::string open_records_candidate();
    StartupTitleMenuContext title_menu_context() const;
    void sync_title_page();
    std::string commit_world(StartupWorldRuntimeSession candidate, bool save_system = false);
    std::string world_action_error() const;
    template <class Action> std::string apply_world_action(Action &&action);
    std::string update_clear(bool confirm);
    // raw14请求经严格稳定导出及既有单系统发布点处理；结果页不成为普通档的一部分。
    std::string update_save_page(std::uint64_t page);
    StartupApplicationPaths paths_;
    StartupApplicationStorageSnapshot storage_;
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
    StartupTitlePresentation title_;
    StartupTitleMenuState title_menu_;
    std::vector<StartupAudioRequest> audio_requests_;
    bool cleanup_pending_{}; // 存储环境清理债务，精确恢复不继承源环境的待清理文件。
};
} // namespace dungeon_village_prototype
