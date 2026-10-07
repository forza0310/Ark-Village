# Product tests are organized by dependency/ownership. Frozen research tests remain in
# WorldSimulation.cmake; this module never changes their source paths or assertions.
include(CMakeParseArguments)
add_library(ark_test_support INTERFACE)
target_include_directories(ark_test_support INTERFACE "${PROJECT_SOURCE_DIR}/tests")

function(ark_test_executable target)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})
    add_executable(${target} ${ARG_SOURCES})
    target_link_libraries(${target} PRIVATE ark_test_support ${ARG_LIBRARIES})
    ark_target(${target})
endfunction()

function(ark_test_case name target)
    cmake_parse_arguments(ARG "" "TIMEOUT;WORKING_DIRECTORY;PASS" "ARGS;LABELS" ${ARGN})
    add_test(NAME ${name} COMMAND ${target} ${ARG_ARGS})
    set_tests_properties(${name} PROPERTIES LABELS "${ARG_LABELS}")
    if(ARG_TIMEOUT)
        set_tests_properties(${name} PROPERTIES TIMEOUT ${ARG_TIMEOUT})
    endif()
    if(ARG_WORKING_DIRECTORY)
        set_tests_properties(${name} PROPERTIES WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}")
    endif()
    if(ARG_PASS)
        set_tests_properties(${name} PROPERTIES PASS_REGULAR_EXPRESSION "${ARG_PASS}")
    endif()
endfunction()

# Current-world contracts. The worker retains its own process; the two pure command suites
# share a binary but CTest selects each in a fresh process, preserving independent fixtures.
ark_test_executable(ark_world_session_tests
    SOURCES tests/app/world_session_test.cpp tests/app/world_task_commands_test.cpp
        tests/app/world_building_commands_test.cpp tests/app/world_save_commands_test.cpp
        tests/app/world_management_commands_test.cpp
    LIBRARIES ark_world_session)
ark_test_case(world_session ark_world_session_tests LABELS runtime TIMEOUT 30)
ark_test_executable(ark_world_save_tests SOURCES tests/app/world_save_test.cpp
    tests/app/world_save_restore_test.cpp
    tests/app/world_save_codec_test.cpp
    LIBRARIES ark_world_save)
ark_test_case(world_save ark_world_save_tests LABELS runtime TIMEOUT 300)
ark_test_executable(ark_world_contract_tests
    SOURCES tests/app/world_contracts_main.cpp tests/app/world_report_test.cpp
        tests/app/world_medals_test.cpp
    LIBRARIES ark_world_session)
foreach(case IN ITEMS world_report world_medals)
    ark_test_case(${case} ark_world_contract_tests ARGS ${case} LABELS runtime TIMEOUT 30)
endforeach()
ark_test_executable(ark_world_facility_queries_tests
    SOURCES tests/app/world_facility_queries_test.cpp LIBRARIES ark_world_queries)
ark_test_case(world_facility_queries ark_world_facility_queries_tests LABELS runtime)
ark_test_executable(ark_launch_tests SOURCES tests/app/launch_options_test.cpp LIBRARIES ark_launch)
ark_test_case(launch_options ark_launch_tests LABELS runtime TIMEOUT 10)
ark_test_executable(ark_original_loop_tests SOURCES tests/app/original_loop_test.cpp LIBRARIES ark_timing)
ark_test_case(original_loop_pacing ark_original_loop_tests LABELS rules)

# Read-only presentation plans have no raylib dependency and run in all four configurations.
foreach(module IN ITEMS world_combat_visuals world_rest_visuals world_dungeon_visuals script_text)
    ark_test_executable(ark_${module}_tests
        SOURCES tests/desktop/${module}_test.cpp LIBRARIES ark_world_visuals)
    ark_test_case(${module} ark_${module}_tests LABELS presentation)
endforeach()
ark_test_executable(ark_metadata_tests SOURCES tests/assets/metadata_test.cpp LIBRARIES ark_asset_metadata)
ark_test_case(asset_metadata ark_metadata_tests LABELS rules)
ark_test_case(startup_data_contract "${ARK_NODE}"
    ARGS "${PROJECT_SOURCE_DIR}/tests/data/startup_data_test.mjs" "${PROJECT_SOURCE_DIR}/assets/data"
    LABELS provenance legacy)
