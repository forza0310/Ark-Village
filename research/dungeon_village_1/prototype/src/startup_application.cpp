#include "dungeon_village_prototype/startup_application.hpp"
#include "dungeon_village_prototype/startup_world_inheritance.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

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
bool paths_distinct(const StartupApplicationPaths &p) {
    std::array<std::filesystem::path, 3> paths{p.system, p.worlds[0], p.worlds[1]};
    std::error_code ec;
    for (auto &path : paths) {
        if (path.empty() || path.filename().empty()) return false;
        path = std::filesystem::weakly_canonical(path, ec);
        if (ec) return false;
    }
    for (std::size_t i = 0; i < paths.size(); ++i)
        for (std::size_t j = 0; j < i; ++j) {
            if (paths[i] == paths[j]) return false;
#ifdef _WIN32
            // 两个路径尚不存在时equivalent不能识别NTFS默认大小写别名。
            if (CompareStringOrdinal(paths[i].c_str(), -1, paths[j].c_str(), -1, TRUE) == CSTR_EQUAL)
                return false;
#endif
            const bool same = std::filesystem::equivalent(paths[i], paths[j], ec);
            if (!ec && same) return false;
        }
    return true;
}
}
StartupApplication::StartupApplication(StartupApplicationPaths paths, ref::WorldRandomStream random,
                                       StartupApplicationMode mode)
    : paths_(std::move(paths)), random_(std::move(random)), mode_(mode) {
    if (!paths_distinct(paths_) || (mode != StartupApplicationMode::logic &&
                                 mode != StartupApplicationMode::title_presentation)) {
        error_ = "应用路径重叠、无效或回放模式未知";
        return;
    }
    auto loaded = load_startup_system_file(paths_.system);
    if (!loaded.records) { error_ = loaded.error; return; }
    records_ = std::move(*loaded.records);
    draft_.slot = records_.last_slot;
}
std::string StartupApplication::request_new_game(int slot) {
    if (!error_.empty()) return error_;
    if (page_ != Page::title || slot < 0 || slot > 1) return "新局入口或栏位非法";
    std::error_code ec;
    const auto status = std::filesystem::symlink_status(paths_.worlds[slot], ec);
    if (ec && ec != std::errc::no_such_file_or_directory) return "不能检查世界栏位";
    const bool exists = status.type() != std::filesystem::file_type::not_found;
    if (exists && !std::filesystem::is_regular_file(status)) return "世界栏位不是普通文件";
    draft_.slot = slot;
    page_ = exists ? Page::overwrite : Page::configure;
    return {};
}
std::string StartupApplication::answer_overwrite(bool yes) {
    if (page_ != Page::overwrite) return "没有覆盖询问";
    page_ = yes ? Page::configure : Page::title;
    return {};
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
std::string StartupApplication::cancel_configuration() {
    if (page_ != Page::configure) return "当前不是配置页";
    page_ = Page::title; return {}; // 原91取消保留先前编辑。
}
std::string StartupApplication::start_game() {
    if (!error_.empty()) return error_;
    if (!paths_distinct(paths_)) return "应用文件路径已经重叠";
    if (page_ != Page::configure || !valid_text(draft_.village) ||
        !valid_startup_world_human_profile(draft_.main_character)) return "新局配置非法";
    try {
        StartupSession reset;
        StartupWorldRuntimeSession candidate(reset.state(), random_);
        if (!install_startup_world_main_character(candidate.state_, draft_.main_character) ||
            !install_startup_world_inheritance(candidate.state_,
                {records_.facility_levels, records_.profession_status})) return "新局配置或继承拒绝";
        candidate.state_.scripts.village_name = draft_.village;
        // 系统是跨局权威；世界仅在实际现金事务时更新此运行镜像。
        candidate.state_.cash_peak = records_.cash_peak;
        candidate.state_.cash_peak_village = records_.cash_village;
        auto records = records_;
        records.last_slot = draft_.slot;
        auto handoff = random_.snapshot();
        const auto error = save_startup_system_file(paths_.system, records);
        if (!error.empty()) return error;
        records_ = std::move(records);
        world_ = std::move(candidate);
        handoff_ = std::move(handoff);
        clear_.reset(); clear_rows_.reset(); clear_id_.reset(); decorations_.clear();
        page_ = Page::world;
        return {};
    } catch (const std::exception &e) { return e.what(); }
}
std::string StartupApplication::return_to_title() {
    if (!error_.empty()) return error_;
    if (clear_) return "计分页仍在运行";
    if (world_ && page_ == Page::world) {
        auto random = world_->state().scene.random;
        random_ = std::move(random);
    }
    page_ = Page::title; decorations_.clear(); return {};
}
std::string StartupApplication::open_records() {
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
    return record_page_ == 0 ? StartupRecordView{"最高通关点数", records_.score_village, "P", records_.high_score, 0}
        : StartupRecordView{"16年为止的最高资金", records_.cash_village, "G", records_.cash_peak, 1};
}
std::optional<StartupTitleReplay> StartupApplication::capture_title_replay() const {
    if (!error_.empty() || world_ || page_ == Page::world || clear_) return {};
    return StartupTitleReplay{"startup-title-v1", mode_, draft_, page_, record_page_, decorations_,
                              random_.snapshot(), requests_};
}
std::string StartupApplication::restore_title_replay(const StartupTitleReplay &s) {
    if (!error_.empty()) return error_;
    if (world_ || s.controller != "startup-title-v1" || s.mode != mode_ ||
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
    auto draft = s.draft;
    auto decorations = s.decorations;
    auto random_candidate = *random;
    static_assert(std::is_nothrow_move_assignable_v<StartupTitleDraft>);
    static_assert(std::is_nothrow_move_assignable_v<ref::WorldRandomStream>);
    draft_ = std::move(draft); page_ = s.page; record_page_ = s.record_page;
    decorations_ = std::move(decorations); random_ = std::move(random_candidate);
    requests_ = s.requests; return {};
}
std::string StartupApplication::commit_world(StartupWorldRuntimeSession candidate, bool save_system) {
    auto records = records_;
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
    if (save_system) {
        if (!paths_distinct(paths_)) return "应用文件路径已经重叠";
        const auto error = save_startup_system_file(paths_.system, records);
        if (!error.empty()) return error;
    }
    records_ = std::move(records); world_ = std::move(candidate); return {};
}
std::string StartupApplication::update(bool confirm) {
    if (!error_.empty()) return error_;
    if (!world_ || page_ != Page::world) return "当前没有活动世界";
    if (is_clear(top(world_->state()))) return update_clear(confirm);
    if (clear_) return "计分页引用已不匹配";
    if (confirm) {
        const auto *p = top(world_->state());
        return p ? acknowledge_page(p->id) : "世界无页面";
    }
    auto result = prepare_startup_world_runtime(world_->state());
    if (!result.candidate) return runtime_error(result.error).empty() ? "世界候选缺失" : runtime_error(result.error);
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
std::vector<int> StartupApplication::take_sound_requests() {
    return world_ ? world_->take_sound_requests() : std::vector<int>{};
}
std::string StartupApplication::save_world() {
    if (!world_ || page_ != Page::world || clear_) return "当前不能保存世界";
    if (!paths_distinct(paths_)) return "应用文件路径已经重叠";
    return save_startup_world_file(paths_.worlds[draft_.slot], *world_, {}).error;
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
    candidate.state_.cash_peak = records_.cash_peak;
    candidate.state_.cash_peak_village = records_.cash_village;
    world_ = std::move(candidate); draft_.slot = slot; page_ = Page::world;
    clear_.reset(); clear_rows_.reset(); clear_id_.reset(); decorations_.clear(); return {};
}
std::string StartupApplication::load_world(int slot) {
    if (!error_.empty()) return error_;
    if (page_ != Page::title || slot < 0 || slot > 1) return "读档页面或栏位非法";
    return install_loaded(load_startup_world_file(paths_.worlds[slot], startup_world_rules(),
                                                  StartupWorldSavePurpose::normal), slot);
}
std::string StartupApplication::load_world_replay(const std::filesystem::path &path,
                                                  const std::string &controller) {
    if (!error_.empty()) return error_;
    if (page_ != Page::title || controller.empty()) return "回放入口或身份非法";
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
        page->captured_high_score = records_.high_score;
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
    s.sound_requests.push_back(s.active_task && s.task.encounter ? 2 : 1);
    for (int event : {6, result.candidate->new_record ? 4 : 5}) {
        const auto next = ref::prepare_world_script(startup_world_runtime_catalog(),
                                                   startup_world_runtime_scripts(s), {event, {}, {}});
        if (!next.candidate || !write_startup_world_runtime_scripts(s, next.candidate->state))
            return "通关收尾脚本失败";
    }
    s.scripts.executing_page.reset();
    auto records = records_;
    if (result.candidate->new_record) {
        records.high_score = result.candidate->sum; records.score_village = s.scripts.village_name;
        records.trophy = result.candidate->trophy;
    }
    if (!paths_distinct(paths_)) return "应用文件路径已经重叠";
    const auto error = save_startup_system_file(paths_.system, records);
    if (!error.empty()) return error;
    records_ = std::move(records); world_ = std::move(candidate);
    clear_.reset(); clear_rows_.reset(); clear_id_.reset(); return {};
}
} // namespace dungeon_village_prototype
