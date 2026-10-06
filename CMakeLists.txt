cmake_minimum_required(VERSION 3.21)
project(ark_village VERSION 0.1.0 LANGUAGES CXX)

option(ARK_BUILD_DESKTOP "Build the raylib executable" ON)
include(CTest)

# Explicitly run as the caller: Windows otherwise treats 32-bit executables with names such
# as "update" as installers and rejects CreateProcess for ordinary users (ERROR_ELEVATION_REQUIRED).
if(WIN32)
    enable_language(RC)
    configure_file(cmake/windows.rc.in "${PROJECT_BINARY_DIR}/ark_windows.rc" @ONLY)
endif()
function(ark_windows_manifest target)
    if(WIN32)
        get_target_property(kind ${target} TYPE)
        if(kind STREQUAL "EXECUTABLE")
            target_sources(${target} PRIVATE "${PROJECT_BINARY_DIR}/ark_windows.rc")
        endif()
    endif()
endfunction()

function(ark_target target)
    ark_windows_manifest(${target})
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

add_library(ark_timing STATIC src/app/fixed_step_clock.cpp src/app/original_loop.cpp
    src/app/simulation_clock.cpp)
target_include_directories(ark_timing PUBLIC include)
ark_target(ark_timing)

find_program(ARK_NODE NAMES node REQUIRED)
include(cmake/WorldSimulation.cmake)
# Immutable UI plans also run in headless tests; only the final draw adapter depends on raylib.
add_library(ark_world_visuals STATIC src/desktop/world_combat_visuals.cpp
    src/desktop/world_rest_visuals.cpp src/desktop/world_dungeon_visuals.cpp
    src/desktop/ui/script_text.cpp)
target_include_directories(ark_world_visuals PUBLIC src/desktop)
target_link_libraries(ark_world_visuals PUBLIC ark_world_runtime)
ark_target(ark_world_visuals)

find_package(Threads REQUIRED)
# Canonical-world read queries do not own another world or expose UI/clock dependencies.
add_library(ark_world_queries STATIC src/app/world_facility_queries.cpp)
target_link_libraries(ark_world_queries PUBLIC ark_world_runtime)
ark_target(ark_world_queries)
# Keep persistence in the same linkage mode as its canonical runtime, so development
# executables reuse one persistence DLL alongside the shared runtime.
get_target_property(ark_runtime_library_type ark_world_runtime TYPE)
if(ark_runtime_library_type STREQUAL "SHARED_LIBRARY")
    set(ark_save_library_type SHARED)
else()
    set(ark_save_library_type STATIC)
endif()
add_library(ark_world_save ${ark_save_library_type} src/app/world_save_codec.cpp src/app/world_save_restore.cpp
    src/app/world_save_files.cpp)
target_link_libraries(ark_world_save PUBLIC ark_world_runtime)
ark_target(ark_world_save)
add_library(ark_world_session STATIC src/app/world_session.cpp src/app/world_commands.cpp src/app/world_report.cpp)
target_link_libraries(ark_world_session PUBLIC ark_world_runtime ark_timing ark_world_save Threads::Threads)
ark_target(ark_world_session)

