#include "ark/simulation/presentation/startup_title_presentation.hpp"
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace ark::simulation;
using Random = ark::simulation::rules::WorldRandomStream;
struct Checks {
    int count{};
    void operator()(bool ok, const std::string &message) {
        ++count;
        if (!ok)
            throw std::runtime_error("标题表现：" + message);
    }
};
bool same(const StartupTitlePresentation &a, const StartupTitlePresentation &b) {
    if (a.l != b.l || a.f132f != b.f132f || a.s != b.s || a.t != b.t)
        return false;
    for (std::size_t i = 0; i < a.slots.size(); ++i) {
        const auto &x = a.slots[i];
        const auto &y = b.slots[i];
        if (x.active != y.active || x.definition != y.definition || x.x != y.x ||
            x.y != y.y || x.direction != y.direction || x.age != y.age)
            return false;
    }
    return true;
}
bool same_random(const Random &a, const Random &b) {
    const auto x = a.snapshot(), y = b.snapshot();
    return x.engine_state == y.engine_state && x.tape == y.tape &&
           x.cursor == y.cursor && x.tape_mode == y.tape_mode;
}
const StartupTitleUpdateCandidate &accepted(Checks &check, const StartupTitleUpdateResult &r) {
    check(r.error.empty() && r.candidate.has_value(), "合法候选应成功：" + r.error);
    return *r.candidate;
}
StartupTitleUpdateRequest update(bool confirm = false) {
    return {StartupTitleAdmission::top_lifecycle_ready, confirm};
}
} // namespace

