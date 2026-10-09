cmake_minimum_required(VERSION 3.16)

project(dungeon_village_prototype LANGUAGES CXX)

include(../cmake/ResearchLibraries.cmake)
include(../cmake/ResearchHash.cmake)

include(CTest)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RAYLIB REQUIRED IMPORTED_TARGET raylib)
dungeon_village_raylib_runtime()

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
add_library(dungeon_village_startup_model src/startup.cpp src/startup_map.cpp src/road_render.cpp
                                        src/startup_ai.cpp src/facility_projection.cpp src/loop_pacing.cpp "${STARTUP_CPP}")
dungeon_village_library(dungeon_village_startup_model)
target_include_directories(dungeon_village_startup_model PUBLIC include)
target_link_libraries(dungeon_village_startup_model PUBLIC dungeon_village_reference)
target_compile_features(dungeon_village_startup_model PUBLIC cxx_std_17)
set_target_properties(dungeon_village_startup_model PROPERTIES CXX_EXTENSIONS OFF)

# 全目录另编译，避免运行时读取研究目录，也不改变旧首访切片的发布接口。
set(STARTUP_WORLD_CPP "${CMAKE_CURRENT_BINARY_DIR}/startup_world_data.cpp")
add_custom_command(OUTPUT "${STARTUP_WORLD_CPP}"
    COMMAND "${STARTUP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/compile_startup_world.mjs"
            "${CMAKE_CURRENT_SOURCE_DIR}/../data/startup" "${CMAKE_CURRENT_SOURCE_DIR}/../data/world"
            "${CMAKE_CURRENT_SOURCE_DIR}/../data/original/tenantData.txt" "${STARTUP_WORLD_CPP}"
    DEPENDS scripts/compile_startup_world.mjs scripts/compile_startup.mjs
            ../data/startup/MAP.json ../data/startup/STATE.json ../data/startup/TABLES.json
            ../data/original/tenantData.txt ../data/world/monster.txt ../data/world/questData.txt
            ../data/world/armour.txt ../data/world/accessory.txt ../data/world/item.txt
            ../data/world/asEventData.txt ../data/world/magicPot.txt
            ../data/scripts/original/events.txt ../data/scripts/original/talk.txt
            ../data/scripts/original/news.txt ../data/scripts/original/evtmsgs.txt
            ../data/scripts/original/popularBonus.txt VERBATIM)
set(PERSISTENCE_IDENTITY_CPP "${CMAKE_CURRENT_BINARY_DIR}/persistence_identity.cpp")
add_custom_command(OUTPUT "${PERSISTENCE_IDENTITY_CPP}"
    COMMAND "${STARTUP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/compile_persistence_identity.mjs"
            "${CMAKE_CURRENT_SOURCE_DIR}/.." "${PERSISTENCE_IDENTITY_CPP}"
    DEPENDS scripts/compile_persistence_identity.mjs
            ../data/startup/MAP.json ../data/startup/STATE.json ../data/startup/TABLES.json
            ../data/original/tenantData.txt ../data/world/monster.txt ../data/world/questData.txt
            ../data/world/armour.txt ../data/world/accessory.txt ../data/world/item.txt
            ../data/world/asEventData.txt ../data/world/magicPot.txt ../data/scripts/original/events.txt
            ../data/scripts/original/talk.txt ../data/scripts/original/news.txt
            ../data/scripts/original/evtmsgs.txt ../data/scripts/original/popularBonus.txt VERBATIM)
add_library(dungeon_village_startup_world src/startup_world_projection.cpp
    src/startup_world_codec.cpp src/startup_world_restore_validation.cpp
    src/startup_world_persistence.cpp src/startup_world_file_io.cpp "${PERSISTENCE_IDENTITY_CPP}"
    src/startup_world_routes.cpp src/startup_world_runtime.cpp
    src/startup_world_profile.cpp src/startup_world_inheritance.cpp
    src/startup_world_runtime_calendar.cpp src/startup_world_runtime_scene.cpp
    src/startup_world_runtime_focus.cpp
    src/startup_world_runtime_arrival.cpp src/startup_world_runtime_tasks.cpp
    src/startup_world_runtime_task_pages.cpp
    src/startup_world_runtime_deadline.cpp
    src/startup_world_runtime_pages.cpp src/startup_world_runtime_nonactors.cpp
    src/startup_world_visuals.cpp src/startup_world_presentation.cpp src/startup_world_building.cpp src/startup_world_expansion.cpp
    src/startup_world_human.cpp src/startup_world_tax.cpp src/startup_world_village_activity.cpp
    src/startup_world_facility_items.cpp src/startup_world_facility_catalog.cpp src/startup_world_magic_pot.cpp
    src/startup_world_commerce.cpp src/startup_world_editing.cpp "${STARTUP_WORLD_CPP}")
