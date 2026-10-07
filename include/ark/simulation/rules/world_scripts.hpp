#pragma once

#include "ark/simulation/rules/facility_economy.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ark::simulation::rules {
using WorldScriptProgram = std::vector<std::vector<int>>;
struct WorldScriptDefinition {
    int id{};
    std::string name;
    int legacy_tag{};
    WorldScriptProgram conditions;
    WorldScriptProgram commands;
};
struct WorldScriptTalk {
    std::string name;
    int speaker_kind{};
    int speaker_definition{-1};
    int legacy_tag{-1};
    std::vector<std::string> paragraphs;
};
struct WorldScriptNews {
    int legacy_tag{};
    std::string title;
    std::string text;
};
struct WorldScriptCatalog {
    std::map<int, WorldScriptDefinition> events;
    // 原av.g/b(int[][])：人气奖励1000+原目录下标、设施E为2000+下标。
    // 不属于aL事件，不增加aM，但等待保存此身份并由同一续体扫描恢复。
    std::map<int, WorldScriptProgram> programs;
    std::vector<WorldScriptTalk> talks;
    std::vector<WorldScriptNews> news;
    std::vector<std::vector<std::pair<int, std::string>>> event_messages;
};
enum class WorldScriptError {
    none,
    invalid_input,
    invalid_catalog,
    missing_event,
    missing_presentation,
    unsupported_opcode,
    numeric_overflow,
    dispatch_limit
};
struct WorldScriptCatalogResult {
    WorldScriptError error{WorldScriptError::none};
    std::optional<WorldScriptCatalog> catalog;
};
// 严格读取已解码UTF-8原表；events按定义ID而非行下标，talk/news按原竖线序。
// 不读取APK或文件，不把语法通过视为所有opcode已有消费者。
WorldScriptCatalogResult parse_world_script_catalog(const std::string &events,
                                                    const std::string &talks,
                                                    const std::string &news,
                                                    const std::string &event_messages = "");

