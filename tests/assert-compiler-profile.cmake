cmake_minimum_required(VERSION 3.28)

foreach(required IN ITEMS PRESETS HOST_SYSTEM PROFILE CXX_COMPILER_ID CXX_COMPILER_VERSION)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "Missing required -D${required}=...")
    endif()
endforeach()

file(READ "${PRESETS}" presets_json)

function(read_profile preset_name output_variable)
    string(JSON preset_count LENGTH "${presets_json}" configurePresets)
    math(EXPR last_preset "${preset_count} - 1")
    foreach(index RANGE 0 ${last_preset})
        string(JSON candidate_name GET "${presets_json}" configurePresets ${index} name)
        if(candidate_name STREQUAL preset_name)
            string(
                JSON profile
                GET "${presets_json}" configurePresets ${index} cacheVariables
                    WIDE_EYE_LINUX_COMPILER_PROFILE
            )
            set(${output_variable} "${profile}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    message(FATAL_ERROR "Configure preset '${preset_name}' was not found")
endfunction()

read_profile("base" base_profile)
read_profile("release-gcc" gcc_profile)
if(NOT base_profile STREQUAL "clang-18")
    message(FATAL_ERROR "The canonical base preset is not pinned to clang-18")
endif()
if(NOT gcc_profile STREQUAL "gcc-13")
    message(FATAL_ERROR "The GCC portability preset is not pinned to gcc-13")
endif()

if(HOST_SYSTEM STREQUAL "Linux")
    if(PROFILE STREQUAL "clang-18")
        set(expected_id "Clang")
        set(expected_minimum "18")
        set(expected_maximum "19")
    elseif(PROFILE STREQUAL "gcc-13")
        set(expected_id "GNU")
        set(expected_minimum "13")
        set(expected_maximum "14")
    else()
        message(FATAL_ERROR "Unknown configured Linux compiler profile: ${PROFILE}")
    endif()
    if(NOT CXX_COMPILER_ID STREQUAL expected_id OR
       CXX_COMPILER_VERSION VERSION_LESS expected_minimum OR
       NOT CXX_COMPILER_VERSION VERSION_LESS expected_maximum)
        message(
            FATAL_ERROR
                "Profile ${PROFILE} resolved to ${CXX_COMPILER_ID} ${CXX_COMPILER_VERSION}"
        )
    endif()
elseif(HOST_SYSTEM STREQUAL "Windows" AND NOT CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "Native Windows preset did not resolve to MSVC")
endif()

message(
    STATUS
        "Compiler profile contract passed: host=${HOST_SYSTEM}; profile=${PROFILE}; "
        "CXX=${CXX_COMPILER_ID} ${CXX_COMPILER_VERSION}"
)
