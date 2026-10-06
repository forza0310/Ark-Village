// Byte/file isolation is a separate responsibility from rule and worker transport tests.
#include "ark/app/world_save_files.hpp"
#include "support/world_fixture.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

void run_restore_tests();
void run_codec_boundary_tests();

namespace {
namespace app = ark::app;
using State = ark::simulation::StartupWorldRuntimeState;
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
State initial() {
    auto state = ark::test::initial_world();
    state.scene.framework_paused = true;
    return state;
}
std::vector<std::uint8_t> bytes(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
void codec_and_policy() {
    auto state = initial();
    const auto before = state;
    auto captured = app::capture_world_save(state);
    check(captured.image.has_value(), captured.message.c_str());
    check(ark::test::same_world_clock(state, before) &&
              state.scene.random.draws() == before.scene.random.draws(),
          "Capture cannot advance world time or randomness");
    auto decoded = app::decode_world_save(captured.image->bytes);
    check(decoded.state.has_value(), decoded.message.c_str());
    check(decoded.metadata &&
              decoded.metadata->funds == state.scene.world.world.ai.accounting.funds() &&
              decoded.metadata->village == state.scripts.village_name,
          "Directory metadata describes verified Owner data");
    std::string reason;
    auto current = ark::test::initial_world(77);
    current.scene.random.draw(101);
    const auto draws = current.scene.random.draws();
    auto expected_random = current.scene.random;
    check(app::prepare_world_save_candidate(*decoded.state, current, reason) ==
              app::WorldSaveError::none,
          reason.c_str());
    check(decoded.state->scene.random.draws() == draws &&
              decoded.state->scene.random.draw(197).ticket == expected_random.draw(197).ticket,
          "Load keeps current process stream rather than the save-time stream");
    check(decoded.state->scene.world.world.ai.accounting.entries().empty(),
          "Restoration does not replay audit charges");
    auto recaptured = app::capture_world_save(*decoded.state);
    check(recaptured.image && recaptured.image->bytes == captured.image->bytes,
          "Durable fields round-trip exactly after pure restoration");
    auto old_fast = state;
    old_fast.scene.speed_setting = 1;
    const auto old_fast_file = app::capture_world_save(old_fast);
    check(old_fast_file.image.has_value(), "Historical speed2 save remains a valid schema2 file");
    auto old_fast_candidate = app::decode_world_save(old_fast_file.image->bytes);
    check(old_fast_candidate.state && old_fast_candidate.state->scene.speed_setting == 1,
          "Decode preserves the recorded value before session restoration");
    check(app::prepare_world_save_candidate(*old_fast_candidate.state, current, reason) ==
                  app::WorldSaveError::none &&
              old_fast_candidate.state->scene.speed_setting == 0 &&
              old_fast_candidate.state->scene.random.draws() == draws &&
              old_fast_candidate.state->scene.world.world.ai.accounting.funds() ==
                  state.scene.world.world.ai.accounting.funds(),
          "Loading old speed2 file uses normal current pacing without changing cash or random");
    state.sound_requests = {1, 2};
    state.visual_effects.push_back({});
    state.scripts.notices.push_back({});
    auto transient = app::capture_world_save(state);
    check(transient.image && transient.image->bytes == captured.image->bytes,
          "Notifications, sound requests and local visuals are not file state");
    state.report_state = 1;
    check(app::capture_world_save(state).error == app::WorldSaveError::ineligible,
          "Unfinished monthly report cannot be saved");
    state = before;
    state.scripts.pages.push_back({});
    state.scripts.pages.back().kind = ark::simulation::rules::WorldScriptPageKind::raw_page;
    state.scripts.pages.back().legacy_page = 30;
    check(app::capture_world_save(state).error == app::WorldSaveError::ineligible,
          "Business modal cannot be silently discarded by saving");
}
void corrupt_files() {
    auto captured = app::capture_world_save(initial());
    check(captured.image.has_value(), captured.message.c_str());
    const auto &valid = captured.image->bytes;
    for (const auto size : {std::size_t{0}, std::size_t{1}, valid.size() / 2, valid.size() - 1}) {
        std::vector<std::uint8_t> truncated(valid.begin(), valid.begin() + size);
        check(!app::decode_world_save(truncated).state, "Truncated record rejected");
    }
    auto altered = valid;
    altered[altered.size() / 2] ^= 1;
    check(!app::decode_world_save(altered).state, "Payload corruption rejected");
    altered = valid;
    altered.push_back(0);
    check(!app::decode_world_save(altered).state, "Trailing bytes rejected");
    check(app::decode_world_save(std::vector<std::uint8_t>(app::world_save_max_bytes + 1)).error ==
              app::WorldSaveError::too_large,
          "Oversize rejected before allocation of world");
}
void slots_and_failure() {
    const auto directory = std::filesystem::current_path() / "world-save-file-test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    auto state = initial();
    const auto first = app::capture_world_save(state);
    check(first.image.has_value(), first.message.c_str());
    check(app::write_world_save_slot(directory, 0, *first.image).error == app::WorldSaveError::none,
          "First slot written");
    const auto old = bytes(app::world_save_slot_path(directory, 0));
    state.scene.world.world.ai.accounting = ark::simulation::rules::PeriodAccounting(12345, 9);
    auto second = app::capture_world_save(state);
    check(second.image.has_value(), second.message.c_str());
    check(app::write_world_save_slot(directory, 1, *second.image).error ==
              app::WorldSaveError::none,
          "Second slot written independently");
    check(bytes(app::world_save_slot_path(directory, 0)) == old,
          "Second slot does not overwrite first slot");
    const auto slots = app::inspect_world_save_slots(directory);
    check(slots[0].metadata && slots[1].metadata && slots[1].metadata->funds == 12345,
          "Both directory summaries derive from valid records");
    auto broken = *second.image;
    broken.bytes.back() ^= 2;
    check(app::write_world_save_slot(directory, 0, broken).error != app::WorldSaveError::none &&
              bytes(app::world_save_slot_path(directory, 0)) == old,
          "Invalid replacement preserves old file");
#ifdef _WIN32
    const auto target = app::world_save_slot_path(directory, 0);
    auto unrelated_temporary = target;
    unrelated_temporary += L".tmp-" + std::to_wstring(GetCurrentProcessId()) + L"-0";
    std::ofstream(unrelated_temporary, std::ios::binary)
        << "existing temporary from another writer";
    const auto unrelated_bytes = bytes(unrelated_temporary);
    const auto locked = CreateFileW(target.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
    check(locked != INVALID_HANDLE_VALUE, "Test exclusively locks existing player file");
    const auto blocked_file = app::write_world_save_slot(directory, 0, *second.image);
    CloseHandle(locked);
    check(blocked_file.error == app::WorldSaveError::io_error && bytes(target) == old,
          "Windows replacement sharing failure preserves old file bytes after temporary flush");
    check(bytes(unrelated_temporary) == unrelated_bytes,
          "Exclusive temporary creation skips and preserves a preexisting writer file");
#endif
    // A directory at the target blocks replacement after temporary write and flush.
    const auto blocked = directory / "blocked";
    std::filesystem::create_directories(app::world_save_slot_path(blocked, 0));
    check(app::write_world_save_slot(blocked, 0, *first.image).error ==
                  app::WorldSaveError::io_error &&
              std::filesystem::is_directory(app::world_save_slot_path(blocked, 0)),
          "Failed OS replacement leaves previous target intact");
    for (const auto &entry : std::filesystem::directory_iterator(blocked))
        check(entry.path().extension() == ".ark", "Failed write removes owned temporary file");
    auto overwritten = app::write_world_save_slot(directory, 0, *second.image);
    check(overwritten.error == app::WorldSaveError::none &&
              app::read_world_save_slot(directory, 0).metadata->funds == 12345,
          "Explicit overwrite atomically replaces existing file");
    std::filesystem::remove_all(directory);
}
} // namespace
int main() {
    codec_and_policy();
    corrupt_files();
    slots_and_failure();
    run_restore_tests();
    run_codec_boundary_tests();
    std::cout << "PASS world save " << checks << " checks\n";
}
