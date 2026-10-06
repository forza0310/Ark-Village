// Adversarial records retain a valid checksum so each protocol boundary is exercised directly.
#include "ark/app/world_save.hpp"
#include "support/world_fixture.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
namespace app = ark::app;
using Bytes = std::vector<std::uint8_t>;
int checks{};
void check(bool condition, const std::string &message) {
    ++checks;
    if (!condition)
        throw std::runtime_error("codec boundary: " + message);
}
void append32(Bytes &bytes, std::uint32_t value) {
    for (int i = 0; i < 4; ++i)
        bytes.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
}
void append64(Bytes &bytes, std::uint64_t value) {
    for (int i = 0; i < 8; ++i)
        bytes.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
}
std::uint32_t read32(const Bytes &bytes, std::size_t offset) {
    check(offset <= bytes.size() && bytes.size() - offset >= 4, "fixture field is in range");
    std::uint32_t value{};
    for (int i = 0; i < 4; ++i)
        value |= static_cast<std::uint32_t>(bytes[offset + i]) << (8 * i);
    return value;
}
void put32(Bytes &bytes, std::size_t offset, std::uint32_t value) {
    check(offset <= bytes.size() && bytes.size() - offset >= 4, "mutation field is in range");
    for (int i = 0; i < 4; ++i)
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
}
void reseal(Bytes &bytes) {
    check(bytes.size() >= 8, "checksum trailer exists");
    std::uint64_t value = 0xcbf29ce484222325ULL;
    for (std::size_t i = 0; i < bytes.size() - 8; ++i) {
        value ^= bytes[i];
        value *= 0x100000001b3ULL;
    }
    for (int i = 0; i < 8; ++i)
        bytes[bytes.size() - 8 + i] = static_cast<std::uint8_t>(value >> (8 * i));
}
struct Header {
    std::size_t dataset{};
    std::size_t funds{};
    std::size_t length{};
    std::size_t payload{};
};
Header header(const Bytes &bytes) {
    // Schema 2: magic8, version4, dataset string, village string, date4*4, funds8, length4.
    const auto dataset_length = read32(bytes, 12);
    const auto village_length_position = 16U + dataset_length;
    const auto village_length = read32(bytes, village_length_position);
    const auto funds = village_length_position + 4U + village_length + 16U;
    const auto length = funds + 8U;
    check(read32(bytes, length) == bytes.size() - length - 4U - 8U,
          "captured header payload size matches schema");
    return {16, funds, length, length + 4U};
}
Bytes record(const Bytes &valid, Header layout, const Bytes &payload) {
    Bytes bytes(valid.begin(), valid.begin() + layout.payload);
    put32(bytes, layout.length, static_cast<std::uint32_t>(payload.size()));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    append64(bytes, 0);
    reseal(bytes);
    return bytes;
}
void rejected(const Bytes &bytes, app::WorldSaveError error, const char *diagnostic,
              const char *scenario) {
    const auto decoded = app::decode_world_save(bytes);
    check(!decoded.state && !decoded.metadata && decoded.error == error &&
              decoded.message.find(diagnostic) != std::string::npos,
          std::string(scenario) + ": " + decoded.message);
}

void headers(const Bytes &valid, Header layout) {
    auto bytes = valid;
    put32(bytes, 8, 99);
    reseal(bytes);
    rejected(bytes, app::WorldSaveError::unsupported_version, "version", "unknown schema");
    bytes = valid;
    bytes[layout.dataset] = bytes[layout.dataset] == '0' ? '1' : '0';
    reseal(bytes);
    rejected(bytes, app::WorldSaveError::dataset_mismatch, "dataset", "foreign frozen dataset");
    for (const std::string previous_dataset : {
             "a955854e17b1c57a0d067f0ef95c33164e09c305850995d27505adfc593f3609",
             "92dfab7c3c640a939ce68bd5741d1e92fd0630c59bfaf5d099e8e0e17ac6301c"}) {
        bytes = valid;
        check(read32(bytes, 12) == previous_dataset.size(),
              "previous frozen identity has fixed width");
        std::copy(previous_dataset.begin(), previous_dataset.end(), bytes.begin() + layout.dataset);
        reseal(bytes);
        rejected(bytes, app::WorldSaveError::dataset_mismatch, "dataset", "previous product dataset");
    }
    bytes = valid;
    put32(bytes, 8, 1);
    reseal(bytes);
    rejected(bytes, app::WorldSaveError::unsupported_version, "version", "previous schema");
    bytes = valid;
    bytes[0] ^= 1;
    reseal(bytes);
    rejected(bytes, app::WorldSaveError::malformed, "signature", "unknown magic");
    bytes = valid;
    bytes[layout.payload] ^= 1;
    rejected(bytes, app::WorldSaveError::malformed, "checksum", "unsealed corruption");
    bytes = valid;
    bytes[layout.funds] ^= 1;
    reseal(bytes);
    rejected(bytes, app::WorldSaveError::malformed, "summary", "header-world funds disagreement");
    bytes = valid;
    put32(bytes, layout.length, read32(bytes, layout.length) + 1);
    reseal(bytes);
    rejected(bytes, app::WorldSaveError::malformed, "length", "incorrect declared payload size");
    bytes = valid;
    bytes.insert(bytes.end() - 8, 0);
    put32(bytes, layout.length, read32(valid, layout.length) + 1);
    reseal(bytes);
    rejected(bytes, app::WorldSaveError::malformed, "Trailing", "sealed trailing payload byte");
    rejected(record(valid, layout, {0}), app::WorldSaveError::malformed, "Truncated",
             "sealed incomplete first map count");
}

