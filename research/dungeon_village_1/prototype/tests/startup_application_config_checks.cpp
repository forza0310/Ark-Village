// 设置的应用主责：即时权威、单队列、文件提交与精确恢复；不重复纯规则矩阵。
#include "dungeon_village_prototype/startup_application_replay.hpp"
#include "dungeon_village_prototype/startup_world_menu.hpp"
#include "startup_application_replay_wire.hpp"
#include "startup_persistence_bytes.hpp"
#include "startup_world_codec.hpp"
#include "../src/startup_window_host.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <type_traits>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace {
using namespace dungeon_village_prototype;
namespace fs = std::filesystem;
struct Checks {
    int count{};
    void operator()(bool ok, const char *message) {
        ++count;
        if (!ok) throw std::runtime_error(std::string("应用设置：") + message);
    }
    void good(const std::string &error) {
        ++count;
        if (!error.empty()) throw std::runtime_error("应用设置：" + error);
    }
};
struct Work {
    fs::path root;
    Work(Checks &c, const fs::path &parent) : root(parent / "application-config") {
        c(fs::create_directory(root), "独占设置夹具根");
    }
    ~Work() { std::error_code error; fs::remove_all(root, error); }
    StartupApplicationPaths paths(Checks &c, const std::string &name) const {
        const auto value = root / name;
        c(fs::create_directory(value), "独占设置存储根");
        return {value};
    }
};
std::string bytes(const fs::path &file) {
    std::ifstream input(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}
const ref::WorldScriptPage *top(const StartupApplication &app) {
    if (!app.world()) return nullptr;
    const auto &pages = app.world()->state().scripts.pages;
    for (auto p = pages.rbegin(); p != pages.rend(); ++p)
        if (p->lifecycle != 4) return &*p;
    return nullptr;
}
StartupApplicationReplayMetadata metadata() {
    StartupApplicationReplayMetadata value;
    value.controller_id = "application-config-contract-v1";
    value.controller_state = {1};
    return value;
}
std::string validate(const StartupApplication &app, const StartupApplicationReplayMetadata &m) {
    return app.error().empty() && m.controller_id == "application-config-contract-v1" &&
        m.controller_state == std::vector<std::uint8_t>{1} && m.next_frame == 0 &&
        m.next_command == 0 && m.extensions.empty() ? std::string{} : "设置夹具Driver不符";
}
// 仅比较一次性输出，不执行音量、GPU或资源音。字段签名保留variant类型和原序。
std::vector<std::string> outputs(StartupApplication &app) {
    std::vector<std::string> result;
    for (const auto &request : app.take_presentation_requests()) std::visit([&](const auto &v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, StartupAudioRequest>)
            result.push_back("audio:" + std::to_string(static_cast<int>(v.operation)) + ":" + std::to_string(v.id));
        else if constexpr (std::is_same_v<T, SteamConfigVolume>)
            result.push_back("volume:" + std::to_string(v.channel) + ":" + std::to_string(v.volume));
        else if constexpr (std::is_same_v<T, SteamConfigMute>)
            result.push_back("mute:" + std::to_string(v.channel) + ":" + std::to_string(v.mute));
        else if constexpr (std::is_same_v<T, SteamConfigLinearFilter>)
            result.push_back("filter:" + std::to_string(v.enabled));
        else if constexpr (std::is_same_v<T, SteamConfigResourceSound>)
            result.push_back("resource:" + std::to_string(v.index));
        else if constexpr (std::is_same_v<T, SteamConfigSetterAction>)
            result.push_back("setter:" + std::to_string(static_cast<int>(v)));
        else result.push_back("application:" + std::to_string(static_cast<int>(v)));
    }, request);
    return result;
}
auto observation(const StartupApplication &source) {
    auto copy = source;
    auto requests = outputs(copy);
    return std::make_pair(startup_application_replay_digest(copy, metadata(), validate), requests);
}
void title_and_environment(Checks &c, Work &work) {
    auto files = work.paths(c, "initial");
    StartupSystemRecords stored;
    stored.settings = {0, 5, std::numeric_limits<std::int32_t>::min()};
    c.good(save_startup_system_file(files.root / "system.avr", stored));
    StartupApplication app(files, ref::WorldRandomStream::from_java_seed(1));
    c.good(app.error());
    const auto requests = app.take_presentation_requests();
    // Steam标题Init及ReflectAudio的独立常量oracle；不调用setter计划生成期望。
    c(requests.size() == 5, "标题初始化恰五项统一输出");
    const auto *v0 = std::get_if<SteamConfigVolume>(&requests[0]);
    const auto *m0 = std::get_if<SteamConfigMute>(&requests[1]);
    const auto *v1 = std::get_if<SteamConfigVolume>(&requests[2]);
    const auto *m1 = std::get_if<SteamConfigMute>(&requests[3]);
    const auto *bgm = std::get_if<StartupAudioRequest>(&requests[4]);
    c(v0 && v0->channel == 0 && v0->volume == 0 && m0 && m0->channel == 0 && m0->mute &&
      v1 && v1->channel == 1 && v1->volume == 255 && m1 && m1->channel == 1 && !m1->mute &&
      bgm && bgm->operation == StartupAudioOperation::replace_bgm && bgm->id == 0,
      "原序为BGM音量/静音、SE音量/静音、B0，保留0与255边界");
    c(app.take_audio_requests().empty() && !app.has_pending_presentation_requests(), "统一队列只领取一次");
    c.good(app.request_new_game(0));
    const auto before = observation(app);
    c(!app.start_game().empty() && observation(app) == before && !app.world(),
      "缺客户区环境显式拒绝，不产生世界或随机/页面修改");
    c(!app.set_new_game_environment({0, 640}).empty() &&
      !app.set_new_game_environment({640, -1}).empty() && observation(app) == before,
      "非正客户区拒绝且不留下部分环境");
    int scenario{};
    for (const auto &entry : std::array<std::array<int, 3>, 4>{{
             {{480, 640, 1}}, {{640, 480, 1}}, {{481, 481, 0}}, {{1, 640, 1}}}}) {
        const auto branch_files = work.paths(c, "environment-" + std::to_string(scenario++));
        c.good(save_startup_system_file(branch_files.root / "system.avr", stored));
        StartupApplication branch(branch_files, ref::WorldRandomStream::from_java_seed(1));
        c.good(branch.request_new_game(0));
        c.good(branch.set_new_game_environment({entry[0], entry[1]}));
        c.good(branch.start_game());
        c(branch.world()->state().quality_setting == entry[2] &&
          branch.world()->state().scene.speed_setting == 0 &&
          branch.world()->state().system_completion_mode == std::numeric_limits<std::int32_t>::min(),
          "任一维<=480使用P12=1，否则0；P13=0且J9完整signed32交接");
        c(!branch.set_new_game_environment({640, 640}).empty(), "世界阶段不能重置新局环境");
        c(branch.records().settings == stored.settings,
          "新局保留完整系统设置，不将高速值归一化");
    }
}
void ready_world(Checks &c, StartupApplication &app) {
    c.good(app.request_new_game(0)); c.good(app.set_new_game_environment({640, 640}));
    c.good(app.start_game()); app.take_presentation_requests();
    // 复用既有首次到访短前缀，不注入资金、人物、日期或奖励。
    for (int n = 0; n < 2200; ++n) {
        c.good(app.update());
        const auto *p = top(app);
        c(p != nullptr, "真实短前缀保留活动页面");
        if (p->kind != ref::WorldScriptPageKind::scene && p->legacy_page != 16 &&
            p->legacy_page != 24 && p->legacy_page != 56 && p->legacy_page != 57 &&
            p->legacy_page != 97 && p->legacy_page != 98) c.good(app.acknowledge_page(p->id));
        app.take_presentation_requests();
        const auto &s = app.world()->state();
        if (!app.world()->checkpoints().empty() && !s.scene.world.world.ai.human_order.empty() &&
            s.scripts.pages.size() == 1 && s.scene.scene_state == 0) return;
    }
    c(false, "真实初始菜单前缀须在2200轮内收口");
}
std::uint64_t enter_config(Checks &c, StartupApplication &app) {
    c.good(app.open_main_menu()); c.good(app.update());
    for (const int tag : {6, 21}) {
        const auto *p = top(app);
        c(p != nullptr, "菜单入口存在");
        const auto id = p->id;
        const auto view = inspect_startup_world_menu_page(app.world()->state(), id);
        c(view.has_value(), "真实菜单已初始化");
        const auto row = std::find(view->tags.begin(), view->tags.end(), tag);
        c(row != view->tags.end(), "真实菜单包含系统/设置标签");
        StartupWorldMenuInput input; input.select_row = static_cast<int>(row - view->tags.begin());
        c.good(app.input_menu_page(id, input));
        input = {}; input.confirm = true; c.good(app.input_menu_page(id, input));
        c.good(app.update());
    }
    const auto *p = top(app);
    c(p && p->legacy_page == 12 && p->lifecycle == 2, "系统10/tag21到真实活动raw12");
    app.take_presentation_requests();
    return p->id;
}
void live_return_and_restore(Checks &c, Work &work) {
    const auto files = work.paths(c, "live");
    StartupApplication app(files, ref::WorldRandomStream::from_java_seed(1));
    ready_world(c, app);
    const auto id = enter_config(c, app);
    const auto disk = bytes(files.root / "system.avr");
    const auto saved = app.records().settings;
    const auto clock = app.world()->state().page_counters.at(id);
    {
        // 窗口只转发一次整轮；检查接线风险，不重复底层setter／格式矩阵。
        StartupWindowHost host(app);
        c(host.set_config_selection(id, 2) == StartupWorldRuntimeError::none,
          "窗口设置行选择交唯一应用Owner");
        StartupConfigInput right; right.right_repeat = true;
        c(host.update(false, nullptr, &right).committed &&
          host.state().page_counters.at(id) == (clock + 2) % 20 &&
          host.application()->live_settings().bgm == saved.bgm + 1,
          "窗口整轮仅执行公共及专属两次加钟，不叠加默认Update");
        c(host.take_presentation_requests().size() == 3 &&
          host.take_presentation_requests().empty(), "窗口完整领取typed输出恰一次");
        StartupWindowHost standalone(host.runtime());
        c(standalone.set_config_value(id, 2, 1) == StartupWorldRuntimeError::invalid_page,
          "独立Session缺跨局权威，窗口明确拒绝设置");
        bool rejected{};
        try { (void)standalone.update(false, nullptr, &right); }
        catch (const std::runtime_error &) { rejected = true; }
        c(rejected, "独立Session不能把设置输入当默认Update成功");
    }
    c.good(app.set_config_value(id, 3, 4));
    c(app.settings_dirty() && app.live_settings().se == 4 && app.records().settings == saved &&
      bytes(files.root / "system.avr") == disk && app.world()->state().page_counters.at(id) == clock,
      "Touch音量即时生效但不落盘、不增加页面钟");
    const auto pending = work.root / "pending.avra";
    c(!save_startup_application_replay(work.root, pending, app, metadata(), validate).empty() &&
      !fs::exists(pending) && app.has_pending_presentation_requests(),
      "尚未领取的typed设置输出阻止快照且不被错误捕获消费");
    auto legacy = app;
    c(legacy.take_audio_requests().empty() && !legacy.has_pending_presentation_requests(),
      "旧音频接口消费全队列，资源槽7不伪装普通声音7");
    const auto requests = app.take_presentation_requests();
    c(requests.size() == 4 && std::holds_alternative<SteamConfigVolume>(requests[0]) &&
      std::get<SteamConfigVolume>(requests[0]).channel == 1 && std::get<SteamConfigVolume>(requests[0]).volume == 204 &&
      std::holds_alternative<SteamConfigMute>(requests[1]) && !std::get<SteamConfigMute>(requests[1]).mute &&
      std::holds_alternative<SteamConfigSetterAction>(requests[2]) &&
      std::get<SteamConfigSetterAction>(requests[2]) == SteamConfigSetterAction::reflect_audio &&
      std::holds_alternative<SteamConfigResourceSound>(requests[3]) && std::get<SteamConfigResourceSound>(requests[3]).index == 7,
      "SE更改原序输出音量204、取消静音、ReflectAudio、typed资源7");
    c(!app.has_pending_audio_requests() && app.take_presentation_requests().empty(), "表现和旧音频共用唯一空闸门");
    c.good(app.set_config_selection(id, 3));
    const auto meta = metadata();
    const auto snapshot = work.root / "config.avra";
    c.good(save_startup_application_replay(work.root, snapshot, app, meta, validate));
    const auto source_digest = startup_application_replay_digest(app, meta, validate);
    auto restored = app;
    auto restored_meta = meta;
    const auto isolated = work.root / "restored";
    c.good(restore_startup_application_replay(snapshot, isolated, meta.controller_id, restored, restored_meta, validate));
    const auto restored_view = inspect_startup_world_config_page(restored.world()->state(), id);
    c(startup_application_replay_digest(restored, restored_meta, validate) == source_digest &&
      restored.settings_dirty() && restored.live_settings().se == 4 && restored.records().settings == saved &&
      restored_view && restored_view->selection == 3 &&
      !restored.has_pending_presentation_requests() && bytes(isolated / "system.avr") == disk,
      "raw12及脏live/原磁盘设置精确恢复，不重新初始化或重发表现");
    StartupConfigInput back; back.right_repeat = true; back.back_soft_label = true;
#ifdef _WIN32
    const auto before = observation(app);
    const auto handle = CreateFileW((files.root / "system.avr").c_str(), GENERIC_READ, FILE_SHARE_READ,
                                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    c(handle != INVALID_HANDLE_VALUE, "锁定本轮独占系统夹具");
    const auto failed = app.input_config_page(id, back);
    const auto closed = CloseHandle(handle);
    c(closed != 0, "释放系统夹具锁");
    c(!failed.empty() && observation(app) == before && bytes(files.root / "system.avr") == disk,
      "返回同轮更改写档失败时保留完整世界、钟、选择、live、输出、随机和旧磁盘");
#endif
    c.good(app.input_config_page(id, back));
    const auto persisted = load_startup_system_file(files.root / "system.avr");
    c(!app.settings_dirty() && app.live_settings().se == 5 && app.records().settings.se == 5 &&
      persisted.records && persisted.records->settings.se == 5 && top(app)->legacy_page != 12,
      "返回同轮先4到5，再保存新值并Pop，不丢本轮更改");
    const auto final = app.take_presentation_requests();
    c(final.size() == 5 && std::holds_alternative<SteamConfigResourceSound>(final[3]) &&
      std::holds_alternative<StartupApplicationPresentationAction>(final[4]) &&
      std::get<StartupApplicationPresentationAction>(final[4]) == StartupApplicationPresentationAction::reset_offscreen,
      "成功联合发布setter输出后仅追加一次ResetOffscreen");
}
// 只替换容器内完整Owner分区；日期标量不改变编码长度，审计历史及Driver保持原样。
// 最终仍沿正式文件读取器做全部来源、引用、预算及跨域校验，不打开可写Owner入口。
void write_calendar_condition(Checks &c, const fs::path &file,
                              const StartupWorldRuntimeSession &source) {
    namespace wire = application_replay_fixture;
    StartupWorldSaveMetadata m;
    m.purpose = StartupWorldSavePurpose::replay;
    m.controller_id = "calendar-endgame-condition-v1";
    m.controller_state = {1};
    auto image = persistence_detail::encode_world_session_bytes(source, m);
    auto condition = source.state();
    condition.scene.calendar = {15, 2, 3, 10773, 10746, 399};
    const auto owner = persistence_detail::encode_state(condition);
    std::size_t at = 20;
    (void)wire::text(image, at); (void)wire::text(image, at);
    const auto count = wire::number(image, at, 4);
    int replaced{};
    for (std::uint64_t n = 0; n < count; ++n) {
        const auto id = wire::number(image, at, 4);
        (void)wire::number(image, at, 4); (void)wire::number(image, at, 4);
        const auto size = wire::number(image, at, 8);
        const auto hash_at = at + 4;
        const auto hash = wire::text(image, at);
        c(at <= image.size() - 64 && size <= image.size() - 64 - at,
          "条件世界分区范围");
        if (id == 2) {
            c(size == owner.size() && hash.size() == 64, "日期条件仅替换等长完整Owner");
            const auto digest = dungeon_village_tools::sha256_hex(owner);
            std::copy(digest.begin(), digest.end(), image.begin() + static_cast<std::ptrdiff_t>(hash_at));
            std::copy(owner.begin(), owner.end(), image.begin() + static_cast<std::ptrdiff_t>(at));
            ++replaced;
        }
        at += static_cast<std::size_t>(size);
    }
    c(replaced == 1 && at == image.size() - 64, "仅有一个当前Owner且完整消费容器");
    wire::resign(image);
    std::ofstream out(file, std::ios::binary);
    out.write(reinterpret_cast<const char *>(image.data()), static_cast<std::streamsize>(image.size()));
    c(bool(out), "写入日期条件世界文件");
}
bool has_clear(const StartupApplication &app) {
    return std::any_of(app.world()->state().scripts.pages.begin(), app.world()->state().scripts.pages.end(),
        [](const auto &p) { return p.legacy_page == 17 && p.lifecycle != 4; });
}
void calendar_high_speed_transaction(Checks &c, Work &work) {
    const auto seed_files = work.paths(c, "calendar-seed");
    StartupApplication seed(seed_files, ref::WorldRandomStream::from_java_seed(1));
    ready_world(c, seed);
    c(!has_clear(seed), "真实短前缀尚无计分页");
    const auto condition = work.root / "calendar-condition.avr";
    write_calendar_condition(c, condition, *seed.world());
    // 明确是日期条件功能夹具，不是从新局自然经营到十五年的通关认证。
    // 负数高速值用于证明producer按新17身份识别，而非只识别J9的0→1。
    for (const int initial_speed : {0, -7}) {
        const auto files = work.paths(c, "calendar-speed-" + std::to_string(initial_speed));
        auto records = seed.records(); records.settings.high_speed = initial_speed;
        c.good(save_startup_system_file(files.root / "system.avr", records));
        StartupApplication app(files, ref::WorldRandomStream::from_java_seed(1));
        c.good(app.load_world_replay(condition, "calendar-endgame-condition-v1"));
        app.take_presentation_requests();
        c(!has_clear(app) && !app.clear_page() && app.live_settings().high_speed == initial_speed &&
          app.world()->state().system_completion_mode == initial_speed,
          "条件文件严格恢复后无17，J9镜像仍由原系统权威绑定");
        bool produced{};
        for (int n = 0; n < 64 && !produced; ++n) {
            const auto before = observation(app);
            const auto disk = bytes(files.root / "system.avr");
            const auto revision = app.records().revision;
#ifdef _WIN32
            const auto handle = CreateFileW((files.root / "system.avr").c_str(), GENERIC_READ,
                                           FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL, nullptr);
            c(handle != INVALID_HANDLE_VALUE, "锁定日历producer的独占系统文件");
            const auto error = app.update();
            const auto closed = CloseHandle(handle);
            c(closed != 0, "释放日历producer系统锁");
            if (!error.empty()) {
                c(observation(app) == before && bytes(files.root / "system.avr") == disk &&
                  !has_clear(app) && app.records().revision == revision,
                  "producer轮发布失败完整回滚世界/页/随机/输出/live/系统及审计历史");
                c.good(app.update());
                c(has_clear(app), "同一失败轮重试必须由真实跨月日历插入17");
            }
#else
            (void)before; (void)disk;
            c.good(app.update());
#endif
            produced = has_clear(app);
            if (produced) {
                const auto &s = app.world()->state();
                const auto page = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                    [](const auto &p) { return p.legacy_page == 17 && p.lifecycle != 4; });
                const auto persisted = load_startup_system_file(files.root / "system.avr");
                c(s.scene.calendar.year == 15 && s.scene.calendar.month == 3 && page->lifecycle == 0 &&
                  !app.clear_page() && s.completion_mode == 1 && s.system_completion_mode == 1 &&
                  app.live_settings().high_speed == 1 && app.records().settings.high_speed == 1 &&
                  !app.settings_dirty() && persisted.records && persisted.records->settings.high_speed == 1 &&
                  persisted.records->revision == app.records().revision && app.records().revision == revision + 1,
                  "真实跨月插17同轮即J9=1联合落盘，早于计分页Init/首次Update/最终确认");
            } else {
                const auto *page = top(app);
                if (page && page->kind != ref::WorldScriptPageKind::scene)
                    c.good(app.acknowledge_page(page->id));
            }
            app.take_presentation_requests();
        }
        c(produced, "日历条件夹具须在64轮内由真实producer插17");
    }
}
} // namespace

int run_startup_application_config_checks(const std::filesystem::path &parent) {
    Checks checks;
    Work work(checks, parent);
    title_and_environment(checks, work);
    live_return_and_restore(checks, work);
    calendar_high_speed_transaction(checks, work);
    return checks.count;
}
