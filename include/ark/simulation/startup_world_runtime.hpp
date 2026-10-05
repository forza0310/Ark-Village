#pragma once

#include "ark/simulation/startup_world_projection.hpp"
#include "ark/simulation/rules/world_award_page.hpp"
#include "ark/simulation/rules/world_exploration.hpp"
#include "ark/simulation/rules/world_runtime.hpp"

#include <limits>
#include <memory>

namespace ark::simulation {
struct StartupWorldHumanCalendar {
    int absent_months{};
    std::array<int, 3> yearly_totals{}; // B2在调用点投影world.human_spending。
    int legacy_F{};
    int legacy_G{};
    int continuation_cost{200}; // a/e.n()的真实初值。
    int contribution{};         // e.am，由真实B[1]/B[2]重算。
};
struct StartupWorldFocusActor {
    ref::BattleActorRecord actor = [] {
        ref::BattleActorRecord value;
        value.id = {std::numeric_limits<std::uint64_t>::max()};
        value.legacy_id = -1;
        value.control.flags = 2U;
        return value;
    }();
    ref::RewardActorContext perception;
    ref::RescueActorContext routes = [] {
        ref::RescueActorContext value;
        value.destination = ref::Position{};
        return value;
    }();
    StartupWorldActorMetadata metadata{0, 3, 0, {}}; // n.J建W后仅f()写ad3。
};
// 唯一持久世界：所有route/finish/report/script值都是本次调用的投影，不保存第二份world。
struct StartupWorldRuntimeState {
    ref::WorldSceneState scene;
    const StartupWorldRules *rules{};
    ref::WorldEventTask task;
    std::map<int, ref::ShopHumanRecord> shop_humans;
    std::map<ref::CharacterId, ref::ShopActorRecord> shop_actors;
    std::map<int, ref::ObjectCatalogRecord> items;
    std::map<std::uint64_t, ref::DungeonFacilityProgress> dungeon_facilities;
    std::map<ref::CharacterId, ref::DungeonActorProgress> dungeon_actors;
    std::map<std::pair<int, int>, ref::ObjectCatalogRecord> catalog;
    std::map<std::uint64_t, ref::ObjectShopRecord>
        shops; // 持久仅category；notices从facility_details投影。
    std::vector<std::uint64_t> shop_order;
    int item_rewards{};
    std::map<int, int> human_definition_state;
    std::map<int, std::array<int, 4>> human_homes;
    std::map<int, int> human_presence;
    std::map<int, std::uint32_t> human_flags; // e.o，住宅希望/季度等动态标志由Owner保存。
    std::map<int, StartupWorldHumanCalendar> human_calendar;
    std::map<ref::CharacterId, StartupWorldActorMetadata> actor_metadata;
    std::map<std::uint64_t, int> facility_original_ids;
    std::map<std::uint64_t, int> facility_ordinals;
    std::map<std::uint64_t, int> facility_residents;
    std::map<std::uint64_t, int> facility_difficulties; // Tenant.l，新局0。
    std::map<std::uint64_t, std::array<int, 3>> neighbourhood;
    std::map<std::uint64_t, ref::WorldMapNeighbourCache> neighbourhood_details;
    std::map<std::uint64_t, ref::WorldExplorationSummary> exploration_summaries;
    std::vector<ref::WorldExplorationGroundDisplay> exploration_displays;
    std::vector<ref::DungeonCrewRequest> dungeon_labels; // 231的真实x参数/actor待表现消费。
    std::vector<ref::DungeonFinishSurface> surface;
    std::vector<std::array<bool, 2>> road_patches;
    // scripts不持久finance、pending_completion、I、human_order或scene计数；读写时投影。
    ref::WorldScriptState scripts;
    std::int64_t cash_peak{};
    std::string cash_peak_village;
    std::array<std::array<std::array<int, 2>, 5>, 12> monthly_cash{};
    std::map<std::uint64_t, std::array<std::array<int, 2>, 12>> facility_monthly_cash;
    std::map<std::uint64_t, int> facility_month_age;
    std::map<int, ref::CalendarMaintenanceShopItem> shop_item_stock;
    int report_state{};
    int report_counter{};
    std::array<int, 6> report_snapshot{};
    std::array<int, 6> report_records{};
    std::array<bool, 6> report_new_records{};
    std::array<int, 5> report_portraits{}; // UserData.J的new int[5]真实零初值。
    std::vector<int> report_kills;
    int maximum_income{};
    int village_points{};
    int popularity{};
    int maximum_popularity{};
    bool popularity_display{};
    std::vector<std::array<int, 3>> popularity_pulses;
    std::vector<ref::WorldPopularityReward> popularity_rewards;
    std::map<int, ref::WorldFacilityUpdateDefinition> facility_definitions;
    std::map<int, int> facility_presence;
    std::map<int, bool> residence_catalog_available;  // o.M由b(true)重算，不是O普通建设开放。
    std::map<std::uint64_t, int> page_human_bindings; // 对话/70/94等真实gVar.m绑定。
    int residence_hint_counter{};                     // UserData.H，新局0。
    std::map<int, std::uint32_t> activity_flags;
    std::map<std::uint64_t, ref::WorldFacilityUpdateDetails> facility_details;
    ref::DungeonTaskSuccessState task_progress;
    std::map<std::uint64_t, ref::DungeonFinishTask> tasks;
    std::vector<std::uint64_t> task_order;
    std::map<std::uint64_t, int> task_original_ids;
    int task_sequence{};                          // static c/k.m，新局0。
    std::uint64_t next_facility_identity{1};      // 维护ID，首次跳过已装入8实例后递增。
    std::uint64_t next_task_identity{1};          // 与原task_sequence/rawID分开。
    std::vector<int> task_replay_order;           // a/m.x，新局空。
    int task_special_selection{};                 // a/m.v，新局0。
    int task_special_selection_index{};           // a/m.w，新局0。
    std::vector<int> task_special_selection_list; // a/m.u，新局空。
    int fence_level{};                            // static n.o，J明确写0。
    std::vector<int> base_variants;               // 原h.i读入数组，不能用显示variant补造。
    std::optional<std::uint64_t> active_task;
    std::vector<int> participants;
    std::map<std::uint64_t, std::vector<int>>
        crew_summaries; // 页31初始化X；H/I只读唯一battle.humans。
    std::map<std::uint64_t, ref::DungeonFinishSite> sites;
    int ground_definition{};
    int special_ground_definition{};
    int arrival_counter{};
    int camera_delay{};  // MainScene.h，影响场后到访资格，不暂停共同AI。
    int camera_follow{}; // MainScene.i。
    int event89_count{};
    int entry_updates{};                                // UserData.V，新局0。
    int global_updates{};                               // static n.aK，新局0。
    std::array<float, 2> camera{};                      // c.a.n：表现坐标，不能与地图格混用。
    std::array<float, 2> previous_camera{};             // c.a.p。
    std::array<float, 2> camera_velocity{};             // bi.w。
    int build_mode{};                                   // a/o.aa，0..7；不是MainScene状态。
    int build_feedback_counter{};                       // a/o.ae，反馈原20tick，state1才递减。
    std::string build_feedback_message;                 // a/o.ad，新静态对象为空。
    StartupWorldFocusActor focus_actor;                 // 原W；从不持久进入bl/bm，UID=-1。
    std::uint32_t focus_held_input{};                   // 原方向held位，不是玩家控制真实冒险者。
    std::vector<ref::ActorEffectRecord> visual_effects; // d.a.X变长载荷，现金浮标7项不得截成5项。
    std::vector<std::array<int, 4>> delayed_effects;    // d.a.Y。
    std::vector<std::array<int, 3>> floating_labels;    // d.a.S的计数投影。
    std::vector<std::array<int, 2>> global_effects;     // n.bu。
    std::vector<int> sound_requests;                    // 原c(sound)输出，不把声音变成领域状态。
    std::array<int, 4> reference_viewport{{0, 23, 240, 297}}; // 明确240×320研究画布的b.c.m/n/o/p。
    std::map<std::uint64_t, int> page_counters; // b.g.f124d；主场景冻结时独立推进栈顶页。
    std::map<std::uint64_t, int> page_phases;   // b.g.i，成果页30两段展示不重复奖励。
    std::map<std::uint64_t, std::vector<int>> award_rankings; // raw87初始化X，定义身份。
    std::map<std::uint64_t, bool> award_announced;
    std::map<std::uint64_t, bool> award_termination_pending;
    int medal_count{};                           // UserData.j，c/n.J新局0，raw87初始化+1。
    std::map<int, int> facility_free_builds;     // br.H真实新对象0。
    std::map<int, int> facility_unlock_counters; // br.q真实新对象0。
    std::map<int, bool> facility_unlock_notices; // br.r按当前定义维护。
    bool confirm_input{};                        // 桌面输入一次性快照；不是自动确认。
    bool cancel_input{};
    bool menu_input{};
    int rank{};             // UserData.k，新局0。
    int quarter_counter{3}; // UserData.q，J明确写3。
    int legacy_D{};
    std::array<std::int32_t, 13> legacy_n{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0}};
    std::vector<std::array<std::int32_t, 10>> yearly_statistics{30}; // UserData.C30×10。
    int events_held{};                                               // UserData.F。
    int task_subperiods{};                                           // UserData.y。
    bool generation_retry{};                                         // MainScene.Y。
    std::optional<std::uint64_t> deadline_page;                      // MainScene.aI。
    int completion_mode{};                                           // P14。
    int system_completion_mode{};                                    // J9。
    int save_marker{};                                               // P15，av.a(true)先置1。
    std::array<std::vector<std::uint8_t>, 2> system_unlock_data;
    std::array<bool, 4> rank_met{};
    std::array<int, 4> rank_values{};
    std::uint64_t simulation_steps{};
    int clock_parameter{80};  // d/a.Q的固定新局输入。
    int calendar_advance{27}; // d/a.R的固定新局输入。
};

