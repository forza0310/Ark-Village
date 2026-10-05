# Run the real standard-C++ entry. Failed arguments must not instantiate or advance the world.
if(NOT DEFINED WORLD_SIMULATION)
    message(FATAL_ERROR "WORLD_SIMULATION executable is required")
endif()

function(reject_arguments)
    execute_process(COMMAND "${WORLD_SIMULATION}" ${ARGN}
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
    if(NOT result EQUAL 2 OR NOT error MATCHES "argument_error:" OR output MATCHES "initial")
        message(FATAL_ERROR "Invalid arguments accepted: ${ARGN}\n${output}\n${error}")
    endif()
endfunction()

reject_arguments(--frames)
reject_arguments(--frames 0)
reject_arguments(--frames 100001)
reject_arguments(--frames -1)
reject_arguments(--frames +1)
reject_arguments(--frames 1.0)
reject_arguments(--frames " 1")
reject_arguments(--frames 2 --frames 3)
reject_arguments(--months 0)
reject_arguments(--months 25)
reject_arguments(--seed 18446744073709551616)
reject_arguments(--seed -1)
reject_arguments(--speed)
reject_arguments(--speed -1)
reject_arguments(--speed 2)
reject_arguments(--speed 1.0)
reject_arguments(--speed 0 --speed 1)
reject_arguments(--auto-confirm --auto-confirm)
reject_arguments(--end-awards --end-awards)
reject_arguments(--unknown)
reject_arguments(--help --frames 1)

execute_process(COMMAND "${WORLD_SIMULATION}" --frames 8 --seed 0
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 20)
if(NOT result EQUAL 0 OR NOT output MATCHES "auto_confirm=0 end_awards=0 speed=0" OR
   NOT output MATCHES "result=frame_budget_complete" OR NOT output MATCHES "confirmations=0")
    message(FATAL_ERROR "Default run must finish without fabricated input\n${output}\n${error}")
endif()

execute_process(COMMAND "${WORLD_SIMULATION}" --frames 8 --months 1 --seed 0
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 20)
if(NOT result EQUAL 3 OR NOT output MATCHES "result=month_goal_unmet" OR
   NOT output MATCHES "confirmations=0")
    message(FATAL_ERROR "Unmet goal must not imply world progress\n${output}\n${error}")
endif()

execute_process(COMMAND "${WORLD_SIMULATION}" --frames 8 --seed 18446744073709551615 --auto-confirm
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 20)
if(NOT result EQUAL 0 OR NOT output MATCHES "seed=18446744073709551615" OR
   NOT output MATCHES "auto_confirm=1 end_awards=0")
    message(FATAL_ERROR "Full Java seed and explicit test input must be accepted\n${output}\n${error}")
endif()

execute_process(COMMAND "${WORLD_SIMULATION}" --frames 8 --speed 1 --auto-confirm --end-awards
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 20)
if(NOT result EQUAL 0 OR NOT output MATCHES "auto_confirm=1 end_awards=1 speed=1")
    message(FATAL_ERROR "Source double speed and explicitly opted-in award input must parse\n${output}\n${error}")
endif()

execute_process(COMMAND "${WORLD_SIMULATION}" --frames 8 --speed 0 --end-awards
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 20)
if(NOT result EQUAL 0 OR NOT output MATCHES "auto_confirm=0 end_awards=1 speed=0" OR
   NOT output MATCHES "confirmations=0")
    message(FATAL_ERROR "Award policy alone must not fabricate ordinary page confirmation\n${output}\n${error}")
endif()