dungeon_village_library(dungeon_village_startup_world)
target_link_libraries(dungeon_village_startup_world PUBLIC dungeon_village_startup_model)
target_link_libraries(dungeon_village_startup_world PRIVATE dungeon_village_hash)
target_include_directories(dungeon_village_startup_world PUBLIC include)
target_compile_features(dungeon_village_startup_world PUBLIC cxx_std_17)
set_target_properties(dungeon_village_startup_world PROPERTIES CXX_EXTENSIONS OFF)
# 应用持有跨世界系统纪录；复用世界文件I/O，不让规则层依赖标题或文件。
add_library(dungeon_village_startup_application src/startup_application.cpp
    src/startup_application_menu.cpp src/startup_title_menu.cpp src/startup_application_storage.cpp
    src/startup_application_storage_replay.cpp
    src/startup_system_records.cpp src/startup_world_clear_score.cpp src/startup_skin.cpp
    src/startup_application_replay_paths.cpp src/startup_application_replay.cpp
    src/startup_title_presentation.cpp src/startup_application_title.cpp src/startup_application_actions.cpp
    src/startup_title_actor_skin.cpp src/startup_information.cpp src/steam_startup_skin.cpp
    src/steam_facility_skin.cpp)
dungeon_village_library(dungeon_village_startup_application)
target_link_libraries(dungeon_village_startup_application PUBLIC dungeon_village_startup_world
    PRIVATE dungeon_village_hash)
