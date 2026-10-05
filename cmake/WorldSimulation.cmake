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

option(ARK_LONG_WORLD_TESTS "Run explicit-seed annual world integration checks" OFF)

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
        if(module STREQUAL "startup_world_task_flow_test")
            # Build in every configuration, but run the identical core-only natural trajectory
            # explicitly. The stage matrix covers Debug/Release once, not again per renderer.
            if(ARK_LONG_WORLD_TESTS)
                add_test(NAME "simulation.${module}" COMMAND ${target} 1 1)
                set_tests_properties("simulation.${module}" PROPERTIES
                    TIMEOUT 5400 LABELS "long_world;natural_tasks" RUN_SERIAL TRUE)
            endif()
            continue()
        elseif(module STREQUAL "startup_world_continuous_test")
            # Cross the naturally reached raw49 page just after the former two-month boundary.
            add_test(NAME "simulation.${module}" COMMAND ${target} 3 1 0)
        else()
            add_test(NAME "simulation.${module}" COMMAND ${target})
        endif()
        set_tests_properties("simulation.${module}" PROPERTIES TIMEOUT 120)
        if(module STREQUAL "startup_world_continuous_test")
            # Full candidate copies are deliberately retained in Debug. Keep the same
            # three-month assertions, allowing a bounded run on a busy developer machine.
            set_tests_properties("simulation.${module}" PROPERTIES TIMEOUT 3600)
        endif()
    endforeach()
    if(ARK_LONG_WORLD_TESTS)
        # Keep source assertions intact. The product wrapper also checks the three
        # published natural-task counts; source tests otherwise only print those counts.
        function(ark_long_world_test name months seed speed tasks)
            add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
                "-DWORLD_TEST=$<TARGET_FILE:ark_simulation_startup_world_continuous_test>"
                "-DMONTHS=${months}" "-DSEED=${seed}" "-DSPEED=${speed}"
                "-DEXPECTED_TASKS=${tasks}"
                -P "${ARK_WORLD_ROOT}/tests/world_long_run_test.cmake")
        endfunction()
        ark_long_world_test(simulation.world_annual_seed1 12 1 0 3)
        ark_long_world_test(simulation.world_multiseed_double_speed 6 20261005 1 2)
        ark_long_world_test(simulation.world_two_years_seed0 24 0 1 3)
        add_test(NAME simulation.world_task_flow_second_seed
            COMMAND ark_simulation_startup_world_task_flow_test 20261005 0)
        set_tests_properties(simulation.world_task_flow_second_seed PROPERTIES
            TIMEOUT 5400 LABELS "long_world;natural_tasks" RUN_SERIAL TRUE)
        set_tests_properties(simulation.world_annual_seed1 simulation.world_multiseed_double_speed
            simulation.world_two_years_seed0 PROPERTIES TIMEOUT 5400
            LABELS "long_world;annual_world" RUN_SERIAL TRUE)
    endif()
    add_test(NAME simulation.startup_world_data
        COMMAND "${ARK_WORLD_NODE}" "${ARK_WORLD_ROOT}/tests/simulation/startup_world_data_test.mjs"
            "${ARK_WORLD_DATA}/startup" "${ARK_WORLD_DATA}/world" "${ARK_WORLD_DATA}/tenantData.txt")
    add_test(NAME simulation.source_provenance
        COMMAND "${ARK_WORLD_NODE}" "${ARK_WORLD_ROOT}/scripts/simulation/verify_sources.mjs"
            "${ARK_WORLD_ROOT}")
endif()
