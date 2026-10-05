// Related current-world contracts share compilation, while CTest selects an isolated case.
#include "support/case_runner.hpp"

namespace ark::test {
void world_report();
void world_medals();
} // namespace ark::test

int main(int argc, char **argv) {
    return ark::test::run_case(
        argc, argv,
        {{"world_report", ark::test::world_report}, {"world_medals", ark::test::world_medals}});
}
