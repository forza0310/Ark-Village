#include "ark/simulation/startup_world_building.hpp"
#include "ark/simulation/startup_world_facility_catalog.hpp"
#include "ark/simulation/startup_world_persistence.hpp"
#include "ark/simulation/startup_world_village_activity.hpp"
#include "ark/assets/sha256.hpp"
#include "startup_world_codec.hpp"
#include "startup_world_codec_checks.hpp"
#include "startup_world_restore_checks.hpp"
#include "startup_world_restore_validation.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace ark::simulation;
namespace {
using Bytes = std::vector<std::uint8_t>;
int checks{};
void check(bool value, const std::string &message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
Bytes read(const std::filesystem::path &p) {
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}
void write(const std::filesystem::path &p, const Bytes &b) {
    std::ofstream f(p, std::ios::binary);
    f.write(reinterpret_cast<const char *>(b.data()), static_cast<std::streamsize>(b.size()));
    check(static_cast<bool>(f), "fixture write");
}
void append32(Bytes &b, std::uint32_t n) {
    for (int i = 0; i < 4; ++i)
        b.push_back(static_cast<std::uint8_t>(n >> (8 * i)));
}
void append64(Bytes &b, std::uint64_t n) {
    for (int i = 0; i < 8; ++i)
        b.push_back(static_cast<std::uint8_t>(n >> (8 * i)));
}
void text(Bytes &b, const std::string &s) {
    append32(b, static_cast<std::uint32_t>(s.size()));
    b.insert(b.end(), s.begin(), s.end());
}
std::uint64_t number(const Bytes &b, std::size_t &at, int n) {
    std::uint64_t v{};
    for (int i = 0; i < n; ++i)
        v |= static_cast<std::uint64_t>(b.at(at++)) << (8 * i);
    return v;
}
void skip_text(const Bytes &b, std::size_t &at) {
    const auto n = number(b, at, 4);
    at += static_cast<std::size_t>(n);
    check(at <= b.size(), "fixture text bounds");
}
void resign(Bytes &b) {
    check(b.size() >= 64, "fixture digest bounds");
    b.resize(b.size() - 64);
    const auto h = ark::assets::sha256_hex(b);
    b.insert(b.end(), h.begin(), h.end());
}
// 独立按公开文件布局替换段并重算摘要，确保拒绝来自业务校验而非损坏校验和。
Bytes replace_state(const Bytes &original, const StartupWorldRuntimeState &state) {
    std::size_t at = 20;
    skip_text(original, at);
    skip_text(original, at);
    const auto count = number(original, at, 4);
    Bytes result(original.begin(), original.begin() + static_cast<std::ptrdiff_t>(at));
    for (std::uint64_t i = 0; i < count; ++i) {
        const auto start = at;
        const auto id = number(original, at, 4);
        const auto version = number(original, at, 4);
        const auto required = number(original, at, 4);
        const auto length = number(original, at, 8);
        skip_text(original, at);
        const auto payload = at;
        at += static_cast<std::size_t>(length);
        check(at <= original.size() - 64, "fixture section bounds");
        if (id != 2)
            result.insert(result.end(), original.begin() + static_cast<std::ptrdiff_t>(start),
                          original.begin() + static_cast<std::ptrdiff_t>(at));
        else {
            const auto bytes = persistence_detail::encode_state(state);
            append32(result, 2);
            append32(result, static_cast<std::uint32_t>(version));
            append32(result, static_cast<std::uint32_t>(required));
            append64(result, bytes.size());
            text(result, ark::assets::sha256_hex(bytes));
            result.insert(result.end(), bytes.begin(), bytes.end());
        }
        (void)payload;
    }
    const auto hash = ark::assets::sha256_hex(result);
    result.insert(result.end(), hash.begin(), hash.end());
    return result;
}
std::vector<int> advance(StartupWorldRuntimeSession &s) {
    const auto step = s.update();
    check(step.candidate.has_value(), "natural update rejected");
    const auto p = s.state().scripts.pages.back();
    if (p.kind != ref::WorldScriptPageKind::scene && p.legacy_page != 16 && p.legacy_page != 24 &&
        p.legacy_page != 56 && p.legacy_page != 57 && p.legacy_page != 97 && p.legacy_page != 98)
        check(s.acknowledge_page(p.id) == StartupWorldRuntimeError::none,
              "natural confirm rejected");
    return s.take_sound_requests();
}
// 最小页面结构夹具，不注入资金、人物、日期或解锁；目录全来自真实稳定世界。
void facility_catalog_replay(const StartupWorldRuntimeSession &baseline_session,
                             const std::filesystem::path &dir,
                             const StartupWorldSaveMetadata &metadata) {
    using Action = StartupFacilityCatalogAction;
    const auto &baseline = baseline_session.state();
    const auto file = dir / "facility-catalog.avrs";
    const auto reencoded = dir / "facility-catalog-roundtrip.avrs";
    const auto broken = dir / "facility-catalog-bad.avrs";
    const auto capture = [&](const StartupWorldRuntimeState &state, const char *scenario) {
        // 现有独立段替换只准备结构夹具，经真实读器校验后才得到私有Session；
        // 不增加测试安装接口，不把被测编码结果用作业务期望。
        const auto seed_file = save_startup_world_file(reencoded, baseline_session, metadata);
        check(seed_file.ok, std::string(scenario) + " baseline fixture: " + seed_file.error);
        write(broken, replace_state(read(reencoded), state));
        auto fixture = load_startup_world_file(broken, startup_world_rules(), metadata.purpose,
                                               metadata.controller_id);
        check(fixture.snapshot.has_value(), std::string(scenario) + " fixture validate: " + fixture.error);
        auto source = std::move(fixture.snapshot->session);
        check(startup_world_state_digest(source.state()) == startup_world_state_digest(state),
              std::string(scenario) + " independent state fixture preserved");
        const auto digest = startup_world_session_digest(source);
        const auto saved = save_startup_world_file(file, source, metadata);
        check(saved.ok, std::string(scenario) + " capture: " + saved.error);
        check(startup_world_session_digest(source) == digest,
              std::string(scenario) + " capture consumes no state/random");
        auto load = load_startup_world_file(file, startup_world_rules(), metadata.purpose,
                                            metadata.controller_id);
        check(load.snapshot.has_value(), std::string(scenario) + " restore: " + load.error);
        check(startup_world_session_digest(load.snapshot->session) == digest,
              std::string(scenario) + " complete state/session roundtrip");
        check(save_startup_world_file(reencoded, load.snapshot->session, load.snapshot->metadata).ok &&
                  read(reencoded) == read(file),
              std::string(scenario) + " canonical file bytes preserved");
        return load.snapshot->session.state();
    };
    const auto equal = [&](const auto &reference, const auto &restored, const char *scenario) {
        check(startup_world_state_digest(reference) == startup_world_state_digest(restored),
              std::string(scenario) + " full state/random/output same after input");
    };
    const auto command = [&](auto &state, std::uint64_t id, Action action, const char *scenario) {
        check(act_startup_world_facility_catalog_page(state, id, action) ==
                  StartupWorldRuntimeError::none,
              std::string(scenario) + " command accepted");
    };
    auto goods = baseline;
    ref::WorldScriptPage page;
    page.id = goods.scripts.next_page_id++;
    page.kind = ref::WorldScriptPageKind::raw_page;
    page.legacy_page = 79;
    page.lifecycle = 0;
    page.legacy_f = 1;
    goods.scripts.pages.push_back(page);
    check(initialize_startup_world_facility_catalog_pages(goods), "initialize goods79 fixture");
    goods.scripts.pages.back().lifecycle = 2;
    command(goods, page.id, Action::next_tab, "goods79 nondefault phase");
    auto restored = capture(goods, "goods79");
    for (auto *state : {&goods, &restored}) {
        command(*state, page.id, Action::inspect, "goods79 open information72");
        check(initialize_startup_world_facility_catalog_pages(*state), "initialize actual child72");
        state->scripts.pages.back().lifecycle = 2;
    }
    equal(goods, restored, "restored79 opens same72");
    const auto child = goods.scripts.pages.back().id;
    check(goods.facility_catalog_page_parents.at(child) == page.id &&
              goods.scripts.pages.back().legacy_page == 72,
          "actual information child binds live79");
    auto restored_child = capture(goods, "information72");
    for (auto *state : {&goods, &restored_child}) {
        command(*state, child, Action::next_tab, "information72 browse");
        command(*state, child, Action::cancel, "information72 close");
        command(*state, page.id, Action::confirm, "goods79 confirm and retire");
    }
    equal(goods, restored_child, "restored72 continues and returns to79");
    check(goods.scripts.pages.back().lifecycle == 4 && goods.facility_catalog_pages_initialized.count(page.id),
          "79/72 close marks retirement without pretending the next framework entry already ran");
    check(std::all_of(goods.catalog.begin(), goods.catalog.end(), [](const auto &v) {
              return v.first.first != 1 || !v.second.newly_unlocked;
          }), "79 clears full weapon category NEW after restore");
    check(goods.scene.random.draws() == baseline.scene.random.draws() &&
              goods.scene.world.world.ai.accounting.funds() ==
                  baseline.scene.world.world.ai.accounting.funds(),
          "79/72 information and close have no draw or payment");
    for (auto *state : {&goods, &restored_child}) {
        const auto retired = prepare_startup_world_runtime(*state);
        check(retired.candidate.has_value(), "79/72 actual framework retirement succeeds");
        *state = *retired.candidate;
    }
    equal(goods, restored_child, "restored72 retires at the same framework entry");
    check(goods.facility_catalog_pages_initialized.empty() &&
              goods.facility_catalog_page_data.empty() && goods.facility_catalog_page_lists.empty() &&
              goods.facility_catalog_page_parents.empty(),
          "79/72 next framework entry retires every catalogue payload");

    auto praise = baseline;
    const auto definition = std::find_if(praise.rules->facilities.begin(), praise.rules->facilities.end(),
                                         [](const auto &v) { return v.legacy_icon == 2; });
    check(definition != praise.rules->facilities.end(), "fixed table icon2 animation source");
    ref::WorldScriptPage animation;
    animation.id = praise.scripts.next_page_id++;
    animation.kind = ref::WorldScriptPageKind::raw_page;
    animation.legacy_page = 82;
    animation.lifecycle = 0;
    animation.legacy_f = 0;
    animation.facility_definition = definition->id;
    praise.scripts.pages.push_back(animation);
    check(initialize_startup_world_facility_catalog_pages(praise), "initialize animation82 fixture");
    praise.scripts.pages.back().lifecycle = 2;
    praise.page_counters.at(animation.id) = 23; // 稳定等待结构夹具，不冒充自然演出可达证据。
    auto restored_first = capture(praise, "animation82 first phase");
    for (auto *state : {&praise, &restored_first}) {
        command(*state, animation.id, Action::confirm, "animation82 fast forward first");
        command(*state, animation.id, Action::confirm, "animation82 enter second phase");
    }
    equal(praise, restored_first, "restored82 preserves phase transition");
    check(praise.page_phases.at(animation.id) == 1 && praise.page_counters.at(animation.id) == 0,
          "82 second phase resets counter before deferred reward");
    praise.page_counters.at(animation.id) = 17;
    auto restored_second = capture(praise, "animation82 second phase");
    const auto queue = baseline.scene.world.popularity_queue;
    for (auto *state : {&praise, &restored_second}) {
        command(*state, animation.id, Action::confirm, "animation82 fast forward second");
        command(*state, animation.id, Action::confirm, "animation82 finish");
    }
    equal(praise, restored_second, "restored82 completes with one deferred reward");
    check(praise.scene.world.popularity_queue.size() == queue.size() + 1 &&
              praise.scene.world.popularity_queue.front() == std::array<int, 3>{10, 20, 1} &&
              std::equal(queue.begin(), queue.end(), praise.scene.world.popularity_queue.begin() + 1),
          "82 queues exact10/20/1 once and preserves original pending order");
    check(praise.popularity == baseline.popularity &&
              praise.scene.random.draws() == baseline.scene.random.draws(),
          "82 retirement changes neither immediate popularity nor random");
    const auto retired_digest = startup_world_state_digest(praise);
    check(act_startup_world_facility_catalog_page(praise, animation.id, Action::confirm) ==
              StartupWorldRuntimeError::invalid_page &&
              startup_world_state_digest(praise) == retired_digest,
          "restored82 cannot queue reward twice after retirement");
    for (auto *state : {&praise, &restored_second}) {
        const auto retired = prepare_startup_world_runtime(*state);
        check(retired.candidate.has_value(), "82 actual framework retirement succeeds");
        *state = *retired.candidate;
    }
    equal(praise, restored_second, "restored82 preserves subsequent world consumption");
    check(praise.facility_catalog_pages_initialized.empty() &&
              praise.facility_catalog_page_data.empty() && praise.facility_catalog_page_lists.empty() &&
              praise.facility_catalog_page_parents.empty(), "82 next framework entry retires all page payloads");

    // 旧布局身份独立改header并重签整文件，拒绝必须来自布局身份而非损坏摘要。
    auto old_layout = read(file);
    std::size_t at = 20;
    skip_text(old_layout, at);
    const auto length = number(old_layout, at, 4);
    const std::string old_schema = "0500cff0cd937c6967836c6bf7c594ff43dd64c23f408e7ea9188e4fc3423403";
    check(length == old_schema.size() &&
              std::string(old_layout.begin() + static_cast<std::ptrdiff_t>(at),
                          old_layout.begin() + static_cast<std::ptrdiff_t>(at + length)) != old_schema,
          "new79/72/82 fields produce a distinct schema identity");
    std::copy(old_schema.begin(), old_schema.end(), old_layout.begin() + static_cast<std::ptrdiff_t>(at));
    resign(old_layout);
    write(broken, old_layout);
    const auto refused = load_startup_world_file(broken, startup_world_rules(), metadata.purpose,
                                                 metadata.controller_id);
    check(!refused.snapshot && refused.error.find("状态字段布局不兼容") != std::string::npos,
          "prior-layout save explicitly rejected without migration: " + refused.error);
    std::filesystem::remove(file);
    std::filesystem::remove(reencoded);
    std::filesystem::remove(broken);
}
void run(const std::filesystem::path &dir) {
    std::filesystem::create_directories(dir);
    const auto replay = dir / "replay.avrs", normal = dir / "normal.avrs", bad = dir / "bad.avrs";
    StartupSession initial;
    StartupWorldRuntimeSession session(initial.state(), ref::WorldRandomStream::from_java_seed(1));
    int frames{};
    for (; frames < 2200; ++frames) {
        advance(session);
        if (!session.checkpoints().empty() &&
            !session.state().scene.world.world.ai.human_order.empty() &&
            session.state().scripts.pages.size() == 1 && session.state().scene.scene_state == 0) {
            ++frames;
            break;
        }
    }
    check(frames < 2200, "stable natural capture boundary");
    checks += check_startup_world_restore_contracts(session.state());
    const auto before = startup_world_session_digest(session);
    StartupWorldSaveMetadata metadata;
    metadata.purpose = StartupWorldSavePurpose::replay;
    metadata.producer_revision = "persistence-contract";
    metadata.controller_id = "test-driver-v1";
    metadata.next_frame = static_cast<std::uint64_t>(frames);
    metadata.controller_state = {1, 0, 255, 42};
    metadata.extensions = {{1024, 7, {255, 0, 254, 1}}};
    auto saved = save_startup_world_file(replay, session, metadata);
    check(saved.ok, "replay save: " + saved.error);
    const auto bytes = read(replay);
    check(std::string(bytes.begin(), bytes.begin() + 8) == "AVRSAVE1" && bytes[8] == 1 &&
              bytes[9] == 0 && bytes[16] == 2,
          "format magic/schema/purpose oracle");
    check(startup_world_session_digest(session) == before,
          "capture consumes no state/random/history");
    auto loaded = load_startup_world_file(replay, startup_world_rules(), metadata.purpose,
                                          metadata.controller_id);
    check(loaded.snapshot.has_value(), "replay load: " + loaded.error);
    check(startup_world_session_digest(loaded.snapshot->session) == before,
          "complete world/random/history roundtrip");
    check(loaded.snapshot->metadata.controller_state == metadata.controller_state &&
              loaded.snapshot->metadata.next_frame == metadata.next_frame,
          "controller position preserved");
    check(loaded.snapshot->metadata.extensions.size() == 1 &&
              loaded.snapshot->metadata.extensions[0].bytes == metadata.extensions[0].bytes,
          "opaque optional payload preserved");
    check(loaded.snapshot->session.checkpoints().front() != session.checkpoints().front(),
          "file restored independent history allocations");
    check(save_startup_world_file(bad, loaded.snapshot->session, loaded.snapshot->metadata).ok &&
              read(bad) == bytes,
          "canonical file byte roundtrip");
    auto second = load_startup_world_file(replay, startup_world_rules(), metadata.purpose,
                                          metadata.controller_id);
    check(second.snapshot.has_value(), "second file restore");
    auto uninterrupted = session;
    for (int frame = 0; frame < 90; ++frame) {
        for (auto *s : {&uninterrupted, &loaded.snapshot->session, &second.snapshot->session}) {
            s->set_paused(frame < 3);
            s->set_speed(frame % 11 == 0 ? 1 : 0);
        }
        const auto sounds = advance(uninterrupted);
        check(advance(loaded.snapshot->session) == sounds &&
                  advance(second.snapshot->session) == sounds,
              "restored output sequence");
        const auto digest = startup_world_session_digest(uninterrupted);
        check(startup_world_session_digest(loaded.snapshot->session) == digest &&
                  startup_world_session_digest(second.snapshot->session) == digest,
              "full canonical state each suffix frame");
    }
    StartupWorldSaveMetadata normal_meta;
    saved = save_startup_world_file(normal, session, normal_meta);
    check(saved.ok, "normal save: " + saved.error);
    auto normal_load =
        load_startup_world_file(normal, startup_world_rules(), StartupWorldSavePurpose::normal);
    check(normal_load.snapshot.has_value(), "normal load: " + normal_load.error);
    check(startup_world_state_digest(normal_load.snapshot->session.state()) ==
              startup_world_state_digest(session.state()),
          "normal complete world/random restored");
    check(normal_load.snapshot->session.checkpoints().empty(),
          "normal save excludes test audit history");
    check(!load_startup_world_file(normal, startup_world_rules(), metadata.purpose,
                                   metadata.controller_id)
               .snapshot,
          "purpose mismatch rejected");
    check(!load_startup_world_file(replay, startup_world_rules(), metadata.purpose, "wrong-driver")
               .snapshot,
          "controller mismatch rejected");
    for (const auto offset :
         {std::size_t(0), std::size_t(8), std::size_t(12), std::size_t(16), std::size_t(24)}) {
        auto altered = bytes;
        altered[offset] ^= 1;
        resign(altered);
        write(bad, altered);
        check(!load_startup_world_file(bad, startup_world_rules(), metadata.purpose,
                                       metadata.controller_id)
                   .snapshot,
              "incompatible header rejected after valid checksum");
    }
    auto truncated = bytes;
    truncated.pop_back();
    write(bad, truncated);
    check(!load_startup_world_file(bad, startup_world_rules(), metadata.purpose,
                                   metadata.controller_id)
               .snapshot,
          "truncated file rejected");
    auto trailing = bytes;
    trailing.push_back(1);
    write(bad, trailing);
    check(!load_startup_world_file(bad, startup_world_rules(), metadata.purpose,
                                   metadata.controller_id)
               .snapshot,
          "trailing bytes rejected");
    auto malformed = session.state();
    malformed.actor_metadata.erase(malformed.scene.world.world.ai.human_order.front());
    write(bad, replace_state(bytes, malformed));
    auto refused = load_startup_world_file(bad, startup_world_rules(), metadata.purpose,
                                           metadata.controller_id);
    check(!refused.snapshot && refused.error.find("状态拒绝") != std::string::npos,
          "missing actor metadata rejected by pure validator: " + refused.error);
    check(startup_world_session_digest(session) == before,
          "failed decode preserves current session");

    auto pages = session;
    check(pages.open_village_activities() == StartupWorldRuntimeError::none,
          "open actual activity page");
    for (int i = 0; i < 5 && pages.state().activity_pages_initialized.empty(); ++i) {
        check(pages.update().candidate.has_value(), "initialize activity page");
        pages.take_sound_requests();
    }
    check(!pages.state().activity_pages_initialized.empty(), "actual initialized page");
    check(!save_startup_world_file(normal, pages, normal_meta).ok, "normal modal capture rejected");
    const auto modal = save_startup_world_file(bad, pages, metadata);
    check(modal.ok, "replay modal capture: " + modal.error);
    const auto modal_bytes = read(bad);
    auto broken_page = pages.state();
    broken_page.page_counters.erase(*broken_page.activity_pages_initialized.begin());
    write(bad, replace_state(modal_bytes, broken_page));
    refused = load_startup_world_file(bad, startup_world_rules(), metadata.purpose,
                                      metadata.controller_id);
    check(!refused.snapshot && refused.error.find("状态拒绝") != std::string::npos,
          "initialized missing counter rejected: " + refused.error);

    facility_catalog_replay(session, dir, metadata);

    const auto normal_before = read(normal);
#ifdef _WIN32
    HANDLE held = CreateFileW(normal.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    check(held != INVALID_HANDLE_VALUE, "lock existing save for real replace failure");
    const auto failed = save_startup_world_file(normal, session, normal_meta);
    CloseHandle(held);
    check(!failed.ok && read(normal) == normal_before, "replace failure keeps old valid file");
#endif
    check(!save_startup_world_file(dir / "missing-parent" / "save.avrs", session, normal_meta).ok,
          "missing parent failure explicit");
    check(save_startup_world_file(normal, session, normal_meta).ok && read(normal) == normal_before,
          "repeated save replaces atomically");
    for (const auto &entry : std::filesystem::directory_iterator(dir))
        check(entry.path().filename().string().find(".tmp.") == std::string::npos,
              "temporary file cleaned");
    check(startup_world_session_digest(session) == before, "all file operations preserve source");
    std::filesystem::remove(replay);
    std::filesystem::remove(normal);
    std::filesystem::remove(bad);
    std::cout << "persistence checks=" << checks << " prefix_frames=" << frames
              << " suffix_frames=90 replay_bytes=" << bytes.size() << '\n';
}
} // namespace
int main(int argc, const char **argv) {
    try {
        check(argc == 2, "expected test directory");
        run_startup_world_codec_checks();
        run(argv[1]);
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
