#include "world_campaign_diagnostics.hpp"
#include "../../src/simulation/persistence/startup_world_codec.hpp"
#include "../support/world_fixture.hpp"
#include "ark/assets/sha256.hpp"
#include "ark/simulation/persistence/startup_world_persistence.hpp"
#include "ark_library_contract.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace codec = sim::persistence_detail;
void require(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error("Campaign diagnostic: " + message);
}
std::string read(const std::filesystem::path &path) {
    require(std::filesystem::file_size(path) <= 64U * 1024U * 1024U, "file exceeds codec budget");
    std::ifstream stream(path, std::ios::binary);
    require(bool(stream), "cannot read " + path.string());
    return {std::istreambuf_iterator<char>(stream), {}};
}
void write_new(const std::filesystem::path &path, const std::string &bytes) {
    require(!std::filesystem::exists(path), "refuse to overwrite " + path.string());
    std::ofstream stream(path, std::ios::binary);
    stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    stream.close();
    require(bool(stream), "cannot write " + path.string());
}
const char *stage_name(sim::rules::WorldScheduleStage stage) {
    using S = sim::rules::WorldScheduleStage;
    switch (stage) {
    case S::arrival_front:
        return "arrival_front";
    case S::popularity:
        return "popularity";
    case S::decision:
        return "decision";
    case S::prefix_effects:
        return "prefix_effects";
    case S::carry_expression:
        return "carry_expression";
    case S::control:
        return "control";
    case S::actor_tail_cache:
        return "actor_tail_cache";
    case S::projectile:
        return "projectile";
    case S::object:
        return "object";
    case S::encounter:
        return "encounter";
    case S::facility:
        return "facility";
    case S::finalize:
        return "finalize";
    }
    return "unknown";
}
} // namespace

std::string describe_campaign_failure(const sim::StartupWorldRuntimeResult &result) {
    std::ostringstream out;
    out << "runtime=" << int(result.error) << " scene=" << int(result.scene_error)
        << " world=" << int(result.world_error);
    if (result.failure) {
        const auto &failure = *result.failure;
        out << " stage=" << stage_name(failure.stage) << " id=";
        if (failure.id)
            out << *failure.id;
        else
            out << "none";
        out << " layer=" << failure.layer << " detail=" << failure.error;
    } else {
        out << " stage=unavailable";
    }
    return out.str();
}

void capture_campaign_failure(const std::filesystem::path &directory, const app::WorldState &before,
                              const sim::StartupWorldRuntimeResult &result,
                              std::uint64_t successful_rounds) {
    require(!result.candidate, "only rejected updates belong in failure evidence");
    const auto encoded = codec::encode_state(before);
    const std::string payload(encoded.begin(), encoded.end());
    std::ostringstream metadata;
    metadata << "ARK_CAMPAIGN_FAILURE_1\n"
             << codec::codec_schema_identity() << '\n'
             << sim::startup_world_persistence_dataset() << '\n'
             << assets::sha256_hex(payload) << '\n'
             << describe_campaign_failure(result) << '\n'
             << ARK_LIBRARY_CONTRACT << '\n';
    write_new(directory / "failure-state.bin", payload);
    write_new(directory / "failure-state.txt", metadata.str());
    std::cout << "FAILURE_SNAPSHOT rounds=" << successful_rounds
              << " world_steps=" << before.simulation_steps
              << " random_draws=" << before.scene.random.draws() << " bytes=" << payload.size()
              << ' ' << describe_campaign_failure(result) << std::endl;
    if (result.failure && result.failure->id) {
        const sim::rules::CharacterId id{*result.failure->id};
        const auto &actors = before.scene.world.world.ai.battle.actors;
        if (const auto found = actors.find(id); found != actors.end())
            std::cout << "PRE_UPDATE_ACTOR id=" << id.value
                      << " definition=" << found->second.definition
                      << " state=" << found->second.control.state << std::endl;
    }
}

