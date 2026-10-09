cmake_minimum_required(VERSION 3.21)
project(ark_village VERSION 0.1.0 LANGUAGES CXX)

option(ARK_BUILD_DESKTOP "Build the raylib executable" ON)
option(ARK_LIBRARIES_ONLY "Build the shared product libraries without applications or tests" OFF)
include(CTest)
include(cmake/SharedRuntime.cmake)

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
    ark_use_shared_runtime(${target})
    target_compile_features(${target} PUBLIC cxx_std_17)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

find_program(ARK_NODE NAMES node REQUIRED)
if(ARK_LIBRARIES_ONLY)
    include(cmake/ProductLibraries.cmake)
    return()
endif()

find_package(Threads REQUIRED)
if(ARK_BUILD_DESKTOP)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RAYLIB REQUIRED IMPORTED_TARGET raylib>=6.0)
endif()
include(cmake/CommonLibraries.cmake)
include(cmake/WorldSimulation.cmake)

add_executable(ark_world_simulation src/app/world_simulation_main.cpp)
target_link_libraries(ark_world_simulation PRIVATE ark_world_runtime)
ark_target(ark_world_simulation)

if(ARK_BUILD_DESKTOP)
    add_executable(ark_village src/desktop/main.cpp src/desktop/desktop_session.cpp
        src/desktop/resources.cpp src/desktop/projection.cpp src/desktop/game_view.cpp
        src/desktop/playability_probe.cpp src/desktop/world_view.cpp src/desktop/world_inspection.cpp
        src/desktop/world_save_menu.cpp src/desktop/world_title.cpp src/desktop/ui/world_startup.cpp src/desktop/world_scene.cpp
        src/desktop/world_rank.cpp src/desktop/world_overlay_render.cpp src/desktop/world_task_inspection.cpp
        src/desktop/world_management.cpp src/desktop/world_management_inspection.cpp
        src/desktop/world_human_inspection.cpp
        src/desktop/world_build_placement.cpp
        src/desktop/world_editing.cpp
        src/desktop/scene.cpp src/desktop/road_render.cpp src/desktop/boundary_render.cpp src/desktop/character_animation.cpp
        src/desktop/character_status.cpp src/desktop/character_visibility.cpp src/desktop/ui/layout.cpp src/desktop/ui/controller.cpp
        src/desktop/ui/skin.cpp src/desktop/ui/world_panels.cpp src/desktop/ui/world_reports.cpp src/desktop/ui/world_award.cpp
        src/desktop/ui/world_building.cpp src/desktop/ui/world_progression.cpp
        src/desktop/ui/world_human.cpp src/desktop/ui/world_tax.cpp
        src/desktop/ui/world_village_activity.cpp
        src/desktop/ui/world_commerce.cpp src/desktop/ui/world_facility_items.cpp
        src/desktop/ui/world_facility_catalog.cpp
        src/desktop/ui/world_magic_pot.cpp
        src/desktop/ui/world_crew_summary.cpp src/desktop/ui/world_tasks.cpp src/desktop/ui/world_menu.cpp src/desktop/ui/hud.cpp src/desktop/ui/pages.cpp
        src/desktop/ui/facility_context.cpp src/desktop/ui/facility_page.cpp)
    target_include_directories(ark_village PRIVATE "${ARK_DESKTOP_GENERATED_DIR}")
    target_link_libraries(ark_village PRIVATE ark_launch ark_game ark_world_visuals ark_world_queries ark_world_session ark_asset_metadata PkgConfig::RAYLIB)
    if(APPLE)
        target_link_libraries(ark_village PRIVATE "-framework CoreGraphics")
    endif()
    ark_target(ark_village)
endif()

if(BUILD_TESTING)
    include(cmake/ProductTests.cmake)
endif()