add_executable(ark_world_simulation src/app/world_simulation_main.cpp)
target_link_libraries(ark_world_simulation PRIVATE ark_world_runtime)
set_target_properties(ark_world_simulation PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
ark_target(ark_world_simulation)
set(ARK_STARTUP_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated/startup_data.cpp")
add_custom_command(OUTPUT "${ARK_STARTUP_CPP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND "${ARK_NODE}" "${PROJECT_SOURCE_DIR}/scripts/compile_startup.mjs"
        "${PROJECT_SOURCE_DIR}/assets/data" "${ARK_STARTUP_CPP}"
    DEPENDS scripts/compile_startup.mjs scripts/compile_loaded_map.mjs scripts/compile_initial_ai.mjs assets/data/MAP.json assets/data/STATE.json
        assets/data/TABLES.json assets/data/tenantData.txt assets/data/LOADED_MAP.tsv
        assets/data/LOADED_INSTANCES.tsv VERBATIM)
add_library(ark_game STATIC src/world/grid.cpp src/world/terrain.cpp src/world/navigation.cpp src/facilities/facility.cpp
    src/facilities/map_binding.cpp src/app/map_queries.cpp src/people/motion.cpp
    src/people/activity_choice.cpp src/people/activity_candidates.cpp
    src/people/facility_choice.cpp src/people/departure.cpp
    src/people/actor_ai.cpp src/people/ai_perception.cpp src/people/actor_control.cpp
    src/people/actor_housekeeping.cpp src/people/combat_strategy.cpp
    src/people/combat_damage.cpp src/people/combat_influence.cpp
    src/facilities/dungeon_task.cpp src/facilities/dungeon_crew.cpp src/facilities/dungeon_completion.cpp
    src/people/decision.cpp
    src/app/world_schedule.cpp src/app/game_world.cpp src/app/ai_schedule.cpp src/people/actor_effects.cpp src/people/weapon_choice.cpp
    src/people/delayed_reward.cpp src/people/human_growth.cpp
    src/people/hp.cpp src/facilities/arrival.cpp src/facilities/exit.cpp src/facilities/service.cpp
    src/economy/cash.cpp src/app/initial_ai.cpp src/app/initial_ai_motion.cpp src/app/initial_ai_service.cpp
    src/app/initial_ai_check.cpp src/app/game_ai_preview.cpp
    src/app/live_life_context.cpp src/app/game_life.cpp src/app/live_departure.cpp src/app/live_motion.cpp
    src/app/random.cpp src/app/live_daily.cpp
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
        src/desktop/playability_probe.cpp src/desktop/world_view.cpp src/desktop/world_save_menu.cpp src/desktop/world_scene.cpp
        src/desktop/world_rank.cpp src/desktop/world_overlay_render.cpp src/desktop/world_task_inspection.cpp
        src/desktop/world_management.cpp src/desktop/world_management_inspection.cpp
        src/desktop/world_human_inspection.cpp
        src/desktop/world_build_placement.cpp
        src/desktop/scene.cpp src/desktop/road_render.cpp src/desktop/boundary_render.cpp src/desktop/character_animation.cpp
        src/desktop/character_status.cpp src/desktop/character_visibility.cpp src/desktop/ui/layout.cpp src/desktop/ui/controller.cpp
        src/desktop/ui/skin.cpp src/desktop/ui/world_panels.cpp src/desktop/ui/world_reports.cpp src/desktop/ui/world_award.cpp
        src/desktop/ui/world_building.cpp src/desktop/ui/world_progression.cpp
        src/desktop/ui/world_human.cpp src/desktop/ui/world_tax.cpp
        src/desktop/ui/world_crew_summary.cpp src/desktop/ui/world_tasks.cpp src/desktop/ui/world_menu.cpp src/desktop/ui/hud.cpp src/desktop/ui/pages.cpp
        src/desktop/ui/facility_context.cpp src/desktop/ui/facility_page.cpp)
    target_link_libraries(ark_village PRIVATE ark_launch ark_game ark_world_visuals ark_world_session ark_asset_metadata PkgConfig::RAYLIB)
    if(APPLE)
        target_link_libraries(ark_village PRIVATE "-framework CoreGraphics")
    endif()
    set_target_properties(ark_village PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    ark_target(ark_village)
    add_custom_target(ark_package_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/bin/assets")
    add_dependencies(ark_village ark_package_assets)
    set(ARK_DESKTOP_FONT "" CACHE FILEPATH "Optional complete CJK TTF/OTF copied beside the desktop executable")
    set(ARK_DESKTOP_FONT_LICENSE "" CACHE FILEPATH "License accompanying ARK_DESKTOP_FONT")
    if(ARK_DESKTOP_FONT)
        get_filename_component(ark_font_extension "${ARK_DESKTOP_FONT}" EXT)
        if(NOT EXISTS "${ARK_DESKTOP_FONT}" OR NOT ark_font_extension MATCHES "^\\.(ttf|otf)$")
            message(FATAL_ERROR "ARK_DESKTOP_FONT must be an existing TTF or OTF")
        endif()
        add_custom_command(TARGET ark_package_assets POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/fonts"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ARK_DESKTOP_FONT}"
                "${CMAKE_BINARY_DIR}/bin/fonts/default${ark_font_extension}")
        if(ARK_DESKTOP_FONT_LICENSE)
            add_custom_command(TARGET ark_package_assets POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ARK_DESKTOP_FONT_LICENSE}"
                    "${CMAKE_BINARY_DIR}/bin/fonts/LICENSE.txt")
        endif()
    endif()
endif()

if(BUILD_TESTING)
    include(cmake/ProductTests.cmake)
endif()
