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
# These explicit players have different horizons and process lifetimes from fast worker tests.
# Quick boundary cases are standard; real economy and first-star routes are opt-in long runs.
ark_test_executable(ark_world_campaign_tests SOURCES tests/app/world_campaign_test.cpp
    tests/app/world_active_strategy.cpp tests/app/world_active_late_strategy.cpp
    tests/app/world_active_pot_strategy.cpp
    tests/app/world_campaign_diagnostics.cpp
    tests/app/world_campaign_coverage.cpp
    tests/app/world_steam_layout.cpp tests/app/world_steam_people.cpp tests/app/world_steam_strategy.cpp
    tests/app/world_active_income_strategy.cpp
    LIBRARIES ark_world_session ark_world_hash ark_world_persistence)
target_include_directories(ark_world_campaign_tests PRIVATE src/app)
ark_test_case(player_active_campaign_contract ark_world_campaign_tests ARGS --contract
    LABELS runtime player TIMEOUT 30)
ark_test_executable(ark_world_economy_tests SOURCES tests/app/world_economy_test.cpp
    LIBRARIES ark_world_runtime)
ark_test_case(player_construction_economy_contract ark_world_economy_tests ARGS --contract
    LABELS runtime player TIMEOUT 30)
set(ARK_PLAYER_FIRST_STAR_PREFIX "" CACHE PATH
    "Verified first-star campaign evidence directory for explicit second-star acceptance")
if(ARK_LONG_WORLD_TESTS)
    ark_test_case(player_active_first_star "${ARK_NODE}"
        ARGS "${PROJECT_SOURCE_DIR}/tests/app/world_campaign_process.mjs"
            "$<TARGET_FILE:ark_world_campaign_tests>" "${CMAKE_CURRENT_BINARY_DIR}/campaign-validation"
        LABELS e2e player long_world TIMEOUT 12060)
    ark_test_case(player_construction_economy "${ARK_NODE}"
        ARGS "${PROJECT_SOURCE_DIR}/tests/app/world_economy_process.mjs"
            "$<TARGET_FILE:ark_world_economy_tests>" "${CMAKE_CURRENT_BINARY_DIR}/economy-validation"
        LABELS e2e player long_world TIMEOUT 1860)
    set_tests_properties(player_active_first_star player_construction_economy
        PROPERTIES RUN_SERIAL TRUE)
    if(ARK_PLAYER_FIRST_STAR_PREFIX)
        ark_test_case(player_active_second_star "${ARK_NODE}"
            ARGS "${PROJECT_SOURCE_DIR}/tests/app/world_campaign_process.mjs"
                "$<TARGET_FILE:ark_world_campaign_tests>" "${CMAKE_CURRENT_BINARY_DIR}/campaign-validation"
                --first-star-prefix "${ARK_PLAYER_FIRST_STAR_PREFIX}"
            LABELS e2e player long_world TIMEOUT 18060)
        set_tests_properties(player_active_second_star PROPERTIES RUN_SERIAL TRUE)
    endif()
endif()
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
# Audio device policy owns a separate fake-device lifetime, without raylib or a window.
ark_test_executable(ark_world_audio_tests SOURCES tests/desktop/world_audio_test.cpp)
target_include_directories(ark_world_audio_tests PRIVATE include src/desktop)
ark_test_case(world_audio ark_world_audio_tests LABELS presentation)

# Native crash hooks require isolated child processes, unlike the in-process UI suites.
if(WIN32)
    ark_test_executable(ark_crash_report_tests
        SOURCES tests/desktop/crash_report_test.cpp src/desktop/platform/crash_report.cpp
            src/desktop/platform/world_diagnostics.cpp
        LIBRARIES ark_world_session)
    target_include_directories(ark_crash_report_tests PRIVATE src/desktop)
    ark_test_case(windows_crash_report ark_crash_report_tests LABELS platform TIMEOUT 60)
endif()

foreach(module IN ITEMS world_combat_visuals world_rest_visuals world_dungeon_visuals script_text)
    ark_test_executable(ark_${module}_tests
        SOURCES tests/desktop/${module}_test.cpp LIBRARIES ark_world_visuals)
    ark_test_case(${module} ark_${module}_tests LABELS presentation)
