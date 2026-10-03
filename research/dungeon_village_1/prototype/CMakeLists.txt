cmake_minimum_required(VERSION 3.16)

project(dungeon_village_prototype LANGUAGES CXX)

include(CTest)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RAYLIB REQUIRED IMPORTED_TARGET raylib)

add_subdirectory(../example domain)

# Structured publishing data is compiled once; the standard C++ model has no JSON/Node runtime.
find_program(STARTUP_NODE_EXECUTABLE NAMES node)
if(NOT STARTUP_NODE_EXECUTABLE)
    message(FATAL_ERROR "构建新局发布数据需要 Node；运行原型不需要 Node")
endif()
set(STARTUP_CPP "${CMAKE_CURRENT_BINARY_DIR}/startup_data.cpp")
add_custom_command(OUTPUT "${STARTUP_CPP}"
    COMMAND "${STARTUP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/compile_startup.mjs"
            "${CMAKE_CURRENT_SOURCE_DIR}/../data/startup"
            "${CMAKE_CURRENT_SOURCE_DIR}/../data/original/tenantData.txt" "${STARTUP_CPP}"
    DEPENDS scripts/compile_startup.mjs ../data/startup/MAP.json ../data/startup/STATE.json
            ../data/startup/TABLES.json ../data/original/tenantData.txt VERBATIM)
add_library(dungeon_village_startup_model src/startup.cpp src/startup_map.cpp "${STARTUP_CPP}")
target_include_directories(dungeon_village_startup_model PUBLIC include)
target_link_libraries(dungeon_village_startup_model PUBLIC dungeon_village_reference)
target_compile_features(dungeon_village_startup_model PUBLIC cxx_std_17)
set_target_properties(dungeon_village_startup_model PROPERTIES CXX_EXTENSIONS OFF)

add_executable(dungeon_village_startup_tests tests/startup_test.cpp)
target_link_libraries(dungeon_village_startup_tests PRIVATE dungeon_village_startup_model)
add_test(NAME dungeon_village_prototype.startup COMMAND dungeon_village_startup_tests)
add_executable(dungeon_village_startup_map_tests tests/startup_map_test.cpp)
target_link_libraries(dungeon_village_startup_map_tests PRIVATE dungeon_village_startup_model)
add_test(NAME dungeon_village_prototype.startup_map COMMAND dungeon_village_startup_map_tests)
add_executable(dungeon_village_dump_startup_map src/dump_startup_map.cpp)
target_link_libraries(dungeon_village_dump_startup_map PRIVATE dungeon_village_startup_model)
add_test(NAME dungeon_village_prototype.loaded_snapshot
    COMMAND "${STARTUP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/loaded_snapshot_test.mjs"
            "$<TARGET_FILE:dungeon_village_dump_startup_map>" "${CMAKE_CURRENT_SOURCE_DIR}/../data/startup")
add_test(NAME dungeon_village_prototype.startup_data
    COMMAND "${STARTUP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/startup_data_test.mjs"
            "${CMAKE_CURRENT_SOURCE_DIR}/../data/startup"
            "${CMAKE_CURRENT_SOURCE_DIR}/../data/original/tenantData.txt")

add_library(dungeon_village_prototype_model src/village.cpp src/catalog.cpp ../tools/src/table.cpp)
target_include_directories(dungeon_village_prototype_model PUBLIC include PRIVATE ../tools/include)
target_link_libraries(dungeon_village_prototype_model PUBLIC dungeon_village_reference)
target_compile_features(dungeon_village_prototype_model PUBLIC cxx_std_17)
set_target_properties(dungeon_village_prototype_model PROPERTIES CXX_EXTENSIONS OFF)

add_executable(dungeon_village_village_tests tests/village_test.cpp)
target_link_libraries(dungeon_village_village_tests PRIVATE dungeon_village_prototype_model)
set_target_properties(dungeon_village_village_tests PROPERTIES CXX_EXTENSIONS OFF)
add_test(NAME dungeon_village_prototype.village COMMAND dungeon_village_village_tests
         "${CMAKE_CURRENT_SOURCE_DIR}/../data/original/tenantData.txt")

