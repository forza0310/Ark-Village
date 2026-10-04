#include "dungeon_village_reference/world_gift_page.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace dungeon_village_reference;
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
    const auto root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() /
                      "data/scripts/original";
    const auto parsed =
        parse_world_script_catalog(read(root / "events.txt"), read(root / "talk.txt"),
                                   read(root / "news.txt"), read(root / "evtmsgs.txt"));
    check(parsed.catalog.has_value(), "actual source scripts parse");
    return *parsed.catalog;
}
// 页/实例/状态为明确夹具；规则直接验证原br/bt/bz/bA/by调用，不用掉落grant代替。
WorldGiftPageState fixture(int mode, int status = 0) {
    WorldGiftPageState state;
    state.facility.scripts.village_name = "FIXTURE";
    WorldScriptPage scene;
    scene.id = 1;
    scene.lifecycle = 3;
    WorldScriptPage gift;
    gift.id = 2;
    gift.kind = WorldScriptPageKind::raw_page;
    gift.legacy_page = 94;
    gift.legacy_r = mode;
    gift.legacy_s = 1;
    state.facility.scripts.pages = {scene, gift};
    state.facility.scripts.next_page_id = 3;
    state.facility.scripts.executing_page = 2;
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
    for (int mode : {3, 5, 6, 7, 8})
        for (int counter : {0, 1, 39, 40, 41})
            for (bool press : {false, true}) {
                const auto input = fixture(mode);
                const auto result = prepare_world_gift_page(input, {2, counter, press}, scripts);
                const bool claim = press && counter >= 40;
                check(result.candidate && result.candidate->claimed == claim &&
                          result.candidate->closed == claim &&
                          result.candidate->counter == (press && counter < 40 ? 40 : counter) &&
                          result.candidate->sound ==
                              (counter == 1 ? std::optional<int>{5} : std::nullopt),
                      "actual94 counter1 sound5; first early confirm only skips40; ready confirm "
                      "claims then close");
                if (!claim)
                    check(result.candidate->state.facility.finish.dungeon.item_rewards == 9 &&
                              result.candidate->state.facility_unlocks.at(1).free_builds == 98 &&
                              result.candidate->state.facility.finish.dungeon.catalog.at({0, 1})
                                      .inventory == 998 &&
                              result.candidate->state.facility.scripts.event_calls.empty(),
                          "no idle/fastforward grant, no eager unlock/stat/script");
                else
                    check(result.candidate->state.facility.scripts.pages.at(1).lifecycle == 4 &&
                              result.candidate->state.facility.scripts.pages.at(0).lifecycle == 1,
                          "successfulclaim uses actualclose lifecycle4 and resumes first lower "
                          "suspended page1");
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
        std::cout << "gift page checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
