# Runs a test executable under a reduced stack limit and fails loudly if it does
# not finish inside that limit.
#
# QA-002 is why this exists: the gameplay-simulation harness held its fixtures by
# value in one `main`, drifted to within 800 KiB of the platform's default stack,
# and then died with SIGSEGV and no output at all — nothing named the fixture,
# nothing said "stack". A budget that is checked is the difference between that
# and a named failing test.

if(NOT DEFINED WIDE_EYE_TEST_EXECUTABLE)
    message(FATAL_ERROR "WIDE_EYE_TEST_EXECUTABLE is required")
endif()

if(NOT DEFINED WIDE_EYE_STACK_BUDGET_KIB)
    message(FATAL_ERROR "WIDE_EYE_STACK_BUDGET_KIB is required")
endif()

if(NOT DEFINED WIDE_EYE_PASS_EXPRESSION)
    message(FATAL_ERROR "WIDE_EYE_PASS_EXPRESSION is required")
endif()

find_program(WIDE_EYE_SHELL NAMES sh)
if(NOT WIDE_EYE_SHELL)
    message(STATUS "No POSIX shell available; stack-budget check skipped")
    return()
endif()

# `ulimit -s` is the only portable way to bound a process stack from outside the
# process, and it is a shell builtin that not every host honours. Probe it and
# read the value back before trusting any result: a host that cannot set the
# limit must report "skipped" rather than a spurious pass or a spurious failure.
execute_process(
    COMMAND "${WIDE_EYE_SHELL}" -c "ulimit -s ${WIDE_EYE_STACK_BUDGET_KIB} && ulimit -s"
    RESULT_VARIABLE wide_eye_probe_result
    OUTPUT_VARIABLE wide_eye_probe_stdout
    ERROR_VARIABLE wide_eye_probe_stderr
)
string(STRIP "${wide_eye_probe_stdout}" wide_eye_probe_stdout)
if(NOT wide_eye_probe_result EQUAL 0
   OR NOT wide_eye_probe_stdout STREQUAL "${WIDE_EYE_STACK_BUDGET_KIB}"
)
    message(
        STATUS
        "'ulimit -s ${WIDE_EYE_STACK_BUDGET_KIB}' is unavailable on this host "
        "(reported '${wide_eye_probe_stdout}'); stack-budget check skipped"
    )
    return()
endif()

# The executable is passed as `$0` so its path needs no shell quoting, and `exec`
# replaces the shell so the reported exit status is the executable's own.
execute_process(
    COMMAND
        "${WIDE_EYE_SHELL}" -c "ulimit -s ${WIDE_EYE_STACK_BUDGET_KIB}; exec \"$0\""
        "${WIDE_EYE_TEST_EXECUTABLE}"
    RESULT_VARIABLE wide_eye_result
    OUTPUT_VARIABLE wide_eye_stdout
    ERROR_VARIABLE wide_eye_stderr
)

set(wide_eye_output "${wide_eye_stdout}${wide_eye_stderr}")
if(NOT wide_eye_result EQUAL 0)
    message(
        FATAL_ERROR
        "failure_stage=stack_budget\n"
        "${WIDE_EYE_TEST_EXECUTABLE} did not complete within "
        "${WIDE_EYE_STACK_BUDGET_KIB} KiB of stack (result '${wide_eye_result}'). "
        "A segmentation fault with no output means a frame grew past the budget: "
        "check for a fixture that is held by value instead of on the heap.\n"
        "${wide_eye_output}"
    )
endif()

if(NOT wide_eye_output MATCHES "${WIDE_EYE_PASS_EXPRESSION}")
    message(
        FATAL_ERROR
        "failure_stage=stack_budget\n"
        "${WIDE_EYE_TEST_EXECUTABLE} exited 0 under ${WIDE_EYE_STACK_BUDGET_KIB} KiB of "
        "stack without printing '${WIDE_EYE_PASS_EXPRESSION}'.\n"
        "${wide_eye_output}"
    )
endif()

message(
    STATUS
    "Stack budget honoured: ${WIDE_EYE_TEST_EXECUTABLE} passed within "
    "${WIDE_EYE_STACK_BUDGET_KIB} KiB of stack"
)