ref::WorldActorRoutesState startup_world_runtime_routes(const StartupWorldRuntimeState &state);
const ref::WorldScriptCatalog &startup_world_runtime_catalog();
bool write_startup_world_runtime_routes(StartupWorldRuntimeState &state,
                                        const ref::WorldActorRoutesState &routes);
ref::WorldScriptState startup_world_runtime_scripts(const StartupWorldRuntimeState &state);
bool write_startup_world_runtime_scripts(StartupWorldRuntimeState &state,
                                         const ref::WorldScriptState &scripts);
ref::DungeonFinishState startup_world_runtime_finish(const StartupWorldRuntimeState &state);
bool write_startup_world_runtime_finish(StartupWorldRuntimeState &state,
                                        const ref::DungeonFinishState &finish);

enum class StartupWorldRuntimeError {
    none,
    invalid_initial_state,
    missing_source,
    runtime_failed,
    invalid_page,
    script_failed
};
StartupWorldRuntimeError acknowledge_startup_world_runtime_page(StartupWorldRuntimeState &state,
                                                                std::uint64_t page);
// 授勋的显式测试输入按独立页面更新消费；普通确认不隐式选择终止，授予/raw88仍拒绝。
StartupWorldRuntimeError act_startup_world_runtime_award_page(StartupWorldRuntimeState &state,
                                                              std::uint64_t page,
                                                              ref::WorldAwardAction action);
