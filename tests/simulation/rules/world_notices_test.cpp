#include "ark/simulation/rules/world_notices.hpp"

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
void queue_and_display() {
    std::vector<WorldScriptNotice> input{
        {2, 0, 80, {}, "first"}, {3, -1, 80, {}, "second"}, {4, -10, 80, {}, "queued"}};
    auto r = prepare_world_notices(input);
    check(r && r->notices[0].counter == 1 && r->notices[1].counter == 0 &&
              r->notices[2].counter == -10 && r->sounds == std::vector<int>{11} &&
              input[0].counter == 0,
          "only original first two advance, delayed zero is silent, input stays unchanged");
    auto plan = world_notice_placements(r->notices);
    check(plan && plan->size() == 1 && plan->front().index == 0 && plan->front().height == 2 &&
              plan->front().offset == -2,
          "first entry has source21 height and integer8 entrance");
    r = prepare_world_notices(r->notices);
    check(r && r->sounds == std::vector<int>{11} && r->notices[1].counter == 1,
          "second message sounds only when its own counter reaches1");
    input[0].counter = input[1].counter = 8;
    plan = world_notice_placements(input);
    check(plan && plan->size() == 2 && (*plan)[0].index == 1 && (*plan)[0].height == 19 &&
              (*plan)[0].offset == -40 && (*plan)[1].index == 0 && (*plan)[1].height == 21 &&
              (*plan)[1].offset == -21,
          "display is reverse source index1 then0 with distinct heights");
    for (int count = 8; count <= 96; ++count) {
        input[0].counter = count;
        input[1].counter = -1;
        plan = world_notice_placements(input);
        const int expected = count < 88 ? 21 : 21 - (count - 88) * 21 / 8;
        check(plan && (expected == 0 ? plan->empty()
                                     : plan->size() == 1 && plan->front().height == expected),
              "80 hold and8 exit use exact integer boundaries");
    }
    input[0].counter = input[1].counter = 96;
    r = prepare_world_notices(input);
    check(r && r->notices.size() == 1 && r->notices.front().message == 4 &&
              r->notices.front().counter == -10 && r->sounds.empty(),
          "reverse removal retires both without progressing newly exposed third in same call");
    r = prepare_world_notices(r->notices);
    check(r && r->notices.front().counter == -9,
          "queued message starts its countdown only on next qualified tail");
    input = {{0, 1, 80, {}, "growth"}};
    r = prepare_world_notices(input);
    check(r && r->sounds.empty() && r->notices.front().counter == 2,
          "direct growthS initialized1 is not ordinary pending message sound");
    input.front().counter = std::numeric_limits<int>::max();
    check(!prepare_world_notices(input), "counter overflow rejects whole candidate");
    plan = world_notice_placements(input);
    check(plan && plan->empty(), "extreme expired counter cannot narrow into visible height");
    input.front().counter = 0;
    input.front().duration = std::numeric_limits<int>::max();
    check(!prepare_world_notices(input) && !world_notice_placements(input),
          "duration overflow rejects update and display");
}
} // namespace
int main() {
    try {
        queue_and_display();
        std::cout << "world notices checks: " << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