endforeach()
ark_test_executable(ark_metadata_tests SOURCES tests/assets/metadata_test.cpp LIBRARIES ark_asset_metadata)
ark_test_case(asset_metadata ark_metadata_tests LABELS rules)
# Build-time glyph demand has independent Unicode/discovery/rejection contracts;
# provenance tests only verify existing asset bytes. No extra C++ executable is needed.
ark_test_case(desktop_glyph_inventory "${ARK_NODE}"
    ARGS "${PROJECT_SOURCE_DIR}/tests/data/desktop_glyphs_test.mjs"
    LABELS presentation provenance TIMEOUT 30)
ark_test_case(shared_library_contract "${ARK_NODE}"
    ARGS "${PROJECT_SOURCE_DIR}/tests/integration/shared_library_contract_test.mjs" "${CMAKE_CXX_COMPILER}"
    LABELS e2e build TIMEOUT 60)
ark_test_case(world_simulation_cli ${CMAKE_COMMAND}
    ARGS "-DWORLD_SIMULATION=$<TARGET_FILE:ark_world_simulation>"
        -P "${PROJECT_SOURCE_DIR}/tests/integration/world_simulation_cli_test.cmake"
    LABELS e2e)

if(ARK_BUILD_DESKTOP)
    # Source portrait assertions keep their CPU image oracle and use the existing product
    # SEB/TSV parsers. No WindowServer is needed, but headless builds do not depend on raylib.
    ark_test_executable(ark_simulation_startup_world_visuals_test
        SOURCES tests/simulation/startup_world_visuals_test.cpp ${ARK_STARTUP_SKIN_TEST_SOURCES}
        # The frozen no-mutation oracle hashes the complete Owner through maintenance codec.
        LIBRARIES ark_world_runtime ark_world_persistence ark_world_hash ark_asset_metadata PkgConfig::RAYLIB)
    ark_test_case(simulation.startup_world_visuals_test ark_simulation_startup_world_visuals_test
        ARGS "${PROJECT_SOURCE_DIR}/assets" LABELS presentation frozen TIMEOUT 120)
    # World UI suites use the same skin/resource/layout lifetime and dependencies.
    # Their bodies remain separate files and each CTest invocation selects one named case.
    ark_test_executable(ark_world_ui_tests
        SOURCES tests/desktop/world_ui_main.cpp tests/desktop/world_panels_test.cpp
            tests/desktop/world_award_ui_test.cpp tests/desktop/world_crew_summary_test.cpp
            tests/desktop/world_tasks_test.cpp tests/desktop/world_menu_test.cpp
            tests/desktop/world_building_test.cpp tests/desktop/world_building_render_fixture.cpp
            tests/desktop/world_business_render_fixture.cpp
            src/desktop/ui/common/world_panels.cpp
            tests/desktop/world_human_test.cpp tests/desktop/world_human_render_fixture.cpp
            tests/desktop/world_combat_render_fixture.cpp
            tests/desktop/world_village_activity_test.cpp src/desktop/ui/village/world_village_activity.cpp
            tests/desktop/world_commerce_test.cpp tests/desktop/world_facility_items_test.cpp
            src/desktop/ui/village/world_commerce.cpp src/desktop/ui/facilities/world_facility_items.cpp
            src/desktop/ui/facilities/world_facility_catalog.cpp
            src/desktop/ui/village/world_magic_pot.cpp
            src/desktop/ui/actors/world_human.cpp src/desktop/ui/actors/world_human_detail.cpp src/desktop/ui/village/world_tax.cpp
            src/desktop/ui/information/world_information.cpp
            src/desktop/ui/village/world_award.cpp src/desktop/ui/tasks/world_crew_summary.cpp src/desktop/ui/common/world_reports.cpp
            src/desktop/ui/tasks/world_tasks.cpp src/desktop/ui/system/world_menu.cpp
            src/desktop/ui/facilities/world_building_view.cpp
        src/desktop/ui/facilities/world_building_input.cpp src/desktop/ui/facilities/world_building_render.cpp src/desktop/ui/facilities/world_facility_upgrade.cpp src/desktop/ui/village/world_progression.cpp
            src/desktop/scene/world_build_placement.cpp src/desktop/inspection/world_human_inspection.cpp
            src/desktop/scene/world_editing.cpp
            src/desktop/scene/world_scene.cpp src/desktop/scene/world_overlay_render.cpp
            src/presentation/world_rank.cpp src/desktop/scene/character_status.cpp
            src/desktop/inspection/world_task_inspection.cpp
            src/desktop/input/world_save_menu.cpp src/desktop/application/world_title.cpp src/desktop/platform/world_audio.cpp src/desktop/ui/system/world_startup.cpp
        LIBRARIES ark_world_ui_test_support ark_world_queries ark_world_session)
    target_compile_definitions(ark_world_ui_tests PRIVATE ARK_TEST_ASSETS="${PROJECT_SOURCE_DIR}/assets"
        ARK_TEST_FONT="${ARK_DESKTOP_FONT}"
        ARK_TEST_OUTPUT="${PROJECT_SOURCE_DIR}/build/validation/human-management/fixtures")
    foreach(case IN ITEMS world_panels world_award_ui world_crew_summary world_tasks world_menu world_building world_human world_tax world_village_activity world_commerce world_facility_items)
        ark_test_case(${case} ark_world_ui_tests ARGS ${case} LABELS presentation)
    endforeach()
    ark_test_executable(ark_world_scene_tests
        SOURCES tests/desktop/world_scene_test.cpp src/presentation/world_rank.cpp
            src/desktop/scene/world_overlay_render.cpp src/desktop/scene/world_scene.cpp
            src/desktop/scene/character_status.cpp
        LIBRARIES ark_world_ui_test_support)
    ark_test_case(world_scene_projection_and_animation ark_world_scene_tests LABELS presentation)

    # Scene/picking contracts use their actual product implementations and asset inputs.
    function(ark_desktop_test target name)
        cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES;ARGS;LABELS" ${ARGN})
        ark_test_executable(${target} SOURCES ${ARG_SOURCES} LIBRARIES ${ARG_LIBRARIES})
        target_include_directories(${target} PRIVATE src/desktop)
        ark_test_case(${name} ${target} ARGS ${ARG_ARGS} LABELS presentation ${ARG_LABELS})
    endfunction()
    ark_desktop_test(ark_character_status_tests character_hp_render
        SOURCES tests/desktop/character_status_test.cpp src/desktop/scene/character_status.cpp
        LIBRARIES ark_world_rules)
    ark_desktop_test(ark_boundary_render_tests boundary_overlay_contract
        SOURCES tests/desktop/boundary_render_test.cpp src/desktop/scene/boundary_render.cpp
        LIBRARIES ark_world_runtime ark_asset_metadata PkgConfig::RAYLIB ARGS "${PROJECT_SOURCE_DIR}/assets")
    ark_desktop_test(ark_road_render_tests road_patch_pixels
        SOURCES tests/desktop/road_render_test.cpp src/desktop/scene/road_render.cpp
        LIBRARIES ark_world_rules ark_asset_metadata PkgConfig::RAYLIB ARGS "${PROJECT_SOURCE_DIR}/assets")
    ark_desktop_test(ark_projection_tests projection_input
        SOURCES tests/desktop/projection_test.cpp src/desktop/scene/projection.cpp
        LIBRARIES PkgConfig::RAYLIB)
    ark_desktop_test(ark_character_render_tests character_sprite_frames
        SOURCES tests/desktop/character_render_test.cpp
        LIBRARIES ark_asset_metadata ARGS "${PROJECT_SOURCE_DIR}/assets")
    ark_test_case(packaged_assets ark_village ARGS --check LABELS e2e
        TIMEOUT 10 WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" PASS "PASS packaged complete world")
    ark_test_case(packaged_frame_contract "${ARK_NODE}"
        ARGS "${PROJECT_SOURCE_DIR}/tests/integration/packaged_frames_test.mjs" "$<TARGET_FILE:ark_village>"
        LABELS e2e TIMEOUT 30)
endif()
