#include "ark/simulation/startup_application_replay.hpp"
#include "ark/assets/sha256.hpp"
#include "startup_application_replay_wire.hpp"
#include "startup_application_replay_paths.hpp"
#include "startup_world_file_io.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace ark::simulation;
// 复用持久化套件的严格条件夹具，不新增Owner安装口或第二套字段替换算法。
std::filesystem::path create_application_clear_entry_fixture(const std::filesystem::path &);
namespace {
using Bytes = std::vector<std::uint8_t>;
using Metadata = StartupApplicationReplayMetadata;
constexpr const char *controller = "application-clear-conditions-v3";
constexpr const char *entry_controller = "application-clear-entry-fixture-v1";
constexpr std::uint64_t limit = 4000;
enum class Phase : std::uint64_t { advancing, row44, row45, final64, failure_observed, finished };

void require(bool ok, const std::string &why) {
    if (!ok) throw std::runtime_error("application replay: " + why);
}
void good(const std::string &error) { require(error.empty(), error); }
Bytes read(const std::filesystem::path &file) {
    std::ifstream input(file, std::ios::binary);
    require(static_cast<bool>(input), "cannot open " + file.string());
    return {std::istreambuf_iterator<char>(input), {}};
}
std::vector<std::filesystem::path> names(const std::filesystem::path &directory) {
    std::vector<std::filesystem::path> result;
    for(const auto &e:std::filesystem::directory_iterator(directory))result.push_back(e.path().filename());
    std::sort(result.begin(),result.end());return result;
}
void u64(Bytes &out, std::uint64_t value) {
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
}
void text(Bytes &out, const std::string &value) {
    u64(out, value.size()); out.insert(out.end(), value.begin(), value.end());
}
struct Reader {
    const Bytes &bytes;
    std::size_t at{};
    std::uint64_t number() {
        require(at <= bytes.size() && bytes.size() - at >= 8, "driver integer truncated");
        std::uint64_t result{};
        for (int i = 0; i < 8; ++i) result |= std::uint64_t(bytes[at++]) << (8 * i);
        return result;
    }
    std::string string() {
        const auto n = number();
        require(n <= 4096 && at <= bytes.size() && n <= bytes.size() - at, "driver text bounds");
        const std::string result(bytes.begin() + at, bytes.begin() + at + static_cast<std::size_t>(n));
        at += static_cast<std::size_t>(n); return result;
    }
};
std::string hex(const Bytes &bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result; result.reserve(bytes.size() * 2);
    for (auto byte : bytes) { result.push_back(digits[byte >> 4]); result.push_back(digits[byte & 15]); }
    return result;
}
Bytes sounds_bytes(const std::vector<StartupAudioRequest> &sounds) {
    Bytes result; u64(result, sounds.size());
    for (const auto &sound : sounds) {
        u64(result, static_cast<std::uint64_t>(sound.operation));
        u64(result, static_cast<std::uint64_t>(sound.id));
    }
    return result;
}
std::array<std::uint64_t, 3> events(const StartupApplication &app) {
    require(app.world() != nullptr, "driver requires retained world");
    const auto state = startup_world_runtime_scripts(app.world()->state());
    std::array<std::uint64_t, 3> result{};
    for (std::size_t i = 0; i < result.size(); ++i) {
        const auto found = state.event_calls.find(static_cast<int>(i + 4));
        require(found == state.event_calls.end() || found->second >= 0, "negative event count");
        if (found != state.event_calls.end()) result[i] = static_cast<std::uint64_t>(found->second);
    }
    return result;
}
const ref::WorldScriptPage *top(const StartupApplication &app) {
    if (!app.world()) return nullptr;
    for (auto p = app.world()->state().scripts.pages.rbegin(); p != app.world()->state().scripts.pages.rend(); ++p)
        if (p->lifecycle != 4) return &*p;
    return nullptr;
}
// 所有影响后继策略/检查/输出的变量均在此规范载荷。路径与输出流只属于测试环境。
struct Driver {
    std::uint64_t scenario{}, next_frame{1}, next_command{1}, clear_id{};
    Phase phase{Phase::advancing};
    std::uint64_t entry_high{}, entry_trophy{}, total{}, random{}, entry_steps{}, entry_history{}, entry_revision{};
    std::string entry_village;
    std::vector<StartupAudioRequest> activation_sounds; // 实际初始化/加载已消费输出，与计分尾段分开。
    std::array<std::uint64_t, 3> event_base{}, event_seen{};
    std::vector<int> sounds;
    std::uint64_t failures{}, successful{}, checks{};
    bool inject_failure{}, outputs_consumed{true};
};
Bytes encode(const Driver &d) {
    Bytes b; text(b, "AVACDRV3"); text(b, "conditional_raw17");
    u64(b, 1); u64(b, 0); // 固定seed1/speed0，不是外推原版新局资格。
    for (auto n : {d.scenario, d.next_frame, d.next_command, d.clear_id, static_cast<std::uint64_t>(d.phase),
                   d.entry_high, d.entry_trophy, d.total, d.random, d.entry_steps, d.entry_history, d.entry_revision}) u64(b, n);
    text(b, d.entry_village);
    const auto activation=sounds_bytes(d.activation_sounds);b.insert(b.end(),activation.begin(),activation.end());
    for (auto n : d.event_base) u64(b, n);
    for (auto n : d.event_seen) u64(b, n);
    u64(b, d.sounds.size()); for (int n : d.sounds) {
        u64(b, static_cast<std::uint64_t>(StartupAudioOperation::replace_bgm));
        u64(b, static_cast<std::uint64_t>(n));
    }
    for (auto n : {d.failures, d.successful, d.checks, std::uint64_t(d.inject_failure),
                   std::uint64_t(d.outputs_consumed), limit, std::uint64_t(2)}) u64(b, n);
    return b;
}
Driver decode(const Bytes &bytes) {
    require(bytes.size() <= 8192, "driver budget"); Reader r{bytes};
    require(r.string() == "AVACDRV3" && r.string() == "conditional_raw17" &&
                r.number() == 1 && r.number() == 0, "driver identity/seed/speed");
    Driver d;
    d.scenario = r.number(); d.next_frame = r.number(); d.next_command = r.number(); d.clear_id = r.number();
    const auto phase = r.number(); require(phase <= static_cast<std::uint64_t>(Phase::finished), "driver phase");
    d.phase = static_cast<Phase>(phase); d.entry_high = r.number(); d.entry_trophy = r.number();
    d.total = r.number(); d.random = r.number(); d.entry_steps = r.number(); d.entry_history = r.number();
    d.entry_revision=r.number();
    d.entry_village = r.string();
    const auto activations=r.number();
    require(activations==(d.scenario==0?2U:4U),"driver actual initialization/activation output count");
    for(std::uint64_t i=0;i<activations;++i) {
        const auto operation=r.number(), id=r.number();
        require(operation==static_cast<std::uint64_t>(StartupAudioOperation::replace_bgm) && id==i%2,
                "driver preserves each real title B0 followed by world G1");
        d.activation_sounds.push_back({StartupAudioOperation::replace_bgm,static_cast<int>(id)});
    }
    for (auto &n : d.event_base) n = r.number();
    for (auto &n : d.event_seen) n = r.number();
    const auto count = r.number(); require(count <= 1, "driver sound count");
    for (std::uint64_t i = 0; i < count; ++i) {
        require(r.number() == static_cast<std::uint64_t>(StartupAudioOperation::replace_bgm),
                "fixture resumes main track with actual BGM replacement operation");
        const auto sound = r.number(); require(sound == 1, "fixture only resumes BGM1"); d.sounds.push_back(1);
    }
    d.failures = r.number(); d.successful = r.number(); d.checks = r.number();
    const auto inject = r.number(), consumed = r.number();
    require(inject <= 1 && consumed == 1 && r.number() == limit && r.number() == 2, "driver flags/bound/target row");
    d.inject_failure = inject != 0; d.outputs_consumed = true;
    require(r.at == bytes.size() && encode(d) == bytes, "driver canonical encoding/trailing bytes");
    require(d.scenario <= 1 && d.next_frame >= 1 && d.next_frame <= limit &&
                d.next_command == d.next_frame && d.clear_id && d.failures <= 1 && d.successful < limit &&
                d.entry_revision==(d.scenario==0?1U:3U) &&
                // 每轮五项既有oracle加typed操作／单队列领取一项，仍严格检查数量。
                d.successful + d.failures == d.next_frame - 1 && d.checks == 6 * (d.next_frame - 1) &&
                d.entry_trophy <= 4 && d.entry_high <= std::uint64_t(std::numeric_limits<std::int64_t>::max()) &&
                d.total > 0 && d.total <= std::uint64_t(std::numeric_limits<std::int64_t>::max()) &&
                (!d.failures || d.inject_failure), "driver counters/record relationships");
    for (auto n : d.event_base) require(n <= std::uint64_t(std::numeric_limits<int>::max()) - 1, "driver event baseline bounds");
    if (d.scenario == 0) require(d.entry_high == 0 && d.entry_trophy == 0, "new-high baseline");
    else require(d.entry_high == d.total && d.entry_trophy == 2 && d.entry_village == "已有通关村", "equal-record baseline");
    return d;
}
std::string validate(const StartupApplication &app, const Metadata &metadata) {
    try {
        require(metadata.controller_id == controller, "controller identity");
        const auto d = decode(metadata.controller_state);
        require(metadata.next_frame == d.next_frame && metadata.next_command == d.next_command, "outer/driver sequence");
        require(app.error().empty() && app.page() == StartupApplicationPage::world && app.world(), "driver app/world binding");
        const auto &state = app.world()->state();
        require(!state.scene.framework_paused && !state.scripts.executing_page && state.sound_requests.empty() &&
                    !app.has_pending_audio_requests() &&
                    state.scene.random.draws() == d.random && state.simulation_steps == d.entry_steps &&
                    app.world()->checkpoints().size() == d.entry_history, "driver frozen world/output/random/history boundary");
        require(events(app) == d.event_seen, "driver actual event counters");
        const auto score = startup_world_clear_score(state);
        require(score.candidate.has_value(), "driver actual six rows");
        std::uint64_t total{}; for (const auto &row : *score.candidate) total += static_cast<std::uint64_t>(row.score);
        require(total == d.total, "driver captured total binding");
        const bool finished = d.phase == Phase::finished;
        require(app.records().revision==d.entry_revision+(finished?1U:0U),
                "system directory transaction commits once only at score completion");
        for(const auto &slot:app.records().save_directory)for(const auto &item:slot)
            require(item.packed_date==-1 && !item.reference && item.village.empty() && item.cash==0,
                    "conditional replay entry never invents a normal manual/interrupt directory record");
        const auto *page = top(app);
        if (!finished) {
            require(page && page->id == d.clear_id && page->legacy_page == 17, "driver live clear binding");
            require(app.records().high_score == static_cast<std::int64_t>(d.entry_high) &&
                        app.records().score_village == d.entry_village && app.records().trophy == static_cast<int>(d.entry_trophy) &&
                        d.event_seen == d.event_base && d.sounds.empty(), "private score before completion");
            if (d.next_frame == 1) require(!app.clear_page() && d.phase == Phase::advancing, "unadvanced entry");
            else require(app.clear_page().has_value(), "missing active clear controller");
            if (app.clear_page()) {
                const auto &clear = *app.clear_page();
                Phase expected = Phase::advancing;
                if (clear.stage == 3 && clear.row == 2 && clear.counter == 44) expected = Phase::row44;
                if (clear.stage == 3 && clear.row == 2 && clear.counter == 45) expected = Phase::row45;
                if (clear.stage == 6 && clear.counter == 64)
                    expected = d.failures ? Phase::failure_observed : Phase::final64;
                require(d.phase == expected, "driver phase matches exact owner boundary");
            }
            require(d.failures == (d.phase == Phase::failure_observed ? 1U : 0U), "failure only at preserved final64");
        } else {
            require(!app.clear_page() && !app.clear_rows() && (!page || page->id != d.clear_id) && d.sounds == std::vector<int>{1},
                    "completed clear retired and actual sound consumed");
            auto expected = d.event_base; ++expected[2]; ++expected[d.scenario == 0 ? 0 : 1];
            require(d.event_seen == expected && d.failures == std::uint64_t(d.inject_failure), "single actual finish events/failure");
            require(app.records().high_score == static_cast<std::int64_t>(d.total), "terminal system score");
            if (d.scenario == 1)
                require(app.records().score_village == d.entry_village && app.records().trophy == static_cast<int>(d.entry_trophy), "equal-score record owner preserved");
            else
                require(app.records().score_village == state.scripts.village_name && app.records().trophy >= 1, "new-score actual record owner");
        }
        return {};
    } catch (const std::exception &e) { return e.what(); }
}
StartupApplicationPaths paths(const std::filesystem::path &dir) {
    return {dir};
}
std::string digest(const StartupApplication &app, const Metadata &metadata) {
    const auto result = startup_application_replay_digest(app, metadata, validate);
    require(result.size() == 64 && result.find_first_not_of("0123456789abcdef") == std::string::npos, "full application digest: " + result);
    return result;
}
Driver initialize(StartupApplication &app, const std::filesystem::path &entry, const std::filesystem::path &dir,
                  std::uint64_t scenario) {
    good(app.error());
    good(app.load_world_replay(entry, entry_controller));
    std::vector<StartupAudioRequest> activation;
    const auto consume_activation=[&] {
        const auto actual=app.take_audio_requests();
        require(actual==std::vector<StartupAudioRequest>{{StartupAudioOperation::replace_bgm,0},
                                                       {StartupAudioOperation::replace_bgm,1}} &&
                    app.take_sound_requests().empty() && !app.has_pending_audio_requests(),
                "actual title initialization B0 then successful replay entry G1 consumed exactly once");
        activation.insert(activation.end(),actual.begin(),actual.end());
    };
    consume_activation();
    const auto score = startup_world_clear_score(app.world()->state());
    require(score.candidate.has_value(), "fixture score rows");
    std::uint64_t total{}; for (const auto &row : *score.candidate) total += static_cast<std::uint64_t>(row.score);
    if (scenario == 1) {
        const auto observed=load_startup_application_storage(dir);
        require(observed.snapshot.has_value(),"equal-record setup observes current system2 catalog");
        auto record=observed.snapshot->records;
        record.high_score = static_cast<std::int64_t>(total); record.score_village = "已有通关村"; record.trophy = 2;
        const auto committed=commit_startup_application_records(dir,*observed.snapshot,record);
        require(committed.snapshot && committed.error.empty(),"equal-score record fixture commits through system2 transaction");
        app = StartupApplication(paths(dir), ref::WorldRandomStream::from_java_seed(1));
        good(app.error()); good(app.load_world_replay(entry, entry_controller));
        consume_activation();
    }
    Driver d; d.scenario = scenario; d.clear_id = top(app)->id; d.total = total;
    d.entry_high = static_cast<std::uint64_t>(app.records().high_score); d.entry_village = app.records().score_village;
    d.entry_trophy = static_cast<std::uint64_t>(app.records().trophy);
    d.entry_revision=app.records().revision;d.activation_sounds=std::move(activation);
    d.random = app.world()->state().scene.random.draws(); d.entry_steps = app.world()->state().simulation_steps;
    d.entry_history = app.world()->checkpoints().size(); d.event_base = d.event_seen = events(app);
#ifdef _WIN32
    d.inject_failure = true;
#endif
    return d;
}
// 本轮局部Driver候选只在全部输出已消费并完成6项行为断言后重新编码提交。
std::string step(StartupApplication &app, Metadata &metadata, const std::filesystem::path &system) {
    good(validate(app, metadata)); auto d = decode(metadata.controller_state);
    require(d.phase != Phase::finished && d.next_frame < limit, "bounded nonterminal update");
    const bool failing = d.phase == Phase::final64 && d.inject_failure;
    const auto before = failing ? digest(app, metadata) : std::string{};
    const auto system_before = failing ? read(system) : Bytes{};
    d.outputs_consumed = false;
    const bool confirm = d.phase != Phase::row44 && d.phase != Phase::final64 && d.phase != Phase::failure_observed;
    std::string error;
#ifdef _WIN32
    if (failing) {
        struct Lock {
            HANDLE handle{INVALID_HANDLE_VALUE};
            ~Lock() { if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
        } lock;
        lock.handle = CreateFileW(system.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        require(lock.handle != INVALID_HANDLE_VALUE, "hold real system replacement lock");
        error = app.update(confirm);
    } else
#endif
        error = app.update(confirm);
    const auto sounds = app.take_audio_requests();
    auto check = [&](bool ok, const char *why) { require(ok, why); ++d.checks; };
    check(failing ? !error.empty() : error.empty(), "actual update/fault result");
    check(!failing || (digest(app, metadata) == before && read(system) == system_before), "failed commit preserves full application and system");
    const bool finished = !app.clear_page();
    std::vector<int> sound_ids;
    for (const auto &sound : sounds) sound_ids.push_back(sound.id);
    check(sound_ids == (finished && !failing ? std::vector<int>{1} : std::vector<int>{}), "exact once-only actual BGM output");
    check(sounds == (finished && !failing ?
              std::vector<StartupAudioRequest>{{StartupAudioOperation::replace_bgm, 1}} :
              std::vector<StartupAudioRequest>{}) && app.take_sound_requests().empty(),
          "BGM operation survives conditional replay and both sinks share once-only consumption");
    auto expected_events = d.event_base;
    if (finished && !failing) { ++expected_events[2]; ++expected_events[d.scenario == 0 ? 0 : 1]; }
    d.event_seen = events(app);
    check(d.event_seen == expected_events, "events6 then4/5 commit once");
    check(app.world()->state().scene.random.draws() == d.random &&
              app.world()->state().simulation_steps == d.entry_steps && app.world()->checkpoints().size() == d.entry_history &&
              app.records().revision==d.entry_revision+(finished && !failing?1U:0U),
          "clear preserves world/random/history and commits system revision exactly once");
    if (failing) ++d.failures; else ++d.successful;
    d.sounds.insert(d.sounds.end(), sound_ids.begin(), sound_ids.end()); d.outputs_consumed = true;
    d.phase = Phase::advancing;
    if (finished) d.phase = Phase::finished;
    else {
        const auto &clear = *app.clear_page();
        if (clear.stage == 3 && clear.row == 2 && clear.counter == 44) d.phase = Phase::row44;
        if (clear.stage == 3 && clear.row == 2 && clear.counter == 45) d.phase = Phase::row45;
        if (clear.stage == 6 && clear.counter == 64) d.phase = failing ? Phase::failure_observed : Phase::final64;
    }
    ++d.next_frame; ++d.next_command;
    auto candidate = metadata; candidate.next_frame = d.next_frame; candidate.next_command = d.next_command;
    candidate.controller_state = encode(d); good(validate(app, candidate));
    static_assert(std::is_nothrow_move_assignable_v<Metadata>);
    metadata = std::move(candidate);
    // 实际输出全量进trace，保存点在这次声音消费之后；路径不参与应用摘要。
    Bytes event_bytes; for (auto n : d.event_seen) u64(event_bytes, n);
    return std::to_string(d.next_frame - 1) + " " + digest(app, metadata) + " " +
        startup_world_session_digest(*app.world()) + " " + hex(metadata.controller_state) + " " +
        hex(sounds_bytes(sounds)) + " " + hex(event_bytes) + " " + ark::assets::sha256_hex(read(system)) + "\n";
}
const char *boundary(Phase phase) {
    switch (phase) {
    case Phase::row44: return "row2-44";
    case Phase::row45: return "row2-45";
    case Phase::final64: return "final64";
    case Phase::failure_observed: return "failed";
    default: return nullptr;
    }
}
Metadata initial_metadata(const Driver &driver) {
    Metadata m; m.controller_id = controller; m.producer_revision = "conditional-raw17-system2-audio-v3";
    m.next_frame = driver.next_frame; m.next_command = driver.next_command; m.controller_state = encode(driver);
    m.extensions = {{1024, 3, {0, 255, 1}}}; return m;
}
// 六个恢复关系场景共用另一应用套件的独立Wire工具，不使用被测control encoder造错档。
int corrupted_clear_relationships(const std::filesystem::path &root, const std::filesystem::path &system,
                                  const std::filesystem::path &source, StartupApplication &app, Metadata &metadata) {
    namespace wire = application_replay_fixture;
    const auto original = read(source);
    auto decoded = wire::unpack(original);
    require(wire::pack(decoded) == original, "independent application wire reconstruction");
    std::size_t semantics_at=12;
    require(wire::number(decoded.prefix,semantics_at,4)==7,"condition source uses application semantics7");
    const auto &control = wire::section(decoded, 3).bytes;
    const auto base = wire::control_offsets(control);
    std::size_t at = base.world;
    require(wire::number(control, at, 4) == 1 && wire::number(control, at, 4) == 1, "clear source has world and six rows");
    const auto row_start = at;
    for (int i = 0; i < 12; ++i) (void)wire::number(control, at, 8);
    require(wire::number(control, at, 4) == 1, "clear source has controller");
    require(wire::number(control, at, 4) == 3, "source stage3");
    const auto counter_at = at;
    require(wire::number(control, at, 4) == 45 && wire::number(control, at, 4) == 2, "nonfirst row2 credited boundary");
    const auto sum_at = at; const auto sum = wire::number(control, at, 8);
    const auto high_at = at; const auto high = wire::number(control, at, 8);
    for (int i = 0; i < 3; ++i) (void)wire::number(control, at, 4);
    const auto id_flag_at = at;
    require(wire::number(control, at, 4) == 1, "clear source has binding ID");
    const auto id_at = at; const auto id = wire::number(control, at, 8);
    // 语义2的标题尾部固定为4计数及20个六字段槽；本logic夹具必须全零。
    for (int i = 0; i < 4 + 20 * 6; ++i)
        require(wire::number(control, at, 4) == 0, "logic clear source has pristine title state");
    require(wire::number(control,at,4)==0 && wire::number(control,at,4)==0 &&
                wire::number(control,at,4)==1 && wire::number(control,at,8)==1 &&
                wire::number(control,at,8)==2,"direct research entry retains default root menu identity");
    for(int i=0;i<4;++i)
        require(wire::number(control,at,4)==0,"no retained title subpage or pending application audio");
    require(at == control.size(), "clear control fully parsed before mutation");
    std::size_t row_at = row_start + 5 * 16;
    const auto future_count = wire::number(control, row_at, 8), future_score = wire::number(control, row_at, 8);
    require(future_count < 1000000 && future_score == future_count * 10,
            "small fixture effort permits exact independent binary32-preserving mutation");
    const std::array<std::string, 6> reasons{{"计分阶段累计关系非法", "捕获六行不是当前世界投影",
        "世界计分页与应用计数阶段不同", "计分控制器页面引用或暂停资格非法",
        "捕获最高分与系统纪录不同", "计分三元组不完整"}};
    const auto before = digest(app, metadata);
    const auto system_before = read(system);
    const auto driver_before = metadata.controller_state;
    const auto frame_before = metadata.next_frame, command_before = metadata.next_command;
    int checks = 1;
    for (std::size_t i = 0; i < reasons.size(); ++i) {
        auto changed = decoded;
        auto &b = wire::section(changed, 3).bytes;
        switch (i) {
        case 0: wire::set_number(b, sum_at, 8, sum + 1); break;
        case 1:
            // 未来努力度行仍满足count*10且不改变当前前缀累计，专门命中world投影关系。
            wire::set_number(b, row_start + 5 * 16, 8, future_count + 1);
            wire::set_number(b, row_start + 5 * 16 + 8, 8, future_score + 10); break;
        case 2: wire::set_number(b, counter_at, 4, 44); break;
        case 3: wire::set_number(b, id_at, 8, id + 1); break;
        case 4: wire::set_number(b, high_at, 8, high + 1); break;
        case 5:
            wire::set_number(b, id_flag_at, 4, 0);
            b.erase(b.begin() + id_at, b.begin() + id_at + 8); break; // 只移除optional ID，保留标题尾部。
        }
        const auto bytes = wire::pack(changed);
        const auto damaged = root / ("bad-clear-" + std::to_string(i) + ".avra");
        require(!std::filesystem::exists(damaged), "exclusive relationship fixture");
        { std::ofstream out(damaged, std::ios::binary);
          out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
          require(static_cast<bool>(out), "relationship fixture output"); }
        const auto isolated = root / ("bad-clear-target-" + std::to_string(i));
        require(!std::filesystem::exists(isolated), "failed restore target root must not already exist");
        const auto prior_names=names(root);
        const auto error = restore_startup_application_replay(damaged, isolated, controller, app, metadata, validate);
        require(error.find(reasons[i]) != std::string::npos, "resigned clear relation must reject at expected guard: " + reasons[i] + ": " + error); ++checks;
        require(digest(app, metadata) == before && metadata.controller_state == driver_before &&
                    metadata.next_frame == frame_before && metadata.next_command == command_before &&
                    read(system) == system_before && read(damaged) == bytes && read(source) == original,
                "bad clear candidate retains complete application/driver/system/source"); ++checks;
        require(!std::filesystem::exists(isolated) && names(root)==prior_names,
                "bad clear relation publishes no application root/blob or staging directory"); ++checks;
    }
    return checks;
}
std::string summary(const StartupApplication &app, const Metadata &metadata) {
    const auto d = decode(metadata.controller_state);
    return "application replay summary frame=" + std::to_string(d.next_frame - 1) + " score=" +
        std::to_string(app.records().high_score) + " failures=" + std::to_string(d.failures) +
        " sounds=" + std::to_string(d.sounds.size()) + " activation_sounds=" +
        std::to_string(d.activation_sounds.size()) + " checks=" + std::to_string(d.checks);
}
struct OwnedDirectory {
    std::filesystem::path path;
    explicit OwnedDirectory(const std::filesystem::path &root) {
        path = root / ("application-replay-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        require(std::filesystem::create_directory(path), "exclusive test directory");
    }
    ~OwnedDirectory() { std::error_code ec; std::filesystem::remove_all(path, ec); }
};
} // namespace

int run_startup_application_replay_checks(const std::filesystem::path &root) {
    OwnedDirectory owned(root);
    const auto fixture_dir = owned.path / "fixture"; std::filesystem::create_directory(fixture_dir);
    const auto entry = create_application_clear_entry_fixture(fixture_dir);
    int checks{};
    for (std::uint64_t scenario : {0U, 1U}) {
        const auto live = owned.path / ("live" + std::to_string(scenario)); std::filesystem::create_directory(live);
        StartupApplication app(paths(live), ref::WorldRandomStream::from_java_seed(1));
        auto metadata = initial_metadata(initialize(app, entry, live, scenario));
        const auto saved = owned.path / ("score" + std::to_string(scenario) + ".avra");
        while (decode(metadata.controller_state).phase != Phase::row45) step(app, metadata, (live / "system.avr"));
        good(save_startup_application_replay(owned.path, saved, app, metadata, validate));
        if (scenario == 0) checks += corrupted_clear_relationships(owned.path, (live / "system.avr"), saved, app, metadata);
        const auto before = digest(app, metadata);
        const auto bytes = read(saved);
        auto invalid = metadata; invalid.controller_state.push_back(0);
        require(!save_startup_application_replay(owned.path, owned.path / "bad.avra", app, invalid, validate).empty() &&
                    !std::filesystem::exists(owned.path / "bad.avra") && digest(app, metadata) == before,
                "driver trailing data rejected before capture"); ++checks;
        invalid = metadata; ++invalid.next_command;
        require(!validate(app, invalid).empty(), "outer command mismatch rejected"); ++checks;
        Driver unconsumed = decode(metadata.controller_state); unconsumed.outputs_consumed = false;
        invalid = metadata; invalid.controller_state = encode(unconsumed);
        require(!validate(app, invalid).empty(), "unconsumed output rejected"); ++checks;
        require(!save_startup_application_replay(owned.path, saved, app, metadata, validate).empty() && read(saved) == bytes,
                "create-only capture retains old valid snapshot"); ++checks;
        const auto isolated = owned.path / ("isolated" + std::to_string(scenario));
        require(!std::filesystem::exists(isolated), "restore root must be absent before atomic publication");
        const auto old_root=owned.path/("unused"+std::to_string(scenario));
        require(std::filesystem::create_directory(old_root),"create current destination app root separately");
        StartupApplication restored(paths(old_root), ref::WorldRandomStream::from_java_seed(9));
        good(restored.error());
        require(restored.take_audio_requests()==std::vector<StartupAudioRequest>{{StartupAudioOperation::replace_bgm,0}},
                "destination original title B0 consumed before replacing app with restored candidate");
        Metadata restored_metadata;
        good(restore_startup_application_replay(saved, isolated, controller, restored, restored_metadata, validate));
        require(digest(restored, restored_metadata) == before && restored_metadata.controller_state == metadata.controller_state &&
                    read((isolated / "system.avr")) == read((live / "system.avr")) &&
                    restored.records().save_directory==app.records().save_directory &&
                    std::filesystem::is_directory(isolated/"worlds") && std::filesystem::is_empty(isolated/"worlds") &&
                    !restored.has_pending_audio_requests(),
                "full app/driver/system2 empty directory view restores without replaying activation audio"); ++checks;
        while (decode(metadata.controller_state).phase != Phase::finished) {
            const auto expected = step(app, metadata, (live / "system.avr"));
            const auto actual = step(restored, restored_metadata, (isolated / "system.avr"));
            require(actual == expected, "same-process full output tail");
        }
        require(read(saved) == bytes && summary(app, metadata) == summary(restored, restored_metadata), "source unchanged and terminal summary equal"); ++checks;
        require(!restored.acknowledge_page(decode(restored_metadata.controller_state).clear_id).empty(), "retired clear cannot award twice"); ++checks;
    }
    std::cout << "application replay checks=" << checks << '\n'; return checks;
}

int run_startup_application_replay_cli(int argc, const char **argv) {
    std::map<std::string, std::string> options;
    for (int i = 2; i < argc; i += 2) {
        require(i + 1 < argc, "application missing option value");
        const std::string key = argv[i];
        require(key == "--work-dir" || key == "--save-directory" || key == "--load-file" || key == "--trace-file",
                "application unknown option");
        require(options.emplace(key, argv[i + 1]).second, "application duplicate option");
    }
    require(options.count("--work-dir") && options.count("--trace-file") &&
                (options.count("--load-file") != options.count("--save-directory")), "application CLI mode/options");
    const auto root = std::filesystem::absolute(options.at("--work-dir"));
    const auto live = root / "application";
    const auto trace_path = std::filesystem::absolute(options.at("--trace-file"));
    const auto current_paths = paths(root / "unused-current");
    const std::vector<std::filesystem::path> source_paths = options.count("--load-file")
        ? std::vector<std::filesystem::path>{options.at("--load-file")} : std::vector<std::filesystem::path>{};
    // 使用已验纯预检先验证root/trace；probe从不写入，不替调用者创建任何父目录。
    (void)persistence_detail::prepare_replay_capture_paths(root, root / ".application-cli-probe", current_paths, {trace_path});
    const auto trace_target = persistence_detail::prepare_replay_capture_paths(root.parent_path(), trace_path, current_paths, source_paths);
    if (options.count("--load-file"))
        (void)persistence_detail::prepare_replay_restore_paths(live, options.at("--load-file"), current_paths, {trace_path});
    else
        for (const char *name : {"row2-44", "row2-45", "final64", "failed"})
            (void)persistence_detail::prepare_replay_capture_paths(root.parent_path(),
                std::filesystem::path(options.at("--save-directory")) / (std::string(name) + ".avra"), current_paths, {trace_path});
    const auto initial_root=options.count("--load-file")?current_paths.root:live;
    require(std::filesystem::create_directory(initial_root), "exclusive current application root; restore target remains absent");
    // 旧调用者应用路径与新隔离目标分开；缺失旧文件也不是允许互相覆盖的理由。
    StartupApplication app(paths(initial_root),
                           ref::WorldRandomStream::from_java_seed(1)); Metadata metadata;
    if (options.count("--load-file")) {
        require(app.take_audio_requests()==std::vector<StartupAudioRequest>{{StartupAudioOperation::replace_bgm,0}},
                "restore harness consumes its initial title B0 before replacing app");
        good(restore_startup_application_replay(options.at("--load-file"), live, controller, app, metadata, validate, {trace_path}));
    } else {
        const auto fixture = root / "fixture"; require(std::filesystem::create_directory(fixture), "fresh fixture directory");
        const auto entry = create_application_clear_entry_fixture(fixture);
        metadata = initial_metadata(initialize(app, entry, live, 0));
    }
    good(validate(app, metadata));
    std::ostringstream trace;
    while (decode(metadata.controller_state).phase != Phase::finished) {
        trace << step(app, metadata, (live / "system.avr")); require(static_cast<bool>(trace), "trace output consumed");
        const auto d = decode(metadata.controller_state);
        if (const auto name = boundary(d.phase); name && options.count("--save-directory")) {
            const auto saved = std::filesystem::path(options.at("--save-directory")) / (std::string(name) + ".avra");
            good(save_startup_application_replay(root.parent_path(), saved, app, metadata, validate, {trace_path}));
            std::cout << "application replay captured boundary=" << name << " next_frame=" << d.next_frame << '\n';
        }
    }
    const auto output = trace.str();
    // 发布前再次验证别名；create-only出口拒绝预检后出现的同名文件，不用ofstream竞态覆盖。
    (void)persistence_detail::prepare_replay_capture_paths(root.parent_path(), trace_path, paths(live), source_paths);
    persistence_detail::create_save_file(trace_target.container, Bytes(output.begin(), output.end()));
    std::cout << summary(app, metadata) << '\n'; return 0;
}