void hostile_prefixes(const Bytes &valid, Header layout) {
    // The first payload field is the live actor map. Hostile counts fail before allocation.
    for (const auto count : {0xffffffffU, 1000001U}) {
        Bytes payload;
        append32(payload, count);
        rejected(record(valid, layout, payload), app::WorldSaveError::too_large, "budget",
                 count == 0xffffffffU ? "negative signed-origin count" : "oversize actor count");
    }
    Bytes payload(1000004, 0);
    put32(payload, 0, 1000000);
    rejected(record(valid, layout, payload), app::WorldSaveError::too_large, "allocation",
             "actor allocation exceeds budget despite enough input bytes");

    // Independently authored minimal actor prefix reaches the first HP boolean directly.
    payload.clear();
    append32(payload, 1); // Actor map count.
    append64(payload, 1); // Stable map key.
    append64(payload, 1); // Actor stable identity.
    append32(payload, 0); // Human kind.
    append32(payload, 0); // Definition.
    for (int i = 0; i < 6; ++i)
        append32(payload, 0); // flags/state/action/counters/facing.
    append32(payload, 0);     // Empty control FIFO.
    for (int i = 0; i < 4; ++i)
        append32(payload, 0); // HP delta/display/origin/target.
    payload.push_back(2);     // Invalid animating byte.
    rejected(record(valid, layout, payload), app::WorldSaveError::malformed, "boolean",
             "noncanonical boolean");

    // Empty actors followed by two identical human-definition keys; records are eight int32s.
    payload.clear();
    append32(payload, 0);
    append32(payload, 2);
    for (int record_index = 0; record_index < 2; ++record_index) {
        append32(payload, 0);
        for (int i = 0; i < 8; ++i)
            append32(payload, 0);
    }
    rejected(record(valid, layout, payload), app::WorldSaveError::malformed, "Duplicate map",
             "duplicate shared-human key");
}

void illegal_enum(const Bytes &valid, Header layout,
                  const ark::simulation::StartupWorldRuntimeState &state) {
    // Locate the public map's schema bytes by a unique independently authored map prefix.
    // This avoids mirroring every preceding world field to calculate a private byte offset.
    const auto &map = state.scene.world.world.map;
    Bytes prefix;
    append32(prefix, static_cast<std::uint32_t>(map.width));
    append32(prefix, static_cast<std::uint32_t>(map.height));
    append32(prefix, static_cast<std::uint32_t>(map.cells.size()));
    for (std::size_t i = 0; i < std::min<std::size_t>(map.cells.size(), 16); ++i) {
        const auto &cell = map.cells[i];
        append32(prefix, static_cast<std::uint32_t>(cell.legacy_state));
        append32(prefix, static_cast<std::uint32_t>(cell.category));
        prefix.push_back(cell.facility ? 1 : 0);
        if (cell.facility) {
            append64(prefix, cell.facility->instance_id.value);
            append32(prefix, static_cast<std::uint32_t>(cell.facility->definition_id));
            append32(prefix, static_cast<std::uint32_t>(cell.facility->fragment_index));
        }
    }
    auto begin = valid.begin() + layout.payload;
    const auto end = valid.end() - 8;
    const auto found = std::search(begin, end, prefix.begin(), prefix.end());
    check(found != end, "map prefix exists in captured payload");
    check(std::search(found + 1, end, prefix.begin(), prefix.end()) == end,
          "map prefix locates exactly one public map");
    auto bytes = valid;
    put32(bytes, static_cast<std::size_t>(found - valid.begin()) + 16U, 99);
    reseal(bytes);
    rejected(bytes, app::WorldSaveError::invalid_world, "map", "invalid map category enum");
}
} // namespace

void run_codec_boundary_tests() {
    const auto state = ark::test::initial_world();
    const auto captured = app::capture_world_save(state);
    check(captured.image.has_value(), "initial capture: " + captured.message);
    const auto &valid = captured.image->bytes;
    const auto layout = header(valid);
    headers(valid, layout);
    hostile_prefixes(valid, layout);
    illegal_enum(valid, layout, state);
    std::cout << "PASS codec boundary " << checks << " checks\n";
}
