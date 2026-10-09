#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_application.hpp"
#include "dungeon_village_prototype/startup_world_facility_catalog.hpp"
#include "dungeon_village_prototype/startup_world_magic_pot.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_prototype/startup_world_presentation.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "dungeon_village_prototype/startup_world_village_activity.hpp"
#include "dungeon_village_tools/archive.hpp"
#include "startup_world_codec.hpp"
#include "startup_world_codec_checks.hpp"
#include "startup_world_restore_checks.hpp"
#include "startup_world_restore_validation.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace dungeon_village_prototype;
int run_startup_application_replay_checks(const std::filesystem::path &);
int run_startup_application_replay_cli(int, const char **);
int run_startup_application_actions_checks(const std::filesystem::path &);
int run_startup_application_natural_replay_cli(int, const char **);
int run_startup_application_natural_driver_checks(const std::filesystem::path &);
namespace {
using Bytes = std::vector<std::uint8_t>;
int checks{};
void check(bool value, const std::string &message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
Bytes read(const std::filesystem::path &p) {
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}
void write(const std::filesystem::path &p, const Bytes &b) {
    std::ofstream f(p, std::ios::binary);
    f.write(reinterpret_cast<const char *>(b.data()), static_cast<std::streamsize>(b.size()));
    check(static_cast<bool>(f), "fixture write");
}
void append32(Bytes &b, std::uint32_t n) {
    for (int i = 0; i < 4; ++i)
        b.push_back(static_cast<std::uint8_t>(n >> (8 * i)));
}
void append64(Bytes &b, std::uint64_t n) {
    for (int i = 0; i < 8; ++i)
        b.push_back(static_cast<std::uint8_t>(n >> (8 * i)));
}
void text(Bytes &b, const std::string &s) {
    append32(b, static_cast<std::uint32_t>(s.size()));
    b.insert(b.end(), s.begin(), s.end());
}
std::uint64_t number(const Bytes &b, std::size_t &at, int n) {
    std::uint64_t v{};
    for (int i = 0; i < n; ++i)
        v |= static_cast<std::uint64_t>(b.at(at++)) << (8 * i);
    return v;
}
void skip_text(const Bytes &b, std::size_t &at) {
    const auto n = number(b, at, 4);
    at += static_cast<std::size_t>(n);
    check(at <= b.size(), "fixture text bounds");
}
void resign(Bytes &b) {
    check(b.size() >= 64, "fixture digest bounds");
    b.resize(b.size() - 64);
    const auto h = dungeon_village_tools::sha256_hex(b);
    b.insert(b.end(), h.begin(), h.end());
}
// 独立按公开文件布局替换段并重算摘要，确保拒绝来自业务校验而非损坏校验和。
Bytes replace_state(const Bytes &original, const StartupWorldRuntimeState &state) {
    std::size_t at = 20;
    skip_text(original, at);
    skip_text(original, at);
    const auto count = number(original, at, 4);
    Bytes result(original.begin(), original.begin() + static_cast<std::ptrdiff_t>(at));
    for (std::uint64_t i = 0; i < count; ++i) {
        const auto start = at;
        const auto id = number(original, at, 4);
        const auto version = number(original, at, 4);
        const auto required = number(original, at, 4);
        const auto length = number(original, at, 8);
        skip_text(original, at);
        const auto payload = at;
        at += static_cast<std::size_t>(length);
        check(at <= original.size() - 64, "fixture section bounds");
        if (id != 2)
            result.insert(result.end(), original.begin() + static_cast<std::ptrdiff_t>(start),
                          original.begin() + static_cast<std::ptrdiff_t>(at));
        else {
            const auto bytes = persistence_detail::encode_state(state);
            append32(result, 2);
            append32(result, static_cast<std::uint32_t>(version));
            append32(result, static_cast<std::uint32_t>(required));
            append64(result, bytes.size());
            text(result, dungeon_village_tools::sha256_hex(bytes));
            result.insert(result.end(), bytes.begin(), bytes.end());
        }
        (void)payload;
    }
    const auto hash = dungeon_village_tools::sha256_hex(result);
    result.insert(result.end(), hash.begin(), hash.end());
    return result;
}
std::vector<StartupAudioRequest> advance(StartupWorldRuntimeSession &s) {
    const auto step = s.update();
    check(step.candidate.has_value(), "natural update rejected");
    const auto p = s.state().scripts.pages.back();
    if (p.kind != ref::WorldScriptPageKind::scene && p.legacy_page != 16 && p.legacy_page != 24 &&
        p.legacy_page != 56 && p.legacy_page != 57 && p.legacy_page != 97 && p.legacy_page != 98)
        check(s.acknowledge_page(p.id) == StartupWorldRuntimeError::none,
              "natural confirm rejected");
    return s.take_audio_requests();
}
// 共用严格文件夹具路径；结构输入经真实loader验证后才做保存往返。
StartupWorldRuntimeState capture_persistence_fixture(
    const StartupWorldRuntimeSession &baseline_session, const StartupWorldRuntimeState &state,
    const StartupWorldSaveMetadata &metadata, const std::filesystem::path &file,
    const std::filesystem::path &reencoded, const std::filesystem::path &broken,
    const char *scenario) {
        // 现有独立段替换只准备结构夹具，经真实读器校验后才得到私有Session；
        // 不增加测试安装接口，不把被测编码结果用作业务期望。
        const auto seed_file = save_startup_world_file(reencoded, baseline_session, metadata);
        check(seed_file.ok, std::string(scenario) + " baseline fixture: " + seed_file.error);
        write(broken, replace_state(read(reencoded), state));
        auto fixture = load_startup_world_file(broken, startup_world_rules(), metadata.purpose,
                                               metadata.controller_id);
        check(fixture.snapshot.has_value(), std::string(scenario) + " fixture validate: " + fixture.error);
        auto source = std::move(fixture.snapshot->session);
        check(startup_world_state_digest(source.state()) == startup_world_state_digest(state),
              std::string(scenario) + " independent state fixture preserved");
        const auto digest = startup_world_session_digest(source);
        const auto saved = save_startup_world_file(file, source, metadata);
        check(saved.ok, std::string(scenario) + " capture: " + saved.error);
        check(startup_world_session_digest(source) == digest,
              std::string(scenario) + " capture consumes no state/random");
        auto load = load_startup_world_file(file, startup_world_rules(), metadata.purpose,
                                            metadata.controller_id);
        check(load.snapshot.has_value(), std::string(scenario) + " restore: " + load.error);
        check(startup_world_session_digest(load.snapshot->session) == digest,
              std::string(scenario) + " complete state/session roundtrip");
        check(save_startup_world_file(reencoded, load.snapshot->session, load.snapshot->metadata).ok &&
                  read(reencoded) == read(file),
              std::string(scenario) + " canonical file bytes preserved");
        return load.snapshot->session.state();
}
// 最小页面结构夹具，不注入资金、人物、日期或解锁；目录全来自真实稳定世界。
void facility_catalog_replay(const StartupWorldRuntimeSession &baseline_session,
                             const std::filesystem::path &dir,
                             const StartupWorldSaveMetadata &metadata) {
    using Action = StartupFacilityCatalogAction;
    const auto &baseline = baseline_session.state();
    const auto file = dir / "facility-catalog.avrs";
    const auto reencoded = dir / "facility-catalog-roundtrip.avrs";
    const auto broken = dir / "facility-catalog-bad.avrs";
    const auto capture = [&](const StartupWorldRuntimeState &state, const char *scenario) {
        return capture_persistence_fixture(baseline_session, state, metadata, file, reencoded,
                                           broken, scenario);
    };
    const auto equal = [&](const auto &reference, const auto &restored, const char *scenario) {
        check(startup_world_state_digest(reference) == startup_world_state_digest(restored),
              std::string(scenario) + " full state/random/output same after input");
    };
    const auto command = [&](auto &state, std::uint64_t id, Action action, const char *scenario) {
        check(act_startup_world_facility_catalog_page(state, id, action) ==
                  StartupWorldRuntimeError::none,
              std::string(scenario) + " command accepted");
    };
    auto goods = baseline;
    ref::WorldScriptPage page;
    page.id = goods.scripts.next_page_id++;
    page.kind = ref::WorldScriptPageKind::raw_page;
    page.legacy_page = 79;
    page.lifecycle = 0;
    page.legacy_f = 1;
    goods.scripts.pages.push_back(page);
    check(initialize_startup_world_facility_catalog_pages(goods), "initialize goods79 fixture");
    goods.scripts.pages.back().lifecycle = 2;
    command(goods, page.id, Action::next_tab, "goods79 nondefault phase");
    auto restored = capture(goods, "goods79");
    for (auto *state : {&goods, &restored}) {
        command(*state, page.id, Action::inspect, "goods79 open information72");
        check(initialize_startup_world_facility_catalog_pages(*state), "initialize actual child72");
        state->scripts.pages.back().lifecycle = 2;
    }
    equal(goods, restored, "restored79 opens same72");
    const auto child = goods.scripts.pages.back().id;
    check(goods.facility_catalog_page_parents.at(child) == page.id &&
              goods.scripts.pages.back().legacy_page == 72,
          "actual information child binds live79");
    auto restored_child = capture(goods, "information72");
    for (auto *state : {&goods, &restored_child}) {
        command(*state, child, Action::next_tab, "information72 browse");
        command(*state, child, Action::cancel, "information72 close");
        command(*state, page.id, Action::confirm, "goods79 confirm and retire");
    }
    equal(goods, restored_child, "restored72 continues and returns to79");
    check(goods.scripts.pages.back().lifecycle == 4 && goods.facility_catalog_pages_initialized.count(page.id),
          "79/72 close marks retirement without pretending the next framework entry already ran");
    check(std::all_of(goods.catalog.begin(), goods.catalog.end(), [](const auto &v) {
              return v.first.first != 1 || !v.second.newly_unlocked;
          }), "79 clears full weapon category NEW after restore");
    check(goods.scene.random.draws() == baseline.scene.random.draws() &&
              goods.scene.world.world.ai.accounting.funds() ==
                  baseline.scene.world.world.ai.accounting.funds(),
          "79/72 information and close have no draw or payment");
    for (auto *state : {&goods, &restored_child}) {
        const auto retired = prepare_startup_world_runtime(*state);
        check(retired.candidate.has_value(), "79/72 actual framework retirement succeeds");
        *state = *retired.candidate;
    }
    equal(goods, restored_child, "restored72 retires at the same framework entry");
    check(goods.facility_catalog_pages_initialized.empty() &&
              goods.facility_catalog_page_data.empty() && goods.facility_catalog_page_lists.empty() &&
              goods.facility_catalog_page_parents.empty(),
          "79/72 next framework entry retires every catalogue payload");

    auto praise = baseline;
    const auto definition = std::find_if(praise.rules->facilities.begin(), praise.rules->facilities.end(),
                                         [](const auto &v) { return v.legacy_icon == 2; });
    check(definition != praise.rules->facilities.end(), "fixed table icon2 animation source");
    ref::WorldScriptPage animation;
    animation.id = praise.scripts.next_page_id++;
    animation.kind = ref::WorldScriptPageKind::raw_page;
    animation.legacy_page = 82;
    animation.lifecycle = 0;
    animation.legacy_f = 0;
    animation.facility_definition = definition->id;
    praise.scripts.pages.push_back(animation);
    check(initialize_startup_world_facility_catalog_pages(praise), "initialize animation82 fixture");
    praise.scripts.pages.back().lifecycle = 2;
    praise.page_counters.at(animation.id) = 23; // 稳定等待结构夹具，不冒充自然演出可达证据。
    auto restored_first = capture(praise, "animation82 first phase");
    for (auto *state : {&praise, &restored_first}) {
        command(*state, animation.id, Action::confirm, "animation82 fast forward first");
        command(*state, animation.id, Action::confirm, "animation82 enter second phase");
    }
    equal(praise, restored_first, "restored82 preserves phase transition");
    check(praise.page_phases.at(animation.id) == 1 && praise.page_counters.at(animation.id) == 0,
          "82 second phase resets counter before deferred reward");
    praise.page_counters.at(animation.id) = 17;
    auto restored_second = capture(praise, "animation82 second phase");
    const auto queue = baseline.scene.world.popularity_queue;
    for (auto *state : {&praise, &restored_second}) {
        command(*state, animation.id, Action::confirm, "animation82 fast forward second");
        command(*state, animation.id, Action::confirm, "animation82 finish");
    }
    equal(praise, restored_second, "restored82 completes with one deferred reward");
    check(praise.scene.world.popularity_queue.size() == queue.size() + 1 &&
              praise.scene.world.popularity_queue.front() == std::array<int, 3>{10, 20, 1} &&
              std::equal(queue.begin(), queue.end(), praise.scene.world.popularity_queue.begin() + 1),
          "82 queues exact10/20/1 once and preserves original pending order");
    check(praise.popularity == baseline.popularity &&
              praise.scene.random.draws() == baseline.scene.random.draws(),
          "82 retirement changes neither immediate popularity nor random");
    const auto retired_digest = startup_world_state_digest(praise);
    check(act_startup_world_facility_catalog_page(praise, animation.id, Action::confirm) ==
              StartupWorldRuntimeError::invalid_page &&
              startup_world_state_digest(praise) == retired_digest,
          "restored82 cannot queue reward twice after retirement");
    for (auto *state : {&praise, &restored_second}) {
        const auto retired = prepare_startup_world_runtime(*state);
        check(retired.candidate.has_value(), "82 actual framework retirement succeeds");
        *state = *retired.candidate;
    }
    equal(praise, restored_second, "restored82 preserves subsequent world consumption");
    check(praise.facility_catalog_pages_initialized.empty() &&
              praise.facility_catalog_page_data.empty() && praise.facility_catalog_page_lists.empty() &&
              praise.facility_catalog_page_parents.empty(), "82 next framework entry retires all page payloads");

    // 旧布局身份独立改header并重签整文件，拒绝必须来自布局身份而非损坏摘要。
    auto old_layout = read(file);
    std::size_t at = 20;
    skip_text(old_layout, at);
    const auto length = number(old_layout, at, 4);
    const std::string old_schema = "0500cff0cd937c6967836c6bf7c594ff43dd64c23f408e7ea9188e4fc3423403";
    check(length == old_schema.size() &&
              std::string(old_layout.begin() + static_cast<std::ptrdiff_t>(at),
                          old_layout.begin() + static_cast<std::ptrdiff_t>(at + length)) != old_schema,
          "new79/72/82 fields produce a distinct schema identity");
    std::copy(old_schema.begin(), old_schema.end(), old_layout.begin() + static_cast<std::ptrdiff_t>(at));
    resign(old_layout);
    write(broken, old_layout);
    const auto refused = load_startup_world_file(broken, startup_world_rules(), metadata.purpose,
                                                 metadata.controller_id);
    check(!refused.snapshot && refused.error.find("状态字段布局不兼容") != std::string::npos,
          "prior-layout save explicitly rejected without migration: " + refused.error);
    std::filesystem::remove(file);
    std::filesystem::remove(reencoded);
    std::filesystem::remove(broken);
}
void magic_pot_replay(const StartupWorldRuntimeSession &baseline_session,
                      const std::filesystem::path &dir,
                      const StartupWorldSaveMetadata &metadata) {
    using Action = StartupMagicPotAction;
    const auto &baseline = baseline_session.state();
    const auto file = dir / "magic-pot.avrs", reencoded = dir / "magic-pot-roundtrip.avrs",
               broken = dir / "magic-pot-bad.avrs";
    const auto capture = [&](const auto &state, const char *scenario) {
        return capture_persistence_fixture(baseline_session, state, metadata, file, reencoded,
                                           broken, scenario);
    };
    const auto same = [&](const auto &a, const auto &b, const char *scenario) {
        check(startup_world_state_digest(a) == startup_world_state_digest(b),
              std::string(scenario) + " full state/random/outputs match");
    };
    const auto command = [&](auto &state, std::uint64_t id, Action action,
                             const char *scenario, int selection = 0) {
        check(act_startup_world_magic_pot_page(state, id, action, selection) ==
                  StartupWorldRuntimeError::none,
              std::string(scenario) + " accepted");
    };
    const auto initialize = [&](auto &state, const char *scenario) {
        check(initialize_startup_world_magic_pot_pages(state), std::string(scenario) + " initialize");
        state.scripts.pages.back().lifecycle = 2;
    };
    auto ready = baseline;
    // 明确壶资格/库存组合夹具；不作为自然新局解锁、首次help或日期前缀证据。
    ready.scripts.user_flags |= 1U;
    ready.items.at(0).inventory = 2;
    ready.catalog.at({0, 0}) = ready.items.at(0);
    ref::WorldScriptPage menu;
    menu.id = ready.scripts.next_page_id++;
    menu.kind = ref::WorldScriptPageKind::raw_page;
    menu.legacy_page = 41;
    menu.lifecycle = 2;
    ready.scripts.pages.push_back(menu);
    ready.magic_pot_pages_initialized.insert(menu.id);
    ready.magic_pot_page_data[menu.id] = {0, 0, -1};
    ready.magic_pot_page_lists[menu.id] = {};
    ready.page_counters[menu.id] = 0;
    ready.page_phases[menu.id] = 0;
    auto original = ready;
    auto restored = capture(original, "magic41 menu");
    for (auto *state : {&original, &restored}) {
        command(*state, menu.id, Action::confirm, "magic41 open42");
        initialize(*state, "magic42 inventory");
    }
    same(original, restored, "restored41 opens exact42");
    const auto deposit = original.scripts.pages.back().id;
    auto restored_deposit = capture(original, "magic42 before inventory commit");
    const auto draws = original.scene.random.draws();
    for (auto *state : {&original, &restored_deposit}) {
        command(*state, deposit, Action::select, "magic42 choose fixed item0", 0);
        command(*state, deposit, Action::confirm, "magic42 actual deposit");
        initialize(*state, "magic44 deposit result");
    }
    same(original, restored_deposit, "restored42 deposits once with same comment");
    check(original.items.at(0).inventory == 1 && original.catalog.at({0, 0}).inventory == 1 &&
              original.legacy_n[1] == 1 && original.legacy_n[4] == 2 &&
              original.legacy_n[8] == 1 && original.scene.random.draws() == draws + 1 &&
              !original.magic_pot_comment.empty(),
          "fixed item0 deposit oracle: stock1, ice2, pending ice1, exactly one comment draw");
    const auto animation = original.scripts.pages.back().id;
    original.page_counters.at(animation) = 35; // 明确稳定等待计数组合。
    auto restored_animation = capture(original, "magic44 stable waiting result");
    const auto applied = original.legacy_n;
    const auto comment = original.magic_pot_comment;
    const auto post_draws = original.scene.random.draws();
    for (auto *state : {&original, &restored_animation}) {
        command(*state, animation, Action::confirm, "magic44 early confirm waits");
        command(*state, animation, Action::cancel, "magic44 cancel result");
    }
    same(original, restored_animation, "restored44 preserves committed deposit");
    check(original.items.at(0).inventory == 1 && original.legacy_n == applied &&
              original.magic_pot_comment == comment && original.scene.random.draws() == post_draws,
          "44 restore/cancel neither refunds nor repeats inventory/element/comment draw");
    const auto closing44 = std::find_if(original.scripts.pages.begin(), original.scripts.pages.end(),
                                      [&](const auto &p) { return p.id == animation; });
    check(closing44 != original.scripts.pages.end() && closing44->lifecycle == 4,
          "44 close marks finish mode before the real framework retires payloads");
    for (auto *state : {&original, &restored_animation}) {
        auto retired = prepare_startup_world_runtime(*state);
        check(retired.candidate.has_value(), "44 real framework retirement candidate");
        *state = std::move(*retired.candidate);
    }
    same(original, restored_animation, "restored44 real framework retires identical state");
    check(!original.magic_pot_pages_initialized.count(animation) &&
              !original.magic_pot_page_data.count(animation) &&
              !original.magic_pot_page_lists.count(animation) &&
              !original.magic_pot_page_parents.count(animation),
          "44 real framework finish retires every associated payload");

    auto catalogue = ready;
    catalogue.magic_pot_recipes.at(1).status = 1; // 明确已发现条件夹具，原配方费用未变。
    catalogue.legacy_n[3] = 100;
    catalogue.legacy_n[4] = 20;
    catalogue.legacy_n[5] = 30;
    catalogue.legacy_n[6] = 0;
    command(catalogue, menu.id, Action::select, "magic41 development selection", 1);
    command(catalogue, menu.id, Action::confirm, "magic41 open43");
    initialize(catalogue, "magic43 recipe list");
    const auto recipes = catalogue.scripts.pages.back().id;
    command(catalogue, recipes, Action::next_tab, "magic43 nondefault phase");
    auto restored_catalogue = capture(catalogue, "magic43 discovered recipe directory");
    for (auto *state : {&catalogue, &restored_catalogue}) {
        command(*state, recipes, Action::select, "magic43 choose recipe1", 0);
        command(*state, recipes, Action::confirm, "magic43 actual recipe47");
        initialize(*state, "magic47 confirmation");
    }
    same(catalogue, restored_catalogue, "restored43 opens same47 without spending elements");
    const auto confirmation = catalogue.scripts.pages.back().id;
    check(catalogue.scripts.pages.back().legacy_page == 47 &&
              catalogue.scripts.pages.back().legacy_g == 1 && catalogue.legacy_n[3] == 100 &&
              catalogue.legacy_n[4] == 20 && catalogue.legacy_n[5] == 30,
          "43 binds recipe1 but has not paid its100/20/30/0 costs");
    catalogue.page_counters.at(confirmation) = 7;
    auto restored_confirmation = capture(catalogue, "magic47 before recipe commit");
    const auto reward_inventory = catalogue.items.at(5).inventory;
    const auto recipe_draws = catalogue.scene.random.draws();
    for (auto *state : {&catalogue, &restored_confirmation})
        command(*state, confirmation, Action::confirm, "magic47 craft recipe1");
    same(catalogue, restored_confirmation, "restored47 crafts exact same reward and message");
    check(catalogue.legacy_n[3] == 0 && catalogue.legacy_n[4] == 0 &&
              catalogue.legacy_n[5] == 0 && catalogue.legacy_n[6] == 0 &&
              catalogue.items.at(5).inventory == reward_inventory + 1 &&
              catalogue.scene.random.draws() == recipe_draws &&
              catalogue.scripts.event_calls.at(107) == 1,
          "47 pays fixed recipe1 four costs once, grants one item5, invokes107 without draw");
    const auto spent = startup_world_state_digest(catalogue);
    check(act_startup_world_magic_pot_page(catalogue, confirmation, Action::confirm) ==
              StartupWorldRuntimeError::invalid_page && startup_world_state_digest(catalogue) == spent,
          "retired47 cannot charge or grant twice");

    for (int raw : {45, 46}) {
        auto result = baseline;
        ref::WorldScriptPage p;
        p.id = result.scripts.next_page_id++;
        p.kind = ref::WorldScriptPageKind::raw_page;
        p.legacy_page = raw;
        p.legacy_g = raw == 46 ? 1 : -1;
        result.scripts.pages.push_back(p);
        initialize(result, raw == 45 ? "magic45 queued display" : "magic46 queued discovery");
        result.magic_pot_display = {{{{7, 8, 9, 10, 11}}, {{8, 9, 10, 11, 12}}, {{1, 1, 1, 1, 1}}}};
        result.magic_pot_output = {1, 1, 1, 1}; // 只读演出值组合，不冒充自然处理m的产物。
        result.magic_pot_comment = "就那样吧";
        result.page_counters.at(p.id) = raw == 45 ? 83 : 7;
        auto replay = capture(result, raw == 45 ? "magic45 settled display" : "magic46 undiscovered waiting");
        const auto slots = result.legacy_n;
        const auto inventory = result.items;
        const auto random = result.scene.random.draws();
        for (auto *state : {&result, &replay}) command(*state, p.id, Action::confirm, "magic45/46 close");
        same(result, replay, "restored result page closes without replaying settlement");
        check(result.legacy_n == slots && result.scene.random.draws() == random &&
                  std::all_of(inventory.begin(), inventory.end(), [&](const auto &v) {
                      return result.items.at(v.first).inventory == v.second.inventory;
                  }), "45/46 restored confirmation repeats no element/inventory/random settlement");
        if (raw == 46)
            check(result.magic_pot_recipes.at(1).status == 1 &&
                      result.magic_pot_recipes.at(1).pending_notice &&
                      !result.scripts.event_calls.count(106),
                  "46 discovers recipe once with NEW, no106 notification");
        const auto closing = std::find_if(result.scripts.pages.begin(), result.scripts.pages.end(),
                                         [&](const auto &page) { return page.id == p.id; });
        check(closing != result.scripts.pages.end() && closing->lifecycle == 4,
              "45/46 confirmation marks finish mode before framework retirement");
        for (auto *state : {&result, &replay}) {
            auto retired = prepare_startup_world_runtime(*state);
            check(retired.candidate.has_value(), "45/46 real framework retirement candidate");
            *state = std::move(*retired.candidate);
        }
        same(result, replay, "restored45/46 real framework retirement same");
        check(!result.magic_pot_pages_initialized.count(p.id) && !result.magic_pot_page_data.count(p.id) &&
                  !result.magic_pot_page_lists.count(p.id) && !result.magic_pot_page_parents.count(p.id),
              "45/46 real framework finish retires every associated payload");
    }
    // 有界窗口消费的具名normal夹具：真实baseline，仅显式壶资格/库存前提，非自然解锁。
    auto window_state = baseline;
    window_state.scripts.user_flags |= 1U;
    window_state.items.at(0).inventory = 2;
    window_state.catalog.at({0, 0}) = window_state.items.at(0);
    const auto expected_window = startup_world_state_digest(window_state);
    const auto window_path = dir / "magic-pot-window.avrs";
    StartupWorldSaveMetadata window_metadata;
    window_metadata.producer_revision = "magic-pot-window:explicit-precondition-fixture-not-natural";
    check(save_startup_world_file(reencoded, baseline_session, window_metadata).ok,
          "window normal baseline template");
    write(broken, replace_state(read(reencoded), window_state));
    auto window_candidate = load_startup_world_file(broken, startup_world_rules(),
                                                    StartupWorldSavePurpose::normal);
    check(window_candidate.snapshot.has_value(), "window normal fixture candidate: " + window_candidate.error);
    check(save_startup_world_file(window_path, window_candidate.snapshot->session, window_metadata).ok,
          "publish owned window normal fixture");
    const auto window_load = load_startup_world_file(window_path, startup_world_rules(),
                                                     StartupWorldSavePurpose::normal);
    check(window_load.snapshot.has_value() &&
              startup_world_state_digest(window_load.snapshot->session.state()) == expected_window &&
              window_load.snapshot->metadata.producer_revision == window_metadata.producer_revision,
          "consume window fixture by exact normal readback");
    const auto window_bytes = std::filesystem::file_size(window_path);
    check(window_bytes > 0 && window_bytes <= 128U * 1024U * 1024U &&
              window_bytes == read(window_path).size(),
          "window fixture published bytes within file budget");
    std::cout << "magic-pot window fixture=" << window_path.generic_string()
              << " bytes=" << window_bytes << " qualification=explicit-fixture-not-natural\n";
    std::filesystem::remove(file);
    std::filesystem::remove(reencoded);
    std::filesystem::remove(broken);
}
void profile_file_roundtrip(const StartupWorldRuntimeSession &baseline, const std::filesystem::path &dir) {
    auto state = baseline.state(); // 未到访定义0覆盖的合法条件档；未改日期/人物/资金。
    state.human_profiles.emplace(0, StartupWorldHumanProfile{"存读姓名", 1, true});
    state.scripts.humans.at(0).name = "存读姓名";
    StartupWorldSaveMetadata metadata;
    const auto file = dir / "profile.avrs", encoded = dir / "profile-roundtrip.avrs",
               broken = dir / "profile-broken.avrs";
    const auto restored = capture_persistence_fixture(baseline, state, metadata, file, encoded,
                                                       broken, "named unvisited definition0");
    const auto profile = startup_world_human_profile(restored, 0);
    check(profile && profile->name == "存读姓名" && profile->sex == 1 && profile->custom_name &&
              restored.human_profiles.size() == 1 && restored.scene.world.world.ai.human_order ==
                  state.scene.world.world.ai.human_order,
          "normal file keeps main profile independently of actor arrival");
    auto old_layout = read(file);
    std::size_t at = 20;
    skip_text(old_layout, at);
    const auto length = number(old_layout, at, 4);
    const std::string old_schema = "e5a40276fe0b1dfc0c8ef3f3f6eb703bcbc475f14e106e0685472acc2df66d7f";
    check(length == old_schema.size() &&
              std::string(old_layout.begin() + static_cast<std::ptrdiff_t>(at),
                          old_layout.begin() + static_cast<std::ptrdiff_t>(at + length)) != old_schema,
          "new profile fields change layout identity, not original behavior oracle");
    std::copy(old_schema.begin(), old_schema.end(), old_layout.begin() + static_cast<std::ptrdiff_t>(at));
    resign(old_layout);
    write(broken, old_layout);
    const auto rejected = load_startup_world_file(broken, startup_world_rules(), metadata.purpose);
    check(!rejected.snapshot && rejected.error.find("状态字段布局不兼容") != std::string::npos,
          "prior profile-less schema explicitly rejects without migration");
    std::filesystem::remove(file);
    std::filesystem::remove(encoded);
    std::filesystem::remove(broken);
}
void application_clear_roundtrip(const StartupWorldRuntimeSession &baseline, const std::filesystem::path &dir) {
    auto state = baseline.state();
    state.completion_mode = state.system_completion_mode = 1;
    ref::WorldScriptPage page;
    page.kind = ref::WorldScriptPageKind::raw_page;
    page.legacy_page = 17;
    const auto pushed = ref::prepare_world_script_page(startup_world_runtime_scripts(state), page);
    check(pushed.candidate && write_startup_world_runtime_scripts(state, pushed.candidate->state),
          "clear page uses actual script stack factory");
    const auto id = state.scripts.pages.back().id;
    StartupWorldSaveMetadata metadata;
    metadata.purpose = StartupWorldSavePurpose::replay;
    metadata.controller_id = "application-clear-entry-fixture-v1";
    metadata.controller_state = {1}; // 版本1入口，无已推进应用计分状态。
    const auto entry = dir / "clear-entry.avrs", encoded = dir / "clear-encoded.avrs", bad = dir / "clear-bad.avrs";
    capture_persistence_fixture(baseline, state, metadata, entry, encoded, bad,
                                "explicit raw17 entry fixture, not natural sixteen years");
    StartupApplicationPaths paths{dir / "clear-application"};
    check(std::filesystem::create_directory(paths.root), "exclusive application root for clear conditions");
    struct ClearRootGuard {
        std::filesystem::path root;
        ~ClearRootGuard() { std::error_code error; std::filesystem::remove_all(root, error); }
    } root_guard{paths.root};
    const auto system = paths.root / "system.avr";
    check(save_startup_system_file(system, {}).empty(), "fresh independent system fixture");
    StartupApplication app(paths, ref::WorldRandomStream::from_java_seed(1));
    check(app.load_world_replay(entry, metadata.controller_id).empty(), "load validated raw17 entry");
    check(app.take_audio_requests() == std::vector<StartupAudioRequest>{
              {StartupAudioOperation::replace_bgm,0},{StartupAudioOperation::replace_bgm,1}} &&
              app.world()->state().sound_requests.empty(),
          "conditional entry activates a world and consumes B0/G before clear-page sound oracle");
    check(app.world()->state().cash_peak == 0 && app.records().cash_peak == 0,
          "loaded old world cash mirror does not reconstruct system record");
    for (int i = 0; i < 4000; ++i) {
        if (app.clear_page() && app.clear_page()->stage == 6 && app.clear_page()->counter == 64) break;
        const auto error = app.update(true);
        check(error.empty(), "application clear update: " + error);
        check(i != 3999, "bounded clear reaches final transaction");
    }
    const auto total = app.clear_page()->sum;
    check(total > 0 && app.records().high_score == 0, "score remains private until actual exit");
    const auto before = startup_world_session_digest(*app.world());
    const auto original = read(system);
    const auto original_directory = app.records().save_directory;
#ifdef _WIN32
    const auto handle = CreateFileW(system.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    check(handle != INVALID_HANDLE_VALUE, "lock clear system fixture");
    const auto error = app.update();
    check(CloseHandle(handle) != 0, "close clear lock");
    check(!error.empty() && app.clear_page() && app.clear_page()->counter == 64 &&
          startup_world_session_digest(*app.world()) == before && read(system) == original &&
          app.records().high_score == 0 && app.records().save_directory == original_directory &&
          app.take_audio_requests().empty(),
          "failed clear commit preserves page, events, random, directories and pending outputs");
#else
    (void)before; (void)original;
#endif
    check(app.update().empty(), "clear final transaction retry");
    check(!app.clear_page() && !app.clear_rows() && app.records().high_score == total &&
          load_startup_system_file(system).records->high_score == total,
          "clear completes once, persists system and retires controller references");
    check(app.take_sound_requests() == std::vector<int>{1}, "clear resumes main BGM once");
    check(!app.acknowledge_page(id).empty(), "retired clear cannot award again");
    const auto storage = load_startup_application_storage(paths.root);
    check(storage.snapshot.has_value(), "clear system remains a valid application storage snapshot");
    const auto view = capture_startup_application_storage(paths.root, *storage.snapshot);
    check(app.records().save_directory == original_directory &&
          original_directory == StartupSystemRecords{}.save_directory && view.view && view.view->blobs.empty() &&
          (!std::filesystem::exists(paths.root / "worlds") || std::filesystem::is_empty(paths.root / "worlds")),
          "clear record save preserves all four empty directories and publishes no world blob");
    const auto scripts = startup_world_runtime_scripts(app.world()->state());
    check(scripts.pages.size() >= 3, "event6 then new-record event publish real message pages");
    // 同一合法入口再次回放：分数相等不能改名或奖杯，不当自然第二次结局。
    check(app.return_to_title().empty() && app.load_world_replay(entry, metadata.controller_id).empty(),
          "replay same clear against existing system record");
    check(app.take_sound_requests() == std::vector<int>({0,1}),
          "second conditional activation consumes its own title B0/G before equal-score run");
    for (int i = 0; i < 4000; ++i) {
        check(app.update(true).empty(), "equal-score clear update");
        if (!app.clear_page()) break;
        check(i != 3999, "bounded equal-score clear completes");
    }
    check(app.records().high_score == total && app.records().trophy == 1 &&
          app.records().score_village == state.scripts.village_name, "equal score preserves existing record owner");
    app.take_sound_requests();
    for (const auto &p : {entry, encoded, bad}) std::filesystem::remove(p);
}
void run(const std::filesystem::path &dir) {
    std::filesystem::create_directories(dir);
    const auto replay = dir / "replay.avrs", normal = dir / "normal.avrs", bad = dir / "bad.avrs";
    StartupSession initial;
    StartupWorldRuntimeSession session(initial.state(), ref::WorldRandomStream::from_java_seed(1));
    int frames{};
    for (; frames < 2200; ++frames) {
        advance(session);
        if (!session.checkpoints().empty() &&
            !session.state().scene.world.world.ai.human_order.empty() &&
            session.state().scripts.pages.size() == 1 && session.state().scene.scene_state == 0) {
            ++frames;
            break;
        }
    }
    check(frames < 2200, "stable natural capture boundary");
    checks += check_startup_world_restore_contracts(session.state());
    profile_file_roundtrip(session, dir);
    application_clear_roundtrip(session, dir);
    const auto before = startup_world_session_digest(session);
    StartupWorldSaveMetadata metadata;
    metadata.purpose = StartupWorldSavePurpose::replay;
    metadata.producer_revision = "persistence-contract";
    metadata.controller_id = "test-driver-v1";
    metadata.next_frame = static_cast<std::uint64_t>(frames);
    metadata.controller_state = {1, 0, 255, 42};
    metadata.extensions = {{1024, 7, {255, 0, 254, 1}}};
    auto saved = save_startup_world_file(replay, session, metadata);
    check(saved.ok, "replay save: " + saved.error);
    const auto bytes = read(replay);
    check(std::string(bytes.begin(), bytes.begin() + 8) == "AVRSAVE1" && bytes[8] == 1 &&
              bytes[9] == 0 && bytes[12] == 4 && bytes[13] == 0 && bytes[16] == 2,
          "format magic/schema/purpose oracle");
    check(startup_world_session_digest(session) == before,
          "capture consumes no state/random/history");
    auto loaded = load_startup_world_file(replay, startup_world_rules(), metadata.purpose,
                                          metadata.controller_id);
    check(loaded.snapshot.has_value(), "replay load: " + loaded.error);
    check(startup_world_session_digest(loaded.snapshot->session) == before,
          "complete world/random/history roundtrip");
    check(loaded.snapshot->metadata.controller_state == metadata.controller_state &&
              loaded.snapshot->metadata.next_frame == metadata.next_frame,
          "controller position preserved");
    check(loaded.snapshot->metadata.extensions.size() == 1 &&
              loaded.snapshot->metadata.extensions[0].bytes == metadata.extensions[0].bytes,
          "opaque optional payload preserved");
    check(loaded.snapshot->session.checkpoints().front() != session.checkpoints().front(),
          "file restored independent history allocations");
    check(save_startup_world_file(bad, loaded.snapshot->session, loaded.snapshot->metadata).ok &&
              read(bad) == bytes,
          "canonical file byte roundtrip");
    auto second = load_startup_world_file(replay, startup_world_rules(), metadata.purpose,
                                          metadata.controller_id);
    check(second.snapshot.has_value(), "second file restore");
    auto uninterrupted = session;
    for (int frame = 0; frame < 90; ++frame) {
        for (auto *s : {&uninterrupted, &loaded.snapshot->session, &second.snapshot->session}) {
            s->set_paused(frame < 3);
            s->set_speed(frame % 11 == 0 ? 1 : 0);
        }
        const auto sounds = advance(uninterrupted);
        check(advance(loaded.snapshot->session) == sounds &&
                  advance(second.snapshot->session) == sounds,
              "restored output sequence");
        const auto digest = startup_world_session_digest(uninterrupted);
        check(startup_world_session_digest(loaded.snapshot->session) == digest &&
                  startup_world_session_digest(second.snapshot->session) == digest,
              "full canonical state each suffix frame");
    }
    StartupWorldSaveMetadata normal_meta;
    saved = save_startup_world_file(normal, session, normal_meta);
    check(saved.ok, "normal save: " + saved.error);
    auto normal_load =
        load_startup_world_file(normal, startup_world_rules(), StartupWorldSavePurpose::normal);
    check(normal_load.snapshot.has_value(), "normal load: " + normal_load.error);
    check(startup_world_state_digest(normal_load.snapshot->session.state()) ==
              startup_world_state_digest(session.state()),
          "normal complete world/random restored");
    check(normal_load.snapshot->session.checkpoints().empty(),
          "normal save excludes test audit history");
    check(!load_startup_world_file(normal, startup_world_rules(), metadata.purpose,
                                   metadata.controller_id)
               .snapshot,
          "purpose mismatch rejected");
    check(!load_startup_world_file(replay, startup_world_rules(), metadata.purpose, "wrong-driver")
               .snapshot,
          "controller mismatch rejected");
    for (const auto offset :
         {std::size_t(0), std::size_t(8), std::size_t(12), std::size_t(16), std::size_t(24)}) {
        auto altered = bytes;
        altered[offset] ^= 1;
        resign(altered);
        write(bad, altered);
        check(!load_startup_world_file(bad, startup_world_rules(), metadata.purpose,
                                       metadata.controller_id)
                   .snapshot,
              "incompatible header rejected after valid checksum");
    }
    for (const auto old_version : {1, 2, 3}) {
        auto old_semantics = bytes;
        old_semantics[12] = static_cast<std::uint8_t>(old_version);
        resign(old_semantics);
        write(bad, old_semantics);
        check(!load_startup_world_file(bad, startup_world_rules(), metadata.purpose,
                                       metadata.controller_id).snapshot,
              "prior initialization, audio or stale arrival price semantics rejects without migration");
    }
    auto truncated = bytes;
    truncated.pop_back();
    write(bad, truncated);
    check(!load_startup_world_file(bad, startup_world_rules(), metadata.purpose,
                                   metadata.controller_id)
               .snapshot,
          "truncated file rejected");
    auto trailing = bytes;
    trailing.push_back(1);
    write(bad, trailing);
    check(!load_startup_world_file(bad, startup_world_rules(), metadata.purpose,
                                   metadata.controller_id)
               .snapshot,
          "trailing bytes rejected");
    auto malformed = session.state();
    malformed.actor_metadata.erase(malformed.scene.world.world.ai.human_order.front());
    write(bad, replace_state(bytes, malformed));
    auto refused = load_startup_world_file(bad, startup_world_rules(), metadata.purpose,
                                           metadata.controller_id);
    check(!refused.snapshot && refused.error.find("状态拒绝") != std::string::npos,
          "missing actor metadata rejected by pure validator: " + refused.error);
    check(startup_world_session_digest(session) == before,
          "failed decode preserves current session");

    auto pages = session;
    check(pages.open_village_activities() == StartupWorldRuntimeError::none,
          "open actual activity page");
    for (int i = 0; i < 5 && pages.state().activity_pages_initialized.empty(); ++i) {
        check(pages.update().candidate.has_value(), "initialize activity page");
        pages.take_sound_requests();
    }
    check(!pages.state().activity_pages_initialized.empty(), "actual initialized page");
    check(!save_startup_world_file(normal, pages, normal_meta).ok, "normal modal capture rejected");
    const auto modal = save_startup_world_file(bad, pages, metadata);
    check(modal.ok, "replay modal capture: " + modal.error);
    const auto modal_bytes = read(bad);
    auto broken_page = pages.state();
    broken_page.page_counters.erase(*broken_page.activity_pages_initialized.begin());
    write(bad, replace_state(modal_bytes, broken_page));
    refused = load_startup_world_file(bad, startup_world_rules(), metadata.purpose,
                                      metadata.controller_id);
    check(!refused.snapshot && refused.error.find("状态拒绝") != std::string::npos,
          "initialized missing counter rejected: " + refused.error);

    facility_catalog_replay(session, dir, metadata);
    magic_pot_replay(session, dir, metadata);

    const auto normal_before = read(normal);
#ifdef _WIN32
    HANDLE held = CreateFileW(normal.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    check(held != INVALID_HANDLE_VALUE, "lock existing save for real replace failure");
    const auto failed = save_startup_world_file(normal, session, normal_meta);
    CloseHandle(held);
    check(!failed.ok && read(normal) == normal_before, "replace failure keeps old valid file");
#endif
    check(!save_startup_world_file(dir / "missing-parent" / "save.avrs", session, normal_meta).ok,
          "missing parent failure explicit");
    check(save_startup_world_file(normal, session, normal_meta).ok && read(normal) == normal_before,
          "repeated save replaces atomically");
    for (const auto &entry : std::filesystem::directory_iterator(dir))
        check(entry.path().filename().string().find(".tmp.") == std::string::npos,
              "temporary file cleaned");
    check(startup_world_session_digest(session) == before, "all file operations preserve source");
    std::filesystem::remove(replay);
    std::filesystem::remove(normal);
    std::filesystem::remove(bad);
    checks += run_startup_application_replay_checks(dir);
    checks += run_startup_application_actions_checks(dir);
    checks += run_startup_application_natural_driver_checks(dir);
    std::cout << "persistence checks=" << checks << " prefix_frames=" << frames
              << " suffix_frames=90 replay_bytes=" << bytes.size() << '\n';
}
// 独立表现调用控制器，使用既有文件metadata；不修改自然AWDRV1或Owner编码。
struct PresentationController {
    std::uint64_t next_round{}, next_ordinal{1}, checked{};
    bool consumed{true};
};
constexpr const char *presentation_controller_id = "presentation-request-v2";
constexpr std::array<int,5> presentation_calls{0,1,2,1,0};
Bytes encode_presentation_controller(const PresentationController &driver) {
    Bytes bytes{'A','V','P','R','Q','0','0','1'};
    append64(bytes,driver.next_round); append64(bytes,driver.next_ordinal);
    append64(bytes,driver.checked); bytes.push_back(driver.consumed?1:0);
    return bytes;
}
PresentationController decode_presentation_controller(const Bytes &bytes) {
    if (bytes.size()!=33 || std::string(bytes.begin(),bytes.begin()+8)!="AVPRQ001")
        throw std::invalid_argument("presentation controller size/magic mismatch");
    std::size_t at=8;
    PresentationController result;
    result.next_round=number(bytes,at,8); result.next_ordinal=number(bytes,at,8);
    result.checked=number(bytes,at,8);
    if (bytes.at(at)!=1 || result.next_round>presentation_calls.size())
        throw std::invalid_argument("presentation controller pending output/round mismatch");
    result.consumed=true;
    std::uint64_t ordinal=1;
    for (std::size_t n=0;n<result.next_round;++n) ordinal+=presentation_calls[n];
    if (result.next_ordinal!=ordinal || result.checked>100000)
        throw std::invalid_argument("presentation controller ordinal/checks mismatch");
    return result;
}
std::string hexadecimal(const Bytes &bytes) {
    constexpr char digits[]="0123456789abcdef";
    std::string result; result.reserve(bytes.size()*2);
    for (const auto b:bytes) { result.push_back(digits[b>>4]); result.push_back(digits[b&15]); }
    return result;
}
Bytes encode_presentation_output(const StartupPresentationRequest &request,
                                 const StartupPresentationPlan &plan) {
    Bytes b;
    append64(b,request.ordinal); append64(b,static_cast<std::uint64_t>(request.mode));
    b.push_back(request.application_preview); b.push_back(request.sound_paused);
    b.push_back(request.gift_wrapper_ready);
    append64(b,request.expected_pages.size()); for (auto id:request.expected_pages) append64(b,id);
    append64(b,plan.ordinal); append64(b,plan.pages.size()); for (auto id:plan.pages) append64(b,id);
    append64(b,plan.dungeon_jitters.size());
    for (const auto &j:plan.dungeon_jitters) {
        append64(b,j.page); append64(b,j.facility); append64(b,j.challenge);
        append64(b,static_cast<std::uint64_t>(static_cast<std::int64_t>(j.offset)));
    }
    b.push_back(plan.cleared_task.has_value()); if (plan.cleared_task) append64(b,*plan.cleared_task);
    append64(b,plan.sound_requests); append64(b,plan.random_before); append64(b,plan.random_after);
    return b;
}
StartupWorldRuntimeSession presentation_fixture(const std::filesystem::path &path,
                                                 PresentationController &driver) {
    StartupSession initial;
    StartupWorldRuntimeSession baseline(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    auto state=baseline.state();
    const auto created=ref::prepare_world_task_creation(startup_world_runtime_factory(state),0);
    check(created.candidate && created.candidate->created_task &&
              write_startup_world_runtime_factory(state,created.candidate->state),
          "presentation condition fixture uses original task factory and complete Owner writer");
    const auto task=*created.candidate->created_task;
    check(state.tasks.at(task).facility.has_value(),"presentation factory binds actual dungeon facility");
    state.active_task=task; state.scene.world.world.ai.task_active=true;
    state.dungeon_facilities.at(*state.tasks.at(task).facility).challenges=
        {{0,1,1,10,0,0},{1,1,1,9,0,0},{2,0,1,20,0,0},{3,1,2,20,0,0},{4,1,1,10,0,0}};
    state.scene.random=ref::WorldRandomStream::from_raw({2,0,1,2,0,1,2,0});
    state.scripts.pages.front().lifecycle=3;
    // 66是明确等待页条件夹具，不宣称真实玩家已经赠送或自然到达计数45。
    ref::WorldScriptPage gift;
    gift.id=state.scripts.next_page_id++; gift.kind=ref::WorldScriptPageKind::raw_page;
    gift.legacy_page=66; gift.lifecycle=2;
    state.scripts.pages.push_back(gift);
    state.scripts.executing_page.reset();
    state.page_human_bindings[gift.id]=1; state.human_equipment_choices[gift.id]={4,0};
    state.human_pages_initialized.insert(gift.id); state.human_page_selections[gift.id]=0;
    state.page_counters[gift.id]=45; state.page_phases[gift.id]=0;
    state.human_gift_scores[gift.id]=0; state.human_gift_messages[gift.id]="表现等待条件夹具";
    state.sound_requests.clear();
    std::string reason;
    check(persistence_detail::validate_restored_state(state,reason),"presentation fixture validates: "+reason);
    StartupWorldSaveMetadata metadata;
    metadata.purpose=StartupWorldSavePurpose::replay; metadata.controller_id=presentation_controller_id;
    metadata.controller_state=encode_presentation_controller(driver);
    const auto seed=std::filesystem::path(path.string()+".seed");
    const auto fixture=std::filesystem::path(path.string()+".fixture");
    if (std::filesystem::exists(seed) || std::filesystem::exists(fixture))
        throw std::invalid_argument("presentation fixture temporary path already exists");
    const auto saved=save_startup_world_file(seed,baseline,metadata);
    if (!saved.ok) throw std::runtime_error("presentation fixture baseline save: "+saved.error);
    write(fixture,replace_state(read(seed),state));
    auto loaded=load_startup_world_file(fixture,startup_world_rules(),metadata.purpose,presentation_controller_id);
    check(loaded.snapshot.has_value(),"presentation fixture formal loader: "+loaded.error);
    std::filesystem::remove(seed); std::filesystem::remove(fixture);
    driver.checked=checks;
    return std::move(loaded.snapshot->session);
}
void presentation_replay(int argc,const char **argv) {
    std::map<std::string,std::string> options;
    for (int n=2;n<argc;n+=2) {
        if (n+1==argc || !options.emplace(argv[n],argv[n+1]).second)
            throw std::invalid_argument("presentation options missing value/duplicate");
        if (std::string(argv[n])!="--save-file" && std::string(argv[n])!="--load-file" &&
            std::string(argv[n])!="--trace-file" && std::string(argv[n])!="--trace-from")
            throw std::invalid_argument("presentation unknown option");
    }
    if (!options.count("--trace-file") || options.count("--save-file")==options.count("--load-file"))
        throw std::invalid_argument("presentation requires trace and exactly one save/load mode");
    const auto file=std::filesystem::path(options.at(options.count("--load-file")?"--load-file":"--save-file"));
    const auto trace_file=std::filesystem::path(options.at("--trace-file"));
    if (file.lexically_normal()==trace_file.lexically_normal())
        throw std::invalid_argument("presentation trace cannot overwrite snapshot");
    PresentationController driver;
    auto session=[&] {
        if (!options.count("--load-file")) return presentation_fixture(file,driver);
        auto loaded=load_startup_world_file(file,startup_world_rules(),StartupWorldSavePurpose::replay,presentation_controller_id);
        if (!loaded.snapshot) throw std::runtime_error("presentation load: "+loaded.error);
        auto candidate=decode_presentation_controller(loaded.snapshot->metadata.controller_state);
        if (candidate.next_round!=loaded.snapshot->metadata.next_frame || candidate.next_round!=2 ||
            loaded.snapshot->session.state().scene.random.draws()!=2 ||
            loaded.snapshot->session.state().scripts.pages.back().legacy_page!=66 ||
            loaded.snapshot->session.state().page_counters.at(loaded.snapshot->session.state().scripts.pages.back().id)!=45)
            throw std::invalid_argument("presentation controller/Owner capture boundary mismatch");
        driver=candidate; checks=static_cast<int>(driver.checked);
        return std::move(loaded.snapshot->session);
    }();
    // 严格控制器拒绝主责与文件用途/Controller身份隔离共用本短模式，不安装失败候选。
    const auto controller=encode_presentation_controller(driver);
    const auto before_rejection=startup_world_session_digest(session);
    auto bad=controller; bad.back()=0;
    bool refused{};
    try { (void)decode_presentation_controller(bad); } catch (const std::invalid_argument &) { refused=true; }
    if (!refused || startup_world_session_digest(session)!=before_rejection)
        throw std::runtime_error("presentation pending output controller must reject without world install");
    bad=controller; bad[16]^=1; refused=false;
    try { (void)decode_presentation_controller(bad); } catch (const std::invalid_argument &) { refused=true; }
    if (!refused || startup_world_session_digest(session)!=before_rejection)
        throw std::runtime_error("presentation wrong next ordinal must reject without world install");
    if (options.count("--trace-from") && (options.at("--trace-from").empty() ||
        options.at("--trace-from").find_first_not_of("0123456789")!=std::string::npos))
        throw std::invalid_argument("presentation trace round must be unsigned integer");
    const auto start=options.count("--trace-from") ? std::stoull(options.at("--trace-from")) : driver.next_round;
    if (start>presentation_calls.size()) throw std::invalid_argument("presentation trace round out of range");
    std::ofstream trace(trace_file,std::ios::binary|std::ios::trunc);
    if (!trace) throw std::runtime_error("presentation trace open failed");
    for (std::size_t round=driver.next_round;round<presentation_calls.size();++round) {
        Bytes outputs; append64(outputs,presentation_calls[round]);
        const auto at_round=startup_world_session_digest(session);
        const auto draws_before=session.state().scene.random.draws();
        for (int call=0;call<presentation_calls[round];++call) {
            driver.consumed=false;
            StartupPresentationRequest request;
            request.ordinal=driver.next_ordinal++; request.mode=StartupPresentationMode::full_redraw;
            request.gift_wrapper_ready=true;
            const auto pages=startup_world_presentation_pages(session.state(),request.mode);
            check(pages.has_value(),"presentation actual stack admission available");
            request.expected_pages=*pages;
            const auto result=session.present(request);
            check(result.error==StartupWorldRuntimeError::none && result.plan && result.candidate,
                  "presentation Session commits actual request transaction");
            const auto &plan=*result.plan;
            constexpr std::array<int,8> jitter{1,-1,0,1,-1,0,1,-1};
            check(plan.ordinal==request.ordinal && plan.pages==request.expected_pages &&
                      plan.dungeon_jitters.size()==2 && plan.dungeon_jitters[0].challenge==4 &&
                      plan.dungeon_jitters[1].challenge==0 && plan.random_after==plan.random_before+2 &&
                      plan.dungeon_jitters[0].offset==jitter.at(plan.random_before) &&
                      plan.dungeon_jitters[1].offset==jitter.at(plan.random_before+1) &&
                      !plan.cleared_task && plan.sound_requests==1,
                  "presentation explicit tickets, inverse challenge order and one ready66 sound per request");
            const auto encoded=encode_presentation_output(request,plan);
            append64(outputs,encoded.size()); outputs.insert(outputs.end(),encoded.begin(),encoded.end());
            driver.consumed=true;
        }
        const auto sounds=session.take_audio_requests();
        std::vector<int> sound_ids;
        for (const auto &sound : sounds) sound_ids.push_back(sound.id);
        check(sound_ids==std::vector<int>(presentation_calls[round],8),"presentation round consumes exact ordered request sound output");
        check(sounds==std::vector<StartupAudioRequest>(presentation_calls[round],
                  {StartupAudioOperation::ordinary_play,8}) && session.take_sound_requests().empty(),
              "presentation retains original ordinary-play operations and cannot consume twice");
        check(session.state().scene.random.draws()==draws_before+2*presentation_calls[round] &&
                  session.state().page_counters.at(session.state().scripts.pages.back().id)==45 &&
                  (presentation_calls[round]!=0 || startup_world_session_digest(session)==at_round),
              "presentation same-state0/1/2 calls do not simulate tick or advance gift counter");
        Bytes encoded_sounds; append64(encoded_sounds,sounds.size());
        for (const auto &sound:sounds) {
            append64(encoded_sounds,static_cast<std::uint64_t>(sound.operation));
            append64(encoded_sounds,static_cast<std::uint64_t>(sound.id));
        }
        driver.next_round=round+1; driver.checked=checks;
        if (round>=start)
            trace<<round<<' '<<startup_world_session_digest(session)<<' '
                 <<hexadecimal(encode_presentation_controller(driver))<<' '<<hexadecimal(outputs)<<' '
                 <<hexadecimal(encoded_sounds)<<'\n';
        if (!trace) throw std::runtime_error("presentation trace write failed");
        if (round==1 && options.count("--save-file")) {
            StartupWorldSaveMetadata metadata;
            metadata.purpose=StartupWorldSavePurpose::replay; metadata.controller_id=presentation_controller_id;
            metadata.next_frame=driver.next_round; metadata.controller_state=encode_presentation_controller(driver);
            const auto saved=save_startup_world_file(file,session,metadata);
            if (!saved.ok) throw std::runtime_error("presentation capture: "+saved.error);
            const auto before=startup_world_session_digest(session);
            if (load_startup_world_file(file,startup_world_rules(),metadata.purpose,
                                       "natural-progression-expansion-v2").snapshot ||
                load_startup_world_file(file,startup_world_rules(),StartupWorldSavePurpose::normal).snapshot ||
                startup_world_session_digest(session)!=before)
                throw std::runtime_error("presentation controller/purpose isolation failed");
        }
    }
    trace.flush(); if (!trace) throw std::runtime_error("presentation trace flush failed");
    std::cout<<"presentation summary rounds="<<driver.next_round<<" ordinal="<<driver.next_ordinal
             <<" random="<<session.state().scene.random.draws()<<" requests=4 sounds=4 checks="<<checks<<'\n';
}
} // namespace
// 多种应用接线条件共享一次可恢复的真实早期世界准备，不跑完整年度。
static StartupWorldRuntimeSession application_fixture_baseline() {
    StartupSession initial;
    StartupWorldRuntimeSession baseline(initial.state(), ref::WorldRandomStream::from_java_seed(1));
    int frame{};
    for (; frame < 2200; ++frame) {
        advance(baseline);
        if (!baseline.checkpoints().empty() && !baseline.state().scene.world.world.ai.human_order.empty() &&
            baseline.state().scripts.pages.size() == 1 && baseline.state().scene.scene_state == 0) break;
    }
    check(frame < 2200, "application fixture stable source boundary");
    return baseline;
}
// 应用回放测试沿相同严格文件夹具取得原raw17入口，不把该条件档称自然通关。
std::filesystem::path create_application_clear_entry_fixture(const std::filesystem::path &dir) {
    auto baseline = application_fixture_baseline();
    auto state = baseline.state();
    state.completion_mode = state.system_completion_mode = 1;
    ref::WorldScriptPage page; page.kind = ref::WorldScriptPageKind::raw_page; page.legacy_page = 17;
    const auto pushed = ref::prepare_world_script_page(startup_world_runtime_scripts(state), page);
    check(pushed.candidate && write_startup_world_runtime_scripts(state, pushed.candidate->state),
          "application fixture uses actual raw17 factory");
    StartupWorldSaveMetadata metadata; metadata.purpose = StartupWorldSavePurpose::replay;
    metadata.controller_id = "application-clear-entry-fixture-v1"; metadata.controller_state = {1};
    const auto entry = dir / "clear-entry.avrs";
    const auto encoded = dir / "clear-encoded.avrs", bad = dir / "clear-bad.avrs";
    capture_persistence_fixture(baseline, state, metadata, entry, encoded, bad,
                                "explicit raw17 application replay entry, not natural sixteen years");
    std::filesystem::remove(encoded); std::filesystem::remove(bad); return entry;
}
// 仅插入待测页面及年度勋章条件；四档不是自然到达年度/晋级/商会的证明。
std::array<std::filesystem::path, 4> create_application_action_entry_fixtures(
    const std::filesystem::path &dir) {
    auto baseline = application_fixture_baseline();
    std::array<std::filesystem::path, 4> entries;
    const std::array<int, 4> raw{48, 83, 87, 87};
    for (std::size_t i = 0; i < entries.size(); ++i) {
        auto state = baseline.state();
        if (raw[i] == 87) state.medal_count = i == 2 ? 1 : 0;
        ref::WorldScriptPage page;
        page.kind = ref::WorldScriptPageKind::raw_page; page.legacy_page = raw[i];
        const auto pushed = ref::prepare_world_script_page(startup_world_runtime_scripts(state), page);
        check(pushed.candidate && write_startup_world_runtime_scripts(state, pushed.candidate->state),
              "application action fixture uses actual page factory");
        if (i == 3) {
            // 原87首次初始化先增加一枚勋章；零勋关闭只可能检查已初始化后的耗尽条件。
            const auto id = state.scripts.pages.back().id;
            check(act_startup_world_runtime_award_page(state, id, ref::WorldAwardAction::update, 0) ==
                      StartupWorldRuntimeError::none && state.medal_count == 1 &&
                      state.award_rankings.count(id),
                  "zero-medal fixture first uses actual annual initialization increment");
            state.sound_requests.clear(); // 夹具领取并丢弃初始化声音，不混入待测请求输出。
            state.medal_count = 0; // 明确条件夹具；不声称自然授出过该枚勋章。
        }
        StartupWorldSaveMetadata metadata; metadata.purpose = StartupWorldSavePurpose::replay;
        metadata.controller_id = "application-action-entry-fixture-v1"; metadata.controller_state = {1};
        entries[i] = dir / ("action-entry-" + std::to_string(i) + ".avrs");
        const auto encoded = dir / ("action-encoded-" + std::to_string(i) + ".avrs");
        const auto bad = dir / ("action-input-" + std::to_string(i) + ".avrs");
        capture_persistence_fixture(baseline, state, metadata, entries[i], encoded, bad,
                                    "explicit application command fixture, not natural progression");
        std::filesystem::remove(encoded); std::filesystem::remove(bad);
    }
    return entries;
}
int main(int argc, const char **argv) {
    try {
        if (argc >= 2 && std::string(argv[1]) == "application-natural-clear-v3")
            return run_startup_application_natural_replay_cli(argc, argv);
        if (argc >= 2 && std::string(argv[1]) == "application-clear-conditions-v3")
            return run_startup_application_replay_cli(argc, argv);
        if (argc>=2 && std::string(argv[1])==presentation_controller_id) {
            presentation_replay(argc,argv); return 0;
        }
        check(argc == 2, "expected test directory");
        run_startup_world_codec_checks();
        run(argv[1]);
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
