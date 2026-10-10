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
    add_executable(ark_village src/desktop/main.cpp src/desktop/platform/desktop_session.cpp
        src/desktop/resources/sprites.cpp src/desktop/resources/text.cpp
        src/desktop/resources/asset_check.cpp src/desktop/resources/resource_metadata.cpp src/desktop/scene/sprite_picking.cpp src/desktop/scene/projection.cpp
        src/desktop/application/world_view.cpp src/desktop/inspection/world_inspection.cpp
        src/desktop/input/world_save_menu.cpp src/desktop/application/world_title.cpp src/desktop/platform/world_audio.cpp src/desktop/ui/system/world_startup.cpp src/desktop/scene/world_scene.cpp
        src/presentation/world_rank.cpp src/desktop/scene/world_overlay_render.cpp src/desktop/inspection/world_task_inspection.cpp
        src/desktop/input/world_management.cpp src/desktop/inspection/world_management_inspection.cpp
        src/desktop/inspection/world_human_inspection.cpp
        src/desktop/scene/world_build_placement.cpp
        src/desktop/scene/world_editing.cpp
        src/desktop/scene/road_render.cpp src/desktop/scene/boundary_render.cpp
        src/desktop/scene/character_status.cpp src/desktop/ui/common/layout.cpp
        src/desktop/ui/common/skin.cpp src/desktop/ui/common/world_panels.cpp src/desktop/ui/common/world_reports.cpp src/desktop/ui/village/world_award.cpp
        src/desktop/ui/facilities/world_building_view.cpp
        src/desktop/ui/facilities/world_building_input.cpp src/desktop/ui/facilities/world_building_render.cpp src/desktop/ui/facilities/world_facility_upgrade.cpp src/desktop/ui/village/world_progression.cpp
        src/desktop/ui/actors/world_human.cpp src/desktop/ui/actors/world_human_detail.cpp src/desktop/ui/village/world_tax.cpp
        src/desktop/ui/village/world_village_activity.cpp
        src/desktop/ui/village/world_commerce.cpp src/desktop/ui/facilities/world_facility_items.cpp
        src/desktop/ui/facilities/world_facility_catalog.cpp
        src/desktop/ui/village/world_magic_pot.cpp
        src/desktop/ui/tasks/world_crew_summary.cpp src/desktop/ui/tasks/world_tasks.cpp src/desktop/ui/system/world_menu.cpp)
    target_include_directories(ark_village PRIVATE "${ARK_DESKTOP_GENERATED_DIR}")
    target_link_libraries(ark_village PRIVATE ark_launch ark_world_visuals ark_world_queries ark_world_session ark_asset_metadata PkgConfig::RAYLIB)
    if(APPLE)
        target_link_libraries(ark_village PRIVATE "-framework CoreGraphics")
    endif()
    ark_target(ark_village)
endif()

if(BUILD_TESTING)
    include(cmake/ProductTests.cmake)
endif()
