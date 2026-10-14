#include "world_campaign_clear.hpp"
#include "../support/world_fixture.hpp"
#include "ark/assets/sha256.hpp"
#include "ark/simulation/actors/startup_world_profile.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace ref = sim::rules;
using State = app::WorldState;
void require(bool ok, const std::string &message) {
    if (!ok)
        throw std::runtime_error("Campaign lifecycle: " + message);
}
void good(const std::string &error) { require(error.empty(), error); }
int month(const State &s) { return s.scene.calendar.year * 12 + s.scene.calendar.month; }
std::array<std::int64_t, 3> events(const State &s) {
    std::array<std::int64_t, 3> result{};
    for (int n = 0; n < 3; ++n) {
        const auto found = s.scripts.event_calls.find(n + 4);
        if (found != s.scripts.event_calls.end())
            result[n] = found->second;
    }
    return result;
}
bool same_rows(const sim::StartupClearScoreRows &a, const sim::StartupClearScoreRows &b) {
    for (std::size_t n = 0; n < a.size(); ++n)
        if (a[n].count != b[n].count || a[n].score != b[n].score)
            return false;
    return true;
}
bool same_random(const ref::WorldRandomStream &a, const ref::WorldRandomStream &b) {
    const auto x = a.snapshot(), y = b.snapshot();
    return x.engine_state == y.engine_state && x.tape == y.tape && x.cursor == y.cursor &&
           x.tape_mode == y.tape_mode;
}
std::string file_bytes(const std::filesystem::path &path) {
    std::ifstream in(path, std::ios::binary);
    require(bool(in), "cannot read " + path.u8string());
    return {std::istreambuf_iterator<char>(in), {}};
}
std::filesystem::path evidence_path(const std::filesystem::path &directory, int slot) {
    require(slot >= 0 && slot <= 1, "bad evidence slot");
    return directory / ("lifecycle" + std::to_string(slot) + ".txt");
}
std::int64_t sales(const State &s) {
    std::int64_t total{};
    for (const auto &[id, facility] : s.scene.world.world.facilities) {
        (void)id;
        require(facility.sales >= 0 &&
                    total <= std::numeric_limits<std::int64_t>::max() - facility.sales,
                "invalid cumulative sales");
        total += facility.sales;
    }
    return total;
}
void bounded_step(CampaignClearDriver &driver, std::uint64_t &rounds,
                  std::chrono::steady_clock::time_point start) {
    require(rounds < 10000 && std::chrono::steady_clock::now() - start < std::chrono::minutes(6),
            "segment exhausted its 6-minute/10000-round budget; no player prefix certified");
    driver.step();
    ++rounds;
    if (rounds % 500 == 0)
        std::cout << "LIFECYCLE_PROGRESS " << driver.diagnose() << ' ' << driver.evidence.diagnose()
                  << std::endl;
}
} // namespace

void CampaignClearEvidence::observe_world(const State &before, const State &after,
                                          const app::WorldSystemState &system) {
    if (phase_ != 0) {
        require(phase_ != 1 || !app::world_clear_page(before),
                "active score must use its application branch, never world runtime");
        return;
    }
    if (before.completion_mode == after.completion_mode)
        return;
    require(before.completion_mode == 0 && before.system_completion_mode == 0 &&
                after.completion_mode == 1 && after.system_completion_mode == 1 &&
                month(before) == 182 && month(after) == 183,
            "clear did not originate at the real raw15/3 monthly transition");
    const ref::WorldScriptPage *created{};
    for (const auto &page : after.scripts.pages)
        if (page.kind == ref::WorldScriptPageKind::raw_page && page.legacy_page == 17 &&
            page.lifecycle == 0) {
            require(!created, "more than one pending raw17");
            require(std::none_of(before.scripts.pages.begin(), before.scripts.pages.end(),
                                 [&](const auto &old) { return old.id == page.id; }),
                    "clear page predates its natural trigger");
            created = &page;
        }
    require(created && sim::valid_startup_world_clear_page(after, *created) && !system.clear,
            "natural producer did not publish its uninitialized raw17");
    // This is the captured G/p wire boundary, not another score or inheritance algorithm.
    auto capture = [](std::vector<std::uint8_t> &out, int value) {
        require(value >= 0 && value <= 0x7fff, "bad inherited short");
        out.push_back(static_cast<std::uint8_t>(value >> 8));
        out.push_back(static_cast<std::uint8_t>(value));
    };
    for (const auto &definition : after.rules->facilities)
        capture(inheritance_[0], after.scene.world.world.facility_uses.at(definition.id).level);
    for (std::size_t n = 0; n < after.rules->jobs.size(); ++n)
        capture(inheritance_[1], after.scripts.professions.at(int(n)).status);
    require(after.system_unlock_data == inheritance_ &&
                system.records.facility_levels == inheritance_[0] &&
                system.records.profession_status == inheritance_[1],
            "natural endgame G/p snapshot was not committed to system2");
    page_ = created->id;
    trigger_step_ = after.simulation_steps;
    event_base_ = events(after);
    village_ = after.scripts.village_name;
    old_high_ = system.records.high_score;
    old_village_ = system.records.score_village;
    old_trophy_ = system.records.trophy;
    phase_ = 1;
}

