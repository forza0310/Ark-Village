#include "dungeon_village_reference/world_residence.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_reference {
namespace {
struct Failure {
    WorldResidenceError error;
};
[[noreturn]] void fail(WorldResidenceError error) { throw Failure{error}; }
int checked(std::int64_t value) {
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        fail(WorldResidenceError::overflow);
    return static_cast<int>(value);
}
// 原d/a.b(text,replacement)逐字段替换；不是把人物名称覆盖为村名。
std::string substitute(std::string text, const std::string &replacement) {
    std::size_t begin{}, field{};
    do {
        const auto end = replacement.find('\t', begin);
        const auto value = replacement.substr(begin, end == std::string::npos ? end : end - begin);
        const std::string token = "<" + std::to_string(field++) + ">";
        if (value.find(token) != std::string::npos)
            fail(WorldResidenceError::invalid_owner);
        std::size_t position{};
        while ((position = text.find(token, position)) != std::string::npos) {
            text.replace(position, token.size(), value);
            position += value.size();
        }
        if (end == std::string::npos)
            break;
        begin = end + 1;
    } while (true);
    return text;
}
} // namespace
WorldResidenceResult prepare_world_residence(const WorldResidenceState &state,
                                             std::uint64_t identity,
                                             const WorldScriptCatalog &catalog) {
    try {
        const auto facility = state.facility.finish.dungeon.world.facilities.find(identity);
        const auto details = state.facility.details.find(identity);
        if (!identity || facility == state.facility.finish.dungeon.world.facilities.end() ||
            details == state.facility.details.end())
            fail(WorldResidenceError::invalid_owner);
        if (facility->second.status != 1 || facility->second.kind != 12 ||
            details->second.residence_mode != 1 || details->second.resident_definition == -1)
            fail(WorldResidenceError::invalid_construction_route);
        const int human = details->second.resident_definition;
        const auto owner = state.humans.find(human);
        const auto shared = state.facility.scripts.humans.find(human);
        if (owner == state.humans.end() || shared == state.facility.scripts.humans.end() ||
            owner->second.definition.legacy_u < 0 || owner->second.definition.legacy_u > 100 ||
            shared->second.satisfaction < 0 || shared->second.satisfaction > 100 ||
            state.facility.finish.event_calls != state.facility.scripts.event_calls ||
            state.facility.finish.dungeon.world.ai.pending_completion !=
                state.facility.scripts.pending_completion ||
            !prepare_world_script_continuations(catalog, state.facility.scripts, false).candidate)
            fail(WorldResidenceError::invalid_owner);
        WorldResidenceCandidate candidate{state, {}, {}, false};
        auto &next = candidate.state;
        auto &resident = next.humans.at(human);
        auto &scripts = next.facility.scripts;
        auto synchronize = [&] {
            next.facility.finish.event_calls = scripts.event_calls;
            next.facility.finish.dungeon.world.ai.pending_completion = scripts.pending_completion;
        };
        auto invoke = [&](int event, std::optional<std::string> replacement = {}) {
            const auto result =
                prepare_world_script(catalog, scripts, {event, std::move(replacement), {}});
            if (!result.candidate)
                fail(WorldResidenceError::script_failed);
            scripts = result.candidate->state;
            synchronize();
            candidate.scripts.insert(candidate.scripts.end(), result.candidate->executed.begin(),
                                     result.candidate->executed.end());
            candidate.pages.insert(candidate.pages.end(), result.candidate->inserted_pages.begin(),
                                   result.candidate->inserted_pages.end());
        };
        auto page = [&](WorldScriptPage value, std::optional<int> definition = {}) {
            const auto result = prepare_world_script_page(scripts, value);
            if (!result.candidate || !result.candidate->last_page)
                fail(WorldResidenceError::page_failed);
            scripts = result.candidate->state;
            // l锁只拒绝入栈；不存在的页对象不进入长期m/o引用索引。
            for (const auto &inserted : result.candidate->inserted_pages)
                next.page_bindings[inserted.id] = {human, definition};
            candidate.pages.insert(candidate.pages.end(), result.candidate->inserted_pages.begin(),
                                   result.candidate->inserted_pages.end());
        };
        const int old_satisfaction = scripts.humans.at(human).satisfaction;
        const int old_effort = resident.definition.legacy_u;
        const int new_satisfaction = std::min(old_satisfaction + 5, 100);
        const int new_effort = std::min(old_effort + 15, 100);
        next.reward_display = {std::array<int, 2>{old_satisfaction, old_effort},
                               std::array<int, 2>{new_satisfaction, new_effort},
                               std::array<int, 2>{5, 15}};
        scripts.humans.at(human).satisfaction =
            std::clamp(old_satisfaction + (new_satisfaction - old_satisfaction), 0, 100);
        resident.definition.legacy_u = new_effort;
        scripts.pending_completion =
            checked(static_cast<std::int64_t>(scripts.pending_completion) + 5);
        synchronize();
        invoke(58); // 即使C已100，仍给请求5的完成量；不以实际满足度差替代。
        candidate.effort_threshold_crossed = new_effort / 10 > old_effort / 10;
        if (candidate.effort_threshold_crossed) {
            auto old_definition = resident.definition;
            old_definition.legacy_u = old_effort;
            const auto before = derive_human_stats(old_definition, next.professions);
            const auto after = derive_human_stats(resident.definition, next.professions);
            if (!before.candidate || !after.candidate)
                fail(WorldResidenceError::stats_failed);
            next.effort_display[0] = before.candidate->combat;
            next.effort_display[1] = after.candidate->combat;
            for (std::size_t slot = 0; slot < 4; ++slot)
                next.effort_display[2][slot] =
                    checked(static_cast<std::int64_t>(after.candidate->combat[slot]) -
                            before.candidate->combat[slot]);
            resident.derived = *after.candidate; // e.b旧/新后e.a再次新u重算，值与after相同。
        }
        WorldScriptPage home;
        home.kind = WorldScriptPageKind::raw_page;
        home.legacy_page = 96;
        page(home, facility->second.placement.definition_id);
        if (candidate.effort_threshold_crossed) {
            WorldScriptPage effort;
            effort.kind = WorldScriptPageKind::raw_page;
            effort.legacy_page = 67;
            effort.legacy_f = old_effort;
            effort.legacy_g = new_effort;
            page(effort);
        }
        // 原人物e.i非aL/bC/br注册程序，固定25条均即时；严格拒绝其他指令而非造程序身份。
        for (const auto &command : resident.arrival_program) {
            if (command.size() != 2 || command[1] < 0)
                fail(WorldResidenceError::unsupported_program);
            WorldScriptPage value;
            if (command[0] == 2) {
                if (static_cast<std::size_t>(command[1]) >= catalog.talks.size())
                    fail(WorldResidenceError::page_failed);
                const auto &talk = catalog.talks.at(command[1]);
                value.kind = WorldScriptPageKind::dialogue;
                value.legacy_page = 0;
                value.source_record = command[1];
                value.replacement = scripts.village_name;
                value.title = talk.name;
                value.speaker_kind = talk.speaker_kind;
                value.speaker_definition = talk.speaker_definition;
                value.legacy_tag = talk.legacy_tag;
                for (const auto &paragraph : talk.paragraphs)
                    value.paragraphs.push_back(substitute(paragraph, scripts.village_name));
            } else if (command[0] >= 24 && command[0] <= 28) {
                value.kind = WorldScriptPageKind::raw_page;
                value.legacy_page = 94;
                value.legacy_r = command[0] == 24 ? 3 : command[0] - 20;
                value.legacy_s = command[1];
            } else
                fail(WorldResidenceError::unsupported_program);
            // d/a override只在k==-1时替换j/k；m总是绑定住户，不覆盖已显式说话者。
            if (value.speaker_definition == -1) {
                value.speaker_kind = 1;
                value.speaker_definition = human;
            }
            page(std::move(value));
        }
        if (!world_script_seen(scripts, 202))
            invoke(202, scripts.village_name + "\t" + resident.localized_name);
        return {WorldResidenceError::none, std::move(candidate)};
    } catch (const Failure &failure) {
        return {failure.error, {}};
    }
}
} // namespace dungeon_village_reference
