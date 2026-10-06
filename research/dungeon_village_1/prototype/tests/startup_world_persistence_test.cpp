#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_prototype/startup_world_village_activity.hpp"
#include "dungeon_village_tools/archive.hpp"
#include "startup_world_codec.hpp"
#include "startup_world_codec_checks.hpp"
#include "startup_world_restore_checks.hpp"
#include "startup_world_restore_validation.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace dungeon_village_prototype;
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
    const auto h = dungeon_village_tools::sha256_hex(b);
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
            text(result, dungeon_village_tools::sha256_hex(bytes));
            result.insert(result.end(), bytes.begin(), bytes.end());
        }
        (void)payload;
    }
    const auto hash = dungeon_village_tools::sha256_hex(result);
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
