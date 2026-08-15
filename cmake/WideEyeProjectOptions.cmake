option(WIDE_EYE_WARNINGS_AS_ERRORS "Treat warnings in Wide Eye targets as errors" ON)
option(WIDE_EYE_ENABLE_SANITIZERS "Enable supported sanitizers in Wide Eye targets" OFF)

function(wide_eye_set_project_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus)
        if(WIDE_EYE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(
            ${target}
            PRIVATE -Wall
                    -Wextra
                    -Wpedantic
                    -Wconversion
                    -Wsign-conversion
        )
        if(WIDE_EYE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    else()
        message(FATAL_ERROR "Wide Eye does not define strict warnings for ${CMAKE_CXX_COMPILER_ID}")
    endif()

    if(NOT WIDE_EYE_ENABLE_SANITIZERS)
        return()
    endif()

    if(MSVC)
        target_compile_options(${target} PRIVATE /fsanitize=address)
        target_link_options(${target} PRIVATE /fsanitize=address)
        message(STATUS "Wide Eye sanitizer coverage: AddressSanitizer")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(
            ${target}
            PRIVATE -fsanitize=address,undefined
                    -fno-omit-frame-pointer
                    -fno-sanitize-recover=all
        )
        target_link_options(${target} PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=all)
        message(STATUS "Wide Eye sanitizer coverage: AddressSanitizer and UndefinedBehaviorSanitizer")
    else()
        message(FATAL_ERROR "Wide Eye does not define sanitizers for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()
