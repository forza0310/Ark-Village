#include "dungeon_village_prototype/startup_application.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace dungeon_village_prototype;
int run_startup_system_records_tests(const std::filesystem::path &);
int run_startup_world_clear_score_tests();
int run_startup_application_replay_paths_checks(const std::filesystem::path &);
int run_startup_application_replay_state_checks(const std::filesystem::path &);
int check_startup_title_presentation();
int run_startup_title_menu_checks();
int run_startup_title_replay_cli(int, const char **);
int run_startup_title_menu_file_replay_cli(int, const char **);
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
bool same_background(const StartupTitlePresentation &a, const StartupTitlePresentation &b) {
    if(a.l!=b.l || a.f132f!=b.f132f || a.s!=b.s || a.t!=b.t) return false;
    for(std::size_t i=0;i<a.slots.size();++i) {
        const auto &x=a.slots[i], &y=b.slots[i];
        if(x.active!=y.active || x.definition!=y.definition || x.x!=y.x || x.y!=y.y ||
           x.direction!=y.direction || x.age!=y.age) return false;
    }
    return true;
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
        const auto root = path / prefix;
        check(std::filesystem::create_directory(root), "每个应用使用独占显式存储根");
        return {root};
    }
};
void title_and_files(Work &work) {
    auto paths = work.paths("title");
    StartupApplication app(paths, ref::WorldRandomStream::from_java_seed(1));
    good(app.error());
    check(!std::filesystem::exists((paths.root / "system.avr")) && app.records().cash_peak == 0,
          "缺失只建立内存默认，不从5000G推导纪录或自动落盘");
    good(app.request_new_game(0));
    good(app.edit_sex(1));
    check(app.draft().main_character.name == "冒险花子", "未命名才切换默认名");
    good(app.edit_main_name("研究主角")); good(app.edit_village("研究村")); good(app.edit_sex(0));
    good(app.cancel_configuration()); good(app.request_new_game(0));
    check(app.draft().main_character.name == "研究主角", "取消重进及切性别保留自定义名");
    check(!app.edit_main_name(std::string("a\0b", 3)).empty(), "控制字节不能写入标题");
    good(app.start_game());
    check(app.records().save_directory == StartupSystemRecords{}.save_directory,
          "开始不发布四目录中的任何世界引用");
    const auto before_pending_save = bytes(paths.root / "system.avr");
    check(!app.save_world().empty() && app.records().save_directory == StartupSystemRecords{}.save_directory &&
          bytes(paths.root / "system.avr") == before_pending_save,
          "应用B0/G尚未领取时normal保存拒绝，不能借world当前队列为空绕过捕获边界");
    check(app.take_audio_requests() == std::vector<StartupAudioRequest>{
              {StartupAudioOperation::replace_bgm,0},{StartupAudioOperation::replace_bgm,1}} &&
              app.world()->state().sound_requests.empty(),
          "标题B0与实际新局G按序仅归应用队列，世界当前输出已移交");
    const auto &s = app.world()->state();
    check(startup_world_human_profile(s, 0)->name == "研究主角" &&
          startup_world_human_profile(s, 1)->name == "丰田龟次郎" &&
          s.scene.world.world.ai.battle.actors.empty(), "定义0覆盖与定义1首访分离");
    check(app.handoff_random() && app.handoff_random()->cursor == 0 && s.scene.random.draws() == 0,
          "逻辑模式开始不增加标题抽数");
    good(app.save_world());
    const auto saved_directory = app.records().save_directory;
    const auto &saved = saved_directory[0][1];
    check(saved.packed_date == 300 && saved.reference && saved.village == "研究村" && saved.cash == 5000,
          "真实保存生成第一栏手动目录，使用原零基日期及实际资金");
    std::string digest;
    for (auto value : saved.reference->sha256) {
        digest += "0123456789abcdef"[value >> 4];
        digest += "0123456789abcdef"[value & 15];
    }
    const auto world_file = paths.root / "worlds" / ("world-" + digest + ".avrs");
    const auto old_world = bytes(world_file);
    check(old_world.size() == saved.reference->bytes && !old_world.empty(),
          "四目录引用精确指向按摘要命名的不可变世界字节");
    const auto observed = load_startup_application_storage(paths.root);
    check(observed.snapshot.has_value(), "保存后系统完整可读");
    const auto view = capture_startup_application_storage(paths.root, *observed.snapshot);
    check(view.view && view.view->blobs.size() == 1 && view.view->blobs[0].reference == *saved.reference &&
          std::string(view.view->blobs[0].bytes.begin(), view.view->blobs[0].bytes.end()) == old_world,
          "规范文件视图复核摘要、长度及唯一被引用blob，不只检查文件存在");
    good(app.return_to_title()); good(app.request_new_game(0));
    check(app.page() == StartupApplicationPage::overwrite, "存在栏位先询问");
    good(app.answer_overwrite(false));
    check(bytes(world_file) == old_world && app.records().save_directory == saved_directory &&
          app.title_menu().save_menu && app.title_menu().save_menu->selection == 1,
          "否决覆盖保留raw20父选择、四目录和原字节");
    check(app.take_sound_requests() == std::vector<int>{0}, "世界重入标题仅一次B0，子页返回不重播");
    StartupTitleMenuRequest cancel; cancel.keys.back = true;
    good(app.apply_title_request(app.title_page_id(), cancel));
    cancel = {}; cancel.kind = StartupTitleMenuRequestKind::consume_return;
    good(app.apply_title_request(app.title_page_id(), cancel));
    good(app.request_new_game(0)); good(app.answer_overwrite(true));
    good(app.edit_main_name("另一个主角")); good(app.start_game());
    check(bytes(world_file) == old_world && app.records().save_directory == saved_directory,
          "开始新局也不提前覆盖旧世界或重写四目录");
    check(app.take_sound_requests() == std::vector<int>{1}, "仅实际开始追加G，覆盖询问不制造B0");
    good(app.return_to_title()); good(app.load_world(0));
    check(startup_world_human_profile(app.world()->state(), 0)->name == "研究主角",
          "载入恢复已保存姓名，不取当前标题草稿");
    check(app.take_sound_requests() == std::vector<int>({0,1}), "标题重入后真实加载按序B0/G");
    good(app.return_to_title()); good(app.request_new_game(1)); good(app.start_game());
    check(app.records().last_slot == 1 && load_startup_system_file((paths.root / "system.avr")).records->last_slot == 1,
          "两栏共享同一系统最后栏位");
    check(bytes(world_file) == old_world && app.records().save_directory == saved_directory,
          "另一新世界不改旧栏及另外三条目录");
    check(app.take_sound_requests() == std::vector<int>({0,1}), "另一栏真实开始同样只追加一次B0/G");
    good(app.return_to_title()); good(app.open_records());
    check(app.take_sound_requests() == std::vector<int>{0}, "纪录子页不重新初始化标题声音");
    check(app.record_view().unit == "P" && app.record_view().value == 0, "标题第一页读系统最高分");
    good(app.turn_record_page(-1));
    check(app.record_view().unit == "G" && app.record_view().value == 0, "第二页仍不是起始资金");
    check(!app.turn_record_page(0).empty(), "越界方向显式拒绝");

    // 在同一已核真实blob上制造另一存储写者的目录隐藏，不造无内容的演示目录。
    good(app.return_to_title()); good(app.load_world(0));
    const auto world_before_refresh = startup_world_session_digest(*app.world());
    const auto random_before_refresh = app.world()->state().scene.random.snapshot();
    check(!app.refresh_title_storage().empty() &&
          startup_world_session_digest(*app.world()) == world_before_refresh && app.has_pending_audio_requests(),
          "活动世界拒绝刷新标题存储，保留世界和实际激活输出");
    good(app.return_to_title()); good(app.request_new_game(0)); good(app.answer_overwrite(false));
    StartupTitleMenuRequest hide;
    hide.kind = StartupTitleMenuRequestKind::touch_up; hide.component = 9; hide.value = 2;
    good(app.apply_title_request(app.title_page_id(), hide));
    hide.component = 3; hide.value = 0;
    good(app.apply_title_request(app.title_page_id(), hide));
    hide = {}; hide.kind = StartupTitleMenuRequestKind::consume_return;
    good(app.apply_title_request(app.title_page_id(), hide));
    const auto old_root = app.title_menu().root_id;
    const auto old_menu = app.title_menu().save_menu->id;
    const auto old_next = app.title_menu().next_id;
    const auto background_before_refresh = app.title_presentation();
    const auto old_revision = app.records().revision;
    const auto other = load_startup_application_storage(paths.root);
    check(other.snapshot.has_value(), "独立写者读取同一真实系统身份");
    const auto hidden = hide_startup_application_slot(paths.root, *other.snapshot,
        other.snapshot->records, 0, StartupSaveKind::manual);
    check(hidden.snapshot && hidden.snapshot->records.save_directory[0][1].packed_date == -1 &&
          hidden.snapshot->records.save_directory[0][1].reference == saved.reference &&
          bytes(world_file) == old_world, "独立隐藏只改目录可见性并保留同一blob引用及字节");
    check(!app.apply_title_request(app.title_page_id(), hide).empty() &&
          app.title_menu().save_menu && app.title_menu().save_menu->returned &&
          app.title_menu().save_menu->id == old_menu && app.records().revision == old_revision,
          "目录写冲突保留待返回raw20，不自动重试旧隐藏意图");
    const auto published = bytes(paths.root / "system.avr");
    { std::ofstream bad_system(paths.root / "system.avr",std::ios::binary|std::ios::trunc);
      bad_system << "refresh-fixture-broken"; }
    check(!app.refresh_title_storage().empty() && app.title_menu().root_id == old_root &&
          app.title_menu().save_menu && app.title_menu().save_menu->id == old_menu &&
          app.title_menu().save_menu->returned && app.title_menu().save_menu->result == 2 &&
          app.title_menu().save_menu->selection == 2 && app.title_menu().next_id == old_next &&
          same_background(app.title_presentation(), background_before_refresh) &&
          app.records().revision == old_revision &&
          startup_world_session_digest(*app.world()) == world_before_refresh &&
          bytes(paths.root / "system.avr") == "refresh-fixture-broken",
          "刷新坏系统失败完整保留旧应用与待返回页，不重写坏文件");
    auto failed_refresh = app;
    check(failed_refresh.take_sound_requests() == std::vector<int>({1,0}),
          "写冲突及刷新失败均保留原真实G/B0队列");
    { std::ofstream restore_system(paths.root / "system.avr",std::ios::binary|std::ios::trunc);
      restore_system.write(published.data(),static_cast<std::streamsize>(published.size()));
      check(static_cast<bool>(restore_system), "恢复本测试自己故障注入前的独立写者字节"); }
    good(app.refresh_title_storage());
    check(app.title_menu().root_id == old_next && app.title_menu().next_id == old_next+1 &&
          same_background(app.title_presentation(), background_before_refresh) && !app.title_menu().save_menu &&
          !app.title_menu().confirmation && app.records().revision == hidden.snapshot->records.revision &&
          app.records().save_directory == hidden.snapshot->records.save_directory &&
          startup_world_session_digest(*app.world()) == world_before_refresh &&
          bytes(paths.root / "system.avr") == published && bytes(world_file) == old_world,
          "显式刷新采用新四目录并退休旧页，不改世界/随机/历史、不重写文件");
    check(!app.apply_title_request(old_root, hide).empty() &&
          !app.apply_title_request(old_menu, hide).empty() &&
          app.take_sound_requests() == std::vector<int>({1,0}) && !app.has_pending_audio_requests(),
          "刷新后旧根和旧子页ID均拒绝，原待发输出仍恰一次可领取");
    good(app.request_new_game(1)); good(app.start_game());
    check(app.handoff_random()->cursor == random_before_refresh.cursor &&
          app.handoff_random()->engine_state == random_before_refresh.engine_state &&
          app.handoff_random()->tape == random_before_refresh.tape &&
          app.handoff_random()->tape_mode == random_before_refresh.tape_mode &&
          app.take_sound_requests() == std::vector<int>{1},
          "刷新后的真正开始交接原完整随机；刷新不暗抽、不额外初始化B0");
}
void title_replay(Work &work) {
    StartupApplication a(work.paths("replay-a"), ref::WorldRandomStream::from_java_seed(42),
                         StartupApplicationMode::title_presentation);
    check(!a.capture_title_replay(), "未领取标题B0不能捕获标题快照");
    check(a.take_sound_requests() == std::vector<int>{0}, "健康冷标题产生B0");
    good(a.open_records());
    const auto snapshot = a.capture_title_replay();
    check(snapshot && snapshot->random.cursor > 0 && !snapshot->decorations.empty(),
          "标题显式请求消费共同随机并保存名单");
    const auto before = snapshot->random.cursor;
    (void)a.record_view(); (void)a.record_view();
    check(a.capture_title_replay()->random.cursor == before, "只读重画不抽取");
    StartupApplication b(work.paths("replay-b"), ref::WorldRandomStream::from_java_seed(9),
                         StartupApplicationMode::title_presentation);
    check(b.take_sound_requests() == std::vector<int>{0}, "恢复目标先显式领取自己的冷标题B0");
    good(b.restore_title_replay(*snapshot));
    check(b.take_sound_requests().empty(), "精确标题恢复不重新初始化或重播B0");
    check(b.decorations() == a.decorations(), "独立标题控制器恢复装饰名单");
    auto bad = *snapshot; bad.decorations.push_back(9999);
    check(!b.restore_title_replay(bad).empty() && b.decorations() == snapshot->decorations,
          "坏引用不部分安装标题");
    StartupApplication logic(work.paths("replay-logic"), ref::WorldRandomStream::from_java_seed(42));
    check(logic.take_sound_requests() == std::vector<int>{0}, "逻辑模式冷构造同样有显式B0输出");
    check(!logic.restore_title_replay(*snapshot).empty(), "两种回放模式不混用");
    for (auto *app : {&a, &b}) {
        good(app->return_to_title()); good(app->request_new_game(0)); good(app->start_game());
        check(app->handoff_random()->cursor == before, "开始显式交接标题随机游标");
        good(app->update()); app->take_sound_requests();
    }
    check(startup_world_state_digest(a.world()->state()) == startup_world_state_digest(b.world()->state()),
          "恢复标题控制器后世界首轮严格复演");
    // 复用已完成首轮的真实世界建立两种合法文件，不为测试阻挡而传入坏档。
    good(a.save_world());
    const auto entry = work.path / "title-input-guards.avrs";
    StartupWorldSaveMetadata metadata;
    metadata.purpose = StartupWorldSavePurpose::replay;
    metadata.controller_id = "title-input-guards-v1";
    metadata.controller_state = {1};
    good(save_startup_world_file(entry, *a.world(), metadata).error);
    good(a.return_to_title());
    const auto initial_background = a.title_presentation();
    const auto random = a.world()->state().scene.random;
    const auto oracle = prepare_startup_title_update(initial_background, random,
        {StartupTitleAdmission::top_lifecycle_ready, true});
    check(oracle.candidate.has_value(), "标题阻挡后续对照使用同旧随机的独立纯更新");
    good(a.request_new_game(0));
    const auto rejected_parent = [&] {
        const auto page = a.page();
        const auto id = a.title_page_id();
        const auto world = startup_world_session_digest(*a.world());
        auto before_output = a;
        const auto pending = before_output.take_audio_requests();
        const auto blocked = a.advance_title_background({StartupTitleAdmission::top_lifecycle_ready,true});
        check(!blocked.error.empty() && blocked.random_draws == 0 &&
              same_background(a.title_presentation(), initial_background),
              "raw20/raw1及待消费返回遮挡父标题，不推进背景或取得随机准入");
        check(!a.load_world_replay(entry, metadata.controller_id).empty() && a.page() == page &&
              a.title_page_id() == id && startup_world_session_digest(*a.world()) == world,
              "合法世界回放也不能绕过尚存标题子页，保留页面和原世界");
        auto after_output = a;
        check(after_output.take_audio_requests() == pending,
              "父标题拒绝不提前发布G、不清掉未领取B0");
    };
    rejected_parent(); // 活动raw1。
    StartupTitleMenuRequest request; request.keys.confirm = true;
    good(a.apply_title_request(a.title_page_id(), request));
    rejected_parent(); // raw1已返回但尚未由raw20消费。
    request = {}; request.kind = StartupTitleMenuRequestKind::consume_return;
    good(a.apply_title_request(a.title_page_id(), request));
    rejected_parent(); // 活动raw20。
    request = {}; request.keys.back = true;
    good(a.apply_title_request(a.title_page_id(), request));
    rejected_parent(); // raw20已返回但尚未由根消费。
    request = {}; request.kind = StartupTitleMenuRequestKind::consume_return;
    good(a.apply_title_request(a.title_page_id(), request));
    const auto advanced = a.advance_title_background({StartupTitleAdmission::top_lifecycle_ready,true});
    good(advanced.error);
    check(same_background(a.title_presentation(), oracle.candidate->state) &&
          advanced.random_draws == oracle.candidate->random_draws,
          "所有拒绝后首次真实标题更新与未插入失败请求的独立oracle相同");
    good(a.request_new_game(1)); good(a.start_game());
    const auto expected_random = oracle.candidate->random.snapshot();
    check(a.handoff_random()->cursor == expected_random.cursor &&
          a.handoff_random()->engine_state == expected_random.engine_state &&
          a.handoff_random()->tape == expected_random.tape &&
          a.handoff_random()->tape_mode == expected_random.tape_mode,
          "后续实际新局交接完整随机证明拒绝期间没有暗抽");
    check(a.take_sound_requests() == std::vector<int>({0,1}), "失败请求链不改变真实B0/G输出次序");
    good(a.return_to_title()); good(a.load_world_replay(entry, metadata.controller_id));
    check(a.take_sound_requests() == std::vector<int>({0,1}), "父子页收齐后同一合法回放源可真实激活");
}
void bad_files_and_rollback(Work &work) {
    auto paths = work.paths("failure");
    { std::ofstream file((paths.root / "system.avr")); file << "broken"; }
    StartupApplication broken(paths, ref::WorldRandomStream::from_java_seed(1));
    check(!broken.error().empty() && !broken.request_new_game(0).empty(), "损坏系统不默认为缺失");
    check(bytes((paths.root / "system.avr")) == "broken", "损坏原文件保留");
    paths = work.paths("locked");
    good(save_startup_system_file((paths.root / "system.avr"), {}));
    StartupApplication locked(paths, ref::WorldRandomStream::from_java_seed(7));
    good(locked.request_new_game(1));
    const auto before = bytes((paths.root / "system.avr"));
#ifdef _WIN32
    const auto handle = CreateFileW((paths.root / "system.avr").c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    check(handle != INVALID_HANDLE_VALUE, "独占替换锁夹具");
    const auto failed = locked.start_game();
    check(CloseHandle(handle) != 0, "释放本轮文件锁");
    check(!failed.empty() && !locked.world() && locked.records().last_slot == 0 &&
          locked.page() == StartupApplicationPage::configure && bytes((paths.root / "system.avr")) == before,
          "系统替换失败不留下世界/栏位/页面部分提交");
    auto failed_copy = locked;
    check(failed_copy.take_sound_requests() == std::vector<int>{0}, "失败保留未领取B0而不提前发布G");
    good(locked.start_game());
    check(locked.handoff_random()->cursor == 0, "失败后重试不多抽随机");
    check(locked.take_sound_requests() == std::vector<int>({0,1}), "成功重试保序领取原B0和唯一G");
#else
    (void)before;
#endif
    // 单root接口已不能传相同system/world参数；风险覆盖转到真实固定对象别名。
    const auto alias = work.paths("alias");
    const auto alias_system = alias.root / "system.avr";
    good(save_startup_system_file(alias_system, {}));
    std::filesystem::create_hard_link(alias_system, alias.root / "system-alias.avr");
    const auto aliased_bytes = bytes(alias_system);
    StartupApplication same(alias, ref::WorldRandomStream{});
    check(!same.error().empty() && bytes(alias_system) == aliased_bytes,
          "固定system存在硬链接别名时拒绝且不改共享字节");
    StartupApplication file_root({alias_system}, ref::WorldRandomStream{});
    check(!file_root.error().empty(), "显式存储根不能是普通文件");
#ifdef _WIN32
    const auto named = work.paths("case");
    StartupApplication case_alias({named.root.string() + "."}, ref::WorldRandomStream{});
    check(!case_alias.error().empty(), "拒绝Windows尾点别名，不规范化到另一个已授权根");
#endif
}
void natural_cash(Work &work) {
    const auto paths = work.paths("cash");
    StartupApplication app(paths, ref::WorldRandomStream::from_java_seed(1));
    good(app.request_new_game(0)); good(app.start_game());
    check(app.take_sound_requests() == std::vector<int>({0,1}), "自然新局先领取标题B0与真实激活G");
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
    check(load_startup_system_file((paths.root / "system.avr")).records->cash_peak == peak,
          "现金Owner提交后独立系统已落盘");
    good(app.return_to_title()); good(app.request_new_game(1)); good(app.start_game());
    check(app.records().cash_peak == peak && app.world()->state().cash_peak == peak &&
          app.world()->state().scene.world.world.ai.accounting.funds() == 5000,
          "新世界沿用系统峰值，起始资金保持原值");
    check(app.take_sound_requests() == std::vector<int>({0,1}), "现金场景退出前领取新标题及新局声音");
    std::cout << "natural-cash frames=" << frames << " peak=" << peak << '\n';
}
}
int main(int argc, const char **argv) {
    try {
        if (argc > 1) {
            if (std::string(argv[1]) == "title-background-replay-v2") return run_startup_title_replay_cli(argc, argv);
            if (std::string(argv[1]) == "title-menu-files-v1") return run_startup_title_menu_file_replay_cli(argc, argv);
            throw std::runtime_error("未知应用测试模式");
        }
        Work work;
        checks += check_startup_title_presentation();
        checks += run_startup_title_menu_checks();
        checks += run_startup_application_replay_paths_checks(work.path);
        checks += run_startup_application_replay_state_checks(work.path);
        if (run_startup_system_records_tests(work.path) || run_startup_world_clear_score_tests()) return 1;
        title_and_files(work); title_replay(work); bad_files_and_rollback(work); natural_cash(work);
        std::cout << "应用所有权检查通过：" << checks << '\n';
        return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
