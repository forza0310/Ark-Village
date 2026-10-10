#include "ark/simulation/village/rules/world_popularity.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool value, const char *what) {
    ++checks;
    if (!value)
        throw std::runtime_error(what);
}
std::string read(const std::filesystem::path &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("missing fixed script table");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
WorldPopularityState fixture(const std::vector<WorldPopularityReward> &rewards) {
    WorldPopularityState s;
    s.rewards = rewards;
    s.scripts.village_name = "FIXTURE";
    WorldScriptPage scene;
    scene.id = 1;
    s.scripts.pages = {scene};
    s.scripts.executing_page = 1;
    s.scripts.next_page_id = 2;
    // 完整目录资格是显式隔离夹具；下述真实奖励表不作为新局人物/设施初值。
    s.scripts.human_catalog_complete = true;
    s.scripts.facility_catalog_complete = true;
    for (int id = 0; id < 100; ++id)
        s.scripts.humans.emplace(id, WorldScriptUnlockDefinition{0, false, "fixture", id % 10, 0});
    return s;
}
void all_rewards(const WorldScriptCatalog &catalog,
                 const std::vector<WorldPopularityReward> &rows) {
    auto s = fixture(rows);
    for (int step = 0; step < 100; ++step) {
        const auto r = prepare_world_popularity(catalog, s, 100, true);
        if (!r.candidate)
            throw std::runtime_error("reward step " + std::to_string(step) + " error " +
                                     std::to_string(static_cast<int>(r.script_error)));
        check(r.candidate->awarded_definition == rows[step].definition &&
                  r.candidate->state.rewards[step].status == 1 &&
                  r.candidate->state.rewards[step].pending_notice,
              "one source-order reward at exact threshold, definition unlock committed now");
        check(!r.candidate->state.scripts.event_calls.count(1000 + step) &&
                  r.candidate->state.scripts.event_calls.at(124) == step + 1 &&
                  r.candidate->state.scripts.event_calls.at(125) == step + 1,
              "reward program never counted as an aL event; 124/125 count each actual call");
        s = r.candidate->state;
        const auto registered = world_popularity_script_catalog(catalog, rows);
        for (int tick = 0; tick < 110; ++tick) {
            const auto advanced = prepare_world_script_continuations(*registered, s.scripts, true);
            check(advanced.candidate.has_value(),
                  "reward continuation restores original1000+index and consumes actual commands");
            s.scripts = advanced.candidate->state;
        }
    }
    check(s.popularity == 10000 && s.maximum == 10000 && s.pulses.size() == 1 &&
              s.pulses.front() == std::array<int, 3>{0, 3, 10000},
          "unbounded popularity and pulse merge preserve original all-row sums");
    for (int id : {220, 221, 222, 223})
        check(world_script_seen(s.scripts, id), "threshold newspaper event fires once");
    auto r = prepare_world_popularity(catalog, s, 0, true);
    check(r.candidate && !r.candidate->awarded_definition && r.candidate->invoked_events.empty(),
          "zero delta no repeat at exactly10000");
    r = prepare_world_popularity(catalog, s, 150, false);
    check(r.candidate && !r.candidate->awarded_definition &&
              r.candidate->invoked_events == std::vector<int>({124, 125}) &&
              r.candidate->state.popularity == 10150,
          "above10000 peak crosses a hundred replays last reward without reopening definition");
}
void guards(const WorldScriptCatalog &catalog, const std::vector<WorldPopularityReward> &rows) {
    auto s = fixture(rows);
    s.popularity = 50;
    s.maximum = 0; // c/n.J 303..305，不把初始最高值补成50。
    auto initial = prepare_world_popularity(catalog, s, 0, false);
    check(initial.candidate && initial.candidate->state.popularity == 50 &&
              initial.candidate->state.maximum == 50 && !initial.candidate->awarded_definition &&
              s.maximum == 0,
          "real reset50/peak0 accepted; first call advances peak without eager reward");
    initial = prepare_world_popularity(catalog, s, -10, false);
    check(initial.candidate && initial.candidate->state.popularity == 40 &&
              initial.candidate->state.maximum == 40,
          "first call peak follows resulting value not pre-call50");
    s.maximum = -1;
    check(!prepare_world_popularity(catalog, s, 0, false).candidate,
          "negative source-impossible maximum still rejected");
    s = fixture(rows);
    auto r = prepare_world_popularity(catalog, s, -1000, true);
    check(r.candidate && r.candidate->state.popularity == -50 &&
              r.candidate->state.pulses == std::vector<std::array<int, 3>>{{0, 0, -1000}},
          "floor affects popularity but pulse keeps requested delta");
    s.popularity = s.maximum = 500;
    r = prepare_world_popularity(catalog, s, 0, false);
    check(r.candidate && r.candidate->awarded_definition == rows.front().definition &&
              r.candidate->state.rewards[1].status == 0,
          "zero delta still catches up only first source-order eligible unclaimed row");
    auto broken = catalog;
    broken.events.erase(125);
    r = prepare_world_popularity(broken, s, 50, true);
    check(!r.candidate && r.error == WorldPopularityError::script_failed &&
              s.rewards.front().status == 0 && s.scripts.pages.size() == 1 && s.pulses.empty(),
          "late missing125 restores popularity, pulse, directory and already-created124 dialogue");
    s.maximum = s.popularity = std::numeric_limits<int>::max();
    check(prepare_world_popularity(catalog, s, 1, true).error == WorldPopularityError::overflow,
          "overflow rejects instead of corrupting shared popularity");
    s = fixture(rows);
    r = prepare_world_popularity(catalog, s, 100, true);
    check(r.candidate && r.candidate->state.reward_display,
          "actual reward sets HUD latch before124 and reward program");
    s = r.candidate->state;
    for (auto &page : s.scripts.pages)
        if (page.legacy_page == 97) {
            page.lifecycle = 2;
            const auto unlocked = prepare_world_popularity_unlock_page(s, page.id);
            check(unlocked.candidate && !unlocked.candidate->state.reward_display &&
                      unlocked.candidate->state.scripts.event_calls == s.scripts.event_calls,
                  "page97 actual update clears R and closes without rerunning script/world");
            break;
        }
    check(!prepare_world_popularity_unlock_page(s, 1).candidate,
          "closing scene or arbitrary dialogue cannot clear reward latch");
}
} // namespace
int main() {
    try {
        const auto root =
            std::filesystem::path(ARK_WORLD_TEST_DATA) / "scripts/original";
        const auto catalog =
            parse_world_script_catalog(read(root / "events.txt"), read(root / "talk.txt"),
                                       read(root / "news.txt"), read(root / "evtmsgs.txt"));
        const auto rewards = parse_world_popularity_rewards(read(root / "popularBonus.txt"));
        check(catalog.catalog && rewards.rewards && rewards.rewards->size() == 100,
              "fixed100 reward definitions parsed without sorting or changing source bytes");
        check(!parse_world_popularity_rewards("0\t0\t100\t0\t-1\t6,100\t2\n\n").rewards &&
                  !parse_world_popularity_rewards("0\t0\t100\t0\t-1\t6,x\t2").rewards,
              "blank rows and invalid program numbers reject strictly");
        all_rewards(*catalog.catalog, *rewards.rewards);
        guards(*catalog.catalog, *rewards.rewards);
        std::cout << checks << " popularity checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
