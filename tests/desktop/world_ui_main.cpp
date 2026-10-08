// Read-only world UI cases use common dependencies without sharing mutable fixture state.
#include "support/case_runner.hpp"

namespace ark::test {
void world_panels();
void world_award_ui();
void world_award_render_fixture();
void world_crew_summary();
void world_tasks();
void world_menu();
void world_building();
void world_human();
void world_tax();
void world_village_activity();
void world_commerce();
void world_facility_items();
void world_human_render_fixture();
void render_world_edit_fixture();
void world_facility_static_render_fixture();
void world_combat_render_fixture();
void world_business_render_fixture();
void world_business_restore_fixture();
} // namespace ark::test

int main(int argc, char **argv) {
    return ark::test::run_case(
        argc, argv,
        {{"world_panels", ark::test::world_panels},
         {"world_award_ui", ark::test::world_award_ui},
         {"world_award_render_fixture", ark::test::world_award_render_fixture},
         {"world_crew_summary", ark::test::world_crew_summary},
         {"world_tasks", ark::test::world_tasks},
         {"world_menu", ark::test::world_menu},
         {"world_building", ark::test::world_building},
         {"world_human", ark::test::world_human},
         {"world_tax", ark::test::world_tax},
         {"world_village_activity", ark::test::world_village_activity},
         {"world_commerce", ark::test::world_commerce},
         {"world_facility_items", ark::test::world_facility_items},
         {"world_human_render_fixture", ark::test::world_human_render_fixture},
         {"world_edit_render_fixture", ark::test::render_world_edit_fixture},
         {"world_facility_static_render_fixture", ark::test::world_facility_static_render_fixture},
         {"world_combat_render_fixture", ark::test::world_combat_render_fixture},
         {"world_business_render_fixture", ark::test::world_business_render_fixture},
         {"world_business_restore_fixture", ark::test::world_business_restore_fixture}});
}