// 归入既有应用套件；固定磁带和手写期望独立于被测生成／排序函数。
int check_startup_title_presentation() {
    Checks check;
    StartupTitlePresentation initial;
    const auto empty = Random::from_raw({});
    auto before_gate = initial;
    before_gate.l = 98;
    const auto gate_wait = prepare_startup_title_update(before_gate, empty, update());
    const auto &wait = accepted(check, gate_wait);
    check(wait.state.l == 99 && wait.state.f132f == 1 && wait.state.s == 0 &&
              wait.state.t == 0 && wait.random_draws == 0 &&
              !wait.confirm_consumed && !wait.menu_confirm_ready,
          "98无确认只推进两个标题计数，不生人");

    // 负raw验证Java余数后abs和100→11→2→100顺序，端点均手写。
    const auto tape = Random::from_raw({-99, -21, -3, -199});
    const auto gate_skip = prepare_startup_title_update(before_gate, tape, update(true));
    const auto &skip = accepted(check, gate_skip);
    const auto &born = skip.state.slots[0];
    check(skip.state.l == 100 && skip.confirm_consumed && !skip.menu_confirm_ready &&
              skip.random_draws == 4 && skip.random.draws() == 4 && skip.state.s == 70 &&
              skip.state.t == 0 && born.active == 1 && born.definition == 10 &&
              born.x == 0 && born.y == 217 && born.direction == 1 && born.age == 0,
          "98确认仅跳动画，首人同轮不移动，四抽顺序及上端点");
    check(before_gate.l == 98 && before_gate.slots[0].active == 0 && tape.draws() == 0,
          "成功返回候选仍不提交输入状态／随机");
    before_gate.l = 99;
    const auto gate_menu = prepare_startup_title_update(before_gate, tape, update(true));
    const auto &menu = accepted(check, gate_menu);
    check(menu.state.l == 100 && !menu.confirm_consumed && menu.menu_confirm_ready,
          "99自然到100保留确认给菜单，不代开菜单");

    for (std::size_t length = 0; length < 4; ++length) {
        const std::vector<std::int32_t> values{-99, -21, -3, -199};
        const auto truncated = Random::from_raw(
            std::vector<std::int32_t>(values.begin(), values.begin() + length));
        const auto saved_state = before_gate;
        const auto saved_random = truncated;
        const auto failure = prepare_startup_title_update(before_gate, truncated, update(true));
        const std::array<std::string, 4> errors{"title.random.interval", "title.random.definition",
                                               "title.random.direction", "title.random.height"};
        check(!failure.candidate && failure.error == errors[length] &&
                  same(before_gate, saved_state) && same_random(truncated, saved_random),
              "第" + std::to_string(length + 1) + "抽耗尽全部回滚且不吞确认");
    }

    auto walking = skip.state;
    walking.t = 0;
    const auto walk_result = prepare_startup_title_update(walking, empty, update());
    const auto &walk = accepted(check, walk_result);
    check(walk.random_draws == 0 && walk.state.t == 1 &&
              walk.state.slots[0].x == 1 && walk.state.slots[0].age == 1,
          "未到间隔先移动旧人且零抽");

    auto full = walking;
    full.s = 20;
    full.t = 19;
    for (auto &slot : full.slots)
        slot = {1, 2, 100, 210, 1, 4};
    const auto one = Random::from_raw({0});
    const auto full_result = prepare_startup_title_update(full, one, update());
    const auto &all = accepted(check, full_result);
    check(all.random_draws == 1 && all.state.s == 20 && all.state.t == 0 &&
              all.state.slots[19].x == 101 && all.state.slots[19].age == 5,
          "满20槽是条件分支，只抽间隔并保留移动");

    auto retiring = full;
    retiring.slots[0] = {1, 7, 250, 216, 1, 39};
    retiring.slots[1] = {1, 3, -10, 215, 0, 9};
    retiring.slots[2].active = 0;
    retiring.slots[2].age = 91;
    const auto zeros = Random::from_raw({0, 0, 0, 0});
    const auto reuse_result = prepare_startup_title_update(retiring, zeros, update());
    const auto &reuse = accepted(check, reuse_result);
    const auto &reused = reuse.state.slots[0];
    check(reused.active == 1 && reused.definition == 0 && reused.direction == 0 &&
              reused.x == 240 && reused.y == 210 && reused.age == 40 &&
              reuse.state.slots[1].active == 0 && reuse.state.slots[1].x == -11 &&
              reuse.state.slots[1].y == 215 && reuse.state.slots[1].age == 10 &&
              reuse.state.slots[2].active == 0 && reuse.state.slots[2].age == 91,
          "先退休再复用最小槽，继承age；其他退休槽保留y及age");
    auto edge = full;
    edge.t = 0;
    edge.slots[0].x = 249;
    edge.slots[1] = {1, 0, -9, 210, 0, 0};
    const auto edge_result = prepare_startup_title_update(edge, empty, update());
    const auto &edges = accepted(check, edge_result);
    check(edges.state.slots[0].active == 1 && edges.state.slots[0].x == 250 &&
              edges.state.slots[1].active == 1 && edges.state.slots[1].x == -10,
          "250和负10仍活跃");
    edge.l = edge.f132f = edge.slots[0].age = 2147483646;
    const auto max_result = prepare_startup_title_update(edge, empty, update());
    const auto &maximum = accepted(check, max_result);
    check(maximum.state.l == 2147483646 && maximum.state.f132f == 0 &&
              maximum.state.slots[0].age == 0,
          "l饱和，f132f和age回卷，均无signed overflow");

    auto sorted = initial;
    for (auto &slot : sorted.slots)
        slot.y = 217;
    sorted.slots[0] = {1, 4, 12, 212, 1, 19};
    sorted.slots[1] = {1, 5, 34, 212, 0, 20};
    sorted.slots[2] = {1, 6, 56, 210, 1, 7};
    const auto saved_sorted = sorted;
    const auto projected = project_startup_title_presentation(sorted, 330);
    const auto repeated = project_startup_title_presentation(sorted, 330);
    check(projected.error.empty() && projected.people.size() == 3 &&
              projected.people[0].slot == 2 && projected.people[1].slot == 1 &&
              projected.people[2].slot == 0,
          "相等y交换反例为2,1,0，不能稳定排序");
    const auto &p = projected.people[2];
    check(p.definition == 4 && p.age == 19 && p.step == 3 && p.facing == 1 &&
              p.anchor == std::array<int, 2>{12, 302} &&
              p.layers[0] == StartupTitlePersonLayer::shadow &&
              p.layers[1] == StartupTitlePersonLayer::body &&
              projected.people[1].step == 0 && projected.people[1].facing == 2,
          "只投影definition、步帧、朝向、纵向偏移及shadow→body顺序");
    check(same(sorted, saved_sorted) && repeated.people.size() == 3 &&
              repeated.people[2].slot == 0 && tape.draws() == 0,
          "重复投影不改状态且无随机访问");
    sorted.slots[2].active = 0;
    const auto inactive_sort = project_startup_title_presentation(sorted, 240);
    check(inactive_sort.error.empty() && inactive_sort.people.size() == 2 &&
              inactive_sort.people[0].slot == 1 && inactive_sort.people[1].slot == 0,
          "inactive低y也先参与排序，之后才过滤");
    check(project_startup_title_presentation(initial, 0).error == "title.surface_height" &&
              project_startup_title_presentation(sorted, std::numeric_limits<int>::max()).error.empty(),
          "坏表面高度拒绝；最大正高度安全相加");

    const auto reject = [&](StartupTitlePresentation bad, const std::string &error) {
        const auto saved = bad;
        const auto result = prepare_startup_title_update(bad, tape, update());
        check(!result.candidate && result.error == error && same(bad, saved) && tape.draws() == 0 &&
                  project_startup_title_presentation(bad, 240).error == error,
              "坏字段显式拒绝且无部分修改：" + error);
    };
    auto bad = initial;
    bad.l = -1; reject(bad, "title.counter");
    bad = initial; bad.f132f = std::numeric_limits<int>::max(); reject(bad, "title.counter");
    bad = initial; bad.s = 19; reject(bad, "title.spawn_interval");
    bad = initial; bad.s = 71; reject(bad, "title.spawn_interval");
    bad = initial; bad.t = 1; reject(bad, "title.spawn_elapsed");
    bad = initial; bad.s = 20; bad.t = 20; reject(bad, "title.spawn_elapsed");
    bad = initial; bad.slots[19].active = 2; reject(bad, "title.slot[19].active");
    bad = initial; bad.slots[19].definition = 11; reject(bad, "title.slot[19].definition");
    bad = initial; bad.slots[19].direction = -1; reject(bad, "title.slot[19].direction");
    bad = initial; bad.slots[19].age = std::numeric_limits<int>::max(); reject(bad, "title.slot[19].age");
    bad = initial; bad.slots[19].x = 252; reject(bad, "title.slot[19].x");
    bad = initial; bad.slots[19].y = 209; reject(bad, "title.slot[19].y");
    bad = initial; bad.slots[19] = {1, 0, 251, 210, 0, 0}; reject(bad, "title.slot[19].x");
    bad = initial; bad.slots[19] = {1, 0, 0, 0, 0, 0}; reject(bad, "title.slot[19].y");
    const auto rejected = prepare_startup_title_update(
        initial, tape, {static_cast<StartupTitleAdmission>(42), true});
    check(!rejected.candidate && rejected.error == "title.admission" && tape.draws() == 0,
          "错误框架准入拒绝，不隐式Update");
    return check.count;
}