# Build-time glyph demand has independent Unicode/discovery/rejection contracts;
# provenance tests only verify existing asset bytes. No extra C++ executable is needed.
ark_test_case(desktop_glyph_inventory "${ARK_NODE}"
    ARGS "${PROJECT_SOURCE_DIR}/tests/data/desktop_glyphs_test.mjs"
    LABELS presentation provenance TIMEOUT 30)
ark_test_case(source_provenance "${ARK_NODE}"
    ARGS "${PROJECT_SOURCE_DIR}/scripts/verify_assets.mjs" "${PROJECT_SOURCE_DIR}/assets"
    LABELS provenance)
ark_test_case(world_simulation_cli ${CMAKE_COMMAND}
    ARGS "-DWORLD_SIMULATION=$<TARGET_FILE:ark_world_simulation>"
        -P "${PROJECT_SOURCE_DIR}/tests/integration/world_simulation_cli_test.cmake"
    LABELS e2e)

# Legacy still has a runnable --legacy-slice entry. Same-named imported tests exercise a
# different implementation; keep all of these contracts and their existing CTest names.
function(ark_legacy_test target name source)
    ark_test_executable(${target} SOURCES tests/legacy/${source} LIBRARIES ${ARGN})
    ark_test_case(${name} ${target} LABELS legacy)
endfunction()
ark_legacy_test(ark_random_tests java_random_stream random_test.cpp ark_game)
ark_legacy_test(ark_live_world_motion_tests live_world_paths_and_exit live_world_motion_test.cpp ark_game)
ark_legacy_test(ark_world_schedule_tests world_schedule_protocol world_schedule_test.cpp ark_game)
ark_legacy_test(ark_dungeon_task_tests dungeon_task_success dungeon_task_test.cpp ark_game)
ark_legacy_test(ark_village_life_tests normal_village_life village_life_test.cpp ark_game)
ark_legacy_test(ark_simulation_clock_tests source_paced_simulation simulation_clock_test.cpp ark_timing ark_game)
ark_legacy_test(ark_dungeon_tests dungeon_crew_and_completion dungeon_rules_test.cpp ark_game)
ark_legacy_test(ark_fixed_step_tests fixed_step_simulation fixed_step_clock_test.cpp ark_timing ark_game)
ark_legacy_test(ark_game_tests game_first_play game_test.cpp ark_game)
ark_legacy_test(ark_facility_tests facility_economy_neighbours facility_rules_test.cpp ark_game)
ark_legacy_test(ark_terrain_tests road_connections terrain_test.cpp ark_game)
ark_legacy_test(ark_navigation_tests loaded_map_navigation_motion navigation_motion_test.cpp ark_game)
ark_legacy_test(ark_activity_choice_tests activity_category_choice activity_choice_test.cpp ark_game)
ark_legacy_test(ark_activity_candidates_tests activity_candidates activity_candidates_test.cpp ark_game)
ark_legacy_test(ark_departure_tests facility_choice_departure departure_test.cpp ark_game)
ark_legacy_test(ark_actor_ai_tests actor_ai_rules actor_ai_test.cpp ark_game)
ark_legacy_test(ark_ai_perception_tests ai_perception_rules ai_perception_test.cpp ark_game)
ark_legacy_test(ark_actor_control_tests actor_control_prefix actor_control_test.cpp ark_game)
ark_legacy_test(ark_actor_housekeeping_tests actor_common_update actor_housekeeping_test.cpp ark_game)
ark_legacy_test(ark_combat_ai_tests combat_strategy_damage_influence combat_ai_test.cpp ark_game)
ark_legacy_test(ark_ai_update_tests ai_common_update_schedule ai_update_test.cpp ark_game)
ark_legacy_test(ark_ai_decision_tests ai_priority_departure_control ai_decision_test.cpp ark_game)
ark_legacy_test(ark_ai_schedule_tests ai_live_roster_schedule ai_schedule_test.cpp ark_game)
ark_legacy_test(ark_actor_effects_tests actor_effect_timeline actor_effects_test.cpp ark_game)
ark_legacy_test(ark_weapon_choice_tests equipment_candidate_choice weapon_choice_test.cpp ark_game)
ark_legacy_test(ark_human_growth_tests human_shared_growth human_growth_test.cpp ark_game)
ark_legacy_test(ark_people_progression_tests actor_schedule_equipment_growth people_progression_test.cpp ark_game)
ark_legacy_test(ark_facility_arrival_tests facility_arrival_statistics facility_arrival_test.cpp ark_game)
ark_legacy_test(ark_facility_exit_tests facility_exit_and_cash facility_exit_test.cpp ark_game)
ark_legacy_test(ark_facility_service_tests facility_service_sequence facility_service_test.cpp ark_game)
ark_legacy_test(ark_hp_tests character_hp_protocol character_hp_test.cpp ark_game)
ark_legacy_test(ark_initial_ai_tests actual_initial_ai_interval initial_ai_test.cpp ark_game)
ark_legacy_test(ark_game_ai_preview_tests game_visible_ai_preview game_ai_preview_test.cpp ark_game)