enum class WorldScriptPageKind { scene, dialogue, simple_message, newspaper, raw_page };
struct WorldScriptPage {
    std::uint64_t id{};
    int lifecycle{2}; // 原bB：0初始化、1开始、2更新、3挂起、4关闭待移除。
    WorldScriptPageKind kind{WorldScriptPageKind::scene};
    int legacy_page{}; // b.g.f121a，非框架n()；dialogue0、newspaper15、raw56。
    int source_record{-1};
    std::string replacement;
    std::string title;
    std::vector<std::string> paragraphs;
    int speaker_kind{};
    int speaker_definition{-1};
    int legacy_tag{-1};
    int legacy_r{};
    int legacy_s{};
    int legacy_t{};
    int legacy_f{};
    int legacy_g{};
    int legacy_l{};
    std::vector<int> message_commands;
    std::optional<std::uint64_t> task_identity{};
    std::optional<int> task_definition{};
    std::optional<int> monster_definition{};
    std::optional<int> facility_definition{}; // raw82的原o引用；定义ID，不是场景实例或程序ID。
};
struct WorldScriptContinuation {
    int event{}; // aL身份；恢复本身不增加aM。
    std::size_t next_instruction{};
    int remaining_updates{};
    int legacy_tag{-1};    // 原f148d保留，不当作暂停守卫。
    int saved_context{-1}; // P[9]，恢复时先写回。
    std::string replacement;
    std::optional<std::vector<int>> parameters;
};
struct WorldScriptNotice {
    int message{};
    int counter{};
    int duration{80};
    std::string replacement;
    std::string text;
    std::vector<int> definitions{};        // 特殊消息32的原第三载荷；普通消息保持空。
    std::optional<int> human_definition{}; // 成长消息0的原第四载荷。
    std::optional<std::array<std::array<int, 2>, 4>> attribute_changes{}; // 成长消息0原ap。
};
struct WorldScriptFinance {
    std::int64_t cash{};
    std::int64_t cash_peak{};
    std::string cash_peak_village;
    int legacy_flags14{};
    int month{};
    std::optional<std::string>
        localized_gold_template; // b.d.a("G",int)的h.b结果，未给不能猜数额文本。
    std::array<std::array<std::array<int, 2>, 5>, 12> monthly_totals{};
};
struct WorldScriptUnlockDefinition {
    int status{};
    bool pending_notice{};
    std::string name;
    int extra{-1};      // profession f41f 或human当前job.i投影；各目录分别解释。
    int satisfaction{}; // 仅human.C，非HP/努力度。
};
struct WorldScriptFacilityDefinition {
    int category{};
    int icon{}; // 原o.f81d；opcode40按2/3分演出，独立于f82e/category。
    int level{1};
    FacilityEconomyDefinition economy;
    std::array<std::int32_t, 4> improvements{};
    std::array<std::int32_t, 4> attributes{};
};
struct WorldScriptState {
    std::map<int, int> event_calls;
    std::vector<WorldScriptContinuation> continuations;
    int context{-1};
    std::string village_name;
    int pending_completion{}; // f215e；唯一外层所有者投影，不保留第二套累计值。
    std::vector<std::array<int, 3>> popularity_queue; // I：front插入[10,delta,1]。
    std::vector<WorldScriptPage> pages;
    std::optional<std::uint64_t> executing_page; // 框架j，缺省才取当前栈顶。
    std::uint64_t next_page_id{1};
    bool page_mutations_locked{};                   // 框架l=true时插入为无变化，而非脚本停止。
    bool redraw_requested{};                        // 框架f386d，真实插入/关闭要求重绘。
    std::optional<std::uint64_t> selected_actor;    // bi.f137e：人物。
    std::optional<std::uint64_t> selected_monster;  // bi.f138f：另一Character引用，不是设施/物品。
    std::optional<std::uint64_t> selected_facility; // bi.g：设施实例。
    std::vector<std::uint64_t> human_order;
    int scene_mode{};
    int scene_updates{}; // l.a(mode)重置f103b；不能只赋f102a。
    std::array<std::string, 2> scene_labels{};
    int selection_mode{};
    std::optional<WorldScriptFinance> finance;
    std::vector<WorldScriptNotice> notices;
    std::map<int, WorldScriptUnlockDefinition> activities; // bx/a.c，不是职业bu/a.h。
    std::map<int, WorldScriptUnlockDefinition> humans;
    std::map<int, WorldScriptUnlockDefinition> professions; // bu/a.h，共享职业开放。
    std::map<int, WorldScriptFacilityDefinition> facilities;
    bool human_catalog_complete{};    // 21刷新职业计数需完整bv，不用当前实例名单替代。
    bool facility_catalog_complete{}; // category3共享定义必须全量，不能漏刷。
    std::array<std::int32_t, 10> job_counts{};
    int medal_count{};          // UserData.j，29不使用其原参数。
    int exploration_phase{};    // UserData.x，38/39仅查原固定T/U表。
    std::uint32_t user_flags{}; // UserData.u，36只OR16，非全局任务/场景flags。
};
struct WorldScriptInput {
    int event{};
    std::optional<std::string> replacement; // null采用当前村名，续体保存当时字符串。
    std::optional<std::vector<int>> parameters;
    std::size_t dispatch_limit{100000}; // 维护资源保护，非原作循环上限。
};
struct WorldScriptTrace {
    int event{};
    std::size_t instruction{};
    int opcode{};
};
struct WorldScriptCandidate {
    WorldScriptState state;
    std::vector<WorldScriptTrace> executed;
    std::vector<WorldScriptPage> inserted_pages;
    std::optional<std::uint64_t> last_page;
    bool entered_program{}; // 即使程序为空仍有真实入口；主场景据此走L159。
};
struct WorldScriptResult {
    WorldScriptError error{WorldScriptError::none};
    std::optional<WorldScriptCandidate> candidate;
};
bool world_script_seen(const WorldScriptState &state, int event);
// 与脚本执行共用同一校验及错误顺序；不复制状态、推进续体或产生页面／输出。
WorldScriptError validate_world_script_state(const WorldScriptCatalog &catalog,
                                             const WorldScriptState &state);
