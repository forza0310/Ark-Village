#include "dungeon_village_prototype/startup_application.hpp"
#include "dungeon_village_prototype/startup_application_replay.hpp"
#include "dungeon_village_prototype/startup_world_village_activity.hpp"
#include "dungeon_village_prototype/startup_world_magic_pot.hpp"
#include "dungeon_village_prototype/startup_world_information.hpp"
#include "dungeon_village_prototype/startup_world_menu.hpp"
#include "dungeon_village_prototype/startup_world_manual.hpp"
#include "dungeon_village_prototype/startup_world_save.hpp"
#include "startup_application_natural_replay.hpp"
#include "../src/startup_window_host.hpp"
#include "support/audio_requests.hpp"
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <algorithm>
#include <tuple>
#include <type_traits>

using namespace dungeon_village_prototype;
std::array<std::filesystem::path, 4> create_application_action_entry_fixtures(
    const std::filesystem::path &);
std::filesystem::path create_application_clear_entry_fixture(const std::filesystem::path &);
namespace {
int checks{};
bool audio_ownership_checked{};
void require(bool ok, const char *message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
void good(const std::string &error) {
    ++checks;
    if (!error.empty()) throw std::runtime_error(error);
}
struct OwnedDirectory {
    std::filesystem::path path;
    explicit OwnedDirectory(std::filesystem::path value) : path(std::move(value)) {
        require(std::filesystem::create_directory(path), "exclusive action bridge fixtures");
    }
    ~OwnedDirectory() { std::error_code error; std::filesystem::remove_all(path, error); }
};
const ref::WorldScriptPage *top(const StartupWorldRuntimeSession &world) {
    const auto &pages = world.state().scripts.pages;
    for (auto page = pages.rbegin(); page != pages.rend(); ++page)
        if (page->lifecycle != 4) return &*page;
    return nullptr;
}
StartupApplicationPaths paths(const std::filesystem::path &root, const std::string &name) {
    const auto storage = root / name;
    require(std::filesystem::create_directory(storage), "exclusive application storage root");
    return {storage};
}
std::string bytes(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}
bool no_world_blobs(const StartupApplicationPaths &files) {
    const auto worlds = files.root / "worlds";
    return !std::filesystem::exists(worlds) || std::filesystem::is_empty(worlds);
}
void audio_ownership(const StartupApplication &source, const StartupWorldRuntimeSession &independent) {
    // 条件动作夹具的真实Owner出口：复制待消费输出验证双接口，不注入声音，也不代表自然路径。
    auto typed_app = source;
    auto legacy_app = source;
    auto typed_session = independent;
    auto legacy_session = independent;
    const auto expected = independent.state().sound_requests;
    const auto ids = test_support::audio_ids(expected);
    require(!expected.empty(), "双接口领取使用非空真实Owner动作输出");
    require(source.world()->state().sound_requests.empty(),
            "应用世界输出已经移交，不能从Session副本再次领取同一请求");
    require(typed_app.take_audio_requests() == expected && typed_app.take_sound_requests().empty() &&
                typed_app.take_audio_requests().empty(),
            "应用typed先领取，旧ID和typed再领取均为空，不重播同一队列");
    require(legacy_app.take_sound_requests() == ids && legacy_app.take_audio_requests().empty(),
            "应用旧ID先领取保留顺序，typed不能再领取");
    require(typed_session.take_audio_requests() == expected && typed_session.take_sound_requests().empty(),
            "Session完整保留操作和ID，typed与旧接口互相消费");
    require(legacy_session.take_sound_requests() == ids && legacy_session.take_audio_requests().empty(),
            "Session旧接口只显式投影ID，没有第二份typed队列");
}
// 此层只核应用接线与事务：完整年度计算继续由原授勋套件主责。
void compare_command(StartupApplication &app, StartupWorldRuntimeSession &expected,
                     const std::string &error, StartupWorldRuntimeError expected_error) {
    good(error);
    require(expected_error == StartupWorldRuntimeError::none, "reference command accepted");
    require(app.world()->state().sound_requests.empty(), "committed application world transfers current outputs once");
    if (!expected.state().sound_requests.empty())
        checks += run_startup_application_natural_pending_sound_check(app);
    if (!audio_ownership_checked && !expected.state().sound_requests.empty()) {
        audio_ownership(app, expected);
        audio_ownership_checked = true;
    }
    auto typed_app = app;
    auto typed_expected = expected;
    require(typed_app.take_audio_requests() == typed_expected.take_audio_requests(),
            "application bridge preserves source audio operations as well as ordered IDs");
    require(app.take_sound_requests() == expected.take_sound_requests(),
            "application bridge emits original ordered sounds once");
    require(app.take_sound_requests().empty() && app.take_audio_requests().empty(),
            "application output sink consumed once across both interfaces");
    require(startup_world_session_digest(*app.world()) == startup_world_session_digest(expected),
            "after identical output consumption application preserves full Session, history, order and random");
}
// 摘要要求输出已消费；只消费副本，另行逐项比较原应用仍待领取的真实输出。
std::pair<std::string, std::vector<StartupAudioRequest>> management_observation(
    const StartupApplication &source) {
    auto copy=source;
    auto sounds=copy.take_audio_requests();
    StartupApplicationReplayMetadata metadata;
    metadata.controller_id="management-bridge-observation-v1";
    metadata.controller_state={1};
    const auto validate=[](const StartupApplication &, const StartupApplicationReplayMetadata &m) {
        return m.controller_id=="management-bridge-observation-v1" &&
               m.controller_state==std::vector<std::uint8_t>{1} && m.next_frame==0 &&
               m.next_command==0 && m.extensions.empty()
                   ? std::string{} : std::string{"bad fixed management observation metadata"};
    };
    return {startup_application_replay_digest(copy,metadata,validate),std::move(sounds)};
}
// 应用接线使用真实新局与当前原表。只做短管理事务，不补人物、资金、活动次数或任务。
void management_bridges(const std::filesystem::path &root) {
    const auto files=paths(root,"management");
    StartupApplication app(files,ref::WorldRandomStream::from_java_seed(1));
    const auto rejected=[&](const auto &action) {
        const auto before=management_observation(app);
        const auto system=bytes(files.root/"system.avr");
        require(action(),"invalid management command returns an explicit error");
        require(management_observation(app)==before && bytes(files.root/"system.avr")==system &&
                    no_world_blobs(files),
                "rejected management command retains full application random audio and storage");
    };
    rejected([&]{return !app.open_build_menu().empty();});
    rejected([&]{return !app.open_village_activities().empty();});
    rejected([&]{return !app.open_magic_pot(StartupMagicPotEntry::main_menu).empty();});
    rejected([&]{return !app.act_magic_pot_page(1,StartupMagicPotAction::confirm).empty();});
    rejected([&]{return !app.open_task_menu().empty();});
    rejected([&]{return !app.open_information_menu().empty();});
    rejected([&]{return !app.open_main_menu().empty();});
    rejected([&]{return !app.set_paused(false).empty();});
    rejected([&]{return !app.set_speed(0).empty();});
    rejected([&]{return !app.set_page_confirm_held(false).empty();});
    good(app.request_new_game(0)); good(app.start_game());
    require(app.take_audio_requests()==std::vector<StartupAudioRequest>{
                {StartupAudioOperation::replace_bgm,0},{StartupAudioOperation::replace_bgm,1}},
            "real new management world consumes actual B0 and G activation outputs");
    const auto system=bytes(files.root/"system.avr");
    rejected([&]{return !app.open_magic_pot(StartupMagicPotEntry::main_menu).empty();});
    const auto directory=app.records().save_directory;
    auto expected=*app.world();
    rejected([&]{return !app.set_speed(-1).empty();});
    rejected([&]{return !app.set_speed(2).empty();});
    const auto same_settings=management_observation(app);
    good(app.set_paused(app.world()->state().scene.framework_paused));
    good(app.set_speed(app.world()->state().scene.speed_setting));
    good(app.set_page_confirm_held(app.world()->state().page_confirm_held));
    require(management_observation(app)==same_settings,
            "same eligible window settings preserve complete application digest and outputs");
    expected.set_paused(true);
    compare_command(app,expected,app.set_paused(true),StartupWorldRuntimeError::none);
    expected.set_speed(1);
    compare_command(app,expected,app.set_speed(1),StartupWorldRuntimeError::none);
    expected.set_page_confirm_held(true);
    compare_command(app,expected,app.set_page_confirm_held(true),StartupWorldRuntimeError::none);
    expected.set_page_confirm_held(false);
    compare_command(app,expected,app.set_page_confirm_held(false),StartupWorldRuntimeError::none);
    expected.set_speed(0);
    compare_command(app,expected,app.set_speed(0),StartupWorldRuntimeError::none);
    expected.set_paused(false);
    compare_command(app,expected,app.set_paused(false),StartupWorldRuntimeError::none);
    const auto stale=app.world()->state().scripts.next_page_id;
    rejected([&]{return !app.act_commerce_page(stale,static_cast<StartupCommerceAction>(0)).empty();});
    rejected([&]{return !app.act_facility_item_page(stale,static_cast<StartupFacilityItemAction>(0)).empty();});
    rejected([&]{return !app.act_facility_catalog_page(stale,static_cast<StartupFacilityCatalogAction>(0)).empty();});
    rejected([&]{return !app.act_tax_page(stale,static_cast<StartupWorldTaxAction>(0)).empty();});
    const auto editing_expected=expected.begin_edit(false);
    const auto editing=app.begin_edit(false);
    require(editing.denial==editing_expected.denial && editing.created==editing_expected.created,
            "new edit bridge retains structured receipt from independent Session");
    compare_command(app,expected,editing.error,editing_expected.error);
    const auto unchanged_edit=management_observation(app);
    const auto rejected_edit_expected=expected.confirm_edit({-1,-1},ref::FacilityOrientation::first);
    const auto rejected_edit=app.confirm_edit({-1,-1},ref::FacilityOrientation::first);
    require(rejected_edit.denial==StartupBuildDenial::outside_map &&
                rejected_edit.denial==rejected_edit_expected.denial && !rejected_edit.created,
            "new edit bridge preserves source business denial after entering edit mode");
    compare_command(app,expected,rejected_edit.error,rejected_edit_expected.error);
    require(management_observation(app)==unchanged_edit,
            "rejected edit confirmation leaves complete entered mode and storage unchanged");
    const auto cancelled_edit=expected.cancel_edit();
    compare_command(app,expected,app.cancel_edit(),cancelled_edit);
    auto error=expected.open_information_menu();
    compare_command(app,expected,app.open_information_menu(),error);
    const auto information_menu=top(*app.world())->id;
    StartupInformationInput info_input;
    info_input.select_row=2;
    rejected([&]{return !app.input_information_page(information_menu,info_input).empty();});
    auto info_tick=expected.update();
    compare_command(app,expected,app.update(),info_tick.error);
    error=expected.input_information_page(information_menu,info_input);
    compare_command(app,expected,app.input_information_page(information_menu,info_input),error);
    error=expected.acknowledge_page(information_menu);
    compare_command(app,expected,app.acknowledge_page(information_menu),error);
    const auto income_page=top(*app.world())->id;
    require(top(*app.world())->legacy_page==36,"information menu opens real income page36");
    info_tick=expected.update();
    compare_command(app,expected,app.update(),info_tick.error);
    info_input={}; info_input.right=true;
    error=expected.input_information_page(income_page,info_input);
    compare_command(app,expected,app.input_information_page(income_page,info_input),error);
    require(app.world()->state().page_phases.at(income_page)==1,
            "application forwards income year selection to the single Owner");
    error=expected.cancel_page(income_page);
    compare_command(app,expected,app.cancel_page(income_page),error);
    rejected([&]{return !app.input_information_page(income_page,info_input).empty();});
    // 上层只验目录命令桥与一次输出；NEW/滚动/损坏载荷由pages和restore套件主责。
    info_tick=expected.update();
    compare_command(app,expected,app.update(),info_tick.error);
    error=expected.open_information_menu();
    compare_command(app,expected,app.open_information_menu(),error);
    const auto equipment_menu=top(*app.world())->id;
    info_tick=expected.update();
    compare_command(app,expected,app.update(),info_tick.error);
    info_input={}; info_input.select_row=4;
    error=expected.input_information_page(equipment_menu,info_input);
    compare_command(app,expected,app.input_information_page(equipment_menu,info_input),error);
    error=expected.acknowledge_page(equipment_menu);
    compare_command(app,expected,app.acknowledge_page(equipment_menu),error);
    const auto equipment_page=top(*app.world())->id;
    require(top(*app.world())->legacy_page==38,"application opens real equipment directory38");
    info_tick=expected.update();
    compare_command(app,expected,app.update(),info_tick.error);
    info_input={}; info_input.right=true; info_input.down=true;
    error=expected.input_information_page(equipment_page,info_input);
    compare_command(app,expected,app.input_information_page(equipment_page,info_input),error);
    require(app.world()->state().page_phases.at(equipment_page)==1 &&
                app.world()->state().information_page_data.at(equipment_page).selection==1,
            "application forwards directory tab and row to the single Owner");
    error=expected.cancel_page(equipment_page);
    compare_command(app,expected,app.cancel_page(equipment_page),error);
    rejected([&]{return !app.input_information_page(equipment_page,info_input).empty();});
    error=expected.open_build_menu();
    compare_command(app,expected,app.open_build_menu(),error);
    const auto build_page=top(*app.world())->id;
    rejected([&]{return !app.select_build_menu(build_page+1,35).error.empty();});
    auto selected=expected.select_build_menu(build_page,35);
    const auto actual_selected=app.select_build_menu(build_page,35);
    require(selected.denial==StartupBuildDenial::none && actual_selected.denial==selected.denial &&
                actual_selected.created==selected.created,
            "real available bakery selection preserves structured result without manufacturing identity");
    compare_command(app,expected,actual_selected.error,selected.error);
    const auto &state=app.world()->state();
    const auto bounds=state.rules->fences.at(state.fence_level);
    const auto &map=state.scene.world.world.map;
    std::optional<ref::Position> anchor;
    for (int y=bounds[1].y+1;y<bounds[0].y && !anchor;++y)
        for (int x=bounds[0].x+1;x<bounds[1].x;++x)
            if (!map.cells.at(y*map.width+x).facility) { anchor=ref::Position{x,y}; break; }
    require(anchor.has_value(),"real initial town has a legal single-cell bakery anchor");
    const auto funds=state.scene.world.world.ai.accounting.funds();
    const auto count=state.scene.world.facility_order.size();
    const auto built=expected.confirm_build(*anchor,ref::FacilityOrientation::first);
    const auto actual_built=app.confirm_build(*anchor,ref::FacilityOrientation::first);
    require(built.created && actual_built.created==built.created &&
                actual_built.denial==StartupBuildDenial::none,
            "application publishes actual committed bakery identity");
    // 在尚未领取建设声音时测试另一领域的错误输入，不能吞掉既有输出。
    rejected([&]{return !app.act_village_activity_page(build_page,
                        StartupVillageActivityAction::confirm).empty();});
    compare_command(app,expected,actual_built.error,built.error);
    require(app.world()->state().scene.world.world.ai.accounting.funds()==funds-600 &&
                app.world()->state().scene.world.facility_order.size()==count+1,
            "source bakery600 charge and one entity commit through application management");
    error=expected.cancel_build();
    compare_command(app,expected,app.cancel_build(),error);
    error=expected.open_village_activities();
    compare_command(app,expected,app.open_village_activities(),error);
    const auto activity=top(*app.world())->id;
    // 初次51必须经过真实框架初始化及说明页；输入不能越过这条生命周期。
    rejected([&]{return !app.act_village_activity_page(activity,
                        StartupVillageActivityAction::select,0).empty();});
    bool initialized{};
    for (int n=0;n<64;++n) {
        const auto page=top(*app.world());
        require(page!=nullptr,"village introduction retains a current framework page");
        if (page->id==activity && app.world()->state().activity_pages_initialized.count(activity)) {
            initialized=true;
            break;
        }
        if (page->id!=activity) {
            const auto page_id=page->id;
            error=expected.acknowledge_page(page_id);
            compare_command(app,expected,app.acknowledge_page(page_id),error);
        }
        const auto tick=expected.update();
        compare_command(app,expected,app.update(),tick.error);
    }
    require(initialized,"real initial village explanation returns to initialized raw51 within bounded updates");
    rejected([&]{return !app.act_village_activity_page(activity+1,
                        StartupVillageActivityAction::select,0).empty();});
    error=expected.act_village_activity_page(activity,StartupVillageActivityAction::select,0);
    compare_command(app,expected,
        app.act_village_activity_page(activity,StartupVillageActivityAction::select,0),error);
    error=expected.act_village_activity_page(activity,StartupVillageActivityAction::cancel);
    compare_command(app,expected,
        app.act_village_activity_page(activity,StartupVillageActivityAction::cancel),error);
    error=expected.open_task_control_menu();
    compare_command(app,expected,app.open_task_control_menu(),error);
    const auto adventure=top(*app.world())->id;
    auto adventure_tick=expected.update();
    compare_command(app,expected,app.update(),adventure_tick.error);
    require(top(*app.world())->legacy_page==4 && !app.world()->state().active_task,
            "real adventure menu is valid without selected task; source tag9 remains available");
    rejected([&]{return !app.act_task_page(adventure,StartupWorldTaskAction::request_abort).error.empty();});
    error=expected.cancel_page(adventure);
    compare_command(app,expected,app.cancel_page(adventure),error);
    error=expected.open_main_menu();
    compare_command(app,expected,app.open_main_menu(),error);
    const auto main_menu=top(*app.world())->id;
    adventure_tick=expected.update();
    compare_command(app,expected,app.update(),adventure_tick.error);
    StartupWorldMenuInput menu_input;menu_input.down=true;
    error=expected.input_menu_page(main_menu,menu_input);
    compare_command(app,expected,app.input_menu_page(main_menu,menu_input),error);
    require(app.world()->state().main_menu_selection==1,
            "application forwards actual main menu input to sole world Owner");
    menu_input={};menu_input.cancel=true;
    error=expected.input_menu_page(main_menu,menu_input);
    compare_command(app,expected,app.input_menu_page(main_menu,menu_input),error);
    error=expected.open_task_menu();
    compare_command(app,expected,app.open_task_menu(),error);
    const auto task_page=top(*app.world())->id;
    rejected([&]{return !app.act_task_page(task_page+1,StartupWorldTaskAction::confirm).error.empty();});
    // 原新局无可接受任务时会转事件29。实际入口及其输出仍须与Session完全一致。
    if (top(*app.world())->legacy_page==22) {
        const auto task=expected.act_task_page(task_page,StartupWorldTaskAction::cancel);
        const auto actual_task=app.act_task_page(task_page,StartupWorldTaskAction::cancel);
        require(actual_task.denial==task.denial && actual_task.accepted==task.accepted &&
                    actual_task.departed==task.departed,
                "task bridge preserves actual result flags when cancelling existing catalogue");
        compare_command(app,expected,actual_task.error,task.error);
    } else {
        require(app.world()->state().task_order.empty(),
                "empty real task catalogue follows original unavailable script without injected tasks");
    }
    require(app.records().save_directory==directory && bytes(files.root/"system.avr")==system &&
                no_world_blobs(files) && app.world()->state().sound_requests.empty() &&
                !app.has_pending_audio_requests(),
            "short management actions retire outputs without publishing world files or changing save directory");
}
// 复用同套件已校验的独立壶资格/库存档；每个入口重新载入，不共享动作后的世界。
// 只核应用接线与事务，费用、发现及炼制组合仍由pages套件主责。
void magic_pot_bridges(const std::filesystem::path &parent, const std::filesystem::path &root) {
    using Action = StartupMagicPotAction;
    auto fixture = load_startup_world_file(parent / "magic-pot-window.avrs", startup_world_rules(),
                                          StartupWorldSavePurpose::normal);
    require(fixture.snapshot.has_value(), "existing validated magic-pot condition fixture");
    fixture.snapshot->session.take_audio_requests(); // 消费夹具出口，不注入声音。
    StartupWorldSaveMetadata metadata;
    metadata.purpose = StartupWorldSavePurpose::replay;
    metadata.controller_id = "application-magic-pot-bridge-fixture-v1";
    metadata.controller_state = {1};
    const auto replay = root / "magic-pot-entry.avrs";
    require(save_startup_world_file(replay, fixture.snapshot->session, metadata).ok,
            "encode private magic-pot application input through existing codec");
    for (const auto entry : {StartupMagicPotEntry::main_menu, StartupMagicPotEntry::development_menu}) {
        const auto files = paths(root, entry == StartupMagicPotEntry::main_menu ? "pot-main" : "pot-development");
        StartupApplication app(files, ref::WorldRandomStream::from_java_seed(1));
        good(app.load_world_replay(replay, metadata.controller_id));
        app.take_audio_requests(); // 正常载入的BGM由本测试领取，后续逐动作核对输出。
        auto expected = *app.world();
        const auto system = bytes(files.root / "system.avr");
        const auto directory = app.records().save_directory;
        const auto rejected = [&](const auto &action) {
            const auto before = management_observation(app);
            require(!action().empty(), "invalid magic-pot bridge returns explicit error");
            require(management_observation(app) == before && bytes(files.root / "system.avr") == system &&
                        app.records().save_directory == directory && no_world_blobs(files),
                    "magic-pot rejection preserves application, random, outputs and storage");
        };
        const auto command = [&](std::uint64_t page, Action action, int selection = 0) {
            const auto error = expected.act_magic_pot_page(page, action, selection);
            compare_command(app, expected, app.act_magic_pot_page(page, action, selection), error);
        };
        const auto update = [&] {
            const auto result = expected.update();
            compare_command(app, expected, app.update(), result.error);
        };
        const auto reach = [&](int raw) {
            for (int step = 0; step < 128; ++step) {
                const auto *page = top(*app.world());
                require(page != nullptr, "magic-pot bridge retains a current page");
                if (page->legacy_page == raw &&
                    inspect_startup_world_magic_pot_page(app.world()->state(), page->id)) return page->id;
                // 首次101/退出105通过dialogue页阻挡壶父页，须走应用真实确认而非只等计数。
                // 45也由真实确认快进/关闭，不能因它拒绝取消就跳过确认。
                if (page->legacy_page != raw && page->kind != ref::WorldScriptPageKind::scene &&
                    page->lifecycle == 2) {
                    const auto id = page->id;
                    const auto error = expected.acknowledge_page(id);
                    compare_command(app, expected, app.acknowledge_page(id), error);
                }
                update();
            }
            const auto *page = top(*app.world());
            const auto &state = app.world()->state();
            const auto counter = page ? state.page_counters.find(page->id) : state.page_counters.end();
            throw std::runtime_error("magic-pot application page did not initialize within bounded updates: entry=" +
                std::to_string(static_cast<int>(entry)) + " target_raw=" + std::to_string(raw) +
                " top_kind=" + std::to_string(page ? static_cast<int>(page->kind) : -1) +
                " top_raw=" + std::to_string(page ? page->legacy_page : -1) +
                " lifecycle=" + std::to_string(page ? page->lifecycle : -1) +
                " counter=" + std::to_string(counter == state.page_counters.end() ? -1 : counter->second));
        };
        rejected([&] { return app.open_magic_pot(static_cast<StartupMagicPotEntry>(99)); });
        const auto error = expected.open_magic_pot(entry);
        compare_command(app, expected, app.open_magic_pot(entry), error);
        const auto uninitialized = top(*app.world())->id;
        rejected([&] { return app.act_magic_pot_page(uninitialized, Action::confirm); });
        const auto menu = reach(41);
        rejected([&] { return app.open_magic_pot(entry); });
        rejected([&] { return app.act_magic_pot_page(menu + 1000, Action::confirm); });
        rejected([&] { return app.act_magic_pot_page(menu, static_cast<Action>(99)); });
        rejected([&] { return app.act_magic_pot_page(menu, Action::select, 2); });
        if (entry == StartupMagicPotEntry::main_menu) {
            command(menu, Action::confirm);
            const auto deposit = reach(42);
            const auto view = inspect_startup_world_magic_pot_page(app.world()->state(), deposit);
            require(view && !view->entries.empty() && view->entries.front() == 0,
                    "existing magic-pot fixture selects actual item0 inventory");
            command(deposit, Action::select, 0);
            const auto inventory = app.world()->state().items.at(0).inventory;
            const auto draws = app.world()->state().scene.random.draws();
            const auto result_error = expected.act_magic_pot_page(deposit, Action::confirm);
            const auto actual_error = app.act_magic_pot_page(deposit, Action::confirm);
            rejected([&] { return app.act_magic_pot_page(deposit, Action::confirm); });
            compare_command(app, expected, actual_error, result_error);
            require(app.world()->state().items.at(0).inventory == inventory - 1 &&
                        app.world()->state().scene.random.draws() == draws + 1,
                    "application deposit consumes one actual inventory and one original comment draw");
            command(reach(44), Action::cancel);
            const auto restored_deposit = reach(42);
            command(restored_deposit, Action::cancel);
        } else {
            command(menu, Action::select, 1);
            command(menu, Action::confirm);
            const auto recipes = reach(43);
            command(recipes, Action::next_tab);
            require(inspect_startup_world_magic_pot_page(app.world()->state(), recipes)->phase == 1,
                    "application forwards recipe tab input");
            command(recipes, Action::previous_tab);
            command(recipes, Action::cancel);
        }
        command(reach(41), Action::cancel);
        rejected([&] { return app.act_magic_pot_page(menu, Action::confirm); });
        update();
        const auto &state = app.world()->state();
        require(state.magic_pot_pages_initialized.empty() && state.magic_pot_page_data.empty() &&
                    state.magic_pot_page_lists.empty() && state.magic_pot_page_parents.empty(),
                "application update retires all closed magic-pot payloads");
        require(bytes(files.root / "system.avr") == system && app.records().save_directory == directory &&
                    no_world_blobs(files) && !app.has_pending_audio_requests(),
                "short magic-pot management leaves storage unchanged and outputs consumed");
    }
}
// 此层只核应用文件提交与真实菜单接线；raw14载荷边界由pages/restore套件主责。
std::uint64_t enter_system_request(StartupApplication &app, int tag, int raw) {
    good(app.open_main_menu());
    good(app.update());
    const auto choose = [&](int tag) {
        const auto id = top(*app.world())->id;
        const auto view = inspect_startup_world_menu_page(app.world()->state(), id);
        require(view.has_value(), "save bridge enters initialized source menu");
        const auto row = std::find(view->tags.begin(), view->tags.end(), tag);
        require(row != view->tags.end(), "source menu contains actual save navigation tag");
        StartupWorldMenuInput input;
        input.select_row = static_cast<int>(row - view->tags.begin());
        good(app.input_menu_page(id, input));
        input = {}; input.confirm = true;
        good(app.input_menu_page(id, input));
    };
    choose(6);
    good(app.update());
    require(top(*app.world())->legacy_page == 10, "main menu opens actual system submenu10");
    choose(tag);
    const auto id = top(*app.world())->id;
    require(top(*app.world())->legacy_page == raw, "system source tag creates its actual target page");
    return id;
}
std::uint64_t enter_save_request(StartupApplication &app) {
    const auto id = enter_system_request(app, 20, 14);
    require(!app.save_world().empty(), "ordinary save cannot bypass pending modal save page");
    good(app.update());
    const auto view = inspect_startup_world_save_page(app.world()->state(), id);
    require(view && view->stage == 1 && !view->saved,
            "first raw14 update only creates application save request");
    const auto before = startup_world_session_digest(*app.world());
    good(app.acknowledge_page(id)); good(app.cancel_page(id)); good(app.update(true));
    require(startup_world_session_digest(*app.world()) == before,
            "early confirm and cancel preserve pending save request without writing");
    return id;
}
auto save_business(const StartupApplication &app) {
    const auto &s = app.world()->state();
    const auto random = s.scene.random.snapshot();
    return std::make_tuple(s.simulation_steps, s.scene.calendar.units,
        s.scene.world.world.ai.accounting.funds(), s.scene.world.updates,
        random.engine_state, random.tape, random.cursor, random.tape_mode);
}
void window_host_save(const std::filesystem::path &root) {
    static_assert(std::is_same_v<decltype(std::declval<StartupWindowHost &>().runtime()),
                                 const StartupWorldRuntimeSession &>);
    static_assert(std::is_same_v<decltype(std::declval<StartupWindowHost &>().state()),
                                 const StartupWorldRuntimeState &>);
    static_assert(std::is_same_v<decltype(std::declval<StartupWindowHost &>().application()),
                                 const StartupApplication *>);
    static_assert(std::is_same_v<decltype(std::declval<StartupWindowHost &>().update()),
                                 StartupWindowUpdate>);
    const auto files=paths(root,"window-host-save");
    std::uint64_t committed_revision{};
    std::string committed_system;
    std::int64_t cash{};
    ref::WorldRandomSnapshot random;
    {
        StartupApplication app(files,ref::WorldRandomStream::from_java_seed(17));
        good(app.request_new_game(0));good(app.start_game());
        StartupWindowHost host(std::move(app));
        require(host.application_mode() && host.application() &&
                    &host.runtime()==host.application()->world(),
                "window host reads its sole application-owned Session");
        bool rejected{};
        try { (void)host.standalone(); } catch(const std::runtime_error &) { rejected=true; }
        require(rejected,"application window cannot escape into standalone persistence path");
        require(host.take_audio_requests()==std::vector<StartupAudioRequest>{
                    {StartupAudioOperation::replace_bgm,0},{StartupAudioOperation::replace_bgm,1}} &&
                    host.take_sound_requests().empty(),
                "window host typed audio takes original startup outputs once");
        const auto before_rejection=startup_world_session_digest(host.runtime());
        const auto before_file=bytes(files.root/"system.avr");
        bool wrong_clear{};
        try { (void)host.update(true); } catch(const std::runtime_error &) { wrong_clear=true; }
        require(wrong_clear && startup_world_session_digest(host.runtime())==before_rejection &&
                    bytes(files.root/"system.avr")==before_file,
                "window clear confirmation outside raw17 refuses before updating world or storage");
        StartupWindowHost standalone(host.runtime());
        wrong_clear=false;
        try { (void)standalone.update(true); } catch(const std::runtime_error &) { wrong_clear=true; }
        require(wrong_clear && !standalone.application_mode() &&
                    startup_world_session_digest(standalone.standalone())==before_rejection,
                "standalone window cannot invent an application score confirmation consumer");
        const auto stale=host.state().scripts.next_page_id;
        StartupWorldMenuInput bad_input;bad_input.confirm=true;
        auto original_error=*host.application();
        const auto expected_diagnostic=original_error.input_menu_page(stale,bad_input);
        require(!expected_diagnostic.empty() &&
                    host.input_menu_page(stale,bad_input)==StartupWorldRuntimeError::runtime_failed &&
                    host.last_error()==expected_diagnostic,
                "window wrong-page command returns failure and original application text without exiting");
        const auto bad_build=host.select_build_menu(stale,35);
        require(bad_build.error==StartupWorldRuntimeError::runtime_failed &&
                    !host.last_error().empty() && !bad_build.created &&
                    bad_build.denial==StartupBuildDenial::none,
                "failed application build command cannot expose a stale success identity or business denial");
        const auto bad_task=host.act_task_page(stale,StartupWorldTaskAction::confirm);
        require(bad_task.error==StartupWorldRuntimeError::runtime_failed &&
                    !host.last_error().empty() && !bad_task.accepted && !bad_task.departed &&
                    bad_task.denial==ref::TaskCommandDenial::none &&
                    startup_world_session_digest(host.runtime())==before_rejection &&
                    bytes(files.root/"system.avr")==before_file && no_world_blobs(files),
                "failed typed commands preserve complete world and files without successful receipts");
        const auto update=[&] {
            const auto result=host.update();
            require(result.committed && result.error==StartupWorldRuntimeError::none &&
                        result.scene_error==ref::WorldSceneError::none &&
                        result.world_error==ref::WorldScheduleError::none,
                    "window update returns small committed receipt without a world candidate");
        };
        require(host.open_main_menu()==StartupWorldRuntimeError::none && host.last_error().empty(),
                "next legal window command opens actual menu and clears stale diagnostic");
        update();
        const auto choose=[&](int tag) {
            const auto id=top(host.runtime())->id;
            const auto view=inspect_startup_world_menu_page(host.state(),id);
            require(view.has_value(),"window menu projection has real Owner payload");
            const auto row=std::find(view->tags.begin(),view->tags.end(),tag);
            require(row!=view->tags.end(),"window navigation tag exists");
            StartupWorldMenuInput input;input.select_row=static_cast<int>(row-view->tags.begin());
            require(host.input_menu_page(id,input)==StartupWorldRuntimeError::none,"window selects row");
            input={};input.confirm=true;
            require(host.input_menu_page(id,input)==StartupWorldRuntimeError::none,"window confirms row");
        };
        choose(6);update();choose(20);update();
        host.take_audio_requests();
        const auto id=top(host.runtime())->id;
        require(inspect_startup_world_save_page(host.state(),id)->stage==1,
                "window stage1 waits for application writer");
        const auto revision=host.application()->records().revision;
        update();
        require(host.application()->records().revision==revision+1 &&
                    inspect_startup_world_save_page(host.state(),id)->saved==true,
                "window next update commits exactly one real manual save");
        committed_revision=host.application()->records().revision;
        committed_system=bytes(files.root/"system.avr");
        cash=host.state().scene.world.world.ai.accounting.funds();
        random=host.state().scene.random.snapshot();
        update();
        require(host.application()->records().revision==committed_revision &&
                    bytes(files.root/"system.avr")==committed_system,
                "result redraw/update does not write the same save again");
    }
    StartupApplication cold(files,ref::WorldRandomStream::from_java_seed(93));
    require(cold.records().revision==committed_revision,"cold application reopens persisted directory");
    good(cold.load_world(0));
    StartupWindowHost loaded(std::move(cold));
    const auto restored_random=loaded.state().scene.random.snapshot();
    require(loaded.take_sound_requests()==std::vector<int>{0,1} && loaded.take_audio_requests().empty(),
            "cold window legacy audio also takes shared output queue only once");
    require(loaded.state().scripts.pages.size()==1 &&
                loaded.state().scripts.pages.front().kind==ref::WorldScriptPageKind::scene &&
                loaded.state().save_marker==1 &&
                loaded.state().scene.world.world.ai.accounting.funds()==cash &&
                std::tie(restored_random.engine_state,restored_random.tape,restored_random.cursor,restored_random.tape_mode)==
                    std::tie(random.engine_state,random.tape,random.cursor,random.tape_mode),
            "destroyed window host cold-loads stable scene with same funds and random, not raw14");
}

void manual_page_bridges(const std::filesystem::path &root) {
    const auto files = paths(root, "manual-bridge");
    StartupApplication app(files, ref::WorldRandomStream::from_java_seed(1));
    good(app.request_new_game(0)); good(app.start_game());
    const auto id = enter_system_request(app, 22, 13);
    require(!app.save_world().empty(), "ordinary storage rejects transient manual page");
    good(app.update());
    const auto view = inspect_startup_world_manual_page(app.world()->state(), id);
    require(view && view->index == 0 && !view->about && !view->text.empty() && view->localize_text,
            "real application3-to10-tag22 initializes source manual text through unique Owner");
    // 先经过真实换页，再到关于页；快照必须保留隐藏正文及“不再LT”的来源状态。
    StartupManualInput previous; previous.left = true;
    good(app.input_manual_page(id, previous));
    StartupManualInput first; first.right = true;
    good(app.input_manual_page(id, first));
    good(app.input_manual_page(id, previous));
    const auto about = inspect_startup_world_manual_page(app.world()->state(), id);
    require(about && about->about && about->text_page == 0 && !about->localize_text && about->text == view->text,
            "about retains first-page cache after real SetText without replaying Init localization");
    const auto audio = app.take_audio_requests();
    require(!audio.empty() && app.take_audio_requests().empty(),
            "application owns real startup and menu outputs, consumed exactly once before capture");
    const auto business = save_business(app);
    const auto system = bytes(files.root / "system.avr");
    StartupApplicationReplayMetadata metadata;
    metadata.controller_id = "manual-page-bridge-v1"; metadata.controller_state = {1};
    const StartupApplicationReplayValidator validator = [](const auto &, const auto &m) {
        return m.controller_id=="manual-page-bridge-v1" && m.controller_state==std::vector<std::uint8_t>{1} &&
               m.next_frame==0 && m.next_command==0 && m.extensions.empty()
            ? std::string{} : std::string{"invalid manual bridge replay driver"};
    };
    const auto replay = root / "manual-page.avra";
    good(save_startup_application_replay(root, replay, app, metadata, validator));
    StartupApplication restored(paths(root, "manual-replay-placeholder"), ref::WorldRandomStream::from_java_seed(99));
    StartupApplicationReplayMetadata restored_metadata;
    good(restore_startup_application_replay(replay, root / "manual-replay-restored",
        metadata.controller_id, restored, restored_metadata, validator));
    require(startup_application_replay_digest(app, metadata, validator) ==
                startup_application_replay_digest(restored, restored_metadata, validator),
            "full application replay restores initialized manual text phase frozen pool and random without Init");
    const auto restored_about = inspect_startup_world_manual_page(restored.world()->state(), id);
    require(restored_about && restored_about->about && !restored_about->localize_text &&
                restored_about->text_page == about->text_page && restored_about->text == about->text,
            "replay retains hidden manual text and cannot reapply initial LT");
    StartupManualInput next; next.confirm = true; next.right = true;
    good(app.input_manual_page(id, next)); good(restored.input_manual_page(id, next));
    good(app.update()); good(restored.update());
    require(startup_application_replay_digest(app, metadata, validator) ==
                startup_application_replay_digest(restored, restored_metadata, validator) &&
                save_business(app) == business && bytes(files.root / "system.avr") == system &&
                app.take_audio_requests().empty() && restored.take_audio_requests().empty(),
            "manual short replay consumes the same command without world advancement storage or duplicate sound");
    good(app.cancel_page(id)); good(app.update());
    require(top(*app.world())->kind == ref::WorldScriptPageKind::scene &&
                !app.world()->state().manual_page_data.count(id) &&
                app.world()->state().menu_page_data.empty(),
            "manual returns to actual GameForm and releases both retired navigation menus and frozen members");
}

void save_page_bridges(const std::filesystem::path &root) {
    for (int slot = 0; slot != 2; ++slot) {
        const auto files = paths(root, "save-slot-" + std::to_string(slot));
        StartupApplication app(files, ref::WorldRandomStream::from_java_seed(1));
        good(app.request_new_game(slot)); good(app.start_game()); app.take_audio_requests();
        good(app.save_world()); // 保留一份真实稳定旧档，检验替换而非仅首次创建。
        const auto before_directory = app.records().save_directory;
        const auto before_revision = app.records().revision;
        const auto before_system = bytes(files.root / "system.avr");
        const auto business = save_business(app);
        const auto page = enter_save_request(app);
        app.take_audio_requests();
        require(bytes(files.root / "system.avr") == before_system &&
                    app.records().save_directory == before_directory && save_business(app) == business,
                "menu and first save update preserve storage money date and complete random stream");

        StartupApplication restored(paths(root, "save-replay-placeholder-" + std::to_string(slot)),
                                    ref::WorldRandomStream::from_java_seed(99));
        StartupApplicationReplayMetadata metadata;
        metadata.controller_id = "save-page-bridge-v1"; metadata.controller_state = {1};
        const auto validator = [](const StartupApplication &, const StartupApplicationReplayMetadata &m) {
            return m.controller_id == "save-page-bridge-v1" && m.controller_state == std::vector<std::uint8_t>{1} &&
                m.next_frame == 0 && m.next_command == 0 && m.extensions.empty()
                ? std::string{} : std::string{"invalid save bridge replay driver"};
        };
        if (slot == 0) {
            const auto replay = root / "save-stage1.avra";
            good(save_startup_application_replay(root, replay, app, metadata, validator));
            good(restore_startup_application_replay(replay, root / "save-stage1-restored",
                metadata.controller_id, restored, metadata, validator));
            require(startup_application_replay_digest(app, metadata, validator) ==
                        startup_application_replay_digest(restored, metadata, validator),
                    "full application replay preserves pending save page and old directory bytes");
            good(restored.update());
        }
        good(app.update());
        const auto view = inspect_startup_world_save_page(app.world()->state(), page);
        require(view && view->stage == 2 && view->saved == true && app.world()->state().save_marker == 1 &&
                    app.records().revision == before_revision + 1 && save_business(app) == business,
                "second save update commits exactly one manual record without advancing business or random");
        for (int s = 0; s != 2; ++s)
            for (int kind = 0; kind != 2; ++kind)
                if (s != slot || kind != static_cast<int>(StartupSaveKind::manual))
                    require(app.records().save_directory[s][kind] == before_directory[s][kind],
                            "manual save leaves other slot and both interrupt directories intact");
        if (slot == 0)
            require(startup_application_replay_digest(app, metadata, validator) ==
                        startup_application_replay_digest(restored, metadata, validator),
                    "restored phase1 request and uninterrupted application commit identical results");
        const auto saved_system = bytes(files.root / "system.avr");
        good(app.update());
        if (slot == 0) good(app.acknowledge_page(page)); else good(app.cancel_page(page));
        require(bytes(files.root / "system.avr") == saved_system &&
                    app.records().revision == before_revision + 1 && save_business(app) == business,
                "result idle update and either exit cannot submit save again");
        StartupApplication cold(files, ref::WorldRandomStream::from_java_seed(99));
        good(cold.load_world(slot)); cold.take_audio_requests();
        require(cold.world()->state().save_marker == 1 && save_business(cold) == business &&
                    top(*cold.world())->kind == ref::WorldScriptPageKind::scene &&
                    cold.world()->state().scene.top_is_main,
                "ordinary cold load restores stable saved world instead of modal result page");
    }

    const auto files = paths(root, "save-revision-conflict");
    StartupApplication conflict(files, ref::WorldRandomStream::from_java_seed(1));
    good(conflict.request_new_game(0)); good(conflict.start_game()); conflict.take_audio_requests();
    good(conflict.save_world());
    const auto old_directory = conflict.records().save_directory;
    const auto marker = conflict.world()->state().save_marker;
    const auto business = save_business(conflict);
    const auto page = enter_save_request(conflict); conflict.take_audio_requests();
    const auto disk = load_startup_application_storage(files.root);
    require(disk.snapshot.has_value(), "conflict fixture starts from actual valid storage");
    const auto concurrent = commit_startup_application_records(files.root, *disk.snapshot, disk.snapshot->records);
    require(concurrent.snapshot.has_value(), "real concurrent revision update creates stale application storage");
    const auto concurrent_bytes = bytes(files.root / "system.avr");
    good(conflict.update());
    const auto failed = inspect_startup_world_save_page(conflict.world()->state(), page);
    require(failed && failed->stage == 2 && failed->saved == false &&
                conflict.world()->state().save_marker == marker && save_business(conflict) == business &&
                conflict.records().save_directory == old_directory &&
                bytes(files.root / "system.avr") == concurrent_bytes,
            "failed application save shows result while preserving old directory world money date and random");
    const auto retained = load_startup_application_slot(files.root, *concurrent.snapshot, 0, StartupSaveKind::manual);
    require(retained.snapshot.has_value() && retained.snapshot->session.state().save_marker == marker,
            "revision conflict leaves previous real manual blob readable");
    good(conflict.cancel_page(page));
    require(bytes(files.root / "system.avr") == concurrent_bytes, "failure result exit never retries storage commit");

    const auto pending_files = paths(root, "save-pending-audio");
    StartupApplication pending(pending_files, ref::WorldRandomStream::from_java_seed(1));
    good(pending.request_new_game(0)); good(pending.start_game());
    enter_save_request(pending); // 真实标题及激活输出留在应用队列，不注入音频。
    require(pending.has_pending_audio_requests(), "pending gate uses real unconsumed application audio");
    const auto before = management_observation(pending);
    const auto system = bytes(pending_files.root / "system.avr");
    require(!pending.update().empty() && management_observation(pending) == before &&
                bytes(pending_files.root / "system.avr") == system && no_world_blobs(pending_files),
            "phase1 pending audio rejects without consuming output or publishing a world file");

    auto paused_session = *pending.world();
    paused_session.set_paused(true);
    StartupWorldSaveMetadata paused_metadata;
    paused_metadata.purpose = StartupWorldSavePurpose::replay;
    paused_metadata.controller_id = "application-paused-save-fixture-v1";
    paused_metadata.controller_state = {1};
    const auto paused_file = root / "paused-save.avrs";
    require(save_startup_world_file(paused_file, paused_session, paused_metadata).ok,
            "paused save condition fixture uses existing world replay codec");
    const auto paused_paths = paths(root, "save-paused");
    StartupApplication paused(paused_paths, ref::WorldRandomStream::from_java_seed(1));
    good(paused.load_world_replay(paused_file, paused_metadata.controller_id)); paused.take_audio_requests();
    const auto paused_before = management_observation(paused);
    const auto paused_system = bytes(paused_paths.root / "system.avr");
    good(paused.update(false));
    require(management_observation(paused) == paused_before &&
                bytes(paused_paths.root / "system.avr") == paused_system && no_world_blobs(paused_paths),
            "paused application phase1 update freezes request and does not publish world storage");
}
} // namespace

int run_startup_application_actions_checks(const std::filesystem::path &parent) {
    checks = 0;
    audio_ownership_checked = false;
    const OwnedDirectory owned(parent / "application-action-bridges");
    const auto &root = owned.path;
    management_bridges(root);
    magic_pot_bridges(parent, root);
    save_page_bridges(root);
    manual_page_bridges(root);
    window_host_save(root);
    // 复用计分套件条件，只验窗口宿主的单次确认接线，不重复六类分值组合。
    const auto clear_entry=create_application_clear_entry_fixture(root);
    const auto clear_files=paths(root,"window-host-clear");
    StartupApplication clear_app(clear_files,ref::WorldRandomStream::from_java_seed(1));
    good(clear_app.load_world_replay(clear_entry,"application-clear-entry-fixture-v1"));
    clear_app.take_audio_requests();
    StartupWindowHost clear_host(std::move(clear_app));
    const auto clear_business=save_business(*clear_host.application());
    const auto clear_system=bytes(clear_files.root/"system.avr");
    require(clear_host.update(true).committed && clear_host.application()->clear_page() &&
                clear_host.application()->clear_page()->stage==0 &&
                clear_host.application()->clear_page()->counter==1,
            "one early window confirmation enters application score exactly once without skipping its gate");
    require(clear_host.update().committed && clear_host.application()->clear_page()->counter==2 &&
                save_business(*clear_host.application())==clear_business &&
                bytes(clear_files.root/"system.avr")==clear_system && clear_host.take_audio_requests().empty(),
            "next score update advances once while preserving world money date random and system file");
    const auto entries = create_application_action_entry_fixtures(root);
    StartupApplication empty(paths(root, "empty"), ref::WorldRandomStream::from_java_seed(1));
    require(!empty.return_rank_page(1).empty() && !empty.leave_commerce_page(1).empty() &&
                !empty.act_award_page(1, ref::WorldAwardAction::request_termination).empty(),
            "commands reject title without a world");
    require(empty.take_sound_requests() == std::vector<int>{0},
            "rejected world commands preserve the cold title output unchanged");
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto files = paths(root, std::to_string(i));
        StartupApplication app(files, ref::WorldRandomStream::from_java_seed(1));
        good(app.load_world_replay(entries[i], "application-action-entry-fixture-v1"));
        require(app.take_sound_requests() == std::vector<int>({0,1}) &&
                    app.world()->state().sound_requests.empty(),
                "conditional world entry consumes cold title B0 then actual activation G, not exact application restore");
        const auto system_before = bytes(files.root / "system.avr");
        const auto directory_before = app.records().save_directory;
        require(!system_before.empty() && directory_before == StartupSystemRecords{}.save_directory &&
                    no_world_blobs(files),
                "world replay entry publishes system activation only, no four-slot world records");
        const auto id = top(*app.world())->id;
        const auto before = startup_world_session_digest(*app.world());
        require(!app.return_rank_page(id + 1).empty() && !app.leave_commerce_page(id + 1).empty() &&
                    !app.act_award_page(id + 1, ref::WorldAwardAction::request_termination).empty(),
                "wrong stable page reference rejects all bridges");
        if (i != 0) require(!app.return_rank_page(id).empty(), "rank return rejects other raw pages");
        if (i != 1) require(!app.leave_commerce_page(id).empty(), "commerce leave rejects other raw pages");
        if (i < 2) require(!app.act_award_page(id, ref::WorldAwardAction::request_termination).empty(),
                           "award action rejects other raw pages");
        if (i >= 2) require(!app.act_award_page(id, ref::WorldAwardAction::confirm_termination).empty(),
                            "termination confirm without request rejects");
        require(startup_world_session_digest(*app.world()) == before &&
                    bytes(files.root / "system.avr") == system_before &&
                    app.records().save_directory == directory_before && no_world_blobs(files) &&
                    app.take_audio_requests().empty(),
                "rejected bridge preserves owner, random, output and files");
        auto expected = *app.world();
        const auto rank = expected.state().rank;
        const auto funds = expected.state().scene.world.world.ai.accounting.funds();
        if (i == 0) {
            const auto expected_error = expected.act_rank_page(id, 0, true);
            compare_command(app, expected, app.return_rank_page(id), expected_error);
        } else if (i == 1) {
            require(!expected.state().commerce_pages_initialized.count(id),
                    "uninitialized commerce cancellation fixture");
            const auto expected_error = expected.cancel_page(id);
            compare_command(app, expected, app.leave_commerce_page(id), expected_error);
            require(app.world()->state().commerce_page_data.empty() &&
                        app.world()->state().commerce_page_lists.empty(),
                    "commerce leave does not manufacture a purchase catalogue");
        } else {
            require(expected.state().medal_count == (i == 2 ? 1 : 0) &&
                        bool(expected.state().award_rankings.count(id)) == (i == 3),
                    "award fixtures distinguish fresh initialization from initialized exhaustion");
            auto expected_error = expected.act_award_page(id, ref::WorldAwardAction::request_termination);
            compare_command(app, expected,
                            app.act_award_page(id, ref::WorldAwardAction::request_termination), expected_error);
            if (i == 2) {
                require(top(*app.world())->id == id &&
                            app.world()->state().award_termination_pending.at(id) &&
                            app.world()->state().medal_count == 2,
                        "first initialization adds one medal before real termination prompt");
                expected_error = expected.act_award_page(id, ref::WorldAwardAction::confirm_termination);
                compare_command(app, expected,
                                app.act_award_page(id, ref::WorldAwardAction::confirm_termination), expected_error);
                require(app.world()->state().medal_count == 2, "termination preserves both medals without awarding");
            }
            // 零勋章自动关闭后不制造第二次确认。
        }
        const auto closed = startup_world_session_digest(*app.world());
        require(!top(*app.world()) || top(*app.world())->id != id, "requested page actually retired");
        require(app.world()->state().rank == rank &&
                    app.world()->state().scene.world.world.ai.accounting.funds() == funds,
                "passive page exits do not promote or spend funds");
        require(!app.return_rank_page(id).empty() && !app.leave_commerce_page(id).empty() &&
                    !app.act_award_page(id, ref::WorldAwardAction::confirm_termination).empty() &&
                    startup_world_session_digest(*app.world()) == closed,
                "retired page cannot be consumed twice");
        require(bytes(files.root / "system.avr") == system_before &&
                    app.records().save_directory == directory_before && no_world_blobs(files),
                "non-record exits preserve exact published system bytes and all four directories without world blobs");
    }
    require(audio_ownership_checked, "单队列双接口领取已使用非空条件动作输出验证，空队列不能代替覆盖");
    return checks;
}
