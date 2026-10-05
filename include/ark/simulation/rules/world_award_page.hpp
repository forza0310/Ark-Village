#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace ark::simulation::rules {
struct WorldAwardHuman {
    int definition{};
    int presence{};                              // e.p，非零参与；不是当前场景人物实例数。
    std::array<std::int32_t, 3> yearly_totals{}; // e.B，贡献只读取[1]/[2]。
    int contribution{};                          // e.am，不是近期击杀K。
};
// 唯一Owner的临时投影；原年度事件26等待1340后仅开页，页初始化自行j++。
struct WorldAwardPageState {
    std::vector<WorldAwardHuman> humans; // 原bv定义顺序，包含p0定义。
    std::vector<int> ranked_definitions; // 原X，按交换循环而非稳定排序生成。
    int medal_count{};                   // UserData.j，原新局0。
    int page_counter{};                  // g.f124d，由框架推进，不替换为秒。
    bool initialized{};
    bool announced{};           // g.w，首次初始化false；首次更新改true。
    bool termination_pending{}; // g.bU，确认必须来自实际终止询问。
    bool closed{};
};
enum class WorldAwardAction {
    update,
    request_termination, // 原按钮10，仅开“是/否”确认，不终止。
    confirm_termination,
    reject_termination,
    request_award,
    confirm_award
};
enum class WorldAwardEffectKind { sound, event, refresh, close, termination_prompt };
struct WorldAwardEffect {
    WorldAwardEffectKind kind{};
    int value{}; // sound3、event22/23；event23参数为当前勋章数。
    std::optional<int> event_argument;
};
enum class WorldAwardError { none, invalid_owner, overflow, unsupported_action };
struct WorldAwardCandidate {
    WorldAwardPageState state;
    std::vector<WorldAwardEffect> effects; // 必须按顺序由Owner消费，失败不能部分提交。
};
struct WorldAwardResult {
    WorldAwardError error{WorldAwardError::none};
    std::optional<WorldAwardCandidate> candidate;
};
// b/g.java3919–3941、a/e.java395–436；初始化仅一次，零在籍显式拒绝原除零路径。
WorldAwardResult prepare_world_award_page_initialization(const WorldAwardPageState &state);
// b/g.java5909–5984。授予与raw88尚未实装，明确拒绝，不能借普通确认退出。
WorldAwardResult prepare_world_award_page(const WorldAwardPageState &state,
                                          WorldAwardAction action);
} // namespace ark::simulation::rules
