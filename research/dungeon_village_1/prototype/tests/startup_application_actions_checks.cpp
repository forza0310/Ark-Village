#include "dungeon_village_prototype/startup_application.hpp"
#include "dungeon_village_prototype/startup_application_replay.hpp"
#include "dungeon_village_prototype/startup_world_village_activity.hpp"
#include "dungeon_village_prototype/startup_world_magic_pot.hpp"
#include "dungeon_village_prototype/startup_world_information.hpp"
#include "startup_application_natural_replay.hpp"
#include "support/audio_requests.hpp"
#include <fstream>
#include <iterator>
#include <stdexcept>

using namespace dungeon_village_prototype;
std::array<std::filesystem::path, 4> create_application_action_entry_fixtures(
    const std::filesystem::path &);
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
    good(app.request_new_game(0)); good(app.start_game());
    require(app.take_audio_requests()==std::vector<StartupAudioRequest>{
                {StartupAudioOperation::replace_bgm,0},{StartupAudioOperation::replace_bgm,1}},
            "real new management world consumes actual B0 and G activation outputs");
    const auto system=bytes(files.root/"system.avr");
    rejected([&]{return !app.open_magic_pot(StartupMagicPotEntry::main_menu).empty();});
    const auto directory=app.records().save_directory;
    auto expected=*app.world();
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
    rejected([&]{return !app.open_task_control_menu().empty();});
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
} // namespace

int run_startup_application_actions_checks(const std::filesystem::path &parent) {
    checks = 0;
    audio_ownership_checked = false;
    const OwnedDirectory owned(parent / "application-action-bridges");
    const auto &root = owned.path;
    management_bridges(root);
    magic_pot_bridges(parent, root);
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