bool refresh_startup_world_runtime_rank(StartupWorldRuntimeState &state);
std::optional<StartupWorldRuntimeState>
update_startup_world_runtime_page(const StartupWorldRuntimeState &state);
struct StartupWorldRuntimeResult {
    StartupWorldRuntimeError error{StartupWorldRuntimeError::none};
    std::optional<StartupWorldRuntimeState> candidate;
    ref::WorldSceneError scene_error{ref::WorldSceneError::none};
    ref::WorldScheduleError world_error{ref::WorldScheduleError::none};
    std::vector<std::shared_ptr<const StartupWorldRuntimeState>> checkpoints; // 独立不可变审计。
};
// 附加真实消费者由routes提供；公开构造不隐式注入演示策略或自动关闭页面。
ref::WorldRuntimeAdapter<StartupWorldRuntimeState> startup_world_runtime_adapter();
// 原a(2)先n.f：只在进入时把镜头逆投影到W.n，不能每帧覆盖运动。
bool enter_startup_world_focus(StartupWorldRuntimeState &state);
std::optional<StartupWorldRuntimeState>
advance_startup_world_focus(const StartupWorldRuntimeState &state,
                            const ref::WorldRuntimeAdapter<StartupWorldRuntimeState> &adapter);
void configure_startup_world_runtime_calendar_adapter(
    ref::WorldRuntimeAdapter<StartupWorldRuntimeState> &adapter);
