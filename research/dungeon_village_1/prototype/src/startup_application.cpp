#include "dungeon_village_prototype/startup_application.hpp"
#include "dungeon_village_prototype/startup_world_inheritance.hpp"
#include "dungeon_village_prototype/startup_world_save.hpp"
#include "dungeon_village_prototype/startup_world_manual.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace dungeon_village_prototype {
static_assert(std::is_nothrow_move_assignable_v<StartupSystemRecords>);
static_assert(std::is_nothrow_move_assignable_v<std::optional<StartupWorldRuntimeSession>>);
static_assert(std::is_nothrow_move_assignable_v<std::optional<ref::WorldRandomSnapshot>>);
namespace {
using Page = StartupApplicationPage;
bool valid_text(const std::string &s) { return valid_startup_world_human_profile({s, 0, false}); }
const ref::WorldScriptPage *top(const StartupWorldRuntimeState &s) {
    for (auto p = s.scripts.pages.rbegin(); p != s.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4) return &*p;
    return nullptr;
}
bool is_clear(const ref::WorldScriptPage *p) {
    return p && p->kind == ref::WorldScriptPageKind::raw_page && p->legacy_page == 17;
}
std::string runtime_error(StartupWorldRuntimeError e) {
    return e == StartupWorldRuntimeError::none ? std::string{} :
           "世界命令拒绝：" + std::to_string(static_cast<int>(e));
}

}
StartupApplication::StartupApplication(StartupApplicationPaths paths, ref::WorldRandomStream random,
                                       StartupApplicationMode mode)
    : paths_(std::move(paths)), random_(std::move(random)), mode_(mode) {
    if (mode != StartupApplicationMode::logic && mode != StartupApplicationMode::title_presentation) {
        error_ = "应用回放模式未知"; return;
    }
    auto loaded = load_startup_application_storage(paths_.root);
    if (!loaded.snapshot) { error_ = loaded.error; return; }
    storage_ = std::move(*loaded.snapshot);
    draft_.slot = storage_.records.last_slot;
    audio_requests_.push_back({StartupAudioOperation::replace_bgm,0});

}
std::string StartupApplication::edit_village(std::string name) {
    if (page_ != Page::configure || !valid_text(name)) return "村名或编辑页面非法";
    draft_.village = std::move(name); return {};
}
std::string StartupApplication::edit_main_name(std::string name) {
    if (page_ != Page::configure || !valid_text(name)) return "姓名或编辑页面非法";
    draft_.main_character.name = std::move(name);
    draft_.main_character.custom_name = true; return {};
}
std::string StartupApplication::edit_sex(int sex) {
    if (page_ != Page::configure || sex < 0 || sex > 1) return "性别或编辑页面非法";
    if (sex != draft_.main_character.sex && !draft_.main_character.custom_name)
        draft_.main_character.name = sex == 0 ? "冒险太郎" : "冒险花子";
    draft_.main_character.sex = sex; return {};
}
std::string StartupApplication::start_game_candidate() {
    if (!error_.empty()) return error_;
    if (!valid_text(draft_.village) ||
        !valid_startup_world_human_profile(draft_.main_character)) return "新局配置非法";
    try {
        StartupSession reset;
        StartupWorldRuntimeSession candidate(reset.state(), random_);
        if (!install_startup_world_main_character(candidate.state_, draft_.main_character) ||
            !install_startup_world_inheritance(candidate.state_,
                {storage_.records.facility_levels, storage_.records.profession_status})) return "新局配置或继承拒绝";
        candidate.state_.scripts.village_name = draft_.village;
        // 系统是跨局权威；世界仅在实际现金事务时更新此运行镜像。
        candidate.state_.cash_peak = storage_.records.cash_peak;
        candidate.state_.cash_peak_village = storage_.records.cash_village;
        auto records = storage_.records;
        records.last_slot = draft_.slot;
        auto handoff = random_.snapshot();
        auto audio = audio_requests_;
        auto initial = candidate.take_audio_requests();
        audio.insert(audio.end(), initial.begin(), initial.end());
        audio.push_back({StartupAudioOperation::replace_bgm,
                        candidate.state_.active_task && candidate.state_.task.encounter ? 2 : 1});
        auto committed = commit_startup_application_records(paths_.root, storage_, records);
        if (!committed.snapshot) return committed.error;
        storage_ = std::move(*committed.snapshot); cleanup_pending_ = committed.cleanup_pending;
        audio_requests_ = std::move(audio);
        world_ = std::move(candidate);
        handoff_ = std::move(handoff);
        clear_.reset(); clear_rows_.reset(); clear_id_.reset(); decorations_.clear();
        title_menu_.save_menu.reset(); title_menu_.confirmation.reset(); title_menu_.external.reset();
        page_ = Page::world;
        return {};
    } catch (const std::exception &e) { return e.what(); }
}
std::string StartupApplication::open_records_candidate() {
    if (!error_.empty()) return error_;
    if (page_ != Page::title || requests_ == std::numeric_limits<std::uint64_t>::max())
        return "纪录请求页面或序号非法";
    auto random = random_;
    std::vector<int> choices;
    if (mode_ == StartupApplicationMode::title_presentation) {
        const auto &rules = startup_world_rules();
        for (const auto &h : rules.humans) {
            int status = h.status;
            if (world_) {
                const auto found = world_->state().human_presence.find(h.identity);
                if (found == world_->state().human_presence.end()) return "装饰人物状态缺失";
                status = found->second;
            }
            if (status == 1) choices.push_back(h.identity);
        }
        for (std::size_t i = 0; i < choices.size(); ++i) {
            const auto draw = random.draw(static_cast<int>(choices.size()));
            if (draw.error != ref::WorldRandomError::none) return "标题随机耗尽";
            std::swap(choices[i], choices[static_cast<std::size_t>(draw.ticket)]);
        }
        if (choices.size() > 5) choices.resize(5);
        if (choices.empty())
            for (const auto &h : rules.humans)
                if (h.flags & 1U) choices.push_back(h.identity);
    }
    decorations_ = std::move(choices); random_ = std::move(random);
    ++requests_; record_page_ = 0; page_ = Page::records; return {};
}
std::string StartupApplication::turn_record_page(int direction) {
    if (page_ != Page::records || (direction != -1 && direction != 1)) return "纪录翻页非法";
    record_page_ = 1 - record_page_; return {};
}
StartupRecordView StartupApplication::record_view() const {
    return record_page_ == 0 ? StartupRecordView{"最高通关点数", storage_.records.score_village, "P", storage_.records.high_score, 0}
        : StartupRecordView{"16年为止的最高资金", storage_.records.cash_village, "G", storage_.records.cash_peak, 1};
}
std::optional<StartupTitleReplay> StartupApplication::capture_title_replay() const {
    if (!error_.empty() || world_ || page_ == Page::world || clear_ || !audio_requests_.empty() ||
        !validate_startup_title_menu(title_menu_).empty()) return {};
    return StartupTitleReplay{"startup-title-v3", mode_, draft_, page_, record_page_, decorations_,
                              random_.snapshot(), requests_, title_, title_menu_, *title_menu_context().catalog};
}
std::string StartupApplication::restore_title_replay(const StartupTitleReplay &s) {
    if (!error_.empty()) return error_;
    if (!validate_startup_title_presentation(s.title).empty() ||
        (s.mode == StartupApplicationMode::logic && !pristine_startup_title_presentation(s.title)))
        return "标题背景状态或逻辑模式不符";
    if (!audio_requests_.empty() || !validate_startup_title_menu(s.menu).empty())
        return "标题待消费输出或菜单状态非法";
    const auto catalog = title_menu_context().catalog;
    if (!catalog || s.catalog.revision != catalog->revision || s.catalog.digest != catalog->digest)
        return "标题内存快照对应的文件目录已经改变";
    Page expected_page = Page::title;
    if (s.menu.external && !s.menu.external->returned)
        expected_page = s.menu.external->kind == StartupTitleExternalPage::records ? Page::records : Page::configure;
    else if (s.menu.confirmation && !s.menu.confirmation->returned) expected_page = Page::overwrite;
    if (s.page != expected_page || (s.menu.save_menu &&
        (s.menu.save_menu->slot != s.draft.slot || !s.menu.save_menu->catalog ||
         s.draft.slot < 0 || s.draft.slot > 1 ||
         storage_.records.save_directory[static_cast<std::size_t>(s.draft.slot)][1].packed_date == -1 ||
         s.menu.save_menu->catalog->revision != catalog->revision || s.menu.save_menu->catalog->digest != catalog->digest)))
        return "标题内存快照的菜单、页面或目录引用不一致";
    if (world_ || s.controller != "startup-title-v3" || s.mode != mode_ ||
        s.page < Page::title || s.page > Page::records || s.record_page < 0 || s.record_page > 1 ||
        s.draft.slot < 0 || s.draft.slot > 1 || !valid_text(s.draft.village) ||
        !valid_startup_world_human_profile(s.draft.main_character)) return "标题快照身份或载荷非法";
    const auto random = ref::WorldRandomStream::from_snapshot(s.random);
    if (!random || s.decorations.size() > startup_world_rules().humans.size()) return "标题随机或名单非法";
    std::vector<int> seen;
    for (int id : s.decorations) {
        if (!startup_world_human_profile(startup_world_rules(), id) ||
            std::find(seen.begin(), seen.end(), id) != seen.end()) return "标题人物引用非法";
        seen.push_back(id);
    }
    if (mode_ == StartupApplicationMode::logic && !s.decorations.empty()) return "逻辑模式不含标题装饰";
    if ((s.page != Page::records && !s.decorations.empty()) ||
        (s.page == Page::records && s.requests == 0)) return "标题快照页面与表现请求不一致";
    if (s.page == Page::records && mode_ == StartupApplicationMode::title_presentation) {
        std::vector<int> pool;
        for (const auto &h : startup_world_rules().humans)
            if (h.status == 1) pool.push_back(h.identity);
        if (pool.empty()) {
            for (const auto &h : startup_world_rules().humans)
                if (h.flags & 1U) pool.push_back(h.identity);
            if (s.decorations != pool) return "标题后备名单不匹配原定义序";
        } else {
            if (s.decorations.size() != std::min<std::size_t>(5, pool.size()) ||
                s.requests > s.random.cursor / pool.size()) return "标题名单数量或请求游标非法";
            for (int id : s.decorations)
                if (std::find(pool.begin(), pool.end(), id) == pool.end()) return "标题装饰人物尚未开放";
        }
    }
    auto menu = s.menu;
    auto draft = s.draft;
    auto decorations = s.decorations;
    auto random_candidate = *random;
    static_assert(std::is_nothrow_move_assignable_v<StartupTitleDraft>);
    static_assert(std::is_nothrow_move_assignable_v<ref::WorldRandomStream>);
    draft_ = std::move(draft); page_ = s.page; record_page_ = s.record_page;
    decorations_ = std::move(decorations); random_ = std::move(random_candidate);
    requests_ = s.requests; title_ = s.title; title_menu_ = std::move(menu); return {};
}
std::string StartupApplication::commit_world(StartupWorldRuntimeSession candidate, bool save_system) {
    auto records = storage_.records;
    const auto &s = candidate.state_;
    // 只观察已完成的Owner现金提交；读档另走install_loaded，不重发纪录。
    if (s.cash_peak > records.cash_peak) {
        records.cash_peak = s.cash_peak; records.cash_village = s.cash_peak_village;
        save_system = true;
    }
    if (world_ && s.system_unlock_data != world_->state().system_unlock_data) {
        records.facility_levels = s.system_unlock_data[0];
        records.profession_status = s.system_unlock_data[1]; save_system = true;
    }
    auto audio = audio_requests_;
    auto output = candidate.take_audio_requests();
    audio.insert(audio.end(), output.begin(), output.end());
    if (save_system) {
        auto committed = commit_startup_application_records(paths_.root, storage_, records);
        if (!committed.snapshot) return committed.error;
        storage_ = std::move(*committed.snapshot); cleanup_pending_ = committed.cleanup_pending;
    }
    world_ = std::move(candidate); audio_requests_ = std::move(audio); return {};

}
std::string StartupApplication::update(bool confirm) {
    if (!error_.empty()) return error_;
    if (!world_ || page_ != Page::world) return "当前没有活动世界";
    if (is_clear(top(world_->state()))) return update_clear(confirm);
    if (clear_) return "计分页引用已不匹配";
    const auto *save_page = top(world_->state());
    if (save_page && save_page->kind == ref::WorldScriptPageKind::raw_page &&
        save_page->legacy_page == 14) {
        const auto view = inspect_startup_world_save_page(world_->state(), save_page->id);
        if (save_page->lifecycle == 2 && !view) return "保存页载荷拒绝";
        if (!confirm && view && view->stage == 1) return update_save_page(save_page->id);
    }
    if (confirm) {
        const auto *p = top(world_->state());
        return p ? acknowledge_page(p->id) : "世界无页面";
    }
    auto result = prepare_startup_world_runtime(world_->state());
    if (!result.candidate) return runtime_error(result.error).empty() ? "世界候选缺失" : runtime_error(result.error);
    // 与Session::update同一轮末消费者；这些缓存还决定下一轮人物/命中声音资格。
    // 只写私有候选，失败不能安装世界、消费随机或触发系统纪录落盘。
    if (!update_startup_world_render_cache(*result.candidate)) return "世界渲染缓存候选拒绝";
    StartupWorldRuntimeSession candidate;
    candidate.state_ = std::move(*result.candidate);
    candidate.checkpoints_ = world_->checkpoints_;
    candidate.checkpoints_.insert(candidate.checkpoints_.end(), result.checkpoints.begin(), result.checkpoints.end());
    return commit_world(std::move(candidate));
}
std::string StartupApplication::acknowledge_page(std::uint64_t id) {
    if (!world_ || page_ != Page::world) return "当前没有活动世界";
    const auto *p = top(world_->state());
    if (!p || p->id != id) return "确认页面不是栈顶";
    if (is_clear(p)) return update_clear(true);
    auto candidate = *world_;
    const auto error = runtime_error(candidate.acknowledge_page(id));
    return error.empty() ? commit_world(std::move(candidate)) : error;
}
// 每次输入只复制一份Session候选。领域拒绝不提交；业务denial保留原合法提示，
// 文件提交失败则保留旧应用、世界、音频与系统目录，不把失败动作伪报成已创建/已接受。
std::string StartupApplication::world_action_error() const {
    if (!error_.empty()) return error_;
    if (!world_ || page_ != Page::world || clear_ || clear_rows_ || clear_id_)
        return "当前没有可操作的普通世界页面";
    const auto *page = top(world_->state());
    if (!page || is_clear(page)) return "管理命令不能绕过计分或缺失页面";
    return {};
}
template <class Action> std::string StartupApplication::apply_world_action(Action &&action) {
    const auto eligibility = world_action_error();
    if (!eligibility.empty()) return eligibility;
    try {
        auto candidate = *world_;
        const auto error = runtime_error(action(candidate));
        return error.empty() ? commit_world(std::move(candidate)) : error;
    } catch (const std::exception &error) {
        return std::string("世界管理候选拒绝：") + error.what();
    }
}
std::string StartupApplication::set_paused(bool paused) {
    const auto eligibility = world_action_error();
    if (!eligibility.empty()) return eligibility;
    if (world_->state().scene.framework_paused == paused) return {};
    return apply_world_action([&](auto &world) {
        world.set_paused(paused); return StartupWorldRuntimeError::none;
    });
}
std::string StartupApplication::input_manual_page(std::uint64_t page, const StartupManualInput &input) {
    return apply_world_action([&](auto &world) { return world.input_manual_page(page, input); });
}
std::string StartupApplication::set_speed(int setting) {
    const auto eligibility = world_action_error();
    if (!eligibility.empty()) return eligibility;
    if (setting != 0 && setting != 1) return "窗口速度设置仅允许0或1";
    if (world_->state().scene.speed_setting == setting) return {};
    return apply_world_action([&](auto &world) {
        world.set_speed(setting); return StartupWorldRuntimeError::none;
    });
}
std::string StartupApplication::set_page_confirm_held(bool held) {
    const auto eligibility = world_action_error();
    if (!eligibility.empty()) return eligibility;
    if (world_->state().page_confirm_held == held) return {};
    return apply_world_action([&](auto &world) {
        world.set_page_confirm_held(held); return StartupWorldRuntimeError::none;
    });
}
std::string StartupApplication::open_build_menu() {
    return apply_world_action([](auto &world) { return world.open_build_menu(); });
}
StartupApplicationBuildResult StartupApplication::select_build_menu(std::uint64_t page,
                                                                    int definition) {
    StartupApplicationBuildResult result;
    result.error = apply_world_action([&](auto &world) {
        const auto r = world.select_build_menu(page, definition);
        result.denial = r.denial; result.created = r.created; return r.error;
    });
    if (!result.error.empty()) { result.denial = StartupBuildDenial::none; result.created.reset(); }
    return result;
}
std::string StartupApplication::cancel_build_menu(std::uint64_t page) {
    return apply_world_action([&](auto &world) { return world.cancel_build_menu(page); });
}
StartupApplicationBuildResult StartupApplication::confirm_build(ref::Position anchor,
                                                                ref::FacilityOrientation orientation) {
    StartupApplicationBuildResult result;
    result.error = apply_world_action([&](auto &world) {
        const auto r = world.confirm_build(anchor, orientation);
        result.denial = r.denial; result.created = r.created; return r.error;
    });
    if (!result.error.empty()) { result.denial = StartupBuildDenial::none; result.created.reset(); }
    return result;
}
std::string StartupApplication::cancel_build() {
    return apply_world_action([](auto &world) { return world.cancel_build(); });
}
StartupApplicationBuildResult StartupApplication::begin_build(int definition) {
    StartupApplicationBuildResult result;
    result.error = apply_world_action([&](auto &world) {
        const auto r = world.begin_build(definition);
        result.denial = r.denial; result.created = r.created; return r.error;
    });
    if (!result.error.empty()) { result.denial = StartupBuildDenial::none; result.created.reset(); }
    return result;
}
StartupApplicationBuildResult StartupApplication::begin_road(int definition) {
    StartupApplicationBuildResult result;
    result.error = apply_world_action([&](auto &world) {
        const auto r = world.begin_road(definition);
        result.denial = r.denial; result.created = r.created; return r.error;
    });
    if (!result.error.empty()) { result.denial = StartupBuildDenial::none; result.created.reset(); }
    return result;
}
StartupApplicationBuildResult StartupApplication::begin_edit(bool move) {
    StartupApplicationBuildResult result;
    result.error = apply_world_action([&](auto &world) {
        const auto r = world.begin_edit(move);
        result.denial = r.denial; result.created = r.created; return r.error;
    });
    if (!result.error.empty()) { result.denial = StartupBuildDenial::none; result.created.reset(); }
    return result;
}
StartupApplicationBuildResult StartupApplication::confirm_edit(ref::Position position,
                                                               ref::FacilityOrientation orientation) {
    StartupApplicationBuildResult result;
    result.error = apply_world_action([&](auto &world) {
        const auto r = world.confirm_edit(position, orientation);
        result.denial = r.denial; result.created = r.created; return r.error;
    });
    if (!result.error.empty()) { result.denial = StartupBuildDenial::none; result.created.reset(); }
    return result;
}
std::string StartupApplication::cancel_edit() {
    return apply_world_action([](auto &world) { return world.cancel_edit(); });
}
std::string StartupApplication::open_facility_page(std::uint64_t facility) {
    return apply_world_action([&](auto &world) { return world.open_facility_page(facility); });
}
std::string StartupApplication::act_facility_page(std::uint64_t page, StartupFacilityPageAction action) {
    return apply_world_action([&](auto &world) { return world.act_facility_page(page, action); });
}
StartupApplicationBuildResult StartupApplication::act_residence_page(std::uint64_t page,
                                                                    int human, bool cancel) {
    StartupApplicationBuildResult result;
    result.error = apply_world_action([&](auto &world) {
        const auto r = world.act_residence_page(page, human, cancel);
        result.denial = r.denial; result.created = r.created; return r.error;
    });
    if (!result.error.empty()) { result.denial = StartupBuildDenial::none; result.created.reset(); }
    return result;
}
std::string StartupApplication::open_village_activities() {
    return apply_world_action([](auto &world) { return world.open_village_activities(); });
}
std::string StartupApplication::open_commerce() {
    return apply_world_action([](auto &world) { return world.open_commerce(); });
}
std::string StartupApplication::act_commerce_page(std::uint64_t page,
                                                 StartupCommerceAction action, int selection) {
    return apply_world_action([&](auto &world) { return world.act_commerce_page(page, action, selection); });
}
std::string StartupApplication::act_facility_item_page(std::uint64_t page,
                                                      StartupFacilityItemAction action, int selection) {
    return apply_world_action([&](auto &world) { return world.act_facility_item_page(page, action, selection); });
}
std::string StartupApplication::act_facility_catalog_page(std::uint64_t page,
                                                         StartupFacilityCatalogAction action, int selection) {
    return apply_world_action([&](auto &world) { return world.act_facility_catalog_page(page, action, selection); });
}
std::string StartupApplication::act_tax_page(std::uint64_t page,
                                            StartupWorldTaxAction action, int selection) {
    return apply_world_action([&](auto &world) { return world.act_tax_page(page, action, selection); });
}
std::string StartupApplication::act_village_activity_page(std::uint64_t page,
                                                          StartupVillageActivityAction action,
                                                          int selection) {
    return apply_world_action([&](auto &world) {
        return world.act_village_activity_page(page, action, selection);
    });
}
std::string StartupApplication::open_task_menu() {
    return apply_world_action([](auto &world) { return world.open_task_menu(); });
}
std::string StartupApplication::open_magic_pot(StartupMagicPotEntry entry) {
    return apply_world_action([&](auto &world) { return world.open_magic_pot(entry); });
}
std::string StartupApplication::act_magic_pot_page(std::uint64_t page,
                                                   StartupMagicPotAction action, int selection) {
    return apply_world_action([&](auto &world) {
        return world.act_magic_pot_page(page, action, selection);
    });
}
std::string StartupApplication::open_task_control_menu() {
    return apply_world_action([](auto &world) { return world.open_task_control_menu(); });
}
StartupApplicationTaskResult StartupApplication::act_task_page(std::uint64_t page,
                                                               StartupWorldTaskAction action,
                                                               int selection) {
    StartupApplicationTaskResult result;
    result.error = apply_world_action([&](auto &world) {
        const auto r = world.act_task_page(page, action, selection);
        result.denial = r.denial; result.accepted = r.accepted; result.departed = r.departed;
        return r.error;
    });
    if (!result.error.empty()) {
        result.denial = ref::TaskCommandDenial::none; result.accepted = result.departed = false;
    }
    return result;
}
std::string StartupApplication::open_human_page(int human) {
    return apply_world_action([&](auto &world) { return world.open_human_page(human); });
}
std::string StartupApplication::open_information_menu() {
    return apply_world_action([&](auto &world) { return world.open_information_menu(); });
}
std::string StartupApplication::open_main_menu() {
    return apply_world_action([](auto &world) { return world.open_main_menu(); });
}
std::string StartupApplication::input_menu_page(std::uint64_t page, const StartupWorldMenuInput &input) {
    return apply_world_action([&](auto &world) { return world.input_menu_page(page, input); });
}
std::string StartupApplication::input_information_page(
    std::uint64_t page, const StartupInformationInput &input) {
    return apply_world_action([&](auto &world) { return world.input_information_page(page, input); });
}
std::string StartupApplication::act_human_page(std::uint64_t page, StartupHumanPageAction action,
                                               int selection) {
    return apply_world_action([&](auto &world) { return world.act_human_page(page, action, selection); });
}
std::string StartupApplication::act_rank_page(std::uint64_t page, int selection, bool cancel) {
    return apply_world_action([&](auto &world) { return world.act_rank_page(page, selection, cancel); });
}
std::string StartupApplication::cancel_page(std::uint64_t page) {
    return apply_world_action([&](auto &world) { return world.cancel_page(page); });
}
std::vector<StartupAudioRequest> StartupApplication::take_audio_requests() {
    std::vector<StartupAudioRequest> result;
    result.swap(audio_requests_); return result;
}
std::vector<int> StartupApplication::take_sound_requests() {
    std::vector<int> result;
    result.reserve(audio_requests_.size());
    const auto requests = take_audio_requests();
    for (const auto &request : requests)
        result.push_back(request.id);
    return result;
}
std::string StartupApplication::save_world() {
    if (!world_ || page_ != Page::world || clear_) return "当前不能保存世界";
    if (!audio_requests_.empty()) return "应用声音输出尚未消费，不能捕获普通存档";
    auto committed = save_startup_application_slot(paths_.root, storage_, storage_.records,
        draft_.slot, StartupSaveKind::manual, *world_);
    if (!committed.snapshot) return committed.error;
    storage_ = std::move(*committed.snapshot); cleanup_pending_ = committed.cleanup_pending;
    return {};
}
std::string StartupApplication::install_loaded(StartupWorldLoadResult loaded, int slot) {
    if (!loaded.snapshot) return loaded.error;
    auto candidate = std::move(loaded.snapshot->session);
    const auto *p = top(candidate.state_);
    if (is_clear(p)) {
        const auto n = candidate.state_.page_counters.find(p->id);
        const auto phase = candidate.state_.page_phases.find(p->id);
        if ((n != candidate.state_.page_counters.end() && n->second != 0) ||
            (phase != candidate.state_.page_phases.end() && phase->second != 0))
            return "世界回放不含已推进的应用计分控制器";
    }
    candidate.state_.cash_peak = storage_.records.cash_peak;
    candidate.state_.cash_peak_village = storage_.records.cash_village;
    auto records = storage_.records;
    records.last_slot = slot;
    auto audio = audio_requests_;
    auto output = candidate.take_audio_requests();
    audio.insert(audio.end(), output.begin(), output.end());
    audio.push_back({StartupAudioOperation::replace_bgm,
                    candidate.state_.active_task && candidate.state_.task.encounter ? 2 : 1});
    auto committed = commit_startup_application_records(paths_.root, storage_, records);
    if (!committed.snapshot) return committed.error;
    storage_ = std::move(*committed.snapshot); cleanup_pending_ = committed.cleanup_pending;
    audio_requests_ = std::move(audio);
    world_ = std::move(candidate); draft_.slot = slot; page_ = Page::world;
    clear_.reset(); clear_rows_.reset(); clear_id_.reset(); decorations_.clear();
    title_menu_.save_menu.reset(); title_menu_.confirmation.reset(); title_menu_.external.reset(); return {};
}
std::string StartupApplication::load_world(int slot, StartupSaveKind kind) {
    if (!error_.empty()) return error_;
    if (page_ != Page::title || slot < 0 || slot > 1) return "读档页面或栏位非法";
    if (title_menu_.save_menu || title_menu_.confirmation || title_menu_.external)
        return "请先消费实际标题子页结果";
    return install_loaded(load_startup_application_slot(paths_.root, storage_, slot, kind), slot);
}
std::string StartupApplication::load_world_replay(const std::filesystem::path &path,
                                                  const std::string &controller) {
    if (!error_.empty()) return error_;
    if (page_ != Page::title || controller.empty()) return "回放入口或身份非法";
    if (title_menu_.save_menu || title_menu_.confirmation || title_menu_.external)
        return "标题子页返回结果尚未消费，不能绕过进入回放";
    return install_loaded(load_startup_world_file(path, startup_world_rules(),
                                                  StartupWorldSavePurpose::replay, controller), draft_.slot);
}
std::string StartupApplication::update_clear(bool confirm) {
    const auto &state = world_->state();
    const auto *p = top(state);
    if (!is_clear(p) || state.scene.framework_paused || (clear_id_ && *clear_id_ != p->id))
        return "计分页无资格或引用失效";
    auto rows = clear_rows_;
    auto page = clear_;
    if (!rows) {
        const auto score = startup_world_clear_score(state);
        if (!score.candidate) return "通关计分输入拒绝";
        rows = score.candidate;
        page = StartupClearScorePageState{};
        page->captured_high_score = storage_.records.high_score;
    }
    const auto result = prepare_startup_clear_score_page(*rows, *page, confirm);
    if (!result.candidate) return "通关演出状态拒绝";
    if (!result.finished_pulse) {
        auto candidate = *world_;
        candidate.state_.page_counters[p->id] = result.candidate->counter;
        candidate.state_.page_phases[p->id] = result.candidate->stage;
        clear_rows_ = std::move(rows); clear_ = result.candidate; clear_id_ = p->id;
        world_ = std::move(candidate);
        return {};
    }
    auto candidate = *world_;
    auto &s = candidate.state_;
    s.scripts.executing_page = p->id;
    const auto closed = ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), p->id);
    if (!closed.candidate || !write_startup_world_runtime_scripts(s, closed.candidate->state))
        return "计分页关闭失败";
    s.sound_requests.push_back({StartupAudioOperation::replace_bgm, s.active_task && s.task.encounter ? 2 : 1});
    for (int event : {6, result.candidate->new_record ? 4 : 5}) {
        const auto next = ref::prepare_world_script(startup_world_runtime_catalog(),
                                                   startup_world_runtime_scripts(s), {event, {}, {}});
        if (!next.candidate || !write_startup_world_runtime_scripts(s, next.candidate->state))
            return "通关收尾脚本失败";
    }
    s.scripts.executing_page.reset();
    auto records = storage_.records;
    if (result.candidate->new_record) {
        records.high_score = result.candidate->sum; records.score_village = s.scripts.village_name;
        records.trophy = result.candidate->trophy;
    }
    auto audio = audio_requests_;
    auto output = candidate.take_audio_requests();
    audio.insert(audio.end(), output.begin(), output.end());
    auto committed = commit_startup_application_records(paths_.root, storage_, records);
    if (!committed.snapshot) return committed.error;
    storage_ = std::move(*committed.snapshot); cleanup_pending_ = committed.cleanup_pending;
    audio_requests_ = std::move(audio); world_ = std::move(candidate);
    clear_.reset(); clear_rows_.reset(); clear_id_.reset(); return {};
}
} // namespace dungeon_village_prototype
