cmake_minimum_required(VERSION 3.16)

project(dungeon_village_tools LANGUAGES CXX)

include(../cmake/ResearchLibraries.cmake)

include(CTest)

include(../cmake/ResearchHash.cmake)
add_library(dungeon_village_archive src/archive.cpp src/sprite.cpp src/table.cpp src/original_save.cpp
                                   src/original_save_audit.cpp)
dungeon_village_library(dungeon_village_archive)
target_link_libraries(dungeon_village_archive PUBLIC dungeon_village_hash)
target_include_directories(dungeon_village_archive PUBLIC include)
target_compile_features(dungeon_village_archive PUBLIC cxx_std_17)
set_target_properties(dungeon_village_archive PROPERTIES CXX_EXTENSIONS OFF)

add_executable(kairo_asset_extract src/main.cpp)
target_link_libraries(kairo_asset_extract PRIVATE dungeon_village_archive)
target_compile_features(kairo_asset_extract PRIVATE cxx_std_17)
set_target_properties(kairo_asset_extract PROPERTIES CXX_EXTENSIONS OFF)

add_executable(kairo_seb_inspect src/seb_inspect.cpp)
target_link_libraries(kairo_seb_inspect PRIVATE dungeon_village_archive)
target_compile_features(kairo_seb_inspect PRIVATE cxx_std_17)
set_target_properties(kairo_seb_inspect PROPERTIES CXX_EXTENSIONS OFF)

add_executable(kairo_asset_publish src/publish_assets.cpp)
target_link_libraries(kairo_asset_publish PRIVATE dungeon_village_archive)
target_compile_features(kairo_asset_publish PRIVATE cxx_std_17)
set_target_properties(kairo_asset_publish PROPERTIES CXX_EXTENSIONS OFF)

add_executable(dungeon_village_archive_tests tests/archive_test.cpp tests/original_save_checks.cpp
                                           tests/original_save_audit_checks.cpp)
target_link_libraries(dungeon_village_archive_tests PRIVATE dungeon_village_archive)
target_compile_features(dungeon_village_archive_tests PRIVATE cxx_std_17)
set_target_properties(dungeon_village_archive_tests PROPERTIES CXX_EXTENSIONS OFF)

add_executable(kairo_table_inspect src/table_inspect.cpp)
target_link_libraries(kairo_table_inspect PRIVATE dungeon_village_archive)
target_compile_features(kairo_table_inspect PRIVATE cxx_std_17)
set_target_properties(kairo_table_inspect PROPERTIES CXX_EXTENSIONS OFF)

add_executable(kairo_save_inspect src/save_inspect.cpp)
target_link_libraries(kairo_save_inspect PRIVATE dungeon_village_archive)
target_compile_features(kairo_save_inspect PRIVATE cxx_std_17)
set_target_properties(kairo_save_inspect PROPERTIES CXX_EXTENSIONS OFF)

add_executable(dungeon_village_table_tests tests/table_test.cpp)
target_link_libraries(dungeon_village_table_tests PRIVATE dungeon_village_archive)
target_compile_features(dungeon_village_table_tests PRIVATE cxx_std_17)
set_target_properties(dungeon_village_table_tests PROPERTIES CXX_EXTENSIONS OFF)

add_executable(dungeon_village_item_table_tests tests/item_table_test.cpp)
target_link_libraries(dungeon_village_item_table_tests PRIVATE dungeon_village_archive)
target_compile_features(dungeon_village_item_table_tests PRIVATE cxx_std_17)
set_target_properties(dungeon_village_item_table_tests PROPERTIES CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    foreach(target dungeon_village_archive kairo_asset_extract kairo_asset_publish kairo_seb_inspect
                   dungeon_village_archive_tests kairo_table_inspect dungeon_village_table_tests
                   dungeon_village_item_table_tests kairo_save_inspect)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endforeach()
endif()

add_test(NAME dungeon_village_tools.archive COMMAND dungeon_village_archive_tests)
add_test(NAME dungeon_village_tools.table COMMAND dungeon_village_table_tests)
add_test(NAME dungeon_village_tools.facility_data
         COMMAND dungeon_village_table_tests "${CMAKE_CURRENT_SOURCE_DIR}/../data/original/tenantData.txt")
add_test(NAME dungeon_village_tools.item_table COMMAND dungeon_village_item_table_tests)
add_test(NAME dungeon_village_tools.item_data
         COMMAND dungeon_village_item_table_tests "${CMAKE_CURRENT_SOURCE_DIR}/../data/items/original/item.txt"
                 "${CMAKE_CURRENT_SOURCE_DIR}/../data/original/tenantData.txt")

include(../cmake/WindowsExecutables.cmake)
