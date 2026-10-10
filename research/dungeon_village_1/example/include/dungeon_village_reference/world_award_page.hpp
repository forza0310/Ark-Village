#pragma once

#include "dungeon_village_reference/world_random.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace dungeon_village_reference {
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
    std::optional<int> pending_award; // 原bV，绑定询问时的在籍定义，不用人物实例ID。
};
enum class WorldAwardAction {
    update,
    request_termination, // 原按钮10，仅开“是/否”确认，不终止。
    confirm_termination,
    reject_termination,
    request_award,
    confirm_award,
    reject_award
};
enum class WorldAwardEffectKind {
    sound,
    event,
    refresh,
    close,
    termination_prompt,
    award_prompt,
    reward
};
struct WorldAwardEffect {
    WorldAwardEffectKind kind{};
    int value{}; // sound3、event22/23；event23参数为当前勋章数。
    std::optional<int> event_argument;
};
enum class WorldAwardError { none, invalid_owner, overflow, unsupported_action };
struct WorldContributionResult {
    WorldAwardError error{WorldAwardError::none};
    std::optional<std::vector<WorldAwardHuman>> candidate;
};
// a/e.l、Steam CharacterData.SetContributeParam：35/87共用的纯贡献重算。
// 保持定义原序、身份和p0贡献；不授勋、不排序。非法群体/溢出无部分候选。
// 按值接收窄投影，Owner可移入私有候选，避免再次复制；lvalue输入保持不变。
WorldContributionResult
prepare_world_human_contributions(std::vector<WorldAwardHuman> humans);
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
// b/g.java5909–6001。请求绑定贡献名单索引；确认扣勋章→奖励→子页→父counter0。
WorldAwardResult prepare_world_award_page(const WorldAwardPageState &state, WorldAwardAction action,
                                          int selection = 0);
struct WorldAwardDisplayState {
    int counter{};
    int phase{};
};
struct WorldAwardDisplayCandidate {
    WorldAwardDisplayState state;
    WorldRandomStream random;
    std::optional<int> event; // 原24/25，Owner实际调用后关闭，并绑定返回页的说话者。
};
std::optional<WorldAwardDisplayCandidate>
prepare_world_award_display(WorldAwardDisplayState state, bool confirm,
                            const WorldRandomStream &random);
struct WorldEffortDisplayCandidate {
    int counter{};
    bool closed{};
};
// raw67只推进全局as[0]的四差额显示；跳过空窗口不改变属性或再次发奖励。
std::optional<WorldEffortDisplayCandidate>
prepare_world_effort_display(int counter, bool confirm, const std::array<int, 4> &deltas);
} // namespace dungeon_village_reference
