if(NOT DEFINED WIDE_EYE_EXECUTABLE)
    message(FATAL_ERROR "WIDE_EYE_EXECUTABLE is required")
endif()

execute_process(
    COMMAND "${WIDE_EYE_EXECUTABLE}" --context-smoke-inject-high-severity
    RESULT_VARIABLE wide_eye_result
    OUTPUT_VARIABLE wide_eye_stdout
    ERROR_VARIABLE wide_eye_stderr
    TIMEOUT 4
)

set(wide_eye_output "${wide_eye_stdout}${wide_eye_stderr}")

if(NOT "${wide_eye_result}" STREQUAL "1")
    message(
        FATAL_ERROR
        "Expected high-severity smoke exit code 1, got '${wide_eye_result}'.\n${wide_eye_output}"
    )
endif()

foreach(
    wide_eye_expected
    "gl_debug_callback=installed"
    "gl_debug_message severity=high source=application type=marker"
    "gl_debug_high_severity_messages=[1-9][0-9]*"
    "context_result=fail"
    "failure_stage=gl_debug_high_severity"
)
    if(NOT wide_eye_output MATCHES "${wide_eye_expected}")
        message(
            FATAL_ERROR
            "Missing expected output '${wide_eye_expected}'.\n${wide_eye_output}"
        )
    endif()
endforeach()

message(STATUS "High-severity OpenGL debug message correctly failed the context smoke")