target_compile_features(dungeon_village_startup_application PUBLIC cxx_std_17)
set_target_properties(dungeon_village_startup_application PROPERTIES CXX_EXTENSIONS OFF)
target_compile_definitions(dungeon_village_startup_application PRIVATE
    DUNGEON_VILLAGE_RESEARCH_WORK_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/../work")
add_executable(dungeon_village_startup_application_cli src/startup_application_main.cpp)
target_link_libraries(dungeon_village_startup_application_cli PRIVATE dungeon_village_startup_application)
add_executable(dungeon_village_startup_application_tests tests/startup_application_test.cpp
    tests/startup_system_records_test.cpp tests/startup_world_clear_score_test.cpp
    tests/startup_application_replay_paths_checks.cpp tests/startup_application_replay_state_checks.cpp
    tests/startup_title_presentation_checks.cpp tests/startup_title_menu_checks.cpp
    tests/startup_title_menu_replay_checks.cpp)
target_link_libraries(dungeon_village_startup_application_tests PRIVATE dungeon_village_startup_application
    dungeon_village_hash)
target_include_directories(dungeon_village_startup_application_tests PRIVATE src)
add_test(NAME dungeon_village_prototype.startup_application COMMAND dungeon_village_startup_application_tests)
if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    target_compile_options(dungeon_village_startup_application PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_startup_application_cli PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_startup_application_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
foreach(module IN ITEMS startup_world_projection startup_world_routes startup_world_scene startup_world_arrival
                        startup_world_runtime_tasks startup_world_runtime startup_world_continuous startup_world_pages
                        startup_world_runtime_nonactors startup_world_task_flow startup_world_deadline startup_world_building)
    add_executable(dungeon_village_${module}_tests tests/${module}_test.cpp)
    target_link_libraries(dungeon_village_${module}_tests PRIVATE dungeon_village_startup_world)
    add_test(NAME dungeon_village_prototype.${module} COMMAND dungeon_village_${module}_tests)
    if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
        target_compile_options(dungeon_village_${module}_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endforeach()
target_sources(dungeon_village_startup_world_continuous_tests PRIVATE tests/startup_world_replay_driver.cpp)
# 文件格式／平台失败隔离使用一个集中套件，业务黄金断言仍属于原持续世界套件。
add_executable(dungeon_village_startup_world_persistence_tests tests/startup_world_persistence_test.cpp
    tests/startup_world_codec_checks.cpp tests/startup_world_restore_checks.cpp
    tests/startup_application_replay_checks.cpp tests/startup_application_actions_checks.cpp
    tests/startup_application_natural_replay.cpp tests/startup_application_active_replay.cpp
    tests/startup_application_second_star_replay.cpp)
target_include_directories(dungeon_village_startup_world_persistence_tests PRIVATE src)
target_link_libraries(dungeon_village_startup_world_persistence_tests PRIVATE dungeon_village_startup_application dungeon_village_hash)
target_compile_features(dungeon_village_startup_world_persistence_tests PRIVATE cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    target_compile_options(dungeon_village_startup_world_persistence_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME dungeon_village_prototype.startup_world_persistence
    COMMAND dungeon_village_startup_world_persistence_tests "${CMAKE_CURRENT_BINARY_DIR}/persistence-tests")
add_test(NAME dungeon_village_prototype.startup_world_replay_driver
    COMMAND dungeon_village_startup_world_continuous_tests replay_driver_contract)
add_test(NAME dungeon_village_prototype.startup_world_replay_process
    COMMAND "${STARTUP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay_file_test.mjs"
        --exe "$<TARGET_FILE:dungeon_village_startup_world_continuous_tests>"
        --work-dir "${CMAKE_CURRENT_BINARY_DIR}/replay-process-tests"
        --presentation-exe "$<TARGET_FILE:dungeon_village_startup_world_persistence_tests>"
        --application-exe "$<TARGET_FILE:dungeon_village_startup_world_persistence_tests>"
        --title-exe "$<TARGET_FILE:dungeon_village_startup_application_tests>"
        --natural-application-exe "$<TARGET_FILE:dungeon_village_startup_world_persistence_tests>"
        --active-application-exe "$<TARGET_FILE:dungeon_village_startup_world_persistence_tests>")
set_tests_properties(dungeon_village_prototype.startup_world_replay_process PROPERTIES TIMEOUT 360)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # 重建字段AST检查覆盖；新成员不能只改运行时而静默漏出存档。
    add_test(NAME dungeon_village_prototype.startup_world_codec_coverage
        COMMAND "${STARTUP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_owner_codec.mjs"
            --root "${CMAKE_CURRENT_SOURCE_DIR}/.."
            --ast "${CMAKE_CURRENT_BINARY_DIR}/owner-codec-coverage.json"
            --compiler "${CMAKE_CXX_COMPILER}" --check)
endif()
add_executable(dungeon_village_startup_world_visuals_tests tests/startup_world_visuals_test.cpp
    tests/startup_skin_checks.cpp tests/steam_startup_skin_checks.cpp tests/steam_facility_skin_checks.cpp
    ../tools/src/sprite.cpp ../tools/src/table.cpp)
target_include_directories(dungeon_village_startup_world_visuals_tests PRIVATE ../tools/include)
target_link_libraries(dungeon_village_startup_world_visuals_tests PRIVATE dungeon_village_startup_application
    dungeon_village_hash PkgConfig::RAYLIB)
add_test(NAME dungeon_village_prototype.startup_world_visuals
    COMMAND dungeon_village_startup_world_visuals_tests "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original")
# 唯一Owner的Debug值事务在并行构建时可能被CPU限流；这里只放宽墙钟保护，
# 真实日期目标、150000框架调用上限及所有领域断言不变，不改世界tick或游戏期限。
set_tests_properties(dungeon_village_prototype.startup_world_continuous PROPERTIES TIMEOUT 1800)
set_tests_properties(dungeon_village_prototype.startup_world_task_flow PROPERTIES TIMEOUT 3600)
option(DUNGEON_VILLAGE_LONG_WORLD_TESTS "启用跨年与不同种子倍速的有界世界验收" OFF)
if(DUNGEON_VILLAGE_LONG_WORLD_TESTS)
    add_test(NAME dungeon_village_prototype.natural_tools
        COMMAND dungeon_village_startup_world_continuous_tests natural_tools 1 0)
    add_test(NAME dungeon_village_prototype.natural_progression
        COMMAND dungeon_village_startup_world_continuous_tests natural_progression 1 0)
    add_test(NAME dungeon_village_prototype.natural_expansion
        COMMAND dungeon_village_startup_world_continuous_tests natural_expansion 1 0)
    add_test(NAME dungeon_village_prototype.natural_housing
        COMMAND dungeon_village_startup_world_continuous_tests natural_housing 1 0)
    add_test(NAME dungeon_village_prototype.world_year
        COMMAND dungeon_village_startup_world_continuous_tests 12 1 0)
    add_test(NAME dungeon_village_prototype.world_speed_seed
        COMMAND dungeon_village_startup_world_continuous_tests 6 20261005 1)
    set_tests_properties(dungeon_village_prototype.world_year
                         dungeon_village_prototype.natural_tools
                         dungeon_village_prototype.world_speed_seed
                         dungeon_village_prototype.natural_housing
                         dungeon_village_prototype.natural_progression
                         dungeon_village_prototype.natural_expansion
                         PROPERTIES TIMEOUT 3600 LABELS "long-world")
    # 新扩张模式保留既有第一星/编辑前缀，再自然经营到人气1000和扩张后完整月。
    # 单独给新增长轨迹墙钟预算；原五项保护、断言与调用次数上限保持不变。
    set_tests_properties(dungeon_village_prototype.natural_expansion PROPERTIES TIMEOUT 10800)
endif()
add_test(NAME dungeon_village_prototype.startup_world_data
    COMMAND "${STARTUP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/startup_world_data_test.mjs"
            "${CMAKE_CURRENT_SOURCE_DIR}/../data/startup" "${CMAKE_CURRENT_SOURCE_DIR}/../data/world"
            "${CMAKE_CURRENT_SOURCE_DIR}/../data/original/tenantData.txt")

add_executable(dungeon_village_startup_tests tests/startup_test.cpp)
target_link_libraries(dungeon_village_startup_tests PRIVATE dungeon_village_startup_model)
add_test(NAME dungeon_village_prototype.startup COMMAND dungeon_village_startup_tests)
add_executable(dungeon_village_startup_map_tests tests/startup_map_test.cpp)
target_link_libraries(dungeon_village_startup_map_tests PRIVATE dungeon_village_startup_model)
add_test(NAME dungeon_village_prototype.startup_map COMMAND dungeon_village_startup_map_tests)
add_executable(dungeon_village_startup_use_tests tests/startup_use_test.cpp)
target_link_libraries(dungeon_village_startup_use_tests PRIVATE dungeon_village_startup_model)
add_test(NAME dungeon_village_prototype.startup_use COMMAND dungeon_village_startup_use_tests)
add_executable(dungeon_village_startup_ai_tests tests/startup_ai_test.cpp)
target_link_libraries(dungeon_village_startup_ai_tests PRIVATE dungeon_village_startup_model)
add_test(NAME dungeon_village_prototype.startup_ai COMMAND dungeon_village_startup_ai_tests)
add_executable(dungeon_village_facility_projection_tests tests/facility_projection_test.cpp)
target_link_libraries(dungeon_village_facility_projection_tests PRIVATE dungeon_village_startup_model)
add_test(NAME dungeon_village_prototype.facility_projection COMMAND dungeon_village_facility_projection_tests)
add_executable(dungeon_village_loop_pacing_tests tests/loop_pacing_test.cpp)
target_link_libraries(dungeon_village_loop_pacing_tests PRIVATE dungeon_village_startup_model)
add_test(NAME dungeon_village_prototype.loop_pacing COMMAND dungeon_village_loop_pacing_tests)
add_executable(dungeon_village_road_render_tests tests/road_render_test.cpp ../tools/src/sprite.cpp)
target_include_directories(dungeon_village_road_render_tests PRIVATE ../tools/include)
target_link_libraries(dungeon_village_road_render_tests PRIVATE dungeon_village_startup_model PkgConfig::RAYLIB)
target_compile_features(dungeon_village_road_render_tests PRIVATE cxx_std_17)
set_target_properties(dungeon_village_road_render_tests PROPERTIES CXX_EXTENSIONS OFF)
add_test(NAME dungeon_village_prototype.road_render COMMAND dungeon_village_road_render_tests
         "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original")
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
dungeon_village_library(dungeon_village_prototype_model)
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
target_link_libraries(dungeon_village_prototype PRIVATE dungeon_village_prototype_model dungeon_village_startup_world PkgConfig::RAYLIB)
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
file(GLOB STARTUP_ACTOR_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/human/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/monster/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/common/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/weapon/*")
set_property(TARGET dungeon_village_prototype APPEND PROPERTY LINK_DEPENDS ${STARTUP_IMAGE_FILES} ${STARTUP_ACTOR_FILES}
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
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/human"
            "$<TARGET_FILE_DIR:dungeon_village_prototype>/assets/original/human"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/monster"
            "$<TARGET_FILE_DIR:dungeon_village_prototype>/assets/original/monster"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/common"
            "$<TARGET_FILE_DIR:dungeon_village_prototype>/assets/original/common"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../assets/original/weapon"
            "$<TARGET_FILE_DIR:dungeon_village_prototype>/assets/original/weapon"
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
    target_compile_options(dungeon_village_startup_world_visuals_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_prototype_model PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_village_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_prototype PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_asset_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_startup_model PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_startup_world PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_startup_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_startup_map_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_startup_use_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_startup_ai_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_road_render_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(dungeon_village_dump_startup_map PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

include(../cmake/WindowsExecutables.cmake)