void configure_startup_world_runtime_arrival_adapter(
    ref::WorldRuntimeAdapter<StartupWorldRuntimeState> &adapter);
// c/m.f()：实际绑定占地首格与形状决定镜头中心，场景与raw57共用。
std::optional<std::array<float, 2>>
startup_world_runtime_facility_target(const StartupWorldRuntimeState &state, std::uint64_t id);
void configure_startup_world_runtime_scene_adapter(
    ref::WorldRuntimeAdapter<StartupWorldRuntimeState> &adapter);
void configure_startup_world_runtime_nonactor_adapter(
    ref::WorldRuntimeAdapter<StartupWorldRuntimeState> &adapter);
ref::Position startup_world_raw_projection(ref::CombatPoint position);
ref::Position startup_world_view_projection(const StartupWorldRuntimeState &state,
                                            ref::CombatPoint position);
std::optional<bool> startup_world_actor_visible(const StartupWorldRuntimeState &state,
                                                ref::CharacterId actor);
std::optional<bool> startup_world_hit_sound_visible(const StartupWorldRuntimeState &state,
                                                    ref::CharacterId actor);
bool update_startup_world_render_cache(
    StartupWorldRuntimeState &state); // 每框架frame一次，非每世界轮。
bool emit_startup_world_actor_sound(StartupWorldRuntimeState &state, ref::CharacterId actor,
                                    int sound); // n.a(sound,旧bm)原二次裁剪。
std::optional<int> startup_world_actor_direction(const StartupWorldRuntimeState &state,
                                                 ref::CharacterId actor, ref::CharacterId target);
std::optional<ref::DropSelectionInput>
startup_world_drop_selection(const StartupWorldRuntimeState &state, ref::CharacterId actor);
StartupWorldRuntimeResult prepare_startup_world_runtime(const StartupWorldRuntimeState &state);

class StartupWorldRuntimeSession {
  public:
    StartupWorldRuntimeSession(const StartupState &startup, ref::WorldRandomStream random);
    const StartupWorldRuntimeState &state() const;
    StartupWorldRuntimeResult update(); // 一次框架b入口，内部保留原1/2轮与日历27。
    void set_paused(bool paused);
    void set_speed(int setting); // 只恰1双轮；不是人物位移乘二。
    StartupWorldRuntimeError acknowledge_page(std::uint64_t page);
    StartupWorldRuntimeError act_award_page(std::uint64_t page, ref::WorldAwardAction action);
    const std::vector<std::shared_ptr<const StartupWorldRuntimeState>> &checkpoints() const;

  private:
    StartupWorldRuntimeState state_;
    std::vector<std::shared_ptr<const StartupWorldRuntimeState>> checkpoints_; // 不嵌入Owner。
};
} // namespace ark::simulation
