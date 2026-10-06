#include "dungeon_village_reference/world_scripts.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
std::string read(const std::filesystem::path &file) {
    std::ifstream stream(file, std::ios::binary);
    if (!stream)
        throw std::runtime_error("missing published script source");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
WorldScriptCatalog source_catalog() {
    const auto root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() /
                      "data/scripts/original";
    const auto result =
        parse_world_script_catalog(read(root / "events.txt"), read(root / "talk.txt"),
                                   read(root / "news.txt"), read(root / "evtmsgs.txt"));
    check(result.catalog && result.catalog->events.size() == 200,
          "published sparse events parse strictly, not dense row-index identities");
    return *result.catalog;
}
WorldScriptState fixture();
WorldScriptState complete_fixture() {
    auto state = fixture();
    state.finance = WorldScriptFinance{};
    state.finance->localized_gold_template = "<0>G"; // 显式本地化夹具，不当作原APK h.b("G")结果。
    state.human_catalog_complete = true;
    state.facility_catalog_complete = true;
    for (int id = 0; id < 100; ++id) {
        state.humans.emplace(id, WorldScriptUnlockDefinition{0, false, "fixture", id % 10, 70});
        state.activities.emplace(id, WorldScriptUnlockDefinition{0, false, "fixture", -1, 0});
        state.professions.emplace(id, WorldScriptUnlockDefinition{0, false, "fixture", -1, 0});
    }
    state.humans.at(1).status = 1;
    state.human_order = {5};
    return state;
}
WorldScriptState fixture() {
    WorldScriptState state;
    state.village_name = "TEST_VILLAGE"; // 名字为夹具；不宣称这是原新局村名。
    state.context = 7;
    WorldScriptPage scene;
    scene.id = 1;
    state.pages.push_back(scene);
    state.next_page_id = 2;
    state.executing_page = 1;
    return state;
}
WorldScriptState invoke(const WorldScriptCatalog &catalog, const WorldScriptState &state, int id) {
    const auto result = prepare_world_script(catalog, state, {id, {}, {}});
    check(result.candidate.has_value(), "real fixed event executes to its wait or end");
    return result.candidate->state;
}
void source_and_entry(const WorldScriptCatalog &catalog) {
    check(catalog.events.at(90).commands == WorldScriptProgram{{2, 70}, {13, 0}} &&
              catalog.events.at(92).commands == WorldScriptProgram{{6, 20}, {2, 72}} &&
              catalog.events.at(126).commands == WorldScriptProgram{{6, 10}, {22, 0}} &&
              catalog.events.at(201).commands == WorldScriptProgram{{6, 300}, {4, 1}},
          "90/92/126/201 programs equal raw original, 201 is ID not table row");
    for (int id : {90, 92, 126, 201, 116, 151, 216}) {
        const auto &definition = catalog.events.at(id);
        check(definition.id == id && definition.conditions == WorldScriptProgram{{0, 0}},
              "source legacy metadata and automatic gate remain unchanged");
        auto state = fixture();
        state.selected_actor = 5;
        state.selected_monster = 7;
        state.selected_facility = 6;
        for (int call = 1; call <= 3; ++call) {
            state = invoke(catalog, state, id);
            check(state.event_calls.at(id) == call && world_script_seen(state, id),
                  "entry increments each call, no implicit once guard");
        }
        if (id == 90) {
            check(!state.selected_actor && !state.selected_facility && !state.selected_monster &&
                      state.pages.size() == 7,
                  "90 clears all actual selection roots and inserts dialogue plus raw56");
            check(state.pages[1].legacy_page == 56 && state.pages[2].source_record == 70 &&
                      state.pages[2].paragraphs.size() == 1,
                  "executing scene anchor reverses adjacent insertions, source talk70 payload");
        }
    }
}
void completion_timing(const WorldScriptCatalog &catalog) {
    for (int amount : {-30, 0, 1, 100, std::numeric_limits<int>::max()}) {
        auto state = invoke(catalog, fixture(), 126);
        check(state.pending_completion == 0 && state.popularity_queue.empty() &&
                  state.continuations.size() == 1 && state.continuations[0].remaining_updates == 10,
              "126 queues wait before caller records completion, no eager settlement");
        state.pending_completion = amount;
        state.context = 55;
        for (int tick = 1; tick <= 10; ++tick) {
            const auto stopped = prepare_world_script_continuations(catalog, state, false);
            check(stopped.candidate &&
                      stopped.candidate->state.continuations[0].remaining_updates == 11 - tick,
                  "modal admission false does not decrement continuation or settle");
            const auto advanced = prepare_world_script_continuations(catalog, state, true);
            check(advanced.candidate.has_value(), "admitted logical continuation step");
            state = advanced.candidate->state;
            check(state.event_calls.at(126) == 1, "resume does not re-register event entry");
            if (tick < 10)
                check(state.pending_completion == amount && state.popularity_queue.empty() &&
                          state.context == 55,
                      "wait keeps accumulation and outer P9 until actual resume");
        }
        check(state.pending_completion == 0 && state.continuations.empty() && state.context == 7 &&
                  state.popularity_queue == std::vector<std::array<int, 3>>{{10, amount, 1}} &&
                  state.pages.size() == 1,
              "10th update restores P9 and front queues CURRENT amount in slot8, no page");
    }
    auto state = fixture();
    state.popularity_queue = {{6, 3, 0}};
    state = invoke(catalog, state, 126);
    state = invoke(catalog, state, 126);
    state.pending_completion = 50;
    for (int tick = 0; tick < 10; ++tick)
        state = prepare_world_script_continuations(catalog, state, true).candidate->state;
    check(state.continuations.size() == 1 && state.continuations[0].remaining_updates == 1 &&
              state.popularity_queue == std::vector<std::array<int, 3>>{{10, 50, 1}, {6, 3, 0}},
          "MainScene first126 resume skips next expired entry until next admitted round");
    state = prepare_world_script_continuations(catalog, state, true).candidate->state;
    check(state.popularity_queue ==
                  std::vector<std::array<int, 3>>{{10, 0, 1}, {10, 50, 1}, {6, 3, 0}} &&
              state.event_calls.at(126) == 2,
          "successive admitted repeat resumes retain source front zero record, no coalescing");
}
void presentation_and_close(const WorldScriptCatalog &catalog) {
    for (int id : {92, 201, 216}) {
        auto state = fixture();
        state.pending_completion = 17;
        state = invoke(catalog, state, id);
        const int delay = id == 92 ? 20 : 300;
        state.village_name = "RENAMED";
        for (int tick = 1; tick <= delay; ++tick) {
            const auto result = prepare_world_script_continuations(catalog, state, true);
            check(result.candidate.has_value(), "real fixed presentation wait advances");
            state = result.candidate->state;
            check(state.pages.size() == (tick < delay ? 1u
                                         : id == 92   ? 2u
                                                      : 3u),
                  "original exact wait controls insertion, not render-frame time or close");
        }
        check(state.pending_completion == 17 && state.popularity_queue.empty(),
              "92 and 201/216 only present, never settle f215e");
        if (id == 92)
            check(state.pages.back().source_record == 72 &&
                      state.pages.back().paragraphs.size() == 3 &&
                      state.pages.back().speaker_kind == 0 &&
                      state.pages.back().speaker_definition == -1,
                  "92 delivers full talk72 paragraphs and original speaker fields");
        else
            check(state.pages[1].kind == WorldScriptPageKind::newspaper &&
                      state.pages[1].source_record == (id == 201 ? 1 : 16) &&
                      state.pages[1].paragraphs[0].find("TEST_VILLAGE") != std::string::npos &&
                      state.pages[1].paragraphs[0].find("RENAMED") == std::string::npos &&
                      state.pages[2].kind == WorldScriptPageKind::simple_message &&
                      state.pages[2].legacy_page == 1,
                  "news uses saved default name, full body, scene anchor insertion semantics");
        for (const auto &page : state.pages) {
            const auto closed = prepare_world_script_close_page(state, page.id);
            check(closed.candidate && closed.candidate->state.pending_completion == 17 &&
                      closed.candidate->state.popularity_queue.empty() &&
                      closed.candidate->state.continuations.empty() &&
                      closed.candidate->state.pages.size() == state.pages.size(),
                  "closing marks lifecycle4, no immediate erase, rewards, AI or continuation");
        }
    }
    auto state = fixture();
    state.executing_page.reset();
    state = invoke(catalog, state, 201);
    state.continuations[0].remaining_updates = 1;
    state = prepare_world_script_continuations(catalog, state, true).candidate->state;
    check(state.pages[1].kind == WorldScriptPageKind::simple_message &&
              state.pages[2].kind == WorldScriptPageKind::newspaper,
          "outside framework update, l() falls back to changing live top and append order");
    state = fixture();
    state.page_mutations_locked = true;
    const auto locked = prepare_world_script(catalog, state, {90, {}, {}});
    check(locked.candidate && locked.candidate->state.pages.size() == 1 &&
              locked.candidate->state.event_calls.at(90) == 1 &&
              locked.candidate->inserted_pages.empty(),
          "framework lock suppresses insertion, not script or call count");
    state = invoke(catalog, fixture(), 151);
    state.pages[0].lifecycle = 3;
    state.pages[1].lifecycle = 2;
    const auto closed = prepare_world_script_close_page(state, state.pages[1].id);
    check(closed.candidate && closed.candidate->state.pages[1].lifecycle == 4 &&
              closed.candidate->state.pages[0].lifecycle == 1 &&
              closed.candidate->state.redraw_requested,
          "close reactivates first live suspended lower page to1, removal is framework later");
}
void composed_continuations(WorldScriptCatalog catalog) {
    // 以下新增脚本只用于检验机制，不替代固定APK输入。
    catalog.events[700] = {700, "fixture", 0, {}, {{6, 1}, {6, 1}, {22, 0}}};
    auto state = fixture();
    state.pending_completion = 8;
    state = invoke(catalog, state, 700);
    const auto first = prepare_world_script_continuations(catalog, state, true);
    check(first.candidate && first.candidate->entered_program &&
              first.candidate->executed.size() == 1 &&
              first.candidate->state.continuations.size() == 1 &&
              first.candidate->state.continuations[0].remaining_updates == 1 &&
              first.candidate->state.pending_completion == 8,
          "MainScene L247 resumes first expiry and L159 leaves appended wait untouched");
    const auto advanced = prepare_world_script_continuations(catalog, state, true, 100000, false);
    check(advanced.candidate && advanced.candidate->executed.size() == 2 &&
              advanced.candidate->state.continuations.empty() &&
              advanced.candidate->state.popularity_queue[0] == std::array<int, 3>{10, 8, 1},
          "explicit exhaustive fixture decrements appended wait; not a MainScene round");
    catalog.events[701] = {701, "fixture", 0, {}, {{1, 126}, {6, 1}, {22, 0}}};
    state = fixture();
    state.pending_completion = 9;
    state = invoke(catalog, state, 701);
    check(state.event_calls.at(701) == 1 && state.event_calls.at(126) == 1 &&
              state.continuations.size() == 2 && state.continuations[0].event == 126,
          "nested event wait only returns from child, parent continues and keeps order");
    const auto resumed = prepare_world_script_continuations(catalog, state, true);
    check(resumed.candidate && resumed.candidate->state.continuations.size() == 1 &&
              resumed.candidate->state.continuations[0].remaining_updates == 9 &&
              resumed.candidate->state.pending_completion == 0,
          "non-expired entry advances once, expired next entry removed without skipping");
    catalog.events[702] = {702, "fixture", 0, {}, {{2, std::numeric_limits<int>::min()}}};
    const auto substituted =
        prepare_world_script(catalog, fixture(), {702, {}, std::vector<int>{72}});
    check(substituted.candidate && substituted.candidate->inserted_pages[0].source_record == 72,
          "non-wait opcode uses original INT_MIN parameter-index protocol");
    catalog.events[703] = {703, "fixture", 0, {}, {{6, std::numeric_limits<int>::min() + 1, 99}}};
    const auto raw = prepare_world_script(catalog, fixture(), {703, {}, std::vector<int>{12}});
    check(raw.candidate &&
              raw.candidate->state.continuations[0].remaining_updates ==
                  std::numeric_limits<int>::min() + 1 &&
              raw.candidate->state.continuations[0].legacy_tag == 99,
          "wait is handled before reference substitution and preserves raw optional tag");
}
void failures(WorldScriptCatalog catalog) {
    auto state = fixture();
    state.pending_completion = 21;
    catalog.events[700] = {700, "fixture", 0, {}, {{22, 0}, {40, 0}}};
    const auto failed = prepare_world_script(catalog, state, {700, {}, {}});
    check(failed.error == WorldScriptError::unsupported_opcode && !failed.candidate &&
              state.event_calls.empty() && state.pending_completion == 21 &&
              state.popularity_queue.empty(),
          "late unsupported opcode rolls back entry count and earlier popularity commit");
    catalog.events[701] = {701, "fixture", 0, {}, {{6, 1}, {40, 0}}};
    state = invoke(catalog, state, 126);
    state = invoke(catalog, state, 701);
    state.continuations[0].remaining_updates = 1;
    const auto first_only = prepare_world_script_continuations(catalog, state, true);
    check(first_only.candidate && first_only.candidate->state.continuations.size() == 1 &&
              first_only.candidate->state.continuations[0].remaining_updates == 1,
          "MainScene skips second expiry after first126 even when later program is invalid");
    const auto late = prepare_world_script_continuations(catalog, state, true, 100000, false);
    check(!late.candidate && late.error == WorldScriptError::unsupported_opcode &&
              state.pending_completion == 21 && state.continuations.size() == 2,
          "explicit exhaustive fixture late second failure rolls back first22, not main routing");
    state = fixture();
    state.event_calls[90] = std::numeric_limits<int>::max();
    check(prepare_world_script(catalog, state, {90, {}, {}}).error ==
              WorldScriptError::numeric_overflow,
          "normal maintainable counters reject Java overflow instead of inventing saturation");
    catalog.events[702] = {702, "fixture", 0, {}, {{1, 702}}};
    check(prepare_world_script(catalog, fixture(), {702, {}, {}, 16}).error ==
              WorldScriptError::dispatch_limit,
          "recursive malformed source stops with no partial state");
    const std::string talk = "talk\t0\t-1\t-1\tbody";
    const std::string news = "0\ttitle\tbody";
    for (const std::string &events :
         {"-1\tname\t0\t\t", "1\tname\t0\t\t6,", "1\tname\t0\t\t6,2147483648",
          "1\tname\t0\t\t\n1\tdup\t0\t\t", "1\tbad"})
        check(!parse_world_script_catalog(events, talk, news).catalog,
              "strict source parser rejects malformed IDs, matrix tokens and bounds");
    check(!parse_world_script_catalog(std::string("\xc0\xaf"), talk, news).catalog &&
              !parse_world_script_catalog("1\tname\t0\t\t", "0", news).catalog &&
              !parse_world_script_catalog("1\tname\t0\t\t", talk, "0\tone").catalog,
          "strict UTF8 and presentation boundaries reject damaged source");
}
void read_only_preflight(const WorldScriptCatalog &catalog) {
    auto pending = invoke(catalog, fixture(), 126);
    pending.pending_completion = 21;
    check(validate_world_script_state(catalog, pending) == WorldScriptError::none &&
              pending.continuations[0].remaining_updates == 10 &&
              pending.pending_completion == 21 && pending.popularity_queue.empty() &&
              pending.event_calls.at(126) == 1 && pending.pages.size() == 1,
          "read-only preflight validates pending126 without advancing or producing outputs");
    // 独立预期覆盖调用点仍须拒绝的页引用／续体损坏，不能用执行结果生成预期。
    for (int damage = 0; damage < 6; ++damage) {
        auto state = pending;
        if (damage == 0)
            state.executing_page = 999;
        else if (damage == 1)
            state.pages.push_back(state.pages.front());
        else if (damage == 2)
            state.selected_actor = 0;
        else if (damage == 3)
            state.continuations[0].event = 999;
        else if (damage == 4)
            state.continuations[0].next_instruction = 999;
        else
            state.continuations[0].remaining_updates = std::numeric_limits<int>::min();
        const auto expected =
            damage == 3 ? WorldScriptError::missing_event : WorldScriptError::invalid_input;
        check(validate_world_script_state(catalog, state) == expected &&
                  prepare_world_script_continuations(catalog, state, false).error == expected,
              "read-only and nonadmitted preflight retain explicit invalid-state rejection");
    }
    auto bad_catalog = catalog;
    bad_catalog.events.at(126).id = 999;
    check(validate_world_script_state(bad_catalog, pending) == WorldScriptError::invalid_catalog,
          "read-only preflight rejects inconsistent event identity");
    pending.executing_page = 999;
    check(validate_world_script_state(bad_catalog, pending) == WorldScriptError::invalid_input,
          "invalid owner is reported before invalid catalogue, preserving original order");
}
void all_fixed_programs(const WorldScriptCatalog &catalog) {
    check(catalog.talks.size() == 182 && catalog.news.size() == 28 &&
              catalog.event_messages.size() == 10,
          "source presentation cardinalities preserve pipe identities including empty titles");
    for (const auto &entry : catalog.events) {
        auto state = complete_fixture();
        state = invoke(catalog, state, entry.first);
        for (int update = 0; !state.continuations.empty(); ++update) {
            check(update < 2000, "all fixed script waits eventually terminate in admitted fixture");
            const auto result = prepare_world_script_continuations(catalog, state, true);
            check(result.candidate.has_value(),
                  "all actual fixed opcode consumers accept full fixture");
            state = result.candidate->state;
        }
        check(state.event_calls.at(entry.first) == 1,
              "each of 200 sparse original entries has a real consumer and exact entry count");
    }
    auto state = complete_fixture();
    state = invoke(catalog, state, 123);
    check(state.pages[1].legacy_page == 98 && state.pages[2].source_record == 100,
          "tax event123 creates actual page98 after dialogue100, not only event request");
    state = complete_fixture();
    state = invoke(catalog, state, 161);
    state.finance->cash = -10;
    for (int update = 0; update < 5; ++update)
        state = prepare_world_script_continuations(catalog, state, true).candidate->state;
    check(state.finance->cash == 2990 && state.finance->monthly_totals[0][4][0] == 3000 &&
              state.finance->cash_peak == 2990 &&
              state.finance->cash_peak_village == "TEST_VILLAGE" && state.notices.size() == 1 &&
              state.notices[0].message == 34 && state.notices[0].counter == -1 &&
              state.notices[0].replacement == "3000G",
          "161 commits cash at5, category4 income, cash peak and explicit localized note34");
    state = complete_fixture();
    state = invoke(catalog, state, 128);
    state.continuations[0].remaining_updates = 1;
    const auto store = prepare_world_script_continuations(catalog, state, true);
    check(store.candidate && store.candidate->state.user_flags == 16 &&
              store.candidate->state.event_calls.at(120) == 1 &&
              store.candidate->state.event_calls.at(121) == 1 &&
              store.candidate->inserted_pages.size() == 3 &&
              store.candidate->inserted_pages[0].source_record == 97 &&
              store.candidate->inserted_pages[1].legacy_page == 83 &&
              store.candidate->inserted_pages[2].source_record == 98 &&
              store.candidate->state.notices[0].message == 23,
          "36 OR16 before120/page83/121/note23, nested scripts own entry records");
    state = complete_fixture();
    state = invoke(catalog, state, 73);
    state = invoke(catalog, state, 74);
    check(state.activities.at(7).status == 1 && state.activities.at(7).pending_notice &&
              state.activities.at(13).status == 1 && state.event_calls.at(61) == 1 &&
              state.continuations.size() == 1 && state.notices.size() == 1 &&
              state.notices[0].counter == -6,
          "17 opens activity not profession, first61 wait and global notice6 suppression");
    state = complete_fixture();
    state = invoke(catalog, state, 52);
    check(state.humans.at(1).satisfaction == 75 && state.humans.at(0).satisfaction == 70 &&
              state.notices[0].message == 9 && state.pages[1].legacy_page == 11 &&
              !state.pages[1].message_commands.empty(),
          "18 adjusts unlocked human satisfaction only,5 retains original event message commands");
    state = complete_fixture();
    state = invoke(catalog, state, 53);
    check(state.medal_count == 1 && state.notices[0].message == 21 &&
              state.notices[0].counter == -20,
          "29 increments one medal regardless command argument and stores original notice delay");
    WorldScriptFacilityDefinition facility;
    facility.category = 3;
    facility.economy.attributes.fill({100, 100});
    facility.economy.legacy_flags = 4096;
    state = complete_fixture();
    state.facilities[3] = facility;
    state.humans.at(0).extra = 2;
    state.humans.at(2).extra = 2;
    auto custom = catalog;
    custom.events[700] = {700, "fixture", 0, {}, {{21, 0, 1}}};
    custom.events[701] = {701, "fixture", 0, {}, {{21, 2, 1}}};
    state = invoke(custom, state, 700);
    check(state.humans.at(0).status == 1 && state.humans.at(0).pending_notice &&
              state.job_counts[2] == 0 && state.facilities.at(3).attributes[0] == 100,
          "21 i() calls category3 derivation BEFORE p1, new human excluded this time");
    state = invoke(custom, state, 701);
    check(state.job_counts[2] == 1 && state.facilities.at(3).attributes[0] == 120,
          "later unlock refresh sees previous human and still excludes current oldp0");
    state = complete_fixture();
    state.human_catalog_complete = false;
    const auto incomplete = prepare_world_script(custom, state, {700, {}, {}});
    check(!incomplete.candidate && state.humans.at(0).status == 0 && state.pages.size() == 1,
          "21 incomplete catalog rejects whole candidate including already prepared page59");
    custom.events[702] = {702, "fixture", 0, {}, {{31, 21}, {31, 22}}};
    state = complete_fixture();
    state.professions.at(22).extra = 3;
    state = invoke(custom, state, 702);
    check(state.professions.at(21).status == 1 && state.professions.at(22).pending_notice &&
              state.pages[1].legacy_page == 95 && state.pages[1].legacy_r == 4 &&
              state.pages[1].legacy_s == 22 && state.pages[1].legacy_t == 3,
          "31 opens actual shared profession and preserves icon extra0 fallback separately");
}
void camera_focus(const WorldScriptCatalog &catalog) {
    auto state = invoke(catalog, fixture(), 90);
    const auto id = state.pages[1].id;
    for (int distance = 0; distance <= 300; ++distance) {
        WorldScriptCameraFocusInput input;
        input.page = id;
        input.camera = {0, 0};
        input.previous_camera = {20, 30};
        input.previous_velocity = {8, 9};
        input.first_monster_cached_view = std::array<float, 2>{static_cast<float>(distance), 0};
        const auto result = prepare_world_script_camera_focus(state, input);
        check(result.candidate.has_value(),
              "real page56 camera range supports continuous distances");
        const auto &candidate = *result.candidate;
        const float step = distance < 10    ? 5.0F
                           : distance > 150 ? 26.0F
                                            : 5.0F + (distance - 10) * 21.0F / 140;
        if (distance < step)
            check(candidate.camera == *input.first_monster_cached_view &&
                      candidate.previous_camera == *input.first_monster_cached_view &&
                      candidate.velocity == input.previous_velocity &&
                      candidate.state.pages[1].lifecycle == 4,
                  "strict less distance snaps both cameras, preserves oldbi.w, closes");
        else
            check(std::abs(candidate.camera[0] - step) < 0.00001F &&
                      std::abs(candidate.previous_camera[0] - (20 + step)) < 0.00001F &&
                      candidate.previous_camera[1] == 30 && candidate.velocity[1] == 0 &&
                      candidate.state.pages[1].lifecycle == state.pages[1].lifecycle,
                  "distance equality still moves and does not close until next update");
        check(
            candidate.state.event_calls == state.event_calls &&
                candidate.state.popularity_queue == state.popularity_queue &&
                candidate.state.continuations.empty() && candidate.redraw,
            "camera page never calls event again, advances AI, queues rewards or consumes random");
    }
    const auto no_monster =
        prepare_world_script_camera_focus(state, {id, {1, 2}, {3, 4}, {5, 6}, {}});
    check(no_monster.candidate && no_monster.candidate->state.pages[1].lifecycle == 4 &&
              no_monster.candidate->camera == std::array<float, 2>{1, 2} &&
              no_monster.candidate->previous_camera == std::array<float, 2>{3, 4},
          "empty livebm closes focus page without fabricating target or changing camera");
    check(!prepare_world_script_camera_focus(
               state, {id,
                       {0, 0},
                       {0, 0},
                       {0, 0},
                       std::array<float, 2>{std::numeric_limits<float>::infinity(), 0}})
               .candidate,
          "non-finite adapter input rejects camera candidate without partial lifecycle mutation");
}
void automatic_events(const WorldScriptCatalog &catalog) {
    struct Gate {
        int event;
        int year;
        int month;
        int subperiod;
    };
    for (const auto &gate : std::vector<Gate>{{35, 0, 5, 0},
                                              {200, 0, 4, 1},
                                              {204, 2, 10, 2},
                                              {225, 9, 3, 0},
                                              {226, 19, 3, 0},
                                              {227, 49, 3, 0}})
        for (int year_delta : {-1, 0, 1})
            for (int month_delta : {-1, 0, 1})
                for (int subperiod_delta : {-1, 0, 1}) {
                    auto state = fixture();
                    for (const auto &event : catalog.events)
                        state.event_calls[event.first] = 1;
                    state.event_calls[gate.event] = 0;
                    WorldScriptAutomaticInput input{gate.year + year_delta,
                                                    gate.month + month_delta,
                                                    gate.subperiod + subperiod_delta, true, 100000};
                    const auto result = prepare_world_script_automatic(catalog, state, input);
                    const bool expected = year_delta >= 0 && month_delta >= 0 &&
                                          (gate.event == 35 || subperiod_delta >= 0);
                    check(result.candidate &&
                              world_script_seen(result.candidate->state, gate.event) == expected &&
                              result.candidate->state.continuations.size() ==
                                  (expected && gate.event != 200 ? 1u : 0u),
                          "real automatic year/month/subperiod gates are inclusive lower limits of "
                          "raw "
                          "counters+1");
                }
    auto state = fixture();
    const WorldScriptAutomaticInput initial{0, 0, 0, true, 100000};
    const auto automatic = prepare_world_script_automatic(catalog, state, initial);
    check(automatic.candidate && automatic.candidate->state.event_calls.size() == 1 &&
              automatic.candidate->state.event_calls.at(7) == 1 &&
              automatic.candidate->state.continuations[0].remaining_updates == 3,
          "initial raw date automatically triggers only empty-condition7,0,0 stays false");
    const auto resumed =
        prepare_world_script_continuations(catalog, automatic.candidate->state, true);
    check(resumed.candidate && resumed.candidate->state.continuations[0].remaining_updates == 2,
          "next admitted scene round decrements automatic wait; trigger round takes L159");
    const auto repeated =
        prepare_world_script_automatic(catalog, resumed.candidate->state, initial);
    check(repeated.candidate && repeated.candidate->state.event_calls.at(7) == 1 &&
              repeated.candidate->state.continuations.size() == 1,
          "automatic scanner checks aM and never duplicates already-started pending script");
    const auto denied =
        prepare_world_script_automatic(catalog, fixture(), {{}, {}, {}, false, 100000});
    check(denied.candidate && denied.candidate->state.event_calls.empty(),
          "nonadmitted main scene does not demand unused date fields or run automatic events");
    const auto lazy_date =
        prepare_world_script_automatic(catalog, fixture(), {{}, {}, {}, true, 100000});
    check(lazy_date.candidate && lazy_date.candidate->state.event_calls.at(7) == 1,
          "MainScene first empty-condition7 returns before later missing date is requested");
    check(!prepare_world_script_automatic(catalog, fixture(),
                                          {{}, {}, {}, true, 100000, false}).candidate,
          "explicit exhaustive fixture missing late date rolls back earlier7 wait");
}
} // namespace
int main() {
    try {
        const auto catalog = source_catalog();
        source_and_entry(catalog);
        completion_timing(catalog);
        presentation_and_close(catalog);
        composed_continuations(catalog);
        failures(catalog);
        read_only_preflight(catalog);
        all_fixed_programs(catalog);
        camera_focus(catalog);
        automatic_events(catalog);
        std::cout << "world script checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
