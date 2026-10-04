#include "dungeon_village_reference/world_scripts.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>

namespace dungeon_village_reference {
namespace {
struct Failure {
    WorldScriptError error;
};
[[noreturn]] void fail(WorldScriptError error) { throw Failure{error}; }
bool utf8(const std::string &text) {
    for (std::size_t i = 0; i < text.size();) {
        const auto first = static_cast<unsigned char>(text[i++]);
        if (first < 128) {
            if (first == 0)
                return false;
            continue;
        }
        int count{};
        std::uint32_t value{}, minimum{};
        if (first >= 0xc2 && first <= 0xdf) {
            count = 1;
            value = first & 0x1f;
            minimum = 0x80;
        } else if (first >= 0xe0 && first <= 0xef) {
            count = 2;
            value = first & 0x0f;
            minimum = 0x800;
        } else if (first >= 0xf0 && first <= 0xf4) {
            count = 3;
            value = first & 7;
            minimum = 0x10000;
        } else
            return false;
        while (count-- > 0) {
            if (i == text.size())
                return false;
            const auto next = static_cast<unsigned char>(text[i++]);
            if ((next & 0xc0) != 0x80)
                return false;
            value = (value << 6) | (next & 0x3f);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
            return false;
    }
    return true;
}
std::vector<std::string> split(const std::string &text, char delimiter) {
    std::vector<std::string> result;
    std::size_t start{};
    for (;;) {
        const auto end = text.find(delimiter, start);
        result.push_back(text.substr(start, end == std::string::npos ? end : end - start));
        if (end == std::string::npos)
            return result;
        start = end + 1;
    }
}
int integer(const std::string &text) {
    int result{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (text.empty() || parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
        fail(WorldScriptError::invalid_catalog);
    return result;
}
std::string trim(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n\f\v");
    if (first == std::string::npos)
        return {};
    return text.substr(first, text.find_last_not_of(" \t\r\n\f\v") - first + 1);
}
WorldScriptProgram program(const std::string &text) {
    WorldScriptProgram result;
    if (text.empty())
        return result;
    for (const auto &row : split(text, '&')) {
        std::vector<int> command;
        for (const auto &field : split(row, ','))
            command.push_back(integer(field));
        result.push_back(std::move(command));
    }
    return result;
}
void validate(const WorldScriptCatalog &catalog, const WorldScriptState &state) {
    if (state.next_page_id == 0 || !utf8(state.village_name))
        fail(WorldScriptError::invalid_input);
    std::set<std::uint64_t> ids;
    for (const auto &page : state.pages)
        if (page.id == 0 || page.id >= state.next_page_id || page.lifecycle < 0 ||
            page.lifecycle > 4 || !ids.insert(page.id).second)
            fail(WorldScriptError::invalid_input);
    if (state.executing_page && !ids.count(*state.executing_page))
        fail(WorldScriptError::invalid_input);
    for (const auto &selection :
         {state.selected_actor, state.selected_monster, state.selected_facility})
        if (selection && *selection == 0)
            fail(WorldScriptError::invalid_input);
    if (std::any_of(state.human_order.begin(), state.human_order.end(),
                    [](std::uint64_t id) { return id == 0; }))
        fail(WorldScriptError::invalid_input);
    for (const auto &entry : state.event_calls)
        if (entry.first < 0 || entry.second < 0)
            fail(WorldScriptError::invalid_input);
    for (const auto &entry : catalog.events)
        if (entry.first < 0 || entry.first != entry.second.id)
            fail(WorldScriptError::invalid_catalog);
    for (const auto &entry : catalog.programs)
        if (entry.first < 1000 || entry.first >= 3000 || catalog.events.count(entry.first))
            fail(WorldScriptError::invalid_catalog);
    for (const auto &continuation : state.continuations) {
        const auto event = catalog.events.find(continuation.event);
        const auto extra = catalog.programs.find(continuation.event);
        if (event == catalog.events.end() && extra == catalog.programs.end())
            fail(WorldScriptError::missing_event);
        const auto &commands =
            event != catalog.events.end() ? event->second.commands : extra->second;
        if (continuation.next_instruction > commands.size() ||
            continuation.remaining_updates == std::numeric_limits<int>::min() ||
            !utf8(continuation.replacement))
            fail(WorldScriptError::invalid_input);
    }
}
std::string substitute(std::string text, const std::string &replacement) {
    const auto fields = split(replacement, '\t');
    for (std::size_t index = 0; index < fields.size(); ++index) {
        const std::string token = "<" + std::to_string(index) + ">";
        if (fields[index].find(token) != std::string::npos)
            fail(WorldScriptError::invalid_input); // 原循环会不终止；拒绝而非悄悄改语义。
        std::size_t start{};
        while ((start = text.find(token, start)) != std::string::npos) {
            text.replace(start, token.size(), fields[index]);
            start += fields[index].size();
        }
    }
    return text;
}
void page(WorldScriptCandidate &candidate, WorldScriptPage value) {
    auto &state = candidate.state;
    if (state.next_page_id == std::numeric_limits<std::uint64_t>::max())
        fail(WorldScriptError::numeric_overflow);
    value.id = state.next_page_id++;
    value.lifecycle = 0;
    candidate.last_page = value.id;
    if (state.page_mutations_locked)
        return;
    const auto anchor = state.executing_page ? state.executing_page
                                             : (state.pages.empty() ? std::optional<std::uint64_t>{}
                                                                    : state.pages.back().id);
    auto position = state.pages.end();
    if (anchor) {
        const auto current = std::find_if(state.pages.begin(), state.pages.end(),
                                          [&](const auto &p) { return p.id == *anchor; });
        if (current != state.pages.end() && current->lifecycle != 0 && current->lifecycle != 4)
            position = current + 1;
    }
    state.pages.insert(position, value);
    state.redraw_requested = true;
    candidate.inserted_pages.push_back(std::move(value));
}
bool close_page(WorldScriptState &state, std::uint64_t identity) {
    const auto page = std::find_if(state.pages.begin(), state.pages.end(),
                                   [&](const auto &value) { return value.id == identity; });
    if (page == state.pages.end())
        return false;
    page->lifecycle = 4;
    state.redraw_requested = true;
    for (auto current = state.pages.rbegin(); current != state.pages.rend(); ++current)
        if (current->lifecycle != 4) {
            if (current->lifecycle == 3)
                current->lifecycle = 1;
            break;
        }
    return true;
}
void instruction_size(const std::vector<int> &command, std::size_t minimum, std::size_t maximum) {
    if (command.size() < minimum || command.size() > maximum)
        fail(WorldScriptError::invalid_input);
}
struct Runner {
    const WorldScriptCatalog &catalog;
    WorldScriptCandidate &candidate;
    std::size_t remaining;
    std::set<int> active;
    void raw_page(int identity, int r = 0, int s = 0, int t = 0) {
        WorldScriptPage value;
        value.kind = WorldScriptPageKind::raw_page;
        value.legacy_page = identity;
        value.legacy_r = r;
        value.legacy_s = s;
        value.legacy_t = t;
        page(candidate, std::move(value));
    }
    void notice(int identity, const std::string &replacement, int delay) {
        static const std::map<int, std::string> templates{
            {6, "举办活动追加 <co=0064FF><0></co> "},
            {9, "冒险者的满足度上升 <co=0064FF><0></co> !"},
            {21, "<co=0064FF>勋章</co> 入手"},
            {23, "商店可以使用了"},
            {34, "<co=0064FF><0></co> 入手"}};
        const auto found = templates.find(identity);
        if (found == templates.end())
            fail(WorldScriptError::missing_presentation);
        candidate.state.notices.push_back(
            {identity, -delay, 80, replacement, substitute(found->second, replacement)});
    }
    WorldScriptUnlockDefinition &definition(std::map<int, WorldScriptUnlockDefinition> &values,
                                            int identity) {
        const auto found = values.find(identity);
        if (found == values.end())
            fail(WorldScriptError::invalid_input);
        return found->second;
    }
    void refresh_category3() {
        auto &state = candidate.state;
        if (!state.human_catalog_complete || !state.facility_catalog_complete)
            fail(WorldScriptError::invalid_input);
        state.job_counts.fill(0);
        for (const auto &human : state.humans)
            if (human.second.status != 0) {
                const int category = human.second.extra;
                if (category < 0 || category >= 10)
                    fail(WorldScriptError::invalid_input);
                if (state.job_counts[category] == std::numeric_limits<std::int32_t>::max())
                    fail(WorldScriptError::numeric_overflow);
                ++state.job_counts[category];
            }
        for (auto &entry : state.facilities) {
            auto &facility = entry.second;
            if (facility.category != 3)
                continue;
            FacilityEconomyInput input;
            input.level = facility.level;
            input.definition_improvements = facility.improvements;
            input.legacy_job_counts = state.job_counts;
            const auto result = derive_facility_economy(facility.economy, input);
            if (!result.values)
                fail(WorldScriptError::invalid_input);
            for (std::size_t slot = 0; slot < 4; ++slot) {
                const auto value = result.values->definition_attributes[slot];
                if (value < std::numeric_limits<std::int32_t>::min() ||
                    value > std::numeric_limits<std::int32_t>::max())
                    fail(WorldScriptError::numeric_overflow);
                facility.attributes[slot] = static_cast<std::int32_t>(value);
            }
        }
    }
    void count() {
        if (remaining == 0)
            fail(WorldScriptError::dispatch_limit);
        --remaining;
    }
    const WorldScriptDefinition &event(int id) {
        const auto found = catalog.events.find(id);
        if (found == catalog.events.end())
            fail(WorldScriptError::missing_event);
        return found->second;
    }
    const WorldScriptProgram &commands(int id) {
        const auto extra = catalog.programs.find(id);
        return extra == catalog.programs.end() ? event(id).commands : extra->second;
    }
    void invoke(int id, const std::string &replacement,
                const std::optional<std::vector<int>> &parameters) {
        count();
        (void)event(id);
        auto &calls = candidate.state.event_calls[id];
        if (calls == std::numeric_limits<int>::max())
            fail(WorldScriptError::numeric_overflow);
        ++calls;
        execute(id, 0, replacement, parameters);
    }
    void execute(int id, std::size_t offset, const std::string &replacement,
                 const std::optional<std::vector<int>> &parameters) {
        candidate.entered_program = true;
        if (!active.insert(id).second)
            fail(WorldScriptError::dispatch_limit);
        struct ActiveScope {
            std::set<int> &values;
            int id;
            ~ActiveScope() { values.erase(id); }
        } scope{active, id};
        candidate.last_page.reset();
        const auto &instructions = commands(id);
        for (std::size_t index = offset; index < instructions.size(); ++index) {
            count();
            const auto &original = instructions[index];
            if (original.empty())
                fail(WorldScriptError::invalid_input);
            const int opcode = original[0];
            candidate.executed.push_back({id, index, opcode});
            if (opcode == 6) {
                instruction_size(original, 2, 3);
                if (original[1] == std::numeric_limits<int>::min())
                    fail(WorldScriptError::numeric_overflow);
                candidate.state.continuations.push_back(
                    {id, index + 1, original[1], original.size() == 3 ? original[2] : -1,
                     candidate.state.context, replacement, parameters});
                return; // wait参数先于通用参数引用替换，直接保存原数值。
            }
            auto command = original;
            for (std::size_t field = 1; field < command.size(); ++field)
                if (command[field] <= std::numeric_limits<int>::min() + 99) {
                    const auto parameter =
                        static_cast<std::size_t>(static_cast<std::int64_t>(command[field]) -
                                                 std::numeric_limits<int>::min());
                    if (!parameters || parameter >= parameters->size())
                        fail(WorldScriptError::invalid_input);
                    command[field] = parameters->at(parameter);
                }
            switch (opcode) {
            case 0: {
                instruction_size(command, 2, 2);
                if (!candidate.state.finance)
                    fail(WorldScriptError::invalid_input);
                auto &finance = *candidate.state.finance;
                if (finance.month < 0 || finance.month >= 12 ||
                    command[1] == std::numeric_limits<int>::min())
                    fail(WorldScriptError::invalid_input);
                const int amount = command[1] < 0 ? -command[1] : command[1];
                const int slot = command[1] < 0 ? 1 : 0;
                auto &total = finance.monthly_totals[finance.month][4][slot];
                if (static_cast<std::int64_t>(total) + amount > std::numeric_limits<int>::max() ||
                    (command[1] > 0 &&
                     finance.cash > std::numeric_limits<std::int64_t>::max() - command[1]) ||
                    (command[1] < 0 &&
                     finance.cash < std::numeric_limits<std::int64_t>::min() - command[1]))
                    fail(WorldScriptError::numeric_overflow);
                finance.cash += command[1];
                total += amount;
                if (slot == 0) {
                    if (finance.legacy_flags14 == 0 && finance.cash > finance.cash_peak) {
                        finance.cash_peak = finance.cash;
                        finance.cash_peak_village = candidate.state.village_name;
                    }
                    if (!finance.localized_gold_template)
                        fail(WorldScriptError::missing_presentation);
                    notice(34, substitute(*finance.localized_gold_template, std::to_string(amount)),
                           1);
                }
                candidate.last_page.reset();
                break;
            }
            case 1:
                instruction_size(command, 2, 2);
                invoke(command[1], replacement, parameters);
                break;
            case 2:
            case 3: {
                instruction_size(command, opcode == 3 ? 4 : 2, opcode == 3 ? 4 : 2);
                if (command[1] < 0 || static_cast<std::size_t>(command[1]) >= catalog.talks.size())
                    fail(WorldScriptError::missing_presentation);
                const auto &talk = catalog.talks[command[1]];
                WorldScriptPage value;
                value.kind = WorldScriptPageKind::dialogue;
                value.legacy_page = 0;
                value.source_record = command[1];
                value.replacement = replacement;
                value.title = talk.name;
                value.speaker_kind = talk.speaker_kind;
                value.speaker_definition = talk.speaker_definition;
                if (opcode == 3) {
                    if (command[2] != -1)
                        value.speaker_kind = command[2];
                    if (command[3] != -1)
                        value.speaker_definition = command[3];
                }
                value.legacy_tag = talk.legacy_tag;
                for (const auto &paragraph : talk.paragraphs)
                    value.paragraphs.push_back(substitute(paragraph, replacement));
                page(candidate, std::move(value));
                break;
            }
            case 4: {
                instruction_size(command, 2, 2);
                if (command[1] < 0 || static_cast<std::size_t>(command[1]) >= catalog.news.size())
                    fail(WorldScriptError::missing_presentation);
                const auto &news = catalog.news[command[1]];
                WorldScriptPage announcement;
                announcement.kind = WorldScriptPageKind::simple_message;
                announcement.legacy_page = 1;
                announcement.paragraphs = {"大家的冒险通信<br>发行了"};
                page(candidate, std::move(announcement));
                WorldScriptPage newspaper;
                newspaper.kind = WorldScriptPageKind::newspaper;
                newspaper.legacy_page = 15;
                newspaper.source_record = command[1];
                newspaper.replacement = replacement;
                newspaper.title = news.title; // 原V[i]不做替换，只替换W[i]正文。
                newspaper.paragraphs = {substitute(news.text, replacement)};
                newspaper.legacy_tag = news.legacy_tag;
                page(candidate, std::move(newspaper));
                break;
            }
            case 5: {
                instruction_size(command, 2, 2);
                if (command[1] < 0 ||
                    static_cast<std::size_t>(command[1]) >= catalog.event_messages.size())
                    fail(WorldScriptError::missing_presentation);
                WorldScriptPage value;
                value.kind = WorldScriptPageKind::raw_page;
                value.legacy_page = 11;
                value.source_record = command[1];
                value.replacement = replacement;
                for (const auto &message : catalog.event_messages[command[1]]) {
                    value.message_commands.push_back(message.first);
                    value.paragraphs.push_back(substitute(message.second, replacement));
                }
                page(candidate, std::move(value));
                break;
            }
            case 7: {
                instruction_size(command, 2, 2);
                WorldScriptPage value;
                value.kind = WorldScriptPageKind::raw_page;
                value.legacy_page = 16;
                value.legacy_l = command[1];
                page(candidate, std::move(value));
                break;
            }
            case 12:
                instruction_size(command, 1, 2);
                if (!candidate.state.human_order.empty()) {
                    candidate.state.selected_actor = candidate.state.human_order.front();
                    candidate.state.scene_mode = 6;
                    candidate.state.scene_updates = 0;
                    candidate.state.scene_labels = {"", ""};
                    candidate.state.selected_facility.reset();
                    candidate.state.selection_mode = 1;
                }
                candidate.last_page.reset();
                break;
            case 13:
            case 14: {
                instruction_size(command, 1, 2);
                candidate.state.selected_actor.reset();
                candidate.state.selected_monster.reset();
                candidate.state.selected_facility.reset();
                WorldScriptPage value;
                value.kind = WorldScriptPageKind::raw_page;
                value.legacy_page = opcode == 13 ? 56 : 57;
                page(candidate, std::move(value));
                candidate.last_page.reset(); // 原13分支gVar=null，已有页面仍在栈。
                break;
            }
            case 15:
                instruction_size(command, 1, 2);
                invoke(21, candidate.state.village_name, {});
                raw_page(87);
                break;
            case 16:
                instruction_size(command, 1, 2);
                raw_page(49);
                break;
            case 17: {
                instruction_size(command, 2, 2);
                auto &activity = definition(candidate.state.activities, command[1]);
                if (activity.status != 1) {
                    if (activity.status == 0)
                        activity.pending_notice = true;
                    activity.status = 1;
                    if (!world_script_seen(candidate.state, 61))
                        invoke(61, activity.name, {});
                    const bool has_notice =
                        std::any_of(candidate.state.notices.begin(), candidate.state.notices.end(),
                                    [](const auto &n) { return n.message == 6; });
                    if (!has_notice)
                        notice(6, activity.name, 6);
                }
                candidate.last_page.reset();
                break;
            }
            case 18:
                instruction_size(command, 2, 2);
                if (!candidate.state.human_catalog_complete)
                    fail(WorldScriptError::invalid_input);
                for (auto &human : candidate.state.humans)
                    if (human.second.status != 0) {
                        const auto changed =
                            static_cast<std::int64_t>(human.second.satisfaction) + command[1];
                        if (changed < std::numeric_limits<int>::min() ||
                            changed > std::numeric_limits<int>::max())
                            fail(WorldScriptError::numeric_overflow);
                        human.second.satisfaction =
                            static_cast<int>(std::clamp<std::int64_t>(changed, 0, 100));
                    }
                notice(9, std::to_string(command[1]), 1);
                candidate.last_page.reset();
                break;
            case 21: {
                // popularBonus原第37行有第四个未读取字段，原消费者只读取[1]/[2]。
                instruction_size(command, 2, 4);
                auto &human = definition(candidate.state.humans, command[1]);
                WorldScriptPage value;
                value.kind = WorldScriptPageKind::raw_page;
                value.legacy_page = 59;
                value.legacy_f = command[1];
                value.legacy_g = command.size() >= 3 ? command[2] : 0;
                page(candidate, std::move(value));
                if (human.status == 0) {
                    human.pending_notice = true;
                    refresh_category3(); // i()中的o.f先于p1，因此计数使用旧p0！
                }
                human.status = 1;
                break;
            }
            case 22:
                instruction_size(command, 1, 2);
                candidate.state.popularity_queue.insert(
                    candidate.state.popularity_queue.begin(),
                    {10, candidate.state.pending_completion, 1});
                candidate.state.pending_completion = 0;
                candidate.last_page.reset();
                break;
            case 29:
                instruction_size(command, 1, 2);
                if (candidate.state.medal_count == std::numeric_limits<int>::max())
                    fail(WorldScriptError::numeric_overflow);
                ++candidate.state.medal_count;
                notice(21, "", 20);
                candidate.last_page.reset();
                break;
            case 30:
                instruction_size(command, 1, 2);
                raw_page(95, 9);
                break;
            case 31: {
                instruction_size(command, 2, 2);
                auto &profession = definition(candidate.state.professions, command[1]);
                raw_page(95, 4, command[1], profession.extra == -1 ? 0 : profession.extra);
                if (profession.status == 0)
                    profession.pending_notice = true;
                profession.status = 1;
                break;
            }
            case 32:
                instruction_size(command, 2, 2);
                raw_page(95, 10);
                break;
            case 33:
            case 34:
                instruction_size(command, 2, 2);
                raw_page(95, opcode == 33 ? 11 : 3, command[1]);
                break;
            case 35:
                instruction_size(command, 1, 2);
                raw_page(97);
                break;
            case 36:
                instruction_size(command, 1, 2);
                candidate.state.user_flags |= 16;
                invoke(120, candidate.state.village_name, {});
                raw_page(83);
                invoke(121, candidate.state.village_name, {});
                notice(23, "", 1);
                candidate.last_page.reset();
                break;
            case 37:
                instruction_size(command, 1, 2);
                raw_page(98);
                break;
            case 38:
            case 39: {
                instruction_size(command, 1, 2);
                const int phase = candidate.state.exploration_phase;
                if (phase < 0 || phase > 6)
                    fail(WorldScriptError::invalid_input);
                constexpr int t[]{3000, 3000, 5000, 10000, 15000, 20000, 20000};
                constexpr int u[]{100, 100, 150, 200, 250, 300, 300};
                raw_page(95, opcode == 38 ? 0 : 1, opcode == 38 ? t[phase] : u[phase]);
                break;
            }
            default:
                fail(WorldScriptError::unsupported_opcode);
            }
        }
    }
};
bool conditions(const WorldScriptDefinition &event, const WorldScriptState &state,
                const WorldScriptAutomaticInput &input) {
    for (const auto &condition : event.conditions) {
        instruction_size(condition, 2, 5);
        std::int64_t value{};
        bool accepted{true};
        std::size_t bounds = 2;
        switch (condition[1]) {
        case 0:
            accepted = false;
            break;
        case 1:
            if (!state.finance)
                fail(WorldScriptError::invalid_input);
            value = state.finance->cash;
            break;
        case 2:
            if (condition.size() < 3)
                fail(WorldScriptError::invalid_input);
            accepted = world_script_seen(state, condition[2]);
            bounds = 3;
            break;
        case 3:
        case 4:
        case 5: {
            const auto counter = condition[1] == 3   ? input.year_counter
                                 : condition[1] == 4 ? input.month_counter
                                                     : input.subperiod_counter;
            if (!counter || *counter == std::numeric_limits<int>::max())
                fail(WorldScriptError::invalid_input);
            value = *counter + 1;
            break;
        }
        default:
            break; // 原小函数默认j0/zF=true，然后仍接受区间和反向。
        }
        if (accepted && bounds < condition.size())
            accepted = condition[bounds] <= value &&
                       (bounds + 1 == condition.size() || value <= condition[bounds + 1]);
        if (condition[0] == 1)
            accepted = !accepted;
        if (!accepted)
            return false;
    }
    return true;
}
} // namespace

WorldScriptCatalogResult parse_world_script_catalog(const std::string &events,
                                                    const std::string &talks,
                                                    const std::string &news,
                                                    const std::string &event_messages) {
    try {
        if (!utf8(events) || !utf8(talks) || !utf8(news) || events.empty() || talks.empty() ||
            news.empty())
            fail(WorldScriptError::invalid_catalog);
        WorldScriptCatalog result;
        auto rows = split(events, '\n');
        if (!rows.empty() && rows.back().empty())
            rows.pop_back();
        for (auto row : rows) {
            if (!row.empty() && row.back() == '\r')
                row.pop_back();
            const auto fields = split(row, '\t');
            if (fields.size() != 5)
                fail(WorldScriptError::invalid_catalog);
            const int id = integer(fields[0]);
            WorldScriptDefinition definition{id, fields[1], integer(fields[2]), program(fields[3]),
                                             program(fields[4])};
            if (id < 0 || !result.events.emplace(id, std::move(definition)).second)
                fail(WorldScriptError::invalid_catalog);
        }
        for (const auto &row : split(talks, '|')) {
            const auto fields = split(row, '\t');
            if (fields.size() < 5)
                fail(WorldScriptError::invalid_catalog);
            WorldScriptTalk talk{
                fields[0], integer(fields[1]), integer(fields[2]), integer(fields[3]), {}};
            talk.paragraphs.assign(fields.begin() + 4, fields.end());
            result.talks.push_back(std::move(talk));
        }
        for (const auto &row : split(news, '|')) {
            const auto fields = split(row, '\t');
            if (fields.size() != 3)
                fail(WorldScriptError::invalid_catalog);
            result.news.push_back({integer(fields[0]), fields[1], fields[2]});
        }
        if (!event_messages.empty()) {
            if (!utf8(event_messages))
                fail(WorldScriptError::invalid_catalog);
            for (const auto &row : split(event_messages, '|')) {
                std::vector<std::pair<int, std::string>> messages;
                for (const auto &field : split(trim(row), '\t')) {
                    const auto delimiter = field.find('&');
                    if (delimiter == std::string::npos)
                        fail(WorldScriptError::invalid_catalog);
                    messages.emplace_back(integer(field.substr(0, delimiter)),
                                          field.substr(delimiter + 1));
                }
                result.event_messages.push_back(std::move(messages));
            }
        }
        return {WorldScriptError::none, std::move(result)};
    } catch (const Failure &failure) {
        return {failure.error, {}};
    }
}
bool world_script_seen(const WorldScriptState &state, int event) {
    if (event < 0)
        return false;
    const auto calls = state.event_calls.find(event);
    return calls != state.event_calls.end() && calls->second > 0;
}
std::optional<WorldScriptProgram> parse_world_script_program(const std::string &text) {
    try {
        return program(text);
    } catch (const Failure &) {
        return {};
    }
}
WorldScriptResult prepare_world_script_program(const WorldScriptCatalog &catalog,
                                               const WorldScriptState &state,
                                               const WorldScriptInput &input) {
    try {
        validate(catalog, state);
        if (!catalog.programs.count(input.event) || input.dispatch_limit == 0 ||
            (input.replacement && !utf8(*input.replacement)))
            fail(WorldScriptError::invalid_input);
        WorldScriptCandidate candidate{state, {}, {}, {}};
        Runner runner{catalog, candidate, input.dispatch_limit, {}};
        runner.execute(input.event, 0, input.replacement.value_or(state.village_name),
                       input.parameters);
        return {WorldScriptError::none, std::move(candidate)};
    } catch (const Failure &failure) {
        return {failure.error, {}};
    }
}
WorldScriptResult prepare_world_script(const WorldScriptCatalog &catalog,
                                       const WorldScriptState &state,
                                       const WorldScriptInput &input) {
    try {
        validate(catalog, state);
        if (input.dispatch_limit == 0 || (input.replacement && !utf8(*input.replacement)))
            fail(WorldScriptError::invalid_input);
        WorldScriptCandidate candidate{state, {}, {}, {}};
        Runner runner{catalog, candidate, input.dispatch_limit, {}};
        runner.invoke(input.event, input.replacement.value_or(state.village_name),
                      input.parameters);
        return {WorldScriptError::none, std::move(candidate)};
    } catch (const Failure &failure) {
        return {failure.error, {}};
    }
}
WorldScriptResult prepare_world_script_continuations(const WorldScriptCatalog &catalog,
                                                     const WorldScriptState &state, bool admitted,
                                                     std::size_t dispatch_limit, bool first_only) {
    try {
        validate(catalog, state);
        if (dispatch_limit == 0)
            fail(WorldScriptError::invalid_input);
        WorldScriptCandidate candidate{state, {}, {}, {}};
        Runner runner{catalog, candidate, dispatch_limit, {}};
        if (admitted)
            for (std::size_t index = 0; index < candidate.state.continuations.size();) {
                runner.count();
                auto &current = candidate.state.continuations[index];
                --current.remaining_updates;
                if (current.remaining_updates > 0) {
                    ++index;
                    continue;
                }
                const auto resumed = current;
                candidate.state.continuations.erase(candidate.state.continuations.begin() + index);
                candidate.state.context = resumed.saved_context;
                runner.execute(resumed.event, resumed.next_instruction, resumed.replacement,
                               resumed.parameters);
                if (first_only)
                    break;
            }
        return {WorldScriptError::none, std::move(candidate)};
    } catch (const Failure &failure) {
        return {failure.error, {}};
    }
}
WorldScriptResult prepare_world_script_close_page(const WorldScriptState &state,
                                                  std::uint64_t page_id) {
    WorldScriptCandidate candidate{state, {}, {}, {}};
    if (!close_page(candidate.state, page_id))
        return {WorldScriptError::invalid_input, {}};
    return {WorldScriptError::none, std::move(candidate)};
}
WorldScriptResult prepare_world_script_page(const WorldScriptState &state,
                                            const WorldScriptPage &value) {
    try {
        if (state.next_page_id == 0)
            fail(WorldScriptError::invalid_input);
        std::set<std::uint64_t> ids;
        for (const auto &existing : state.pages)
            if (existing.id == 0 || existing.id >= state.next_page_id || existing.lifecycle < 0 ||
                existing.lifecycle > 4 || !ids.insert(existing.id).second)
                fail(WorldScriptError::invalid_input);
        if (state.executing_page && !ids.count(*state.executing_page))
            fail(WorldScriptError::invalid_input);
        WorldScriptCandidate candidate{state, {}, {}, {}};
        page(candidate, value);
        return {WorldScriptError::none, std::move(candidate)};
    } catch (const Failure &failure) {
        return {failure.error, {}};
    }
}
WorldScriptResult prepare_world_script_automatic(const WorldScriptCatalog &catalog,
                                                 const WorldScriptState &state,
                                                 const WorldScriptAutomaticInput &input) {
    try {
        validate(catalog, state);
        if (input.dispatch_limit == 0)
            fail(WorldScriptError::invalid_input);
        WorldScriptCandidate candidate{state, {}, {}, {}};
        Runner runner{catalog, candidate, input.dispatch_limit, {}};
        if (input.admitted)
            for (const auto &entry : catalog.events) {
                runner.count();
                if (!world_script_seen(candidate.state, entry.first) &&
                    conditions(entry.second, candidate.state, input)) {
                    runner.invoke(entry.first, candidate.state.village_name, {});
                    if (input.first_only)
                        break;
                }
            }
        return {WorldScriptError::none, std::move(candidate)};
    } catch (const Failure &failure) {
        return {failure.error, {}};
    }
}
WorldScriptCameraFocusResult
prepare_world_script_camera_focus(const WorldScriptState &state,
                                  const WorldScriptCameraFocusInput &input) {
    const auto found = std::find_if(state.pages.begin(), state.pages.end(),
                                    [&](const auto &page) { return page.id == input.page; });
    const auto finite = [](const auto &values) {
        return std::all_of(values.begin(), values.end(),
                           [](float value) { return std::isfinite(value); });
    };
    if (found == state.pages.end() || found->legacy_page != 56 ||
        found->kind != WorldScriptPageKind::raw_page || !finite(input.camera) ||
        !finite(input.previous_camera) || !finite(input.previous_velocity) ||
        (input.first_monster_cached_view && !finite(*input.first_monster_cached_view)))
        return {WorldScriptError::invalid_input, {}};
    WorldScriptCameraFocusCandidate candidate{state, input.camera, input.previous_camera,
                                              input.previous_velocity, true};
    if (!input.first_monster_cached_view) {
        close_page(candidate.state, input.page);
        return {WorldScriptError::none, std::move(candidate)};
    }
    const auto target = *input.first_monster_cached_view;
    const float x = target[0] - input.camera[0];
    const float y = target[1] - input.camera[1];
    const float square = x * x + y * y;
    if (!std::isfinite(square))
        return {WorldScriptError::numeric_overflow, {}};
    const float distance = std::sqrt(square);
    const float step = distance < 10 ? 5 : distance > 150 ? 26 : 5 + (distance - 10) * 21 / 140;
    if (distance < step) {
        candidate.camera = target;
        candidate.previous_camera = target;
        close_page(candidate.state, input.page);
    } else {
        candidate.velocity = {x * step / distance, y * step / distance};
        for (std::size_t axis = 0; axis < 2; ++axis) {
            candidate.camera[axis] += candidate.velocity[axis];
            candidate.previous_camera[axis] += candidate.velocity[axis];
        }
        if (!finite(candidate.camera) || !finite(candidate.previous_camera))
            return {WorldScriptError::numeric_overflow, {}};
    }
    return {WorldScriptError::none, std::move(candidate)};
}
} // namespace dungeon_village_reference
