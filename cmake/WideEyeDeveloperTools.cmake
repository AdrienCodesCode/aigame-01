include_guard(GLOBAL)

function(wide_eye_add_developer_checks)
    set(multi_value_arguments SOURCES HEADERS)
    cmake_parse_arguments(PARSE_ARGV 0 wide_eye_check "" "" "${multi_value_arguments}")

    if(wide_eye_check_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown developer-check arguments: ${wide_eye_check_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT wide_eye_check_SOURCES)
        message(FATAL_ERROR "wide_eye_add_developer_checks requires at least one project source")
    endif()

    set(wide_eye_local_tool_hint "${PROJECT_SOURCE_DIR}/.tools/phase0/sysroot/usr/bin")
    set(wide_eye_clang_format_result 1)
    set(wide_eye_clang_format_version "")

    find_program(
        wide_eye_clang_format
        NAMES clang-format-18 clang-format
        NO_CACHE
    )
    if(NOT wide_eye_clang_format)
        find_program(
            wide_eye_clang_format
            NAMES clang-format-18 clang-format
            HINTS "${wide_eye_local_tool_hint}"
            NO_DEFAULT_PATH
            NO_CACHE
        )
    endif()
    if(wide_eye_clang_format)
        execute_process(
            COMMAND "${wide_eye_clang_format}" --version
            RESULT_VARIABLE wide_eye_clang_format_result
            OUTPUT_VARIABLE wide_eye_clang_format_version
            ERROR_VARIABLE wide_eye_clang_format_error
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE
        )
    endif()

    if(
        wide_eye_clang_format_result EQUAL 0
        AND wide_eye_clang_format_version MATCHES "version 18\\."
    )
        add_custom_target(
            format-check
            COMMAND
                "${wide_eye_clang_format}" --dry-run --Werror --style=file
                ${wide_eye_check_SOURCES} ${wide_eye_check_HEADERS}
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            COMMENT "Checking Wide Eye C++ formatting with clang-format 18"
            VERBATIM
        )
        message(STATUS "Wide Eye format check: ${wide_eye_clang_format_version}")
    else()
        message(STATUS "Wide Eye format check unavailable: install clang-format 18")
    endif()

    set(wide_eye_clang_tidy_result 1)
    set(wide_eye_clang_tidy_version "")
    find_program(
        wide_eye_clang_tidy
        NAMES clang-tidy-18 clang-tidy
        NO_CACHE
    )
    if(NOT wide_eye_clang_tidy)
        find_program(
            wide_eye_clang_tidy
            NAMES clang-tidy-18 clang-tidy
            HINTS "${wide_eye_local_tool_hint}"
            NO_DEFAULT_PATH
            NO_CACHE
        )
    endif()
    if(wide_eye_clang_tidy)
        execute_process(
            COMMAND "${wide_eye_clang_tidy}" --version
            RESULT_VARIABLE wide_eye_clang_tidy_result
            OUTPUT_VARIABLE wide_eye_clang_tidy_version
            ERROR_VARIABLE wide_eye_clang_tidy_error
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE
        )
    endif()

    if(
        wide_eye_clang_tidy_result EQUAL 0
        AND wide_eye_clang_tidy_version MATCHES "version 18\\."
    )
        add_custom_target(
            clang-tidy-check
            COMMAND
                "${wide_eye_clang_tidy}" --quiet
                "--config-file=${PROJECT_SOURCE_DIR}/.clang-tidy"
                "--warnings-as-errors=*"
                "-p=${CMAKE_BINARY_DIR}"
                ${wide_eye_check_SOURCES}
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            COMMENT "Checking Wide Eye C++ sources with clang-tidy 18"
            VERBATIM
        )
        string(REPLACE "\n" " " wide_eye_clang_tidy_version_line "${wide_eye_clang_tidy_version}")
        message(STATUS "Wide Eye clang-tidy check: ${wide_eye_clang_tidy_version_line}")
    else()
        message(STATUS "Wide Eye clang-tidy check unavailable: install clang-tidy 18")
    endif()
endfunction()
