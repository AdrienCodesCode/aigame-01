cmake_minimum_required(VERSION 3.28)

foreach(required_variable IN ITEMS CMAKE_CTEST_COMMAND FIXTURE_DIRECTORY
                                   WIDE_EYE_FAILURE_EXPRESSION)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

file(REMOVE_RECURSE "${FIXTURE_DIRECTORY}")
file(MAKE_DIRECTORY "${FIXTURE_DIRECTORY}")

set(emitter "${FIXTURE_DIRECTORY}/emit-pass-and-failure.cmake")
file(
    WRITE
    "${emitter}"
    "if(NOT DEFINED EMITTED_MARKER)\n"
    "    message(FATAL_ERROR \"EMITTED_MARKER is required\")\n"
    "endif()\n"
    "message(\"smoke_result=pass\")\n"
    "message(\"\${EMITTED_MARKER}\")\n"
)

set(fixture_file "${FIXTURE_DIRECTORY}/CTestTestfile.cmake")
file(WRITE "${fixture_file}" "")
set(
    failure_markers
    "failure_stage=intentional_fixture"
    "ERROR: AddressSanitizer: intentional fixture"
    "ERROR: LeakSanitizer: intentional fixture"
    "runtime error: intentional fixture"
)

set(fixture_index 0)
foreach(marker IN LISTS failure_markers)
    math(EXPR fixture_index "${fixture_index} + 1")
    set(fixture_name "ctest_failure_${fixture_index}")
    file(
        APPEND
        "${fixture_file}"
        "add_test([=[${fixture_name}]=] [=[${CMAKE_COMMAND}]=] "
        "[==[-DEMITTED_MARKER=${marker}]==] -P [=[${emitter}]=])\n"
        "set_tests_properties([=[${fixture_name}]=] PROPERTIES "
        "PASS_REGULAR_EXPRESSION [=[smoke_result=pass]=] "
        "FAIL_REGULAR_EXPRESSION [=[${WIDE_EYE_FAILURE_EXPRESSION}]=])\n"
    )
endforeach()

set(budget_emitter "${FIXTURE_DIRECTORY}/emit-budget.cmake")
file(
    WRITE
    "${budget_emitter}"
    "if(NOT DEFINED BUDGET_RESULT)\n"
    "    message(FATAL_ERROR \"BUDGET_RESULT is required\")\n"
    "endif()\n"
    "message(\"within_provisional_low_budget=\${BUDGET_RESULT}\")\n"
)
foreach(budget_result IN ITEMS no yes)
    set(fixture_name "ctest_budget_${budget_result}")
    file(
        APPEND
        "${fixture_file}"
        "add_test([=[${fixture_name}]=] [=[${CMAKE_COMMAND}]=] "
        "[==[-DBUDGET_RESULT=${budget_result}]==] -P [=[${budget_emitter}]=])\n"
        "set_tests_properties([=[${fixture_name}]=] PROPERTIES "
        "PASS_REGULAR_EXPRESSION [=[within_provisional_low_budget=yes]=])\n"
    )
endforeach()

execute_process(
    COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${FIXTURE_DIRECTORY}" --output-on-failure
    RESULT_VARIABLE fixture_result
    OUTPUT_VARIABLE fixture_stdout
    ERROR_VARIABLE fixture_stderr
    TIMEOUT 4
)
set(fixture_output "${fixture_stdout}${fixture_stderr}")

if(fixture_result EQUAL 0)
    message(FATAL_ERROR "CTest failure regexes did not reject the fixtures:\n${fixture_output}")
endif()
foreach(fixture_index RANGE 1 4)
    if(NOT fixture_output MATCHES "ctest_failure_${fixture_index}.*Failed")
        message(
            FATAL_ERROR
            "CTest fixture ${fixture_index} did not fail as required:\n${fixture_output}"
        )
    endif()
endforeach()
if(NOT fixture_output MATCHES "ctest_budget_no.*Failed")
    message(FATAL_ERROR "CTest accepted a negative budget marker:\n${fixture_output}")
endif()
if(NOT fixture_output MATCHES "ctest_budget_yes.*Passed")
    message(FATAL_ERROR "CTest rejected the affirmative budget marker:\n${fixture_output}")
endif()

message("ctest_failure_regex_result=pass")
