#include "dungeon_village_prototype/startup_application.hpp"
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
} // namespace

int run_startup_application_actions_checks(const std::filesystem::path &parent) {
    checks = 0;
    audio_ownership_checked = false;
    const OwnedDirectory owned(parent / "application-action-bridges");
    const auto &root = owned.path;
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
