#include "ark/presentation/world_rank.hpp"

#include <algorithm>
#include <stdexcept>

namespace ark::desktop {
std::string world_rank_conditions(const simulation::StartupWorldRuntimeState &state) {
    const auto terms = simulation::rules::fixed_calendar_task_rank_terms();
    const auto found = terms.find(state.rank);
    if (found == terms.end() || !state.rules || found->second.size() != state.rank_met.size())
        throw std::invalid_argument("Rank condition display requires a valid rank and catalogue");
    std::string text;
    for (std::size_t index = 0; index < found->second.size(); ++index) {
        const auto &term = found->second[index];
        if (index)
            text += '\n';
        // Type3 stores a definition ID as its threshold; its numeric display cache remains zero.
        if (term.type == 3) {
            const auto &definitions = state.rules->facilities;
            const auto definition =
                std::find_if(definitions.begin(), definitions.end(),
                             [&](const auto &entry) { return entry.id == term.threshold; });
            if (definition == definitions.end())
                throw std::invalid_argument("Rank condition facility is absent from catalogue");
            text += "建造" + definition->name;
        } else {
            const char *label{};
            switch (term.type) {
            case 0:
                label = "最高月收入";
                break;
            case 1:
                label = "设施数量";
                break;
            case 2:
                label = "住宅数量";
                break;
            case 4:
                label = "任务成功次数";
                break;
            case 5:
                label = "人气";
                break;
            case 6:
                label = "活动举办次数";
                break;
            default:
                throw std::invalid_argument("Unknown rank condition display type");
            }
            text += std::string(label) + " " + std::to_string(state.rank_values[index]) + " / " +
                    std::to_string(term.threshold);
        }
        text += state.rank_met[index] ? " [满足]" : " [未满足]";
    }
    return text;
}
} // namespace ark::desktop
