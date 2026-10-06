#include "ark/simulation/rules/world_gift_page.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
std::string read(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("missing real scripts");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
WorldScriptCatalog catalog() {
    const auto root = std::filesystem::path(ARK_WORLD_TEST_DATA) / "scripts/original";
    const auto parsed =
        parse_world_script_catalog(read(root / "events.txt"), read(root / "talk.txt"),
                                   read(root / "news.txt"), read(root / "evtmsgs.txt"));
    check(parsed.catalog.has_value(), "actual source scripts parse");
    return *parsed.catalog;
}
// 页/实例/状态为明确夹具；规则直接验证原br/bt/bz/bA/by调用，不用掉落grant代替。
WorldGiftPageState fixture(int mode, int status = 0, int raw = 94) {
    WorldGiftPageState state;
    state.facility.scripts.village_name = "FIXTURE";
    WorldScriptPage scene;
    scene.id = 1;
    scene.lifecycle = 3;
    WorldScriptPage gift;
    gift.id = 2;
    gift.kind = WorldScriptPageKind::raw_page;
    gift.legacy_page = raw;
    gift.legacy_r = mode;
    gift.legacy_s = 1;
    state.facility.scripts.pages = {scene, gift};
    state.facility.scripts.next_page_id = 3;
    state.facility.scripts.executing_page = 2;
    if (raw == 95) {
        auto &scripts = state.facility.scripts;
        scripts.finance = WorldScriptFinance{};
        scripts.finance->cash = 1000;
        scripts.finance->cash_peak = 1200;
        scripts.finance->cash_peak_village = "OLD";
        scripts.finance->month = 3;
        scripts.finance->monthly_totals[3][4][0] = 7;
        scripts.professions[1] = {status, false, "职业夹具"};
        scripts.activities[1] = {status, false, "活动夹具"};
        scripts.user_flags = 8;
        scripts.medal_count = 2;
        state.village_points = 998;
    }
    state.facility_unlocks[1] = {status, false, 9, 98};
    ObjectCatalogRecord item;
    item.status = status;
    item.unlock_counter = 9;
    item.inventory = 998;
    item.free_purchases = 4;
    item.flags = 32;
    for (int kind = 0; kind < 4; ++kind)
        state.facility.finish.dungeon.catalog[{kind, 1}] = item;
    state.facility.finish.dungeon.item_rewards = 9;
    for (int id = 1; id <= 4; ++id) {
        RescueFacility shop;
        shop.detail = id == 1 ? 1 : id == 2 ? 4 : id == 3 ? 5 : 0;
        state.facility.finish.dungeon.world.facilities[id] = shop;
        state.facility.details[id] = {};
        state.facility_order.push_back(id);
    }
    state.facility_order.push_back(1); // 原g允许重复引用，提示仍按m.c7去重。
    return state;
}
void timing(const WorldScriptCatalog &scripts) {
    const std::vector<std::pair<int, int>> routes{{94, 3}, {94, 5}, {94, 6},  {94, 7},
                                                  {94, 8}, {95, 0}, {95, 1},  {95, 3},
                                                  {95, 4}, {95, 9}, {95, 10}, {95, 11}};
    for (const auto &[raw, mode] : routes)
        for (int counter : {0, 1, 39, 40, 41})
            for (bool press : {false, true}) {
                const auto input = fixture(mode, 0, raw);
                const auto result = prepare_world_gift_page(input, {2, counter, press}, scripts);
                const bool claim = press && counter >= 40;
                check(
                    result.candidate && result.candidate->claimed == claim &&
                        result.candidate->closed == claim &&
                        result.candidate->counter == (press && counter < 40 ? 40 : counter) &&
                        result.candidate->sound ==
                            (counter == 1 ? std::optional<int>{5} : std::nullopt),
                    "actual94/95 counter1 sound5; first early confirm only skips40; ready confirm "
                    "claims then close");
                if (!claim) {
                    check(result.candidate->state.facility.finish.dungeon.item_rewards == 9 &&
                              result.candidate->state.facility_unlocks.at(1).free_builds == 98 &&
                              result.candidate->state.facility.finish.dungeon.catalog.at({0, 1})
                                      .inventory == 998 &&
                              result.candidate->state.facility.scripts.event_calls.empty(),
                          "no idle/fastforward grant, no eager unlock/stat/script");
                    if (raw == 95) {
                        const auto &next = result.candidate->state;
                        check(next.facility.scripts.finance->cash == 1000 &&
                                  next.village_points == 998 &&
                                  next.facility.scripts.professions.at(1).status == 0 &&
                                  next.facility.scripts.activities.at(1).status == 0 &&
                                  next.facility.scripts.medal_count == 2 &&
                                  next.facility.scripts.user_flags == 8,
                              "95 idle and early confirmation preserve every supported effect "
                              "domain");
                    }
                } else
                    check(result.candidate->state.facility.scripts.pages.at(1).lifecycle == 4 &&
                              result.candidate->state.facility.scripts.pages.at(0).lifecycle == 1,
                          "successfulclaim uses actualclose lifecycle4 and resumes first lower "
                          "suspended page1");
            }
}
void script_rewards(const WorldScriptCatalog &scripts) {
    for (int mode : {0, 1, 3, 4, 9, 10, 11}) {
        auto input = fixture(mode, 0, 95);
        if (mode == 0)
            input.facility.scripts.pages.back().legacy_s = 300;
        if (mode == 1)
            input.facility.scripts.pages.back().legacy_s = 3;
        const auto result = prepare_world_gift_page(input, {2, 40, true}, scripts);
        check(result.candidate && result.candidate->claimed && result.candidate->closed,
              "each fixed-script95 route commits its real reward and closes");
        const auto &next = result.candidate->state;
        const auto &state = next.facility.scripts;
        check(state.notices.empty() && state.event_calls.empty() &&
                  result.candidate->scripts.empty() &&
                  next.facility.finish.dungeon.item_rewards == 9,
              "95 direct effects add no opcode0 notice34, opcode29 notice21 or drop statistics");
        if (mode == 0)
            check(state.finance->cash == 1300 && state.finance->cash_peak == 1300 &&
                      state.finance->cash_peak_village == "FIXTURE" &&
                      state.finance->monthly_totals[3][4][0] == 307 &&
                      state.finance->monthly_totals[3][4][1] == 0,
                  "95 cash updates original other-income slot and unlocked cash peak once");
        if (mode == 1)
            check(next.village_points == 999 && state.finance->cash == 1000,
                  "95 points cap999 independently of cash");
        if (mode == 3)
            check(next.facility_unlocks.at(1).free_builds == 99 &&
                      next.facility_unlocks.at(1).status == 2 &&
                      next.facility_unlocks.at(1).pending_notice,
                  "95 building reward retains authentic94 free-build semantics");
        if (mode == 4 || mode == 11) {
            const auto &value = mode == 4 ? state.professions.at(1) : state.activities.at(1);
            check(value.status == 1 && value.pending_notice,
                  "95 unopened profession or activity gets p1 and a new notice");
        }
        if (mode == 9)
            check(state.user_flags == 40, "95 flag32 preserves other user flags");
        if (mode == 10)
            check(state.medal_count == 3, "95 medal is exactly one, independently of payloads");
        check(!prepare_world_gift_page(next, {2, 41, true}, scripts).candidate,
              "retired95 cannot repeat cash, points, stock, flags, medals or unlocks");
    }
    for (int mode : {4, 11})
        for (int status : {1, 2})
            for (bool pending : {false, true}) {
                auto input = fixture(mode, status, 95);
                auto &value = mode == 4 ? input.facility.scripts.professions.at(1)
                                        : input.facility.scripts.activities.at(1);
                value.pending_notice = pending;
                const auto result = prepare_world_gift_page(input, {2, 40, true}, scripts);
                check(result.candidate.has_value(), "already open95 definition is valid");
                const auto &out = mode == 4
                                      ? result.candidate->state.facility.scripts.professions.at(1)
                                      : result.candidate->state.facility.scripts.activities.at(1);
                check(out.status == 1 && out.pending_notice == pending,
                      "95 leaves existing notice unchanged; script31 unlock is not granted twice");
            }
    for (int flags : {0, 1})
        for (int amount : {0, 200, 300}) {
            auto input = fixture(0, 0, 95);
            input.facility.scripts.pages.back().legacy_s = amount;
            input.facility.scripts.finance->legacy_flags14 = flags;
            const auto result = prepare_world_gift_page(input, {2, 40, true}, scripts);
            check(result.candidate.has_value(), "nonnegative95 cash request is valid");
            const auto &finance = *result.candidate->state.facility.scripts.finance;
            const bool new_peak = flags == 0 && amount > 200;
            check(finance.cash == 1000 + amount && finance.cash_peak == (new_peak ? 1300 : 1200) &&
                      finance.cash_peak_village == (new_peak ? "FIXTURE" : "OLD"),
                  "cash peak changes only on strict improvement with legacy flags14 clear");
        }
    for (const int points : {0, 998, 999}) {
        auto input = fixture(1, 0, 95);
        input.village_points = points;
        input.facility.scripts.pages.back().legacy_s = 0;
        const auto result = prepare_world_gift_page(input, {2, 40, true}, scripts);
        check(result.candidate && result.candidate->state.village_points == points,
              "zero95 point request preserves both clamp endpoints");
    }
}
void script_reward_failures(const WorldScriptCatalog &scripts) {
    for (int mode : {0, 1, 3, 4, 9, 10, 11}) {
        auto input = fixture(mode, 0, 95);
        input.facility.scripts.pages.back().legacy_s = -1;
        check(prepare_world_gift_page(input, {2, 40, true}, scripts).error ==
                  WorldGiftPageError::invalid_owner,
              "95 negative source payload is explicitly rejected for every supported route");
    }
    for (int mode : {2, 5, 6, 7, 8, 12})
        check(prepare_world_gift_page(fixture(mode, 0, 95), {2, 40, true}, scripts).error ==
                  WorldGiftPageError::unsupported_page,
              "95 acceptance stays limited to fixed-script routes");
    for (int fault = 0; fault < 11; ++fault) {
        const int mode = fault < 4     ? 0
                         : fault < 6   ? 1
                         : fault == 6  ? 4
                         : fault == 7  ? 11
                         : fault == 10 ? 3
                                       : 10;
        auto input = fixture(mode, 0, 95);
        if (fault == 0)
            input.facility.scripts.finance.reset();
        if (fault == 1)
            input.facility.scripts.finance->month = 12;
        if (fault == 2)
            input.facility.scripts.finance->cash = std::numeric_limits<std::int64_t>::max();
        if (fault == 3)
            input.facility.scripts.finance->monthly_totals[3][4][0] =
                std::numeric_limits<int>::max();
        if (fault == 4)
            input.village_points = -1;
        if (fault == 5)
            input.facility.scripts.pages.back().legacy_s = std::numeric_limits<int>::max();
        if (fault == 6)
            input.facility.scripts.professions.clear();
        if (fault == 7)
            input.facility.scripts.activities.clear();
        if (fault == 8)
            input.facility.scripts.medal_count = std::numeric_limits<int>::max();
        if (fault == 9)
            input.facility.scripts.medal_count = -1;
        if (fault == 10)
            input.facility_unlocks.clear();
        const auto result = prepare_world_gift_page(input, {2, 40, true}, scripts);
        const bool overflow = fault == 2 || fault == 3 || fault == 5 || fault == 8;
        check(!result.candidate &&
                  result.error == (overflow ? WorldGiftPageError::overflow
                                            : WorldGiftPageError::invalid_owner) &&
                  input.facility.scripts.pages.back().lifecycle == 2 &&
                  input.facility.scripts.notices.empty() &&
                  (fault == 10 ? input.facility_unlocks.empty()
                               : input.facility_unlocks.at(1).free_builds == 98),
              ("95 invalid source or arithmetic rejects whole candidate, fault=" +
               std::to_string(fault))
                  .c_str());
        if (!overflow)
            for (const bool confirm : {false, true}) {
                const auto early = prepare_world_gift_page(input, {2, 1, confirm}, scripts);
                check(!early.candidate && early.error == WorldGiftPageError::invalid_owner &&
                          input.facility.scripts.pages.back().lifecycle == 2,
                      ("95 rejects incomplete payload before initial sound or fastforward, fault=" +
                       std::to_string(fault))
                          .c_str());
            }
    }
}
void definitions(const WorldScriptCatalog &scripts) {
    for (int status : {0, 1, 2})
        for (int mode : {3, 5, 6, 7, 8}) {
            auto input = fixture(mode, status);
            const auto result = prepare_world_gift_page(input, {2, 40, true}, scripts);
            check(
                result.candidate &&
                    result.candidate->state.facility.finish.dungeon.item_rewards == 9 &&
                    result.candidate->state.facility.scripts.notices.empty() &&
                    !world_script_seen(result.candidate->state.facility.scripts, 110) &&
                    !world_script_seen(result.candidate->state.facility.scripts, 151),
                "raw gift definition methods never add E or direct-drop2/10/11/110/151 consumers");
            const auto &next = result.candidate->state;
            if (mode == 3)
                check(next.facility_unlocks.at(1).free_builds == 99 &&
                          next.facility_unlocks.at(1).status == (status == 0 ? 2 : status) &&
                          next.facility_unlocks.at(1).pending_notice == (status == 0) &&
                          next.facility_unlocks.at(1).unlock_counter == (status == 0 ? 0 : 9),
                      "buildinggift alwaysH+1cap99; oldp0only br.b setsr/p2/q0");
            else if (mode == 8) {
                const auto &item = next.facility.finish.dungeon.catalog.at({0, 1});
                check(item.inventory == 999 && item.status == (status == 0 ? 1 : status) &&
                          item.newly_unlocked == (status == 0) &&
                          item.unlock_counter == (status == 0 ? 0 : 9),
                      "itemgift onlyz+1cap999, oldp0 b resetsq/r/p1; oldp2 stays2");
            } else {
                const auto &item = next.facility.finish.dungeon.catalog.at({mode - 4, 1});
                check(
                    item.status == 1 && item.newly_unlocked == (status == 0) &&
                        item.free_purchases == (status == 0 ? 1 : 4) && item.unlock_counter == 9,
                    "equipmentgift alwaysp1, oldp0only free1/rtrue, preservesq and nonzero stock");
                for (int id = 1; id <= 4; ++id) {
                    const int expected = mode == 5 ? 1 : mode == 6 ? 2 : 3;
                    check(next.facility.details.at(id).notices.size() ==
                              (status == 0 && id == expected ? 1u : 0u),
                          "actualo.g matches instance detail1/4/5, not kind/category; duplicate "
                          "refs dedup7");
                }
                check(world_script_seen(next.facility.scripts, 216) == (mode == 5 && status == 0) &&
                          next.facility.scripts.continuations.size() ==
                              (mode == 5 && status == 0 ? 1u : 0u),
                      "newflag32 weapon triggers actual216 wait300 only, armor/accessory do not");
            }
        }
    auto input = fixture(5);
    input.facility.details.at(1).notices = {{7, 19}};
    const auto existing = prepare_world_gift_page(input, {2, 40, true}, scripts);
    check(existing.candidate && existing.candidate->state.facility.details.at(1).notices ==
                                    std::vector<std::array<int, 2>>{{7, 19}},
          "alreadyqueued shop7 keeps age19, no duplicate or timer reset");
    input = fixture(3);
    input.facility_unlocks.at(1).free_builds = 99;
    check(prepare_world_gift_page(input, {2, 40, true}, scripts)
                  .candidate->state.facility_unlocks.at(1)
                  .free_builds == 99,
          "repeatbuildingfree cap99");
    input = fixture(8);
    input.facility.finish.dungeon.catalog.at({0, 1}).inventory = 999;
    check(prepare_world_gift_page(input, {2, 40, true}, scripts)
                  .candidate->state.facility.finish.dungeon.catalog.at({0, 1})
                  .inventory == 999,
          "repeatitem inventory cap999, no extra money");
}
void failures(const WorldScriptCatalog &scripts) {
    auto input = fixture(5);
    input.facility_order.push_back(99);
    const auto late = prepare_world_gift_page(input, {2, 40, true}, scripts);
    check(!late.candidate && late.error == WorldGiftPageError::invalid_owner &&
              input.facility.scripts.event_calls.empty() &&
              input.facility.finish.dungeon.catalog.at({1, 1}).status == 0 &&
              input.facility.details.at(1).notices.empty(),
          "late staleg afteractual216 rolls back stock/script/shop notices/close");
    input = fixture(8);
    input.facility.finish.dungeon.catalog.erase({0, 1});
    check(prepare_world_gift_page(input, {2, 1, true}, scripts).candidate.has_value() &&
              !prepare_world_gift_page(input, {2, 40, true}, scripts).candidate,
          "skipforward does not read unusedcatalog, actualclaim missingdefinition fails");
    input = fixture(3);
    auto claimed = prepare_world_gift_page(input, {2, 40, true}, scripts);
    check(claimed.candidate &&
              !prepare_world_gift_page(claimed.candidate->state, {2, 41, true}, scripts).candidate,
          "closedpage not dispatched again, no permissive double gift API");
    input = fixture(4);
    check(prepare_world_gift_page(input, {2, 40, true}, scripts).error ==
              WorldGiftPageError::unsupported_page,
          "professionpage route remains outside actual gift subset, cannot silently success");
}
} // namespace
int main() {
    try {
        auto scripts = catalog();
        timing(scripts);
        definitions(scripts);
        failures(scripts);
        script_rewards(scripts);
        script_reward_failures(scripts);
        std::cout << "gift page checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
