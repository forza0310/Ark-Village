#include "dungeon_village_reference/world_overlap.hpp"
#include "dungeon_village_reference/world_random_consumers.hpp"
#include <iostream>
#include <stdexcept>
using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool yes, const char *what) {
    ++checks;
    if (!yes)
        throw std::runtime_error(what);
}
} // namespace
int main() {
    try {
        for (bool table : {false, true})
            for (int type = 0; type < 19; ++type) {
                auto random = WorldRandomStream::from_raw({0, -7});
                const auto r = prepare_world_random_expression(random, {}, type, 0, table);
                check(r.ticket && r.candidate,
                      "every original expression produces valid candidate");
                check(random.draws() == (type == 6 ? 1U : 2U) &&
                          r.ticket->variant_count ==
                              reference_world_expression_variants(type, table),
                      "probability0 suppresses variant but all guaranteed/zero ticket expressions "
                      "draw");
                auto suppressed = WorldRandomStream::from_raw({0});
                const auto hidden =
                    prepare_world_random_expression(suppressed, {{{24, 0}}, {}}, type, 0, table);
                check(hidden.ticket && !hidden.candidate->consumed_variant &&
                          suppressed.draws() == 1,
                      "existing award label suppresses all expression variants after probability "
                      "draw");
                auto exhausted = WorldRandomStream::from_raw({0});
                const auto failure = prepare_world_random_expression(exhausted, {}, type, 0, table);
                check(type == 6 ? failure.ticket.has_value()
                                : (!failure.ticket &&
                                   failure.random_error == WorldRandomError::exhausted &&
                                   exhausted.draws() == 0),
                      "actual variant exhaustion restores helper's private cursor");
            }
        check(reference_world_expression_variants(15, true) == 3 &&
                  reference_world_expression_variants(15, false) == 1 &&
                  reference_world_expression_variants(16, true) == 2 &&
                  reference_world_expression_variants(16, false) == 1,
              "two original tables differ at15/16, no screenshot-language guess");
        WorldOverlapInput overlap;
        WorldOverlapActor h;
        h.id = {1};
        h.state = 1;
        h.cell = {0, 10};
        WorldOverlapActor m = h;
        m.id = {2};
        m.cell = {20, 20};
        m.decision_area = true;
        overlap.humans = {h};
        overlap.monsters = {m};
        auto random = WorldRandomStream::from_raw({-3});
        overlap.draw = [&](int bound) -> std::optional<int> {
            check(bound == 2, "overlap draw uses source bound2");
            const auto r = random.draw(bound);
            return r.error == WorldRandomError::none ? std::optional<int>(r.ticket) : std::nullopt;
        };
        const auto separated = prepare_world_overlap(overlap);
        check(separated.candidate && random.draws() == 1 &&
                  !separated.candidate->attempts.front().adjacent,
              "overlap consumes actual pair draw before nonadjacent guard");
        overlap.monsters.front().state = 14;
        random = WorldRandomStream::from_raw({});
        check(prepare_world_overlap(overlap).candidate && random.draws() == 0,
              "excluded state does not consume an overlap pair draw");
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