if(ARK_BUILD_DESKTOP)
    # Source portrait assertions keep their CPU image oracle and use the existing product
    # SEB/TSV parsers. No WindowServer is needed, but headless builds do not depend on raylib.
    ark_test_executable(ark_simulation_startup_world_visuals_test
        SOURCES tests/simulation/startup_world_visuals_test.cpp
        LIBRARIES ark_world_runtime ark_asset_metadata PkgConfig::RAYLIB)
    ark_test_case(simulation.startup_world_visuals_test ark_simulation_startup_world_visuals_test
        ARGS "${PROJECT_SOURCE_DIR}/assets" LABELS presentation frozen TIMEOUT 120)
    # World UI suites use the same skin/resource/layout lifetime and dependencies.
    # Their bodies remain separate files and each CTest invocation selects one named case.
    ark_test_executable(ark_world_ui_tests
        SOURCES tests/desktop/world_ui_main.cpp tests/desktop/world_panels_test.cpp
            tests/desktop/world_award_ui_test.cpp tests/desktop/world_crew_summary_test.cpp
            tests/desktop/world_tasks_test.cpp tests/desktop/world_menu_test.cpp
            tests/desktop/world_building_test.cpp tests/desktop/world_building_render_fixture.cpp
            src/desktop/ui/world_panels.cpp
            tests/desktop/world_human_test.cpp tests/desktop/world_human_render_fixture.cpp
            tests/desktop/world_combat_render_fixture.cpp
            tests/desktop/world_village_activity_test.cpp src/desktop/ui/world_village_activity.cpp
            tests/desktop/world_commerce_test.cpp tests/desktop/world_facility_items_test.cpp
            src/desktop/ui/world_commerce.cpp src/desktop/ui/world_facility_items.cpp
            src/desktop/ui/world_human.cpp src/desktop/ui/world_tax.cpp
            src/desktop/ui/world_award.cpp src/desktop/ui/world_crew_summary.cpp src/desktop/ui/world_reports.cpp
            src/desktop/ui/world_tasks.cpp src/desktop/ui/world_menu.cpp
            src/desktop/ui/world_building.cpp src/desktop/ui/world_progression.cpp
            src/desktop/world_build_placement.cpp src/desktop/world_human_inspection.cpp
            src/desktop/world_editing.cpp
            src/desktop/world_scene.cpp src/desktop/world_overlay_render.cpp
            src/desktop/world_rank.cpp src/desktop/character_status.cpp
            src/desktop/character_visibility.cpp
            src/desktop/world_task_inspection.cpp
            src/desktop/world_save_menu.cpp
        LIBRARIES ark_world_ui_test_support ark_world_queries ark_world_session)
    target_compile_definitions(ark_world_ui_tests PRIVATE ARK_TEST_ASSETS="${PROJECT_SOURCE_DIR}/assets"
        ARK_TEST_FONT="${ARK_DESKTOP_FONT}"
        ARK_TEST_OUTPUT="${PROJECT_SOURCE_DIR}/build/validation/human-management/fixtures")
    foreach(case IN ITEMS world_panels world_award_ui world_crew_summary world_tasks world_menu world_building world_human world_tax world_village_activity world_commerce world_facility_items)
        ark_test_case(${case} ark_world_ui_tests ARGS ${case} LABELS presentation)
    endforeach()
    ark_test_executable(ark_world_scene_tests
        SOURCES tests/desktop/world_scene_test.cpp src/desktop/world_rank.cpp
            src/desktop/world_overlay_render.cpp src/desktop/world_scene.cpp
            src/desktop/character_status.cpp src/desktop/character_visibility.cpp
        LIBRARIES ark_world_ui_test_support)
    ark_test_case(world_scene_projection_and_animation ark_world_scene_tests LABELS presentation)
    ark_test_case(complete_world_packaged_check ark_village ARGS --world --check LABELS e2e
        PASS "PASS packaged complete world")

    # Scene/picking contracts use their actual product implementations and asset inputs.
    function(ark_desktop_test target name)
        cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES;ARGS;LABELS" ${ARGN})
        ark_test_executable(${target} SOURCES ${ARG_SOURCES} LIBRARIES ${ARG_LIBRARIES})
        target_include_directories(${target} PRIVATE src/desktop)
        ark_test_case(${name} ${target} ARGS ${ARG_ARGS} LABELS presentation ${ARG_LABELS})
    endfunction()
    ark_desktop_test(ark_character_status_tests character_hp_render
        SOURCES tests/desktop/character_status_test.cpp src/desktop/character_status.cpp
            src/desktop/character_visibility.cpp LIBRARIES ark_game LABELS legacy)
    ark_desktop_test(ark_boundary_render_tests boundary_overlay_contract
        SOURCES tests/desktop/boundary_render_test.cpp src/desktop/boundary_render.cpp
        LIBRARIES ark_game ark_asset_metadata PkgConfig::RAYLIB ARGS "${PROJECT_SOURCE_DIR}/assets")
    ark_desktop_test(ark_road_render_tests road_patch_pixels
        SOURCES tests/desktop/road_render_test.cpp src/desktop/road_render.cpp
        LIBRARIES ark_game ark_asset_metadata PkgConfig::RAYLIB ARGS "${PROJECT_SOURCE_DIR}/assets")
    ark_desktop_test(ark_projection_tests projection_input
        SOURCES tests/desktop/projection_test.cpp src/desktop/projection.cpp
        LIBRARIES ark_game PkgConfig::RAYLIB)
    ark_desktop_test(ark_character_render_tests character_ground_and_animation
        SOURCES tests/desktop/character_render_test.cpp src/desktop/character_animation.cpp
            src/desktop/character_visibility.cpp src/desktop/projection.cpp
        LIBRARIES ark_game ark_asset_metadata PkgConfig::RAYLIB ARGS "${PROJECT_SOURCE_DIR}/assets")
    ark_desktop_test(ark_ui_tests ui_navigation
        SOURCES tests/desktop/ui_navigation_test.cpp src/desktop/projection.cpp
            src/desktop/ui/layout.cpp src/desktop/ui/controller.cpp src/desktop/ui/facility_context.cpp
        LIBRARIES ark_game PkgConfig::RAYLIB LABELS legacy)
    ark_test_case(packaged_assets ark_village ARGS --check LABELS e2e
        TIMEOUT 10 WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" PASS "PASS packaged complete world")
    ark_test_case(legacy_slice_packaged_check ark_village ARGS --legacy-slice --check LABELS e2e legacy
        PASS "PASS packaged source assets")
    ark_test_case(packaged_initial_ai ark_village ARGS --check-ai LABELS e2e legacy
        TIMEOUT 30 WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
    ark_test_case(packaged_frame_contract "${ARK_NODE}"
        ARGS "${PROJECT_SOURCE_DIR}/tests/integration/packaged_frames_test.mjs" "$<TARGET_FILE:ark_village>"
        LABELS e2e TIMEOUT 30)
endif()