bool CampaignClearEvidence::confirm_due(const app::WorldSystemState &system) const {
    require(phase_ == 1, "score input lacks a real captured trigger");
    if (!system.clear)
        return false;
    require(system.clear->page == page_, "score input bound to a different raw17");
    const auto &score = system.clear->score;
    return (score.stage == 0 && score.counter >= 75) || (score.stage == 3 && score.counter >= 45);
}

void CampaignClearEvidence::observe_score(const State &before,
                                          const app::WorldSystemState &old_system,
                                          const State &after, const app::WorldSystemState &system) {
    require(phase_ == 1 && app::world_clear_page(before) &&
                before.simulation_steps == after.simulation_steps &&
                same_random(before.scene.random, after.scene.random) && month(before) == 183 &&
                month(after) == 183 && before.scene.calendar.units == after.scene.calendar.units &&
                before.scene.world.world.ai.accounting.funds() ==
                    after.scene.world.world.ai.accounting.funds() &&
                before.system_unlock_data == after.system_unlock_data,
            "score branch advanced the business world, cash, random or captured inheritance");
    if (!old_system.clear) {
        require(!rows_captured_ && system.clear && system.clear->page == page_ &&
                    system.clear->score.captured_high_score == old_high_,
                "score Init lost the captured old system record");
        rows_ = system.clear->rows;
        rows_captured_ = true;
        sum_ = std::accumulate(rows_.begin(), rows_.end(), std::int64_t{},
                               [](auto sum, const auto &row) { return sum + row.score; });
        require(
            after.sound_requests ==
                std::vector<sim::StartupAudioRequest>{{sim::StartupAudioOperation::replace_bgm, 0}},
            "score Init did not request its one BGM0");
    } else {
        require(rows_captured_ && old_system.clear->page == page_ &&
                    same_rows(old_system.clear->rows, rows_),
                "score rows changed after Init");
        if (system.clear)
            require(same_rows(system.clear->rows, rows_) && after.sound_requests.empty(),
                    "score update rebuilt rows or repeated Init audio");
    }
    ++score_updates_;
    if (system.clear)
        return;
    require(old_system.clear && old_system.clear->score.stage == 6 &&
                old_system.clear->score.sum == sum_ && !app::world_clear_page(after),
            "score retired without the six real accumulated rows");
    const auto expected = events(after);
    const bool high = sum_ > old_high_;
    require(expected[2] == event_base_[2] + 1 &&
                expected[high ? 0 : 1] == event_base_[high ? 0 : 1] + 1 &&
                expected[high ? 1 : 0] == event_base_[high ? 1 : 0] &&
                system.records.high_score == (high ? sum_ : old_high_) &&
                system.records.score_village == (high ? village_ : old_village_) &&
                (!high || (system.records.trophy >= 1 && system.records.trophy <= 4)) &&
                (high || system.records.trophy == old_trophy_) &&
                after.sound_requests ==
                    std::vector<sim::StartupAudioRequest>{
                        {sim::StartupAudioOperation::replace_bgm,
                         after.active_task && after.task.encounter ? 2 : 1}},
            "score tail lost strict-high records, once-only events or return BGM");
    phase_ = 2;
    result_trophy_ = system.records.trophy;
}

