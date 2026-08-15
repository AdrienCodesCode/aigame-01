if(NOT DEFINED WIDE_EYE_EXECUTABLE)
    message(FATAL_ERROR "WIDE_EYE_EXECUTABLE is required")
endif()

execute_process(
    COMMAND "${WIDE_EYE_EXECUTABLE}" --assertion-smoke
    RESULT_VARIABLE wide_eye_result
    OUTPUT_VARIABLE wide_eye_stdout
    ERROR_VARIABLE wide_eye_stderr
)

set(wide_eye_output "${wide_eye_stdout}${wide_eye_stderr}")
if(wide_eye_result EQUAL 0)
    message(FATAL_ERROR "Assertion smoke unexpectedly succeeded:\n${wide_eye_output}")
endif()

if(NOT wide_eye_output MATCHES "level=fatal event=assertion_failed")
    message(FATAL_ERROR "Assertion smoke did not emit the fatal structured log:\n${wide_eye_output}")
endif()

if(NOT wide_eye_output MATCHES "intentional assertion smoke")
    message(FATAL_ERROR "Assertion smoke did not retain its diagnostic message:\n${wide_eye_output}")
endif()

message(STATUS "Assertion smoke rejected the violated invariant as expected")
