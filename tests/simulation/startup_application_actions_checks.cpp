#include "ark/simulation/startup_application.hpp"
#include "startup_application_natural_replay.hpp"
#include <stdexcept>

using namespace ark::simulation;
std::array<std::filesystem::path, 4> create_application_action_entry_fixtures(
    const std::filesystem::path &);
namespace {
int checks{};
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
    return {root / (name + "-system.avr"),
            {root / (name + "-world0.avr"), root / (name + "-world1.avr")}};
}
// 此层只核应用接线与事务：完整年度计算继续由原授勋套件主责。
void compare_command(StartupApplication &app, StartupWorldRuntimeSession &expected,
                     const std::string &error, StartupWorldRuntimeError expected_error) {
    good(error);
    require(expected_error == StartupWorldRuntimeError::none, "reference command accepted");
    require(startup_world_session_digest(*app.world()) == startup_world_session_digest(expected),
            "application bridge preserves full Session, history, order and random");
    if (!app.world()->state().sound_requests.empty())
        checks += run_startup_application_natural_pending_sound_check(app);
    require(app.take_sound_requests() == expected.take_sound_requests(),
            "application bridge emits original ordered sounds once");
    require(app.take_sound_requests().empty(), "application output sink consumed once");
}
} // namespace

int run_startup_application_actions_checks(const std::filesystem::path &parent) {
    checks = 0;
    const OwnedDirectory owned(parent / "application-action-bridges");
    const auto &root = owned.path;
    const auto entries = create_application_action_entry_fixtures(root);
    StartupApplication empty(paths(root, "empty"), ref::WorldRandomStream::from_java_seed(1));
    require(!empty.return_rank_page(1).empty() && !empty.leave_commerce_page(1).empty() &&
                !empty.act_award_page(1, ref::WorldAwardAction::request_termination).empty(),
            "commands reject title without a world");
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto files = paths(root, std::to_string(i));
        StartupApplication app(files, ref::WorldRandomStream::from_java_seed(1));
        good(app.load_world_replay(entries[i], "application-action-entry-fixture-v1"));
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
                    !std::filesystem::exists(files.system) && !std::filesystem::exists(files.worlds[0]) &&
                    !std::filesystem::exists(files.worlds[1]),
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
        require(!std::filesystem::exists(files.system) && !std::filesystem::exists(files.worlds[0]) &&
                    !std::filesystem::exists(files.worlds[1]),
                "non-record exits do not write system or world files");
    }
    return checks;
}
