if(NOT DEFINED WIDE_EYE_EXECUTABLE OR NOT DEFINED CAPTURE_ONE OR NOT DEFINED CAPTURE_TWO)
    message(FATAL_ERROR "Deterministic PNG smoke requires executable and two capture paths")
endif()
if(NOT DEFINED WIDE_EYE_SMOKE_ARGUMENT)
    set(WIDE_EYE_SMOKE_ARGUMENT --voxel-cube-smoke)
endif()
if(NOT DEFINED WIDE_EYE_RESULT_MARKER)
    set(WIDE_EYE_RESULT_MARKER "voxel_cube_result=pass")
endif()

file(REMOVE "${CAPTURE_ONE}" "${CAPTURE_TWO}")

function(run_capture output_path output_variable)
    execute_process(
        COMMAND "${WIDE_EYE_EXECUTABLE}" "${WIDE_EYE_SMOKE_ARGUMENT}" --capture "${output_path}"
        RESULT_VARIABLE capture_result
        OUTPUT_VARIABLE capture_stdout
        ERROR_VARIABLE capture_stderr
    )
    set(capture_output "${capture_stdout}${capture_stderr}")
    if(NOT capture_result EQUAL 0)
        message(FATAL_ERROR "Capture failed (${capture_result}):\n${capture_output}")
    endif()
    if(NOT capture_output MATCHES "capture_result=pass" OR
       NOT capture_output MATCHES "${WIDE_EYE_RESULT_MARKER}")
        message(FATAL_ERROR "Capture omitted a required pass result:\n${capture_output}")
    endif()
    if(NOT EXISTS "${output_path}")
        message(FATAL_ERROR "Capture did not create ${output_path}")
    endif()
    set(${output_variable} "${capture_output}" PARENT_SCOPE)
endfunction()

run_capture("${CAPTURE_ONE}" first_output)
run_capture("${CAPTURE_TWO}" second_output)

foreach(capture_path IN ITEMS "${CAPTURE_ONE}" "${CAPTURE_TWO}")
    file(READ "${capture_path}" signature LIMIT 8 HEX)
    if(NOT signature STREQUAL "89504e470d0a1a0a")
        message(FATAL_ERROR "Capture has invalid PNG signature: ${capture_path}")
    endif()
endforeach()

file(SHA256 "${CAPTURE_ONE}" first_sha256)
file(SHA256 "${CAPTURE_TWO}" second_sha256)
if(NOT first_sha256 STREQUAL second_sha256)
    message(FATAL_ERROR "Repeated capture hashes differ: ${first_sha256} vs ${second_sha256}")
endif()

file(REMOVE "${CAPTURE_ONE}" "${CAPTURE_TWO}")
message("${first_output}")
message("capture_repeat_sha256=${first_sha256}")
