// Read-only world UI cases use common dependencies without sharing mutable fixture state.
#include "support/case_runner.hpp"

namespace ark::test {
void world_panels();
void world_award_ui();
void world_crew_summary();
void world_tasks();
void world_menu();
void world_building();
void world_human();
void world_tax();
void world_village_activity();
void world_human_render_fixture();
} // namespace ark::test

int main(int argc, char **argv) {
    return ark::test::run_case(
        argc, argv,
        {{"world_panels", ark::test::world_panels},
         {"world_award_ui", ark::test::world_award_ui},
         {"world_crew_summary", ark::test::world_crew_summary},
         {"world_tasks", ark::test::world_tasks},
         {"world_menu", ark::test::world_menu},
         {"world_building", ark::test::world_building},
         {"world_human", ark::test::world_human},
         {"world_tax", ark::test::world_tax},
         {"world_village_activity", ark::test::world_village_activity},
         {"world_human_render_fixture", ark::test::world_human_render_fixture}});
}
