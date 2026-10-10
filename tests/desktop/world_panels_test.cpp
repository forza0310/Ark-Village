// Layout contracts keep active text and confirmation inside the viewport at every supported size.
#include "../../src/desktop/ui/common/world_panels.hpp"
#include "../../src/desktop/ui/common/world_reports.hpp"
#include "../support/checks.hpp"
#include "../support/world_fixture.hpp"
#include <cmath>
#include <iostream>

namespace {
bool contains(Rectangle outer, Rectangle inner) {
    return inner.width > 0 && inner.height > 0 && inner.x >= outer.x && inner.y >= outer.y &&
           inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}
} // namespace
namespace ark::test {
void world_panels() {
    Checks check{"world_panels"};
    using namespace ark::desktop;
    using namespace ark::simulation::rules;
    check(ui::world_hud_buttons_visible(nullptr), "Main scene retains mouse pause/menu controls");
    for (const int raw : {9, 21, 35, 36, 37, 38, 74, 75, 60}) {
        WorldScriptPage page;
        page.kind = WorldScriptPageKind::raw_page;
        page.legacy_page = raw;
        check(
            ui::world_hud_buttons_visible(&page) == (raw == 75 || raw == 60),
            "Source information and building footers hide both HUD drawing and pointer admission");
    }
    struct DateCase {
        int year, month, week, units;
        const char *text;
        float progress;
    };
    for (const auto &test :
         {DateCase{0, 3, 0, 0, "1年4月1周", 0}, DateCase{0, 3, 2, 5400, "1年4月3周", .5F},
          DateCase{0, 11, 3, 10773, "1年12月4周", .9975F},
          DateCase{1, 0, 0, 27, "2年1月1周", .0025F}}) {
        const WorldCalendarState calendar{test.year, test.month, test.week, test.units, 10799, 77};
        const auto date = ui::world_date_view(calendar);
        check(date.text == test.text && std::abs(date.week_progress - test.progress) < .00001F,
              "HUD shows one-based year/month/week and progress within this week, not month ticks");
    }
    for (const auto size :
         {Extent{240, 256}, Extent{360, 240 + 80}, Extent{540, 360}, Extent{1080, 720}}) {
        for (const auto kind :
             {WorldScriptPageKind::dialogue, WorldScriptPageKind::newspaper,
              WorldScriptPageKind::simple_message, WorldScriptPageKind::raw_page}) {
            WorldScriptPage page;
            page.kind = kind;
            page.legacy_page = kind == WorldScriptPageKind::dialogue ? 0 : 15;
            const auto layout = ui::world_page_layout(page, size);
            check(
                contains({0, 24, static_cast<float>(size.width), size.height - 53.F}, layout.panel),
                "Modal must clear the top HUD and lower playback buttons");
            check(contains(layout.panel, layout.body) && contains(layout.panel, layout.confirm),
                  "Body and confirmation must remain inside the modal");
            check(layout.body.height >= 34 && layout.body.width >= 100,
                  "Smallest viewport must retain readable scrollable text");
            check(layout.body.y + layout.body.height + 6 < layout.confirm.y,
                  "White text frame must not cover the confirmation button");
        }
    }
    for (const int raw : {99, 100}) {
        WorldScriptPage task;
        task.kind = WorldScriptPageKind::raw_page;
        task.legacy_page = raw;
        const auto layout = ui::world_page_layout(task, {240, 256});
        check(layout.body.y - layout.panel.y >= 95 && layout.body.height >= 34 &&
                  contains(layout.panel, layout.body),
              "Task monster display reserves its own area above readable source paragraphs");
    }
    WorldScriptPage page;
    page.kind = WorldScriptPageKind::dialogue;
    page.legacy_page = 0;
    const auto with_secretary = ui::world_page_layout(page, {540, 360});
    page.speaker_definition = 3;
    const auto other_speaker = ui::world_page_layout(page, {540, 360});
    check(other_speaker.body.width > with_secretary.body.width,
          "Unknown speakers must not reserve an invented secretary portrait");
    // UI owns only the footer transform and plain-text decoding. The source suite already
    // exhausts queue lifetime boundaries; these examples cover their product wiring.
    std::vector<WorldScriptNotice> notices(3);
    notices[0].message = 2;
    notices[0].counter = 8;
    notices[0].text = "<co=ff0000>先到</co><br>通知";
    notices[1].message = 3;
    notices[1].counter = 8;
    notices[1].text = "<po=cm>后到通知";
    notices[2].message = 4;
    notices[2].counter = 8;
    notices[2].text = "第三条仍在队列";
    const auto original = notices;
    for (const auto extent : {Extent{240, 256}, Extent{540, 360}}) {
        const auto lines = ui::world_notice_view(notices, extent);
        check(lines.size() == 2 && lines[0].index == 1 && lines[1].index == 0 &&
                  lines[0].text == "后到通知" && lines[1].text == "先到\n通知",
              "Notice wiring uses source reverse first-two order and decodes script tags");
        const float footer = extent.height - 42.F;
        check(lines[0].box.x == 0 && lines[1].box.x == 0 && lines[0].box.width == extent.width &&
                  lines[1].box.width == extent.width && lines[0].box.y == footer - 40 &&
                  lines[0].box.height == 19 && lines[1].box.y == footer - 21 &&
                  lines[1].box.height == 21 &&
                  lines[0].box.y + lines[0].box.height == lines[1].box.y &&
                  lines[1].box.y + lines[1].box.height == footer,
              "Notice heights and offsets translate to the desktop footer without rescaling source "
              "rows");
    }
    check(notices.size() == original.size() && notices[0].counter == original[0].counter &&
              notices[1].counter == original[1].counter &&
              notices[2].counter == original[2].counter && notices[0].text == original[0].text &&
              notices[1].text == original[1].text && notices[2].text == original[2].text,
          "Repeated notice projections neither advance counters nor mutate queued source text");
    notices[1].message = 1;
    const auto hidden = ui::world_notice_view(notices, {540, 360});
    check(hidden.size() == 1 && hidden[0].index == 0 && hidden[0].box.y == 297 &&
              hidden[0].box.height == 21,
          "Unshipped special notice1 stays hidden without promoting a third queue record");
    notices[0].counter = notices[1].counter = 0;
    check(ui::world_notice_view(notices, {540, 360}).empty(),
          "Unstarted first-two notices do not expose a later queued record");
    auto state = initial_world();
    const auto running_date = ui::world_date_view(state.scene.calendar);
    const auto initial_clock = state;
    state.scene.framework_paused = true;
    const auto paused_date = ui::world_date_view(state.scene.calendar);
    check(paused_date.text == running_date.text &&
              paused_date.week_progress == running_date.week_progress &&
              same_world_clock(state, initial_clock),
          "Paused HUD reads the same source date and progress without advancing calendar units");
    state.scene.framework_paused = initial_clock.scene.framework_paused;
    const int human_id = state.rules->humans.front().identity;
    const int monster_id = state.rules->monsters.front().identity;
    state.report_state = 1;
    state.report_snapshot = {0, 18, 3165, 4000, -835, human_id};
    state.report_portraits = {monster_id, monster_id, monster_id, monster_id, monster_id};
    for (const int defeats : {0, 1, 4, 5}) {
        state.report_snapshot[0] = defeats;
        const auto view = ui::world_month_view(state);
        const auto expected = defeats > 4 ? 3U : static_cast<unsigned>(defeats);
        check(view.defeats == defeats && view.monsters.size() == expected &&
                  view.ellipsis == (defeats > 4) && view.human_image.has_value() == (defeats == 0),
              "Monthly 0/1/4/5 defeats preserve count, zero-human and overflow branches");
        for (const auto &portrait : view.monsters)
            check(portrait.definition == monster_id,
                  "Repeated monster portraits must not be deduplicated");
    }
    state.report_portraits[1] = -1;
    check(ui::world_month_view(state).monsters.size() == 1,
          "Monthly portrait -1 ends the sequence even when later slots contain definitions");
    state.report_state = 2;
    state.report_new_records[4] = true;
    const auto income = ui::world_month_view(state);
    check(income.income == 3165 && income.expenses == 4000 && income.balance == -835 &&
              income.record && income.monsters.empty(),
          "Income view consumes the prepared snapshot and signed total, independent of live funds");
    WorldScriptNotice growth;
    growth.message = 0;
    growth.counter = 8;
    growth.text = "人物 Lv.<co=0064ff>3</co>";
    growth.human_definition = human_id;
    growth.attribute_changes = std::array<std::array<int, 2>, 4>{{{3, 8}, {1, 8}, {0, 7}, {2, 6}}};
    state.scripts.notices = {growth};
    const auto before = state;
    const auto growth_view = ui::world_notice_view(state, {240, 256});
    check(growth_view.size() == 1 && growth_view[0].portrait_image &&
              growth_view[0].attributes == std::vector<std::array<int, 2>>{{3, 8}, {1, 8}},
          "Growth keeps source ap order and only its first half, including equal increments");
    for (const auto input : {std::pair<float, std::size_t>{40, 2}, {110, 1}, {180, 0}, {400, 0}}) {
        const auto layout = ui::world_notice_layout(growth_view[0], input.first);
        check(layout.attributes.size() == input.second && layout.end + 16 <= 240,
              "Narrow growth notices omit whole attribute groups when name/level consumes width");
    }
    const auto &person = state.rules->humans.front();
    const auto profession =
        state.scene.world.world.ai.growth.at(human_id).definition.current_profession;
    check(growth_view[0].portrait_image == state.rules->jobs.at(profession).sprites.at(person.sex),
          "Growth portrait reads current profession and sex");
    check(same_world_clock(state, before) && state.village_points == before.village_points &&
              state.scene.world.world.ai.accounting.funds() ==
                  before.scene.world.world.ai.accounting.funds() &&
              state.scripts.notices[0].counter == 8 &&
              state.scripts.notices[0].attribute_changes == growth.attribute_changes,
          "Report and growth projections do not consume points, fees, counters or ap");
    // Entry/exit positions follow source report counters, never a drawing-frame timer.
    for (const auto &sample : std::vector<std::array<int, 3>>{
             {1, 0, -104}, {1, 3, -52}, {1, 6, 0}, {2, 64, 0}, {2, 67, -52}, {2, 70, -104}}) {
        state.report_state = sample[0];
        state.report_counter = sample[1];
        check(ui::world_month_view(state).offset_x == sample[2] &&
                  state.report_counter == sample[1],
              "Monthly overlay slides with the existing counter without changing world time");
    }
    for (const Extent extent : {Extent{240, 256}, Extent{540, 360}}) {
        const auto layout = ui::world_victory_layout(extent);
        const Rectangle usable{0, 24, static_cast<float>(extent.width), extent.height - 51.F};
        check(contains(usable, layout.panel) && contains(usable, layout.confirm) &&
                  layout.confirm.y >= layout.panel.y + layout.panel.height,
              "Victory confirmation stays clear of artwork and footer at narrow and wide sizes");
    }
    // raw95 renders the actual script payload. This view neither creates a reward nor uses
    // a screenshot amount; the frozen reward consumer owns its seven distinct transactions.
    auto reward_state = initial_world();
    auto reward_rules = *reward_state.rules;
    reward_rules.facilities.at(28).name = "设施奖";
    reward_rules.jobs.at(0).name = "职业奖";
    reward_rules.activities.at(4).name = "活动奖";
    reward_state.rules = &reward_rules;
    WorldScriptPage reward;
    reward.id = 719;
    reward.kind = WorldScriptPageKind::raw_page;
    reward.legacy_page = 95;
    reward.lifecycle = 1;
    reward_state.scripts.pages.push_back(reward);
    check(!ui::world_script_reward_view(reward_state, reward).initialized,
          "Reward view does not initialize a fresh source page or create its counter");
    struct RewardCase {
        int kind, value;
        const char *label;
    };
    for (const auto &sample :
         {RewardCase{0, 237, "237G"}, RewardCase{1, 17, "村子点 17"}, RewardCase{3, 28, "设施奖"},
          RewardCase{4, 0, "职业奖"}, RewardCase{9, 32, "配置更替"}, RewardCase{10, 1, "勋章"},
          RewardCase{11, 4, "活动奖"}}) {
        reward.legacy_r = sample.kind;
        reward.legacy_s = sample.value;
        reward.legacy_t =
            991; // Preserve the other bound payload; UI never reinterprets it as money.
        for (const int counter : {0, 1, 39, 40, 41}) {
            reward_state.page_counters[reward.id] = counter;
            const auto before = reward_state;
            const auto view = ui::world_script_reward_view(reward_state, reward);
            const auto again = ui::world_script_reward_view(reward_state, reward);
            check(view.initialized && view.label == sample.label && view.page == reward.id &&
                      view.kind == sample.kind && view.counter == counter &&
                      view.ready_to_claim == (counter >= 40) && again.label == view.label,
                  "Seven bound reward labels and original40 claim gate stay distinct under "
                  "repeated reads");
            check(reward_state.scene.world.world.ai.accounting.funds() ==
                          before.scene.world.world.ai.accounting.funds() &&
                      reward_state.village_points == before.village_points &&
                      reward_state.medal_count == before.medal_count &&
                      reward_state.sound_requests == before.sound_requests &&
                      reward_state.scene.random.draws() == before.scene.random.draws() &&
                      reward_state.scripts.pages.size() == before.scripts.pages.size(),
                  "Reward presentation does not pay, request sound, draw random or retire the "
                  "owning page");
        }
    }
    reward.lifecycle = 4;
    check(!ui::world_script_reward_view(reward_state, reward).initialized,
          "Retired reward cannot expose another claim button");
    std::cout << "PASS world page layout contracts\n";
}
} // namespace ark::test
