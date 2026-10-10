#pragma once

#include "dungeon_village_prototype/startup_application.hpp"

#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace dungeon_village_prototype {
// 窗口只保留更新回执；完整候选在原Owner提交后释放，不复制给绘制层。
struct StartupWindowUpdate {
    bool committed{};
    StartupWorldRuntimeError error{StartupWorldRuntimeError::none};
    ref::WorldSceneError scene_error{ref::WorldSceneError::none};
    ref::WorldScheduleError world_error{ref::WorldScheduleError::none};
};

// 唯一宿主二选一。应用模式的命令必须经应用事务，不开放可写Session绕过保存桥。
class StartupWindowHost {
  private:
    static StartupWorldRuntimeError checked(const std::string &error) {
        if (!error.empty()) throw std::runtime_error(error);
        return StartupWorldRuntimeError::none;
    }
    StartupWorldRuntimeError normalize(StartupWorldRuntimeError result) { return result; }
    StartupWorldRuntimeError normalize(const std::string &result) {
        last_error_ = result;
        return result.empty() ? StartupWorldRuntimeError::none : StartupWorldRuntimeError::runtime_failed;
    }
    StartupBuildResult normalize(StartupBuildResult result) { return result; }
    StartupBuildResult normalize(const StartupApplicationBuildResult &result) {
        const auto error = normalize(result.error);
        if (error != StartupWorldRuntimeError::none) return {error};
        return {StartupWorldRuntimeError::none, result.denial, result.created};
    }
    StartupWorldTaskPageResult normalize(StartupWorldTaskPageResult result) { return result; }
    StartupWorldTaskPageResult normalize(const StartupApplicationTaskResult &result) {
        const auto error = normalize(result.error);
        if (error != StartupWorldRuntimeError::none) return {error};
        return {StartupWorldRuntimeError::none, result.denial, result.accepted, result.departed};
    }
    template<class Action> auto command(Action &&action) {
        last_error_.clear();
        return std::visit([&](auto &owner) { return normalize(action(owner)); }, owner_);
    }
    template<class Action> void setting(Action &&action) {
        std::visit([&](auto &owner) {
            if constexpr (std::is_void_v<decltype(action(owner))>) action(owner);
            else checked(action(owner));
        }, owner_);
    }
  public:
    explicit StartupWindowHost(StartupWorldRuntimeSession session) : owner_(std::move(session)) {}
    explicit StartupWindowHost(StartupApplication application) : owner_(std::move(application)) {
        const auto &app = std::get<StartupApplication>(owner_);
        if (!app.error().empty()) throw std::runtime_error(app.error());
        if (!app.world() || app.page() != StartupApplicationPage::world)
            throw std::runtime_error("窗口应用宿主需要已进入世界的应用");
    }
    const StartupApplication *application() const { return std::get_if<StartupApplication>(&owner_); }
    // 仅保留最近显式命令的原应用诊断；普通重绘/音频领取不清除它，不属于Owner或存档。
    const std::string &last_error() const { return last_error_; }
    bool application_mode() const { return application() != nullptr; }
    const StartupWorldRuntimeSession &runtime() const {
        return std::visit([](const auto &owner) -> const StartupWorldRuntimeSession & {
            if constexpr (std::is_same_v<std::decay_t<decltype(owner)>, StartupApplication>) {
                if (!owner.world()) throw std::runtime_error("窗口应用世界不可用");
                return *owner.world();
            } else return owner;
        }, owner_);
    }
    const StartupWorldRuntimeState &state() const { return runtime().state(); }
    const StartupWorldRuntimeSession &standalone() const {
        if (application_mode()) throw std::runtime_error("应用宿主不能导出独立可操作Session");
        return std::get<StartupWorldRuntimeSession>(owner_);
    }
    StartupWindowUpdate update(bool clear_confirm = false) {
        return std::visit([clear_confirm](auto &owner) -> StartupWindowUpdate {
            if constexpr (std::is_same_v<std::decay_t<decltype(owner)>, StartupApplication>) {
                if (clear_confirm) {
                    if (!owner.world() || owner.page() != StartupApplicationPage::world)
                        throw std::runtime_error("计分确认需要活动世界raw17");
                    const auto &pages = owner.world()->state().scripts.pages;
                    const auto page = std::find_if(pages.rbegin(), pages.rend(),
                        [](const auto &p) { return p.lifecycle != 4; });
                    if (page == pages.rend() || page->kind != ref::WorldScriptPageKind::raw_page ||
                        page->legacy_page != 17)
                        throw std::runtime_error("计分确认只能提交给活动raw17");
                }
                checked(owner.update(clear_confirm));
                return {true};
            } else {
                if (clear_confirm) throw std::runtime_error("独立Session没有应用计分确认消费者");
                const auto result = owner.update();
                return {result.candidate.has_value(), result.error, result.scene_error, result.world_error};
            }
        }, owner_);
    }
    void set_paused(bool value) { setting([&](auto &owner) { return owner.set_paused(value); }); }
    void set_speed(int value) { setting([&](auto &owner) { return owner.set_speed(value); }); }
    void set_page_confirm_held(bool value) {
        setting([&](auto &owner) { return owner.set_page_confirm_held(value); });
    }
    std::vector<StartupAudioRequest> take_audio_requests() {
        return std::visit([](auto &owner) { return owner.take_audio_requests(); }, owner_);
    }
    std::vector<int> take_sound_requests() {
        return std::visit([](auto &owner) { return owner.take_sound_requests(); }, owner_);
    }
    StartupWorldRuntimeError acknowledge_page(std::uint64_t page) {
        return command([&](auto &owner) { return owner.acknowledge_page(page); });
    }
    StartupWorldRuntimeError cancel_page(std::uint64_t page) {
        return command([&](auto &owner) { return owner.cancel_page(page); });
    }
    StartupWorldRuntimeError act_award_page(std::uint64_t page, ref::WorldAwardAction action, int selection = 0) {
        return command([&](auto &owner) { return owner.act_award_page(page, action, selection); });
    }
    StartupWorldRuntimeError act_rank_page(std::uint64_t page, int selection = 0, bool cancel = false) {
        return command([&](auto &owner) { return owner.act_rank_page(page, selection, cancel); });
    }
    StartupWorldRuntimeError open_task_menu() {
        return command([&](auto &owner) { return owner.open_task_menu(); });
    }
    StartupWorldRuntimeError open_task_control_menu() {
        return command([&](auto &owner) { return owner.open_task_control_menu(); });
    }
    StartupWorldRuntimeError open_village_activities() {
        return command([&](auto &owner) { return owner.open_village_activities(); });
    }
    StartupWorldRuntimeError open_commerce() {
        return command([&](auto &owner) { return owner.open_commerce(); });
    }
    StartupWorldRuntimeError open_information_menu() {
        return command([&](auto &owner) { return owner.open_information_menu(); });
    }
    StartupWorldRuntimeError open_main_menu() {
        return command([&](auto &owner) { return owner.open_main_menu(); });
    }
    StartupWorldRuntimeError open_build_menu() {
        return command([&](auto &owner) { return owner.open_build_menu(); });
    }
    StartupWorldRuntimeError cancel_build() {
        return command([&](auto &owner) { return owner.cancel_build(); });
    }
    StartupWorldRuntimeError cancel_edit() {
        return command([&](auto &owner) { return owner.cancel_edit(); });
    }
    StartupWorldRuntimeError input_menu_page(std::uint64_t page, const StartupWorldMenuInput &input) {
        return command([&](auto &owner) { return owner.input_menu_page(page, input); });
    }
    StartupWorldRuntimeError input_information_page(std::uint64_t page, const StartupInformationInput &input) {
        return command([&](auto &owner) { return owner.input_information_page(page, input); });
    }
    StartupWorldRuntimeError open_magic_pot(StartupMagicPotEntry entry) {
        return command([&](auto &owner) { return owner.open_magic_pot(entry); });
    }
    StartupWorldRuntimeError act_magic_pot_page(std::uint64_t page, StartupMagicPotAction action, int selection = 0) {
        return command([&](auto &owner) { return owner.act_magic_pot_page(page, action, selection); });
    }
    StartupWorldRuntimeError act_commerce_page(std::uint64_t page, StartupCommerceAction action, int selection = 0) {
        return command([&](auto &owner) { return owner.act_commerce_page(page, action, selection); });
    }
    StartupWorldRuntimeError act_facility_item_page(std::uint64_t page, StartupFacilityItemAction action, int selection = -1) {
        return command([&](auto &owner) { return owner.act_facility_item_page(page, action, selection); });
    }
    StartupWorldRuntimeError act_facility_catalog_page(std::uint64_t page, StartupFacilityCatalogAction action, int selection = -1) {
        return command([&](auto &owner) { return owner.act_facility_catalog_page(page, action, selection); });
    }
    StartupWorldRuntimeError act_village_activity_page(std::uint64_t page, StartupVillageActivityAction action, int selection = 0) {
        return command([&](auto &owner) { return owner.act_village_activity_page(page, action, selection); });
    }
    StartupWorldRuntimeError act_human_page(std::uint64_t page, StartupHumanPageAction action, int selection = 0) {
        return command([&](auto &owner) { return owner.act_human_page(page, action, selection); });
    }
    StartupWorldRuntimeError act_tax_page(std::uint64_t page, StartupWorldTaxAction action, int selection = 0) {
        return command([&](auto &owner) { return owner.act_tax_page(page, action, selection); });
    }
    StartupWorldRuntimeError open_human_page(int human) {
        return command([&](auto &owner) { return owner.open_human_page(human); });
    }
    StartupWorldRuntimeError open_facility_page(std::uint64_t facility) {
        return command([&](auto &owner) { return owner.open_facility_page(facility); });
    }
    StartupWorldRuntimeError act_facility_page(std::uint64_t page, StartupFacilityPageAction action) {
        return command([&](auto &owner) { return owner.act_facility_page(page, action); });
    }
    StartupWorldRuntimeError cancel_build_menu(std::uint64_t page) {
        return command([&](auto &owner) { return owner.cancel_build_menu(page); });
    }
    StartupBuildResult begin_build(int definition) {
        return command([&](auto &owner) { return owner.begin_build(definition); });
    }
    StartupBuildResult begin_road(int definition) {
        return command([&](auto &owner) { return owner.begin_road(definition); });
    }
    StartupBuildResult begin_edit(bool move) {
        return command([&](auto &owner) { return owner.begin_edit(move); });
    }
    StartupBuildResult confirm_edit(ref::Position position, ref::FacilityOrientation orientation) {
        return command([&](auto &owner) { return owner.confirm_edit(position, orientation); });
    }
    StartupBuildResult select_build_menu(std::uint64_t page, int definition) {
        return command([&](auto &owner) { return owner.select_build_menu(page, definition); });
    }
    StartupBuildResult confirm_build(ref::Position anchor, ref::FacilityOrientation orientation) {
        return command([&](auto &owner) { return owner.confirm_build(anchor, orientation); });
    }
    StartupBuildResult act_residence_page(std::uint64_t page, int human, bool cancel = false) {
        return command([&](auto &owner) { return owner.act_residence_page(page, human, cancel); });
    }
    StartupWorldTaskPageResult act_task_page(std::uint64_t page, StartupWorldTaskAction action, int selection = 0) {
        return command([&](auto &owner) { return owner.act_task_page(page, action, selection); });
    }

  private:
    std::variant<StartupWorldRuntimeSession, StartupApplication> owner_;
    std::string last_error_;
};
} // namespace dungeon_village_prototype