void replay_campaign_failure(const std::filesystem::path &directory, bool expect_fixed) {
    std::istringstream metadata(read(directory / "failure-state.txt"));
    std::string magic, schema, dataset, digest, expected, producer;
    require(bool(std::getline(metadata, magic)) && magic == "ARK_CAMPAIGN_FAILURE_1" &&
                bool(std::getline(metadata, schema)) && schema == codec::codec_schema_identity() &&
                bool(std::getline(metadata, dataset)) &&
                dataset == sim::startup_world_persistence_dataset() &&
                bool(std::getline(metadata, digest)) && bool(std::getline(metadata, expected)) &&
                bool(std::getline(metadata, producer)),
            "snapshot schema/data identity is unsupported");
    const auto payload = read(directory / "failure-state.bin");
    require(assets::sha256_hex(payload) == digest, "snapshot bytes changed");
    const auto initial = initial_world();
    const auto state = codec::decode_state({payload.begin(), payload.end()}, *initial.rules);
    const auto before = sim::startup_world_state_digest(state);
    const auto result = sim::prepare_startup_world_runtime(state);
    if (expect_fixed) {
        require(result.candidate.has_value(),
                "fixed runtime still rejects: " + describe_campaign_failure(result));
        require(sim::startup_world_state_digest(state) == before,
                "fixed update mutated input Owner");
        std::cout << "PASS formerly failing exact Owner/random update now commits; prior="
                  << expected << " producer_library=" << producer
                  << " current_library=" << ARK_LIBRARY_CONTRACT << std::endl;
        return;
    }
    require(!result.candidate && describe_campaign_failure(result) == expected,
            "single update differs: " + describe_campaign_failure(result) + "; expected " +
                expected);
    require(sim::startup_world_state_digest(state) == before,
            "rejected update mutated the input Owner/random");
    std::cout << "PASS exact Owner/random single-update failure replay: " << expected
              << " producer_library=" << producer << " current_library=" << ARK_LIBRARY_CONTRACT
              << std::endl;
    if (result.failure && result.failure->stage == sim::rules::WorldScheduleStage::projectile) {
        // Secondary probe of the public adapter, after the exact production replay above.
        // Forward every request unchanged; inspect the failing transaction, never install it.
        auto adapter = sim::startup_world_runtime_adapter();
        const auto consumer = adapter.nonactors.request;
        const auto identity = result.failure->id;
        adapter.nonactors.request = [consumer, identity](auto &owner, const auto &request) {
            const auto out = consumer(owner, request);
            if (identity && request.identity == *identity) {
                std::cout << "NONACTOR_REQUEST kind=" << int(request.kind)
                          << " identity=" << request.identity
                          << " caster=" << (request.caster ? request.caster->value : 0)
                          << " target=" << (request.target ? request.target->value : 0)
                          << " visual=" << request.visual
                          << " hit=" << (request.hit ? int(request.hit->kind) : -1)
                          << " parameter=" << (request.hit ? request.hit->parameter : -1)
                          << " accepted=" << bool(out);
                if (request.target) {
                    const auto &ai = owner.scene.world.world.ai;
                    std::cout << " target_live=" << ai.battle.actors.count(*request.target)
                              << " target_retired=" << ai.retired_actors.count(*request.target)
                              << " target_context=" << ai.contexts.count(*request.target)
                              << " target_metadata=" << owner.actor_metadata.count(*request.target);
                }
                if (out)
                    std::cout << " facing=" << out->target_facing.value_or(-1)
                              << " effects=" << bool(out->target_effects) << " effects_valid="
                              << (!out->target_effects ||
                                  sim::rules::valid_actor_effect_state(*out->target_effects));
                std::cout << std::endl;
            }
            return out;
        };
        const auto probe =
            sim::rules::prepare_owned_world_runtime(state, {state.calendar_advance, true}, adapter);
        require(!probe.state && probe.failure && probe.failure->stage == result.failure->stage &&
                    probe.failure->id == identity && probe.failure->error == result.failure->error,
                "adapter probe differs from exact production failure; do not attribute its trace");
        require(sim::startup_world_state_digest(state) == before, "adapter probe mutated input");
        std::cout << "PASS unchanged-adapter secondary failure trace" << std::endl;
    }
}

void campaign_diagnostic_contract(const std::filesystem::path &build) {
    const auto directory =
        build / "validation" /
        ("campaign-diagnostic-contract-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory.parent_path());
    require(std::filesystem::create_directory(directory), "fresh contract directory required");
    auto state = initial_world();
    capture_campaign_observation(directory, state);
    require(sim::startup_world_state_digest(read_campaign_observation(directory)) ==
                sim::startup_world_state_digest(state),
            "observation must preserve the full Owner/random");
    state.scripts.pages.clear(); // Deliberately invalid page root, not a reachable business prefix.
    const auto result = sim::prepare_startup_world_runtime(state);
    require(!result.candidate, "invalid page fixture must reject");
    capture_campaign_failure(directory, state, result, 0);
    replay_campaign_failure(directory);
    const auto original = read(directory / "failure-state.bin");
    bool refused{};
    try {
        capture_campaign_failure(directory, state, result, 0);
    } catch (const std::exception &) {
        refused = true;
    }
    require(refused && read(directory / "failure-state.bin") == original,
            "capture must preserve an existing failure snapshot");
    auto corrupt = original;
    corrupt.at(0) ^= 1;
    std::ofstream damaged(directory / "failure-state.bin", std::ios::binary);
    damaged.write(corrupt.data(), static_cast<std::streamsize>(corrupt.size()));
    damaged.close();
    refused = false;
    try {
        replay_campaign_failure(directory);
    } catch (const std::exception &) {
        refused = true;
    }
    require(refused, "altered snapshot must reject before replay");
    std::filesystem::remove_all(directory); // Only this exclusively created fixture directory.
}

// Observation snapshots retain experimental endpoints even if a business assertion fails.
// They contain no player-strategy certificate and are never installed into player slots.
void capture_campaign_observation(const std::filesystem::path &directory,
                                  const app::WorldState &state) {
    const auto encoded = codec::encode_state(state);
    const std::string payload(encoded.begin(), encoded.end());
    std::ostringstream metadata;
    metadata << "ARK_CAMPAIGN_OBSERVATION_1\n"
             << codec::codec_schema_identity() << '\n'
             << sim::startup_world_persistence_dataset() << '\n'
             << assets::sha256_hex(payload) << '\n'
             << ARK_LIBRARY_CONTRACT << '\n';
    write_new(directory / "observation.bin", payload);
    write_new(directory / "observation.txt", metadata.str());
}
app::WorldState read_campaign_observation(const std::filesystem::path &directory) {
    std::istringstream metadata(read(directory / "observation.txt"));
    std::string magic, schema, dataset, digest;
    require(bool(std::getline(metadata, magic)) && magic == "ARK_CAMPAIGN_OBSERVATION_1" &&
                bool(std::getline(metadata, schema)) && schema == codec::codec_schema_identity() &&
                bool(std::getline(metadata, dataset)) &&
                dataset == sim::startup_world_persistence_dataset() &&
                bool(std::getline(metadata, digest)),
            "unsupported observation identity");
    const auto payload = read(directory / "observation.bin");
    require(assets::sha256_hex(payload) == digest, "observation bytes changed");
    const auto initial = initial_world();
    return codec::decode_state({payload.begin(), payload.end()}, *initial.rules);
}
} // namespace ark::test