void CampaignClearEvidence::verify(const State &s, const app::WorldSystemState &system) const {
    require(!system.clear, "stable evidence cannot own a pending score controller");
    if (phase_ == 0) {
        require(s.completion_mode == 0 && s.system_completion_mode == 0 && month(s) < 183,
                "uncertified completed world cannot masquerade as a pre-clear prefix");
        return;
    }
    require(phase_ >= 2 && rows_captured_ && score_updates_ > 0 && page_ && trigger_step_ &&
                system.records.facility_levels == inheritance_[0] &&
                system.records.profession_status == inheritance_[1] &&
                system.records.high_score == std::max(sum_, old_high_) &&
                system.records.score_village == (sum_ > old_high_ ? village_ : old_village_) &&
                system.records.trophy == result_trophy_,
            "stable prefix lacks its captured score/system/inheritance qualification");
    if (phase_ == 2) {
        const auto now = events(s);
        // Event calls are durable player fields; cold restore must retain their real once tail.
        require(s.completion_mode == 1 && s.system_completion_mode == 1 && month(s) >= 183 &&
                    s.system_unlock_data == inheritance_ && s.scripts.village_name == village_ &&
                    now[2] == event_base_[2] + 1 &&
                    now[0] == event_base_[0] + (sum_ > old_high_ ? 1 : 0) &&
                    now[1] == event_base_[1] + (sum_ > old_high_ ? 0 : 1),
                "completed player world lost durable completion or captured G/p");
    } else {
        require(s.completion_mode == 0 && s.system_completion_mode == 0 && month(s) < 183,
                "inherited new world reused the old world's completed clock");
    }
}

void CampaignClearEvidence::verify_cold_inheritance(const State &cold,
                                                    const app::WorldSystemState &system) {
    require(phase_ == 2 && continued() && cold.simulation_steps == 0 &&
                cold.scene.world.updates == 0 && cold.global_updates == 0 &&
                cold.scene.world.world.ai.battle.actors.empty(),
            "inheritance needs the completed/continued certificate and a real cold Owner");
    require(system.records.facility_levels == inheritance_[0] &&
                system.records.profession_status == inheritance_[1],
            "inheritance changed the date-triggered G/p snapshot");
    const auto defaults = initial_world();
    require(cold.scene.world.world.ai.accounting.funds() ==
                    defaults.scene.world.world.ai.accounting.funds() &&
                cold.rank == defaults.rank && cold.popularity == defaults.popularity &&
                cold.village_points == defaults.village_points &&
                cold.scene.world.facility_order == defaults.scene.world.facility_order &&
                cold.tasks.size() == defaults.tasks.size() &&
                cold.task_progress.successes == defaults.task_progress.successes,
            "inheritance copied the previous world's cash, progress, tasks or layout");
    for (const auto &[id, growth] : cold.scene.world.world.ai.growth) {
        const auto &original = defaults.scene.world.world.ai.growth.at(id).definition;
        require(growth.definition.current_profession == original.current_profession &&
                    growth.definition.profession_levels == original.profession_levels &&
                    growth.definition.extra == original.extra &&
                    growth.definition.learned_spells == original.learned_spells,
                "inheritance confused profession openness with retained human growth/equipment");
    }
    for (const auto &[id, item] : cold.items)
        require(item.inventory == defaults.items.at(id).inventory,
                "inheritance retained old consumable inventory");
    for (const auto &[id, instance] : cold.scene.world.world.facilities) {
        const auto &original = defaults.scene.world.world.facilities.at(id);
        require(instance.placement.definition_id == original.placement.definition_id &&
                    instance.placement.anchor == original.placement.anchor,
                "inheritance reused the old village layout");
        const auto actual = sim::startup_world_facility_values(cold, id);
        require(actual && instance.price == actual->instance_attributes[0],
                "cold inheritance did not rebuild instance price with its real neighbours");
    }
    auto decoded = [](const std::vector<std::uint8_t> &bytes, std::size_t n) {
        require(2 * n + 1 < bytes.size(), "truncated inherited definition");
        return int(bytes[2 * n]) * 256 + bytes[2 * n + 1];
    };
    for (std::size_t n = 0; n < cold.rules->facilities.size(); ++n) {
        const auto id = cold.rules->facilities[n].id;
        const auto quote = sim::startup_world_build_quote(cold, id);
        require(quote &&
                    cold.scene.world.world.facility_uses.at(id).level ==
                        decoded(inheritance_[0], n) &&
                    cold.scripts.facilities.at(id).level == decoded(inheritance_[0], n),
                "cold inheritance lost shared G or actual quote");
        for (std::size_t field = 0; field < 4; ++field)
            require(cold.scripts.facilities.at(id).attributes[field] ==
                        quote->definition_attributes[field],
                    "cold inherited facility did not rebuild actual shared attributes");
    }
    for (std::size_t n = 0; n < cold.rules->jobs.size(); ++n)
        require(cold.scripts.professions.at(int(n)).status == decoded(inheritance_[1], n) &&
                    cold.scene.world.world.ai.professions[n].unlocked ==
                        (decoded(inheritance_[1], n) != 0),
                "cold inheritance lost profession p, distinct from human mastery");
    phase_ = 3;
    verify(cold, system);
}

