# Product acceptance wrapper around the unchanged maintained continuous-world regression.
# Each named long run pins its requested input and the naturally created task count; exit0
# alone would only prove the maintained date/income invariants, not this product milestone.
foreach(required IN ITEMS WORLD_TEST MONTHS SEED SPEED EXPECTED_TASKS)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "Missing required long-world argument: ${required}")
    endif()
endforeach()
if(NOT EXISTS "${WORLD_TEST}")
    message(FATAL_ERROR "Continuous-world executable does not exist: ${WORLD_TEST}")
endif()
foreach(argument IN ITEMS MONTHS SEED SPEED EXPECTED_TASKS)
    if(NOT "${${argument}}" MATCHES "^(0|[1-9][0-9]*)$")
        message(FATAL_ERROR "Long-world ${argument} must be an unsigned decimal integer")
    endif()
endforeach()
if(MONTHS LESS 1 OR MONTHS GREATER 36 OR SPEED GREATER 1 OR EXPECTED_TASKS LESS 1)
    message(FATAL_ERROR "Invalid long-world month, speed or positive expected-task contract")
endif()

execute_process(
    COMMAND "${WORLD_TEST}" "${MONTHS}" "${SEED}" "${SPEED}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
    TIMEOUT 5300)
# Preserve the maintained page/month trace in CTest output even when a later check rejects it.
message("${output}")
if(NOT "${error}" STREQUAL "")
    message("${error}")
endif()
if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "Continuous-world run failed or timed out: ${result}")
endif()

string(REGEX MATCHALL "continuous summary [^\r\n]*" summaries "${output}")
list(LENGTH summaries summary_count)
if(NOT summary_count EQUAL 1)
    message(FATAL_ERROR "Expected exactly one continuous-world summary, got ${summary_count}")
endif()
list(GET summaries 0 summary)
if(NOT "${summary}" MATCHES
   "^continuous summary months=${MONTHS} seed=${SEED} speed=${SPEED} ")
    message(FATAL_ERROR "Continuous-world summary does not match requested month/seed/speed")
endif()
if(NOT "${summary}" MATCHES " tasks=${EXPECTED_TASKS}( |$)")
    message(FATAL_ERROR "Natural task count does not match expected ${EXPECTED_TASKS}: ${summary}")
endif()

string(REGEX MATCHALL "startup world continuous checks: [0-9]+" check_lines "${output}")
list(LENGTH check_lines check_line_count)
if(NOT check_line_count EQUAL 1)
    message(FATAL_ERROR "Expected exactly one maintained continuous-world check total")
endif()
list(GET check_lines 0 check_line)
string(REGEX REPLACE "^startup world continuous checks: " "" checks "${check_line}")
if(NOT checks GREATER 0)
    message(FATAL_ERROR "Continuous-world regression did not report positive checks")
endif()
message(STATUS "PASS long world: months=${MONTHS} seed=${SEED} speed=${SPEED} tasks=${EXPECTED_TASKS} checks=${checks}")
