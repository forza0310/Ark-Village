# One independent product-library build; consumers import these targets without recompiling them.
add_library(ark_launch SHARED src/app/bootstrap/launch_options.cpp)
target_include_directories(ark_launch PUBLIC include)
ark_target(ark_launch)

add_library(ark_timing SHARED src/app/timing/original_loop.cpp)
target_include_directories(ark_timing PUBLIC include)
ark_target(ark_timing)

include("${CMAKE_CURRENT_LIST_DIR}/WorldSimulation.cmake")
# Immutable UI plans also run in headless tests; only the final draw adapter depends on raylib.
add_library(ark_world_visuals SHARED src/presentation/world_combat_visuals.cpp
    src/presentation/world_rest_visuals.cpp src/presentation/world_dungeon_visuals.cpp
    src/presentation/script_text.cpp)
target_include_directories(ark_world_visuals PUBLIC include)
target_link_libraries(ark_world_visuals PUBLIC ark_world_runtime)
ark_target(ark_world_visuals)

find_package(Threads REQUIRED)
# Canonical-world read queries do not own another world or expose UI/clock dependencies.
add_library(ark_world_queries SHARED src/app/queries/world_facility_queries.cpp)
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
add_library(ark_world_save ${ark_save_library_type} src/app/save/world_save_codec.cpp src/app/save/world_save_restore.cpp
    src/app/save/world_save_files.cpp)
target_link_libraries(ark_world_save PUBLIC ark_world_runtime)
ark_target(ark_world_save)
add_library(ark_world_session SHARED src/app/session/world_session.cpp src/app/session/world_commands.cpp src/app/session/world_report.cpp
    src/app/session/world_system.cpp)
target_link_libraries(ark_world_session PUBLIC ark_world_runtime ark_world_system ark_timing ark_world_save Threads::Threads)
ark_target(ark_world_session)

add_library(ark_asset_metadata SHARED src/assets/sprite.cpp src/assets/table.cpp)
target_include_directories(ark_asset_metadata PUBLIC include)
ark_target(ark_asset_metadata)

set(ARK_PRODUCT_LIBRARIES ark_launch ark_timing ark_world_hash ark_world_rules ark_world_runtime
    ark_world_file_io ark_world_system ark_world_persistence ark_startup_application
    ark_world_visuals ark_world_queries ark_world_save ark_world_session ark_asset_metadata)

if(ARK_BUILD_DESKTOP)
    # One shared desktop-only Unicode inventory feeds runtime atlases and the font subset.
    # Scan on every common build so newly added source/catalog files are included without
    # GLOB or four consumer copies. The scanner preserves unchanged output timestamps.
    set(ARK_DESKTOP_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/desktop-generated")
    set(ARK_DESKTOP_GLYPH_HEADER "${ARK_DESKTOP_GENERATED_DIR}/desktop_glyphs.hpp")
    set(ARK_DESKTOP_GLYPH_INVENTORY "${ARK_DESKTOP_GENERATED_DIR}/desktop_glyphs.json")
    add_custom_target(ark_desktop_glyphs ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ARK_DESKTOP_GENERATED_DIR}"
        COMMAND "${ARK_NODE}" "${PROJECT_SOURCE_DIR}/scripts/compile_desktop_glyphs.mjs"
            --root "${PROJECT_SOURCE_DIR}"
            --json "${ARK_DESKTOP_GLYPH_INVENTORY}" --header "${ARK_DESKTOP_GLYPH_HEADER}"
        BYPRODUCTS "${ARK_DESKTOP_GLYPH_HEADER}" "${ARK_DESKTOP_GLYPH_INVENTORY}"
        DEPENDS "${PROJECT_SOURCE_DIR}/scripts/compile_desktop_glyphs.mjs"
        VERBATIM)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RAYLIB REQUIRED IMPORTED_TARGET raylib>=6.0)
    ark_copy_raylib_runtime()
    add_library(ark_world_ui_test_support SHARED src/desktop/ui/common/layout.cpp
        src/desktop/ui/common/skin.cpp src/desktop/resources/sprites.cpp src/desktop/resources/text.cpp
        src/desktop/resources/asset_check.cpp src/desktop/resources/resource_metadata.cpp src/desktop/scene/sprite_picking.cpp
        src/desktop/scene/projection.cpp src/desktop/scene/boundary_render.cpp src/desktop/scene/road_render.cpp)
    target_include_directories(ark_world_ui_test_support PUBLIC src/desktop
        PRIVATE "${ARK_DESKTOP_GENERATED_DIR}")
    add_dependencies(ark_world_ui_test_support ark_desktop_glyphs)
    target_link_libraries(ark_world_ui_test_support PUBLIC ark_world_visuals
        ark_asset_metadata PkgConfig::RAYLIB)
    ark_target(ark_world_ui_test_support)
    list(APPEND ARK_PRODUCT_LIBRARIES ark_world_ui_test_support)
    add_custom_target(ark_package_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/assets" "${ARK_SHARED_RUNTIME_DIRECTORY}/assets")
    set(ARK_DESKTOP_FONT "" CACHE FILEPATH "Optional complete CJK TTF/OTF copied beside the desktop executable")
    set(ARK_DESKTOP_FONT_LICENSE "" CACHE FILEPATH "License accompanying ARK_DESKTOP_FONT")
    if(ARK_DESKTOP_FONT)
        get_filename_component(ark_font_extension "${ARK_DESKTOP_FONT}" EXT)
        if(NOT EXISTS "${ARK_DESKTOP_FONT}" OR NOT ark_font_extension MATCHES "^\\.(ttf|otf)$")
            message(FATAL_ERROR "ARK_DESKTOP_FONT must be an existing TTF or OTF")
        endif()
        add_custom_command(TARGET ark_package_assets POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${ARK_SHARED_RUNTIME_DIRECTORY}/fonts"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ARK_DESKTOP_FONT}"
                "${ARK_SHARED_RUNTIME_DIRECTORY}/fonts/default${ark_font_extension}")
        if(ARK_DESKTOP_FONT_LICENSE)
            add_custom_command(TARGET ark_package_assets POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ARK_DESKTOP_FONT_LICENSE}"
                    "${ARK_SHARED_RUNTIME_DIRECTORY}/fonts/LICENSE.txt")
        endif()
    endif()
endif()

export(TARGETS ${ARK_PRODUCT_LIBRARIES} FILE "${CMAKE_BINARY_DIR}/ArkLibraries.cmake")
file(WRITE "${CMAKE_BINARY_DIR}/ArkLibraryBuild.cmake"
    "set(ARK_LIBRARY_COMPILER [[${CMAKE_CXX_COMPILER}]])\n"
    "set(ARK_LIBRARY_COMPILER_ID [[${CMAKE_CXX_COMPILER_ID}]])\n"
    "set(ARK_LIBRARY_COMPILER_VERSION [[${CMAKE_CXX_COMPILER_VERSION}]])\n"
    "set(ARK_LIBRARY_POINTER_SIZE [[${CMAKE_SIZEOF_VOID_P}]])\n"
    "set(ARK_LIBRARY_BUILD_TYPE [[${CMAKE_BUILD_TYPE}]])\n")

include("${CMAKE_CURRENT_LIST_DIR}/LibraryContract.cmake")
ark_publish_library_contract()