void CampaignClearEvidence::certify_continuation(int complete_month, std::int64_t revenue,
                                                 std::uint64_t inputs, std::uint64_t effects) {
    require(phase_ == 2 && complete_month > 183 && revenue > 0 && inputs > 0 && effects > 0,
            "continuation needs a whole later business month and real input/effect");
    continued_month_ = complete_month;
    continued_sales_ = revenue;
    continued_inputs_ = inputs;
    continued_effects_ = effects;
}
void CampaignClearEvidence::certify_inherited_business() {
    require(phase_ == 3 && continued(), "inherited business lacks its original continuation");
    inherited_business_ = true;
}

void advance_campaign_clear(State &state, app::WorldSystemState &system,
                            CampaignClearEvidence &evidence, bool confirm,
                            const std::filesystem::path &directory, std::ostream &log) {
    auto candidate = state;
    auto next = system;
    auto observation = evidence;
    good(app::advance_world_clear(candidate, next, confirm));
    observation.observe_score(state, system, candidate, next);
    const bool finished = system.clear && !next.clear;
    good(app::commit_world_system(directory, state, candidate, next, finished));
    if (!system.clear)
        for (std::size_t row = 0; row < next.clear->rows.size(); ++row)
            log << "CLEAR_ROW index=" << row << " count=" << next.clear->rows[row].count
                << " score=" << next.clear->rows[row].score << '\n';
    log << "CLEAR_UPDATE page=" << (system.clear ? system.clear->page : next.clear->page)
        << " confirm=" << confirm << " stage=" << (next.clear ? next.clear->score.stage : 7)
        << " stage_counter=" << (next.clear ? next.clear->score.counter : 0)
        << " row=" << (next.clear ? next.clear->score.row : 6)
        << " sum=" << (next.clear ? next.clear->score.sum : system.clear->score.sum)
        << " high_record=" << next.records.high_score << " system_committed=" << finished << '\n';
    candidate.sound_requests.clear();
    state = std::move(candidate);
    system = std::move(next);
    evidence = std::move(observation);
}

