# Independent, standard C++ world module. No research path is a build/runtime dependency.
get_filename_component(ARK_WORLD_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
include("${CMAKE_CURRENT_LIST_DIR}/WorldSimulationSources.cmake")
find_program(ARK_WORLD_NODE NAMES node REQUIRED)

function(ark_world_target target)
    target_compile_features(${target} PUBLIC cxx_std_17)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

set(ARK_WORLD_DATA "${ARK_WORLD_ROOT}/assets/simulation")
set(ARK_WORLD_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/simulation-generated")
set(ARK_WORLD_STARTUP_CPP "${ARK_WORLD_GENERATED}/startup_data.cpp")
set(ARK_WORLD_CATALOG_CPP "${ARK_WORLD_GENERATED}/world_data.cpp")
set(ARK_WORLD_DATA_INPUTS
    "${ARK_WORLD_DATA}/startup/MAP.json" "${ARK_WORLD_DATA}/startup/STATE.json"
    "${ARK_WORLD_DATA}/startup/TABLES.json" "${ARK_WORLD_DATA}/tenantData.txt"
    "${ARK_WORLD_DATA}/world/monster.txt" "${ARK_WORLD_DATA}/world/questData.txt"
    "${ARK_WORLD_DATA}/world/armour.txt" "${ARK_WORLD_DATA}/world/accessory.txt"
    "${ARK_WORLD_DATA}/world/item.txt" "${ARK_WORLD_DATA}/world/asEventData.txt"
    "${ARK_WORLD_DATA}/scripts/original/events.txt" "${ARK_WORLD_DATA}/scripts/original/talk.txt"
    "${ARK_WORLD_DATA}/scripts/original/news.txt" "${ARK_WORLD_DATA}/scripts/original/evtmsgs.txt"
    "${ARK_WORLD_DATA}/scripts/original/popularBonus.txt")
add_custom_command(OUTPUT "${ARK_WORLD_STARTUP_CPP}" "${ARK_WORLD_CATALOG_CPP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ARK_WORLD_GENERATED}"
    COMMAND "${ARK_WORLD_NODE}" "${ARK_WORLD_ROOT}/scripts/simulation/compile_startup.mjs"
        "${ARK_WORLD_DATA}/startup" "${ARK_WORLD_DATA}/tenantData.txt" "${ARK_WORLD_STARTUP_CPP}"
    COMMAND "${ARK_WORLD_NODE}" "${ARK_WORLD_ROOT}/scripts/simulation/compile_startup_world.mjs"
        "${ARK_WORLD_DATA}/startup" "${ARK_WORLD_DATA}/world"
        "${ARK_WORLD_DATA}/tenantData.txt" "${ARK_WORLD_CATALOG_CPP}"
    DEPENDS "${ARK_WORLD_ROOT}/scripts/simulation/compile_startup.mjs"
        "${ARK_WORLD_ROOT}/scripts/simulation/compile_startup_world.mjs" ${ARK_WORLD_DATA_INPUTS}
    VERBATIM)
add_library(ark_world_rules STATIC ${ARK_WORLD_RULE_SOURCES})
target_include_directories(ark_world_rules PUBLIC "${ARK_WORLD_ROOT}/include")
ark_world_target(ark_world_rules)
add_library(ark_world_runtime STATIC ${ARK_WORLD_RUNTIME_SOURCES}
    "${ARK_WORLD_STARTUP_CPP}" "${ARK_WORLD_CATALOG_CPP}")
target_include_directories(ark_world_runtime PUBLIC "${ARK_WORLD_ROOT}/include")
target_link_libraries(ark_world_runtime PUBLIC ark_world_rules)
ark_world_target(ark_world_runtime)

if(BUILD_TESTING)
    foreach(source IN LISTS ARK_WORLD_TEST_SOURCES)
        get_filename_component(module "${source}" NAME_WE)
        set(target "ark_simulation_${module}")
        add_executable(${target} "${source}")
        target_link_libraries(${target} PRIVATE ark_world_runtime)
        if(module MATCHES "^world_(arrivals|calendar_tasks|exploration|facility_update|gift_page|popularity|residence|runtime|scripts|world_entry)_test$")
            target_compile_definitions(${target} PRIVATE "ARK_WORLD_TEST_DATA=\"${ARK_WORLD_DATA}\"")
        endif()
        ark_world_target(${target})
        add_test(NAME "simulation.${module}" COMMAND ${target})
        set_tests_properties("simulation.${module}" PROPERTIES TIMEOUT 120)
        if(module STREQUAL "startup_world_continuous_test")
            set_tests_properties("simulation.${module}" PROPERTIES TIMEOUT 900)
        endif()
    endforeach()
    add_test(NAME simulation.startup_world_data
        COMMAND "${ARK_WORLD_NODE}" "${ARK_WORLD_ROOT}/tests/simulation/startup_world_data_test.mjs"
            "${ARK_WORLD_DATA}/startup" "${ARK_WORLD_DATA}/world" "${ARK_WORLD_DATA}/tenantData.txt")
    add_test(NAME simulation.source_provenance
        COMMAND "${ARK_WORLD_NODE}" "${ARK_WORLD_ROOT}/scripts/simulation/verify_sources.mjs"
            "${ARK_WORLD_ROOT}")
endif()