// 支持固定200事件实际使用的25种opcode、设施定义程序40及嵌套1；未证opcode显式失败。
// 无隐含去重，调用入口立即aM+1；126等待早于调用者累加完成量。
WorldScriptResult prepare_world_script(const WorldScriptCatalog &catalog,
                                       const WorldScriptState &state,
                                       const WorldScriptInput &input);
WorldScriptResult prepare_world_script_program(const WorldScriptCatalog &catalog,
                                               const WorldScriptState &state,
                                               const WorldScriptInput &input);
// 与原表其他程序共用同一严格数字语法，不重新手拆命令字符串。
std::optional<WorldScriptProgram> parse_world_script_program(const std::string &text);
// 主场景只恢复首个到期项即走L159；其后的项和恢复中新追加项本轮不递减。
// first_only=false仅供明确批量夹具，不是MainScene的实际逻辑轮。
// admitted=false不推进；页面关闭仅标4，不驱动本函数/人气/奖励。
WorldScriptResult prepare_world_script_continuations(const WorldScriptCatalog &catalog,
                                                     const WorldScriptState &state, bool admitted,
                                                     std::size_t dispatch_limit = 100000,
                                                     bool first_only = true);
struct WorldScriptAutomaticInput {
    std::optional<int> year_counter;      // P[1]；判定时+1，不把显示年数当原字段。
    std::optional<int> month_counter;     // P[2]；判定时+1。
    std::optional<int> subperiod_counter; // P[3]月内子周期；判定时+1，不是村级n.k。
    bool admitted{true};                  // 主场景mode0且本逻辑轮有更新资格；外层先判定。
    std::size_t dispatch_limit{100000};
    bool first_only{true}; // 首个触发后L159；false仅用于显式批量研究夹具。
};
// 稀疏ID升序扫描，已见跳过；空条件true，0,0是false而非无条件。
// 首个触发执行到等待点并登记aM，主场景本轮不再扫描续体或世界。
WorldScriptResult prepare_world_script_automatic(const WorldScriptCatalog &catalog,
                                                 const WorldScriptState &state,
                                                 const WorldScriptAutomaticInput &input);
WorldScriptResult prepare_world_script_close_page(const WorldScriptState &state,
                                                  std::uint64_t page);
// 其他已证领域创建动态页时共用真实锚/ID/锁行为，不复制一套append-only页栈。
WorldScriptResult prepare_world_script_page(const WorldScriptState &state,
                                            const WorldScriptPage &page);
struct WorldScriptCameraFocusInput {
    std::uint64_t page{};
    std::array<float, 2> camera{};            // 表现适配的c.a.n，不由人物n格坐标补造。
    std::array<float, 2> previous_camera{};   // c.a.p，同步接受本次位移。
    std::array<float, 2> previous_velocity{}; // bi.w；终止/无怪物分支保留旧值。
    std::optional<std::array<float, 2>> first_monster_cached_view;  // 实时bm[0].u。
    std::optional<std::array<float, 2>> first_task_facility_view{}; // raw57：bq[0].b().f()。
};
struct WorldScriptCameraFocusCandidate {
    WorldScriptState state;
    std::array<float, 2> camera{};
    std::array<float, 2> previous_camera{};
    std::array<float, 2> velocity{};
    bool redraw{true}; // ax.h()仅要求重绘，不等于推进AI。
};
struct WorldScriptCameraFocusResult {
    WorldScriptError error{WorldScriptError::none};
    std::optional<WorldScriptCameraFocusCandidate> candidate;
};
// raw56/57逐更新镜头：第一怪物u或第一任务绑定设施f()，距离映射10..150→5..26，
// 严格distance<step才直接对齐并标关闭；相等仍移动一次、下一更新才关闭。
WorldScriptCameraFocusResult
prepare_world_script_camera_focus(const WorldScriptState &state,
                                  const WorldScriptCameraFocusInput &input);
} // namespace ark::simulation::rules