void run_campaign_clear_segment(CampaignClearDriver driver, bool continuation) {
    driver.evidence.verify(driver.state, driver.system);
    require(!driver.evidence.inherited() &&
                (continuation ? driver.evidence.finished() : !driver.evidence.finished()),
            "wrong natural/continuation route for this player prefix");
    const auto start = std::chrono::steady_clock::now();
    const int start_month = month(driver.state);
    const auto start_commands = driver.commands(), start_effects = driver.input_effects();
    std::uint64_t rounds{};
    for (;;) {
        const bool ready = driver.ready() && app::world_save_eligible(driver.state);
        if (ready && !continuation &&
            (driver.evidence.finished() || month(driver.state) > start_month))
            break;
        if (ready && continuation && month(driver.state) > start_month + 1) {
            const auto whole = driver.trade.revenue.find(start_month + 1);
            if (whole != driver.trade.revenue.end() && whole->second > 0 &&
                driver.commands() > start_commands && driver.input_effects() > start_effects) {
                driver.evidence.certify_continuation(start_month + 1, whole->second,
                                                     driver.commands() - start_commands,
                                                     driver.input_effects() - start_effects);
                break;
            }
        }
        bounded_step(driver, rounds, start);
    }
    driver.evidence.verify(driver.state, driver.system);
    driver.save(1);
    std::cout << "PASS "
              << (continuation                 ? "continued whole-month business"
                  : driver.evidence.finished() ? "natural score and stable player save"
                                               : "natural route business month prefix")
              << "; " << driver.evidence.diagnose() << "; not all-content completion" << std::endl;
}

void run_campaign_inherited_segment(CampaignClearDriver driver) {
    require(driver.evidence.inherited() && driver.commands() == 0 && driver.input_effects() == 0 &&
                driver.state.simulation_steps == 0,
            "inheritance must start with empty player/effect history");
    const auto start = std::chrono::steady_clock::now();
    const auto original_sales = sales(driver.state);
    std::uint64_t rounds{};
    while (!(driver.ready() && app::world_save_eligible(driver.state) &&
             sales(driver.state) > original_sales && driver.commands() > 0 &&
             driver.input_effects() > 0))
        bounded_step(driver, rounds, start);
    driver.evidence.certify_inherited_business();
    driver.evidence.verify(driver.state, driver.system);
    driver.save(1); // The completed parent world remains the untouched input slot0.
    std::cout << "PASS inherited cold-new-game construction, completion and actual trade; "
              << driver.evidence.diagnose() << "; not all-content completion" << std::endl;
}

void CampaignClearEvidence::encode(std::ostream &out) const {
    require(phase_ != 1, "player evidence cannot save a score in flight");
    out << "ARK_CAMPAIGN_LIFECYCLE_1\n"
        << phase_ << ' ' << page_ << ' ' << trigger_step_ << ' ' << score_updates_ << ' '
        << rows_captured_ << ' ' << inherited_business_ << ' ' << old_high_ << ' ' << sum_ << ' '
        << old_trophy_ << ' ' << result_trophy_ << ' ' << continued_month_ << ' '
        << continued_sales_ << ' ' << continued_inputs_ << ' ' << continued_effects_ << '\n'
        << std::quoted(old_village_) << ' ' << std::quoted(village_) << '\n';
    for (auto count : event_base_)
        out << count << ' ';
    out << '\n';
    for (const auto &row : rows_)
        out << row.count << ' ' << row.score << '\n';
    for (const auto &section : inheritance_) {
        out << section.size();
        for (auto byte : section)
            out << ' ' << int(byte);
        out << '\n';
    }
}

