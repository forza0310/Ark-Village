cmake_minimum_required(VERSION 3.21)
project(ark_village VERSION 0.1.0 LANGUAGES CXX)

option(ARK_BUILD_DESKTOP "Build the raylib executable" ON)
include(CTest)

function(ark_target target)
    target_compile_features(${target} PUBLIC cxx_std_17)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

add_library(ark_launch STATIC src/app/launch_options.cpp)
target_include_directories(ark_launch PUBLIC include)
ark_target(ark_launch)

find_program(ARK_NODE NAMES node REQUIRED)
set(ARK_STARTUP_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated/startup_data.cpp")
add_custom_command(OUTPUT "${ARK_STARTUP_CPP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND "${ARK_NODE}" "${PROJECT_SOURCE_DIR}/scripts/compile_startup.mjs"
        "${PROJECT_SOURCE_DIR}/assets/data" "${ARK_STARTUP_CPP}"
    DEPENDS scripts/compile_startup.mjs scripts/compile_loaded_map.mjs assets/data/MAP.json assets/data/STATE.json
        assets/data/TABLES.json assets/data/tenantData.txt assets/data/LOADED_MAP.tsv
        assets/data/LOADED_INSTANCES.tsv VERBATIM)
add_library(ark_game STATIC src/world/grid.cpp src/world/terrain.cpp src/world/navigation.cpp src/facilities/facility.cpp
    src/facilities/map_binding.cpp src/app/map_queries.cpp src/people/motion.cpp
    src/people/activity_choice.cpp src/people/activity_candidates.cpp
    src/people/facility_choice.cpp src/people/departure.cpp
    src/facilities/economy.cpp src/facilities/neighbourhood.cpp src/app/facility_queries.cpp
    src/people/adventurer.cpp src/app/game.cpp "${ARK_STARTUP_CPP}")
target_include_directories(ark_game PUBLIC include)
ark_target(ark_game)
add_library(ark_asset_metadata STATIC src/assets/sprite.cpp src/assets/table.cpp)
target_include_directories(ark_asset_metadata PUBLIC include)
ark_target(ark_asset_metadata)

if(ARK_BUILD_DESKTOP)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RAYLIB REQUIRED IMPORTED_TARGET raylib>=6.0)
    add_executable(ark_village src/desktop/main.cpp src/desktop/desktop_session.cpp
        src/desktop/resources.cpp src/desktop/projection.cpp src/desktop/game_view.cpp
        src/desktop/scene.cpp src/desktop/ui/layout.cpp src/desktop/ui/controller.cpp
        src/desktop/ui/skin.cpp src/desktop/ui/hud.cpp src/desktop/ui/pages.cpp
        src/desktop/ui/facility_context.cpp src/desktop/ui/facility_page.cpp)
    target_link_libraries(ark_village PRIVATE ark_launch ark_game ark_asset_metadata PkgConfig::RAYLIB)
    if(APPLE)
        target_link_libraries(ark_village PRIVATE "-framework CoreGraphics")
    endif()
    set_target_properties(ark_village PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    ark_target(ark_village)
    add_custom_target(ark_package_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/bin/assets")
    add_dependencies(ark_village ark_package_assets)
endif()

if(BUILD_TESTING)
    add_executable(ark_launch_tests tests/launch_options_test.cpp)
    target_link_libraries(ark_launch_tests PRIVATE ark_launch)
    ark_target(ark_launch_tests)
    add_test(NAME launch_options COMMAND ark_launch_tests)
    set_tests_properties(launch_options PROPERTIES TIMEOUT 10)
    add_executable(ark_game_tests tests/game_test.cpp)
    target_link_libraries(ark_game_tests PRIVATE ark_game)
    ark_target(ark_game_tests)
    add_test(NAME game_first_play COMMAND ark_game_tests)
    add_executable(ark_facility_tests tests/facility_rules_test.cpp)
    target_link_libraries(ark_facility_tests PRIVATE ark_game)
    ark_target(ark_facility_tests)
    add_test(NAME facility_economy_neighbours COMMAND ark_facility_tests)
    add_executable(ark_terrain_tests tests/terrain_test.cpp)
    target_link_libraries(ark_terrain_tests PRIVATE ark_game)
    ark_target(ark_terrain_tests)
    add_test(NAME road_connections COMMAND ark_terrain_tests)
    add_executable(ark_navigation_tests tests/navigation_motion_test.cpp)
    target_link_libraries(ark_navigation_tests PRIVATE ark_game)
    ark_target(ark_navigation_tests)
    add_test(NAME loaded_map_navigation_motion COMMAND ark_navigation_tests)
    add_executable(ark_activity_choice_tests tests/activity_choice_test.cpp)
    target_link_libraries(ark_activity_choice_tests PRIVATE ark_game)
    ark_target(ark_activity_choice_tests)
    add_test(NAME activity_category_choice COMMAND ark_activity_choice_tests)
    add_executable(ark_activity_candidates_tests tests/activity_candidates_test.cpp)
    target_link_libraries(ark_activity_candidates_tests PRIVATE ark_game)
    ark_target(ark_activity_candidates_tests)
    add_test(NAME activity_candidates COMMAND ark_activity_candidates_tests)
    add_executable(ark_departure_tests tests/departure_test.cpp)
    target_link_libraries(ark_departure_tests PRIVATE ark_game)
    ark_target(ark_departure_tests)
    add_test(NAME facility_choice_departure COMMAND ark_departure_tests)
    add_executable(ark_metadata_tests tests/metadata_test.cpp)
    target_link_libraries(ark_metadata_tests PRIVATE ark_asset_metadata)
    ark_target(ark_metadata_tests)
    add_test(NAME asset_metadata COMMAND ark_metadata_tests)
    add_test(NAME startup_data_contract COMMAND "${ARK_NODE}" "${PROJECT_SOURCE_DIR}/tests/startup_data_test.mjs"
        "${PROJECT_SOURCE_DIR}/assets/data")
    add_test(NAME source_provenance COMMAND "${ARK_NODE}" "${PROJECT_SOURCE_DIR}/scripts/verify_assets.mjs"
        "${PROJECT_SOURCE_DIR}/assets")
    if(ARK_BUILD_DESKTOP)
        add_test(NAME packaged_assets COMMAND ark_village --check)
        set_tests_properties(packaged_assets PROPERTIES TIMEOUT 10 WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
        add_executable(ark_projection_tests tests/projection_test.cpp src/desktop/projection.cpp)
        target_include_directories(ark_projection_tests PRIVATE include src/desktop)
        target_link_libraries(ark_projection_tests PRIVATE ark_game PkgConfig::RAYLIB)
        ark_target(ark_projection_tests)
        add_test(NAME projection_input COMMAND ark_projection_tests)
        add_executable(ark_ui_tests tests/ui_navigation_test.cpp src/desktop/projection.cpp
            src/desktop/ui/layout.cpp src/desktop/ui/controller.cpp src/desktop/ui/facility_context.cpp)
        target_include_directories(ark_ui_tests PRIVATE include src/desktop)
        target_link_libraries(ark_ui_tests PRIVATE ark_game PkgConfig::RAYLIB)
        ark_target(ark_ui_tests)
        add_test(NAME ui_navigation COMMAND ark_ui_tests)
        add_test(NAME packaged_frame_contract COMMAND "${ARK_NODE}"
            "${PROJECT_SOURCE_DIR}/tests/packaged_frames_test.mjs" "$<TARGET_FILE:ark_village>")
        set_tests_properties(packaged_frame_contract PROPERTIES TIMEOUT 30)
    endif()
endif()
