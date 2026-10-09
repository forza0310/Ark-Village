#include "ark/simulation/startup_application.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace ark::simulation;
int run_startup_system_records_tests(const std::filesystem::path &);
int run_startup_world_clear_score_tests();
int run_startup_application_replay_paths_checks(const std::filesystem::path &);
int run_startup_application_replay_state_checks(const std::filesystem::path &);
int check_startup_title_presentation();
int run_startup_title_replay_cli(int, const char **);
namespace {
int checks{};
void check(bool ok, const std::string &message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
void good(const std::string &e) { check(e.empty(), e); }
std::string bytes(const std::filesystem::path &p) {
    std::ifstream input(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}
struct Work {
    std::filesystem::path path;
    Work() {
        path = std::filesystem::current_path() / ("application-test-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        check(std::filesystem::create_directory(path), "独占项目内夹具目录");
    }
    ~Work() { std::error_code ec; std::filesystem::remove_all(path, ec); }
    StartupApplicationPaths paths(const std::string &prefix) const {
        return {path / (prefix + "-system.avr"),
                {path / (prefix + "-0.avr"), path / (prefix + "-1.avr")}};
    }
};
void title_and_files(Work &work) {
    auto paths = work.paths("title");
    StartupApplication app(paths, ref::WorldRandomStream::from_java_seed(1));
    good(app.error());
    check(!std::filesystem::exists(paths.system) && app.records().cash_peak == 0,
          "缺失只建立内存默认，不从5000G推导纪录或自动落盘");
    good(app.request_new_game(0));
    good(app.edit_sex(1));
    check(app.draft().main_character.name == "冒险花子", "未命名才切换默认名");
    good(app.edit_main_name("研究主角")); good(app.edit_village("研究村")); good(app.edit_sex(0));
    good(app.cancel_configuration()); good(app.request_new_game(0));
    check(app.draft().main_character.name == "研究主角", "取消重进及切性别保留自定义名");
    check(!app.edit_main_name(std::string("a\0b", 3)).empty(), "控制字节不能写入标题");
    good(app.start_game());
    check(!std::filesystem::exists(paths.worlds[0]), "开始不保存世界文件");
    const auto &s = app.world()->state();
    check(startup_world_human_profile(s, 0)->name == "研究主角" &&
          startup_world_human_profile(s, 1)->name == "丰田龟次郎" &&
          s.scene.world.world.ai.battle.actors.empty(), "定义0覆盖与定义1首访分离");
    check(app.handoff_random() && app.handoff_random()->cursor == 0 && s.scene.random.draws() == 0,
          "逻辑模式开始不增加标题抽数");
    good(app.save_world());
    const auto old_world = bytes(paths.worlds[0]);
    good(app.return_to_title()); good(app.request_new_game(0));
    check(app.page() == StartupApplicationPage::overwrite, "存在栏位先询问");
    good(app.answer_overwrite(false));
    check(bytes(paths.worlds[0]) == old_world, "取消覆盖保留文件");
    good(app.request_new_game(0)); good(app.answer_overwrite(true));
    good(app.edit_main_name("另一个主角")); good(app.start_game());
    check(bytes(paths.worlds[0]) == old_world, "开始新局也不提前覆盖旧世界");
    good(app.return_to_title()); good(app.load_world(0));
    check(startup_world_human_profile(app.world()->state(), 0)->name == "研究主角",
          "载入恢复已保存姓名，不取当前标题草稿");
    good(app.return_to_title()); good(app.request_new_game(1)); good(app.start_game());
    check(app.records().last_slot == 1 && load_startup_system_file(paths.system).records->last_slot == 1,
          "两栏共享同一系统最后栏位");
    check(bytes(paths.worlds[0]) == old_world, "另一新世界不改旧栏");
    good(app.return_to_title()); good(app.open_records());
    check(app.record_view().unit == "P" && app.record_view().value == 0, "标题第一页读系统最高分");
    good(app.turn_record_page(-1));
    check(app.record_view().unit == "G" && app.record_view().value == 0, "第二页仍不是起始资金");
    check(!app.turn_record_page(0).empty(), "越界方向显式拒绝");
}
void title_replay(Work &work) {
    StartupApplication a(work.paths("replay-a"), ref::WorldRandomStream::from_java_seed(42),
                         StartupApplicationMode::title_presentation);
    good(a.open_records());
    const auto snapshot = a.capture_title_replay();
    check(snapshot && snapshot->random.cursor > 0 && !snapshot->decorations.empty(),
          "标题显式请求消费共同随机并保存名单");
    const auto before = snapshot->random.cursor;
    (void)a.record_view(); (void)a.record_view();
    check(a.capture_title_replay()->random.cursor == before, "只读重画不抽取");
    StartupApplication b(work.paths("replay-b"), ref::WorldRandomStream::from_java_seed(9),
                         StartupApplicationMode::title_presentation);
    good(b.restore_title_replay(*snapshot));
    check(b.decorations() == a.decorations(), "独立标题控制器恢复装饰名单");
    auto bad = *snapshot; bad.decorations.push_back(9999);
    check(!b.restore_title_replay(bad).empty() && b.decorations() == snapshot->decorations,
          "坏引用不部分安装标题");
    StartupApplication logic(work.paths("replay-logic"), ref::WorldRandomStream::from_java_seed(42));
    check(!logic.restore_title_replay(*snapshot).empty(), "两种回放模式不混用");
    for (auto *app : {&a, &b}) {
        good(app->return_to_title()); good(app->request_new_game(0)); good(app->start_game());
        check(app->handoff_random()->cursor == before, "开始显式交接标题随机游标");
        good(app->update()); app->take_sound_requests();
    }
    check(startup_world_state_digest(a.world()->state()) == startup_world_state_digest(b.world()->state()),
          "恢复标题控制器后世界首轮严格复演");
}
void bad_files_and_rollback(Work &work) {
    auto paths = work.paths("failure");
    { std::ofstream file(paths.system); file << "broken"; }
    StartupApplication broken(paths, ref::WorldRandomStream::from_java_seed(1));
    check(!broken.error().empty() && !broken.request_new_game(0).empty(), "损坏系统不默认为缺失");
    check(bytes(paths.system) == "broken", "损坏原文件保留");
    paths = work.paths("locked");
    good(save_startup_system_file(paths.system, {}));
    StartupApplication locked(paths, ref::WorldRandomStream::from_java_seed(7));
    good(locked.request_new_game(1));
    const auto before = bytes(paths.system);
#ifdef _WIN32
    const auto handle = CreateFileW(paths.system.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    check(handle != INVALID_HANDLE_VALUE, "独占替换锁夹具");
    const auto failed = locked.start_game();
    check(CloseHandle(handle) != 0, "释放本轮文件锁");
    check(!failed.empty() && !locked.world() && locked.records().last_slot == 0 &&
          locked.page() == StartupApplicationPage::configure && bytes(paths.system) == before,
          "系统替换失败不留下世界/栏位/页面部分提交");
    good(locked.start_game());
    check(locked.handoff_random()->cursor == 0, "失败后重试不多抽随机");
#else
    (void)before;
#endif
    auto alias = work.paths("alias"); alias.worlds[0] = alias.system;
    StartupApplication same(alias, ref::WorldRandomStream{});
    check(!same.error().empty(), "系统与世界路径不能重叠");
#ifdef _WIN32
    alias = work.paths("case"); alias.worlds[0] = work.path / "CASE-SYSTEM.AVR";
    StartupApplication case_alias(alias, ref::WorldRandomStream{});
    check(!case_alias.error().empty(), "不存在文件也拒绝Windows大小写别名");
#endif
}
void natural_cash(Work &work) {
    const auto paths = work.paths("cash");
    StartupApplication app(paths, ref::WorldRandomStream::from_java_seed(1));
    good(app.request_new_game(0)); good(app.start_game());
    int frames{};
    int visible_oracle_rounds{};
    for (; frames < 10000 && app.records().cash_peak == 0; ++frames) {
        // 复用既有首访经营前缀，从同一旧世界并排验证Session完整消费者。
        // 出现真实可见人物后再比较8轮即可关闭额外oracle，不重复整条现金长轨迹。
        std::optional<StartupWorldRuntimeSession> oracle;
        if (visible_oracle_rounds < 8) {
            oracle = *app.world();
            const auto result = oracle->update();
            check(result.error == StartupWorldRuntimeError::none && result.candidate.has_value(),
                  "独立Session oracle真实更新成功并保留完整candidate");
        }
        good(app.update());
        if (oracle) {
            const auto expected_sounds = oracle->take_sound_requests();
            const auto actual_sounds = app.take_sound_requests();
            check(actual_sounds == expected_sounds, "应用与Session同轮原序声音一致");
            check(startup_world_session_digest(*app.world()) == startup_world_session_digest(*oracle),
                  "应用与Session完整轮末Owner/历史/随机/render缓存一致，frame=" + std::to_string(frames));
            const auto &s = oracle->state();
            const bool visible = std::any_of(s.scene.world.world.ai.battle.actors.begin(),
                s.scene.world.world.ai.battle.actors.end(), [&](const auto &entry) {
                    const auto seen = startup_world_actor_visible(s, entry.first);
                    return entry.second.kind == ref::ActorKind::human && seen && *seen;
                });
            if (visible) ++visible_oracle_rounds;
        }
        const auto &pages = app.world()->state().scripts.pages;
        const auto &p = pages.back();
        if (p.lifecycle != 4 && p.kind != ref::WorldScriptPageKind::scene &&
            p.legacy_page != 16 && p.legacy_page != 24 && p.legacy_page != 56 &&
            p.legacy_page != 57 && p.legacy_page != 97 && p.legacy_page != 98)
            good(app.acknowledge_page(p.id));
        app.take_sound_requests();
    }
    check(visible_oracle_rounds == 8, "真实可见人物的8轮已由独立Session完整oracle覆盖");
    check(app.records().cash_peak > 0, "真实新局首访经营产生资金纪录");
    const auto peak = app.records().cash_peak;
    check(load_startup_system_file(paths.system).records->cash_peak == peak,
          "现金Owner提交后独立系统已落盘");
    good(app.return_to_title()); good(app.request_new_game(1)); good(app.start_game());
    check(app.records().cash_peak == peak && app.world()->state().cash_peak == peak &&
          app.world()->state().scene.world.world.ai.accounting.funds() == 5000,
          "新世界沿用系统峰值，起始资金保持原值");
    std::cout << "natural-cash frames=" << frames << " peak=" << peak << '\n';
}
}
int main(int argc, const char **argv) {
    try {
        if (argc > 1) {
            if (std::string(argv[1]) == "title-background-replay-v2") return run_startup_title_replay_cli(argc, argv);
            throw std::runtime_error("未知应用测试模式");
        }
        Work work;
        checks += check_startup_title_presentation();
        checks += run_startup_application_replay_paths_checks(work.path);
        checks += run_startup_application_replay_state_checks(work.path);
        if (run_startup_system_records_tests(work.path) || run_startup_world_clear_score_tests()) return 1;
        title_and_files(work); title_replay(work); bad_files_and_rollback(work); natural_cash(work);
        std::cout << "应用所有权检查通过：" << checks << '\n';
        return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