CampaignClearEvidence CampaignClearEvidence::decode(std::istream &in) {
    CampaignClearEvidence result;
    std::string magic;
    int captured{}, inherited{};
    in >> magic >> result.phase_ >> result.page_ >> result.trigger_step_ >> result.score_updates_ >>
        captured >> inherited >> result.old_high_ >> result.sum_ >> result.old_trophy_ >>
        result.result_trophy_ >> result.continued_month_ >> result.continued_sales_ >>
        result.continued_inputs_ >> result.continued_effects_ >> std::quoted(result.old_village_) >>
        std::quoted(result.village_);
    require(bool(in) && magic == "ARK_CAMPAIGN_LIFECYCLE_1" &&
                (result.phase_ == 0 || result.phase_ == 2 || result.phase_ == 3) && captured >= 0 &&
                captured <= 1 && inherited >= 0 && inherited <= 1 && result.old_high_ >= 0 &&
                result.sum_ >= 0 && result.old_trophy_ >= 0 && result.old_trophy_ <= 4 &&
                result.continued_month_ >= -1 && result.result_trophy_ >= 0 &&
                result.result_trophy_ <= 4 && result.continued_month_ <= 12000 &&
                result.continued_sales_ >= 0,
            "invalid lifecycle identity/phase/score");
    result.rows_captured_ = captured != 0;
    result.inherited_business_ = inherited != 0;
    for (auto &count : result.event_base_) {
        in >> count;
        require(bool(in) && count >= 0, "bad captured event count");
    }
    for (auto &row : result.rows_) {
        in >> row.count >> row.score;
        require(bool(in) && row.count >= 0 && row.score >= 0, "bad captured score row");
    }
    for (auto &section : result.inheritance_) {
        std::size_t size{};
        in >> size;
        require(bool(in) && size <= 10000 && size % 2 == 0, "bad inherited section length");
        for (std::size_t n = 0; n < size; ++n) {
            int value{};
            in >> value;
            require(bool(in) && value >= 0 && value <= 255, "bad inherited byte");
            section.push_back(static_cast<std::uint8_t>(value));
        }
    }
    if (result.phase_ == 0) {
        std::ostringstream expected, actual;
        CampaignClearEvidence{}.encode(expected);
        result.encode(actual);
        require(actual.str() == expected.str(), "business phase contains forged clear facts");
    } else {
        require(result.rows_captured_ && result.page_ && result.trigger_step_ &&
                    result.score_updates_ && !result.inheritance_[0].empty() &&
                    !result.inheritance_[1].empty() &&
                    sim::validate_startup_clear_score_page(result.rows_, {}) ==
                        sim::StartupClearScoreError::none &&
                    result.sum_ ==
                        std::accumulate(result.rows_.begin(), result.rows_.end(), std::int64_t{},
                                        [](auto sum, const auto &row) { return sum + row.score; }),
                "completed evidence lacks valid captured rows/G/p");
        require(result.continued() ? result.continued_month_ > 183 && result.continued_sales_ > 0 &&
                                         result.continued_inputs_ && result.continued_effects_
                                   : result.continued_sales_ == 0 && !result.continued_inputs_ &&
                                         !result.continued_effects_,
                "invalid continuation evidence");
        require(result.phase_ == 3 ? result.continued() && result.inherited_business_
                                   : !result.inherited_business_,
                "inherited evidence lacks completed continuation/business");
    }
    in >> std::ws;
    require(in.eof(), "trailing lifecycle evidence");
    return result;
}

std::string CampaignClearEvidence::diagnose() const {
    std::ostringstream out;
    out << "clear_phase=" << phase_ << " clear_page=" << page_ << " trigger_tick=" << trigger_step_
        << " score_updates=" << score_updates_ << " score=" << sum_
        << " captured_high=" << old_high_ << " continued_month=" << continued_month_
        << " inherited_business=" << inherited_business_;
    return out.str();
}

void write_campaign_clear_evidence(const std::filesystem::path &directory, int slot,
                                   const CampaignClearEvidence &evidence) {
    const auto path = evidence_path(directory, slot);
    require(!std::filesystem::exists(path), "existing lifecycle evidence must be preserved");
    std::ofstream out(path, std::ios::binary);
    out << assets::sha256_hex(file_bytes(app::world_save_slot_path(directory, slot))) << '\n';
    evidence.encode(out);
    out.close();
    require(bool(out), "lifecycle evidence write failed; player prefix is not certified");
}
CampaignClearEvidence read_campaign_clear_evidence(const std::filesystem::path &directory, int slot,
                                                   const State &state,
                                                   const app::WorldSystemState &system,
                                                   bool allow_business_without_evidence) {
    const auto path = evidence_path(directory, slot);
    if (!std::filesystem::exists(path)) {
        require(allow_business_without_evidence, "player prefix lacks bound lifecycle evidence");
        CampaignClearEvidence empty;
        empty.verify(state, system);
        return empty;
    }
    std::ifstream in(path, std::ios::binary);
    std::string digest;
    in >> digest;
    require(bool(in) && digest == assets::sha256_hex(
                                      file_bytes(app::world_save_slot_path(directory, slot))),
            "lifecycle sidecar does not match this actual player file");
    auto result = CampaignClearEvidence::decode(in);
    result.verify(state, system);
    return result;
}

} // namespace ark::test
