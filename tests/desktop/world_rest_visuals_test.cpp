// Explicit display fixtures verify the maintained inn contract, not a natural startup trace.
#include "support/world_fixture.hpp"
#include "world_rest_visuals.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
namespace sim = ark::simulation;
namespace rules = sim::rules;
using namespace ark::desktop;
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
sim::StartupWorldRuntimeState fixture() {
    auto state = ark::test::initial_world();
    auto &world = state.scene.world.world;
    world.facilities.clear();
    world.ai.battle.actors.clear();
    world.ai.retired_actors.clear();
    rules::RescueFacility inn;
    inn.category = 2;
    inn.occupants = {{1}};
    world.facilities.emplace(7, inn);
    for (std::uint64_t i = 1; i <= 5; ++i) {
        rules::BattleActorRecord actor;
        actor.id = {i};
        actor.control.flags = 1U | 32U; // Hidden inside a facility; its row remains visible.
        actor.definition = static_cast<int>(i - 1);
        actor.capacity = 901; // Deliberately stale actor cache; the original UI reads shared h().
        auto &growth = world.ai.growth.at(actor.definition);
        growth.derived.combat[0] = 22;
        growth.definition.current_profession = 0;
        actor.hp.displayed = 11;
        actor.hp.target = 22;
        world.ai.battle.actors.emplace(actor.id, actor);
    }
    return state;
}
float colored_width(const OverlayPlan &plan, std::array<unsigned char, 3> rgb) {
    float width{};
    for (const auto &command : plan)
        if (const auto *rectangle = std::get_if<OverlayRectangle>(&command);
            rectangle && rectangle->rgb == rgb)
            width += rectangle->width;
    return width;
}
std::string numbers(const OverlayPlan &plan) {
    std::string result;
    for (const auto &command : plan)
        if (const auto *sprite = std::get_if<OverlaySprite>(&command)) {
            check(sprite->name == "number11.seb" && sprite->frame >= 0 && sprite->frame <= 9,
                  "Inn capacity uses the published number11 digits");
            result += static_cast<char>('0' + sprite->frame);
        }
    return result;
}
OverlayPortrait portrait(const OverlayPlan &plan) {
    const OverlayPortrait *face{};
    int count{};
    for (const auto &command : plan)
        if (const auto *current = std::get_if<OverlayPortrait>(&command)) {
            face = current;
            ++count;
        }
    check(count == 1 && face, "Each visible rest row contains exactly one current-job portrait");
    return *face;
}
void portraits() {
    auto state = fixture();
    auto &world = state.scene.world.world;
    auto &actor = world.ai.battle.actors.at({1});
    auto &growth = world.ai.growth.at(actor.definition);
    const int sex = state.rules->humans.at(actor.definition).sex;
    actor.state_counter = 169;
    const auto waiting = portrait(world_rest_rows(state, 7).front().plan);
    actor.state_counter = 170;
    const auto healing = portrait(world_rest_rows(state, 7).front().plan);
    check(waiting.image == state.rules->jobs.at(0).sprites.at(sex) &&
              healing.image == waiting.image && waiting.source == healing.source &&
              waiting.destination == healing.destination &&
              waiting.source == std::array<float, 4>{1, 27, 15, 14} &&
              waiting.destination == std::array<float, 4>{1, 1, 15, 14},
          "Waiting and healing use the same walk01 frame-zero PNG crop at the left panel inset");
    // Changing shared profession must bypass any actor birth-time rendering metadata.
    state.actor_metadata[actor.id].profession = 0;
    growth.definition.current_profession = 1;
    const auto changed = portrait(world_rest_rows(state, 7).front().plan);
    check(changed.image == state.rules->jobs.at(1).sprites.at(sex) &&
              changed.source == healing.source && changed.destination == healing.destination &&
              state.actor_metadata.at(actor.id).profession == 0,
          "A current profession change resolves the real sex-specific body while retaining crop "
          "geometry");
    world.facilities.at(7).occupants = {{1}, {2}, {1}};
    world.ai.battle.actors.at({2}).control.flags &= ~32U;
    const auto rows = world_rest_rows(state, 7);
    const auto upper = portrait(rows.at(1).plan);
    check(upper.image == changed.image && upper.source == changed.source &&
              upper.destination == std::array<float, 4>{1, -19, 15, 14},
          "Portraits follow compact visible rows, preserving duplicate occupant identities");
    const auto before = state;
    (void)world_rest_rows(state, 7);
    (void)world_rest_rows(state, 7);
    check(
        growth.definition.current_profession ==
                before.scene.world.world.ai.growth.at(actor.definition)
                    .definition.current_profession &&
            growth.derived.combat ==
                before.scene.world.world.ai.growth.at(actor.definition).derived.combat &&
            state.actor_metadata.at(actor.id).profession ==
                before.actor_metadata.at(actor.id).profession &&
            actor.capacity == before.scene.world.world.ai.battle.actors.at(actor.id).capacity &&
            state.scene.random.draws() == before.scene.random.draws() &&
            ark::test::same_world_clock(state, before),
        "Portrait projection does not refresh shared stats, actor caches, random or source timing");
}
void phases() {
    auto state = fixture();
    auto &actor = state.scene.world.world.ai.battle.actors.at({1});
    auto &capacity = state.scene.world.world.ai.growth.at(actor.definition).derived.combat[0];
    for (const int counter : {0, 169, 170, 199}) {
        actor.state_counter = counter;
        const auto rows = world_rest_rows(state, 7);
        check(rows.size() == 1 && rows.front().actor == actor.id && rows.front().counter == counter,
              "Hidden facility occupant retains its real counter and identity");
        const auto &plan = rows.front().plan;
        const auto &image = std::get<OverlayImage>(plan.front());
        check(image.name == "restBar00.png" && image.source[2] == (counter < 170 ? 48 : 17) &&
                  image.source[3] == 17,
              "B170 replaces the complete rest image with the portrait base");
        if (counter < 170) {
            check(numbers(plan).empty() &&
                      colored_width(plan, {255, 128, 192}) == (counter == 0 ? 0 : 29),
                  "The first phase maps the actor counter to30 without showing HP digits");
        } else {
            check(numbers(plan) == "22" && colored_width(plan, {255, 128, 192}) == 0 &&
                      colored_width(plan, {83, 255, 0}) == 13 &&
                      colored_width(plan, {68, 100, 104}) == 13,
                  "The second phase displays shared capacity and displayed HP, not actor "
                  "cache/target HP/time");
            bool frame{}, inset{};
            for (const auto &command : plan)
                if (const auto *rectangle = std::get_if<OverlayRectangle>(&command)) {
                    frame |= rectangle->width == 29 && rectangle->height == 5;
                    inset |= rectangle->width == 27 && rectangle->height == 3;
                }
            check(frame && inset, "Inn HP uses its own29x5 frame and27x3 inset");
        }
    }
    actor.hp.animating = false;
    actor.hp.displayed = -50;
    auto plan = world_rest_rows(state, 7).front().plan;
    check(colored_width(plan, {83, 255, 0}) == 0 && colored_width(plan, {68, 100, 104}) == 26,
          "Negative displayed HP is safely clamped without an HP animation eligibility gate");
    capacity = std::numeric_limits<int>::max();
    actor.hp.displayed = std::numeric_limits<int>::max();
    plan = world_rest_rows(state, 7).front().plan;
    check(colored_width(plan, {83, 255, 0}) == 26 && colored_width(plan, {68, 100, 104}) == 0,
          "Large capacity uses a safe width mapping without integer multiplication overflow");
    capacity = 1;
    plan = world_rest_rows(state, 7).front().plan;
    check(colored_width(plan, {83, 255, 0}) == 26, "Over-capacity display saturates visually");
    capacity = 0;
    bool rejected{};
    try {
        world_rest_rows(state, 7);
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected, "An invalid visible HP capacity is explicit, not divided by zero");
}
void eligibility() {
    auto state = fixture();
    auto &world = state.scene.world.world;
    auto &inn = world.facilities.at(7);
    inn.occupants = {{1}, {2}, {3}, {1}, {5}};
    world.ai.battle.actors.at({2}).control.flags = 1U;
    auto rows = world_rest_rows(state, 7);
    check(rows.size() == 3 && rows[0].actor == rules::CharacterId{1} &&
              rows[1].actor == rules::CharacterId{3} && rows[2].actor == rules::CharacterId{1},
          "First-four truncation precedes eligibility; source order and duplicates are retained");
    for (std::size_t i = 0; i < rows.size(); ++i)
        check(std::get<OverlayImage>(rows[i].plan.front()).destination[1] == -20.F * i,
              "Eligible rows stack compactly, without gaps for skipped occupants");
    inn.category = 9;
    check(world_rest_rows(state, 7).empty(), "Residential display does not inherit inn behavior");
    inn.category = 2;
    inn.occupants.clear();
    check(world_rest_rows(state, 7).empty(), "Cleared occupancy immediately removes inn status");
    inn.occupants = {{1}};
    world.ai.battle.actors.at({1}).control.flags &= ~32U;
    check(world_rest_rows(state, 7).empty(), "A non-resting occupant is not a visible inn row");
}
void readonly() {
    auto state = fixture();
    auto &world = state.scene.world.world;
    auto &actor = world.ai.battle.actors.at({1});
    actor.state_counter = 180;
    const auto before = state;
    world_rest_rows(state, 7);
    world_rest_rows(state, 7);
    const auto &old_actor = before.scene.world.world.ai.battle.actors.at({1});
    check(actor.state_counter == old_actor.state_counter &&
              actor.control.flags == old_actor.control.flags &&
              actor.capacity == old_actor.capacity && actor.definition == old_actor.definition &&
              world.ai.growth.at(actor.definition).definition.current_profession ==
                  before.scene.world.world.ai.growth.at(actor.definition)
                      .definition.current_profession &&
              world.ai.growth.at(actor.definition).derived.combat ==
                  before.scene.world.world.ai.growth.at(actor.definition).derived.combat &&
              actor.hp.displayed == old_actor.hp.displayed &&
              actor.hp.target == old_actor.hp.target &&
              actor.hp.legacy_tick == old_actor.hp.legacy_tick &&
              world.facilities.at(7).occupants ==
                  before.scene.world.world.facilities.at(7).occupants &&
              world.ai.accounting.funds() == before.scene.world.world.ai.accounting.funds() &&
              state.scene.random.draws() == before.scene.random.draws() &&
              ark::test::same_world_clock(state, before),
          "Repeated rendering never advances rest/HP or writes occupancy, money, RNG or ticks");
}
void retained_occupant() {
    auto state = fixture();
    auto &world = state.scene.world.world;
    auto retired = world.ai.battle.actors.at({1});
    retired.state_counter = 169;
    world.ai.retired_actors.emplace(retired.id, retired);
    world.ai.battle.actors.erase(retired.id);
    auto rows = world_rest_rows(state, 7);
    check(
        rows.size() == 1 && rows.front().actor == retired.id && rows.front().counter == 169 &&
            colored_width(rows.front().plan, {255, 128, 192}) == 29 &&
            world.ai.retired_actors.at(retired.id).control.flags == retired.control.flags,
        "A retained facility reference displays the original retired actor without changing flags");
    const auto face = portrait(rows.front().plan);
    check(face.image == state.rules->jobs.at(0).sprites.at(
                            state.rules->humans.at(retired.definition).sex) &&
              face.source == std::array<float, 4>{1, 27, 15, 14},
          "Retired occupants still resolve their current shared profession portrait");
    world.ai.retired_actors.at(retired.id).state_counter = 180;
    rows = world_rest_rows(state, 7);
    check(numbers(rows.front().plan) == "22" &&
              colored_width(rows.front().plan, {83, 255, 0}) == 13,
          "Retained actor HP phase reads current shared capacity and retained displayed HP");
    world.ai.retired_actors.at(retired.id).control.flags &= ~32U;
    check(world_rest_rows(state, 7).empty(),
          "A retained actor without the actual rest flag has no inn row");
    world.ai.retired_actors.erase(retired.id);
    bool rejected{};
    try {
        world_rest_rows(state, 7);
    } catch (const std::out_of_range &) {
        rejected = true;
    }
    check(rejected, "A reference missing from both live and retained owners remains an error");
}
} // namespace
int main() {
    phases();
    portraits();
    eligibility();
    readonly();
    retained_occupant();
    std::cout << "PASS inn rest visuals " << checks << " checks\n";
}
