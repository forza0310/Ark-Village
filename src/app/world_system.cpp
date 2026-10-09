#include "ark/app/world_system.hpp"
#include "ark/simulation/startup_world_inheritance.hpp"

#include <limits>
#include <type_traits>

namespace ark::app {
namespace {
using State = simulation::StartupWorldRuntimeState;
namespace ref = simulation::rules;
const ref::WorldScriptPage *top(const State &state) {
    for (auto p = state.scripts.pages.rbegin(); p != state.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4)
            return &*p;
    return nullptr;
}
std::string path_error(const std::filesystem::path &directory) {
    if (directory.empty())
        return {};
    const auto system = world_system_path(directory);
    for (int slot = 0; slot < 2; ++slot) {
        std::error_code error;
        if (std::filesystem::equivalent(system, world_save_slot_path(directory, slot), error) &&
            !error)
            return "系统纪录路径与世界存档重叠";
    }
    return {};
}
} // namespace
std::filesystem::path world_system_path(const std::filesystem::path &directory) {
    return directory / "system.arksys";
}
simulation::StartupSystemLoadResult read_world_system(const std::filesystem::path &directory) {
    if (directory.empty())
        return {simulation::StartupSystemRecords{}, true, {}};
    const auto error = path_error(directory);
    if (!error.empty())
        return {{}, false, error};
    return simulation::load_startup_system_file(world_system_path(directory));
}
std::string write_world_system(const std::filesystem::path &directory,
                               const simulation::StartupSystemRecords &records) {
    const auto error = path_error(directory);
    if (!error.empty())
        return error;
    if (directory.empty())
        return simulation::validate_startup_system_records(records);
    std::error_code io_error;
    std::filesystem::create_directories(directory, io_error);
    if (io_error)
        return "无法创建系统纪录目录";
    return simulation::save_startup_system_file(world_system_path(directory), records);
}
bool world_clear_page(const State &state) {
    const auto *page = top(state);
    return page && page->kind == ref::WorldScriptPageKind::raw_page && page->legacy_page == 17;
}
std::string advance_world_clear(State &candidate, WorldSystemState &system, bool confirm) {
    const auto *page = top(candidate);
    if (!world_clear_page(candidate) || candidate.scene.framework_paused)
        return "计分页无更新资格";
    if (!system.clear) {
        const auto rows = simulation::startup_world_clear_score(candidate);
        if (!rows.candidate)
            return "通关计分输入无效";
        WorldClearPage clear{*rows.candidate, {}, page->id, 0};
        clear.score.captured_high_score = system.records.high_score;
        system.clear = std::move(clear);
    }
    auto &clear = *system.clear;
    if (clear.page != page->id || clear.animation_counter == std::numeric_limits<int>::max())
        return "计分页引用或计数无效";
    const auto result =
        simulation::prepare_startup_clear_score_page(clear.rows, clear.score, confirm);
    if (!result.candidate)
        return "通关计分阶段无效";
    ++clear.animation_counter;
    clear.score = *result.candidate;
    if (!result.finished_pulse) {
        candidate.page_counters[page->id] = clear.score.counter;
        candidate.page_phases[page->id] = clear.score.stage;
        return {};
    }
    // Same maintained application consumers: retire17, BGM, event6 then strict-high event4/5.
    candidate.scripts.executing_page = page->id;
    const auto closed = ref::prepare_world_script_close_page(
        simulation::startup_world_runtime_scripts(candidate), page->id);
    if (!closed.candidate ||
        !simulation::write_startup_world_runtime_scripts(candidate, closed.candidate->state))
        return "计分页关闭失败";
    candidate.sound_requests.push_back(candidate.active_task && candidate.task.encounter ? 2 : 1);
    for (const int event : {6, clear.score.new_record ? 4 : 5}) {
        const auto next = ref::prepare_world_script(
            simulation::startup_world_runtime_catalog(),
            simulation::startup_world_runtime_scripts(candidate), {event, {}, {}});
        if (!next.candidate ||
            !simulation::write_startup_world_runtime_scripts(candidate, next.candidate->state))
            return "通关收尾脚本失败";
    }
    candidate.scripts.executing_page.reset();
    if (clear.score.new_record) {
        system.records.high_score = clear.score.sum;
        system.records.score_village = candidate.scripts.village_name;
        system.records.trophy = clear.score.trophy;
    }
    system.clear.reset();
    return {};
}
std::string commit_world_system(const std::filesystem::path &directory, const State &before,
                                const State &candidate, WorldSystemState &system,
                                bool clear_finished) {
    bool changed = clear_finished;
    if (candidate.cash_peak > system.records.cash_peak) {
        system.records.cash_peak = candidate.cash_peak;
        system.records.cash_village = candidate.cash_peak_village;
        changed = true;
    }
    if (candidate.system_unlock_data != before.system_unlock_data) {
        system.records.facility_levels = candidate.system_unlock_data[0];
        system.records.profession_status = candidate.system_unlock_data[1];
        changed = true;
    }
    return changed ? write_world_system(directory, system.records) : std::string{};
}
void change_world_draft_sex(WorldNewGameDraft &draft, int sex) {
    if (sex != 0 && sex != 1)
        return;
    if (draft.human.sex != sex && !draft.human.custom_name)
        draft.human.name = sex == 0 ? "冒险太郎" : "冒险花子";
    draft.human.sex = sex;
}
std::string start_world_draft(State &initial, WorldSystemState &system,
                              const WorldNewGameDraft &draft,
                              const std::filesystem::path &directory) {
    if (draft.slot < 0 || draft.slot > 1 ||
        !simulation::valid_startup_world_human_profile({draft.village, 0, false}))
        return "新局配置无效";
    auto candidate = initial;
    if (!simulation::install_startup_world_main_character(candidate, draft.human) ||
        !simulation::install_startup_world_inheritance(
            candidate, {system.records.facility_levels, system.records.profession_status}))
        return "主角配置或继承无效";
    candidate.scripts.village_name = draft.village;
    candidate.cash_peak = system.records.cash_peak;
    candidate.cash_peak_village = system.records.cash_village;
    auto next = system;
    next.records.last_slot = draft.slot;
    next.clear.reset();
    const auto error = write_world_system(directory, next.records);
    if (!error.empty())
        return error;
    static_assert(std::is_nothrow_move_assignable_v<State>);
    static_assert(std::is_nothrow_move_assignable_v<WorldSystemState>);
    initial = std::move(candidate);
    system = std::move(next);
    return {};
}
} // namespace ark::app