add_executable(dungeon_village_prototype src/main.cpp src/asset_manifest.cpp src/startup_view.cpp ../tools/src/sprite.cpp)
target_include_directories(dungeon_village_prototype PRIVATE include ../tools/include)
target_link_libraries(dungeon_village_prototype PRIVATE dungeon_village_prototype_model dungeon_village_startup_model PkgConfig::RAYLIB)
target_compile_features(dungeon_village_prototype PRIVATE cxx_std_17)
set_target_properties(dungeon_village_prototype PROPERTIES CXX_EXTENSIONS OFF)
set_property(TARGET dungeon_village_prototype APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/MANIFEST.tsv"
    "${CMAKE_CURRENT_SOURCE_DIR}/../data/original/tenantData.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/normalized/building.inn.single.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/normalized/building.inn.pair.front.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/normalized/building.inn.pair.back.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/normalized/building.cafe.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/normalized/character.night.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/normalized/terrain.grass.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/normalized/terrain.road.png")
file(GLOB STARTUP_IMAGE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/image/*")
set_property(TARGET dungeon_village_prototype APPEND PROPERTY LINK_DEPENDS ${STARTUP_IMAGE_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/human/chara_flower00.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/human/walk00.seb"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/common/chara_hishoko01.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/common/chara_hishoko01.seb")
add_custom_command(TARGET dungeon_village_prototype POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../assets/normalized"
            "$<TARGET_FILE_DIR:dungeon_village_prototype>/assets/normalized"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/image"
            "$<TARGET_FILE_DIR:dungeon_village_prototype>/assets/original/image"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:dungeon_village_prototype>/assets/original/human"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:dungeon_village_prototype>/assets/original/common"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/common/chara_hishoko01.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/common/chara_hishoko01.seb"
            "$<TARGET_FILE_DIR:dungeon_village_prototype>/assets/original/common"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/human/chara_flower00.png"
            "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/human/walk00.seb"
            "$<TARGET_FILE_DIR:dungeon_village_prototype>/assets/original/human"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../assets/MANIFEST.tsv"
            "$<TARGET_FILE_DIR:dungeon_village_prototype>/assets/MANIFEST.tsv"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:dungeon_village_prototype>/data"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../data/original/tenantData.txt"
            "$<TARGET_FILE_DIR:dungeon_village_prototype>/data/tenantData.txt")
add_test(NAME dungeon_village_prototype.check COMMAND dungeon_village_prototype --check)
add_test(NAME dungeon_village_prototype.check_fixture COMMAND dungeon_village_prototype --fixture --check)
add_test(NAME dungeon_village_prototype.check_second
         COMMAND dungeon_village_prototype --fixture --check --orientation 1)
add_test(NAME dungeon_village_prototype.reject_orientation
         COMMAND dungeon_village_prototype --check --orientation 2)
add_test(NAME dungeon_village_prototype.reject_frames
         COMMAND dungeon_village_prototype --check --frames 0)
add_test(NAME dungeon_village_prototype.reject_inspect
         COMMAND dungeon_village_prototype --inspect-page shops)
set_tests_properties(dungeon_village_prototype.reject_orientation
                     dungeon_village_prototype.reject_frames dungeon_village_prototype.reject_inspect PROPERTIES WILL_FAIL TRUE)
set_tests_properties(dungeon_village_prototype.check PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/domain")

add_executable(dungeon_village_asset_tests tests/asset_test.cpp src/asset_manifest.cpp)
target_include_directories(dungeon_village_asset_tests PRIVATE include)
target_link_libraries(dungeon_village_asset_tests PRIVATE PkgConfig::RAYLIB)
target_compile_features(dungeon_village_asset_tests PRIVATE cxx_std_17)
set_target_properties(dungeon_village_asset_tests PROPERTIES CXX_EXTENSIONS OFF)
add_test(NAME dungeon_village_prototype.assets
         COMMAND dungeon_village_asset_tests "${CMAKE_CURRENT_SOURCE_DIR}/../assets")

if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    target_compile_options(dungeon_village_prototype_model PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_village_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_prototype PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_asset_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_startup_model PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_startup_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_startup_map_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_dump_startup_map PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
